"""Audit the first nine additional ScriptEvent contexts at their recorded versions.

This deliberately awards no whole FRC!E138 credit. It reuses separate passing
unattended versions rather than replaying them after unrelated observations.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
import xml.etree.ElementTree as ET

FAMILIES = ['Propagate', 'Maneuver', 'BeginFiniteBurn', 'EndFiniteBurn',
            'Save', 'Report', 'Global', 'Toggle', 'NonlinearConstraint']
CANONICAL_SHA = '947d3cec648e123a606c3af5d10f0bff61b32af0bc3a9c75520f442bee475bef'


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    return json.loads(Path(path).read_text())


def require(condition, message):
    if not condition:
        raise ValueError(message)


def oracle(spec):
    radius, mu, time = 7000, 398600.4415, 20
    speed = math.sqrt(mu/radius)
    angle = math.sqrt(mu/radius**3)*time
    dx, dy, dz = spec['velocity_increment']
    return [time, radius*math.cos(angle), radius*math.sin(angle), 0,
            -speed*math.sin(angle)+dx, speed*math.cos(angle)+dy, dz,
            41, *spec['expected_tail']]


def check_values(values, expected):
    require(len(values) == len(expected) == 12, 'Report width differs')
    for i, (value, target) in enumerate(zip(values, expected)):
        require(math.isfinite(value) and abs(value-target) < (1e-4 if i == 0 else 2e-5),
                f'Independent orbit/control column {i} differs')


def check_effects(family, proof, case, expected):
    phases = load(case/'phase-observations.json') if (case/'phase-observations.json').exists() else []
    if family in ['BeginFiniteBurn', 'EndFiniteBurn']:
        require([(x['before_command'], x['burn_firing']) for x in phases] ==
                [('BeginFiniteBurn', False), ('EndFiniteBurn', True), ('Report', False)],
                'Burn never demonstrated both execution transitions')
        require(proof['effects']['burn_active'] is False, 'Burn left firing')
    elif family == 'Global':
        require([x['before_command'] for x in phases] == ['Global', 'Report'],
                'Global execution transition absent')
        for i, phase in enumerate(phases):
            require(phase['Keep_global'] is True and phase['Count_global'] is False and
                    phase['Keep_in_local'] == (i == 0) and phase['Keep_in_global'] == (i == 1) and
                    phase['Count_in_local'] is True and phase['Count_in_global'] is False and
                    phase['same_Keep_pointer'] is True, 'Global store migration/identity differs')
    elif family == 'Save':
        text = (case/'saved-runtime.txt').read_text()
        require('Create Spacecraft Sat' in text, 'Spacecraft export missing')
        for i, axis in enumerate(['X', 'Y', 'Z', 'VX', 'VY', 'VZ']):
            found = re.findall(r'^(?:GMAT )?Sat\.'+axis+r' = ([^;\n]+);', text, re.M)
            require(len(found) == 1, 'Saved axis missing/ambiguous')
            value = float(found[0])
            require(math.isfinite(value) and abs(value-expected[i+1]) < 2e-5,
                    'Save exported stale/wrong propagated value')
    elif family == 'Report':
        require(float((case/'extra-values.txt').read_text().strip()) == 41 and
                proof['effects']['extra_value'] == 41, 'Report selected value differs')
    elif family == 'Toggle':
        require(proof['effects']['trace_active'] is False, 'Toggle effect absent')


def audit(root):
    repo = Path(__file__).resolve().parents[3]
    root = Path(root).resolve()
    require(digest(root/'event-command-coverage-snapshot.json') == CANONICAL_SHA, 'Recorded canonical scope changed')
    archive_cache = {}
    consumed = {root/'event-command-coverage-snapshot.json'}

    def archive_hashes(path):
        path = Path(path)
        if path not in archive_cache:
            with tarfile.open(path) as archive:
                archive_cache[path] = {m.name: hashlib.sha256(archive.extractfile(m).read()).hexdigest()
                                       for m in archive if m.isfile()}
        return archive_cache[path]

    receipts, pre_sources = {}, {}

    def receipt(stem):
        if stem in receipts:
            return receipts[stem]
        run = root/stem
        data = load(run/'results.json')
        require(data.get('finished_utc') and data.get('end_reason') == 'completed-selection',
                'Run not terminal')
        before, after = load(root/(stem+'-inputs-before.json')), load(root/(stem+'-inputs-after.json'))
        require(before == after, 'Inputs changed during execution')
        pre_sources[stem] = before['sources']
        capsule = load(root/(stem+'-source-reuse.json'))
        consumed.update([run/'results.json', root/(stem+'-inputs-before.json'),
                         root/(stem+'-inputs-after.json'), root/(stem+'-source-reuse.json')])
        for field in ['receipt', 'reference', 'delta']:
            require(digest(capsule[field]) == capsule[field+'_sha256'], 'Source capsule artifact changed')
            consumed.add(Path(capsule[field]))
        retained = {**archive_hashes(capsule['reference']), **archive_hashes(capsule['delta'])}
        for key, value in data['source_hashes'].items():
            if key in before['sources']:
                require(before['sources'][key] == value, 'Executed source/pre-run input differs: '+key)
            if key in capsule['recorded_authority_ELF_or_existing_physical_inputs_not_copied']:
                require(capsule['recorded_authority_ELF_or_existing_physical_inputs_not_copied'][key] == value,
                        'Recorded external authority/runtime/input differs')
            else:
                require(retained.get(key) == value, 'Exact executed source unavailable: '+key)
        executable = str(repo/'build/linux-gui/src/qtgui/GmatQtNasaScriptEventCommandTests')
        require(data['binary_hashes'][executable] == before['binaries'][executable],
                'Executed driver binary identity differs')
        # binary_hashes inventories every registered executable, including ones
        # not run. Compare the selected inputs, not unrelated plan entries.
        for key in data['binary_hashes'].keys() & before['binaries'].keys():
            require(data['binary_hashes'][key] == before['binaries'][key], 'Selected binary/pre-run identity differs')
        receipts[stem] = data
        return data

    rows = []
    for mode in ['headless', 'visible']:
        for family in FAMILIES:
            if mode == 'visible':
                stem = 'event-command-visible-v2' if family == 'Global' else 'event-command-visible-v1'
            elif family == 'Global':
                stem = 'event-command-headless-v5'
            elif family in ['BeginFiniteBurn', 'EndFiniteBurn', 'Save']:
                stem = 'event-command-headless-v4'
            elif family == 'Propagate':
                stem = 'event-command-headless-v2'
            else:
                stem = 'event-command-headless-v3'
            data = receipt(stem)
            name = 'QtGui.NasaScriptEventCommand'+family
            case = root/stem/name
            result, proof, spec = [load(case/f) for f in ['result.json', 'proof.json', 'case.json']]
            require(result['name'] == name and result['status'] == 'passed' and
                    result['returncode'] == 0 and result['requested_visibility'] == mode,
                    'Selected context did not pass in requested mode')
            matches = [x for x in data['results'] if x['name'] == name]
            require(len(matches) == 1 and matches[0] == result, 'Per-case/aggregate receipt differs')
            require(proof['outcome'] == 'passed' and proof['family'] == spec['family'] == family,
                    'Command proof identity differs')
            process_records = [line.split('NASA_CHECK ', 1)[1] for line in (case/'test.log').read_text().splitlines()
                               if 'NASA_CHECK ' in line]
            logged = [json.loads(line) for line in process_records]
            require(logged == [proof], 'Saved proof differs from actual CTest process output')
            junit = ET.parse(case/'ctest.xml').getroot()
            tests = list(junit.iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == name and
                    tests[0].find('failure') is None and tests[0].find('error') is None,
                    'Actual CTest JUnit outcome differs')
            junit_output = '\n'.join(x.text or '' for x in tests[0].findall('system-out'))
            emitted = [line.split('NASA_CHECK ', 1)[1] for line in junit_output.splitlines() if 'NASA_CHECK ' in line]
            # CTest caps passing JUnit output at 1024 bytes. The full verbose
            # process log above remains mandatory; JUnit must contain its exact
            # matching prefix plus CTest's explicit truncation notice.
            if '[This part of the test output was removed since it exceeds the threshold of 1024 bytes.]' in junit_output:
                prefix = junit_output.rsplit('...\n[This part', 1)[0]
                process_log = re.sub(r'(?m)^\d+: ?', '', (case/'test.log').read_text())
                require(len(prefix) >= 512 and prefix in process_log,
                        'JUnit truncated process prefix differs from complete CTest log')
            else:
                require([json.loads(line) for line in emitted] == [proof], 'Saved proof differs from JUnit process output')
            require(digest(case/'case.json') == proof['fixture_sha256'], 'Fixture binding differs')
            fixture = f'src/qtgui/tests/mission-command-names/{family}.json'
            require(data['source_hashes'][fixture] == digest(case/'case.json'), 'Executed fixture differs')
            native = load(case/'native-commands.json')
            require(native['all'] == proof['native_types'] and native['inside_event'] == proof['contained_types'] and
                    spec['command'] in native['inside_event'] and native['inside_event'][0] == 'BeginScript' and
                    native['inside_event'][-1] == 'EndScript', 'Command containment witness differs')
            require(load(case/'factory-commands.json') == proof['factory_types'], 'Runtime factory census differs')
            raw = (case/'results.csv').read_text().strip().splitlines()
            require(len(raw) == 1, 'Raw report row count differs')
            values = [float(v) for v in raw[0].split(',')]
            expected = oracle(spec)
            check_values(values, expected)
            require(values == proof['observed'], 'Raw report/proof values differ')
            check_effects(family, proof, case, expected)
            require(not result['trace']['screenshots'], 'Passing screenshots retained')
            if mode == 'visible':
                desktop = data['visible_display']
                require(data['resizable'] and data['display_route'] == 'nested' and
                        data['display_scale'] == 'auto' and result['trace']['native_exposure_events'] > 0,
                        'Requested visible desktop/exposure not proved')
                require(desktop['start_minimized'] and desktop['resizable'] and
                        'window state: Iconic' in desktop['initial_host_window_state'] and
                        '_NET_WM_STATE_FULLSCREEN' not in desktop['initial_host_window_state'] and
                        not desktop['resize_errors'], 'Minimized/resizable/non-fullscreen desktop not proved')
            rows.append(dict(family=family, mode=mode, receipt=str(case/'result.json'),
                             source_version=data['source_hashes']['src/qtgui/tests/NasaScriptEventCommandTests.cpp'],
                             exposures=result['trace']['native_exposure_events']))
            consumed.update(p for p in case.iterdir() if p.is_file() and not p.is_symlink())
    failed = receipt('event-command-headless-v1')
    require(failed['counts']['failed'] == 9, 'Initial crashes lost')
    for x in failed['results']:
        if x['status'] == 'failed':
            require('Test process exit: -11' in Path(x['log']).read_text(), 'Initial failure classification differs')
            consumed.add(Path(x['log']))
    prior = load(root/'planet-conversion-visible-v1-inputs-after.json')
    current = load(root/'event-command-headless-v1-inputs-before.json')
    consumed.add(root/'planet-conversion-visible-v1-inputs-after.json')
    production = {k:v for k,v in prior['sources'].items() if '/tests/' not in k and
                  Path(k).suffix in ['.cpp', '.hpp'] and k in current['sources']}
    require(production and all(current['sources'][k] == v for k,v in production.items()),
            'Production changed since previous increment')
    for stem, data in receipts.items():
        require(data['source_hashes']['src/qtgui/tests/NasaScriptEventTests.cpp'] ==
                'ee975809be45b55f17bc3e304a3dfd57876e7cc5c412340c926aa68a09cfe88a' and
                pre_sources[stem]['src/qtgui/tests/NasaScriptEventCases.json'] ==
                '15a1fd92a4263c02e82a93ecd939e4db3b265acd21df2fb71f423e5b890c8f7d',
                'Retained original archived ScriptEvent driver/fixtures changed')
    return dict(status='passed-bounded-command-evidence-audit', selected=rows,
                unique_contexts=9, unattended_contexts=9, visible_contexts=9,
                initial_crashes_preserved=9, original_requirements_verified=1002,
                total_original_requirements=1158, NASA_credit=0, qualified_keys=[],
                unchanged_recorded_production_sources=len(production),
                auditor_sha256=digest(__file__),
                consumed_sha256={str(p):digest(p) for p in sorted(consumed)},
                runtime_binding='Selected executable plus unchanged before/after project runtime-image tables. Registered-executable inventory is not a loaded-library trace.',
                scope='Nine additional command contexts only. Complete supported-command inventory, aliases/legal contexts, nested events, stop, functions and plugins remain open. Evidence is source-versioned, not full current-build qualification.')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path)
    p.add_argument('--verify-receipt', type=Path)
    p.add_argument('--output', type=Path)
    args = p.parse_args()
    if args.verify_receipt:
        require(not args.root and not args.output, 'Receipt verification takes no root/output')
        sealed = load(args.verify_receipt)
        require(sealed['consumed_sha256'], 'No consumed artifact identities')
        for path, expected in sealed['consumed_sha256'].items():
            require(digest(path) == expected, 'Consumed evidence changed: '+path)
        print('Verified', len(sealed['consumed_sha256']), 'consumed evidence hashes')
        raise SystemExit(0)
    require(args.root, 'Supply --root or --verify-receipt')
    result = audit(args.root)
    text = json.dumps(result, indent=2)+'\n'
    if args.output:
        require(not args.output.exists(), 'Refusing to overwrite a sealed audit')
        args.output.write_text(text)
    print(json.dumps({k:v for k,v in result.items() if k not in ['selected', 'consumed_sha256']}, indent=2))
