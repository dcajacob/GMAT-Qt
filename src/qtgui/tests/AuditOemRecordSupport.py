"""Replay the bounded OEM repair evidence, retaining failures and zero row credit."""
import hashlib
import json
from pathlib import Path
import sys
import tarfile

from CcsdsOemB1Oracle import ConformanceError, validate
from RunOemWriterRecordSupport import audit as writer_audit


def audit(repo, stage):
    repo, stage = Path(repo), Path(stage)
    sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
    read = lambda p: json.loads(Path(p).read_text())
    versions = {}
    for version in range(1, 6):
        identities = read(stage/f'executed-source-sha256-v{version}.json')
        with tarfile.open(stage/f'executed-sources-v{version}.tar.gz') as archive:
            members = {m.name:m for m in archive.getmembers()}
            assert set(members) == set(identities)
            assert all(hashlib.sha256(archive.extractfile(members[n]).read()).hexdigest() == h
                       for n, h in identities.items())
        versions[version] = identities
    for version in range(2, 6):
        build = read(stage/f'build-receipt-v{version}.json')
        assert build['status'] == 'passed' and build['returncode'] == 0
        assert build['inputs_unchanged'] and build['source_sha256'] == versions[version]
    original = ['src/qtgui/tests/NasaEphemerisSamplingTests.cpp',
                'src/qtgui/tests/NasaEphemerisSamplingCases.json']
    for name in original:
        before = sha(stage/'preimages-v1'/name)
        assert all(v[name] == before for v in versions.values())
    with tarfile.open(stage/'executed-sources-v5.tar.gz') as archive:
        catalog = json.load(archive.extractfile('src/qtgui/tests/NasaOemRecordSupportCases.json'))
    cases = {c['key']:c for c in catalog['cases']}
    runs = [
        ('before-fix-short-sparse-headless-v2', 2, 0, 2),
        ('after-fix-new-and-support-headless-v3', 3, 5, 1),
        ('affected-original-headless-v5', 5, 2, 1),
        ('new-and-affected-visible-v5', 5, 7, 2),
    ]
    observations, processes = [], []
    for name, version, passed, failed in runs:
        root = stage/name
        maps = read(stage/(name+'-loaded-images.json'))
        assert maps['inputs_unchanged'] and maps['source_sha256'] == versions[version]
        assert maps['returncode'] == 1 and maps['runtime_binary_copies'] == 0
        receipt = stage/f'build-receipt-v{version}.json'
        assert maps['build_receipt_sha256'] == sha(receipt)
        assert maps['binary_sha256'] == read(receipt)['binary_sha256']
        results = [r for r in read(root/'results.json')['results'] if r['status'] != 'not-run']
        assert sum(r['status']=='passed' for r in results) == passed
        assert sum(r['status']=='failed' for r in results) == failed
        assert len(results) == passed+failed == len(maps['processes'])
        for process in maps['processes']:
            assert process['snapshots']
            assert any(s['project_images'].get(process['executable']) ==
                       maps['binary_sha256'][process['executable']] for s in process['snapshots'])
            processes.append(process['pid'])
        for result in results:
            prefix = 'QtGui.NasaOemRecordSupport'
            if not result['name'].startswith(prefix):
                continue
            key = result['name'][len(prefix):]
            path = root/result['name']/'mission.oem'
            try:
                finding = validate(path.read_bytes(), icd_object_ids=[catalog['object_id']])
            except ConformanceError as error:
                finding = dict(status='failed',clause=error.clause,line=error.line,message=error.message)
            if result['status'] == 'passed':
                assert finding['status'] == 'passed'
                assert [s['records'] for s in finding['segments']] == [len(s) for s in cases[key]['expected_segments']]
                assert [int(s['metadata']['INTERPOLATION_DEGREE']) for s in finding['segments']] == cases[key]['expected_degrees']
                proof = read(root/result['name']/'proof.json')
                assert proof['outcome'] == 'passed' and proof['numeric_sentinel'] == 9
                assert proof['max_position_error'] < cases[key]['position_tolerance']
                assert proof['max_velocity_error'] < cases[key]['velocity_tolerance']
            else:
                assert finding['status'] == 'failed'
            observations.append(dict(run=name,case=key,native=result['status'],B1=finding['status']))
    visible = read(stage/'new-and-affected-visible-v5/results.json')
    display = visible['visible_display']
    assert visible['resizable'] and display['resizable'] and display['start_minimized']
    assert 'Iconic' in display['initial_host_window_state'] and not display['resize_errors']
    assert visible['end_reason'] == 'completed-selection'
    support = read(stage/'writer-support-headless-v5-loaded-images.json')
    assert support['returncode'] == 0 and support['inputs_unchanged']
    assert support['source_sha256'] == versions[5] and len(support['processes']) == 1
    assert support['runtime_binary_copies'] == 0
    assert support['build_receipt_sha256'] == sha(stage/'build-receipt-v5.json')
    for process in support['processes']:
        assert any(s['project_images'].get(process['executable']) ==
                   support['binary_sha256'][process['executable']] for s in process['snapshots'])
    processes.extend(p['pid'] for p in support['processes'])
    writer = writer_audit(stage/'writer-support-headless-v5')
    assert writer['status'] == 'passed' and writer['exact_states_and_epochs'] == 1007
    assert read(stage/'writer-support-headless-v4-loaded-images.json')['returncode'] != 0
    legacy = validate((stage/'affected-original-headless-v5/QtGui.NasaEphemerisSamplingShortArc/mission.oem').read_bytes())
    assert legacy['status'] == 'incomplete' and legacy['segments'][0]['records'] == 2
    assert legacy['segments'][0]['metadata']['INTERPOLATION_DEGREE'] == '1'
    before = read(stage/'core-before-v1.json')
    core = read(stage/'core-change-inventory-v1/inventory.json')
    actual = {i['path']:i['working_sha256'] for i in core['files']}
    assert len(actual) == 283 and all(actual.get(n)==h for n,h in before['sha256'].items())
    assert all(versions[5].get(n)==h for n,h in actual.items())
    assert len(set(actual)-set(before['sha256'])) == 3
    remaining = read(stage/'remaining-requirement-audit-v1.json')
    assert remaining['remaining'] == 107 and remaining['still_needed'] == 50
    assert len(remaining['original_hold_keys']) == 59
    terminal = read(stage/'process-terminal-v1.json')
    assert terminal['native_identities'] == len(processes) == 21
    assert terminal['display_identities'] == 1
    assert {x['pid'] for x in terminal['records']} == set(processes+[display['server_pid']])
    assert all(x['observed_terminal'] for x in terminal['records'])
    return dict(status='bounded-repair-evidence-replayed;whole-row-open',
                canonical_verified=1051,original_total=1158,target=1101,still_needed=50,
                remaining_original_requirements=107,original_hold_identities=59,
                native_GUI_contexts=20,native_GUI_passes=14,native_GUI_failures_retained=6,
                writer_support_records=1007,new_GUI_cases_passed_both_profiles=5,
                new_equal_window_failure_retained_both_profiles=True,
                unchanged_original_ShortArc_conflict_retained_both_profiles=True,
                original_driver_and_catalog_unchanged=True,prior_280_core_paths_unchanged=True,
                current_pending_core_paths=283,observations=observations,
                externally_mapped_native_pids=processes,visible_server_pid=display['server_pid'],
                new_requirement_credit=0,whole_row_qualified=False,technical_core_approval=False,
                full_current_build_qualified=False,full_NASA_qualified=False,goal_complete=False)


if __name__ == '__main__':
    print(json.dumps(audit(Path.cwd(), sys.argv[1]),indent=2))
