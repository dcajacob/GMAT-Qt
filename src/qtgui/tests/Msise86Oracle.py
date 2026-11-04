"""Independently check GUI density using unchanged NASA Fortran in a subprocess.

Inputs come from literal weather choices, Python civil dates and a separately
implemented inverse ellipsoid calculation on the retained EarthFixed report.
No GMAT clock, geodetic, atmosphere or force routine supplies expected density.
The original NASA test table separately validates the upstream model/build.
"""
import argparse
import datetime as dt
import hashlib
import json
import math
from pathlib import Path
import subprocess
import tempfile


def geodetic(x,y,z,a,f):
    e2=f*(2-f);r=math.hypot(x,y);phi=math.atan2(z,r*(1-e2))
    for _ in range(30):
        n=a/math.sqrt(1-e2*math.sin(phi)**2)
        new=math.atan2(z+e2*n*math.sin(phi),r)
        if abs(new-phi)<1e-14:phi=new;break
        phi=new
    n=a/math.sqrt(1-e2*math.sin(phi)**2)
    return r*math.cos(phi)+z*math.sin(phi)-a*math.sqrt(1-e2*math.sin(phi)**2),math.degrees(phi),math.degrees(math.atan2(y,x))


def check(runs,repo):
    source=repo/'plugins/Msise86Plugin/src/base/model/msis86.for';data=source.with_suffix('.dat');driver=repo/'src/qtgui/tests/fixtures/msise86/query.for';matrix=json.loads((repo/'src/qtgui/tests/NasaMsise86Cases.json').read_text());pending=[]
    for run in runs:
        receipt=json.loads((run/'results.json').read_text());assert receipt.get('finished_utc')
        for case in receipt['results']:
            if not case['name'].startswith('QtGui.NasaMsise86') or case['status']=='not-run':continue
            assert case['status']=='passed',case['name'];directory=Path(case['log']).parent
            spec=json.loads((directory/'case.json').read_text());assert spec in matrix['cases']
            for phase in [0,1]:
                rows=[[float(v) for v in line.split(',')] for line in (directory/f'{phase}-values.csv').read_text().splitlines() if line.strip()]
                assert len(rows)==2 and len(rows[0])==9 and rows[1]==[42.]
                raw=rows[0];assert raw[8]==41 and abs(raw[0]-spec['utc_mjd']-.001/86400)<2e-10
                height,latitude,longitude=geodetic(*raw[1:4],matrix['earth_radius_km'],spec['flattening'])
                assert abs(height-spec['altitude_km'])<.02 and abs(latitude-spec['latitude_deg'])<.001
                when=dt.datetime(1941,1,5,12)+dt.timedelta(days=raw[0]);seconds=(when-when.replace(hour=0,minute=0,second=0,microsecond=0)).total_seconds();day=when.timetuple().tm_yday
                # IAGA/ISGI Kp->ap table: 1o ->4 and 4o ->27.
                ap={1.:4.,4.:27.}[spec['kp']]
                inputs=[when.year*1000+day,seconds,height,latitude,longitude,seconds/3600+longitude/15,spec['f107a'],spec['f107'],ap]
                pending.append(dict(run=str(run),case=spec['key'],phase=phase,raw=raw,inputs=inputs))
    assert pending
    with tempfile.TemporaryDirectory(prefix='gmat-msise86-oracle-') as temp:
        root=Path(temp);(root/'MSIS86.DAT').symlink_to(data);exe=root/'query'
        build=subprocess.run(['gfortran','-std=legacy','-fno-automatic','-ffixed-line-length-none',str(source),str(driver),'-o',str(exe)],capture_output=True,text=True);assert build.returncode==0,build.stderr
        original=subprocess.run([str(exe)],input=''.join(' '.join(format(v,'.17g') for v in row['inputs'])+'\n' for row in pending),cwd=root,capture_output=True,text=True);assert original.returncode==0,original.stderr
        numbers=[[float(v) for v in line.split()] for line in original.stdout.splitlines() if line.strip()];assert len(numbers)==len(pending) and all(len(row)==10 for row in numbers)
    maximum=0.
    for row,values in zip(pending,numbers):
        expected=values[5]*1000;actual=row['raw'][7]/1e9;error=abs(actual-expected)/expected;maximum=max(maximum,error)
        row.update(expected_density_kg_m3=expected,relative_error=error)
        assert expected>0 and error<=5e-6,(row['case'],row['phase'],actual,expected,error,row['inputs'])
    return dict(status='passed',native_vectors=len(pending),relative_tolerance=5e-6,max_relative_error=maximum,source_sha256={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [source,data,driver]},records=pending,compiler_stderr=build.stderr)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--run',type=Path,action='append',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=check([v.resolve() for v in a.run],Path(__file__).resolve().parents[3]);a.output.write_text(json.dumps(result,indent=2)+'\n');print('Independent MSIS-86:',result['native_vectors'],'vectors; maximum relative error',result['max_relative_error'])
