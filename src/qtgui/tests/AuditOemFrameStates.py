"""Replay declared OEM six-state rotation/origin proofs;do not admit a partial row."""
from collections import Counter
import hashlib,json,re,sys,tarfile
from pathlib import Path
from RunOemFrameStates import audit as frame_audit
from OemFrameStateOracle import catalog


def audit(repo,stage):
    repo,stage=Path(repo),Path(stage)
    read=lambda p:json.loads(Path(p).read_text())
    sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
    ids=read(stage/'executed-source-sha256-v1.json')
    retained=['NasaEphemerisSamplingTests.cpp','NasaEphemerisSamplingCases.json','NasaOemRecordSupportTests.cpp',
              'NasaOemRecordSupportCases.json','NasaOemShortWindowCases.json']
    with tarfile.open(stage/'executed-sources-v1.tar.gz')as tar:
        assert set(tar.getnames())==set(ids)
        assert all(hashlib.sha256(tar.extractfile(n).read()).hexdigest()==h for n,h in ids.items())
        fixture=json.load(tar.extractfile('src/qtgui/tests/NasaOemFrameStateCases.json'))
        source=tar.extractfile('src/qtgui/tests/NasaOemFrameStateTests.cpp').read().decode()
        old=tar.extractfile('src/qtgui/tests/NasaOemRecordSupportTests.cpp').read().decode()
    assert fixture==catalog()and len(fixture['cases'])==9
    assert {(c['frame_kind'],c['plane'])for c in fixture['cases']}=={(f,p)for f in ['EarthEquator','EarthEcliptic','RelativeEquator']for p in ['XY','YZ','ZX']}
    assert all(c['position_tolerance']==2e-5 and c['velocity_tolerance']==2e-8 and c['expected_degrees']==[4]and len(c['expected_segments'][0])==5 for c in fixture['cases'])
    for n in retained:assert ids['src/qtgui/tests/'+n]==sha(stage/'preimages-v1/src/qtgui/tests'/n)
    old_assertions=Counter(re.findall(r'\brequire\(.*?\);',old,re.S));new_assertions=Counter(re.findall(r'\brequire\(.*?\);',source,re.S))
    assert sum(old_assertions.values())==56 and sum(new_assertions.values())==58 and sum((old_assertions-new_assertions).values())==2
    assert all('meta["TIME_SYSTEM"]'in s or'last[i].toDouble()-final[i].toDouble()'in s for s in old_assertions-new_assertions)
    assert 'last[i].toDouble()-frameFinal[i].toDouble())<(i<4 ? 2e-10 : 2e-13)'in source
    assert 'meta["CENTER_NAME"].toString().toUpper()==spec["expected_center"].toString()'in source
    assert 'meta["REF_FRAME"]==spec["expected_frame"].toString()'in source
    contract=read(stage/'assertion-contract-v1.json');assert contract['original_assertion_changes']==0 and contract['original_drivers_and_catalogs_unchanged']
    approved=read(repo/'build/nasa-coverage/coverage95-20261104-oem-creation-utc/assertion-exception-approval-v1.json')
    assert ids['src/qtgui/tests/NasaEphemerisSamplingCases.json']==approved['after_sha256']
    build=stage/'build-receipt-v1.json';record=read(build)
    assert record['status']=='passed'and record['returncode']==0 and record['inputs_unchanged']and record['source_sha256']==ids
    prior_build=read(repo/'build/nasa-coverage/coverage95-20261104-oem-b1-text/build-receipt-v5.json')
    common=set(record['binary_sha256'])&set(prior_build['binary_sha256'])
    assert len(common)==4 and all(record['binary_sha256'][n]==prior_build['binary_sha256'][n]for n in common)
    core=read(stage/'core-before-v1.json')['sha256'];assert len(core)==285 and all(ids.get(n)==h for n,h in core.items())
    prior=read(repo/'build/nasa-coverage/coverage95-20261104-oem-b1-text/core-change-inventory-v1/inventory.json')
    assert core=={r['path']:r['working_sha256']for r in prior['files']}
    pids=[];observations=[];maximum_position=maximum_velocity=0.
    for mode in ['headless','visible']:
        name='frames-'+mode+'-v1';root=stage/name;maps=read(stage/(name+'-loaded-images.json'))
        assert maps['returncode']==0 and maps['inputs_unchanged']and maps['source_sha256']==ids and maps['runtime_binary_copies']==0
        assert maps['build_receipt_sha256']==sha(build)and maps['binary_sha256']==record['binary_sha256']and len(maps['processes'])==9
        for process in maps['processes']:
            assert any(s['project_images'].get(process['executable'])==maps['binary_sha256'][process['executable']]for s in process['snapshots'])
            pids.append(process['pid'])
        receipt=read(root/'results.json');selected=[r for r in receipt['results']if r['status']!='not-run']
        expected={'QtGui.NasaOemFrameState'+c['key']for c in fixture['cases']}
        assert {r['name']for r in selected}==expected and len(selected)==9 and all(r['status']=='passed'and r['returncode']==0 for r in selected)
        independent=frame_audit(root,fixture)
        assert independent==read(root/'independent-frame-state-audit.json')and independent['status']=='passed'and len(independent['observations'])==9
        assert all(x['records']==5 for x in independent['observations'])
        for row in independent['observations']:
            proof=read(root/row['test']/'proof.json')
            assert proof['outcome']=='passed'and proof['points']==5 and proof['segments']==1 and proof['numeric_sentinel']==9
            assert proof['max_position_error']==row['max_position_error_km']and proof['max_velocity_error']==row['max_velocity_error_km_s']
            maximum_position=max(maximum_position,row['max_position_error_km']);maximum_velocity=max(maximum_velocity,row['max_velocity_error_km_s'])
        observations.append(dict(mode=mode,cases=9,retained_records=45,status='passed'))
    visible=read(stage/'frames-visible-v1/results.json');display=visible['visible_display']
    assert visible['resizable']and display['resizable']and display['start_minimized']and not display['resize_errors']
    assert 'Iconic'in display['initial_host_window_state']and 'FULLSCREEN'not in display['initial_host_window_state']and visible['end_reason']=='completed-selection'
    terminal=read(stage/'process-terminal-v1.json')
    assert terminal['native_identities']==len(pids)==18 and terminal['display_identities']==1
    assert {r['pid']for r in terminal['records']}==set(pids+[display['server_pid']])and all(r['observed_terminal']for r in terminal['records'])
    canonical=repo/'build/nasa-coverage/coverage95-20261104-matlab-assignment-row'
    ledger=read(canonical/'coverage-snapshot-v1.json')
    assert all(sha(repo/n)==h for n,h in ledger['authority_sha256'].items())
    for name,count in [('requirement-by-requirement-audit-v1.json',1158),('remaining-requirement-audit-v1.json',107)]:
        original=read(canonical/name);current=read(stage/name)
        assert current['rows']==original['rows']and len(current['rows'])==count and current['original_hold_keys']==original['original_hold_keys']and len(current['original_hold_keys'])==59
    prior_replay=read(stage/'prior-text-component-replay-v1.json');assert prior_replay['requirements_verified']==1051 and prior_replay['native_tests_rerun']==0
    return dict(status='declared-OEM-frame-state-proof-replayed;whole-row-open',requirements_verified=1051,original_total=1158,
        remaining=107,still_needed=50,original_hold_identities=59,new_requirement_credit=0,native_GUI_contexts=18,
        unique_cases=9,frame_kinds=3,spanning_planes=3,unique_exported_states=45,retained_exported_states=90,
        max_position_error_km=maximum_position,max_velocity_error_km_s=maximum_velocity,
        core_paths_unchanged=285,production_changes=0,new_native_interface_members=0,
        original_driver_and_catalog_changes=0,new_fixture_adapted_assertions=2,new_fixture_added_assertions=2,
        prior_NarrowAtLast_scientific_failure_retained=True,E404_conflict_unresolved=True,E381_scope_unresolved=True,
        observations=observations,terminal_native_identities=18,terminal_display_identities=1,
        technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,goal_complete=False)


if __name__=='__main__':print(json.dumps(audit(Path.cwd(),sys.argv[1]),indent=2))
