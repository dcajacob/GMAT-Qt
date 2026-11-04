"""Independent product-form interpolation, Rodrigues rotation and quadrature.

No GMAT imports or native converter samples. The shipped rotation table is a
shared, hashed model input; this checks its use, not astronomical accuracy.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path


def rotation(table, epoch, vector, inverse=False):
    assert table[0][0] <= epoch <= table[-1][0]
    # Ten adjacent daily nodes; direct product form rather than GMAT's
    # interpolator. Its midpoint convention is reproduced explicitly.
    lo, hi = 0, len(table) - 1
    while lo < hi - 1:
        mid = (lo + hi) // 2
        if epoch > table[mid][0]:
            lo = mid
        else:
            hi = mid
    first = max(0, mid - 4)
    last = min(len(table) - 1, first + 9)
    first = max(0, last - 9)
    nodes = table[first:last + 1]
    assert len(nodes) == 10
    weights = [math.prod((epoch - other[0]) / (node[0] - other[0])
                         for other in nodes if other is not node)
               for node in nodes]
    rv = [math.fsum(w * n[j + 1] for w, n in zip(weights, nodes))
          for j in range(3)]
    angle = math.sqrt(math.fsum(x * x for x in rv))
    assert angle > 0
    axis = [x / angle for x in rv]
    cross = [axis[1] * vector[2] - axis[2] * vector[1],
             axis[2] * vector[0] - axis[0] * vector[2],
             axis[0] * vector[1] - axis[1] * vector[0]]
    dot = math.fsum(a * b for a, b in zip(axis, vector))
    sign = -1 if inverse else 1
    return [math.cos(angle) * vector[j] + sign * math.sin(angle) * cross[j]
            + (1 - math.cos(angle)) * dot * axis[j] for j in range(3)]


def truth(fixture, spec, table, seconds, shutoff, subdivisions, omit_rotation=False):
    initial = fixture['initial_state']
    epoch = spec['epoch_a1mjd']
    def fk5(vector, t):
        return rotation(table, epoch + t / 86400, vector) if (
            spec['frame'] == 'ICRF' and not omit_rotation) else list(vector)
    if spec['kind'] == 'Impulse':
        dv = fk5(fixture['impulse'], 0)
        velocity = [initial[j + 3] + dv[j] for j in range(3)]
        return [initial[j] + seconds * velocity[j] for j in range(3)] + velocity
    norm = math.sqrt(sum(x*x for x in fixture['direction']))
    acceleration = [fixture['thrust_newtons'] / fixture['mass_kg'] / 1000 * x / norm
                    for x in fixture['direction']]
    end = min(seconds, shutoff)
    h = end / subdivisions
    dv, dr = [], []
    for j in range(3):
        a, r = [], []
        for i in range(subdivisions + 1):
            t = i*h
            weight = 1 if i in (0, subdivisions) else 4 if i % 2 else 2
            value = fk5(acceleration, t)[j]
            a.append(weight * value)
            r.append(weight * (seconds - t) * value)
        dv.append(h / 3 * math.fsum(a))
        dr.append(h / 3 * math.fsum(r))
    return [initial[j] + seconds * initial[j+3] + dr[j] for j in range(3)] + [
        initial[j+3] + dv[j] for j in range(3)]


def validate(fixture, spec, table, raw):
    lines = [list(map(float, line.split(','))) for line in raw.strip().splitlines()]
    assert len(lines) == 6 and lines[-1] == [42]
    rows = lines[:-1]
    assert all(len(row) == 18 and all(map(math.isfinite, row)) for row in rows)
    assert all(abs(row[1] - t) < 5e-5 for row, t in zip(rows, fixture['nominal_times_seconds']))
    metrics = dict(max_position_error_km=0., max_velocity_error_km_s=0.,
                   max_quadrature_refinement_error=0., wrong_frame_position_contrast_km=0.)
    observations = []
    # The Report parameter recovers ElapsedSecs from a rounded MJD. Use the
    # commanded integration intervals, independently checked against native
    # per-arc integrator clocks, for trajectory truth. Keep the reported-clock
    # check above separate from the unchanged sub-millimetre state bounds.
    times = fixture['nominal_times_seconds']
    failures = []
    for index, row in enumerate(rows):
        assert row[0] == index and row[3:5] == [100, fixture['mass_kg']] and row[-1] == 41
        assert abs(row[2] - (spec['epoch_a1mjd'] + row[1]/86400)) < 2e-10
        state = truth(fixture, spec, table, times[index], times[3], 256)
        refined = truth(fixture, spec, table, times[index], times[3], 512)
        refinement = max(abs(a-b) for a, b in zip(state, refined))
        assert refinement < 1e-10, ('quadrature not converged', refinement)
        metrics['max_quadrature_refinement_error'] = max(metrics['max_quadrature_refinement_error'], refinement)
        icrf = rotation(table, row[2], state[:3], True) + rotation(table, row[2], state[3:], True)
        wrong = truth(fixture, spec, table, times[index], times[3], 256, True)
        contrast = max(abs(a-b) for a, b in zip(state[:3], wrong[:3]))
        metrics['wrong_frame_position_contrast_km'] = max(metrics['wrong_frame_position_contrast_km'], contrast)
        errors = []
        for frame, expected, actual in [('FK5', state, row[5:11]), ('ICRF', icrf, row[11:17])]:
            for c, (a, b) in enumerate(zip(actual, expected)):
                error = abs(a-b)
                key = 'max_position_error_km' if c < 3 else 'max_velocity_error_km_s'
                bound = fixture['position_bound_km'] if c < 3 else fixture['velocity_bound_km_s']
                metrics[key] = max(metrics[key], error)
                errors.append(error)
                if error > bound:
                    failures.append(dict(row=index, frame=frame, component=c, actual=a, expected=b, error=error, bound=bound))
        observations.append(dict(row=index, seconds=row[1], expected_fk5=state, expected_icrf=icrf, errors=errors))
    if spec['frame'] == 'ICRF':
        assert metrics['wrong_frame_position_contrast_km'] > 20 * fixture['position_bound_km'], 'Fixture cannot distinguish missing conversion'
    result = dict(case=spec['key'], passed=not failures, metrics=metrics, failures=failures, observations=observations,
                  position_bound_km=fixture['position_bound_km'], velocity_bound_km_s=fixture['velocity_bound_km_s'],
                  model_limit=fixture['limits'])
    return result


def main():
    parser = argparse.ArgumentParser()
    for name in ['cases', 'key', 'table', 'report', 'output']:
        parser.add_argument('--'+name, required=True)
    args = parser.parse_args()
    fixture = json.loads(Path(args.cases).read_text())
    spec = next(case for case in fixture['cases'] if case['key'] == args.key)
    table_bytes = Path(args.table).read_bytes()
    table = [list(map(float, line.split(','))) for line in table_bytes.decode().splitlines() if line.strip()]
    assert all(len(row) == 4 for row in table) and all(a[0] < b[0] for a,b in zip(table,table[1:]))
    result = validate(fixture, spec, table, Path(args.report).read_text())
    result['table_sha256'] = hashlib.sha256(table_bytes).hexdigest()
    Path(args.output).write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(dict(case=args.key, passed=result['passed'], **result['metrics'])))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
