"""Replay the frozen numeric-camera evidence without starting GMAT."""
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
    result = {}
    captions = {'ViewPointReference': 'Viewpoint reference', 'ViewPointVector': 'Viewpoint offset',
                'ViewDirection': 'View direction / target'}
    for field, caption in captions.items():
        for phase in ['initial-vector', 'object', 'return-vector']:
            for component in [1, 2, 3]:
                result[field, phase, component] = (caption + ('' if phase == 'object' else ' (km)'), phase != 'object')
    return result

def observations(proof, bad):
    wanted = expected();seen = set();failures = 0
    for row in proof['observations']:
        key = row['field'], row['phase'], row['component']
        need(key in wanted and key not in seen, 'Unknown or duplicate camera component')
        seen.add(key);caption, enabled = wanted[key]
        need(row['expected_caption'] == caption, 'Camera expectation changed')
        passed = row['actual_caption'] == caption and row['enabled'] == enabled
        need(row['passed'] == passed, 'Camera observation flag differs');failures += not passed
    need(seen == set(wanted) and failures == bad == proof['mismatches'], 'Camera observation inventory differs')
    need(proof['status'] == ('failed' if bad else 'passed')
         and proof['source_native_history_file_and_parameter_inventory_unchanged']
         and proof['vector_values_and_pending_settings_unchanged']
         and proof['mission_executions'] == 0 and proof['new_requirement_credit'] == 0,
         'Camera unit display changed persistent values')

def audit(stage):
    stage = Path(stage)
    import xml.etree.ElementTree as ET
    root = ET.parse(stage / 'orbit-view-unit-definition.xml').getroot()
    found = set()
    for row in root.iter():
        if row.tag.rsplit('}', 1)[-1] != 'row' or not list(row): continue
        name = ''.join(list(row)[0].itertext()).strip()
        if name in ['ViewPointReference', 'ViewPointVector', 'ViewDirection']:
            need('km for a literal three-component vector; no physical quantity for an object-name selection' in ' '.join(''.join(row.itertext()).split()), 'Camera guide unit definition differs')
            found.add(name)
    need(len(found) == 3, 'Camera guide definitions incomplete')
    versions = {}
    for version in ['before-v1', 'after-v1']:
        ids = load(stage / ('source-sha256-' + version + '.json'))
        build = load(stage / ('build-' + version + '.json'))
        need(build['status'] == 'passed' and build['inputs_unchanged']
             and build['source_sha256'] == ids
             and build['log_sha256'] == sha(stage / ('build-' + version + '.log')),
             'Camera build identity changed')
        with tarfile.open(stage / ('sources-' + version + '.tar.gz')) as archive:
            need(set(archive.getnames()) == set(ids), 'Camera source archive incomplete')
            for name, digest in ids.items():
                need(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest,
                     'Camera frozen source changed')
        versions[version] = ids, build
    before, after = versions['before-v1'], versions['after-v1']
    production = load(stage / 'implementation-plan-v1.json')['production_files']
    need({name for name, digest in before[0].items() if after[0][name] != digest}
         == set(production) and len(production) == 1, 'Camera patch exceeds planned files')
    need(all(before[0][name] == digest for name, digest in production.items()),
         'Camera production preimage differs')
    for name, digest in before[1]['binary_sha256'].items():
        if Path(name).name.startswith('lib'):
            need(after[1]['binary_sha256'][name] == digest, 'Camera change altered native libraries')
    passes = failures = 0
    for mode, version, bad in [('headless', 'before-v1', 18),
                               ('headless', 'after-v1', 0), ('visible', 'after-v1', 0)]:
        key = 'gui-' + mode + '-' + version + '-all'
        root = stage / key
        receipt = load(stage / (key + '-receipt.json'))
        ids, build = versions[version]
        need(receipt['inputs_unchanged'] and receipt['returncode'] == int(bool(bad))
             and all(receipt['source_sha256'].get(name) == digest for name, digest in ids.items()),
             'Camera runtime source identity differs')
        for name, digest in build['binary_sha256'].items():
            if Path(name).name == 'GmatQt':
                continue
            need(any(value == digest and Path(path).parent == Path(name).parent
                     and (path == name or Path(path).name.startswith(Path(name).name + '.'))
                     for path, value in receipt['image_sha256'].items()),
                 'Camera runtime binary identity differs')
        batch = load(root / 'results.json')
        need([row['name'] for row in batch['results'] if row['status'] != 'not-run']
             == ['QtGui.NativeOrbitCameraUnits'] and batch['end_reason'] == 'completed-selection',
             'Camera check selection differs')
        folder = root / 'QtGui.NativeOrbitCameraUnits'
        result, proof = load(folder / 'result.json'), load(folder / 'proof.json')
        need(result['status'] == ('failed' if bad else 'passed')
             and result['returncode'] == (8 if bad else 0) and result['requested_visibility'] == mode,
             'Camera execution disposition differs')
        if mode == 'visible':
            need(result['trace']['native_exposure_events'] > 0, 'Camera was not visibly exposed')
        need([json.loads(line.split('NASA_CHECK ', 1)[1])
              for line in (folder / 'test.log').read_text().splitlines() if 'NASA_CHECK ' in line] == [proof]
             and proof['observations'] == load(folder / 'unit-observations.json'),
             'Camera raw observations differ')
        observations(proof, bad)
        passes += 27 - bad
        failures += bad
    return dict(status='passed', passing_gui_observations=passes,
                preserved_unit_failures=failures, native_missions=0,
                source_native_history_file_and_parameter_inventory_unchanged=True,
                new_requirement_credit=0, whole_row_qualified=False)
