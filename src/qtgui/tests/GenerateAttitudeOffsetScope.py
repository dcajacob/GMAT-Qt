"""Compose fixed-offset witnesses for every original FRR-35 origin/axis cell.

No native GMAT output or attitude/frame helper is used as oracle input. Existing
independent frame bases and their original bounds are retained verbatim; an
independent passive Rodrigues rotation supplies the new frame-to-body offset.
This catalog is a test partition, not evidence that any cell has passed.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent
ORIGINS = [('Spacecraft', 'Vehicle', 428), ('CelestialBody', 'Earth', 429),
           ('Barycenter', 'Center', 430), ('LibrationPoint', 'LP', 431),
           ('GroundStation', 'Site', 432)]
AXES = [('MJ2000Eq', 434), ('MJ2000Ec', 435), ('TOEEq', 436),
        ('TOEEc', 437), ('MOEEq', 438), ('MOEEc', 439), ('MODEq', 440),
        ('MODEc', 441), ('BodyFixed', 442), ('BodyInertial', 443),
        ('GSM', 444), ('GSE', 445), ('ObjectReferenced', 446),
        ('Topocentric', 447), ('TODEq', 448), ('TODEc', 449)]


def multiply(a, b):
    return [[math.fsum(a[i][k] * b[k][j] for k in range(3))
             for j in range(3)] for i in range(3)]


def generate():
    paths = [HERE / 'NasaAttitudeDeclaredFrameCases.json',
             HERE / 'NasaNamedFrameConsumerCases.json']
    declared, named = [json.loads(p.read_text()) for p in paths]
    frames = {c['key']: copy.deepcopy(c) for c in declared['cases']}
    named_cases = {c['key']: c for c in named['cases']}
    identity = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
    zero = [[0, 0, 0] for _ in range(3)]
    frames['MJ2000Eq'] = copy.deepcopy(named_cases['MJ2000Eq'])
    epsilon = math.radians(84381.448 / 3600)
    frames['MJ2000Ec'] = copy.deepcopy(frames['MJ2000Eq'])
    frames['MJ2000Ec']['axis'] = 'MJ2000Ec'
    for sample in frames['MJ2000Ec']['samples']:
        sample['rows'] = [[1, 0, 0], [0, math.cos(epsilon), math.sin(epsilon)],
                          [0, -math.sin(epsilon), math.cos(epsilon)]]
    frames['BodyInertial'] = copy.deepcopy(frames['BodyInertialCustom'])
    frames['BodyFixed'] = copy.deepcopy(named_cases['BodyFixed'])
    for sample in frames['BodyFixed']['samples']:
        # Prescribed pole is exactly the inertial Z direction. The existing
        # independent basis includes its pinned A1/TDB phase convention.
        sample['angular_velocity_degrees_s'] = [0, 0, 120 / 86400]
        w = math.radians(120 / 86400)
        rows = sample['rows']
        sample['basis_rates'] = [[w*x for x in rows[1]],
                                 [-w*x for x in rows[0]], [0, 0, 0]]
    # Two distinct inertial spacecraft define a nonsingular oblique R/N frame.
    # Reports sample their unpropagated instantaneous Cartesian states. Relative
    # velocity is +Z and relative acceleration is zero: derive the complete
    # instantaneous basis derivative independently, without a frame helper.
    r = [7000, -1200, 900]
    x = [v / math.sqrt(math.fsum(t*t for t in r)) for v in r]
    normal = [x[1], -x[0], 0]
    z = [v / math.sqrt(math.fsum(t*t for t in normal)) for v in normal]
    y = [z[1]*x[2]-z[2]*x[1], z[2]*x[0]-z[0]*x[2],
         z[0]*x[1]-z[1]*x[0]]
    magnitude = math.sqrt(math.fsum(t*t for t in r))
    dx = [((1 if i == 2 else 0)-x[i]*x[2])/magnitude for i in range(3)]
    dy = [z[1]*dx[2]-z[2]*dx[1], z[2]*dx[0]-z[0]*dx[2],
          z[0]*dx[1]-z[1]*dx[0]]
    rows, derivatives = [x, y, z], [dx, dy, [0, 0, 0]]
    angular = [-math.fsum(derivatives[a][k]*rows[b][k] for k in range(3))*180/math.pi
               for a, b in [(2, 1), (0, 2), (1, 0)]]
    frames['ObjectReferenced'] = dict(axis='ObjectReferenced', matrix_bound=2e-10,
        samples=[dict(epoch=e, rows=rows, basis_rates=derivatives,
                      angular_velocity_degrees_s=angular)
                 for e in [21545, 25300.25, 30000.75]])
    u = [v / math.sqrt(14) for v in [1, 2, 3]]
    angle = math.pi / 5
    c, s = math.cos(angle), math.sin(angle)
    cross = [[0, -u[2], u[1]], [u[2], 0, -u[0]], [-u[1], u[0], 0]]
    rotation = [[c*identity[i][j] + (1-c)*u[i]*u[j] - s*cross[i][j]
                 for j in range(3)] for i in range(3)]
    q = [v*math.sin(angle/2) for v in u] + [math.cos(angle/2)]
    common = frames['MOEEq']['setup'].split('CreatedFrame.Axes =')[0]
    common += ('Create LibrationPoint LP;\nLP.Primary = Earth;\n'
               'LP.Secondary = Luna;\nLP.Point = L1;\n'
               'Vehicle.CoordinateSystem = EarthMJ2000Eq;\n'
               'Vehicle.DisplayStateType = Cartesian;\n'
               'Vehicle.X = 8000;\nVehicle.Y = 1200;\nVehicle.Z = -400;\n'
               'Create Spacecraft AxisPrimary AxisSecondary;\n')
    for name, position in [('AxisPrimary', [1000, 2000, -500]),
                           ('AxisSecondary', [8000, 800, 400])]:
        common += (f'{name}.CoordinateSystem = EarthMJ2000Eq;\n'
                   f'{name}.DisplayStateType = Cartesian;\n')
        velocity = [0, 0, 1] if name == 'AxisSecondary' else [0, 0, 0]
        for field, value in zip(['X', 'Y', 'Z', 'VX', 'VY', 'VZ'], position+velocity):
            common += f'{name}.{field} = {value};\n'
    cases, exclusions = [], []
    for origin, native_origin, origin_id in ORIGINS:
        for axis, axis_id in AXES:
            cell = dict(origin_requirement=f'FRR!E{origin_id}',
                        axis_requirement=f'FRR!E{axis_id}', axis=axis,
                        origin_class=origin, key=f'{origin}_{axis}')
            if (axis in ('BodyFixed', 'BodyInertial') and origin != 'CelestialBody'
                    or axis == 'Topocentric' and origin != 'GroundStation'):
                exclusions.append(cell)
                continue
            frame = frames[axis]
            if axis in ('BodyFixed', 'BodyInertial', 'Topocentric'):
                setup = frame['setup']
            else:
                setup = common + f'CreatedFrame.Origin = {native_origin};\nCreatedFrame.Axes = {axis};\n'
                if axis in ('MOEEq', 'MOEEc', 'TOEEq', 'TOEEc'):
                    setup += 'CreatedFrame.Epoch = 25300.25;\n'
                if axis == 'ObjectReferenced':
                    setup += ('CreatedFrame.Primary = AxisPrimary;\n'
                              'CreatedFrame.Secondary = AxisSecondary;\n'
                              'CreatedFrame.XAxis = R;\nCreatedFrame.ZAxis = N;\n')
            cell.update(setup=setup, reference='CreatedFrame',
                        matrix_bound=frame['matrix_bound'], rate_bound_degrees_s=5e-9,
                        rate_convention=frame.get('rate_convention',
                            'Independent prescribed inertial/geometric basis derivative'),
                        frame_oracle_key=frame.get('key', axis), samples=[])
            for old in frame['samples']:
                rate = old.get('angular_velocity_degrees_s', [0, 0, 0])
                cell['samples'].append(dict(epoch=old['epoch'],
                    frame_rows=old['rows'], frame_basis_rates=old.get('basis_rates', zero),
                    frame_angular_velocity_degrees_s=rate,
                    rows=multiply(rotation, old['rows']),
                    angular_velocity_degrees_s=[math.fsum(rotation[i][j]*rate[j]
                                                          for j in range(3)) for i in range(3)]))
            cases.append(cell)
    assert len(cases) == 68 and len(exclusions) == 12
    return dict(scope='All 68 source-permitted FRR-35 origin/axis cells with a nonzero fixed offset. '
                'The 12 original body/station restrictions alone are excluded; no native limitation '
                'changes applicability. Actual execution and all whole-row gates remain separate.',
                input_state=declared['input_state'], numeric_sentinel=41,
                offset_quaternion=q, offset_matrix=rotation,
                offset_oracle='Independent passive Rodrigues: 36 degrees about normalized [1,2,3].',
                source_sha256={p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                physical_inputs=declared['physical_inputs'],
                original_combinations=80, original_explicit_exclusions=exclusions,
                cases=cases)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    with args.output.open('x') as stream:
        json.dump(generate(), stream, indent=2, ensure_ascii=False)
        stream.write('\n')
