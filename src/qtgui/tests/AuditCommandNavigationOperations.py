"""Audit navigation tree operations, retaining the known nested-estimator failure."""
import argparse
import copy
import datetime
import hashlib
import json
import math
import re
import tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require, sha, load

FAMILIES = ['RunSimulator', 'RunEstimator', 'RunSmoother']
NAMES = ['QtGui.NasaCommandNavigationOperations' + f + p
         for f in FAMILIES for p in ['Root', 'Branch']]
FAILED = 'QtGui.NasaCommandNavigationOperationsRunEstimatorBranch'
OPERATIONS = ['open-context-reuse', 'close-tree', 'open-double-click',
              'insert-before', 'insert-after', 'delete-cancel-confirm',
              'summary-context', 'unicode-reopen']


def circle(seconds):
    speed = math.sqrt(398600.4415 / 7000)
    angle = speed / 7000 * seconds
    return [7000 * math.cos(angle), 7000 * math.sin(angle), 0,
            -speed * math.sin(angle), speed * math.cos(angle), 0]


def compare(values, expected, tolerances):
    require(len(values) == len(expected) == len(tolerances), 'Wrong numeric width')
    require(all(math.isfinite(a) and abs(a-b) <= t
                for a, b, t in zip(values, expected, tolerances)), 'Independent values differ')


def check(proof, family, position):
    require(proof['outcome'] == 'passed' and proof['family'] == family
            and proof['position'] == position, 'Wrong operation context')
    require(proof['operations'] == OPERATIONS, 'Missing tree operation')
    require([r['phase'] for r in proof['results']] == [0, 1], 'Missing Unicode reopen')
    for row in proof['results']:
        truth = [20] + circle(20) + [41, 3, 1, 0, 0]
        compare(row['raw'], truth, [1e-4] + [2e-5]*3 + [2e-8]*3 + [1e-12]*5)
        compare(row['expected'], truth, [1e-10]*12)


def summary_values(text, family):
    headers = list(re.finditer(r'Spacecraft\s*:\s*(\w+)', text))
    require(sorted(m[1] for m in headers) == ['EstSat', 'Sat', 'SimSat'], 'Summary spacecraft census differs')
    seconds = {'Sat': 20, 'SimSat': 600, 'EstSat': 0 if family == 'RunSimulator' else 600}
    for i, header in enumerate(headers):
        section = text[header.end():headers[i+1].start() if i+1 < len(headers) else len(text)]
        require('Coordinate System: EarthMJ2000Eq' in section, 'Summary frame differs')
        actual = []
        for label in ['X', 'Y', 'Z', 'VX', 'VY', 'VZ']:
            values = re.findall(r'(?:^|\s)' + label + r'\s*=\s*([-+0-9.eE]+)', section)
            require(len(values) == 1, 'Missing/duplicate summary component')
            actual.append(float(values[0]))
        compare(actual, circle(seconds[header[1]]), [2e-5]*6)


def gps_report(text, smoother):
    active = False
    records = {}
    epoch = datetime.datetime(2010, 6, 10)
    for line in text.splitlines():
        if 'RecNum' in line:
            active = ('Observed (O)' in line and 'Computed (C)' in line and
                      ('Residual (O-C)' if smoother else 'Pre-Update (O-C)') in line)
            continue
        if not active:
            continue
        m = re.match(r'^\s*(\d+)\s+(\d{2} [A-Za-z]{3} \d{4} \d{2}:\d{2}:\d{2}\.\d+)\s+GPS_PosVec\s+\S+\s+(.*)$', line)
        if not m:
            continue
        index = int(m[1])
        require(0 <= index < 11 and abs((datetime.datetime.strptime(m[2], '%d %b %Y %H:%M:%S.%f') - epoch).total_seconds()-index*60) <= .001, 'Wrong GPS record epoch')
        row = [float(v) for v in m[3].split()]
        require(len(row) == 3 and all(math.isfinite(v) for v in row) and abs(row[2]) < 2e-5, 'Wrong GPS residual row')
        records.setdefault(index, []).append(row)
    require(set(records) == set(range(11)) and all(len(v) == 3 for v in records.values()), 'Missing/duplicate GPS record')
    for values in records.values():
        for column in [0, 1]:
            require(abs(math.sqrt(sum(v[column]**2 for v in values))-7000) < 2e-5, 'GPS radius differs from independent circle')
    return records


def audit(repo, stage):
    manifest = load(stage/'evidence-inputs-v2.json')['artifact_sha256']
    for name, digest in manifest.items():
        require(not Path(name).is_symlink() and sha(name) == digest, 'Changed regular artifact '+name)
    def bound(path):
        path = Path(path)
        require(str(path) in manifest, 'Unbound artifact '+str(path))
        return path
    before = load(bound(stage/'visible-inputs-before.json'))
    require(before == load(bound(stage/'visible-inputs-after.json')), 'Changed native inputs')
    launch = load(bound(stage/'visible-launch.json'))
    require(launch['returncode'] == 1 and launch['inputs_unchanged'] and launch['mapped_processes'] == 6, 'Lost failing native launch')
    with tarfile.open(bound(stage/'executed-sources-v2.tar.gz')) as tar:
        frozen = {m.name: tar.extractfile(m).read() for m in tar.getmembers()}
    require({p: hashlib.sha256(v).hexdigest() for p, v in frozen.items()} == before['sources'], 'Executed source archive differs')
    maps = load(bound(stage/'loaded-images.json'))['processes']
    selected = []
    failures = []
    last = None
    for mode in ['headless', 'visible']:
        receipt = load(bound(repo/'build/gui-acceptance'/('command-navigation-'+mode+'-v1')/'results.json'))
        require(receipt['mode'] == mode and receipt.get('finished_utc') and receipt['end_reason'] == 'completed-selection', 'Nonterminal run')
        require(all(before['sources'].get(p) == h for p, h in receipt['source_hashes'].items()), 'Wrong source version')
        active = {r['name']: r for r in receipt['results'] if r['status'] != 'not-run'}
        require(set(active) == set(NAMES), 'Wrong executed census')
        if mode == 'visible':
            display = receipt['visible_display']
            require(receipt['max_screenshots'] == 0 and display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'], 'Wrong window policy')
        for name, row in active.items():
            log = bound(row['log'])
            root = log.parent
            junit = ET.parse(bound(root/'ctest.xml')).getroot().findall('testcase')
            require(len(junit) == 1 and junit[0].attrib['name'] == name and junit[0].find('skipped') is None, 'Wrong JUnit case')
            failed = name == FAILED
            require(row['status'] == ('failed' if failed else 'passed') and (row['returncode'] != 0) == failed and (junit[0].find('failure') is not None) == failed, 'Failure/pass classification changed')
            exe = [p for p in row['registered_command'] if p in receipt['binary_hashes']]
            require(len(exe) == 1 and before['images'].get(exe[0]) == receipt['binary_hashes'][exe[0]], 'Wrong executed image')
            if mode == 'visible':
                require(row['trace']['native_exposure_events'] > 0 and not row['trace']['screenshots'], 'Case was not exposed')
                traces = list(root.glob('trace-*.jsonl'))
                require(traces, 'Missing process trace')
                for trace in traces:
                    process = maps.get(trace.stem.removeprefix('trace-'))
                    require(process and process['executable'] == exe[0], 'Missing process mapping')
                    for p, h in process['images'].items():
                        require(before['images'].get(p) == h, 'Mapped image differs')
                    for p, h in before['images'].items():
                        if p == exe[0] or Path(p).name.startswith('libGmat'):
                            require(process['images'].get(p) == h, 'Missing core/test mapping')
            if failed:
                require('FAIL Operation restored mission failed' in log.read_text() and not (root/'proof.json').exists(), 'Failure was replaced by pass proof')
                engine = bound(root/'runtime/output/GmatLog.txt').read_text()
                require('Estimation Completed' in engine and 'There is no forward filter information because no EKF estimation has been run before running RunSmoother' in engine and 'Mission run failed' in engine, 'Lost precise backend failure')
                failures.append(dict(mode=mode, name=name, log=str(log)))
                continue
            family = next(f for f in FAMILIES if f in name)
            position = 'Branch' if name.endswith('Branch') else 'Root'
            proof = load(bound(root/'proof.json'))
            messages = re.findall(r'NASA_CHECK (\{[^\n]+\})', log.read_text())
            require(len(messages) == 1 and json.loads(messages[0]) == proof, 'Proof differs from stdout')
            check(proof, family, position)
            last = (proof, family, position)
            fixture = 'src/qtgui/tests/command-operation-navigation/'+family+'.json'
            spec = json.loads(frozen[fixture])
            origin = frozen[spec['operation_fixture_origin']]
            require(hashlib.sha256(origin).hexdigest() == spec['operation_fixture_sha256'] and spec['script'] == json.loads(origin)['script'], 'Original navigation fixture changed')
            require(proof['fixture_sha256'] == hashlib.sha256(frozen[fixture]).hexdigest() == sha(bound(root/'fixture.json')), 'Executed fixture differs')
            require(('If Keep == 41; % operation branch begin' in bound(root/'initial.script').read_text()) == (position == 'Branch'), 'Wrong branch context')
            for phase in range(2):
                raw = [float(v) for v in bound(root/f'{phase}-states.csv').read_text().strip().split(',')]
                require(raw == proof['results'][phase]['raw'], 'Raw state differs')
                summary_values(bound(root/f'{phase}-summary.txt').read_text(), family)
                extra = [[float(v) for v in re.split(r'[,\s]+', line.strip())] for line in bound(root/f'{phase}-extra.csv').read_text().splitlines() if line.strip()]
                require(len(extra) == 3 and extra == proof['results'][phase]['side_effects'], 'Missing navigation states')
                for index, values in enumerate(extra):
                    compare(values, [index+1, 25357.5073383609] + circle(600), [0, 2e-10] + [2e-5]*3 + [2e-8]*3)
                for kind in ['filter', 'smoother']:
                    records = gps_report(bound(root/f'{phase}-{kind}-report.txt').read_text(), kind == 'smoother')
                    declared = proof['results'][phase]['navigation']['navigation_records'][kind+'.txt']
                    require(len(declared) == 11 and proof['results'][phase]['navigation']['navigation_rows'][kind+'.txt'] == 33, 'Missing declared GPS records')
                    for index, record in enumerate(declared):
                        require(record['record'] == index and record['observed'] == [v[0] for v in records[index]] and record['computed'] == [v[1] for v in records[index]], 'GPS proof differs from raw output')
                observations = [line.split() for line in bound(root/f'{phase}-navigation.gmd').read_text().splitlines() if line.strip() and not line.startswith('%')]
                require(len(observations) == 11, 'Missing observation file records')
                require(bound(root/f'{phase}-filter-warm.csv').stat().st_size > 0, 'Missing warm start file')
            selected.append(dict(mode=mode, name=name, log=str(log)))
    negatives = []
    for label, mutate in [('wrong-state', lambda p: p['results'][0]['raw'].__setitem__(1, 0)), ('missing-close', lambda p: p['operations'].remove('close-tree')), ('missing-reopen', lambda p: p['results'].pop()), ('wrong-position', lambda p: p.update(position='Missing'))]:
        corrupt = copy.deepcopy(last[0])
        mutate(corrupt)
        try:
            check(corrupt, last[1], last[2])
        except (ValueError, AssertionError):
            negatives.append(label)
        else:
            raise ValueError('Accepted corrupt '+label)
    work = load(bound(stage/'scope-continuation.json'))
    require(work['factory_census_count'] == len(work['factory_types']) == 53 and work['qualified_keys'] == [], 'Narrowed original census')
    require(sha(bound(work['prior_worklist'])) == work['prior_worklist_sha256'], 'Prior census changed')
    return dict(status='audited-partial-navigation-operations-with-retained-failure', qualified_keys=[],
                attempted_gui_contexts=6, passing_gui_contexts=5, new_native_gui_passes=10,
                retained_native_failures=2, raw_operation_rows=20, independent_summary_components=360,
                extra_navigation_state_rows=60, gps_report_component_rows=1320,
                negative_controls=negatives, selected=selected, failures=failures,
                remaining_parent='All Others!E196', technical_core_approval=False,
                scientific_approval=False, full_current_build_qualified=False, full_NASA_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(Path(__file__).resolve().parents[3], args.stage.resolve())
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k: v for k, v in result.items() if k not in ('selected', 'failures')}))
