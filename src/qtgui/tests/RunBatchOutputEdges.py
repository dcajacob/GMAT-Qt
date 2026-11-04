"""Replay five batch output edge cases, including rank-deficient Internal inversion."""
import argparse,hashlib,json,subprocess,sys,uuid
from pathlib import Path
import NasaGuiCoverage as nasa


def main():
    repo=Path(__file__).resolve().parents[3]
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--visible',action='store_true')
    p.add_argument('--output',type=Path);p.add_argument('--build-dir',type=Path,default=repo/'build/linux-gui')
    a=p.parse_args();identity=uuid.uuid4().hex[:12];stage=repo/'build/gui-acceptance';stage.mkdir(parents=True,exist_ok=True)
    names=['QtGui.NasaBatchOutputEdge'+n for n in ['Stop','Singular','IterationLimit','InvalidSolveFors','InternalRankDeficient']]
    files=['plugins/EstimationPlugin/src/base/command/RunEstimator.cpp','plugins/EstimationPlugin/src/base/estimator/BatchEstimatorBase.cpp','plugins/EstimationPlugin/src/base/estimator/BatchEstimator.cpp','plugins/EstimationPlugin/src/base/estimator/EstimationStateManager.cpp','src/base/spacecraft/Spacecraft.cpp','src/base/forcemodel/PointMassForce.cpp','src/qtgui/tests/NasaBatchOutputEdgeTests.cpp','src/qtgui/tests/FormationAcceptance.hpp','src/qtgui/tests/NasaBatchIterationCases.json','src/qtgui/CMakeLists.txt']
    files += [str(x.relative_to(repo)) for x in (repo/'src/qtgui/tests/batch-output-edges').glob('*.json')]
    review=stage/('batch-output-edges-'+identity+'-review.json')
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest() for n in files}),indent=2)+'\n')
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--build-dir',str(a.build_dir),'--filter','^('+'|'.join(names)+')$','--max-screenshots','0','--max-checkpoints','250','--pause-ms','0','--nasa-review',str(review.relative_to(repo)),'--output',str(a.output or stage/('batch-output-edges-'+identity))]
    if a.visible:command+=['--visible','--start-minimized']
    return subprocess.call(command,cwd=repo)


if __name__=='__main__':raise SystemExit(main())
