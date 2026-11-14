"""Replay numerical-text unit evidence without executing a native test."""
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

def validate_observation(row, expected):
    pair = row['case'], row['field']
    need(pair in expected and row['expected_unit'] == expected[pair] and row['phase'] == 'initial',
         'Numeric-text observation expectation differs')
    passed = row['actual_unit'] == expected[pair] and expected[pair] in row['visible_unit_or_caption']
    need(row['passed'] == passed, 'Numeric-text observation result differs')

def audit(stage):
    stage = Path(stage)
    versions = {}
    for version in ['before-v1', 'before-v2', 'before-v3', 'after-v1', 'after-v2']:
        ids = load(stage / ('source-sha256-' + version + '.json'))
        build = load(stage / ('build-' + version + '.json'))
        need(build['status'] == 'passed' and build['inputs_unchanged']
             and build['source_sha256'] == ids
             and build['log_sha256'] == sha(stage / ('build-' + version + '.log')),
             'Numeric-text build identity differs')
        with tarfile.open(stage / ('sources-' + version + '.tar.gz')) as archive:
            need(set(archive.getnames()) == set(ids), 'Numeric-text archive incomplete')
            for name, digest in ids.items():
                need(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest,
                     'Numeric-text source archive differs')
        versions[version] = ids, build
    before, after, corrected = [versions[v] for v in ['before-v3', 'after-v1', 'after-v2']]
    planned = load(stage / 'implementation-plan-v1.json')['candidate_files']
    need({name for name, digest in before[0].items() if after[0][name] != digest} == set(planned),
         'Numeric-text production change exceeds planned files')
    need(all(before[0][name] == digest for name, digest in planned.items()), 'Production preimage differs')
    need({name for name, digest in after[0].items() if corrected[0][name] != digest}
         == {'src/qtgui/CMakeLists.txt', 'src/qtgui/tests/DimensionlessInputUnitTests.cpp'},
         'Inspector correction changed production code')
    for name, digest in before[1]['binary_sha256'].items():
        if Path(name).name.startswith('lib'):
            need(after[1]['binary_sha256'][name] == corrected[1]['binary_sha256'][name] == digest,
                 'Numeric-text change altered a native library')
    with tarfile.open(stage / 'sources-after-v1.tar.gz') as archive:
        cases = json.loads(archive.extractfile('src/qtgui/tests/NumericTextUnitCases.json').read())['cases']
        old_inspector = archive.extractfile('src/qtgui/tests/DimensionlessInputUnitTests.cpp').read()
    with tarfile.open(stage / 'sources-after-v2.tar.gz') as archive:
        new_inspector = archive.extractfile('src/qtgui/tests/DimensionlessInputUnitTests.cpp').read()
    need(old_inspector.replace(b'!qobject_cast<QAbstractButton *>(candidate)&&candidate->objectName()',
         b'!qobject_cast<QAbstractButton *>(candidate)&&!qobject_cast<QLabel *>(candidate)&&candidate->objectName()')
         == new_inspector, 'Inspector change exceeds excluding labels from editable-input lookup')
    expected = {(case['key'], field): unit for case in cases for field, unit in case['units'].items()}
    need(len(cases) == 28 and len(expected) == 44, 'Numeric-text expectation inventory differs')
    need(len({(case['kind'], field) for case in cases for field in case['units']}) == 22,
         'Numeric-text field inventory differs')
    for case in cases:
        for field, unit in case['units'].items():
            wanted = 'dimensionless' if case['kind'] == 'Transponder' else 's' if field == 'StepSize' else (
                'days' if case['format'] == 'TAIModJulian' else 'calendar date') + ' (' + case['format'] + ')'
            need(unit == wanted, 'Numeric-text unit definition differs')

    def run(mode, version, selection, test, complete=True):
        key = 'gui-' + mode + '-' + version + '-' + selection
        root = stage / key
        receipt, batch = load(stage / (key + '-receipt.json')), load(root / 'results.json')
        ids, build = versions[version]
        need(receipt['inputs_unchanged'] and all(receipt['source_sha256'].get(n) == h for n, h in ids.items()),
             'Numeric-text runtime source differs')
        for name, digest in build['binary_sha256'].items():
            if Path(name).name == 'GmatQt':
                continue
            need(any(h == digest and Path(p).parent == Path(name).parent
                     and (p == name or Path(p).name.startswith(Path(name).name + '.'))
                     for p, h in receipt['image_sha256'].items()), 'Numeric-text runtime image differs')
        need([r['name'] for r in batch['results'] if r['status'] != 'not-run'] == [test]
             and batch['end_reason'] == 'completed-selection', 'Numeric-text test selection differs')
        folder = root / test
        result = load(folder / 'result.json')
        need(result['requested_visibility'] == mode, 'Numeric-text visibility differs')
        if mode == 'visible':
            need(result['trace']['native_exposure_events'] > 0, 'Numeric-text widgets not exposed')
        rows = load(folder / 'unit-observations.json')
        if complete:
            proof = load(folder / 'proof.json')
            need(proof['source_native_history_file_isolation'] and proof['observations'] == rows,
                 'Numeric-text source/native isolation differs')
            need([json.loads(line.split('NASA_CHECK ', 1)[1]) for line in (folder / 'test.log').read_text().splitlines()
                  if 'NASA_CHECK ' in line] == [proof], 'Numeric-text raw proof differs')
            need(proof['mismatches'] == sum(not row['passed'] for row in rows), 'Numeric-text mismatch count differs')
        else:
            need(result['status'] == 'failed' and 'FAIL Unit case script did not build' in (folder / 'test.log').read_text(),
                 'Original fixture rejection lost')
        for row in rows:
            validate_observation(row, expected)
        return rows, result

    early, _ = run('headless', 'before-v1', 'all', 'QtGui.NativeNumericTextUnits', False)
    prefix, _ = run('headless', 'before-v2', 'all', 'QtGui.NativeNumericTextUnits', False)
    tail, _ = run('headless', 'before-v3', 'tail', 'QtGui.NativeNumericTextUnitsTail')
    baseline = prefix + tail
    need(len(early) == 12 and len(baseline) == len(expected) and all(not r['passed'] for r in early + baseline),
         'Original missing-unit failures lost')
    need({(r['case'], r['field']) for r in baseline} == set(expected), 'Failing baseline incomplete')
    first, _ = run('headless', 'after-v1', 'all', 'QtGui.NativeNumericTextUnits')
    repair, repaired_result = run('headless', 'after-v2', 'ephemeris', 'QtGui.NativeNumericTextUnitsEphemerisRepair')
    visible, visible_result = run('visible', 'after-v2', 'all', 'QtGui.NativeNumericTextUnits')
    failures = [r for r in first if not r['passed']]
    need(len(failures) == len(repair) == 4 and all(r['actual_unit'] == expected[r['case'], r['field']]
         and r['visible_unit_or_caption'] == '' for r in failures), 'Inspector-failure disposition differs')
    valid = [r for r in first if r['passed']] + repair
    need(len(valid) == len(visible) == 44 and all(r['passed'] for r in valid + visible), 'Numeric-text pass set incomplete')
    need({(r['case'], r['field']) for r in valid} == {(r['case'], r['field']) for r in visible} == set(expected),
         'Numeric-text pass set differs from inventory')
    need(repaired_result['status'] == visible_result['status'] == 'passed', 'Repaired runs did not pass')
    owner = (stage / 'estimator-epoch-definition.cpp').read_text()
    factory = (stage / 'estimator-factory-definition.cpp').read_text()
    need(owner.count('if (value != "FromParticipants")') >= 2
         and 'if (ofType == "BatchEstimatorInv")' in factory and 'return new BatchEstimator(withName);' in factory,
         'Symbolic estimator epoch classification unsupported')
    for name, digest in load(stage / 'definition-source-sha256-v1.json').items():
        need(sha(stage / 'definitions' / name) == digest, 'Unit definition source changed')
    return dict(status='passed', numeric_fields=22, symbolic_epoch_fields_classified=2,
                passing_gui_observations=88, preserved_missing_unit_observations=56,
                distinct_baseline_observations=44, preserved_fixture_rejections=2,
                preserved_inspector_failures=4, reused_headless_passes=40,
                native_missions=0, new_requirement_credit=0, whole_row_qualified=False,
                live_format_transitions_still_required=True)
