"""Independent synthetic J2/J3/J4 variational flow and Cartesian frame differentials.

The native field poses are shared geometric inputs. The higher-zonal force, complex-step
spatial differential, RK4 propagation and Decimal frame map are independent of
the native gravity and STM implementations.
"""
import cmath
from decimal import localcontext
import json
import math
from pathlib import Path

from StmOwnerFrameOracle import D, TYPES, PROVIDERS, dot, add, sub, scale, norm, unit, cross, mul, inverse


def need(ok, message):
    if not ok:
        raise ValueError(message)


def pole_at(poses, time):
    for left, right in zip(poses, poses[1:]):
        if time <= right['seconds'] + 1e-9:
            ratio = (time - left['seconds']) / (right['seconds'] - left['seconds'])
            value = [(1-ratio)*a+ratio*b for a, b in
                     zip(left['input_to_fixed'][2], right['input_to_fixed'][2])]
            length = math.sqrt(sum(v*v for v in value))
            return [v/length for v in value]
    raise ValueError('Independent higher-zonal field pose is unavailable')


def acceleration(plan, state, pole, decimal=False):
    # Gradient of U = mu/r * (1 - sum(Jn*(R/r)**n*Pn(h/r))).
    # Expanded P2, P3 and P4 give Cartesian forces independent of native Pines
    # recursion. Decimal differentiation below also uses this full potential.
    number = D if decimal else float
    x = state[:3]
    mu, radius = number(plan['mu']), number(plan['potential_radius_km'])
    j2, j3, j4 = [number(plan[k]) for k in ('j2', 'j3', 'j4')]
    r2 = sum(v*v for v in x)
    r = r2.sqrt() if decimal else cmath.sqrt(r2)
    h = sum(v*n for v, n in zip(x, pole))
    k2 = number('1.5')*mu*j2*radius**2/r**5
    k3 = mu*j3*radius**3/(2*r**7)
    k4 = mu*j4*radius**4/(8*r**7)
    return [-mu*v/r**3 + k2*((5*h*h/r2-1)*v-2*h*n)
            + k3*((35*h**3/r2-15*h)*v+(3*r2-15*h*h)*n)
            + k4*((315*h**4/r2**2-210*h*h/r2+15)*v+(-140*h**3/r2+60*h)*n)
            for v, n in zip(x, pole)]


def integrate(plan, poses, radius, time, step):
    # Initial conditions are frozen in the native mission, independent of higher-zonal.
    value = [radius, 0., 0., 0., math.sqrt(plan['mu']/radius), 0.]
    value += [float(i == j) for i in range(6) for j in range(6)]
    count = round(time/step)
    need(count > 0 and abs(count*step-time) < 1e-6, 'Independent RK4 endpoint grid differs')
    step = time/count

    def derivative(t, y):
        pole = pole_at(poses, t)
        force = acceleration(plan, y, pole)
        gradient = [[0.]*3 for _ in range(3)]
        for col in range(3):
            trial = list(y[:6]); trial[col] += 1e-30j
            perturbed = acceleration(plan, trial, pole)
            for row in range(3):
                gradient[row][col] = perturbed[row].imag/1e-30
        phi = y[6:]
        return y[3:6] + [v.real for v in force] + phi[18:36] + [
            sum(gradient[row][k]*phi[6*k+col] for k in range(3))
            for row in range(3) for col in range(6)]

    def moved(y, rate, amount):
        return [v+amount*d for v, d in zip(y, rate)]

    for k in range(count):
        t = k*step
        a = derivative(t, value)
        b = derivative(t+step/2, moved(value, a, step/2))
        c = derivative(t+step/2, moved(value, b, step/2))
        d = derivative(t+step, moved(value, c, step))
        value = [v+step*(aa+2*bb+2*cc+dd)/6 for v, aa, bb, cc, dd in zip(value, a, b, c, d)]
    return value[:6], [value[6+6*i:12+6*i] for i in range(6)]


def frame_value(plan, spec, owner, guide, pole):
    zero = [D(0)]*6
    primary = guide if spec['primary'] == 'Ref' else zero
    origin = guide if spec['origin'] == 'Ref' else zero
    r, v = sub(owner[:3], primary[:3]), sub(owner[3:], primary[3:])
    a = acceleration(plan, owner, pole, True)
    if spec['primary'] == 'Ref':
        a = sub(a, acceleration(plan, guide, pole, True))
    normal = cross(r, v); zu = unit(normal)
    nr = cross(r, a); zd = scale(sub(nr, scale(zu, dot(zu, nr))), D(1)/norm(normal))
    axis, rate = (r, v) if spec['x_axis'] == 'R' else (v, a)
    xu = unit(axis); xd = scale(sub(rate, scale(xu, dot(xu, rate))), D(1)/norm(axis))
    yu = cross(zu, xu); yd = add(cross(zd, xu), cross(zu, xd))
    position, velocity = sub(owner[:3], origin[:3]), sub(owner[3:], origin[3:])
    return [dot(e, position) for e in (xu, yu, zu)] + [
        dot(e, velocity)+dot(ed, position) for e, ed in zip((xu, yu, zu), (xd, yd, zd))]


def jacobian(plan, spec, owner, guide, pole):
    owner, guide, pole = [[D(v) for v in values] for values in (owner, guide, pole)]
    result = [[D(0)]*6 for _ in range(6)]
    for col in range(6):
        step = D('1e-10') if col < 3 else D('1e-13')
        samples = []
        for k in (-2, -1, 1, 2):
            trial = list(owner); trial[col] += k*step
            samples.append(frame_value(plan, spec, trial, guide, pole))
        for row in range(6):
            result[row][col] = (samples[0][row]-8*samples[1][row]+8*samples[2][row]-samples[3][row])/(12*step)
    return result


def matrix_errors(actual, expected, bounds):
    need(len(actual) == len(expected) == 6 and all(len(row) == 6 for row in actual), 'Native higher-zonal STM shape differs')
    errors = [abs(actual[i][j]-expected[i][j]) for i in range(6) for j in range(6)]
    need(all(math.isfinite(error) and error <= bounds[2*(i//3)+j//3]
             for (i, j), error in zip(((i,j) for i in range(6) for j in range(6)), errors)),
         'Native higher-zonal STM differs from independent variational flow')
    return max(errors)


def audit(root, fixture):
    root = Path(root); plan = json.loads(Path(fixture).read_text())
    proof = json.loads((root/'proof.json').read_text())
    force = json.loads((root/'force-provider.json').read_text())
    pose_record = json.loads((root/'force-poses.json').read_text()); poses = pose_record['samples']
    bounds = plan['matrix_block_tolerances']
    need(bounds == [2e-8,2e-6,2e-11,2e-8] and plan['position_tolerance_km'] == 2e-5 and
         plan['velocity_tolerance_km_s'] == 2e-8 and plan['degree'] == plan['stm_limit'] == 4 and
         plan['order'] == 0 and plan['j2'] == .00108262668 and plan['j3'] == -.00025 and
         plan['j4'] == -.00016, 'Frozen higher-zonal case/bounds differ')
    need(force['type'] == 'GravityField' and force['degree'] == force['stm_limit'] == 4 and
         force['order'] == 0 and force['body'] == 'Earth' and force['mu'] == plan['mu'] and
         Path(force['potential']).resolve() == (root/'STM-higher-zonal.cof').resolve(), 'Actual higher-zonal provider differs')
    coefficients = [line.split() for line in (root/'STM-higher-zonal.cof').read_text().splitlines() if line.startswith('RECOEF')]
    need([(int(v[1]),int(v[2])) for v in coefficients] == [(n,m) for n in range(2,5) for m in range(n+1)],
         'Higher-zonal coefficient census differs')
    for row in coefficients:
        n,m = map(int,row[1:3])
        wanted = -plan['j'+str(n)]/math.sqrt(2*n+1) if m == 0 else 0.
        need(abs(float(row[3])-wanted) < 1e-17 and float(row[4]) == 0., 'Synthetic higher-zonal coefficient differs')
    time = proof['integration_seconds']
    need(math.isfinite(time) and abs(time-60) < 1e-6 and len(poses) == 9 and
         pose_record['sampled_detached_frames'] and pose_record['input_base'] == 'FK5' and
         pose_record['input_axes'] == 'MJ2000Eq', 'Shared higher-zonal pose/endpoint scope differs')
    for k, pose in enumerate(poses):
        m = pose['input_to_fixed']
        need(abs(pose['seconds']-k*time/8) < 1e-9 and len(m) == 3 and all(len(row) == 3 for row in m), 'higher-zonal pose grid/shape differs')
        need(all(math.isfinite(v) for row in m for v in row) and
             all(abs(sum(m[i][n]*m[j][n] for n in range(3))-float(i == j)) < 5e-13
                 for i in range(3) for j in range(3)), 'Shared higher-zonal pose is not a finite orthogonal basis')
    truths = []; maximum = 0.
    for radius, state_key, matrix_key in [(plan['radius'],'owner_state','native_stm'), (plan['reference_radius'],'guide_state','guide_stm')]:
        coarse_state, coarse_phi = integrate(plan, poses, radius, time, plan['coarse_step_seconds'])
        state, phi = integrate(plan, poses, radius, time, plan['fine_step_seconds'])
        need(all(abs(a-b) <= plan['refinement_state_bounds'][0 if i < 3 else 1]
                 for i, (a,b) in enumerate(zip(state, coarse_state))), 'Independent higher-zonal state refinement failed')
        matrix_errors(phi, coarse_phi, plan['refinement_matrix_block_bounds'])
        actual = proof[state_key]
        need(len(actual) == 6 and all(math.isfinite(v) and abs(v-state[i]) <=
             (plan['position_tolerance_km'] if i < 3 else plan['velocity_tolerance_km_s']) for i,v in enumerate(actual)),
             'Native higher-zonal trajectory differs from independent force integration')
        maximum = max(maximum, matrix_errors(proof[matrix_key], phi, bounds))
        truths.append((state, phi))
    expected = {}
    with localcontext() as context:
        context.prec = 80
        initial_owner = [plan['radius'],0,0,0,math.sqrt(plan['mu']/plan['radius']),0]
        initial_guide = [plan['reference_radius'],0,0,0,math.sqrt(plan['mu']/plan['reference_radius']),0]
        for spec in plan['cases']:
            if spec['singular']:
                continue
            a = jacobian(plan, spec, initial_owner, initial_guide, pole_at(poses, 0))
            b = jacobian(plan, spec, truths[0][0], truths[1][0], pole_at(poses, time))
            result = mul(b, mul([[D(v) for v in row] for row in truths[0][1]], inverse(a)))
            expected[spec['key']] = [[float(v) for v in row] for row in result]
    observations = proof['observations']; specs = {c['key']:c for c in plan['cases']}
    need([(v['case'],v['provider'],v['type']) for v in observations] ==
         [(key,p,t) for key in specs for p in PROVIDERS for t in TYPES], 'Nonzero-field Parameter route inventory differs')
    for row in observations:
        need(all(row.get(k,False) for k in ('owner_unchanged','guide_unchanged','cache_atomic','readonly','owner_bound','frame_bound')),
             'higher-zonal frame binding/readonly/rejection isolation differs')
        if specs[row['case']]['singular']:
            need(row['rejected'] and 'singular' in row['diagnostic'].lower(), 'Centered higher-zonal chart not explicitly rejected as singular')
            continue
        index = TYPES.index(row['type']); size = 6 if index == 0 else 3
        r,c = (0,0) if index == 0 else (3*((index-1)//2),3*((index-1)%2))
        actual = row['actual']; truth = expected[row['case']]
        need(not row['rejected'] and len(actual) == size and all(len(v) == size for v in actual), 'Defined higher-zonal frame rejected or wrong shape')
        need(all(math.isfinite(actual[i][j]) and abs(actual[i][j]-truth[i+r][j+c]) <= bounds[2*((i+r)//3)+(j+c)//3]
                 for i in range(size) for j in range(size)), 'higher-zonal owner-frame STM differs from independent full Cartesian differential')
    return dict(status='passed', native_reads=45, defined_reads=45, singular_rejections=0,
                force_provider='GravityField', force_degree=4, max_native_stm_error=maximum,
                independent_force='closed-form synthetic J2 J3 J4 potential', shared_native_inputs='field poses only',
                independent_earth_orientation_qualified=False, native_tests_run_by_replay=0,
                new_requirement_credit=0, whole_row_qualified=False)
