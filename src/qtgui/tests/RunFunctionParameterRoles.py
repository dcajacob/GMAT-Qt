"""Record native factory parameter roles without assigning function-scope credit."""
import argparse,hashlib,json,subprocess,sys,uuid
from pathlib import Path
import NasaGuiCoverage as nasa


def main():
    repo=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser(description=__doc__);p.add_argument('--visible',action='store_true');p.add_argument('--with-vectors',action='store_true');p.add_argument('--filter');p.add_argument('--output',type=Path);a=p.parse_args();identity=uuid.uuid4().hex[:12];stage=repo/'build/gui-acceptance';stage.mkdir(exist_ok=True)
    refs=['src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaFunctionParameterRolesTests.cpp','src/qtgui/tests/FormationAcceptance.hpp','src/base/factory/FactoryManager.cpp','src/base/factory/ParameterFactory.cpp','src/base/foundation/GmatType.cpp','src/base/executive/Moderator.cpp','src/base/interpreter/Interpreter.cpp','src/base/interpreter/ScriptInterpreter.cpp','src/base/command/ManageObject.cpp','src/base/function/UserDefinedFunction.cpp','plugins/GmatFunctionPlugin/src/base/command/Global.cpp','plugins/GmatFunctionPlugin/src/base/function/GmatFunction.cpp']
    files=set(refs)
    files.add('src/qtgui/tests/NasaFunctionVectorResourceTests.cpp')
    files.update(['src/base/command/Assignment.cpp','src/base/foundation/ElementWrapper.cpp','src/base/foundation/GmatBase.cpp','src/base/configs/ConfigManager.cpp'])
    for base in [repo/'src/base/parameter',repo/'plugins']:
        for path in base.rglob('*'):
            if path.is_file() and (base.name=='parameter' or 'parameter' in path.parts or path.name.endswith('ParameterFactory.cpp')) and path.suffix in {'.cpp','.hpp'}:files.add(str(path.relative_to(repo)))
    review=stage/('function-parameter-roles-'+identity+'-review.json');review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={name:hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in sorted(files)}),indent=2)+'\n')
    selection='^QtGui.NasaFunction(ParameterRoles|VectorResource)$' if a.with_vectors else '^QtGui.NasaFunctionParameterRoles$'
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',a.filter or selection,'--max-screenshots','0','--max-checkpoints','200','--pause-ms','0','--nasa-review',str(review.relative_to(repo)),'--output',str(a.output or stage/('function-parameter-roles-'+identity))]
    if a.visible:command+=['--visible','--start-minimized']
    return subprocess.call(command,cwd=repo)


if __name__=='__main__':raise SystemExit(main())
