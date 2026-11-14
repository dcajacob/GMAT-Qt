"""Check spacecraft-centered MJ2000Eq samples and gaps from native GUI receipts."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import re
from AuditPlotStepEphemerisEvidence import check, digest, document
from AuditPlotStepResetEvidence import invocation


CASES=['FrameFollowerForward','FrameFollowerBackward','FrameLeaderMissing','FrameLeaderThinned','FrameLiveOrigin','FrameNewArcMissing']
REGRESSIONS=['TranslatedOrigin','ResetPeer']
PROTOCOL=['bound-state-and-precision','ambiguous-state-rejected','exception-restores-outer-scope','nested-raw-isolation','nested-return-restores-origin','outside-scope-isolation']


def state(index,t):
    radius,phase=(7000,0) if index==0 else (9000,.4)
    rate=math.sqrt(398600.4415/radius**3);angle=phase+rate*t
    x,y,vx,vy=radius*math.cos(angle),radius*math.sin(angle),-radius*rate*math.sin(angle),radius*rate*math.cos(angle)
    return (x,y,0,vx,vy,0) if index==0 else (0,x,y,0,vx,vy)


def check_case(p, counts):
    name=p['case'];thin=name=='FrameLeaderThinned';leader=name.startswith('FrameLeader');newarc=name=='FrameNewArcMissing';live=name=='FrameLiveOrigin'
    check(name in CASES and p['outcome']=='passed' and p['skip']==(3 if thin else 0), 'Unexpected frame case')
    check(p['origin_checks']==(PROTOCOL if name=='FrameFollowerForward' else []),'Origin isolation protocol differs')
    leader_times=[0] if thin else [0,10,10,20] if newarc else [0,10,20,23]
    follower_times=[0,10,20] if thin else [] if live else [0,3,6,9,10,13,16] if newarc else [0,3,6,9,10,13,16,19,20,23]
    check([(c['plot'],c['object']) for c in p['outputs']]==[(v,o) for v in ('Orb','Map') for o in ('One','Two')],'Curve inventory differs')
    for curve in p['outputs']:
        orb=curve['plot']=='Orb';index=0 if curve['object']=='One' else 1
        times=leader_times if index==0 else follower_times
        if orb and index==1 and name=='FrameLeaderMissing':times=[0,10,20,23]
        if orb and index==0 and newarc:times=[0,10]
        if orb and index==0 and live:times=[0]
        points=curve['points'];check(len(points)==len(times),'Selected/missing epoch count differs')
        for i,(point,t) in enumerate(zip(points,times)):
            t=-t if name=='FrameFollowerBackward' else t
            connected=i>0
            if orb and index==1 and name=='FrameLeaderMissing':connected=i==3
            if not orb and index==0 and newarc and i==2:connected=False
            check(point['connect'] is connected and abs(point['elapsed']-t)<1e-4,'Epoch or line continuity differs')
            counts['epochs_and_connections']+=1
            xyz=[point[k] for k in ('x','y','z')];check(all(map(math.isfinite,xyz)),'Nonfinite coordinates')
            if orb:
                a=state(index,t);origin=state(0 if leader else 1,t);wanted=[x-y for x,y in zip(a,origin)]
                check(all(abs(x-y)<4e-5 for x,y in zip(xyz,wanted)), 'Relative Cartesian position differs')
                check(len(point['velocity'])==3 and all(abs(x-y)<5e-8 for x,y in zip(point['velocity'],wanted[3:])), 'Relative Cartesian velocity differs')
                counts['relative_cartesian_components']+=6
                if index==(0 if leader else 1):counts['origin_zero_states']+=1


def check_messages(case, name, counts):
    current=(case/(name+'-run-messages.txt')).read_text()
    rows=re.findall(r"Plot 'Orb' omitted a sample at (\S+) A1MJD: origin '([^']+)' has no coherent state for frame 'RelativeFrame'\.",current)
    expected={'FrameLeaderMissing':([3,13],'One'),'FrameLiveOrigin':([10],'Two'),'FrameNewArcMissing':([10],'Two')}
    times,who=expected.get(name,([],''))
    check(len(rows)==len(times),'Missing-origin warning inventory differs')
    for (epoch,origin),t in zip(rows,times):
        check(origin==who and abs((float(epoch)-21545)*86400-t)<1e-4,'Missing-origin warning epoch/spacecraft differs')
        counts['missing_origin_warnings']+=1


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--manifest',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);a=parser.parse_args();m=document(a.manifest)
    check(digest(Path(__file__))==m['auditor_sha256'],'Auditor differs')
    for name,sha in m['artifacts'].items():check(digest(Path(name))==sha,'Artifact differs '+name)
    counts=Counter();seen=set()
    for run in m['runs']:
        root=Path(run['directory']);case=root/'QtGui.PlotIntegrationSteps';mode=run['mode'];result=invocation(root,mode,case.name,counts)
        archive=a.manifest.parent/run['version']
        for name,sha in document(archive/'hashes.json').items():check(sha==digest(archive/name)==result['source_hashes'][name],'Executed source differs '+name)
        proof=document(case/'proof.json');logs=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in (case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
        check(logs==proof['cases'] and [p['case'] for p in logs]==REGRESSIONS+CASES and proof['new_requirement_credit']==0,'Case inventory differs')
        for p in logs:
            key=p['case'];check(document(case/(key+'-proof.json'))==p and document(case/(key+'-points.json'))==p['outputs'],'Raw plots/proof differ')
            if key in REGRESSIONS:
                check(p['outputs']==document(Path(m['prior_regressions'][mode][key]))['outputs'],'Affected regression output differs');counts['matched_regressions']+=1
            else:check_case(p,counts);check_messages(case,key,counts);seen.add((mode,key))
    check(seen=={(mode,key) for mode in ('headless','visible') for key in CASES},'Final frame cases missing')
    reg=Path(m['availability_run']);mode=document(reg/'QtGui.OrbitDataAvailability/result.json')['requested_visibility'];invocation(reg,mode,'QtGui.OrbitDataAvailability',counts)
    b=document(Path(m['baseline']));check(b['coverage']['requirements']['verified']==1009 and b['coverage']['requirements']['total']==1158 and b['remaining_counts']['requirements']==149,'Canonical ledger differs')
    out=dict(bounded_evidence_status='passed',checked=dict(counts),new_requirement_credit=0,manifest_sha256=digest(a.manifest));a.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))


if __name__=='__main__':main()
