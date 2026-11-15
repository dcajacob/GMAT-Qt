"""Conjoin executed B1 format clauses and the already admitted SPK sibling.

Keep the independent short-arc accuracy failure failed. B1 section1.2 expressly
places orbit accuracy outside this format standard. No test expectation changes.
"""
from pathlib import Path
import hashlib
import json

from AuditOemFileInvariantEvidence import audit as invariant_audit

KEYS = ['FRR!E375', 'FRR!E374']
KIND = 'complete-source-versioned-CCSDS-B1-format-conjunction'
GATES = ['scientific_approval', 'technical_core_approval', 'full_current_build_qualified', 'full_NASA_qualified']
load = lambda p: json.loads(Path(p).read_text())
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(ok, message):
    if not ok: raise ValueError(message)


def qualify(repo, path, inventory, admitted):
    q = load(path); repo = Path(repo)
    need(q['contract_kind'] == KIND and q['qualified_keys'] == KEYS, 'Different format admission')
    need(q['authority_sha256'] == inventory['sources'] and all(q[k] is False for k in GATES), 'Authority or qualification changed')
    known = {r['key']: r for r in inventory['requirements']}
    need(q['candidate_rows'] == [known[k] for k in KEYS], 'Original format rows changed')
    need(known['FRR!E375']['fingerprint'] == '1df0dff106d6b263a5387c53cccfb3a0f958b290fb5889a9aaeed282db36d72b' and
         known['FRR!E374']['fingerprint'] == '63b8da3fd5f1ff6759038c7fe2577a7f6cf3fd6944ca04573425090cad04c964', 'Row fingerprints changed')
    need('FRR!E376' in admitted and q['parent_children'] == [known[k] for k in ['FRR!E375', 'FRR!E376']], 'Parent lacks an accepted original format child')
    need({'FRR!E380', 'FRR!E404'} <= set(admitted), 'Accepted time/interpolation prerequisites missing')
    for n, h in q['artifact_sha256'].items(): need(sha(n) == h, 'Changed format evidence: ' + n)
    for n in list(q['components'].values()) + [q[k] for k in ['scope_review', 'engineering_audit',
            'retained_accuracy_result', 'protected_check', 'reader_archive']]:
        need(n in q['artifact_sha256'], 'Unbound format obligation evidence: ' + n)
    standard = q['standard']
    need(standard['sha256'] == sha(standard['path']) == 'fab306a994e5bce9cad8c64d77af172a5819af38faf459faab1b7e2f6163223c', 'Pinned B1 standard changed')
    text = Path(standard['text']).read_text()
    need('Definition of the orbit accuracy underlying a particular orbit message is outside of the scope' in text and
         'All data blocks must contain a sufficient number of ephemeris data' in text and
         'The OEM shall be a text file consisting of orbit data for a single object.' in text, 'B1 scope/mandatory clauses lost')
    scope = load(q['scope_review'])
    need(scope['standard_orbit_accuracy_outside_scope'] and scope['all_mandatory_B1_format_clauses_conjoined'] and
         scope['original_accuracy_failure_preserved'] and scope['original_assertions_changed'] == 0 and
         scope['all_supported_frames_or_platforms_qualified'] is False and scope['unrelated_rows_admitted'] == [], 'Scope narrowed or overstated')
    need([c['id'] for c in scope['clauses']] == ['4.1', '4.2/Tables4-1..3', '4.3.1', '4.3.2', '4.3.3',
         '4.3.4', '4.3.5(a..e)', '4.3.5(f)', '4.3.6', '4.3.7'], 'Mandatory clause omitted')
    for c in scope['clauses']:
        need(c['complete'] and c['evidence'] and all(p in q['artifact_sha256'] for p in c['evidence']), 'Clause lacks executed evidence')
    for p in q['component_replays']:
        d = load(p)
        need(d['status'] == 'sealed-component-replayed;original-row-open' and d['native_tests_rerun'] == 0 and
             d['requirement_credit'] == 0 and p in q['artifact_sha256'], 'Earlier source-versioned evidence relabelled')
    components = {k: load(p) for k, p in q['components'].items()}
    need(components['text']['native_writer_cases'] == 36 and components['text']['writer_post_repair_case_passes'] == 72 and
         components['text']['native_GUI_passes'] == 26, 'Text/optional-field conjunction incomplete')
    need(components['frames']['native_GUI_contexts'] == 18 and components['frames']['unique_cases'] == 9 and
         components['frames']['unique_exported_states'] == 45 and components['frames']['retained_exported_states'] == 90 and
         components['frames']['max_position_error_km'] < 2e-5 and components['frames']['max_velocity_error_km_s'] < 2e-8,
         'Independent frame/state/unit observations incomplete')
    need(components['clocks']['post_repair_support_zones_passed'] == 3 and
         components['clocks']['post_repair_GUI_passed_both_profiles'], 'UTC creation incomplete')
    need(components['time']['required_systems'] == ['A1', 'TAI', 'UTC', 'TT', 'TDB'] and
         components['time']['native_writer_systems_passed'] == 5, 'Requested output-time paths incomplete')
    need(components['numbers']['status'] == 'passed' and components['numbers']['numeric_cases'] == 57 and
         components['numbers']['writer_rejections'] == 27 and components['numbers']['subscriber_rejections'] == 22,
         'Finite/locale/numeric coverage incomplete')
    engineering = invariant_audit(repo, Path(q['engineering_stage']))
    need(engineering == load(q['engineering_audit']) and engineering['after_cases_passed'] == 10 and
         engineering['affected_original_cases'] == 13, 'New invariants/affected original check incomplete')
    # Validate the retained failure as a failure. Its scientific bound is not
    # replaced by the producer's polynomial agreement or a format-only pass.
    failed = load(q['retained_accuracy_result'])
    need(failed['status'] == 'failed' and failed['returncode'] != 0, 'Original short-arc failure removed or inverted')
    need(q['original_accuracy_bounds'] == {'position_km': 2e-5, 'velocity_km_s': 2e-8} and
         scope['retained_accuracy_errors'] == {'position_km': 2.6942092880233304e-5, 'velocity_km_s': 2.9043781227255927e-8}, 'Original accuracy bounds/results changed')
    protected = load(q['protected_check'])
    need(protected['unchanged'] == 1334 and not protected['changed'], 'Original inputs changed')
    summary = dict(status='passed', qualified_keys=KEYS, whole_row_conjunction=True,
                   format_standard='CCSDS502.0-B-1', clause_groups=10, new_file_invariant_cases=10,
                   original_chronology_cases=13, original_inputs_unchanged=1334,
                   independent_state_unit_frame_contexts_reused=18, native_tests_run=0,
                   source_versioned_only=True, original_accuracy_failure_retained=True,
                   original_accuracy_assertions_changed=0, new_requirement_credit=2,
                   parent_original_children=['FRR!E375', 'FRR!E376'],
                   scientific_approval=False, technical_core_approval=False,
                   full_current_build_qualified=False, full_NASA_qualified=False)
    return q, KEYS, summary
