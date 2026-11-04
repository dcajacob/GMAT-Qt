#!/usr/bin/env python3
"""Bounded real-input GmatQt session on an owned, device-free Wayland compositor.

The installed bwrap, GNOME Shell, PipeWire, Mesa EGL, Qt Wayland and /usr/bin/python3 Gio
are required. No compositor, host bus/display/socket, physical GPU or input
fallback is allowed. This qualifies private software Wayland only, not the
user's running GNOME desktop, portals or hardware drivers.

CLI: --app /path/application/bin/GmatQt --startup /path/application/bin/gmat_startup_qt.txt
     --artifacts /tmp/evidence [--seconds 180]
Optional --portal-theme-file accepts only the SHA-pinned Ubuntu Qt6.10.2 theme
extracted during preparation. It binds one file read-only, selects the theme
only for GmatQt, and traces FileChooser requests/Response on the owned bus.
One JSON action per stdin line: screenshot(label), windows, state, move(x,y),
click(x,y,button=1,count=1), button(button,down), key(keys="CTRL+O"), text(text),
focus(window), wait(ms<=2000), exit_application, quit. exit_application sends
Alt+F4 only to the sole focused owned main window, waits at most five seconds
for code 0, records other process status, and ends the worker normally. Input enters Mutter's virtual input seat,
then Qt's Wayland event path; no Qt test hooks are used. Import this class for
adaptive use. sandbox_path() maps artifact paths to the private /evidence mount.
"""
from __future__ import annotations
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import select
import shutil
import signal
import subprocess
import sys
import tempfile
import time


# Ubuntu resolute qt6-xdgdesktopportal-platformtheme6.10.2+dfsg-7, amd64.
# Package origin, license and complete preparation hashes are retained separately.
PORTAL_THEME_SHA256 = '8abfe7b383ccecc39ae357bc05a98c35b92b483e05a147582f23bca2622cd7b8'
PORTAL_THEME_MOUNT = '/portal-plugins/platformthemes/libqxdgdesktopportal.so'
WORKER_FRAME = b'GMAT_PRIVATE_WAYLAND_V1 '


def worker_reply(value):
    # Library stdout is not a protocol response. Keep one explicit frame per reply.
    sys.stdout.buffer.write(WORKER_FRAME + json.dumps(value).encode('utf-8') + b'\n')
    sys.stdout.buffer.flush()


def sha(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''): h.update(block)
    return h.hexdigest()


def identity(path):
    path = Path(path).resolve()
    before = path.stat(); digest = sha(path); after = path.stat()
    fields = ('st_dev', 'st_ino', 'st_size', 'st_mtime_ns')
    if any(getattr(before, f) != getattr(after, f) for f in fields):
        raise RuntimeError('Launch file changed while recording identity: ' + str(path))
    return {'path': str(path), 'sha256': digest, 'device': after.st_dev, 'inode': after.st_ino,
            'size': after.st_size, 'mtime_ns': after.st_mtime_ns}


def stop_group(child):
    if child is None or child.poll() is not None: return
    try: os.killpg(child.pid, signal.SIGTERM)
    except ProcessLookupError: return
    try: child.wait(timeout=2)
    except subprocess.TimeoutExpired:
        try: os.killpg(child.pid, signal.SIGKILL)
        except ProcessLookupError: pass
        child.wait(timeout=2)


def host_shells():
    result = []
    for p in Path('/proc').glob('[0-9]*'):
        try:
            if (p / 'comm').read_text().strip() != 'gnome-shell': continue
            stat = (p / 'stat').read_text()
            result.append({'pid': int(p.name), 'start_ticks': stat[stat.rfind(')') + 2:].split()[19]})
        except (OSError, ValueError, IndexError): continue
    return sorted(result, key=lambda x: x['pid'])


class IsolatedWaylandSession:
    def __init__(self, app, startup, artifacts, *, seconds=180, width=1600, height=1200, script=None, portal_theme_file=None):
        if not 10 <= seconds <= 600: raise ValueError('Session bound must be 10..600 seconds')
        if not (640 <= width <= 2560 and 480 <= height <= 1600): raise ValueError('Screen size outside bounded range')
        self.app, self.startup = Path(app).resolve(), Path(startup).resolve()
        if not self.app.is_file() or not os.access(self.app, os.X_OK) or not self.startup.is_file():
            raise ValueError('Existing application executable and startup file required')
        self.script = Path(script).resolve() if script else None
        if self.script and not self.script.is_file(): raise ValueError('Existing optional fixture required')
        self.portal_theme_file = Path(portal_theme_file).resolve() if portal_theme_file else None
        self.portal_identity = None
        if self.portal_theme_file:
            if not self.portal_theme_file.is_file(): raise ValueError('Existing prepared portal theme file required')
            self.portal_identity = identity(self.portal_theme_file)
            if self.portal_identity['sha256'] != PORTAL_THEME_SHA256:
                raise ValueError('Portal theme differs from the prepared matching Ubuntu Qt6.10.2 plugin')
        self.application = self.startup.parent.parent
        if not self.app.is_relative_to(self.application): raise ValueError('App must be in the startup application directory')
        root = Path(artifacts).resolve(); root.mkdir(parents=True, exist_ok=True)
        self.directory = Path(tempfile.mkdtemp(prefix='isolated-wayland-', dir=root))
        os.chmod(self.directory, 0o700)
        for name in ('runtime', 'home', 'config', 'cache', 'data', 'settings', 'output', 'screenshots', 'etc'):
            (self.directory / name).mkdir(mode=0o700)
        self.seconds, self.width, self.height = seconds, width, height
        self.deadline = time.monotonic() + seconds
        self.process = None; self.log = None; self.closed = False; self.buffer = b''
        self.request_id = 0; self.active_request = None; self.stdout_bytes = 0
        self.before = host_shells()
        fixture = {'kind': 'application-generated default mission', 'sha256': None,
                   'limit': 'Source is generated at launch; record GUI-saved source with the fixture action'}
        if self.script:
            original_fixture = identity(self.script)
            (self.directory / 'fixture.script').write_bytes(self.script.read_bytes())
            fixture = {'kind': 'explicit existing fixture', 'original': original_fixture,
                       'sha256': sha(self.directory / 'fixture.script')}
            if fixture['sha256'] != original_fixture['sha256']: raise RuntimeError('Fixture changed while copying')
        original = self.startup.read_bytes()
        matches = list(re.finditer(rb'(?m)^([ \t]*OUTPUT_PATH[ \t]*=[ \t]*)([^\r\n]*)(\r?)$', original))
        log = re.search(rb'(?m)^[ \t]*LOG_FILE[ \t]*=[ \t]*([^\r\n]*)', original)
        if len(matches) != 1 or not log or not log.group(1).strip().startswith(b'OUTPUT_PATH/'):
            raise ValueError('Startup must contain one OUTPUT_PATH and LOG_FILE using OUTPUT_PATH')
        match = matches[0]
        clone = original[:match.start(2)] + b'/evidence/output/' + original[match.end(2):]
        (self.directory / 'original-startup.txt').write_bytes(original)
        (self.directory / 'gmat_startup_isolated.txt').write_bytes(clone)
        uid, gid = os.getuid(), os.getgid()
        (self.directory / 'etc/passwd').write_text(f'private:x:{uid}:{gid}:Private Wayland:/evidence/home:/usr/bin/false\n')
        (self.directory / 'etc/group').write_text(f'private:x:{gid}:\n')
        (self.directory / 'etc/nsswitch.conf').write_text('passwd: files\ngroup: files\nhosts: files\n')
        (self.directory / 'etc/hosts').write_text('127.0.0.1 localhost\n::1 localhost\n')
        (self.directory / 'etc/machine-id').write_text(os.urandom(16).hex() + '\n')
        self.record = {'app': str(self.app), 'app_sha256': sha(self.app), 'startup': str(self.startup),
                       'original_startup_sha256': hashlib.sha256(original).hexdigest(),
                       'clone_startup_sha256': hashlib.sha256(clone).hexdigest(), 'startup_change': 'OUTPUT_PATH value only',
                       'fixture': fixture, 'artifacts': str(self.directory), 'seconds': seconds, 'screen': [width, height],
                       'host_gnome_before': self.before, 'limits': 'Owned private software Wayland only; host GNOME, portals and hardware drivers unqualified'}
        if self.portal_identity:
            self.record['portal_theme_opt_in'] = self.portal_identity
            self.record['limits'] = 'Owned private software Wayland only; host GNOME, host portals and hardware drivers unqualified; trace must establish private FileChooser use'
        self._record()

    def _record(self):
        (self.directory / 'session.json').write_text(json.dumps(self.record, indent=2) + '\n')

    def sandbox_path(self, path):
        path = Path(path).resolve()
        if path.is_relative_to(self.directory): return '/evidence/' + str(path.relative_to(self.directory))
        if path.is_relative_to(self.application): return str(path)
        raise ValueError('Path must be a session artifact or read-only application file')

    def _bindings(self):
        # ldd identifies this trusted application's actual external runtime libs.
        # Bind only their library directories, never their containing home/profile.
        result = subprocess.run(['/usr/bin/ldd', str(self.app)], env={'PATH': '/usr/bin', 'LC_ALL': 'C'},
                                stdin=subprocess.DEVNULL, capture_output=True, text=True, close_fds=True, timeout=5)
        (self.directory / 'app-ldd.txt').write_text(result.stdout + result.stderr)
        if result.returncode or 'not found' in result.stdout: raise RuntimeError('Application dependency missing; see app-ldd.txt')
        external = set()
        for match in re.finditer(r'=> (/\S+) \(', result.stdout):
            p = Path(match.group(1)).resolve()
            if p.is_relative_to('/usr') or p.is_relative_to(self.application): continue
            if p.parent.name not in ('lib', 'lib64'): raise RuntimeError('Unexpected external library path: ' + str(p))
            external.add(str(p.parent))
        return sorted(external)

    def _sandbox(self):
        for executable in ('bwrap', 'gnome-shell', 'pipewire', 'dbus-run-session'):
            if not Path('/usr/bin/' + executable).is_file(): raise RuntimeError('Required installed executable missing: ' + executable)
        if self.portal_theme_file and not Path('/usr/bin/dbus-monitor').is_file():
            raise RuntimeError('Portal opt-in requires installed dbus-monitor')
        mesa = '/usr/share/glvnd/egl_vendor.d/50_mesa.json'
        if not Path(mesa).is_file(): raise RuntimeError('Installed Mesa EGL vendor file missing')
        argv = ['/usr/bin/bwrap', '--unshare-all', '--die-with-parent', '--new-session', '--clearenv',
                '--ro-bind', '/usr', '/usr', '--proc', '/proc', '--dev', '/dev', '--dir', '/run',
                '--tmpfs', '/tmp', '--dir', '/etc', '--dir', '/var', '--dir', '/var/cache',
                '--dir', '/home', '--dir', '/harness', '--bind', str(self.directory), '/evidence',
                '--ro-bind', str(self.application), str(self.application),
                '--ro-bind', str(Path(__file__).resolve()), '/harness/IsolatedWaylandSession.py']
        if self.portal_theme_file:
            argv += ['--dir', '/portal-plugins', '--dir', '/portal-plugins/platformthemes',
                     '--ro-bind', str(self.portal_theme_file), PORTAL_THEME_MOUNT]
        for alias in ('bin', 'sbin', 'lib', 'lib64'):
            path = Path('/' + alias)
            if path.is_symlink(): argv += ['--symlink', os.readlink(path), '/' + alias]
            elif path.is_dir(): argv += ['--ro-bind', str(path), str(path)]
        for name in ('passwd', 'group', 'nsswitch.conf', 'hosts', 'machine-id'):
            argv += ['--ro-bind', str(self.directory / 'etc' / name), '/etc/' + name]
        for path in ('/etc/fonts', '/etc/ld.so.cache', '/etc/os-release', '/etc/mime.types', '/etc/gtk-3.0', '/var/cache/fontconfig'):
            if Path(path).exists(): argv += ['--ro-bind', path, path]
        external = self._bindings()
        for path in external: argv += ['--ro-bind', path, path]
        env = {'PATH': '/usr/bin', 'LANG': 'C.UTF-8', 'LC_ALL': 'C.UTF-8', 'HOME': '/evidence/home',
               'XDG_RUNTIME_DIR': '/evidence/runtime', 'XDG_CONFIG_HOME': '/evidence/config',
               'XDG_CACHE_HOME': '/evidence/cache', 'XDG_DATA_HOME': '/evidence/data',
               'XDG_DATA_DIRS': '/usr/share', 'XDG_CURRENT_DESKTOP': 'GNOME', 'XDG_SESSION_TYPE': 'wayland',
               'XDG_SESSION_DESKTOP': 'GMATPrivateWayland', 'GSETTINGS_BACKEND': 'memory',
               'DBUS_SYSTEM_BUS_ADDRESS': 'unix:path=/run/no-system-bus', 'NO_AT_BRIDGE': '1', 'QT_ACCESSIBILITY': '0',
               'LIBGL_ALWAYS_SOFTWARE': '1', 'GALLIUM_DRIVER': 'llvmpipe', 'MESA_LOADER_DRIVER_OVERRIDE': 'swrast',
               '__EGL_VENDOR_LIBRARY_FILENAMES': mesa, 'MUTTER_DEBUG_DISABLE_HW_CURSORS': '1'}
        for key, value in env.items(): argv += ['--setenv', key, value]
        self.record.update(readonly_external_library_binds=external, sandbox_environment=env,
                           physical_devices_bound=False, host_runtime_bound=False, network_namespace='private')
        config = {'app': str(self.app), 'seconds': self.seconds, 'width': self.width, 'height': self.height,
                  'fixture': '/evidence/fixture.script' if self.script else None}
        if self.portal_theme_file: config['portal_theme_file'] = PORTAL_THEME_MOUNT
        (self.directory / 'worker-config.json').write_text(json.dumps(config) + '\n')
        argv += ['--chdir', str(self.app.parent), '--', '/usr/bin/dbus-run-session', '--',
                 '/usr/bin/python3', '-u', '/harness/IsolatedWaylandSession.py', '--worker', '/evidence/worker-config.json']
        self.record['sandbox_argv'] = argv; self._record()
        return argv

    def _protocol_failure(self, message):
        detail = {'request': self.active_request, 'error': message}
        with (self.directory / 'protocol-errors.jsonl').open('a') as log:
            log.write(json.dumps(detail) + '\n')
        self.record['failure'] = message; self._record()
        raise RuntimeError(message)

    def _receive(self, seconds):
        end = min(self.deadline, time.monotonic() + seconds)
        while time.monotonic() < end:
            if b'\n' in self.buffer:
                line, self.buffer = self.buffer.split(b'\n', 1)
                if not line.startswith(WORKER_FRAME):
                    with (self.directory / 'worker-stdout-noise.log').open('ab') as log:
                        log.write(line + b'\n')
                    continue
                try: value = json.loads(line[len(WORKER_FRAME):])
                except (ValueError, UnicodeDecodeError) as error:
                    self._protocol_failure('Malformed private worker frame: ' + str(error))
                if not isinstance(value, dict):
                    self._protocol_failure('Private worker frame must contain a JSON object')
                return value
            if self.process.poll() is not None: raise RuntimeError(f'Private sandbox exited ({self.process.returncode}); see sandbox.log')
            ready, _, _ = select.select([self.process.stdout], [], [], min(.2, end - time.monotonic()))
            if ready:
                chunk = os.read(self.process.stdout.fileno(), 65536)
                if not chunk: raise RuntimeError('Private worker closed its output; see sandbox.log')
                self.stdout_bytes += len(chunk)
                if self.stdout_bytes > 8 * 1024 * 1024:
                    self._protocol_failure('Private worker stdout exceeds session bound')
                with (self.directory / 'worker-stdout.log').open('ab') as log: log.write(chunk)
                self.buffer += chunk
                if len(self.buffer) > 1024 * 1024:
                    self._protocol_failure('Private worker response exceeds bound')
        raise TimeoutError('Private Wayland response/session time bound reached')

    def start(self):
        try:
            argv = self._sandbox()
            self.log = open(self.directory / 'sandbox.log', 'wb')
            self.process = subprocess.Popen(argv, env={'PATH': '/usr/bin', 'LANG': 'C.UTF-8'}, stdin=subprocess.PIPE,
                                            stdout=subprocess.PIPE, stderr=self.log, close_fds=True, start_new_session=True)
            result = self._receive(40)
            if 'fatal' in result: raise RuntimeError(result['fatal'])
            if result.get('request_id') is not None or not result.get('ready'):
                self._protocol_failure('Private worker failed readiness contract')
            self.record.update(result['ready']); self.record['sandbox_pid'] = self.process.pid; self._record()
            return self.record
        except BaseException as error:
            self.record['failure'] = str(error); self._record(); self.close(); raise

    def action(self, request):
        if self.closed or self.process is None: raise RuntimeError('Private session is not running')
        if not isinstance(request, dict): raise ValueError('Action must be a JSON object')
        request_id = self.request_id + 1
        payload = json.dumps({'request_id': request_id, 'request': request}).encode() + b'\n'
        if len(payload) > 32768: raise ValueError('Action exceeds size bound')
        self.request_id = request_id
        self.active_request = {'request_id': request_id, 'action': request.get('action')}
        self.process.stdin.write(payload); self.process.stdin.flush()
        result = self._receive(12)
        if 'fatal' in result: self._protocol_failure('Private worker failed: ' + str(result['fatal']))
        if result.get('request_id') != request_id or result.get('action') != request.get('action'):
            self._protocol_failure('Private worker reply does not match request: ' + json.dumps(result))
        if 'error' in result:
            self.active_request = None
            raise ValueError(result['error'])
        if request.get('action') == 'screenshot':
            path = result.get('result')
            if not isinstance(path, str) or not path.startswith('/evidence/screenshots/'):
                self._protocol_failure('Private screenshot reply must name its owned PNG path')
            actual = (self.directory / path.removeprefix('/evidence/')).resolve()
            if not actual.is_relative_to(self.directory / 'screenshots') or actual.suffix != '.png' or not actual.is_file():
                self._protocol_failure('Private screenshot reply does not resolve to an owned PNG')
            result['result'] = str(actual)
        self.active_request = None
        return result

    def close(self):
        if self.closed: return
        self.closed = True
        if self.process and self.process.poll() is None:
            try:
                request = {'request_id': self.request_id + 1, 'request': {'action': 'quit'}}
                self.process.stdin.write(json.dumps(request).encode() + b'\n'); self.process.stdin.flush()
                self.process.wait(timeout=3)
            except (BrokenPipeError, OSError, subprocess.TimeoutExpired): stop_group(self.process)
        if self.process:
            for stream in (self.process.stdin, self.process.stdout):
                if stream: stream.close()
        if self.log: self.log.close()
        self.record.update(closed=True, sandbox_exit=self.process.returncode if self.process else None,
                           host_gnome_after=host_shells())
        self.record['host_gnome_identity_unchanged'] = self.record['host_gnome_after'] == self.before
        self._record()
        # Settings/output are evidence; only this owned runtime socket directory is removed.
        shutil.rmtree(self.directory / 'runtime', ignore_errors=True)

    def __enter__(self): self.start(); return self
    def __exit__(self, *_): self.close()


class PrivateWorker:
    def __init__(self, config):
        from gi.repository import Gio, GLib
        self.Gio, self.GLib = Gio, GLib
        self.config = config; self.deadline = time.monotonic() + config['seconds']
        self.processes = []; self.logs = []; self.connection = None; self.remote = None; self.stream = None; self.portal_details = None
        self.keys_down = set(); self.buttons_down = set(); self.count = 0; self.closed = False
        self.xkb = ctypes.CDLL('libxkbcommon.so.0')
        self.xkb.xkb_keysym_from_name.argtypes = [ctypes.c_char_p, ctypes.c_int]
        self.xkb.xkb_keysym_from_name.restype = ctypes.c_uint

    def call(self, destination, path, interface, method, signature='()', arguments=()):
        return self.connection.call_sync(destination, path, interface, method,
                    self.GLib.Variant(signature, arguments), None, self.Gio.DBusCallFlags.NONE,
                    min(5000, max(1, int((self.deadline - time.monotonic()) * 1000))), None).unpack()

    def alive(self):
        if self.closed: raise RuntimeError('Private worker is closed')
        if time.monotonic() >= self.deadline: raise TimeoutError('Private Wayland session bound reached')
        for name, child in self.processes:
            if child.poll() is not None:
                error = f'Owned {name} exited ({child.returncode}); see {name}.log'
                if name == 'portal-monitor' and self.portal_details:
                    self.portal_details['monitor']['error'] = error
                    self.portal_record()
                raise RuntimeError(error)

    def spawn(self, argv, name, env):
        log = open('/evidence/' + name + '.log', 'wb'); self.logs.append(log)
        child = subprocess.Popen(argv, env=env, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT,
                                 close_fds=True, start_new_session=True)
        self.processes.append((name, child)); return child

    def portal_record(self):
        Path('/evidence/portal-preparation.json').write_text(json.dumps(self.portal_details, indent=2) + '\n')

    def prepare_portal(self, env, bus):
        # This is the private dbus-run-session connection already verified by
        # start(). Only generated values enter its activation environment;
        # no host display address, socket, credential or device is exposed.
        activation = {'WAYLAND_DISPLAY': 'gmat-private', 'GDK_BACKEND': 'wayland',
                      'XDG_RUNTIME_DIR': '/evidence/runtime', 'DBUS_SYSTEM_BUS_ADDRESS': bus}
        filters = ["type='method_call',interface='org.freedesktop.portal.FileChooser'",
                   "type='signal',interface='org.freedesktop.portal.Request',member='Response'"]
        self.portal_details = {'activation_environment': {'values': activation, 'updated': False},
                               'monitor': {'argv': ['/usr/bin/dbus-monitor', '--session', *filters],
                                           'log': '/evidence/portal-monitor.log'},
                               'limit': 'Plugin selection and activation are preparation; actual requests and responses must prove FileChooser use'}
        self.portal_record()
        try:
            self.call('org.freedesktop.DBus', '/org/freedesktop/DBus', 'org.freedesktop.DBus',
                      'UpdateActivationEnvironment', '(a{ss})', (activation,))
            self.portal_details['activation_environment']['updated'] = True
        except Exception as error:
            self.portal_details['activation_environment']['error'] = str(error)
            raise RuntimeError('Owned bus portal activation environment update failed: ' + str(error)) from error
        finally: self.portal_record()
        try:
            monitor = self.spawn(self.portal_details['monitor']['argv'], 'portal-monitor', env)
            self.portal_details['monitor']['pid_inside_namespace'] = monitor.pid
            self.portal_record()
            # Catch immediate monitor setup failure; later exits are caught by
            # alive() throughout startup/actions and retained in this record.
            time.sleep(.05)
            self.alive()
        except Exception as error:
            self.portal_details['monitor']['error'] = str(error); self.portal_record()
            raise RuntimeError('Owned private portal monitor failed: ' + str(error)) from error

    def shell(self, expression):
        ok, result = self.call('org.gnome.Shell', '/org/gnome/Shell', 'org.gnome.Shell', 'Eval', '(s)', (expression,))
        if not ok: raise RuntimeError('Private shell state query failed: ' + result)
        value = json.loads(result)
        return json.loads(value) if isinstance(value, str) else value

    def windows(self):
        self.alive()
        return self.shell('JSON.stringify(global.get_window_actors().map(a => {const w=a.meta_window,r=w.get_frame_rect();return {window:w.get_id(),title:w.get_title(),pid:w.get_pid(),class:w.get_wm_class(),mapped:a.visible,focus:w.has_focus(),x:r.x,y:r.y,width:r.width,height:r.height};}))')

    def start(self):
        if Path('/dev/dri').exists() or Path('/dev/input').exists() or Path('/run/dbus/system_bus_socket').exists():
            raise RuntimeError('Mandatory physical-device/system-bus isolation failed')
        for variable in ('DISPLAY', 'WAYLAND_DISPLAY', 'WAYLAND_SOCKET', 'XAUTHORITY', 'AT_SPI_BUS_ADDRESS'):
            if variable in os.environ: raise RuntimeError('Inherited host display/auth variable: ' + variable)
        bus = os.environ.get('DBUS_SESSION_BUS_ADDRESS', '')
        if not bus.startswith('unix:path=/tmp/'): raise RuntimeError('Private dbus-run-session socket is outside private /tmp')
        self.connection = self.Gio.bus_get_sync(self.Gio.BusType.SESSION, None)
        env = dict(os.environ)
        # Shell 50 requires a connected Gio.DBus.system even with no host
        # services. Route those probes to this same owned bus; never expose
        # /run/dbus, the host bus address, logind, device services or credentials.
        env['DBUS_SYSTEM_BUS_ADDRESS'] = bus
        if self.config.get('portal_theme_file'): self.prepare_portal(env, bus)
        self.spawn(['/usr/bin/gnome-shell', '--wayland', '--headless', '--no-x11', '--unsafe-mode',
                    '--virtual-monitor=' + str(self.config['width']) + 'x' + str(self.config['height']),
                    '--wayland-display=gmat-private'], 'compositor', env)
        end = min(self.deadline, time.monotonic() + 20)
        while time.monotonic() < end:
            self.alive()
            if Path('/evidence/runtime/gmat-private').is_socket():
                try:
                    names = self.call('org.freedesktop.DBus', '/org/freedesktop/DBus', 'org.freedesktop.DBus', 'ListNames')[0]
                    if 'org.gnome.Shell' in names and 'org.gnome.Mutter.RemoteDesktop' in names: break
                except self.GLib.Error: pass
            time.sleep(.1)
        else: raise TimeoutError('Private headless compositor not ready within 20 seconds')
        # Absolute input is mapped by Mutter through a paired monitor stream.
        # Start our own PipeWire in the same mount/PID/network namespaces: no
        # host PipeWire socket, session manager, audio or physical device exists.
        self.spawn(['/usr/bin/pipewire'], 'pipewire', env)
        end = min(self.deadline, time.monotonic() + 5)
        while time.monotonic() < end:
            self.alive()
            if Path('/evidence/runtime/pipewire-0').is_socket(): break
            time.sleep(.05)
        else: raise TimeoutError('Owned PipeWire did not create its private socket')
        self.remote = self.call('org.gnome.Mutter.RemoteDesktop', '/org/gnome/Mutter/RemoteDesktop',
                                'org.gnome.Mutter.RemoteDesktop', 'CreateSession')[0]
        session_id = self.call('org.gnome.Mutter.RemoteDesktop', self.remote, 'org.freedesktop.DBus.Properties',
                               'Get', '(ss)', ('org.gnome.Mutter.RemoteDesktop.Session', 'SessionId'))[0]
        screencast = self.call('org.gnome.Mutter.ScreenCast', '/org/gnome/Mutter/ScreenCast',
                              'org.gnome.Mutter.ScreenCast', 'CreateSession', '(a{sv})',
                              ({'remote-desktop-session-id': self.GLib.Variant('s', session_id)},))[0]
        self.stream = self.call('org.gnome.Mutter.ScreenCast', screencast,
                                'org.gnome.Mutter.ScreenCast.Session', 'RecordMonitor', '(sa{sv})',
                                ('', {'cursor-mode': self.GLib.Variant('u', 0)}))[0]
        parameters = self.call('org.gnome.Mutter.ScreenCast', self.stream, 'org.freedesktop.DBus.Properties',
                               'Get', '(ss)', ('org.gnome.Mutter.ScreenCast.Stream', 'Parameters'))[0]
        if list(parameters.get('position', [])) != [0, 0] or list(parameters.get('size', [])) != [self.config['width'], self.config['height']]:
            raise RuntimeError('Private monitor coordinate mapping differs from requested unit scale: ' + repr(parameters))
        self.input('Start')
        env.update(WAYLAND_DISPLAY='gmat-private', QT_QPA_PLATFORM='wayland')
        # Capture the actual mounted launch files immediately before spawning.
        # These identities do not assert that current source produced the files.
        binary = Path(self.config['app'])
        launch_files = {'application': identity(binary),
                        'helper': identity('/harness/IsolatedWaylandSession.py'),
                        'startup': identity('/evidence/gmat_startup_isolated.txt'),
                        'original_startup': identity('/evidence/original-startup.txt')}
        for name in ('libGmatBase.so.R2026a', 'libGmatUtil.so.R2026a'):
            library = binary.parent / name
            if not library.is_file(): raise RuntimeError('Required engine library missing: ' + str(library))
            launch_files[name] = identity(library)
        if self.config.get('portal_theme_file'):
            portal = identity(self.config['portal_theme_file'])
            if portal['sha256'] != PORTAL_THEME_SHA256:
                raise RuntimeError('Mounted portal plugin changed from prepared identity')
            launch_files['portal_theme'] = portal
            # Keep compositor, monitor and service environments free of these
            # Qt-only settings; installed default Qt plugin paths remain usable.
            env.update(QT_PLUGIN_PATH='/portal-plugins', QT_QPA_PLATFORMTHEME='xdgdesktopportal')
            self.portal_details['application_environment'] = {'QT_PLUGIN_PATH': '/portal-plugins',
                                                             'QT_QPA_PLATFORMTHEME': 'xdgdesktopportal'}
            self.portal_record()
        fixture = identity(self.config['fixture']) if self.config['fixture'] else {'kind': 'application-generated default mission', 'sha256': None}
        Path('/evidence/launch-files.json').write_text(json.dumps({'files': launch_files, 'fixture': fixture}, indent=2) + '\n')
        command = [self.config['app'], '--startup', '/evidence/gmat_startup_isolated.txt', '--settings-dir', '/evidence/settings']
        if self.config['fixture']: command.append(self.config['fixture'])
        self.spawn(command, 'application', env)
        end = min(self.deadline, time.monotonic() + 15)
        while time.monotonic() < end:
            self.alive()
            try:
                if any(w['pid'] == self.processes[-1][1].pid and w['mapped'] and w['width'] > 300 and w['height'] > 150 for w in self.windows()): break
            except self.GLib.Error: pass
            time.sleep(.1)
        else: raise TimeoutError('Actual GmatQt did not map a private Wayland GUI within 15 seconds')
        ready = {'display': 'gmat-private', 'qpa': 'wayland', 'software_gl_requested': True,
                'launch_files': launch_files, 'launch_fixture': fixture,
                'input': 'Mutter RemoteDesktop persistent same-peer virtual input with absolute paired-monitor mapping',
                'input_monitor': parameters, 'input_stream': self.stream,
                'pipewire': 'owned process and runtime socket in same isolated namespaces',
                'private_bus_address': bus, 'system_api_bus': 'same owned private session bus; host services absent',
                'isolation_verified': {'physical_devices': False, 'host_system_bus': False,
                'host_display_variables': False, 'runtime': '/evidence/runtime'}, 'windows': self.windows()}
        if self.portal_details: ready['portal_preparation'] = self.portal_details
        return ready

    def input(self, method, signature='()', arguments=()):
        return self.call('org.gnome.Mutter.RemoteDesktop', self.remote,
                         'org.gnome.Mutter.RemoteDesktop.Session', method, signature, arguments)

    def move(self, x, y):
        if not isinstance(x, int) or not isinstance(y, int) or not (0 <= x < self.config['width'] and 0 <= y < self.config['height']):
            raise ValueError('Pointer coordinates outside private monitor')
        # Source-defined monitor-stream mapping, not acceleration-dependent
        # relative deltas. GNOME may clamp the first diagonal motion from its
        # initial (0,0) hot-corner barrier while letting the other axis slide.
        # At most three actual virtual motions allow that barrier to release;
        # every attempt still needs compositor coordinate readback before click.
        end = min(self.deadline, time.monotonic() + 1)
        observed = []; current = None
        for attempt in range(3):
            self.input('NotifyPointerMotionAbsolute', '(sdd)', (self.stream, float(x), float(y)))
            poll_end = min(end, time.monotonic() + .15)
            while time.monotonic() < poll_end:
                current = self.shell('JSON.stringify(global.get_pointer())')
                if abs(current[0] - x) <= 1 and abs(current[1] - y) <= 1:
                    with open('/evidence/pointer-delivery.jsonl', 'a') as log:
                        log.write(json.dumps({'requested': [x, y], 'attempts': attempt + 1,
                                              'intermediate': observed, 'delivered': current}) + '\n')
                    return current[:2]
                time.sleep(.01)
            observed.append(current)
        raise RuntimeError(f'Private compositor pointer mismatch: requested {[x, y]}, observed {observed}')

    def button(self, button, down):
        if button not in (1, 2, 3) or not isinstance(down, bool): raise ValueError('Button requires 1..3 and boolean down')
        code = {1: 272, 2: 274, 3: 273}[button]
        self.input('NotifyPointerButton', '(ib)', (code, down))
        if down: self.buttons_down.add(code)
        else: self.buttons_down.discard(code)

    def click(self, x, y, button=1, count=1):
        if count not in (1, 2): raise ValueError('Click count must be 1 or 2')
        self.move(x, y)
        # Seat readback precedes Wayland enter/motion delivery to the client.
        # Allow that delivery and an actual press interval, so the first click
        # cannot collapse into focus activation before Qt receives its press.
        self.wait(60)
        for _ in range(count):
            self.button(button, True); self.wait(60)
            self.button(button, False); self.wait(60)

    def key(self, keys):
        if not isinstance(keys, str): raise ValueError('Key chord must be text')
        aliases = {'CTRL': 'Control_L', 'SHIFT': 'Shift_L', 'ALT': 'Alt_L', 'SUPER': 'Super_L',
                   'ESC': 'Escape', 'ENTER': 'Return', 'SPACE': 'space', 'TAB': 'Tab', 'DELETE': 'Delete',
                   'BACKSPACE': 'BackSpace', 'HOME': 'Home', 'END': 'End', 'LEFT': 'Left', 'RIGHT': 'Right',
                   'UP': 'Up', 'DOWN': 'Down', 'PLUS': 'plus', 'MINUS': 'minus'}
        parts = keys.split('+')
        if not 1 <= len(parts) <= 5: raise ValueError('Key chord outside bound')
        symbols = []
        for part in parts:
            name = aliases.get(part.upper(), part)
            code = self.xkb.xkb_keysym_from_name(name.encode('ascii'), 1)
            if not code: raise ValueError('Unknown XKB key: ' + part)
            symbols.append(code)
        try:
            for code in symbols: self.input('NotifyKeyboardKeysym', '(ub)', (code, True)); self.keys_down.add(code)
        finally:
            for code in reversed(symbols):
                if code in self.keys_down: self.input('NotifyKeyboardKeysym', '(ub)', (code, False)); self.keys_down.discard(code)

    def text(self, text):
        if not isinstance(text, str) or len(text) > 8192 or any(ord(c) > 126 or (ord(c) < 32 and c not in '\n\t') for c in text):
            raise ValueError('Text supports bounded ASCII keyboard input only')
        for char in text:
            self.alive()
            code = 0xff0d if char == '\n' else 0xff09 if char == '\t' else ord(char)
            self.input('NotifyKeyboardKeysym', '(ub)', (code, True)); self.keys_down.add(code)
            self.input('NotifyKeyboardKeysym', '(ub)', (code, False)); self.keys_down.discard(code)

    def focus(self, window):
        if not isinstance(window, int) or not any(w['window'] == window and w['mapped'] for w in self.windows()):
            raise ValueError('Focus accepts only a mapped window in this private compositor')
        return self.shell(f'JSON.stringify((()=>{{const w=global.get_window_actors().map(a=>a.meta_window).find(w=>w.get_id()==={window});w.activate(global.get_current_time());return true;}})())')

    def wait(self, ms=250):
        if not isinstance(ms, int) or not 0 <= ms <= 2000: raise ValueError('Wait must be 0..2000 milliseconds')
        time.sleep(ms / 1000); self.alive()

    def screenshot(self, label='screen'):
        if not isinstance(label, str) or not re.fullmatch(r'[A-Za-z0-9_.-]{1,64}', label): raise ValueError('Invalid screenshot label')
        self.count += 1
        path = f'/evidence/screenshots/{self.count:03d}-{label}.png'
        ok, saved = self.call('org.gnome.Shell.Screenshot', '/org/gnome/Shell/Screenshot',
                             'org.gnome.Shell.Screenshot', 'Screenshot', '(bbs)', (False, False, path))
        if not ok or saved != path or not Path(path).is_file(): raise RuntimeError('Private compositor screenshot failed')
        return path

    def exit_application(self):
        # Arm the terminal observation before sending real input: alive() must
        # continue treating every *unexpected* application exit as a failure.
        applications = [child for name, child in self.processes if name == 'application']
        if len(applications) != 1: raise RuntimeError('Exactly one owned application required')
        application = applications[0]
        windows = [w for w in self.windows() if w['pid'] == application.pid and w['mapped']]
        if len(windows) != 1 or not windows[0]['focus'] or 'GMAT Qt 6' not in windows[0]['title']:
            raise ValueError('Normal exit requires the sole mapped owned main window to have focus')
        self.key('ALT+F4')
        timeout = min(5, max(0, self.deadline - time.monotonic()))
        try: code = application.wait(timeout=timeout)
        except subprocess.TimeoutExpired as error:
            raise RuntimeError('Owned application did not exit after Alt+F4 within bound') from error
        if code != 0: raise RuntimeError(f'Owned application exited abnormally after Alt+F4 ({code})')
        others = {name: child.poll() for name, child in self.processes if name != 'application'}
        if any(code is not None for code in others.values()):
            raise RuntimeError('Private service exited before normal-close observation: ' + repr(others))
        return {'application_exit': code, 'input': 'ALT+F4', 'focused_window': windows[0],
                'other_process_exits_before_cleanup': others}

    def fixture(self, path):
        path = Path(path).resolve()
        if not path.is_relative_to('/evidence') or path.suffix != '.script' or not path.is_file():
            raise ValueError('Fixture identity accepts an owned GUI-saved .script artifact only')
        value = identity(path)
        with open('/evidence/fixtures.jsonl', 'a') as log: log.write(json.dumps(value) + '\n')
        return value

    def state(self):
        return {'windows': self.windows(), 'pointer': self.shell('JSON.stringify(global.get_pointer())'), 'runtime_socket': Path('/evidence/runtime/gmat-private').is_socket(),
                'remaining_seconds': max(0, self.deadline - time.monotonic()),
                'processes': {name: {'pid_inside_namespace': p.pid, 'exit': p.poll()} for name, p in self.processes}}

    def action(self, request, request_id):
        self.alive()
        if not isinstance(request, dict): raise ValueError('Action must be an object')
        kind = request.get('action'); values = {k: v for k, v in request.items() if k != 'action'}
        if kind == 'quit': return {'action': 'quit', 'quit': True}
        if kind not in ('screenshot', 'windows', 'state', 'move', 'click', 'button', 'key', 'text', 'focus', 'wait', 'fixture', 'exit_application'):
            raise ValueError('Unknown private Wayland action')
        record = {'elapsed': self.config['seconds'] - (self.deadline - time.monotonic()),
                  **request, 'request_id': request_id}
        try:
            result = getattr(self, kind)(**values); record['result'] = result
        except Exception as error:
            record['error'] = str(error); raise
        finally:
            with open('/evidence/actions.jsonl', 'a') as log: log.write(json.dumps(record) + '\n')
        return {'action': kind, 'result': result, **({'quit': True} if kind == 'exit_application' else {})}

    def close(self):
        if self.closed: return
        self.closed = True
        if self.remote:
            try:
                for code in self.keys_down: self.input('NotifyKeyboardKeysym', '(ub)', (code, False))
                for code in self.buttons_down: self.input('NotifyPointerButton', '(ib)', (code, False))
                self.input('Stop')
            except Exception: pass
        for _, child in reversed(self.processes): stop_group(child)
        for log in self.logs: log.close()
        Path('/evidence/worker-exits.json').write_text(json.dumps({name: p.returncode for name, p in self.processes}) + '\n')
        if self.connection: self.connection.close_sync(None)


def worker_main(path):
    worker = None
    def stop(*_): raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, stop); signal.signal(signal.SIGINT, stop); signal.signal(signal.SIGALRM, stop)
    try:
        config = json.loads(Path(path).read_text()); signal.alarm(config['seconds'])
        worker = PrivateWorker(config)
        worker_reply({'request_id': None, 'ready': worker.start()})
        pending = b''; quitting = False; request_id = None
        while not quitting:
            worker.alive()
            if not select.select([sys.stdin], [], [], .2)[0]: continue
            chunk = os.read(sys.stdin.fileno(), 32768)
            if not chunk: break
            pending += chunk
            if len(pending) > 32768: raise ValueError('Action exceeds size bound')
            while b'\n' in pending:
                line, pending = pending.split(b'\n', 1)
                request_id = None; request = None
                try:
                    envelope = json.loads(line)
                    if not isinstance(envelope, dict): raise ValueError('Worker request envelope must be an object')
                    request_id = envelope.get('request_id'); request = envelope.get('request')
                    if isinstance(request_id, bool) or not isinstance(request_id, int) or request_id <= 0:
                        raise ValueError('Worker request requires a positive integer request_id')
                    result = worker.action(request, request_id)
                    worker_reply({'request_id': request_id, **result})
                    if result.get('quit'): quitting = True; break
                except (ValueError, TypeError) as error:
                    worker_reply({'request_id': request_id,
                                  'action': request.get('action') if isinstance(request, dict) else None,
                                  'error': str(error)})
        return 0
    except BaseException as error:
        worker_reply({'request_id': locals().get('request_id'),
                      'fatal': str(error) or type(error).__name__}); return 1
    finally:
        signal.alarm(0)
        if worker: worker.close()


def main():
    if len(sys.argv) == 3 and sys.argv[1] == '--worker': return worker_main(sys.argv[2])
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--app', required=True); parser.add_argument('--startup', required=True); parser.add_argument('--artifacts', required=True)
    parser.add_argument('--script', help='Optional explicit existing fixture; omit for GUI construction')
    parser.add_argument('--portal-theme-file', help='Opt in with the exact prepared Ubuntu Qt6.10.2 portal plugin file')
    parser.add_argument('--seconds', type=int, default=180); parser.add_argument('--width', type=int, default=1600); parser.add_argument('--height', type=int, default=1200)
    args = vars(parser.parse_args()); session = None
    def stop(*_): raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, stop); signal.signal(signal.SIGINT, stop)
    try:
        session = IsolatedWaylandSession(**args)
        with session:
            print(json.dumps({'ready': session.record}), flush=True)
            pending = b''; quitting = False
            while not quitting:
                remaining = session.deadline - time.monotonic()
                if remaining <= 0: raise TimeoutError('Private Wayland session bound reached')
                if not select.select([sys.stdin], [], [], min(.2, remaining))[0]: continue
                chunk = os.read(sys.stdin.fileno(), 32768)
                if not chunk: break
                pending += chunk
                if len(pending) > 32768: raise ValueError('Action exceeds size bound')
                while b'\n' in pending:
                    line, pending = pending.split(b'\n', 1)
                    try:
                        result = session.action(json.loads(line)); print(json.dumps(result), flush=True)
                        if result.get('quit'): quitting = True; break
                    except (ValueError, TypeError) as error: print(json.dumps({'error': str(error)}), flush=True)
        print(json.dumps({'closed': True, 'artifacts': str(session.directory)}), flush=True); return 0
    except (Exception, KeyboardInterrupt) as error:
        print(json.dumps({'fatal': str(error) or type(error).__name__, 'artifacts': str(session.directory) if session else None}), flush=True)
        if session:
            session.record['failure'] = str(error) or type(error).__name__
            session._record(); session.close()
        return 1


if __name__ == '__main__': sys.exit(main())
