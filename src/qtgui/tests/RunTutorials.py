#!/usr/bin/env python3
"""Rebuild Help tutorials using QtTest input on live identified GUI controls.

The default driver rejects coordinate fallback. References are opened only
after the new GUI construction is frozen. Legacy replay code is available
only under an explicit diagnostic driver.
"""
from __future__ import annotations
import argparse
import base64
import ctypes
import datetime
import difflib
import hashlib
import io
import json
import math
import os
from pathlib import Path
import re
import signal
import shutil
import subprocess
import sys
import tempfile
import time

from IsolatedX11Session import IsolatedX11Session, WindowAttributes

REPO = Path(__file__).resolve().parents[3]
RECIPES = Path(__file__).with_name('tutorials')


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save_json(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n')


def scale_value(value):
    if value.lower() == 'auto':
        return None
    try:
        scale = float(value)
    except ValueError:
        raise argparse.ArgumentTypeError('Scale must be auto or a number from 0.5 to 4') from None
    if not math.isfinite(scale) or not .5 <= scale <= 4:
        raise argparse.ArgumentTypeError('Scale must be auto or a number from 0.5 to 4')
    return scale


def host_display_info(probe):
    if not os.environ.get('DISPLAY'):
        return None
    try:
        # Query the same X11 toolkit path used by Xephyr's host window. This
        # accounts for the desktop's fractional/XWayland rendering scale.
        result = subprocess.run([str(probe), '--host-display-info'],
                                env=dict(os.environ, QT_QPA_PLATFORM='xcb'),
                                capture_output=True, text=True, check=True, timeout=10)
        info = json.loads(result.stdout)
        if not math.isfinite(info['scale']) or not .5 <= info['scale'] <= 4:
            return None
        if len(info['available_logical_size']) != 2 or any(v <= 0 for v in info['available_logical_size']):
            return None
        return info
    except (OSError, subprocess.SubprocessError, ValueError, KeyError, TypeError):
        return None


def configure_display(args, probe):
    info = host_display_info(probe) if args.visible and args.driver == 'qttest' else None
    requested_scale = args.scale
    args.scale = requested_scale if requested_scale is not None else (info['scale'] if info else 1.0)
    args.scale_source = 'explicit' if requested_scale is not None else ('host display' if info else 'default')
    args.host_display = info
    size = [min(4096, round(1600 * args.scale)), min(3072, round(1200 * args.scale))]
    if info:
        # Leave room for host decorations. Explicit pixel sizes remain exact.
        size = [min(value, round(available * info['scale'] * .9))
                for value, available in zip(size, info['available_logical_size'])]
    args.width = args.width if args.width is not None else max(640, size[0])
    args.height = args.height if args.height is not None else max(480, size[1])


def semantic_action(original, work):
    """QtTest's protocol contains selectors and values, never screen positions."""
    action = {key: value for key, value in original.items()
              if key in {'action', 'target_name', 'target_text', 'target_role', 'field_hint',
                         'field_button', 'scope_name', 'button', 'count', 'keys', 'text', 'value', 'checked', 'state'}}
    if action.get('keys', '').upper() == 'ALT+F10':
        return {'action': 'window', 'state': 'maximized'}
    if action['action'] == 'text':
        action['text'] = relocate(action['text'], work)
    if action['action'] in {'click', 'select', 'check'} and not any(action.get(key) for key in ('target_name', 'target_text', 'field_hint', 'field_button')):
        hint = original.get('hint', '')
        if hint in {'Apply', 'Close', 'Create', 'OK', 'Cancel', 'Yes', 'Discard', 'Save', 'Open'}:
            action.update(target_text=hint, target_role='button')
        elif hint in {'Resources', 'Mission', 'Output', 'Setup', 'Advanced'}:
            action.update(target_text=hint, target_role='tab')
        else:
            raise RuntimeError('This step still requires a semantic control selector; coordinate replay is disabled')
    if action['action'] not in {'click', 'key', 'text', 'select', 'check', 'window'}:
        raise RuntimeError('This step needs a named QtTest gesture: '+action['action'])
    return action


class QtTestInputs:
    def __init__(self, session):
        self.session, self.sequence = session, 0

    def action(self, action):
        self.sequence += 1
        identifier = str(self.sequence)
        state = self.session.directory / 'state'
        state.mkdir(exist_ok=True)
        request = state / 'input-request.pending'
        save_json(request, {'id': identifier, 'action': action})
        request.replace(state / 'input-request.json')
        # Long literal function-editor lines can spend time in nested GUI
        # validation/highlighting. Keep selector lookup bounded, but allow an
        # accepted text event enough time to finish before sending another.
        started=time.monotonic()
        deadline = started+20
        while time.monotonic()<deadline:
            self.session._alive()
            finished = state / 'input-finished' / (identifier+'.json')
            if finished.exists():
                result = json.loads(finished.read_text())
                if not result['ok']:
                    raise RuntimeError(result['error'])
                return result
            suspended = state / 'input-suspended' / (identifier+'.json')
            if suspended.exists():
                return json.loads(suspended.read_text())
            if (state/'input-accepted'/(identifier+'.json')).exists():
                deadline=max(deadline,started+max(60,len(action.get('text',''))*.4))
            status = state / 'state.json'
            if status.exists() and json.loads(status.read_text()).get('running'):
                deadline = max(deadline, self.session.deadline-1)
            time.sleep(.02)
        pending = state / 'input-pending.json'
        detail = json.loads(pending.read_text()).get('error', '') if pending.exists() else ''
        raise RuntimeError('QtTest input did not complete: '+detail)


def normalized_source(text):
    # Preserve all executable statements and command order. This is deliberately
    # conservative; omitted defaults and renamed viewers remain differences.
    lines = []
    for line in text.splitlines():
        quoted = False
        end = len(line)
        for i, char in enumerate(line):
            if char == "'":
                quoted = not quoted
            elif char == '%' and not quoted:
                end = i
                break
        value = re.sub(r'^\s*GMAT\s+', '', line[:end]).strip()
        if value:
            # Normalize whitespace between grammar tokens, preserving spaces
            # inside strings and boundaries between adjacent vector numbers.
            tokens = re.findall(r"'(?:''|[^'])*'|[A-Za-z_]\w*|(?:\d+\.?\d*|\.\d+)(?:[eEdD][+-]?\d+)?|\.\.\.|==|<=|>=|~=|!=|[^\s]", value)
            lines.append(' '.join(tokens))
    return '\n'.join(lines) + '\n'


class TutorialSession(IsolatedX11Session):
    def __init__(self, *args, visible=False, resizable=True, ui_scale=1.0, work=None, seed=424242, **kwargs):
        self.visible, self.work, self.seed = visible, work, seed
        self.ui_scale = ui_scale
        self.resizable = visible and resizable
        self._resize_check_at = 0
        self._resize_screen = None
        self._xrandr = shutil.which('xrandr') if self.resizable else None
        if self.resizable and not self._xrandr:
            raise RuntimeError('Resizable replay requires xrandr; use --fixed-size if it is unavailable')
        requested_seconds=kwargs.get('seconds',180)
        kwargs['seconds']=min(requested_seconds,600)
        super().__init__(*args, **kwargs)
        # The shared desktop helper bounds active work to ten minutes. A
        # tutorial may additionally include the user's explicit viewing delays.
        self.deadline+=max(0,requested_seconds-self.seconds)
        self.seconds=requested_seconds
        self.record['tutorial_bound_seconds']=requested_seconds
        self.record['resizable']=self.resizable
        self.record['ui_scale']=self.ui_scale
        save_json(self.directory/'session.json',self.record)

    def _alive(self):
        super()._alive()
        if not self.resizable or not self.connection or not hasattr(self, 'root'):
            return
        now = time.monotonic()
        if now < self._resize_check_at:
            return
        self._resize_check_at = now + .2
        geometry = WindowAttributes()
        if not self.x.XGetWindowAttributes(self.connection, self.root, ctypes.byref(geometry)):
            raise RuntimeError('Cannot read the resized replay desktop')
        screen = (geometry.width, geometry.height)
        if screen == self._resize_screen:
            return
        # Xephyr changes its root size before refreshing the RandR monitor
        # resources. Polling those resources publishes the new bounds to Qt
        # and the private window manager, so maximized windows follow too.
        subprocess.run([self._xrandr, '--query'], env=self.env, check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, timeout=5)
        self._resize_screen = screen
        self.record.setdefault('observed_screen_sizes', []).append(list(screen))
        save_json(self.directory/'session.json', self.record)

    def wait(self, ms):
        # Keep each shared-helper wait interruptible and within its 2 s bound.
        while ms>0:
            part=min(ms,2000)
            super().wait(part)
            ms-=part

    def _environment(self):
        env = super()._environment()
        env['GMAT_QT_TUTORIAL_STATE'] = str(self.directory / 'state')
        env['GMAT_QT_TUTORIAL_WORKDIR'] = str(self.work)
        env['XDG_DATA_HOME'] = str(self.private / 'data')
        env['XDG_DATA_DIRS'] = '/usr/local/share:/usr/share'
        env['QT_SCALE_FACTOR'] = str(self.ui_scale)
        env['QT_ENABLE_HIGHDPI_SCALING'] = '1'
        env['QT_SCALE_FACTOR_ROUNDING_POLICY'] = 'PassThrough'
        if self.seed is not None:
            env['GMAT_QT_TUTORIAL_SEED'] = str(self.seed)
        else:
            env.pop('GMAT_QT_TUTORIAL_SEED', None)
        return env

    def __exit__(self, kind, value, traceback):
        if kind:
            try:
                self.screenshot('failure')
            except Exception:
                pass
        return super().__exit__(kind, value, traceback)

    def _prepare_startup(self):
        super()._prepare_startup()
        # The probe lives in the build tree. Resolve installation-relative
        # literals against the supplied *installation* startup, not that tree.
        text = self.clone.read_text()
        text = re.sub(r'^(\w+\s*=\s*)([^\n]*)$', lambda m: m[1] + (
            str((self.startup.parent / m[2].strip()).resolve())
            if m[2].strip().startswith(('../', './')) else m[2]), text, flags=re.M)
        self.clone.write_text(text)
        self.record['startup_change'] = 'Private output and absolute installation-relative literals'
        self.record['clone_startup_sha256'] = digest(self.clone)
        save_json(self.directory / 'session.json', self.record)

    def _spawn(self, argv, name, env, **kwargs):
        if name == 'xvfb' and self.visible:
            # Xephyr is a visible window containing the same private X server.
            # XTest still addresses only the owned, authenticated inner display.
            parent = os.environ.get('DISPLAY')
            if not parent:
                raise RuntimeError('--visible requires the logged-in desktop DISPLAY')
            argv = ['Xephyr' if arg == 'Xvfb' else arg for arg in argv]
            argv[argv.index('-screen') + 1:argv.index('-screen') + 3] = [f'{self.width}x{self.height}']
            argv += ['-title', 'GMAT Help tutorial replay', '-no-host-grab']
            if self.resizable:
                argv.append('-resizeable')
            env = dict(env, DISPLAY=parent)
            if os.environ.get('XAUTHORITY'):
                env['XAUTHORITY'] = os.environ['XAUTHORITY']
        return super()._spawn(argv, name, env, **kwargs)


def match_anchor(encoded, x, y, current):
    """Locate the recorded control near its old position, allowing layout drift.

    Newly closed dialogs can invalidate the preceding image. In that case the
    original recorded position remains explicit; outcome assertions still apply.
    """
    import numpy as np
    from PIL import Image
    from scipy.signal import fftconvolve
    patch = np.asarray(Image.open(io.BytesIO(base64.b64decode(encoded['png']))).convert('L'), dtype=float)
    screen = np.asarray(Image.open(current).convert('L'), dtype=float)
    h, w = patch.shape
    left, top = encoded['offset']
    # Local search prevents a control in another editor winning a global match.
    x0, y0 = max(0, x-left-240), max(0, y-top-280)
    x1, y1 = min(screen.shape[1], x-left+w+240), min(screen.shape[0], y-top+h+280)
    area = screen[y0:y1, x0:x1]
    centered = patch - patch.mean()
    kernel = np.ones_like(patch)
    sums = fftconvolve(area, kernel, mode='valid')
    squares = fftconvolve(area * area, kernel, mode='valid')
    energy = np.maximum(squares - sums * sums / patch.size, 0)
    denominator = np.sqrt(energy * (centered * centered).sum())
    score = np.divide(fftconvolve(area, centered[::-1, ::-1], mode='valid'), denominator,
                      out=np.full_like(denominator, -1), where=denominator > 1e-8)
    maximum = float(score.max())
    if maximum < .98:
        return x, y, maximum
    # Prefer the closest equally good instance (e.g. identical small buttons).
    candidates = np.argwhere(score >= max(.98, maximum - .002))
    cy, cx = min(candidates, key=lambda p: (p[1]+x0+left-x)**2 + (p[0]+y0+top-y)**2)
    return int(cx+x0+left), int(cy+y0+top), maximum


def relocate(text, work):
    # Recipe generation enumerates every writable path. No arbitrary historical
    # output location or prior authored mission is accessed during construction.
    return text.replace('${WORK}', str(work)).replace('${REPO}', str(REPO))


def control_target(action, inventory):
    """Resolve a reviewed named target or recorded visible text to UI geometry.

    This only reads the probe's inventory. The resulting operation remains a
    real XTest click, including field focus and subsequent keyboard entry.
    """
    normalize = lambda value: re.sub(r'[^a-z0-9]', '', value.lower())
    candidates = []
    name = action.get('target_name') or action.get('field_button')
    names = {'scriptEditor', 'inactiveScriptEditor'} if name == 'documentEditor' else {name}
    target_text = normalize(action.get('target_text', ''))
    target_role = action.get('target_role')
    label = normalize(action.get('field_hint', '').split('|')[0])
    hint = normalize(action.get('hint', ''))
    for entry in inventory:
        bounds = None
        priority = 0
        text = normalize(entry.get('text', ''))
        if name and entry.get('name') in names:
            bounds, priority = entry['rect'], 3
        elif target_text and (not target_role or entry.get('role') == target_role) and (text == target_text or target_text in text):
            bounds, priority = entry['rect'], 4 if text == target_text else 3
        elif label and entry.get('role') == 'label' and text == label and 'buddy' in entry:
            bounds, priority = entry['buddy'], 2
        elif hint and entry.get('role') in {'button', 'menu', 'tab', 'tree', 'combo', 'list'}:
            if entry['role'] == 'tree' and action['x'] > 280:
                continue
            if text == hint or (len(hint) >= 5 and hint in text):
                bounds, priority = entry['rect'], 1 if text == hint else 0
        if bounds:
            if action.get('field_button') and not entry.get('offscreen'):
                bx, by, bw, bh = bounds
                buttons = [e for e in inventory if e.get('role') == 'button' and
                           e.get('active_panel') == entry.get('active_panel') and
                           e['rect'][0] >= bx+bw-3 and e['rect'][0] < bx+bw+100 and
                           abs(e['rect'][1]+e['rect'][3]/2-(by+bh/2)) < 5]
                if not buttons:
                    raise RuntimeError('Required field selector is absent: '+name)
                bounds = min(buttons, key=lambda e:e['rect'][0])['rect']
            priority = priority * 2 + int(entry.get('active_panel', False))
            x, y, width, height = bounds
            point = (x+width//2, y+height//2)
            if entry.get('role') == 'tree' or (entry.get('role') == 'list' and not action.get('target_text')):
                # A context menu opens at the pointer, not at the item center.
                # Preserve the recorded horizontal location inside the row.
                point = (min(max(action['x'], x+4), x+width-4), y+height//2)
            if not entry.get('offscreen') and not (0 <= point[0] < 1600 and 0 <= point[1] < 1200):
                continue
            trusted_active = entry.get('active_panel') and (priority >= 5 or (entry.get('role') == 'button' and text == hint))
            if not name and not trusted_active and (abs(point[0]-action['x']) > 450 or abs(point[1]-action['y']) > 350):
                continue
            destination = {'scroll': entry['scroll_rect'], 'direction': entry['scroll_direction']} if entry.get('offscreen') else point
            candidates.append((-priority, (point[0]-action['x'])**2+(point[1]-action['y'])**2, destination, entry))
    if not candidates:
        if name or target_text:
            raise RuntimeError('Required visible control is absent: ' + (name or action['target_text']))
        return None
    return min(candidates, key=lambda value: value[:2])[2]


def synchronize_input(session, token):
    state = session.directory / 'state'
    token = token.encode()
    (state / 'barrier.request').write_bytes(token)
    inventory = state / 'controls.json'
    file_dialog = inventory.exists() and any(e.get('name') == 'fileNameEdit' for e in json.loads(inventory.read_text()))
    acknowledgement_seconds = 60 if file_dialog else 20
    deadline = time.monotonic() + acknowledgement_seconds
    while time.monotonic() < deadline:
        session._alive()
        response = state / 'barrier.response'
        if response.exists() and response.read_bytes() == token:
            break
        current = state / 'state.json'
        if current.exists() and json.loads(current.read_text()).get('running'):
            deadline = max(deadline, session.deadline-1)
        time.sleep(.015)
    else:
        raise RuntimeError('GUI input acknowledgement timed out')
    while (state / 'state.json').exists() and json.loads((state / 'state.json').read_text()).get('running'):
        session._alive()
        time.sleep(.05)
    # Dialog acceptance can acknowledge input inside a nested event loop before
    # its caller refreshes/expands the trees. Observe the next settled turn too.
    time.sleep(.12)
    settled = token+b':settled'
    (state / 'barrier.request').write_bytes(settled)
    deadline = time.monotonic()+(60 if file_dialog else 10)
    while time.monotonic()<deadline:
        session._alive()
        if (state / 'barrier.response').exists() and (state / 'barrier.response').read_bytes()==settled:
            return
        time.sleep(.015)
    raise RuntimeError('GUI settling acknowledgement timed out')


def reference_milestone(stage_records):
    milestones=[s for s in stage_records if s.get('reference_milestone')]
    if not milestones:
        return {}
    components=[s for s in stage_records if s.get('reference_spacecraft')]
    selected=components or milestones[-1:]
    if any(s['run']['status']!='Mission completed' for s in selected):
        raise RuntimeError('A reference milestone has no completed GUI run')
    baseline=json.loads(json.dumps(milestones[-1]['run']))
    if components:
        baseline['spacecraft']={}
        baseline['output_files']={}
        for component in components:
            for name in component['reference_spacecraft']:
                baseline['spacecraft'][name]=component['run']['spacecraft'][name]
            for name,record in component['run']['output_files'].items():
                previous=baseline['output_files'].get(name)
                if previous and previous.get('content_sha256',previous['sha256'])!=record.get('content_sha256',record['sha256']):
                    raise RuntimeError('Reference milestone output collision: '+name)
                baseline['output_files'][name]=record
    return {'reference_milestone_run':baseline,
            'reference_milestone_sources':[{k:v for k,v in s.items() if k!='run'} for s in selected]}


def replay(recipe, args, directory, probe, wm):
    work = directory / 'files'
    work.mkdir()
    stages = []
    stage_records = []
    runs = []
    candidate = None
    stage_list = list(recipe['stages'])
    for prerequisite in reversed(recipe.get('prerequisites', [])):
        dependency = json.loads((RECIPES / (prerequisite + '.json')).read_text())
        stage_list[0:0] = dependency['stages']
    executed_recipe=directory/'executed-recipe.json'
    save_json(executed_recipe, dict(recipe, stages=stage_list, driver=args.driver))
    for number, stage in enumerate(stage_list, 1):
        script = work / stage['input'] if stage.get('input') else None
        if script and not script.is_file():
            raise RuntimeError(f'Stage {number} requires a GUI-created milestone missing from this run: {script.name}')
        pacing_seconds=(len(stage['actions'])*args.input_delay_ms+
                        sum(a.get('ms',0) for a in stage['actions'] if a['action']=='wait')+
                        sum(args.pause_ms for a in stage['actions'] if a['action']=='screenshot'))/1000
        with TutorialSession(probe, args.startup, directory / f'stage-{number:02}',
                             seconds=600+math.ceil(pacing_seconds), script=script, wm_command=wm,
                             wm_library_path=args.wm_library_path, wm_data_dir=args.wm_data_dir,
                             visible=args.visible, work=work, seed=args.seed,
                             resizable=not args.fixed_size and args.driver == 'qttest',
                             ui_scale=args.scale,
                             width=args.width, height=args.height) as session:
            stages.append(str(session.directory))
            session.wait(500)
            qttest = QtTestInputs(session) if args.driver == 'qttest' else None
            context_delta = (0, 0)
            context_original = None
            filename_pending = False
            closed_normally = False
            for index, original in enumerate(stage['actions']):
                if qttest:
                    try:
                        if original['action'] == 'wait':
                            session.wait(original['ms'])
                        elif original['action'] == 'screenshot':
                            session.screenshot(original['label'])
                            print(f"  {recipe['id']} {number}.{index+1}: {original['label']}", flush=True)
                            if args.pause_ms:
                                session.wait(args.pause_ms)
                        elif original.get('keys', '').upper() == 'ALT+F4' and all(a['action'] in {'wait', 'screenshot'} for a in stage['actions'][index+1:]):
                            break  # The owned session handles process cleanup.
                        else:
                            qttest.action(semantic_action(original, work))
                            if args.input_delay_ms:
                                session.wait(args.input_delay_ms)
                    except Exception as error:
                        raise RuntimeError(f"QtTest stage {number} ({stage['name']}), action {index+1}: {error}") from error
                    continue
                action = dict(original)
                anchor = action.pop('anchor', None)
                if filename_pending and action['action'] == 'key' and action.get('keys') == 'ENTER':
                    inventory_path = session.directory / 'state/controls.json'
                    dialog = json.loads(inventory_path.read_text())
                    accepts = [e for e in dialog if e.get('role') == 'button' and e.get('text', '').replace('&', '') in {'Save', 'Open'}]
                    if any(e.get('name') == 'fileNameEdit' for e in dialog) and accepts:
                        x, y, w, h = accepts[0]['rect']
                        action = {'action': 'click', 'x': x+w//2, 'y': y+h//2}
                    filename_pending = False
                if action['action'] == 'text':
                    action['text'] = relocate(action['text'], work)
                    inventory_path = session.directory / 'state/controls.json'
                    if action['text'].startswith(str(work)+'/') and inventory_path.exists():
                        dialog_entries = json.loads(inventory_path.read_text())
                        previous = next((a for a in reversed(stage['actions'][:index]) if a['action'] not in {'wait', 'screenshot'}), {})
                        expects_dialog = (original.get('target_name') == 'fileNameEdit' or
                                          previous.get('target_name') == 'fileNameEdit' or
                                          previous.get('keys', '').upper() in {'CTRL+O', 'CTRL+SHIFT+O', 'CTRL+S', 'CTRL+SHIFT+S'})
                        ready_by = time.monotonic()+3
                        while expects_dialog and not any(e.get('name') == 'fileNameEdit' for e in dialog_entries) and time.monotonic()<ready_by:
                            session._alive()
                            time.sleep(.05)
                            dialog_entries = json.loads(inventory_path.read_text())
                        if expects_dialog and not any(e.get('name') == 'fileNameEdit' for e in dialog_entries):
                            raise RuntimeError(f'Stage {number}, action {index+1}: the requested file dialog did not become ready')
                        if any(e.get('name') == 'fileNameEdit' for e in dialog_entries):
                            point = control_target({'action': 'click', 'target_name': 'fileNameEdit', 'x': 780, 'y': 725}, dialog_entries)
                            session.action({'action': 'click', 'x': point[0], 'y': point[1]})
                            session.action({'action': 'key', 'keys': 'CTRL+A'})
                        current_directory = next((e['directory'] for e in dialog_entries if e.get('role') == 'file-dialog'), None)
                        destination = Path(action['text'])
                        if current_directory and Path(current_directory) == destination.parent:
                            action['text'] = destination.name
                target = None
                entries = []
                select_control = None
                if action['action'] in {'click', 'select', 'check'} or action.get('target_name'):
                    inventory = session.directory / 'state/controls.json'
                    if inventory.exists():
                        entries = json.loads(inventory.read_text())
                        if action.get('target_name') and any(e.get('role') == 'popup' and e.get('kind') == 'combo' and e.get('owner') != action['target_name'] for e in entries):
                            session.action({'action': 'key', 'keys': 'ENTER'})
                            synchronize_input(session, f'{number}:{index}:commit-choice')
                            entries = json.loads(inventory.read_text())
                        if action['action'] == 'select':
                            choices = sorted((e for e in entries if action.get('target_name') and e.get('name') == action['target_name'] and 'options' in e),
                                             key=lambda e: (not e.get('active_panel'), not e.get('focused')))
                            if choices and choices[0].get('current') == action['value']:
                                continue
                            select_control = choices[0] if choices else None
                        target_names = {'scriptEditor', 'inactiveScriptEditor'} if action.get('target_name') == 'documentEditor' else {action.get('target_name')}
                        already_focused = (action.get('target_name') and action['action'] not in {'click', 'select', 'check'} and
                                           any(e.get('name') in target_names and e.get('focused') and (action['target_name'] != 'documentEditor' or e.get('active_panel')) for e in entries))
                        if not already_focused:
                            target = control_target(dict(action, x=action.get('x', 800), y=action.get('y', 600)), entries)
                            for scroll_attempt in range(30):
                                if not isinstance(target, dict):
                                    break
                                x, y, width, height = target['scroll']
                                session.action({'action': 'click', 'x': x+3, 'y': y+height//2,
                                                'button': 5 if target['direction'] > 0 else 4})
                                synchronize_input(session, f'{number}:{index}:scroll:{scroll_attempt}')
                                entries = json.loads(inventory.read_text())
                                target = control_target(dict(action, x=action.get('x', 800), y=action.get('y', 600)), entries)
                            else:
                                raise RuntimeError('Required control could not be scrolled into view')
                            if action['action'] == 'select' and not select_control and isinstance(target, tuple):
                                options = [e for e in entries if 'options' in e and
                                           e['rect'][0] <= target[0] < e['rect'][0]+e['rect'][2] and
                                           e['rect'][1] <= target[1] < e['rect'][1]+e['rect'][3]]
                                select_control = options[0] if options else None
                for hint in ['target_name', 'target_text', 'target_role', 'hint', 'field_hint', 'field_button']:
                    action.pop(hint, None)
                if target:
                    if action['action'] == 'click':
                        action['x'], action['y'] = target
                    elif action['action'] != 'check':
                        session.action({'action': 'click', 'x': target[0], 'y': target[1]})
                        if action['action'] == 'select':
                            synchronize_input(session, f'{number}:{index}:open-choice')
                            entries = json.loads(inventory.read_text())
                elif anchor:
                    screen = session.directory / 'pointer-check.png'
                    subprocess.run(['import', '-display', session.display, '-window', 'root', str(screen)],
                                   env=session.env, check=True, timeout=10, capture_output=True)
                    action['x'], action['y'], score = match_anchor(anchor, action['x'], action['y'], screen)
                    with (session.directory / 'anchors.jsonl').open('a') as log:
                        log.write(json.dumps({'action': index+1, 'score': score,
                                              'recorded': [original['x'], original['y']],
                                              'actual': [action['x'], action['y']]}) + '\n')
                if action['action'] == 'click' and original['action'] == 'click' and not target and (action['x'], action['y']) == (original['x'], original['y']):
                    if any(e.get('role') == 'menu' for e in entries):
                        action['x'] += context_delta[0]
                        action['y'] += context_delta[1]
                    elif action['x'] < 280 and action['y'] >= 260 and any(e.get('role') == 'tree' and e.get('text') == 'Regions' for e in entries):
                        action['y'] += stage.get('resource_tree_dy', 0)
                if action['action'] == 'click' and action.get('button') == 3:
                    context_delta = (action['x']-original['x'], action['y']-original['y'])
                    context_original = (original['x'], original['y'])
                if action['action'] == 'click' and action.get('button', 1) == 1 and not original.get('target_name') and context_original:
                    menus = [e for e in entries if e.get('role') == 'menu']
                    if any(e['text'].startswith('Insert before') for e in menus):
                        ordinal = int((original['y']-context_original[1])/25)
                        captions = ['Edit command', 'Insert before', 'Insert after']
                        if any(e['text'].startswith('Append inside') for e in menus):
                            captions.append('Append inside branch')
                        captions += ['Delete command', 'Command summary', 'Breakpoint before command']
                        if 0 <= ordinal < len(captions):
                            choice = next((e for e in menus if e['text'].startswith(captions[ordinal])), None)
                            if choice:
                                x, y, w, h = choice['rect']
                                action['x'], action['y'] = x+w//2, y+h//2
                try:
                    if action['action'] == 'check':
                        items = [e for e in entries if 'checked' in e and target and
                                 e['rect'][0] <= target[0] < e['rect'][0]+e['rect'][2] and
                                 e['rect'][1] <= target[1] < e['rect'][1]+e['rect'][3]]
                        if not items:
                            raise RuntimeError('Required checkable item is absent')
                        item = items[0]
                        if item['checked'] != original['checked']:
                            session.action({'action': 'click', 'x': target[0], 'y': target[1]})
                            if item['role'] == 'list':
                                session.action({'action': 'key', 'keys': 'SPACE'})
                            synchronize_input(session, f'{number}:{index}:check')
                        continue
                    if action['action'] == 'select':
                        control = select_control
                        if not control or original['value'] not in control['options']:
                            raise RuntimeError('Required combo choice is absent: ' + original['value'])
                        if control.get('current') != original['value']:
                            session.action({'action': 'key', 'keys': 'Home'})
                            for _ in range(control['options'].index(original['value'])):
                                session.action({'action': 'key', 'keys': 'DOWN'})
                            session.action({'action': 'key', 'keys': 'ENTER'})
                            synchronize_input(session, f'{number}:{index}:select')
                        continue
                    session.action(action)
                    tail = stage['actions'][index+1:]
                    if action['action'] == 'key' and action.get('keys') == 'ALT+F4' and all(a['action'] in {'wait', 'screenshot'} for a in tail):
                        time.sleep(.5)
                        application = next(process for name, process in session.processes if name == 'application')
                        if application.poll() == 0:
                            closed_normally = True
                            break
                    if action['action'] in {'click', 'text', 'key', 'button'}:
                        if action['action'] == 'text' and not entries and (session.directory / 'state/controls.json').exists():
                            entries = json.loads((session.directory / 'state/controls.json').read_text())
                        if action['action'] == 'text' and any(e.get('name') == 'fileNameEdit' for e in entries):
                            # QFileDialog's path completion can defer timer
                            # callbacks until the following Enter/click. Keep
                            # the typed path and its acceptance in one input
                            # transaction; the acceptance is still acknowledged.
                            session.wait(250)
                            filename_pending = True
                        else:
                            synchronize_input(session, f'{number}:{index}')
                except Exception as error:
                    raise RuntimeError(f"Stage {number} ({stage['name']}), action {index+1}: {error}") from error
                if action['action'] == 'screenshot':
                    print(f"  {recipe['id']} {number}.{index+1}: {action['label']}", flush=True)
                    if args.pause_ms:
                        session.wait(args.pause_ms)
                elif args.input_delay_ms:
                    session.wait(args.input_delay_ms)
            if not closed_normally:
                session.wait(300)
                session.screenshot('stage-finished')
            state = session.directory / 'state'
            final_ui = json.loads((state / 'state.json').read_text())
            if number == len(stage_list) and final_ui.get('selected_document_active') is False:
                raise RuntimeError(f"Guide stage {stage['name']} ended on an inactive script document")
            candidate = (state / 'current.script').read_bytes()
            stage_source = directory / f'stage-{number:02}-authored.script'
            stage_source.write_bytes(candidate)
            stage_runs = []
            for result in sorted(state.glob('run-*.json')):
                entry = json.loads(result.read_text())
                entry['evidence'] = str(result)
                runs.append(entry)
                stage_runs.append(entry)
            current = normalized_source(candidate.decode())
            matching_stage_runs = [run for run in stage_runs if normalized_source(Path(run['evidence']).with_suffix('.script').read_text()) == current]
            stage_record = {'name': stage['name'], 'source': str(stage_source), 'sha256': digest(stage_source),
                            'run': matching_stage_runs[-1] if matching_stage_runs else None}
            for key in ('reference_milestone','reference_spacecraft'):
                if key in stage: stage_record[key]=stage[key]
            stage_records.append(stage_record)
            accepted_statuses=stage.get('accepted_run_statuses', ['Mission completed'])
            if stage.get('requires_run') and (not stage_record['run'] or stage_record['run']['status'] not in accepted_statuses):
                save_json(directory / 'failed-stage.json', stage_record)
                raise RuntimeError(f"Guide stage {stage['name']} did not complete its constructed mission")
            # These are fresh generated data, never old mission configurations.
            # Later guide stages need observations produced by their own earlier
            # simulation. Keep the original per-stage output and its hash too.
            for generated in session.output.iterdir():
                if generated.is_file() and generated.suffix.lower() in {'.gmd', '.e', '.oem', '.warmstart', '.csv'}:
                    (work / generated.name).write_bytes(generated.read_bytes())
            for alias, generated_name in stage.get('data_aliases', {}).items():
                if Path(alias).name != alias or Path(generated_name).name != generated_name:
                    raise RuntimeError('Generated data aliases must be plain filenames')
                generated = work / generated_name
                if not generated.is_file() or generated.suffix.lower() not in {'.gmd', '.e', '.oem', '.warmstart', '.csv'}:
                    raise RuntimeError('A required fresh generated data file is absent: ' + generated_name)
                (work / alias).write_bytes(generated.read_bytes())
    # Freeze source and outputs before touching any reference file.
    authored = directory / 'authored.script'
    authored.write_bytes(candidate)
    freeze = {'time_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'sha256': digest(authored), 'stages': stages, 'stage_records': stage_records, 'runs': runs,
              'guide_sha256': recipe['guide_sha256'], 'recipe_sha256': digest(executed_recipe),
              'recipe_snapshot': str(executed_recipe)}
    current = normalized_source(candidate.decode())
    matching = [run for run in runs if normalized_source(Path(run['evidence']).with_suffix('.script').read_text()) == current]
    freeze['final_source_run'] = matching[-1] if matching else None
    freeze.update(reference_milestone(stage_records))
    save_json(directory / 'construction-freeze.json', freeze)
    if not matching or matching[-1]['status'] != 'Mission completed':
        raise RuntimeError('The final constructed mission has no matching completed GUI run; earlier stages do not qualify it')
    return authored, freeze


def execute_reference(reference, args, directory, probe, authored_work, input_aliases=None):
    root = directory / 'reference'
    root.mkdir()
    output = root / 'output'
    output.mkdir()
    work = root / 'bin'
    work.mkdir()
    (work / 'output').symlink_to(output, target_is_directory=True)
    reference_copy = work / reference.name
    shutil.copyfile(reference, reference_copy)
    reference_text=reference.read_text()
    # Historical missions name nested output directories that were shipped in
    # older installations. Recreate their directories inside the private tree.
    # Keep every original output filename and every source byte unchanged.
    for literal in re.findall(r"(?:Filename|ReportFile)\s*=\s*'?((?:\./|\.\./)[^';\n]+)", reference_text):
        destination=(work/literal.strip()).resolve()
        if not destination.is_relative_to(root.resolve()):
            raise RuntimeError('Reference output escapes its private execution tree: '+literal)
        destination.parent.mkdir(parents=True,exist_ok=True)
    # The guide's navigation missions use ../output for observations and ramps.
    # Supply only this case's freshly produced data to the comparison run.
    for source in authored_work.iterdir():
        if source.is_file() and source.suffix.lower() in {'.gmd', '.e', '.oem', '.warmstart', '.csv', '.rmp'}:
            shutil.copyfile(source, output / source.name)
            shutil.copyfile(source, work / source.name)
    # A supplied sample may require a bundled support ramp. This access occurs
    # after the construction freeze and never supplies a candidate mission.
    for support in (REPO / 'application/samples/SupportFiles').glob('*.rmp'):
        if support.name in reference.read_text() and not (output / support.name).exists():
            shutil.copyfile(support, output / support.name)
    staged_inputs = []
    for literal in re.findall(r"'([^'\n]+\.[A-Za-z0-9_-]+)'", reference_text):
        if not literal.startswith(('./', '../')):
            continue
        if Path(literal).suffix.lower() not in {'.rmp', '.gmd', '.gmf', '.csv', '.e'} and not literal.startswith(('../samples/SupportFiles/', '../data/')):
            continue
        destination = (work / literal).resolve()
        if not destination.is_relative_to(root.resolve()):
            raise RuntimeError('Reference dependency escapes its private execution tree: ' + literal)
        basename = Path(literal).name
        bundled = [reference.parent / literal, REPO / 'application/bin' / literal,
                   REPO / 'application/samples/SupportFiles' / basename, reference.parent / basename]
        own_basename=(input_aliases or {}).get(basename,basename)
        if Path(own_basename).name!=own_basename:
            raise RuntimeError('Reference input alias must name a current-case file')
        if basename in (input_aliases or {}) and not (authored_work/own_basename).is_file():
            raise RuntimeError('Required current-case reference input is missing: '+own_basename)
        sources = bundled if Path(literal).suffix.lower() == '.gmf' else [authored_work / own_basename] + bundled
        source = next((p.resolve() for p in sources if p.is_file()), None)
        if source:
            destination.parent.mkdir(parents=True, exist_ok=True)
            if source != destination:
                shutil.copyfile(source, destination)
            staged_inputs.append({'literal': literal, 'source': str(source), 'sha256': digest(source),
                                  'destination': str(destination)})
    input_baseline = {p.name: (p.stat().st_mtime_ns, digest(p)) for p in output.iterdir() if p.is_file()}
    startup_text = args.startup.read_text()
    startup_text = re.sub(r'^(\w+\s*=\s*)([^\n]*)$', lambda m: m[1] + (
        str((args.startup.parent / m[2].strip()).resolve())
        if m[2].strip().startswith(('../', './')) else m[2]), startup_text, flags=re.M)
    startup_text = re.sub(r'^OUTPUT_PATH\s*=.*$', 'OUTPUT_PATH = ' + str(output) + '/', startup_text, flags=re.M)
    startup = root / 'startup.txt'
    startup.write_text(startup_text)
    environment = dict(os.environ, QT_QPA_PLATFORM='offscreen', LIBGL_ALWAYS_SOFTWARE='1',
                       GMAT_QT_TUTORIAL_STATE=str(root / 'state'),
                       GMAT_QT_TUTORIAL_WORKDIR=str(work), XDG_CONFIG_HOME=str(root / 'config'),
                       XDG_DATA_HOME=str(root / 'data'), XDG_DATA_DIRS='/usr/local/share:/usr/share',
                       XDG_CACHE_HOME=str(root / 'cache'))
    if args.seed is not None:
        environment['GMAT_QT_TUTORIAL_SEED'] = str(args.seed)
    else:
        environment.pop('GMAT_QT_TUTORIAL_SEED', None)
    command = [str(probe), '--startup', str(startup), '--settings-dir', str(root / 'settings'),
               '--run', '--convert-views', str(reference_copy)]
    started = time.monotonic()
    with (root / 'process.log').open('wb') as log:
        child = subprocess.Popen(command, env=environment, cwd=root, stdout=log, stderr=subprocess.STDOUT,
                                 start_new_session=True)
        try:
            code = child.wait(timeout=300)
        except (subprocess.TimeoutExpired, KeyboardInterrupt):
            os.killpg(child.pid, signal.SIGTERM)
            try:
                child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(child.pid, signal.SIGKILL)
                child.wait()
            raise
    results = [json.loads(p.read_text()) for p in sorted((root / 'state').glob('run-*.json'))]
    comparison_run = json.loads(json.dumps(results[-1])) if results else None
    unchanged_inputs = []
    if comparison_run:
        for name, baseline in input_baseline.items():
            path = output / name
            if path.is_file() and (path.stat().st_mtime_ns, digest(path)) == baseline:
                comparison_run.get('output_files', {}).pop(name, None)
                unchanged_inputs.append(name)
    receipt = {'command': command, 'exit': code, 'duration_seconds': time.monotonic()-started,
               'reference_sha256': digest(reference), 'runs': results,
               'reference_original': str(reference), 'execution_copy_sha256': digest(reference_copy),
               'staged_inputs': staged_inputs,
               'comparison_run': comparison_run, 'unchanged_input_files': unchanged_inputs,
               'route': 'Separate post-freeze GUI frontend process, offscreen; source unchanged'}
    save_json(root / 'result.json', receipt)
    if code != 0 or not results or results[-1]['status'] != 'Mission completed':
        raise RuntimeError('The supplied reference did not complete; see reference/process.log')
    return comparison_run


def compare_states(authored, reference):
    a, b = authored.get('spacecraft', {}), reference.get('spacecraft', {})
    comparison = {'matched': bool(a) and a.keys() == b.keys(), 'fields': [],
                  'tolerances': {'epoch_a1mjd': 1e-9, 'position_km': 1e-5,
                                 'velocity_km_s': 1e-8, 'total_mass_kg': 1e-7},
                  'note': 'Same-engine endpoint equivalence within explicit absolute tolerances; not independent physics qualification.'}
    comparison['authored_spacecraft'], comparison['reference_spacecraft'] = list(a), list(b)
    for name in a.keys() & b.keys():
        if a[name]['j2000_body'] != b[name]['j2000_body']:
            comparison['matched'] = False
        scalars = [('epoch_a1mjd', a[name]['epoch_a1mjd'], b[name]['epoch_a1mjd'], 1e-9),
                   ('total_mass_kg', a[name]['total_mass_kg'], b[name]['total_mass_kg'], 1e-7)]
        if len(a[name]['cartesian_km_km_s']) != 6 or len(b[name]['cartesian_km_km_s']) != 6:
            comparison['matched'] = False
        for i, (av, bv) in enumerate(zip(a[name]['cartesian_km_km_s'][:6], b[name]['cartesian_km_km_s'][:6])):
            scalars.append((('X', 'Y', 'Z', 'VX', 'VY', 'VZ')[i], av, bv, 1e-5 if i < 3 else 1e-8))
        for field, av, bv, tolerance in scalars:
            delta = abs(av-bv)
            matched = math.isfinite(av) and math.isfinite(bv) and delta <= tolerance
            comparison['fields'].append({'spacecraft': name, 'field': field, 'authored': av,
                                         'reference': bv, 'absolute_difference': delta,
                                         'tolerance': tolerance, 'matched': matched})
            comparison['matched'] &= matched
    return comparison


def compare_outputs(authored, reference):
    a, b = authored.get('output_files'), reference.get('output_files')
    if a is None or b is None:
        return {'matched': False, 'error': 'An output snapshot is missing'}
    names = sorted(a.keys() | b.keys())
    files = []
    for name in names:
        av, bv = a.get(name), b.get(name)
        raw = bool(av and bv and av['sha256'] == bv['sha256'])
        content = bool(av and bv and av.get('content_sha256', av['sha256']) == bv.get('content_sha256', bv['sha256']))
        whitespace = bool(av and bv and av.get('whitespace_sha256', av.get('content_sha256', av['sha256'])) ==
                          bv.get('whitespace_sha256', bv.get('content_sha256', bv['sha256'])))
        files.append({'name': name, 'authored': av, 'reference': bv,
                      'byte_identical': raw, 'content_identical': content, 'whitespace_equivalent': whitespace})
    return {'matched': all(item['content_identical'] for item in files),
            'byte_identical_all': all(item['byte_identical'] for item in files), 'files': files,
            'whitespace_equivalent_all': all(item['whitespace_equivalent'] for item in files),
            'note': 'Complete output bytes retained. Content comparison replaces only the estimator report Run Date wall-clock header; observation epochs and all numerical values remain exact. Diagnostic GmatLog is excluded.'}


def compare_source(authored, reference, directory):
    actual, expected = authored.read_text(), reference.read_text()
    a, b = normalized_source(actual), normalized_source(expected)
    (directory / 'script.diff').write_text(''.join(difflib.unified_diff(
        expected.splitlines(True), actual.splitlines(True), fromfile=str(reference), tofile=str(authored))))
    (directory / 'statements.diff').write_text(''.join(difflib.unified_diff(
        b.splitlines(True), a.splitlines(True), fromfile='reference statements', tofile='authored statements')))
    return {'reference': str(reference), 'reference_sha256': digest(reference),
            'byte_identical': actual == expected, 'statement_identical': a == b,
            'note': 'No defaults, executable properties, commands or viewer settings are silently removed.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--visible', action='store_true', help='Show the isolated replay desktop in a Xephyr window')
    parser.add_argument('--width', type=int, help='Initial desktop width in pixels, 640..4096 (default: fit the scaled UI to the host)')
    parser.add_argument('--height', type=int, help='Initial desktop height in pixels, 480..3072 (default: fit the scaled UI to the host)')
    parser.add_argument('--scale', type=scale_value, default=None, metavar='auto|FACTOR', help='UI scale: auto uses the host display for visible replay; numbers 0.5..4 override it (1.5 = 150%%)')
    parser.add_argument('--fixed-size', action='store_true', help='Disable resizing of the visible replay window')
    parser.add_argument('--driver', choices=['qttest', 'legacy-x11'], default='qttest', help='QtTest semantic selectors; legacy-x11 is retained only for old-recording diagnosis')
    parser.add_argument('--tutorial', action='append', help='Tutorial number/id; repeat to select more (default: all published tutorials)')
    parser.add_argument('--list', action='store_true')
    parser.add_argument('--pause-ms', type=int, default=700, help='Pause at each named viewing checkpoint, 0..60000')
    parser.add_argument('--input-delay-ms', type=int, default=100, help='Settle time between inputs, 0..1000')
    parser.add_argument('--seed', type=int, default=424242, help='Identical test-only RNG seed before each GUI/reference run')
    parser.add_argument('--unseeded', action='store_true', help='Retain the engine default randomized seed')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--build', action='store_true')
    parser.add_argument('--build-dir', type=Path, default=REPO / 'build/linux-gui')
    parser.add_argument('--startup', type=Path, default=REPO / 'application/bin/gmat_startup_qt.txt')
    parser.add_argument('--wm', type=Path, required=False, help='Explicit Openbox executable (system or privately extracted)')
    parser.add_argument('--wm-library-path', type=Path)
    parser.add_argument('--wm-data-dir', type=Path)
    args = parser.parse_args()
    if args.unseeded:
        args.seed = None
    if args.seed is not None and not 0 <= args.seed <= 4294967295:
        parser.error('--seed must be an unsigned 32-bit integer')
    if not 0 <= args.pause_ms <= 60000 or not 0 <= args.input_delay_ms <= 1000:
        parser.error('Pacing is outside the supported range')
    if (args.width is not None and not 640 <= args.width <= 4096) or (args.height is not None and not 480 <= args.height <= 3072):
        parser.error('Replay desktop size must be 640..4096 pixels wide and 480..3072 pixels high')
    if args.driver == 'legacy-x11' and (args.width not in (None,1600) or args.height not in (None,1200) or args.scale not in (None,1.0)):
        parser.error('Custom desktop sizes and scaling require the default qttest driver; legacy recordings use 1600x1200 at scale 1')
    recipes = [json.loads(path.read_text()) for path in sorted(RECIPES.glob('[0-9][0-9]-*.json'))]
    if args.tutorial:
        selected = {str(int(value)).zfill(2) if value.isdigit() else value for value in args.tutorial}
        valid = {r['id'] for r in recipes} | {r['id'][:2] for r in recipes}
        if selected - valid:
            parser.error('Unknown tutorial selection: ' + ', '.join(sorted(selected-valid)))
        recipes = [r for r in recipes if r['id'] in selected or r['id'][:2] in selected]
        if not recipes:
            parser.error('No tutorial matches the selection')
    if args.list:
        for recipe in recipes:
            print(f"{recipe['id']}  {recipe['title']} ({len(recipe['stages'])} stages)")
        return 0
    if not recipes:
        parser.error('No recorded tutorial recipes are installed')
    if args.build:
        subprocess.run(['cmake', '--build', str(args.build_dir), '--target', 'GmatQtTutorialProbe', '--parallel', '4'], check=True)
    probe = args.build_dir.resolve() / 'src/qtgui/GmatQtTutorialProbe'
    if not args.wm:
        private = REPO / 'build/tools/tutorial-desktop/extracted/usr'
        if (private / 'bin/openbox').is_file():
            args.wm = private / 'bin/openbox'
            args.wm_library_path = args.wm_library_path or private / 'lib/x86_64-linux-gnu'
            args.wm_data_dir = args.wm_data_dir or private / 'share'
        elif shutil.which('openbox'):
            args.wm = Path(shutil.which('openbox'))
    if not probe.is_file() or not args.wm:
        parser.error('Build GmatQtTutorialProbe and supply --wm /absolute/path/to/openbox; see doc/DevelopersDocs/Qt6TutorialReplay.rst')
    configure_display(args, probe)
    out = args.output.absolute() if args.output else Path(tempfile.mkdtemp(prefix='gmat-tutorial-replay-'))
    if args.output:
        out.mkdir(parents=True, exist_ok=False)
    print('Tutorial evidence: ' + str(out), flush=True)
    print(f'Replay UI scale: {args.scale:g} ({args.scale_source}); starting desktop: {args.width}x{args.height}', flush=True)
    config = RECIPES / 'openbox.xml'
    wm = [str(args.wm.resolve()), '--sm-disable', '--config-file', str(config)]
    report = {'probe_sha256': digest(probe), 'visible': args.visible, 'pause_ms': args.pause_ms,
              'random_seed': args.seed, 'screen': [args.width, args.height],
              'resizable': args.visible and not args.fixed_size and args.driver == 'qttest',
              'ui_scale': args.scale, 'ui_scale_source': args.scale_source, 'host_display': args.host_display,
              'input': 'QtTest events on live identified widgets' if args.driver == 'qttest' else 'Legacy XTest recordings with coordinate fallback',
              'driver': args.driver, 'tutorials': []}
    code = 0
    try:
        for recipe in recipes:
            folder = out / recipe['id']
            folder.mkdir()
            item = {'id': recipe['id'], 'title': recipe['title'], 'status': 'failed'}
            report['tutorials'].append(item)
            print('Replaying ' + recipe['id'] + ': ' + recipe['title'], flush=True)
            try:
                if digest(REPO / recipe['guide']) != recipe['guide_sha256']:
                    raise RuntimeError('The guide changed; review the recorded steps before replaying them')
                authored, freeze = replay(recipe, args, folder, probe, wm)
                if not recipe.get('reference'):
                    item['status'] = 'no-reference'
                    item['detail'] = 'GUI construction, completed run and reopen succeeded; no standalone completed example exists for this chapter'
                    item['construction_sha256'] = freeze['sha256']
                    code = 1
                    save_json(folder / 'result.json', item)
                    save_json(out / 'results.json', report)
                    continue
                reference = REPO / recipe['reference']
                item['source_comparison'] = compare_source(authored, reference, folder)
                reference_result = execute_reference(reference, args, folder, probe, folder / 'files', recipe.get('reference_input_aliases'))
                constructed_run = freeze['final_source_run']
                item['numerical_comparison'] = compare_states(constructed_run, reference_result)
                item['output_comparison'] = compare_outputs(constructed_run, reference_result)
                if freeze.get('reference_milestone_run'):
                    item['reference_milestone_comparison']={
                        'sources':freeze['reference_milestone_sources'],
                        'numerical_comparison':compare_states(freeze['reference_milestone_run'],reference_result),
                        'output_comparison':compare_outputs(freeze['reference_milestone_run'],reference_result),
                        'note':'Explicit guide comparison milestone(s), frozen before reference access. Final extension comparisons remain above.'}
                milestones = [{'run_evidence': run['evidence'], 'status': run['status'],
                               'numerical_comparison': compare_states(run, reference_result),
                               'output_comparison': compare_outputs(run, reference_result)}
                              for run in freeze['runs'] if run['status'] == 'Mission completed']
                save_json(folder / 'run-comparisons.json', milestones)
                save_json(folder / 'numerical-comparison.json', item['numerical_comparison'])
                save_json(folder / 'output-comparison.json', item['output_comparison'])
                matched = (item['source_comparison']['statement_identical'] and
                           item['numerical_comparison']['matched'] and item['output_comparison']['matched'])
                item['status'] = 'matched' if matched else 'mismatch'
                item['detail'] = ('Statements: ' + str(item['source_comparison']['statement_identical']) +
                                  '; final states within tolerance: ' + str(item['numerical_comparison']['matched']) +
                                  '; output bytes: ' + str(item['output_comparison']['matched']))
                if not matched:
                    code = 1
            except Exception as error:
                item['error'] = str(error)
                print('FAIL: ' + str(error), flush=True)
                code = 1
            save_json(folder / 'result.json', item)
            save_json(out / 'results.json', report)
    except KeyboardInterrupt:
        code = 130
        report['interrupted'] = True
        if report['tutorials']:
            report['tutorials'][-1]['status'] = 'interrupted'
    finally:
        save_json(out / 'results.json', report)
        lines = ['# Help tutorial GUI replay', '', 'Input: '+report['input']+'.', '',
                 '| Tutorial | Outcome | Detail |', '| --- | --- | --- |']
        for item in report['tutorials']:
            lines.append(f"| {item['id']} {item['title']} | {item['status']} | {item.get('error', item.get('detail', ''))} |")
        (out / 'report.md').write_text('\n'.join(lines) + '\n')
    print('Results: ' + str(out), flush=True)
    return code


if __name__ == '__main__':
    raise SystemExit(main())
