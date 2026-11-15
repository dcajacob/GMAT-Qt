"""Replay the frozen numeric-nested-unit evidence without starting GMAT."""
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

def expected():
    return {'atmosphere-F107': 'sfu', 'atmosphere-F107A': 'sfu', 'atmosphere-MagneticIndex': '0–9',
            'gravity-Degree': 'Degree', 'gravity-Order': 'Order', 'gravity-StmLimit': 'STM degree/order limit',
            'polyhedron-shape': 'coordinates in km', 'polyhedron-density': 'kg/m³'}

def observations(proof, bad):
    wanted = expected()
    seen = set()
    failures = 0
    for row in proof['observations']:
        key = row['key']
        need(key not in seen and key in wanted, 'Duplicate or unknown nested-unit observation')
        seen.add(key)
        need(row['expected'] == wanted[key], 'Nested-unit expectation changed')
        passed = wanted[key] in row['actual']
        need(row['passed'] == passed, 'Nested-unit result flag differs')
        failures += not passed
    need(seen == set(wanted), 'Nested-unit observation inventory incomplete')
    need(failures == bad == proof['mismatches'], 'Nested-unit failure count differs')
    need(proof['status'] == ('failed' if bad else 'passed'), 'Nested-unit status differs')
    need(proof['source_native_history_file_unchanged']
         and proof['mission_executions'] == 0 and proof['new_requirement_credit'] == 0,
         'Nested-unit inspection changed native, source or parameter state')

def audit(stage):
    stage = Path(stage)
    versions = {}
    for version in ['current-v1']:
        ids = load(stage / ('source-sha256-' + version + '.json'))
        build = load(stage / ('build-' + version + '.json'))
        need(build['status'] == 'passed' and build['inputs_unchanged']
             and build['source_sha256'] == ids
             and build['log_sha256'] == sha(stage / ('build-' + version + '.log')),
             'Nested-unit build identity changed')
        with tarfile.open(stage / ('sources-' + version + '.tar.gz')) as archive:
            need(set(archive.getnames()) == set(ids), 'Nested-unit source archive incomplete')
            for name, digest in ids.items():
                need(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest,
                     'Nested-unit frozen source changed')
        versions[version] = ids, build
    current = versions['current-v1'][0]
    previous = load(stage.parent / 'complete18-20261112-camera-vector-units-v1/source-sha256-after-v1.json')
    need({name for name, digest in previous.items() if current.get(name) != digest}
         == {'src/qtgui/CMakeLists.txt'}, 'Display option test changed production source')
    plan = load(stage / 'frozen-plan-v1.json')
    need(plan['observations_per_profile'] == 8 and plan['test_sha256']
         == current['src/qtgui/tests/NestedPhysicalUnitTests.cpp'], 'Display option oracle changed')
    passes = failures = 0
    for mode, version, bad in [('headless', 'current-v1', 0), ('visible', 'current-v1', 0)]:
        key = 'gui-' + mode + '-' + version + '-all'
        root = stage / key
        receipt = load(stage / (key + '-receipt.json'))
        ids, build = versions[version]
        need(receipt['inputs_unchanged'] and receipt['returncode'] == int(bool(bad))
             and all(receipt['source_sha256'].get(name) == digest for name, digest in ids.items()),
             'Nested-unit runtime source identity differs')
        for name, digest in build['binary_sha256'].items():
            if Path(name).name == 'GmatQt':
                continue
            need(any(value == digest and Path(path).parent == Path(name).parent
                     and (path == name or Path(path).name.startswith(Path(name).name + '.'))
                     for path, value in receipt['image_sha256'].items()),
                 'Nested-unit runtime binary identity differs')
        batch = load(root / 'results.json')
        need([row['name'] for row in batch['results'] if row['status'] != 'not-run']
             == ['QtGui.NativeNestedPhysicalUnits'] and batch['end_reason'] == 'completed-selection',
             'Nested-unit check selection differs')
        folder = root / 'QtGui.NativeNestedPhysicalUnits'
        result, proof = load(folder / 'result.json'), load(folder / 'proof.json')
        need(result['status'] == ('failed' if bad else 'passed')
             and result['returncode'] == (8 if bad else 0) and result['requested_visibility'] == mode,
             'Nested-unit execution disposition differs')
        if mode == 'visible':
            need(result['trace']['native_exposure_events'] > 0, 'Nested-unit was not visibly exposed')
        need([json.loads(line.split('NASA_CHECK ', 1)[1])
              for line in (folder / 'test.log').read_text().splitlines() if 'NASA_CHECK ' in line] == [proof]
             and proof['observations'] == load(folder / 'unit-observations.json'),
             'Nested-unit raw observations differ')
        observations(proof, bad)
        passes += 8 - bad
        failures += bad
    return dict(status='passed', passing_gui_observations=passes,
                preserved_unit_failures=failures, native_missions=0,
                source_native_history_file_unchanged=True,
                new_requirement_credit=0, whole_row_qualified=False)
