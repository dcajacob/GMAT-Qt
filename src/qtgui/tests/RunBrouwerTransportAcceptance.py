"""Run only the missing transport witness and its independent numerical audit."""
from pathlib import Path
import json,os,subprocess,sys,tempfile
from AuditBrouwerTransportEvidence import audit

def run(directory):
 executable,startup,fixture,repo=map(Path,sys.argv[1:]);repo=repo.resolve();startup=startup.resolve();directory=directory.resolve();directory.mkdir(parents=True,exist_ok=True)
 env=dict(os.environ,GMAT_QT_ACCEPTANCE_DIR=str(directory))
 result=subprocess.run([str(executable),str(startup),str(fixture),str(repo)],cwd=startup.parent,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=60)
 with(directory/'transport-native.log').open('x')as f:f.write(result.stdout)
 if result.returncode:
  print(result.stdout[-3500:],file=sys.stderr);return result.returncode
 checked=audit(repo,directory)
 with(directory/'independent-equivalence.json').open('x')as f:json.dump(checked,f,indent=2);f.write('\n')
 print('NASA_CHECK '+json.dumps({k:v for k,v in checked.items()if k!='results'}))
 return 0
if __name__=='__main__':
 if len(sys.argv)!=5:raise SystemExit('Supply observer executable,startup,fixture,repository')
 configured=os.environ.get('GMAT_QT_ACCEPTANCE_DIR')
 if configured:raise SystemExit(run(Path(configured)))
 with tempfile.TemporaryDirectory(prefix='gmat-brouwer-transport-')as temporary:raise SystemExit(run(Path(temporary)))
