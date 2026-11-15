"""Independent frozen R/Rdot transport of the retained fixed-pole NASA model.

No native frame or conversion API supplies expected values. Force-model truth
is the retained NASA prototype, not a claim of independent physical averaging.
"""
from pathlib import Path
import hashlib,json,math
from BrouwerPrototypeOracle import adapt,cartesian
ROOT=Path(__file__).resolve().parents[3]
MU=398600.4418
PI=math.pi

def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def scale(v,s):return [x*s for x in v]
def subtract(a,b):return [x-y for x,y in zip(a,b)]
def norm(v):return math.sqrt(dot(v,v))
def matrix(theta,tilt):
 c,s,C,S=math.cos(theta),math.sin(theta),math.cos(tilt),math.sin(tilt)
 return [[c,-s*C,s*S],[s,c*C,-c*S],[0,S,C]]
def mul(m,v):return [dot(row,v)for row in m]
def transpose(m):return [list(row)for row in zip(*m)]
def local(value,theta,tilt,rate):
 rotation=transpose(matrix(theta,tilt));r=value[:3];v=subtract(value[3:],cross([0,0,rate],r))
 return mul(rotation,r)+mul(rotation,v)
def internal(value,theta,tilt,rate):
 rotation=matrix(theta,tilt);r=mul(rotation,value[:3]);v=mul(rotation,value[3:]);spin=cross([0,0,rate],r)
 return r+[x+y for x,y in zip(v,spin)]
def elements(state):
 r,v=state[:3],state[3:];R=norm(r);h=cross(r,v);H=norm(h);normal=scale(h,1/H)
 ecc=subtract(scale(cross(v,h),1/MU),scale(r,1/R));e=norm(ecc);p=scale(ecc,1/e)
 n=[-normal[1],normal[0],0];N=norm(n);n=scale(n,1/N)
 a=1/(2/R-dot(v,v)/MU);i=math.atan2(N,normal[2]);node=math.atan2(n[1],n[0])
 peri=math.atan2(dot(cross(n,p),normal),dot(n,p));nu=math.atan2(dot(cross(p,scale(r,1/R)),normal),dot(p,scale(r,1/R)))
 E=math.atan2(math.sqrt(1-e*e)*math.sin(nu),e+math.cos(nu));M=E-e*math.sin(E)
 return [a,e,math.degrees(i),math.degrees(node)%360,math.degrees(peri)%360,math.degrees(M)%360]
def fixture():
 cases=[]
 for kind,prototype in [('Short','BROLYD_short.m'),('Long','BROLYD.m')]:
  forward,_,_=adapt(ROOT/'prototype/StateConv'/prototype)
  for inclination in [47.,63.2,118.]:
   earth_mean=[10800.,.17,inclination,112.,38.,143.]
   mean_cart=cartesian(earth_mean,MU);osc=cartesian(forward(earth_mean),MU)
   for degrees,tilt_degrees in [(37.,23.4392911111111),(123.,-37.),(219.,72.)]:
    for rate in [0.,.00003]:
     theta,tilt=map(math.radians,[degrees,tilt_degrees]);local_mean=elements(local(mean_cart,theta,tilt,rate))
     assert 0<local_mean[1]<.99 and local_mean[0]*(1-local_mean[1])>3000
     recovered=elements(internal(cartesian(local_mean,MU),theta,tilt,rate))
     errors=[abs(a-b) if i<3 else abs(math.remainder(a-b,360))for i,(a,b)in enumerate(zip(recovered,earth_mean))]
     assert max(errors)<1e-9,errors
     writes=[]
     for index,delta in enumerate([30.,.01,1.,2.,3.,4.]):
      updated=list(local_mean);updated[index]+=delta
      mean_eq=elements(internal(cartesian(updated,MU),theta,tilt,rate));expected=cartesian(forward(mean_eq),MU)
      assert 0<mean_eq[1]<.99 and mean_eq[0]*(1-mean_eq[1])>3000
      writes.append(dict(field=index,input=updated[index],local_mean=updated,earth_mean=mean_eq,expected_internal=expected))
     key=f'{kind}_i{inclination:g}_z{degrees:g}_x{tilt_degrees:g}_w{rate:g}'
     cases.append(dict(key=key,kind=kind,theta=theta,tilt=tilt,rate=rate,earth_mean=earth_mean,earth_osculating=osc,local_osculating=local(osc,theta,tilt,rate),local_mean=local_mean,writes=writes))
 refs=['src/qtgui/tests/BrouwerTransportOracle.py','src/qtgui/tests/BrouwerPrototypeOracle.py','prototype/StateConv/BROLYD.m','prototype/StateConv/BROLYD_short.m']
 return dict(schema_version=1,mu=MU,epoch=21545.125,position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9,mean_field_tolerances=[1e-6,1e-9,1e-6,1e-6,1e-6,1e-6],relative_tolerance=2e-12,reference_sha256={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in refs},cases=cases,scope='Two models,three physical inclinations including critical and retrograde,three tilted bases,static/rotating R/Rdot,both native epoch paths. Six scalar writes and complete-property writes. Retained prototype numerical model and independent Cartesian/frame geometry; no absolute astronomical frame or independent averaging qualification.')
if __name__=='__main__':
 import argparse
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);a=p.parse_args()
 with a.output.open('x')as f:json.dump(fixture(),f,indent=2);f.write('\n')
