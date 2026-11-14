"""Replay frozen STM solver/restoration/association evidence without native runs."""
from pathlib import Path
import hashlib
import json
import tarfile
import tempfile

from StmSolverFrameOracle import audit as solver_audit
from StmFunctionRecoveryOracle import audit as recovery_audit
from StmFunctionFrameOracle import compare_matrix
from StmMissionOracle import circle, circular_stm
from StmFrameOracle import multiply

load = lambda p: json.loads(Path(p).read_text())
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(ok, message):
    if not ok:
        raise ValueError(message)


def capsule(stage, version):
    ids = load(stage/('source-sha256-'+version+'.json'))
    with tarfile.open(stage/('sources-'+version+'.tar.gz')) as bundle:
        need(set(bundle.getnames()) == set(ids), 'Source capsule inventory differs')
        for n, h in ids.items():
            need(hashlib.sha256(bundle.extractfile(n).read()).hexdigest() == h, 'Source capsule bytes differ: '+n)
    build = load(stage/('build-'+version+'.json'))
    failed = version == 'recovery-v1'
    need(build['source_sha256'] == ids and build['inputs_unchanged'] and
         build['status'] == ('failed' if failed else 'passed') and build['returncode'] == (1 if failed else 0) and
         build['log_sha256'] == sha(stage/('build-'+version+'.log')), 'Build/source/compile-failure guard differs')
    return ids, build


def member(stage, version, name):
    with tarfile.open(stage/('sources-'+version+'.tar.gz')) as bundle:
        return bundle.extractfile(name).read()


def mapped(root, build, worker):
    run, images = load(root/'run.json'), load(root/'loaded-images.json')
    need(run['returncode'] == 0 and run['native_terminal'] and run['runtime_binary_copies'] == 0, 'Native process not terminal/pass')
    for n, h in build['binary_sha256'].items():
        logical = Path(n)
        if logical.name != worker and not logical.name.endswith('.so'):
            continue
        need(any(Path(p).parent == logical.parent and
                 (Path(p).name == logical.name or Path(p).name.startswith(logical.name+'.')) and digest == h
                 for p, digest in images.items()), 'Wrong mapped native worker/library/plugin: '+n)
    return run


def association(root, correct):
    for tag in ['failure', 'recovery']:
        d = load(root/(tag+'-initial-diagnostic.json'))
        rows = [v for v in d['state_map'] if v['subelement'] == 1]
        cart = next(v for v in rows if v['object'] == 'guide' and v['element'] == 'CartesianState')
        stm = next(v for v in rows if v['object'] == 'guide' and v['element'] == 'STM')
        need(cart['index'] == 6 and stm['associate_index'] == (6 if correct else 0), 'Demonstrated guide association differs')
        need(stm['associate'] == ('Ref' if correct else 'guide') and stm['full_name'] == 'Ref' and
             d['craft_full_name'] == 'Sat' and d['guide_full_name'] == 'Ref', 'Function alias/full-name evidence differs')
        actual = d['guide']['stm']
        compare_matrix(actual, circular_stm(398600.4418, 8000., d['guide']['seconds']), [2e-8, 2e-6, 2e-11, 2e-8])


def wrong_model(root):
    initial = load(root/'failure-initial-diagnostic.json')['guide']['stm']
    actual = load(root/'proof.json')['phases'][0]['local']['guide']
    mu, h = 398600.4418, .01
    def derivative(t, phi):
        state = circle(mu, 7000., t); a = [[0.]*6 for _ in range(6)]
        for i in range(3):
            a[i][i+3] = 1.
            for j in range(3):
                a[i+3][j] = mu*(3*state[i]*state[j]/7000.**5-float(i == j)/7000.**3)
        return multiply(a, phi)
    def plus(a, b, scale):
        return [[x+scale*y for x, y in zip(ar, br)] for ar, br in zip(a, b)]
    value = initial
    for step in range(3000):
        t = 60.+step*h
        k1 = derivative(t, value); k2 = derivative(t+h/2, plus(value, k1, h/2))
        k3 = derivative(t+h/2, plus(value, k2, h/2)); k4 = derivative(t+h, plus(value, k3, h))
        value = [[value[i][j]+h*(k1[i][j]+2*k2[i][j]+2*k3[i][j]+k4[i][j])/6 for j in range(6)] for i in range(6)]
    correct = circular_stm(mu, 8000., actual['seconds'])
    residual = max(abs(actual['stm'][i][j]-value[i][j]) for i in range(6) for j in range(6))
    correct_error = max(abs(actual['stm'][i][j]-correct[i][j]) for i in range(6) for j in range(6))
    need(residual < 1e-9 and correct_error > 1e-3, 'Independent wrong-first-craft derivative diagnosis differs')
    return residual, correct_error


def audit(repo, stage):
    repo, stage = Path(repo), Path(stage)
    plan = load(stage/'frozen-plan-v1.json')
    need(plan['cases'] == ['Target', 'Optimize'] and plan['minimum_trials_each'] == 3 and
         plan['all_executed_trials_required'] and plan['matrix_block_tolerances'] == [2e-8, 2e-6, 2e-11, 2e-8] and
         not plan['full_regression'] and plan['new_requirement_credit'] == 0, 'Frozen solver obligation changed')
    versions = {v: capsule(stage, v) for v in ['before-v1', 'recovery-v1', 'recovery-v2', 'recovery-v3', 'after-v1', 'metadata-v1']}
    before, after, metadata = [versions[v][0] for v in ['recovery-v3', 'after-v1', 'metadata-v1']]
    need(before.keys() == after.keys() and {n for n, h in before.items() if after[n] != h} ==
         {'src/base/propagator/PropagationStateManager.cpp'}, 'Production repair changed unrelated sources or frozen tests')
    need({n for n, h in after.items() if metadata[n] != h} ==
         {'src/qtgui/tests/StmFunctionRecoveryTests.cpp', 'src/qtgui/tests/StmFunctionRecoveryOracle.py'}, 'Metadata cleanup changed unrelated inputs')
    worker = 'src/qtgui/tests/StmFunctionRecoveryTests.cpp'
    need(member(stage, 'after-v1', worker).replace(b'{"production_changes",0}', b'{"native_worker_source_writes",0}') ==
         member(stage, 'metadata-v1', worker), 'Metadata cleanup changed native assertions or execution')
    fix = load(stage/'association-repair-frozen-plan-v1.json')
    need(before['src/base/propagator/PropagationStateManager.cpp'] == fix['preimage_sha256'] and
         fix['assertion_or_tolerance_changes'] == 0 and fix['public_API_changes'] == 0, 'Repair preimage/scope differs')
    need('return-statement with no value' in (stage/'build-recovery-v1.log').read_text(), 'Original supplementary compile failure lost')
    previous = Path(plan['previous_engineering_receipt'])
    need(sha(previous) == plan['previous_engineering_receipt_sha256'] ==
         '654a3b61691fa1d8825b79746e03200df8d45374e3dd3cf92ca0f4d0603adf35', 'Previous engineering seal changed')
    prior = load(previous.parent/'source-sha256-source-link-v1.json')
    need({n for n, h in prior.items() if versions['before-v1'][0][n] != h} == {'src/qtgui/CMakeLists.txt'},
         'Production source changed before the demonstrated defect')
    fixture_name = 'src/qtgui/tests/StmFunctionRecoveryCase.json'
    need(len({versions[v][0][fixture_name] for v in ['recovery-v1', 'recovery-v2', 'recovery-v3', 'after-v1', 'metadata-v1']}) == 1,
         'Frozen recovery fixture or bounds changed')
    solver_results = []
    with tempfile.TemporaryDirectory(prefix='gmat-stm-loop-replay-') as temporary:
        for key, count in [('Target', 10), ('Optimize', 13)]:
            name = 'src/qtgui/tests/StmSolver'+key+'Case.json'; fixture = Path(temporary)/(key+'.json')
            fixture.write_bytes(member(stage, 'before-v1', name))
            for suffix, version in [('before-v1', 'before-v1'), ('after-v1', 'after-v1')]:
                root = stage/(key.lower()+'-'+suffix)
                run = mapped(root, versions[version][1], 'GmatStmSolverFrameTests')
                need(run['fixture_sha256'] == sha(fixture), 'Executed solver fixture identity differs')
                result = solver_audit(root, fixture)
                need(result == load(root/'independent-audit.json') and result['solver_trials'] == count and
                     load(root/'proof.json')['samples'] == load(root/'trial-samples.json') and
                     not (root/'observer-error.txt').read_text(), 'Native loop replay or paired samples differ')
                if suffix == 'after-v1':
                    solver_results.append(result)
        fixture = Path(temporary)/'recovery.json'; fixture.write_bytes(member(stage, 'recovery-v3', fixture_name))
        recovery_plan = load(stage/'function-recovery-frozen-plan-v1.json')
        need(sha(fixture) == recovery_plan['fixture_sha256'] and recovery_plan['expected_native_statuses'] == [-5, 1],
             'Frozen intentional error/recovery status changed')
        for folder, version in [('function-recovery-before-v1', 'recovery-v2'), ('function-recovery-before-v2', 'recovery-v3')]:
            root = stage/folder; run = mapped(root, versions[version][1], 'GmatStmFunctionRecoveryTests')
            need(run['fixture_sha256'] == sha(fixture), 'Executed failed recovery fixture differs')
            held = load(root/'independent-audit.json')
            need(held['status'] == 'failed' and held['reason'] == 'Full matrix differs from circular/two-end oracle', 'Original independent failure reclassified')
            try:
                recovery_audit(root, fixture)
            except ValueError as error:
                need(str(error) == held['reason'], 'Retained numerical failure reason differs')
            else:
                raise ValueError('Original guide STM failure was waived')
            need([v['run_status'] for v in load(root/'proof.json')['phases']] == [-5, 1], 'Original raw native error/success statuses changed')
        association(stage/'function-recovery-before-v2', False)
        residual, wrong_error = wrong_model(stage/'function-recovery-before-v2')
        diagnosis = load(stage/'guide-association-diagnosis-v1.json')
        need(residual == diagnosis['wrong_first_craft_variational_model_max_residual'] and
             wrong_error == diagnosis['correct_guide_variational_model_max_error'], 'Independent derivative diagnosis replay differs')
        root = stage/'function-recovery-after-v1'; run = mapped(root, versions['after-v1'][1], 'GmatStmFunctionRecoveryTests')
        need(run['fixture_sha256'] == sha(fixture), 'Passing recovery fixture identity differs')
        recovery = recovery_audit(root, fixture)
        need(recovery == load(root/'independent-audit.json') and load(root/'proof.json')['phases'] == load(root/'phases.json'),
             'Independent rejection/unwind/recovery evidence differs')
        association(root, True)
    protected = load(stage/'protected-input-check-v1.json')
    need(protected['status'] == 'passed' and protected['original_inputs_unchanged'] == 1334 and not protected['changed'] and
         sha(protected['manifest']) == protected['manifest_sha256'], 'Protected original inputs changed')
    correction = load(stage/'producer-metadata-correction-v1.json')
    need(correction['actual_production_source_files_changed'] == 1 and correction['native_tests_rerun'] == 0 and
         not correction['numerical_expectations_or_assertions_changed'], 'Producer metadata scope correction lost')
    for n, h in versions['after-v1'][1]['binary_sha256'].items():
        if n.endswith('.so'):
            need(versions['metadata-v1'][1]['binary_sha256'][n] == h, 'Production library changed during metadata-only cleanup')
    return dict(status='passed', production_source_files_changed=1, native_missions_after_repair=4,
                solver_trials=sum(v['solver_trials'] for v in solver_results),
                native_parameter_reads=sum(v['native_parameter_reads'] for v in solver_results)+recovery['native_parameter_reads'],
                reported_matrix_entries=sum(v['reported_matrix_entries'] for v in solver_results)+recovery['reported_matrix_entries'],
                guide_STM_association_corrected=True, nested_failure_unwind_exact=True, native_rejections=5,
                native_error_status_retained=-5, recovery_passed=True, original_independent_failures_retained=2,
                original_compile_failure_retained=True, original_inputs_unchanged=1334,
                wrong_first_craft_model_residual=residual, old_guide_STM_max_error=wrong_error,
                native_tests_run_by_replay=0, new_requirement_credit=0, requirements_verified=1106, requirements_total=1158,
                agreed_complete=5, agreed_total=18, whole_row_qualified=False, full_current_build_qualified=False,
                current_metadata_worker_native_reexecuted=False, full_regression=False)
