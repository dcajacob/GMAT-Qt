"""Join the final resource/function rows under the user's accepted meanings."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

KIND = 'final-agreed-resource-function-conjunction-v1'
KEYS = ['FRR!E470', 'FRR!E480', 'All Others!E157', 'All Others!E156',
        'FRR!E450', 'FRR!E472', 'FRC!E161']
AGREED_REMAINING = ['All Others!E156', 'All Others!E157', 'FRC!E161',
                    'FRR!E450', 'FRR!E472']


def load(path):
    return json.loads(Path(path).read_text())


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def verify_manifest(root):
    root = Path(root)
    receipt = load(root / 'stage-receipt-v1.json')
    manifest_path = root / 'evidence-manifest-v1.json'
    require(sha(manifest_path) == receipt['manifest_sha256'], 'Component manifest seal changed')
    manifest = load(manifest_path)
    entries = manifest.get('artifact_sha256', manifest)
    count = 0
    for name, digest in entries.items():
        if not isinstance(digest, str) or len(digest) != 64:
            continue
        path = Path(name)
        if not path.is_absolute():
            path = root / path
        require(sha(path) == digest, 'Changed component artifact ' + str(path))
        count += 1
    return count


def audit_index(index):
    require(index['qualified_keys'] == KEYS and index['agreed_remaining_rows'] == AGREED_REMAINING,
            'Final batch scope changed')
    for name, digest in index['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed bound evidence ' + name)
    decision = load(index['accepted_decisions'])
    require(decision['status'] == 'explicitly-approved-by-user' and
            decision['user_instruction'] ==
            'Accept both and finish the remaining five. Reuse the passing evidence and only run targeted checks where something is missing.',
            'Missing the user acceptance decision')
    require(decision['agreed_remaining_rows'] == AGREED_REMAINING and
            decision['necessary_prerequisite_rows'] == ['FRR!E470', 'FRR!E480'] and
            decision['original_denominator'] == 1158,
            'Accepted scope or denominator changed')
    require(set(decision['accepted_interpretations']) == {'function_compatibility', 'formation_copy'},
            'One accepted interpretation is missing')

    scope = load(index['scope_review'])
    require(scope['qualified_keys'] == KEYS and scope['whole_original_rows_with_approved_interpretations'],
            'Incomplete original row scope')
    require(scope['resource_catalog_reference']['original'] == 'FRAC-4.5' and
            scope['resource_catalog_reference']['resolved'] == 'FRAC-6.1' and
            scope['function_reference']['original'] == 'FRR-38' and
            scope['function_reference']['resolved'] == 'FRR-37',
            'Source cross-reference interpretation changed')
    require(scope['original_assertions_changed'] == 0 and not scope['failures_waived'] and
            scope['source_versioned_only'] and not scope['full_current_build_qualified'],
            'Evidence qualification expanded or original failures waived')

    roots = {name: Path(path) for name, path in index['component_stages'].items()}
    require(set(roots) == {'clone', 'clone_repair', 'formation', 'functions'}, 'Component family missing')
    artifact_count = sum(verify_manifest(root) for root in roots.values())

    clone = json.loads(subprocess.check_output(
        [sys.executable, str(roots['clone'] / 'audit-stage-v1.py')], text=True))
    require(clone['status'] == 'passed' and clone['catalog_types_with_positive_witnesses'] == 33 and
            clone['qualified_native_test_executions'] == 9 and clone['native_missions'] == 27 and
            clone['native_tests_run_by_replay'] == 0, 'Incomplete clone catalog execution evidence')
    catalog = load(roots['clone'] / 'complete-catalog-index-v1.json')
    require(len(catalog['entries']) == 33 and catalog['missing_types'] == [], 'Clone catalog incomplete')
    require([row['key'] for row in catalog['entries']] ==
            [f'All Others!E{number}' for number in range(121,154)], 'Resource catalog identity changed')
    old_clones = load(roots['clone_repair'] / 'saved-lifecycle-clone-evidence-v1.json')
    supplements = load(roots['clone'] / 'saved-supplementary-witnesses-v1.json')
    saved_cases = {row['case'] for row in old_clones['cases']} | {row['case'] for row in supplements['cases']}
    repaired_types = {row['type'] for row in load(roots['clone_repair'] / 'qualified-cases-v1.json')}
    direct_types = {'PropSetup', 'VF13ad', 'GmatFunction', 'MatlabFunction'}
    for row in catalog['entries']:
        status = row['status']
        if status in {'saved-lifecycle-clone-pass', 'saved-clone-self-reference-pass'}:
            require(row['case'] in saved_cases, 'Catalog row lacks a saved executed clone case')
        elif status == 'repaired-original-clone-cases-pass':
            require(row['native_type'] in repaired_types, 'User-body clone repair lacks a typed execution')
        else:
            require(status == 'direct-copy-witness-passed' and row['native_type'] in direct_types,
                    'Unexpected clone evidence route')

    formation_receipt = load(roots['formation'] / 'stage-receipt-v1.json')
    require(sha(formation_receipt['reader']) == formation_receipt['reader_sha256'],
            'Formation reader changed')
    spec = importlib.util.spec_from_file_location('final_formation_reader', formation_receipt['reader'])
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    formation = module.audit(roots['formation'])
    require(formation == formation_receipt['summary'] and formation['successful_GUI_cases'] == 4 and
            formation['independent_native_missions'] == 4 and
            formation['shared_copy_and_independent_member_edit'] and
            formation['exclusive_and_one_sided_sharing_rejected'], 'Formation policy witness incomplete')

    functions = json.loads(subprocess.check_output(
        [sys.executable, str(roots['functions'] / 'verify-stage-v1.py')], text=True))
    require(functions['status'] == 'passed' and functions['new_boundary_cases'] == 8 and
            functions['reused_single_output_cases'] == 4 and
            functions['reused_source_policy_cases'] == 12 and functions['native_tests_run'] == 0,
            'Function syntax/source policy evidence incomplete')
    policy = load(roots['functions'] / 'saved-policy-evidence-v1.json')
    syntax = next(row['verification'] for row in policy['cases'] if row['verification']['case'] == 'FunctionSyntax')
    source = next(row['verification'] for row in policy['cases'] if row['verification']['case'] == 'SourcePolicy')
    require(any(not row['strict'] and not row['brackets'] and row['accepted'] for row in syntax['observations']),
            'Native call compatibility missing')
    require(any(not row['strict'] and row.get('unicode_literal') and row['accepted'] for row in source['observations']),
            'Native Unicode compatibility missing')
    plan = load(roots['functions'] / 'frozen-plan-v3.json')
    require([row['name'] for row in plan['cases']] == ['zero', 'multiple', 'unbracketed_multiple', 'array_element'],
            'Output cardinality/array-element boundary missing')

    dependencies = load(index['function_dependencies'])
    require([row['parent']['key'] for row in dependencies['families']] == ['FRR!E450', 'FRR!E472'],
            'Function parents changed')
    for family in dependencies['families']:
        key = family['parent']['key']
        require([row['source']['key'] for row in family['children']] == scope['parent_dependencies'][key],
                'Function child obligations changed')
    protected = load(index['protected_check'])
    require(protected['status'] == 'passed' and protected['original_inputs'] == 1334 and
            protected['unchanged'] == 1333 and protected['original_assertions_changed'] == 0 and
            protected['authorized_display_title_delta'] == ['src/qtgui/tests/VisibleTestDisplay.py'],
            'Protected inputs changed')
    return {'status': 'passed', 'catalog_types': 33, 'new_clone_type_executions_reused': 9,
            'user_body_executions_reused': 16, 'formation_GUI_cases_reused': 4,
            'function_boundary_cases_reused': 8, 'single_output_cases_reused': 4,
            'source_policy_cases_reused': 12, 'component_artifacts_checked': artifact_count,
            'agreed_rows_completed': 5, 'additional_prerequisite_rows': 2,
            'native_tests_run': 0, 'source_versioned_only': True,
            'full_current_build_qualified': False, 'full_NASA_qualified': False}


def qualify(repo, path, inventory, admitted):
    qualification = load(path)
    require(qualification['contract_kind'] == KIND and qualification['qualified_keys'] == KEYS,
            'Different final qualification')
    for name, digest in qualification['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed qualification artifact ' + name)
    known = {row['key']: row for row in inventory['requirements']}
    require(not set(KEYS) & set(admitted), 'Duplicate final-row credit')
    require(qualification['candidate_rows'] == [known[key] for key in KEYS] and
            qualification['authority_sha256'] == inventory['sources'], 'Original authority changed')
    index = load(qualification['evidence_index'])
    scope = load(index['scope_review'])
    require([known[key]['fingerprint'] for key in scope['resource_catalog_reference']['rows']] ==
            [row['fingerprint'] for row in load(Path(index['component_stages']['clone']) /
                                               'complete-catalog-index-v1.json')['entries']],
            'Resource catalog fingerprints changed')
    require(not any(row.get('declared_id') == 'FRAC-4.5' for row in known.values()),
            'Resource cross-reference needs review')
    require([known[f'FRR!E{row}']['declared_id'] for row in [450,472,491]] ==
            ['FRR-37.1.0', 'FRR-37.2.0', 'FRR-37.3.0'], 'Function cross-reference needs review')
    satisfied = set(admitted)
    for key in KEYS:
        require(set(scope['parent_dependencies'].get(key, [])) <= satisfied,
                'An original parent obligation remains open for ' + key)
        satisfied.add(key)
    result = audit_index(index)
    return qualification, KEYS, {'status': 'passed', 'qualified_keys': KEYS,
            'whole_row_conjunction': True, 'source_fingerprints': {key: known[key]['fingerprint'] for key in KEYS},
            'conjunction': result, 'user_interpretations_applied': True,
            'source_versioned_only': True, 'native_tests_run': 0,
            'full_current_build_qualified': False, 'full_NASA_qualified': False}
