"""Capture bundled missions in the published GMAT Qt alpha.5 GUI."""

import argparse
import base64
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
GROUND_TRACK_MISSION = 'Ex_StationContactLocator.script'
GROUND_TRACK_TABS = ('OFI_ThreeDView', 'GroundTrackPlot')
GROUND_TRACK_PLAYBACK = 38
PLAYBACK_CHECKPOINTS = {'GroundTrack': GROUND_TRACK_PLAYBACK, 'MarsOrbit': 1000}


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


def mac_presentation_script(original, path, placements, weather_file=None):
    # Cocoa hides GMAT's dynamically populated Window menu. Arrange the
    # sample's plots using their presentation fields before launching.
    contents = original.read_bytes()
    for field, value in placements.items():
        pattern = rb'(^' + re.escape(field.encode()) + rb'\s*= )\[[^\]]+\]'
        contents, count = re.subn(pattern, rb'\g<1>' + value.encode(), contents, flags=re.MULTILINE)
        if count != 1:
            raise RuntimeError(f'Missing presentation field {field!r}')
    if weather_file is not None:
        relative = b'../samples/SupportFiles/CSSI_2004To2026.txt'
        if not weather_file.is_file() or contents.count(relative) != 1:
            raise RuntimeError('Expected the packaged lunar space-weather input')
        contents = contents.replace(relative, str(weather_file.resolve()).encode(), 1)
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
                elif mission_name == GROUND_TRACK_MISSION:
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
                if mission_name in TARGETING_MISSIONS:
                    pattern = '%d %b %Y %H:%M:%S.%f'
                    offset = datetime.strptime(epoch, pattern) - datetime.strptime(expected_epoch, pattern)
                    # The lunar event stops and finite-difference targeter
                    # shift the final UTC slightly across native builds.
                    tolerance = .05 if mission_name == 'Ex_LunarTransfer.script' else .002
                    matches = abs(offset.total_seconds()) <= tolerance
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
                log = Path(state['run']['directory'])/'engine.log'
                if log.is_file():
                    lines = log.read_text(encoding='utf-8', errors='replace').splitlines()
                    print('\n'.join(lines[-45:]), flush=True)
                # The GUI appends some native messages through queued signals.
                # Read the live message pane after its queue has drained.
                time.sleep(1)
                messages = bridge(record, ('attach', {}), ('logs', {'limit': 65536}))
                recent = base64.b64decode(messages['content']).decode('utf-8', errors='replace')
                print('\n'.join(recent.splitlines()[-35:]), flush=True)
                raise RuntimeError(f'GMAT mission {status}')
        time.sleep(1)
    raise TimeoutError(f'{mission_name} did not report a completed run')


def find_linux_control(pid, title, role):
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
        if node.get_role() == getattr(Atspi.Role, role) and node.get_name().startswith(title):
            rectangle = node.get_component_iface().get_extents(Atspi.CoordType.SCREEN)
            if rectangle.width > 0 and rectangle.height > 0:
                return node, rectangle
        remaining.extend(child for index in range(node.get_child_count())
                         if (child := node.get_child_at_index(index)) is not None)
    raise RuntimeError(f'Control is missing or hidden: {title}')


def select_workspace_tab(pid, title):
    _, rectangle = find_linux_control(pid, title, 'PAGE_TAB')
    x, y = rectangle.x + rectangle.width//2, rectangle.y + rectangle.height//2
    run('xdotool', 'mousemove', str(x), str(y), 'click', '1')
    time.sleep(.25)


def set_linux_playback(pid, value):
    node, _ = find_linux_control(pid, 'All plots playback position', 'SLIDER')
    slider = node.get_value_iface()
    # The unlinked slider initially shows 1000. Change it first so setting
    # Latest emits the value change that links the views.
    if slider.get_current_value() == value:
        slider.set_current_value(0)
    if not slider.set_current_value(value) or slider.get_current_value() != value:
        raise RuntimeError(f'Global playback slider did not settle at {value}')


def capture(target, process, bundle, destination, ground_track, tabs, native_scene, mac_helper):
    if target.startswith('linux'):
        run('pgrep', '-x', 'xfce4-panel')
        run('wmctrl', '-lp')
        run('wmctrl', '-a', 'GMAT')
        run('wmctrl', '-r', 'GMAT', '-b', 'add,maximized_vert,maximized_horz')
        if ground_track and not native_scene:
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
        if native_scene in PLAYBACK_CHECKPOINTS:
            set_linux_playback(process.pid, PLAYBACK_CHECKPOINTS[native_scene])
        width, height = map(int, run('xdotool', 'getdisplaygeometry', capture_output=True).stdout.split())
        run('xdotool', 'mousemove', str(width - 1), str(height - 3))
        time.sleep(1)
        run('import', '-window', 'root', str(destination))
    elif target.startswith('windows'):
        script = Path(__file__).with_name('CaptureAlpha5Windows.ps1').resolve()
        switches = {'MarsCaptureTargeting': '-MarsTargeting', 'MarsTransfer': '-MarsTransfer',
                    'LunarInsertionTargeting': '-LunarTargeting', 'GroundTrack': '-GroundTrack',
                    'MarsOrbit': '-MarsOrbit'}
        extra = (switches[native_scene],) if native_scene else ()
        if native_scene in PLAYBACK_CHECKPOINTS:
            extra += ('-PlaybackPosition', str(PLAYBACK_CHECKPOINTS[native_scene]))
        prefix = ('powershell.exe', '-NoProfile', '-File', str(script), '-GmatPid', str(process.pid))
        try:
            run(*prefix, '-Destination', str(destination), *extra)
        except subprocess.CalledProcessError:
            if native_scene:
                run(*prefix, '-Destination', str(destination.with_name('diagnostic.png')))
            raise
    else:
        run('pgrep', '-x', 'Dock')
        run('pgrep', '-x', 'SystemUIServer')
        run('open', '-a', str(bundle))
        if native_scene:
            if native_scene == 'MarsTransfer':
                run(str(mac_helper), 'transfer', str(process.pid))
            elif native_scene in PLAYBACK_CHECKPOINTS:
                try:
                    if native_scene == 'MarsOrbit':
                        run(str(mac_helper), 'mars-orbit', str(process.pid))
                    run(str(mac_helper), 'playback', str(process.pid), str(PLAYBACK_CHECKPOINTS[native_scene]))
                except subprocess.CalledProcessError:
                    run('screencapture', '-x', '-m', str(destination.with_name('diagnostic.png')))
                    raise
            else:
                action = 'lunar-solver' if native_scene == 'LunarInsertionTargeting' else 'solver'
                run(str(mac_helper), action)
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
    scenes.add_argument('--mars-transfer', action='store_true')
    scenes.add_argument('--lunar-targeting', action='store_true')
    scenes.add_argument('--ground-track-hd', action='store_true')
    scenes.add_argument('--mars-orbit-hd', action='store_true')
    args = parser.parse_args()
    target = args.target
    if (args.ground_track or args.targeting) and target != 'linux-x86_64':
        parser.error('The additional compositions use the Linux desktop layout')
    native_hd = (args.mars_targeting or args.mars_transfer or args.lunar_targeting
                 or args.ground_track_hd or args.mars_orbit_hd)
    if args.mars_orbit_hd:
        missions = {MISSION: (('MarsOrbit', ('OFI_MarsMJ2KView',)),)}
    elif args.ground_track_hd:
        missions = {GROUND_TRACK_MISSION: (('GroundTrack', GROUND_TRACK_TABS),)}
    elif args.lunar_targeting:
        lunar = 'Ex_LunarTransfer.script'
        missions = {lunar: TARGETING_MISSIONS[lunar][2]}
    elif native_hd:
        view_index = 0 if args.mars_transfer else 1
        missions = {'Ex_MarsBPlane.script': (TARGETING_MISSIONS['Ex_MarsBPlane.script'][2][view_index],)}
    elif args.targeting:
        missions = {name: details[2] for name, details in TARGETING_MISSIONS.items()}
    elif args.ground_track:
        missions = {GROUND_TRACK_MISSION: (('GroundTrack', ()),)}
    else:
        missions = {MISSION: (('MarsOrbit', ()),)}
    output = Path('desktop-capture').resolve()
    output.mkdir(exist_ok=True)

    with tempfile.TemporaryDirectory(prefix='gmat-desktop-') as directory:
        work = Path(directory)
        mac_helper = None
        if native_hd:
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

        if target.startswith('linux') and (args.targeting or native_hd):
            environment['QT_LINUX_ACCESSIBILITY_ALWAYS_ON'] = '1'

        for mission_name, views in missions.items():
            mission = samples/mission_name
            session = work/mission.stem
            session.mkdir()
            discovery = session/'private-discovery'
            placements = {}
            if mac_helper and args.ground_track_hd:
                placements = {
                    'GroundTrackPlot.UpperLeft': '[ 0 0 ]',
                    'GroundTrackPlot.Size': '[ 0.499 0.985 ]',
                    'OFI_ThreeDView.UpperLeft': '[ 0.5 0 ]',
                    'OFI_ThreeDView.Size': '[ 0.499 0.985 ]',
                }
            elif mac_helper and (args.mars_targeting or args.lunar_targeting):
                plot = views[0][1][0]
                placements = {f'{plot}.UpperLeft': '[ 0.524 0 ]', f'{plot}.Size': '[ 0.476 1 ]'}
            # The macOS bundle stores the sample data under Resources.
            # Resolve the bundled file explicitly in the temporary script.
            weather_file = (samples/'SupportFiles/CSSI_2004To2026.txt'
                            if mac_helper and args.lunar_targeting else None)
            input_mission = (mac_presentation_script(mission, session/mission.name, placements, weather_file)
                             if placements else mission)
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
                    try:
                        endpoint = mission_endpoint(process, discovery, mission_name)
                    except RuntimeError:
                        if mac_helper:
                            run('screencapture', '-x', '-m', str(output/'diagnostic.png'))
                        raise
                    for scene, tabs in views:
                        picture = output/f'GMAT-Qt-alpha5-{scene}-{target}.png'
                        capture(target, process, bundle, picture, args.ground_track, tabs,
                                scene if native_hd else None, mac_helper)
                        pixels = png_resolution(picture)
                        if native_hd and (pixels[0] < 1920 or pixels[1] < 1080):
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
                        if input_mission != mission:
                            receipt['presentation_changes'] = list(placements)
                            receipt['presented_mission_sha256'] = digest(input_mission)
                        if native_hd and scene in PLAYBACK_CHECKPOINTS:
                            receipt['playback'] = {'mode': 'interpolated steps', 'position': PLAYBACK_CHECKPOINTS[scene],
                                                   'range_maximum': 1000, 'paused': True}
                        if weather_file:
                            receipt['resolved_packaged_input'] = {'file': weather_file.name, 'sha256': digest(weather_file)}
                        receipt_name = f'{scene}-receipt.json' if args.targeting else 'receipt.json'
                        (output/receipt_name).write_text(json.dumps(receipt, indent=2) + '\n')
                        print(json.dumps(receipt, indent=2), flush=True)
                finally:
                    engine_log = session/'run/engine.log'
                    if engine_log.is_file():
                        shutil.copyfile(engine_log, output/f'{mission.stem}-engine.log')
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
