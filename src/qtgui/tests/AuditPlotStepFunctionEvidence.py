"""Audit caller and callee object identity, function results, plot histories and local subscriptions."""
import argparse
from collections import Counter
import csv
import json
import math
from pathlib import Path
from AuditPlotStepEphemerisEvidence import check, digest, document
from AuditPlotStepResetEvidence import invocation


CASES=['FunctionShadow','FunctionGlobalTwice','FunctionNested','FunctionLocalPlot']


def state(peer,t):
    r,phase=(9000,.4) if peer else (7000,0)
    rate=math.sqrt(398600.4415/r**3);a=phase+t*rate
    x,y,vx,vy=r*math.cos(a),r*math.sin(a),-r*rate*math.sin(a),r*rate*math.cos(a)
    return (0,x,y,0,vx,vy) if peer else (x,y,0,vx,vy,0)


def check_case(p,counts):
    name=p['case'];shadow=name in ('FunctionShadow','FunctionLocalPlot');nested=name=='FunctionNested'
    arcs=[('',[0,10])]
    if shadow:
        arcs += [('StepArc',[0,10,20,27]),('',[10,20,30,37])];returned=[27,27];caller=[10,37];selected=[0,20,37]
    elif nested:
        arcs += [('OuterArc',[10,20]),('StepArc',[20,30,40,47]),('',[47,57,60])];returned=[47,47];caller=[47,60];selected=[0,20,40,57]
    else:
        arcs += [('StepArc',[10,20,30,37]),('StepArc',[37,47,57,64]),('',[64,74,77])];returned=[37,64,64];caller=[37,64,77];selected=[0,20,37,57,74]
    packets=p['function_packets'];expected=[(fn,i,t) for fn,times in arcs for i,t in enumerate(times)]
    check(len(packets)==len(expected),'Packet inventory differs')
    for packet,(fn,i,t) in zip(packets,expected):
        local=bool(fn) and shadow;identity=2 if local else 1
        check(packet['function']==fn and packet['initial'] is (i==0) and packet['roots']==[identity],'Caller/callee identity differs')
        check(abs(packet['raw_elapsed']-t)<1e-4 and len(packet['steps'])==1,'Raw publication grid differs')
        sample=packet['steps'][0];truth=state(local,t)
        check(sample['object_id']==identity and sample['object']=='One' and sample['initial'] is (i==0) and abs(sample['elapsed']-t)<1e-4,'Step identity/epoch differs')
        check(abs(sample['y']-truth[1])<4e-5 and abs(sample['vz']-truth[5])<5e-8,'Sampled function state differs')
        counts['provider_packets']+=1
        if name=='FunctionLocalPlot':
            check(sorted(packet['local_subscribers'])==(['InnerMap','InnerOrb'] if fn else []),'Local registrations differ')
            if fn and t==27:
                views=packet['local_views'];check([v['plot'] for v in views]==['InnerOrb','InnerMap'],'Local plot set differs')
                for view in views:
                    check(view['present'] and len(view['points'])==2,'Local spacecraft missing')
                    for j,point in enumerate(view['points']):
                        check(abs(point['elapsed']-20*j)<1e-4 and point['connect'] is bool(j),'Local membership or connection differs')
                        xyz=[point[k] for k in ('x','y','z')];check(all(map(math.isfinite,xyz)),'Local point nonfinite')
                        if view['plot']=='InnerOrb':check(all(abs(v-w)<4e-5 for v,w in zip(xyz,state(True,20*j))),'Local trajectory differs')
                        counts['local_plot_epochs']+=1
    rows=p['function_report'];check(len(rows)==len(returned),'Function return count differs')
    for row,r,t in zip(rows,returned,caller):
        local=state(shadow,r);wanted=[r,local[1],local[5],t,state(False,t)[1],0,state(True,0)[1]]
        check(len(row)==7 and all(abs(v-w)<tol for v,w,tol in zip(row,wanted,[1e-4,4e-5,5e-8,1e-4,4e-5,1e-4,4e-5])),'Function result or caller orbit differs')
        counts['report_values']+=7
    check([(c['plot'],c['object']) for c in p['outputs']]==[(kind,obj) for kind in ('Orb','Map') for obj in ('One','Two')],'Main curve inventory differs')
    for c in p['outputs']:
        times=selected if c['object']=='One' else [];check(len(c['points'])==len(times),'Caller cadence differs')
        for point,t in zip(c['points'],times):
            check(abs(point['elapsed']-t)<1e-4,'Caller epoch differs');xyz=[point[k] for k in ('x','y','z')]
            check(all(map(math.isfinite,xyz)),'Caller plot nonfinite')
            if c['plot']=='Orb':check(all(abs(v-w)<4e-5 for v,w in zip(xyz,state(False,t))),'Caller plotted orbit differs')
            counts['caller_plot_epochs']+=1


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--manifest',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();m=document(a.manifest)
    check(digest(Path(__file__))==m['auditor_sha256'],'Auditor differs')
    for name,sha in m['artifacts'].items():check(digest(Path(name))==sha,'Changed artifact '+name)
    counts=Counter();seen=set()
    for run in m['runs']:
        root=Path(run['directory']);case=root/'QtGui.PlotIntegrationSteps';mode=run['mode'];result=invocation(root,mode,case.name,counts)
        archive=a.manifest.parent/run['version']
        for name,sha in document(archive/'hashes.json').items():check(digest(archive/name)==sha==result['source_hashes'][name],'Executed source differs '+name)
        proof=document(case/'proof.json');logs=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in (case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
        check(logs==proof['cases'] and proof['new_requirement_credit']==0 and [p['case'] for p in logs]==run['cases'],'Case evidence differs')
        for p in logs:
            key=p['case'];check(document(case/(key+'-proof.json'))==p and document(case/(key+'-points.json'))==p['outputs'] and document(case/(key+'-packets.json'))==p['function_packets'],'Raw case records differ')
            rows=[[float(v) for v in row] for row in csv.reader((case/(key+'-report.csv')).read_text().splitlines())];check(rows.pop(0)==[1],'Report arm sentinel differs')
            for row in rows:
                for j in (0,3,5):row[j]=(row[j]-21545)*86400
            check(rows==p['function_report'],'Raw report differs');check_case(p,counts);seen.add((mode,key))
    check(seen=={(mode,key) for mode in ('headless','visible') for key in CASES},'Native cases incomplete')
    reg=Path(m['original_regression']);mode=document(reg/'QtGui.GlobalScopes/result.json')['requested_visibility'];invocation(reg,mode,'QtGui.GlobalScopes',counts)
    b=document(Path(m['baseline']));check(b['coverage']['requirements']['verified']==1009 and b['coverage']['requirements']['total']==1158 and b['remaining_counts']['requirements']==149,'Canonical ledger differs')
    out=dict(bounded_evidence_status='passed',new_requirement_credit=0,checked=dict(counts),manifest_sha256=digest(a.manifest));a.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))


if __name__=='__main__':main()
