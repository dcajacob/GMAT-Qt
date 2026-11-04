"""Independent conic inputs for scalar inclination calculation acceptance."""
import argparse,json,math
from pathlib import Path
from KeplerBoundaryOracle import cartesian

def cases():
 result=[];mu=398600.4415;names=['X','Y','Z','VX','VY','VZ']
 for key,inc in [('NearPrograde',1e-7),('NearRetrograde',179.9999999),('Equatorial',0.),('Retrograde',180.)]:
  elements=[10000.,.2,inc,112.,38.,143.];state=cartesian(elements,mu)
  setup='Earth.Mu = '+format(mu,'.17g')+';\nSat.DateFormat = TAIModJulian;\nSat.Epoch = 21545;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\n'
  setup+=''.join('Sat.'+name+' = '+format(value,'.17g')+';\n'for name,value in zip(names,state))
  result.append(dict(key=key,orbital_case=True,mu=mu,initial_elements=elements,setup=setup,destination='R',before='0',expression='Sat.EarthMJ2000Eq.INC',report='R Sat.EarthMJ2000Eq.INC '+' '.join('Sat.EarthMJ2000Eq.'+n for n in names),invalid=['AbsentNASA + 1','1 +'],absolute_tolerance=2e-8,relative_tolerance=0.,column_absolute_tolerances=[1e-8,1e-8,2e-8,2e-8,2e-8,2e-11,2e-11,2e-11],expected=[inc,inc]+state,scope='Scalar INC in Assignment expression and direct Report parameter,unchanged Cartesian witness,selected EarthMJ2000Eq boundary orbit;not all calculation parameters or all domains.'))
 return dict(schema_version=1,scope='Four distinct prograde/retrograde near/exact-equatorial scalar calculation inputs using the unchanged Assignment driver',cases=result)

if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();args.output.write_text(json.dumps(cases(),indent=2)+'\n')
