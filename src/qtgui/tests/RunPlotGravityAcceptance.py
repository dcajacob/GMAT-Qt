"""Replay new plot-gravity or affected harmonic cases with a fresh bounded review."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import uuid
import NasaGuiCoverage as nasa

CASES=','.join('Rotating'+kind+'Gravity'+direction for kind in ('Origin','Earth','Velocity','Pair')for direction in ('Forward','Backward'))+',RotatingPairGravityThinned'
def main():
    repo=Path(__file__).resolve().parents[3]
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--group',choices=['plot','harmonic'],default='plot');p.add_argument('--build-dir',type=Path,default=repo/'build/linux-gui');p.add_argument('--visible',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--list',action='store_true');args=p.parse_args()
    selected='^QtGui.PlotIntegrationSteps$'if args.group=='plot'else'^QtGui.NasaHarmonicGravity(Earth|Ceres)$'
    if args.list:return subprocess.call(['ctest','--test-dir',str(args.build_dir),'-N','-R',selected],cwd=repo)
    output=args.output or repo/'build/gui-acceptance'/('plot-gravity-'+args.group+'-'+uuid.uuid4().hex[:12])
    if args.group=='plot':
        command=[sys.executable,str(Path(__file__).with_name('RunPlotIntegrationStepAcceptance.py')),'--build-dir',str(args.build_dir),'--cases',CASES,'--pause-ms','150'if args.visible else'0','--output',str(output)]
        if args.visible:command+=['--visible']
    else:
        review=repo/'build/gui-acceptance'/('plot-gravity-review-'+uuid.uuid4().hex[:12]+'.json');review.parent.mkdir(parents=True,exist_ok=True)
        files=[f for root in ('src/base','src/gmatutil','plugins')for f in(repo/root).rglob('*')if f.is_file()and f.suffix in {'.c','.cpp','.h','.hpp','.inc'}]
        record=dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={str(f.relative_to(repo)):hashlib.sha256(f.read_bytes()).hexdigest()for f in files},scope='Bounded sampled harmonic force/clock evidence;zero whole NASA requirement credit.')
        review.write_text(json.dumps(record,indent=2)+'\n')
        command=[sys.executable,str(Path(__file__).with_name('RunAcceptance.py')),'--build-dir',str(args.build_dir),'--filter',selected,'--nasa-review',str(review.relative_to(repo)),'--max-screenshots','0','--max-checkpoints','2000','--pause-ms','150'if args.visible else'0','--output',str(output)]
        if args.visible:command+=['--visible','--display-route','nested','--display-scale','auto','--start-minimized']
    return subprocess.call(command,cwd=repo,env=dict(os.environ,PYTHONDONTWRITEBYTECODE='1'))
if __name__=='__main__':raise SystemExit(main())
