"""Independent constant-mass finite-thrust flow and frame differentials."""
from decimal import localcontext
import json
import math
from pathlib import Path

import StmSrpOracle as common


def audit(root, fixture):
    root = Path(root)
    plan = json.loads(Path(fixture).read_text())
    proof = json.loads((root / 'proof.json').read_text())
    provider = json.loads((root / 'force-provider.json').read_text())
    initial = json.loads((root / 'initial-states.json').read_text())
    poses = json.loads((root / 'force-poses.json').read_text())['samples']
    bounds = plan['matrix_block_tolerances']
    common.require(bounds == [2e-8, 2e-6, 2e-11, 2e-8] and
        plan['position_tolerance_km'] == 2e-5 and plan['velocity_tolerance_km_s'] == 2e-8,
        'Frozen finite-thrust state and STM bounds differ')
    duration = proof['integration_seconds']
    endpoint = common.time_dependent_plan(plan, duration)
    expected_provider = dict(type='FiniteThrust', mass=endpoint['mass_kg'],
        reference_mass=plan['reference_ballistics']['mass_kg'],
        thrust_newtons=plan['thrust_newtons'], direction=plan['thrust_direction'],
        axes=plan['thrust_axes'], decrement_mass=plan.get('decrement_mass', False), duty_cycle=1, scale_factor=1,
        attitude=provider['attitude'])
    if plan.get('force_type') == 'FileThrust':
        expected_provider = dict(type='FileThrust', mass=plan['mass_kg'],
            reference_mass=plan['reference_ballistics']['mass_kg'], mode='ModelAccelOnly',
            axes=plan['thrust_axes'], acceleration_m_s2=plan['file_acceleration_m_s2'],
            attitude=provider['attitude'])
        expected = common.finite_acceleration(dict(plan, file_owner_frame=False), proof['owner_state'])
        common.require(all(abs(value-1000*component.real) < 1e-15
            for value,component in zip(plan['file_acceleration_m_s2'],expected)),
            'Frozen file-acceleration units differ')
        fields = [line.split() for line in (root / 'constant-acceleration.thf').read_text().splitlines()]
        data = [row for row in fields if row and row[0] in ('0','172800')]
        common.require(len(data) == 2 and all([float(value) for value in row[1:]] ==
            plan['file_acceleration_m_s2'] for row in data), 'Executed thrust data differs')
    if plan.get('decrement_mass'):
        common.require(plan['mass_tolerance_kg'] == 1e-9 and
            abs(plan['mass_flow_kg_s']-plan['thrust_newtons']/(plan['isp_s']*plan['gravity_m_s2'])) < 1e-16,
            'Frozen mass-flow model differs')
        expected_fuel = plan['fuel_mass_kg']-plan['mass_flow_kg_s']*duration
        common.require(abs(provider['mass']-endpoint['mass_kg']) <= plan['mass_tolerance_kg'] and
            abs(provider['fuel_mass']-expected_fuel) <= plan['mass_tolerance_kg'],
            'Native integrated mass differs from independent mass flow')
        expected_provider.update(mass=provider['mass'], fuel_mass=provider['fuel_mass'],
            dry_mass=plan['mass_kg']-plan['fuel_mass_kg'], isp=plan['isp_s'], gravity_accel=plan['gravity_m_s2'])
    common.require(provider == expected_provider, 'Executed finite-thrust configuration differs')
    axes = common.body_axes(plan, proof['owner_state'])
    common.require(all(abs(value-axes[i][j]) < 1e-14
        for i, row in enumerate(provider['attitude']) for j, value in enumerate(row)),
        'Finite-thrust attitude differs')
    common.require(abs(duration-60) < 1e-6 and len(poses) == 61,
                   'Finite-thrust integration interval differs')
    maximum = 0
    for name, radius, state_key, matrix_key, subject in [
        ('Sat', plan['radius'], 'owner_state', 'native_stm', plan),
        ('Ref', plan['reference_radius'], 'guide_state', 'guide_stm', common.reference_plan(plan))]:
        expected_initial = common.initial_state(plan, common.sun_at(poses, 0), radius)
        common.require(all(abs(a-b) < 1e-8 for a, b in zip(initial[name], expected_initial)),
                       'Finite-thrust initial state differs')
        coarse_state, coarse_phi = common.integrate(subject, poses, radius, duration,
                                                   plan['coarse_step_seconds'])
        state, phi = common.integrate(subject, poses, radius, duration, plan['fine_step_seconds'])
        common.require(all(abs(a-b) <= plan['refinement_state_bounds'][0 if i < 3 else 1]
            for i, (a, b) in enumerate(zip(state, coarse_state))),
            'Independent finite-thrust propagation did not converge')
        common.matrix_errors(phi, coarse_phi, plan['refinement_matrix_block_bounds'])
        common.require(all(math.isfinite(a) and abs(a-b) <=
            (plan['position_tolerance_km'] if i < 3 else plan['velocity_tolerance_km_s'])
            for i, (a, b) in enumerate(zip(proof[state_key], state))),
            'Native finite-thrust state differs from independent propagation')
        if plan.get('matrix_mode') != 'AMatrix':
            maximum = max(maximum, common.matrix_errors(proof[matrix_key], phi, bounds))

    if plan.get('matrix_mode') == 'AMatrix':
        maximum = 0
        entries = 0
        for state_key, matrix_key, subject in [('owner_state', 'force_amatrix', endpoint)]:
            actual = proof[matrix_key]
            common.require(len(actual) == 6 and all(len(row) == 6 for row in actual),
                           'Finite-thrust A-matrix shape differs')
            for column in range(6):
                trial = list(proof[state_key])
                trial[column] += 1e-30j
                expected = common.finite_acceleration(subject, trial)
                for row in range(6):
                    derivative = 0 if row < 3 else expected[row-3].imag/1e-30
                    error = abs(actual[row][column]-derivative)
                    common.require(error <= plan['amatrix_tolerance'],
                                   'Finite-thrust A-matrix derivative differs')
                    maximum = max(maximum, error)
                    entries += 1
        return dict(status='passed', native_reads=0, independent_derivative_checks=entries,
                    max_native_derivative_error=maximum, new_requirement_credit=0,
                    whole_row_qualified=False, native_tests_run_by_replay=0)

    inputs = json.loads((root / 'frame-sun-inputs.json').read_text())
    common.require(plan['initial_frame_force'] == 'powered at first propagated STM state',
                   'Finite-thrust initial frame reference is unspecified')
    initial_plan = plan
    reference = json.loads((root / 'reference-acceleration.json').read_text())
    for key, subject, state in [('owner_acceleration', initial_plan, initial['Sat']),
                               ('guide_acceleration', common.reference_plan(plan), initial['Ref'])]:
        expected = common.acceleration(subject, state, inputs['initial'])
        common.require(all(abs(a-b.real) < 1e-13 for a,b in zip(reference[key], expected)),
                       'Initial frame acceleration differs from the powered reference')
    truths = {}
    with localcontext() as context:
        context.prec = 80
        for spec in plan['cases']:
            start = common.jacobian(initial_plan, spec, initial['Sat'], initial['Ref'], inputs['initial'])
            end = common.jacobian(endpoint, spec, proof['owner_state'], proof['guide_state'], inputs['final'])
            flow = [[common.D(value) for value in row] for row in proof['native_stm']]
            transformed = common.mul(end, common.mul(flow, common.inverse(start)))
            truths[spec['key']] = [[float(value) for value in row] for row in transformed]
    observations = proof['observations']
    common.require([(row['case'], row['type'], row['provider']) for row in observations] ==
        [(spec['key'], kind, provider) for spec in plan['cases']
            for kind in common.TYPES for provider in common.PROVIDERS],
        'Finite-thrust frame route inventory differs')
    for row in observations:
        common.require(all(row.get(key, False) for key in ['owner_unchanged', 'guide_unchanged',
            'cache_atomic', 'readonly', 'owner_bound', 'frame_bound']),
            'Finite-thrust frame binding or read isolation differs')
        common.require(not row['rejected'], 'Defined finite-thrust frame was rejected')
        index = common.TYPES.index(row['type'])
        size = 6 if index == 0 else 3
        first_row, first_col = (0, 0) if index == 0 else (3*((index-1)//2), 3*((index-1)%2))
        actual = row['actual']
        truth = truths[row['case']]
        common.require(len(actual) == size and all(len(value) == size for value in actual),
                       'Finite-thrust frame matrix shape differs')
        common.require(all(math.isfinite(actual[i][j]) and
            abs(actual[i][j]-truth[i+first_row][j+first_col]) <=
            bounds[2*((i+first_row)//3)+(j+first_col)//3]
            for i in range(size) for j in range(size)), 'Finite-thrust frame differential differs')
    query = json.loads((root / 'queried-gradient.json').read_text())
    common.require(query['available'], 'Finite-thrust captured gradient is unavailable')
    expected = common.finite_acceleration(endpoint, proof['owner_state'])
    common.require(all(abs(a-b.real) <= 1e-12 for a, b in zip(query['acceleration'], expected)),
                   'Finite-thrust sampled acceleration differs')
    gradient_error = 0
    for column in range(6):
        trial = list(proof['owner_state'])
        trial[column] += 1e-30j
        expected = common.finite_acceleration(endpoint, trial)
        for row in range(3):
            error = abs(query['gradient'][row][column]-expected[row].imag/1e-30)
            common.require(error <= plan['gradient_tolerance'], 'Finite-thrust captured derivative differs')
            gradient_error = max(gradient_error, error)
    return dict(status='passed', native_reads=len(observations), max_native_stm_error=maximum,
                max_captured_gradient_error=gradient_error, force_provider=plan['force_type'],
                native_tests_run_by_replay=0, new_requirement_credit=0, whole_row_qualified=False)
