"""Run only new/affected remaining Parameter-owner cases with fresh source review."""
import argparse,hashlib,json,subprocess,sys,uuid
from pathlib import Path
import NasaGuiCoverage as nasa
def main():
    repo=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser(description=__doc__);p.add_argument('--visible',action='store_true');p.add_argument('--filter');p.add_argument('--output',type=Path);p.add_argument('--list',action='store_true');args=p.parse_args()
    selection=args.filter or'^QtGui.NasaFunctionEnvironmentBinding.*Read$'
    if args.list:return subprocess.call(['ctest','--test-dir','build/linux-gui','-N','-R',selection],cwd=repo)
    catalog=repo/'src/qtgui/tests/NasaFunctionEnvironmentCases.json';data=json.loads(catalog.read_text());assert len(data['cases'])==321 and len(data['environment_binding_cases'])==23
    identity=uuid.uuid4().hex[:12];directory=repo/'build/gui-acceptance';directory.mkdir(exist_ok=True);review=directory/('function-environment-'+identity+'-review.json')
    refs={str(f.relative_to(repo))for root in ['src/base','src/gmatutil','plugins']for f in(repo/root).rglob('*')if f.is_file()and f.suffix in {'.c','.cpp','.h','.hpp','.inc'}}
    refs.update(['src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaFunctionEnvironmentBindingTests.cpp','src/qtgui/tests/FunctionEnvironmentBindingOracle.py','src/qtgui/tests/PrepareFunctionEnvironmentBindings.py','src/qtgui/tests/NasaFunctionEnvironmentCases.json','src/qtgui/tests/FormationAcceptance.hpp',data['baseline'],data['worklist']])
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={name:hashlib.sha256((repo/name).read_bytes()).hexdigest()for name in sorted(refs)},scope='All23 remaining live Parameter-owner routes in preparation;zero whole-row qualification.'),indent=2)+'\n')
    command=[sys.executable,'src/qtgui/tests/RunAcceptance.py','--filter',selection,'--nasa-review',str(review.relative_to(repo)),'--max-screenshots','0','--max-checkpoints','500','--pause-ms','0','--output',str(args.output or directory/('function-environment-'+identity))]
    if args.visible:command+=['--visible','--display-route','nested','--display-scale','auto','--start-minimized']
    return subprocess.call(command,cwd=repo)
if __name__=='__main__':raise SystemExit(main())
