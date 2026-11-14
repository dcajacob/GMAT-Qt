"""Independent degree-two potential, variational flow and frame differentials."""
import cmath
from decimal import localcontext
import json
import math
from pathlib import Path
import re

from StmOwnerFrameOracle import D, TYPES, PROVIDERS, dot, add, sub, scale, norm, unit, cross, mul, inverse


def require(condition, message):
    if not condition:
        raise ValueError(message)


def orientation_at(poses, time):
    for left, right in zip(poses, poses[1:]):
        if time <= right['seconds'] + 1e-9:
            ratio = (time - left['seconds']) / (right['seconds'] - left['seconds'])
            rows = [[(1-ratio)*a + ratio*b for a, b in zip(first, second)]
                    for first, second in zip(left['input_to_fixed'], right['input_to_fixed'])]
            length = math.sqrt(sum(value*value for value in rows[0]))
            x = [value/length for value in rows[0]]
            z = rows[2]
            y = [z[1]*x[2]-z[2]*x[1], z[2]*x[0]-z[0]*x[2], z[0]*x[1]-z[1]*x[0]]
            length = math.sqrt(sum(value*value for value in y))
            y = [value/length for value in y]
            z = [x[1]*y[2]-x[2]*y[1], x[2]*y[0]-x[0]*y[2], x[0]*y[1]-x[1]*y[0]]
            return [x, y, z]
    raise ValueError('Tesseral field orientation is unavailable')


def plan_at(plan, poses, time):
    if 'love_k2' not in plan:
        return plan
    for left,right in zip(poses,poses[1:]):
        if time <= right['seconds']+1e-9:
            ratio = (time-left['seconds'])/(right['seconds']-left['seconds'])
            bodies = [dict(first,position=[(1-ratio)*a+ratio*b for a,b in zip(first['position'],second['position'])])
                      for first,second in zip(left['tide_bodies'],right['tide_bodies'])]
            return dict(plan,tide_bodies=bodies)
    raise ValueError('Tide-generating body sample is unavailable')


def tide_terms(plan, state, decimal=False):
    number = D if decimal else float
    squared = sum(value*value for value in state[:3])
    radius = squared.sqrt() if decimal else cmath.sqrt(squared)
    potential = number(0)
    gradient = [number(0)]*3
    for body in plan.get('tide_bodies',[]):
        position = [number(value) for value in body['position']]
        distance_squared = sum(value*value for value in position)
        distance = distance_squared.sqrt() if decimal else cmath.sqrt(distance_squared)
        direction = [value/distance for value in position]
        projection = sum(a*b for a,b in zip(state[:3],direction))
        coefficient = number(plan['love_k2'])*number(body['mu'])*number(plan['potential_radius_km'])**5/(2*distance**3)
        polynomial = 3*projection**2-squared
        potential += coefficient*polynomial/radius**5
        for i in range(3):
            gradient[i] += coefficient*((6*projection*direction[i]-2*state[i])/radius**5-
                                        5*polynomial*state[i]/radius**7)
    return potential,gradient


def quadrupole(plan, state, orientation, decimal=False):
    number = D if decimal else float
    j2, c21, s21, c22, s22 = [number(plan[key]) for key in ('j2', 'c21', 's21', 'c22', 's22')]
    root15 = D(15).sqrt() if decimal else math.sqrt(15)
    x, y, z = [sum(a*b for a, b in zip(row, state[:3])) for row in orientation]
    # Fully normalized, geodesy-sign harmonics expanded as a Cartesian polynomial.
    value = j2*(x*x+y*y-2*z*z)/2 + root15*(c21*x*z+s21*y*z)
    value += root15*(c22*(x*x-y*y)+2*s22*x*y)/2
    body_gradient = [j2*x + root15*(c21*z+c22*x+s22*y),
                     j2*y + root15*(s21*z-c22*y+s22*x),
                     -2*j2*z + root15*(c21*x+s21*y)]
    gradient = [sum(orientation[row][col]*body_gradient[row] for row in range(3))
                for col in range(3)]
    return value, gradient


def potential(plan, state, orientation):
    distance = cmath.sqrt(sum(value*value for value in state[:3]))
    value, _ = quadrupole(plan, state, orientation)
    result = plan['mu']/distance + plan['mu']*plan['potential_radius_km']**2*value/distance**5
    if plan.get('tide_bodies'):
        result += tide_terms(plan,state)[0]
    return result


def acceleration(plan, state, orientation, decimal=False):
    number = D if decimal else float
    mu = number(plan['mu'])
    radius = number(plan['potential_radius_km'])
    radius_squared = sum(value*value for value in state[:3])
    distance = radius_squared.sqrt() if decimal else cmath.sqrt(radius_squared)
    value, gradient = quadrupole(plan, state, orientation, decimal)
    result = [-mu*x/distance**3 + mu*radius**2*(g/distance**5-5*value*x/distance**7)
              for x,g in zip(state[:3],gradient)]
    if plan.get('tide_bodies'):
        tide = tide_terms(plan,state,decimal)[1]
        result = [a+b for a,b in zip(result,tide)]
    return result


def integrate(plan, poses, radius, duration, step):
    state = [radius, 0., 0., 0., math.sqrt(plan['mu']/radius), 0.]
    state += [float(i == j) for i in range(6) for j in range(6)]
    count = round(duration/step)
    require(count > 0 and abs(count*step-duration) < 1e-6, 'Independent endpoint grid differs')
    step = duration/count

    def derivative(time, value):
        orientation = orientation_at(poses, time)
        current_plan = plan_at(plan,poses,time)
        force = acceleration(current_plan, value, orientation)
        gradient = [[0.]*3 for _ in range(3)]
        for col in range(3):
            trial = list(value[:6])
            trial[col] += 1e-30j
            differential = acceleration(current_plan, trial, orientation)
            for row in range(3):
                gradient[row][col] = differential[row].imag/1e-30
        phi = value[6:]
        return value[3:6] + [component.real for component in force] + phi[18:36] + [
            sum(gradient[row][k]*phi[6*k+col] for k in range(3))
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
    orientation = [[D(value) for value in row] for row in orientation]
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
            'Tesseral STM shape differs')
    errors = [abs(actual[row][col]-expected[row][col]) for row in range(6) for col in range(6)]
    require(all(math.isfinite(error) and error <= bounds[2*(row//3)+col//3]
                for (row, col), error in zip(((i, j) for i in range(6) for j in range(6)), errors)),
            'Tesseral STM differs from independent variational flow')
    return max(errors)


def preflight(plan):
    identity = [[float(i == j) for j in range(3)] for i in range(3)]
    maximum = 0.
    count = 0
    for state in ([7000., 400., 1300.], [-1900., 7500., -900.], [3400., -2700., 6600.]):
        force = acceleration(plan, state, identity)
        for col in range(3):
            trial = list(state)
            trial[col] += 1e-25j
            error = abs(potential(plan, trial, identity).imag/1e-25-force[col].real)
            require(error <= 1e-16, 'Expanded potential gradient preflight failed')
            maximum = max(maximum, error)
            count += 1
    return {'status': 'passed', 'potential_gradient_checks': count, 'maximum_error': maximum}


def audit(root, fixture):
    root = Path(root)
    plan = json.loads(Path(fixture).read_text())
    proof = json.loads((root/'proof.json').read_text())
    force = json.loads((root/'force-provider.json').read_text())
    pose_record = json.loads((root/'force-poses.json').read_text())
    poses = pose_record['samples']
    bounds = plan['matrix_block_tolerances']
    require(bounds == [2e-8, 2e-6, 2e-11, 2e-8] and
            plan['position_tolerance_km'] == 2e-5 and plan['velocity_tolerance_km_s'] == 2e-8 and
            [plan[key] for key in ('j2', 'c21', 's21', 'c22', 's22')] ==
            [.00108262668, .00003, -.00002, .0001, -.00015], 'Frozen tesseral inputs/bounds differ')
    require(force['type'] == 'GravityField' and force['degree'] == force['order'] == force['stm_limit'] == 2 and
            force['body'] == 'Earth' and force['mu'] == plan['mu'] and
            Path(force['potential']).resolve() == (root/'STM-tesseral.cof').resolve(),
            'Actual tesseral force provider differs')
    if 'love_k2' in plan:
        require(force['tide_model']=='Solid' and Path(force['tide_file']).name=='isotropic.tide',
                'Executed tide model differs')
        rows = [line.split() for line in (root/'isotropic.tide').read_text().splitlines()]
        require([row[:3] for row in rows]==[['k','2',str(i)] for i in range(3)] and
            all(float(row[3])==plan['love_k2'] for row in rows),'Frozen Love numbers differ')
        for pose in poses:
            require([body['name'] for body in pose['tide_bodies']]==['Sun','Luna'] and
                all(body['mu']>0 and len(body['position'])==3 for body in pose['tide_bodies']),
                'Tide-generating body inventory differs')
    # COF uses adjacent fixed-width fields; a negative sine term can touch the
    # preceding exponent without a separating space.
    number = r'[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[Ee][+-]?\d+)?'
    coefficients = [re.findall(number, line) for line in
                    (root/'STM-tesseral.cof').read_text().splitlines() if line.startswith('RECOEF')]
    require(all(len(row) == 4 for row in coefficients), 'Malformed tesseral coefficient record')
    require([(int(row[0]), int(row[1])) for row in coefficients] == [(2, 0), (2, 1), (2, 2)],
            'Tesseral coefficient census differs')
    expected_coefficients = [(-plan['j2']/math.sqrt(5), 0), (plan['c21'], plan['s21']),
                             (plan['c22'], plan['s22'])]
    for row, expected in zip(coefficients, expected_coefficients):
        require(all(abs(float(value)-wanted) < 1e-17 for value, wanted in zip(row[2:4], expected)),
                'Tesseral coefficient differs')
    duration = proof['integration_seconds']
    require(math.isfinite(duration) and abs(duration-60) < 1e-6 and len(poses) == 61 and
            pose_record['sampled_detached_frames'] and pose_record['input_base'] == 'FK5' and
            pose_record['input_axes'] == 'MJ2000Eq', 'Tesseral pose/endpoint scope differs')
    for index, pose in enumerate(poses):
        matrix = pose['input_to_fixed']
        require(abs(pose['seconds']-index*duration/60) < 1e-9 and len(matrix) == 3 and
                all(len(row) == 3 for row in matrix), 'Tesseral pose grid/shape differs')
        require(all(math.isfinite(value) for row in matrix for value in row) and
                all(abs(sum(matrix[i][k]*matrix[j][k] for k in range(3))-float(i == j)) < 5e-13
                    for i in range(3) for j in range(3)), 'Tesseral orientation is not orthogonal')
    truths = []
    maximum = 0.
    for radius, state_key, matrix_key in [(plan['radius'], 'owner_state', 'native_stm'),
                                         (plan['reference_radius'], 'guide_state', 'guide_stm')]:
        coarse_state, coarse_phi = integrate(plan, poses, radius, duration, plan['coarse_step_seconds'])
        state, phi = integrate(plan, poses, radius, duration, plan['fine_step_seconds'])
        require(all(abs(a-b) <= plan['refinement_state_bounds'][0 if i < 3 else 1]
                    for i, (a, b) in enumerate(zip(state, coarse_state))), 'Tesseral state refinement failed')
        matrix_errors(phi, coarse_phi, plan['refinement_matrix_block_bounds'])
        actual = proof[state_key]
        require(len(actual) == 6 and all(math.isfinite(value) and abs(value-state[i]) <=
                (plan['position_tolerance_km'] if i < 3 else plan['velocity_tolerance_km_s'])
                for i, value in enumerate(actual)), 'Tesseral trajectory differs from independent potential')
        maximum = max(maximum, matrix_errors(proof[matrix_key], phi, bounds))
        truths.append((state, phi))
    expected = {}
    with localcontext() as context:
        context.prec = 80
        initial_owner = [plan['radius'], 0, 0, 0, math.sqrt(plan['mu']/plan['radius']), 0]
        initial_guide = [plan['reference_radius'], 0, 0, 0, math.sqrt(plan['mu']/plan['reference_radius']), 0]
        for spec in plan['cases']:
            initial = jacobian(plan_at(plan,poses,0), spec, initial_owner, initial_guide, orientation_at(poses, 0))
            final = jacobian(plan_at(plan,poses,duration), spec, truths[0][0], truths[1][0], orientation_at(poses, duration))
            transformed = mul(final, mul([[D(value) for value in row] for row in truths[0][1]], inverse(initial)))
            expected[spec['key']] = [[float(value) for value in row] for row in transformed]
    observations = proof['observations']
    require([(row['case'], row['type'], row['provider']) for row in observations] ==
            [(spec['key'], kind, provider) for spec in plan['cases'] for kind in TYPES for provider in PROVIDERS],
            'Tesseral Parameter route inventory differs')
    for row in observations:
        require(all(row.get(key, False) for key in ('owner_unchanged', 'guide_unchanged', 'cache_atomic',
                'readonly', 'owner_bound', 'frame_bound')), 'Tesseral binding or read isolation differs')
        index = TYPES.index(row['type'])
        size = 6 if index == 0 else 3
        first_row, first_col = (0, 0) if index == 0 else (3*((index-1)//2), 3*((index-1)%2))
        actual = row['actual']
        truth = expected[row['case']]
        require(not row['rejected'] and len(actual) == size and all(len(value) == size for value in actual),
                'Defined tesseral frame rejected or shape differs')
        require(all(math.isfinite(actual[i][j]) and abs(actual[i][j]-truth[i+first_row][j+first_col]) <=
                    bounds[2*((i+first_row)//3)+(j+first_col)//3]
                    for i in range(size) for j in range(size)),
                'Tesseral owner-frame STM differs from independent Cartesian differential')
    clock_checks = 0
    if 'love_k2' in plan:
        samples = json.loads((root/'sampled-force-clocks.json').read_text())
        require([row['offset'] for row in samples]==plan['clock_offsets_seconds'],'Force-clock probe inventory differs')
        current_plan = plan_at(plan,poses,duration)
        orientation = orientation_at(poses,duration)
        expected_force = acceleration(current_plan,proof['owner_state'],orientation)
        for sample in samples:
            require(sample['clock_unchanged'] and sample['available'] and
                sample['epoch_before']==sample['epoch_after'] and sample['time_before']==sample['time_after'] and
                sample['time_before']-samples[0]['time_before']==sample['offset'],
                'Tidal sampling changed its clock or lost its gradient')
            require(all(abs(a-b.real)<=plan['force_tolerance'] for a,b in zip(sample['acceleration'],expected_force)),
                    'Tidal sampling used the wrong absolute epoch')
            for column in range(6):
                trial = list(proof['owner_state'])
                trial[column] += 1e-30j
                expected = acceleration(current_plan,trial,orientation)
                for row in range(3):
                    require(abs(sample['gradient'][row][column]-expected[row].imag/1e-30)<=plan['gradient_tolerance'],
                            'Tidal sampling gradient differs from the independent potential')
                    clock_checks += 1
    result = dict(status='passed', native_reads=len(observations), defined_reads=len(observations),
                force_provider='GravityField', force_degree=2, force_order=2,
                max_native_stm_error=maximum, independent_force='expanded full degree-two Cartesian potential',
                shared_native_inputs='field orientations only', independent_earth_orientation_qualified=False,
                native_tests_run_by_replay=0, new_requirement_credit=0, whole_row_qualified=False)
    if 'love_k2' in plan:
        result.update(tide_model='Solid',shifted_clock_derivative_checks=clock_checks,
            independent_force='expanded degree-two Cartesian potential plus isotropic solid-tide potential',
            shared_native_inputs='field orientations and tide-generating body positions/gravitational parameters')
    return result
