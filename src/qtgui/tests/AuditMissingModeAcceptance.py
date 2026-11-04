"""Complete exact original GUI conjunctions with previously missing executions.

Old positive clauses retain their recorded source versions. New executions
bind their actual receipt and wider independent before/after source snapshot.
Unchanged normative references may be reviewed after execution; they are never
represented as executable inputs that were captured by the original receipt.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import tarfile
import tempfile

import AuditRemainingRecovery as original
import AuditAcceptanceRecovery as recovery
from AuditSolverPublicationAcceptance import read_sources
import NasaGuiCoverage as nasa
from BuildNasaCoverageExtension import validate_visible
from RunHarnessAcceptance import passed_junit

KEYS={'FRR!E309','FRR!E397','FRC!E13'}|{'All Others!E'+str(i) for i in (25,28,60,79,80,84,112,158)}
# Reviewed pre-existing evolution: active branch contracts/oracles are preserved.
EVOLUTION={
 'c178a76b7c509d5297302da67fd14e073232686f18ad53e40a8d2c12392a0c59':'5b9e15e9a59a3146157d646f52be2eb8d1191bc9fae22e477db7f7fcd18877ef',
 'e14e37538654695045c13550672e59c4f3312dc7f35134f6a2fa72d2731952b2':'82f96a4888d0da7b487e8aee9673c65c9e36d14f5282e34fd6c5d84e94f4898a',
 'a0f15e20fafd96e57984d68ee7ecc96035c7327cf8232d06ad95adf5788d317b':'82f96a4888d0da7b487e8aee9673c65c9e36d14f5282e34fd6c5d84e94f4898a',
 '6b3be011fbc4003f34d1396272f744d3ea09c50956758726d9c8106f5eef6383':'9c52a9c4e9a998835a21fbccc3582da2ca8cde277ba79e538c7cda9e5cd94c5a',
}
APPROVED_REFERENCE_REVIEW_SHA256='55868f04aa1037b71ad9c5262a8df60915fa76ae362bc6be74886cc54d110bcc'

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def load(p):
    return json.loads(Path(p).read_text())

def need(ok,message):
    if not ok:raise ValueError(message)

def proof_matches(claim,result,source):
    need(claim['anchor'] in source.decode(),'Original assertion/checkpoint anchor absent')
    if 'legacy_assertions' in claim:
        # Only unchanged exact-source legacy cases are used as new witnesses.
        need(hashlib.sha256(source).hexdigest()==claim['source_sha256'],'Changed legacy assertion source')
        with tempfile.TemporaryDirectory(prefix='gmat-original-clause-') as folder:
            p=Path(folder)/claim['source'];p.parent.mkdir(parents=True);p.write_bytes(source)
            local=copy.deepcopy(result)
            for field in ('log','junit'):local[field+'_sha256']=sha(local[field])
            need(nasa.legacy_assertions_verified(Path(folder),local,claim),'Original exact terminal/assertion/JUnit conjunction failed')
    else:
        records=[p for p in nasa.proof_records(result) if p.get('checkpoint')==claim['checkpoint']
                 and set(claim['facets'])<=set(p.get('facets',[]))
                 and all(p.get(k)==v for k,v in claim.get('expected',{}).items())]
        need(len(records)==1,'Original expected facts/facets are missing or ambiguous')
        p=records[0]
        if claim['test'].startswith('QtGui.NasaOptimizerRule'):
            # Unique checks baseline and recovered phases; Feasible has one
            # accepted report containing both independent optimizer results.
            need(len(p['reports'])>=(2 if p['mode']=='Unique' else 1),'Original independent optimizer results absent')
            for r in p['reports']:
                v=r['values'];need(len(v)==5 and abs(v[1]+1)<2e-5 and abs(v[3])<2e-5 and v[4]==42,'Independent optimizer/sentinel/schema failed')
                need(abs(v[2]-(v[0]-2)**2)<2e-5,'Independent quadratic result failed')
                need(3-2e-5<=v[0]<=4+2e-5 if r['feasibility_only'] else abs(v[0]-r['expected_x'])<2e-5,'Independent optimum/feasibility bounds failed')
    return True

def qualify(repo,path,inventory):
    q=load(path)
    need(q['contract_kind']=='source-versioned-missing-GUI-conjunction' and set(q['qualified_keys'])==KEYS and len(q['qualified_keys'])==11,'Partial/extra/duplicate original-conjunction credit')
    need(all(q.get(k) is False for k in ('full_current_build_qualified','full_NASA_qualified','technical_core_approval')),'Conjunction exceeds qualification scope')
    for name,digest in q['artifact_sha256'].items():need(sha(name)==digest,'Changed conjunction artifact: '+name)
    need(q['authority_sha256']==inventory['sources'],'Original NASA authority changed')
    old=load(q['original_audit']);checked=original.audit(repo,load(q['original_plan']))
    need(old['cohorts']==checked['cohorts'],'Original complete clause/mode audit changed')
    review=load(q['review']);known={r['key']:r for r in inventory['requirements']}
    need({r['key'] for r in review['reviews']}==KEYS and len(review['reviews'])==11 and review['sources']==inventory['sources'],'Original row review conjunction changed')
    for r in review['reviews']:need(r['fingerprint']==known[r['key']]['fingerprint'] and r['source_text']==known[r['key']]['text'] and r['rationale'],'Original row text/fingerprint changed')
    references={}
    with tarfile.open(q['reference_archive']) as archive:
        need({m.name for m in archive}==set(q['reference_sha256']),'Reference preservation partition differs')
        for name,digest in q['reference_sha256'].items():
            need(not Path(name).is_absolute() and '..' not in Path(name).parts,'Unsafe reference member')
            data=archive.extractfile(name).read();need(not data.startswith(b'\x7fELF') and hashlib.sha256(data).hexdigest()==digest,'Changed/non-source reference member');references[name]=data
    observations={}
    for spec in q['observations']:
        need(spec['id'] not in observations,'Duplicate execution observation')
        receipt,sources=read_sources(repo,spec,inventory);results={r['name']:r for r in receipt['results'] if r['status']!='not-run'}
        need(set(results)==set(spec['cases']) and receipt['mode']==spec['mode'],'Execution mode or exact selected case set differs')
        if spec['mode']=='visible':validate_visible(receipt,spec['cases'])
        else:need(spec['mode']=='headless','Unrecognized execution mode')
        for name,result in results.items():
            case=spec['cases'][name];need(result['status']=='passed' and result['returncode']==0 and passed_junit(Path(case['junit']),name,0),'Missing exact passing execution/JUnit')
            need(result['log']==case['log'] and Path(case['directory'])==Path(result['log']).parent,'Case artifacts belong to another execution')
            for field in ('log','junit'):need(sha(case[field])==case[field+'_sha256'],'Changed actual execution '+field)
        observations[spec['id']]=(spec,receipt,sources,results,load(spec['inputs_before']))
    evolution=load(q['source_evolution']);versions={(r['source'],r['old_sha256'],r['new_sha256']):r for r in evolution['reviews']}
    need(len(versions)==4 and all(EVOLUTION.get(old)==new and r['rationale'] for (_,old,new),r in versions.items()),'Unreviewed test-source evolution')
    for (_,old_digest,_),r in versions.items():need(sha(r['old_path'])==old_digest,'Original evolved assertion source was not preserved')
    need(sha(q['reference_evolution'])==APPROVED_REFERENCE_REVIEW_SHA256,'Unreviewed oracle/documentation reference evolution')
    reference_reviews=load(q['reference_evolution'])['reviews']
    reference_versions={(r['path'],r['old_sha256'],r['new_sha256']) for r in reference_reviews}
    need(len(reference_versions)==len(reference_reviews)==10 and all(r['rationale'] and sha(r['old_path'])==r['old_sha256'] for r in reference_reviews),'Incomplete original reference preservation/review')
    used=set();clauses=0;normative=set()
    old_rows={(cohort['cohort'],r['key']):r for cohort in old['cohorts'] for r in cohort['rows']}
    for key in sorted(KEYS):
        for mode in ('visible','unattended'):
            row=old_rows[(mode,key)];mapping=recovery.checked_json(row['original_mapping'],row['original_mapping_sha256']);mapped=next(r for r in mapping['reviews'] if r['key']==key)
            claims=mapped['evidence'];need(len(claims)==row['original_required_clauses']==len(row['checks']),'Original clause removed/added')
            need(mapping['sources']==inventory['sources'] and mapped['fingerprint']==known[key]['fingerprint'],'Original mapping source row changed')
            for claim,prior in zip(claims,row['checks']):
                clauses+=1;need(claim['test']==prior['test'],'Original clause order/identity changed')
                if prior['verified']:continue
                binding=q['bindings'].get(mode,{}).get(claim['test']);need(binding in observations,'Missing mandatory new mode/clause binding')
                spec,receipt,sources,results,before=observations[binding];need(spec['mode']==('visible' if mode=='visible' else 'headless'),'New witness is in the wrong execution mode')
                name=claim['test'];need(name in results,'Bound test not actually executed');result=results[name];source=sources[claim['source']]
                source_hash=hashlib.sha256(source).hexdigest()
                if source_hash!=claim['source_sha256']:
                    need((claim['source'],claim['source_sha256'],source_hash) in versions,'Changed original assertion source lacks exact semantic review')
                    if name.startswith('QtGui.NasaOptimizerRule'):
                        text=source.decode();need('const bool nonlinear=mode=="Nonlinear"' in text and 'const int count=nonlinear ? 7:5' in text,'Reviewed optimizer schema partition changed')
                        if name.endswith('Unique'):need('Duplicate Minimize was offered by the chooser' in text and 'Objective-free optimizer context did not build' in text,'Original uniqueness contract was not strengthened/preserved')
                # RunAcceptance keeps exact JUnit paths in per-case evidence;
                # bind a local assessment copy, never rewrite its receipt.
                case=spec['cases'][name]
                assessed=dict(result,junit=case['junit'],junit_sha256=case['junit_sha256'],log_sha256=case['log_sha256'])
                proof_matches(claim,assessed,source)
                for ref,expected in claim.get('reference_sources',{}).items():
                    need(ref in references,'Original reference guard omitted: '+ref)
                    data=references[ref];actual=hashlib.sha256(data).hexdigest()
                    if ref in before['sources']:
                        need(before['sources'][ref]==actual,'Reference differs from independent pre-execution capture')
                        if actual!=expected:
                            production=ref.startswith(('src/','plugins/')) and '/tests/' not in ref and Path(ref).suffix in ('.cpp','.hpp','.h')
                            reviewed_reference=(ref,expected,actual) in reference_versions
                            if not production and not reviewed_reference:
                                need(ref=='src/qtgui/tests/NasaEphemerisSamplingCases.json' and name=='QtGui.NasaEphemerisSamplingRawSteps','Unreviewed test/normative reference change')
                                oldcase=load(q['original_ephemeris_cases']);need(sha(q['original_ephemeris_cases'])==expected,'Original case input version differs')
                                current=json.loads(data);need({k:v for k,v in oldcase.items() if k!='cases'}=={k:v for k,v in current.items() if k!='cases'} and [c for c in oldcase['cases'] if c['key']=='RawSteps']==[c for c in current['cases'] if c['key']=='RawSteps'],'Executed ephemeris case changed')
                                need(result['registered_command'][-1]=='RawSteps' and str(repo/ref) in result['registered_command'],'Selected case file/key not an actual command input')
                    else:
                        permitted=ref.startswith('src/gui/') or ref in ('application/README.txt','build/nasa-coverage/help-links-review/primary-sources.json','build/nasa-coverage/help-contact-review/primary-sources.json')
                        need(permitted and actual==expected,'Unrecorded executable/test input cannot be treated as a normative reference: '+ref)
                        normative.add(ref)
                    if ref.startswith('application/samples/'):
                        need(ref in before['sources'] and before['sources'][ref]==actual==expected,'Actual tracking template lacked its pre-execution identity')
                used.add((mode,name,binding))
    need(clauses==q['original_clause_count'],'Original full conjunction size changed')
    need(normative==set(q['post_execution_unchanged_normative_references']),'Normative review incorrectly attributed to pre-execution input capture')
    return q,sorted(KEYS)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);parser.add_argument('--qualification',type=Path,required=True);args=parser.parse_args();_,keys=qualify(args.repo.resolve(),args.qualification,nasa.inventory(args.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,new_test_executions=0)))
