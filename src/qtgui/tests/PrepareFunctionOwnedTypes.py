"""Enumerate every uncovered owned type; preserve the full 566-row universe."""
import argparse,hashlib,json
from pathlib import Path

NUMERICAL=['RungeKutta89','PrinceDormand78','PrinceDormand45','RungeKutta68','RungeKutta56','AdamsBashforthMoulton','RungeKutta4','BulirschStoer','PrinceDormand853']
EPHEMERIS=['SPK','Code500','STK','CCSDS-OEM','SPICESGP4']
AXES=['MJ2000Eq','MJ2000Ec','TOEEq','TOEEc','MOEEq','MOEEc','TODEq','TODEc','MODEq','MODEc','ObjectReferenced','Equator','BodyFixed','BodyInertial','GSE','GSM','Topocentric','LocalAlignedConstrained','SPICE','ICRF','BodySpinSun','TEME']

def axes(name,kind,private=False):
    text=f'Create CoordinateSystem {name};\n{name}.Origin = '+('ScopeSite'if kind=='Topocentric'else'Earth')+f';\n{name}.Axes = {kind};\n'
    if kind in ['TOEEq','TOEEc','MOEEq','MOEEc']:text+=f'{name}.Epoch = '+('25301.75'if private else'25300.25')+';\n'
    if kind=='ObjectReferenced':text+=f'{name}.Primary = Earth;\n{name}.Secondary = Luna;\n{name}.XAxis = R;\n{name}.ZAxis = N;\n'
    if kind=='SPICE':text+=f"{name}.SpiceFrameId = 'J2000';\n"
    if kind=='LocalAlignedConstrained':
        text+=f'{name}.ReferenceObject = Luna;\n{name}.ConstraintCoordinateSystem = EarthMJ2000Eq;\n'
        for prefix,vector in [('AlignmentVector',[1,0,0]),('ConstraintVector',[0,0,1]),('ConstraintReferenceVector',[0,0,1])]:
            text+=''.join(f'{name}.{prefix}{axis} = {value};\n'for axis,value in zip('XYZ',vector))
    return text

def propagator(name,kind,private=False):
    if kind in NUMERICAL:
        fm='FunctionFM'if private else'MainFM'
        extra=(f'{name}.LowerError = '+('1e-14'if private else'1e-13')+f';\n{name}.TargetError = '+('1e-13'if private else'1e-12')+';\n')if kind=='AdamsBashforthMoulton'else''
        return f'Create ForceModel {fm};\n{fm}.CentralBody = Earth;\n{fm}.PrimaryBodies = {{}};\n{fm}.PointMasses = {{Earth}};\n{fm}.Drag = None;\n{fm}.SRP = Off;\nCreate Propagator {name};\n{name}.FM = {fm};\n{name}.Type = {kind};\n{name}.Accuracy = '+('1e-12'if private else'1e-11')+f';\n{name}.InitialStepSize = '+('1'if private else'3')+f';\n{name}.MinStep = 0;\n{name}.MaxStep = 5;\n'+extra
    text=f'Create Propagator {name};\n{name}.Type = {kind};\n{name}.StepSize = 5;\n'
    if kind!='SPICESGP4':text+=f"{name}.StartEpoch = 'FromSpacecraft';\n"
    return text

def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);args=p.parse_args();repo=Path(__file__).resolve().parents[3]
    baseline=repo/'build/nasa-coverage/coverage95-20261103-function-environment-bindings/environment-parameter-binding-matrix-v1.json';matrix=json.loads(baseline.read_text());rows=matrix['rows'];assert len(rows)==566
    selected=[r for r in rows if r['role'].startswith(('coordinate-axes','propagation-implementation'))and not r.get('owned_component_scope_witnesses')];assert len(selected)==32
    assert set(NUMERICAL+EPHEMERIS)=={r['factory_type']for r in rows if r['role'].startswith('propagation-implementation')}
    assert set(AXES)=={r['factory_type']for r in rows if r['role'].startswith('coordinate-axes')}
    cases=[]
    for row in selected:
        kind=row['factory_type'];frame=kind in AXES;family='Axes'if frame else'Numerical'if kind in NUMERICAL else'Ephemeris'
        common='Create GroundStation ScopeSite;\nScopeSite.CentralBody = Earth;\nScopeSite.StateType = Spherical;\nScopeSite.HorizonReference = Sphere;\nScopeSite.Location1 = 31.25;\nScopeSite.Location2 = -81.5;\nScopeSite.Location3 = 1.2;\n'if kind=='Topocentric'else''
        configure=axes if frame else propagator
        cases.append(dict(key=kind,factory_type=kind,category_id=row['category_id'],family=family,root_configuration=configure('Shared',kind),private_configuration=configure('FunctionOwned',kind,True),common_configuration=common,reference_configuration=(axes('RootOracle',kind)+axes('PrivateOracle',kind,True))if frame else'',frame_source='EarthMJ2000Eq'if family!='Ephemeris'or kind!='SPK'else'EarthICRF',position_bound_km=1e-7,velocity_bound_km_s=1e-10,requested_root_seconds=30 if kind=='SPICESGP4'else 15,requested_private_seconds=30,stop_time_bound_seconds=1e-6,qualified=False,oracle_scope='Independent matrix application using explicitly shared reference-frame inputs;analytic point-mass circles;authored affine ephemerides;separate root SGP4 control at identical epochs. No general provider/orientation truth inferred.'))
    result=dict(baseline=str(baseline.relative_to(repo)),baseline_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),factory_rows=566,full_owned_universe=dict(axes=AXES,propagators=NUMERICAL+EPHEMERIS),previously_witnessed=dict(axes=['MJ2000Eq','MJ2000Ec'],propagators=['RungeKutta89','PrinceDormand78']),selected_missing_count=32,cases=cases,new_requirement_credit=0,whole_scoping_qualified=False)
    assert not args.output.exists();args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(cases=len(cases),axes=sum(x['family']=='Axes'for x in cases),propagators=sum(x['family']!='Axes'for x in cases))))
if __name__=='__main__':main()
