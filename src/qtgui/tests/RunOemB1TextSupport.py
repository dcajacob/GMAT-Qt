"""Execute native OEM text boundaries and independently inspect retained bytes."""
import argparse
from datetime import datetime,timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from CcsdsOemB1Oracle import ConformanceError,validate
from RunOemCreationClock import creation_check


def audit(root):
    root=Path(root)
    run=json.loads((root/'text-run.json').read_text())
    proof=json.loads((root/'text-proof.json').read_text())
    observations=[]
    for case in proof['cases']:
        raw=(root/(case['case']+'.oem')).read_bytes()
        issues=[]
        try:
            finding=validate(raw,icd_object_ids=['GMAT-B1-text-native','A'*231,'A  B'])
        except ConformanceError as error:
            finding=dict(status='failed',clause=error.clause,line=error.line,message=error.message)
        if case['expected_rejection']:
            if not case['actual_rejection']:issues.append('Invalid text was exported successfully')
            if case['actual_rejection']:
                if not case['error']:issues.append('Rejection has no diagnostic')
                if not all(len(line)<=254 and all(32<=c<=126 for c in line)for line in raw.splitlines()):
                    issues.append('Invalid bytes reached partial output')
                metadata_error=bool(case['field'])and case['field']!='ORIGINATOR'
                if metadata_error and b'META_START'in raw:issues.append('Invalid metadata reached partial output')
        else:
            if case['actual_rejection']:issues.append('Valid text was rejected')
            allowed_minimal=(case['minimal']and finding.get('status')=='incomplete'and
                finding.get('gaps')==['No recommended interpolation method supplied'])
            if finding.get('status')!='passed'and not allowed_minimal:issues.append('Valid case failed independent B1 inspection')
            if case['case'].endswith('Multiline'):
                expected=[b'COMMENT  first  ',b'COMMENT second',b'COMMENT  third',b'COMMENT last',b'COMMENT ']
                actual=[line for line in raw.splitlines()if line.startswith(b'COMMENT')]
                if actual!=expected:issues.append('Multiline comment prefix or whitespace changed')
            if b'USEABLE_START_TIME'in raw or b'USEABLE_STOP_TIME'in raw:
                issues.append('Absent optional usable bounds were serialized')
        header=raw.partition(b'META_START')[0]
        # A malformed originator must not suppress the independent format finding.
        clock_bytes=b'\n'.join(line for line in header.splitlines()if line.startswith(b'CREATION_DATE'))+b'\n'
        clock=creation_check(clock_bytes,run['lower_utc'],run['upper_utc'])if b'CREATION_DATE'in header else None
        if clock and clock['status']!='passed':issues.append('Creation time outside UTC execution bounds')
        observations.append(dict(**case,raw_sha256=hashlib.sha256(raw).hexdigest(),
            format_scope=finding,creation_clock=clock,issues=issues,status='failed'if issues else'passed'))
    return dict(status='passed'if all(x['status']=='passed'for x in observations)else'failed',
        observations=observations,native_returncode=run['native_returncode'],
        test_interface_profile=dict(sender='OemB1TextTests new native fixture',receiver='RunOemB1TextSupport independent checker',
            object_ids=['GMAT-B1-text-native','A'*231,'A  B'],object_name='TextSat',originator='GMAT B1 text test or explicit boundary value',
            version='1.0',center='EARTH',frame='EME2000',time_system='UTC',units=['km','km/s'],
            filenames='case-name.oem',exchange='unchanged local bytes bound by SHA256;private test interface only'),
        new_requirement_credit=0,whole_row_qualified=False)


def run(binary,startup,root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True)
    lower=datetime.now(timezone.utc).isoformat()
    code=subprocess.call([str(binary),str(startup)],cwd=Path(startup).resolve().parent,env=dict(os.environ,GMAT_QT_ACCEPTANCE_DIR=str(root)))
    receipt=dict(lower_utc=lower,upper_utc=datetime.now(timezone.utc).isoformat(),native_returncode=code)
    (root/'text-run.json').write_text(json.dumps(receipt,indent=2)+'\n')
    result=audit(root)
    (root/'independent-B1-text-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(status=result['status'],native_returncode=code,cases=len(result['observations']))))
    return code or (0 if result['status']=='passed'else 1)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('binary',type=Path);p.add_argument('startup',type=Path)
    a=p.parse_args()
    if os.environ.get('GMAT_QT_ACCEPTANCE_DIR'):
        raise SystemExit(run(a.binary,a.startup,Path(os.environ['GMAT_QT_ACCEPTANCE_DIR'])))
    with tempfile.TemporaryDirectory(prefix='gmat-oem-b1-text-')as directory:
        raise SystemExit(run(a.binary,a.startup,Path(directory)))
