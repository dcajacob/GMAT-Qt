#!/usr/bin/env python3
"""Watch the existing widget-focused tests on your current X11/XWayland desktop.

This is in-process Qt interaction, not an OS mouse/keyboard driver. It does not
run the example corpus, replay tutorials, or exercise the 3D/lifecycle suite.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pause-ms', type=int, default=1500, help='Viewing delay per observable step (0 disables pauses; maximum 5000)')
    parser.add_argument('--output', type=Path, help='New result directory (must not exist)')
    args = parser.parse_args()
    if not 0 <= args.pause_ms <= 5000:
        parser.error('--pause-ms must be between 0 and 5000')
    repo = Path(__file__).resolve().parents[3]
    binary = repo / 'build/linux-gui/src/qtgui/GmatQtDesktopTests'
    startup = repo / 'application/bin/gmat_startup_qt.txt'
    if not binary.is_file() or not startup.is_file():
        parser.error('Build the selected target first: cmake --build ' + str(repo / 'build/linux-gui') + ' --target GmatQtDesktopTests --parallel 2')
    display = os.environ.get('DISPLAY')
    if not display:
        parser.error('Run from a terminal in your logged-in desktop; DISPLAY is absent. No private display will be started.')
    try:
        probe = subprocess.run(['xdpyinfo', '-display', display], capture_output=True, text=True, timeout=5)
    except (OSError, subprocess.TimeoutExpired) as error:
        parser.error('Cannot check the current desktop display with xdpyinfo: ' + str(error))
    if probe.returncode:
        parser.error('Cannot access your current display ' + display + ': ' + probe.stderr.strip())
    out = args.output.absolute() if args.output else Path(tempfile.mkdtemp(prefix='gmat-qt-watch-'))
    if args.output:
        out.mkdir(parents=True, exist_ok=False)
    (out / 'output').mkdir()
    text = startup.read_text()
    replacements = {'ROOT_PATH': str(repo / 'application') + '/',
                    'OUTPUT_PATH': str(out / 'output') + '/',
                    'LOG_FILE': str(out / 'output/GmatLog.txt'),
                    'PERSONALIZATION_FILE': str(out / 'MyGmat.ini')}
    for key, value in replacements.items():
        text = re.sub(r'^' + key + r'\s*=.*$', key + ' = ' + value, text, flags=re.M)
    # Make literal relative paths independent of the private startup location.
    def absolute_literal(match):
        prefix, value = match.group(1), match.group(2).strip()
        if value.startswith(('../', './')):
            value = str((startup.parent / value).resolve())
        return prefix + value
    text = re.sub(r'^(\w+\s*=\s*)([^\n]*)$', absolute_literal, text, flags=re.M)
    selected = out / 'startup.txt'
    selected.write_text(text)
    environment = dict(os.environ, QT_QPA_PLATFORM='xcb', GMAT_QT_WATCH_MS=str(args.pause_ms), PYTHONDONTWRITEBYTECODE='1')
    command = [str(binary), str(selected)]
    receipt = {'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'command': command,
               'display': display, 'qpa': 'xcb', 'display_route': 'existing desktop DISPLAY; no Xvfb or nested display',
               'pause_ms': args.pause_ms, 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
               'startup_original_sha256': hashlib.sha256(startup.read_bytes()).hexdigest(),
               'limits': 'Qt signal/button/widget events and frontend methods; no OS input injection; no full tutorial/corpus/3D/compositor qualification'}
    print('Visible tests on DISPLAY=' + display + ' through X11/XWayland. Keep this desktop visible.', flush=True)
    print('Results: ' + str(out), flush=True)
    start = time.monotonic()
    with (out / 'test.log').open('wb') as log:
        process = subprocess.Popen(command, cwd=repo / 'application/bin', env=environment, stdout=log, stderr=subprocess.STDOUT)
        receipt['pid'] = process.pid
        (out / 'result.json').write_text(json.dumps(receipt, indent=2) + '\n')
        timeout = max(180, args.pause_ms * 80 / 1000 + 30)
        try:
            code = process.wait(timeout=timeout)
        except (subprocess.TimeoutExpired, KeyboardInterrupt):
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill(); process.wait()
            code = 124
    receipt.update(exit=code, duration_seconds=round(time.monotonic() - start, 3))
    (out / 'result.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print((out / 'test.log').read_text(errors='replace'), end='')
    print(('PASS' if code == 0 else 'FAIL') + ': exit ' + str(code) + '; results ' + str(out), flush=True)
    return code if code >= 0 else 128 - code


if __name__ == '__main__':
    raise SystemExit(main())
