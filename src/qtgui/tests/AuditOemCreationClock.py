"""Replay UTC header repair and the single explicitly approved ShortArc expectation change."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import tarfile

from CcsdsOemB1Oracle import validate
from RunOemCreationClock import audit as clock_audit, creation_check


def audit(repo,stage):
    repo,stage=Path(repo),Path(stage)
    read=lambda p:json.loads(Path(p).read_text())
    sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
    versions={};catalogs={}
    for version in [1,2]:
        identities=read(stage/f'executed-source-sha256-v{version}.json')
        with tarfile.open(stage/f'executed-sources-v{version}.tar.gz')as archive:
            members={m.name:m for m in archive.getmembers()}
            assert set(members)==set(identities)
            assert all(hashlib.sha256(archive.extractfile(members[n]).read()).hexdigest()==h
                       for n,h in identities.items())
            catalogs[version]={}
            for name in ['NasaEphemerisSamplingCases.json','NasaOemRecordSupportCases.json']:
                catalogs[version][name]=json.load(archive.extractfile('src/qtgui/tests/'+name))
        build=read(stage/f'build-receipt-v{version}.json')
        assert build['status']=='passed' and build['returncode']==0 and build['inputs_unchanged']
        assert build['source_sha256']==identities
        versions[version]=identities
    before=read(stage/'preimages-v1/src/qtgui/tests/NasaEphemerisSamplingCases.json')
    expected=copy.deepcopy(before)
    case=next(c for c in expected['cases']if c['key']=='ShortArc')
    assert case['expected_degree']==2 and len(case['expected_segments'][0])==2
    case['expected_degree']=1
    approval=read(stage/'assertion-exception-approval-v1.json')
    assert approval['status']=='explicitly-approved-and-applied' and approval['user_reply']=='Sure'
    assert approval['before_sha256']==sha(stage/'preimages-v1/src/qtgui/tests/NasaEphemerisSamplingCases.json')
    for version in [1,2]:
        assert catalogs[version]['NasaEphemerisSamplingCases.json']==expected
        assert versions[version]['src/qtgui/tests/NasaEphemerisSamplingCases.json']==approval['after_sha256']
        assert versions[version]['src/qtgui/tests/NasaEphemerisSamplingTests.cpp']==sha(stage/'preimages-v1/src/qtgui/tests/NasaEphemerisSamplingTests.cpp')
        assert versions[version]['src/gmatutil/util/TimeTypes.cpp']==sha(stage/'preimages-v1/src/gmatutil/util/TimeTypes.cpp')
        assert versions[version]['src/gmatutil/util/CCSDSEMWriter.hpp']==sha(stage/'preimages-v1/src/gmatutil/util/CCSDSEMWriter.hpp')
    support=[('before-UTC-v1',1,'passed'),('before-Denver-v1',1,'failed'),
             ('before-Kolkata-v1',1,'failed'),('after-UTC-v2',2,'passed'),
             ('after-Denver-v2',2,'passed'),('after-Kolkata-v2',2,'passed')]
    gui=[('before-GUI-headless-v1',1,'failed'),('after-GUI-headless-v2',2,'passed'),
         ('after-GUI-visible-v2',2,'passed')]
    pids=[];observations=[]
    for name,version,status in support+gui:
        maps=read(stage/(name+'-loaded-images.json'))
        assert maps['inputs_unchanged'] and maps['runtime_binary_copies']==0
        assert maps['source_sha256']==versions[version]
        assert maps['returncode']==(0 if status=='passed'else 1)
        receipt=stage/f'build-receipt-v{version}.json'
        assert maps['build_receipt_sha256']==sha(receipt)
        assert maps['binary_sha256']==read(receipt)['binary_sha256']
        for process in maps['processes']:
            assert any(s['project_images'].get(process['executable'])==maps['binary_sha256'][process['executable']]
                       for s in process['snapshots'])
            pids.append(process['pid'])
    for name,version,status in support:
        result=clock_audit(stage/name)
        assert result==read(stage/name/'clock-independent-audit.json')
        assert result['status']==status and result['generic_local_clock_preserved']
        assert len(read(stage/(name+'-loaded-images.json'))['processes'])==1
        observations.append(dict(run=name,scope='native-header-clock-and-preserved-local-clock',status=status))
    for name,version,status in gui:
        root=stage/name;results=read(root/'results.json')
        selected=[r for r in results['results']if r['status']!='not-run']
        assert len(selected)==2 and all(r['status']=='passed'for r in selected)
        assert len(read(stage/(name+'-loaded-images.json'))['processes'])==2
        recorded=read(root/'independent-UTC-creation-audit.json')['observations']
        assert {x['test']for x in recorded}=={r['name']for r in selected}
        for row in selected:
            controlled=row['name']=='QtGui.NasaOemRecordSupportOneRecord'
            assert controlled or row['name']=='QtGui.NasaEphemerisSamplingShortArc'
            case=next(c for c in catalogs[version]['NasaOemRecordSupportCases.json'if controlled else'NasaEphemerisSamplingCases.json']['cases']
                      if c['key']==('OneRecord'if controlled else'ShortArc'))
            raw=(root/row['name']/'mission.oem').read_bytes()
            finding=validate(raw,icd_object_ids=['GMAT-Qt-OEM-record-support']if controlled else[])
            assert finding['status']==('passed'if controlled else'incomplete')
            if not controlled:
                assert finding['gaps']==['Non-COSPAR object ID needs an ICD: SATID']
            assert finding['segments'][0]['records']==(1 if controlled else 2)
            clock=creation_check(raw,results['started_utc'],results['finished_utc'])
            assert clock['status']==status
            proof=read(root/row['name']/'proof.json')
            assert proof['outcome']=='passed' and proof['numeric_sentinel']==9
            assert proof['max_position_error']<case['position_tolerance']
            assert proof['max_velocity_error']<case['velocity_tolerance']
            observations.append(dict(run=name,test=row['name'],native='passed',UTC_creation=status,
                                     format_scope=finding['status'],legacy_ICD_invented=False))
    visible=read(stage/'after-GUI-visible-v2/results.json')
    display=visible['visible_display']
    assert visible['resizable'] and display['resizable'] and display['start_minimized']
    assert 'Iconic'in display['initial_host_window_state'] and not display['resize_errors']
    assert visible['end_reason']=='completed-selection'
    terminal=read(stage/'process-terminal-v1.json')
    assert terminal['native_identities']==len(pids)==12 and terminal['display_identities']==1
    assert {r['pid']for r in terminal['records']}==set(pids+[display['server_pid']])
    assert all(r['observed_terminal']for r in terminal['records'])
    core=read(stage/'core-change-inventory-v1/inventory.json')
    actual={r['path']:r['working_sha256']for r in core['files']}
    before=read(stage/'core-before-v1.json')['sha256']
    assert len(actual)==284 and all(actual.get(n)==h for n,h in before.items())
    assert set(actual)-set(before)=={'src/gmatutil/util/CCSDSEMWriter.cpp'}
    assert all(versions[2].get(n)==h for n,h in actual.items())
    remaining=read(stage/'remaining-requirement-audit-v1.json')
    assert remaining['remaining']==107 and remaining['still_needed']==50
    assert len(remaining['original_hold_keys'])==59
    canonical=repo/'build/nasa-coverage/coverage95-20261104-matlab-assignment-row'
    original_remaining=read(canonical/'remaining-requirement-audit-v1.json')
    assert remaining['rows']==original_remaining['rows']
    assert remaining['original_hold_keys']==original_remaining['original_hold_keys']
    full=read(stage/'requirement-by-requirement-audit-v1.json')
    original_full=read(canonical/'requirement-by-requirement-audit-v1.json')
    assert full['rows']==original_full['rows'] and len(full['rows'])==1158
    assert full['original_hold_keys']==original_full['original_hold_keys']
    assert full['source_versioned_qualified']==1051 and full['still_needed']==50
    return dict(status='UTC-creation-repair-and-approved-expectation-replayed;whole-row-open',
                canonical_verified=1051,original_total=1158,target=1101,remaining=107,still_needed=50,
                original_hold_identities=59,new_requirement_credit=0,
                native_support_contexts=6,native_support_semantic_failures_retained=2,
                native_GUI_contexts=6,native_GUI_assertion_passes=6,pre_repair_GUI_UTC_failures_retained=2,
                post_repair_support_zones_passed=3,post_repair_GUI_passed_both_profiles=2,
                generic_local_time_utility_unchanged=True,approved_original_assertion_changes=1,
                all_other_original_catalog_values_and_bounds_preserved=True,
                pending_core_paths=284,previous_283_core_paths_unchanged=True,new_core_APIs=0,
                observations=observations,externally_mapped_native_pids=pids,
                visible_server_pid=display['server_pid'],technical_core_approval=False,
                full_current_build_qualified=False,full_NASA_qualified=False,goal_complete=False)


if __name__=='__main__':
    print(json.dumps(audit(Path.cwd(),sys.argv[1]),indent=2))
