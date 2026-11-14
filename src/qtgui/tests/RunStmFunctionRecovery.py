"""Execute one missing nested-function STM rejection/recovery mission, with retained native maps."""
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
from StmFunctionRecoveryOracle import audit


def run(binary, startup, fixture, output):
    output = Path(output).resolve(); output.mkdir(parents=True, exist_ok=True)
    private = isolate_startup(startup, output/'runtime/bin'); ready, ack = output/'map-ready', output/'map-ack'
    command = [str(Path(binary).resolve()), str(private), str(Path(fixture).resolve())]
    env = dict(os.environ, GMAT_QT_ACCEPTANCE_DIR=str(output), GMAT_STM_MAP_READY=str(ready),
               GMAT_STM_MAP_ACK=str(ack), XDG_CONFIG_HOME=str(output/'config'), XDG_CACHE_HOME=str(output/'cache'))
    start = datetime.now(timezone.utc).isoformat()
    with (output/'native.log').open('xb') as log:
        process = subprocess.Popen(command, cwd=private.parent, env=env, stdout=log, stderr=subprocess.STDOUT)
        try:
            until = time.monotonic()+20
            while not ready.exists() and process.poll() is None and time.monotonic()<until: time.sleep(.01)
            if not ready.exists() or int(ready.read_text()) != process.pid: raise RuntimeError('STM mission mapping handshake failed')
            maps = Path(f'/proc/{process.pid}/maps').read_text(); (output/'loaded-images.txt').write_text(maps)
            paths = {Path(v.split()[-1]) for v in maps.splitlines() if len(v.split())>=6 and v.split()[-1].startswith('/')}
            (output/'loaded-images.json').write_text(json.dumps({str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths if p.is_file()}, indent=2)+'\n')
            ack.write_text('mapped\n'); code = process.wait(timeout=90)
        finally:
            if process.poll() is None: process.kill(); process.wait()
    (output/'run.json').write_text(json.dumps(dict(command=command, returncode=code, native_pid=process.pid,
        native_terminal=process.poll() is not None, started_utc=start, finished_utc=datetime.now(timezone.utc).isoformat(),
        fixture_sha256=hashlib.sha256(Path(fixture).read_bytes()).hexdigest(), runtime_binary_copies=0), indent=2)+'\n')
    if code: print('Native function-recovery mission failed; see native.log'); return code
    try: result = audit(output, fixture)
    except Exception as error:
        result = dict(status='failed', reason=str(error), exception=type(error).__name__)
    (output/'independent-audit.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if not isinstance(v,(dict,list))}))
    return 0 if result['status']=='passed' else 1


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('binary',type=Path);p.add_argument('startup',type=Path);p.add_argument('fixture',type=Path);p.add_argument('--output',type=Path);a=p.parse_args()
    if a.output: raise SystemExit(run(a.binary,a.startup,a.fixture,a.output))
    with tempfile.TemporaryDirectory(prefix='gmat-stm-frame-mission-') as tmp: raise SystemExit(run(a.binary,a.startup,a.fixture,tmp))
