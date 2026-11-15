"""Run the missing tesseral owner-frame STM mission and retain mapped binaries."""
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
from StmTesseralOracle import audit


def run(binary, startup, fixture, output):
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    private = isolate_startup(startup, output/'runtime/bin')
    ready, acknowledgement = output/'map-ready', output/'map-ack'
    command = [str(Path(binary).resolve()), str(private), str(Path(fixture).resolve())]
    environment = dict(os.environ, GMAT_QT_ACCEPTANCE_DIR=str(output),
                       GMAT_STM_MAP_READY=str(ready), GMAT_STM_MAP_ACK=str(acknowledgement),
                       XDG_CONFIG_HOME=str(output/'config'), XDG_CACHE_HOME=str(output/'cache'))
    started = datetime.now(timezone.utc).isoformat()
    with (output/'native.log').open('xb') as log:
        process = subprocess.Popen(command, cwd=private.parent, env=environment,
                                   stdout=log, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic()+20
            while not ready.exists() and process.poll() is None and time.monotonic() < deadline:
                time.sleep(.01)
            if not ready.exists() or int(ready.read_text()) != process.pid:
                raise RuntimeError('Tesseral mapped-binary handshake failed')
            mappings = Path(f'/proc/{process.pid}/maps').read_text()
            (output/'loaded-images.txt').write_text(mappings)
            paths = {Path(line.split()[-1]) for line in mappings.splitlines()
                     if len(line.split()) >= 6 and line.split()[-1].startswith('/')}
            images = {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                      for path in paths if path.is_file()}
            (output/'loaded-images.json').write_text(json.dumps(images, indent=2)+'\n')
            acknowledgement.write_text('mapped\n')
            returncode = process.wait(timeout=90)
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
    receipt = dict(command=command, returncode=returncode, native_pid=process.pid,
                   native_terminal=process.poll() is not None, started_utc=started,
                   finished_utc=datetime.now(timezone.utc).isoformat(),
                   fixture_sha256=hashlib.sha256(Path(fixture).read_bytes()).hexdigest(),
                   runtime_binary_copies=0)
    (output/'run.json').write_text(json.dumps(receipt, indent=2)+'\n')
    if returncode:
        print('Native tesseral mission failed; see native.log')
        return returncode
    try:
        result = audit(output, fixture)
    except Exception as error:
        result = dict(status='failed', reason=str(error), exception=type(error).__name__)
    (output/'independent-audit.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))
    return 0 if result['status'] == 'passed' else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('startup', type=Path)
    parser.add_argument('fixture', type=Path)
    parser.add_argument('--output', type=Path)
    arguments = parser.parse_args()
    if arguments.output:
        raise SystemExit(run(arguments.binary, arguments.startup, arguments.fixture, arguments.output))
    with tempfile.TemporaryDirectory(prefix='gmat-stm-tesseral-') as temporary:
        raise SystemExit(run(arguments.binary, arguments.startup, arguments.fixture, temporary))
