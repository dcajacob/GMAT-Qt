"""Run the new function-local navigation ownership workflow without awarding a parent row."""
import argparse,hashlib,json,subprocess,sys,uuid
from pathlib import Path
import NasaGuiCoverage as nasa


def main():
    repo=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--visible',action='store_true');p.add_argument('--with-regressions',action='store_true');p.add_argument('--filter');p.add_argument('--output',type=Path);p.add_argument('--build-dir',type=Path,default=repo/'build/linux-gui');a=p.parse_args();identity=uuid.uuid4().hex[:12];stage=repo/'build/gui-acceptance';stage.mkdir(exist_ok=True)
    files=['src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaFunctionNavigationScopeTests.cpp','src/qtgui/tests/FormationAcceptance.hpp','src/qtgui/tests/script-event-backends/Navigation.json','src/qtgui/tests/ScriptEventBackendOracle.py','src/base/function/FunctionManager.cpp','src/base/command/Create.cpp','src/base/executive/Sandbox.cpp','src/base/command/BranchCommand.cpp','plugins/GmatFunctionPlugin/src/base/function/GmatFunction.cpp','plugins/EstimationPlugin/src/base/command/RunSimulator.cpp','plugins/EstimationPlugin/src/base/command/RunEstimator.cpp','plugins/ExtendedKalmanFilterPlugin/src/base/command/RunSmoother.cpp','application/samples/Navigation/Ex_FilterSmoother_GpsPosVec.script']
    review=stage/('function-navigation-'+identity+'-review.json');review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest() for n in files}),indent=2)+'\n')
    names=['QtGui.NasaFunctionNavigationScope'+v for v in ['', 'NestedBranch','Expression']]
    if a.with_regressions:names+=['QtGui.NasaFunctionControlMutual','QtGui.NativeNasaFunctionPropagatorGlobalExisting','QtGui.NasaScriptEventBackendNavigation']
    cmd=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--build-dir',str(a.build_dir),'--filter',a.filter or '^('+'|'.join(names)+')$','--max-screenshots','0','--max-checkpoints','300','--pause-ms','0','--nasa-review',str(review.relative_to(repo)),'--output',str(a.output or stage/('function-navigation-'+identity))]
    if a.visible:cmd+=['--visible','--start-minimized']
    return subprocess.call(cmd,cwd=repo)


if __name__=='__main__':raise SystemExit(main())
