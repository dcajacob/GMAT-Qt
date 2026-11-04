"""Run only new/affected math-node main/private/Global witnesses."""
import argparse,hashlib,json,subprocess,sys,uuid
from pathlib import Path
import NasaGuiCoverage as nasa

def main():
    repo=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--visible',action='store_true');p.add_argument('--filter');p.add_argument('--output',type=Path);a=p.parse_args()
    identity=uuid.uuid4().hex[:12];stage=repo/'build/gui-acceptance';stage.mkdir(exist_ok=True)
    refs={'src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaFunctionMathResourceTests.cpp','src/qtgui/tests/NasaFunctionMathResourceCases.json','src/qtgui/tests/FormationAcceptance.hpp','src/qtgui/tests/MathNodeCloneTests.cpp','src/qtgui/tests/NasaMathNodeCloneCases.json','src/qtgui/tests/MathResourceCopyTests.cpp','src/qtgui/tests/NasaFunctionBuiltinTests.cpp','src/qtgui/tests/NasaFunctionBuiltinCases.json'}
    for base in [repo/'src/base',repo/'plugins']:
        refs.update(str(path.relative_to(repo)) for path in base.rglob('*') if path.is_file() and path.suffix in {'.cpp','.hpp'})
    review=stage/('function-math-resources-'+identity+'-review.json');review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={name:hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in sorted(refs)}),indent=2)+'\n')
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',a.filter or '^QtGui.NasaFunctionMathResource','--max-screenshots','0','--max-checkpoints','400','--pause-ms','0','--nasa-review',str(review.relative_to(repo)),'--output',str(a.output or stage/('function-math-resources-'+identity))]
    if a.visible:command+=['--visible','--start-minimized']
    return subprocess.call(command,cwd=repo)

if __name__=='__main__':raise SystemExit(main())
