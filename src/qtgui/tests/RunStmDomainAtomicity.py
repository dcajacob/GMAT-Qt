"""Run/replay supplementary public-STM domain and transaction checks."""
import argparse
import json
import math
from pathlib import Path
import tempfile
import RunStmFrameReference as runtime
from StmFrameOracle import expected

TYPES = ['OrbitSTM', 'OrbitSTMA', 'OrbitSTMB', 'OrbitSTMC', 'OrbitSTMD']


def audit(root, fixture):
    plan = json.loads(Path(fixture).read_text())
    proof = json.loads((Path(root) / 'proof.json').read_text())
    rows = proof['observations']
    cases = {c['key']: c for c in plan['cases']}
    issues, checks = [], []
    if [(r['case'], r['type']) for r in rows] != [(c, t) for c in cases for t in TYPES]:
        issues.append('Domain/type census differs')
    if plan['matrix_tolerance'] != 1e-9 or proof['native_missions'] != 0:
        issues.append('Numerical bound/execution scope differs')
    truth = expected(plan['native_stm'], .37, .23, .001, 0., 45.)
    if truth != plan['expected_stm']:
        issues.append('Frozen analytic two-end spin expectation differs')
    for row in rows:
        c = cases[row['case']]
        failures = []
        for part in ['owner', 'guide', 'frame']:
            before, after = row[part+'_before'], row[part+'_after']
            if part == 'owner' and c['mode'] == 'fresh_valid_axes':
                before = dict(before); after = dict(after)
                old_history, new_history = before.pop('history'), after.pop('history')
                if old_history['present'] or not new_history['present'] or new_history['advanced'] or new_history['epoch'] != after['epoch'] or new_history['self']['state'] != after['state'][:6]:
                    failures.append('Successful read did not commit its correct initial reference')
            if before != after:
                failures.append(part+' changed')
        if not row['readonly']:
            failures.append('Parameter is writable')
        if c['outcome'] == 'reject':
            if not row['rejected'] or c['diagnostic'].lower() not in row['diagnostic'].lower():
                failures.append('Required diagnostic/rejection absent')
            if row['cache_before'] != row['cache_after']:
                failures.append('Rejected read changed Parameter cache')
        else:
            if row['rejected']:
                failures.append('Valid matrix rejected')
            t = TYPES.index(row['type']); n = 6 if t == 0 else 3
            a, b = (0, 0) if t == 0 else (3*((t-1)//2), 3*((t-1)%2))
            full = [[0.]*6 for _ in range(6)] if c['mode'] == 'singular_phi' else ([[float(i == j) for j in range(6)] for i in range(6)] if c['mode'] == 'fresh_valid_axes' else truth)
            actual = row['actual']
            shape = len(actual) == n and all(len(x) == n for x in actual)
            if not shape or any(not isinstance(actual[i][j], (float, int)) or not math.isfinite(actual[i][j]) or abs(actual[i][j]-full[i+a][j+b]) > 1e-9 for i in range(n) for j in range(n)):
                failures.append('Public Cartesian result/partition differs from analytic oracle')
        checks.append(dict(case=row['case'], type=row['type'], passed=not failures, failures=failures))
    failed = sum(not c['passed'] for c in checks)
    return dict(status='passed' if not issues and not failed else 'failed', passed=len(checks)-failed,
                failed=failed, reads=len(checks), contexts=len(cases), issues=issues, checks=checks,
                native_tests_run_by_replay=0, native_missions=0, new_requirement_credit=0)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('binary', type=Path); p.add_argument('startup', type=Path); p.add_argument('fixture', type=Path)
    p.add_argument('--output', type=Path); p.add_argument('--replay', action='store_true')
    a = p.parse_args()
    if a.replay:
        if a.output is None:
            p.error('--replay requires --output')
        result = audit(a.output, a.fixture)
        print(json.dumps(result, indent=2))
        raise SystemExit(result['status'] != 'passed')
    runtime.audit = audit
    if a.output:
        raise SystemExit(runtime.run(a.binary, a.startup, a.fixture, a.output))
    with tempfile.TemporaryDirectory(prefix='gmat-stm-domain-') as directory:
        raise SystemExit(runtime.run(a.binary, a.startup, a.fixture, directory))
