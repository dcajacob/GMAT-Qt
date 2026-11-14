"""Oriented orbital-plane angle conventions and bounded GUI calculation inputs.

Individual AOP/TA truth near nonzero circularity is explicitly held separately
from the joint Cartesian reconstruction. Existing GMAT singular conventions
are applied;no assertion tolerance is changed to absorb ill-conditioning.
"""
import argparse,json,math
from pathlib import Path
from KeplerBoundaryOracle import cartesian,generate

def angles(elements):
 _,ecc,inc,node,peri,ta=elements;equatorial=math.radians(inc)<1e-11 or math.radians(inc)>math.pi-1e-11;retro=inc>90.;circular=ecc<1e-11
 raan=0. if equatorial else node%360.
 if circular:
  aop=0.;anomaly=(peri+ta+((-node if retro else node)if equatorial else 0.))%360.
 else:
  aop=(peri+((-node if retro else node)if equatorial else 0.))%360.;anomaly=ta%360.
 return [aop,raan,anomaly]

def boundary():
 samples=[]
 for source in generate()['samples']:
  row=dict(source);e=row['keplerian'][1];row.update(expected_angles=angles(row['keplerian']),condition_limited=[1e-11<=e<1e-5,False,1e-11<=e<1e-5]);samples.append(row)
 return dict(schema_version=1,angle_names=['AOP','RAAN','TA'],angle_tolerance_degrees=1e-8,position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9,scope='All2070joint scalar-angle Cartesian reconstructions;individual AOP/TA fields for nonzero ecc below1e-5 remain condition-limited diagnostics. Exact circular/equatorial canonical conventions follow the shipped guide;threshold endpoints/general singular science remain open.',samples=samples)

def gui():
 rows=[];mu=398600.4415;names=['X','Y','Z','VX','VY','VZ']
 for key,elements,parameter in [
  ('InclinedApsis',[10000,.2,47,359.99999999,179.999999,179.999999],'TA'),
  ('NearPrograde',[10000,.2,1e-7,112,38,143],'RAAN'),
  ('RetrogradeEquatorial',[10000,.2,180,112,38,143],'AOP'),
  ('CircularInclined',[9000,0,47,112,38,143],'TA'),
  ('CircularPrograde',[9000,0,0,112,38,143],'RAAN'),
  ('CircularRetrograde',[9000,0,180,112,38,143],'TA'),
  ('ProgradeEquatorial',[10000,.2,0,112,38,143],'AOP'),
  ('NearRetrograde',[10000,.2,179.9999999,1e-8,179.999999,179.999999],'TA')]:
  state=cartesian(elements,mu);expected=angles(elements);selected=expected[['AOP','RAAN','TA'].index(parameter)]
  setup='Earth.Mu = '+format(mu,'.17g')+';\nSat.DateFormat = TAIModJulian;\nSat.Epoch = 21545;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\n'+''.join('Sat.'+n+' = '+format(v,'.17g')+';\n'for n,v in zip(names,state))
  rows.append(dict(key=key,orbital_case=True,mu=mu,initial_elements=elements,setup=setup,destination='R',before='0',expression=('Sat.Earth.'if parameter=='TA'else'Sat.EarthMJ2000Eq.')+parameter,report='R '+' '.join(('Sat.Earth.'if n=='TA'else'Sat.EarthMJ2000Eq.')+n for n in ['AOP','RAAN','TA']+names),invalid=['AbsentNASA + 1','1 +'],absolute_tolerance=2e-8,relative_tolerance=0.,column_absolute_tolerances=[1e-8]*4+[2e-8]*3+[2e-11]*3,expected=[selected]+expected+state,scope='Selected scalar angular convention/calculation read through Assignment and direct Report parameters with Cartesian witness;not all angle domains/frames.'))
 return dict(schema_version=1,cases=rows)

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-directory',type=Path,required=True);a=p.parse_args()
 for name,value in [('ScalarAngleBoundaryCases.json',boundary()),('ScalarAngleGuiCases.json',gui())]:(a.output_directory/name).write_text(json.dumps(value,indent=2)+'\n')
