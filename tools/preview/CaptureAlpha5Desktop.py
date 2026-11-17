"""Capture bundled missions in the published GMAT Qt alpha.5 GUI."""

import argparse
from datetime import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import socket
import struct
import subprocess
import tempfile
import time

from BuildPythonRange import unpack

VERSION = '0.1.0-alpha.5'
ARCHIVE_SHA256 = {
    'linux-x86_64': 'b4e27e699a6c733b15be6321309e8a13ecb3adab09d49254a735714dadb72e83',
    'windows-x64': 'b7a23a3776897ea5a0bef02b09cc31ac9e99d61860e5e2b63482981db781ed51',
    'macos-arm64': '3b91630d3aed9fb7d27bee96981bfa7a491a0b34dc677f2f84a298a1091185b8',
    'macos-x64': 'e982238ac4f193b50fa0797a86b46fdda7750c271560f0dabc346cc434948342',
}
MISSION = 'Ex_MarsOrbit.script'
# The last Propagate runs for a day starting at the first periapsis.
FINAL_MPS1_TAI_MJD = 21646.24072759801


TARGETING_MISSIONS = {
    'Ex_MarsBPlane.script': ('MAVEN', '23 Sep 2014 03:34:07.942', (
        ('MarsTransfer', ('OFI_SolarSystemView',)),
        ('MarsCaptureTargeting', ('OFI_MarsView', 'Solver — Target DC {')),
    )),
    'Ex_LunarTransfer.script': ('Sat', '12 Aug 2014 22:07:27.456', (
        ('LunarInsertionTargeting', ('OFI_MoonInertialView', "Solver — Target 'Target Lunar Insertion'")),
    )),
}


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def run(*args, **kwargs):
    return subprocess.run(args, check=True, text=True, **kwargs)


def bridge(record, *operations):
    with socket.create_connection(('127.0.0.1', record['port']), timeout=8) as connection:
        with connection.makefile('rb') as stream:
            for operation, arguments in operations:
                request = {'token': record['token'], 'operation': operation, 'arguments': arguments}
                connection.sendall(json.dumps(request).encode() + b'\n')
                reply = json.loads(stream.readline())
                if not reply['ok']:
                    raise RuntimeError(reply['error'])
    return reply['result']


def run_after_fitting_mac_desktop(process, discovery, bundle, helper):
    for _ in range(120):
        files = list(discovery.glob('*.json'))
        if files:
            record = json.loads(files[0].read_text(encoding='utf-8'))
            state = bridge(record, ('hello', {}))
            break
        if process.poll() is not None:
            raise RuntimeError(f'GMAT exited with code {process.returncode}')
        time.sleep(.5)
    else:
        raise TimeoutError('GMAT desktop bridge did not start')
    run('open', '-a', str(bundle))
    run(str(helper), 'fit', str(process.pid))
    args = {'document_id': state['document_id'], 'expected_version': state['model_version'],
            'directory': str(discovery.parent/'run'), 'owner': 'desktop'}
    bridge(record, ('attach', {}), ('start_run', args))


def mac_presentation_script(original, path):
    # Cocoa hides GMAT's dynamically populated Window menu. Set the sample's
    # Mars plot placement through its own presentation fields. Keep all
    # mission commands, force models, goal tolerances, and camera fields intact.
    contents = original.read_bytes()
    for field, value in [(b'UpperLeft', b'[ 0.524 0 ]'), (b'Size', b'[ 0.476 1 ]')]:
        pattern = rb'(^OFI_MarsView\.' + field + rb'\s*= )\[[^\]]+\]'
        contents, count = re.subn(pattern, rb'\g<1>' + value, contents, flags=re.MULTILINE)
        if count != 1:
            raise RuntimeError(f'Missing Mars plot presentation field {field!r}')
    path.write_bytes(contents)
    return path


def mission_endpoint(process, discovery, mission_name):
    deadline = time.monotonic() + 240
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError(f'GMAT exited with code {process.returncode}')
        files = list(discovery.glob('*.json'))
        if files:
            try:
                record = json.loads(files[0].read_text(encoding='utf-8'))
                state = bridge(record, ('hello', {}))
            except (ValueError, OSError):
                time.sleep(1)
                continue
            status = state['run'].get('status')
            if status == 'succeeded' and not state['running']:
                if mission_name == MISSION:
                    name, epoch_format, expected_epoch = 'MPS1', 'TAIModJulian', FINAL_MPS1_TAI_MJD
                elif mission_name == 'Ex_StationContactLocator.script':
                    name, epoch_format, expected_epoch = 'GEOSat', 'UTCGregorian', '06 Jan 2015 11:59:28.000'
                else:
                    name, expected_epoch, _ = TARGETING_MISSIONS[mission_name]
                    epoch_format = 'UTCGregorian'
                spacecraft = bridge(record, ('attach', {}), ('get_object', {
                    'document_id': state['document_id'], 'name': name,
                    'view': 'runtime', 'fields': ['Epoch'],
                }))
                context = spacecraft['context']
                epoch = context['Epoch']
                matches = (abs(float(epoch) - expected_epoch) <= 1e-7
                           if epoch_format == 'TAIModJulian' else epoch == expected_epoch)
                if mission_name == 'Ex_MarsBPlane.script':
                    pattern = '%d %b %Y %H:%M:%S.%f'
                    offset = datetime.strptime(epoch, pattern) - datetime.strptime(expected_epoch, pattern)
                    matches = abs(offset.total_seconds()) <= .002
                if not matches or context['DateFormat'] != epoch_format:
                    raise RuntimeError(f'{name} did not reach the expected endpoint: {context}')
                result = {'status': status, 'native_status': state['run']['native_status'],
                          'spacecraft': name, 'epoch_format': epoch_format, 'epoch': epoch}
                if mission_name in TARGETING_MISSIONS:
                    solver_path = Path(state['run']['directory'])/'solvers.json'
                    solvers = json.loads(solver_path.read_text(encoding='utf-8'))
                    if len(solvers) != 2 or any(s['convergence'] != 'converged' for s in solvers):
                        raise RuntimeError(f'{mission_name} did not converge in both targeting stages')
                    if mission_name == 'Ex_MarsBPlane.script':
                        goals = {goal['name']: goal['residual']
                                 for solver in solvers for goal in solver['goals']}
                        expected = {'MAVEN.MarsInertial.BdotT', 'MAVEN.MarsInertial.BdotR', 'MAVEN.Mars.RMAG'}
                        if goals.keys() != expected or any(abs(residual) > .1 for residual in goals.values()):
                            raise RuntimeError(f'Mars goals are outside their script tolerances: {goals}')
                    result['solvers'] = solvers
                return result
            if status in ('failed', 'cancelled'):
                raise RuntimeError(f'GMAT mission {status}')
        time.sleep(1)
    raise TimeoutError(f'{mission_name} did not report a completed run')


def select_workspace_tab(pid, title):
    # This runs under Ubuntu's system Python, which supplies GObject bindings.
    import gi
    gi.require_version('Atspi', '2.0')
    from gi.repository import Atspi

    desktop = Atspi.get_desktop(0)
    apps = (desktop.get_child_at_index(i) for i in range(desktop.get_child_count()))
    app = next((app for app in apps if app and app.get_process_id() == pid), None)
    if app is None:
        raise RuntimeError(f'GMAT process {pid} is missing from the accessibility tree')
    remaining = [app]
    while remaining:
        node = remaining.pop()
        if node.get_role() == Atspi.Role.PAGE_TAB and node.get_name().startswith(title):
            rectangle = node.get_component_iface().get_extents(Atspi.CoordType.SCREEN)
            if rectangle.width > 0 and rectangle.height > 0:
                x = rectangle.x + rectangle.width//2
                y = rectangle.y + rectangle.height//2
                run('xdotool', 'mousemove', str(x), str(y), 'click', '1')
                time.sleep(.25)
                return
        remaining.extend(child for index in range(node.get_child_count())
                         if (child := node.get_child_at_index(index)) is not None)
    raise RuntimeError(f'Workspace tab is missing or hidden: {title}')


def capture(target, process, bundle, destination, ground_track, tabs, mars_targeting, mac_helper):
    if target.startswith('linux'):
        run('pgrep', '-x', 'xfce4-panel')
        run('wmctrl', '-lp')
        run('wmctrl', '-a', 'GMAT')
        run('wmctrl', '-r', 'GMAT', '-b', 'add,maximized_vert,maximized_horz')
        if ground_track:
            time.sleep(1)
            run('xdotool', 'key', 'alt+w', 'sleep', '0.3', 'key', 't')
            # The fixed 1600x1000 Xfce session and fresh settings place the
            # global playback slider at this position. Scrub back into the
            # first few orbits so individual ground tracks remain legible.
            run('xdotool', 'mousemove', '536', '90', 'mousedown', '1',
                'sleep', '0.2', 'mousemove', '435', '90', 'sleep', '0.2', 'mouseup', '1')
        if tabs:
            time.sleep(1)
            for title in tabs:
                select_workspace_tab(process.pid, title)
            if len(tabs) == 2:
                run('xdotool', 'key', 'alt+w', 'sleep', '0.3', 'key', 't')
        width, height = map(int, run('xdotool', 'getdisplaygeometry', capture_output=True).stdout.split())
        run('xdotool', 'mousemove', str(width - 1), str(height - 3))
        time.sleep(1)
        run('import', '-window', 'root', str(destination))
    elif target.startswith('windows'):
        script = Path(__file__).with_name('CaptureAlpha5Windows.ps1').resolve()
        extra = ('-MarsTargeting',) if mars_targeting else ()
        prefix = ('powershell.exe', '-NoProfile', '-File', str(script), '-GmatPid', str(process.pid))
        try:
            run(*prefix, '-Destination', str(destination), *extra)
        except subprocess.CalledProcessError:
            if mars_targeting:
                run(*prefix, '-Destination', str(destination.with_name('diagnostic.png')))
            raise
    else:
        run('pgrep', '-x', 'Dock')
        run('pgrep', '-x', 'SystemUIServer')
        run('open', '-a', str(bundle))
        if mars_targeting:
            run(str(mac_helper), 'solver')
        time.sleep(2)
        run('screencapture', '-x', '-m', str(destination))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('target', choices=ARCHIVE_SHA256)
    parser.add_argument('--archive', type=Path)
    scenes = parser.add_mutually_exclusive_group()
    scenes.add_argument('--ground-track', action='store_true')
    scenes.add_argument('--targeting', action='store_true')
    scenes.add_argument('--mars-targeting', action='store_true')
    args = parser.parse_args()
    target = args.target
    if (args.ground_track or args.targeting) and target != 'linux-x86_64':
        parser.error('The additional compositions use the Linux desktop layout')
    if args.mars_targeting:
        missions = {'Ex_MarsBPlane.script': TARGETING_MISSIONS['Ex_MarsBPlane.script'][2][1:]}
    elif args.targeting:
        missions = {name: details[2] for name, details in TARGETING_MISSIONS.items()}
    elif args.ground_track:
        missions = {'Ex_StationContactLocator.script': (('GroundTrack', ()),)}
    else:
        missions = {MISSION: (('MarsOrbit', ()),)}
    output = Path('desktop-capture').resolve()
    output.mkdir(exist_ok=True)

    with tempfile.TemporaryDirectory(prefix='gmat-desktop-') as directory:
        work = Path(directory)
        mac_helper = None
        if args.mars_targeting:
            if target.startswith('windows'):
                script = Path(__file__).with_name('CaptureAlpha5Windows.ps1').resolve()
                run('powershell.exe', '-NoProfile', '-File', str(script), '-ConfigureHD')
            elif target.startswith('macos'):
                mac_helper = work/'CaptureAlpha5Mac'
                source = Path(__file__).with_name('CaptureAlpha5Mac.swift').resolve()
                run('xcrun', 'swiftc', str(source), '-o', str(mac_helper))
                run(str(mac_helper), 'configure')

        suffix = 'tar.gz' if target.startswith('linux') else 'zip'
        asset = f'GMAT-Qt-{VERSION}-{target}.{suffix}'
        archive = args.archive or work/asset
        if args.archive is None:
            url = f'https://github.com/dcajacob/GMAT-Qt/releases/download/v{VERSION}/{asset}'
            run('curl.exe' if os.name == 'nt' else 'curl', '--fail', '--location', '--silent',
                '--show-error', '--retry', '3', '--output', str(archive), url)
        if digest(archive) != ARCHIVE_SHA256[target]:
            raise RuntimeError(f'Checksum does not match the published archive for {target}')

        payload = unpack(archive, work/'package')
        bundle = payload/'GMAT-Qt.app'
        if target.startswith('linux'):
            executable, samples = payload/'run-gmat', payload/'samples'
        elif target.startswith('windows'):
            executable, samples = payload/'bin/GmatQt.exe', payload/'samples'
        else:
            executable = bundle/'Contents/MacOS/GmatQt'
            samples = bundle/'Contents/Resources/samples'

        environment = os.environ.copy()
        renderer = 'platform default'
        if target.startswith('windows'):
            # The hosted VM lacks an OpenGL-capable display driver. Use the
            # Mesa renderer shipped with Qt, for both Qt and the OSG imports.
            driver = executable.with_name('opengl32.dll')
            shutil.copy2(executable.with_name('opengl32sw.dll'), driver)
            environment.update(QT_OPENGL='desktop', QT_OPENGL_DLL=str(driver))
            renderer = 'bundled Mesa software OpenGL'
        elif target.startswith('linux'):
            renderer = 'Mesa software OpenGL'

        if args.targeting:
            environment['QT_LINUX_ACCESSIBILITY_ALWAYS_ON'] = '1'

        for mission_name, views in missions.items():
            mission = samples/mission_name
            session = work/mission.stem
            session.mkdir()
            discovery = session/'private-discovery'
            input_mission = mac_presentation_script(mission, session/mission.name) if mac_helper else mission
            run_options = [] if mac_helper else ['--run']
            command = [str(executable), '--settings-dir', str(session/'settings'), '--mcp-bridge',
                       '--mcp-discovery-dir', str(discovery), '--convert-views', *run_options, str(input_mission)]
            log_name = f'{mission.stem}.log' if args.targeting else 'gmat.log'
            with (output/log_name).open('w', encoding='utf-8') as log:
                process = subprocess.Popen(command, cwd=session, env=environment, stdout=log,
                                           stderr=subprocess.STDOUT)
                try:
                    if mac_helper:
                        try:
                            run_after_fitting_mac_desktop(process, discovery, bundle, mac_helper)
                        except (RuntimeError, subprocess.CalledProcessError):
                            run('screencapture', '-x', '-m', str(output/'diagnostic.png'))
                            raise
                    endpoint = mission_endpoint(process, discovery, mission_name)
                    for scene, tabs in views:
                        picture = output/f'GMAT-Qt-alpha5-{scene}-{target}.png'
                        capture(target, process, bundle, picture, args.ground_track, tabs,
                                args.mars_targeting, mac_helper)
                        pixels = png_resolution(picture)
                        if args.mars_targeting and (pixels[0] < 1920 or pixels[1] < 1080):
                            raise RuntimeError(f'Native desktop capture is below 1920x1080: {pixels}')
                        view = list(tabs) if tabs else (
                            'Linked playback with OrbitView and GroundTrackPlot' if args.ground_track
                            else 'OFI_MarsMJ2KView converted to Qt OrbitView at Latest')
                        receipt = {
                            'platform': target, 'system': platform.platform(), 'machine': platform.machine(),
                            'archive': asset, 'archive_sha256': ARCHIVE_SHA256[target],
                            'mission': mission_name, 'mission_sha256': digest(mission),
                            'view': view, 'renderer': renderer, 'run': endpoint, 'screenshot': picture.name,
                            'screenshot_sha256': digest(picture), 'pixels': pixels,
                        }
                        if mac_helper:
                            receipt['presentation_changes'] = ['OFI_MarsView.UpperLeft', 'OFI_MarsView.Size']
                            receipt['presented_mission_sha256'] = digest(input_mission)
                        receipt_name = f'{scene}-receipt.json' if args.targeting else 'receipt.json'
                        (output/receipt_name).write_text(json.dumps(receipt, indent=2) + '\n')
                        print(json.dumps(receipt, indent=2), flush=True)
                finally:
                    if process.poll() is None:
                        process.terminate()
                        try:
                            process.wait(timeout=10)
                        except subprocess.TimeoutExpired:
                            process.kill()
                            process.wait()


def png_resolution(path):
    with path.open('rb') as stream:
        header = stream.read(24)
    if header[:8] != b'\x89PNG\r\n\x1a\n':
        raise RuntimeError('Desktop capture did not produce a PNG')
    return struct.unpack('>II', header[16:24])


if __name__ == '__main__':
    main()
