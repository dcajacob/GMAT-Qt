"""Independent function STM rejection, exact unwind, and corrected-frame recovery."""
import json
import math
from pathlib import Path
from StmMissionOracle import circle, circular_stm
from StmFunctionFrameOracle import compare_state, compare_matrix, selected, need, TYPES


def snapshot(value, plan, seconds):
    time = value['seconds']
    need(abs(time-seconds) <= 1e-6, 'Function unwind/recovery epoch differs')
    compare_state(value['state'], circle(plan['mu'], plan['radius'], time), plan)
    compare_matrix(value['stm'], circular_stm(plan['mu'], plan['radius'], time), plan['matrix_block_tolerances'])


def reads(values, truth, plan):
    need([v['type'] for v in values] == TYPES, 'Recovery readonly Parameter inventory differs')
    for k, v in enumerate(values):
        size = 6 if k == 0 else 3
        r, c = (0, 0) if k == 0 else (3*((k-1)//2), 3*((k-1)%2))
        actual = v['actual']
        need(v['readonly'] and len(actual) == size and all(len(row) == size for row in actual), 'Recovery Parameter readonly/shape differs')
        need(all(math.isfinite(actual[i][j]) and abs(actual[i][j]-truth[i+r][j+c]) <=
                 plan['matrix_block_tolerances'][2*((i+r)//3)+(j+c)//3]
                 for i in range(size) for j in range(size)), 'Recovery Parameter differs from independent STM')


def audit(root, fixture):
    root = Path(root); plan = json.loads(Path(fixture).read_text()); proof = json.loads((root/'proof.json').read_text())
    need(plan['expected_failure_status'] == -5 and plan['matrix_block_tolerances'] == [2e-8, 2e-6, 2e-11, 2e-8] and
         plan['position_tolerance_km'] == 2e-5 and plan['velocity_tolerance_km_s'] == 2e-8, 'Frozen recovery expectation/bounds differ')
    need(proof['status'] == 'passed' and proof['case'] == plan['key'] and proof['native_missions'] == 2 and
         proof['nested_calls'] == 2 and proof.get('native_worker_source_writes', proof.get('production_changes')) == 0,
         'Recovery run scope differs')
    phases = proof['phases']
    need([p['phase'] for p in phases] == ['failure', 'recovery'], 'Failure/recovery census differs')
    for n, phase in enumerate(phases):
        tag = phase['phase']; failure = n == 0
        need(phase['run_status'] == (-5 if failure else 1) and phase['caller_unchanged'] and phase['prototype_unchanged'] and
             phase['caller_before'] == phase['caller_after'], 'Native error/success status or caller unwind differs')
        local = phase['local']
        need(local['private_inputs'] and local['self_origin'] == failure, 'Local frame origin/scope differs')
        for label in ['caller_before', 'caller_after']:
            caller = phase[label]
            snapshot(caller['craft'], plan, 60.)
            guide = dict(plan, radius=plan['reference_radius'])
            snapshot(caller['guide'], guide, 60.)
            reads(caller['reads'], selected(plan, caller['craft']['seconds']), plan)
        snapshot(local['craft'], plan, 90.)
        snapshot(local['guide'], dict(plan, radius=plan['reference_radius']), 90.)
        need([v['type'] for v in local['reads']] == TYPES, 'Local recovery native Parameter inventory differs')
        for value in local['reads']:
            need(value['owner_bound'] and value['frame_bound'] and value['readonly'] and value['owner_unchanged'] and
                 value['guide_unchanged'], 'Local rejection/recovery binding or owner mutation differs')
            need(value['rejected'] == failure, 'Defined/undefined native STM route was misclassified')
            if failure:
                need(plan['diagnostic_fragment'] in value['diagnostic'] and value['cache_unchanged'] and not value['actual'],
                     'Undefined STM rejection lost its diagnostic/cache atomicity')
        text = (root/(tag+'-messages.txt')).read_text()
        need((plan['diagnostic_fragment'] in text) == failure, 'Real engine Report exception diagnostic differs')
        need(phase['keep'] == (41 if failure else 42), 'Failure leaked post-call execution or recovery did not continue')
        if failure:
            need(phase['result'] == -1, 'Failed nested function partially changed its output')
            for prefix in ['Function', 'Root']:
                for k in range(5):
                    path = root/(tag+'-'+prefix+str(k)+'.txt')
                    need(not path.exists() or not path.read_bytes().strip(), 'Rejected function Report emitted partial data')
        else:
            local_time = local['craft']['seconds']
            truth = circular_stm(plan['mu'], plan['radius'], local_time)
            reads(local['reads'], truth, plan)
            need(abs(phase['result']-circle(plan['mu'], plan['radius'], local_time)[0]) <= plan['position_tolerance_km'],
                 'Recovered scalar output differs from local trajectory')
            for prefix, full in [('Function', truth), ('Root', selected(plan, phase['caller_after']['craft']['seconds']))]:
                raw = []
                for k in range(5):
                    size = 6 if k == 0 else 3
                    r, c = (0, 0) if k == 0 else (3*((k-1)//2), 3*((k-1)%2))
                    values = [float(v) for v in (root/(tag+'-'+prefix+str(k)+'.txt')).read_text().split()]
                    need(len(values) == size*size and all(math.isfinite(v) and abs(v-full[i//size+r][i%size+c]) <=
                         plan['matrix_block_tolerances'][2*((i//size+r)//3)+(i%size+c)//3] for i, v in enumerate(values)),
                         'Recovered native function/root Report differs from independent STM')
                    raw.append(values)
                for k in range(1, 5):
                    r, c = 3*((k-1)//2), 3*((k-1)%2)
                    need(all(raw[k][3*i+j] == raw[0][6*(i+r)+j+c] for i in range(3) for j in range(3)), 'Recovered Report partitions disagree')
    failure_body, recovery_body = (root/'failure.gmf').read_text(), (root/'recovery.gmf').read_text()
    need(failure_body.replace('CheckFrame.Origin = craft;', 'CheckFrame.Origin = Earth;') == recovery_body,
         'Recovery changed unrelated function behavior')
    return dict(status='passed', native_missions=2, expected_native_run_statuses=[-5, 1], nested_calls=2,
                native_parameter_reads=30, native_rejection_checks=5, rejected_native_report_commands=1,
                reported_matrix_entries=144, caller_restoration_exact=True, rejection_atomic=True,
                recovery_passed=True, native_tests_run_by_replay=0, new_requirement_credit=0, whole_row_qualified=False)
