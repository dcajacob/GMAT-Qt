"""Run one missing CK backend conjunction and its independent packet audit."""
from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile
from AuditEstimatorCkEvidence import audit_case


def run(output):
    executable,startup,fixture,family=sys.argv[1:]
    output=Path(output).resolve();output.mkdir(parents=True,exist_ok=True)
    env=dict(os.environ,GMAT_QT_ACCEPTANCE_DIR=str(output))
    result=subprocess.run([executable,startup,fixture,family],cwd=Path(startup).resolve().parent,
                          env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=120)
    with(output/'native-ck.log').open('x')as f:f.write(result.stdout)
    if result.returncode:
        print(result.stdout[-3000:],file=sys.stderr);return result.returncode
    checked=audit_case(output,fixture)
    with(output/'independent-ck-audit.json').open('x')as f:json.dump(checked,f,indent=2);f.write('\n')
    print('NASA_CHECK '+json.dumps(dict(status='passed',**checked,new_requirement_credit=0)))
    return 0


if __name__=='__main__':
    if len(sys.argv)!=5:raise SystemExit('Supply observer executable,startup,fixture,family')
    configured=os.environ.get('GMAT_QT_ACCEPTANCE_DIR')
    if configured:raise SystemExit(run(configured))
    with tempfile.TemporaryDirectory(prefix='gmat-estimator-ck-')as temporary:raise SystemExit(run(temporary))
