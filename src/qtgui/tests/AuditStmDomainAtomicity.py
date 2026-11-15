"""Join retained STM domain witnesses without launching native tests or granting row credit."""
import argparse
import hashlib
import json
from pathlib import Path
import tempfile

from RunStmDomainAtomicity import audit as check


def load(path):
    return json.loads(Path(path).read_text())


def require(value, message):
    if not value:
        raise AssertionError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def audit(stage):
    stage = Path(stage)
    original = load(stage/'StmDomainAtomicityCases.json')
    full = load(stage/'after-source-v1/src/qtgui/tests/StmDomainAtomicityCases.json')
    require(len(original['cases']) == 17 and full['cases'][:17] == original['cases'] and
            full['cases'][17] == dict(key='fresh_valid_axes', mode='fresh_valid_axes', outcome='accept'),
            'Original expectations changed or positive control lost')
    for name in ['native_stm', 'expected_stm', 'matrix_tolerance', 'row', 'fingerprint']:
        require(original[name] == full[name], 'Frozen numerical/requirement contract changed')
    prefix = check(stage/'retained-nonfinite-prefix-v1', stage/'completed-nonfinite-prefix-v1.json')
    before = check(stage/'before-v3', stage/'pending-shape-and-history-v1.json')
    after = check(stage/'after-v1', stage/'history-repair-controls-v1.json')
    require(prefix['status'] == 'passed' and prefix['reads'] == 15, 'Retained nonfinite prefix differs')
    require(before['issues'] == [] and before['passed'] == 65 and before['failed'] == 5,
            'Original failure/pass census differs')
    failures = [r for r in before['checks'] if not r['passed']]
    require(all(r['case'] == 'fresh_invalid_axes' and r['failures'] == ['owner changed'] for r in failures),
            'Unexpected original defect')
    require(after['status'] == 'passed' and after['reads'] == 10, 'Repair/positive control failed')
    reused = prefix['checks'] + [r for r in before['checks'] if r['passed']]
    selected = reused + after['checks']
    require(len(selected) == 90 and len({(r['case'], r['type']) for r in selected}) == 90,
            'Incomplete or duplicate selected domain/type conjunction')
    require({(r['case'], r['type']) for r in selected} ==
            {(c['key'], t) for c in full['cases'] for t in
             ['OrbitSTM', 'OrbitSTMA', 'OrbitSTMB', 'OrbitSTMC', 'OrbitSTMD']},
            'Missing public Parameter/domain route')
    provenance = load(stage/'retained-nonfinite-prefix-v1/provenance.json')
    require(sha(provenance['original_proof']) == provenance['original_proof_sha256'],
            'Changed partial-run failure receipt')
    raw = load(provenance['original_proof'])['observations']
    require(raw == load(stage/'retained-nonfinite-prefix-v1/proof.json')['observations'],
            'Retained observations were relabeled or edited')
    require(load(stage/'before-v2/run.json')['returncode'] == 1 and
            load(stage/'before-v3/run.json')['returncode'] == 0 and
            load(stage/'after-v1/run.json')['returncode'] == 0,
            'Enclosing native statuses changed')
    delta = load(stage/'native-image-delta-v1.json')
    require(len(delta['changed_application_images']) == 1 and
            sha(stage/'libGmatBase-before.so.R2026a') == delta['before_base_sha256'] and
            sha(stage/'libGmatBase-after.so.R2026a') == delta['after_base_sha256'],
            'Missing immutable native-library versions')
    negative = []
    with tempfile.TemporaryDirectory(prefix='stm-domain-replay-negative-') as directory:
        root = Path(directory); proof = load(stage/'after-v1/proof.json')
        row = proof['observations'][0]
        row['owner_after']['history']['present'] = True
        (root/'proof.json').write_text(json.dumps(proof))
        negative.append(check(root, stage/'history-repair-controls-v1.json')['status'] == 'failed')
        proof = load(stage/'after-v1/proof.json'); proof['observations'][0]['rejected'] = False
        (root/'proof.json').write_text(json.dumps(proof))
        negative.append(check(root, stage/'history-repair-controls-v1.json')['status'] == 'failed')
        proof = load(stage/'after-v1/proof.json'); proof['observations'][5]['actual'][0][0] += .01
        (root/'proof.json').write_text(json.dumps(proof))
        negative.append(check(root, stage/'history-repair-controls-v1.json')['status'] == 'failed')
    require(all(negative), 'A perturbed history/rejection/numerical result was accepted')
    return dict(status='passed', row='FRR!E525', source_versioned_only=True,
                selected_domain_type_witnesses=90, reused_pre_repair_passes=80,
                repaired_rejections=5, successful_new_reference_controls=5,
                original_domain_failures_retained=5, harness_failures_retained=2,
                negative_controls=len(negative), production_files_changed=1,
                public_api_changes=0, original_assertions_changed=0,
                native_tests_run_by_replay=0, new_requirement_credit=0,
                whole_row_qualified=False, full_current_build_qualified=False,
                full_regression=False)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('stage', type=Path); p.add_argument('--output', type=Path)
    args = p.parse_args(); result = audit(args.stage)
    if args.output:
        with args.output.open('x') as handle:
            json.dump(result, handle, indent=2); handle.write('\n')
    print(json.dumps(result))
