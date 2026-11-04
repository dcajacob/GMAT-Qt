#!/usr/bin/env python3
"""Bounded real-input session for GmatQt on a private authenticated Xvfb.

No existing display is accepted. All mouse/keyboard events go through XTest on
this utility's own X server. Screenshots are displayed X11 pixels, not Qt/model
captures. Software GL and an optional private WM do not qualify GNOME/Wayland.

Example (the WM is optional and must already be supplied):
  python3 IsolatedX11Session.py --app /path/application/bin/GmatQt \
    --startup /path/application/bin/gmat_startup_qt.txt --artifacts /tmp/evidence \
    --wm-command-json '["/path/openbox","--sm-disable","--config-file","/path/openbox.xml"]' \
    --wm-library-path /path/wm/usr/lib/x86_64-linux-gnu --wm-data-dir /path/wm/usr/share

The CLI reports JSON and reads one JSON object per stdin line. Supported actions:
  {"action":"screenshot","label":"initial"}
  {"action":"windows"}
  {"action":"move","x":100,"y":200}
  {"action":"click","x":100,"y":200,"button":3,"count":1}
  {"action":"button","button":1,"down":false}
  {"action":"key","keys":"CTRL+SHIFT+C"}   # F5, ESC, ALT+F9, etc.
  {"action":"text","text":"ASCII keyboard text"}
  {"action":"focus","window":12345}       # a mapped window on this server
  {"action":"wait","ms":500}              # at most 2000 milliseconds
  {"action":"quit"}

Import IsolatedX11Session as a context manager for an adaptive driver. Artifacts
are retained; app/WM/server process groups and temporary authentication/runtime
files are cleaned up even on exceptions/signals. The total session is bounded.
"""
from __future__ import annotations

import argparse
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import select
import shutil
import signal
import subprocess
import sys
import tempfile
import time


class WindowAttributes(C.Structure):
    _fields_ = [(name, C.c_int) for name in ("x", "y", "width", "height", "border_width", "depth")] + [
        ("visual", C.c_void_p), ("root", C.c_ulong), ("class_", C.c_int),
        ("bit_gravity", C.c_int), ("win_gravity", C.c_int), ("backing_store", C.c_int),
        ("backing_planes", C.c_ulong), ("backing_pixel", C.c_ulong), ("save_under", C.c_int),
        ("colormap", C.c_ulong), ("map_installed", C.c_int), ("map_state", C.c_int),
        ("all_event_masks", C.c_long), ("your_event_mask", C.c_long),
        ("do_not_propagate_mask", C.c_long), ("override_redirect", C.c_int), ("screen", C.c_void_p)]


class XErrorEvent(C.Structure):
    _fields_ = [("type", C.c_int), ("display", C.c_void_p),
                ("resourceid", C.c_ulong), ("serial", C.c_ulong),
                ("error_code", C.c_ubyte), ("request_code", C.c_ubyte),
                ("minor_code", C.c_ubyte)]


class IsolatedX11Session:
    def __init__(self, app, startup, artifacts, *, seconds=180, width=1600, height=1200,
                 wm_command=None, wm_library_path=None, wm_data_dir=None, script=None):
        if not 10 <= seconds <= 600:
            raise ValueError("Session bound must be from 10 to 600 seconds")
        if not (640 <= width <= 4096 and 480 <= height <= 3072):
            raise ValueError("Screen size is outside the bounded test range")
        self.app, self.startup = Path(app).resolve(), Path(startup).resolve()
        if not self.app.is_file() or not os.access(self.app, os.X_OK) or not self.startup.is_file():
            raise ValueError("An existing executable and startup file are required")
        self.script = Path(script).resolve() if script else None
        if self.script and not self.script.is_file():
            raise ValueError("The optional existing mission does not exist")
        self.wm_command = list(wm_command or [])
        if self.wm_command and (not Path(self.wm_command[0]).is_absolute() or not os.access(self.wm_command[0], os.X_OK)):
            raise ValueError("WM executable must be an explicit existing absolute path")
        self.wm_library_path, self.wm_data_dir = wm_library_path, wm_data_dir
        self.seconds, self.width, self.height = seconds, width, height
        artifact_root = Path(artifacts).resolve()
        artifact_root.mkdir(parents=True, exist_ok=True)
        self.directory = Path(tempfile.mkdtemp(prefix="isolated-x11-", dir=artifact_root))
        self.runtime = tempfile.TemporaryDirectory(prefix="runtime-", dir=self.directory)
        self.private = Path(self.runtime.name)
        self.settings, self.output = self.directory / "settings", self.directory / "output"
        self.settings.mkdir(); self.output.mkdir(); (self.directory / "screenshots").mkdir()
        self.processes, self.logs = [], []
        self.display = None
        self.connection = None
        self.keys_down, self.buttons_down = set(), set()
        self.count = 0
        self.deadline = time.monotonic() + seconds
        self.closed = False
        self._prepare_startup()

    def _prepare_startup(self):
        original = self.startup.read_bytes()
        matches = list(re.finditer(rb"(?m)^([ \t]*OUTPUT_PATH[ \t]*=[ \t]*)([^\r\n]*)(\r?)$", original))
        if len(matches) != 1:
            raise ValueError("Expected exactly one active OUTPUT_PATH assignment")
        log = re.search(rb"(?m)^[ \t]*LOG_FILE[ \t]*=[ \t]*([^\r\n]*)", original)
        if not log or not log.group(1).strip().startswith(b"OUTPUT_PATH/"):
            raise ValueError("LOG_FILE must use OUTPUT_PATH so no host log is overwritten")
        match = matches[0]
        clone = original[:match.start(2)] + (str(self.output) + "/").encode() + original[match.end(2):]
        (self.directory / "original-startup.txt").write_bytes(original)
        self.clone = self.directory / "gmat_startup_isolated.txt"
        self.clone.write_bytes(clone)
        self.record = {
            "app": str(self.app), "startup": str(self.startup), "script": str(self.script) if self.script else None,
            "app_sha256": hashlib.sha256(self.app.read_bytes()).hexdigest(),
            "original_startup_sha256": hashlib.sha256(original).hexdigest(),
            "clone_startup_sha256": hashlib.sha256(clone).hexdigest(),
            "startup_change": "OUTPUT_PATH value only", "artifacts": str(self.directory),
            "seconds": self.seconds, "screen": [self.width, self.height],
            "wm_command": self.wm_command,
            "limits": "Own X11/software GL input and rendering only; GNOME/Wayland/portal and hardware-driver acceptance remain open"}
        (self.directory / "session.json").write_text(json.dumps(self.record, indent=2) + "\n")

    def _environment(self):
        env = dict(os.environ)
        for name in ("DISPLAY", "XAUTHORITY", "WAYLAND_DISPLAY", "DBUS_SESSION_BUS_ADDRESS", "AT_SPI_BUS_ADDRESS",
                     "SESSION_MANAGER", "GNOME_DESKTOP_SESSION_ID", "QT_QPA_PLATFORMTHEME", "QT_PLUGIN_PATH",
                     "QT_QPA_PLATFORM_PLUGIN_PATH", "QT_SCALE_FACTOR", "QT_SCREEN_SCALE_FACTORS", "GDK_BACKEND"):
            env.pop(name, None)
        env.update(QT_QPA_PLATFORM="xcb", LIBGL_ALWAYS_SOFTWARE="1", GALLIUM_DRIVER="llvmpipe",
                   QT_ACCESSIBILITY="0", NO_AT_BRIDGE="1", XDG_CURRENT_DESKTOP="GMATIsolatedX11",
                   XDG_SESSION_TYPE="x11", XDG_SESSION_DESKTOP="GMATIsolatedX11",
                   XDG_RUNTIME_DIR=str(self.private), XDG_CONFIG_HOME=str(self.private / "config"),
                   XDG_CACHE_HOME=str(self.private / "cache"), XAUTHORITY=str(self.private / "Xauthority"),
                   DBUS_SESSION_BUS_ADDRESS="unix:path=" + str(self.private / "disabled-session-bus"))
        return env

    def _spawn(self, argv, name, env, **kwargs):
        log = open(self.directory / f"{name}.log", "wb")
        self.logs.append(log)
        child = subprocess.Popen(argv, cwd=self.app.parent, env=env, stdin=subprocess.DEVNULL,
                                 stdout=log, stderr=subprocess.STDOUT, start_new_session=True, **kwargs)
        self.processes.append((name, child))
        return child

    def _alive(self):
        if self.closed:
            raise RuntimeError("Session is closed")
        if time.monotonic() >= self.deadline:
            raise TimeoutError("Isolated session time bound reached")
        for name, process in self.processes:
            if process.poll() is not None:
                raise RuntimeError(f"Owned {name} process exited ({process.returncode}); see {name}.log")

    def start(self):
        try:
            for program in ("Xvfb", "xauth", "import"):
                if not shutil.which(program):
                    raise RuntimeError(f"Required local executable missing: {program}")
            self.env = self._environment()
            cookie = secrets.token_hex(16)
            # The server loads this cookie from the file irrespective of its
            # display-number record. After -displayfd chooses a FREE display,
            # add that client lookup record with the same cookie.
            subprocess.run(["xauth", "-f", self.env["XAUTHORITY"], "add", ":65535", "MIT-MAGIC-COOKIE-1", cookie],
                           env=self.env, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
            read_fd, write_fd = os.pipe()
            try:
                server = self._spawn(["Xvfb", "-displayfd", str(write_fd), "-auth", self.env["XAUTHORITY"],
                                      "-screen", "0", f"{self.width}x{self.height}x24", "-nolisten", "tcp",
                                      "+extension", "GLX", "+extension", "XTEST", "-noreset"], "xvfb", self.env, pass_fds=(write_fd,))
                os.close(write_fd); write_fd = None
                if not select.select([read_fd], [], [], 10)[0]:
                    raise TimeoutError("Private Xvfb did not allocate a display within 10 seconds")
                number = os.read(read_fd, 64).strip()
                if not re.fullmatch(rb"[0-9]+", number) or server.poll() is not None:
                    raise RuntimeError("Private Xvfb failed to allocate an authenticated display")
                self.display = ":" + number.decode()
            finally:
                os.close(read_fd)
                if write_fd is not None:
                    os.close(write_fd)
            self.env["DISPLAY"] = self.display
            subprocess.run(["xauth", "-f", self.env["XAUTHORITY"], "add", self.display, "MIT-MAGIC-COOKIE-1", cookie],
                           env=self.env, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
            self._connect()
            if self.wm_command:
                wm_env = dict(self.env)
                if self.wm_library_path:
                    wm_env["LD_LIBRARY_PATH"] = str(Path(self.wm_library_path).resolve())
                if self.wm_data_dir:
                    wm_env["XDG_DATA_DIRS"] = str(Path(self.wm_data_dir).resolve()) + ":/usr/local/share:/usr/share"
                self._spawn(self.wm_command, "wm", wm_env)
                self.wait(400)
            command = [str(self.app), "--startup", str(self.clone), "--settings-dir", str(self.settings)]
            if self.script:
                command.append(str(self.script))
            self._spawn(command, "application", self.env)
            end = min(self.deadline, time.monotonic() + 15)
            while time.monotonic() < end:
                self._alive()
                if any(w["mapped"] and w["width"] > 300 and w["height"] > 150 for w in self.windows()):
                    break
                time.sleep(.1)
            else:
                raise TimeoutError("Actual GmatQt did not map a GUI window within 15 seconds")
            self.record.update(display=self.display, qpa="xcb", software_gl=True,
                               processes={name: process.pid for name, process in self.processes})
            (self.directory / "session.json").write_text(json.dumps(self.record, indent=2) + "\n")
            return self.record
        except BaseException:
            self.close()
            raise

    def _connect(self):
        self.x = C.CDLL("libX11.so.6")
        self.xt = C.CDLL("libXtst.so.6")
        prototypes = {
            "XOpenDisplay": ([C.c_char_p], C.c_void_p), "XCloseDisplay": ([C.c_void_p], C.c_int),
            "XDefaultRootWindow": ([C.c_void_p], C.c_ulong), "XFlush": ([C.c_void_p], C.c_int),
            "XSync": ([C.c_void_p, C.c_int], C.c_int),
            "XNextRequest": ([C.c_void_p], C.c_ulong),
            "XSetErrorHandler": ([C.c_void_p], C.c_void_p),
            "XQueryTree": ([C.c_void_p, C.c_ulong, C.POINTER(C.c_ulong), C.POINTER(C.c_ulong), C.POINTER(C.POINTER(C.c_ulong)), C.POINTER(C.c_uint)], C.c_int),
            "XGetWindowAttributes": ([C.c_void_p, C.c_ulong, C.POINTER(WindowAttributes)], C.c_int),
            "XFetchName": ([C.c_void_p, C.c_ulong, C.POINTER(C.c_void_p)], C.c_int),
            "XFree": ([C.c_void_p], C.c_int), "XStringToKeysym": ([C.c_char_p], C.c_ulong),
            "XKeysymToKeycode": ([C.c_void_p, C.c_ulong], C.c_ubyte),
            "XKeycodeToKeysym": ([C.c_void_p, C.c_ubyte, C.c_int], C.c_ulong),
            "XSetInputFocus": ([C.c_void_p, C.c_ulong, C.c_int, C.c_ulong], C.c_int)}
        for name, (arguments, result) in prototypes.items():
            function = getattr(self.x, name); function.argtypes, function.restype = arguments, result
        for name, arguments in {
            "XTestQueryExtension": [C.c_void_p, C.POINTER(C.c_int), C.POINTER(C.c_int), C.POINTER(C.c_int), C.POINTER(C.c_int)],
            "XTestFakeKeyEvent": [C.c_void_p, C.c_uint, C.c_int, C.c_ulong],
            "XTestFakeButtonEvent": [C.c_void_p, C.c_uint, C.c_int, C.c_ulong],
            "XTestFakeMotionEvent": [C.c_void_p, C.c_int, C.c_int, C.c_int, C.c_ulong]}.items():
            function = getattr(self.xt, name); function.argtypes, function.restype = arguments, C.c_int
        # Xlib obtains authorization from its process environment. Do not alter
        # os.environ to point at the private display; set only the private cookie
        # during this single connection, then restore the caller's environment.
        before = os.environ.get("XAUTHORITY")
        try:
            os.environ["XAUTHORITY"] = self.env["XAUTHORITY"]
            self.connection = self.x.XOpenDisplay(self.display.encode())
        finally:
            if before is None: os.environ.pop("XAUTHORITY", None)
            else: os.environ["XAUTHORITY"] = before
        if not self.connection:
            raise RuntimeError("Cannot authenticate to the utility's own Xvfb")
        event, error, major, minor = C.c_int(), C.c_int(), C.c_int(), C.c_int()
        if not self.xt.XTestQueryExtension(self.connection, C.byref(event), C.byref(error), C.byref(major), C.byref(minor)):
            raise RuntimeError("The private X server does not support XTest input")
        self.root = self.x.XDefaultRootWindow(self.connection)

    def windows(self):
        self._alive()
        root, parent, children, count = C.c_ulong(), C.c_ulong(), C.POINTER(C.c_ulong)(), C.c_uint()
        result, errors = [], []
        # A window can disappear between QueryTree and an attribute/name query.
        # The default Xlib error handler exits the process, bypassing Python's
        # owned-process cleanup. Trap only this enumeration's BadWindow race;
        # preserve other protocol errors as failures after restoring the handler.
        callback_type = C.CFUNCTYPE(C.c_int, C.c_void_p, C.POINTER(XErrorEvent))
        def on_error(display, event):
            value = event.contents
            errors.append(dict(code=int(value.error_code), request=int(value.request_code),
                               resource=int(value.resourceid), serial=int(value.serial)))
            return 0
        callback = callback_type(on_error)
        previous = self.x.XSetErrorHandler(C.cast(callback, C.c_void_p))
        first_serial = self.x.XNextRequest(self.connection)
        try:
            if self.x.XQueryTree(self.connection, self.root, C.byref(root), C.byref(parent), C.byref(children), C.byref(count)):
                for index in range(count.value):
                    window = children[index]; attributes = WindowAttributes(); title = C.c_void_p()
                    if not self.x.XGetWindowAttributes(self.connection, window, C.byref(attributes)):
                        continue
                    name = ""
                    if self.x.XFetchName(self.connection, window, C.byref(title)) and title.value:
                        try: name = C.string_at(title).decode("utf-8", "replace")
                        finally: self.x.XFree(title)
                    result.append(dict(window=int(window), title=name, x=attributes.x, y=attributes.y,
                                       width=attributes.width, height=attributes.height, mapped=attributes.map_state == 2))
        finally:
            if children: self.x.XFree(children)
            last_serial = self.x.XNextRequest(self.connection) - 1
            self.x.XSync(self.connection, 0)
            self.x.XSetErrorHandler(previous)
        if errors:
            with open(self.directory / "window-query-errors.jsonl", "a") as log:
                for error in errors: log.write(json.dumps(error) + "\n")
            if any(error["code"] != 3 or error["request"] not in (3, 15, 20) or
                   not first_serial <= error["serial"] <= last_serial for error in errors):
                raise RuntimeError(f"Private X11 window enumeration failed: {errors}")
        return result

    def move(self, x, y):
        self._alive()
        if not (isinstance(x, int) and isinstance(y, int) and 0 <= x < self.width and 0 <= y < self.height):
            raise ValueError("Pointer coordinates must lie inside the private screen")
        self.xt.XTestFakeMotionEvent(self.connection, 0, x, y, 0); self.x.XFlush(self.connection)

    def button(self, button=1, down=True):
        self._alive()
        if button not in range(1, 8) or not isinstance(down, bool):
            raise ValueError("Button must be 1..7 and down a boolean")
        self.xt.XTestFakeButtonEvent(self.connection, button, int(down), 0)
        (self.buttons_down.add if down else self.buttons_down.discard)(button)
        self.x.XFlush(self.connection)

    def click(self, x, y, button=1, count=1):
        if count not in (1, 2): raise ValueError("Click count must be one or two")
        self.move(x, y)
        for _ in range(count):
            self.button(button, True); time.sleep(.03); self.button(button, False); time.sleep(.04)

    def _keycode(self, name):
        aliases = {"CTRL": "Control_L", "CONTROL": "Control_L", "SHIFT": "Shift_L", "ALT": "Alt_L",
                   "SUPER": "Super_L", "ESC": "Escape", "ENTER": "Return", "SPACE": "space", "BACKSPACE": "BackSpace",
                   "DELETE": "Delete", "TAB": "Tab", "UP": "Up", "DOWN": "Down", "LEFT": "Left", "RIGHT": "Right"}
        name = aliases.get(name.upper(), name)
        symbol = self.x.XStringToKeysym(name.encode("ascii"))
        code = self.x.XKeysymToKeycode(self.connection, symbol) if symbol else 0
        if not code: raise ValueError(f"Private keyboard does not contain keysym {name}")
        return code

    def key(self, keys):
        self._alive()
        names = keys.split("+") if isinstance(keys, str) else list(keys)
        if not 1 <= len(names) <= 5: raise ValueError("A key/chord contains one to five keysyms")
        codes = [self._keycode(name) for name in names]
        try:
            for code in codes:
                self.xt.XTestFakeKeyEvent(self.connection, code, 1, 0); self.keys_down.add(code)
            self.x.XFlush(self.connection); time.sleep(.03)
        finally:
            for code in reversed(codes):
                self.xt.XTestFakeKeyEvent(self.connection, code, 0, 0); self.keys_down.discard(code)
            self.x.XFlush(self.connection)

    def text(self, text):
        if not isinstance(text, str) or len(text) > 1000 or any(ord(ch) < 32 or ord(ch) > 126 for ch in text):
            raise ValueError("Text input accepts at most 1000 printable ASCII characters")
        for ch in text:
            self._alive()
            symbol = ord(ch)
            code = self.x.XKeysymToKeycode(self.connection, symbol)
            if not code: raise ValueError(f"Private keyboard cannot type {ch!r}")
            base = self.x.XKeycodeToKeysym(self.connection, code, 0)
            shifted = self.x.XKeycodeToKeysym(self.connection, code, 1)
            if base != symbol and shifted != symbol:
                raise ValueError(f"Text keysym {ch!r} is outside the supported unshifted/Shift layout")
            shift = self._keycode("SHIFT") if base != symbol else None
            try:
                if shift: self.xt.XTestFakeKeyEvent(self.connection, shift, 1, 0); self.keys_down.add(shift)
                self.xt.XTestFakeKeyEvent(self.connection, code, 1, 0); self.keys_down.add(code)
                self.xt.XTestFakeKeyEvent(self.connection, code, 0, 0); self.keys_down.discard(code)
                if shift: self.xt.XTestFakeKeyEvent(self.connection, shift, 0, 0); self.keys_down.discard(shift)
                self.x.XFlush(self.connection)
            finally:
                for held in list(self.keys_down): self.xt.XTestFakeKeyEvent(self.connection, held, 0, 0); self.keys_down.discard(held)
                self.x.XFlush(self.connection)

    def focus(self, window):
        self._alive()
        if not any(w["window"] == window and w["mapped"] for w in self.windows()):
            raise ValueError("Focus accepts only a mapped top-level window on this own display")
        self.x.XSetInputFocus(self.connection, window, 2, 0); self.x.XFlush(self.connection)

    def wait(self, ms=250):
        if not isinstance(ms, int) or not 0 <= ms <= 2000: raise ValueError("Wait must be 0..2000 milliseconds")
        self._alive(); time.sleep(min(ms / 1000, max(0, self.deadline - time.monotonic()))); self._alive()

    def screenshot(self, label="screen"):
        self._alive()
        if not isinstance(label, str) or not re.fullmatch(r"[A-Za-z0-9_.-]{1,64}", label):
            raise ValueError("Screenshot label must be 1..64 filename characters")
        self.count += 1
        path = self.directory / "screenshots" / f"{self.count:03d}-{label}.png"
        subprocess.run(["import", "-display", self.display, "-window", "root", str(path)], env=self.env,
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, timeout=min(10, max(1, self.deadline - time.monotonic())))
        return str(path)

    def action(self, request):
        kind = request.get("action")
        values = {key: value for key, value in request.items() if key != "action"}
        methods = {name: getattr(self, name) for name in ("screenshot", "windows", "move", "click", "button", "key", "text", "focus", "wait")}
        if kind == "quit": return {"quit": True}
        if kind not in methods: raise ValueError("Unknown isolated input action")
        result = methods[kind](**values)
        with open(self.directory / "actions.jsonl", "a") as log:
            log.write(json.dumps({"elapsed": self.seconds - (self.deadline - time.monotonic()), **request, "result": result}) + "\n")
        return {"action": kind, "result": result}

    def close(self):
        if self.closed: return
        self.closed = True
        if self.connection:
            for code in self.keys_down: self.xt.XTestFakeKeyEvent(self.connection, code, 0, 0)
            for button in self.buttons_down: self.xt.XTestFakeButtonEvent(self.connection, button, 0, 0)
            self.x.XFlush(self.connection); self.x.XCloseDisplay(self.connection); self.connection = None
        for name, process in reversed(self.processes):
            if process.poll() is None:
                try: os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError: pass
                try: process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    try: os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError: pass
                    process.wait(timeout=3)
        for log in self.logs: log.close()
        self.runtime.cleanup()
        self.record["closed"] = True
        self.record["exit_codes"] = {name: process.returncode for name, process in self.processes}
        (self.directory / "session.json").write_text(json.dumps(self.record, indent=2) + "\n")

    def __enter__(self): self.start(); return self
    def __exit__(self, *_): self.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--app", required=True); parser.add_argument("--startup", required=True)
    parser.add_argument("--artifacts", required=True); parser.add_argument("--script")
    parser.add_argument("--seconds", type=int, default=180)
    parser.add_argument("--width", type=int, default=1600); parser.add_argument("--height", type=int, default=1200)
    parser.add_argument("--wm-command-json"); parser.add_argument("--wm-library-path"); parser.add_argument("--wm-data-dir")
    args = vars(parser.parse_args())
    wm_json = args.pop("wm_command_json")
    args["wm_command"] = json.loads(wm_json) if wm_json else []
    if not isinstance(args["wm_command"], list) or not all(isinstance(value, str) for value in args["wm_command"]):
        parser.error("WM command must be a JSON array of argument strings")
    def stopped(*_): raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, stopped); signal.signal(signal.SIGINT, stopped)
    session = None
    try:
        session = IsolatedX11Session(**args)
        with session:
            print(json.dumps({"ready": session.record, "windows": session.windows()}), flush=True)
            while True:
                session._alive()
                ready, _, _ = select.select([sys.stdin], [], [], min(.25, max(0, session.deadline - time.monotonic())))
                if not ready: continue
                line = sys.stdin.readline()
                if not line: break
                try:
                    request = json.loads(line)
                    if not isinstance(request, dict): raise ValueError("Action must be a JSON object")
                    result = session.action(request)
                    print(json.dumps(result), flush=True)
                    if result.get("quit"): break
                except (ValueError, TypeError, subprocess.SubprocessError) as error:
                    print(json.dumps({"error": str(error)}), flush=True)
        print(json.dumps({"closed": True, "artifacts": str(session.directory)}), flush=True)
        return 0
    except (Exception, KeyboardInterrupt) as error:
        print(json.dumps({"fatal": str(error), "artifacts": str(session.directory) if session else None}), flush=True)
        if session: session.close()
        return 1


if __name__ == "__main__":
    sys.exit(main())
