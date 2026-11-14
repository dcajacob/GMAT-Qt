"""Independently check every native estimator CK packet and accepted epoch."""
from pathlib import Path
import argparse
import copy
import hashlib
import json
import math
import re
import tarfile
from AuditSolverCkEvidence import decode,matrix,require,sha


def cross(a,b):
    return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]


def unit(a):
    length=math.sqrt(sum(x*x for x in a));require(length>0,'Singular nadir reference')
    return [x/length for x in a]


def nadir(state):
    x=[-v for v in unit(state[:3])];y=unit(cross(state[:3],state[3:]));z=cross(x,y)
    return x+y+z


def circle(spec,elapsed):
    radius=spec['radius'];rate=math.sqrt(spec['mu']/radius**3);speed=radius*rate
    c,s=math.cos(rate*elapsed),math.sin(rate*elapsed)
    ci,si=math.cos(math.radians(spec['inclination_degrees'])),math.sin(math.radians(spec['inclination_degrees']))
    return [radius*c,radius*s*ci,radius*s*si,-speed*s,speed*c*ci,speed*c*si]


def check(segments,observations,raw,spec):
    family=spec['family'];require(observations['family']==family,'Wrong estimator family')
    require(observations['status']=='observed-native-completion;independent-audit-required','Native observation failed')
    require(observations['phase_boundaries']==spec['boundaries'],'Wrong native phase order')
    require(observations['raw_count']==len(raw)and raw,'Missing raw estimator stream')
    eligible=[r for r in raw if r['phase']==spec['selected_phase']and
              (r['final_pass']if family=='Batch'else r['run_state']==10007)]
    require(eligible,'No selected final estimator publications')
    expected=spec['expected_unique_elapsed_seconds'];step=expected[1]-expected[0]
    by_epoch={t:[]for t in expected};raw_geometry_error=0
    for row in raw:
        require(len(row['state'])==6 and len(row['quaternion'])==4,'Wrong native state/quaternion shape')
        require(all(math.isfinite(x)for x in row['state']+row['quaternion']),'Nonfinite native stream')
        q=row['quaternion'];observed=matrix([q[3],-q[0],-q[1],-q[2]])
        for a,b in zip(observed,nadir(row['state'])):
            raw_geometry_error=max(raw_geometry_error,abs(a-b))
            require(abs(a-b)<spec['matrix_tolerance'],'Native quaternion differs from independent nadir geometry')
    maximum_position=maximum_velocity=0
    for row in eligible:
        elapsed=(row['epoch']-spec['a1_epoch'])*86400
        t=round(elapsed/step)*step
        require(t in by_epoch and abs(elapsed-t)<spec['epoch_tolerance_seconds'],'Accepted epoch outside independent grid')
        expected_state=circle(spec,t)
        for i,(actual,wanted)in enumerate(zip(row['state'],expected_state)):
            error=abs(actual-wanted)
            if i<3:maximum_position=max(maximum_position,error)
            else:maximum_velocity=max(maximum_velocity,error)
            require(error<spec['position_tolerance'if i<3 else'velocity_tolerance'],'Accepted native orbit differs from independent circle')
        by_epoch[t].append(row)
    require(all(by_epoch.values()),'Native final stream omitted a scheduled epoch')
    require(len(segments)==len(observations['segments'])and segments,'Missing CK segments')
    covered=set();packets=0;max_geometry=max_membership=max_epoch=0
    for segment,record in zip(segments,observations['segments']):
        require((segment['instrument'],segment['frame'],segment['type'],segment['av'])==(-100057000,1,5,0),'Wrong CK identity/frame/type')
        require(segment['window']==8 and segment['intervals']==1,'Wrong CK interpolator')
        require(0<len(segment['ticks'])<=1000 and len(segment['ticks'])==len(record['packets']),'Wrong CK segment packet census')
        require(segment['first']==segment['ticks'][0]and segment['last']==segment['ticks'][-1],'Wrong CK endpoint descriptors')
        for i,(tick,quaternion,packet)in enumerate(zip(segment['ticks'],segment['quaternions'],record['packets'])):
            require(tick==packet['tick']and quaternion==packet['quaternion'],'Raw CK packet differs from observed binary parser')
            require(all(math.isfinite(packet[k])for k in ['et','elapsed','tick']),'Nonfinite CK epoch')
            if i:require(tick>segment['ticks'][i-1],'CK segment epochs are not increasing')
            t=round(packet['elapsed']/step)*step
            require(t in by_epoch,'CK packet outside independent final span')
            error=abs(packet['elapsed']-t);max_epoch=max(max_epoch,error)
            require(error<spec['epoch_tolerance_seconds'],'CK packet epoch differs from independent final schedule')
            observed=matrix(quaternion);physical=nadir(circle(spec,t))
            for a,b in zip(observed,physical):
                error=abs(a-b);max_geometry=max(max_geometry,error)
                require(error<spec['matrix_tolerance'],'CK attitude differs from independent final circle/nadir geometry')
            membership=min(max(abs(a-b)for a,b in zip(observed,matrix([r['quaternion'][3]]+[-q for q in r['quaternion'][:3]])))for r in by_epoch[t])
            max_membership=max(max_membership,membership)
            require(membership<2e-12,'CK contains a quaternion outside the selected final publication set')
            covered.add(t);packets+=1
    require(covered==set(expected),'CK omitted a scheduled final epoch')
    # Repeated accepted discontinuity endpoints are legitimate native segments.
    # This requires every packet's physical/final membership, not a guessed
    # one-segment prediction from the first supplementary plan.
    return dict(family=family,segments=len(segments),packets=packets,unique_epochs=len(covered),
                accepted_publications=len(eligible),native_publications=len(raw),
                native_nadir_geometry_error=raw_geometry_error,CK_nadir_geometry_error=max_geometry,
                CK_final_membership_error=max_membership,CK_epoch_error_seconds=max_epoch,
                maximum_native_position_error_km=maximum_position,maximum_native_velocity_error_km_s=maximum_velocity,
                initial_segment_prediction_matched=[len(s['ticks'])for s in segments]==spec['expected_segment_counts'])


def audit_case(root,fixture):
    root=Path(root);spec=json.loads(Path(fixture).read_text())
    observations=json.loads((root/'observations.json').read_text());raw=json.loads((root/'raw-publications.json').read_text())
    require((root/'observer-error.txt').read_text()=='','Native observer failed')
    require('CK cleanup failed'not in(root/'messages.txt').read_text(),'Native CK cleanup still failed')
    segments=decode((root/'solution.bc').read_bytes())
    return check(segments,observations,raw,spec)


def audit(repo,stage):
    repo,stage=Path(repo),Path(stage)
    plan=json.loads((stage/'estimator-ck-frozen-plan-v2.json').read_text());results=[];last=None
    for family in ['Batch','Filter','Smoother']:
        root=stage/(family.lower()+'-ck-v2');receipt=json.loads((root/'receipt.json').read_text())
        require(receipt['status']=='observed-native-completion'and receipt['returncode']==0,'Native CK collection did not complete')
        require(receipt['protected_inputs_unchanged']==1334 and receipt['inputs_unchanged'],'Original inputs changed')
        require(all(sha(root/n)==h for n,h in receipt['artifacts'].items()),'Changed native CK artifact')
        with tarfile.open(root/'sources.tar.gz')as archive:
            sources={m.name:hashlib.sha256(archive.extractfile(m).read()).hexdigest()for m in archive.getmembers()if m.isfile()}
        require(sources==receipt['source_sha256'],'Wrong estimator CK executed archive')
        fixture=repo/f'src/qtgui/tests/estimator-ck/{family}.json';spec=json.loads(fixture.read_text())
        require(sources[str(fixture.relative_to(repo))]==plan['source_sha256'][str(fixture.relative_to(repo))]==sha(fixture),'CK fixture changed')
        require(sources['src/qtgui/tests/EstimatorCkObservationTests.cpp']==plan['source_sha256']['src/qtgui/tests/EstimatorCkObservationTests.cpp'],'Wrong native CK observer version')
        require(all(sources[n]==h for n,h in plan['implementation_sha256'].items()),'Wrong CK production repair')
        require(sha(repo/spec['origin'])==spec['origin_sha256'],'Original estimator fixture changed')
        require((root/'mission.script').read_text()==spec['script'].replace('@OUTPUT@',str(root)),'Executed estimator CK mission changed')
        records=re.findall(r'NASA_OBSERVATION (\{[^\n]+\})',(root/'test.log').read_text())
        require(len(records)==1 and json.loads(records[0])==json.loads((root/'observations.json').read_text()),'CK observations/stdout mismatch')
        result=audit_case(root,fixture);results.append(result)
        if family=='Batch':last=(decode((root/'solution.bc').read_bytes()),json.loads((root/'observations.json').read_text()),json.loads((root/'raw-publications.json').read_text()),spec)
    negatives=[]
    for label in ['trial-attitude','missing-epoch','unselected-phase','wrong-epoch']:
        segments,observations,raw,spec=copy.deepcopy(last)
        if label=='trial-attitude':
            trial=next(r for r in raw if not r['final_pass']and abs((r['epoch']-spec['a1_epoch'])*86400)<spec['epoch_tolerance_seconds'])
            q=trial['quaternion'];quaternion=[q[3],-q[0],-q[1],-q[2]]
            segments[0]['quaternions'][0]=quaternion;observations['segments'][0]['packets'][0]['quaternion']=quaternion
        elif label=='missing-epoch':
            segments[0]['ticks'].pop(5);segments[0]['quaternions'].pop(5);observations['segments'][0]['packets'].pop(5)
        elif label=='unselected-phase':
            for r in raw:r['phase']='trial'
        else:observations['segments'][0]['packets'][5]['elapsed']+=2
        try:check(segments,observations,raw,spec)
        except ValueError:negatives.append(label)
        else:raise ValueError('Accepted corrupt estimator CK '+label)
    failures=[]
    for family in ['Filter','Smoother']:
        root=stage/(family.lower()+'-ck-v1');r=json.loads((root/'receipt.json').read_text())
        require(r['status']=='failed'and r['returncode']==1,'Original CK failure was relabeled')
        require('SCLK_DATA_TYPE_100057 was not found'in(root/'test.log').read_text()and'CK cleanup failed'in(root/'messages.txt').read_text(),'Original CK failure/cleanup diagnostic lost')
        require(all(sha(root/n)==h for n,h in r['artifacts'].items()),'Original CK failure artifact changed')
        failures.append(family)
    return dict(status='passed-bounded-estimator-CK-final-membership',results=results,negative_controls=negatives,
                native_passes=3,original_native_failures_retained=failures,native_tests_launched_by_audit=0,
                numerical_bounds_unchanged=True,original_assertions_changed=0,new_requirement_credit=0,
                segment_prediction_revision='Original one-segment supplementary prediction preserved; actual native accepted discontinuities verified packet-by-packet.',
                terminal_policy='pending-user-answer',full_current_build_qualified=False)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--stage',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();result=audit(Path(__file__).resolve().parents[3],args.stage.resolve())
    with args.output.open('x')as f:json.dump(result,f,indent=2);f.write('\n')
    print(json.dumps(result))
