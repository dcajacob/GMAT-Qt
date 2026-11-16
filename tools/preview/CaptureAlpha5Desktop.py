"""Capture the published alpha.5 GUI at the end of the stock Mars mission."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
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


def mission_endpoint(process, discovery, ground_track=False):
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
                name = 'GEOSat' if ground_track else 'MPS1'
                spacecraft = bridge(record, ('attach', {}), ('get_object', {
                    'document_id': state['document_id'], 'name': name,
                    'view': 'runtime', 'fields': ['Epoch'],
                }))
                context = spacecraft['context']
                if ground_track:
                    matches = (context['DateFormat'] == 'UTCGregorian' and
                               context['Epoch'] == '06 Jan 2015 11:59:28.000')
                else:
                    matches = (context['DateFormat'] == 'TAIModJulian' and
                               abs(float(context['Epoch']) - FINAL_MPS1_TAI_MJD) <= 1e-7)
                if not matches:
                    raise RuntimeError(f'{name} did not reach the expected endpoint: {context}')
                return {'status': status, 'native_status': state['run']['native_status'],
                        'spacecraft': name, 'epoch_format': context['DateFormat'], 'epoch': context['Epoch']}
            if status in ('failed', 'cancelled'):
                raise RuntimeError(f'GMAT mission {status}')
        time.sleep(1)
    raise TimeoutError('Mars mission did not report a completed run')


def capture(target, process, bundle, destination, ground_track):
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
        run('xdotool', 'mousemove', '1599', '997')
        time.sleep(1)
        run('import', '-window', 'root', str(destination))
    elif target.startswith('windows'):
        script = Path(__file__).with_name('CaptureAlpha5Windows.ps1').resolve()
        run('pwsh', '-NoProfile', '-File', str(script), '-GmatPid', str(process.pid),
            '-Destination', str(destination))
    else:
        run('pgrep', '-x', 'Dock')
        run('pgrep', '-x', 'SystemUIServer')
        run('open', '-a', str(bundle))
        time.sleep(2)
        run('screencapture', '-x', '-m', str(destination))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('target', choices=ARCHIVE_SHA256)
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--ground-track', action='store_true')
    args = parser.parse_args()
    target = args.target
    if args.ground_track and target != 'linux-x86_64':
        parser.error('The optional ground-track composition uses the Linux desktop layout')
    mission_name = 'Ex_StationContactLocator.script' if args.ground_track else MISSION
    scene = 'GroundTrack' if args.ground_track else 'MarsOrbit'
    output = Path('desktop-capture').resolve()
    output.mkdir(exist_ok=True)

    with tempfile.TemporaryDirectory(prefix='gmat-desktop-') as directory:
        work = Path(directory)
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

        discovery = work/'private-discovery'
        mission = samples/mission_name
        command = [str(executable), '--settings-dir', str(work/'settings'), '--mcp-bridge',
                   '--mcp-discovery-dir', str(discovery), '--convert-views', '--run', str(mission)]
        with (output/'gmat.log').open('w', encoding='utf-8') as log:
            process = subprocess.Popen(command, cwd=work, env=environment, stdout=log,
                                       stderr=subprocess.STDOUT)
            try:
                endpoint = mission_endpoint(process, discovery, args.ground_track)
                picture = output/f'GMAT-Qt-alpha5-{scene}-{target}.png'
                capture(target, process, bundle, picture, args.ground_track)
                with picture.open('rb') as stream:
                    header = stream.read(24)
                if header[:8] != b'\x89PNG\r\n\x1a\n':
                    raise RuntimeError('Desktop capture did not produce a PNG')
                width, height = struct.unpack('>II', header[16:24])
                receipt = {
                    'platform': target, 'system': platform.platform(), 'machine': platform.machine(),
                    'archive': asset, 'archive_sha256': ARCHIVE_SHA256[target],
                    'mission': mission_name, 'mission_sha256': digest(mission),
                    'view': ('Linked playback with OrbitView and GroundTrackPlot' if args.ground_track
                             else 'OFI_MarsMJ2KView converted to Qt OrbitView at Latest'),
                    'renderer': renderer,
                    'run': endpoint, 'screenshot': picture.name, 'screenshot_sha256': digest(picture),
                    'pixels': [width, height],
                }
                (output/'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
                print(json.dumps(receipt, indent=2), flush=True)
            finally:
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()


if __name__ == '__main__':
    main()
