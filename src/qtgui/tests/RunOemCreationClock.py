"""Check actual native OEM creation dates against independent UTC bounds in three zones."""
import argparse
from datetime import datetime, timedelta, timezone
import json
import os
from pathlib import Path
import subprocess
import tempfile
from zoneinfo import ZoneInfo

from CcsdsOemB1Oracle import validate


ZONES=('UTC','America/Denver','Asia/Kolkata')


def creation_check(raw, lower, upper):
    values=[]
    for line in raw.decode('ascii').splitlines():
        if line.strip()=='META_START':break
        if '='in line:
            key,value=line.split('=',1)
            if key.strip()=='CREATION_DATE':values.append(value.strip())
    assert len(values)==1
    value=values[0]
    parsed=datetime.fromisoformat(value)
    assert parsed.tzinfo is None
    stamp=parsed.replace(tzinfo=timezone.utc)
    # Native creation fields have whole-second precision;quantize the UTC lower bound accordingly.
    low=datetime.fromisoformat(lower).replace(microsecond=0)
    high=datetime.fromisoformat(upper)
    return dict(status='passed'if low<=stamp<=high else'failed',serialized=value,
                lower_utc=lower,upper_utc=upper,UTC_within_creation_interval=low<=stamp<=high)


def audit(root):
    root=Path(root);receipt=json.loads((root/'clock-run.json').read_text())
    proof=json.loads((root/'clock-proof.json').read_text())
    assert receipt['native_returncode']==0 and proof['closed_writer_rejected'] and proof['native_files']==2
    assert proof['observed_timezone']==receipt['timezone']
    files=[]
    for version in ['1.0','2.0']:
        path=root/('clock-'+version+'.oem');raw=path.read_bytes()
        assert raw.startswith(('CCSDS_OEM_VERS = '+version+'\n').encode())
        clock=creation_check(raw,receipt['lower_utc'],receipt['upper_utc'])
        if version=='1.0':
            finding=validate(raw,icd_object_ids=['GMAT-UTC-header-test'])
            assert finding['status']=='passed' and finding['records']==2
        else:
            finding=dict(status='version2-header-only;outside-B1-format-scope')
        files.append(dict(version=version,creation_clock=clock,format_scope=finding))
    local=datetime.fromisoformat(proof['generic_local_time'])
    lower=datetime.fromisoformat(receipt['lower_utc']).replace(microsecond=0)
    upper=datetime.fromisoformat(receipt['upper_utc'])
    candidates=[local.replace(tzinfo=timezone(timedelta(seconds=offset)))
                for offset in receipt['independent_local_offsets_seconds']]
    assert any(lower<=candidate<=upper for candidate in candidates), 'Generic local-time reporting changed'
    return dict(status='passed'if all(x['creation_clock']['status']=='passed'for x in files)else'failed',
                timezone=receipt['timezone'],files=files,generic_local_clock_preserved=True,
                new_requirement_credit=0,whole_row_qualified=False)


def run(binary,startup,zone,root):
    root.mkdir(parents=True,exist_ok=True)
    lower=datetime.now(timezone.utc)
    with (root/'clock-native.log').open('wb')as log:
        code=subprocess.call([str(binary),str(startup)],cwd=startup.parent,
            env=dict(os.environ,TZ=zone,GMAT_QT_ACCEPTANCE_DIR=str(root)),stdout=log,stderr=subprocess.STDOUT)
    upper=datetime.now(timezone.utc)
    offsets=sorted({int(t.astimezone(ZoneInfo(zone)).utcoffset().total_seconds())for t in [lower,upper]})
    receipt=dict(timezone=zone,lower_utc=lower.isoformat(),upper_utc=upper.isoformat(),
                 independent_local_offsets_seconds=offsets,native_returncode=code,
                 test_identifier_profile='Native synthetic ClockSat sender and this independent receiver agree on GMAT-UTC-header-test')
    (root/'clock-run.json').write_text(json.dumps(receipt,indent=2)+'\n')
    if code:
        print((root/'clock-native.log').read_text(errors='replace')[-4000:]);return code
    result=audit(root)
    (root/'clock-independent-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(status=result['status'],timezone=zone,generic_local_clock_preserved=True)))
    return 0 if result['status']=='passed'else 1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path);parser.add_argument('startup',type=Path)
    parser.add_argument('--zone',choices=ZONES,required=True)
    args=parser.parse_args();output=os.environ.get('GMAT_QT_ACCEPTANCE_DIR')
    if output:return run(args.binary.resolve(),args.startup.resolve(),args.zone,Path(output).resolve())
    with tempfile.TemporaryDirectory(prefix='gmat-oem-creation-clock-')as folder:
        return run(args.binary.resolve(),args.startup.resolve(),args.zone,Path(folder))


if __name__=='__main__':
    raise SystemExit(main())
