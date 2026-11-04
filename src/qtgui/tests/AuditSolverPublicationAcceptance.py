"""Complete held solver-drawing scopes with live, source-versioned GUI evidence.

Original legacy clauses/cohorts remain mandatory. This adds raw-publication and
numerical-solver-state observations, plus the missing exact-source unattended
mixed case. Post-execution review does not rewrite any execution receipt.
"""
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile

import AuditAcceptanceRecovery as recovery
import NasaGuiCoverage as nasa
from BuildNasaCoverageExtension import validate_visible
from RunHarnessAcceptance import passed_junit

LEAVES={f'FRR!E{i}' for i in (334,335,336,349,350,351,638,639,640)}
PARENTS={'FRR!E333','FRR!E348','FRR!E637'}
KINDS=('Orb','Map','XY')
MODES=('All','Current','None')

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def load(p):
    return json.loads(Path(p).read_text())

def need(ok,message):
    if not ok:raise ValueError(message)

def read_sources(repo,spec,inventory):
    before,after=load(spec['inputs_before']),load(spec['inputs_after'])
    need(before==after,'Live GUI source/runtime inputs changed')
    reuse=load(spec['reuse']);receipt=load(reuse['receipt'])
    need(sha(reuse['receipt'])==reuse['receipt_sha256'],'Live receipt identity differs')
    need(sha(reuse['reference'])==reuse['reference_sha256'] and sha(reuse['delta'])==reuse['delta_sha256'],'Changed preserved live source archive')
    sources={};all_hashes={}
    for field,path in [('reused_source_sha256',reuse['reference']),('changed_source_sha256',reuse['delta'])]:
        with tarfile.open(path) as bundle:
            for name,digest in reuse[field].items():
                need(not Path(name).is_absolute() and '..' not in Path(name).parts,'Unsafe live source member')
                data=bundle.extractfile(name).read()
                need(hashlib.sha256(data).hexdigest()==digest and not data.startswith(b'\x7fELF'),'Changed/non-source live source member')
                need(name not in all_hashes,'Duplicate live source partition')
                all_hashes[name]=digest;sources[name]=data
    other=reuse['recorded_authority_or_ELF_inputs_not_copied']
    need(not set(other)&set(all_hashes) and {**all_hashes,**other}==receipt['source_hashes'],'Incomplete live execution source preservation')
    need({p:other[p] for p in inventory['sources']}==inventory['sources'],'Live NASA authority differs')
    for name,digest in other.items():
        if name not in inventory['sources']:
            need(Path(name).name=='GmatQtAcceptanceProbe' and before['binaries'].get(str(repo/name))==digest,'Unknown or unbound noncopied live source input')
    for name,digest in all_hashes.items():
        if name in before['sources']:need(before['sources'][name]==digest,'Independent live source snapshot disagrees')
    for name,result in {r['name']:r for r in receipt['results'] if r['status']!='not-run'}.items():
        need(name in spec['cases'],'Extra live execution not declared')
        executables=[p for p in result['registered_command'] if p in receipt['binary_hashes']]
        need(len(executables)==1 and before['binaries'].get(executables[0])==receipt['binary_hashes'][executables[0]],'Actual live executable not bound')
    return receipt,sources

def integrate(state,seconds):
    def rhs(s):
        r=math.sqrt(sum(x*x for x in s[:3]));return s[3:]+[-398600.4415*x/r**3 for x in s[:3]]
    s=list(state);n=max(1,math.ceil(abs(seconds)/.1));h=seconds/n
    for _ in range(n):
        a=rhs(s);b=rhs([x+h*y/2 for x,y in zip(s,a)]);c=rhs([x+h*y/2 for x,y in zip(s,b)]);d=rhs([x+h*y for x,y in zip(s,c)])
        s=[x+h*(aa+2*bb+2*cc+dd)/6 for x,aa,bb,cc,dd in zip(s,a,b,c,d)]
    return s

def check_live(spec,solver,result,sources):
    case=Path(spec['directory']);proof=load(case/'proof.json');raw=load(case/'raw-publications.json');observations=load(case/'live-membership-diagnostics.json')
    need(proof['solver']==solver and proof['checkpoint']=='native-live-solver-membership' and proof['outcome']=='passed' and proof['plots']==9,'Incomplete live solver checkpoint')
    need([r for r in nasa.proof_records(result) if r.get('checkpoint')=='native-live-solver-membership']==[proof],'Live checkpoint differs from actual CTest log')
    code=sources['src/qtgui/tests/NasaSolverPublicationTests.cpp'].decode()
    for token in ('accepted=accepted&&state==Solver::FINISHED','p.solver==!s.accepted','points.size()!=size_t(expected.size())',
                  's.pass==pass&&!s.accepted','QDialogButtonBox::Cancel','Iteration draft escaped Apply','Iteration exact Undo failed',
                  'Publication Unicode reopen failed','Live iteration content/order differs from independent raw publication',
                  'Quadratic optimum differs','Nested result differs from independent scalar goal','Iteration Run changed source/history/file'):
        need(token in code,'Missing original live solver assertion: '+token)
    need('GetPlotRunState' not in code,'Plot classification substituted for numerical solver oracle')
    need(len(raw)==proof['raw_count'] and len(raw)==3*len(proof['passes'])+6,'Incomplete unfiltered publication grid')
    passes={p['pass']:p for p in proof['passes']};need(set(passes)==set(range(1,len(passes)+1)),'Missing/duplicate solver pass')
    accepted_ids=set();nested_count=0;states_seen=set()
    for index,p in passes.items():
        states=p['states'];need(len(states)==(2 if solver in ('Nested','Mixed') else 1),'Numerical solver clone context omitted')
        expected_names={'DC'} if solver=='DC' else {'Opt'} if solver=='Yukon' else {'DC','Inner'} if solver=='Nested' else {'Opt','Inner'}
        need({s['solver'] for s in states}==expected_names,'Wrong numerical solver family')
        accepted=all(s['state']==10014 for s in states);states_seen|={s['state'] for s in states}
        need(p['accepted']==accepted,'Accepted pass not derived from all numerical solver clones')
        if accepted:accepted_ids.add(index)
        if any(s['solver']=='Inner' and s['state']==10014 for s in states) and not accepted:nested_count+=1
    need(len(accepted_ids)==proof['accepted_passes']==1 and proof['trial_passes']==len(passes)-1>=2 and {10002,10003,10014}<=states_seen,'Nominal/perturbation/accepted partitions incomplete')
    need(nested_count==proof['nested_inner_accepted_outer_trial'] and (nested_count>0 if solver in ('Nested','Mixed') else nested_count==0),'Nested local accepted/global trial partition omitted')
    for sample in raw:
        need(len(sample['state'])==6 and all(math.isfinite(x) for x in sample['state']) and math.isfinite(sample['epoch']) and math.isfinite(sample['longitude']) and math.isfinite(sample['latitude']),'Nonfinite or incomplete raw publication')
        need(sample['pass'] in {0,*passes} and sample['accepted']==(sample['pass']==0 or sample['pass'] in accepted_ids),'Raw trial membership differs from numerical solver states')
    for index in passes:
        samples=[s for s in raw if s['pass']==index];need(len(samples)==3,'Incomplete per-pass fixed-step stream')
        for j,s in enumerate(samples):
            seconds=(s['epoch']-samples[0]['epoch'])*86400
            need(abs(seconds-5*j)<.0001,'Literal per-pass publication cadence differs')
            want=integrate(samples[0]['state'],5*j)
            need(all(abs(x-y)<(3e-5 if i<3 else 4e-8) for i,(x,y) in enumerate(zip(s['state'],want))),'Raw iteration state differs from independent Newton integration')
    accepted=[s for s in raw if s['pass'] in accepted_ids]
    want=[7200,320 if solver in ('Nested','Mixed') else 20,0,.2,7.2,0]
    need(all(abs(x-y)<2e-5 for x,y in zip(accepted[0]['state'],want)),'Independently known solved controls do not reach published spacecraft state')
    final=integrate(want,20)
    need(all(abs(x-y)<(3e-5 if i<3 else 4e-8) for i,(x,y) in enumerate(zip(raw[-1]['state'],final))),'Post-solver continuation differs from independent dynamics')
    seen={};flush_passes=set()
    for row in observations:
        count=row['raw_samples'];need(0<count<=len(raw),'Invalid live observation stream position')
        phase=row['pass'];need(phase in {0,*passes} and row['accepted']==(phase==0 or phase in accepted_ids),'Live observation phase differs from numerical solver oracle')
        names=set(row)&{kind+mode for kind in KINDS for mode in MODES}
        for name in names:
            mode=next(m for m in MODES if name.endswith(m));selected=[s for s in raw[:count] if mode=='All' or s['accepted'] or (mode=='Current' and s['pass']==phase and not s['accepted'])]
            need(row[name]['actual']==row[name]['expected']==len(selected),'Live plot membership count differs from unfiltered stream')
        key=(row['boundary'],phase,count);seen.setdefault(key,set()).update(names)
        if row['boundary']=='native-flush':flush_passes.add(phase)
    all_names={kind+mode for kind in KINDS for mode in MODES}
    for i,s in enumerate(raw,1):need(seen.get(('publication',s['pass'],i))==all_names-{'OrbCurrent'},'Missing per-publication viewer/mode observation')
    need(set(passes)<=flush_passes and proof['flush_observations']>=len(passes),'Missing live Current replacement at a native completed-pass boundary')
    need(seen.get(('completed',0,len(raw)))==all_names,'Missing final all-viewer conjunction')
    for (boundary,_,_),names in seen.items():
        if boundary=='native-flush':need(names==all_names,'Partial native-flush viewer matrix')
    return proof

def qualify(repo,path,inventory):
    q=load(path)
    need(q['contract_kind']=='source-versioned-GUI-live-composition' and set(q['qualified_keys'])==LEAVES|PARENTS and len(q['qualified_keys'])==12,'Partial or extra solver-drawing credit')
    need(all(q.get(k) is False for k in ('full_current_build_qualified','full_NASA_qualified','technical_core_approval')),'Live GUI proof exceeds scope')
    for p,digest in q['artifact_sha256'].items():need(sha(p)==digest,'Changed live composition artifact: '+p)
    need(q['authority_sha256']==inventory['sources'],'Live composition NASA authority differs')
    review=load(q['review']);rows={r['key']:r for r in inventory['requirements']}
    need(review['sources']==inventory['sources'],'Post-execution review authority differs')
    need({r['key'] for r in review['reviews']}==LEAVES|PARENTS and len(review['reviews'])==12,'Original solver row conjunction omitted')
    for row in review['reviews']:need(row['fingerprint']==rows[row['key']]['fingerprint'] and row['source_text']==rows[row['key']]['text'] and row['rationale'],'Changed solver source-row scope')
    old=load(q['legacy_leaf_audit']);rechecked=recovery.audit(repo,load(q['legacy_leaf_plan']))
    need(old['cohorts']==rechecked['cohorts'] and {r['key'] for r in old['decisions']}==LEAVES and old['pending_count']==9,'Original both-mode leaf evidence or held obligation changed')
    leaf_mapping=recovery.checked_json(old['cohorts'][0]['mapping'],old['cohorts'][0]['mapping_sha256'])
    leaf_claims={r['key']:r['evidence'] for r in leaf_mapping['reviews']}
    parent_audit=load(q['legacy_parent_audit']);legacy_claims=[]
    for cohort in parent_audit['cohorts']:
        for row in cohort['rows']:
            if row['key'] not in PARENTS:continue
            need(row['original_required_clauses']==len(row['checks'])==11,'Original eleven-clause parent scope changed')
            mapping=recovery.checked_json(row['original_mapping'],row['original_mapping_sha256']);original=next(r for r in mapping['reviews'] if r['key']==row['key'])
            need(len(original['evidence'])==11,'Original parent assertion list changed')
            for claim in original['evidence']:
                origin=claim['origin_requirement_key'];matches=[c for c in leaf_claims[origin] if c['test']==claim['test']]
                need(len(matches)==1,'Original parent clause has no complete leaf counterpart')
                for field in ('source','source_sha256','anchor','legacy_assertions','expected','facets'):
                    need(claim.get(field)==matches[0].get(field),'Original parent assertion differs from re-audited leaf: '+field)
            legacy_claims.extend(e for e in original['evidence'] if e['test']=='QtGui.MixedSolverScopes')
            for check in row['checks']:
                if not check['verified']:
                    need(cohort['cohort']=='unattended' and check['test']=='QtGui.MixedSolverScopes','Unhandled old parent obligation')
                    continue
                for field in ('receipt','log','junit'):
                    need(recovery.digest(recovery.payload(check[field]))==check[field+'_sha256'],'Old parent passing evidence changed')
    observed={};unattended=None
    for spec in q['observations']:
        receipt,sources=read_sources(repo,spec,inventory)
        results={r['name']:r for r in receipt['results'] if r['status']!='not-run'}
        need(set(results)==set(spec['cases']),'Executed case set differs')
        if spec['mode']=='visible':validate_visible(receipt,spec['cases'])
        else:
            need(spec['mode']==receipt['mode']=='headless' and set(results)=={'QtGui.MixedSolverScopes'},'Wrong missing unattended case')
        for name,result in results.items():
            case=spec['cases'][name]
            need(result['log']==case['log'] and Path(case['directory'])==Path(result['log']).parent,'Case evidence belongs to another executed directory')
            need(result['status']=='passed' and result['returncode']==0 and passed_junit(Path(case['junit']),name,0),'Live/missing-mode execution did not pass')
            for field in ('log','junit'):need(sha(case[field])==case[field+'_sha256'],'Changed actual execution '+field)
            if name=='QtGui.MixedSolverScopes':
                need(unattended is None,'Duplicate unattended witness');unattended=(result,case,sources)
            else:
                solver=name.removeprefix('QtGui.NativeNasaSolverPublication');need(solver in ('DC','Yukon','Nested','Mixed') and solver not in observed,'Missing/extra/duplicate live solver context')
                observed[solver]=check_live(case,solver,result,sources)
    need(set(observed)=={'DC','Yukon','Nested','Mixed'} and unattended,'Complete original solver and execution-mode conjunction absent')
    result,case,sources=unattended;source='src/qtgui/tests/MixedSolverScopeTests.cpp'
    need(legacy_claims and all(hashlib.sha256(sources[source]).hexdigest()==claim['source_sha256'] for claim in legacy_claims),'Missing-mode assertion source differs from original')
    text=sources[source].decode();log=Path(case['log']).read_text();lines=[re.sub(r'^\s*\d+:\s?','',s).strip() for s in log.splitlines()]
    for claim in legacy_claims:
        need(nasa._valid_legacy_assertions(claim['legacy_assertions'],text),'Original missing-mode assertion conjunction changed')
        for marker in claim['legacy_assertions']['terminal_markers']:
            # Only the measured elapsed field varies in this unchanged source.
            prefix=marker.rsplit('Elapsed=',1)[0]
            need(prefix.endswith('No previous solver mode matrices repeated. '),'Unexpected variable legacy terminal field')
            matches=[s for s in lines if s.startswith(prefix+'Elapsed=')]
            need(len(matches)==1 and re.fullmatch(re.escape(prefix)+r'Elapsed=\d+(?:\.\d+)? s',matches[0]),'Missing exact mixed-solver completion context')
            seconds=float(matches[0].split('Elapsed=')[1][:-2]);need(0<=seconds<45,'Original mixed-case runtime bound failed')
    return q,sorted(LEAVES|PARENTS)


if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    parser.add_argument('--qualification',type=Path,required=True)
    args=parser.parse_args();repo=args.repo.resolve()
    _,keys=qualify(repo,args.qualification,nasa.inventory(repo))
    print(json.dumps(dict(status='passed',qualified_keys=keys,new_test_executions=0)))
