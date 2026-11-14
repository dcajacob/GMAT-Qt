"""Independent circular STM/frame truth for repeated by-value function calls."""
import json
import math
from pathlib import Path
from StmFrameOracle import expected
from StmMissionOracle import circular_stm, circle

TYPES = ['OrbitSTM', 'OrbitSTMA', 'OrbitSTMB', 'OrbitSTMC', 'OrbitSTMD']


def need(ok, message):
    if not ok:
        raise ValueError(message)


def compare_matrix(actual, truth, tolerances):
    need(len(actual) == len(truth) == 6 and all(len(v) == 6 for v in actual), 'Full matrix shape differs')
    errors = [abs(actual[i][j] - truth[i][j]) for i in range(6) for j in range(6)]
    need(all(math.isfinite(e) and e <= tolerances[2*(i//3)+j//3] for (i, j), e in
             zip(((i, j) for i in range(6) for j in range(6)), errors)), 'Full matrix differs from circular/two-end oracle')
    return max(errors)


def selected(plan, time):
    phi = circular_stm(plan['mu'], plan['radius'], time)
    frequency = math.sqrt(plan['mu'] / plan['reference_radius']**3)
    return expected(phi, math.pi/2, 0., frequency, 0., time)


def compare_state(actual, truth, plan):
    need(len(actual) == 6 and all(math.isfinite(v) and abs(v-truth[i]) <=
         (plan['position_tolerance_km'] if i < 3 else plan['velocity_tolerance_km_s'])
         for i, v in enumerate(actual)), 'Spacecraft state differs from independent circle')


def report(root, name, type_index, truths, plan):
    values = [float(v) for v in (root/(name+'.txt')).read_text().split()]
    size = 6 if type_index == 0 else 3
    r, c = (0, 0) if type_index == 0 else (3*((type_index-1)//2), 3*((type_index-1)%2))
    need(len(values) == len(truths)*size*size, 'Actual Report row/component census differs: ' + name)
    for n, truth in enumerate(truths):
        actual = values[n*size*size:(n+1)*size*size]
        need(all(math.isfinite(v) and abs(v-truth[i//size+r][i%size+c]) <=
                 plan['matrix_block_tolerances'][2*((i//size+r)//3)+(i%size+c)//3]
                 for i, v in enumerate(actual)), 'Actual function/caller Report differs from independent STM: ' + name)
    return values


def audit(root, fixture):
    root = Path(root); plan = json.loads(Path(fixture).read_text()); proof = json.loads((root/'proof.json').read_text())
    need(plan['matrix_block_tolerances'] == [2e-8, 2e-6, 2e-11, 2e-8] and
         plan['position_tolerance_km'] == 2e-5 and plan['velocity_tolerance_km_s'] == 2e-8,
         'Frozen numerical bounds changed')
    need(proof['status'] == 'passed' and proof['case'] == plan['key'] and proof['frame_mode'] == plan['frame_mode'] and
         proof['calls'] == 3 and proof['keep'] == 42 and proof['configured_prototypes_unchanged'] and
         proof['native_missions'] == 1 and proof['function_calls'] == 3 and proof['native_propagations'] == 4,
         'Function census, prototypes or continuation differs')
    rows = proof['samples']
    need([(v['call'], v['phase']) for v in rows] == [(i, p) for i in range(1, 4) for p in ['before', 'after']],
         'Repeated/nested function sample census differs')
    max_error, reads = 0., 0
    times = {'before': [], 'after': []}
    for row in rows:
        time = row['seconds']; nominal = 60. if row['phase'] == 'before' else 90.
        need(abs(time-nominal) <= 1e-6 and abs(row['guide_seconds']-time) <= 1e-9 and
             row['private_inputs'] and row['references_rebound'] and row['guide_pointer'] == row['frame_secondary_pointer'] and
             row['craft_pointer'] != row['root_craft_pointer'] and row['guide_pointer'] != row['root_guide_pointer'] and
             row['function'] == 'InspectStm', 'Live frame/input scope or endpoint differs')
        compare_state(row['state'], circle(plan['mu'], plan['radius'], time), plan)
        compare_state(row['guide_state'], circle(plan['mu'], plan['reference_radius'], time), plan)
        max_error = max(max_error, compare_matrix(row['native_stm'], circular_stm(plan['mu'], plan['radius'], time),
                                                plan['matrix_block_tolerances']))
        truth = selected(plan, time)
        need([v['type'] for v in row['reads']] == TYPES, 'Native function Parameter route inventory differs')
        for k, value in enumerate(row['reads']):
            size = 6 if k == 0 else 3
            r, c = (0, 0) if k == 0 else (3*((k-1)//2), 3*((k-1)%2))
            actual = value['actual']
            need(value['owner_bound'] and value['frame_bound'] and value['readonly'] and len(actual) == size and
                 all(len(v) == size for v in actual), 'Native function Parameter binding/shape differs')
            need(all(math.isfinite(actual[i][j]) and abs(actual[i][j]-truth[i+r][j+c]) <=
                     plan['matrix_block_tolerances'][2*((i+r)//3)+(j+c)//3]
                     for i in range(size) for j in range(size)), 'Native function frame STM differs from two-end oracle')
            reads += 1
        times[row['phase']].append(time)
    root_time = proof['caller_seconds']
    need(abs(root_time-60.) <= 1e-6, 'By-value function mutated caller epoch')
    compare_state(proof['caller_state'], circle(plan['mu'], plan['radius'], root_time), plan)
    compare_state(proof['caller_reference_state'], circle(plan['mu'], plan['reference_radius'], root_time), plan)
    max_error = max(max_error, compare_matrix(proof['caller_stm'], circular_stm(plan['mu'], plan['radius'], root_time),
                                            plan['matrix_block_tolerances']))
    need(len(proof['marks']) == 3 and all(abs(value-circle(plan['mu'], plan['radius'], times['after'][i])[0]) <=
         plan['position_tolerance_km'] for i, value in enumerate(proof['marks'])), 'Function scalar return differs from local trajectory')
    reports, values = {}, 0
    for prefix, ts in [('Before', times['before']), ('After', times['after']), ('Root', [root_time])]:
        for k in range(5):
            actual = report(root, prefix+str(k), k, [selected(plan, t) for t in ts], plan)
            reports[prefix+str(k)] = actual; values += len(actual)
        full = reports[prefix+'0']
        for k in range(1, 5):
            r, c = 3*((k-1)//2), 3*((k-1)%2)
            need(all(reports[prefix+str(k)][n*9+3*i+j] == full[n*36+6*(i+r)+j+c]
                     for n in range(len(ts)) for i in range(3) for j in range(3)), 'Actual Report partitions disagree with full STM')
    return dict(status='passed', case=plan['key'], native_missions=1, function_calls=3, nested_function_calls=1,
                native_parameter_reads=reads, live_function_samples=6, reported_matrix_entries=values,
                private_input_restoration=True, frame_rebinding=True, max_internal_stm_error=max_error,
                native_tests_run_by_replay=0, new_requirement_credit=0, whole_row_qualified=False)
