"""Run new GUI OEM frame cases and check every retained state independently."""
import argparse
import csv
from datetime import datetime
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import uuid

from CcsdsOemB1Oracle import ConformanceError,validate
from OemFrameStateOracle import catalog,exported,circle
from RunOemCreationClock import creation_check


def audit(root,fixture):
    root=Path(root)
    assert fixture==catalog(),'Executed frame fixture differs from independent derivation'
    receipt=json.loads((root/'results.json').read_text())
    observations=[]
    selected=[r for r in receipt['results']if r['status']!='not-run']
    by_name={r['name']:r for r in selected}
    for case in fixture['cases']:
        name='QtGui.NasaOemFrameState'+case['key']
        if name not in by_name:continue
        folder=root/name;path=folder/'mission.oem';issues=[]
        if by_name[name]['status']!='passed':issues.append('Native GUI assertions failed')
        if not path.exists():
            observations.append(dict(test=name,status='failed',issues=issues+['No retained OEM output']))
            continue
        raw=path.read_bytes()
        try:format_scope=validate(raw,icd_object_ids=[fixture['object_id']])
        except ConformanceError as error:
            format_scope=dict(status='failed',clause=error.clause,line=error.line,message=error.message)
        if format_scope['status']!='passed':issues.append('Independent B1 file check failed')
        else:
            assert len(format_scope['segments'])==1
            metadata=format_scope['segments'][0]['metadata']
            if metadata['OBJECT_ID']!=fixture['object_id']or metadata['OBJECT_NAME']!='Sat':issues.append('Exported identity differs')
            if metadata['CENTER_NAME'].upper()!=case['expected_center']or metadata['REF_FRAME']!=case['expected_frame']:issues.append('Frame/center metadata differs')
        rows=[]
        for line in raw.decode('ascii').splitlines():
            fields=line.split()
            if fields and fields[0].startswith('2000-'):
                seconds=(datetime.fromisoformat(fields[0])-datetime(2000,1,1,12)).total_seconds()
                rows.append([seconds]+list(map(float,fields[1:])))
        wanted=[[t]+exported(t,case['plane'],case['frame_kind'])for t in [0,5,10,15,20]]
        if [r[0]for r in rows]!=[r[0]for r in wanted]or any(len(r)!=7 for r in rows):issues.append('Complete native-step schedule differs')
        position=velocity=0.
        if len(rows)==len(wanted)and all(len(r)==7 for r in rows):
            position=max(abs(a-b)for row,truth in zip(rows,wanted)for a,b in zip(row[1:4],truth[1:4]))
            velocity=max(abs(a-b)for row,truth in zip(rows,wanted)for a,b in zip(row[4:7],truth[4:7]))
            if not all(math.isfinite(v)for r in rows for v in r):issues.append('Nonfinite output state')
            if position>=case['position_tolerance']or velocity>=case['velocity_tolerance']:issues.append('Independent transformed-circle bound failed')
        terminal=list(csv.reader((folder/'frame-final.csv').read_text().splitlines()))if(folder/'frame-final.csv').exists()else[]
        if len(terminal)!=1 or len(terminal[0])!=7:issues.append('Output-frame terminal witness missing')
        elif rows:
            values=list(map(float,terminal[0]))
            if any(abs(rows[-1][i]-values[i])>=(2e-10 if i<4 else 2e-13)for i in range(1,7)):
                issues.append('Terminal OEM differs from same-frame native witness')
        native=list(csv.reader((folder/'final.csv').read_text().splitlines()))if(folder/'final.csv').exists()else[]
        if len(native)!=1 or len(native[0])!=8:issues.append('Earth-frame final/sentinel witness missing')
        else:
            values=list(map(float,native[0]));truth=[20]+circle(20,case['plane'])+[9]
            if any(abs(a-b)>=(1e-4 if i==0 else 2e-5 if i<4 else 2e-8)for i,(a,b)in enumerate(zip(values,truth))):issues.append('Earth-frame final/sentinel differs')
        clock=creation_check(raw,receipt['started_utc'],receipt['finished_utc'])
        if clock['status']!='passed':issues.append('UTC creation field outside run interval')
        observations.append(dict(test=name,status='failed'if issues else'passed',issues=issues,
            frame_kind=case['frame_kind'],plane=case['plane'],records=len(rows),max_position_error_km=position,
            max_velocity_error_km_s=velocity,format_scope=format_scope,creation_clock=clock,
            raw_sha256=hashlib.sha256(raw).hexdigest()))
    return dict(status='passed'if observations and all(r['status']=='passed'for r in observations)else'failed',
        observations=observations,test_interface_profile=fixture['test_interface_profile'],
        new_requirement_credit=0,whole_row_qualified=False,
        prior_NarrowAtLast_failure_retained=True,scope='New every-record declared rotation/origin/units proof;original source assertions and all broader gates retained')


def main():
    import NasaGuiCoverage as nasa
    repo=Path(__file__).resolve().parents[3]
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--visible',action='store_true')
    p.add_argument('--filter',default='^QtGui.NasaOemFrameState');p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    names={'src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaOemFrameStateTests.cpp','src/qtgui/tests/NasaOemFrameStateCases.json',
        'src/qtgui/tests/OemFrameStateOracle.py','src/qtgui/tests/CcsdsOemB1Oracle.py','src/qtgui/tests/RunOemCreationClock.py'}
    names.update(str(p.relative_to(repo))for root in ['src/base','src/gmatutil','plugins']for p in(repo/root).rglob('*')if p.is_file()and p.suffix in {'.cpp','.hpp'})
    review=repo/'build/gui-acceptance'/('oem-frame-state-'+uuid.uuid4().hex[:12]+'-review.json');review.parent.mkdir(exist_ok=True)
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],
        reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest()for n in sorted(names)},
        scope='New declared OEM rotation/origin/units cases only;no original assertion or scope waiver'),indent=2)+'\n')
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',a.filter,'--nasa-review',str(review.relative_to(repo)),
        '--max-screenshots','0','--max-checkpoints','500','--pause-ms','0','--output',str(a.output)]
    if a.visible:command+=['--visible','--display-route','nested','--display-scale','auto','--start-minimized']
    code=subprocess.call(command,cwd=repo)
    result=audit(a.output,json.loads((repo/'src/qtgui/tests/NasaOemFrameStateCases.json').read_text()))
    (a.output/'independent-frame-state-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    return code or(0 if result['status']=='passed'else 1)


if __name__=='__main__':raise SystemExit(main())
