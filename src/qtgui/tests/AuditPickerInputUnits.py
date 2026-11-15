"""Replay the frozen numeric-picker evidence without starting GMAT."""
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
    return {
        'reference': 'Units: km',
        'literal': 'Units: km',
        'changed-context': 'Units: s',
        'different-reference': 'Units: km; field units: s',
        'variable': 'Units: user-defined units; field units: km',
        'array': 'Units: user-defined units; field units: kg',
        'local-control': 'Units: user-defined units',
        'stop': 'Units: days',
        'single-literal': 'Units: days',
        'ratio': 'Units: dimensionless',
        'achieve-caller': 'Units: km',
        'propagation-caller': 'Units: s',
    }

def observations(proof, bad):
    wanted = expected()
    seen = set()
    failures = 0
    for row in proof['observations']:
        key = row['key']
        need(key not in seen and key in wanted, 'Duplicate or unknown picker observation')
        seen.add(key)
        need(row['expected'] == wanted[key], 'Picker expectation changed')
        passed = row['actual'] == wanted[key]
        need(row['passed'] == passed, 'Picker result flag differs')
        failures += not passed
    need(seen == set(wanted), 'Picker observation inventory incomplete')
    need(failures == bad == proof['mismatches'], 'Picker failure count differs')
    need(proof['status'] == ('failed' if bad else 'passed'), 'Picker status differs')
    need(proof['source_native_history_file_and_parameter_inventory_unchanged']
         and proof['mission_executions'] == 0 and proof['new_requirement_credit'] == 0,
         'Picker inspection changed native, source or parameter state')

def audit(stage):
    stage = Path(stage)
    versions = {}
    for version in ['before-v1', 'after-v1']:
        ids = load(stage / ('source-sha256-' + version + '.json'))
        build = load(stage / ('build-' + version + '.json'))
        need(build['status'] == 'passed' and build['inputs_unchanged']
             and build['source_sha256'] == ids
             and build['log_sha256'] == sha(stage / ('build-' + version + '.log')),
             'Picker build identity changed')
        with tarfile.open(stage / ('sources-' + version + '.tar.gz')) as archive:
            need(set(archive.getnames()) == set(ids), 'Picker source archive incomplete')
            for name, digest in ids.items():
                need(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest,
                     'Picker frozen source changed')
        versions[version] = ids, build
    before, after = versions['before-v1'], versions['after-v1']
    production = load(stage / 'implementation-plan-v1.json')['production_files']
    need({name for name, digest in before[0].items() if after[0][name] != digest}
         == set(production) and len(production) == 7, 'Picker patch exceeds planned files')
    need(all(before[0][name] == digest for name, digest in production.items()),
         'Picker production preimage differs')
    for name, digest in before[1]['binary_sha256'].items():
        if Path(name).name.startswith('lib'):
            need(after[1]['binary_sha256'][name] == digest, 'Picker change altered native libraries')
    passes = failures = 0
    for mode, version, bad in [('headless', 'before-v1', 12),
                               ('headless', 'after-v1', 0), ('visible', 'after-v1', 0)]:
        key = 'gui-' + mode + '-' + version + '-all'
        root = stage / key
        receipt = load(stage / (key + '-receipt.json'))
        ids, build = versions[version]
        need(receipt['inputs_unchanged'] and receipt['returncode'] == int(bool(bad))
             and all(receipt['source_sha256'].get(name) == digest for name, digest in ids.items()),
             'Picker runtime source identity differs')
        for name, digest in build['binary_sha256'].items():
            if Path(name).name == 'GmatQt':
                continue
            need(any(value == digest and Path(path).parent == Path(name).parent
                     and (path == name or Path(path).name.startswith(Path(name).name + '.'))
                     for path, value in receipt['image_sha256'].items()),
                 'Picker runtime binary identity differs')
        batch = load(root / 'results.json')
        need([row['name'] for row in batch['results'] if row['status'] != 'not-run']
             == ['QtGui.NativePickerInputUnits'] and batch['end_reason'] == 'completed-selection',
             'Picker check selection differs')
        folder = root / 'QtGui.NativePickerInputUnits'
        result, proof = load(folder / 'result.json'), load(folder / 'proof.json')
        need(result['status'] == ('failed' if bad else 'passed')
             and result['returncode'] == (8 if bad else 0) and result['requested_visibility'] == mode,
             'Picker execution disposition differs')
        if mode == 'visible':
            need(result['trace']['native_exposure_events'] > 0, 'Picker was not visibly exposed')
        need([json.loads(line.split('NASA_CHECK ', 1)[1])
              for line in (folder / 'test.log').read_text().splitlines() if 'NASA_CHECK ' in line] == [proof]
             and proof['observations'] == load(folder / 'unit-observations.json'),
             'Picker raw observations differ')
        observations(proof, bad)
        passes += 12 - bad
        failures += bad
    return dict(status='passed', passing_gui_observations=passes,
                preserved_unit_failures=failures, native_missions=0,
                source_native_history_file_and_parameter_inventory_unchanged=True,
                new_requirement_credit=0, whole_row_qualified=False)
