"""New pacing-budget contracts; does not execute a GMAT mission or regression."""
from contextlib import redirect_stderr
import hashlib,io,json,os
from pathlib import Path
import shutil,subprocess,sys,unittest,uuid
from unittest.mock import patch
import RunAcceptance as runner

probe=Path(sys.argv[1]).resolve();output=Path(sys.argv[2]).resolve()/str(uuid.uuid4())
output.mkdir(parents=True);sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()

class Parsed(Exception):pass
def main():
 for limit in [0,1000,2000,5000]:
  with patch.object(runner,'configure_display',side_effect=Parsed):
   try:runner.main(['--list','--max-checkpoints',str(limit)])
   except Parsed:pass
   else:raise AssertionError('Valid pacing budget did not reach display configuration')
 for limit in [-1,5001]:
  with redirect_stderr(io.StringIO()):
   try:runner.main(['--list','--max-checkpoints',str(limit)])
   except SystemExit as error:assert error.code==2
   else:raise AssertionError('Invalid pacing budget accepted')
 launcher=shutil.which('xvfb-run');assert launcher,'Native checkpoint budget contract requires xvfb-run'
 cases=[]
 for limit in [1000,2000]:
  directory=output/str(limit);directory.mkdir();env=dict(os.environ,QT_QPA_PLATFORM='xcb',GMAT_QT_ACCEPTANCE_MODE='visible',GMAT_QT_NATIVE_TEST_LAUNCH='1',GMAT_QT_ACCEPTANCE_PAUSE_MS='0',GMAT_QT_ACCEPTANCE_MAX_CHECKPOINTS=str(limit),GMAT_QT_ACCEPTANCE_MAX_SCREENSHOTS='0',GMAT_QT_ACCEPTANCE_DIR=str(directory),GMAT_QT_ACCEPTANCE_TEST='checkpoint-budget-contract')
  result=subprocess.run([launcher,'-a',str(probe),'--checkpoint-budget-contract'],env=env,capture_output=True,text=True,timeout=20);(directory/'stdout.txt').write_text(result.stdout);(directory/'stderr.txt').write_text(result.stderr);assert result.returncode==0 and'budget_boundaries=1100 final=Known final result = 42'in result.stdout
  traces=list(directory.glob('trace*.jsonl'));assert len(traces)==1;events=[json.loads(s)for s in traces[0].read_text().splitlines()];bounds=[(i,e)for i,e in enumerate(events)if e['event']=='viewing_boundary'];assert len(bounds)==1100 and all(e['visible']for i,e in bounds);checkpoints=[e for e in events if e['event']=='checkpoint'];paced=[events[i+1]for i,e in bounds if events[i+1]['event']=='checkpoint'];assert all(e['exposed']and e['platform']=='xcb'for e in paced);assert any(e['event']=='native_exposure'and e['exposed']for e in events)and not list(directory.rglob('*.png'))
  if limit==1000:assert len(checkpoints)==1000 and len(paced)<1100
  else:assert len(checkpoints)>1000 and len(paced)==1100 and len(checkpoints)<=2000
  cases.append(dict(limit=limit,viewing_boundaries=1100,paced_boundaries=len(paced),checkpoints=len(checkpoints),raw_trace=str(traces[0]),raw_trace_sha256=sha(traces[0]),returncode=result.returncode,new_screenshots=0))
 paths=['RunAcceptance.py','AcceptanceSupport.cpp','AcceptanceObserverProbe.cpp','AcceptanceBudgetTests.py'];sources={str(Path(__file__).parent/p):sha(Path(__file__).parent/p)for p in paths}
 summary=dict(status='passed',cases=cases,valid_cli_limits=[0,1000,2000,5000],invalid_cli_limits=[-1,5001],sources=sources,probe=str(probe),probe_sha256=sha(probe),new_gmat_missions=0,new_screenshots=0,scope='Harness-only private native-window probe:1100fresh artificial boundaries,old1000cap preserved and new2000budget fully paced. Does not replay or qualify GMAT checkbox cases/full current-build suites.')
 (output/'summary.json').write_text(json.dumps(summary,indent=2)+'\n');(output.parent/'latest.json').write_text(json.dumps(dict(summary=str(output/'summary.json'),summary_sha256=sha(output/'summary.json')),indent=2)+'\n');print(json.dumps(summary,indent=2))
if __name__=='__main__':main()
