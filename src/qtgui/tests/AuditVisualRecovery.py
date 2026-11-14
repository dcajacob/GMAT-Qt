"""Recover only FRR47/48/49 from explicit original rendering executions.

The newer driver's only functional difference is an unexecuted Load3DS picker
branch. Its six transformation cases must have byte-identical applicable code.
Original mappings and receipts are never changed or attributed to newer builds.
"""
import argparse
import copy
import json
from pathlib import Path

import AcceptanceSnapshot as snapshot
import AuditAcceptanceRecovery as prior

BASE = 'visual-transforms-seven'
LEGACY = 'legacy-visual-three'
KEYS = {'FRR!E47','FRR!E48','FRR!E49'}
SOURCE = 'src/qtgui/tests/NasaVisualTransformTests.cpp'
FAMILIES = {'Translate','RotateX','RotateY','RotateZ','Scale','Compose'}
load = lambda p: json.loads(prior.payload(p))
sha = lambda p: prior.digest(prior.payload(p))
need = prior.need


def applicable_source_equal(earlier, later, family):
    """Exact source normalization for the six non-Load3DS branches only."""
    need(family in FAMILIES, 'Source equivalence does not cover Load3DS')
    prefix='acceptanceView(*d);if(key=="Load3DS"){'
    suffix='}else{d->selectFile(asset);selected=QMetaObject::invokeMethod(d,"accept",Qt::DirectConnection);}'
    need(later.count(prefix)==1 and later.count(suffix)==1, 'Unexpected later driver branch')
    start=later.index(prefix);end=later.index(suffix,start)+len(suffix)
    selected=later[start:end]
    need('QInputMethodEvent input' in selected and 'QDialogButtonBox::Open' in selected,
         'Unexpected Load3DS-specific repair')
    normalized=later[:start]+'acceptanceView(*d);d->selectFile(asset);selected=QMetaObject::invokeMethod(d,"accept",Qt::DirectConnection);'+later[end:]
    include='#include <QInputMethodEvent>\n'
    diagnostic='save("picker-result.json",QJsonDocument(QJsonObject{{"expected",asset},{"actual",field("ModelFile")->text()},{"accepted",selected}}).toJson());'
    need(normalized.count(include)==1 and normalized.count(diagnostic)==1, 'Unexpected later driver diagnostics')
    normalized=normalized.replace(include,'').replace(diagnostic,'')
    need(normalized==earlier, 'Executed transformation code or an assertion changed')
    return dict(applicable_source_byte_equal=True,excluded_branch='Load3DS',family=family)


def audit(repo, plan):
    repo=Path(repo)
    ledger=prior.checked_json(repo/plan['ledger'],plan['ledger_sha256'])
    baseline=prior.checked_json(repo/plan['baseline_report'],plan['baseline_report_sha256'])
    union=prior.checked_json(repo/plan['baseline_union'],plan['baseline_union_sha256'])
    need(plan['version']==BASE and set(plan['required_cohorts'])=={'visible','unattended'},
         'Wrong visual recovery scope or execution modes')
    need({d['key'] for d in plan['decisions']}==KEYS and len(plan['decisions'])==3,
         'Visual adapter is limited to the three transform rows')
    semantic=prior.checked_json(plan['semantic_review']['path'],plan['semantic_review']['sha256'])
    measured={(r['test'],r['mode']):r for r in semantic['records']}
    expected={('QtGui.NativeNasaVisual'+family,mode) for family in FAMILIES for mode in ('unattended','visible')}
    need(semantic['status']=='passed' and set(measured)==expected
         and len(semantic['records'])==12 and len(semantic['negative_mutations_rejected'])==36
         and len(semantic['original_images_rechecked'])==36
         and semantic['new_GUI_executions']==semantic['new_images']==semantic['image_copies']==0,
         'Incomplete independent original pixel/geometry review')
    for path,value in semantic['input_sha256'].items():
        need(sha(path)==value,'Independent visual review input changed')
    # Passing image files can later be pruned; their independently checked
    # measurements and original hashes remain in this immutable review.
    for image in semantic['original_images_rechecked']:
        path=Path(image['path'])
        if path.is_file():need(sha(path)==image['sha256'],'Retained original image changed')
    versions={v['label']:v for v in ledger['versions']}
    origin={};contexts={}
    for label in (BASE,LEGACY):
        version=versions[label]
        mapping=prior.checked_json(version['mapping_manifest'],version['mapping_manifest_sha256'])
        old=prior.checked_json(version['audit'],version['audit_sha256'])
        for field in ('report','receipt'):prior.checked_json(version[field],version[field+'_sha256'])
        need(mapping['sources']==baseline['sources'], 'NASA source authorities differ')
        origin[label]={r['key']:r for r in mapping['reviews']}
        for name,context in old['source_bound_evidence'].items():contexts[label+':'+name]=context
    candidates={r['key']:r for r in baseline['requirements']}
    observations=[];cache={};source_equivalence=[]
    for mode in ('unattended','visible'):
        spec=plan['mode_reviews'][mode]
        mapping=prior.checked_json(spec['mapping'],spec['mapping_sha256'])
        need(mapping['sources']==baseline['sources'], 'Reviewed NASA source authorities differ')
        rows={r['key']:r for r in mapping['reviews']}
        need(set(rows)==KEYS and len(mapping['reviews'])==3, 'Review omits a transform row')
        observed=[];required_tests=set()
        for key,row in rows.items():
            blueprint=origin[BASE][key]
            need(row['fingerprint']==blueprint['fingerprint']==candidates[key]['fingerprint'],
                 'Transform source row identity changed')
            need(len(row['evidence'])==len(blueprint['evidence']) and row.get('rationale'),
                 'Transform review omits an original clause or rationale')
            checks=[]
            for claim,original in zip(row['evidence'],blueprint['evidence']):
                required_tests.add(claim['test'])
                stripped=lambda c:{k:v for k,v in c.items() if k not in ('source_sha256','reference_sources')}
                need(stripped(claim)==stripped(original), 'Transform assertion/scope/expected/facet changed')
                need(set(claim['reference_sources'])==set(original['reference_sources']),
                     'Transform reference path guard dropped or added')
                alias=spec['bindings'][claim['test']]
                need(alias in contexts, 'Receipt outside original declared visual collections')
                context=contexts[alias]
                receipt=prior.checked_json(context['receipt'],context['receipt_sha256'])
                manifest=load(receipt['snapshot_manifest']);identity=dict(manifest);sid=identity.pop('snapshot_id')
                need(snapshot.fingerprint(identity)==sid==receipt['snapshot_id'], 'Original snapshot identity changed')
                need(receipt.get('finished_utc') and receipt['snapshot_unchanged'], 'Original execution incomplete')
                need(receipt['mode']==('headless' if mode=='unattended' else 'visible'), 'Wrong actual execution mode')
                frozen=Path(manifest['root'])
                need(prior.nasa.inventory(frozen)['sources']==baseline['sources'], 'Frozen NASA authorities differ')
                need(all(receipt['source_hashes'].get(p)==h for p,h in claim['reference_sources'].items()),
                     'References do not describe the actual recorded source versions')
                result=next(r for r in receipt['results'] if r['name']==claim['test'])
                if claim['source']==SOURCE:
                    family=claim['test'].removeprefix('QtGui.NativeNasaVisual')
                    need(result['registered_command'][-1]==family,
                         'Source equivalence must match the actually executed family argument')
                    # The only execution containing the newer source is its
                    # original separately bound Load3DS run. It is source
                    # comparison evidence, never a transformation pass.
                    later_context=contexts[BASE+':20261027-visual-transform-3ds-visible']
                    later_receipt=prior.checked_json(later_context['receipt'],later_context['receipt_sha256'])
                    later_manifest=load(later_receipt['snapshot_manifest'])
                    later=Path(later_manifest['root'])/SOURCE
                    need(sha(later)==original['source_sha256'], 'Mapped later assertion source changed')
                    matrix_path='src/qtgui/tests/NasaVisualTransformCases.json'
                    later_matrix=Path(later_manifest['root'])/matrix_path
                    need(sha(later_matrix)==original['reference_sources'][matrix_path],
                         'Mapped transform input matrix changed')
                    selected=lambda data:next(c for c in data['cases'] if c['key']==family)
                    need(selected(load(frozen/matrix_path))==selected(load(later_matrix)),
                         'Executed transform inputs differ from the mapped family')
                    proof=applicable_source_equal(prior.payload(frozen/SOURCE).decode(),prior.payload(later).decode(),family)
                    token=(claim['test'],mode)
                    if token not in cache:source_equivalence.append(dict(test=claim['test'],mode=mode,**proof))
                else:
                    need(claim['source_sha256']==original['source_sha256'], 'Legacy assertion source changed')
                    # Keep every source-level assertion/terminal marker from
                    # the original clause; no scope fragment is substituted.
                    prior_claim=next(c for c in origin[LEGACY][key]['evidence'] if c['test']==claim['test'])
                    need(claim['source']==prior_claim['source'] and claim['source_sha256']==prior_claim['source_sha256'],
                         'Legacy original clause assertion identity differs')
                check=prior.verify_claim(frozen,manifest,receipt,result,claim)
                if claim['source']==SOURCE:
                    record=measured[(claim['test'],mode)]
                    proof=prior.nasa.proof_records(result)[0]
                    need(record['receipt_sha256']==context['receipt_sha256']
                         and record['log_sha256']==result['log_sha256']
                         and record['proof_sha256']==prior.digest(json.dumps(proof,sort_keys=True).encode()),
                         'Independent pixels do not bind this exact original execution proof')
                support=claim['test']=='QtGui.ModelDrawingRuntime'
                if support:
                    text=''.join(prior.payload(frozen/claim['source']).decode().split())
                    need(result.get('requested_visibility')=='headless'
                         and 'QGuiApplication::platformName()!="offscreen"' in text,
                         'Runtime support must retain its explicit offscreen execution guard')
                elif mode=='visible':need(check['native_exposure_events']>0, 'Visible transform lacks native exposure')
                cache[(claim['test'],mode)]=True
                checks.append(dict(check,receipt=context['receipt'],receipt_sha256=context['receipt_sha256'],
                                   original_context=alias,role='offscreen-runtime-support' if support else 'GUI-witness'))
            observed.append(dict(key=key,checks=checks))
        need(set(spec['bindings'])==required_tests, 'Visual composition omits or adds a mandatory test binding')
        observations.append(dict(cohort=mode,rows=observed,mapping=spec['mapping'],mapping_sha256=spec['mapping_sha256'],explicitly_composed_source_versioned=True))
    need(len(cache)==18 and len(source_equivalence)==12, 'Incomplete original transformation conjunction')
    need(all(d['decision']=='accept_versioned_scope' and d.get('rationale') and d.get('remaining_limits')
             for d in plan['decisions']), 'Incomplete substantive visual review')
    need(not KEYS&{r['key'] for r in union['requirements']['rows']}, 'Transform row already in baseline')
    return dict(version=BASE,decisions=plan['decisions'],cohorts=observations,
                accepted_source_versioned_scope_keys=dict(requirements=sorted(KEYS),historical_cases=[]),
                accepted_count=3,pending_count=0,new_test_executions=0,
                full_current_build_qualified=False,full_NASA_qualified=False,audit_type='visual-recovery',
                source_equivalence=source_equivalence,reused_contexts=len(cache),
                independent_pixel_review=plan['semantic_review'],
                limit='Three selected transformation capabilities from original source-versioned contexts. '
                      'No Load3DS repair is inferred for an earlier source; inactive-branch equivalence is explicit. '
                      'Arbitrary mesh/file/transform domains, OS/GPU/platform and current-build/NASA qualification remain open.')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    parser.add_argument('--plan',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();result=audit(args.repo,load(args.plan))
    result.update(plan_path=str(args.plan.resolve()),plan_sha256=sha(args.plan),auditor_sha256=sha(__file__),
                  original_claim_verifier_sha256=sha(prior.__file__))
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(accepted=3,reused_contexts=18,source_equivalence_checks=12,new_executions=0)))


if __name__=='__main__':main()
