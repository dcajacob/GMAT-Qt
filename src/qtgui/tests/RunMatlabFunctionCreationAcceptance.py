"""Exercise real Resource Tree MATLAB-function creation; no inferred row credit."""
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
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    identity = uuid.uuid4().hex[:12]
    refs = {'src/qtgui/tests/NasaMatlabFunctionCreationTests.cpp',
            'src/qtgui/tests/FormationAcceptance.hpp', 'src/qtgui/CMakeLists.txt',
            'plugins/MatlabInterfacePlugin/src/base/CMakeLists.txt',
            'src/base/function/Function.cpp', 'src/base/function/Function.hpp',
            'src/base/executive/Moderator.cpp', 'src/base/interpreter/ScriptInterpreter.cpp',
            'src/gmatutil/util/FileManager.cpp', 'src/gmatutil/util/GmatGlobal.cpp'}
    base = repo/'plugins/MatlabInterfacePlugin/src/base'
    refs.update(str(p.relative_to(repo)) for p in base.rglob('*')
                if p.is_file() and p.suffix in {'.cpp', '.hpp'})
    review = repo/'build/gui-acceptance'/('matlab-function-creation-'+identity+'-review.json')
    review.parent.mkdir(exist_ok=True)
    review.write_text(json.dumps({'sources': nasa.inventory(repo)['sources'], 'reviews': [],
        'reference_sources': {p: hashlib.sha256((repo/p).read_bytes()).hexdigest() for p in sorted(refs)},
        'scope': 'Actual MATLAB function resource creation/configuration; independent two-scalar call witness. No automatic complete-row admission.'}, indent=2)+'\n')
    command = [sys.executable, str(repo/'src/qtgui/tests/RunAcceptance.py'),
               '--filter', '^QtGui.NasaMatlabFunctionCreation$', '--nasa-review', str(review.relative_to(repo)),
               '--max-screenshots', '0', '--max-checkpoints', '500', '--pause-ms', '150' if args.visible else '0',
               '--output', str(args.output or repo/'build/gui-acceptance'/('matlab-function-creation-'+identity))]
    if args.visible:
        command += ['--visible', '--display-route', 'nested', '--display-scale', 'auto', '--start-minimized']
    return subprocess.call(command, cwd=repo)


if __name__ == '__main__':
    raise SystemExit(main())
