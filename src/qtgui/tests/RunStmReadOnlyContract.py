"""Run/replay readonly STM setter checks with isolated startup and native maps."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

from AcceptanceRuntime import isolate_startup

TYPES = ['OrbitSTM', 'OrbitSTMA', 'OrbitSTMB', 'OrbitSTMC', 'OrbitSTMD']
PROVIDERS = ['original', 'clone', 'copy']


def audit(root):
    root = Path(root)
    proof = json.loads((root / 'proof.json').read_text())
    rows = proof['generic_cases']; typed = proof['typed_controls']
    issues = []
    if [(c['type'], c['provider']) for c in rows] != [(t, p) for t in TYPES for p in PROVIDERS]:
        issues.append('Generic setter case census differs')
    if [c['type'] for c in typed] != TYPES: issues.append('Typed control census differs')
    checked = []
    for row in rows:
        size = 6 if row['type'] == 'OrbitSTM' else 3
        shape = all(len(row[k]) == size and all(len(v) == size for v in row[k]) for k in ['cached_before', 'cached_after'])
        unchanged = row['cached_before'] == row['cached_after']
        diagnostic = any(word in row['diagnostic'].lower() for word in ['read-only', 'read only'])
        passed = shape and not row['settable'] and row['rejected'] and diagnostic and unchanged and row['owner_unchanged']
        if row['cached_unchanged'] != unchanged or row['passed'] != passed:
            issues.append('Native setter proof disagrees with raw observations')
        checked.append(dict(type=row['type'], provider=row['provider'], passed=passed))
    typed_valid = all(c['rejected'] and c['unchanged'] and c['diagnostic'] and c['passed'] for c in typed)
    failed = sum(not c['passed'] for c in checked) + sum(not c['passed'] for c in typed)
    if proof['failed_cases'] != failed or proof['native_missions'] != 0: issues.append('Native counts differ')
    if not typed_valid: issues.append('Typed setter control failed')
    status = 'passed' if not issues and not failed else 'failed'
    if proof['status'] != status: issues.append('Native status differs')
    return dict(status='failed' if issues else status, generic_cases=checked,
                generic_passed=sum(c['passed'] for c in checked), generic_failed=sum(not c['passed'] for c in checked),
                typed_controls_passed=sum(bool(c['passed']) for c in typed), issues=issues,
                native_missions=0, native_tests_run_by_replay=0, new_requirement_credit=0,
                selected_frame_conversion_qualified=False)


def run(binary, startup, output):
    output = Path(output).resolve(); output.mkdir(parents=True, exist_ok=True)
    private = isolate_startup(startup, output / 'runtime/bin')
    ready, ack = output / 'map-ready', output / 'map-ack'
    command = [str(Path(binary).resolve()), str(private)]
    environment = dict(os.environ, GMAT_QT_ACCEPTANCE_DIR=str(output), GMAT_STM_MAP_READY=str(ready),
                       GMAT_STM_MAP_ACK=str(ack), XDG_CONFIG_HOME=str(output / 'config'), XDG_CACHE_HOME=str(output / 'cache'))
    started = datetime.now(timezone.utc).isoformat()
    with (output / 'native.log').open('xb') as log:
        worker = subprocess.Popen(command, cwd=private.parent, env=environment, stdout=log, stderr=subprocess.STDOUT)
        try:
            until = time.monotonic() + 20
            while not ready.exists() and worker.poll() is None and time.monotonic() < until: time.sleep(.01)
            if not ready.exists() or int(ready.read_text()) != worker.pid: raise RuntimeError('STM native image handshake failed')
            maps = Path(f'/proc/{worker.pid}/maps').read_text()
            (output / 'loaded-images.txt').write_text(maps)
            paths = {Path(line.split()[-1]) for line in maps.splitlines() if len(line.split()) >= 6 and line.split()[-1].startswith('/')}
            images = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths if p.is_file()}
            (output / 'loaded-images.json').write_text(json.dumps(images, indent=2) + '\n')
            ack.write_text('mapped\n'); code = worker.wait(timeout=60)
        finally:
            if worker.poll() is None: worker.kill(); worker.wait()
    receipt = dict(command=command, returncode=code, native_pid=worker.pid, native_terminal=worker.poll() is not None,
                   started_utc=started, finished_utc=datetime.now(timezone.utc).isoformat(), runtime_binary_copies=0)
    (output / 'run.json').write_text(json.dumps(receipt, indent=2) + '\n')
    result = audit(output)
    (output / 'independent-audit.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ['status', 'generic_passed', 'generic_failed', 'typed_controls_passed']}))
    return code or (0 if result['status'] == 'passed' else 1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path); parser.add_argument('startup', type=Path)
    parser.add_argument('--output', type=Path); args = parser.parse_args()
    if args.output: raise SystemExit(run(args.binary, args.startup, args.output))
    with tempfile.TemporaryDirectory(prefix='gmat-stm-readonly-') as directory:
        raise SystemExit(run(args.binary, args.startup, directory))
