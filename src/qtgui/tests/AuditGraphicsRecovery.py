"""Audit the seven complete OrbitView display-option source rows.

Original qualified report clauses, all child assertions and both actual GUI
modes remain mandatory. Offscreen raster is an explicit support component.
"""
import argparse
import json
from pathlib import Path
import AcceptanceSnapshot as snapshot
import AuditAcceptanceRecovery as prior

BASE='orbit-star-control-availability-raster-and-first-unattended-routes'
OLD='orbitview-six-optional-displays-with-body-grid-repair'
EXTRA='five-missing-force-input-routes-and-central-body-type-guard'
KEYS={'FRR!E'+str(i) for i in range(321,328)}
load=lambda p:json.loads(prior.payload(p))
sha=lambda p:prior.digest(prior.payload(p))
need=prior.need
FLAGS={'verified_in_run','result','log','native_exposure_events','claim_id'}


def assertion_part(claim):
    return {k:v for k,v in claim.items() if k not in FLAGS|{'source_sha256','reference_sources'}}


def viewer_source_equal(left,right,mode):
    """Remove only the unexecuted Resource branch; compare every other byte."""
    need(mode=='Viewer','Source equivalence excludes Resource')
    def common(text):
        prefix='if(mode=="Resource"){'
        need(text.count(prefix)==1,'Unknown orbit-star source structure')
        start=text.index(prefix)+len(prefix);index=start;depth=1;quote=None;escape=False
        while depth:
            char=text[index];index+=1
            if quote:
                if escape:escape=False
                elif char=='\\':escape=True
                elif char==quote:quote=None
            elif char in ('"',"'"):quote=char
            elif char=='{':depth+=1
            elif char=='}':depth-=1
        return text[:start]+'/* unexecuted Resource branch */'+text[index-1:]
    need(common(left)==common(right),'Executed Viewer code or assertion changed')
    return True


def audit(repo,plan):
    repo=Path(repo)
    ledger=prior.checked_json(repo/plan['ledger'],plan['ledger_sha256'])
    baseline=prior.checked_json(repo/plan['baseline_report'],plan['baseline_report_sha256'])
    union=prior.checked_json(repo/plan['baseline_union'],plan['baseline_union_sha256'])
    need(plan['version']==BASE and set(plan['required_cohorts'])=={'unattended','visible'},'Wrong graphics recovery modes/version')
    need({d['key'] for d in plan['decisions']}==KEYS and len(plan['decisions'])==7,'Graphics review must retain all seven source rows')
    semantic=prior.checked_json(plan['semantic_review']['path'],plan['semantic_review']['sha256'])
    measured={(r['test'],r['receipt']):r for r in semantic['records']}
    need(semantic['status']=='passed' and len(measured)==len(semantic['records'])==9
         and len(semantic['original_images_rechecked'])==22 and len(semantic['negative_mutations_rejected'])==9
         and semantic['new_GUI_executions']==semantic['new_images']==semantic['image_copies']==0,
         'Incomplete independent original graphics data review')
    for path,value in semantic['input_sha256'].items():need(sha(path)==value,'Original graphics review input changed')
    for path,value in semantic['original_images_rechecked'].items():
        if Path(path).is_file():need(sha(path)==value,'Retained original graphics image changed')
    versions={v['label']:v for v in ledger['versions']};maps={};reports={};contexts={}
    for label in (BASE,OLD,EXTRA):
        v=versions[label]
        old=prior.checked_json(v['audit'],v['audit_sha256'])
        reports[label]=prior.checked_json(v['report'],v['report_sha256'])
        maps[label]=prior.checked_json(v['mapping_manifest'],v['mapping_manifest_sha256'])
        prior.checked_json(v['receipt'],v['receipt_sha256'])
        for name,context in old.get('source_bound_evidence',{}).items():contexts[label+':'+name]=context
        if label==EXTRA:
            path=str(repo/'build/gui-acceptance/20261027-tide-suite-headless/results.json')
            need(path in old['raw_artifact_sha256'],'Original unattended component receipt was not bound')
            contexts[label+':retained-headless']=dict(receipt=path,receipt_sha256=old['raw_artifact_sha256'][path])
    base={r['key']:r for r in maps[BASE]['reviews']};candidates={r['key']:r for r in baseline['requirements']}
    observations=[];executions=set();checks_count=0;support=set();GUI=set()
    for mode in ('unattended','visible'):
        spec=plan['mode_reviews'][mode]
        mapping=prior.checked_json(spec['mapping'],spec['mapping_sha256'])
        rows={r['key']:r for r in mapping['reviews']}
        need(mapping['sources']==baseline['sources']==maps[BASE]['sources'],'NASA graphics authority changed')
        need(set(rows)==KEYS and len(mapping['reviews'])==7,'Incomplete reviewed graphics rows')
        required=set();observed=[]
        for key,row in rows.items():
            original=base[key]
            need(row['fingerprint']==original['fingerprint']==candidates[key]['fingerprint'],'Graphics row identity changed')
            need(len(row['evidence'])==len(original['evidence']) and row.get('rationale'),'Graphics review drops an original child clause')
            checks=[]
            for clause,blueprint in zip(row['evidence'],original['evidence']):
                required.add(clause['test'])
                need(assertion_part(clause)==assertion_part(blueprint),'Graphics assertion/scope/expected/facet altered')
                need(set(clause['reference_sources'])==set(blueprint['reference_sources']),'Graphics reference path guard changed')
                alias=spec['bindings'][clause['test']];need(alias in contexts,'Graphics receipt outside original collections')
                context=contexts[alias]
                receipt=prior.checked_json(context['receipt'],context['receipt_sha256'])
                manifest=load(receipt['snapshot_manifest']);identity=dict(manifest);sid=identity.pop('snapshot_id')
                need(snapshot.fingerprint(identity)==sid==receipt['snapshot_id'],'Graphics snapshot identity changed')
                need(receipt.get('finished_utc') and receipt['snapshot_unchanged'],'Graphics execution did not finish unchanged')
                frozen=Path(manifest['root']);need(prior.nasa.inventory(frozen)['sources']==baseline['sources'],'Frozen graphics NASA authorities differ')
                result=next(r for r in receipt['results'] if r['name']==clause['test'])
                raster=clause['test']=='QtGui.NasaOrbitGuideRaster'
                if raster:
                    text=''.join(prior.payload(frozen/clause['source']).decode().split())
                    need(result.get('requested_visibility')=='headless' and result['registered_command'][-1]=='GridRaster'
                         and 'QGuiApplication::platformName()=="offscreen"' in text,
                         'Raster must retain its declared offscreen support route')
                    support.add((clause['test'],context['receipt']))
                else:
                    need(receipt['mode']==('headless' if mode=='unattended' else 'visible'),'Wrong actual graphics GUI mode')
                    if mode=='visible':need(result.get('trace',{}).get('native_exposure_events',0)>0,'Graphics GUI witness was not exposed')
                    GUI.add((clause['test'],context['receipt']))
                # A source change is admitted only if this complete original
                # source-version clause was already recorded by a hash-bound
                # qualified report. Legacy assertion sources must stay exact.
                if clause['source_sha256']!=blueprint['source_sha256']:
                    qualified=[]
                    for report in (reports[BASE],reports[OLD]):
                        for rr in report['requirements']:
                            if rr['key']==key:
                                qualified += [e for e in rr['evidence'] if e.get('verified_in_run')
                                              and assertion_part(e)==assertion_part(clause)
                                              and e['source_sha256']==clause['source_sha256']]
                    if not qualified and clause['test']=='QtGui.NativeNasaOrbitStarControlsViewer':
                        need(result['registered_command'][-1]=='Viewer','Viewer source review does not cover the executed argument')
                        original_context=contexts[BASE+':stars-auto']
                        other_receipt=prior.checked_json(original_context['receipt'],original_context['receipt_sha256'])
                        other_manifest=load(other_receipt['snapshot_manifest']);other=Path(other_manifest['root'])/clause['source']
                        need(sha(other)==blueprint['source_sha256'],'Mapped Viewer source changed')
                        viewer_source_equal(prior.payload(other).decode(),prior.payload(frozen/clause['source']).decode(),'Viewer')
                    else:need(bool(qualified),'Different graphics assertion source lacks an original qualified clause')
                check=prior.verify_claim(frozen,manifest,receipt,result,clause)
                if clause['test'] not in ('QtGui.OrbitSetup','QtGui.NativeOrbit'):
                    record=measured[(clause['test'],context['receipt'])]
                    proof=prior.nasa.proof_records(result)[0]
                    need(record['receipt_sha256']==context['receipt_sha256'] and record['log_sha256']==result['log_sha256']
                         and record['proof_sha256']==prior.digest(json.dumps(proof,sort_keys=True).encode()),
                         'Independent graphics data does not bind the exact original proof')
                checks.append(dict(check,receipt=context['receipt'],receipt_sha256=context['receipt_sha256'],
                                   original_context=alias,role='offscreen-raster-support' if raster else 'GUI-witness'))
                executions.add((clause['test'],context['receipt']));checks_count+=1
            observed.append(dict(key=key,checks=checks))
        need(set(spec['bindings'])==required,'Graphics bindings omit or add an original test')
        observations.append(dict(cohort=mode,rows=observed,mapping=spec['mapping'],mapping_sha256=spec['mapping_sha256'],explicitly_composed_source_versioned=True))
    need(all(d['decision']=='accept_versioned_scope' and d.get('rationale') and d.get('remaining_limits') for d in plan['decisions']),
         'Missing complete graphics semantic review')
    need(not KEYS&{r['key'] for r in union['requirements']['rows']},'Graphics row already counted')
    need(len(GUI)==12 and len(support)==1 and len(executions)==13,'Incomplete original graphics execution conjunction')
    return dict(version=BASE,decisions=plan['decisions'],cohorts=observations,
                accepted_source_versioned_scope_keys=dict(requirements=sorted(KEYS),historical_cases=[]),
                accepted_count=7,pending_count=0,new_test_executions=0,full_current_build_qualified=False,full_NASA_qualified=False,
                audit_type='graphics-recovery',distinct_GUI_contexts=len(GUI),distinct_raster_support_contexts=len(support),
                total_original_clause_checks=checks_count,
                independent_payload_review=plan['semantic_review'],
                limit='Complete original selected graphics-option conjunction across explicitly versioned executions. '
                      'Offscreen raster support is never presented as a native visible witness. Arbitrary views/bodies/frames, '
                      'physical science, OS/GPU/platform and current-build/NASA qualification remain open.')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--plan',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    result=audit(args.repo,load(args.plan));result.update(plan_path=str(args.plan.resolve()),plan_sha256=sha(args.plan),auditor_sha256=sha(__file__),original_claim_verifier_sha256=sha(prior.__file__))
    args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(accepted=7,distinct_GUI_contexts=12,offscreen_support_contexts=1,new_executions=0)))


if __name__=='__main__':main()
