"""Admit only the original complete FRC176 MATLAB Assignment RHS obligation."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import NasaGuiCoverage as nasa
from BuildNasaCoverageExtension import load, require, sha

KIND = 'source-versioned-complete-MATLAB-Assignment-RHS'
KEYS = ['FRC!E176']
FINGERPRINT = '1208c603c9962c90deb486ca333c0c3323b8535b632aef38632400dc2807090e'
CLAUSES = {'original-source-context', 'actual-single-equals-editor-entry',
           'licensed-MATLAB-result-and-caller-isolation', 'user-Variable-Array-String',
           'settable-real-string-and-all-writable-Rvector-types',
           'literal-variable-nested-Parameter-array-indices',
           'native-result-type-dimension-and-index-rejections',
           'source-history-files-reopen-repeat-and-viewer'}


def review(q, scope, inventory, before, overlay):
    require(q['contract_kind'] == KIND and q['qualified_keys'] == KEYS,
            'Different exact Assignment row')
    require(all(q[k] is False for k in ['scientific_approval', 'technical_core_approval',
            'full_current_build_qualified', 'full_NASA_qualified', 'MATLAB_bundle_qualified']),
            'Broader qualification falsely closed')
    known = {r['key']: r for r in inventory['requirements']}
    require(len(known) == 1158 and len(inventory['historical_cases']) == 2367
            and q['authority_sha256'] == scope['authority_sha256'] == inventory['sources'],
            'Original authority/denominators differ')
    row = known[KEYS[0]]
    require(row['fingerprint'] == FINGERPRINT and row['declared_id'] == 'FRC-19.3.8'
            and scope['candidate_row'] == row, 'Original exact Assignment leaf changed')
    require(scope['original_context'] == [known['FRC!E'+str(i)] for i in range(162, 179)],
            'Original parent/LHS/RHS/type/dimension context changed')
    require(scope['whole_row_qualified'] is True
            and set(scope['clauses']) == CLAUSES
            and all(v['satisfied'] is True and v['evidence'] for v in scope['clauses'].values())
            and scope['eligibility_exclusions'] == [], 'Incomplete or narrowed exact-row conjunction')
    require(before['coverage']['requirements']['verified'] == 1050
            and before['coverage']['requirements']['total'] == 1158
            and before['coverage']['historical_cases']['verified'] == 1816
            and before['requirements_still_needed'] == 51
            and KEYS[0] in {r['key'] for r in before['remaining_original_rows']['requirements']},
            'Prior exact union or original remaining membership differs')
    original = next(r for r in overlay['reviewed_source_rows'] if r['key'] == KEYS[0])
    require(scope['previous_closure_record'] == original
            and len(overlay['reviewed_source_rows']) == 139
            and scope['original_hold_keys'] == overlay['original_hold_keys_retained']
            and len(set(scope['original_hold_keys'])) == 59,
            'Original closure records/hold identities lost')
    require(scope['distinct_already_admitted_key'] == 'All Others!E180'
            and scope['compound_MATLAB_math_qualified'] is False,
            'Duplicate Mission Tree credit or invented compound-expression claim')


def qualify(repo, path, inventory):
    q = load(path)
    scope, before, overlay = load(q['scope_review']), load(q['prior_coverage']), load(q['prior_overlay'])
    review(q, scope, inventory, before, overlay)
    for name, digest in q['artifact_sha256'].items():
        require(Path(name).is_file() and not Path(name).is_symlink() and sha(name) == digest,
                'Changed exact-row artifact: '+name)
    with tarfile.open(q['reader_source_archive']) as archive:
        require(set(archive.getnames()) == set(q['reader_source_sha256']), 'Admission reader universe differs')
        for name, digest in q['reader_source_sha256'].items():
            require(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest
                    and sha(repo/name) == digest, 'Executing admission reader differs: '+name)
    stage = Path(q['component_stage'])
    manifest = load(stage/'evidence-manifest-v1.json')
    require(sha(stage/'evidence-manifest-v1.json') ==
            'f3b003f8269305faf922c84c36cb284110cb2c5242e42e6bc120daae177c6c62',
            'Actual Assignment component seal differs')
    replay = load(stage/'frozen-audit-replay-v1.json')
    require(replay['returncode'] == 0 and replay['byte_identical_to_component_audit']
            and replay['reader_sha256'] == manifest['auditor_sha256']
            and replay['manifest_sha256'] == sha(stage/'evidence-manifest-v1.json')
            and replay['native_tests_rerun'] == replay['runtime_binary_copies'] == 0,
            'Frozen actual Assignment replay unproven')
    source_review = load(stage/'row-scope-review-v1.json')
    contract = load(repo/'build/nasa-coverage/coverage95-20261104-matlab-assignment-destinations/source-contract-review-v3.json')
    require(source_review['native_parameter_metadata'] == scope['native_parameter_metadata']
            == contract['native_parameter_metadata'] and len(scope['native_parameter_metadata']) == 324,
            'Original native type inventory narrowed')
    partitions = {t: [r['factory_type'] for r in scope['native_parameter_metadata']
                     if r['metadata']['settable'] and r['metadata']['return_type'] == t]
                  for t in [4, 7, 11]}
    require([len(partitions[t]) for t in [4, 7, 11]] == [224, 5, 3]
            and partitions[11] == ['Quaternion', 'ThrustAngle1', 'ThrustAngle2']
            and sum(r['metadata']['settable'] for r in scope['native_parameter_metadata']) == 232,
            'An allowable settable return category omitted')
    with tempfile.TemporaryDirectory(prefix='gmat-MATLAB-Assignment-admission-') as folder:
        root = Path(folder)
        reader, output = root/'audit.py', root/'audit.json'
        with tarfile.open(stage/'reviewed-sources-v1.tar.gz') as archive:
            reader.write_bytes(archive.extractfile('src/qtgui/tests/AuditMatlabAssignmentOperator.py').read())
        require(sha(reader) == manifest['auditor_sha256'], 'Frozen component reader changed')
        result = subprocess.run([sys.executable, str(reader), '--repo', str(repo),
            '--manifest', str(stage/'evidence-manifest-v1.json'), '--selections',
            str(stage/'selections-v1.json'), '--output', str(output)], capture_output=True, text=True)
        require(result.returncode == 0, 'Complete frozen Assignment/typed/index/predecessor replay failed: '+result.stderr[-1800:])
        require(output.read_bytes() == (stage/'evidence-audit-v1.json').read_bytes()
                == (stage/'component-audit-v1.json').read_bytes(), 'Complete frozen audit output differs')
        component = load(output)
    require(component['new_native_contexts'] == 6 and component['original_regression_contexts'] == 2
            and component['damaged_models_rejected'] == 40
            and component['original_assertions_preserved'] == 76
            and component['new_requirement_credit'] == 0,
            'Complete operator conjunction differs')
    controls = 0
    for mutate in [lambda x: x.update(candidate_row=dict(x['candidate_row'], fingerprint='0'*64)),
                   lambda x: x['clauses'].pop('user-Variable-Array-String'),
                   lambda x: x['clauses']['actual-single-equals-editor-entry'].update(satisfied=False),
                   lambda x: x['original_hold_keys'].pop(),
                   lambda x: x.update(eligibility_exclusions=['Quaternion'])]:
        damaged = copy.deepcopy(scope)
        mutate(damaged)
        rejected = False
        try:
            review(q, damaged, inventory, before, overlay)
        except (ValueError, KeyError, AssertionError):
            rejected = True
        require(rejected, 'Damaged whole-row scope escaped')
        controls += 1
    q['audit_summary'] = dict(status='passed-complete-original-MATLAB-Assignment-RHS',
        qualified_keys=KEYS, whole_row_qualified=True, original_holds_retained=59,
        original_requirements=1158, historical_operations=2367,
        source_versioned_component_replay=component, damaged_scope_models_rejected=controls,
        duplicate_Mission_Tree_credit=0, compound_MATLAB_math_qualified=False,
        scientific_approval=False, technical_core_approval=False,
        full_current_build_qualified=False, full_NASA_qualified=False, MATLAB_bundle_qualified=False)
    return q, KEYS


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    q, keys = qualify(args.repo.resolve(), args.qualification, nasa.inventory(args.repo.resolve()))
    result = dict(status='passed', qualified_keys=keys, summary=q['audit_summary'])
    require(not args.output.exists(), 'Refuse overwrite')
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))
