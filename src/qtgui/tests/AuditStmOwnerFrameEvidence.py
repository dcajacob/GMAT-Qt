"""Replay the bounded owner-dependent STM evidence without launching native code."""
import hashlib
import json
from pathlib import Path
import tarfile
import tempfile

from StmOwnerFrameOracle import audit as frame_audit
from StmOwnerFunctionOracle import audit as function_audit
from RunStmFrameReference import audit as reference_audit

load = lambda p: json.loads(Path(p).read_text())
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(ok, message):
    if not ok:
        raise ValueError(message)


def capsule(stage, version):
    ids = load(stage / ('source-sha256-' + version + '.json'))
    with tarfile.open(stage / ('sources-' + version + '.tar.gz')) as bundle:
        need(set(bundle.getnames()) == set(ids), 'Source capsule inventory differs')
        for name, digest in ids.items():
            need(hashlib.sha256(bundle.extractfile(name).read()).hexdigest() == digest,
                 'Source capsule bytes differ: ' + name)
    build = load(stage / ('build-' + version + '.json'))
    need(build['status'] == 'passed' and build['returncode'] == 0 and build['inputs_unchanged'] and
         build['source_sha256'] == ids and build['log_sha256'] == sha(stage / ('build-' + version + '.log')),
         'Source/build guard differs')
    return ids, build


def member(stage, version, name):
    with tarfile.open(stage / ('sources-' + version + '.tar.gz')) as bundle:
        return bundle.extractfile(name).read()


def mapped(root, build, worker):
    run, images = load(root / 'run.json'), load(root / 'loaded-images.json')
    need(run['returncode'] == 0 and run['native_terminal'] and run['runtime_binary_copies'] == 0,
         'Native process was not terminal/successful')
    for name, digest in build['binary_sha256'].items():
        logical = Path(name)
        if logical.name != worker and not logical.name.endswith('.so'):
            continue
        need(any(Path(p).parent == logical.parent and h == digest and
                 (Path(p).name == logical.name or Path(p).name.startswith(logical.name + '.'))
                 for p, h in images.items()), 'Wrong mapped image: ' + name)
    return run


def audit(repo, stage):
    repo, stage = Path(repo), Path(stage)
    versions = {v: capsule(stage, v) for v in ('before-v1', 'after-v1', 'functions-v1')}
    before, after, functions = [versions[v][0] for v in ('before-v1', 'after-v1', 'functions-v1')]
    fix = load(stage / 'implementation-plan-v1.json')
    changed = {'src/base/spacecraft/OrbitStmContext.hpp', 'src/base/parameter/OrbitStmFrame.hpp',
               'src/base/forcemodel/ODEModel.cpp'}
    need({n for n, h in before.items() if after.get(n) != h} == changed and
         set(after) - set(before) == {'src/base/parameter/OrbitStmOwnerFrame.hpp'} and
         all(before[n] == h for n, h in fix['production_preimages'].items()) and
         fix['public_API_changes'] == fix['original_assertion_or_tolerance_changes'] == 0,
         'Production repair/preimage scope differs')
    need({n for n, h in after.items() if functions.get(n) != h} == {'src/qtgui/CMakeLists.txt'} and
         set(functions) - set(after) == {'src/qtgui/tests/' + n for n in (
             'StmOwnerFunctionTests.cpp', 'StmOwnerFunctionOracle.py', 'RunStmOwnerFunction.py',
             'StmOwnerFunctionInputFrameCase.json', 'StmOwnerFunctionLocalFrameCase.json')},
         'Function additions changed previously executed inputs')
    for name, digest in versions['after-v1'][1]['binary_sha256'].items():
        if name.endswith('.so'):
            need(versions['functions-v1'][1]['binary_sha256'][name] == digest,
                 'Production library changed after core owner-frame passes')
    plan = load(stage / 'frozen-plan-v1.json')
    previous = Path(plan['previous_engineering_receipt'])
    need(sha(previous) == plan['previous_engineering_receipt_sha256'] ==
         'f1864af9d4fadce42bed651c5f0976d5a28bd167ca4ed402f55315c35a1757a7',
         'Previous engineering receipt differs')
    prior = load(previous.parent / 'source-sha256-metadata-v1.json')
    need({n for n, h in prior.items() if before.get(n) != h} == {'src/qtgui/CMakeLists.txt'},
         'Production source changed before the baseline')
    need(plan['expected_reads'] == 60 and plan['expected_singular_rejections'] == 15 and
         plan['matrix_block_tolerances'] == [2e-8, 2e-6, 2e-11, 2e-8], 'Frozen owner-frame obligation differs')
    function_plan = load(stage / 'function-frozen-plan-v1.json')
    need(function_plan['expected_native_parameter_reads'] == 60 and
         function_plan['expected_reported_matrix_entries'] == 1008 and
         function_plan['expected_nested_calls'] == 2, 'Frozen function obligation differs')
    with tempfile.TemporaryDirectory(prefix='gmat-stm-owner-replay-') as temporary:
        fixture = Path(temporary) / 'owner.json'
        name = 'src/qtgui/tests/StmOwnerFrameCases.json'
        fixture.write_bytes(member(stage, 'before-v1', name))
        need(sha(fixture) == plan['fixture_sha256'] == before[name] == after[name],
             'Frozen owner-frame fixture changed')
        for version, passed in [('before-v1', 0), ('after-v1', 60)]:
            root = stage / version
            run = mapped(root, versions[version][1], 'GmatStmOwnerFrameTests')
            need(run['fixture_sha256'] == sha(fixture), 'Executed owner-frame fixture differs')
            result = frame_audit(root, fixture)
            need(result == load(root / 'independent-audit.json') and result['passed'] == passed and
                 result['failed'] == 60 - passed and not result['issues'],
                 'Original failure or passing owner-frame replay differs')
        results = []
        for key in ('InputFrame', 'LocalFrame'):
            name = 'src/qtgui/tests/StmOwnerFunction' + key + 'Case.json'
            fixture = Path(temporary) / (key + '.json')
            fixture.write_bytes(member(stage, 'functions-v1', name))
            need(sha(fixture) == function_plan['fixture_sha256'][name], 'Frozen function fixture changed')
            root = stage / ('function-' + ('input' if key == 'InputFrame' else 'local') + '-v1')
            run = mapped(root, versions['functions-v1'][1], 'GmatStmOwnerFunctionTests')
            need(run['fixture_sha256'] == sha(fixture), 'Executed function fixture differs')
            result = function_audit(root, fixture)
            need(result == load(root / 'independent-audit.json') and result['status'] == 'passed' and
                 result['native_parameter_reads'] == 30 and result['reported_matrix_entries'] == 504,
                 'Function ownership/report replay differs')
            results.append(result)
        name = 'src/qtgui/tests/StmFrameReferenceCases.json'
        fixture = Path(temporary) / 'reference.json'
        fixture.write_bytes(member(stage, 'functions-v1', name))
        root = stage / 'dependent-reference-v1'
        run = mapped(root, versions['functions-v1'][1], 'GmatStmFrameReferenceTests')
        need(run['fixture_sha256'] == sha(fixture), 'Executed dependent fixture differs')
        dependent = reference_audit(root, fixture)
        need(dependent == load(root / 'independent-audit.json') and dependent['passed'] == 90 and
             dependent['failed'] == 0 and not dependent['issues'], 'Dependent snapshot/reference replay differs')
    protected = load(stage / 'protected-input-check-v1.json')
    need(protected['status'] == 'passed' and protected['original_inputs_unchanged'] == 1334 and
         not protected['changed'] and sha(protected['manifest']) == protected['manifest_sha256'],
         'Protected original input record differs')
    return dict(status='passed', production_source_files_modified=3, production_private_headers_added=1,
                native_missions_after_repair=3, native_owner_frame_reads=60, native_function_reads=60,
                dependent_reference_reads=90, native_parameter_reads=210, defined_core_reads=45,
                mathematical_singularity_rejections=15, function_calls=6, nested_function_calls=2,
                reported_function_matrix_entries=1008, original_failed_reads_retained=60,
                original_inputs_unchanged=1334, native_tests_run_by_replay=0, new_requirement_credit=0,
                requirements_verified=1106, requirements_total=1158, agreed_complete=5, agreed_total=18,
                whole_row_qualified=False, full_current_build_qualified=False, full_regression=False)
