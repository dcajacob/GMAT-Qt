"""Replay A1 summary acceptance, optionally with its two affected regressions.

Create a fresh runtime-only source manifest; qualification remains a separate
source-versioned audit. Existing NASA receipts/manifests are never rewritten.
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
    p.add_argument('--build-dir', type=Path, default=repo / 'build/linux-gui')
    p.add_argument('--output', type=Path)
    p.add_argument('--visible', action='store_true')
    p.add_argument('--include-affected', action='store_true')
    p.add_argument('--pause-ms', type=int, default=350)
    p.add_argument('--list', action='store_true')
    a = p.parse_args()
    identity = uuid.uuid4().hex[:12]
    root = repo / 'build/gui-acceptance'
    review = root / ('summary-a1-' + identity + '-runtime-review.json')
    root.mkdir(parents=True, exist_ok=True)
    refs = ['src/base/command/GmatCommand.cpp', 'src/base/command/GmatCommand.hpp',
            'src/qtgui/MainWindow.cpp', 'src/qtgui/MainWindow.hpp',
            'src/qtgui/InspectionDialog.cpp', 'src/qtgui/InspectionDialog.hpp',
            'src/qtgui/tests/NasaSummaryA1Tests.cpp', 'src/qtgui/tests/NasaCommandSummaryTests.cpp']
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'], reviews=[],
        reference_sources={name: hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in refs},
        scope='Runtime replay only; no new NASA credit. Original cumulative source-versioned coverage is a separate audited union.'), indent=2)+'\n')
    pattern = '^QtGui\\.(NasaSummaryA1|NasaCommandSummaryFields|Inspection)$' if a.include_affected else '^QtGui.NasaSummaryA1$'
    command = [sys.executable, str(Path(__file__).with_name('RunAcceptance.py')),
               '--build-dir', str(a.build_dir), '--filter', pattern,
               '--nasa-review', str(review.relative_to(repo)), '--max-screenshots', '0',
               '--max-checkpoints', '200', '--pause-ms', str(a.pause_ms),
               '--output', str(a.output or root / ('summary-a1-' + identity))]
    if a.visible:
        command += ['--visible', '--display-route', 'nested', '--display-scale', 'auto', '--start-minimized']
    if a.list:
        command.append('--list')
    return subprocess.call(command, cwd=repo)


if __name__ == '__main__':
    raise SystemExit(main())
