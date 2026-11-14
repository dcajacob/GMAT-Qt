"""Independent circular variational flow and observer-frame comparison."""
import math
from pathlib import Path
import json
from StmFrameOracle import multiply, expected


def circular_stm(mu, radius, time):
    n = math.sqrt(mu / radius**3); u = n*time; c, s = math.cos(u), math.sin(u)
    hill = [[4-3*c, 0, 0, s/n, 2*(1-c)/n, 0],
            [6*(s-u), 1, 0, -2*(1-c)/n, (4*s-3*u)/n, 0],
            [0, 0, c, 0, 0, s/n],
            [3*n*s, 0, 0, c, 2*s, 0],
            [-6*n*(1-c), 0, 0, -2*s, 4*c-3, 0],
            [0, 0, -n*s, 0, 0, c]]
    initial = [[float(i == j) for j in range(6)] for i in range(6)]
    initial[3][1] = n; initial[4][0] = -n
    rotation = [[c, -s, 0], [s, c, 0], [0, 0, 1]]
    omega = [[0, -n, 0], [n, 0, 0], [0, 0, 0]]
    derivative = multiply(rotation, omega)
    final = [[0.] * 6 for _ in range(6)]
    for i in range(3):
        for j in range(3):
            final[i][j] = final[i+3][j+3] = rotation[i][j]
            final[i+3][j] = derivative[i][j]
    return multiply(final, multiply(hill, initial))


def circle(mu, radius, time):
    n = math.sqrt(mu / radius**3); u = n*time
    return [radius*math.cos(u), radius*math.sin(u), 0., -radius*n*math.sin(u), radius*n*math.cos(u), 0.]


def audit(root, fixture):
    root = Path(root); p = json.loads(Path(fixture).read_text()); proof = json.loads((root/'proof.json').read_text())
    assert p['matrix_block_tolerances'] == [2e-8, 2e-6, 2e-11, 2e-8]
    t = proof['elapsed_seconds']; assert abs(t - p['duration_seconds']) <= 1e-6
    phi = circular_stm(p['mu'], p['radius'], t)
    n = math.sqrt(p['mu'] / p['reference_radius']**3)
    selected = expected(phi, math.pi/2, 0., n, 0., t)
    observations = []
    for k, name in enumerate(['OrbitSTM', 'OrbitSTMA', 'OrbitSTMB', 'OrbitSTMC', 'OrbitSTMD']):
        values = [float(v) for v in (root/(name+'.txt')).read_text().split()]
        size = 6 if k == 0 else 3; assert len(values) == size*size
        r, col = (0, 0) if k == 0 else (3*((k-1)//2), 3*((k-1)%2))
        actual = [values[i*size:(i+1)*size] for i in range(size)]
        errors = [[actual[i][j]-selected[i+r][j+col] for j in range(size)] for i in range(size)]
        assert all(math.isfinite(actual[i][j]) and abs(errors[i][j]) <= p['matrix_block_tolerances'][2*((i+r)//3)+(j+col)//3]
                   for i in range(size) for j in range(size)), name
        observations.append(dict(type=name, actual=actual, errors=errors, max_error=max(abs(v) for row in errors for v in row)))
    for row in observations[1:]:
        k = ['OrbitSTMA', 'OrbitSTMB', 'OrbitSTMC', 'OrbitSTMD'].index(row['type']); r, c = 3*(k//2), 3*(k%2)
        assert all(row['actual'][i][j] == observations[0]['actual'][i+r][j+c] for i in range(3) for j in range(3))
    for i in range(6):
        for j in range(6): assert abs(proof['internal_stm'][i][j] - phi[i][j]) <= p['matrix_block_tolerances'][2*(i//3)+j//3]
    for key, radius in [('sat_state', p['radius']), ('reference_state', p['reference_radius'])]:
        truth = circle(p['mu'], radius, t)
        assert all(math.isfinite(v) and abs(v-truth[i]) <= (p['position_tolerance_km'] if i < 3 else p['velocity_tolerance_km_s']) for i, v in enumerate(proof[key]))
    assert proof['reference_bound_to_executed_object'] and proof['native_missions'] == 1
    for key in p['required_rejections']:
        v = proof[key]; assert v['rejected'] and v['diagnostic'] and v['owner_unchanged']
    assert proof['reference_epoch_restored']
    return dict(status='passed', native_missions=1, matrix_values_checked=72, circular_states_checked=2,
                complete_partitions=4, rejections_checked=2, elapsed_seconds=t, observations=observations,
                max_selected_stm_error=max(v['max_error'] for v in observations),
                native_tests_run_by_replay=0, new_requirement_credit=0, whole_row_qualified=False)
