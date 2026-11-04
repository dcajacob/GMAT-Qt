"""An authenticated, local-only Xephyr desktop that the user can watch.

The host compositor is never restarted or reconfigured. Test clients use the
same un-managed X11 geometry as their unattended Xvfb counterparts.
"""
import atexit
import os
from pathlib import Path
import secrets
import select
import shutil
import subprocess
import re
import threading
import ctypes
import ctypes.util
import time

TITLE = 'GMAT GUI acceptance — live test desktop'


def host_windows(environment):
    if not shutil.which('xwininfo'):
        raise RuntimeError('Starting minimized requires xwininfo')
    try:
        result = subprocess.run(['xwininfo', '-root', '-tree'], env=environment,
                                capture_output=True, text=True, timeout=5, check=True)
    except subprocess.CalledProcessError:
        raise RuntimeError('Cannot enumerate host test-desktop windows') from None
    return {int(line.split()[0], 16) for line in result.stdout.splitlines()
            if TITLE in line and '("Xephyr" "Xephyr")' in line}


def iconify(window, environment):
    """Minimize one newly-created host window without changing inner clients."""
    library = ctypes.util.find_library('X11')
    if not library:
        raise RuntimeError('Starting minimized requires libX11')
    x = ctypes.CDLL(library)
    for name, args, result in [
        ('XOpenDisplay', [ctypes.c_char_p], ctypes.c_void_p),
        ('XDefaultScreen', [ctypes.c_void_p], ctypes.c_int),
        ('XIconifyWindow', [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int], ctypes.c_int),
        ('XSync', [ctypes.c_void_p, ctypes.c_int], ctypes.c_int),
        ('XCloseDisplay', [ctypes.c_void_p], ctypes.c_int),
        ('XSetErrorHandler', [ctypes.c_void_p], ctypes.c_void_p),
    ]:
        function = getattr(x, name); function.argtypes = args; function.restype = result
    previous = os.environ.get('XAUTHORITY')
    try:
        if environment.get('XAUTHORITY'):
            os.environ['XAUTHORITY'] = environment['XAUTHORITY']
        else:
            os.environ.pop('XAUTHORITY', None)
        connection = x.XOpenDisplay(environment.get('DISPLAY', '').encode())
    finally:
        if previous is None: os.environ.pop('XAUTHORITY', None)
        else: os.environ['XAUTHORITY'] = previous
    if not connection:
        raise RuntimeError('Cannot access the host display to minimize the test desktop')
    errors = []
    callback = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)(lambda *_: errors.append(True) or 0)
    previous_handler = x.XSetErrorHandler(ctypes.cast(callback, ctypes.c_void_p))
    try:
        accepted = x.XIconifyWindow(connection, window, x.XDefaultScreen(connection))
        x.XSync(connection, False)
        if not accepted or errors:
            raise RuntimeError('The host rejected minimizing the test desktop')
    finally:
        x.XSetErrorHandler(previous_handler)
        x.XCloseDisplay(connection)


def minimize_new_window(environment, previous):
    if not shutil.which('xprop'):
        raise RuntimeError('Starting minimized requires xprop')
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        windows = host_windows(environment) - previous
        if len(windows) == 1:
            window = windows.pop(); iconify(window, environment)
            while time.monotonic() < deadline:
                result = subprocess.run(['xprop', '-id', hex(window), 'WM_STATE', '_NET_WM_STATE'],
                                        env=environment, capture_output=True, text=True, timeout=5, check=True)
                if 'Iconic' in result.stdout or '_NET_WM_STATE_HIDDEN' in result.stdout:
                    return {'host_window': hex(window), 'start_minimized': True,
                            'initial_host_window_state': result.stdout.strip()}
                time.sleep(.05)
            break
        if len(windows) > 1:
            raise RuntimeError('Multiple new test desktops appeared; refusing to minimize an ambiguous window')
        time.sleep(.05)
    raise RuntimeError('The test desktop did not enter the minimized state')


def start(directory, host_environment, *, width=1600, height=1200, resizable=False, start_minimized=False):
    xephyr, xauth = shutil.which('Xephyr'), shutil.which('xauth')
    if not xephyr or not xauth:
        raise RuntimeError('Visible test desktop requires Xephyr and xauth; use --display-route native for the existing desktop, with its geometry limitations.')
    xrandr = shutil.which('xrandr') if resizable else None
    if resizable and not xrandr:
        raise RuntimeError('Resizable test desktop requires xrandr; use --fixed-size if it is unavailable')
    directory = Path(directory)
    directory.mkdir(mode=0o700)
    authority = directory / 'Xauthority'
    authority.touch(mode=0o600)
    cookie = secrets.token_hex(16)

    def authorize(display):
        # Never expose the cookie in logs, receipts or exception command text.
        try:
            result = subprocess.run([xauth, '-f', str(authority), 'source', '-'],
                                    input='add ' + display + ' MIT-MAGIC-COOKIE-1 ' + cookie + '\n',
                                    text=True, capture_output=True, timeout=5)
        except subprocess.TimeoutExpired:
            raise RuntimeError('Private Xephyr authorization timed out') from None
        if result.returncode:
            raise RuntimeError('Could not create private Xephyr authorization')

    # The server loads MIT cookies independently of the display-name field.
    # Add the actual client lookup entry once -displayfd allocates its display.
    authorize(':99')
    read_fd, write_fd = os.pipe()
    log = (directory / 'xephyr.log').open('wb')
    process = None
    stopping = threading.Event()
    monitor = None
    record = {'route': 'nested Xephyr window on existing desktop',
              'host_display': host_environment.get('DISPLAY'),
              'screen': f'{width}x{height}', 'resizable': resizable,
              'observed_screen_sizes': [], 'resize_errors': [],
              'authorization': 'private MIT-MAGIC-COOKIE-1 file; TCP disabled',
              'qualification_limit': 'Visible test display, not physical GPU/host compositor qualification'}
    prior_host_windows = set()

    def stop():
        stopping.set()
        if monitor:
            monitor.join(timeout=6)
        if process is not None and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill(); process.wait()
        log.close()

    try:
        if start_minimized:
            prior_host_windows = host_windows(host_environment)
        command = [xephyr, '-displayfd', str(write_fd), '-screen', f'{width}x{height}',
                   '-nolisten', 'tcp', '-noreset', '-auth', str(authority), '-no-host-grab',
                   '-title', TITLE]
        if resizable:
            command.append('-resizeable')
        process = subprocess.Popen(command,
                                   env=host_environment, stdout=log, stderr=subprocess.STDOUT,
                                   pass_fds=(write_fd,), start_new_session=True)
        os.close(write_fd); write_fd = None
        if not select.select([read_fd], [], [], 10)[0]:
            raise RuntimeError('Xephyr did not become ready; see display/xephyr.log')
        display_number = os.read(read_fd, 64).strip().decode('ascii')
        if not display_number.isdigit():
            raise RuntimeError('Xephyr failed to allocate a display; see display/xephyr.log')
        display = ':' + display_number
        authorize(display)
        environment = dict(host_environment, DISPLAY=display, XAUTHORITY=str(authority))
        probe = subprocess.run(['xdpyinfo', '-display', display], env=environment, capture_output=True, timeout=5)
        if probe.returncode:
            raise RuntimeError('Private visible desktop authorization failed; see display/xephyr.log')
        record.update(test_display=display, server_pid=process.pid)
        if start_minimized:
            record.update(minimize_new_window(host_environment, prior_host_windows))
        if resizable:
            def refresh_screen():
                # The tutorial runner refreshes RandR after a Xephyr resize so
                # Qt sees the new screen bounds. Poll here because CTest owns
                # the GUI event loops and does not call TutorialSession._alive.
                while not stopping.is_set():
                    try:
                        result = subprocess.run([xrandr, '--query'], env=environment,
                                                capture_output=True, text=True, timeout=5)
                        if result.returncode:
                            raise RuntimeError(result.stderr.strip() or 'xrandr failed')
                        found = re.search(r'current (\d+) x (\d+)', result.stdout)
                        if found:
                            size = [int(value) for value in found.groups()]
                            if not record['observed_screen_sizes'] or record['observed_screen_sizes'][-1] != size:
                                record['observed_screen_sizes'].append(size)
                    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
                        record['resize_errors'].append(str(error))
                        break
                    stopping.wait(.25)
            monitor = threading.Thread(target=refresh_screen, daemon=True)
            monitor.start()
    except BaseException:
        stop()
        raise
    finally:
        os.close(read_fd)
        if write_fd is not None:
            os.close(write_fd)
    atexit.register(stop)
    return environment, record, stop
