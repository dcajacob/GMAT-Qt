"""Run the missing B1 file invariants and inspect retained bytes independently."""
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
from CcsdsOemB1Oracle import ConformanceError, validate
from RunOemCreationClock import creation_check

CASES = ['SameObject', 'EquivalentObjectId', 'ChangedObject', 'ChangedObjectRewrite',
         'ReopenOtherObject', 'CloneOtherObject', 'B1Acceleration', 'B1Covariance',
         'B1BothExtensions', 'OEM2Extensions']
REJECTIONS = {'ChangedObject', 'ChangedObjectRewrite', 'B1Acceleration', 'B1Covariance', 'B1BothExtensions'}


def audit(root):
    root = Path(root)
    proof = json.loads((root / 'proof.json').read_text())
    receipt = json.loads((root / 'run.json').read_text())
    issues, observations = [], []
    if [c['case'] for c in proof['cases']] != CASES:
        issues.append('Case census differs')
    if proof['native_missions'] != 0:
        issues.append('Unexpected mission execution')
    for case in proof['cases']:
        name = case['case']
        errors = []
        if case['expected_rejection'] != (name in REJECTIONS):
            errors.append('Rejection expectation differs')
        if not case['passed']:
            errors.append('Native witness failed')
        if name in REJECTIONS and not (case.get('rejected') and case.get('unchanged') and case.get('diagnostic')):
            errors.append('Rejection was absent, mutating or unexplained')
        files = [root / (name + '.oem')]
        if name == 'CloneOtherObject': files.append(root / (name + '.oem.clone'))
        if name == 'ReopenOtherObject': files.append(root / (name + '.oem.original'))
        findings = []
        for path in files:
            raw = path.read_bytes()
            file_errors = []
            if name == 'OEM2Extensions':
                lines = raw.decode('ascii').splitlines()
                data = [line.split() for line in lines if line.startswith('2000-')]
                if len(data) != 2 or any(len(row) != 10 for row in data):
                    file_errors.append('OEM2 acceleration record shape differs')
                else:
                    if [list(map(float, row[1:])) for row in data] != [
                            [7000., float(i), 0., 0., 1., 0., 28., 29., 30.] for i in range(2)]:
                        file_errors.append('OEM2 state/acceleration values differ')
                if lines.count('COVARIANCE_START') != 1 or lines.count('COVARIANCE_STOP') != 1:
                    file_errors.append('OEM2 covariance blocks differ')
                else:
                    cov = lines[lines.index('COVARIANCE_START') + 1:lines.index('COVARIANCE_STOP')]
                    matrices = [line for line in cov if line.strip() and not line.startswith('EPOCH')]
                    expected = [[float(j) for j in range(7 + k * (k + 1) // 2, 7 + (k + 1) * (k + 2) // 2)] for k in range(6)]
                    if [list(map(float, line.split())) for line in matrices] != expected * 2:
                        file_errors.append('OEM2 covariance values differ')
                finding = {'status': 'passed' if not file_errors else 'failed', 'scope': 'OEM2 shared extension compatibility only'}
            else:
                try: finding = validate(raw, icd_object_ids=['GMAT ONE'])
                except ConformanceError as error:
                    finding = dict(status='failed', clause=error.clause, line=error.line, message=error.message)
                if finding['status'] != 'passed': file_errors.append('Independent B1 conformance failed')
                data = [line.split() for line in raw.decode('ascii').splitlines() if line.startswith('2000-')]
                count = 4 if name in CASES[:4] else 2
                if len(data) != count or any(len(row) != 7 for row in data):
                    file_errors.append('Recovered B1 record shape differs')
                else:
                    expected = [[7000., float(i), 0., 0., 1., 0.] for i in range(count)]
                    if [list(map(float, row[1:])) for row in data] != expected:
                        file_errors.append('Recovered B1 states differ')
                    if [row[0] for row in data] != [f'2000-01-01T12:00:0{i}.000' for i in range(count)]:
                        file_errors.append('Recovered B1 epochs differ')
                if name in ('CloneOtherObject', 'ReopenOtherObject'):
                    expected_id = '2000-002A' if str(path).endswith('.clone') or (name == 'ReopenOtherObject' and not str(path).endswith('.original')) else '2000-001A'
                    ids = [line.split('=', 1)[1].strip() for line in raw.decode('ascii').splitlines() if line.startswith('OBJECT_ID')]
                    if ids != [expected_id]: file_errors.append('Independent writer identity not preserved')
            clock = creation_check(raw.partition(b'META_START')[0], receipt['started_utc'], receipt['finished_utc'])
            if clock['status'] != 'passed': file_errors.append('Creation clock differs')
            findings.append(dict(file=path.name, sha256=hashlib.sha256(raw).hexdigest(), finding=finding,
                                 clock=clock, issues=file_errors))
            errors.extend(file_errors)
        observations.append(dict(case=name, status='failed' if errors else 'passed', issues=errors, files=findings))
    if receipt['returncode'] or proof['failures']:
        issues.append('Native run failed')
    return dict(status='passed' if not issues and all(o['status'] == 'passed' for o in observations) else 'failed',
                issues=issues, observations=observations, native_missions=0, new_requirement_credit=0,
                full_current_build_qualified=False)


def run(binary, startup, root):
    root = Path(root).resolve(); root.mkdir(parents=True, exist_ok=True)
    private = isolate_startup(startup, root / 'runtime/bin')
    ready, ack = root / 'map-ready', root / 'map-ack'
    env = dict(os.environ, GMAT_QT_ACCEPTANCE_DIR=str(root), GMAT_OEM_MAP_READY=str(ready),
               GMAT_OEM_MAP_ACK=str(ack), XDG_CONFIG_HOME=str(root / 'config'), XDG_CACHE_HOME=str(root / 'cache'))
    command = [str(Path(binary).resolve()), str(private)]
    started = datetime.now(timezone.utc).isoformat()
    with (root / 'native.log').open('xb') as log:
        process = subprocess.Popen(command, cwd=private.parent, env=env, stdout=log, stderr=subprocess.STDOUT)
        try:
            until = time.monotonic() + 8
            while not ready.exists() and process.poll() is None and time.monotonic() < until: time.sleep(.01)
            if not ready.exists(): raise RuntimeError('Native image handshake did not start')
            if int(ready.read_text()) != process.pid: raise RuntimeError('Native image PID differs')
            maps = Path(f'/proc/{process.pid}/maps').read_text()
            (root / 'loaded-images.txt').write_text(maps)
            paths = {Path(line.split()[-1]) for line in maps.splitlines() if len(line.split()) >= 6 and line.split()[-1].startswith('/')}
            images = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths if path.is_file()}
            (root / 'loaded-images.json').write_text(json.dumps(images, indent=2) + '\n')
            ack.write_text('mapped\n')
            code = process.wait(timeout=30)
        finally:
            if process.poll() is None: process.kill(); process.wait()
    receipt = dict(command=command, returncode=code, started_utc=started,
                   finished_utc=datetime.now(timezone.utc).isoformat(), native_pid=process.pid,
                   native_terminal=process.poll() is not None, runtime_binary_copies=0)
    (root / 'run.json').write_text(json.dumps(receipt, indent=2) + '\n')
    result = audit(root)
    (root / 'independent-audit.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(status=result['status'], cases=len(result['observations']),
                         passed=sum(o['status'] == 'passed' for o in result['observations']), returncode=code)))
    return code or (0 if result['status'] == 'passed' else 1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path); parser.add_argument('startup', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if args.output: raise SystemExit(run(args.binary, args.startup, args.output))
    with tempfile.TemporaryDirectory(prefix='gmat-oem-file-invariants-') as directory:
        raise SystemExit(run(args.binary, args.startup, directory))
