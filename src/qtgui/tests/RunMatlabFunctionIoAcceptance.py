"""Run real main/private/nested MATLAB typed I/O constituents; no row admission."""
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
    parser.add_argument('--filter', default='^QtGui.NasaMatlabFunctionIo')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    refs = {'src/qtgui/CMakeLists.txt', 'src/qtgui/tests/NasaMatlabFunctionIoTests.cpp',
            'src/qtgui/tests/MatlabAcceptanceStartup.hpp', 'src/qtgui/tests/FormationAcceptance.hpp'}
    for root in ['src/base', 'src/gmatutil', 'plugins']:
        refs.update(str(p.relative_to(repo)) for p in (repo/root).rglob('*')
                    if p.is_file() and p.suffix in {'.cpp', '.hpp'})
    review = repo/'build/gui-acceptance'/('matlab-function-io-'+uuid.uuid4().hex[:12]+'-review.json')
    review.parent.mkdir(exist_ok=True)
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'], reviews=[],
        reference_sources={p: hashlib.sha256((repo/p).read_bytes()).hexdigest() for p in sorted(refs)},
        scope='Typed main/private/nested calls; preserve original FRR36 ambiguity and all full-row gates.'), indent=2)+'\n')
    command = [sys.executable, str(repo/'src/qtgui/tests/RunAcceptance.py'), '--filter', args.filter,
               '--nasa-review', str(review.relative_to(repo)), '--max-screenshots', '0',
               '--max-checkpoints', '500', '--pause-ms', '150' if args.visible else '0',
               '--output', str(args.output)]
    if args.visible:
        command += ['--visible', '--display-route', 'nested', '--display-scale', 'auto', '--start-minimized']
    return subprocess.call(command, cwd=repo)


if __name__ == '__main__':
    raise SystemExit(main())
