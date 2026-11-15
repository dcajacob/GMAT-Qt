"""Save/reload witnesses for the newly introduced fixed-offset attitude state."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path

HERE=Path(__file__).resolve().parent
REPO=HERE.parents[2]


def generate():
    parent=HERE/'NasaSaveAttitudeCases.json'
    frames=HERE/'AttitudeOffsetScopeCases.json'
    original=json.loads(parent.read_text())
    frame_catalog=json.loads(frames.read_text())
    fixed=copy.deepcopy(next(c for c in original['cases']if c['key']=='FixedEcliptic'))
    body=next(c for c in frame_catalog['cases']if c['key']=='CelestialBody_BodyFixed')
    q=frame_catalog['offset_quaternion']
    epsilon=math.radians(84381.448/3600)
    ecliptic=[[1,0,0],[0,math.cos(epsilon),math.sin(epsilon)],[0,-math.sin(epsilon),math.cos(epsilon)]]
    identity=[[int(i==j)for j in range(3)]for i in range(3)]
    cases=[]
    for key,raw,reference,basis,rates in [
        ('Identity', [0,0,0,1], 'EarthMJ2000Ec',ecliptic,[0,0,0]),
        ('Named', q, 'SavedFrame',identity,[0,0,0]),
        ('Body', [-7*v for v in q], 'SavedFrame',body['samples'][0]['frame_rows'],body['samples'][0]['frame_angular_velocity_degrees_s']),
        ('HalfTurn',[0,1,0,0],'EarthMJ2000Ec',ecliptic,[0,0,0])]:
        case=copy.deepcopy(fixed);case['key']=key
        setup=fixed['setup'].replace('Subject.AttitudeCoordinateSystem = EarthMJ2000Ec;',
                                    'Subject.AttitudeCoordinateSystem = '+reference+';')
        if key=='Named':
            setup='Create CoordinateSystem SavedFrame;\nSavedFrame.Origin = Pluto;\nSavedFrame.Axes = MJ2000Eq;\n'+setup
        elif key=='Body':
            # Retain the independently qualified prescribed body/frame inputs;
            # omit unrelated globals/region resources from this new graph.
            start=body['setup'].index('Create Asteroid Probe;')
            end=body['setup'].index('Create Barycenter Center;')
            body_setup=body['setup'][start:end].replace('Probe','AxisBody')
            station=str((HERE/'frame-consumer-station-origin.bsp').resolve())
            body_setup=body_setup.replace('@STATION@',station)
            setup=(body_setup+'Create CoordinateSystem SavedFrame;\nSavedFrame.Origin = AxisBody;\nSavedFrame.Axes = BodyFixed;\n'+setup)
        setup+='Subject.OffsetQuaternion = ['+' '.join(format(v,'.17g')for v in raw)+'];\nCreate Variable Sample;\nSample = 0;\n'
        scale=max(abs(v)for v in raw);length=math.hypot(*(v/scale for v in raw))
        normalized=[v/scale/length for v in raw]
        x,y,z,w=normalized
        rotation=[[1-2*(y*y+z*z),2*(x*y+z*w),2*(x*z-y*w)],
                  [2*(x*y-z*w),1-2*(x*x+z*z),2*(y*z+x*w)],
                  [2*(x*z+y*w),2*(y*z-x*w),1-2*(x*x+y*y)]]
        attitude=[[math.fsum(rotation[i][k]*basis[k][j]for k in range(3))for j in range(3)]for i in range(3)]
        rate=[math.fsum(rotation[i][j]*rates[j]for j in range(3))for i in range(3)]
        objects=['Subject']+(['SavedFrame']if key in ['Named','Body']else[])+(['AxisBody']if key=='Body'else[])+['Sample']
        witnesses=[dict(object='Subject',owned_types=['CoordinateSystemFixed'],field='AttitudeCoordinateSystem',type='Object',expected=reference),
                   dict(object='Subject',owned_types=['CoordinateSystemFixed'],field='OffsetQuaternion',type='Rvector',expected=normalized),
                   dict(object='Sample',field='Value',type='Real',expected=attitude[0][0])]
        if key in ['Named','Body']:
            witnesses.extend([dict(object='SavedFrame',field='Origin',type='Object',expected='AxisBody'if key=='Body'else'Pluto'),
                              dict(object='SavedFrame',field='Axes',type='Object',expected='BodyFixed'if key=='Body'else'MJ2000Eq')])
        probe=copy.deepcopy(witnesses)
        for i in range(3):
            for j in range(3):probe.append(dict(object='Subject',owned_types=['CoordinateSystemFixed'],field=f'DCM{i+1}{j+1}',type='Real',expected=attitude[i][j]))
            probe.append(dict(object='Subject',owned_types=['CoordinateSystemFixed'],field='AngularVelocity'+'XYZ'[i],type='Real',expected=rate[i]))
        case.update(setup=setup,selected=objects,objects=objects,filename='_'.join(objects)+'.data',
                    definitions=['Create Spacecraft Subject;']+(['Create CoordinateSystem SavedFrame;']if key in ['Named','Body']else[])+(['Create Asteroid AxisBody;']if key=='Body'else[])+['Create Variable Sample'],
                    witnesses=witnesses,before_save='Sample = Subject.DCM11;\n',
                    reload_probe='Sample = Subject.DCM11;\n',probe_witnesses=probe,
                    expected_offset_quaternion=normalized,expected_matrix=attitude,expected_rate_degrees_s=rate,
                    offset_serialized=normalized!=[0,0,0,1])
        if key=='Body':case['runtime_solar_system_probe']='Subject'
        cases.append(case)
    physical=HERE/'frame-consumer-station-origin.bsp'
    return dict(schema_version=1,scope='New fixed-offset Save state, exact source/export/native persistence and post-rebuild evaluation only; whole original FRC!E132 remains separate.',
                source_sha256={str(p.relative_to(REPO)):hashlib.sha256(p.read_bytes()).hexdigest()for p in [parent,frames]},
                physical_input_sha256={str(physical.relative_to(REPO)):hashlib.sha256(physical.read_bytes()).hexdigest()},cases=cases)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    with a.output.open('x')as stream:json.dump(generate(),stream,indent=2);stream.write('\n')
