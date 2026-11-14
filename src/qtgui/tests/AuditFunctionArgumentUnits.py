"""Replay the frozen numeric-function-argument evidence without starting GMAT."""
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
    notice = 'Units for numeric arguments are defined by the called function; values are passed without unit conversion.'
    caption = 'Inputs (numeric units defined by function)'
    return {'direct-picker': notice, 'gmat-input': caption, 'gmat-picker': notice,
            'python-input': caption, 'python-picker': notice}

def observations(proof, bad):
    wanted = expected()
    seen = set()
    failures = 0
    for row in proof['observations']:
        key = row['key']
        need(key not in seen and key in wanted, 'Duplicate or unknown function-argument observation')
        seen.add(key)
        need(row['expected'] == wanted[key], 'Function-argument expectation changed')
        passed = row['actual'] == wanted[key]
        need(row['passed'] == passed, 'Function-argument result flag differs')
        failures += not passed
    need(seen == set(wanted), 'Function-argument observation inventory incomplete')
    need(failures == bad == proof['mismatches'], 'Function-argument failure count differs')
    need(proof['status'] == ('failed' if bad else 'passed'), 'Function-argument status differs')
    need(proof['function_executions'] == 0 and proof['arguments_and_commands_unchanged'], 'Function argument semantics changed')
    need(proof['source_native_history_file_and_parameter_inventory_unchanged']
         and proof['mission_executions'] == 0 and proof['new_requirement_credit'] == 0,
         'Function-argument inspection changed native, source or parameter state')

def audit(stage):
    stage = Path(stage)
    versions = {}
    for version in ['before-v3', 'after-v1']:
        ids = load(stage / ('source-sha256-' + version + '.json'))
        build = load(stage / ('build-' + version + '.json'))
        need(build['status'] == 'passed' and build['inputs_unchanged']
             and build['source_sha256'] == ids
             and build['log_sha256'] == sha(stage / ('build-' + version + '.log')),
             'Function-argument build identity changed')
        with tarfile.open(stage / ('sources-' + version + '.tar.gz')) as archive:
            need(set(archive.getnames()) == set(ids), 'Function-argument source archive incomplete')
            for name, digest in ids.items():
                need(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest,
                     'Function-argument frozen source changed')
        versions[version] = ids, build
    before, after = versions['before-v3'], versions['after-v1']
    production = load(stage / 'implementation-plan-v1.json')['production_files']
    need({name for name, digest in before[0].items() if after[0][name] != digest}
         == set(production) and len(production) == 2, 'Function-argument patch exceeds planned files')
    need(all(before[0][name] == digest for name, digest in production.items()),
         'Function-argument production preimage differs')
    for name, digest in before[1]['binary_sha256'].items():
        if Path(name).name.startswith('lib'):
            need(after[1]['binary_sha256'][name] == digest, 'Function-argument change altered native libraries')
    passes = failures = 0
    for mode, version, bad in [('headless', 'before-v3', 5),
                               ('headless', 'after-v1', 0), ('visible', 'after-v1', 0)]:
        key = 'gui-' + mode + '-' + version + '-all'
        root = stage / key
        receipt = load(stage / (key + '-receipt.json'))
        ids, build = versions[version]
        need(receipt['inputs_unchanged'] and receipt['returncode'] == int(bool(bad))
             and all(receipt['source_sha256'].get(name) == digest for name, digest in ids.items()),
             'Function-argument runtime source identity differs')
        for name, digest in build['binary_sha256'].items():
            if Path(name).name == 'GmatQt':
                continue
            need(any(value == digest and Path(path).parent == Path(name).parent
                     and (path == name or Path(path).name.startswith(Path(name).name + '.'))
                     for path, value in receipt['image_sha256'].items()),
                 'Function-argument runtime binary identity differs')
        batch = load(root / 'results.json')
        need([row['name'] for row in batch['results'] if row['status'] != 'not-run']
             == ['QtGui.NativeFunctionArgumentUnits'] and batch['end_reason'] == 'completed-selection',
             'Function-argument check selection differs')
        folder = root / 'QtGui.NativeFunctionArgumentUnits'
        result, proof = load(folder / 'result.json'), load(folder / 'proof.json')
        need(result['status'] == ('failed' if bad else 'passed')
             and result['returncode'] == (8 if bad else 0) and result['requested_visibility'] == mode,
             'Function-argument execution disposition differs')
        if mode == 'visible':
            need(result['trace']['native_exposure_events'] > 0, 'Function-argument was not visibly exposed')
        need([json.loads(line.split('NASA_CHECK ', 1)[1])
              for line in (folder / 'test.log').read_text().splitlines() if 'NASA_CHECK ' in line] == [proof]
             and proof['observations'] == load(folder / 'unit-observations.json'),
             'Function-argument raw observations differ')
        observations(proof, bad)
        passes += 5 - bad
        failures += bad
    need(load(stage / 'build-before-v1.json')['status'] == 'failed', 'Original test build failure lost')
    pilot = stage / 'gui-headless-before-v2-all/QtGui.NativeFunctionArgumentUnits'
    need(load(pilot / 'result.json')['status'] == 'failed' and 'FAIL Function argument input missing' in (pilot / 'test.log').read_text(), 'Original function fixture failure lost')
    return dict(status='passed', passing_gui_observations=passes,
                preserved_unit_failures=failures, preserved_fixture_failures=1, preserved_test_build_failures=1, native_missions=0,
                source_native_history_file_and_parameter_inventory_unchanged=True,
                new_requirement_credit=0, whole_row_qualified=False)
