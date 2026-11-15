"""Independent circular phase/variational/frame truth for every solver trial."""
import json
import math
from pathlib import Path
from StmFrameOracle import multiply, expected
from StmMissionOracle import circle, circular_stm
from StmFunctionFrameOracle import compare_matrix, compare_state, need, TYPES


def turn(angle):
    c, s = math.cos(angle), math.sin(angle)
    r = [[c, -s, 0.], [s, c, 0.], [0., 0., 1.]]
    result = [[0.]*6 for _ in range(6)]
    for i in range(3):
        for j in range(3):
            result[i][j] = result[i+3][j+3] = r[i][j]
    return result


def native(plan, time, angle):
    return multiply(turn(angle), multiply(circular_stm(plan['mu'], plan['radius'], time), turn(-angle)))


def framed(plan, time, angle):
    frequency = math.sqrt(plan['mu']/plan['reference_radius']**3)
    return expected(native(plan, time, angle), math.pi/2, 0., frequency, 0., time)


def orbit(plan, time, angle):
    value = circle(plan['mu'], plan['radius'], time)
    return [sum(a*b for a, b in zip(row, value)) for row in turn(angle)]


def audit(root, fixture):
    root = Path(root); plan = json.loads(Path(fixture).read_text()); proof = json.loads((root/'proof.json').read_text())
    need(plan['matrix_block_tolerances'] == [2e-8, 2e-6, 2e-11, 2e-8] and
         plan['position_tolerance_km'] == 2e-5 and plan['velocity_tolerance_km_s'] == 2e-8 and
         plan['phase_tolerance'] == 1e-9 and plan['duration_seconds'] == 60., 'Frozen solver bounds differ')
    need(proof['status'] == 'passed' and proof['case'] == plan['key'] and proof['branch'] == plan['branch'] and
         proof['solver_type'] == plan['solver_type'] and proof['converged'] and proof['solver_status'] == 1 and
         proof['keep'] == 42 and proof['prototype_unchanged'] and proof['native_missions'] == 1,
         'Native solver status/prototype/continuation differs')
    samples = proof['samples']
    need(len(samples) >= 6 and len(samples)%2 == 0 and [v['phase'] for v in samples] ==
         ['before', 'after']*(len(samples)//2), 'Missing paired solver trial samples')
    after = samples[1::2]
    need(len({round(v['angle'], 12) for v in after}) >= 3, 'Actual solver did not exercise multiple controls')
    largest, reads = 0., 0
    identity = [[float(i == j) for j in range(6)] for i in range(6)]
    for before, row in zip(samples[::2], after):
        angle, time = row['angle'], row['seconds']
        need(before['angle'] == angle and abs(before['seconds']) <= 1e-9 and abs(before['reference_seconds']) <= 1e-9 and
             abs(time-60.) <= 1e-6 and abs(row['reference_seconds']-time) <= 1e-9 and
             before['frame_rebound'] and row['frame_rebound'], 'Trial input restoration/clock/frame binding differs')
        compare_state(before['state'], orbit(plan, 0., angle), plan)
        compare_state(before['reference_state'], circle(plan['mu'], plan['reference_radius'], 0.), plan)
        compare_matrix(before['native_stm'], identity, [1e-12]*4)
        compare_state(row['state'], orbit(plan, time, angle), plan)
        compare_state(row['reference_state'], circle(plan['mu'], plan['reference_radius'], time), plan)
        largest = max(largest, compare_matrix(row['native_stm'], native(plan, time, angle), plan['matrix_block_tolerances']))
        truth = framed(plan, time, angle)
        need([v['type'] for v in row['reads']] == TYPES, 'Native selected-frame Parameter route census differs')
        for k, value in enumerate(row['reads']):
            size = 6 if k == 0 else 3
            r, c = (0, 0) if k == 0 else (3*((k-1)//2), 3*((k-1)%2))
            actual = value['actual']
            need(value['owner_bound'] and value['frame_bound'] and value['readonly'] and len(actual) == size and
                 all(len(v) == size for v in actual), 'Native Parameter binding/readonly/shape differs')
            need(all(math.isfinite(actual[i][j]) and abs(actual[i][j]-truth[i+r][j+c]) <=
                     plan['matrix_block_tolerances'][2*((i+r)//3)+(j+c)//3]
                     for i in range(size) for j in range(size)), 'Selected-frame trial STM differs from independent two-end Jacobians')
            reads += 1
    final_angle, final_time = proof['control'], proof['seconds']
    need(abs(final_angle-plan['solution_phase']) <= 1e-9 and abs(proof['solver_control']-final_angle) <= 1e-12 and
         abs(final_time-60.) <= 1e-6 and abs(proof['reference_seconds']-final_time) <= 1e-9, 'Final solver control/clock differs')
    compare_state(proof['state'], orbit(plan, final_time, plan['solution_phase']), plan)
    compare_state(proof['reference_state'], circle(plan['mu'], plan['reference_radius'], final_time), plan)
    largest = max(largest, compare_matrix(proof['native_stm'], native(plan, final_time, final_angle), plan['matrix_block_tolerances']))
    need(abs(proof['state'][0]-plan['target_x']) <= plan['position_tolerance_km'], 'Final trajectory did not reach independent phase goal')
    values_checked = 0
    for prefix, rows in [('Trial', after), ('Final', [dict(seconds=final_time, angle=final_angle)])]:
        reports = []
        for k in range(5):
            size = 6 if k == 0 else 3
            r, c = (0, 0) if k == 0 else (3*((k-1)//2), 3*((k-1)%2))
            values = [float(v) for v in (root/(prefix+str(k)+'.txt')).read_text().split()]
            need(len(values) == len(rows)*size*size, 'Actual Report trial/component count differs: '+prefix+str(k))
            for n, row in enumerate(rows):
                truth = framed(plan, row['seconds'], row['angle'])
                need(all(math.isfinite(v) and abs(v-truth[i//size+r][i%size+c]) <=
                         plan['matrix_block_tolerances'][2*((i//size+r)//3)+(i%size+c)//3]
                         for i, v in enumerate(values[n*size*size:(n+1)*size*size])), 'Actual solver Report differs from independent trial STM')
            reports.append(values); values_checked += len(values)
        for k in range(1, 5):
            r, c = 3*((k-1)//2), 3*((k-1)%2)
            need(all(reports[k][n*9+3*i+j] == reports[0][n*36+6*(i+r)+j+c]
                     for n in range(len(rows)) for i in range(3) for j in range(3)), 'Actual solver Report partition consistency differs')
    return dict(status='passed', case=plan['key'], native_missions=1, solver_trials=len(after),
                native_parameter_reads=reads, reported_matrix_entries=values_checked,
                trial_restoration=True, final_reference_preserved=True, max_internal_stm_error=largest,
                native_tests_run_by_replay=0, new_requirement_credit=0, whole_row_qualified=False)
