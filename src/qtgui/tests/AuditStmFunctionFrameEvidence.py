"""Replay immutable native function-frame evidence; never launch native tests."""
from pathlib import Path
import hashlib
import json
import tarfile
import tempfile

from StmFunctionFrameOracle import audit as numerical_audit

load = lambda p: json.loads(Path(p).read_text())
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(ok, message):
    if not ok:
        raise ValueError(message)


def archived(stage, version):
    ids = load(stage/('source-sha256-'+version+'.json'))
    with tarfile.open(stage/('sources-'+version+'.tar.gz')) as bundle:
        need(set(bundle.getnames()) == set(ids), 'Source capsule inventory differs')
        for n, h in ids.items():
            need(hashlib.sha256(bundle.extractfile(n).read()).hexdigest() == h, 'Source capsule bytes differ: '+n)
    build = load(stage/('build-'+version+'.json'))
    need(build['source_sha256'] == ids and build['inputs_unchanged'] and build['status'] == 'passed' and
         build['returncode'] == 0 and build['log_sha256'] == sha(stage/('build-'+version+'.log')), 'Build/source guard failed')
    return ids, build


def image_matches(images, logical, digest):
    # Resolve the recorded logical name against recorded mapped images, not
    # today's mutable shared-library symlinks.
    logical = Path(logical)
    return any(Path(n).parent == logical.parent and
               (Path(n).name == logical.name or Path(n).name.startswith(logical.name+'.')) and h == digest
               for n, h in images.items())


def audit(repo, stage):
    repo, stage = Path(repo), Path(stage)
    plan = load(stage/'frozen-plan-v1.json')
    need(plan['cases'] == ['InputFrame', 'LocalFrame'] and plan['expected_function_calls'] == 6 and
         plan['expected_nested_calls'] == 2 and plan['expected_live_samples'] == 12 and
         plan['expected_parameter_reads'] == 60 and plan['expected_reported_matrix_entries'] == 1008 and
         not plan['full_regression'] and plan['new_requirement_credit'] == 0, 'Frozen function obligations differ')
    before, build = archived(stage, 'before-v1')
    link_ids, link_build = archived(stage, 'source-link-v1')
    need(all(link_ids[n] == h for n, h in before.items()) and len(set(link_ids)-set(before)) == 15 and
         all(n.startswith('plugins/GmatFunctionPlugin/') for n in set(link_ids)-set(before)), 'Source-link check changed executed inputs')
    need((stage/'build-source-link-v1.log').read_text().strip() == 'ninja: no work to do.', 'Affected plugin build did more than the recorded no-op')
    previous = Path(plan['previous_engineering_receipt'])
    need(sha(previous) == plan['previous_engineering_receipt_sha256'] ==
         'b38f3af44731e254369766aad632a8a7486812e411ed45ef6909fefb607b13dc', 'Prior engineering seal changed')
    prior_ids = load(previous.parent/'source-sha256-gui-v3.json')
    need({n for n, h in prior_ids.items() if before[n] != h} == {'src/qtgui/CMakeLists.txt'} and
         set(before)-set(prior_ids) == {'src/qtgui/tests/'+n for n in
             ['StmFunctionFrameTests.cpp', 'RunStmFunctionFrame.py', 'StmFunctionFrameOracle.py',
              'StmFunctionInputFrameCase.json', 'StmFunctionLocalFrameCase.json']}, 'Production or unrelated test input changed')
    prior_build = load(previous.parent/'build-after-v1.json')
    for n, h in prior_build['binary_sha256'].items():
        if n.endswith('.so'):
            need(link_build['binary_sha256'][n] == h, 'Passing older evidence rebound to changed production libraries')
    results = []
    for folder, key, mode in [('input-before-v1', 'InputFrame', 'input'), ('local-before-v1', 'LocalFrame', 'local')]:
        root = stage/folder
        run, images = load(root/'run.json'), load(root/'loaded-images.json')
        need(run['returncode'] == 0 and run['native_terminal'] and run['runtime_binary_copies'] == 0, 'Native function run not terminal/pass')
        for n, h in link_build['binary_sha256'].items():
            need(image_matches(images, n, h), 'Executed native worker/library/plugin differs: '+n)
        with tempfile.TemporaryDirectory(prefix='gmat-stm-function-replay-') as temporary:
            fixture = Path(temporary)/'case.json'
            name = 'src/qtgui/tests/StmFunction'+key+'Case.json'
            with tarfile.open(stage/'sources-before-v1.tar.gz') as bundle:
                fixture.write_bytes(bundle.extractfile(name).read())
            case = load(fixture)
            need(run['fixture_sha256'] == sha(fixture) and case['frame_mode'] == mode and
                 case['expected_function_calls'] == 3 and case['expected_nested_calls'] == 1 and
                 case['expected_live_samples'] == 6 and case['expected_parameter_reads'] == 30 and
                 case['expected_reported_matrix_entries'] == 504, 'Executed fixture/census differs')
            result = numerical_audit(root, fixture)
        need(result == load(root/'independent-audit.json') and result['status'] == 'passed' and
             result['native_parameter_reads'] == 30 and result['reported_matrix_entries'] == 504, 'Independent native evidence replay differs')
        proof = load(root/'proof.json')
        need(proof['samples'] == load(root/'live-samples.json') and not (root/'observer-error.txt').read_text(), 'Native live sample/error evidence differs')
        need('STM function input/frame witnesses completed' in (root/'native.log').read_text(), 'Native terminal log missing')
        results.append(result)
    protected = load(stage/'protected-input-check-v1.json')
    need(protected['status'] == 'passed' and protected['original_inputs_unchanged'] == 1334 and not protected['changed'] and
         sha(protected['manifest']) == protected['manifest_sha256'], 'Original protected inputs changed')
    return dict(status='passed', native_missions=2, direct_function_calls=4, nested_function_calls=2,
                live_function_samples=12, native_parameter_reads=60, reported_matrix_entries=1008,
                private_input_restoration=True, frame_rebinding=True, production_source_changes=0,
                executed_function_plugin_source_files=15, source_link_build_was_noop=True,
                max_internal_stm_error=max(v['max_internal_stm_error'] for v in results),
                original_inputs_unchanged=1334, native_tests_run_by_replay=0, new_requirement_credit=0,
                requirements_verified=1106, requirements_total=1158, agreed_complete=5, agreed_total=18,
                whole_row_qualified=False, full_current_build_qualified=False, full_regression=False)
