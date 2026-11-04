"""Qualify complete retained propulsion conjunctions plus the missing inertial route.

Older passing numerical GUI proofs keep their own source versions. New receipts
fill only missing modes; no original clause or scientific assertion is removed.
"""
import argparse,hashlib,json,math,tarfile
from pathlib import Path
import AuditAcceptanceRecovery as recovery
import NasaGuiCoverage as nasa
from AuditSolverPublicationAcceptance import read_sources
from BuildNasaCoverageExtension import validate_visible
from RunHarnessAcceptance import passed_junit

KEYS={'FRR!E85','FRR!E90'}
REFERENCE_REVIEW='7b82739c7d3e999a5ad0f67a91b571cdc5f11294c7368b82b450c6d66d392d7d'
ORIGINAL_IMPULSE='3cdfc11d3fdb6e2171a1460aef2ff6c2fef87b11459dcec72be3476802f164a9'
FINAL_IMPULSE='57d18ed16dcd26a7210acb26a9773aeabb3eec427b85e39c498f9af8128fcbca'
MISSING_MODE_IMPULSE='5a6dfd11558e491fab850588001f2761b83b6032e053e942c2ccc315dd8a0515'
THRUSTER='6e37722f10489afbe0bb2bf0d3d8b14fa904c26e197166c078b2f3bf4f7358d5'

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text())
def need(ok,message):
    if not ok:raise ValueError(message)
def close(a,b,tol):return math.isfinite(a) and math.isfinite(b) and abs(a-b)<=tol

def integration(p):
    mode=p['mode'];regulated=mode=='Regulated';off=mode=='MassOff';k2=0 if mode in ('Regulated','BlowDown') else .05
    duties=[.4,0,.8,.8] if mode=='DutySwitch' else [.4]*4;scales=[1.5,0,2.25,2.25] if mode=='ScaleSwitch' else [1.5]*4
    need(p['regulated']==regulated and p['mass_off']==off and p['k2']==k2 and p['duties']==duties and p['scales']==scales,'Original propulsion model partition differs')
    def mass(m,t,d):
        if off or d==0:return m
        if regulated:return m-d*.1*1800/(10*(300+k2*1800))*t
        gas=2-m/1000;v=300*gas+k2*1800;c=d*.1*1800/10000
        return m-1000*(2*c*t/(math.sqrt(v*v+600*c*t)+v))
    def velocity(m,t,d,s):
        if not d or not s:return 0
        if off:return d*s*180/(1000*(937+m))*t
        final=mass(m,t,d);log=math.log1p((m-final)/(937+final))
        return s*.01*((300+k2*1800)*log if regulated else 300*log+k2*1800000/2937*(log+math.log1p((m-final)/(2000-m))))
    direction=[2/7,-3/7,6/7];position=[7000,-1200,900];initial_v=[.25,-.1,.05]
    t=0;m=1000;dv=0;displacement=0;expected=[]
    def row(index,firing,d,s):
        pressure=1800 if regulated else 1800/(2-m/1000);isp=300+k2*pressure if firing else 0;force=d*s*.1*pressure if firing else 0;flow=-d*.1*pressure/(10*isp) if firing and not off else 0
        expected.extend([index,t,m,pressure,20,20,137,937+m]+[position[i]+initial_v[i]*t+direction[i]*displacement for i in range(3)]+[initial_v[i]+direction[i]*dv for i in range(3)]+[d,s,force,isp,flow]+[x*force for x in direction]+[x*force/(1000*(937+m)) for x in direction]+[flow,0,0,0,41])
    def advance(firing,d,s):
        nonlocal t,m,dv,displacement
        extra=0
        if firing:extra=sum((1 if i in (0,256) else 4 if i%2 else 2)*velocity(m,600*i/256,d,s) for i in range(257))*600/(3*256)
        displacement+=dv*600+extra
        if firing:dv+=velocity(m,600,d,s);m=mass(m,600,d)
        t+=600
    row(0,False,.4,1.5);row(1,True,.4,1.5)
    for i in range(3):advance(True,duties[i],scales[i]);row(i+2,True,duties[i],scales[i])
    row(5,False,duties[2],scales[2]);advance(False,duties[2],scales[2]);row(6,False,duties[2],scales[2]);advance(True,duties[3],scales[3]);row(7,True,duties[3],scales[3]);row(8,False,duties[3],scales[3]);advance(False,duties[3],scales[3]);row(9,False,duties[3],scales[3])
    need(len(p['actual'])==len(p['expected'])==len(expected)==300,'Missing complete integrated data')
    for i,(actual,reported,want) in enumerate(zip(p['actual'],p['expected'],expected)):
        c=i%30;tol=1e-5 if c==1 else 2e-6 if c in (2,7,17) else 5e-6 if c in (3,8,9,10) else 1e-7 if c in (16,19,20,21) else 5e-12 if c in (22,23,24) else 1e-9
        need(close(actual,want,tol) and close(reported,want,1e-9),'Independent pressure/rocket/mass/trajectory recalculation failed')

def impulse(p):
    states=[[7000,-1200,900,1.5,6.75,-2],[5100,4000,-1800,-4,5,1.25],[-3300,6100,2600,-6,-2,1]];kicks=[[.01,.02,-.03],[-.02,.01,.04],[0,.03,.02]];attitude=[[0,1,0],[-1,0,0],[0,0,1]];probe=p['probe_relative_earth_state'];mode=p['mode']
    need(mode in ('ProbeVNB','ProbeLVLH','ProbeMJ2000Eq') and p['states']==states and p['impulses']==kicks and p['attitude_inertial_to_body']==attitude and p['origin']=='Probe' and p['frame']=='Local','Original impulsive inputs/context changed')
    need(len(probe)==6 and all(math.isfinite(x) for x in probe) and sum(x*x for x in probe[:3])>1e10,'Explicit custom-body origin input absent')
    def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
    def unit(v):return [x/math.sqrt(sum(y*y for y in v)) for x in v]
    expected=[]
    for state,kick in zip(states,kicks):
        if mode=='ProbeMJ2000Eq':basis=[[1,0,0],[0,1,0],[0,0,1]]
        else:
            r=[state[i]-probe[i] for i in range(3)];v=[state[i+3]-probe[i+3] for i in range(3)];normal=unit(cross(r,v));radial=unit(r);velocity=unit(v);basis=[velocity,normal,cross(velocity,normal)] if mode.endswith('VNB') else [radial,cross(normal,radial),normal]
        dv=[sum(basis[j][i]*kick[j] for j in range(3)) for i in range(3)];remaining=1000+1937*math.expm1(-math.sqrt(sum(x*x for x in kick))*1000/4000)
        for phase in range(3):
            fuel=remaining if phase==1 else 1000;observed=state[:]
            if phase==1:observed[3:]=[x+y for x,y in zip(observed[3:],dv)]
            expected.extend([states.index(state),phase,0,fuel,137,937+fuel]+observed+[x-y for x,y in zip(observed,probe)]+sum(attitude,[])+[41])
    need(len(p['actual'])==len(p['expected'])==len(expected)==252,'Missing complete impulsive data')
    for i,(actual,reported,want) in enumerate(zip(p['actual'],p['expected'],expected)):
        c=i%28;tol=2e-6 if 12<=c<=17 else 1e-8 if c in (3,5) else 1e-9
        need(close(actual,want,tol) and close(reported,want,1e-9),'Independent custom-body impulse/rocket/reversal recalculation failed')
    need(p['axes']==('MJ2000Eq' if mode=='ProbeMJ2000Eq' else mode.removeprefix('Probe')) and (p['origin_delta_contrast']==0 if mode=='ProbeMJ2000Eq' else p['origin_delta_contrast']>1e-4),'Custom-body inertial/orbital distinction missing')

def qualify(repo,path,inventory):
    q=load(path);need(q['contract_kind']=='source-versioned-hardware-conjunction' and len(q['qualified_keys'])==2 and set(q['qualified_keys'])==KEYS,'Partial/extra/duplicate propulsion credit')
    need(all(q.get(k) is False for k in ('full_current_build_qualified','full_NASA_qualified','technical_core_approval')),'Propulsion scope exceeds qualification')
    for p,d in q['artifact_sha256'].items():need(sha(p)==d,'Changed propulsion artifact: '+p)
    need(q['authority_sha256']==inventory['sources'],'Propulsion NASA authority differs')
    old=load(q['original_audit']);checked=recovery.audit(repo,load(q['original_plan']));need(old['cohorts']==checked['cohorts'] and old['pending_count']==2,'Original both-mode legacy conjunction changed')
    for co in old['cohorts']:need({r['key'] for r in co['rows']}==KEYS and all(len(r['checks'])==1 and all(c.get('receipt') and c.get('receipt_sha256') for c in r['checks']) for r in co['rows']),'Original legacy clause omitted')
    parent=load(q['parent_review']);strong=parent['stronger_unqualified_obligations'];need({r['key'] for r in strong}==KEYS,'Stronger original scope omitted')
    for p,d in parent['input_sha256'].items():need(recovery.digest(recovery.payload(p))==d,'Changed original parent review input')
    expected={};refs={}
    for row in strong:
        need(row['full_original_claims']==(7 if row['key']=='FRR!E85' else 3),'Stronger original clause count changed')
        need(len(row['native_visible_claims_preserved'])==(6 if row['key']=='FRR!E85' else 2),'Native original clause omitted')
        for record in row['native_visible_claims_preserved']:
            claim=record['claim'];expected[claim['test']]=claim;refs.update(claim['reference_sources'])
            need(record['matches'],'Original visible passing evidence absent')
            for prior in record['matches']:
                receipt=recovery.checked_json(prior['receipt'],prior['receipt_sha256']);manifest=load(receipt['snapshot_manifest']);result=next(r for r in receipt['results'] if r['name']==claim['test'] and r['status']=='passed');proof=recovery.verify_claim(Path(manifest['root']),manifest,receipt,result,claim)
                need(receipt['mode']=='visible' and proof['native_exposure_events']>0,'Original native visible context lost')
    need(len(expected)==8 and sha(q['original_impulse_source'])==ORIGINAL_IMPULSE and q['missing_mode_impulse_sha256']==MISSING_MODE_IMPULSE,'Original propulsion assertions not preserved')
    need(sha(q['reference_review'])==REFERENCE_REVIEW,'Unreviewed propulsion documentation evolution')
    reviewed={(r['path'],r['old_sha256'],r['new_sha256']) for r in load(q['reference_review'])['reviews']}
    references={}
    with tarfile.open(q['reference_archive']) as b:
        need({m.name for m in b}==set(q['reference_sha256']),'Propulsion reference source partition differs')
        for name,d in q['reference_sha256'].items():
            data=b.extractfile(name).read();need(hashlib.sha256(data).hexdigest()==d and not data.startswith(b'\x7fELF'),'Changed propulsion source reference');references[name]=data
    seen={};new_name='QtGui.NasaImpulsiveFrameProbeMJ2000Eq'
    for spec in q['observations']:
        receipt,sources=read_sources(repo,spec,inventory);need(receipt['mode']==spec['mode'],'Wrong propulsion execution mode')
        if spec['mode']=='visible':validate_visible(receipt,spec['cases'])
        else:need(spec['mode']=='headless','Unsupported propulsion mode')
        before=load(spec['inputs_before']);results={r['name']:r for r in receipt['results'] if r['status']!='not-run'}
        need(set(results)==set(spec['cases']),'Extra/missing propulsion execution')
        for name,result in results.items():
            case=spec['cases'][name]
            need(result['log']==case['log'] and str(Path(result['log']).parent)==case['directory'],'Wrong propulsion artifact directory')
            for field in ('log','junit'):need(sha(case[field])==case[field+'_sha256'],'Changed actual propulsion '+field)
            if result['status']!='passed':
                need(spec['id']=='sixth-unattended2' and name==new_name and result['status']=='failed' and result['returncode']!=0,'Undisposed failed propulsion case')
                continue
            need(result['returncode']==0 and passed_junit(Path(case['junit']),name,0),'Actual propulsion JUnit did not pass')
            if name==new_name:
                source='src/qtgui/tests/NasaImpulsiveFrameTests.cpp';need(hashlib.sha256(sources[source]).hexdigest()==FINAL_IMPULSE,'Unknown inertial assertion version')
                code=sources[source].decode();need('configuredKick && configuredKick->GetStringParameter("CoordinateSystem")=="Local"' in code and 'configuredKick->GetStringParameter("Origin")=="Probe"' in code,'Actual configured custom-body identity check missing')
                records=[p for p in nasa.proof_records(result) if p.get('checkpoint')=='impulsive-frame-ProbeMJ2000Eq'];need(len(records)==1 and records[0]['outcome']=='passed','Missing inertial checkpoint');impulse(records[0])
            else:
                need(spec['mode']=='headless' and name in expected,'Unexpected legacy propulsion replay')
                claim=expected[name];source=claim['source'];digest=hashlib.sha256(sources[source]).hexdigest()
                need(digest==(THRUSTER if 'Thruster' in name else q['missing_mode_impulse_sha256']),'Unreviewed original assertion source version')
                need(claim['anchor'] in sources[source].decode(),'Original propulsion assertion anchor missing')
                records=[p for p in nasa.proof_records(result) if p.get('checkpoint')==claim['checkpoint'] and set(claim['facets'])<=set(p.get('facets',[])) and all(p.get(k)==v for k,v in claim['expected'].items())];need(len(records)==1 and records[0]['outcome']=='passed','Original propulsion expected/facet conjunction failed')
                (integration if 'Thruster' in name else impulse)(records[0])
            key=(spec['mode'],name);need(key not in seen,'Duplicate propulsion witness');seen[key]=1
        for ref,expected_digest in refs.items():
            if ref in q['unchanged_normative_images']:
                frozen=q['unchanged_normative_images'][ref];need(sha(frozen)==expected_digest,'Original normative illustration changed');continue
            need(ref in references and before['sources'].get(ref)==q['reference_sha256'][ref],'Original reference omitted from actual pre-execution capture')
            actual=q['reference_sha256'][ref]
            production=ref.startswith('src/') and '/tests/' not in ref and Path(ref).suffix in ('.cpp','.hpp','.h')
            need(actual==expected_digest or production or (ref,expected_digest,actual) in reviewed,'Unreviewed original reference evolution')
    need(set(seen)=={('headless',name) for name in expected}|{('headless',new_name),('visible',new_name)},'Complete original/native propulsion conjunction missing')
    known={r['key']:r for r in inventory['requirements']};review=load(q['review']);need(review['sources']==inventory['sources'] and len(review['reviews'])==2 and {r['key'] for r in review['reviews']}==KEYS,'Exact NASA row review missing')
    for row in review['reviews']:need(row['fingerprint']==known[row['key']]['fingerprint'] and row['source_text']==known[row['key']]['text'] and row['rationale'],'Changed NASA propulsion row scope')
    return q,sorted(KEYS)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);parser.add_argument('--qualification',type=Path,required=True);args=parser.parse_args();_,keys=qualify(args.repo.resolve(),args.qualification,nasa.inventory(args.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,new_test_executions=0)))
