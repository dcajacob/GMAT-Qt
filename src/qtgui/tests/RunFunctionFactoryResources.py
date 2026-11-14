"""Run additional factory resource types through the unchanged local-resource workflow."""
import argparse,hashlib,json,subprocess,sys,uuid
from pathlib import Path
import NasaGuiCoverage as nasa


def main():
    repo=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser(description=__doc__);p.add_argument('--visible',action='store_true');p.add_argument('--with-regression',action='store_true');p.add_argument('--filter');p.add_argument('--output',type=Path);a=p.parse_args();identity=uuid.uuid4().hex[:12];stage=repo/'build/gui-acceptance';stage.mkdir(exist_ok=True)
    files=['src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaFunctionAdditionalFactoryCases.json','src/qtgui/tests/NasaFunctionExtendedResourceCases.json','plugins/ExtendedKalmanFilterPlugin/src/base/estimatedparam/EstimatedParameter.cpp','plugins/ExtendedKalmanFilterPlugin/src/base/estimatedparam/EstimatedParameter.hpp','src/qtgui/tests/NasaFunctionResourceTests.cpp','src/base/spacecraft/Plate.cpp','src/base/factory/PlateFactory.cpp','plugins/ExtendedKalmanFilterPlugin/src/base/estimatedparam/FirstOrderGaussMarkov.cpp','plugins/ExtendedKalmanFilterPlugin/src/base/estimatedparam/FirstOrderGaussMarkov.hpp','plugins/ExtendedKalmanFilterPlugin/src/base/estimatedparam/EstimatedParameterModel.cpp','plugins/ExtendedKalmanFilterPlugin/src/base/estimatedparam/EstimatedParameterModel.hpp','src/qtgui/tests/FirstOrderGaussMarkovCopyTests.cpp','src/base/command/Create.cpp','src/base/function/FunctionManager.cpp','src/gmatutil/util/FileUtil.cpp']
    review=stage/('function-factory-'+identity+'-review.json');review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest() for n in files}),indent=2)+'\n')
    names=['QtGui.NasaFunctionFactoryPlate','QtGui.NasaFunctionFactoryFirstOrderGaussMarkov']
    if a.with_regression:names.append('QtGui.NasaFunctionResourceExtendedEstimatedParameter')
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',a.filter or '^('+'|'.join(names)+')$','--max-screenshots','0','--max-checkpoints','200','--pause-ms','0','--nasa-review',str(review.relative_to(repo)),'--output',str(a.output or stage/('function-factory-'+identity))]
    if a.visible:command+=['--visible','--start-minimized']
    return subprocess.call(command,cwd=repo)


if __name__=='__main__':raise SystemExit(main())
