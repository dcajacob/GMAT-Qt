"""Independent point-mass propagation and one-step range least squares.

Station inertial coordinates are explicitly shared input definitions, exported
separately. No GMAT propagation, state-transition, measurement or solver routine
is called. This does not independently qualify station/frame/time physics.
"""
from pathlib import Path
from decimal import Decimal
import csv, json, math, re
import numpy as np

MU=398600.4415
RADIUS=7000.

def stumpff(z):
    z=np.asarray(z);c=np.empty_like(z);s=np.empty_like(z)
    near=np.abs(z)<1e-4;positive=(z>0)&~near;negative=(z<0)&~near
    q=z[near]
    c[near]=.5-q/24+q*q/720-q**3/40320+q**4/3628800
    s[near]=1/6-q/120+q*q/5040-q**3/362880+q**4/39916800
    a=np.sqrt(z[positive]);c[positive]=(1-np.cos(a))/z[positive];s[positive]=(a-np.sin(a))/(a**3)
    a=np.sqrt(-z[negative]);c[negative]=(np.cosh(a)-1)/(-z[negative]);s[negative]=(np.sinh(a)-a)/(a**3)
    return c,s

def propagate(initial,times):
    initial=np.asarray(initial,dtype=float);times=np.atleast_1d(times).astype(float)
    r0=np.linalg.norm(initial[:3]);vr=np.dot(initial[:3],initial[3:])/np.sqrt(MU)
    alpha=2/r0-np.dot(initial[3:],initial[3:])/MU
    chi=np.sqrt(MU)*alpha*times
    for _ in range(30):
        z=alpha*chi*chi;c,s=stumpff(z)
        f=vr*chi*chi*c+(1-alpha*r0)*chi**3*s+r0*chi-np.sqrt(MU)*times
        derivative=vr*chi*(1-z*s)+(1-alpha*r0)*chi*chi*c+r0
        step=f/derivative;chi-=step
        if np.max(np.abs(step))<1e-11:break
    else:raise AssertionError('universal anomaly did not converge')
    z=alpha*chi*chi;c,s=stumpff(z)
    f=1-chi*chi*c/r0;g=times-chi**3*s/np.sqrt(MU)
    position=f[:,None]*initial[:3]+g[:,None]*initial[3:]
    radius=np.linalg.norm(position,axis=1)
    fdot=np.sqrt(MU)/(radius*r0)*(alpha*chi**3*s-chi)
    gdot=1-chi*chi*c/radius
    velocity=fdot[:,None]*initial[:3]+gdot[:,None]*initial[3:]
    return np.hstack((position,velocity))

def circle(times):
    t=np.atleast_1d(times);angle=np.sqrt(MU/RADIUS**3)*t;v=np.sqrt(MU/RADIUS);c=np.sqrt(.5)
    return np.array([RADIUS*np.cos(angle),RADIUS*np.sin(angle)*c,RADIUS*np.sin(angle)*c,
                     -v*np.sin(angle),v*np.cos(angle)*c,v*np.cos(angle)*c]).T

def load_inputs(observation_file,station_file,report_file):
    observations=[]
    for line in Path(observation_file).read_text().splitlines():
        if not line.strip()or line.startswith('%'):continue
        f=line.split();assert len(f)==6 and f[1]=='Range_Skin'
        t=float((Decimal(f[0])-Decimal('26088.5'))*86400-Decimal('34'))
        observations.append(dict(t=t,tai=f[0],station=f[3],observed=float(f[5])))
    with Path(station_file).open() as stream:
        station=list(csv.DictReader(stream))
    assert len(station)==len(observations)==358
    for geometry,observation in zip(station,observations):
        assert geometry['tai_mjd']==observation['tai']and geometry['station']==observation['station']
        observation['position']=list(map(float,[geometry[k]for k in ['x','y','z']]))
    reported=[]
    for line in Path(report_file).read_text().splitlines():
        if re.match(r'^\s+0\s+\d+\s+\d\d [A-Z][a-z]{2} 2012 .*Range_Skin',line):
            f=line.split();flag=''if len(f)==12 else f[8];offset=8+bool(flag)
            reported.append(dict(index=int(f[1]),station=f[7].split(',')[0],edit=flag,
                                 observed=float(f[offset]),computed=float(f[offset+1]),residual=float(f[offset+2])))
    assert len(reported)==358 and [x['index']for x in reported]==list(range(358))
    return observations,reported

def ranges(initial,times,stations):
    # Range_Skin retains both equal geometric legs with light time disabled.
    # The native adapter uses the Range multiplier (one), not a half-range.
    return 2*np.linalg.norm(propagate(initial,times)[:,:3]-stations,axis=1)

def one_step(initial,times,stations,observed,mask,step_multiplier=1.):
    initial=np.asarray(initial);computed=ranges(initial,times,stations)
    residual=observed-computed;steps=np.array([.001]*3+[1e-7]*3)*step_multiplier
    jacobian=np.empty((len(times),6))
    for component,step in enumerate(steps):
        delta=np.zeros(6);delta[component]=step
        jacobian[:,component]=(ranges(initial+delta,times,stations)-ranges(initial-delta,times,stations))/(2*step)
    scale=np.array([1.,1.,1.,.001,.001,.001])
    design=jacobian[mask]*scale
    solution,residual_sum,rank,singular=np.linalg.lstsq(design,residual[mask],rcond=None)
    assert rank==6
    correction=solution*scale
    return dict(state=(initial+correction).tolist(),correction=correction.tolist(),
                scaled_condition=float(singular[0]/singular[-1]),records=int(np.sum(mask)),
                residual_rms=float(np.sqrt(np.mean(residual[mask]**2))),
                linearized_residual_rms=float(np.sqrt(np.mean((jacobian[mask]@correction-residual[mask])**2))))

def analyze(observation_file,station_file,report_file):
    observations,reported=load_inputs(observation_file,station_file,report_file)
    times=np.array([x['t']for x in observations]);stations=np.array([x['position']for x in observations]);observed=np.array([x['observed']for x in observations])
    truth=circle([0])[0];initial=truth+np.array([1,-.5,.25,.0005,-.0003,.0002])
    truth_range=2*np.linalg.norm(circle(times)[:,:3]-stations,axis=1)
    computed=ranges(initial,times,stations);residual=observed-computed
    accepted=np.array([not x['edit']for x in reported]);predicted_accepted=np.abs(residual)<=1000*.01
    return dict(records=len(observations),accepted_records=int(accepted.sum()),
                edit_flags={k:sum(x['edit']==k for x in reported)for k in sorted({x['edit']for x in reported})},
                geometry_radius_min=float(np.linalg.norm(stations,axis=1).min()),geometry_radius_max=float(np.linalg.norm(stations,axis=1).max()),
                truth_observation_max_error_km=float(np.max(np.abs(truth_range-observed))),
                native_computed_max_error_km=float(np.max(np.abs(computed-np.array([x['computed']for x in reported])))),
                native_residual_max_error_km=float(np.max(np.abs(residual-np.array([x['residual']for x in reported])))),
                predicted_acceptance_matches_native=bool(np.array_equal(accepted,predicted_accepted)),
                native_mask_steps=[one_step(initial,times,stations,observed,accepted,m)for m in [.5,1.,2.]],
                all_record_steps=[one_step(initial,times,stations,observed,np.ones(len(times),dtype=bool),m)for m in [.5,1.,2.]],
                initial=initial.tolist(),truth=truth.tolist())

def validate_reference(result):
    assert result['records']==358 and result['accepted_records']==58
    assert result['truth_observation_max_error_km']<2e-6
    assert result['native_computed_max_error_km']<2e-6
    assert result['native_residual_max_error_km']<2e-6
    assert result['predicted_acceptance_matches_native']
    states=np.array([x['state']for x in result['native_mask_steps']])
    spread=np.ptp(states,axis=0)
    assert np.max(spread[:3])<5e-6 and np.max(spread[3:])<5e-9
    assert all(x['records']==58 and x['scaled_condition']<100 for x in result['native_mask_steps'])
    return True

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser();parser.add_argument('observations');parser.add_argument('stations');parser.add_argument('report');parser.add_argument('--output',required=True)
    args=parser.parse_args();result=analyze(args.observations,args.stations,args.report);validate_reference(result)
    Path(args.output).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items()if k not in ['native_mask_steps','all_record_steps']},indent=2))
