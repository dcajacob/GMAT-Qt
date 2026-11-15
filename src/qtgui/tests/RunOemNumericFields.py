"""Run actual native OEM numeric guards and independently validate output bytes."""
import argparse
from datetime import datetime, timezone
from decimal import Decimal
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
from CcsdsOemB1Oracle import validate, ConformanceError


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def audit(root):
    proof=json.loads((root/'numeric-proof.json').read_text())
    observations=[]
    for item in proof['finite']:
        raw=(root/item['file']).read_bytes()
        try:
            format_check=validate(raw,icd_object_ids=['GMAT-Qt-OEM-numeric'])
        except ConformanceError as error:
            format_check={'status':'failed','clause':error.clause,'line':error.line,'message':error.message}
        issues=[]
        if format_check['status']!='passed':issues.append('Finite output violates independent B-1 numeric/format contract')
        rows=[line.split() for line in raw.decode('ascii').splitlines() if line.startswith('2000-')]
        if len(rows)!=2:issues.append('Finite output row count differs')
        for row in rows:
            if len(row)!=7:
                issues.append('Finite output has wrong Cartesian shape');continue
            for token,expected in zip(row[1:],item['state']):
                try:
                    value=Decimal(token); target=Decimal(str(expected))
                except Exception:
                    issues.append('Output number cannot be read independently');continue
                if not value.is_finite() or abs(value-target)>max(abs(target)*Decimal('1e-15'),Decimal('4.9406564584124654e-324')):
                    issues.append('Finite output changed the value beyond its declared lexical precision')
        observations.append({'case':item['file'],'sha256':hashlib.sha256(raw).hexdigest(),'format':format_check,'issues':issues,'status':'failed' if issues else 'passed'})
    for item in proof['extended']:
        raw=(root/item['file']).read_bytes();issues=[]
        lines=raw.decode('ascii').splitlines()
        states=[line.split()[1:] for line in lines if line.startswith('2000-')]
        rows=[];in_cov=False
        for line in lines:
            if line=='COVARIANCE_START':in_cov=True;continue
            if line=='COVARIANCE_STOP':in_cov=False;continue
            if in_cov and line.strip() and not line.startswith('EPOCH'):rows.append(line.split())
        expected=[Decimal(str(v)) for v in item['values']]
        try:
            if len(states)!=2 or any([Decimal(v) for v in row]!=expected[:6]+expected[27:] for row in states):issues.append('Extended Cartesian/acceleration numeric values or order differ')
            if [len(row) for row in rows]!=[1,2,3,4,5,6]*2 or [Decimal(v) for row in rows for v in row]!=expected[6:27]*2:issues.append('Extended covariance numeric values or lower-triangular order differ')
        except Exception:issues.append('Extended numeric syntax cannot be read independently')
        observations.append({'case':item['file'],'kind':'shared-OEM2-numeric-compatibility-only','sha256':hashlib.sha256(raw).hexdigest(),'issues':issues,'status':'failed' if issues else 'passed'})
    for item in proof['writer_rejections']:
        issues=[]
        if not item['passed'] or not item['rejected'] or item['count_after']!=1 or not item['error']:issues.append('Invalid native writer sample was not rejected atomically')
        if item['passed']:
            raw=(root/item['file']).read_bytes()
            try:check=validate(raw,icd_object_ids=['GMAT-Qt-OEM-numeric'])
            except ConformanceError as error:check={'status':'failed','message':str(error)}
            if check['status']!='passed' or check.get('records')!=2:issues.append('Recovered writer exported an invalid record or lost valid records')
            rows=[line.split() for line in raw.decode('ascii').splitlines() if line.startswith('2000-')]
            if len(rows)!=2 or any(len(row)!=7 or [Decimal(v) for v in row[1:]]!=[Decimal(str(v)) for v in [7000.125,-1000.5,2000.25,-1.25,6.5,2.75]] for row in rows):issues.append('Recovered records differ from the exact valid input')
        observations.append({'case':item['case'],'kind':'native-writer-rejection-and-recovery','issues':issues,'status':'failed' if issues else 'passed'})
    for index,item in enumerate(proof['subscriber_rejections']):
        observations.append({'case':'subscriber-'+str(index),'kind':'native-buffer-rejection-before-flush','status':'passed' if item['passed'] and item['rejected'] else 'failed'})
    assert len(proof['finite'])==6 and len(proof['extended'])==2 and len(proof['writer_rejections'])==27 and len(proof['subscriber_rejections'])==22
    return {'status':'passed' if proof['failures']==0 and all(v['status']=='passed' for v in observations) else 'failed','observations':observations,'finite_files':6,'extended_numeric_files':2,'writer_rejections':27,'subscriber_rejections':22,'new_requirement_credit':0,'whole_row_qualified':False,'scope':'OEM1.0 numeric syntax/finite guards, count/buffer recovery and process numeric locale. No trajectory/frame/physical-domain or whole NASA claim.'}


def run(binary,startup,root):
    root=root.resolve();root.mkdir(parents=True,exist_ok=True)
    ready=root/'native-ready.pid';ack=root/'native-maps.ack'
    env=dict(os.environ,GMAT_QT_ACCEPTANCE_DIR=str(root),GMAT_OEM_MAP_READY=str(ready),GMAT_OEM_MAP_ACK=str(ack))
    start=datetime.now(timezone.utc).isoformat()
    with (root/'native.log').open('w') as log:
        process=subprocess.Popen([str(binary.resolve()),str(startup.resolve())],cwd=startup.resolve().parent,env=env,stdout=log,stderr=subprocess.STDOUT)
        deadline=time.monotonic()+8
        while not ready.exists() and process.poll() is None and time.monotonic()<deadline:time.sleep(.01)
        if not ready.exists():
            process.wait(timeout=10)
            raise RuntimeError('Native numeric process did not reach the live mapping handshake')
        pid=int(ready.read_text());assert pid==process.pid
        maps=Path('/proc')/str(pid)/'maps'
        raw=maps.read_text();(root/'native-maps.txt').write_text(raw)
        images={}
        for line in raw.splitlines():
            fields=line.split(None,5)
            if len(fields)!=6 or not fields[5].startswith('/'):continue
            path=Path(fields[5])
            if path.is_file():
                with path.open('rb') as f:is_elf=f.read(4)==b'\x7fELF'
                if is_elf:images[str(path)]={'sha256':sha(path),'size':path.stat().st_size}
        (root/'native-image-identities.json').write_text(json.dumps(images,indent=2)+'\n')
        ack.write_text('mapped\n')
        code=process.wait(timeout=45)
    (root/'native-run.json').write_text(json.dumps({'pid':pid,'started_utc':start,'finished_utc':datetime.now(timezone.utc).isoformat(),'returncode':code,'binary_sha256':sha(binary),'startup_sha256':sha(startup),'loaded_ELF_images':len(images),'runtime_image_copies':0},indent=2)+'\n')
    if not (root/'numeric-proof.json').exists():raise RuntimeError('Native numeric process failed before producing proof; inspect native.log')
    result=audit(root);(root/'numeric-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'status':result['status'],'native_returncode':code,'cases':len(result['observations']),'new_requirement_credit':0}))
    return code or (0 if result['status']=='passed' else 1)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('binary',type=Path);p.add_argument('startup',type=Path);args=p.parse_args()
    output=os.environ.get('GMAT_QT_ACCEPTANCE_DIR')
    if output:raise SystemExit(run(args.binary,args.startup,Path(output)))
    with tempfile.TemporaryDirectory(prefix='gmat-oem-numeric-') as temp:raise SystemExit(run(args.binary,args.startup,Path(temp)))
