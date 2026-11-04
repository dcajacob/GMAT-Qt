"""SI drag law, prescribed exponential density and independent RK4; no GMAT imports."""
from pathlib import Path
import argparse, json, math

def density(f,state):
    altitude=math.hypot(*state[:3])-f['earth_radius_km']
    return f['reference_density_kg_m3']*math.exp(-altitude/f['scale_height_km'])
def drag(f,key,state,omega):
    r=state[:3];v=state[3:]
    cross=[omega[1]*r[2]-omega[2]*r[1],omega[2]*r[0]-omega[0]*r[2],omega[0]*r[1]-omega[1]*r[0]]
    relative=[a-b for a,b in zip(v,cross)];speed=math.hypot(*relative)
    common=-.5*density(f,state)*1000/f['dry_mass_kg']
    if key=='Spherical':return [common*f['cd']*f['drag_area_m2']*speed*x for x in relative]
    return [common*speed**2*f['spad_scale']*a for a in f['spad_body_area_m2']]
def rhs(f,key,state,omega):
    r=math.hypot(*state[:3]);a=drag(f,key,state,omega)
    return state[3:]+[-f['mu_km3_s2']*x/r**3+y for x,y in zip(state[:3],a)]
def integrate(f,key,omega,h):
    state=[f['orbit_radius_km'],0.,0.,0.,math.sqrt(f['mu_km3_s2']/f['orbit_radius_km']),0.]
    n=round(f['duration_seconds']/h);assert abs(n*h-f['duration_seconds'])<1e-12
    for _ in range(n):
        k1=rhs(f,key,state,omega)
        k2=rhs(f,key,[a+h*b/2 for a,b in zip(state,k1)],omega)
        k3=rhs(f,key,[a+h*b/2 for a,b in zip(state,k2)],omega)
        k4=rhs(f,key,[a+h*b for a,b in zip(state,k3)],omega)
        state=[a+h*(b+2*c+2*d+e)/6 for a,b,c,d,e in zip(state,k1,k2,k3,k4)]
    return state
def validate(f,key,r,raw):
    assert key in ['Spherical','SPADFile']and r['case']==r['selected_model']==key
    rows=[list(map(float,l.split(',')))for l in raw.strip().splitlines()[:2]]
    assert raw.strip().splitlines()[-1]=='42'and rows==r['rows']and all(len(v)==8 and all(map(math.isfinite,v))for v in rows)
    assert rows[0][-1]==rows[1][-1]==41 and abs(rows[0][0])<1e-6 and abs(rows[1][0]-f['duration_seconds'])<1e-6
    start=[f['orbit_radius_km'],0,0,0,math.sqrt(f['mu_km3_s2']/f['orbit_radius_km']),0]
    assert max(abs(a-b)for a,b in zip(rows[0][1:7],start))<1e-9
    state=rows[1][1:7];omega=r['angular_velocity_rad_s'];assert len(omega)==3 and all(map(math.isfinite,omega))and 7e-5<math.hypot(*omega)<8e-5
    predicted=density(f,state);wanted=drag(f,key,state,omega);aerr=max(abs(a-b)for a,b in zip(wanted,r['native_acceleration']))
    abound=math.hypot(*wanted)*f['acceleration_relative_bound']+f['acceleration_absolute_bound_km_s2']
    derr=abs(predicted-r['native_density_kg_m3']);dbound=predicted*f['density_relative_bound']+f['density_absolute_bound_kg_m3']
    integrated=integrate(f,key,omega,f['oracle_step_seconds']);refined=integrate(f,key,omega,f['oracle_step_seconds']/2)
    errors=[abs(a-b)for a,b in zip(state,refined)];refinement=[abs(a-b)for a,b in zip(integrated,refined)]
    assert max(refinement[:3])<1e-8 and max(refinement[3:])<1e-10
    gates=dict(native_force=aerr<abound,density=derr<dbound,position=max(errors[:3])<f['position_bound_km'],velocity=max(errors[3:])<f['velocity_bound_km_s'])
    return dict(case=key,passed=all(gates.values()),gates=gates,expected_acceleration=wanted,native_acceleration=r['native_acceleration'],max_acceleration_error=aerr,acceleration_bound=abound,expected_density_kg_m3=predicted,density_error=derr,density_bound=dbound,independent_state=refined,state_component_errors=errors,RK4_refinement_errors=refinement,shared_native_angular_velocity_input=True,empirical_atmosphere_qualified=False)
if __name__=='__main__':
    p=argparse.ArgumentParser()
    for n in ['cases','key','runtime','values','output']:p.add_argument('--'+n,required=True)
    a=p.parse_args();result=validate(json.loads(Path(a.cases).read_text()),a.key,json.loads(Path(a.runtime).read_text()),Path(a.values).read_text());Path(a.output).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));assert result['passed'],'Independent drag force/density/integration gate failed'
