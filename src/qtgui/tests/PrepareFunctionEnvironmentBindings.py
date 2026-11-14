"""Prepare all23 remaining owner recipes without dropping any321 catalog entry."""
import argparse,copy,hashlib,json,math
from pathlib import Path
from FunctionEnvironmentBindingOracle import TYPES,RATES,PLANET,ACCEL,TORQUE,SCOPES,state,covariance,plates,self_check

def number(x):return format(x,'.17g')
def vector(values):return '['+' '.join(map(number,values))+']'
def matrix(values):return '['+'; '.join(' '.join(map(number,row))for row in values)+']'
def spacecraft(name,scope,calls=0):
    index=SCOPES.index(scope);fields=['X','Y','Z','VX','VY','VZ']
    text=f'Create Spacecraft {name};\n{name}.DateFormat = A1ModJulian;\n{name}.Epoch = \'21545\';\n{name}.CoordinateSystem = EarthMJ2000Eq;\n{name}.DisplayStateType = Cartesian;\n'
    text+=''.join(f'{name}.{field} = {number(value)};\n'for field,value in zip(fields,state(scope,calls)))
    text+=f'{name}.DryMass = {800+100*index};\n{name}.Cd = 0;\n{name}.Attitude = CoordinateSystemFixed;\n{name}.AttitudeCoordinateSystem = EarthMJ2000Eq;\n'
    native_id=-128101 if scope=='private'else-126001 if scope=='root'else-126002
    text+=f'{name}.NAIFId = {native_id};\n{name}.NAIFIdReferenceFrame = {native_id-8000000};\n'
    for field,value in [('DryMomentOfInertiaXX',100*(index+1)),('DryMomentOfInertiaYY',200*(index+1)),('DryMomentOfInertiaZZ',300*(index+1)),('DryMomentOfInertiaXY',0),('DryMomentOfInertiaXZ',0),('DryMomentOfInertiaYZ',0),('DryCenterOfMassX',0),('DryCenterOfMassY',0),('DryCenterOfMassZ',0)]:text+=f'{name}.{field} = {number(value)};\n'
    return text+f'{name}.OrbitErrorCovariance = {matrix(covariance(scope,calls))};\n'

def provider(kind,owner,scope):
    index=SCOPES.index(scope);prefix='Shared'if scope=='shared'else'Private';model=prefix+'FM'
    text=f'Create ForceModel {model};\n{model}.PrimaryBodies = {{}};\n{model}.PointMasses = {{Earth}};\n{model}.Drag = None;\n{model}.SRP = Off;\n'
    resources=[model];active='';finish=''
    if kind=='AtmosDensity':text+=f"{model}.Drag.AtmosphereModel = Exponential;\n{model}.Drag.InputFile = '@DENSITY_FILE@';\n"
    if kind in TORQUE:
        text+=f'{owner}.ExtendedMassPropertiesModelType = Analytic;\n{owner}.ExtendedMassPropertiesModel = CenterOfMassAndMomentOfInertia;\n'
        tank=prefix+'Tank';thruster=prefix+'Thr';burn=prefix+'Burn';resources+=[tank,thruster,burn]
        text+=f'Create ChemicalTank {tank};\n{tank}.FuelMass = 0;\nCreate ChemicalThruster {thruster};\n{thruster}.Tank = {{{tank}}};\n{thruster}.DecrementMass = false;\n{thruster}.CoordinateSystem = Local;\n{thruster}.Axes = SpacecraftBody;\n{thruster}.ThrustDirection1 = 1;\n{thruster}.ThrustDirection2 = 0;\n{thruster}.ThrustDirection3 = 0;\n{thruster}.C1 = {8+3*index};\n{thruster}.K1 = 300;\n'
        for field,value in [('HWOriginInBCSX',.4*(index+1)),('HWOriginInBCSY',.7),('HWOriginInBCSZ',.8)]:text+=f'{thruster}.{field} = {number(value)};\n'
        text+=''.join(f'{tank}.FuelMomentOfInertia{axis} = 0;\n'for axis in ['XX','XY','XZ','YY','YZ','ZZ'])
        text+=f'{owner}.Tanks = {{{tank}}};\n{owner}.Thrusters = {{{thruster}}};\nCreate FiniteBurn {burn};\n{burn}.Thrusters = {{{thruster}}};\n'
        active=f'BeginFiniteBurn {burn}({owner});\n';finish=f'EndFiniteBurn {burn}({owner});\n'
        if kind in ('SRPTorque','TotalTorque'):
            names=[]
            for i,plate in enumerate(plates(scope)):
                name=prefix+'Plate'+str(i);names.append(name);resources.append(name)
                text+=f'Create Plate {name};\n{name}.Type = FixedInBody;\n{name}.PlateNormal = {vector(plate["normal"])};\n{name}.Area = {number(plate["area"])};\n{name}.SpecularFraction = 0;\n{name}.DiffuseFraction = 0;\n{name}.LitFraction = 1;\n'
                for field,value in zip(['PlateX','PlateY','PlateZ'],plate['location']):text+=f'{name}.{field} = {number(value)};\n'
            text+=f'{owner}.AddPlates = {{{", ".join(names)}}};\n{model}.SRP = On;\n{model}.SRP.SRPModel = NPlate;\n{model}.SRP.Flux = 1361;\n{model}.SRP.Nominal_Sun = 149597870.691;\n'
    if kind=='AtmosDensity':
        text=text.replace(f'{model}.PrimaryBodies = {{}};\n{model}.PointMasses = {{Earth}};',f'{model}.PrimaryBodies = {{Earth}};\n{model}.PointMasses = {{}};\n{model}.GravityField.Earth.Degree = 0;\n{model}.GravityField.Earth.Order = 0;')
    return dict(configuration=text,resources=resources,begin=active,end=finish,model=model)

def main():
    repo=Path(__file__).resolve().parents[3];p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    baseline=repo/'src/qtgui/tests/NasaFunctionSpecialOrbitCases.json';catalog=copy.deepcopy(json.loads(baseline.read_text()))
    queue=repo/'build/nasa-coverage/coverage95-20261103-function-special-orbit-bindings/remaining-parameter-binding-worklist-v1.json';pending=json.loads(queue.read_text())
    assert {r['factory_type']for r in pending['rows']}==set(TYPES) and pending['remaining_count']==23 and len(catalog['cases'])==321
    cases={r['factory_type']:r for r in catalog['cases']};new=[]
    ode=set(RATES+ACCEL+TORQUE+['AtmosDensity','SurfaceHeight'])
    for kind in TYPES:
        entry=copy.deepcopy(cases[kind]);entry.update(argument=kind+'Read',family='Environment',qualified=False,writable_probe=False,shape=[6,6]if kind=='OrbitErrorCovariance'else[3,1]if kind in TORQUE else[1,1],observations={},fixtures={},frame_name='EarthMJ2000Eq',frame_axes='MJ2000Eq',frame_role='owner',mu=398600.4415,dependency_kind='OdeModel'if kind in ode else'Origin'if kind in PLANET else'CoordinateSystem')
        for scope in SCOPES:
            owner='SharedSC'if scope=='shared'else'PrivateOnly';dependency=('SharedFM'if scope=='shared'else'PrivateFM')if kind in ode else'Earth'if kind in PLANET else'EarthMJ2000Eq'
            reference=owner+'.'+dependency+'.'+kind
            entry[{'root':'root_private_reference','private':'local_private_reference','shared':'shared_reference'}[scope]]=reference
            entry['fixtures'][scope]=dict(owner=owner,configuration=spacecraft(owner,scope),provider=provider(kind,owner,scope)if kind in ode else None)
            entry['observations'][scope]=[dict(state=state(scope,calls),covariance=covariance(scope,calls),expected_status='Compute independent recipe from explicit recorded shared provider inputs;not yet native evidence.')for calls in range(4)]
        new.append(entry)
    result=dict(factory_rows=566,system_parameter_count=321,cases=catalog['cases'],environment_binding_cases=new,initial_types=TYPES,selected_arguments=[r['argument']for r in new],previous_live_bound_types=298,selected_missing_types=23,baseline=str(baseline.relative_to(repo)),baseline_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),worklist=str(queue.relative_to(repo)),worklist_sha256=hashlib.sha256(queue.read_bytes()).hexdigest(),recipe_self_check=self_check(),new_requirement_credit=0,all_system_parameter_whole_obligations_open=True)
    assert not args.output.exists();args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(types=23,retained_catalog=321,output=str(args.output),new_requirement_credit=0)))
if __name__=='__main__':main()
