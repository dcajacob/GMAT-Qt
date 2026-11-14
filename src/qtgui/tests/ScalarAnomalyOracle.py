"""Independent eccentric/hyperbolic/mean anomaly and GUI caller fixtures."""
import argparse,json,math
from pathlib import Path
from KeplerBoundaryOracle import cartesian,generate
from ScalarAnglesOracle import angles

def reference(elements):
 eccentricity=elements[1];true=math.radians(angles(elements)[2])
 if eccentricity<1:
  eccentric=(2*math.atan2(math.sqrt(1-eccentricity)*math.sin(true/2),math.sqrt(1+eccentricity)*math.cos(true/2)))%(2*math.pi)
  mean=(eccentric-eccentricity*math.sin(eccentric))%(2*math.pi)
  return ['EA','MA'],[math.degrees(eccentric),math.degrees(mean)]
 hyperbolic=math.asinh(math.sqrt(eccentricity*eccentricity-1)*math.sin(true)/(1+eccentricity*math.cos(true)))
 mean=eccentricity*math.sinh(hyperbolic)-hyperbolic
 return ['HA','MA'],[math.degrees(hyperbolic),math.degrees(mean)]

def boundary():
 rows=[]
 for sample in generate()['samples']:
  names,expected=reference(sample['keplerian']);e=sample['keplerian'][1]
  rows.append(dict(**sample,anomaly_names=names,expected_anomalies=expected,condition_limited=1e-11<=e<1e-5,cyclic=e<1))
 return dict(schema_version=1,absolute_tolerance_degrees=1e-8,position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9,scope='3420checked anomaly fields and4140independent anomaly-based state reconstructions over2070states.720nonzero-near-circular EA/MA individual fields remain condition-limited. Parabolic/exact anomaly thresholds/all physical domains remain open.',samples=rows)

def gui():
 rows=[];mu=398600.4415;names=['X','Y','Z','VX','VY','VZ']
 for key,elements,parameter in [
  ('EllipticNearPeri',[10000,.2,47,112,38,1e-8],'EA'),
  ('EllipticNearApo',[12000,.95,47,112,38,179.999999],'MA'),
  ('EllipticInbound',[10000,.2,179.9999999,112,38,359.999999],'MA'),
  ('CircularRetrograde',[9000,0,180,112,38,143],'EA'),
  ('CircularInclined',[9000,0,47,112,38,143],'MA'),
  ('HyperbolicOutbound',[-15000,1.2,47,112,38,37],'HA'),
  ('HyperbolicInbound',[-15000,1.2,179.9999999,112,38,-37],'HA'),
  ('HyperbolicNearPeri',[-15000,1.2,47,112,38,1e-8],'MA')]:
  state=cartesian(elements,mu);anomaly_names,expected=reference(elements);selected=expected[anomaly_names.index(parameter)]
  setup='Earth.Mu = '+format(mu,'.17g')+';\nSat.DateFormat = TAIModJulian;\nSat.Epoch = 21545;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\n'+''.join('Sat.'+n+' = '+format(v,'.17g')+';\n'for n,v in zip(names,state))
  report='R '+' '.join('Sat.Earth.'+n for n in anomaly_names+['TA'])+' '+' '.join('Sat.EarthMJ2000Eq.'+n for n in names)
  rows.append(dict(key=key,orbital_case=True,mu=mu,initial_elements=elements,setup=setup,destination='R',before='0',expression='Sat.Earth.'+parameter,read_only=True,reject_destination='Sat.Earth.'+parameter,report=report,invalid=['AbsentNASA + 1','1 +'],absolute_tolerance=2e-8,relative_tolerance=0.,column_absolute_tolerances=[1e-8]*4+[2e-8]*3+[2e-11]*3,expected=[selected]+expected+[angles(elements)[2]]+state,scope='Selected origin-qualified readonly anomaly calculations,Assignment/direct Report values,unchanged Cartesian witness;not all anomaly/threshold/dynamical domains.'))
 return dict(schema_version=1,cases=rows)

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-directory',type=Path,required=True);a=p.parse_args()
 for name,value in [('ScalarAnomalyBoundaryCases.json',boundary()),('ScalarAnomalyGuiCases.json',gui())]:(a.output_directory/name).write_text(json.dumps(value,indent=2)+'\n')
