"""Audit the final thirty retained-evidence candidates without changing assertions.

Reference-version restoration is explicit. Assertion sources and every original
clause stay exact. Missing bindings can only produce unqualified diagnostics.
"""
import argparse,copy,json
from pathlib import Path
import AuditAcceptanceRecovery as a

KEYS={'FRR!E'+str(i) for i in [309,333,348,378,388,389,391,393,397,519,637]}|{'FRC!E'+str(i) for i in [12,13,91,99,153,158,159]}|{'All Others!E'+str(i) for i in [25,28,60,75,76,77,78,79,80,84,112,158]}
CASE_REFERENCE='src/qtgui/tests/NasaReportOutputCases.json'
load=lambda p:json.loads(a.payload(p));sha=lambda p:a.digest(a.payload(p))


def selected_report_case_equal(expected,actual,key):
    a.need(key=='SubscriberToggle','Reference equivalence excludes other report cases')
    a.need(set(expected)==set(actual) and {k:v for k,v in expected.items() if k!='cases'}=={k:v for k,v in actual.items() if k!='cases'},'Report case-file metadata changed')
    left=[c for c in expected['cases'] if c.get('key')==key];right=[c for c in actual['cases'] if c.get('key')==key]
    a.need(len(left)==len(right)==1 and left==right,'Executed report case or expected result changed')
    return True


def rebind_claim(claim,references,case_check=None):
    """Preserve every assertion/path; change only reviewed reference versions."""
    a.need(set(references)==set(claim.get('reference_sources',{})),'Reference restoration drops/adds a guard')
    for path,value in references.items():
        if value==claim['reference_sources'][path]:continue
        production=(path.startswith(('src/','plugins/')) and '/tests/' not in path
                    and Path(path).suffix in ('.cpp','.hpp','.h'))
        a.need(production or (path==CASE_REFERENCE and case_check and case_check()),'Changed assertion/data/normative reference is not qualified')
    return dict(copy.deepcopy(claim),reference_sources=dict(references))


def bound_receipt(repo,versions,spec):
    label=spec['collection'];a.need(label in versions,'Unknown original receipt collection')
    version=versions[label]
    for field in ('audit','report','receipt','mapping_manifest'):a.checked_json(repo/version[field],version[field+'_sha256'])
    audit=load(repo/version['audit']);wanted=str(Path(spec['receipt']).resolve());digest=spec['receipt_sha256'];matches=[]
    def visit(value):
        if isinstance(value,dict):
            if 'receipt' in value and 'receipt_sha256' in value:
                if str(Path(value['receipt']).resolve())==wanted and value['receipt_sha256']==digest:matches.append(value)
            for v in value.values():visit(v)
        elif isinstance(value,list):
            for v in value:visit(v)
    visit(audit)
    direct=str(Path(version['receipt']).resolve())==wanted and version['receipt_sha256']==digest
    raw=any(str(Path(p).resolve())==wanted and h==digest for p,h in audit.get('raw_artifact_sha256',{}).items())
    a.need(direct or raw or matches,'Receipt is outside its original hash-bound collection')
    a.need(spec['receipt'].endswith('/results.json'),'Only original execution results are admitted')
    receipt=a.checked_json(spec['receipt'],digest);manifest=load(receipt['snapshot_manifest']);identity=dict(manifest);sid=identity.pop('snapshot_id')
    a.need(a.snapshot.fingerprint(identity)==sid==receipt['snapshot_id'] and receipt.get('finished_utc') and receipt.get('snapshot_unchanged'),'Original execution unfinished or snapshot identity changed')
    return receipt,manifest


def audit(repo,plan):
    repo=Path(repo);ledger=a.checked_json(repo/plan['ledger'],plan['ledger_sha256']);versions={v['label']:v for v in ledger['versions']}
    baseline=a.checked_json(repo/plan['baseline_report'],plan['baseline_report_sha256']);union=a.checked_json(repo/plan['baseline_union'],plan['baseline_union_sha256'])
    before=a.checked_json(plan['before_campaign'],plan['before_campaign_sha256']);pending={r['key'] for r in before['candidates'] if r['task']=='recovery' and r['status']=='pending'}
    a.need(pending==KEYS==set(plan['rows'])=={d['key'] for d in plan['decisions']} and len(plan['decisions'])==30,'Must review every remaining original candidate exactly once')
    a.need(set(plan['required_cohorts'])=={'unattended','visible'},'Both original execution modes remain mandatory')
    source_rows={r['key']:r for r in baseline['requirements']};decisions={d['key']:d for d in plan['decisions']};accepted={k for k,d in decisions.items() if d['decision']=='accept_versioned_scope'}
    verified={r['key'] for r in union['requirements']['rows']}|{r['key'] for r in before['candidates'] if r['status'] in ('recovered','verified_contract')}|accepted
    caches={};maps={};observations=[];actual_contexts=set();case_equivalence=[];all_clauses=0;rebound_count=0
    for mode in ['unattended','visible']:
        rows=[]
        for key,spec in plan['rows'].items():
            d=decisions[key];a.need(d['decision'] in ('accept_versioned_scope','missing_obligation') and d.get('rationale') and d.get('remaining_limits'),'Missing substantive source-scope review')
            a.need(d.get('source_text')==source_rows[key]['text'] and (d['decision']!='missing_obligation' or d.get('missing_obligations')),'Missing source text or exact remaining obligation')
            version=versions[spec['version']]
            if spec['version'] not in maps:
                for field in ('audit','report','receipt','mapping_manifest'):a.checked_json(repo/version[field],version[field+'_sha256'])
                maps[spec['version']]=load(repo/version['mapping_manifest'])
            mapping=maps[spec['version']];original=next(r for r in mapping['reviews'] if r['key']==key)
            a.need(mapping['sources']==baseline['sources'] and original['fingerprint']==source_rows[key]['fingerprint'],'Original NASA source row/authority changed')
            claims=original['evidence'];a.need(bool(claims) and set(spec['bindings'][mode])=={c['test'] for c in claims},'Original clause/test conjunction omitted or added')
            for test,binding in spec['bindings'][mode].items():
                if binding is not None:
                    paths={p for c in claims if c['test']==test for p in c.get('reference_sources',{})}
                    a.need(set(binding['reference_sources'])==paths,'Execution reference binding drops/adds an original guard')
            if key in accepted:
                prefix=source_rows[key]['declared_id'].removesuffix('.0')+'.'
                children={k for k,r in source_rows.items() if k!=key and r['declared_id'].startswith(prefix)}
                a.need(children<=verified,'Accepted parent retains an unqualified source child')
            checks=[]
            for claim in claims:
                all_clauses+=1;b=spec['bindings'][mode][claim['test']]
                if b is None:
                    a.need(key not in accepted,'Accepted row is missing a mandatory original execution')
                    checks.append(dict(test=claim['test'],verified=False,rejection='No admissible exact-source original execution bound for this mandatory mode/clause.',native_exposure_events=0,source=claim['source'],source_sha256=claim['source_sha256']));continue
                identity=(b['collection'],b['receipt'],b['receipt_sha256'])
                try:
                    if identity not in caches:
                        receipt,manifest=bound_receipt(repo,versions,b)
                        a.need(a.nasa.inventory(Path(manifest['root']))['sources']==baseline['sources'],'Frozen NASA authority changed')
                        caches[identity]=(receipt,manifest)
                    receipt,manifest=caches[identity];frozen=Path(manifest['root'])
                    a.need(receipt['mode']==('headless' if mode=='unattended' else 'visible'),'Wrong actual execution mode')
                    result=next(r for r in receipt['results'] if r['name']==claim['test'])
                    refs={p:b['reference_sources'][p] for p in claim.get('reference_sources',{})}
                    def case_check():
                        ref=plan['case_reference_source'];rr,mm=bound_receipt(repo,versions,ref);path=Path(mm['root'])/CASE_REFERENCE
                        a.need(rr['source_hashes'].get(CASE_REFERENCE)==sha(path)==claim['reference_sources'][CASE_REFERENCE] and mm['artifact_hashes'][CASE_REFERENCE]==sha(path),'Mapped expected-case reference is not the original recorded source')
                        command=result['registered_command'];actual_path=frozen/CASE_REFERENCE
                        a.need(command[-2:]==[str(actual_path),'SubscriberToggle'] and claim['test']=='QtGui.NasaReportOutputSubscriberToggle','Case-reference comparison does not cover the executed argument')
                        source=a.payload(frozen/claim['source']).decode()
                        a.need('const QString key=argv[3]' in source and 'if(c.toObject()["key"]==key)spec=c.toObject()' in source,'Unknown report case selection mechanism')
                        selected_report_case_equal(load(path),load(actual_path),'SubscriberToggle')
                        case_equivalence.append(dict(test=claim['test'],receipt=b['receipt'],mapped_case_source_sha256=sha(path),actual_case_source_sha256=sha(actual_path),selected_case_equal=True,noncase_metadata_equal=True));return True
                    rebound=rebind_claim(claim,refs,case_check);rebound_count+=sum(refs[p]!=h for p,h in claim.get('reference_sources',{}).items())
                    check=a.verify_claim(frozen,manifest,receipt,result,rebound)
                    if mode=='visible':a.need(check['native_exposure_events']>0,'Missing native visible exposure')
                    actual_contexts.add((claim['test'],b['receipt']));checks.append(dict(check,verified=True,receipt=b['receipt'],receipt_sha256=b['receipt_sha256'],reference_sources=refs,original_reference_sources=claim.get('reference_sources',{}),assertion_fields_unchanged=True))
                except (ValueError,KeyError,StopIteration) as error:
                    a.need(key not in accepted,'Accepted row has an unqualified mandatory clause: '+key+' '+str(error))
                    checks.append(dict(test=claim['test'],verified=False,rejection=str(error),receipt=b['receipt'],receipt_sha256=b['receipt_sha256'],native_exposure_events=0,source=claim['source'],source_sha256=claim['source_sha256']))
            rows.append(dict(key=key,original_mapping=version['mapping_manifest'],original_mapping_sha256=version['mapping_manifest_sha256'],original_required_clauses=len(claims),checks=checks))
        observations.append(dict(cohort=mode,rows=rows,explicitly_composed_source_versioned=True))
    return dict(version=plan['version'],audit_type='remaining-recovery',decisions=plan['decisions'],cohorts=observations,accepted_source_versioned_scope_keys=dict(requirements=sorted(accepted),historical_cases=[]),accepted_count=len(accepted),pending_count=30-len(accepted),original_clause_checks=all_clauses,reference_version_restorations=rebound_count,distinct_verified_contexts=len(actual_contexts),selected_case_reference_equivalence=case_equivalence,new_test_executions=0,full_current_build_qualified=False,full_NASA_qualified=False,limit='Full original candidates reviewed; all assertions and paths preserved. Missing-mode/source/result obligations stay unqualified. Actual old reference versions do not establish newest-binary or scientific/NASA qualification.')


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);parser.add_argument('--plan',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    report=audit(args.repo,load(args.plan));report.update(plan_path=str(args.plan.resolve()),plan_sha256=sha(args.plan),auditor_sha256=sha(__file__),original_claim_verifier_sha256=sha(a.__file__));args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['accepted_count','pending_count','original_clause_checks','distinct_verified_contexts']}))

if __name__=='__main__':main()
