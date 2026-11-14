"""Check tabulated ephemeris source states, published primitive endpoints and plotted states."""
import argparse
from collections import Counter
from datetime import datetime, timezone
import json
import math
from pathlib import Path
from AuditPlotStepEphemerisEvidence import cartesian, check, digest, document
from AuditPlotStepResetEvidence import invocation
from BatchFormatOracle import BASE, code500

LABELS = ['StkForward','StkBackward','OemForward','OemBackward','Code500LittleForward','Code500BigBackward','StkMixed','OemMixed','Code500Mixed']
SCENARIO = datetime(2000,1,1,11,55,tzinfo=timezone.utc)
TEXT_KNOTS = [0,31,87,132,194,231,260,283,318,374,451,499,600]


def compare(vector,t,counts,key,obj='One'):
    expected=cartesian(obj,t)
    check(len(vector)==6 and all(abs(x-y)<(4e-5 if i<3 else 5e-8) for i,(x,y) in enumerate(zip(vector,expected))), 'Table Cartesian state differs')
    counts[key]+=6


def fixture(case,name,counts):
    if 'Code500' in name:
        rows,_=code500((case/(name+'.ephem')).read_bytes(),{'byte_order':'BigEndian' if 'Big' in name else 'LittleEndian'})
        samples=[(point[0]-(SCENARIO-BASE).total_seconds(),point[1:]) for point in rows]
        knots=list(range(0,601,20))
    elif 'Stk' in name:
        data=(case/(name+'.e')).read_text()
        check('NumberOfEphemerisPoints 13' in data and 'ScenarioEpoch 01 Jan 2000 11:55:00.000' in data and 'CoordinateSystem J2000' in data and 'DistanceUnit Kilometers' in data,'STK header differs')
        rows=data.split('EphemerisTimePosVel\n')[1].split('END Ephemeris')[0].strip().splitlines()
        samples=[(float(r.split()[0]),[float(x) for x in r.split()[1:]]) for r in rows];knots=TEXT_KNOTS
    else:
        data=(case/(name+'.oem')).read_text()
        check(all(k in data for k in ('CCSDS_OEM_VERS = 1.0','REF_FRAME = EME2000','TIME_SYSTEM = UTC','CENTER_NAME = Earth','INTERPOLATION_DEGREE = 5')),'OEM metadata differs')
        samples=[]
        for row in data.split('META_STOP')[1].strip().splitlines():
            stamp,*state=row.split()
            offset=(datetime.fromisoformat(stamp).replace(tzinfo=timezone.utc)-SCENARIO).total_seconds()
            samples.append((offset,list(map(float,state))))
        knots=TEXT_KNOTS
    check(len(samples)==len(knots),'Fixture knot count differs')
    for (offset,vector),knot in zip(samples,knots):
        check(abs(offset-knot)<.001,'Fixture epoch differs')
        compare(vector,knot-300+32.0343817,counts,'fixture_components')


def verify(case,proof,counts):
    name=proof['case'];check(name in {'Ephem'+n for n in LABELS} and proof['outcome']=='passed' and proof['skip']==1,'Case identity differs')
    fixture(case,name,counts)
    direction=-1 if name.endswith('Backward') else 1
    method='Code500' if 'Code500' in name else 'STK' if 'Stk' in name else 'CCSDS-OEM'
    mixed=name.endswith('Mixed')
    grid_one=[0,10,20,23] if mixed else [0,10,10,20,30,37]
    grid_two=[0,3,6,9,10,13,16,19,20,23] if mixed else []
    events=document(case/(name+'-endpoints.json'))
    check(events==proof['ephemeris_endpoints'] and len(events)==len(grid_one)+len(grid_two),'Endpoint inventory differs')
    for obj,grid in [('One',grid_one),('Two',grid_two)]:
        rows=[row for row in events if row['objects']==[obj]]
        check(len(rows)==len(grid),'Provider grid cardinality differs')
        for i,(r,t) in enumerate(zip(rows,grid)):
            initial=i==0 or (not mixed and i==2)
            start=t if initial else grid[i-1]
            base=10 if (not mixed and i>=2) else 0
            check(r['ephemeris'] is (obj=='One') and r['method']==(method if obj=='One' else 'RungeKutta89') and r['initial'] is initial,'Endpoint provider differs')
            for key,v in [('elapsed',t),('start',start-base),('end',t-base)]:
                check(abs(r[key]-direction*v)<1e-4,'Endpoint clock differs')
            check(len(r['states'])==1,'Endpoint state count differs')
            compare(r['states'][0],direction*t,counts,'published_components',obj)
            counts['provider_records']+=1
    curves=document(case/(name+'-points.json'))
    check(curves==proof['outputs'] and [(r['plot'],r['object']) for r in curves]==[(p,o) for p in ('Orb','Map') for o in ('One','Two')],'Plot inventory differs')
    for curve in curves:
        grid=([0,20] if mixed else [0,20,37]) if curve['object']=='One' else ([0,6,10,16,20] if mixed else [])
        check(len(curve['points'])==len(grid),'Plot point count differs')
        for point,t in zip(curve['points'],grid):
            xyz=[point[k] for k in ('x','y','z')]
            check(abs(point['elapsed']-direction*t)<1e-4 and all(map(math.isfinite,xyz)),'Plot point differs')
            if curve['plot']=='Orb':
                compare(xyz+point['velocity'],direction*t,counts,'plotted_components',curve['object'])
            counts['plotted_epochs']+=1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();m=document(args.manifest)
    check(m['auditor_sha256']==digest(Path(__file__)),'Auditor changed')
    for p,sha in m['artifacts'].items():check(digest(Path(p))==sha,'Artifact changed '+p)
    seen=set();controls=set();counts=Counter()
    for run in m['runs']:
        root=Path(run['directory']);mode=run['mode'];case=root/'QtGui.PlotIntegrationSteps'
        result=invocation(root,mode,case.name,counts)
        version=args.manifest.parent/run['version']
        for p,sha in document(version/'hashes.json').items():
            check(sha==digest(version/p)==result['source_hashes'][p], 'Executed source differs '+p)
        logged=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in (case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
        proof=document(case/'proof.json')
        check(proof['cases']==logged and proof['new_requirement_credit']==0,'Native log differs')
        for p in logged:
            name=p['case'];check(document(case/(name+'-proof.json'))==p,'Saved proof differs')
            if name in ('EphemType9Forward','EphemType13Mixed'):
                prior=document(Path(m['spk_baselines'][mode][name]))
                check(p['outputs']==prior['outputs'] and p['ephemeris_endpoints']==prior['ephemeris_endpoints'],'SPK regression differs')
                controls.add((mode,name));continue
            check((name,mode) not in seen,'Duplicate provider case');verify(case,p,counts);seen.add((name,mode))
    check(seen=={('Ephem'+n,m) for n in LABELS for m in ('visible','headless')} and controls=={(m,n) for m in ('visible','headless') for n in ('EphemType9Forward','EphemType13Mixed')},'Provider matrix incomplete')
    ledger=document(Path(m['baseline']))
    check(ledger['coverage']['requirements']['verified']==1009 and ledger['coverage']['requirements']['total']==1158 and ledger['remaining_counts']['requirements']==149,'Canonical ledger differs')
    out=dict(bounded_evidence_status='passed',manifest_sha256=digest(args.manifest),checked=dict(counts),new_requirement_credit=0)
    args.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))


if __name__=='__main__':
    main()
