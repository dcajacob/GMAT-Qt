"""Independent Earth FK5 state reference from ERFA and daily IERS inputs.

No GMAT coordinate/time routine or ICRF table supplies expected values. This
uses IAU1976/1980, NASA's stated pre-1997 equinox policy and sidereal-only rate.
"""
import argparse
import bisect
import hashlib
import json
import math
from pathlib import Path
import re
import erfa
import numpy as np

JD=2430000.0
DAY=86400.
ARCSEC=math.pi/648000


def read_eop(path):
    result=[]
    for line in path.read_text().splitlines():
        v=line.split()
        if len(v)<8 or not v[0].isdigit() or len(v[0])!=4:continue
        year,month,day=map(int,v[:3]);mjd,x,y,dut1,lod=map(float,v[3:8]);assert erfa.cal2jd(year,month,day)[1]==mjd
        dat=float(erfa.dat(year,month,day,0.));result.append(dict(mjd=mjd,x=x,y=y,dut1=dut1,lod=lod,dat=dat,tai=mjd+dat/DAY))
    assert result and all(b['mjd']-a['mjd']==1 for a,b in zip(result,result[1:]))
    return result


def clock(tai_gmat,eop):
    tai_mjd=tai_gmat+(JD-2400000.5)
    assert eop[0]["tai"]<=tai_mjd<=eop[-1]["tai"],"Outside EOP coverage"
    midnight=math.floor(tai_gmat+.5)-.5
    tai=(JD+midnight,tai_gmat-midnight)
    tt=erfa.taitt(*tai);utc=erfa.taiutc(*tai)
    # ERFA UTC is quasi-JD on leap days. Decode calendar to nominal UTC day
    # fraction for x/y; continuous TAI interpolation is used for UT1-TAI.
    year,month,day,parts=erfa.d2dtf('UTC',9,*utc);hh,mm,ss,ns=map(int,parts);utc_day=float(erfa.cal2jd(int(year),int(month),int(day))[1]);utc_fraction=(hh*3600+mm*60+ss+ns/1e9)/DAY;utc_mjd=utc_day+utc_fraction
    tai_mjd=tai_gmat+(JD-2400000.5)
    assert eop[0]['tai']<=tai_mjd<=eop[-1]['tai'],'Outside EOP coverage'
    i=min(len(eop)-2,max(0,bisect.bisect_right([r['tai'] for r in eop],tai_mjd)-1));a,b=eop[i:i+2]
    alpha=(tai_mjd-a['tai'])/(b['tai']-a['tai']);ut1_tai=(1-alpha)*(a['dut1']-a['dat'])+alpha*(b['dut1']-b['dat'])
    ut1=erfa.taiut1(*tai,ut1_tai)
    j=min(len(eop)-2,max(0,bisect.bisect_right([r['mjd'] for r in eop],utc_day)-1));left,right=eop[j:j+2];f=(utc_day-left['mjd'])+utc_fraction
    return dict(tai=list(tai),tt=list(tt),ut1=list(ut1),utc_mjd=utc_mjd,utc_day=utc_day,utc_fraction=utc_fraction,x=(1-f)*left['x']+f*right['x'],y=(1-f)*left['y']+f*right['y'],lod=left['lod'],ut1_minus_tai=ut1_tai,eop_brackets=[a,b,left,right])


def reduction(c):
    tt=c['tt'];ut1=c['ut1'];pn=erfa.pnm80(*tt)
    eq=erfa.eqeq94(*tt) if tt[0]+tt[1]>2450449.5 else erfa.nut80(*tt)[0]*math.cos(erfa.obl80(*tt))
    theta=erfa.gmst82(*ut1)+eq;cs,sn=math.cos(theta),math.sin(theta)
    spin=np.array([[cs,sn,0],[-sn,cs,0],[0,0,1.]])
    # NASA MathematicalSpecification CoordinateSystems.tex: PM=R2(-xp)R1(-yp).
    # These are passive rotations, with no IAU2000 TIO locator s'.
    xp,yp=c['x']*ARCSEC,c['y']*ARCSEC
    cx,sx,cy,sy=math.cos(xp),math.sin(xp),math.cos(yp),math.sin(yp)
    ry=np.array([[cx,0,sx],[0,1,0],[-sx,0,cx]])
    rx=np.array([[1,0,0],[0,cy,-sy],[0,sy,cy]])
    pole=ry@rx;rate=7.29211514670698e-5*(1-c['lod']/DAY)
    derivative=rate*np.array([[-sn,cs,0],[-cs,-sn,0],[0,0,0.]])
    rot=pole@spin@pn;dot=pole@derivative@pn
    assert np.max(abs(rot@rot.T-np.eye(3)))<2e-15
    return rot,dot


def coefficient_identity(native,reference):
    table=reference.read_text().split('} x[] = {',1)[1].split('};',1)[0]
    official=[tuple(float(v) for v in body.split(',')) for body in re.findall(r'\{([^{}]+)\}',table)]
    native_text=native.read_text().split('xx 1980 IAU',1)[1].split('xx 1996 IAU',1)[0]
    observed=[tuple(map(float,line.split()[:9])) for line in native_text.splitlines() if line.strip() and line.split()[0].lstrip('-').isdigit()]
    assert len(official)==len(observed)==106 and sorted(official)==sorted(observed),'IAU1980 coefficients differ'
    return 106


def generate(repo,reference):
    assert erfa.__version__=='2.0.1.5' and erfa.version.erfa_version=='2.0.1' and erfa.version.sofa_version=='20231011'
    eop_path=repo/'application/data/planetary_coeff/eopc04_08.62-now';eop=read_eop(eop_path)
    coefficients=coefficient_identity(repo/'application/data/planetary_coeff/NUTATION.DAT',reference/'nut80.c')
    # Predeclare numeric limits before any native comparison. Position allows
    # 1 mm on 10,000 km basis vectors for split/double epoch arithmetic; the
    # velocity limit is 0.2 micrometres/s. No fitted offsets or residuals.
    dates=[(2000,1,1,12,0,0.),(1980,3,21,6,15,0.),(1996,12,31,23,58,0.),(1997,1,1,0,2,0.),
           (1998,12,31,23,59,59.5),(1999,1,1,0,0,.5),(2016,12,31,12,0,0.),(2016,12,31,23,59,59.5),(2017,1,1,0,0,.5),
           (2024,4,8,18,17,23.25),(2004,4,6,0,0,0.),(2004,4,6,12,0,0.),(2004,4,7,0,0,0.),(2004,4,6,0,0,0.),(2000,1,1,12,0,0.)]
    rows=[];epochs=[]
    states=[[7000.,-1200.,900.,-1.2,7.4,.6]]+[[10000.*float(i==j) if i<3 else 10.*float(i==j) for i in range(6)] for j in range(6)]
    for index,date in enumerate(dates):
        utc=erfa.dtf2d('UTC',*date);tai=erfa.utctai(*utc);epoch=float((tai[0]-JD)+tai[1]);c=clock(epoch,eop);rot,dot=reduction(c)
        epochs.append(dict(index=index,utc=list(date),tai_mjd=epoch,clock=c,rotation=rot.tolist(),derivative=dot.tolist()))
        for witness,state in enumerate(states):
            state=np.array(state);out=np.r_[rot@state[:3],rot@state[3:]+dot@state[:3]]
            rows.append(dict(epoch_index=index,witness='mixed' if witness==0 else 'basis-'+str(witness-1),tai_mjd=epoch,input_state=state.tolist(),fixed_state=out.tolist()))
    return dict(model='Earth-FK5-IAU1976-1980',position_tolerance_km=1e-6,velocity_tolerance_km_s=2e-10,time_tolerance_days=1e-11,official_nutation_terms=coefficients,eop_sha256=hashlib.sha256(eop_path.read_bytes()).hexdigest(),epochs=epochs,rows=rows,scope='Model fidelity in retained EOP domain with explicit NASA equinox/rate conventions; no ICRF table or physical forecasting accuracy claim.')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();j=generate(Path(__file__).resolve().parents[3],a.reference);a.output.write_text(json.dumps(j,indent=2)+'\n');print('Generated',len(j['rows']),'independent Earth FK5 states; official terms',j['official_nutation_terms'])
