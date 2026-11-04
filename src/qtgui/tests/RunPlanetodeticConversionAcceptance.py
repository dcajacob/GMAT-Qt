"""Replay only the new Planetodetic GUI conversion cases, with optional viewing.

This creates a runtime-only review; the cumulative NASA qualification stays
separate. It never copies a runtime or modifies existing passing receipts.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import uuid
import NasaGuiCoverage as nasa


if __name__ == '__main__':
    repo = Path(__file__).resolve().parents[3]
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-dir', type=Path, default=repo/'build/linux-gui')
    p.add_argument('--visible', action='store_true')
    p.add_argument('--pause-ms', type=int, default=350)
    p.add_argument('--filter', default='^QtGui.NasaPlanetodeticConversion')
    p.add_argument('--output', type=Path)
    p.add_argument('--list', action='store_true')
    a = p.parse_args(); identity = uuid.uuid4().hex[:12]
    root = repo/'build/gui-acceptance'; root.mkdir(parents=True, exist_ok=True)
    review = root/('planet-conversion-'+identity+'-runtime-review.json')
    refs = ['src/qtgui/tests/NasaPlanetodeticConversionTests.cpp',
            'src/qtgui/tests/NasaPlanetodeticConversionCases.json',
            'src/qtgui/tests/PlanetodeticConversionOracle.py',
            'src/qtgui/tests/PlanetodeticOracle.py',
            'src/qtgui/SpacecraftOrbit.cpp', 'src/gmatutil/util/StateConversionUtil.cpp']
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'], reviews=[],
        reference_sources={name:hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in refs},
        scope='Runtime-only new Planetodetic conversion replay. Whole FRR!E13 and cumulative original-scope qualification remain separate.'),indent=2)+'\n')
    command = [sys.executable, str(Path(__file__).with_name('RunAcceptance.py')),
        '--build-dir', str(a.build_dir), '--filter', a.filter,
        '--nasa-review', str(review.relative_to(repo)), '--max-screenshots', '0',
        '--max-checkpoints', '200', '--pause-ms', str(a.pause_ms),
        '--output', str(a.output or root/('planet-conversion-'+identity))]
    if a.visible: command += ['--visible', '--display-route', 'nested', '--display-scale', 'auto', '--start-minimized']
    if a.list: command.append('--list')
    raise SystemExit(subprocess.call(command, cwd=repo))
