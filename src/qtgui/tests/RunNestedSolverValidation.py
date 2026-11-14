"""Run isolated native branch-scope validation cases and retain every outcome."""
import argparse
from datetime import datetime,timezone
import hashlib,json,os
from pathlib import Path
import subprocess,tempfile,time
from AcceptanceRuntime import isolate_startup
CASES=['TargetDirect','TargetIf','TargetElse','TargetFor','TargetWhile','TargetDoubleNesting','TargetInsideIf','NestedIndependentSolver','NestedSameSolver','OptimizeIf','OptimizeElse','OptimizeFor','OptimizeWhile','OutsideIfVary','OutsideElseAchieve','TargetWrongNameIf','TargetWrongNameElse','NestedWrongOwner','TargetWrongSolverType','OptimizeWrongControlType']
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()

def run(binary,startup,root,selected=None):
 root=root.resolve();root.mkdir(parents=True,exist_ok=True);results=[]
 for name in selected or CASES:
  case=root/name;case.mkdir();private=isolate_startup(startup,case/'runtime'/'bin')
  ready=case/'native-ready.pid';ack=case/'native-maps.ack'
  env=dict(os.environ,GMAT_QT_ACCEPTANCE_DIR=str(case),GMAT_SOLVER_MAP_READY=str(ready),GMAT_SOLVER_MAP_ACK=str(ack))
  started=datetime.now(timezone.utc).isoformat();command=[str(binary.resolve()),str(private),name]
  with (case/'native.log').open('w') as log:
   process=subprocess.Popen(command,cwd=private.parent,env=env,stdout=log,stderr=subprocess.STDOUT)
   deadline=time.monotonic()+15
   while not ready.exists() and process.poll() is None and time.monotonic()<deadline:time.sleep(.01)
   mapped={}
   if ready.exists():
    pid=int(ready.read_text());assert pid==process.pid
    maps=(Path('/proc')/str(pid)/'maps').read_text();(case/'native-maps.txt').write_text(maps)
    for line in maps.splitlines():
     fields=line.split(None,5)
     if len(fields)!=6 or not fields[5].startswith('/'):continue
     path=Path(fields[5])
     if path.is_file():
      with path.open('rb') as f:is_elf=f.read(4)==b'\x7fELF'
      if is_elf:mapped[str(path)]={'sha256':sha(path),'size':path.stat().st_size}
    (case/'native-image-identities.json').write_text(json.dumps(mapped,indent=2)+'\n');ack.write_text('mapped\n')
   try:code=process.wait(timeout=90)
   except subprocess.TimeoutExpired:
    process.kill();code=process.wait();code=124
  proof=json.loads((case/'proof.json').read_text()) if (case/'proof.json').exists() else None
  receipt={'case':name,'returncode':code,'proof':proof,'command':command,'started_utc':started,'finished_utc':datetime.now(timezone.utc).isoformat(),'binary_sha256':sha(binary),'startup_sha256':sha(private),'loaded_ELF_images':len(mapped),'passed':bool(code==0 and proof and proof['passed']),'runtime_image_copies':0}
  (case/'native-run.json').write_text(json.dumps(receipt,indent=2)+'\n');results.append(receipt)
  (root/'results.json').write_text(json.dumps({'cases':results,'new_requirement_credit':0},indent=2)+'\n')
  print(json.dumps({'case':name,'passed':receipt['passed'],'returncode':code}),flush=True)
 print(json.dumps({'passed':sum(r['passed'] for r in results),'failed':sum(not r['passed'] for r in results),'total':len(results),'new_requirement_credit':0}))
 return 0 if all(r['passed'] for r in results) else 1

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('binary',type=Path);p.add_argument('startup',type=Path);p.add_argument('--case',action='append',choices=CASES);a=p.parse_args();output=os.environ.get('GMAT_QT_ACCEPTANCE_DIR')
 if output:raise SystemExit(run(a.binary,a.startup,Path(output),a.case))
 with tempfile.TemporaryDirectory(prefix='gmat-solver-scope-') as temp:raise SystemExit(run(a.binary,a.startup,Path(temp),a.case))
