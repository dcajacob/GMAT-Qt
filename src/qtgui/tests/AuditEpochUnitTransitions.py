"""Replay the frozen numeric-epoch-transition evidence without starting GMAT."""
from pathlib import Path
import hashlib
import json
import tarfile

def load(path):
    return json.loads(Path(path).read_text())

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def need(value, message):
    if not value:
        raise ValueError(message)

def expected(cases):
    states = [('initial', 'TAIModJulian', 'days (TAIModJulian)'),
              ('calendar', 'UTCGregorian', 'calendar date (UTCGregorian)'),
              ('return', 'TAIModJulian', 'days (TAIModJulian)')]
    result = {}
    for case in cases:
        routes = [('resource-editor', case['fields'])]
        if case['interval_dialog']:
            routes.append(('interval-dialog', ['InitialEpoch', 'FinalEpoch']))
        for route, fields in routes:
            for phase, format_name, unit in states:
                for field in fields:
                    result[case['key'], route, field, phase] = (format_name, unit)
    return result

def observations(proof, wanted):
    seen = set()
    for row in proof['observations']:
        key = row['case'], row['route'], row['field'], row['phase']
        need(key in wanted and key not in seen, 'Unknown or duplicate epoch transition')
        seen.add(key)
        format_name, unit = wanted[key]
        need(row['format'] == format_name and row['expected_unit'] == unit,
             'Epoch transition expectation changed')
        need(row['passed'] and unit in row['actual_caption'], 'Epoch transition caption is incorrect')
    need(seen == set(wanted) and len(seen) == 78, 'Epoch transition inventory incomplete')
    need(proof['status'] == 'passed' and proof['mismatches'] == 0,
         'Epoch transition result is not passing')
    need(proof['source_native_history_file_and_parameter_inventory_unchanged']
         and proof['applied_transactions'] == 0 and proof['mission_executions'] == 0
         and proof['new_requirement_credit'] == 0, 'Epoch display test changed persistent state')

def audit(stage):
    stage = Path(stage)
    versions = {}
    for version in ['current-v1']:
        ids = load(stage / ('source-sha256-' + version + '.json'))
        build = load(stage / ('build-' + version + '.json'))
        need(build['status'] == 'passed' and build['inputs_unchanged']
             and build['source_sha256'] == ids
             and build['log_sha256'] == sha(stage / ('build-' + version + '.log')),
             'Epoch-transition build identity changed')
        with tarfile.open(stage / ('sources-' + version + '.tar.gz')) as archive:
            need(set(archive.getnames()) == set(ids), 'Epoch-transition source archive incomplete')
            for name, digest in ids.items():
                need(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest,
                     'Epoch-transition frozen source changed')
        versions[version] = ids, build
    current = versions['current-v1'][0]
    previous = load(stage.parent / 'complete18-20261112-numeric-text-units-v1/source-sha256-after-v2.json')
    need({name for name, digest in previous.items() if current.get(name) != digest}
         == {'src/qtgui/CMakeLists.txt'}, 'Display option test changed production source')
    plan = load(stage / 'frozen-plan-v1.json')
    with tarfile.open(stage / 'sources-current-v1.tar.gz') as archive:
        cases = json.loads(archive.extractfile('src/qtgui/tests/EpochUnitTransitionCases.json').read())['cases']
    need(len(cases) == plan['cases'] == 13 and plan['observations_per_profile'] == 78,
         'Epoch transition plan changed')
    need(plan['test_sha256'] == current['src/qtgui/tests/EpochUnitTransitionTests.cpp']
         and plan['cases_sha256'] == current['src/qtgui/tests/EpochUnitTransitionCases.json'],
         'Epoch transition oracle changed')
    wanted = expected(cases)
    passes = failures = 0
    for mode, version, bad in [('headless', 'current-v1', 0), ('visible', 'current-v1', 0)]:
        key = 'gui-' + mode + '-' + version + '-all'
        root = stage / key
        receipt = load(stage / (key + '-receipt.json'))
        ids, build = versions[version]
        need(receipt['inputs_unchanged'] and receipt['returncode'] == int(bool(bad))
             and all(receipt['source_sha256'].get(name) == digest for name, digest in ids.items()),
             'Epoch-transition runtime source identity differs')
        for name, digest in build['binary_sha256'].items():
            if Path(name).name == 'GmatQt':
                continue
            need(any(value == digest and Path(path).parent == Path(name).parent
                     and (path == name or Path(path).name.startswith(Path(name).name + '.'))
                     for path, value in receipt['image_sha256'].items()),
                 'Epoch-transition runtime binary identity differs')
        batch = load(root / 'results.json')
        need([row['name'] for row in batch['results'] if row['status'] != 'not-run']
             == ['QtGui.NativeEpochUnitTransitions'] and batch['end_reason'] == 'completed-selection',
             'Epoch-transition check selection differs')
        folder = root / 'QtGui.NativeEpochUnitTransitions'
        result, proof = load(folder / 'result.json'), load(folder / 'proof.json')
        need(result['status'] == ('failed' if bad else 'passed')
             and result['returncode'] == (8 if bad else 0) and result['requested_visibility'] == mode,
             'Epoch-transition execution disposition differs')
        if mode == 'visible':
            need(result['trace']['native_exposure_events'] > 0, 'Epoch-transition was not visibly exposed')
        need([json.loads(line.split('NASA_CHECK ', 1)[1])
              for line in (folder / 'test.log').read_text().splitlines() if 'NASA_CHECK ' in line] == [proof]
             and proof['observations'] == load(folder / 'unit-observations.json'),
             'Epoch-transition raw observations differ')
        observations(proof, wanted)
        passes += 78 - bad
        failures += bad
    return dict(status='passed', passing_gui_observations=passes, resource_types=13,
                distinct_visible_input_routes=26, live_date_format_changes=True,
                preserved_unit_failures=failures, native_missions=0,
                source_native_history_file_and_parameter_inventory_unchanged=True,
                new_requirement_credit=0, whole_row_qualified=False)
