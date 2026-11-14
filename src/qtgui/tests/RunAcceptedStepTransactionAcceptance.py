"""Run only the native accepted-step transaction matrix; visible starts minimized.

This is a producer contract, not a completed integration-step plotting claim.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import uuid
import NasaGuiCoverage as nasa


def main():
    repo=Path(__file__).resolve().parents[3]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=repo/'build/linux-gui')
    parser.add_argument('--visible',action='store_true')
    parser.add_argument('--pause-ms',type=int,default=350)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--list',action='store_true')
    args=parser.parse_args();selection='^QtGui.AcceptedStepTransactions$'
    if args.list:return subprocess.call(['ctest','--test-dir',str(args.build_dir),'-N','-R',selection],cwd=repo)
    identity=uuid.uuid4().hex[:12];output=args.output or repo/'build/gui-acceptance'/('accepted-step-transactions-'+identity)
    review=repo/'build/gui-acceptance'/('accepted-step-transactions-'+identity+'-runtime-review.json');review.parent.mkdir(parents=True,exist_ok=True)
    refs=['src/base/propagator/AcceptedStepHistory.cpp','src/base/propagator/AcceptedStepHistory.hpp',
          'src/base/propagator/RungeKutta.cpp','src/base/propagator/RungeKuttaNystrom.cpp','src/base/propagator/PredictorCorrector.cpp',
          'plugins/ExtraPropagatorsPlugin/src/base/propagator/BulirschStoer.cpp',
          'src/qtgui/tests/AcceptedStepTransactionTests.cpp','src/qtgui/tests/FormationAcceptance.hpp']
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],
        reference_sources={name:hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in refs},
        scope='Native accepted-primitive snapshot/commit contract; zero new NASA or GUI-mechanism credit.'),indent=2)+'\n')
    command=[sys.executable,str(Path(__file__).with_name('RunAcceptance.py')),'--build-dir',str(args.build_dir),
             '--filter',selection,'--nasa-review',str(review.relative_to(repo)),'--max-screenshots','0',
             '--max-checkpoints','200','--pause-ms',str(args.pause_ms),'--output',str(output)]
    if args.visible:command+=['--visible','--display-route','nested','--display-scale','auto','--start-minimized']
    return subprocess.call(command,cwd=repo)


if __name__=='__main__':raise SystemExit(main())
