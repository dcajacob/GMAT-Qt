"""Audit the four grouped backend contexts without whole-row credit.

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
import datetime


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    return json.loads(Path(path).read_text())


def require(condition, message):
    if not condition:
        raise ValueError(message)


def oracle(spec):
    radius, mu, time = 7000, 398600.4415, spec.get('orbit_duration', 20)
    speed = math.sqrt(mu/radius)
    angle = math.sqrt(mu/radius**3)*time
    dx, dy, dz = spec['velocity_increment']
    if spec['family'] == 'FindEvents':
        c = math.sqrt(0.5)
        return [time, radius*math.cos(angle), radius*math.sin(angle)*c, radius*math.sin(angle)*c,
                -speed*math.sin(angle), speed*math.cos(angle)*c, speed*math.cos(angle)*c, 41, *spec['expected_tail']]
    return [time, radius*math.cos(angle), radius*math.sin(angle), 0,
            -speed*math.sin(angle)+dx, speed*math.cos(angle)+dy, dz, 41, *spec['expected_tail']]



def check_values(values, expected):
    require(len(values) == len(expected) == 12, 'Report width differs')
    for i, (value, target) in enumerate(zip(values, expected)):
        require(math.isfinite(value) and abs(value-target) < (1e-4 if i == 0 else 2e-5),
                f'Independent orbit/control column {i} differs')


def check_contact(case, proof, spec):
    text = (case/'contact-events.txt').read_text()
    require(text.count('Observer: ') == 1 and 'Observer: East' in text, 'Contact observer differs')
    pattern = r'(\d{2} [A-Za-z]{3} \d{4} \d{2}:\d{2}:\d{2}\.\d+)\s+(\d{2} [A-Za-z]{3} \d{4} \d{2}:\d{2}:\d{2}\.\d+)\s+([0-9.eE+\-]+)'
    reference = spec['contact_reference']
    expected = reference['events']['East']
    epoch = datetime.datetime(2000, 1, 1, 12)
    actual = []
    for start, end, duration in re.findall(pattern, text):
        seconds = lambda x:(datetime.datetime.strptime(x, '%d %b %Y %H:%M:%S.%f')-epoch).total_seconds()
        actual.append(dict(start=seconds(start), end=seconds(end), duration=float(duration)))
    require(len(actual) == len(expected) == 1, 'Contact interval count differs')
    for observed, target in zip(actual, expected):
        for name in ['start', 'end', 'duration']:
            bound = reference['duration_tolerance' if name == 'duration' else 'time_tolerance']
            require(abs(observed[name]-target[name]) < bound, 'Independent contact endpoint/duration differs')
        require(abs(observed['end']-observed['start']-observed['duration']) < 0.002, 'Printed contact duration inconsistent')
    require(proof['effects']['contact_intervals'] == actual, 'Contact raw/proof differs')
    for name in ['application/data/icrf/ICRF_Table.txt', 'application/data/planetary_coeff/SPICEPlanetaryConstantsKernel.tpc']:
        require(proof['backend_inputs'][name] == spec['physical_inputs'][name], 'Selected runtime physical input differs')
    expected_orbit = oracle(spec)[1:7]
    require(all(abs(x-y) < 1e-10 for x,y in zip(reference['final_state'], expected_orbit)), 'Retained contact reference/circle differs')
    for i in range(4, 7):
        require(abs(proof['observed'][i]-oracle(spec)[i]) < 2e-8, 'Tight contact velocity bound differs')


def navigation_report(text, kind):
    table = False
    records = {}
    residual_column = 'Pre-Update (O-C)' if kind == 'filter.txt' else 'Residual (O-C)'
    pattern = r'^\s*(\d+)\s+(\d{2} [A-Za-z]{3} \d{4} \d{2}:\d{2}:\d{2}\.\d+)\s+GPS_PosVec\s+\S+\s+(.*)$'
    epoch = datetime.datetime(2010, 6, 10)
    for line in text.splitlines():
        if 'RecNum' in line:
            table = all(x in line for x in ['Observed (O)', 'Computed (C)', residual_column])
            continue
        match = re.match(pattern, line) if table else None
        if not match:
            continue
        number = int(match[1])
        require(0 <= number < 11, 'Unexpected navigation record')
        time = datetime.datetime.strptime(match[2], '%d %b %Y %H:%M:%S.%f')
        require(abs((time-epoch).total_seconds()-60*number) <= 0.001, 'Navigation record epoch differs')
        fields = match[3].split()
        require(len(fields) == 3, 'Wrong residual section, edited row or changed columns')
        observed, computed, residual = map(float, fields)
        require(all(math.isfinite(x) for x in [observed, computed, residual]) and abs(residual) < 2e-5,
                'Noiseless GPS residual differs')
        require(abs(observed-computed-residual) <= 2e-6, 'Printed observed/computed/residual inconsistent')
        row = records.setdefault(number, dict(record=number, utc=match[2], observed=[], computed=[]))
        row['observed'].append(observed)
        row['computed'].append(computed)
    require(set(records) == set(range(11)), 'Missing/duplicated GPS records')
    for row in records.values():
        for key in ['observed', 'computed']:
            require(len(row[key]) == 3, 'Missing/duplicated GPS components')
            require(abs(math.sqrt(sum(x*x for x in row[key]))-7000) < 2e-5, 'Independent GPS circle norm differs')
    return [records[i] for i in range(11)]


def check_navigation(case, proof, spec):
    for key in ['configured', 'propagator_owned']:
        require(proof['backend_inputs'][key]['force_count'] == 1 and
                proof['backend_inputs'][key]['type'] == 'PointMassForce', 'Wrong navigation force model/clone')
    require(load(case/'navigation-force-census.json') == proof['backend_inputs'], 'Force census/raw proof differs')
    for filename, kind in [('filter-report.txt', 'filter.txt'), ('smoother-report.txt', 'smoother.txt')]:
        records = navigation_report((case/filename).read_text(), kind)
        require(records == proof['effects']['navigation_records'][kind] and
                proof['effects']['navigation_rows'][kind] == 33, 'Processed navigation records/proof differ')
    data = [x.split() for x in (case/'navigation.gmd').read_text().splitlines() if x.strip() and not x.startswith('%')]
    require(len(data) == 11, 'GPS observation file count differs')
    utc = datetime.datetime(2010, 6, 10, tzinfo=datetime.timezone.utc)
    tai_epoch = 10587.5+utc.timestamp()/86400+34/86400
    for number, row in enumerate(data):
        require(len(row) == 7 and row[1] == 'GPS_PosVec' and row[3] == '800', 'GPS data type/sensor differs')
        require(abs(float(row[0])-(tai_epoch+number*60/86400)) < 2e-11, 'GPS observation epoch differs')
        vector = [float(x) for x in row[-3:]]
        require(all(math.isfinite(x) for x in vector) and abs(math.sqrt(sum(x*x for x in vector))-7000) < 2e-5,
                'Independent GPS observation circle norm differs')
    require((case/'filter-warm.csv').stat().st_size > 0, 'Filter warm-start output absent')


def backend_reference(family):
    if family == 'Set':
        return [[21545, 1.4, 7000, 0, 0, 0, math.sqrt(398600.4415/7000), 0]], [1e-11, 1e-12, 1e-8, 1e-8, 1e-8, 1e-10, 1e-10, 1e-10]
    if family == 'FileThrust':
        return [[10, 95], [20, 95]], [1e-4, 1e-8]
    if family == 'Navigation':
        utc = datetime.datetime(2010, 6, 10, tzinfo=datetime.timezone.utc)
        epoch = 10587.5+utc.timestamp()/86400+(34+0.0343817)/86400+600/86400
        angle = math.sqrt(398600.4415/7000**3)*600
        speed = math.sqrt(398600.4415/7000)
        state = [7000*math.cos(angle), 7000*math.sin(angle), 0, -speed*math.sin(angle), speed*math.cos(angle), 0]
        return [[phase, epoch, *state] for phase in [1, 2, 3]], [0, 2e-10, 2e-5, 2e-5, 2e-5, 2e-8, 2e-8, 2e-8]
    raise ValueError('Unknown backend reference')


def audit(root):
    root = Path(root).resolve()
    repo = Path(__file__).resolve().parents[3]
    consumed = {root/'event-command-coverage-snapshot.json'}
    require(digest(next(iter(consumed))) == '947d3cec648e123a606c3af5d10f0bff61b32af0bc3a9c75520f442bee475bef',
            'Original coverage snapshot differs')
    archives, records, tables = {}, {}, {}
    build_guards = {}

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
        if stem in ['event-backend-headless-v4', 'event-backend-headless-v5', 'event-backend-visible-v1']:
            candidates = list(root.glob('event-backend-verified-build-*.json'))
            matches = []
            for path in candidates:
                guard = load(path)
                if (guard['executable_sha256'] == result['binary_hashes'][executable] and
                    guard['compiled_sources']['src/qtgui/tests/NasaScriptEventCommandTests.cpp'] ==
                        result['source_hashes']['src/qtgui/tests/NasaScriptEventCommandTests.cpp']):
                    original = load(guard['original_guard'])
                    require(digest(guard['original_guard']) == guard['original_guard_sha256'] and
                            original['compiled_sources'] == guard['compiled_sources'] and
                            original['executable_sha256'] == guard['executable_sha256'], 'Build guard derivation changed')
                    log = Path(guard['successful_build_log'])
                    require(digest(log) == guard['successful_build_log_sha256'] and
                            guard['successful_build_exit_code'] == 0, 'Successful build log not bound')
                    text = log.read_text()
                    require(not any(x in text for x in ['FAILED:', 'error:', 'build stopped', 'Configuring incomplete']),
                            'Failed build used as successful provenance')
                    terminal = re.search(r'\[(\d+)/(\d+)\] Linking CXX executable src/qtgui/GmatQtNasaScriptEventCommandTests', text)
                    require(terminal and terminal[1] == terminal[2], 'Target link not terminal successful build step')
                    dependency_archive = Path(guard['compile_dependency_archive'])
                    require(digest(dependency_archive) == guard['compile_dependency_archive_sha256'], 'Compiled dependency archive changed')
                    dependency_hashes = {name:hashlib.sha256(value).hexdigest() for name,value in archived(dependency_archive).items()}
                    require(dependency_hashes == guard['compiled_sources'], 'Recorded compiled dependency bytes missing')
                    for name in guard['compiled_sources'].keys() & before['sources'].keys():
                        require(guard['compiled_sources'][name] == before['sources'][name], 'Compiled dependency/pre-run source differs')
                    matches.append(path)
                    consumed.update([Path(guard['original_guard']), log, dependency_archive])
            require(len(matches) == 1, 'Successful build/source guard missing or ambiguous')
            consumed.add(matches[0])
            build_guards[stem] = str(matches[0])
        consumed.update([run/'results.json', root/(stem+'-inputs-before.json'),
                         root/(stem+'-inputs-after.json'), root/(stem+'-source-reuse.json')])
        records[stem], tables[stem] = result, (before, sources)
        return result

    families = ['Set', 'FileThrust', 'FindEvents', 'Navigation']
    selected = []
    for mode in ['headless', 'visible']:
        for family in families:
            if mode == 'visible':
                stem = 'event-backend-visible-v1'
            elif family in ['Set', 'FileThrust']:
                stem = 'event-backend-headless-v1'
            elif family == 'FindEvents':
                stem = 'event-backend-headless-v4'
            else:
                stem = 'event-backend-headless-v5'
            aggregate = receipt(stem)
            name = 'QtGui.NasaScriptEventBackend'+family
            case = root/stem/name
            result, proof, spec = [load(case/f) for f in ['result.json', 'proof.json', 'case.json']]
            require(result['name'] == name and result['status'] == 'passed' and result['returncode'] == 0 and
                    result['requested_visibility'] == mode, 'Requested context/mode not passed')
            require([x for x in aggregate['results'] if x['name'] == name] == [result], 'Aggregate/case differs')
            require(proof['family'] == spec['family'] == family and proof['outcome'] == 'passed', 'Proof identity differs')
            require(digest(case/'case.json') == proof['fixture_sha256'], 'Fixture hash differs')
            require(aggregate['source_hashes']['src/qtgui/tests/script-event-backends/'+family+'.json'] ==
                    proof['fixture_sha256'], 'Executed fixture differs')
            for source, expected_hash in spec.get('source_inputs', {}).items():
                require(aggregate['source_hashes'][source] == expected_hash, 'Borrowed source input identity differs')
            for source, expected_hash in spec.get('physical_inputs', {}).items():
                require(aggregate['source_hashes'][source] == expected_hash, 'Borrowed physical input identity differs')
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
            for kind in spec.get('required_types', []):
                require(kind in native['inside_event'], 'Backend command omitted')
            if 'expected_extra_rows' in spec:
                rows = [[float(v) for v in line.split(',')] for line in (case/'backend-values.csv').read_text().strip().splitlines()]
                require(rows == effects['backend_values'], 'Backend raw/proof differs')
                targets, bounds = backend_reference(family)
                require(spec['expected_extra_rows'] == targets and spec['extra_tolerances'] == bounds, 'Fixture changed reference/bounds')
                require(len(rows) == len(targets), 'Backend row count differs')
                for observed, target in zip(rows, targets):
                    require(len(observed) == len(target) == len(bounds), 'Backend width differs')
                    require(all(math.isfinite(x) and abs(x-y) <= tolerance for x,y,tolerance in zip(observed,target,bounds)), 'Independent backend reference differs')
            if family == 'FindEvents':
                original = json.loads(tables[stem][1]['src/qtgui/tests/NasaContactResultCases.json'])
                require(spec['contact_reference'] == next(x for x in original['cases'] if x['key'] == 'SingleStation') and
                        spec['physical_inputs'] == original['physical_inputs']['kernels'], 'Retained independent contact reference differs')
                check_contact(case, proof, spec)
            if family == 'Navigation':
                check_navigation(case, proof, spec)
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
    before = tables['event-backend-headless-v1'][0]['sources']
    after = tables['event-backend-visible-v1'][0]['sources']
    common = {k for k in before.keys() & after.keys() if '/tests/' not in k and Path(k).suffix in ['.cpp', '.hpp']}
    require(all(before[k] == after[k] for k in common), 'Production changed during backend batch')
    quarantine = load(root/'event-backend-quarantined-run.json')
    require(quarantine['stem'] == 'event-backend-headless-v3', 'Quarantine record lost')
    require(not any('/event-backend-headless-v3/' in row['receipt'] for row in selected), 'Quarantined binary/source mismatch selected')
    consumed.add(root/'event-backend-quarantined-run.json')
    return dict(status='passed-bounded-backend-command-audit', selected=selected,
                unattended_contexts=4, visible_contexts=4, loaded_command_types=7, core_changes=[],
                other_recorded_production_sources_unchanged=len(common),
                requirements_verified=1002, total_requirements=1158, NASA_credit=0, qualified_keys=[],
                successful_build_guards=build_guards,
                consumed_sha256={str(p):digest(p) for p in sorted(consumed)}, auditor_sha256=digest(__file__),
                scope='Grouped backend support/effects at recorded versions; not whole FRC!E138, all science/backend contracts or complete current-build qualification.',
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
