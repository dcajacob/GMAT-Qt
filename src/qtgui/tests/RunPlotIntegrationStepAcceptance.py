"""Run only the native plot integration-step consumer matrix; visible starts minimized.

This is bounded consumer evidence, not complete NASA requirement qualification.
"""
import argparse
import os
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
    parser.add_argument('--cases',help='Comma-separated new or affected case names')
    parser.add_argument('--pause-ms',type=int,default=350)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--list',action='store_true')
    args=parser.parse_args();selection='^QtGui.PlotIntegrationSteps$'
    if args.list:return subprocess.call(['ctest','--test-dir',str(args.build_dir),'-N','-R',selection],cwd=repo)
    identity=uuid.uuid4().hex[:12];output=args.output or repo/'build/gui-acceptance'/('plot-integration-steps-'+identity)
    review=repo/'build/gui-acceptance'/('plot-integration-steps-'+identity+'-runtime-review.json');review.parent.mkdir(parents=True,exist_ok=True)
    refs=['src/base/propagator/AcceptedStepHistory.cpp','src/base/propagator/AcceptedStepHistory.hpp',
          'src/base/propagator/RungeKutta.cpp','src/base/propagator/RungeKuttaNystrom.cpp','src/base/propagator/PredictorCorrector.cpp',
          'plugins/ExtraPropagatorsPlugin/src/base/propagator/BulirschStoer.cpp',
          'src/qtgui/tests/PlotIntegrationStepTests.cpp', 'src/qtgui/tests/PlotStepLiveObservation.hpp', 'src/qtgui/tests/PlotStepCadenceObservation.hpp', 'src/qtgui/tests/PlotStepBurnOracle.hpp', 'src/qtgui/tests/PlotStepEphemerisOracle.hpp', 'src/qtgui/tests/PlotStepLifetimeOracle.hpp', 'src/qtgui/tests/PlotStepRedrawOracle.hpp', 'src/qtgui/tests/PlotStepBoundaryOracle.hpp', 'src/qtgui/tests/PlotStepFunctionOracle.hpp', 'src/base/foundation/ObjectInitializer.cpp', 'src/base/executive/PlotOriginScope.hpp', 'src/qtgui/tests/PlotStepOriginOracle.hpp', 'src/base/spacecraft/SpaceObject.cpp', 'src/qtgui/QtPlotReceiver.cpp', 'src/base/subscriber/GroundTrack.cpp', 'src/base/subscriber/GroundTrack.hpp', 'src/base/subscriber/OrbitPlot.cpp', 'src/base/subscriber/OrbitPlot.hpp', 'src/base/subscriber/IntegrationStepSampler.hpp', 'src/qtgui/GroundTrackDialog.cpp', 'src/qtgui/OrbitViewDialog.cpp', 'src/base/command/Propagate.cpp', 'src/base/command/Propagate.hpp', 'src/base/executive/Publisher.cpp', 'src/base/executive/Publisher.hpp', 'src/base/forcemodel/ODEModel.cpp', 'src/base/forcemodel/ODEModel.hpp', 'src/base/command/SolverBranchCommand.cpp','src/base/subscriber/Subscriber.cpp','src/qtgui/tests/FormationAcceptance.hpp']
    refs += ['src/qtgui/tests/PlotStepLacOracle.hpp','src/base/coordsystem/LocalAlignedConstrainedAxes.cpp','src/base/coordsystem/CoordinateSystem.cpp','src/base/coordsystem/AxisSystem.cpp', 'src/qtgui/tests/PlotStepRotatingOracle.hpp','src/base/forcemodel/PointMassForce.cpp','src/base/forcemodel/PointMassForce.hpp','src/base/coordsystem/ObjectReferencedAxes.cpp']
    providers='plugins/EphemPropagatorPlugin/src/base/propagator/'
    refs += ['src/qtgui/tests/PlotStepGravityOracle.hpp','src/base/forcemodel/GravityField.cpp','src/base/forcemodel/GravityField.hpp','src/base/forcemodel/HarmonicField.cpp','src/base/forcemodel/HarmonicField.hpp']
    refs += [providers+name+'.cpp' for name in ('EphemerisPropagator','SPKPropagator','CcsdsEphPropagator','StkEPropagator','Code500Propagator')]
    refs += ['src/gmatutil/util/Code500EphemerisFile.cpp','src/gmatutil/util/Code500EphemerisFile.hpp']
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],
        reference_sources={name:hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in refs},
        scope='Bounded plot step-consumer contract; zero new NASA or GUI-mechanism credit.'),indent=2)+'\n')
    command=[sys.executable,str(Path(__file__).with_name('RunAcceptance.py')),'--build-dir',str(args.build_dir),
             '--filter',selection,'--nasa-review',str(review.relative_to(repo)),'--max-screenshots','0',
             '--max-checkpoints','200','--pause-ms',str(args.pause_ms),'--output',str(output)]
    if args.visible:command+=['--visible','--display-route','nested','--display-scale','auto','--start-minimized']
    environment=dict(os.environ)
    if args.cases:environment["GMAT_PLOT_STEP_CASE"]=args.cases
    return subprocess.call(command,cwd=repo,env=environment)


if __name__=='__main__':raise SystemExit(main())
