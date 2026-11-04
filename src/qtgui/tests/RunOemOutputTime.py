"""Run new output-time GUI cases with independent civil tags and every state."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import uuid

from CcsdsOemB1Oracle import ConformanceError,validate
from OemOutputTimeOracle import catalog
from RunOemCreationClock import creation_check


def audit(root,fixture):
    root=Path(root);assert fixture==catalog()
    receipt=json.loads((root/'results.json').read_text());observations=[]
    selected={r['name']:r for r in receipt['results']if r['status']!='not-run'}
    for case in fixture['cases']:
        name='QtGui.NasaOemOutputTime'+case['key']
        if name not in selected:continue
        folder=root/name;path=folder/'mission.oem';issues=[]
        if selected[name]['status']!='passed':issues.append('Native GUI assertions failed')
        if not path.exists():
            observations.append(dict(test=name,status='failed',issues=issues+['No retained OEM output']))
            continue
        raw=path.read_bytes()
        try:finding=validate(raw,icd_object_ids=[fixture['object_id']])
        except ConformanceError as error:finding=dict(status='failed',clause=error.clause,line=error.line,message=error.message)
        profiled_A1=case['time_system']=='A1'and finding.get('status')=='incomplete'and finding.get('gaps')==['Unreviewed ICD time system: A1']
        if finding['status']!='passed'and not profiled_A1:issues.append('Independent B1 format/support check failed')
        if finding['status']in ['passed','incomplete']:
            meta=finding['segments'][0]['metadata']
            if meta['TIME_SYSTEM']!=case['time_system']:issues.append('Metadata time system differs')
            if meta['START_TIME']!=case['expected_epochs'][0]or meta['STOP_TIME']!=case['expected_epochs'][-1]:issues.append('Metadata epochs differ from independent selected-system times')
        rows=[]
        for line in raw.decode('ascii').splitlines():
            fields=line.split()
            if fields and(fields[0].startswith('2000-')or fields[0].startswith('2016-')or fields[0].startswith('2017-')):
                rows.append((fields[0],list(map(float,fields[1:]))))
        expected=case['expected_segments'][0]
        if [r[0]for r in rows]!=case['expected_epochs']:issues.append('Every data epoch does not match independent selected-system calendar')
        position=velocity=0.
        if len(rows)==len(expected)and all(len(r[1])==6 for r in rows):
            position=max(abs(a-b)for row,wanted in zip(rows,expected)for a,b in zip(row[1][:3],wanted[1:4]))
            velocity=max(abs(a-b)for row,wanted in zip(rows,expected)for a,b in zip(row[1][3:],wanted[4:7]))
            if position>=case['position_tolerance']or velocity>=case['velocity_tolerance']:issues.append('Selected clock changed independent physical state')
        else:issues.append('Complete Cartesian record inventory differs')
        clock=creation_check(raw,receipt['started_utc'],receipt['finished_utc'])
        if clock['status']!='passed':issues.append('Creation date is not actual UTC')
        observations.append(dict(test=name,status='failed'if issues else'passed',issues=issues,time_system=case['time_system'],
            records=len(rows),epochs=[r[0]for r in rows],max_position_error_km=position,max_velocity_error_km_s=velocity,
            format_scope=finding,A1_forward_interface_convention_checked=profiled_A1,creation_clock=clock,
            raw_sha256=hashlib.sha256(raw).hexdigest()))
    return dict(status='passed'if observations and all(r['status']=='passed'for r in observations)else'failed',observations=observations,
        test_interface_profile=fixture['test_interface_profile'],reference=fixture['reference'],new_requirement_credit=0,whole_row_qualified=False,
        prior_NarrowAtLast_failure_retained=True,original_assertions_changed=0)


def main():
    import NasaGuiCoverage as nasa
    repo=Path(__file__).resolve().parents[3]
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--visible',action='store_true');p.add_argument('--filter',default='^QtGui.NasaOemOutputTime')
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    names={'src/qtgui/CMakeLists.txt','src/qtgui/tests/NasaOemOutputTimeTests.cpp','src/qtgui/tests/NasaOemOutputTimeCases.json','src/qtgui/tests/OemOutputTimeOracle.py','src/qtgui/tests/CcsdsOemB1Oracle.py','src/qtgui/EphemerisDialog.cpp'}
    names.update(str(p.relative_to(repo))for root in ['src/base','src/gmatutil','plugins']for p in(repo/root).rglob('*')if p.is_file()and p.suffix in {'.cpp','.hpp'})
    review=repo/'build/gui-acceptance'/('oem-output-time-'+uuid.uuid4().hex[:12]+'-review.json');review.parent.mkdir(exist_ok=True)
    review.write_text(json.dumps(dict(sources=nasa.inventory(repo)['sources'],reviews=[],reference_sources={n:hashlib.sha256((repo/n).read_bytes()).hexdigest()for n in sorted(names)},scope='Original E380 five output time systems;no original assertion or science/specification waiver'),indent=2)+'\n')
    command=[sys.executable,str(repo/'src/qtgui/tests/RunAcceptance.py'),'--filter',a.filter,'--nasa-review',str(review.relative_to(repo)),
        '--max-screenshots','0','--max-checkpoints','500','--pause-ms','0','--output',str(a.output)]
    if a.visible:command+=['--visible','--display-route','nested','--display-scale','auto','--start-minimized']
    code=subprocess.call(command,cwd=repo)
    result=audit(a.output,json.loads((repo/'src/qtgui/tests/NasaOemOutputTimeCases.json').read_text()))
    (a.output/'independent-output-time-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    return code or(0 if result['status']=='passed'else 1)


if __name__=='__main__':raise SystemExit(main())
