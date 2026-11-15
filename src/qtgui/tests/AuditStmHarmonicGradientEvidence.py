"""Replay the exact-force gradient repair and its bounded native witnesses."""
import importlib.util
import json
from pathlib import Path
import tempfile

from AuditStmOwnerFrameEvidence import capsule, member, mapped, load, sha, need
from StmOwnerFrameOracle import audit as pointmass_audit
from StmHarmonicFrameOracle import audit as monopole_audit
from StmHarmonicJ2Oracle import audit as j2_audit


def audit(repo, stage):
    repo, stage = Path(repo), Path(stage)
    versions = {v: capsule(stage, v) for v in
                ('before-v1', 'diagnostic-v1', 'baseline-v1', 'after-v1', 'nonzero-v1',
                 'limited-before-v1', 'limited-native-before-v1', 'limited-native-after-v1', 'registration-v1')}
    before, after, nonzero = [versions[v][0] for v in ('baseline-v1', 'after-v1', 'nonzero-v1')]
    plan = load(stage/'frozen-plan-v1.json')
    fix = load(stage/'implementation-plan-v1.json')
    previous = Path(plan['previous_engineering_receipt'])
    need(sha(previous) == plan['previous_engineering_receipt_sha256'] ==
         'd03c8f0e0375910ec4abf4807d734e8117cffa829af2d65627031b0f4ece7fc4',
         'Previous engineering receipt differs')
    prior = load(previous.parent/'reviewed-source-sha256-v1.json')
    need({n for n,h in prior.items() if versions['before-v1'][0].get(n) != h} == {'src/qtgui/CMakeLists.txt'},
         'Production source changed before the new provider witness')
    changed = {'src/base/forcemodel/ODEModel.cpp', 'src/base/forcemodel/GravityField.cpp'}
    need({n for n,h in before.items() if after.get(n) != h} == changed and
         set(after)-set(before) == {'src/base/forcemodel/OrbitStmForceGradient.hpp'} and
         all(before[n] == h for n,h in fix['production_preimages'].items()) and
         fix['public_API_changes'] == fix['native_layout_changes'] == fix['expectation_or_tolerance_changes'] == 0,
         'Force-gradient repair/preimage scope differs')
    need({n for n,h in after.items() if nonzero.get(n) != h} ==
         {'src/qtgui/CMakeLists.txt','src/qtgui/tests/StmHarmonicFrameTests.cpp'} and
         set(nonzero)-set(after) == {'src/qtgui/tests/'+n for n in
             ('RunStmHarmonicJ2.py','StmHarmonicJ2Cases.json','StmHarmonicJ2Oracle.py')},
         'Nonzero fixture extension changed production or prior numerical readers')
    for name,h in versions['after-v1'][1]['binary_sha256'].items():
        if name.endswith('.so') or Path(name).name == 'GmatStmOwnerFrameTests':
            need(versions['nonzero-v1'][1]['binary_sha256'][name] == h,
                 'Production/point-mass image changed during test-only extension')
    limited_before, limited_after, registration = [versions[v][0] for v in
        ('limited-native-before-v1','limited-native-after-v1','registration-v1')]
    first_limited = versions['limited-before-v1'][0]
    need({n for n,h in nonzero.items() if first_limited.get(n) != h} ==
         {'src/qtgui/tests/StmHarmonicJ2Oracle.py'} and set(first_limited)-set(nonzero) ==
         {'src/qtgui/tests/StmHarmonicJ2LimitedCases.json'}, 'Limited-case draft changed unrelated inputs')
    need({n for n,h in first_limited.items() if limited_before.get(n) != h} ==
         {'src/qtgui/tests/StmHarmonicJ2Oracle.py'} and set(limited_before)-set(first_limited) ==
         {'src/qtgui/tests/StmHarmonicJ2LimitedNativeCases.json'}, 'Oracle correction changed production or prior fixtures')
    limit_fix = load(stage/'full-frame-gradient-frozen-plan-v1.json')
    need({n for n,h in limited_before.items() if limited_after.get(n) != h} ==
         {'src/base/forcemodel/GravityField.cpp'} and set(limited_before) == set(limited_after) and
         limited_before['src/base/forcemodel/GravityField.cpp'] == limit_fix['preimage_sha256'] and
         not limit_fix['native_StmLimit_mutation'] and not limit_fix['numerical_fixture_or_bounds_changed'],
         'Full frame-gradient truncation repair scope differs')
    need({n for n,h in limited_after.items() if registration.get(n) != h} == {'src/qtgui/CMakeLists.txt'} and
         set(limited_after) == set(registration), 'Registration changed runtime sources')
    need(versions['limited-native-after-v1'][1]['binary_sha256'] == versions['registration-v1'][1]['binary_sha256'],
         'Native image changed during registration-only build')
    correction = load(stage/'generator-path-correction-v1.json')
    need(correction['numerical_fixture_unchanged'] and not correction['expectations_or_tolerances_changed'] and
         correction['production_changes'] == 0, 'Original generator failure correction scope differs')
    for version in ('before-v1','diagnostic-v1'):
        root = stage/version
        run = load(root/'run.json'); images = load(root/'loaded-images.json')
        need(run['returncode'] == 1 and run['native_terminal'] and run['runtime_binary_copies'] == 0 and
             not (root/'proof.json').exists(), 'Original pre-force-path failure lost or reclassified')
        for name,h in versions[version][1]['binary_sha256'].items():
            if name.endswith('.so') or Path(name).name == 'GmatStmHarmonicFrameTests':
                logical = Path(name)
                need(any(Path(p).parent == logical.parent and digest == h and
                     (Path(p).name == logical.name or Path(p).name.startswith(logical.name+'.'))
                     for p,digest in images.items()), 'Original failed native image differs')
    need('contains characters outside of the ASCII character set' in
         (stage/'diagnostic-v1/build-messages.txt').read_text(), 'Native generator failure diagnostic lost')
    need(plan['expected_reads'] == 60 and plan['expected_baseline_defined_failures'] == 45 and
         plan['expected_singular_rejections'] == 15 and plan['new_requirement_credit'] == 0,
         'Frozen harmonic-provider obligation differs')
    with tempfile.TemporaryDirectory(prefix='gmat-harmonic-gradient-replay-') as temp:
        fixture = Path(temp)/'monopole.json'
        name = 'src/qtgui/tests/StmHarmonicFrameCases.json'
        fixture.write_bytes(member(stage,'baseline-v1',name))
        need(sha(fixture) == plan['fixture_sha256'] and
             all(ids[name] == sha(fixture) for ids,build in versions.values()), 'Frozen monopole fixture changed')
        for version,passed in [('baseline-v1',15),('after-v1',60)]:
            root = stage/version
            run = mapped(root,versions[version][1],'GmatStmHarmonicFrameTests')
            need(run['fixture_sha256'] == sha(fixture), 'Executed monopole fixture differs')
            result = monopole_audit(root,fixture)
            need(result == load(root/'independent-audit.json') and result['passed'] == passed and
                 result['failed'] == 60-passed and not result['issues'], 'Monopole baseline/pass replay differs')
        rows = load(stage/'baseline-v1/proof.json')['observations']
        need(sum(v['rejected'] and 'captured acceleration gradient is required' in v['diagnostic']
                 for v in rows) == 45, 'Original missing-gradient failures were not preserved')
        fixture = Path(temp)/'pointmass.json'
        fixture.write_bytes(member(stage,'after-v1','src/qtgui/tests/StmOwnerFrameCases.json'))
        root = stage/'dependent-pointmass-v1'
        run = mapped(root,versions['after-v1'][1],'GmatStmOwnerFrameTests')
        need(run['fixture_sha256'] == sha(fixture), 'Executed dependent point-mass fixture differs')
        result = pointmass_audit(root,fixture)
        need(result == load(root/'independent-audit.json') and result['passed'] == 60 and not result['issues'],
             'Shared provider-loop dependent check differs')
        fixture = Path(temp)/'j2.json'
        fixture.write_bytes(member(stage,'nonzero-v1','src/qtgui/tests/StmHarmonicJ2Cases.json'))
        j2_plan = load(stage/'nonzero-field-frozen-plan-v1.json')
        need(sha(fixture) == j2_plan['fixture_sha256'] and j2_plan['expected_reads'] == 60 and
             not j2_plan['expectations_or_tolerances_relaxed'], 'Frozen nonzero field obligation differs')
        root = stage/'nonzero-v1'
        run = mapped(root,versions['nonzero-v1'][1],'GmatStmHarmonicFrameTests')
        need(run['fixture_sha256'] == sha(fixture), 'Executed J2 fixture differs')
        result = j2_audit(root,fixture)
        need(result == load(root/'independent-audit.json') and result['native_reads'] == 60,
             'Independent nonzero harmonic variational replay differs')
        old_fixture = Path(temp)/'old-limited.json'
        name = 'src/qtgui/tests/StmHarmonicJ2LimitedCases.json'
        old_fixture.write_bytes(member(stage,'limited-before-v1',name))
        old_reader = Path(temp)/'original_j2_reader.py'
        old_reader.write_bytes(member(stage,'limited-before-v1','src/qtgui/tests/StmHarmonicJ2Oracle.py'))
        specification = importlib.util.spec_from_file_location('_original_limited_j2_reader',old_reader)
        module = importlib.util.module_from_spec(specification); specification.loader.exec_module(module)
        original = stage/'limited-before-v1'
        run = mapped(original,versions['limited-before-v1'][1],'GmatStmHarmonicFrameTests')
        need(run['fixture_sha256'] == sha(old_fixture), 'Original limited-case fixture differs')
        held = load(original/'independent-audit.json')
        try:
            module.audit(original,old_fixture)
        except ValueError as error:
            need(held['status'] == 'failed' and str(error) == held['reason'] ==
                 'Native J2 STM differs from independent variational flow', 'Original oracle-model failure reclassified')
        else:
            raise ValueError('Original incorrect truncation-model failure was waived')
        correction = load(stage/'native-truncation-model-correction-v1.json')
        need(sha(old_fixture) == correction['original_fixture_sha256'] and
             sha(original/'independent-audit.json') == correction['original_failure_sha256'] and
             not correction['tolerances_changed'] and not correction['original_failure_reclassified'],
             'Native truncation-model correction lost its original failure')
        fixture = Path(temp)/'native-limited.json'
        name = 'src/qtgui/tests/StmHarmonicJ2LimitedNativeCases.json'
        fixture.write_bytes(member(stage,'limited-native-after-v1',name))
        need(sha(fixture) == correction['corrected_fixture_sha256'], 'Corrected native truncation fixture changed')
        for version,success in [('limited-native-before-v1',False),('limited-native-after-v1',True)]:
            root = stage/version
            run = mapped(root,versions[version][1],'GmatStmHarmonicFrameTests')
            need(run['fixture_sha256'] == sha(fixture), 'Executed native truncated fixture differs')
            if success:
                result = j2_audit(root,fixture)
                need(result == load(root/'independent-audit.json') and result['native_reads'] == 60,
                     'Full-gradient correction replay differs')
            else:
                held = load(root/'independent-audit.json')
                try:
                    j2_audit(root,fixture)
                except ValueError as error:
                    need(held['status'] == 'failed' and str(error) == held['reason'] ==
                         'J2 owner-frame STM differs from independent full Cartesian differential',
                         'Original truncated frame-gradient failure reclassified')
                else:
                    raise ValueError('Original truncated frame-gradient failure was waived')
        a,b = [load(stage/v/'proof.json') for v in ('limited-native-before-v1','limited-native-after-v1')]
        need(all(a[k] == b[k] for k in ('integration_seconds','owner_state','guide_state','native_stm','guide_stm')),
             'Native propagation changed during frame-gradient repair')
        preservation = load(stage/'native-truncation-preservation-v1.json')
        need(preservation['status'] == 'passed' and all(preservation['unchanged_values'].values()) and
             preservation['unchanged_native_StmLimit'] == 0 and not preservation['native_variational_accuracy_claimed'],
             'Native truncated approximation preservation record differs')
    zero = load(stage/'independent-oracle-zero-limit-v1.json')
    need(zero['status'] == 'passed' and zero['max_state_error'] < 1e-8 and zero['max_stm_error'] < 1e-10 and
         zero['native_tests_run'] == 0, 'Independent J2 zero-coefficient limit differs')
    protected = load(stage/'protected-input-check-v1.json')
    need(protected['status'] == 'passed' and protected['original_inputs_unchanged'] == 1334 and
         not protected['changed'] and sha(protected['manifest']) == protected['manifest_sha256'],
         'Protected original input record differs')
    index = load(stage/'coordinate-obligation-index-v1.json')
    need(len(index['named_property_children']) == 22 and len(index['coordinate_source_candidate']['axes']) == 16 and
         len(index['coordinate_source_candidate']['origins']) == 5 and index['new_requirement_credit'] == 0,
         'Original coordinate conjunction inventory differs')
    for item in index['saved_evidence']:
        need(sha(item['receipt']) == item['sha256'], 'Reused source-versioned engineering receipt differs')
    return dict(status='passed',production_source_files_modified=2,production_private_headers_added=1,
                native_missions_after_repairs=4,native_parameter_reads=240,defined_reads=180,
                mathematical_singularity_rejections=60,original_missing_gradient_failures_retained=45,
                original_pre_force_path_failures_retained=2,original_inputs_unchanged=1334,
                original_truncation_model_failure_retained=True,original_truncated_frame_failure_retained=True,
                native_truncated_propagation_unchanged=True,
                nonzero_harmonic_degree_qualified=2,higher_degree_or_tide_gradient_qualified=False,
                independent_earth_orientation_qualified=False,native_tests_run_by_replay=0,
                new_requirement_credit=0,requirements_verified=1106,requirements_total=1158,
                agreed_complete=5,agreed_total=18,whole_row_qualified=False,
                full_current_build_qualified=False,full_regression=False)
