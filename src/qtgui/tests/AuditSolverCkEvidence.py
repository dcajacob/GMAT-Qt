"""Independently decode the new CK packets; do not launch native tests."""
from pathlib import Path
import argparse
import copy
import hashlib
import json
import math
import re
import struct
import tarfile


def require(value,message):
    if not value:raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def decode(payload):
    require(payload[:8]==b'DAF/CK  'and payload[88:96]==b'LTL-IEEE','Wrong CK file architecture')
    require(struct.unpack_from('<ii',payload,8)==(2,6),'Wrong CK summary dimensions')
    record=struct.unpack_from('<i',payload,76)[0]
    segments=[];seen=set()
    while record:
        require(record not in seen and record>0,'Cyclic CK summary records');seen.add(record)
        offset=(record-1)*1024
        following,previous,count=struct.unpack_from('<ddd',payload,offset)
        require(count==int(count)and 0<count<=25,'Invalid CK summary count')
        for i in range(int(count)):
            start,end=struct.unpack_from('<dd',payload,offset+24+40*i)
            instrument,frame,kind,av,first,last=struct.unpack_from('<6i',payload,offset+40+40*i)
            require(0<first<=last and last*8<=len(payload),'Invalid CK packet addresses')
            words=list(struct.unpack_from('<'+'d'*(last-first+1),payload,(first-1)*8))
            require(all(math.isfinite(x)for x in words),'Nonfinite CK segment')
            require(len(words)>=6,'Incomplete CK segment')
            rate,subtype,window,intervals,count=words[-5:]
            n=int(count);m=int(intervals)
            require(count==n and intervals==m and n>0 and m>0,'Nonintegral CK count')
            require(subtype==1 and len(words)==5*n+(n-1)//100+m+(m-1)//100+5,'Wrong CK subtype/layout')
            segments.append(dict(instrument=instrument,frame=frame,type=kind,av=av,first=start,last=end,
                                 rate=rate,window=window,intervals=m,
                                 quaternions=[words[4*i:4*i+4]for i in range(n)],ticks=words[4*n:5*n]))
        require(following==int(following),'Nonintegral CK next record');record=int(following)
    return segments


def matrix(quaternion):
    w,x,y,z=quaternion
    require(abs(sum(q*q for q in quaternion)-1)<2e-12,'CK quaternion is not normalized')
    # Independent scalar-first quaternion polynomial, without CSPICE q2m.
    return [1-2*(y*y+z*z),2*(x*y-w*z),2*(x*z+w*y),
            2*(x*y+w*z),1-2*(x*x+z*z),2*(y*z-w*x),
            2*(x*z-w*y),2*(y*z+w*x),1-2*(x*x+y*y)]


def check(segments,proof,fixture):
    require(proof['status']=='passed'and proof['solver_converged'],'No converged CK witness')
    require(abs(proof['final_control']-2)<fixture['control_tolerance'],'Wrong final control')
    require(proof['trial_rows']>=4 and proof['nonfinal_trial_rows']>=2,'Missing distinguishable trials')
    require(len(segments)==1,'Extra CK trial segments')
    s=segments[0]
    require((s['instrument'],s['frame'],s['type'],s['av'])==(-100057000,1,5,0),'Wrong CK identity')
    require(s['window']==8 and s['intervals']==1,'Changed CK interpolation window')
    require(len(s['ticks'])==len(s['quaternions'])==len(proof['knots'])==fixture['expected_packets']==25,'Wrong final CK census')
    require(s['first']==s['ticks'][0]and s['last']==s['ticks'][-1],'Descriptor endpoints differ')
    maximum=0
    for i,(quaternion,tick,knot)in enumerate(zip(s['quaternions'],s['ticks'],proof['knots'])):
        require(knot['quaternion']==quaternion and knot['tick']==tick,'Raw CK/proof packet mismatch')
        observed=matrix(quaternion)
        require(all(abs(a-b)<2e-15 for a,b in zip(observed,knot['matrix'])),'Independent quaternion reconstruction differs')
        for a,b in zip(observed,fixture['expected_matrix']):
            maximum=max(maximum,abs(a-b));require(abs(a-b)<fixture['matrix_tolerance'],'Trial attitude leaked into CK')
        require(knot['elapsed']==fixture['expected_elapsed_seconds'][i],'Changed final epoch schedule')
        require(abs(knot['et']-knot['expected_et'])<fixture['epoch_tolerance_seconds'],'CK absolute epoch check failed')
        require(abs((tick-s['ticks'][0])/1000-5*i)<fixture['epoch_tolerance_seconds'],'CK relative raw tick schedule differs')
        if i:require(tick>s['ticks'][i-1],'Non-increasing CK epochs')
    return maximum


def audit(repo,stage):
    repo,stage=Path(repo),Path(stage)
    plan=json.loads((stage/'frozen-plan-v2.json').read_text())
    require(sha(repo/plan['base_prefix'])==plan['base_prefix_sha256'],'Parent seal changed')
    fixture=plan['expected_fixture']
    require(fixture==json.loads((repo/'src/qtgui/tests/SolverCkOutputCases.json').read_text()),'Frozen CK fixture changed')
    require(sha(repo/fixture['origin'])==fixture['origin_sha256'],'Original solver fixture changed')
    # Executed sources are bound below to their own immutable archive. Later
    # independent target registrations do not relabel these historical passes.
    drift=[n for n,h in plan['source_sha256'].items()if sha(repo/n)!=h]
    results=[];last=None
    for name,family in [('target-v2','Target'),('optimize-v1','Optimize')]:
        root=stage/name;r=json.loads((root/'receipt.json').read_text())
        require(r['case']==family and r['status']=='passed'and r['returncode']==0 and r['native_processes']==1,'Native CK receipt failed')
        require(r['inputs_unchanged']and r['protected_inputs_unchanged']==1334 and not r['full_regression'],'Changed original inputs/scope')
        require(all(sha(root/n)==h for n,h in r['artifacts'].items()),'Changed CK execution artifact')
        with tarfile.open(root/'sources.tar.gz')as archive:
            sources={m.name:hashlib.sha256(archive.extractfile(m).read()).hexdigest()for m in archive.getmembers()if m.isfile()}
        require(sources==r['source_sha256'],'CK executed source archive differs')
        require(all(sources.get(n)==h for n,h in plan['source_sha256'].items()),'Wrong executed fixture/worker')
        require(any(Path(x['path']).name.startswith('libGmatBase.so')and r['image_sha256'][x['path']]==x['sha256']for x in r['loaded_project_images']),'Native base image not observed')
        proof=json.loads((root/'proof.json').read_text())
        records=re.findall(r'NASA_CHECK (\{[^\n]+\})',(root/'test.log').read_text())
        require(len(records)==1 and json.loads(records[0])==proof,'CK proof/stdout mismatch')
        segments=decode((root/'solution.bc').read_bytes());maximum=check(segments,proof,fixture)
        trials=json.loads((root/'trials.json').read_text())
        require(len(trials)==proof['trial_rows']and len([row for row in trials if abs(row[0]-2)>1e-3])==proof['nonfinal_trial_rows'],'Raw trial census differs')
        require((root/'mission.script').read_text()==next(c['script']for c in fixture['cases']if c['key']==family).replace('@OUTPUT@',str(root)),'Executed CK mission differs')
        results.append(dict(case=family,packets=25,trial_rows=len(trials),nonfinal_trial_rows=proof['nonfinal_trial_rows'],
                            maximum_matrix_error=maximum,maximum_epoch_error_seconds=proof['max_epoch_error_seconds']))
        last=(segments,proof)
    def trial_attitude(s,p):
        quaternion=[math.cos(math.radians(5)),0,0,-math.sin(math.radians(5))]
        s[0]['quaternions'][0]=quaternion
        p['knots'][0].update(quaternion=quaternion,matrix=matrix(quaternion))
    def wrong_epoch(s,p):
        s[0]['ticks'][5]=s[0]['ticks'][4]
        p['knots'][5]['tick']=s[0]['ticks'][5]
    failures=[]
    for label,mutate in [
        ('trial-attitude',trial_attitude),
        ('extra-segment',lambda s,p:s.append(copy.deepcopy(s[0]))),
        ('wrong-epoch',wrong_epoch),
        ('not-converged',lambda s,p:p.update(solver_converged=False))]:
        s,p=copy.deepcopy(last);mutate(s,p)
        try:check(s,p,fixture)
        except ValueError:failures.append(label)
        else:raise ValueError('Accepted corrupt CK '+label)
    original=json.loads((stage/'target-v1/receipt-recovered-v1.json').read_text())
    require(original['status']=='failed'and original['native_returncode']==1,'Original failed attempt was relabeled')
    require(all(sha(stage/'target-v1'/n)==h for n,h in original['artifacts'].items()),'Original failure receipt changed')
    return dict(status='passed-bounded-native-script-CK-final-pass-membership',results=results,
                new_native_passes=2,native_failures_retained=1,new_CK_packets=50,negative_controls=failures,
                native_tests_launched_by_audit=0,original_assertions_changed=0,new_requirement_credit=0,
                terminal_policy='pending-user-answer',later_source_changes=drift,full_current_build_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();result=audit(Path(__file__).resolve().parents[3],args.stage.resolve())
    with args.output.open('x')as f:json.dump(result,f,indent=2);f.write('\n')
    print(json.dumps(result))
