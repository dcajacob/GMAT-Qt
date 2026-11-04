"""Independent arithmetic for the bounded STK/OEM file/query clock matrix."""
from collections import Counter
from decimal import Decimal, localcontext
import hashlib
import json
from pathlib import Path

D=Decimal
def check(ok,message):
    if not ok:raise ValueError(message)
def seconds(stamp):
    return D(str(stamp['day']))*86400+D(str(stamp['sec']))+D(str(stamp['frac']))
def polynomial(degree,t):
    out=[];vel=[]
    for i,(r,v) in enumerate(zip(['9000','100','300'],['-.1','7.1','.4'])):
        a=D(i+1)*D('1e-5') if degree==3 else D(0)
        b=D(i-1)*D('1e-9') if degree==3 else D(0)
        out.append(D(r)+D(v)*t+a*t*t+b*t*t*t)
        vel.append(D(v)+2*a*t+3*b*t*t)
    return out+vel
def compare(actual,expected,counts):
    check(len(actual)==6,'State width changed')
    for i,(a,e) in enumerate(zip(actual,expected)):
        check(abs(D(str(a))-e)<=D('1e-7' if i<3 else '1e-10'),'Physical component exceeds original bound')
        counts['state_component_checks']+=1
def audit(proof,root=None):
    with localcontext() as context:
        context.prec=60
        check(proof['recording_errors']==0 and proof['new_requirement_credit']==0 and proof['whole_row_qualified'] is False,'Recording errors or unsupported credit')
        files={f['key']:f for f in proof['files']};check(len(files)==30,'Incomplete file matrix');counts=Counter()
        expected_keys={(fmt,method,fraction,degree,count) for fmt in ['STK','OEM'] for method in ['Lagrange','Hermite'] for fraction in ['000000000','123456789'] for degree in [1,3] for count in [2,9,17] if (degree,count)!=(3,2) and (fmt,method)!=('OEM','Hermite')}
        check({(f['format'],f['method'],f['fraction'],f['degree'],f['count'])for f in files.values()}==expected_keys,'File matrix identities changed')
        for f in files.values():
            origin=D(21545)*86400+D('32.0343817')+D('0.'+f['fraction'])
            check(abs(seconds(f['origin'])-origin)<=D('1e-11'),'Independent UTC/A1 origin differs')
            check(len(f['nodes'])==f['count'],'Node count differs')
            if root:
                text=(Path(root)/(f['key']+'-input.txt')).read_bytes()
                check(hashlib.sha256(text).hexdigest()==f['sha256'],'Generated file identity differs')
            for i,node in enumerate(f['nodes']):
                check(node['precise'] and abs(seconds(node['epoch'])-origin-30*i)<=D('1e-11'),'File timestamp lost precision')
                compare(node['state'],polynomial(f['degree'],D(30*i)),counts)
            counts['files']+=1
        check(len(proof['observations'])==1860,'Query matrix incomplete')
        identities=Counter()
        for observation in proof['observations']:
            f=files[observation['file']];origin=D(21545)*86400+D('32.0343817')+D('0.'+f['fraction']);end=D(30*(f['count']-1))
            t=seconds(observation['query'])-origin
            if observation['api']=='Real' and D(str(observation['requested_seconds'])) in (0,end):t=D(str(observation['requested_seconds']))
            outside=t<0 or t>end
            # Raw clock components have sub-picosecond decimal serialization
            # noise. Exact-input GT endpoint identity is verified above.
            if observation['api']=='GmatTime' and not observation.get('outside'):
                requested=D(str(observation['requested_seconds']))
                check(abs(t-requested)<=D('1e-11'),'Precise query clock differs')
                t=requested;outside=False
            if outside:
                check(observation['outcome']=='exception' and 'outside' in observation['message'],'Outside-span query accepted or wrong failure')
                counts['outside_rejections']+=1
            else:
                check(observation['outcome']=='state','Valid interpolation failed: '+observation['file'])
                compare(observation['state'],polynomial(f['degree'],t),counts);counts['valid_states']+=1
            if not observation.get('outside'):
                identities[(observation['file'],observation['direction'],observation['api'])]+=1
            counts['queries']+=1
        check(set(identities.values())=={10} and len(identities)==180,'API/direction partition incomplete')
        check(len(proof['controls'])==6,'Copy/duplicate/leap controls missing')
        check(proof['controls'][0]['unsupported_oem_hermite_rejected'],'Unsupported OEM interpolation method admitted')
        for control in proof['controls'][1:4]:
            check(control['same_rounded_mjd'] and control['count']==2 and control['duplicate_rejected'] and control['data']==[2,3,4,5,6,7],'Precise duplicate/copy contract differs')
            target=D(21545)*86400+D('32.0343817')+D('.123456799')
            check(abs(seconds(control['epoch'])-target)<=D('1e-11'),'Copied precise epoch changed');counts['copied_epoch_controls']+=1
        leap=proof['controls'][4]
        check(abs(seconds(leap['leap_inserted'])-seconds(leap['leap_before'])-1)<=D('1e-11') and abs(seconds(leap['leap_after'])-seconds(leap['leap_inserted'])-1)<=D('1e-11'),'UTC leap-second spacing differs')
        control=proof['controls'][5]
        check(control['gap_rejected'] and control['public_epoch_mutation_uses_real'],'Gap or legacy public-epoch fallback changed')
        compare(control['single_state'],polynomial(3,D(0)),counts)
        check(len(proof['base_controls'])==12,'Legacy/precise method-switch controls absent')
        for item in proof['base_controls']:
            a,b=item['nodes'];dt=seconds(b['epoch'])-seconds(a['epoch']);t=(seconds(item['query'])-seconds(a['epoch']))/dt
            pa,pb=map(lambda n:[D(str(v))for v in n['state']],[a,b])
            if item['hermite']:
                # Independent two-node cubic Hermite basis and its derivative.
                expected=[(2*t**3-3*t*t+1)*pa[i]+(t**3-2*t*t+t)*dt*pa[i+3]+(-2*t**3+3*t*t)*pb[i]+(t**3-t*t)*dt*pb[i+3]for i in range(3)]
                expected += [((6*t*t-6*t)*pa[i]+(3*t*t-4*t+1)*dt*pa[i+3]+(-6*t*t+6*t)*pb[i]+(3*t*t-2*t)*dt*pb[i+3])/dt for i in range(3)]
            else:expected=[(1-t)*pa[i]+t*pb[i]for i in range(6)]
            compare(item['state'],expected,counts);counts['base_legacy_precise_segment_controls']+=1
        check(len(proof['subulp_controls'])==2 and {c['format']for c in proof['subulp_controls']}=={'STK','OEM'},'Sub-MJD-ulp file controls missing')
        for item in proof['subulp_controls']:
            origin=D(21545)*86400+D('32.0343817')+D('.123456789')
            check(item['same_rounded_mjd'] and len(item['nodes'])==3,'Close input clocks collapsed or duplicate retained')
            if root:
                raw=(Path(root)/(item['format']+'-subulp-input.txt')).read_bytes()
                check(hashlib.sha256(raw).hexdigest()==item['sha256'],'Subulp generated file changed')
            for i,node in enumerate(item['nodes']):
                check(abs(seconds(node['epoch'])-origin-D(i)*D('1e-8'))<=D('1e-12'),'Distinct encoded clock differs')
                compare(node['state'],polynomial(1,D(i)*D('1e-8')),counts)
            query=seconds(item['query'])-origin
            check(abs(query-D('1.5e-8'))<=D('1e-12'),'Close query clock differs')
            compare(item['state'],polynomial(1,query),counts);counts['subulp_file_controls']+=1
        return dict(status='passed',counts=dict(counts),new_requirement_credit=0,whole_row_qualified=False,scientific_approval=False)

if __name__=='__main__':
    import sys
    path=Path(sys.argv[1]);print(json.dumps(audit(json.loads(path.read_text()),path.parent),indent=2))
