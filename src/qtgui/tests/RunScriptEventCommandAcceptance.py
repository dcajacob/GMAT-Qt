"""Replay additional command contexts inside the real Script Event GUI.

Build GmatQtNasaScriptEventCommandTests first. This runtime-only replay does not
award the universal FRC!E138 requirement; the complete inventory is separate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import uuid
import NasaGuiCoverage as nasa


def main():
    repo = Path(__file__).resolve().parents[3]
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-dir', type=Path, default=repo/'build/linux-gui')
    p.add_argument('--visible', action='store_true')
    p.add_argument('--pause-ms', type=int, default=350)
    p.add_argument('--filter', default='^QtGui.NasaScriptEvent(Command|Extra|Backend|Marker|Starter)')
    p.add_argument('--output', type=Path)
    p.add_argument('--list', action='store_true')
    a = p.parse_args()
    identity = uuid.uuid4().hex[:12]
    root = repo/'build/gui-acceptance'
    root.mkdir(parents=True, exist_ok=True)
    review = root/('event-command-'+identity+'-runtime-review.json')
    fixtures = ['Propagate', 'Maneuver', 'BeginFiniteBurn', 'EndFiniteBurn',
                'Save', 'Report', 'Global', 'Toggle', 'NonlinearConstraint']
    refs = ['src/qtgui/tests/NasaScriptEventCommandTests.cpp',
            'src/qtgui/tests/NasaScriptEventTests.cpp',
            'src/qtgui/ScriptEventDialog.cpp', 'src/base/command/BeginScript.cpp',
            'src/base/command/EndScript.cpp', 'src/base/executive/Sandbox.hpp',
            'src/base/executive/Sandbox.cpp', 'src/base/executive/Moderator.hpp',
            'plugins/GmatFunctionPlugin/src/base/command/Global.cpp',
            'src/base/command/SaveMission.cpp', 'src/base/command/CommandUtil.cpp',
            'src/qtgui/MissionModel.cpp', 'src/qtgui/tests/ScriptEventAdditionalOracle.py',
            'src/qtgui/tests/ScriptEventBackendOracle.py',
            'src/qtgui/tests/ScriptEventMarkerOracle.py',
            'src/qtgui/tests/ScriptEventStarterOracle.py',
            'src/base/executive/Moderator.cpp', 'src/base/interpreter/Interpreter.cpp',
            'src/base/command/NoOp.cpp', 'src/base/command/BeginMissionSequence.cpp',
            'plugins/CInterfacePlugin/src/command/PrepareMissionSequence.cpp'] + [
                f'src/qtgui/tests/mission-command-names/{name}.json' for name in fixtures]
    refs += [str(path.relative_to(repo)) for path in sorted(
        (repo/'src/qtgui/tests/script-event-commands').glob('*.json'))]
    for path in sorted((repo/'src/qtgui/tests/script-event-backends').glob('*.json')):
        refs.append(str(path.relative_to(repo)))
        spec = json.loads(path.read_text())
        refs += list(spec.get('source_inputs', {})) + list(spec.get('physical_inputs', {}))
    refs += [str(path.relative_to(repo)) for path in sorted(
        (repo/'src/qtgui/tests/script-event-markers').glob('*.json'))]
    refs += [str(path.relative_to(repo)) for path in sorted(
        (repo/'src/qtgui/tests/script-event-starters').glob('*.json'))]
    refs = sorted(set(refs))
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'], reviews=[],
        reference_sources={name: hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in refs},
        scope='Runtime-only new ScriptEvent command contexts; full inventory and exact original-scope qualification remain separate.'), indent=2)+'\n')
    command = [sys.executable, str(Path(__file__).with_name('RunAcceptance.py')),
        '--build-dir', str(a.build_dir), '--filter', a.filter,
        '--nasa-review', str(review.relative_to(repo)), '--max-screenshots', '0',
        '--max-checkpoints', '200', '--pause-ms', str(a.pause_ms),
        '--output', str(a.output or root/('event-command-'+identity))]
    if a.visible:
        command += ['--visible', '--display-route', 'nested', '--display-scale', 'auto', '--start-minimized']
    if a.list:
        command.append('--list')
    return subprocess.call(command, cwd=repo)


if __name__ == '__main__':
    raise SystemExit(main())
