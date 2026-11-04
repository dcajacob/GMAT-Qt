"""Replay explicit OEM window evidence, preserving all scientific and source conflicts."""
from datetime import datetime, timedelta, timezone
import hashlib
import json
from pathlib import Path
import re
import sys
import tarfile

from CcsdsOemB1Oracle import ConformanceError, validate
from OemShortWindowScience import diagnostic


def require_expressions(text):
    calls=[]
    for match in re.finditer(r'\brequire\s*\(',text):
        begin=match.start();position=match.end();depth=1;quote=None
        while depth:
            char=text[position];position+=1
            if quote:
                if char=='\\':position+=1
                elif char==quote:quote=None
            elif char in ('"',"'"):quote=char
            elif char=='(':depth+=1
            elif char==')':depth-=1
        calls.append(text[begin:position])
    return calls


def audit(repo,stage):
    repo,stage=Path(repo),Path(stage)
    read=lambda p:json.loads(Path(p).read_text())
    sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
    versions={};catalogs={};drivers={}
    for version in range(1,6):
        identities=read(stage/f'executed-source-sha256-v{version}.json')
        with tarfile.open(stage/f'executed-sources-v{version}.tar.gz')as archive:
            members={m.name:m for m in archive.getmembers()}
            assert set(members)==set(identities)
            assert all(hashlib.sha256(archive.extractfile(members[n]).read()).hexdigest()==h
                       for n,h in identities.items())
            catalogs[version]=json.load(archive.extractfile('src/qtgui/tests/NasaOemShortWindowCases.json'))
            drivers[version]=archive.extractfile('src/qtgui/tests/NasaOemRecordSupportTests.cpp').read().decode()
        versions[version]=identities
        build=read(stage/f'build-receipt-v{version}.json')
        assert build['status']=='passed' and build['returncode']==0 and build['inputs_unchanged']
        assert build['source_sha256']==identities
    original=['src/qtgui/tests/NasaEphemerisSamplingTests.cpp',
              'src/qtgui/tests/NasaEphemerisSamplingCases.json',
              'src/qtgui/tests/NasaOemRecordSupportCases.json']
    for name in original:
        assert all(v[name]==sha(stage/'preimages-v1'/name) for v in versions.values())
    expressions=require_expressions((stage/'preimages-v1/src/qtgui/tests/NasaOemRecordSupportTests.cpp').read_text())
    assert all(require_expressions(driver)==expressions for driver in drivers.values())
    runs=[('before-fix-windows-headless-v1',1,0,6),
          ('after-fix-windows-headless-v2',2,7,2),
          ('eof-and-diagnostic-headless-v4',4,2,1),
          ('windows-and-aligned-visible-v4',4,9,1),
          ('split-boundary-headless-v5',5,1,0),
          ('split-boundary-visible-v5',5,1,0)]
    observations=[];pids=[];passes={};findings={}
    for name,version,passed,failed in runs:
        root=stage/name;maps=read(stage/(name+'-loaded-images.json'))
        assert maps['returncode']==(1 if failed else 0) and maps['inputs_unchanged'] and maps['runtime_binary_copies']==0
        assert maps['source_sha256']==versions[version]
        receipt=stage/f'build-receipt-v{version}.json'
        assert maps['build_receipt_sha256']==sha(receipt)
        assert maps['binary_sha256']==read(receipt)['binary_sha256']
        results=[x for x in read(root/'results.json')['results']if x['status']!='not-run']
        assert len(results)==len(maps['processes'])==passed+failed
        assert sum(x['status']=='passed'for x in results)==passed
        assert sum(x['status']=='failed'for x in results)==failed
        cases={c['key']:c for c in catalogs[version]['cases']}
        profile='visible'if maps['visible']else'headless'
        for process in maps['processes']:
            assert any(s['project_images'].get(process['executable'])==maps['binary_sha256'][process['executable']]
                       for s in process['snapshots'])
            pids.append(process['pid'])
        for result in results:
            prefix='QtGui.NasaOemShortWindow'
            if result['name'].startswith(prefix):case=cases[result['name'][len(prefix):]]
            else:
                assert result['name']=='QtGui.NasaOemRecordSupportOneRecord'
                with tarfile.open(stage/f'executed-sources-v{version}.tar.gz')as archive:
                    prior=json.load(archive.extractfile('src/qtgui/tests/NasaOemRecordSupportCases.json'))
                case=next(c for c in prior['cases']if c['key']=='OneRecord')
            path=root/result['name']/'mission.oem'
            try:finding=validate(path.read_bytes(),icd_object_ids=[catalogs[version]['object_id']])
            except ConformanceError as error:finding=dict(status='failed',clause=error.clause,line=error.line,message=error.message)
            if result['status']=='passed':
                assert finding['status']=='passed'
                assert [s['records']for s in finding['segments']]==[len(s)for s in case['expected_segments']]
                assert [int(s['metadata']['INTERPOLATION_DEGREE'])for s in finding['segments']]==case['expected_degrees']
                proof=read(root/result['name']/'proof.json')
                assert proof['outcome']=='passed' and proof['numeric_sentinel']==9
                assert proof['max_position_error']<case['position_tolerance']
                assert proof['max_velocity_error']<case['velocity_tolerance']
                passes.setdefault(result['name'],set()).add(profile)
            observations.append(dict(run=name,test=result['name'],native=result['status'],format_syntax_counts=finding['status']))
            findings[name,result['name']]=finding
    assert len([x for x in passes.values()if x=={'headless','visible'}])==10
    final=read(stage/'windows-and-aligned-visible-v4/results.json')
    display=final['visible_display']
    assert final['resizable'] and display['resizable'] and display['start_minimized']
    assert 'Iconic'in display['initial_host_window_state'] and not display['resize_errors']
    assert final['end_reason']=='completed-selection'
    split=read(stage/'split-boundary-visible-v5/results.json')
    split_display=split['visible_display']
    assert split['resizable'] and split_display['resizable'] and split_display['start_minimized']
    assert 'Iconic'in split_display['initial_host_window_state'] and not split_display['resize_errors']
    assert split['end_reason']=='completed-selection'
    servers=[display['server_pid'],split_display['server_pid']]
    case=next(c for c in catalogs[4]['cases']if c['key']=='NarrowAtLast')
    science=[]
    for name in ['eof-and-diagnostic-headless-v4','windows-and-aligned-visible-v4']:
        result=diagnostic(stage/name/'QtGui.NasaOemShortWindowNarrowAtLast',case)
        assert result==read(stage/(name+'-science-diagnostic.json'))
        assert findings[name,'QtGui.NasaOemShortWindowNarrowAtLast']['status']=='passed'
        science.append(dict(run=name,position_error_km=result['max_position_error_km'],
                            velocity_error_km_s=result['max_velocity_error_km_s']))
    before=read(stage/'core-before-v1.json')['sha256']
    core=read(stage/'core-change-inventory-v1/inventory.json')
    after={x['path']:x['working_sha256']for x in core['files']}
    assert set(after)==set(before) and len(after)==283
    assert {n for n,h in after.items()if before[n]!=h}=={'src/base/subscriber/EphemWriterCCSDS.cpp'}
    assert all(versions[version].get(n)==h for version in [4,5] for n,h in after.items())
    terminal=read(stage/'process-terminal-v1.json')
    assert {x['pid']for x in terminal['records']}==set(pids+servers)
    assert all(x['observed_terminal']for x in terminal['records'])
    remaining=read(stage/'remaining-requirement-audit-v1.json')
    assert remaining['remaining']==107 and remaining['still_needed']==50
    assert len(remaining['original_hold_keys'])==59
    clock=read(stage/'creation-clock-source-review-v1.json')
    stamp=datetime.fromisoformat(clock['serialized_creation_date']).replace(tzinfo=timezone.utc)
    lower=datetime.fromisoformat(final['started_utc']);upper=datetime.fromisoformat(final['finished_utc'])
    assert not lower<=stamp<=upper and lower<=stamp+timedelta(hours=7)<=upper
    return dict(status='window-repair-replayed;scientific-and-format-gaps-retained',
                canonical_verified=1051,original_total=1158,target=1101,remaining=107,still_needed=50,
                original_hold_identities=59,new_requirement_credit=0,native_GUI_contexts=30,
                native_GUI_passes=20,native_GUI_failures_retained=10,passing_cases_both_profiles=10,
                original_require_expressions_preserved=len(expressions),original_catalogs_unchanged=True,
                pending_core_paths=283,prior_core_paths_unchanged=282,new_core_paths=0,
                source3_native_executions=0,observations=observations,retained_scientific_failure=science,
                confirmed_creation_clock_UTC_violation=True,externally_mapped_native_pids=pids,
                visible_server_pids=servers,technical_core_approval=False,
                full_current_build_qualified=False,full_NASA_qualified=False,goal_complete=False)


if __name__=='__main__':
    print(json.dumps(audit(Path.cwd(),sys.argv[1]),indent=2))
