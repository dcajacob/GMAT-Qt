"""Independent drag, variational flow and frame checks."""
from decimal import localcontext
import json
import math
from pathlib import Path

import StmSrpOracle as common


def preflight(plan):
    state=[7000.,100.,300.,-.3,7.5,1.]
    omega=[1e-8,-2e-8,7.292115e-5]
    with localcontext() as context:
        context.prec=80
        exact=common.drag_acceleration(plan,[common.D(v) for v in state],
                                       [common.D(v) for v in omega],True)
        actual=common.drag_acceleration(plan,state,omega)
        error=max(abs(a.real-float(b)) for a,b in zip(actual,exact))
        common.require(error<1e-15,'Independent drag force preflight failed')
    return dict(status='passed',double_decimal_force_error=error)


def audit(root,fixture):
    root=Path(root)
    plan=json.loads(Path(fixture).read_text())
    proof=json.loads((root/'proof.json').read_text())
    provider=json.loads((root/'force-provider.json').read_text())
    poses=json.loads((root/'force-poses.json').read_text())['samples']
    initial=json.loads((root/'initial-states.json').read_text())
    bounds=plan['matrix_block_tolerances']
    common.require(bounds==[2e-8,2e-6,2e-11,2e-8] and
                   plan['position_tolerance_km']==2e-5 and plan['velocity_tolerance_km_s']==2e-8,
                   'Frozen drag state/STM bounds differ')
    expected_provider=dict(type='DragForce',body='Earth',shape=plan.get('shape','Spherical'),
                   atmosphere='Exponential',mass=plan['mass_kg'],cd=plan['cd'],area=plan['drag_area_m2'])
    if plan.get('shape')=='SPADFile':
        axes=common.body_axes(plan,proof['owner_state'])
        common.require(all(abs(v-axes[i][j])<1e-14 for i,row in enumerate(provider['attitude']) for j,v in enumerate(row)),
                       'SPAD drag attitude differs')
        expected_provider.update(spad_scale=plan['spad_scale'],interpolation=plan.get('interpolation','Bilinear'),
                                 attitude=provider['attitude'],attitude_type=plan['attitude'])
        records=[]
        for line in (root/'constant-vector.spo').read_text().splitlines():
            fields=line.split()
            if len(fields)==5:
                try:
                    records.append([float(v) for v in fields])
                except ValueError:
                    pass
        common.require(len(records)==2701, 'SPAD drag table row count differs')
        for row in records:
            azimuth, elevation = row[:2]
            expected = [value+a*azimuth+e*elevation+aa*azimuth**2+ee*elevation**2
                for value,a,e,aa,ee in zip(plan['spad_body_area_m2'],
                    plan.get('spad_azimuth_slope',[0,0,0]),
                    plan.get('spad_elevation_slope',[0,0,0]),
                    plan.get('spad_azimuth_quadratic',[0,0,0]),
                    plan.get('spad_elevation_quadratic',[0,0,0]))]
            common.require(all(abs(a-b)<=1e-12 for a,b in zip(row[2:],expected)),
                           'SPAD drag table differs from the frozen polynomial fixture')
    if 'reference_ballistics' in plan:
        expected_provider['reference_ballistics']=plan['reference_ballistics']
    if plan.get('controlled_gravity'):
        common.require(abs(provider['gravity_mu']-plan['mu'])<1e-9,
                       'Controlled monopole constant differs')
        expected_provider.update(gravity_mu=provider['gravity_mu'],gravity_degree=0,gravity_order=0)
    if plan.get('spherical_atmosphere'):
        radius=math.sqrt(sum(value*value for value in proof['owner_state'][:3]))
        expected_density=plan['reference_density']*math.exp(-(radius-plan['earth_radius_km'])/plan['scale_height_km'])*plan.get('density_scale',1)
        common.require(abs(provider['density']-expected_density)<=1e-12*expected_density+1e-25,
                       'Native altitude-dependent density differs')
        if 'minimum_density' in plan:
            common.require(plan['minimum_density']<=provider['density']<=plan['maximum_density'],
                           'Atmosphere case does not exercise the frozen density range')
        expected_provider.update(density=provider['density'],earth_radius=plan['earth_radius_km'],earth_flattening=0)
    common.require(provider==expected_provider,
                   'Executed drag configuration differs')
    upper_density=plan['reference_density']*math.exp(-10000/plan['scale_height_km'])
    expected_table=[[0,plan['reference_density'],plan['scale_height_km']],
                    [10000,upper_density,plan['scale_height_km']]]
    table=[[float(value) for value in row.split(',')] for row in (root/'density.txt').read_text().splitlines()]
    common.require(len(table)==2 and all(len(row)==3 for row in table) and
                   all(abs(a-b)<=max(abs(b)*1e-14,1e-300) for row,wanted in zip(table,expected_table)
                       for a,b in zip(row,wanted)),'Synthetic atmosphere table differs')
    common.require(plan['scale_height_km']==1e300 or plan.get('spherical_atmosphere'),
                   'Finite scale height requires the declared spherical atmosphere')
    duration=proof['integration_seconds']
    common.require(abs(duration-60)<1e-6 and len(poses)==61,'Drag propagation interval differs')
    for index,pose in enumerate(poses):
        common.require(abs(pose['seconds']-index*duration/60)<1e-9 and
                       len(pose['angular_velocity'])==3 and
                       all(math.isfinite(v) for v in pose['angular_velocity']),
                       'Drag rotation samples differ')
    for name,radius in [('Sat',plan['radius']),('Ref',plan['reference_radius'])]:
        wanted=common.initial_state(plan,common.sun_at(poses,0),radius)
        common.require(all(abs(a-b)<1e-8 for a,b in zip(initial[name],wanted)),
                       'Drag initial state differs')
    maximum=0.
    for radius,state_key,matrix_key in [(plan['radius'],'owner_state','native_stm'),
                                         (plan['reference_radius'],'guide_state','guide_stm')]:
        subject=common.reference_plan(plan) if state_key=='guide_state' else plan
        coarse_state,coarse_phi=common.integrate(subject,poses,radius,duration,plan['coarse_step_seconds'])
        state,phi=common.integrate(subject,poses,radius,duration,plan['fine_step_seconds'])
        common.require(all(abs(a-b)<=plan['refinement_state_bounds'][0 if i<3 else 1]
                           for i,(a,b) in enumerate(zip(state,coarse_state))),
                       'Drag independent integration did not converge')
        common.matrix_errors(phi,coarse_phi,plan['refinement_matrix_block_bounds'])
        common.require(all(math.isfinite(a) and abs(a-b)<=(2e-5 if i<3 else 2e-8)
                           for i,(a,b) in enumerate(zip(proof[state_key],state))),
                       'Drag propagated state differs from independent integration')
        if plan.get('matrix_mode')!='AMatrix':
            maximum=max(maximum,common.matrix_errors(proof[matrix_key],phi,bounds))
    if plan.get('matrix_mode')=='AMatrix':
        common.require(proof['matrix_mode']=='AMatrix','Drag A-matrix route differs')
        omega=common.sun_at(poses,duration,'angular_velocity')
        maximum=0.
        entries=0
        for state_key,matrix_key,subject in [('owner_state','force_amatrix',plan),
                ('guide_state','guide_amatrix',common.reference_plan(plan))]:
            value=proof[matrix_key]
            common.require(len(value)==6 and all(len(row)==6 for row in value),
                           'Drag A-matrix shape differs')
            for column in range(6):
                trial=list(proof[state_key]);trial[column]+=1e-30j
                expected=common.drag_acceleration(subject,trial,omega)
                for row in range(6):
                    derivative=0. if row<3 else expected[row-3].imag/1e-30
                    error=abs(value[row][column]-derivative)
                    common.require(math.isfinite(error) and error<=plan['amatrix_tolerance'],
                                   'Drag A-matrix derivative differs')
                    maximum=max(maximum,error)
                    entries+=1
        return dict(status='passed',native_reads=0,independent_derivative_checks=entries,
                    matrix_mode='AMatrix',max_native_derivative_error=maximum,
                    source='two actual propagated spacecraft A-matrix blocks',
                    native_tests_run_by_replay=0,new_requirement_credit=0,whole_row_qualified=False)
    inputs=json.loads((root/'frame-force-inputs.json').read_text())
    truths={}
    with localcontext() as context:
        context.prec=80
        for spec in plan['cases']:
            start=common.jacobian(plan,spec,initial['Sat'],initial['Ref'],inputs['initial'])
            end=common.jacobian(plan,spec,proof['owner_state'],proof['guide_state'],inputs['final'])
            transformed=common.mul(end,common.mul([[common.D(v) for v in row]
                                   for row in proof['native_stm']],common.inverse(start)))
            truths[spec['key']]=[[float(v) for v in row] for row in transformed]
    observations=proof['observations']
    common.require([(row['case'],row['type'],row['provider']) for row in observations]==
                   [(spec['key'],kind,provider) for spec in plan['cases']
                    for kind in common.TYPES for provider in common.PROVIDERS],
                   'Drag frame route inventory differs')
    for row in observations:
        common.require(all(row.get(key,False) for key in ['owner_unchanged','guide_unchanged',
                       'cache_atomic','readonly','owner_bound','frame_bound']),
                       'Drag frame binding or read isolation differs')
        common.require(not row['rejected'],'Defined drag owner frame was rejected')
        index=common.TYPES.index(row['type'])
        size=6 if index==0 else 3
        first_row,first_col=(0,0) if index==0 else (3*((index-1)//2),3*((index-1)%2))
        actual=row['actual'];truth=truths[row['case']]
        common.require(len(actual)==size and all(len(v)==size for v in actual),'Drag matrix shape differs')
        common.require(all(math.isfinite(actual[i][j]) and abs(actual[i][j]-truth[i+first_row][j+first_col])<=
                       bounds[2*((i+first_row)//3)+(j+first_col)//3] for i in range(size) for j in range(size)),
                       'Drag frame differential differs')
    query=json.loads((root/'queried-gradient.json').read_text())
    common.require(query['available'],'Drag captured gradient is unavailable')
    state=proof['owner_state'];omega=inputs['final']
    wanted=common.drag_acceleration(plan,state,omega)
    common.require(all(abs(a-b.real)<=1e-12 for a,b in zip(query['acceleration'],wanted)),
                   'Drag sampled force differs')
    gradient_error=0.
    for col in range(6):
        trial=list(state);trial[col]+=1e-30j
        wanted=common.drag_acceleration(plan,trial,omega)
        for row in range(3):
            error=abs(query['gradient'][row][col]-wanted[row].imag/1e-30)
            common.require(error<=plan['gradient_tolerance'],'Drag captured derivative differs')
            gradient_error=max(gradient_error,error)
    if plan.get('zero_speed_gradient'):
        zero=json.loads((root/'zero-speed-gradient.json').read_text())
        common.require(zero['available'] and zero['live_spacecraft_unchanged'],
                       'Zero-relative-speed gradient is unavailable or mutated the live spacecraft')
        common.require(all(abs(value)<=1e-24 for value in zero['acceleration']) and
                       len(zero['gradient'])==3 and all(len(row)==6 for row in zero['gradient']) and
                       all(abs(value)<=1e-13 for row in zero['gradient'] for value in row),
                       'Zero-relative-speed drag differential differs')
    return dict(status='passed',native_reads=len(observations),max_native_stm_error=maximum,
                max_captured_gradient_error=gradient_error,force_provider='DragForce',
                shared_environment_inputs='Native atmosphere angular velocity',
                independent_empirical_atmosphere_qualified=False,
                frame_input_mode='propagated_native_matrix',native_tests_run_by_replay=0,
                new_requirement_credit=0,whole_row_qualified=False)
