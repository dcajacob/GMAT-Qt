"""Capture the complete loaded native factory/type map; no requirement admission."""
import argparse,hashlib,json,subprocess,sys,uuid
from pathlib import Path
import NasaGuiCoverage as nasa


def main():
    repo=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser(description=__doc__);p.add_argument('--visible',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args();identity=uuid.uuid4().hex[:12];stage=repo/'build/gui-acceptance';stage.mkdir(exist_ok=True)
    files=['src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaFunctionResourceCensusTests.cpp','src/qtgui/tests/FormationAcceptance.hpp','src/base/factory/FactoryManager.cpp','src/base/factory/Factory.cpp','src/base/executive/Moderator.cpp','src/base/interpreter/Interpreter.cpp','src/base/interpreter/ScriptInterpreter.cpp','src/base/foundation/GmatType.cpp']
    review=stage/('function-resource-census-'+identity+'-review.json');review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest() for n in files}),indent=2)+'\n')
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter','^QtGui.NasaFunctionResourceCensus$','--max-screenshots','0','--max-checkpoints','20','--pause-ms','0','--nasa-review',str(review.relative_to(repo)),'--output',str(a.output or stage/('function-resource-census-'+identity))]
    if a.visible:command+=['--visible','--start-minimized']
    return subprocess.call(command,cwd=repo)


if __name__=='__main__':raise SystemExit(main())
