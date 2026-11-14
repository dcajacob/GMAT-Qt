"""Audit the 18 new grouped contexts and two narrow production repairs.

Receipts remain source-versioned. This does not qualify whole FRC!E138 or the
complete current build. Existing passing GUI cases need not be replayed.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
import xml.etree.ElementTree as ET


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


def audit(root):
    root = Path(root).resolve()
    repo = Path(__file__).resolve().parents[3]
    consumed = {root/'event-command-coverage-snapshot.json'}
    require(digest(next(iter(consumed))) == '947d3cec648e123a606c3af5d10f0bff61b32af0bc3a9c75520f442bee475bef',
            'Original coverage snapshot differs')
    archives, records, tables = {}, {}, {}

    def archived(path):
        path = Path(path)
        if path not in archives:
            with tarfile.open(path) as archive:
                archives[path] = {m.name:archive.extractfile(m).read() for m in archive if m.isfile()}
        return archives[path]

    def receipt(stem):
        if stem in records:
            return records[stem]
        run = root/stem
        result = load(run/'results.json')
        require(result.get('finished_utc') and result.get('end_reason') == 'completed-selection', 'Run incomplete')
        before, after = [load(root/(stem+'-inputs-'+phase+'.json')) for phase in ['before', 'after']]
        require(before == after, 'Execution inputs changed')
        capsule = load(root/(stem+'-source-reuse.json'))
        for field in ['receipt', 'reference', 'delta']:
            require(digest(capsule[field]) == capsule[field+'_sha256'], 'Executed-source capsule drift')
            consumed.add(Path(capsule[field]))
        sources = {**archived(capsule['reference']), **archived(capsule['delta'])}
        for name, expected in result['source_hashes'].items():
            if name in before['sources']:
                require(before['sources'][name] == expected, 'Source/pre-run mismatch: '+name)
            if name in capsule['recorded_authority_ELF_or_existing_physical_inputs_not_copied']:
                require(capsule['recorded_authority_ELF_or_existing_physical_inputs_not_copied'][name] == expected,
                        'Recorded authority/input identity differs')
            else:
                require(name in sources and hashlib.sha256(sources[name]).hexdigest() == expected,
                        'Exact executed source missing: '+name)
        executable = str(repo/'build/linux-gui/src/qtgui/GmatQtNasaScriptEventCommandTests')
        require(result['binary_hashes'][executable] == before['binaries'][executable], 'Selected executable differs')
        for name in result['binary_hashes'].keys() & before['binaries'].keys():
            require(result['binary_hashes'][name] == before['binaries'][name], 'Selected binary identity differs')
        consumed.update([run/'results.json', root/(stem+'-inputs-before.json'),
                         root/(stem+'-inputs-after.json'), root/(stem+'-source-reuse.json')])
        records[stem], tables[stem] = result, (before, sources)
        return result

    families = ['SaveMissionTerminal', 'SaveMissionNested', 'SaveMissionFollowedInside',
                'SaveMissionFollowedOutside', 'SaveMissionBranch', 'SaveMissionFunction',
                'NestedEvent', 'Stop', 'ClearPlot', 'PenUp', 'PenDown', 'MarkPoint',
                'Write', 'CommandEcho', 'CallBuiltinGmatFunction', 'CallGmatFunction',
                'CallPythonFunction', 'UpdateDynamicData']
    selected = []
    for mode in ['headless', 'visible']:
        for family in families:
            if mode == 'visible':
                stem = 'event-extra-visible-v1'
            elif family == 'SaveMissionTerminal':
                stem = 'event-extra-headless-v3'
            elif family in ['SaveMissionBranch', 'MarkPoint']:
                stem = 'event-extra-headless-v5'
            elif family == 'CallBuiltinGmatFunction':
                stem = 'event-extra-headless-v6'
            else:
                stem = 'event-extra-headless-v4'
            aggregate = receipt(stem)
            name = 'QtGui.NasaScriptEventExtra'+family
            case = root/stem/name
            result, proof, spec = [load(case/f) for f in ['result.json', 'proof.json', 'case.json']]
            require(result['name'] == name and result['status'] == 'passed' and result['returncode'] == 0 and
                    result['requested_visibility'] == mode, 'Requested context/mode not passed')
            require([x for x in aggregate['results'] if x['name'] == name] == [result], 'Aggregate/case differs')
            require(proof['family'] == spec['family'] == family and proof['outcome'] == 'passed', 'Proof identity differs')
            require(digest(case/'case.json') == proof['fixture_sha256'], 'Fixture hash differs')
            require(aggregate['source_hashes']['src/qtgui/tests/script-event-commands/'+family+'.json'] ==
                    proof['fixture_sha256'], 'Executed fixture differs')
            require(tables[stem][0]['sources']['src/qtgui/tests/'+spec['fixture_origin']] == spec['fixture_origin_sha256'],
                    'Reused circle fixture origin differs')
            log = (case/'test.log').read_text()
            emitted = [json.loads(line.split('NASA_CHECK ', 1)[1]) for line in log.splitlines() if 'NASA_CHECK ' in line]
            require(emitted == [proof], 'Proof differs from actual process output')
            junit = ET.parse(case/'ctest.xml').getroot()
            tests = list(junit.iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == name and tests[0].find('failure') is None and
                    tests[0].find('error') is None, 'Actual CTest JUnit outcome differs')
            text = '\n'.join(x.text or '' for x in tests[0].findall('system-out'))
            if '[This part of the test output was removed since it exceeds the threshold of 1024 bytes.]' in text:
                prefix = text.rsplit('...\n[This part', 1)[0]
                require(len(prefix) >= 512 and prefix in re.sub(r'(?m)^\d+: ?', '', log), 'JUnit/process prefix differs')
            else:
                require([json.loads(line.split('NASA_CHECK ', 1)[1]) for line in text.splitlines() if 'NASA_CHECK ' in line] == [proof],
                        'JUnit/process proof differs')
            native = load(case/'native-commands.json')
            require(native['all'] == proof['native_types'] and native['inside_event'] == proof['contained_types'] and
                    spec['command'] in native['inside_event'], 'Native contained command differs')
            require(load(case/'factory-commands.json') == proof['factory_types'], 'Factory census differs')
            if 'marker_pairs' in spec:
                require(native['inside_event'].count('BeginScript') == native['inside_event'].count('EndScript') == spec['marker_pairs'],
                        'Nested event boundaries missing')
            raw = (case/'results.csv').read_text().strip().splitlines()
            require(len(raw) == 1, 'Report row count differs')
            values, expected = [float(x) for x in raw[0].split(',')], oracle(spec)
            check_values(values, expected)
            require(proof['observed'] == values, 'Raw report/proof differs')
            effects = proof['effects']
            if spec.get('mission_save'):
                require(effects['mission_saved'] == spec['expect_saved'], 'SaveMission effect differs')
                if spec['expect_saved']:
                    exported = (case/'native-mission-export.script').read_text()
                    require(all(x in exported for x in ['Create Spacecraft Sat', 'BeginMissionSequence', 'Propagate', 'SaveMission']),
                            'Saved native mission incomplete')
                if spec.get('plain_control'):
                    paired = load(case/'mission-export-equivalence.json')
                    require(paired['plain'] and paired['plain'] == paired['grouped'], 'Plain/grouped native export differs')
                    require((case/'plain-control-export.script').is_file(), 'Plain main-sequence control absent')
                    plain_values = [float(x) for x in (case/'plain-control-results.csv').read_text().strip().split(',')]
                    check_values(plain_values, expected)
            if family == 'Stop':
                require(proof['native_outcome'] == 'Stopped' and spec['skipped_extra_report'] and values[8] == 0,
                        'Stop outcome/skipped assignment differs')
            else:
                require(proof['native_outcome'] == 'Completed', 'Unexpected stopped mission')
            if family in ['ClearPlot', 'PenUp', 'PenDown', 'MarkPoint']:
                points, control = effects['trace_points'], effects['control_points']
                require(any(p[0] < 10-1e-5 for p in control), 'Unselected plot cleared')
                if family == 'ClearPlot':
                    require(points and all(p[0] >= 10-1e-5 for p in points), 'Pre-clear data retained')
                elif family == 'MarkPoint':
                    require(sum(bool(p[3]) for p in points) == 1 and
                            all(abs(p[0]-10) < 1e-4 for p in points if p[3]) and not any(p[3] for p in control),
                            'MarkPoint epoch/target isolation differs')
                elif family == 'PenUp':
                    require(effects['trace_pen_down'] is False and any(p[0] > 10+1e-5 for p in points) and
                            not any(p[2] for p in points if p[0] > 10+1e-5), 'PenUp connectivity differs')
                else:
                    require(effects['trace_pen_down'] is True and any(not p[2] for p in points if 1e-5 < p[0] <= 10+1e-5) and
                            any(p[2] for p in points if p[0] > 10+1e-5) and
                            all(p[2] for p in points if p[0] > 10+1e-5), 'PenDown connectivity differs')
            if family == 'Write':
                require(effects['written_message'] == spec['expected_message'] and
                        spec['expected_message'] in (case/'messages.txt').read_text(), 'Write output missing')
            if family == 'CommandEcho':
                require([x['echo_commands'] for x in load(case/'phase-observations.json')] == [True, False], 'Echo transition differs')
            if family == 'UpdateDynamicData':
                require([float(x.split(' = ')[1]) for x in effects['display_values']] == [41, 7] and values[8] == 99,
                        'Dynamic captured/live values differ')
            if family == 'CallBuiltinGmatFunction':
                require(len(proof['call_mapping_rejections']) == 6 and
                        proof['call_mapping_rejections'] == load(case/'call-mapping-rejections.json'), 'Unsafe call mappings not rejected')
            if 'support_files' in spec:
                for support in spec['support_files']:
                    actual = (case/('support-'+support['name'])).read_text()
                    require('function [out] = Accept(input)' in actual and 'out = input * input + 3;' in actual,
                            'Executed function support file differs')
            require(not result['trace']['screenshots'], 'Screenshots retained')
            if mode == 'visible':
                desktop = aggregate['visible_display']
                require(aggregate['resizable'] and aggregate['display_scale'] == 'auto' and
                        aggregate['display_route'] == 'nested' and desktop['start_minimized'] and
                        'window state: Iconic' in desktop['initial_host_window_state'] and
                        '_NET_WM_STATE_FULLSCREEN' not in desktop['initial_host_window_state'] and
                        not desktop['resize_errors'] and result['trace']['native_exposure_events'] > 0,
                        'Visible/minimized/resizable/non-fullscreen evidence missing')
            consumed.update(p for p in case.iterdir() if p.is_file() and not p.is_symlink())
            selected.append(dict(family=family, mode=mode, receipt=str(case/'result.json'),
                                 source_version=aggregate['source_hashes']['src/qtgui/tests/NasaScriptEventCommandTests.cpp'],
                                 exposures=result['trace']['native_exposure_events']))
    first = receipt('event-extra-headless-v1')
    require(first['counts']['failed'] == 1, 'Original missing-file failure lost')
    failed_case = root/'event-extra-headless-v1/QtGui.NasaScriptEventExtraSaveMissionTerminal'
    require((failed_case/'plain-control-export.script').is_file() and
            'FAIL SaveMission terminal-marker/function/following-command contract differs' in (failed_case/'test.log').read_text(),
            'Paired core defect not established')
    consumed.update(p for p in failed_case.iterdir() if p.is_file() and not p.is_symlink())
    before = tables['event-extra-headless-v1'][0]['sources']
    after = tables['event-extra-visible-v1'][0]['sources']
    common = {k for k in before.keys() & after.keys() if '/tests/' not in k and Path(k).suffix in ['.cpp', '.hpp']}
    changed = sorted(k for k in common if before[k] != after[k])
    require(changed == ['src/base/command/SaveMission.cpp', 'src/qtgui/MissionModel.cpp'], 'Unexpected production changes')
    for source in changed:
        original = root/'event-extra-core-before'/Path(source).name
        require(digest(original) == before[source], 'Core preimage differs')
        consumed.add(original)
    return dict(status='passed-bounded-additional-command-audit', selected=selected,
                unattended_contexts=18, visible_contexts=18, core_changes=changed,
                other_recorded_production_sources_unchanged=len(common)-2,
                requirements_verified=1002, total_requirements=1158, NASA_credit=0, qualified_keys=[],
                consumed_sha256={str(p):digest(p) for p in sorted(consumed)}, auditor_sha256=digest(__file__),
                scope='Command support/effects and narrow SaveMission/mapping repairs at recorded versions; not whole FRC!E138, all science/backend contracts or complete current-build qualification.',
                runtime_binding='Selected executable plus recorded unchanged pre/post runtime-image tables; executable plan is not loaded-library observation.')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    require(not args.output.exists(), 'Refusing to overwrite an audit')
    result = audit(args.root)
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ['selected', 'consumed_sha256']}, indent=2))
