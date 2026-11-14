"""Replay retained STM frame/reference ownership evidence; never run native tests."""
from pathlib import Path
import hashlib
import json
import math
import re
import tarfile

from RunStmFrameReference import audit as frame_audit
from RunStmReadOnlyContract import audit as readonly_audit
from StmMissionOracle import audit as mission_audit

load = lambda p: json.loads(Path(p).read_text())
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(ok, message):
    if not ok: raise ValueError(message)


def archive(stage, version):
    ids = load(stage / ('source-sha256-' + version + '.json'))
    with tarfile.open(stage / ('sources-' + version + '.tar.gz')) as bundle:
        need(set(bundle.getnames()) == set(ids), 'Source capsule inventory differs')
        for n, h in ids.items(): need(hashlib.sha256(bundle.extractfile(n).read()).hexdigest() == h, 'Source capsule bytes differ: ' + n)
    return ids


def build(stage, version, ids):
    d = load(stage / ('build-' + version + '.json'))
    need(d['status'] == 'passed' and d['returncode'] == 0 and d['inputs_unchanged'] and
         d['source_sha256'] == ids and d['log_sha256'] == sha(stage / ('build-' + version + '.log')), 'Build/source guard differs')
    return d


def mapped(repo, folder, receipt, worker):
    d = load(folder / 'run.json'); images = load(folder / 'loaded-images.json')
    need(d['native_terminal'] and d['runtime_binary_copies'] == 0, 'Native worker not terminal or runtime copied')
    name = str(repo / 'build/linux-gui/src/qtgui' / worker)
    need(images[name] == receipt['binary_sha256'][name], 'Wrong native worker executed')
    for lib in ['libGmatBase.so', 'libGmatUtil.so']:
        p = repo / 'application/bin' / lib
        need(images[str(p.resolve())] == receipt['binary_sha256'][str(p)], 'Wrong native library executed')
    return d


def audit(repo, stage):
    repo, stage = Path(repo), Path(stage)
    plan = load(stage / 'frozen-plan-v1.json')
    need(plan['matrix_tolerance'] == 1e-9 and plan['expected_parameter_reads'] == 90 and
         plan['expected_contexts'] == 18 and plan['new_requirement_credit'] == 0, 'Frozen frame obligation changed')
    versions = {v: archive(stage, v) for v in ['before-v1', 'before-v2', 'after-v1', 'after-v2',
               'after-v3', 'after-v4', 'after-v5', 'mission-v1', 'mission-v2', 'mission-v3']}
    need(len(versions['before-v2']) == 971 and len(versions['after-v5']) == 973 and
         len(versions['mission-v3']) == 978, 'Source/fixture denominator differs')
    expected = {'src/base/spacecraft/SpaceObject.cpp', 'src/base/spacecraft/Spacecraft.cpp',
                'src/base/parameter/OrbitData.cpp', 'src/base/parameter/OrbitStmParameters.cpp',
                'src/base/propagator/PropagationStateManager.cpp', 'src/base/forcemodel/ODEModel.cpp'}
    a, b = versions['before-v2'], versions['after-v5']
    need({n for n, h in a.items() if h != b[n]} == expected and set(b)-set(a) ==
         {'src/base/spacecraft/OrbitStmContext.hpp', 'src/base/parameter/OrbitStmFrame.hpp'}, 'Frozen test or unrelated production owner changed')
    fixture_name = 'src/qtgui/tests/StmFrameReferenceCases.json'
    need(len({ids[fixture_name] for ids in versions.values()}) == 1 and
         len({ids['src/qtgui/tests/StmFrameOracle.py'] for ids in versions.values()}) == 1 and
         len({ids['src/qtgui/tests/StmFrameReferenceTests.cpp'] for key, ids in versions.items() if key != 'before-v1'}) == 1,
         'Original executed matrix witness/math expectations changed')
    # Restore only mathematical inputs from the exact executed source capsule.
    # No current mutable fixture is substituted during subsequent replay.
    import tempfile
    with tempfile.TemporaryDirectory(prefix='gmat-stm-frame-math-') as temporary:
        fixture = Path(temporary) / 'frames.json'
        with tarfile.open(stage/'sources-before-v2.tar.gz') as bundle: fixture.write_bytes(bundle.extractfile(fixture_name).read())
        data = load(fixture); source = Path(data['source_matrix_archive'])
        need(sha(source) == data['source_matrix_archive_sha256'], 'Saved source-versioned input matrices changed')
        old = load(source)
        for c in data['cases']:
            if c['key'] == 'FreshIdentity': continue
            direction = 'backward' if c['final_seconds'] < c['initial_seconds'] else 'forward'
            need(c['native_stm'] == old['unattended-'+direction]['native_stm'], 'A literal saved matrix was relabelled or altered')
        cases = [('before-v1', 'before-v2', 15, 75), ('after-v2', 'after-v3', 0, 90),
                 ('after-v3', 'after-v4', 81, 9), ('after-v4', 'after-v5', 90, 0)]
        for folder, version, passed, failed in cases:
            receipt = build(stage, version, versions[version]); root = stage/folder
            run = mapped(repo, root, receipt, 'GmatStmFrameReferenceTests')
            result = frame_audit(root, fixture)
            need(result == load(root/'independent-audit.json') and result['passed'] == passed and result['failed'] == failed and
                 result['reads'] == 90 and not result['issues'], 'Retained frame comparison differs')
            need(run['returncode'] == (1 if failed else 0), 'Original native failure status changed')
        failed = load(stage/'after-v3/proof.json')['observations']
        need(all(v['provider'] == 'copy' for v in failed if not v['passed']), 'Copy-only clock failure was reclassified')
    crash = mapped(repo, stage/'after-v1', build(stage, 'after-v2', versions['after-v2']), 'GmatStmFrameReferenceTests')
    need(crash['returncode'] == -11 and not (stage/'after-v1/proof.json').exists(), 'Original native crash removed or counted as a pass')
    need('SIGSEGV' in (stage/'crash-diagnostic-v2/gdb.log').read_text() and
         'RotationProducts' in (stage/'crash-diagnostic-v2/gdb.log').read_text(), 'Native crash diagnosis lost')
    final_build = build(stage, 'after-v5', versions['after-v5'])
    readonly = mapped(repo, stage/'dependent-readonly-v1', final_build, 'GmatStmReadOnlyContractTests')
    result = readonly_audit(stage/'dependent-readonly-v1')
    need(readonly['returncode'] == 0 and result == load(stage/'dependent-readonly-v1/independent-audit.json') and
         result['generic_passed'] == 15 and result['typed_controls_passed'] == 5, 'Affected readonly contract regressed')
    gui = load(stage/'dependent-gui-receipt-v1.json'); receipt = load(stage/'dependent-gui-v1/results.json')
    rows = [r for r in receipt['results'] if r['status'] != 'not-run']
    need(gui['returncode'] == 0 and gui['inputs_unchanged'] and not gui['full_regression'] and receipt['mode'] == 'headless' and
         {r['name'] for r in rows} == {'QtGui.NasaStmParameterforward', 'QtGui.NasaStmParameterbackward'}, 'Wrong/repeated GUI selection')
    with tarfile.open(stage/'dependent-gui-owned-sources-v1.tar.gz') as bundle:
        need(set(bundle.getnames()) == set(gui['source_sha256']), 'Executed GUI source partition differs')
        for n, h in gui['source_sha256'].items(): need(hashlib.sha256(bundle.extractfile(n).read()).hexdigest() == h, 'GUI source bytes differ')
    for n, h in versions['after-v5'].items(): need(gui['source_sha256'][n] == h, 'GUI kernel source differs')
    for row in rows:
        root = stage/'dependent-gui-v1'/row['name']; proof = load(root/'proof.json')
        need(row['status'] == 'passed' and row['returncode'] == 0 and proof['outcome'] == 'passed' and
             proof['direction'] in ['forward', 'backward'] and len(proof['full_stm']) == 6 and
             proof['max_analytic_error'] < 2e-6 and proof['max_finite_error'] < 2e-6 and proof['symplectic_error'] < 2e-8,
             'Original complete STM/trajectory/GUI assertion failed')
        log = Path(row['log']).read_text()
        need(proof in [json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})', log)], 'GUI proof differs from native stdout')
    mission_summaries = []
    for folder, version, name in [('reference-mission-v1', 'mission-v1', 'StmFrameMissionCases.json'),
                                  ('reference-mission-split-v2', 'mission-v3', 'StmFrameMissionSplitCases.json')]:
        receipt = build(stage, version, versions[version]); root = stage/folder
        run = mapped(repo, root, receipt, 'GmatStmFrameMissionTests')
        with tempfile.TemporaryDirectory(prefix='gmat-stm-observer-math-') as temporary:
            fixture = Path(temporary)/name
            with tarfile.open(stage/('sources-'+version+'.tar.gz')) as bundle: fixture.write_bytes(bundle.extractfile('src/qtgui/tests/'+name).read())
            result = mission_audit(root, fixture)
        need(run['returncode'] == 0 and result == load(root/'independent-audit.json') and result['matrix_values_checked'] == 72 and
             result['rejections_checked'] == 2, 'Actual observer mission failed independent reconstruction')
        mission_summaries.append(result['max_selected_stm_error'])
        for lib in ['libGmatBase.so', 'libGmatUtil.so']:
            need(receipt['binary_sha256'][str(repo/'application/bin'/lib)] == final_build['binary_sha256'][str(repo/'application/bin'/lib)],
                 'Passing older cases were rebound to changed production images')
    failed_split = stage/'reference-mission-split-v1'
    run = mapped(repo, failed_split, build(stage, 'mission-v2', versions['mission-v2']), 'GmatStmFrameMissionTests')
    need(run['returncode'] == 0 and load(failed_split/'independent-audit.json')['status'] == 'failed' and
         abs(load(failed_split/'proof.json')['elapsed_seconds']-80) < 1e-6, 'Incorrect split-script failure removed/relabelled')
    need(re.findall(r'ElapsedSecs = ([0-9.]+),', (stage/'reference-mission-split-v2/mission.script').read_text()) == ['20', '40'], 'Frozen60-second split endpoint not realized')
    protected = load(stage/'protected-input-check-v1.json')
    need(protected['unchanged'] == 1334 and not protected['changed'], 'Protected original inputs changed')
    guard = load(stage/'source-after-execution-v1.json')
    need(guard['inputs_unchanged'] and guard['source_sha256'] == versions['mission-v3'], 'Final source guard differs')
    prefix = stage.parent/'complete18-20261111-oem-file-invariants-v1/sealed-current-prefix-v1.json'
    need(sha(prefix) == 'de77757b896aa68c65df2462c026b58bf11ed684b502148fa5a14c0593111574', 'Previously admitted prefix changed')
    return dict(status='passed', native_frame_contexts=18, native_parameter_reads=90,
                native_frame_values_checked=1296, dependent_readonly_checks=20,
                original_GUI_cases=2, observer_missions=2, observer_matrix_values_checked=144,
                observer_rejections_checked=4, original_baseline_failures_retained=75,
                original_native_crash_retained=True, detached_frame_binding_failures_retained=90,
                precise_assignment_failures_retained=9, split_fixture_failure_retained=True,
                original_inputs_unchanged=1334, production_existing_owners_changed=6,
                private_helpers_added=2, max_observer_matrix_entry_error=max(mission_summaries),
                native_tests_run_by_replay=0, source_versioned_only=True, new_requirement_credit=0,
                requirements_verified=1106, requirements_total=1158, agreed_complete=5,
                agreed_total=18, whole_row_qualified=False, full_current_build_qualified=False,
                full_regression=False)
