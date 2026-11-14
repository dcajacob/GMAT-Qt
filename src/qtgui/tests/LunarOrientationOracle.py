"""Independent DE405 and IAU/IAG 2000 (published 2002) lunar orientation.

Uses direct binary coefficient decoding and Clenshaw evaluation for DE405;
parses NASA's historical text PCK and differentiates its periodic series for
IAU. No GMAT/SPICE orientation, time-conversion or state helper is called.
Epoch inputs are TDB seconds/days specified by the fixture.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct

RAD = math.pi/180
INPUT = [1000., -200., 300., 1.25, -.5, .75]


def mm(a,b): return [[sum(a[i][k]*b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
def mv(a,v): return [sum(a[i][j]*v[j] for j in range(3)) for i in range(3)]
def transpose(a): return [list(x) for x in zip(*a)]
def plus(a,b): return [[a[i][j]+b[i][j] for j in range(3)] for i in range(3)]
def rotation(axis,angle,rate):
    c,s=math.cos(angle),math.sin(angle);i,j=(axis+1)%3,(axis+2)%3
    a=[[float(row==col) for col in range(3)] for row in range(3)];d=[[0.]*3 for _ in range(3)]
    a[i][i]=a[j][j]=c;a[i][j]=-s;a[j][i]=s
    d[i][i]=d[j][j]=-s*rate;d[i][j]=-c*rate;d[j][i]=c*rate
    return a,d


def compose(items):
    r=[[float(i==j) for j in range(3)] for i in range(3)];d=[[0.]*3 for _ in range(3)]
    for axis,angle,rate in items:
        a,b=rotation(axis,angle,rate);d=plus(mm(d,a),mm(r,b));r=mm(r,a)
    return r,d


def fixed_state(r,d,state=INPUT):
    # r maps fixed to inertial. Differentiate the inverse transformation.
    position=mv(transpose(r),state[:3]);velocity=mv(transpose(r),[v-w for v,w in zip(state[3:],mv(d,position))])
    return position+velocity


def clenshaw(coefficients,x):
    b1=b2=d1=d2=0.
    for c in reversed(coefficients[1:]):
        b=2*x*b1-b2+c;d=2*b1+2*x*d1-d2;b2,b1=b1,b;d2,d1=d1,d
    return x*b1-b2+coefficients[0],b1+x*d1-d2


class De405:
    def __init__(self,path):
        self.path=Path(path);self.data=self.path.read_bytes()
        self.start,self.end,self.span=struct.unpack_from('<3d',self.data,2652)
        self.version=struct.unpack_from('<i',self.data,2840)[0]
        self.offset,self.degree_count,self.granules=struct.unpack_from('<3i',self.data,2844)
        self.words=self.offset-1+3*self.degree_count*self.granules
        assert self.version==405 and (self.offset,self.degree_count,self.granules)==(899,10,4)
        assert self.words==1018 and len(self.data)==int((self.end-self.start)/self.span+2)*self.words*8
        self.digest=hashlib.sha256(self.data).hexdigest()
    def record(self,jd):
        index=min(int((jd-self.start)//self.span),int((self.end-self.start)/self.span)-1)
        assert index>=0 and self.start<=jd<=self.end
        return struct.unpack_from('<1018d',self.data,(index+2)*self.words*8)
    def angles(self,tdb_mjd):
        jd=tdb_mjd+2430000.;record=self.record(jd);start,end=record[:2]
        width=(end-start)/self.granules;granule=min(int((jd-start)//width),self.granules-1)
        x=2*(jd-(start+granule*width))/width-1;offset=self.offset-1+3*self.degree_count*granule
        result=[clenshaw(record[offset+i*self.degree_count:offset+(i+1)*self.degree_count],x) for i in range(3)]
        return [v for v,d in result],[d*2/(width*86400) for v,d in result]
    def orientation(self,tdb_mjd):
        angles,rates=self.angles(tdb_mjd)
        return compose(list(zip([2,0,2],angles,rates)))
    def validate_official(self,subset,header):
        text=Path(header).read_text();block=text.split('GROUP   1050')[1].split('GROUP')[0]
        rows=[[int(x) for x in line.split()] for line in block.splitlines() if line.strip()]
        assert [r[-1] for r in rows]==[self.offset,self.degree_count,self.granules]
        original=json.loads(Path(subset).read_text())
        for row in original['records']:
            actual=self.record(row['jd_start']);assert actual[:2]==(row['jd_start'],row['jd_end'])
            assert list(actual[898:1018])==row['libration_coefficients']
        return len(original['records'])*120


class Iau2002:
    def __init__(self,path):
        text=Path(path).read_text();data='\n'.join(b.split('\\begintext')[0] for b in text.split('\\begindata')[1:])
        self.values={key:[float(x.replace('D','E')) for x in body.split()] for key,body in re.findall(r'(BODY(?:301|3)_[A-Z_]+)\s*=\s*\(([^)]*)\)',data)}
        assert self.values['BODY301_POLE_RA']==[269.9949,0.0031,0.0]
        assert self.values['BODY301_PM']==[38.3213,13.17635815,-1.4e-12]
        assert len(self.values['BODY3_NUT_PREC_ANGLES'])==26
    def angles(self,tdb_mjd):
        days=tdb_mjd-21545.;centuries=days/36525;terms=self.values['BODY3_NUT_PREC_ANGLES'];result=[]
        for name in ['POLE_RA','POLE_DEC','PM']:
            t,scale=(days,1/86400) if name=='PM' else (centuries,1/(36525*86400))
            a,b,c=self.values['BODY301_'+name];value=a+b*t+c*t*t;rate=(b+2*c*t)*scale
            series=self.values['BODY301_NUT_PREC_'+{'POLE_RA':'RA','POLE_DEC':'DEC','PM':'PM'}[name]]
            for i,coefficient in enumerate(series):
                angle=(terms[2*i]+terms[2*i+1]*centuries)*RAD;speed=terms[2*i+1]*RAD/(36525*86400)
                if name=='POLE_DEC':value+=coefficient*math.cos(angle);rate-=coefficient*math.sin(angle)*speed
                else:value+=coefficient*math.sin(angle);rate+=coefficient*math.cos(angle)*speed
            result.append((value*RAD,rate*RAD))
        return result
    def orientation(self,tdb_mjd):
        (ra,dra),(dec,ddec),(w,dw)=self.angles(tdb_mjd)
        return compose([(2,ra+math.pi/2,dra),(0,math.pi/2-dec,-ddec),(2,w,dw)])


def generate(repo,reference):
    de=De405(repo/'application/data/planetary_ephem/de/leDE1941.405')
    official=de.validate_official(reference/'official-DE405-libration-subset.json',reference/'header.405')
    pck=repo/'src/qtgui/tests/fixtures/lunar/pck00008.tpc';iau=Iau2002(pck)
    # Include DE record/granule boundaries, out-of-order epochs and broad dates.
    epochs=[21545.,21536.5,21544.5-1/86400,21544.5,21544.5+1/86400,21568.5,21600.5,21545.125,21545.5,21546.25,10000.25,30000.75,50000.25,70000.5,21545.]
    cases=[]
    for model,oracle,frame in [('DE405',de,'MJ2000Eq'),('IAU2002',iau,'ICRF')]:
        rows=[]
        for t in epochs:
            if model=='IAU2002' and t==70000.5: t=57000.5
            rotation,derivative=oracle.orientation(t)
            # Orthogonality and a finite-difference derivative check use no native helper.
            unit=mm(transpose(rotation),rotation)
            assert max(abs(unit[i][j]-(i==j)) for i in range(3) for j in range(3))<1e-14
            step=0.5/86400
            before=oracle.orientation(t-step)[0];after=oracle.orientation(t+step)[0]
            dt=((t+step)-(t-step))*86400
            assert max(abs((after[i][j]-before[i][j])/dt-derivative[i][j]) for i in range(3) for j in range(3))<2e-10
            for witness,state in [('mixed',INPUT)]+[(f'basis-{k}',[(1000. if k<3 else 1.) if j==k else 0. for j in range(6)]) for k in range(6)]:
                rows.append(dict(tdb_mjd=t,witness=witness,input_state=state,fixed_state=fixed_state(rotation,derivative,state),rotation=rotation,rotation_rate=derivative))
        cases.append(dict(model=model,inertial_axes=frame,rows=rows,unsupported_icrf_epoch=70000.5 if model=='IAU2002' else None))
    return dict(scope='Lunar DE405 and IAU/IAG 2000 published in 2002; orientation model fidelity, not modern measured orientation accuracy.',input_state=INPUT,position_tolerance_km=5e-7,velocity_tolerance_km_s=5e-10,tdb_tolerance_days=2e-10,official_DE405_coefficients_checked=official,DE405_sha256=de.digest,pck_sha256=hashlib.sha256(pck.read_bytes()).hexdigest(),cases=cases)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--reference',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();repo=Path(__file__).resolve().parents[3]
    result=generate(repo,args.reference);args.output.write_text(json.dumps(result,indent=2)+'\n');print('Generated',sum(len(c['rows']) for c in result['cases']),'orientation witnesses; verified',result['official_DE405_coefficients_checked'],'official coefficients')
