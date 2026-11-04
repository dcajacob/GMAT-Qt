"""Run actual Mission Tree MATLAB-call creation/configuration; no automatic row admission."""
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
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--visible', action='store_true')
    parser.add_argument('--filter', default='^QtGui.NasaMatlabMission(Call|Catalogue)')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    refs = {'src/qtgui/CMakeLists.txt', 'src/qtgui/tests/NasaMatlabMissionCallTests.cpp',
            'src/qtgui/tests/MatlabAcceptanceStartup.hpp', 'src/qtgui/tests/FormationAcceptance.hpp',
            'src/qtgui/tests/NasaSolverMenuCatalogueTests.cpp', 'src/qtgui/tests/RunMatlabMissionCatalogueTest.py',
            'src/qtgui/MainWindow.cpp','src/qtgui/tests/NasaMatlabScriptEventCallTests.cpp'}
    for root in ['src/base', 'src/gmatutil', 'plugins']:
        refs.update(str(p.relative_to(repo)) for p in (repo/root).rglob('*')
                    if p.is_file() and p.suffix in {'.cpp', '.hpp'})
    review = repo/'build/gui-acceptance'/('matlab-mission-call-'+uuid.uuid4().hex[:12]+'-review.json')
    review.parent.mkdir(exist_ok=True)
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'], reviews=[],
        reference_sources={p: hashlib.sha256((repo/p).read_bytes()).hexdigest() for p in sorted(refs)},
        scope='Actual Mission Tree MATLAB calls in ordinary and branch positions; preserve all original whole-row and broader MATLAB gates.'), indent=2)+'\n')
    command = [sys.executable, str(repo/'src/qtgui/tests/RunAcceptance.py'), '--filter', args.filter,
               '--nasa-review', str(review.relative_to(repo)), '--max-screenshots', '0',
               '--max-checkpoints', '500', '--pause-ms', '150' if args.visible else '0',
               '--output', str(args.output)]
    if args.visible:
        command += ['--visible', '--display-route', 'nested', '--display-scale', 'auto', '--start-minimized']
    return subprocess.call(command, cwd=repo)


if __name__ == '__main__':
    raise SystemExit(main())
