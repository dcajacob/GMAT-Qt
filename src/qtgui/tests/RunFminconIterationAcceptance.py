"""Run actual MATLAB fmincon iterations,limits and progress/report truth; no automatic row admission."""
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
    parser.add_argument('--filter', default='^QtGui.NasaFminconIteration')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    refs = {'src/qtgui/tests/NasaFminconIterationTests.cpp','src/qtgui/CommandForm.cpp','src/qtgui/CommandEditor.cpp','src/qtgui/tests/NasaFminconVaryTests.cpp','src/qtgui/ResourceProperties.cpp','src/qtgui/ResourceProperties.hpp',
        'src/qtgui/ResourceEditor.cpp','src/qtgui/tests/NasaFminconResourceTests.cpp','src/qtgui/CMakeLists.txt','src/qtgui/QtInterpreter.cpp','src/qtgui/QtInterpreter.hpp',
        'src/qtgui/QtMatlabCallbackServer.cpp','src/qtgui/QtMatlabCallbackServer.hpp',
        'src/qtgui/MainWindow.cpp','src/qtgui/tests/NasaFminconCallbackStartupTests.cpp',
        'src/qtgui/tests/NasaFminconSolveTests.cpp','src/qtgui/tests/NasaMatlabLibraryWriterTests.cpp',
        'src/qtgui/tests/NasaFminconProtocolTests.cpp',
        'src/qtgui/tests/MatlabAcceptanceStartup.hpp','src/qtgui/tests/FormationAcceptance.hpp'}
    for root in ['src/base','src/gmatutil','plugins','application/matlab']:
        refs.update(str(p.relative_to(repo)) for p in (repo/root).rglob('*')
            if p.is_file() and p.suffix in {'.cpp','.hpp','.m'})
    review = repo/'build/gui-acceptance'/('fmincon-iterations-'+uuid.uuid4().hex[:12]+'-review.json')
    review.parent.mkdir(exist_ok=True)
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],
        reference_sources={p:hashlib.sha256((repo/p).read_bytes()).hexdigest() for p in sorted(refs)},
        scope='Actual Qt/fmincon startup and callback integration; no whole-row credit without independent complete original-scope admission.'),indent=2)+'\n')
    cmd = [sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',args.filter,
        '--nasa-review',str(review.relative_to(repo)),'--max-screenshots','0','--max-checkpoints','500',
        '--pause-ms','150' if args.visible else '0','--output',str(args.output)]
    if args.visible:
        cmd += ['--visible','--display-route','nested','--display-scale','auto','--start-minimized']
    return subprocess.call(cmd,cwd=repo)


if __name__ == '__main__':
    raise SystemExit(main())
