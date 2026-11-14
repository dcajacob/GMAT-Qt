"""Independent post-Newtonian force, variational flow and frame differentials."""
import cmath
from decimal import localcontext
import json
import math
from pathlib import Path

from StmOwnerFrameOracle import D, TYPES, PROVIDERS, dot, add, sub, scale, norm, unit, cross, mul, inverse


def require(condition, message):
    if not condition:
        raise ValueError(message)


def environment_at(plan, poses, time):
    def sample(pose):
        rotation = pose['fixed_to_input']
        rate = pose['fixed_to_input_dot']
        components = [-sum(rotation[i][2]*rate[i][1] for i in range(3)),
                       sum(rotation[i][2]*rate[i][0] for i in range(3)),
                      -sum(rotation[i][1]*rate[i][0] for i in range(3))]
        magnitude = math.sqrt(sum(value*value for value in components))
        factor = .4*plan['central_body_radius_km']**2*magnitude
        spin = [rotation[i][2]*factor for i in range(3)]
        precession = [0., 0., 0.]
        if plan.get('body', 'Sun') != 'Sun':
            position = pose['body_wrt_sun'][:3]
            velocity = pose['body_wrt_sun'][3:]
            distance = math.sqrt(sum(value*value for value in position))
            factor = 1.5*plan['sun_mu']/(plan['speed_of_light_km_s']**2*distance**3)
            precession = [factor*(position[1]*velocity[2]-position[2]*velocity[1]),
                          factor*(position[2]*velocity[0]-position[0]*velocity[2]),
                          factor*(position[0]*velocity[1]-position[1]*velocity[0])]
        return dict(spin=spin, precession=precession)
    for left, right in zip(poses, poses[1:]):
        if time <= right['seconds'] + 1e-9:
            ratio = (time-left['seconds'])/(right['seconds']-left['seconds'])
            first, second = sample(left), sample(right)
            return {key: [(1-ratio)*a+ratio*b for a, b in zip(first[key], second[key])]
                    for key in ('spin', 'precession')}
    raise ValueError('Relativistic spin sample is unavailable')


def relativistic_acceleration(plan, state, environment, decimal=False):
    number = D if decimal else float
    mu, light = number(plan['mu']), number(plan['speed_of_light_km_s'])
    spin = environment['spin']
    precession = environment['precession']
    position, velocity = state[:3], state[3:6]
    radius2 = sum(value*value for value in position)
    radius = radius2.sqrt() if decimal else cmath.sqrt(radius2)
    velocity2 = sum(value*value for value in velocity)
    radial_velocity = sum(a*b for a,b in zip(position,velocity))
    spin_projection = sum(a*b for a,b in zip(position,spin))
    angular = [position[1]*velocity[2]-position[2]*velocity[1],
               position[2]*velocity[0]-position[0]*velocity[2],
               position[0]*velocity[1]-position[1]*velocity[0]]
    spin_cross = [velocity[1]*spin[2]-velocity[2]*spin[1],
                  velocity[2]*spin[0]-velocity[0]*spin[2],
                  velocity[0]*spin[1]-velocity[1]*spin[0]]
    factor = mu/(light*light*radius**3)
    spin_factor = number(plan.get('spin_scale',1))
    geodesic_factor = number(plan.get('geodesic_scale',1))
    geodesic = [2*(precession[1]*velocity[2]-precession[2]*velocity[1]),
                2*(precession[2]*velocity[0]-precession[0]*velocity[2]),
                2*(precession[0]*velocity[1]-precession[1]*velocity[0])]
    return [factor*((4*mu/radius-velocity2)*position[i]+4*radial_velocity*velocity[i] +
                    2*spin_factor*(3*spin_projection/radius2*angular[i]+spin_cross[i])) +
            geodesic_factor*geodesic[i]
            for i in range(3)]


def acceleration(plan, state, spin, decimal=False):
    number = D if decimal else float
    radius2 = sum(value*value for value in state[:3])
    radius = radius2.sqrt() if decimal else cmath.sqrt(radius2)
    correction = relativistic_acceleration(plan,state,spin,decimal)
    factor = number(plan.get('relativistic_scale',1))
    return [-number(plan['mu'])*state[i]/radius**3+factor*correction[i] for i in range(3)]


def integrate(plan, poses, radius, duration, step):
    state = [radius, 0., 0., 0., math.sqrt(plan['mu']/radius), 0.]
    state += [float(i == j) for i in range(6) for j in range(6)]
    count = round(duration/step)
    require(count > 0 and abs(count*step-duration) < 1e-6, 'Independent endpoint grid differs')
    step = duration/count

    def derivative(time, value):
        orientation = environment_at(plan, poses, time)
        force = acceleration(plan, value, orientation)
        gradient = [[0.]*6 for _ in range(3)]
        for col in range(6):
            trial = list(value[:6])
            trial[col] += 1e-30j
            differential = acceleration(plan, trial, orientation)
            for row in range(3):
                gradient[row][col] = differential[row].imag/1e-30
        phi = value[6:]
        return value[3:6] + [component.real for component in force] + phi[18:36] + [
            sum(gradient[row][k]*phi[6*k+col] for k in range(6))
            for row in range(3) for col in range(6)]

    def moved(value, rate, amount):
        return [component+amount*change for component, change in zip(value, rate)]

    for i in range(count):
        time = i*step
        a = derivative(time, state)
        b = derivative(time+step/2, moved(state, a, step/2))
        c = derivative(time+step/2, moved(state, b, step/2))
        d = derivative(time+step, moved(state, c, step))
        state = [value+step*(aa+2*bb+2*cc+dd)/6
                 for value, aa, bb, cc, dd in zip(state, a, b, c, d)]
    return state[:6], [state[6+6*i:12+6*i] for i in range(6)]


def frame_value(plan, spec, owner, guide, orientation):
    zero = [D(0)]*6
    primary = guide if spec['primary'] == 'Ref' else zero
    origin = guide if spec['origin'] == 'Ref' else zero
    position = sub(owner[:3], primary[:3])
    velocity = sub(owner[3:], primary[3:])
    force = acceleration(plan, owner, orientation, True)
    if spec['primary'] == 'Ref':
        force = sub(force, acceleration(plan, guide, orientation, True))
    normal = cross(position, velocity)
    z = unit(normal)
    normal_rate = cross(position, force)
    z_rate = scale(sub(normal_rate, scale(z, dot(z, normal_rate))), D(1)/norm(normal))
    axis, rate = (position, velocity) if spec['x_axis'] == 'R' else (velocity, force)
    x = unit(axis)
    x_rate = scale(sub(rate, scale(x, dot(x, rate))), D(1)/norm(axis))
    y = cross(z, x)
    y_rate = add(cross(z_rate, x), cross(z, x_rate))
    offset = sub(owner[:3], origin[:3])
    relative_velocity = sub(owner[3:], origin[3:])
    return [dot(axis, offset) for axis in (x, y, z)] + [
        dot(axis, relative_velocity)+dot(rate, offset)
        for axis, rate in zip((x, y, z), (x_rate, y_rate, z_rate))]


def jacobian(plan, spec, owner, guide, orientation):
    owner, guide = [[D(value) for value in vector] for vector in (owner, guide)]
    orientation = {key: [D(value) for value in values] for key, values in orientation.items()}
    result = [[D(0)]*6 for _ in range(6)]
    for col in range(6):
        step = D('1e-10') if col < 3 else D('1e-13')
        samples = []
        for offset in (-2, -1, 1, 2):
            trial = list(owner)
            trial[col] += offset*step
            samples.append(frame_value(plan, spec, trial, guide, orientation))
        for row in range(6):
            result[row][col] = (samples[0][row]-8*samples[1][row]+8*samples[2][row]-samples[3][row])/(12*step)
    return result


def matrix_errors(actual, expected, bounds):
    require(len(actual) == len(expected) == 6 and all(len(row) == 6 for row in actual),
            'Relativistic STM shape differs')
    errors = [abs(actual[row][col]-expected[row][col]) for row in range(6) for col in range(6)]
    require(all(math.isfinite(error) and error <= bounds[2*(row//3)+col//3]
                for (row, col), error in zip(((i, j) for i in range(6) for j in range(6)), errors)),
            'Relativistic STM differs from independent variational flow')
    return max(errors)


def preflight(plan):
    state = [1e6, 2e5, -1e5, -80., 330., 60.]
    spin = dict(spin=[1e5, -2e5, 5e5], precession=[1e-13, -2e-13, 3e-13])
    maximum = 0.
    checks = 0
    with localcontext() as context:
        context.prec = 80
        for col in range(6):
            trial = list(state)
            trial[col] += 1e-20j
            derivatives = [value.imag/1e-20 for value in relativistic_acceleration(plan,trial,spin)]
            step = D('.1') if col < 3 else D('.0001')
            samples=[]
            for offset in (-2,-1,1,2):
                values=[D(value) for value in state]
                values[col]+=offset*step
                decimal_environment = {key: [D(value) for value in vector] for key, vector in spin.items()}
                samples.append(relativistic_acceleration(plan,values,decimal_environment,True))
            for row in range(3):
                expected=float((samples[0][row]-8*samples[1][row]+8*samples[2][row]-samples[3][row])/(12*step))
                error=abs(derivatives[row]-expected)
                require(error <= 1e-22,'Independent relativistic derivative preflight failed')
                maximum=max(maximum,error)
                checks+=1
    return dict(status='passed',independent_derivative_checks=checks,maximum_error=maximum)


def audit(root, fixture):
    root = Path(root)
    plan = json.loads(Path(fixture).read_text())
    proof = json.loads((root/'proof.json').read_text())
    force = json.loads((root/'force-provider.json').read_text())
    pose_record = json.loads((root/'force-poses.json').read_text())
    poses = pose_record['samples']
    bounds = plan['matrix_block_tolerances']
    require(bounds == [2e-8,2e-6,2e-11,2e-8] and
            plan['position_tolerance_km']==2e-5 and plan['velocity_tolerance_km_s']==2e-8 and
            (plan.get('body','Sun'), plan['mu'], plan['central_body_radius_km']) in
            [('Sun',132712440017.99,695990.0), ('Earth',398600.4418,6378.1363)] and
            plan['speed_of_light_km_s']==299792.458,'Frozen relativistic inputs/bounds differ')
    require(force['type']=='RelativisticCorrection' and force['body']==plan.get('body','Sun') and
            force['mu']==plan['mu'] and force['radius']==plan['central_body_radius_km'],
            'Actual relativistic force provider differs')
    if plan.get('body', 'Sun') != 'Sun':
        require(force['sun_mu']==plan['sun_mu']==132712440017.99,
                'Frozen heliocentric gravity constant differs')
    duration = proof['integration_seconds']
    require(math.isfinite(duration) and abs(duration-600) < 1e-6 and len(poses) == 61 and
            pose_record['sampled_detached_frames'] and pose_record['input_base'] == 'FK5' and
            pose_record['input_axes'] == 'MJ2000Eq', 'Relativistic pose/endpoint scope differs')
    for index, pose in enumerate(poses):
        matrix = pose['fixed_to_input']
        require(abs(pose['seconds']-index*duration/60) < 1e-9 and len(matrix) == 3 and
                all(len(row) == 3 for row in matrix), 'Relativistic pose grid/shape differs')
        require(all(math.isfinite(value) for row in matrix for value in row) and
                all(abs(sum(matrix[i][k]*matrix[j][k] for k in range(3))-float(i == j)) < 5e-13
                    for i in range(3) for j in range(3)), 'Relativistic orientation is not orthogonal')
        if plan.get('body', 'Sun') != 'Sun':
            ephemeris = pose['body_wrt_sun']
            require(len(ephemeris)==6 and all(math.isfinite(value) for value in ephemeris) and
                    sum(value*value for value in ephemeris[:3])>0,
                    'Heliocentric body state is invalid')
    truths = []
    maximum = 0.
    for radius, state_key, matrix_key in [(plan['radius'], 'owner_state', 'native_stm'),
                                         (plan['reference_radius'], 'guide_state', 'guide_stm')]:
        coarse_state, coarse_phi = integrate(plan, poses, radius, duration, plan['coarse_step_seconds'])
        state, phi = integrate(plan, poses, radius, duration, plan['fine_step_seconds'])
        require(all(abs(a-b) <= plan['refinement_state_bounds'][0 if i < 3 else 1]
                    for i, (a, b) in enumerate(zip(state, coarse_state))), 'Relativistic state refinement failed')
        matrix_errors(phi, coarse_phi, plan['refinement_matrix_block_bounds'])
        actual = proof[state_key]
        require(len(actual) == 6 and all(math.isfinite(value) and abs(value-state[i]) <=
                (plan['position_tolerance_km'] if i < 3 else plan['velocity_tolerance_km_s'])
                for i, value in enumerate(actual)), 'Relativistic trajectory differs from independent potential')
        if plan.get('matrix_mode','STM')=='STM':
            maximum = max(maximum, matrix_errors(proof[matrix_key], phi, bounds))
        truths.append((state, phi))
    expected = {}
    with localcontext() as context:
        context.prec = 80
        initial_owner = [plan['radius'], 0, 0, 0, math.sqrt(plan['mu']/plan['radius']), 0]
        initial_guide = [plan['reference_radius'], 0, 0, 0, math.sqrt(plan['mu']/plan['reference_radius']), 0]
        for spec in (plan['cases'] if plan.get('matrix_mode','STM')=='STM' else []):
            initial = jacobian(plan, spec, initial_owner, initial_guide, environment_at(plan, poses, 0))
            final = jacobian(plan, spec, truths[0][0], truths[1][0], environment_at(plan, poses, duration))
            transformed = mul(final, mul([[D(value) for value in row] for row in truths[0][1]], inverse(initial)))
            expected[spec['key']] = [[float(value) for value in row] for row in transformed]
    observations = proof['observations']
    require([(row['case'], row['type'], row['provider']) for row in observations] ==
            [(spec['key'], kind, provider) for spec in (plan['cases'] if plan.get('matrix_mode','STM')=='STM' else []) for kind in TYPES for provider in PROVIDERS],
            'Relativistic Parameter route inventory differs')
    for row in observations:
        require(all(row.get(key, False) for key in ('owner_unchanged', 'guide_unchanged', 'cache_atomic',
                'readonly', 'owner_bound', 'frame_bound')), 'Relativistic binding or read isolation differs')
        index = TYPES.index(row['type'])
        size = 6 if index == 0 else 3
        first_row, first_col = (0, 0) if index == 0 else (3*((index-1)//2), 3*((index-1)%2))
        actual = row['actual']
        truth = expected[row['case']]
        require(not row['rejected'] and len(actual) == size and all(len(value) == size for value in actual),
                'Defined relativistic frame rejected or shape differs')
        require(all(math.isfinite(actual[i][j]) and abs(actual[i][j]-truth[i+first_row][j+first_col]) <=
                    bounds[2*((i+first_row)//3)+(j+first_col)//3]
                    for i in range(size) for j in range(size)),
                'Relativistic owner-frame STM differs from independent Cartesian differential')
    queried=json.loads((root/'queried-gradient.json').read_text())
    require(queried['available'] and len(queried['gradient'])==3 and
            all(len(row)==6 for row in queried['gradient']),'Relativistic captured gradient is unavailable')
    state=queried['state']
    require(len(state)==6 and all(abs(a-b) <= (plan['position_tolerance_km'] if i<3 else plan['velocity_tolerance_km_s']) for i,(a,b) in enumerate(zip(state,proof['owner_state']))),'Sampled force state differs from propagated state')
    pose=queried['pose']
    spin=environment_at(plan,[dict(pose,seconds=0.),dict(pose,seconds=1.)],0.)
    expected_acceleration=relativistic_acceleration(plan,state,spin)
    require(all(abs(value-expected.real)<=1e-18 for value,expected in
                zip(queried['acceleration'],expected_acceleration)),'Relativistic sampled acceleration differs')
    maximum_gradient=0.
    for col in range(6):
        trial=list(state)
        trial[col]+=1e-20j
        expected=relativistic_acceleration(plan,trial,spin)
        for row in range(3):
            error=abs(queried['gradient'][row][col]-expected[row].imag/1e-20)
            require(error<=1e-22,'Relativistic captured derivative differs')
            maximum_gradient=max(maximum_gradient,error)
    maximum_amatrix=0.
    if plan.get('matrix_mode','STM')=='AMatrix':
        require(plan['amatrix_tolerance']==1e-15,'Frozen A-matrix bound differs')
        require(plan['amatrix_source']=='force-derivative-array','A-matrix source differs')
        actual=proof['force_amatrix']
        require(len(actual)==6 and all(len(row)==6 for row in actual),'Native A-matrix shape differs')
        for col in range(6):
            trial=list(state)
            trial[col]+=1e-20j
            differential=relativistic_acceleration(plan,trial,spin)
            for row in range(6):
                expected=0. if row<3 else differential[row-3].imag/1e-20
                error=abs(actual[row][col]-expected)
                require(error<=plan['amatrix_tolerance'],'Native A-matrix differs from independent state derivative')
                maximum_amatrix=max(maximum_amatrix,error)
    return dict(status='passed',native_reads=len(observations),defined_reads=len(observations),
                force_provider='RelativisticCorrection',max_native_stm_error=maximum,
                max_captured_gradient_error=maximum_gradient,max_native_amatrix_error=maximum_amatrix,
                independent_force='post-Newtonian Schwarzschild, geodesic and Lense-Thirring equations',
                shared_native_inputs='body orientations, rates and heliocentric ephemeris only',
                independent_earth_orientation_qualified=False,native_tests_run_by_replay=0,
                new_requirement_credit=0,whole_row_qualified=False)
