"""Analytic Cartesian fixtures around circular/equatorial/angular boundaries."""
import argparse,json,math
from pathlib import Path

def cartesian(elements,mu):
 a,e,inc,node,peri,ta=elements;i,o,w,v=map(math.radians,[inc,node,peri,ta])
 p=a*(1-e*e);r=p/(1+e*math.cos(v));scale=math.sqrt(mu/p)
 P=[math.cos(o)*math.cos(w)-math.sin(o)*math.sin(w)*math.cos(i),math.sin(o)*math.cos(w)+math.cos(o)*math.sin(w)*math.cos(i),math.sin(w)*math.sin(i)]
 Q=[-math.cos(o)*math.sin(w)-math.sin(o)*math.cos(w)*math.cos(i),-math.sin(o)*math.sin(w)+math.cos(o)*math.cos(w)*math.cos(i),math.cos(w)*math.sin(i)]
 return [r*(P[k]*math.cos(v)+Q[k]*math.sin(v))for k in range(3)]+[scale*(-P[k]*math.sin(v)+Q[k]*(e+math.cos(v)))for k in range(3)]

def generate():
 samples=[];mu=398600.4418
 for e in [0.,1e-12,1e-8,.2,.95,1.2]:
  for inc in [0.,1e-10,1e-7,.0001,45.,90.,179.9999,179.9999999,180.]:
   for node,peri in [(0.,0.),(1e-8,1e-8),(112.,38.),(180.,180.),(359.99999999,359.99999999)]:
    anomalies=[0.,1e-8,37.,90.,179.999999,180.,270.,359.999999]if e<1 else[0.,1e-8,37.,90.,-90.,-37.]
    for ta in anomalies:
     elements=[9000. if e<1 else-15000.,e,inc,node,peri,ta]
     samples.append(dict(index=len(samples),mu=mu,keplerian=elements,cartesian=cartesian(elements,mu)))
 assert len(samples)==2070
 return dict(schema_version=1,scope='Analytic Cartesian reconstruction for all four circular/equatorial branch classes and elliptic/hyperbolic angular endpoints. Individual near-singular angles remain separate diagnostics.',position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9,samples=samples)

def gui(elements):
 return dict(schema_version=1,scope='New near-equatorial or retrograde-apsis positive Keplerian definition through unchanged original GUI driver',cases=[dict(key='Keplerian',mu=398600.4418,canonical_keplerian=elements,names=['SMA','ECC','INC','RAAN','AOP','TA'],values=elements,units=['km','','deg','deg','deg','deg'],invalid_field='SMA',invalid=['nan','0','-1'],expected_cartesian=cartesian(elements,398600.4418),position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9)])

if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output-directory',type=Path,required=True);args=parser.parse_args()
 for name,value in [('KeplerBoundaryCases.json',generate()),('NasaNearEquatorDefinitionCases.json',gui([10000.,.2,1e-7,112.,38.,143.])),('NasaNearRetroApsisDefinitionCases.json',gui([23000.,.7,179.9999999,1e-8,179.999999,179.999999]))]:
  (args.output_directory/name).write_text(json.dumps(value,indent=2)+'\n')
