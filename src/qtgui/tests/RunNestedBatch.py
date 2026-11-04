"""Replay nested batch output; optionally include the open unconstrained VF13ad accuracy gate."""
import argparse,hashlib,json,subprocess,sys,uuid
from pathlib import Path
import NasaGuiCoverage as nasa


def main():
    repo=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--visible',action='store_true');p.add_argument('--include-unconstrained-conflict',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--build-dir',type=Path,default=repo/'build/linux-gui');a=p.parse_args();identity=uuid.uuid4().hex[:12];stage=repo/'build/gui-acceptance';stage.mkdir(exist_ok=True)
    names=['QtGui.NasaNestedBatch'+x for x in ['TargetDC','OptimizeYukon','OptimizeVF13ad']]
    if a.include_unconstrained_conflict:names.append('QtGui.NasaNestedBatchOptimizeVF13adUnconstrained')
    files=['src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaNestedBatchTests.cpp','src/qtgui/tests/NasaBatchOutputEdgeTests.cpp','src/qtgui/tests/FormationAcceptance.hpp','plugins/EstimationPlugin/src/base/command/RunSimulator.cpp','plugins/EstimationPlugin/src/base/estimator/Simulator.cpp','src/base/subscriber/EphemerisFile.cpp','src/base/command/SolverBranchCommand.cpp'];files += [str(x.relative_to(repo)) for x in (repo/'src/qtgui/tests/nested-batch').glob('*.json')]
    review=stage/('nested-batch-'+identity+'-review.json');review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest() for n in files}),indent=2)+'\n')
    cmd=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--build-dir',str(a.build_dir),'--filter','^('+'|'.join(names)+')$','--max-screenshots','0','--max-checkpoints','500','--pause-ms','0','--nasa-review',str(review.relative_to(repo)),'--output',str(a.output or stage/('nested-batch-'+identity))]
    if a.visible:cmd+=['--visible','--start-minimized']
    return subprocess.call(cmd,cwd=repo)


if __name__=='__main__':raise SystemExit(main())
