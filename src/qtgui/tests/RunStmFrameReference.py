"""Run/replay the missing two-end STM frame/reference ownership witnesses."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import tempfile
import time

from AcceptanceRuntime import isolate_startup
from StmFrameOracle import expected

TYPES = ['OrbitSTM', 'OrbitSTMA', 'OrbitSTMB', 'OrbitSTMC', 'OrbitSTMD']
PROVIDERS = ['original', 'clone', 'copy']


def audit(root, fixture):
    root = Path(root); fixture = json.loads(Path(fixture).read_text())
    proof = json.loads((root / 'proof.json').read_text()); rows = proof['observations']
    cases = {c['key']: c for c in fixture['cases']}; issues, checks = [], []
    if [(v['case'], v['provider'], v['type']) for v in rows] != [
            (c, p, t) for c in cases for p in PROVIDERS for t in TYPES]: issues.append('Case census differs')
    if fixture['matrix_tolerance'] != 1e-9 or proof['native_missions'] != 0: issues.append('Bounds or execution scope differs')
    for row in rows:
        c = cases[row['case']]
        truth = expected(c['native_stm'], c['theta'], c['tilt'], c['rate'],
                         c['effective_initial_seconds'], c['effective_final_seconds'])
        if truth != c['expected_stm']: issues.append('Frozen mathematical expectation differs')
        t = TYPES.index(row['type']); size = 6 if t == 0 else 3
        r, col = (0, 0) if t == 0 else (3*((t-1)//2), 3*((t-1)%2))
        actual = row.get('actual', [])
        shape = len(actual) == size and all(len(v) == size for v in actual)
        error = max((abs(actual[i][j] - truth[i+r][j+col]) for i in range(size) for j in range(size)), default=math.inf) if shape else math.inf
        passed = shape and math.isfinite(error) and error <= 1e-9 and row.get('owner_unchanged', False)
        if row['passed'] != passed: issues.append('Native proof disagrees with retained values')
        checks.append(dict(case=row['case'], provider=row['provider'], type=row['type'], passed=passed,
                           max_error=error if math.isfinite(error) else None))
    failed = sum(not v['passed'] for v in checks)
    if proof['failed'] != failed: issues.append('Native failed count differs')
    return dict(status='passed' if not issues and not failed else 'failed', passed=len(checks)-failed,
                failed=failed, reads=len(checks), contexts=len(cases)*3, issues=issues, checks=checks,
                native_tests_run_by_replay=0, native_missions=0, new_requirement_credit=0)


def run(binary, startup, fixture, output):
    output = Path(output).resolve(); output.mkdir(parents=True, exist_ok=True)
    private = isolate_startup(startup, output / 'runtime/bin')
    ready, ack = output / 'map-ready', output / 'map-ack'
    command = [str(Path(binary).resolve()), str(private), str(Path(fixture).resolve())]
    env = dict(os.environ, GMAT_QT_ACCEPTANCE_DIR=str(output), GMAT_STM_MAP_READY=str(ready),
               GMAT_STM_MAP_ACK=str(ack), XDG_CONFIG_HOME=str(output / 'config'), XDG_CACHE_HOME=str(output / 'cache'))
    started = datetime.now(timezone.utc).isoformat()
    with (output / 'native.log').open('xb') as log:
        process = subprocess.Popen(command, cwd=private.parent, env=env, stdout=log, stderr=subprocess.STDOUT)
        try:
            until = time.monotonic() + 20
            while not ready.exists() and process.poll() is None and time.monotonic() < until: time.sleep(.01)
            if not ready.exists() or int(ready.read_text()) != process.pid: raise RuntimeError('STM frame mapping handshake failed')
            maps = Path(f'/proc/{process.pid}/maps').read_text(); (output / 'loaded-images.txt').write_text(maps)
            paths = {Path(line.split()[-1]) for line in maps.splitlines() if len(line.split()) >= 6 and line.split()[-1].startswith('/')}
            images = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths if p.is_file()}
            (output / 'loaded-images.json').write_text(json.dumps(images, indent=2) + '\n')
            ack.write_text('mapped\n'); code = process.wait(timeout=60)
        finally:
            if process.poll() is None: process.kill(); process.wait()
    (output / 'run.json').write_text(json.dumps(dict(command=command, returncode=code, native_pid=process.pid,
        native_terminal=process.poll() is not None, started_utc=started, finished_utc=datetime.now(timezone.utc).isoformat(),
        fixture_sha256=hashlib.sha256(Path(fixture).read_bytes()).hexdigest(), runtime_binary_copies=0), indent=2) + '\n')
    result = audit(output, fixture); (output / 'independent-audit.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ['status', 'passed', 'failed', 'reads', 'contexts']}))
    return code or (0 if result['status'] == 'passed' else 1)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('binary', type=Path); p.add_argument('startup', type=Path); p.add_argument('fixture', type=Path)
    p.add_argument('--output', type=Path); a = p.parse_args()
    if a.output: raise SystemExit(run(a.binary, a.startup, a.fixture, a.output))
    with tempfile.TemporaryDirectory(prefix='gmat-stm-frame-') as directory:
        raise SystemExit(run(a.binary, a.startup, a.fixture, directory))
