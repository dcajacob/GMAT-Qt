"""Run 9 hardware rotation reads and strict writable probes Parameter bindings;retain all321 types."""
import argparse,hashlib,json,subprocess,sys,uuid
from pathlib import Path
import NasaGuiCoverage as nasa


def main():
    repo=Path(__file__).resolve().parents[3];parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--visible',action='store_true');parser.add_argument('--filter');parser.add_argument('--output',type=Path);args=parser.parse_args()
    catalog_path=repo/'src/qtgui/tests/NasaFunctionRotationParameterCases.json';catalog=json.loads(catalog_path.read_text());types=catalog['initial_types'];arguments=catalog['selected_arguments']
    require_names=all(v.replace('_','').isalnum() for v in types);assert require_names and len(types)==9 and len(catalog['cases'])==321
    identity=uuid.uuid4().hex[:12];stage=repo/'build/gui-acceptance';stage.mkdir(exist_ok=True)
    refs={'src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaFunctionRotationParameterBindingTests.cpp','src/qtgui/tests/NasaFunctionRotationParameterCases.json','src/qtgui/tests/FormationAcceptance.hpp','src/qtgui/tests/NasaFunctionMathResourceTests.cpp','src/qtgui/tests/PlotStepFunctionOracle.hpp','src/qtgui/tests/NasaFunctionVectorResourceTests.cpp','src/qtgui/tests/NasaFunctionOwnedScopeTests.cpp',catalog['baseline'],catalog['discovery']}
    for base in ['src/base','src/gmatutil','plugins']:
        refs.update(str(p.relative_to(repo)) for p in (repo/base).rglob('*') if p.is_file() and p.suffix in {'.cpp','.hpp'})
    review=stage/('function-rotation-parameter-bindings-'+identity+'-review.json')
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest() for n in sorted(refs)}),indent=2)+'\n')
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',args.filter or '^QtGui.NasaFunctionRotationParameterBinding('+'|'.join(arguments)+')$','--max-screenshots','0','--max-checkpoints','500','--pause-ms','0','--nasa-review',str(review.relative_to(repo)),'--output',str(args.output or stage/('function-rotation-parameter-bindings-'+identity))]
    if args.visible:command+=['--visible','--start-minimized']
    return subprocess.call(command,cwd=repo)


if __name__=='__main__':
    raise SystemExit(main())
