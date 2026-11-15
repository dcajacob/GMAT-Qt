"""Replay the frozen numeric-data-display evidence without starting GMAT."""
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
    rows = {}
    names = {'warnLower': 'Warning lower', 'warnUpper': 'Warning upper',
             'critLower': 'Critical lower', 'critUpper': 'Critical upper'}
    for unit in ['km', 'kg']:
        for control, caption in names.items():
            rows['dynamic-' + unit, 'dynamic_' + control] = caption + ' (' + unit + ')'
    for axis in ['X', 'Y']:
        for limit in ['Minimum', 'Maximum']:
            rows['plot-limits', 'plot' + axis + 'Axis' + limit] = limit + ' (' + axis + '-axis data units)'
    for name in ['Line width', 'Marker size']:
        rows['plot-style', name] = name + ' (pixels)'
    return rows

def observations(proof, bad):
    wanted = expected()
    seen = set()
    failures = 0
    for row in proof['observations']:
        key = row['key'], row['control']
        need(key not in seen and key in wanted, 'Duplicate or unknown data-display observation')
        seen.add(key)
        need(row['expected'] == wanted[key], 'Data-display expectation changed')
        passed = row['actual'] == wanted[key]
        need(row['passed'] == passed, 'Data-display result flag differs')
        failures += not passed
    need(seen == set(wanted), 'Data-display observation inventory incomplete')
    need(failures == bad == proof['mismatches'], 'Data-display failure count differs')
    need(proof['status'] == ('failed' if bad else 'passed'), 'Data-display status differs')
    need(proof['dynamic_bounds_and_pending_settings_unchanged']
         and proof['plot_settings_and_data_unchanged'], 'Data-display unit labels altered numerical settings')
    need(proof['source_native_history_file_and_parameter_inventory_unchanged']
         and proof['mission_executions'] == 0 and proof['new_requirement_credit'] == 0,
         'Data-display inspection changed native, source or parameter state')

def audit(stage):
    stage = Path(stage)
    for name, digest in load(stage / 'definition-source-sha256-v1.json').items():
        need(sha(stage / 'definitions' / name) == digest, 'Data-display definition changed')
    native = (stage / 'definitions/src/base/subscriber/DynamicDataDisplay.cpp').read_text()
    need('yParamWrappers[wrapperIdx]->EvaluateReal() <' in native
         and 'displayData[i][j].critLowerBound' in native
         and 'displayData[i][j].warnUpperBound' in native,
         'Dynamic bounds must compare directly with the numeric parameter')
    drawing = (stage / 'definitions/src/qtgui/PlotWidget.cpp').read_text()
    need('QPen pen(color,curve.width,curve.style);' in drawing
         and 'const double r=size/2;' in drawing
         and 'curve.markerSize,point.color,point.highlighted)' in drawing,
         'Curve width and markers must use display coordinates')
    versions = {}
    for version in ['before-v1', 'after-v1']:
        ids = load(stage / ('source-sha256-' + version + '.json'))
        build = load(stage / ('build-' + version + '.json'))
        need(build['status'] == 'passed' and build['inputs_unchanged']
             and build['source_sha256'] == ids
             and build['log_sha256'] == sha(stage / ('build-' + version + '.log')),
             'Data-display build identity changed')
        with tarfile.open(stage / ('sources-' + version + '.tar.gz')) as archive:
            need(set(archive.getnames()) == set(ids), 'Data-display source archive incomplete')
            for name, digest in ids.items():
                need(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest,
                     'Data-display frozen source changed')
        versions[version] = ids, build
    before, after = versions['before-v1'], versions['after-v1']
    production = load(stage / 'implementation-plan-v1.json')['production_files']
    need({name for name, digest in before[0].items() if after[0][name] != digest}
         == set(production) and len(production) == 2, 'Data-display patch exceeds planned files')
    need(all(before[0][name] == digest for name, digest in production.items()),
         'Data-display production preimage differs')
    for name, digest in before[1]['binary_sha256'].items():
        if Path(name).name.startswith('lib'):
            need(after[1]['binary_sha256'][name] == digest, 'Data-display change altered native libraries')
    passes = failures = 0
    for mode, version, bad in [('headless', 'before-v1', 14),
                               ('headless', 'after-v1', 0), ('visible', 'after-v1', 0)]:
        key = 'gui-' + mode + '-' + version + '-all'
        root = stage / key
        receipt = load(stage / (key + '-receipt.json'))
        ids, build = versions[version]
        need(receipt['inputs_unchanged'] and receipt['returncode'] == int(bool(bad))
             and all(receipt['source_sha256'].get(name) == digest for name, digest in ids.items()),
             'Data-display runtime source identity differs')
        for name, digest in build['binary_sha256'].items():
            if Path(name).name == 'GmatQt':
                continue
            need(any(value == digest and Path(path).parent == Path(name).parent
                     and (path == name or Path(path).name.startswith(Path(name).name + '.'))
                     for path, value in receipt['image_sha256'].items()),
                 'Data-display runtime binary identity differs')
        batch = load(root / 'results.json')
        need([row['name'] for row in batch['results'] if row['status'] != 'not-run']
             == ['QtGui.NativeDataDisplayUnits'] and batch['end_reason'] == 'completed-selection',
             'Data-display check selection differs')
        folder = root / 'QtGui.NativeDataDisplayUnits'
        result, proof = load(folder / 'result.json'), load(folder / 'proof.json')
        need(result['status'] == ('failed' if bad else 'passed')
             and result['returncode'] == (8 if bad else 0) and result['requested_visibility'] == mode,
             'Data-display execution disposition differs')
        if mode == 'visible':
            need(result['trace']['native_exposure_events'] > 0, 'Data-display was not visibly exposed')
        need([json.loads(line.split('NASA_CHECK ', 1)[1])
              for line in (folder / 'test.log').read_text().splitlines() if 'NASA_CHECK ' in line] == [proof]
             and proof['observations'] == load(folder / 'unit-observations.json'),
             'Data-display raw observations differ')
        observations(proof, bad)
        passes += 14 - bad
        failures += bad
    return dict(status='passed', passing_gui_observations=passes,
                dynamic_bounds_and_pending_settings_unchanged=True, plot_settings_and_data_unchanged=True,
                preserved_unit_failures=failures, native_missions=0,
                source_native_history_file_and_parameter_inventory_unchanged=True,
                new_requirement_credit=0, whole_row_qualified=False)
