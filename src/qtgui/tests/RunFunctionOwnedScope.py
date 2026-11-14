"""Run main and function-declared PropSetup/CoordinateSystem creation and use."""
import argparse, hashlib, json, subprocess, sys, uuid
from pathlib import Path
import NasaGuiCoverage as nasa


def main():
    repo = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--visible', action='store_true')
    parser.add_argument('--filter')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    identity = uuid.uuid4().hex[:12]
    stage = repo/'build/gui-acceptance'; stage.mkdir(exist_ok=True)
    refs = {'src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaFunctionOwnedScopeTests.cpp',
            'src/qtgui/tests/FormationAcceptance.hpp','src/qtgui/tests/NasaFunctionMathResourceTests.cpp',
            'src/qtgui/tests/NasaFunctionPropagatorGlobalTests.cpp','src/qtgui/tests/NasaFunctionFrameTests.cpp'}
    for base in ['src/base','src/gmatutil','plugins']:
        refs.update(str(p.relative_to(repo)) for p in (repo/base).rglob('*') if p.is_file() and p.suffix in {'.cpp','.hpp'})
    review = stage/('function-owned-scope-'+identity+'-review.json')
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest() for n in sorted(refs)}),indent=2)+'\n')
    command = [sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',args.filter or '^QtGui.NasaFunctionOwned(PropSetupStoppedTime|CoordinateSystem)$','--max-screenshots','0','--max-checkpoints','400','--pause-ms','0','--nasa-review',str(review.relative_to(repo)),'--output',str(args.output or stage/('function-owned-scope-'+identity))]
    if args.visible: command += ['--visible','--start-minimized']
    return subprocess.call(command,cwd=repo)


if __name__ == '__main__':
    raise SystemExit(main())
