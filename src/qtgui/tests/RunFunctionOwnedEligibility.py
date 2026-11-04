"""Run new/affected named main probes without inferring scope eligibility."""
import argparse,hashlib,json,subprocess,sys,uuid
from pathlib import Path
import NasaGuiCoverage as nasa

def main():
    repo=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--visible',action='store_true');p.add_argument('--filter');p.add_argument('--output',type=Path);a=p.parse_args()
    identity=uuid.uuid4().hex[:12];stage=repo/'build/gui-acceptance';stage.mkdir(exist_ok=True)
    refs={'src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaFunctionOwnedNamedTests.cpp','src/qtgui/tests/NasaFunctionOwnedNamedCases.json','src/qtgui/tests/FormationAcceptance.hpp'}
    for base in [repo/'src/base',repo/'plugins']:
        refs.update(str(path.relative_to(repo)) for path in base.rglob('*') if path.is_file() and path.suffix in {'.cpp','.hpp'})
    review=stage/('function-owned-eligibility-'+identity+'-review.json')
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={name:hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in sorted(refs)}),indent=2)+'\n')
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',a.filter or '^QtGui.NasaFunctionOwnedNamed','--max-screenshots','0','--max-checkpoints','400','--pause-ms','0','--nasa-review',str(review.relative_to(repo)),'--output',str(a.output or stage/('function-owned-eligibility-'+identity))]
    if a.visible:command+=['--visible','--start-minimized']
    return subprocess.call(command,cwd=repo)

if __name__=='__main__':raise SystemExit(main())
