"""Independent affine-system dynamics and external-force frame differentials."""
from decimal import localcontext
import json
import math
from pathlib import Path

import numpy as np

import StmSrpOracle as common


def exponential(matrix):
    result = np.eye(matrix.shape[0])
    term = result.copy()
    for order in range(1,80):
        term = term @ matrix / order
        result += term
        if np.max(np.abs(term)) < 1e-18:
            return result
    raise ValueError('Affine matrix exponential did not converge')


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
        'Frozen external-force state or STM bounds differ')
    common.require(provider == dict(type='ExternalModel',module=plan['external_module'],
        entry_point='GetDerivatives',dimension=84), 'Executed external-force provider differs')
    equation = {key:value for key,value in plan.items() if key.startswith('external_')}
    common.require(json.loads((root/'external-equation.json').read_text()) == equation,
                   'External-force coefficients differ')
    module = plan['external_module']+'.py'
    common.require((root/module).read_bytes() == (Path(fixture).parent/module).read_bytes(),
                   'Executed external callback source differs')
    layout = json.loads((root/'external-callback-layout.json').read_text())
    common.require(layout['state_count'] == layout['description_count'] == 84 and
        len(layout['cartesian_starts']) == 2 and
        {layout['descriptions'][i].split('.',1)[0] for i in layout['cartesian_starts']} == {'Sat','Ref'},
        'Full external-force callback layout differs')
    duration = proof['integration_seconds']
    common.require(abs(duration-60) < 1e-6 and len(poses) == 61, 'External-force interval differs')
    augmented = np.zeros((7,7))
    augmented[:3,3:6] = np.eye(3)
    augmented[3:6,:3] = plan['external_position_matrix']
    augmented[3:6,3:6] = plan['external_velocity_matrix']
    augmented[3:6,6] = plan['external_constant']
    transition = exponential(augmented*duration)
    maximum = 0.0
    for name,radius,state_key,matrix_key in [('Sat',plan['radius'],'owner_state','native_stm'),
        ('Ref',plan['reference_radius'],'guide_state','guide_stm')]:
        expected = common.initial_state(plan,common.sun_at(poses,0),radius)
        common.require(all(abs(a-b) < 1e-8 for a,b in zip(initial[name],expected)),
                       'External-force initial state differs')
        coarse_state,coarse_phi = common.integrate(plan,poses,radius,duration,plan['coarse_step_seconds'])
        state,phi = common.integrate(plan,poses,radius,duration,plan['fine_step_seconds'])
        common.require(all(abs(a-b) <= plan['refinement_state_bounds'][0 if i<3 else 1]
            for i,(a,b) in enumerate(zip(state,coarse_state))), 'External RK4 integration did not converge')
        common.matrix_errors(phi,coarse_phi,plan['refinement_matrix_block_bounds'])
        analytic = transition @ np.array(initial[name]+[1.0])
        common.require(all(abs(a-b) < (1e-8 if i<3 else 1e-11)
            for i,(a,b) in enumerate(zip(state,analytic[:6]))), 'External RK4 and affine solution differ')
        common.matrix_errors(phi,transition[:6,:6].tolist(),plan['refinement_matrix_block_bounds'])
        common.require(all(math.isfinite(a) and abs(a-b) <=
            (plan['position_tolerance_km'] if i<3 else plan['velocity_tolerance_km_s'])
            for i,(a,b) in enumerate(zip(proof[state_key],state))),
            'Native external-force state differs from affine solution')
        if plan.get('matrix_mode') != 'AMatrix':
            maximum = max(maximum,common.matrix_errors(proof[matrix_key],phi,bounds))
    gradient = [a+b for a,b in zip(plan['external_position_matrix'],plan['external_velocity_matrix'])]
    if plan.get('callback_error_checks'):
        rejection = json.loads((root/'callback-rejections.json').read_text())
        common.require(rejection['recovered'] and rejection['state_unchanged'] and
            [(row['entry'],row['native']) for row in rejection['cases']] ==
            [(entry,native) for entry in ['BadLength','NonFinite','NonNumeric','Raises'] for native in [False,True]] and
            all(row['rejected'] and row['atomic'] and row['message'] for row in rejection['cases']),
            'External callback rejection or recovery evidence differs')
    if plan.get('matrix_mode') == 'AMatrix':
        for key in ['force_amatrix','guide_amatrix']:
            actual = proof[key]
            for row in range(6):
                for column in range(6):
                    expected = 0 if row<3 else gradient[row-3][column]
                    common.require(abs(actual[row][column]-expected) <= plan['amatrix_tolerance'],
                                   'External native A-matrix differs from affine derivative')
        return dict(status='passed',native_reads=0,independent_derivative_checks=72,
                    callback_rejections=8 if plan.get('callback_error_checks') else 0,
                    new_requirement_credit=0,whole_row_qualified=False,native_tests_run_by_replay=0)
    inputs = json.loads((root/'frame-sun-inputs.json').read_text())
    reference = json.loads((root/'reference-acceleration.json').read_text())
    for key,name in [('owner_acceleration','Sat'),('guide_acceleration','Ref')]:
        expected = common.acceleration(plan,initial[name],inputs['initial'])
        common.require(all(abs(a-b) < 1e-13 for a,b in zip(reference[key],expected)),
                       'Initial external-force frame acceleration differs')
    truths = {}
    with localcontext() as context:
        context.prec = 80
        for spec in plan['cases']:
            start = common.jacobian(plan,spec,initial['Sat'],initial['Ref'],inputs['initial'])
            end = common.jacobian(plan,spec,proof['owner_state'],proof['guide_state'],inputs['final'])
            flow = [[common.D(value) for value in row] for row in proof['native_stm']]
            truths[spec['key']] = common.mul(end,common.mul(flow,common.inverse(start)))
    observations = proof['observations']
    common.require([(row['case'],row['type'],row['provider']) for row in observations] ==
        [(spec['key'],kind,provider) for spec in plan['cases'] for kind in common.TYPES for provider in common.PROVIDERS],
        'External-force frame route inventory differs')
    for row in observations:
        common.require(all(row.get(key,False) for key in ['owner_unchanged','guide_unchanged',
            'cache_atomic','readonly','owner_bound','frame_bound']) and not row['rejected'],
            'External-force frame binding or isolation failed')
        index = common.TYPES.index(row['type'])
        size = 6 if index==0 else 3
        first_row,first_col = (0,0) if index==0 else (3*((index-1)//2),3*((index-1)%2))
        actual = row['actual']
        common.require(len(actual)==size and all(len(value)==size for value in actual),
                       'External-force frame matrix shape differs')
        truth = truths[row['case']]
        common.require(all(math.isfinite(actual[i][j]) and abs(actual[i][j]-float(truth[i+first_row][j+first_col])) <=
            bounds[2*((i+first_row)//3)+(j+first_col)//3] for i in range(size) for j in range(size)),
            'External-force frame differential differs')
    query = json.loads((root/'queried-gradient.json').read_text())
    common.require(query['available'], 'External-force captured gradient is unavailable')
    expected = common.acceleration(plan,proof['owner_state'],inputs['final'])
    common.require(all(abs(a-b) < 1e-13 for a,b in zip(query['acceleration'],expected)),
                   'External-force sampled acceleration differs')
    error = max(abs(query['gradient'][i][j]-gradient[i][j]) for i in range(3) for j in range(6))
    common.require(error <= plan['gradient_tolerance'], 'External-force sampled gradient differs')
    return dict(status='passed',native_reads=len(observations),max_native_stm_error=maximum,
                max_captured_gradient_error=error,new_requirement_credit=0,
                whole_row_qualified=False,native_tests_run_by_replay=0)
