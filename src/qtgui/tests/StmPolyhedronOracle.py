"""Uniform-cube gravity from volume quadrature, independent of face/edge formulas."""
from decimal import localcontext
from functools import lru_cache
from itertools import product
import json
import math
from pathlib import Path

import numpy as np

import StmSrpOracle as common


@lru_cache(maxsize=None)
def quadrature(order):
    nodes, weights = np.polynomial.legendre.leggauss(order)
    points = []
    values = []
    for indices in product(range(order), repeat=3):
        points.append([float(nodes[index]) for index in indices])
        values.append(float(np.prod([weights[index] for index in indices])))
    return np.array(points), np.array(values)


def cube_acceleration(plan, state, orientation, decimal=False):
    points, weights = quadrature(plan['quadrature_order'])
    half = plan['cube_half_length_km']
    factor = plan['gravity_constant_km3_kg_s2']*1e9*plan['density_kg_m3']*half**3
    if decimal:
        rotation = [[common.D(orientation[3*i+j]) for j in range(3)] for i in range(3)]
        position = [sum(rotation[i][j]*state[j] for j in range(3)) for i in range(3)]
        acceleration = [common.D(0)]*3
        for point, weight in zip(points, weights):
            delta = [common.D(float(point[i]))*common.D(half)-position[i] for i in range(3)]
            radius = sum(value*value for value in delta).sqrt()
            for i in range(3):
                acceleration[i] += common.D(float(weight))*delta[i]/radius**3
        return [common.D(factor)*sum(rotation[j][i]*acceleration[j] for j in range(3)) for i in range(3)]
    rotation = np.array(orientation).reshape(3,3)
    delta = half*points - rotation @ np.array(state[:3])
    distance_cubed = np.sum(delta*delta, axis=1)**1.5
    acceleration = np.sum(weights[:,None]*delta/distance_cubed[:,None], axis=0)
    return (factor*(rotation.T @ acceleration)).tolist()


def audit(root, fixture):
    root = Path(root)
    plan = json.loads(Path(fixture).read_text())
    proof = json.loads((root/'proof.json').read_text())
    provider = json.loads((root/'force-provider.json').read_text())
    poses = json.loads((root/'force-poses.json').read_text())['samples']
    initial = json.loads((root/'initial-states.json').read_text())
    bounds = plan['matrix_block_tolerances']
    common.require(bounds == [2e-8,2e-6,2e-11,2e-8] and
        plan['position_tolerance_km'] == 2e-5 and plan['velocity_tolerance_km_s'] == 2e-8,
        'Frozen polyhedron state or STM bounds differ')
    common.require(provider == dict(type='PolyhedronGravityModel', density=plan['density_kg_m3'],
        shape_file=str(root/'uniform-cube.txt')), 'Executed polyhedron provider differs')
    lines = (root/'uniform-cube.txt').read_text().splitlines()
    common.require(int(lines[0]) == 8 and int(lines[9]) == 12, 'Polyhedron shape counts differ')
    common.require([[float(value) for value in line.split()[1:]] for line in lines[1:9]] ==
        plan['polyhedron_vertices_km'], 'Polyhedron vertices differ')
    common.require([[int(value) for value in line.split()[1:]] for line in lines[10:]] ==
        plan['polyhedron_faces'], 'Polyhedron faces differ')
    common.require(all(abs(value) == plan['cube_half_length_km'] for point in plan['polyhedron_vertices_km']
        for value in point), 'Volume oracle requires the frozen uniform cube')
    duration = proof['integration_seconds']
    common.require(abs(duration-60) < 1e-6 and len(poses) == 61, 'Polyhedron integration interval differs')
    refined = dict(plan, quadrature_order=plan['refined_quadrature_order'])
    maximum = 0.0
    for name, radius, state_key, matrix_key in [
        ('Sat', plan['radius'], 'owner_state', 'native_stm'),
        ('Ref', plan['reference_radius'], 'guide_state', 'guide_stm')]:
        expected = common.initial_state(plan, common.sun_at(poses,0), radius)
        common.require(all(abs(a-b) < 1e-8 for a,b in zip(initial[name], expected)),
                       'Polyhedron initial state differs')
        coarse_state, coarse_phi = common.integrate(plan, poses, radius, duration, plan['coarse_step_seconds'])
        state, phi = common.integrate(refined, poses, radius, duration, plan['fine_step_seconds'])
        common.require(all(abs(a-b) <= plan['refinement_state_bounds'][0 if i < 3 else 1]
            for i,(a,b) in enumerate(zip(state,coarse_state))), 'Volume integration did not converge')
        common.matrix_errors(phi, coarse_phi, plan['refinement_matrix_block_bounds'])
        common.require(all(math.isfinite(a) and abs(a-b) <=
            (plan['position_tolerance_km'] if i < 3 else plan['velocity_tolerance_km_s'])
            for i,(a,b) in enumerate(zip(proof[state_key],state))),
            'Native polyhedron state differs from independent volume integration')
        if plan.get('matrix_mode') != 'AMatrix':
            maximum = max(maximum, common.matrix_errors(proof[matrix_key], phi, bounds))

    orientation = common.sun_at(poses,duration,'body_rotation')
    gradient = [[0.]*6 for _ in range(3)]
    for column in range(6):
        trial = list(proof['owner_state'])
        trial[column] += 1e-30j
        value = cube_acceleration(refined,trial,orientation)
        for row in range(3):
            gradient[row][column] = value[row].imag/1e-30
    if plan.get('matrix_mode') == 'AMatrix':
        entries = 0
        for state_key, matrix_key in [('owner_state','force_amatrix'),('guide_state','guide_amatrix')]:
            actual = proof[matrix_key]
            for column in range(6):
                trial = list(proof[state_key])
                trial[column] += 1e-30j
                value = cube_acceleration(refined,trial,orientation)
                for row in range(6):
                    expected = 0 if row < 3 else value[row-3].imag/1e-30
                    common.require(abs(actual[row][column]-expected) <= plan['amatrix_tolerance'],
                                   'Polyhedron native A-matrix differs from volume derivative')
                    entries += 1
        return dict(status='passed', native_reads=0, independent_derivative_checks=entries,
                    new_requirement_credit=0, whole_row_qualified=False, native_tests_run_by_replay=0)

    inputs = json.loads((root/'frame-sun-inputs.json').read_text())
    reference = json.loads((root/'reference-acceleration.json').read_text())
    for key, name in [('owner_acceleration','Sat'),('guide_acceleration','Ref')]:
        expected = cube_acceleration(refined,initial[name],inputs['initial'])
        common.require(all(abs(a-b) < 1e-13 for a,b in zip(reference[key],expected)),
                       'Initial polyhedron frame acceleration differs from volume integration')
    truths = {}
    with localcontext() as context:
        context.prec = 80
        for spec in plan['cases']:
            start = common.jacobian(refined,spec,initial['Sat'],initial['Ref'],inputs['initial'])
            end = common.jacobian(refined,spec,proof['owner_state'],proof['guide_state'],inputs['final'])
            flow = [[common.D(value) for value in row] for row in proof['native_stm']]
            truths[spec['key']] = common.mul(end,common.mul(flow,common.inverse(start)))
    observations = proof['observations']
    common.require([(row['case'],row['type'],row['provider']) for row in observations] ==
        [(spec['key'],kind,provider) for spec in plan['cases'] for kind in common.TYPES for provider in common.PROVIDERS],
        'Polyhedron matrix route inventory differs')
    for row in observations:
        common.require(all(row.get(key,False) for key in ['owner_unchanged','guide_unchanged',
            'cache_atomic','readonly','owner_bound','frame_bound']) and not row['rejected'],
            'Polyhedron frame binding, isolation or evaluation failed')
        index = common.TYPES.index(row['type'])
        size = 6 if index == 0 else 3
        first_row, first_col = (0,0) if index == 0 else (3*((index-1)//2),3*((index-1)%2))
        actual = row['actual']
        common.require(len(actual) == size and all(len(value) == size for value in actual),
                       'Polyhedron frame matrix shape differs')
        truth = truths[row['case']]
        common.require(all(math.isfinite(actual[i][j]) and abs(actual[i][j]-float(truth[i+first_row][j+first_col])) <=
            bounds[2*((i+first_row)//3)+(j+first_col)//3] for i in range(size) for j in range(size)),
            'Polyhedron frame differential differs from volume integration')
    query = json.loads((root/'queried-gradient.json').read_text())
    common.require(query['available'], 'Polyhedron captured gradient is unavailable')
    expected = cube_acceleration(refined,proof['owner_state'],orientation)
    common.require(all(abs(a-b) < 1e-13 for a,b in zip(query['acceleration'],expected)),
                   'Polyhedron sampled acceleration differs from volume integration')
    error = max(abs(query['gradient'][i][j]-gradient[i][j]) for i in range(3) for j in range(6))
    common.require(error <= plan['gradient_tolerance'], 'Polyhedron sampled gradient differs from volume derivative')
    return dict(status='passed', native_reads=len(observations), max_native_stm_error=maximum,
                max_captured_gradient_error=error, new_requirement_credit=0,
                whole_row_qualified=False, native_tests_run_by_replay=0)
