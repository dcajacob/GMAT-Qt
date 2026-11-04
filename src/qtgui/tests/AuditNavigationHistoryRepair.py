"""Audit the bounded completed-filter lookup repair and reuse prior GUI passes."""
import argparse
import copy
import difflib
import hashlib
import json
import re
import tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require, sha, load, validate_visible
import AuditCommandNavigationOperations as previous

CASES = ['NestedIf', 'Else', 'SkippedMatching', 'FutureSkipped', 'WrongNamed']
GUARDS = ['QtGui.NasaNavigationHistory'+c for c in CASES]
BRANCH = 'QtGui.NasaCommandNavigationOperationsRunEstimatorBranch'
REGRESSION = 'QtGui.NasaScriptEventBackendNavigation'
NAMES = GUARDS + [BRANCH, REGRESSION]
CORE = ['plugins/EstimationPlugin/src/base/command/RunEstimator.cpp',
        'plugins/EstimationPlugin/src/base/command/RunEstimator.hpp',
        'plugins/ExtendedKalmanFilterPlugin/src/base/command/RunSmoother.cpp']


def check_navigation(proof):
    require(proof['outcome'] == 'passed' and proof['family'] == 'Navigation'
            and proof['native_outcome'] == 'Completed', 'Navigation mission did not complete')
    previous.compare(proof['observed'], [20]+previous.circle(20)+[41, 3, 1, 0, 0],
                     [1e-4]+[2e-5]*3+[2e-8]*3+[1e-12]*5)
    require(len(proof['effects']['backend_values']) == 3, 'Missing backend state')
    for index, values in enumerate(proof['effects']['backend_values']):
        previous.compare(values, [index+1, 25357.5073383609]+previous.circle(600),
                         [0, 2e-10]+[2e-5]*3+[2e-8]*3)


def audit(repo, stage):
    manifest = load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for name, digest in manifest.items():
        require(not Path(name).is_symlink() and sha(name) == digest, 'Changed regular artifact '+name)
    def bound(path):
        path = Path(path)
        require(str(path) in manifest, 'Unbound artifact '+str(path))
        return path
    def archive(path):
        with tarfile.open(bound(path)) as tar:
            return {m.name: tar.extractfile(m).read() for m in tar.getmembers()}
    before = load(bound(stage/'visible-inputs-before.json'))
    require(before == load(bound(stage/'visible-inputs-after.json')), 'Native inputs changed')
    launch = load(bound(stage/'visible-launch.json'))
    require(launch['returncode'] == 0 and launch['inputs_unchanged'] and launch['mapped_processes'] == 7, 'Visible run incomplete')
    frozen = archive(stage/'executed-sources-v4.tar.gz')
    require({n: hashlib.sha256(v).hexdigest() for n, v in frozen.items()} == before['sources'], 'Executed source archive differs')
    original = archive(stage/'source-v1.tar.gz')
    repaired = archive(stage/'source-v2.tar.gz')
    delta = []
    for name in CORE:
        require(original[name] == bound(stage/Path(name).name).read_bytes(), 'Core preimage differs')
        require(repaired[name] == frozen[name], 'Core changed after unattended repair run')
        delta.extend(difflib.unified_diff(original[name].decode().splitlines(True), frozen[name].decode().splitlines(True), fromfile=name+'.before', tofile=name))
    require(''.join(delta) == bound(stage/'core-v2.patch').read_text(), 'Exact production patch differs')
    for name in ['src/qtgui/tests/NasaScriptEventCommandTests.cpp', 'src/qtgui/tests/NasaCommandNavigationOperationTests.cpp'] + ['src/qtgui/tests/navigation-history/'+c+'.json' for c in CASES]:
        require(original[name] == repaired[name] == frozen[name], 'Pre-fix assertion/fixture changed')
    old_images = load(bound(stage/'before-core-images.json'))
    for library in ['libGmatEstimation.so.R2026a', 'libEKF.so.R2026a']:
        found = [n for n in before['images'] if Path(n).name == library]
        require(len(found) == 1 and old_images[found[0]] != before['images'][found[0]], 'Repaired plugin not rebuilt')

    old_run = load(bound(repo/'build/gui-acceptance/navigation-history-headless-v1/results.json'))
    active = {v['name']: v for v in old_run['results'] if v['status'] != 'not-run'}
    require(set(active) == set(GUARDS), 'Original guard census changed')
    for name, row in active.items():
        failed = name.endswith(('NestedIf', 'Else', 'WrongNamed'))
        require(row['status'] == ('failed' if failed else 'passed'), 'Original failure set changed')
        if failed:
            log = bound(row['log'])
            require('FAIL Contained command mission failed or timed out' in log.read_text(), 'Original failure lost')
            require('Mission run failed' in bound(log.parent/'runtime/output/GmatLog.txt').read_text(), 'Original backend failure lost')

    mapped = load(bound(stage/'loaded-images.json'))['processes']
    selected = []
    final_proof = None
    for mode, version in [('headless', 'v2'), ('visible', 'v1')]:
        receipt = load(bound(repo/'build/gui-acceptance'/('navigation-history-'+mode+'-'+version)/'results.json'))
        require(receipt['mode'] == mode and receipt.get('finished_utc') and receipt['end_reason'] == 'completed-selection', 'Nonterminal repaired run')
        source_version = dict(before['sources'])
        if mode == 'headless':
            source_version.update({n: hashlib.sha256(v).hexdigest() for n, v in repaired.items()})
        require(all(source_version.get(n) == h for n, h in receipt['source_hashes'].items()), 'Wrong source version')
        active = {v['name']: v for v in receipt['results'] if v['status'] != 'not-run'}
        require(set(active) == set(NAMES), 'Repaired selection changed')
        visible_cases = {}
        for name, row in active.items():
            require(row['status'] == 'passed' and row['returncode'] == 0, 'Repaired GUI case failed')
            log = bound(row['log']); root = log.parent; junit = bound(root/'ctest.xml')
            tests = ET.parse(junit).getroot().findall('testcase')
            require(len(tests) == 1 and tests[0].attrib['name'] == name and tests[0].find('failure') is None and tests[0].find('skipped') is None, 'Missing passing JUnit')
            proof = load(bound(root/'proof.json'))
            lines = re.findall(r'NASA_CHECK (\{[^\n]+\})', log.read_text())
            require(len(lines) == 1 and json.loads(lines[0]) == proof, 'Proof differs from stdout')
            if name == BRANCH:
                previous.check(proof, 'RunEstimator', 'Branch')
                for phase in range(2):
                    raw = [float(v) for v in bound(root/f'{phase}-states.csv').read_text().strip().split(',')]
                    require(raw == proof['results'][phase]['raw'], 'Branch raw state differs')
                    previous.summary_values(bound(root/f'{phase}-summary.txt').read_text(), 'RunEstimator')
                groups = [(f'{phase}-extra.csv', f'{phase}-', proof['results'][phase]['side_effects'], proof['results'][phase]['navigation']) for phase in range(2)]
            else:
                check_navigation(proof); final_proof = proof
                raw = [float(v) for v in bound(root/'results.csv').read_text().strip().split(',')]
                require(raw == proof['observed'], 'Navigation raw state differs')
                case = name.removeprefix('QtGui.NasaNavigationHistory')
                fixture = 'src/qtgui/tests/navigation-history/'+case+'.json' if name != REGRESSION else 'src/qtgui/tests/script-event-backends/Navigation.json'
                require(proof['fixture_sha256'] == hashlib.sha256(frozen[fixture]).hexdigest(), 'Executed fixture differs')
                groups = [('backend-values.csv', '', proof['effects']['backend_values'], proof['effects'])]
            for extra_name, prefix, declared, effects in groups:
                values = [[float(v) for v in re.split(r'[,\s]+', line.strip())] for line in bound(root/extra_name).read_text().splitlines() if line.strip()]
                require(values == declared and len(values) == 3, 'Missing raw navigation states')
                for index, value in enumerate(values):
                    previous.compare(value, [index+1, 25357.5073383609]+previous.circle(600), [0, 2e-10]+[2e-5]*3+[2e-8]*3)
                for kind in ['filter', 'smoother']:
                    records = previous.gps_report(bound(root/(prefix+kind+'-report.txt')).read_text(), kind == 'smoother')
                    actual = effects['navigation_records'][kind+'.txt']
                    require(len(actual) == 11 and effects['navigation_rows'][kind+'.txt'] == 33, 'Missing GPS records')
                    for i, record in enumerate(actual):
                        require(record['record'] == i and record['observed'] == [v[0] for v in records[i]] and record['computed'] == [v[1] for v in records[i]], 'GPS proof differs from raw report')
                require(bound(root/(prefix+'filter-warm.csv')).stat().st_size > 0, 'Missing warm-start output')
            exe = [p for p in row['registered_command'] if p in receipt['binary_hashes']]
            require(len(exe) == 1 and before['images'].get(exe[0]) == receipt['binary_hashes'][exe[0]], 'Wrong executed test binary')
            visible_cases[name] = dict(log=str(log), log_sha256=sha(log), junit=str(junit), junit_sha256=sha(junit))
            if mode == 'visible':
                traces = list(root.glob('trace-*.jsonl')); require(traces, 'No visible process trace')
                for trace in traces:
                    process = mapped.get(trace.stem.removeprefix('trace-'))
                    require(process and process['executable'] == exe[0], 'Missing mapped process')
                    for p, h in process['images'].items():
                        require(before['images'].get(p) == h, 'Mapped image changed')
                    for p, h in before['images'].items():
                        if p == exe[0] or Path(p).name.startswith(('libGmat', 'libEKF')):
                            require(process['images'].get(p) == h, 'Missing mapped core/plugin/test')
            selected.append(dict(mode=mode, name=name, log=str(log)))
        if mode == 'visible': validate_visible(receipt, visible_cases)

    direct_images = load(bound(stage/'direct-inputs-v5.json'))
    require(direct_images['returncode'] == 0 and direct_images['before'] == direct_images['after'], 'Direct runtime inputs changed')
    direct = ET.parse(bound(stage/'direct-v5.xml')).getroot().findall('testcase')
    require(len(direct) == 1 and direct[0].attrib['name'] == 'QtAcceptance.NavigationHistorySelection' and direct[0].find('failure') is None and 'NASA_HISTORY_SELECTION passed:' in direct[0].find('system-out').text, 'Direct graph guards failed')
    negatives = []
    for label, mutate in [('wrong-final-state', lambda p: p['observed'].__setitem__(1, 0)), ('missing-filter-state', lambda p: p['effects']['backend_values'].pop()), ('wrong-smoother-state', lambda p: p['effects']['backend_values'][2].__setitem__(2, 0)), ('false-completion', lambda p: p.update(native_outcome='Failed'))]:
        bad = copy.deepcopy(final_proof); mutate(bad)
        try: check_navigation(bad)
        except (AssertionError, ValueError): negatives.append(label)
        else: raise ValueError('Accepted corrupt '+label)
    reused = previous.audit(repo, repo/'build/nasa-coverage/coverage95-20261103-command-navigation')
    require(reused['passing_gui_contexts'] == 5 and reused['retained_native_failures'] == 2, 'Prior source-versioned outcomes changed')
    return dict(status='passed-bounded-navigation-history-repair', qualified_keys=[],
                new_guard_contexts=5, repaired_operation_contexts=1, affected_regression_contexts=1,
                current_native_gui_passes=14, reused_prior_operation_passes=10,
                complete_navigation_operation_contexts=6, raw_current_state_rows=16,
                current_summary_components=72, current_navigation_state_rows=48,
                current_gps_component_rows=1056, original_guard_failures=3,
                direct_history_selection='passed', negative_controls=negatives, selected=selected,
                remaining_parent='All Others!E196', technical_core_approval=False,
                scientific_approval=False, full_current_build_qualified=False, full_NASA_qualified=False)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--stage', type=Path, required=True); p.add_argument('--output', type=Path, required=True)
    args = p.parse_args(); result = audit(Path(__file__).resolve().parents[3], args.stage.resolve())
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k: v for k, v in result.items() if k != 'selected'}))
