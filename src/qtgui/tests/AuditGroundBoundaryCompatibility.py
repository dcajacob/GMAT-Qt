"""Audit the original split-command gate, publication cadence and identity controls."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
from AuditPlotStepEphemerisEvidence import check, digest, document
from AuditPlotStepResetEvidence import invocation


GATE='QtGui.NasaGroundIntegrationSkipBoundary'
ORIGINAL_SHA='b4d640e8b425682f5f95c72b3bfa0cf7d93478d1e36e132881b6a5804a7a759d'
CONTROLS=['LegacyChangedState','LegacyEveryPacket','LegacyReverseBoundary']
REGRESSIONS=['QtGui.NativeNasaPlotCadence','QtGui.PlotSetup',
             *['QtGui.NasaGroundTrackBodySymbols'+v for v in ('Dense','Sparse')],
             *['QtGui.NativeNasaSolverPublication'+v for v in ('DC','Yukon','Nested','Mixed')]]


def audit_boundary(rows):
    expected=[0,10,*range(10,611,10)]
    if len(rows)!=len(expected):raise ValueError('Original boundary stream length differs')
    frame=0;maxpos=maxvel=0
    for ordinal,(row,seconds) in enumerate(zip(rows,expected),1):
        selected=seconds%20==0
        frame+=int(selected)
        if not (row['publication']==ordinal and row['integration_step']==seconds//10 and abs(row['elapsed']-seconds)<1e-4 and row['selected'] is selected and row['frame']==frame):raise ValueError('Boundary selection/epoch differs')
        rate=math.sqrt(398600.4415/7000**3);angle=rate*seconds
        truth=(7000*math.cos(angle),7000*math.sin(angle),0,-7000*rate*math.sin(angle),7000*rate*math.cos(angle),0)
        if len(row['state'])!=6:raise ValueError('Raw position/velocity schema differs')
        for i,(v,w) in enumerate(zip(row['state'],truth)):
            if not math.isfinite(v) or abs(v-w)>(3e-5 if i<3 else 4e-8):raise ValueError('Raw circle differs')
            if i<3:maxpos=max(maxpos,abs(v-w))
            else:maxvel=max(maxvel,abs(v-w))
    return dict(publications=len(rows),collected=frame,cartesian_components=len(rows)*6,max_position_error_km=maxpos,max_velocity_error_km_s=maxvel)


def check_source(result, archive):
    for path,sha in document(archive/'hashes.json').items():
        check(digest(archive/path)==sha==result['source_hashes'][path],'Source identity differs '+path)


def check_controls(proof, counts):
    name=proof['case']
    check(name in CONTROLS and proof['outcome']=='passed','Wrong compatibility case')
    fixed={'LegacyChangedState': ([0,10,30],[0,10,30]),
           'LegacyEveryPacket': ([0,10,20,30,37],[0,10,10,20,30,37]),
           'LegacyReverseBoundary': ([0,0],[0,10,-10])}
    check([(v['plot'],v['object']) for v in proof['outputs']]==[(p,o) for p in ('Orb','Map') for o in ('One','Two')],'Curve inventory differs')
    for curve in proof['outputs']:
        orb=curve['plot']=='Orb'
        times=fixed[name][0 if orb else 1] if curve['object']=='One' else []
        check(len(times)==len(curve['points']),'Curve count differs')
        for i,(point,t) in enumerate(zip(curve['points'],times)):
            check(abs(point['elapsed']-t)<1e-4,'Membership epoch differs')
            check(all(math.isfinite(point[k]) for k in ('x','y','z')),'Nonfinite coordinates')
            counts['control_epochs']+=1
            if name=='LegacyChangedState':
                rate=math.sqrt(1000/1500**3);angle=.7+rate*(t+(2000 if i else 0));c=math.cos(angle);s=math.sin(angle)
                if not orb:
                    check(abs(point['x']-math.degrees(math.atan2(s*math.sqrt(3)/2,c)))<2e-6 and abs(point['y']-math.degrees(math.asin(s/2)))<2e-6,'Changed-state geography differs')
                    counts['geographic_components']+=2
                xyz=[1500*c,1500*s*math.sqrt(3)/2,750*s]
            else:
                rate=math.sqrt(398600.4415/7000**3);angle=rate*t;xyz=[7000*math.cos(angle),7000*math.sin(angle),0]
            if orb:
                check(all(abs(point[k]-v)<4e-5 for k,v in zip(('x','y','z'),xyz)),'Control trajectory differs')
                counts['control_positions']+=3
    protocol=proof['boundary_protocol']
    if name=='LegacyEveryPacket':
        expected=[('ordinary',False,False,False),('matching-start',True,False,True),('ordinary-repeat',False,True,False),('same-name-distinct-object',True,True,False),('changed-state',True,False,False),('reversal',True,True,False),('trial',True,False,False),('resume-after-trial',True,True,False)]
        check([(p['case'],p['initial'],p['step_metadata'],p['repeated']) for p in protocol]==expected,'Boundary protocol matrix differs')
        counts['protocol_cases']+=len(protocol)
    else:check(not protocol,'Unexpected protocol record')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();m=document(args.manifest);stage=args.manifest.parent
    check(digest(Path(__file__))==m['auditor_sha256'],'Auditor differs')
    for path,sha in m['artifacts'].items():check(digest(Path(path))==sha,'Artifact differs '+path)
    failed=Path(m['original_failure'])/GATE
    check(document(failed/'result.json')['status']=='failed','Original failure missing')
    rows=document(failed/'publications.json')
    check([(r['integration_step'],r['selected']) for r in rows[:3]]==[(0,True),(1,False),(1,True)],'Original counterexample differs')
    counts=Counter();observations={}
    for run in m['gate_runs']:
        root=Path(run['directory']);case=root/GATE;mode=run['mode']
        result=invocation(root,mode,GATE,counts);check_source(result,stage/run['version'])
        check(result['source_hashes']['src/qtgui/tests/NasaGroundTrackIntegrationSkipTests.cpp']==ORIGINAL_SHA,'Original gate changed')
        proof=document(case/'proof.json');rows=document(case/'publications.json')
        logged=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in (case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
        check(logged==[proof] and proof['publications']==rows and proof['outcome']=='passed' and proof['native_interval']==2 and proof['requested_skipped_steps']==1,'Original proof differs')
        observations[root.name]=audit_boundary(rows)
    original_runs=m['original_regression_runs']
    for run in original_runs:
        root=Path(run['directory'])
        for test in REGRESSIONS:
            receipt=document(root/test/'result.json');mode=receipt['requested_visibility']
            result=invocation(root,mode,test,counts)
            check_source(result,stage/run['version']);counts['regression_passes']+=1
            if test=='QtGui.NativeNasaPlotCadence':
                rows=document(root/test/'cadence-collection-diagnostics.json')
                for i,row in enumerate(rows,1):
                    check(row['publication']==i and row['Map']==(i+1)//2 and row['Orb']==1+i//2,'Legacy publication cadence changed')
                counts['cadence_publications']+=len(rows)
    seen=set()
    for run in m['controls']:
        root=Path(run['directory']);case=root/'QtGui.PlotIntegrationSteps';mode=run['mode']
        result=invocation(root,mode,case.name,counts);check_source(result,stage/run['version'])
        proof=document(case/'proof.json');logged=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in (case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
        check(logged==proof['cases'] and [p['case'] for p in logged]==CONTROLS and proof['new_requirement_credit']==0,'Control cases differ')
        for p in logged:
            key=p['case'];script=(case/(key+'.script')).read_text()
            check('Map.IntegrationStepsToSkip = -1;' in script and ('Map.DataCollectFrequency = '+('1' if key=='LegacyEveryPacket' else '2')+';') in script,'Legacy control silently migrated')
            check(document(case/(key+'-proof.json'))==p and document(case/(key+'-points.json'))==p['outputs'],'Control raw evidence differs')
            check_controls(p,counts);seen.add((mode,key))
    check(seen=={(mode,key) for mode in ('visible','headless') for key in CONTROLS},'Control delivery incomplete')
    baseline=document(Path(m['baseline']))
    check(baseline['coverage']['requirements']['verified']==1009 and baseline['coverage']['requirements']['total']==1158 and baseline['remaining_counts']['requirements']==149,'Canonical ledger differs')
    out=dict(bounded_evidence_status='passed',original_gate='passed at recorded sources',original_gate_runs=observations,new_requirement_credit=0,checked=dict(counts),manifest_sha256=digest(args.manifest))
    args.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))


if __name__=='__main__':main()
