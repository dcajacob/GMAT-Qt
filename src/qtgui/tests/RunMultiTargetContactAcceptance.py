"""Replay multiple-spacecraft contact selection; --visible starts minimized.

This runs current binaries and records their identities. It does not transfer
earlier source-versioned NASA qualification to a different build.
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
    parser.add_argument('--include-native',action='store_true')
    parser.add_argument('--include-default-report',action='store_true')
    parser.add_argument('--pause-ms',type=int,default=350)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--list',action='store_true')
    parser.add_argument('--case',action='append',default=[],help='Run only named new case(s); default is all eight.')
    parser.add_argument('--native-only',action='store_true')
    args=parser.parse_args();identity=uuid.uuid4().hex[:12]
    names=[] if args.native_only else ['NasaMultiTargetContact'+name for name in args.case] if args.case else ['NasaMultiTargetContact.*']
    if args.include_native or args.native_only:names+=['MultiTargetContactParameters','StationEventParameters']
    if args.include_default_report:names+=['NasaContactResultMultiStation','NasaStationEventBoth']
    selection='^QtGui.('+'|'.join(names)+')$'
    if args.list:
        return subprocess.call(['ctest','--test-dir',str(args.build_dir),'-N','-R',selection],cwd=repo)
    output=args.output or repo/'build/gui-acceptance'/('multi-target-contact-'+identity)
    review=repo/'build/gui-acceptance'/('multi-target-contact-'+identity+'-runtime-review.json')
    review.parent.mkdir(parents=True,exist_ok=True)
    refs=['src/base/subscriber/EphemManager.cpp','src/base/subscriber/EphemManager.hpp',
          'plugins/EventLocatorPlugin/src/base/locator/ContactLocator.cpp',
          'plugins/EventLocatorPlugin/src/base/locator/ContactLocator.hpp',
          'src/base/interpreter/Interpreter.cpp','src/qtgui/ResourceProperties.cpp',
          'src/qtgui/EventLocatorDialog.cpp','src/qtgui/tests/NasaContactResultTests.cpp',
          'src/qtgui/tests/StationCrossingKernelTests.cpp','src/qtgui/tests/StationEventParameterTests.cpp',
          'src/qtgui/tests/NasaStationEventCases.json','src/qtgui/tests/StationEventOracle.py',
          'src/qtgui/tests/NasaMultiTargetContactTests.cpp','src/qtgui/tests/MultiTargetContactParameterTests.cpp',
          'src/qtgui/tests/NasaMultiTargetContactCases.json','src/qtgui/tests/MultiTargetContactOracle.py',
          'src/qtgui/tests/NasaContactResultCases.json','src/qtgui/tests/ContactGeometryOracle.py']
    fixture=json.loads((repo/'src/qtgui/tests/NasaStationEventCases.json').read_text())
    refs+=list(fixture['physical_inputs']['kernels'])
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],
        reference_sources={name:hashlib.sha256((repo/name).read_bytes()).hexdigest() for name in refs},
        scope='Current multiple-target runtime replay; exact historical-source qualification is a separate audited artifact.'),indent=2)+'\n')
    command=[sys.executable,str(Path(__file__).with_name('RunAcceptance.py')),
             '--build-dir',str(args.build_dir),'--filter',selection,
             '--nasa-review',str(review.relative_to(repo)),'--max-screenshots','0','--max-checkpoints','200',
             '--pause-ms',str(args.pause_ms),'--output',str(output)]
    if args.visible:command+=['--visible','--display-route','nested','--display-scale','auto','--start-minimized']
    return subprocess.call(command,cwd=repo)


if __name__=='__main__':
    raise SystemExit(main())
