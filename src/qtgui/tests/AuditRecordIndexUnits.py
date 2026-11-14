"""Replay the frozen numeric-record-index evidence without starting GMAT."""
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
    return {('AcceptFilter', 'RecordNumbers'): 'record indices',
            ('RejectFilter', 'RecordNumbers'): 'record indices'}

def observations(proof, bad):
    wanted = expected();seen = set();failures = 0
    for row in proof['observations']:
        key = row['case'], row['field']
        need(key in wanted and key not in seen, 'Duplicate or unknown record-index input')
        seen.add(key)
        need(row['expected_unit'] == wanted[key], 'Record-index expectation changed')
        passed = row['actual_unit'] == wanted[key] and wanted[key] in row['visible_unit_or_caption']
        need(row['passed'] == passed, 'Record-index result flag differs')
        failures += not passed
    need(seen == set(wanted), 'Record-index inventory incomplete')
    need(failures == bad == proof['mismatches'] and proof['status'] == ('failed' if bad else 'passed'),
         'Record-index disposition differs')
    need(proof['source_native_history_file_isolation'] and proof['mission_executions'] == 0
         and proof['new_requirement_credit'] == 0, 'Record-index observation changed persistent state')

def audit(stage):
    stage = Path(stage)
    versions = {}
    for version in ['before-v1', 'after-v1']:
        ids = load(stage / ('source-sha256-' + version + '.json'))
        build = load(stage / ('build-' + version + '.json'))
        need(build['status'] == 'passed' and build['inputs_unchanged']
             and build['source_sha256'] == ids
             and build['log_sha256'] == sha(stage / ('build-' + version + '.log')),
             'Record-index build identity changed')
        with tarfile.open(stage / ('sources-' + version + '.tar.gz')) as archive:
            need(set(archive.getnames()) == set(ids), 'Record-index source archive incomplete')
            for name, digest in ids.items():
                need(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest,
                     'Record-index frozen source changed')
        versions[version] = ids, build
    before, after = versions['before-v1'], versions['after-v1']
    production = load(stage / 'implementation-plan-v1.json')['production_files']
    need({name for name, digest in before[0].items() if after[0][name] != digest}
         == set(production) and len(production) == 1, 'Record-index patch exceeds planned files')
    need(all(before[0][name] == digest for name, digest in production.items()),
         'Record-index production preimage differs')
    for name, digest in before[1]['binary_sha256'].items():
        if Path(name).name.startswith('lib'):
            need(after[1]['binary_sha256'][name] == digest, 'Record-index change altered native libraries')
    passes = failures = 0
    for mode, version, bad in [('headless', 'before-v1', 2),
                               ('headless', 'after-v1', 0), ('visible', 'after-v1', 0)]:
        key = 'gui-' + mode + '-' + version + '-all'
        root = stage / key
        receipt = load(stage / (key + '-receipt.json'))
        ids, build = versions[version]
        need(receipt['inputs_unchanged'] and receipt['returncode'] == int(bool(bad))
             and all(receipt['source_sha256'].get(name) == digest for name, digest in ids.items()),
             'Record-index runtime source identity differs')
        for name, digest in build['binary_sha256'].items():
            if Path(name).name == 'GmatQt':
                continue
            need(any(value == digest and Path(path).parent == Path(name).parent
                     and (path == name or Path(path).name.startswith(Path(name).name + '.'))
                     for path, value in receipt['image_sha256'].items()),
                 'Record-index runtime binary identity differs')
        batch = load(root / 'results.json')
        need([row['name'] for row in batch['results'] if row['status'] != 'not-run']
             == ['QtGui.NativeRecordIndexUnits'] and batch['end_reason'] == 'completed-selection',
             'Record-index check selection differs')
        folder = root / 'QtGui.NativeRecordIndexUnits'
        result, proof = load(folder / 'result.json'), load(folder / 'proof.json')
        need(result['status'] == ('failed' if bad else 'passed')
             and result['returncode'] == (8 if bad else 0) and result['requested_visibility'] == mode,
             'Record-index execution disposition differs')
        if mode == 'visible':
            need(result['trace']['native_exposure_events'] > 0, 'Record-index was not visibly exposed')
        need([json.loads(line.split('NASA_CHECK ', 1)[1])
              for line in (folder / 'test.log').read_text().splitlines() if 'NASA_CHECK ' in line] == [proof]
             and proof['observations'] == load(folder / 'unit-observations.json'),
             'Record-index raw observations differ')
        observations(proof, bad)
        passes += 2 - bad
        failures += bad
    return dict(status='passed', passing_gui_observations=passes,
                preserved_unit_failures=failures, native_missions=0,
                source_native_history_file_unchanged=True,
                new_requirement_credit=0, whole_row_qualified=False)
