"""Conjoin the registered command families under the approved native-context rule.

Availability is distinguished from all numerical domains, argument combinations,
solver modes, provider releases, and whole-current-build qualification.
"""
from pathlib import Path
import hashlib
import json
import math

from AuditCommandContextGaps import audit as audit_gaps

KEYS = ['FRC!E7', 'FRC!E55', 'FRR!E477']
FINGERPRINTS = ['dbd82e1be48866f3487ef30cd454ef3b7e920ea1f76833e3f2130b11e21826d5',
                '0d4beb6ae1b0657d77f6f1925a9d9230786e76bd63700f5c069b514077d687bb',
                '8951d4eec5d385f9706e97cbbad214b9770c10ac7806712ecec2c9095ce393fd']
load = lambda p: json.loads(Path(p).read_text())
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(value, message):
    if not value:
        raise ValueError(message)


def near(actual, expected, bounds):
    need(len(actual) == len(expected) == len(bounds), 'Numeric witness shape changed')
    need(all(math.isfinite(a) and math.isfinite(b) and t > 0 and abs(a-b) <= t
             for a, b, t in zip(actual, expected, bounds)), 'Saved independent numerical oracle failed')


def proof(record):
    if record['proof']:
        return load(record['proof'])
    lines = (Path(record['run'])/'test.log').read_text().splitlines()
    values = [json.loads(line.split('NASA_CHECK ', 1)[1]) for line in lines if 'NASA_CHECK ' in line]
    need(len(values) == 1, 'No unique executed assertion receipt')
    return values[0]


def audit_map(mapping):
    need(mapping['factory_count'] == len(mapping['factory_identifiers']) == 54, 'Factory universe changed')
    need(len(set(mapping['factory_identifiers'])) == 54, 'Duplicate factory identifier')
    cells = mapping['context_cells']
    expected = {(command, context) for command in mapping['factory_identifiers'] for context in ['Target', 'Optimize', 'Function']}
    need(len(cells) == 162 and {(v['command'], v['context']) for v in cells} == expected, 'Missing command/context conjunction')
    records = mapping['records']
    need(all(v['evidence'] in records and v['role'] for v in cells), 'Unwitnessed context cell')
    census = proof(records['Target/NoOp'])['factory_types']
    need(set(census) == set(mapping['factory_identifiers']) and len(census) == 54, 'Recorded runtime command census differs')
    numeric = 0
    for name, record in records.items():
        result = load(record['result'])
        p = proof(record)
        need(result['returncode'] == 0, 'Nonpassing original native process')
        if record['kind'] == 'gap':
            need(p['passed'], 'Missing targeted native pass')
        else:
            need(result['status'] == 'passed' and (p.get('status') == 'passed' or p.get('outcome') == 'passed'),
                 'Original assertion receipt did not pass')
        if isinstance(p.get('expected'), list) and isinstance(p.get('observed'), list):
            bounds = p.get('column_tolerances')
            if not bounds and p.get('outer_context') in ['Target', 'Optimize']:
                # The original enclosing worker freezes these exact bounds.
                bounds = [1e-4] + [2e-5]*11
                need(p['solver_mode'] == 'RunInitialGuess', 'Original solver mode relabelled')
                need(len(p['nominal_and_final_reports']) == 2, 'Missing original nominal/final reports')
                for row in p['nominal_and_final_reports']:
                    near(row, p['expected'], bounds)
            if bounds:
                observed = p['observed']
                if observed and isinstance(observed[0], list):
                    need(len(observed) == 1, 'Unexpected backend report rows')
                    observed = observed[0]
                near(observed, p['expected'], bounds)
                numeric += 1
        if p.get('family') == 'Toggle' and p.get('outer_context'):
            need(p['effects']['trace_active'] is False, 'Toggle effect missing')
        if p.get('family') == 'SaveMissionFollowedInside':
            need(p['effects']['mission_saved'] and p['effects']['saved_host_rebuilt_and_replayed'], 'SaveMission export/replay missing')
        if p.get('family') == 'Save' and p.get('outer_context'):
            near(p['effects']['saved_cartesian'], p['expected'][1:7], [2e-5]*6)
        run = Path(record['run'])
        if 'SolverParent' in name:
            need(p['missions'] == 2 and p['position'] == 'Append', 'Missing positive/negative solver placement')
            kind = p['child_type']
            values = load(run/'native-results.json')
            need(len(values) == 2, 'Solver parent phase missing')
            for phase in values:
                goal = 5 if phase['phase'] == 'positive' else -5
                child = 3 if kind in ['Target', 'Optimize'] else 0
                count = 6 if kind in ['For', 'While'] else (7 if goal > 0 else -11) if kind == 'IfElse' else 0
                wanted = [goal, child, count, 3 if kind in ['For', 'While'] else 0, 42, 21545, 7000, 0, 0, 0, 7.5, 1]
                need(phase['completed'] and phase['expected'] == wanted, 'Solver branch semantics changed')
                near(phase['values'], wanted, [5e-6]*2 + [1e-9]*10)
                raw = [float(v) for v in (run/(phase['phase']+'-values.csv')).read_text().strip().split(',')]
                near(raw, wanted, [5e-6]*2 + [1e-9]*10)
            numeric += 1
        if 'SolverContext' in name:
            target = p['case'] == 'Target'
            need(p['rejected_actions'] >= (4 if target else 6), 'Illegal direct solver placements untested')
            near(p['actual'], [4, -2, 9, 0, 41, 1] if target else [4, 0, 0, 4, 41, 1], [1e-7]*6)
            numeric += 1
        if 'FunctionControlRecursion' in name:
            need(p['max_numeric_error'] == 0, 'Function recurrence failed')
            need(p['reports'][0]['actual'] == [0, 1, 1, 1, 1, 3, 5, 120, 9], 'Independent factorial/call counts changed')
        if 'FunctionControlSubscribers' in name:
            need(p['max_numeric_error'] == 0 and p['reports'], 'Function loop/subscriber recurrence failed')
        if 'FunctionBuiltinQuaternionProduct' in name:
            wanted = [.5, .5, .5, .5, .5, .5, -.5, .5, 0, 0, -math.sqrt(3)/2, .5]
            need(p['missions'] == 2 and p['function_calls'] == 4, 'Builtin function execution incomplete')
            for phase in p['runs']:
                near(phase['first'], wanted, [1e-12]*12)
                near(phase['second'], wanted, [1e-12]*12)
                need(phase['private_name_absent'] and phase['configured_prototypes_unchanged'] and phase['continuation'] == 42, 'Function scope changed')
            numeric += 1
        if name.endswith('MatlabCallObjectInOut'):
            need(len(p['phases']) == 3, 'MATLAB function repeat/reopen missing')
            for phase in p['phases']:
                need(phase['inputs'] == [15, 30, 45, 8] and phase['outputs'] == [12, 33, 48, 11]
                     and phase['function_calls'] == 3 and phase['matlab_calls'] == 12 and phase['viewer_matches'], 'Typed MATLAB call result changed')
            numeric += 1
        if name.endswith('FunctionCommandPolicySaveMission'):
            selected = [v for v in p['observations'] if v.get('context') == 'Function']
            need(any(v['policy'] == 'Immediate' and v['exported'] and v['export_replayed'] for v in selected), 'Function export effect not proved')
            need(any(v['policy'] == 'LegacyTerminal' and not v['exported'] for v in selected), 'Native compatibility changed')
        if 'BurnParameterBinding' in name or 'FiniteBurnBinding' in name:
            need(p['function_calls'] == 12 and p['actual_parameter_owner_checks'] > 0 and len(p['phases']) == 3, 'Burn command scope incomplete')
            for phase in p['phases']:
                for sample in phase['live_samples']:
                    for value in sample['parameters']:
                        if isinstance(value.get('expected'), (int, float)):
                            near([value['actual']], [value['expected']], [value['absolute_bound']])
        if name.startswith('marker/'):
            need(p['rejections'], 'Main-sequence marker rejection absent')
    need(numeric >= 50, 'Missing independent numerical/effect family witnesses')
    # All solver/body classes are covered by actual assertions; aliases and
    # structural pairs refer to those same executed implementations explicitly.
    return dict(status='passed', command_identifiers=54, context_cells=162,
                executed_witness_records=len(records), independently_rechecked_numeric_records=numeric)


def qualify(repo, path, inventory, admitted):
    q = load(path)
    need(q['contract_kind'] == 'complete-native-legal-command-context-conjunction' and q['qualified_keys'] == KEYS, 'Wrong admission scope')
    need(q['authority_sha256'] == inventory['sources'], 'Workbook authority changed')
    known = {v['key']: v for v in inventory['requirements']}
    need(q['candidate_rows'] == [known[k] for k in KEYS], 'Original rows changed')
    need([known[k]['fingerprint'] for k in KEYS] == FINGERPRINTS, 'Row fingerprints changed')
    for name, digest in q['artifact_sha256'].items():
        need(sha(name) == digest, 'Changed immutable command evidence: '+name)
    decision = load(q['scope_decision'])
    need(decision['answer'] == 'Yes—preserve GMAT’s normal context rules' and set(KEYS) <= set(decision['applies_to']), 'Native context scope was not authorized')
    mapping = load(q['matrix'])
    factory = Path(q['factory_source']).read_text()
    need('ofType == "GMAT" || ofType == "Equation" || ofType == "Assignment"' in factory,
         'Assignment alias contract differs')
    for name, digest in mapping['artifact_sha256'].items():
        need(name in q['artifact_sha256'] and q['artifact_sha256'][name] == digest, 'Unbound command witness')
    result = audit_map(mapping)
    gaps = audit_gaps(Path(q['gap_stage']))
    need(gaps == load(Path(q['gap_stage'])/'evidence-audit-v1.json') and gaps['targeted_cases'] == 9, 'Missing targeted gaps or recovery witnesses')
    protection = load(q['protected_check'])
    need(protection['count'] == 1334 and protection['changed'] == [], 'Original tests changed')
    need(q['native_tests_rerun_for_admission'] == 0 and q['all_solver_modes_claimed'] is False and q['all_argument_domains_claimed'] is False, 'Qualification overstates its scope')
    need(all(q[k] is False for k in ['scientific_approval', 'technical_core_approval', 'full_current_build_qualified', 'full_NASA_qualified']), 'Unjustified qualification gate')
    result.update(qualified_keys=KEYS, new_requirement_credit=3, whole_row_conjunction=True,
                  source_versioned_only=True, normal_context_rules_preserved=True,
                  original_inputs_unchanged=1334, native_tests_run=0,
                  all_solver_modes_claimed=False, all_argument_domains_claimed=False,
                  scientific_approval=False, technical_core_approval=False,
                  full_current_build_qualified=False, full_NASA_qualified=False)
    return q, KEYS, result
