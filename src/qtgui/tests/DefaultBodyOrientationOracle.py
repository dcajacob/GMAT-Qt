"""Historical IAU 2002 default-body orientation from NASA's exact text PCK.

The stated NASA default-body state convention holds the pole factors fixed in
the velocity reduction and differentiates prime-meridian angle W. The full
angle derivatives are also calculated as independent diagnostic quantities.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
from LunarOrientationOracle import RAD,INPUT,compose,fixed_state,mm,transpose

BODIES={'Sun':10,'Mercury':199,'Venus':299,'Mars':499,'Jupiter':599,'Saturn':699,'Uranus':799,'Neptune':899,'Pluto':999}
DEFAULTS=['Sun','Mercury','Venus','Earth','Luna','Mars','Jupiter','Saturn','Uranus','Neptune','Pluto']

class IauBodies:
    def __init__(self,path):
        text=path.read_text();data='\n'.join(block.split('\\begintext')[0] for block in text.split('\\begindata')[1:])
        self.values={key:[float(v.replace('D','E')) for v in body.split()] for key,body in re.findall(r'(BODY[0-9]+_[A-Z_]+)\s*=\s*\(([^)]*)\)',data)}
    def angles(self,body,epoch):
        number=BODIES[body];days=epoch-21545;centuries=days/36525;out=[]
        for name in ['POLE_RA','POLE_DEC','PM']:
            t,scale=(days,1/86400) if name=='PM' else (centuries,1/(36525*86400))
            a,b,c=self.values[f'BODY{number}_{name}'];angle=a+b*t+c*t*t;rate=(b+2*c*t)*scale
            if body=='Neptune':
                angles=self.values['BODY8_NUT_PREC_ANGLES'];key={'POLE_RA':'RA','POLE_DEC':'DEC','PM':'PM'}[name]
                for i,amplitude in enumerate(self.values['BODY899_NUT_PREC_'+key]):
                    argument=(angles[2*i]+angles[2*i+1]*centuries)*RAD;speed=angles[2*i+1]*RAD/(36525*86400)
                    if name=='POLE_DEC':angle+=amplitude*math.cos(argument);rate-=amplitude*math.sin(argument)*speed
                    else:angle+=amplitude*math.sin(argument);rate+=amplitude*math.cos(argument)*speed
            out.append((angle*RAD,rate*RAD))
        return out
    def orientation(self,body,epoch):
        (ra,dra),(dec,ddec),(w,dw)=self.angles(body,epoch)
        # NASA MathematicalSpecification: default bodies retain only Wdot in
        # the state transform; do not silently replace that stated approximation.
        return compose([(2,ra+math.pi/2,0),(0,math.pi/2-dec,0),(2,w,dw)])
    def parameters(self,body):
        number=BODIES[body];out=[]
        for name in ['POLE_RA','POLE_DEC','PM']:out+=self.values[f'BODY{number}_{name}'][:2]
        if body=='Neptune':out[1]=self.values['BODY899_NUT_PREC_RA'][0];out[3]=self.values['BODY899_NUT_PREC_DEC'][0]
        return out


def generate(repo):
    path=repo/'src/qtgui/tests/fixtures/lunar/pck00008.tpc';oracle=IauBodies(path)
    epochs=[21545.,21545.+.1/86400,21545.125,21545.5,21600.5,10000.25,30000.75,50000.25,70000.5,80000.25,21000.25,21545.]
    cases=[]
    for body,number in BODIES.items():
        rows=[]
        for epoch in epochs:
            r,d=oracle.orientation(body,epoch);angles=oracle.angles(body,epoch)
            assert max(abs(mm(transpose(r),r)[i][j]-(i==j)) for i in range(3) for j in range(3))<1e-14
            # Wdot must differentiate W, including degrees/radians in Neptune's sine.
            step=1000/86400;minus=oracle.angles(body,epoch-step)[2][0];plus=oracle.angles(body,epoch+step)[2][0]
            actual_dt=((epoch+step)-(epoch-step))*86400
            assert abs((plus-minus)/actual_dt-angles[2][1])<3e-12
            for witness,state in [('mixed',INPUT)]+[(f'basis-{k}',[(1000. if k<3 else 1.) if j==k else 0. for j in range(6)]) for k in range(6)]:
                rows.append(dict(tdb_mjd=epoch,witness=witness,input_state=state,fixed_state=fixed_state(r,d,state),angles_rad=[v[0] for v in angles],angle_rates_rad_s=[v[1] for v in angles]))
        cases.append(dict(body=body,naif_id=number,rotation_source='IAU2002' if body=='Neptune' else 'IAUSimplified',orientation_parameters=oracle.parameters(body),rows=rows))
    return dict(default_bodies=DEFAULTS,position_tolerance_km=5e-6,input_tolerance_km=5e-6,velocity_tolerance_km_s=5e-10,tdb_tolerance_days=2e-10,pck_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),cases=cases,scope='All nine non-Earth/non-Luna default orientation models at the IAU/IAG2000 report published2002; NASA documented MJ2000Eq and spin-only velocity convention. Earth and Luna require their separate complete proofs.')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();j=generate(Path(__file__).resolve().parents[3]);a.output.write_text(json.dumps(j,indent=2)+'\n');print('Generated',len(j['cases']),'models,',sum(len(c['rows']) for c in j['cases']),'states')
