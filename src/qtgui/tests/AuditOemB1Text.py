"""Replay retained OEM text repairs without native execution or assertion waivers."""
from collections import Counter
import hashlib,json,re,sys,tarfile
from pathlib import Path
from CcsdsOemB1Oracle import validate,ConformanceError
from RunOemB1TextSupport import audit as support_audit
from RunOemCreationClock import creation_check


def audit(repo,stage):
    repo,stage=Path(repo),Path(stage)
    read=lambda p:json.loads(Path(p).read_text())
    sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
    versions={};catalogs={};texts={}
    retained=['NasaEphemerisSamplingTests.cpp','NasaEphemerisSamplingCases.json',
              'NasaOemRecordSupportTests.cpp','NasaOemRecordSupportCases.json']
    for v in range(1,6):
        ids=read(stage/f'executed-source-sha256-v{v}.json')
        with tarfile.open(stage/f'executed-sources-v{v}.tar.gz')as tar:
            assert set(tar.getnames())==set(ids)
            assert all(hashlib.sha256(tar.extractfile(n).read()).hexdigest()==h for n,h in ids.items())
            texts[v]={n:tar.extractfile('src/qtgui/tests/'+n).read().decode()for n in retained+['NasaOemTextFieldsTests.cpp','NasaOemTextFieldsCases.json']}
            catalogs[v]=json.loads(texts[v]['NasaOemTextFieldsCases.json'])
        build=read(stage/f'build-receipt-v{v}.json')
        assert build['status']=='passed'and build['returncode']==0 and build['inputs_unchanged']and build['source_sha256']==ids
        assert catalogs[v]==catalogs[1]
        for n in retained:
            assert sha(stage/'preimages-v1/src/qtgui/tests'/n)==ids['src/qtgui/tests/'+n]
        old=Counter(re.findall(r'\brequire\(.*?\);',texts[v]['NasaOemRecordSupportTests.cpp'],re.S))
        new=Counter(re.findall(r'\brequire\(.*?\);',texts[v]['NasaOemTextFieldsTests.cpp'],re.S))
        assert sum(old.values())==56 and not(old-new),'Original positive assertions lost'
        versions[v]=ids
    catalog=catalogs[1];assert len(catalog['cases'])==7
    assert [c['key']for c in catalog['cases']if c['reject_export']]==['IdOver254','UnicodeId','EmptyId','BlankId']
    assert all(c['position_tolerance']==2e-5 and c['velocity_tolerance']==2e-8 and c['expected_degree']==1 for c in catalog['cases'])
    approved=read(repo/'build/nasa-coverage/coverage95-20261104-oem-creation-utc/assertion-exception-approval-v1.json')
    assert all(v['src/qtgui/tests/NasaEphemerisSamplingCases.json']==approved['after_sha256']for v in versions.values())
    pids=[];native_runs=[]
    runs=[('before-text-headless-v1',1,1,7),('before-text-support-v1',2,1,0),
          ('before-text-support-v2',3,1,1),('after-text-support-v1',4,0,1),
          ('after-text-headless-v1',4,1,9),('after-early-id-headless-v1',5,0,9),
          ('after-early-id-visible-v1',5,0,9),('after-early-id-support-v1',5,0,1)]
    for name,v,code,count in runs:
        maps=read(stage/(name+'-loaded-images.json'));build=stage/f'build-receipt-v{v}.json'
        assert maps['inputs_unchanged']and maps['runtime_binary_copies']==0 and maps['returncode']==code
        assert maps['source_sha256']==versions[v]and maps['build_receipt_sha256']==sha(build)
        assert maps['binary_sha256']==read(build)['binary_sha256']and len(maps['processes'])==count
        for process in maps['processes']:
            assert any(s['project_images'].get(process['executable'])==maps['binary_sha256'][process['executable']]for s in process['snapshots'])
            pids.append(process['pid'])
        native_runs.append(dict(run=name,source_version=v,returncode=code,mapped_processes=count))
    for name,status in [('before-text-support-v2','failed'),('after-text-support-v1','passed'),('after-early-id-support-v1','passed')]:
        result=support_audit(stage/name)
        assert result==read(stage/name/'independent-B1-text-audit.json')and result['status']==status
        assert len(result['observations'])==36
        assert all(x['status']==status for x in result['observations'])
        if status=='passed':
            for observation in result['observations']:
                if observation['expected_rejection']:continue
                finding=observation['format_scope']
                expected_id='A'*231 if observation['case']=='IdAt254'else'A  B'if observation['case']=='SpacedId'else'GMAT-B1-text-native'
                assert finding['segments'][0]['metadata']['OBJECT_ID']==expected_id
                if observation['case']=='OriginatorAt254':
                    assert finding['header']['ORIGINATOR']=='A'*(254-len('ORIGINATOR     = '))
                raw=(stage/name/(observation['case']+'.oem')).read_text()
                states=[line.split()for line in raw.splitlines()if line.startswith('2000-')]
                assert [s[0]for s in states]==['2000-01-01T12:00:00.000','2000-01-01T12:00:01.000']
                assert [list(map(float,s[1:]))for s in states]==[[7000.,0.,0.,0.,7.5,0.],[7000.,7.5,0.,0.,7.5,0.]]
    gui=[]
    for name,v,expected_count in [('before-text-headless-v1',1,7),('after-text-headless-v1',4,9),
                                   ('after-early-id-headless-v1',5,9),('after-early-id-visible-v1',5,9)]:
        root=stage/name;receipt=read(root/'results.json')
        selected=[r for r in receipt['results']if r['status']!='not-run']
        assert len(selected)==expected_count
        for row in selected:
            folder=root/row['name'];rawpath=folder/'mission.oem'
            case=next((c for c in catalog['cases']if row['name']=='QtGui.NasaOemTextFields'+c['key']),None)
            negative=bool(case and case['reject_export'])
            expected='failed'if negative and v<5 else'passed'
            assert row['status']==expected
            if negative and v==5:
                proof=read(folder/'proof.json')
                assert proof['expected_rejection']and proof['original_identifier_unchanged']and proof['source_history_unchanged']
                if rawpath.exists():
                    raw=rawpath.read_bytes()
                    assert all(len(line)<=254 and all(32<=c<=126 for c in line)for line in raw.splitlines())
                    assert b'OBJECT_ID'not in raw
            else:
                raw=rawpath.read_bytes()
                if negative:
                    try:validate(raw,icd_object_ids=catalog['test_interface_profile']['object_ids'])
                    except ConformanceError as error:
                        if v==1:assert error.clause=={'IdOver254':'4.3.1(a)','UnicodeId':'4.3.1(b)','EmptyId':'4.3.3(h)','BlankId':'4.3.3(h)'}[case['key']]
                    else:raise AssertionError('Retained invalid/partial file passed')
                else:
                    legacy=row['name']=='QtGui.NasaEphemerisSamplingShortArc'
                    controlled=row['name']=='QtGui.NasaOemRecordSupportOneRecord'
                    finding=validate(raw,icd_object_ids=[]if legacy else['GMAT-Qt-OEM-record-support']if controlled else catalog['test_interface_profile']['object_ids'])
                    assert finding['status']==('incomplete'if legacy else'passed')
                    if case:
                        assert finding['segments'][0]['metadata']['OBJECT_ID']==case.get('object_id',catalog['object_id'])
                    if legacy:assert finding['gaps']==['Non-COSPAR object ID needs an ICD: SATID']
                    proof=read(folder/'proof.json');assert proof['outcome']=='passed'and proof['numeric_sentinel']==9
                    assert proof['max_position_error']<2e-5 and proof['max_velocity_error']<2e-8
                    clock=creation_check(raw.partition(b'META_START')[0],receipt['started_utc'],receipt['finished_utc'])
                    assert clock['status']=='passed'
            gui.append(dict(run=name,test=row['name'],status=expected))
    visible=read(stage/'after-early-id-visible-v1/results.json');display=visible['visible_display']
    assert visible['resizable']and display['resizable']and display['start_minimized']and not display['resize_errors']
    assert 'Iconic'in display['initial_host_window_state']and 'FULLSCREEN'not in display['initial_host_window_state']
    assert visible['end_reason']=='completed-selection'
    terminal=read(stage/'process-terminal-v1.json')
    assert terminal['native_identities']==len(pids)==37 and terminal['display_identities']==1
    assert {r['pid']for r in terminal['records']}==set(pids+[display['server_pid']])
    assert all(r['observed_terminal']for r in terminal['records'])
    before=read(stage/'core-before-v1.json')['sha256'];inventory=read(stage/'core-change-inventory-v1/inventory.json')
    actual={x['path']:x['working_sha256']for x in inventory['files']}
    increment=read(stage/'core-increment-v1.json')
    assert len(before)==284 and len(actual)==285 and set(actual)-set(before)=={'src/gmatutil/util/CcsdsOemB1Text.hpp'}
    assert {n for n,h in before.items()if actual[n]!=h}==set(increment['existing_increments'])
    assert len(increment['existing_increments'])==5 and increment['unchanged_prior_paths']==279
    assert all(versions[5].get(n)==h for n,h in actual.items())
    canonical=repo/'build/nasa-coverage/coverage95-20261104-matlab-assignment-row'
    for name,count in [('requirement-by-requirement-audit-v1.json',1158),('remaining-requirement-audit-v1.json',107)]:
        original=read(canonical/name);current=read(stage/name)
        assert current['rows']==original['rows']and len(current['rows'])==count
        assert current['original_hold_keys']==original['original_hold_keys']and len(current['original_hold_keys'])==59
    assert read(stage/'prior-UTC-component-replay-v1.json')['requirements_verified']==1051
    assert read(stage/'prior-1051-prefix-replay-v1.json')['status']=='passed'
    return dict(status='B1-text-repair-replayed;whole-OEM-row-open',requirements_verified=1051,original_total=1158,
        remaining=107,still_needed=50,original_hold_identities=59,new_requirement_credit=0,
        native_GUI_contexts=34,native_GUI_passes=26,native_GUI_failures_retained=8,
        native_writer_cases=36,writer_runs_with_mapped_images=3,writer_baseline_semantic_failures_retained=36,
        writer_post_repair_case_passes=72,unmapped_startup_preflight_failures_retained=1,
        original_positive_require_expressions_preserved=56,original_catalog_assertion_changes_this_stage=0,
        approved_original_ShortArc_degree_one_preserved=True,pending_core_paths=285,unchanged_prior_core_paths=279,
        new_members=0,new_virtual_slots=0,new_exported_APIs=0,native_runs=native_runs,GUI=gui,
        terminal_native_identities=37,terminal_display_identities=1,
        prior_NarrowAtLast_scientific_failure_retained=True,technical_core_approval=False,
        full_current_build_qualified=False,full_NASA_qualified=False,goal_complete=False)


if __name__=='__main__':
    print(json.dumps(audit(Path.cwd(),sys.argv[1]),indent=2))
