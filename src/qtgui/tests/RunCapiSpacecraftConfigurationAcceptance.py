"""Check new C-interface configuration combinations using existing oracles.

No GUI replay, GMAT conversion oracle, private C++ probe or runtime copies.
This runner supplies evidence; exact E250 admission is a separate audit.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import sys
import uuid
import xml.etree.ElementTree as ET
from RunCapiAcceptance import run

ROOT=Path(__file__).resolve().parents[3]
PARENT=Path(__file__).parent
INPUTS=['NasaOrbitDefinitionCases.json','NasaEpochDefinitionCases.json','NasaAttitudeDefinitionCases.json']
COMPONENTS=['X','Y','Z','VX','VY','VZ']
FIELDS=['A1ModJulian',*COMPONENTS,
        *('DCM'+str(i)+str(j)for i in range(1,4)for j in range(1,4)),
        'AngularVelocityX','AngularVelocityY','AngularVelocityZ',
        'DryMass','Cd','Cr','DragArea','SRPArea','TotalMass']


def fixtures(raw_inputs=None):
    raw_inputs=raw_inputs or {name:(PARENT/name).read_bytes() for name in INPUTS}
    orbit,epochs,attitudes=[json.loads(raw_inputs[name])['cases'] for name in INPUTS]
    profiles=[]
    for family,cases in [('orbit',orbit),('epoch',epochs),('attitude',attitudes)]:
        for case in cases:
            profiles.append(dict(family=family,key=case['key'],source=case))
    assert len(profiles)==31
    setup='Earth.Mu = 398600.4418;\n'
    columns=[];epoch_file=''
    for i,profile in enumerate(profiles):
        name=f'CFG{i:02d}';tank=f'Tank{i:02d}';case=profile['source'];family=profile['family']
        state=case['expected_cartesian'] if family=='orbit' else [7000,0,0,0,7.5,1]
        epoch=case['expected']['A1'] if family=='epoch' else 21545.
        tolerance=case['epoch_tolerance_seconds'] if family=='epoch' else 2e-6
        matrix=case['matrix'] if family=='attitude' else [[1,0,0],[0,1,0],[0,0,1]]
        rates=case['angular_velocity_degrees'] if family=='attitude' else [0,0,0]
        physical=[100+3*i,1.2+.02*i,1.0+.01*i,2+.1*i,4+.2*i]
        fuel=10+i
        setup+=f'Create ChemicalTank {tank};\n{tank}.FuelMass = {fuel};\nCreate Spacecraft {name};\n{name}.Tanks = {{{tank}}};\n{name}.CoordinateSystem = EarthMJ2000Eq;\n'
        date=case['key'] if family=='epoch' else 'A1ModJulian'
        literal=case['literal'] if family=='epoch' else '21545'
        setup+=f"{name}.DateFormat = {date};\n{name}.Epoch = '{literal}';\n"
        representation=case['key'] if family=='orbit' else 'Cartesian'
        values=case['values'] if family=='orbit' else state
        names=case['names'] if family=='orbit' else COMPONENTS
        setup+=f'{name}.DisplayStateType = {representation};\n'
        setup+=''.join(f'{name}.{field} = {value:.17g};\n'for field,value in zip(names,values))
        setup+=f'{name}.Attitude = Spinner;\n'
        if family=='attitude':
            setup+=f"{name}.AttitudeDisplayStateType = {case['representation']};\n{name}.EulerAngleSequence = '{case['sequence']}';\n{name}.AttitudeRateDisplayStateType = {case['rate_representation']};\n"
            setup+=''.join(f'{name}.{field} = {value:.17g};\n'for field,value in zip(case['names'],case['values']))
            setup+=''.join(f'{name}.{field} = {value:.17g};\n'for field,value in zip(case['rate_names'],case['rate_values']))
        else:
            setup+=f'{name}.AttitudeDisplayStateType = Quaternion;\n'
            setup+=''.join(f'{name}.Q{j+1} = {value};\n'for j,value in enumerate([0,0,0,1]))
            setup+=''.join(f'{name}.AngularVelocity{axis} = 0;\n'for axis in 'XYZ')
        setup+=''.join(f'{name}.{field} = {value:.17g};\n'for field,value in zip(['DryMass','Cd','Cr','DragArea','SRPArea'],physical))
        profile.update(name=name,expected=[epoch,*state,*(x for row in matrix for x in row),*rates,*physical,physical[0]+fuel],epoch_tolerance_seconds=tolerance)
        epoch_file+=f'{epoch:.17g} {tolerance:.17g}\n'
        columns.append('Report Configuration '+ ' '.join(name+'.'+field for field in FIELDS)+';\n')
    setup+="Create ReportFile Configuration;\nConfiguration.Filename = 'configuration.csv';\nConfiguration.WriteHeaders = false;\nConfiguration.FixedWidth = false;\nConfiguration.Delimiter = ',';\nConfiguration.Precision = 17;\nBeginMissionSequence;\n"+''.join(columns)
    return setup,epoch_file,dict(profiles=profiles,fields=FIELDS,source_inputs={str(PARENT/name):hashlib.sha256(raw_inputs[name]).hexdigest()for name in INPUTS})


def check_rows(path,matrix):
    rows=[[float(value)for value in row]for row in csv.reader(path.open())if row]
    assert len(rows)==len(matrix['profiles'])==31,'Missing configuration rows'
    comparisons=0
    for row,profile in zip(rows,matrix['profiles']):
        assert len(row)==len(FIELDS)==25,'Configuration report field count differs'
        for index,(actual,expected)in enumerate(zip(row,profile['expected'])):
            tolerance=profile['epoch_tolerance_seconds']/86400 if index==0 else 1e-6 if 1<=index<=3 else 1e-9 if 4<=index<=6 else 2e-10
            assert math.isfinite(actual)and abs(actual-expected)<=tolerance,(profile['name'],profile['key'],FIELDS[index],actual,expected,tolerance)
            comparisons+=1
    return dict(status='passed',profiles=31,comparisons=comparisons,rows=rows)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--repo',type=Path,default=ROOT);parser.add_argument('--executable',type=Path,required=True)
    dest=parser.add_mutually_exclusive_group(required=True);dest.add_argument('--output',type=Path);dest.add_argument('--output-root',type=Path)
    args=parser.parse_args();output=args.output or args.output_root/('spacecraft-configuration-'+uuid.uuid4().hex[:12])
    raw_inputs={name:(PARENT/name).read_bytes()for name in INPUTS}
    script,epochs,matrix=fixtures(raw_inputs)
    assets={'expected-epochs.txt':epochs,**{'oracles/'+name:raw.decode()for name,raw in raw_inputs.items()}}
    status=run(args.repo.resolve(),args.executable,'spacecraftconfiguration',output,fixture_text=script,fixture_assets=assets)
    (output/'configuration-oracle.json').write_text(json.dumps(matrix,indent=2)+'\n')
    if status:return status
    try:checked=check_rows(output/'session/output/configuration.csv',matrix)
    except Exception as error:
        checked=dict(status='failed',error=str(error))
    (output/'configuration-validation.json').write_text(json.dumps(checked,indent=2)+'\n')
    suite=ET.Element('testsuite',name='CInterfaceConfiguration',tests='1',failures=str(int(checked['status']!='passed')))
    case=ET.SubElement(suite,'testcase',name='QtAcceptance.NasaCInterfaceSpacecraftConfiguration')
    if checked['status']!='passed':ET.SubElement(case,'failure',message=checked['error'])
    ET.ElementTree(suite).write(output/'configuration-junit.xml',encoding='unicode')
    print(json.dumps({k:v for k,v in checked.items()if k!='rows'}));return 0 if checked['status']=='passed'else 1


if __name__=='__main__':
    raise SystemExit(main())
