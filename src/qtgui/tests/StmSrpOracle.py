"""Independent force, variational flow and frame differentials."""
import cmath
from decimal import Decimal, localcontext
import json
import math
from pathlib import Path

from StmOwnerFrameOracle import TYPES, PROVIDERS, dot, add, sub, scale, norm, unit, cross, mul, inverse, sincos


def D(value):
    # The frame conversion consumes native doubles. Preserve their exact input
    # values when checking a highly conditioned differential at high precision.
    if isinstance(value, float):
        return Decimal.from_float(value)
    if isinstance(value, bool):
        return Decimal(int(value))
    return value if isinstance(value, Decimal) else Decimal(str(value))


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sun_at(poses, time, key='earth_to_sun'):
    for left, right in zip(poses, poses[1:]):
        if time <= right['seconds'] + 1e-9:
            ratio=(time-left['seconds'])/(right['seconds']-left['seconds'])
            return [(1-ratio)*a+ratio*b for a,b in zip(left[key],right[key])]
    raise ValueError('Independent Sun sample is unavailable')


def initial_state(plan, sun, radius):
    length=math.sqrt(sum(value*value for value in sun))
    along=[value/length for value in sun]
    across=[-along[1],along[0],0.]
    length=math.sqrt(sum(value*value for value in across))
    across=[value/length for value in across]
    sign=1 if plan.get('sunward',False) else -1
    return [sign*radius*a+plan['transverse_offset_km']*b for a,b in zip(along,across)]+[
        plan['transverse_speed_km_s']*math.sqrt(plan['radius']/radius)*value for value in across]


def arcsin_decimal(value):
    estimate=D(math.asin(float(value)))
    for _ in range(7):
        sine, cosine=sincos(estimate)
        estimate+=(value-sine)/cosine
    return estimate


def angle_acos(value, decimal):
    if decimal:
        return 2*arcsin_decimal(((1-value)/2).sqrt())
    return cmath.acos(value)


def angle_atan(value, decimal):
    if decimal:
        return arcsin_decimal(value/(1+value*value).sqrt())
    return cmath.atan(value)


def body_axes(plan, state, decimal=False):
    number = D if decimal else float
    if plan.get('attitude') != 'NadirPointing':
        return [[number(i == j) for j in range(3)] for i in range(3)]
    def normalize(vector):
        squared = sum(value*value for value in vector)
        length = squared.sqrt() if decimal else cmath.sqrt(squared)
        return [value/length for value in vector]
    radial = normalize(state[:3])
    normal = normalize(cross(state[:3], state[3:6]))
    body_alignment = normalize([number(value) for value in plan['attitude_alignment']])
    body_constraint = [number(value) for value in plan['attitude_body_constraint']]
    projection = sum(a*b for a,b in zip(body_alignment,body_constraint))
    body_transverse = normalize([value-projection*axis
                                 for value,axis in zip(body_constraint,body_alignment)])
    body_basis = [body_alignment,body_transverse,cross(body_alignment,body_transverse)]
    nadir = [-value for value in radial]
    target_transverse = normal if plan['attitude_constraint']=='OrbitNormal' else cross(normal,radial)
    target_basis = [nadir,target_transverse,cross(nadir,target_transverse)]
    return [[sum(target_basis[k][row]*body_basis[k][column] for k in range(3))
             for row in range(3)] for column in range(3)]


def illumination(plan, state, sun, decimal=False):
    number=D if decimal else float
    position=state[:3]
    delta=[a-b for a,b in zip(position,sun)]
    body_squared=sum(value*value for value in position)
    sun_squared=sum(value*value for value in delta)
    body_distance=body_squared.sqrt() if decimal else cmath.sqrt(body_squared)
    sun_distance=sun_squared.sqrt() if decimal else cmath.sqrt(sun_squared)
    if sum(float(complex(a).real)*float(complex(b).real) for a,b in zip(position,sun))>0:
        return number(1)
    a=arcsin_decimal(number(plan['sun_radius_km'])/sun_distance) if decimal else cmath.asin(plan['sun_radius_km']/sun_distance)
    b=arcsin_decimal(number(plan['earth_radius_km'])/body_distance) if decimal else cmath.asin(plan['earth_radius_km']/body_distance)
    separation_cross=cross(position,delta)
    separation_dot=sum(x*y for x,y in zip(position,delta))
    real_cross_norm=math.sqrt(sum(complex(value).real**2 for value in separation_cross))
    real_c=math.atan2(real_cross_norm,float(complex(separation_dot).real))
    real_a,real_b=float(complex(a).real),float(complex(b).real)
    if real_c >= real_a+real_b:
        return number(1)
    if real_b>=real_a and real_c<=real_b-real_a:
        return number(0)
    if real_a>real_b and real_c<=real_a-real_b:
        return 1-b*b/(a*a)
    # Cross/dot avoids cancellation in acos when the apparent disks align.
    cross_squared=sum(value*value for value in separation_cross)
    cross_norm=cross_squared.sqrt() if decimal else cmath.sqrt(cross_squared)
    c=angle_atan(cross_norm/separation_dot,decimal)
    x=(c*c+a*a-b*b)/(2*c)
    y=(a*a-x*x).sqrt() if decimal else cmath.sqrt(a*a-x*x)
    pi=number('3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986')
    area=a*a*angle_acos(x/a,decimal)+b*b*angle_acos((c-x)/b,decimal)-c*y
    return 1-area/(pi*a*a)


def plate_reflectance(plan,state,sun,decimal=False):
    number=D if decimal else float
    incoming=[b-a for a,b in zip(state[:3],sun)]
    squared=sum(value*value for value in incoming)
    length=squared.sqrt() if decimal else cmath.sqrt(squared)
    incoming=[value/length for value in incoming]
    axes=body_axes(plan,state,decimal)
    if plan['plate_type']=='SunFacing':
        normal=incoming
    else:
        normal=[sum(number(plan['plate_normal'][j])*axes[j][i] for j in range(3)) for i in range(3)]
    cosine=sum(value*axis for value,axis in zip(incoming,normal))
    if float(complex(cosine).real)<=0:
        return [number(0)]*3
    specular=number(plan['plate_specular'])
    diffuse=number(plan['plate_diffuse'])
    area=number(plan['plate_area_m2'])*number(plan['plate_area_coefficient'])*number(plan['plate_lit_fraction'])
    return [area*cosine*((1-specular)*value+2*(diffuse/3+specular*cosine)*axis)
            for value,axis in zip(incoming,normal)]


def spad_body_area(plan, direction, decimal=False):
    number = D if decimal else float
    area = [number(value) for value in plan['spad_body_area_m2']]
    if 'spad_azimuth_slope' not in plan:
        return area
    pi = number('3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986')
    azimuth = angle_atan(direction[1] / direction[0], decimal)
    if float(complex(direction[0]).real) < 0:
        azimuth += pi if float(complex(direction[1]).real) >= 0 else -pi
    xy = direction[0]**2 + direction[1]**2
    elevation = angle_atan(direction[2] / (xy.sqrt() if decimal else cmath.sqrt(xy)), decimal)
    azimuth *= 180 / pi
    elevation *= 180 / pi
    return [value + number(a)*azimuth + number(e)*elevation +
            number(aa)*azimuth**2 + number(ee)*elevation**2
            for value, a, e, aa, ee in zip(area, plan['spad_azimuth_slope'],
                plan['spad_elevation_slope'], plan.get('spad_azimuth_quadratic', [0,0,0]),
                plan.get('spad_elevation_quadratic', [0,0,0]))]


def radiation_acceleration(plan,state,sun,decimal=False):
    number=D if decimal else float
    delta=[a-b for a,b in zip(state[:3],sun)]
    distance_squared=sum(value*value for value in delta)
    distance=distance_squared.sqrt() if decimal else cmath.sqrt(distance_squared)
    fraction=illumination(plan,state,sun,decimal)
    pressure=number(plan['flux_w_m2'])/number(299792458)
    if plan.get('shape') == 'SPADFile':
        coefficient=pressure*number('.001')*number(plan['nominal_sun_km'])**2/number(plan['mass_kg'])
        axes=body_axes(plan,state,decimal)
        incoming=[sum(-value*axis for value,axis in zip(delta,basis)) for basis in axes]
        area=spad_body_area(plan,incoming,decimal)
        inertial_area=[sum(area[j]*axes[j][i] for j in range(3)) for i in range(3)]
        return [coefficient*fraction*number(plan['spad_scale'])*value/distance**2 for value in inertial_area]
    if plan.get('shape') == 'NPlate':
        if 'plate_type' in plan:
            coefficient=pressure*number('.001')*number(plan['nominal_sun_km'])**2/number(plan['mass_kg'])
            return [-coefficient*fraction*value/distance**2
                    for value in plate_reflectance(plan,state,sun,decimal)]
        coefficient=(2*number(plan['plate_area_m2'])*number(plan['plate_area_coefficient'])/
            number(plan['mass_kg'])*pressure*number('.001')*number(plan['nominal_sun_km'])**2)
        return [coefficient*fraction*value/distance**3 for value in delta]
    coefficient=number(plan['cr'])*number(plan['area_m2'])/number(plan['mass_kg'])*pressure*number('.001')*number(plan['nominal_sun_km'])**2
    return [coefficient*fraction*value/distance**3 for value in delta]


def drag_acceleration(plan,state,omega,decimal=False):
    number=D if decimal else float
    rotation=cross(omega,state[:3])
    relative=[value-spin for value,spin in zip(state[3:6],rotation)]
    squared=sum(value*value for value in relative)
    speed=squared.sqrt() if decimal else cmath.sqrt(squared)
    density=number(plan['reference_density'])
    if plan.get('spherical_atmosphere'):
        radius_squared=sum(value*value for value in state[:3])
        radius=radius_squared.sqrt() if decimal else cmath.sqrt(radius_squared)
        exponent=-(radius-number(plan['earth_radius_km']))/number(plan['scale_height_km'])
        density*=exponent.exp() if decimal else cmath.exp(exponent)
    if plan.get('shape')=='SPADFile':
        axes=body_axes(plan,state,decimal)
        incoming=[sum(-value*axis for value,axis in zip(relative,basis)) for basis in axes]
        body_area=spad_body_area(plan,incoming,decimal)
        area=[sum(body_area[j]*axes[j][i] for j in range(3)) for i in range(3)]
        factor=-number(500)*density*number(plan.get('density_scale',1))*number(plan['spad_scale'])/number(plan['mass_kg'])
        return [factor*squared*value for value in area]
    factor=-number(500)*density*number(plan.get('density_scale',1))*number(plan['cd'])*number(plan['drag_area_m2'])/number(plan['mass_kg'])
    return [factor*speed*value for value in relative]


def finite_acceleration(plan, state, decimal=False):
    number = D if decimal else float
    direction = [number(value) for value in plan['thrust_direction']]
    squared = sum(value*value for value in direction)
    length = squared.sqrt() if decimal else cmath.sqrt(squared)
    axes = body_axes(plan, state, decimal)
    if plan.get('file_owner_frame'):
        radial_squared = sum(value*value for value in state[:3])
        radial_length = radial_squared.sqrt() if decimal else cmath.sqrt(radial_squared)
        radial = [value/radial_length for value in state[:3]]
        momentum = cross(state[:3], state[3:6])
        momentum_squared = sum(value*value for value in momentum)
        momentum_length = momentum_squared.sqrt() if decimal else cmath.sqrt(momentum_squared)
        normal = [value/momentum_length for value in momentum]
        axes = [radial, cross(normal, radial), normal]
    inertial = [sum(direction[j]*axes[j][i] for j in range(3))/length for i in range(3)]
    factor = number('.001')*number(plan['thrust_newtons'])/number(plan['mass_kg'])
    return [factor*value for value in inertial]


def acceleration(plan,state,sun,decimal=False):
    if plan.get('force_type') == 'ExternalModel':
        number = D if decimal else float
        return [sum(number(plan['external_position_matrix'][i][j])*state[j] +
                    number(plan['external_velocity_matrix'][i][j])*state[j+3] for j in range(3)) +
                number(plan['external_constant'][i]) for i in range(3)]
    if plan.get('force_type') == 'PolyhedronGravityModel':
        from StmPolyhedronOracle import cube_acceleration
        return cube_acceleration(plan,state,sun,decimal)
    number=D if decimal else float
    radius_squared=sum(value*value for value in state[:3])
    radius=radius_squared.sqrt() if decimal else cmath.sqrt(radius_squared)
    if plan.get('force_type') == 'DragForce':
        radiation=drag_acceleration(plan,state,sun,decimal)
    elif plan.get('force_type') in ('FiniteThrust', 'FileThrust'):
        radiation=finite_acceleration(plan,state,decimal)
    else:
        radiation=radiation_acceleration(plan,state,sun,decimal)
    return [-number(plan['mu'])*value/radius**3+srp for value,srp in zip(state[:3],radiation)]


def integrate(plan, poses, radius, duration, step):
    state = initial_state(plan, sun_at(poses, 0), radius)
    state += [float(i == j) for i in range(6) for j in range(6)]
    count = round(duration/step)
    require(count > 0 and abs(count*step-duration) < 1e-6, 'Independent endpoint grid differs')
    step = duration/count

    def derivative(time, value):
        current_plan = time_dependent_plan(plan, time)
        orientation_key = 'angular_velocity' if plan.get('force_type')=='DragForce' else 'earth_to_sun'
        if plan.get('force_type') == 'PolyhedronGravityModel':
            orientation_key = 'body_rotation'
        orientation = sun_at(poses, time, orientation_key)
        force = acceleration(current_plan, value, orientation)
        gradient = [[0.]*6 for _ in range(3)]
        for col in range(6):
            trial = list(value[:6])
            trial[col] += 1e-30j
            differential = acceleration(current_plan, trial, orientation)
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


def reference_plan(plan):
    return dict(plan,**plan.get('reference_ballistics',{}))


def time_dependent_plan(plan, seconds):
    if 'mass_flow_kg_s' not in plan:
        return plan
    return dict(plan, mass_kg=plan['mass_kg']-plan['mass_flow_kg_s']*seconds)


def frame_value(plan, spec, owner, guide, orientation):
    zero = [D(0)]*6
    primary = guide if spec['primary'] == 'Ref' else zero
    origin = guide if spec['origin'] == 'Ref' else zero
    position = sub(owner[:3], primary[:3])
    velocity = sub(owner[3:], primary[3:])
    force = acceleration(plan, owner, orientation, True)
    if spec['primary'] == 'Ref':
        force = sub(force, acceleration(reference_plan(plan), guide, orientation, True))
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
    value = [dot(axis, offset) for axis in (x, y, z)] + [
        dot(axis, relative_velocity)+dot(rate, offset)
        for axis, rate in zip((x, y, z), (x_rate, y_rate, z_rate))]
    if 'coordinate_permutation' in spec:
        transform = spec['coordinate_permutation']
        value = [sum(D(transform[i][j])*value[start+j] for j in range(3))
                 for start in (0,3) for i in range(3)]
    return value


def jacobian(plan, spec, owner, guide, orientation):
    owner, guide = [[D(value) for value in vector] for vector in (owner, guide)]
    orientation = [D(value) for value in orientation]
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
            'Spherical SRP STM shape differs')
    errors = [abs(actual[row][col]-expected[row][col]) for row in range(6) for col in range(6)]
    require(all(math.isfinite(error) and error <= bounds[2*(row//3)+col//3]
                for (row, col), error in zip(((i, j) for i in range(6) for j in range(6)), errors)),
            'Spherical SRP STM differs from independent variational flow')
    return max(errors)


def preflight(plan):
    sun=[149597870.691,0.,0.]
    state=initial_state(plan,sun,plan['radius'])
    with localcontext() as context:
        context.prec=80
        exact=radiation_acceleration(plan,[D(value) for value in state],[D(value) for value in sun],True)
        actual=radiation_acceleration(plan,state,sun)
        error=max(abs(value.real-float(wanted)) for value,wanted in zip(actual,exact))
    require(error<1e-14,'Independent radiation acceleration preflight failed')
    return dict(status='passed',maximum_double_decimal_acceleration_difference=error)


def audit(root, fixture):
    root = Path(root)
    plan = json.loads(Path(fixture).read_text())
    proof = json.loads((root/'proof.json').read_text())
    force = json.loads((root/'force-provider.json').read_text())
    pose_record = json.loads((root/'force-poses.json').read_text())
    poses = pose_record['samples']
    bounds = plan['matrix_block_tolerances']
    require(bounds==[2e-8,2e-6,2e-11,2e-8] and plan['position_tolerance_km']==2e-5 and
            plan['velocity_tolerance_km_s']==2e-8,'Frozen spherical SRP bounds differ')
    require(force['type']=='SolarRadiationPressure' and force['body']=='Earth' and
            force['shape']==plan.get('shape','Spherical') and force['flux']==plan['flux_w_m2'] and
            force['nominal_sun']==plan['nominal_sun_km'] and force['sun_radius']==plan['sun_radius_km'] and
            force['earth_radius']==plan['earth_radius_km'] and force['mass']==plan['mass_kg'] and
            force['area']==plan['area_m2'] and force['cr']==plan['cr'],'Actual spherical SRP provider differs')
    require(abs(force['flux_pressure']-plan['flux_w_m2']/299792458)<1e-18,'Radiation pressure conversion differs')
    if plan.get('shape') == 'SPADFile':
        axes=body_axes(plan,proof['owner_state'])
        require(force['spad_scale']==plan['spad_scale'] and
                force['spad_interpolation']==plan.get('interpolation','Bilinear') and
                all(abs(value-axes[i][j])<1e-14 for i,row in enumerate(force['attitude'])
                    for j,value in enumerate(row)),'SPAD scale or prescribed attitude differs')
        if plan.get('attitude') == 'NadirPointing':
            require(force['attitude_type']=='NadirPointing' and
                    plan['attitude_reference_body']=='Earth' and plan['attitude_constraint']=='OrbitNormal' and
                    plan['attitude_alignment']==[-1,0,0] and plan['attitude_body_constraint']==[0,1,0],
                    'Nadir-pointing configuration differs')
        records=[]
        for line in (root/'constant-vector.spo').read_text().splitlines():
            fields=line.split()
            if len(fields)==5:
                try: records.append([float(value) for value in fields])
                except ValueError: pass
        require(len(records)==2701,'SPAD record inventory differs')
        for row in records:
            expected=[value+a*row[0]+e*row[1] for value,a,e in zip(plan['spad_body_area_m2'],
                      plan.get('spad_azimuth_slope',[0,0,0]),plan.get('spad_elevation_slope',[0,0,0]))]
            require(row[2:]==expected,'SPAD area records differ')
    if plan.get('shape') == 'NPlate':
        plate=force['plate']
        require(plate['type']==plan.get('plate_type','SunFacing') and plate['area']==plan['plate_area_m2'] and
                plate['area_coefficient']==plan['plate_area_coefficient'] and
                plate['specular']==plan.get('plate_specular',1) and
                plate['diffuse']==plan.get('plate_diffuse',0),'N-plate configuration differs')
        if 'plate_type' in plan:
            require(plate['lit_fraction']==plan['plate_lit_fraction'] and
                    all(abs(a-b)<1e-14 for a,b in zip(plate['normal'],plan['plate_normal'])),
                    'N-plate normal or lit fraction differs')
            expected_attitude=body_axes(plan,proof['owner_state'])
            require(force['attitude_type']==plan['attitude'] and
                    all(abs(value-expected_attitude[i][j])<1e-14
                        for i,row in enumerate(force['attitude']) for j,value in enumerate(row)),
                    'N-plate attitude differs')
    plate_derivative_error = None
    if 'plate_derivative_tolerance' in plan:
        record = json.loads((root/'plate-derivatives.json').read_text())
        require(record['state']==proof['owner_state'], 'Plate derivative state differs')
        plate_derivative_error = 0.
        for route in ['body_route','inertial_route']:
            require(len(record[route])>=6 and all(len(row)==3 for row in record[route][:6]),
                    'Plate derivative inventory differs')
            for coordinate in range(6):
                trial=list(record['state'])
                trial[coordinate]+=1e-30j
                expected=plate_reflectance(plan,trial,record['sun'])
                for row in range(3):
                    error=abs(record[route][coordinate][row]-expected[row].imag/1e-30)
                    require(math.isfinite(error) and error<=plan['plate_derivative_tolerance'],
                            'Plate reflectance derivative differs')
                    plate_derivative_error=max(plate_derivative_error,error)
    attitude_derivative_error = None
    if 'attitude_derivative_tolerance' in plan:
        record = json.loads((root/'attitude-derivatives.json').read_text())
        require(record['state']==proof['owner_state'] and len(record['matrices'])==6,
                'Attitude derivative state or inventory differs')
        attitude_derivative_error = 0.
        for coordinate,actual in enumerate(record['matrices']):
            require(len(actual)==3 and all(len(row)==3 for row in actual),
                    'Attitude derivative matrix shape differs')
            trial=list(record['state'])
            trial[coordinate]+=1e-30j
            axes=body_axes(plan,trial)
            for row in range(3):
                for column in range(3):
                    error=abs(actual[row][column]-axes[column][row].imag/1e-30)
                    require(math.isfinite(error) and error<=plan['attitude_derivative_tolerance'],
                            'Nadir attitude rotation derivative differs')
                    attitude_derivative_error=max(attitude_derivative_error,error)
    duration=proof['integration_seconds']
    require(math.isfinite(duration) and abs(duration-60)<1e-6 and len(poses)==61,
            'Spherical SRP pose/endpoint scope differs')
    for index,pose in enumerate(poses):
        require(abs(pose['seconds']-index*duration/60)<1e-9 and len(pose['earth_to_sun'])==3 and
                all(math.isfinite(value) for value in pose['earth_to_sun']),'Invalid Sun ephemeris sample')
    initial=json.loads((root/'initial-states.json').read_text())
    for name,radius in [('Sat',plan['radius']),('Ref',plan['reference_radius'])]:
        require(all(abs(a-b)<1e-8 for a,b in zip(initial[name],initial_state(plan,sun_at(poses,0),radius))),
                'Frozen initial state construction differs')
    if plan.get('matrix_mode') == 'AMatrix':
        require(proof['matrix_mode']=='AMatrix', 'A-matrix-only route differs')
        actual=proof['force_amatrix']
        require(len(actual)==6 and all(len(row)==6 for row in actual), 'Native A-matrix shape differs')
        query=json.loads((root/'queried-gradient.json').read_text())
        require(query['available'], 'A-matrix captured gradient unavailable')
        state=proof['owner_state']
        sun=sun_at(poses,duration)
        expected_force=radiation_acceleration(plan,state,sun)
        require(all(abs(value-wanted.real)<=1e-12 for value,wanted in zip(query['acceleration'],expected_force)),
                'A-matrix acceleration differs')
        maximum=0.
        for column in range(6):
            trial=list(state)
            trial[column]+=1e-20j
            partial=radiation_acceleration(plan,trial,sun)
            for row in range(6):
                expected=0. if row<3 else partial[row-3].imag/1e-20
                error=abs(actual[row][column]-expected)
                require(math.isfinite(error) and error<=1e-20, 'Native SPAD A-matrix derivative differs')
                maximum=max(maximum,error)
            for row in range(3):
                require(abs(query['gradient'][row][column]-partial[row].imag/1e-20)<=1e-20,
                        'A-matrix capture derivative differs')
        return dict(status='passed',native_reads=0,independent_derivative_checks=36,
                    matrix_mode='AMatrix',max_native_derivative_error=maximum,
                    max_plate_derivative_error=plate_derivative_error,
                    native_tests_run_by_replay=0,new_requirement_credit=0,whole_row_qualified=False)
    truths = []
    maximum = 0.
    for radius, state_key, matrix_key in [(plan['radius'], 'owner_state', 'native_stm'),
                                         (plan['reference_radius'], 'guide_state', 'guide_stm')]:
        coarse_state, coarse_phi = integrate(plan, poses, radius, duration, plan['coarse_step_seconds'])
        state, phi = integrate(plan, poses, radius, duration, plan['fine_step_seconds'])
        require(all(abs(a-b) <= plan['refinement_state_bounds'][0 if i < 3 else 1]
                    for i, (a, b) in enumerate(zip(state, coarse_state))), 'Spherical SRP state refinement failed')
        matrix_errors(phi, coarse_phi, plan['refinement_matrix_block_bounds'])
        actual = proof[state_key]
        require(len(actual) == 6 and all(math.isfinite(value) and abs(value-state[i]) <=
                (plan['position_tolerance_km'] if i < 3 else plan['velocity_tolerance_km_s'])
                for i, value in enumerate(actual)), 'Spherical SRP trajectory differs from independent potential')
        maximum = max(maximum, matrix_errors(proof[matrix_key], phi, bounds))
        truths.append((state, phi))
    expected = {}
    with localcontext() as context:
        context.prec = 80
        initial_owner = initial_state(plan,sun_at(poses,0),plan['radius'])
        initial_guide = initial_state(plan,sun_at(poses,0),plan['reference_radius'])
        final_owner, final_guide = truths[0][0], truths[1][0]
        initial_sun, final_sun = sun_at(poses,0), sun_at(poses,duration)
        flow = truths[0][1]
        if plan.get('frame_input_mode') == 'propagated_native_matrix':
            # The frame conversion consumes this matrix and these endpoints.
            # Their dynamics are independently checked above before using them
            # as inputs to the separate Cartesian frame-differential check.
            initial_owner, initial_guide = initial['Sat'], initial['Ref']
            final_owner, final_guide = proof['owner_state'], proof['guide_state']
            flow = proof['native_stm']
            frame_sun = json.loads((root/'frame-sun-inputs.json').read_text())
            initial_sun, final_sun = frame_sun['initial'], frame_sun['final']
        for spec in plan['cases']:
            initial = jacobian(plan, spec, initial_owner, initial_guide, initial_sun)
            final = jacobian(plan, spec, final_owner, final_guide, final_sun)
            transformed = mul(final, mul([[D(value) for value in row] for row in flow], inverse(initial)))
            expected[spec['key']] = [[float(value) for value in row] for row in transformed]
    observations = proof['observations']
    if any('axis_labels' in spec for spec in plan['cases']):
        selected = json.loads((root/'axis-configuration.json').read_text())
        require(selected == {spec['key']:spec['axis_labels'] for spec in plan['cases']},
                'Executed signed-axis selectors differ from the frozen inventory')
    require([(row['case'], row['type'], row['provider']) for row in observations] ==
            [(spec['key'], kind, provider) for spec in plan['cases'] for kind in TYPES for provider in PROVIDERS],
            'Spherical SRP Parameter route inventory differs')
    for row in observations:
        require(all(row.get(key, False) for key in ('owner_unchanged', 'guide_unchanged', 'cache_atomic',
                'readonly', 'owner_bound', 'frame_bound')), 'Spherical SRP binding or read isolation differs')
        index = TYPES.index(row['type'])
        size = 6 if index == 0 else 3
        first_row, first_col = (0, 0) if index == 0 else (3*((index-1)//2), 3*((index-1)%2))
        actual = row['actual']
        truth = expected[row['case']]
        require(not row['rejected'] and len(actual) == size and all(len(value) == size for value in actual),
                'Defined spherical SRP frame rejected or shape differs')
        require(all(math.isfinite(actual[i][j]) and abs(actual[i][j]-truth[i+first_row][j+first_col]) <=
                    bounds[2*((i+first_row)//3)+(j+first_col)//3]
                    for i in range(size) for j in range(size)),
                'Spherical SRP owner-frame STM differs from independent Cartesian differential')
    query=json.loads((root/'queried-gradient.json').read_text())
    require(query['available'] and len(query['gradient'])==3 and all(len(row)==6 for row in query['gradient']),
            'Spherical SRP captured gradient is unavailable')
    state=proof['owner_state'];sun=final_sun
    expected=radiation_acceleration(plan,state,sun)
    require(all(abs(value-wanted.real)<=1e-12 for value,wanted in zip(query['acceleration'],expected)),
            'Spherical SRP sampled acceleration differs')
    fraction=float(complex(illumination(plan,state,sun)).real)
    require(abs(query['illumination']-fraction)<1e-9,'Spherical SRP illumination differs')
    domain=plan['illumination_domain']
    require((domain=='penumbra' and 0.05<fraction<.95) or
            (domain=='full_sun' and fraction==1) or (domain=='umbra' and fraction==0) or
            (domain=='annular' and 0<fraction<1),'Unexpected illumination domain')
    gradient_error=0.
    for col in range(6):
        trial=list(state);trial[col]+=1e-20j
        expected=radiation_acceleration(plan,trial,sun)
        for row in range(3):
            error=abs(query['gradient'][row][col]-expected[row].imag/1e-20)
            gradient_bound=1e-20 if plan.get('shape') in ('SPADFile','NPlate') else 1e-12
            require(error<=gradient_bound,'SRP captured derivative differs')
            gradient_error=max(gradient_error,error)
    force_description = {
        'SPADFile': 'affine angular SPAD area and inverse-square pressure',
        'NPlate': 'one-sided plate optical reflection and inverse-square pressure',
        'Spherical': 'conical disk-overlap spherical radiation pressure',
    }[plan.get('shape','Spherical')]
    shared_inputs = 'Sun ephemeris and native endpoint states for force/frame differential checks'
    if plan.get('frame_input_mode') == 'propagated_native_matrix':
        shared_inputs += '; native STM for the separate frame-conversion check'
    return dict(status='passed',native_reads=len(observations),defined_reads=len(observations),
                force_provider='SolarRadiationPressure',illumination_domain=domain,illumination=fraction,
                max_native_stm_error=maximum,max_captured_gradient_error=gradient_error,
                max_attitude_derivative_error=attitude_derivative_error,
                max_plate_derivative_error=plate_derivative_error,
                frame_input_mode=plan.get('frame_input_mode','independent_flow'),
                independent_cartesian_dynamics_checked=True,
                srp_shape=plan.get('shape','Spherical'),
                independent_force=force_description,
                shared_native_inputs=shared_inputs,independent_ephemeris_qualified=False,
                native_tests_run_by_replay=0,new_requirement_credit=0,whole_row_qualified=False)
