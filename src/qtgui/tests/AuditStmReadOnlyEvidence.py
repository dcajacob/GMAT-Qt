"""Read retained native STM setter evidence without executing a mission/test."""
from pathlib import Path
import hashlib
import json
import tarfile

from RunStmReadOnlyContract import audit as output_audit

load = lambda p: json.loads(Path(p).read_text())
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(ok, message):
    if not ok: raise ValueError(message)


def audit(repo, stage):
    repo, stage = Path(repo), Path(stage)
    plan = load(stage / 'frozen-plan-v1.json')
    need(plan['row'] == 'FRR!E525' and plan['fingerprint'] == '2be2d97f1f8fd7ca79b2e65cfd084cf206c186135f8ee2e5ccc0a1bcca3a7b39', 'Original STM parent changed')
    need(plan['generic_rejections_required'] == 15 and plan['typed_rejection_controls'] == 5 and
         plan['numerical_tolerances_changed'] == 0 and plan['new_requirement_credit'] == 0, 'Frozen obligation changed')
    versions, source_text = {}, {}
    for version in ('before', 'after'):
        ids = load(stage / f'source-sha256-{version}-v1.json')
        need(len(ids) == 279, 'Executed source census differs')
        with tarfile.open(stage / f'sources-{version}-v1.tar.gz') as archive:
            need(set(archive.getnames()) == set(ids), 'Executed source archive inventory differs')
            for n, h in ids.items(): need(hashlib.sha256(archive.extractfile(n).read()).hexdigest() == h, 'Executed source bytes differ')
            source_text[version] = {n: archive.extractfile('src/base/parameter/' + n).read().decode()
                                    for n in ['OrbitStmParameters.cpp', 'OrbitStmParameters.hpp']}
        build = load(stage / f'build-{version}-v1.json')
        need(build['status'] == 'passed' and build['returncode'] == 0 and build['inputs_unchanged'] and
             build['source_sha256'] == ids and build['log_sha256'] == sha(stage / f'build-{version}-v1.log'), 'Build/source guard failed')
        output = stage / f'{version}-v1'; result = output_audit(output)
        need(result == load(output / 'independent-audit.json'), 'Retained independent audit differs')
        run = load(output / 'run.json'); images = load(output / 'loaded-images.json')
        need(run['native_terminal'] and run['runtime_binary_copies'] == 0, 'Native process not terminal or runtime copied')
        binary = str(repo / 'build/linux-gui/src/qtgui/GmatStmReadOnlyContractTests')
        need(images[binary] == build['binary_sha256'][binary], 'Wrong native worker executed')
        for library in ('libGmatBase.so', 'libGmatUtil.so'):
            name = repo / 'application/bin' / library
            need(images[str(name.resolve())] == build['binary_sha256'][str(name)], 'Wrong native library executed')
        if version == 'before':
            need(result['status'] == 'failed' and result['generic_failed'] == 15 and result['generic_passed'] == 0 and
                 result['typed_controls_passed'] == 5 and run['returncode'] == 1, 'Original setter failures lost or relabelled')
            rows = load(output / 'proof.json')['generic_cases']
            need(all(not c['rejected'] and not c['cached_unchanged'] and c['owner_unchanged'] and not c['settable'] for c in rows), 'Baseline cache-only defect differs')
        else:
            need(result['status'] == 'passed' and result['generic_passed'] == 15 and result['generic_failed'] == 0 and
                 result['typed_controls_passed'] == 5 and run['returncode'] == 0, 'Repaired native contract failed')
        versions[version] = ids
    changed = {n for n, h in versions['before'].items() if versions['after'][n] != h}
    need(changed == {'src/base/parameter/OrbitStmParameters.cpp', 'src/base/parameter/OrbitStmParameters.hpp'}, 'Witness or unrelated owner changed')
    before, after = source_text['before'], source_text['after']
    declaration = '   void SetRmatrix(const Rmatrix &value) override;\n'
    need(after['OrbitStmParameters.hpp'].count(declaration) == 5 and
         after['OrbitStmParameters.hpp'].replace(declaration, '') == before['OrbitStmParameters.hpp'], 'Existing declarations/layout changed')
    need(after['OrbitStmParameters.cpp'].replace('#include "ParameterException.hpp"\n', '', 1).startswith(before['OrbitStmParameters.cpp']),
         'Existing STM getter/equation/Clone/Copy implementation changed')
    for name in ['OrbitStm', 'OrbitStmA', 'OrbitStmB', 'OrbitStmC', 'OrbitStmD']:
        need(('void ' + name + '::SetRmatrix(const Rmatrix &)') in after['OrbitStmParameters.cpp'], 'Missing concrete read-only native dispatch')
    protected = load(stage / 'protected-input-check-v1.json')
    need(protected['unchanged'] == 1334 and protected['changed'] == [], 'Original test inputs changed')
    source_guard = load(stage / 'source-after-execution-v1.json')
    need(source_guard['inputs_unchanged'] and source_guard['source_sha256'] == versions['after'], 'Post-run source guard differs')
    prefix = stage.parent / 'complete18-20261111-oem-file-invariants-v1/sealed-current-prefix-v1.json'
    need(sha(prefix) == 'de77757b896aa68c65df2462c026b58bf11ed684b502148fa5a14c0593111574', 'Latest admitted prefix changed')
    need(load(stage / 'remaining-coordinate-obligation-v1.json')['whole_row_qualified'] is False, 'Unfinished coordinate parent was admitted')
    return dict(status='passed', generic_readonly_rejections=15, typed_controls_passed=5,
                original_generic_failures_retained=15, native_executions=2, native_missions=0,
                native_tests_run_by_replay=0, original_inputs_unchanged=1334,
                existing_getter_equations_and_clone_copy_implementations_unchanged=True,
                public_instance_layout_and_existing_virtual_slots_unchanged=True,
                source_versioned_only=True, new_requirement_credit=0,
                requirements_verified=1106, requirements_total=1158, agreed_complete=5,
                agreed_total=18, selected_frame_conversion_qualified=False,
                full_current_build_qualified=False, full_regression=False)
