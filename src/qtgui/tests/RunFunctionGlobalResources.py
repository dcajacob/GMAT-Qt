"""Run only selected new/affected global-resource identity workflows."""
import argparse,hashlib,json,subprocess,sys,uuid
from pathlib import Path
import NasaGuiCoverage as nasa


def main():
    repo=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser(description=__doc__);p.add_argument('--visible',action='store_true');p.add_argument('--filter');p.add_argument('--output',type=Path);a=p.parse_args();identity=uuid.uuid4().hex[:12];stage=repo/'build/gui-acceptance';stage.mkdir(exist_ok=True)
    fixture='src/qtgui/tests/NasaFunctionGlobalResourceCases.json';catalog=json.loads((repo/fixture).read_text());refs={'src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaFunctionGlobalResourceTests.cpp',fixture,'src/qtgui/tests/FormationAcceptance.hpp','application/samples/Navigation/Ex_Estimate_RangeSkin.script'};refs.update(v['path'] for v in catalog['source_catalogs']);refs.update(v['native_source'] for v in catalog['cases'] if 'native_source' in v)
    classes={v['type'] for v in catalog['cases']}|{'GmatBase','Moderator','Sandbox','FactoryManager','Factory','Interpreter','ScriptInterpreter','FunctionManager','GmatFunction','ObjectManagedFunction','UserDefinedFunction','Global','ManageObject','CallFunction','CallGmatFunction','Assignment','Parameter','ParameterWrapper','ElementWrapper','ObjectWrapper','Hardware','RFHardware','Sensor','FieldOfView','Thruster','FuelTank','PowerSystem','Solver','Subscriber','OrbitPlot','EstimatedParameterModel','YukonOptimizer','ProcessNoiseBase'}
    refs.add('src/qtgui/tests/NasaFunctionGlobalUserResourceCases.json')
    classes.update({'Validator','ParameterInfo','ObjectPropertyWrapper','String','Variable','Array'})
    for base in [repo/'src/base',repo/'plugins']:
        for path in base.rglob('*'):
            if path.is_file() and path.suffix in {'.cpp','.hpp'} and (path.stem in classes or path.stem.endswith('Factory')):refs.add(str(path.relative_to(repo)))
    review=stage/('function-global-resources-'+identity+'-review.json');review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={name:hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in sorted(refs)}),indent=2)+'\n')
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',a.filter or '^QtGui.NasaFunctionGlobal(Resource|Array)','--max-screenshots','0','--max-checkpoints','400','--pause-ms','0','--nasa-review',str(review.relative_to(repo)),'--output',str(a.output or stage/('function-global-resources-'+identity))]
    if a.visible:command+=['--visible','--start-minimized']
    return subprocess.call(command,cwd=repo)


if __name__=='__main__':raise SystemExit(main())
