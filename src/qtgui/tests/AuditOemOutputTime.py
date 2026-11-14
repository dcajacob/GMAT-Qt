"""Replay all five OEM output clocks without weakening original evidence or gates."""
from collections import Counter
import hashlib,json,re,sys,tarfile
from pathlib import Path
from RunOemOutputTime import audit as time_audit
from RunOemOutputTimeSupport import audit as support_audit
from OemOutputTimeOracle import catalog,support_catalog,SYSTEMS
from CcsdsOemB1Oracle import validate
from RunOemCreationClock import creation_check


def audit(repo,stage):
    repo,stage=Path(repo),Path(stage)
    read=lambda p:json.loads(Path(p).read_text())
    sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
    versions={};fixtures={};bodies={}
    retained=['NasaEphemerisSamplingTests.cpp','NasaEphemerisSamplingCases.json','NasaOemRecordSupportTests.cpp','NasaOemRecordSupportCases.json','CcsdsOemB1Oracle.py']
    for v in range(1,6):
        ids=read(stage/f'executed-source-sha256-v{v}.json')
        with tarfile.open(stage/f'executed-sources-v{v}.tar.gz')as tar:
            assert set(tar.getnames())==set(ids)
            assert all(hashlib.sha256(tar.extractfile(n).read()).hexdigest()==h for n,h in ids.items())
            fixture=json.load(tar.extractfile('src/qtgui/tests/NasaOemOutputTimeCases.json'))
            bodies[v]=tar.extractfile('src/qtgui/tests/NasaOemOutputTimeTests.cpp').read().decode()
            old=tar.extractfile('src/qtgui/tests/NasaOemRecordSupportTests.cpp').read().decode()
        assert fixture==catalog()and len(fixture['cases'])==16
        assert {(c['time_system'],c['epoch_family'])for c in fixture['cases']if c['key']!='TAI_TTInputWindow'}=={(s,e)for s in SYSTEMS for e in ['J2000','April2000','Leap2016']}
        assert all(c['position_tolerance']==2e-5 and c['velocity_tolerance']==2e-8 for c in fixture['cases'])
        original=Counter(re.findall(r'\brequire\(.*?\);',old,re.S));current=Counter(re.findall(r'\brequire\(.*?\);',bodies[v],re.S))
        assert sum(original.values())==56 and sum((original-current).values())==2
        assert all('date.isValid()'in s or'meta["TIME_SYSTEM"]=="UTC"'in s for s in original-current)
        for n in retained:assert ids['src/qtgui/tests/'+n]==sha(stage/'preimages-v1/src/qtgui/tests'/n)
        assert ids['src/gmatutil/util/TimeSystemConverter.cpp']==sha(stage/'preimages-v1/src/gmatutil/util/TimeSystemConverter.cpp')
        if v==4:
            assert not(stage/'build-receipt-v4.json').exists()and read(stage/'unexecuted-source4-review-v1.json')['tests_rerun']==0
        else:
            build=read(stage/f'build-receipt-v{v}.json');assert build['status']=='passed'and build['returncode']==0 and build['inputs_unchanged']and build['source_sha256']==ids
        versions[v]=ids;fixtures[v]=fixture
    approved=read(repo/'build/nasa-coverage/coverage95-20261104-oem-creation-utc/assertion-exception-approval-v1.json')
    assert all(ids['src/qtgui/tests/NasaEphemerisSamplingCases.json']==approved['after_sha256']for ids in versions.values())
    core=read(stage/'core-change-inventory-v1/inventory.json');actual={r['path']:r['working_sha256']for r in core['files']}
    before=read(stage/'core-before-v1.json')['sha256'];increment=read(stage/'core-increment-v1.json')
    assert len(before)==285 and len(actual)==286
    assert set(actual)-set(before)=={'src/base/subscriber/EphemWriterCCSDS.hpp'}
    assert {n for n,h in before.items()if actual[n]!=h}==set(increment['existing_increments'])and len(increment['existing_increments'])==5
    assert increment['unchanged_prior_paths']==280 and increment['new_virtual_slots']==0
    assert all(versions[v].get(n)==h for v in [2,3,4,5]for n,h in actual.items())
    assert all(versions[1].get(n)==h for n,h in before.items())
    for n in ['src/qtgui/EphemerisDialog.cpp','src/qtgui/ResourceProperties.cpp']:
        assert len({versions[v][n]for v in [2,3,4,5]})==1
    producer=read(stage/'build-receipt-v2.json')['binary_sha256']
    producer_names=[n for n in producer if Path(n).name in ['GmatQt-R2026a','libGmatBase.so.R2026a','libGmatUtil.so.R2026a','libMatlabInterface.so.R2026a']]
    assert len(producer_names)==4
    for v in [3,5]:assert all(read(stage/f'build-receipt-v{v}.json')['binary_sha256'][n]==producer[n]for n in producer_names)
    runs=[('missing-option-headless-v1',1,1,1),('five-clocks-headless-v2',2,0,18),('five-clocks-visible-v2',2,0,18),
        ('writer-clock-support-v3',3,0,1),('committed-format-headless-v3',3,1,1),('committed-format-headless-v5',5,0,1),('committed-format-visible-v5',5,0,1)]
    pids=[]
    for name,v,code,count in runs:
        maps=read(stage/(name+'-loaded-images.json'));build=stage/f'build-receipt-v{v}.json'
        assert maps['returncode']==code and maps['inputs_unchanged']and maps['runtime_binary_copies']==0 and maps['source_sha256']==versions[v]
        assert maps['build_receipt_sha256']==sha(build)and maps['binary_sha256']==read(build)['binary_sha256']and len(maps['processes'])==count
        for process in maps['processes']:
            assert any(s['project_images'].get(process['executable'])==maps['binary_sha256'][process['executable']]for s in process['snapshots'])
            pids.append(process['pid'])
    baseline=read(stage/'missing-option-headless-v1/results.json');selected=[r for r in baseline['results']if r['status']!='not-run']
    assert len(selected)==1 and selected[0]['status']=='failed'and 'OEM OutputTimeSystem selector missing'in(stage/'missing-option-headless-v1-driver.log').read_text()
    observations=[]
    for name,v,count,status in [('five-clocks-headless-v2',2,16,'passed'),('five-clocks-visible-v2',2,16,'passed'),
        ('committed-format-headless-v3',3,1,'failed'),('committed-format-headless-v5',5,1,'passed'),('committed-format-visible-v5',5,1,'passed')]:
        root=stage/name;result=time_audit(root,fixtures[v]);assert result==read(root/'independent-output-time-audit.json')and result['status']==status and len(result['observations'])==count
        for row in result['observations']:
            assert row['status']==status
            if status=='passed':
                proof=read(root/row['test']/'proof.json');assert proof['outcome']=='passed'and proof['numeric_sentinel']==9
                assert proof['max_position_error']==row['max_position_error_km']and proof['max_velocity_error']==row['max_velocity_error_km_s']
            else:assert row['issues']==['Native GUI assertions failed']
            if row['time_system']=='A1':assert row['A1_forward_interface_convention_checked']and row['format_scope']['gaps']==['Unreviewed ICD time system: A1']
        if count==16:
            assert {r['test']for r in result['observations']}=={'QtGui.NasaOemOutputTime'+c['key']for c in catalog()['cases']}
        observations.append(dict(run=name,time_cases=count,status=status))
    assert 'const auto beforeSwitch=source->toPlainText();'not in bodies[2]
    assert 'const auto beforeSwitch=source->toPlainText();'in bodies[5]and 'currentItems=tree->findItems'in bodies[5]
    support=support_audit(stage/'writer-clock-support-v3',support_catalog())
    assert support==read(stage/'writer-clock-support-v3/independent-output-time-support-audit.json')and support['status']=='passed'and len(support['observations'])==5
    normalized=lambda raw:b'\n'.join(line for line in raw.splitlines()if not line.startswith(b'CREATION_DATE'))
    for mode in ['headless','visible']:
        root=stage/('five-clocks-'+mode+'-v2');receipt=read(root/'results.json')
        selected=[r for r in receipt['results']if r['status']!='not-run'];assert len(selected)==18 and all(r['status']=='passed'and r['returncode']==0 for r in selected)
        for name,old in [('QtGui.NasaEphemerisSamplingShortArc',repo/'build/nasa-coverage/coverage95-20261104-oem-b1-text'/('after-early-id-'+mode+'-v1')),
            ('QtGui.NasaOemFrameStateRelativeEquator_XY',repo/'build/nasa-coverage/coverage95-20261104-oem-frame-state'/('frames-'+mode+'-v1'))]:
            raw=(root/name/'mission.oem').read_bytes();assert normalized(raw)==normalized((old/name/'mission.oem').read_bytes())
            proof=read(root/name/'proof.json');assert proof['outcome']=='passed'and proof['numeric_sentinel']==9
            assert creation_check(raw,receipt['started_utc'],receipt['finished_utc'])['status']=='passed'
    displays=[]
    for name in ['five-clocks-visible-v2','committed-format-visible-v5']:
        receipt=read(stage/name/'results.json');display=receipt['visible_display']
        assert receipt['resizable']and display['resizable']and display['start_minimized']and not display['resize_errors']
        assert 'Iconic'in display['initial_host_window_state']and 'FULLSCREEN'not in display['initial_host_window_state']and receipt['end_reason']=='completed-selection'
        displays.append(display['server_pid'])
    terminal=read(stage/'process-terminal-v1.json');assert terminal['native_identities']==len(pids)==41 and terminal['display_identities']==2
    assert {r['pid']for r in terminal['records']}==set(pids+displays)and all(r['observed_terminal']for r in terminal['records'])
    canonical=repo/'build/nasa-coverage/coverage95-20261104-matlab-assignment-row';ledger=read(canonical/'coverage-snapshot-v1.json')
    assert all(sha(repo/n)==h for n,h in ledger['authority_sha256'].items())
    for name,count in [('requirement-by-requirement-audit-v1.json',1158),('remaining-requirement-audit-v1.json',107)]:
        old=read(canonical/name);current=read(stage/name)
        assert current['rows']==old['rows']and len(current['rows'])==count and current['original_hold_keys']==old['original_hold_keys']and len(current['original_hold_keys'])==59
    prior=read(stage/'prior-frame-component-replay-v1.json')
    assert prior['requirements_verified']==1051 and prior['native_tests_rerun']==0
    return dict(status='five-system-OEM-output-verified;exact-row-admission-pending',requirements_verified=1051,original_total=1158,remaining=107,still_needed=50,
        original_hold_identities=59,new_requirement_credit=0,required_systems=SYSTEMS,new_time_cases=16,time_matrix_passes=32,committed_switch_passes=2,
        affected_UTC_control_passes=4,native_GUI_contexts=40,native_GUI_passes=38,native_GUI_failures_retained=2,native_writer_systems_passed=5,
        native_writer_serialized_records=6,pending_core_paths=286,unchanged_prior_core_paths=280,new_string_members=2,new_parameter_IDs=1,new_nonvirtual_APIs=3,new_virtual_slots=0,
        original_driver_catalog_oracle_assertion_changes=0,default_UTC_payload_unchanged_except_creation_date=True,creation_always_UTC=True,input_epoch_format_preserved=True,
        A1_forward_ICD_only=True,generic_A1_oracle_gap_retained=True,prior_NarrowAtLast_scientific_failure_retained=True,
        terminal_native_identities=41,terminal_display_identities=2,observations=observations,
        technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,goal_complete=False)


if __name__=='__main__':print(json.dumps(audit(Path.cwd(),sys.argv[1]),indent=2))
