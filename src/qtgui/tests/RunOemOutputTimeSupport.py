"""Check selected-clock native copy/guards and serialized rounding groups."""
import argparse
from datetime import datetime,timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from CcsdsOemB1Oracle import validate
from OemOutputTimeOracle import support_catalog
from RunOemCreationClock import creation_check


def audit(root,fixture):
    root=Path(root);assert fixture==support_catalog()
    proof=json.loads((root/'writer-time-proof.json').read_text());run=json.loads((root/'writer-time-run.json').read_text())
    assert proof['outcome']=='passed'and len(proof['cases'])==5 and run['native_returncode']==0
    observations=[]
    for case,record in zip(fixture['cases'],proof['cases']):
        assert record['system']==case['system']and record['actual_labels']==case['expected_labels']and record['serialized_count']==case['expected_count']
        assert all(record[k]for k in ['copy_assignment_preserved','invalid_assignments_rejected','mid_file_change_rejected','metadata_mismatch_rejected','open_file_assignment_rejected','native_parameter_clone_preserved','input_epoch_format_preserved','inactive_format_reset'])
        raw=(root/(case['system']+'.oem')).read_bytes()
        finding=validate(raw,icd_object_ids=[fixture['object_id']])
        expected='incomplete'if case['system']=='A1'else'passed'
        assert finding['status']==expected
        if expected=='incomplete':assert finding['gaps']==['Unreviewed ICD time system: A1']
        assert finding['header']['ORIGINATOR']==fixture['test_interface_profile']['originator']
        meta=finding['segments'][0]['metadata'];assert meta['TIME_SYSTEM']==case['system']and meta['OBJECT_ID']==fixture['object_id']
        assert int(meta['INTERPOLATION_DEGREE'])==case['expected_count']-1
        rows=[]
        for line in raw.decode('ascii').splitlines():
            fields=line.split()
            if fields and fields[0].startswith('2000-'):rows.append((fields[0],list(map(float,fields[1:]))))
        wanted=[(g['epoch'],[7000.+g['marker'],2.,3.,4.,5.,6.])for g in case['expected_groups']]
        assert rows==wanted,'Selected-clock duplicate groups or retained last state differ'
        clock=creation_check(raw,run['lower_utc'],run['upper_utc']);assert clock['status']=='passed'
        observations.append(dict(system=case['system'],serialized_count=len(rows),format_scope=finding,creation_clock=clock,raw_sha256=hashlib.sha256(raw).hexdigest(),native_guards=record))
    assert {x['system']:x['serialized_count']for x in observations}=={'A1':2,'TAI':1,'UTC':1,'TT':1,'TDB':1}
    return dict(status='passed',observations=observations,test_interface_profile=fixture['test_interface_profile'],
        new_requirement_credit=0,whole_row_qualified=False,original_assertion_changes=0)


def run(binary,startup,fixture_path,root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True);lower=datetime.now(timezone.utc).isoformat()
    code=subprocess.call([str(binary),str(startup),str(fixture_path)],cwd=Path(startup).resolve().parent,
        env=dict(os.environ,GMAT_QT_ACCEPTANCE_DIR=str(root)))
    record=dict(lower_utc=lower,upper_utc=datetime.now(timezone.utc).isoformat(),native_returncode=code)
    (root/'writer-time-run.json').write_text(json.dumps(record,indent=2)+'\n')
    if code:return code
    result=audit(root,json.loads(Path(fixture_path).read_text()))
    (root/'independent-output-time-support-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(status=result['status'],systems=len(result['observations']))))
    return 0


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('binary',type=Path);p.add_argument('startup',type=Path);p.add_argument('fixture',type=Path);a=p.parse_args()
    if os.environ.get('GMAT_QT_ACCEPTANCE_DIR'):raise SystemExit(run(a.binary,a.startup,a.fixture,Path(os.environ['GMAT_QT_ACCEPTANCE_DIR'])))
    with tempfile.TemporaryDirectory(prefix='gmat-oem-output-time-')as directory:raise SystemExit(run(a.binary,a.startup,a.fixture,Path(directory)))
