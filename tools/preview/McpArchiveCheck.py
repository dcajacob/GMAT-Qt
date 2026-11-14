"""Exercise one extracted installation through the installed GMAT MCP server."""
import argparse
import asyncio
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import tomllib

from mcp import Client, StdioServerParameters


async def call(client, name, **arguments):
    result = await client.call_tool(name, arguments)
    if result.is_error or result.structured_content is None:
        raise RuntimeError(f'{name} failed: {result}')
    return result.structured_content


def command(arguments, destination):
    result = subprocess.run([sys.executable, '-m', 'gmat_mcp', *arguments],
                            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    destination.write_text(result.stdout)
    destination.with_suffix('.stderr').write_text(result.stderr)
    if result.returncode:
        raise RuntimeError(f'GMAT MCP command failed: {arguments}; {result.stderr}')
    return json.loads(result.stdout)


def mission():
    speed = math.sqrt(398600.4415 / 7000)
    return f'''Create Spacecraft Sat;
Sat.DateFormat = A1ModJulian;
Sat.Epoch = '21545';
Sat.CoordinateSystem = EarthMJ2000Eq;
Sat.DisplayStateType = Cartesian;
Sat.X = 7000;
Sat.Y = 0;
Sat.Z = 0;
Sat.VX = 0;
Sat.VY = {speed:.17g};
Sat.VZ = 0;
Create ForceModel FM;
FM.PrimaryBodies = {{}};
FM.PointMasses = {{Earth}};
FM.Drag = None;
FM.SRP = Off;
Create Propagator P;
P.FM = FM;
P.Accuracy = 1e-12;
P.MinStep = 0;
P.MaxStep = 5;
Create OrbitView Orbit;
Orbit.Add = {{Sat, Earth}};
BeginMissionSequence;
Propagate P(Sat) {{Sat.ElapsedSecs = 600}};
'''


async def execute(client, session):
    configured = await call(client, 'configure_results', session_id=session['session_id'],
        expected_version=session['model_version'], spec={'datasets': [{
            'id': 'orbit', 'spacecraft': 'Sat', 'frame': 'EarthMJ2000Eq', 'body': 'Earth',
            'cartesian': True, 'keplerian': False}]})
    run = await call(client, 'run_mission', session_id=session['session_id'])
    async with asyncio.timeout(180):
        while run['status'] in ('queued', 'running'):
            await asyncio.sleep(.05)
            run = await call(client, 'get_run', run_id=run['run_id'])
    assert run['status'] == 'succeeded', run
    state = await call(client, 'get_object', session_id=session['session_id'], name='Sat',
                       fields=['X', 'Y', 'Z', 'VX', 'VY', 'VZ'], view='runtime')
    values = [float(field['value']) for field in state['fields']]
    omega = math.sqrt(398600.4415 / 7000 ** 3)
    angle = omega * 600
    truth = [7000 * math.cos(angle), 7000 * math.sin(angle), 0,
             -7000 * omega * math.sin(angle), 7000 * omega * math.cos(angle), 0]
    errors = [abs(a - b) for a, b in zip(values, truth)]
    assert max(errors[:3]) < 2e-5 and max(errors[3:]) < 3e-8, errors
    datasets = await call(client, 'list_datasets', run_id=run['run_id'])
    dataset = next(d for d in datasets['datasets'] if d['id'] == 'orbit')
    assert dataset['complete'] and dataset['rows'] >= 2, dataset
    samples = await call(client, 'query_results', run_id=run['run_id'], dataset_id='orbit', limit=2)
    assert len(samples['rows']) == 2
    return {'status': run['status'], 'run_id': run['run_id'], 'state': values,
            'max_position_error_km': max(errors[:3]), 'max_velocity_error_km_s': max(errors[3:]),
            'result_rows': dataset['rows'], 'retrieved_rows': len(samples['rows']),
            'configuration_committed': configured['committed']}


async def qualify(args):
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    root = args.root.resolve()
    discovery = command(['discover', '--root', str(root), '--python', args.worker], output/'discovery.json')
    assert len(discovery['profiles']) == 1 and discovery['profiles'][0]['ready'], discovery
    config = output/'profiles.toml'
    initialized = command(['init', '--root', str(root), '--python', args.worker, '--output', str(config)],
                          output/'profile-generation.json')
    assert len(initialized['profiles']) == 1
    contents = config.read_text().replace('[server]\n', '[server]\n' +
        'data_dir = ' + json.dumps(str(output/'data')) + '\n' +
        'desktop_discovery_dir = ' + json.dumps(str(output/'desktops')) + '\n')
    config.write_text(contents)
    parsed = tomllib.loads(contents)
    assert len(parsed['profiles']) == 1
    profile_id = next(iter(parsed['profiles']))
    assert not parsed['profiles'][profile_id].get('desktop_only', False)
    assert 'validation_profile' not in parsed['profiles'][profile_id]
    doctor = command(['--config', str(config), 'doctor'], output/'doctor.json')
    assert len(doctor['profiles']) == 1 and doctor['profiles'][0]['ok'], doctor
    engine = doctor['profiles'][0]['engine']
    assert {'Spacecraft', 'GroundStation', 'BatchEstimator'} <= set(engine['classes'])
    assert Path(engine['api']).resolve().is_relative_to(root), engine['api']
    for library in engine['native_libraries']:
        assert Path(library['path']).resolve().is_relative_to(root), library
    result = {'status': 'running', 'profile_id': profile_id, 'root': str(root),
              'worker_python': args.worker, 'doctor': doctor, 'one_profile': True}
    parameters = StdioServerParameters(command=sys.executable,
        args=['-m', 'gmat_mcp', '--config', str(config), 'serve'], env=dict(os.environ))
    gui = None
    try:
        async with Client(parameters, cache=None) as client:
            session = await call(client, 'create_session', profile_id=profile_id)
            assert session['backend'] == 'worker', session
            await call(client, 'load_script', session_id=session['session_id'], text=mission())
            session = await call(client, 'get_session', session_id=session['session_id'])
            result['worker'] = await execute(client, session)
            await call(client, 'close_session', session_id=session['session_id'])
            script = output/'desktop.script'
            script.write_text(mission())
            launcher = root/'run-gmat'
            if not launcher.is_file():
                launcher = root/('Contents/MacOS/GmatQt' if root.suffix == '.app' else 'bin/GmatQt.exe')
            environment = dict(os.environ, QT_QPA_PLATFORM='offscreen',
                               QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM='1')
            with (output/'desktop.log').open('w') as log:
                gui = subprocess.Popen([str(launcher), '--mcp-bridge', '--mcp-discovery-dir',
                    str(output/'desktops'), '--settings-dir', str(output/'settings'), str(script)],
                    env=environment, cwd=output, stdout=log, stderr=subprocess.STDOUT)
            async with asyncio.timeout(60):
                while True:
                    desktops = await call(client, 'list_desktops')
                    if desktops['desktops']:
                        break
                    assert gui.poll() is None, (output/'desktop.log').read_text()
                    await asyncio.sleep(.1)
            assert len(desktops['desktops']) == 1
            session = await call(client, 'attach_desktop', profile_id=profile_id,
                desktop_id=desktops['desktops'][0]['desktop_id'])
            assert session['backend'] == 'desktop'
            edited = await call(client, 'edit_mission', session_id=session['session_id'],
                expected_version=session['model_version'],
                changes=[{'op': 'set_field', 'name': 'Sat', 'field': 'DryMass', 'value': 1234}])
            assert edited['committed']
            obj = await call(client, 'get_object', session_id=session['session_id'], name='Sat', fields=['DryMass'])
            assert obj['fields'][0]['value'] == 1234
            session = await call(client, 'get_session', session_id=session['session_id'])
            result['desktop'] = await execute(client, session)
            result['desktop']['mission_edit_committed'] = True
            await call(client, 'detach_desktop', session_id=session['session_id'])
            assert gui.poll() is None
            result['desktop']['detach_preserved_gui'] = True
        result['status'] = 'passed'
    finally:
        if gui is not None and gui.poll() is None:
            gui.terminate()
            try:
                gui.wait(timeout=15)
            except subprocess.TimeoutExpired:
                gui.kill()
                gui.wait()
        (output/'mcp-verification.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({'status': result['status'], 'profile_id': profile_id}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', required=True, type=Path)
    parser.add_argument('--worker', required=True)
    parser.add_argument('--output', required=True, type=Path)
    asyncio.run(qualify(parser.parse_args()))
