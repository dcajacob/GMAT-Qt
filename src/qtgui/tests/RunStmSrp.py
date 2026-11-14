"""Run the missing spherical-SRP owner-frame STM mission and retain mapped binaries."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import shutil
import tempfile
import time

from AcceptanceRuntime import isolate_startup
from StmSrpOracle import audit


def run(binary, startup, fixture, output):
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    private = isolate_startup(startup, output/'runtime/bin')
    plan = json.loads(Path(fixture).read_text())
    if plan.get('force_type') == 'ExternalModel':
        module = plan['external_module'] + '.py'
        shutil.copyfile(Path(fixture).parent/module, output/module)
        equation = {key: value for key, value in plan.items() if key.startswith('external_')}
        (output/'external-equation.json').write_text(json.dumps(equation, indent=2)+'\n')
        with private.open('a') as stream:
            stream.write('\nPYTHON_MODULE_PATH = '+str(output)+'\n')
    ready, acknowledgement = output/'map-ready', output/'map-ack'
    command = [str(Path(binary).resolve()), str(private), str(Path(fixture).resolve())]
    environment = dict(os.environ, GMAT_QT_ACCEPTANCE_DIR=str(output),
                       GMAT_STM_MAP_READY=str(ready), GMAT_STM_MAP_ACK=str(acknowledgement),
                       XDG_CONFIG_HOME=str(output/'config'), XDG_CACHE_HOME=str(output/'cache'))
    final_ready, final_ack = output/'final-map-ready', output/'final-map-ack'
    if plan.get('force_type') == 'ExternalModel':
        environment.update(GMAT_STM_FINAL_MAP_READY=str(final_ready), GMAT_STM_FINAL_MAP_ACK=str(final_ack))
    started = datetime.now(timezone.utc).isoformat()
    with (output/'native.log').open('xb') as log:
        process = subprocess.Popen(command, cwd=private.parent, env=environment,
                                   stdout=log, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic()+20
            while not ready.exists() and process.poll() is None and time.monotonic() < deadline:
                time.sleep(.01)
            if not ready.exists() or int(ready.read_text()) != process.pid:
                raise RuntimeError('Srp mapped-binary handshake failed')
            mappings = Path(f'/proc/{process.pid}/maps').read_text()
            (output/'loaded-images.txt').write_text(mappings)
            paths = {Path(line.split()[-1]) for line in mappings.splitlines()
                     if len(line.split()) >= 6 and line.split()[-1].startswith('/')}
            images = {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                      for path in paths if path.is_file()}
            (output/'loaded-images.json').write_text(json.dumps(images, indent=2)+'\n')
            acknowledgement.write_text('mapped\n')
            deadline = time.monotonic()+90
            if plan.get('force_type') == 'ExternalModel':
                while not final_ready.exists() and process.poll() is None and time.monotonic() < deadline:
                    time.sleep(.01)
                if final_ready.exists():
                    if int(final_ready.read_text()) != process.pid:
                        raise RuntimeError('Final mapped-binary handshake failed')
                    final_mappings = Path(f'/proc/{process.pid}/maps').read_text()
                    (output/'final-loaded-images.txt').write_text(final_mappings)
                    final_paths = {Path(line.split()[-1]) for line in final_mappings.splitlines()
                                   if len(line.split()) >= 6 and line.split()[-1].startswith('/')}
                    final_images = {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                                    for path in final_paths if path.is_file()}
                    if any(name in images and images[name] != value for name,value in final_images.items()):
                        raise RuntimeError('A loaded image changed during the mission')
                    (output/'initial-loaded-images.json').write_text(json.dumps(images, indent=2)+'\n')
                    (output/'final-loaded-images.json').write_text(json.dumps(final_images, indent=2)+'\n')
                    (output/'loaded-images.json').write_text(json.dumps(dict(images,**final_images), indent=2)+'\n')
                    final_ack.write_text('mapped\n')
            returncode = process.wait(timeout=max(.1,deadline-time.monotonic()))
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
        print('Native spherical-SRP mission failed; see native.log')
        return returncode
    try:
        if plan.get('force_type') == 'ExternalModel':
            from StmExternalOracle import audit as audit_external
            result = audit_external(output, fixture)
        elif json.loads(Path(fixture).read_text()).get('force_type') == 'PolyhedronGravityModel':
            from StmPolyhedronOracle import audit as audit_polyhedron
            result = audit_polyhedron(output, fixture)
        elif json.loads(Path(fixture).read_text()).get('force_type') == 'DragForce':
            from StmDragOracle import audit as audit_drag
            result = audit_drag(output, fixture)
        elif json.loads(Path(fixture).read_text()).get('force_type') in ('FiniteThrust', 'FileThrust'):
            if json.loads(Path(fixture).read_text()).get('post_reports'):
                from StmFiniteThrustReportsOracle import audit as audit_thrust
            else:
                from StmFiniteThrustOracle import audit as audit_thrust
            result = audit_thrust(output, fixture)
        else:
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
    with tempfile.TemporaryDirectory(prefix='gmat-stm-spherical-SRP-') as temporary:
        raise SystemExit(run(arguments.binary, arguments.startup, arguments.fixture, temporary))
