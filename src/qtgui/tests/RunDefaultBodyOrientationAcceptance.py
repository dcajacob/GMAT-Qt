"""Replay the default-body IAU2002 orientation witnesses; visible desktop starts minimized.

No numerical solver qualification or whole-row credit follows from replay alone.
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
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--visible',action='store_true')
    p.add_argument('--output',type=Path);p.add_argument('--build-dir',type=Path,default=repo/'build/linux-gui')
    args=p.parse_args();identity=uuid.uuid4().hex[:12]
    selection='QtGui.NasaDefaultOrientation(Sun|Mercury|Venus|Mars|Jupiter|Saturn|Uranus|Neptune|Pluto)'
    stage=repo/'build/gui-acceptance';stage.mkdir(parents=True,exist_ok=True)
    review=stage/('default-orientation-'+identity+'-runtime-review.json')
    files=['src/base/solarsys/CelestialBody.cpp','src/base/solarsys/Planet.cpp','src/base/coordsystem/BodyFixedAxes.cpp','src/qtgui/tests/DefaultBodyOrientationOracle.py','src/qtgui/tests/LunarOrientationOracle.py','src/qtgui/tests/NasaDefaultBodyOrientationCases.json','src/qtgui/tests/fixtures/lunar/pck00008.tpc']
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest() for n in files}),indent=2)+'\n')
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--build-dir',str(args.build_dir),'--filter','^('+selection+')$',
             '--max-screenshots','0','--max-checkpoints','200','--pause-ms','0','--nasa-review',str(review.relative_to(repo)),
             '--output',str(args.output or stage/('default-orientation-'+identity))]
    if args.visible:command+=['--visible','--start-minimized']
    return subprocess.call(command,cwd=repo)


if __name__=='__main__':raise SystemExit(main())
