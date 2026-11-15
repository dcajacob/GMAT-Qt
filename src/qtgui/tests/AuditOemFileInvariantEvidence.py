"""Replay exact before/after B1 invariants, guards and the affected old check."""
from pathlib import Path
import hashlib
import json
import tarfile

from RunOemFileInvariants import audit as output_audit
from CcsdsOemB1Oracle import validate

load = lambda p: json.loads(Path(p).read_text())
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(value, message):
    if not value: raise ValueError(message)


def audit(repo, stage):
    repo, stage = Path(repo), Path(stage)
    plan = load(stage / 'frozen-plan-v1.json')
    need(plan['fingerprint'] == '1df0dff106d6b263a5387c53cccfb3a0f958b290fb5889a9aaeed282db36d72b', 'Original row differs')
    need(plan['standard']['sha256'] == 'fab306a994e5bce9cad8c64d77af172a5819af38faf459faab1b7e2f6163223c', 'Original standard differs')
    versions = {}
    for version in ('before', 'after'):
        ids = load(stage / f'source-sha256-{version}-v1.json')
        with tarfile.open(stage / f'sources-{version}-v1.tar.gz') as archive:
            need(set(archive.getnames()) == set(ids), 'Executed source inventory differs')
            for n, h in ids.items():
                need(hashlib.sha256(archive.extractfile(n).read()).hexdigest() == h, 'Executed source bytes differ')
        build = load(stage / f'build-{version}-v1.json')
        need(build['status'] == 'passed' and build['returncode'] == 0 and build['inputs_unchanged'], 'Build guard failed')
        need(build['source_sha256'] == ids, 'Build sources differ')
        need(build['log_sha256'] == sha(stage / f'build-{version}-v1.log'), 'Build log differs')
        output = stage / f'{version}-v1'
        result = output_audit(output)
        need(result == load(output / 'independent-audit.json'), 'Retained independent audit differs')
        proof = load(output / 'proof.json'); run = load(output / 'run.json')
        need([x['case'] for x in proof['cases']] == plan['cases'], 'Frozen native cases differ')
        need([x['case'] for x in proof['cases'] if x['expected_rejection']] == plan['expected_rejection'], 'Frozen expectations differ')
        need(run['native_terminal'] and run['runtime_binary_copies'] == 0, 'Native process not terminal or runtime copied')
        images = load(output / 'loaded-images.json')
        worker = str(repo / 'build/linux-gui/src/qtgui/GmatOemFileInvariantTests')
        util = str((repo / 'application/bin/libGmatUtil.so').resolve())
        need(images[worker] == build['binary_sha256'][worker] and
             images[util] == build['binary_sha256'][str(repo / 'application/bin/libGmatUtil.so')], 'Executed native images differ')
        if version == 'before':
            need(result['status'] == 'failed' and run['returncode'] == 1 and proof['failures'] == 5, 'Original five failures removed or relabelled')
            need([x['case'] for x in result['observations'] if x['status'] == 'failed'] == plan['expected_rejection'], 'Wrong original failures')
        else:
            need(result['status'] == 'passed' and run['returncode'] == 0 and proof['failures'] == 0, 'Post-repair invariant failed')
        versions[version] = ids
    need(len(versions['before']) == len(versions['after']) == 185, 'Executed source census differs')
    changed = {n for n, h in versions['before'].items() if h != versions['after'][n]}
    need(changed == {'src/gmatutil/util/CCSDSOEMWriter.cpp', 'src/gmatutil/util/CcsdsOemB1Chronology.hpp'}, 'Frozen witness or unrelated owner changed')
    dependent = stage / 'dependent-wire-v2'
    receipt = load(dependent / 'receipt.json'); proof = load(dependent / 'proof.json')
    need(receipt['status'] == 'passed' and receipt['returncode'] == 0 and receipt['inputs_unchanged'], 'Affected old chronology check failed')
    need(proof['outcome'] == 'passed' and proof['failed_cases'] == 0 and len(proof['cases']) == 13, 'Original chronology case omitted')
    with tarfile.open(dependent / 'sources.tar.gz') as archive:
        need(set(archive.getnames()) == set(receipt['source_sha256']), 'Dependent source archive differs')
        for n, h in receipt['source_sha256'].items():
            need(hashlib.sha256(archive.extractfile(n).read()).hexdigest() == h, 'Dependent source bytes differ')
    for n, h in versions['after'].items(): need(receipt['source_sha256'][n] == h, 'Dependent writer differs')
    need(len(receipt['processes']) == 1, 'Affected native execution missing')
    findings = []
    for case in proof['cases']:
        if case['expected_rejection']: continue
        result = validate((dependent / (case['case'] + '.oem')).read_bytes())
        need(result['status'] == 'passed' or
             (case['case'] == 'OptionalInterpolationOmitted' and result['gaps'] == ['No recommended interpolation method supplied']), 'Affected conformance check failed')
        findings.append(dict(case=case['case'], sha256=sha(dependent / (case['case'] + '.oem')), finding=result))
    expected = dict(status='passed', native_cases=13, valid_files=len(findings), findings=findings, original_assertions_changed=0)
    need(expected == load(dependent / 'independent-audit.json'), 'Dependent independent report differs')
    protected = load(repo / 'build/nasa-coverage/coverage95-20261110-aggregate-regression-v1/authorized-test-input-hashes-v1.json')
    recorded = load(stage / 'protected-input-check-v1.json')
    need(len(protected) == recorded['unchanged'] == 1334 and recorded['changed'] == [], 'Protected inputs changed')
    # The later supplementary chronology worker is outside the original1334
    # campaign inputs. Keep its separately published November8 identity too.
    need(receipt['source_sha256']['src/qtgui/tests/OemWireOrderTests.cpp'] ==
         '883d1f9d97804d98ca630de31a3057b0317a8d9773d659383aa476ef0379a8c1', 'Original chronology worker changed')
    terminals = load(stage / 'process-terminal-v1.json')
    need(terminals['status'] == 'passed' and len(terminals['observations']) == 3 and
         all(x['terminal'] for x in terminals['observations']), 'Native process termination unobserved')
    return dict(status='passed', frozen_native_cases=10, preserved_before_failures=5,
                after_cases_passed=10, recovered_rejections=5, affected_original_cases=13,
                original_inputs_unchanged=1334, production_owners_changed=2,
                native_executions_with_cases=3, display_startup_failure_retained=True,
                native_missions=0, native_tests_run_by_replay=0, new_requirement_credit=0,
                full_regression=False, full_current_build_qualified=False)
