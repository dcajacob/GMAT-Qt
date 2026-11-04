"""Audit bounded sampled harmonic-force evidence without admitting whole rows."""
import argparse
from collections import Counter
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
from xml.etree import ElementTree
from AuditPlotStepRotatingEvidence import check_case as rotating_case
from AuditPlotStepResetEvidence import invocation
from HarmonicGravityOracle import field_from_cof,force

NEW=['Rotating'+kind+'Gravity'+direction for kind in ('Origin','Earth','Velocity','Pair') for direction in ('Forward','Backward')]+['RotatingPairGravityThinned']
REGRESSIONS=['FrameFollowerForward','RotatingOriginForward','RotatingPairForward','LACConstraintForward']
PLOT_ROWS={'FRR!E329','FRR!E330','FRR!E633','FRR!E634'}
def check(ok,message):
    if not ok:raise ValueError(message)
def document(p):return json.loads(Path(p).read_text())
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def check_plot(proof,runtime,counts,potential_digest=None):
    check(proof['case']in NEW,'Unknown gravity case')
    ordinary=copy.deepcopy(proof);ordinary['case']=ordinary['case'].replace('Gravity','')
    rotating_case(ordinary,counts)
    check(len(runtime)==2,'Missing actual gravity providers')
    for r in runtime:
        check(r['force_type']=='GravityField'and r['degree']==r['order']==0 and abs(r['mu']-398600.4415)<1e-9,'Wrong force,degree,order,or Mu')
        check(Path(r['potential']).name=='JGM2.cof'and (potential_digest or sha(r['potential']))==r['potential_sha256'],'Potential identity differs')
        counts['actual_gravity_providers']+=1
def check_sampled(rows,model,degree,order,counts):
    offsets=[-10000.25,-3.125,17.5,10000.75]
    check(len(rows)==28 and [r['epoch_offset']for r in rows]==[t for t in offsets for _ in range(7)],'Wrong sampled epoch/grid matrix')
    for r in rows:
        check(r['clock_restored']is True and r['input_unchanged']is True and r['force_clock_seconds']==1234.5,'Force clock/input contract differs')
        check(abs((r['absolute_mjd']-21545)*86400-r['epoch_offset'])<1e-6,'Absolute sample epoch differs')
        rotation=r['rotation'];check(len(rotation)==3 and all(len(row)==3 for row in rotation),'Rotation shape differs')
        for i in range(3):
            for j in range(3):check(abs(math.fsum(rotation[i][k]*rotation[j][k]for k in range(3))-(i==j))<5e-13,'Sample rotation not orthogonal')
        check(all(len(r[k])==3 for k in ('local_position','native_acceleration','independent_acceleration','errors')),'Sampled component cardinality differs')
        expected=force(r['local_position'],model,degree,order,rotation)
        for observed,wanted,cpp in zip(r['native_acceleration'],expected,r['independent_acceleration']):
            check(math.isfinite(observed)and abs(observed-wanted)<1e-11 and abs(cpp-wanted)<1e-11,'Complex-step sampled gravity differs')
            counts['sampled_acceleration_components']+=1
        counts['sampled_force_queries']+=1
def custody(stage,name,version,counts,allow_suite_failure=False):
    maps=document(stage/(name+'-loaded-images.json'));build=document(stage/f'build-receipt-v{version}.json')
    check((maps['returncode']==0 or allow_suite_failure and maps['returncode']==1)and maps['inputs_unchanged']and maps['processes']and maps['runtime_binary_copies']==0,'Missing native image custody')
    check(build['status']=='passed'and build['returncode']==0 and build['inputs_unchanged']and maps['build_receipt_sha256']==sha(stage/f'build-receipt-v{version}.json'),'Build custody differs')
    check(maps['source_sha256']==build['source_sha256']==document(stage/f'executed-source-sha256-v{version}.json'),'Source versions differ')
    check(maps['binary_sha256']==build['binary_sha256'],'Build image identities differ')
    for process in maps['processes']:
        check(process['snapshots'],'Missing native maps')
        for snap in process['snapshots']:
            check(hashlib.sha256(snap['maps'].encode()).hexdigest()==snap['maps_sha256'],'Raw maps differ')
            for path,digest in snap['project_images'].items():check(maps['loaded_project_images'][path]==digest,'Mapped image partition differs')
    counts['native_processes']+=len(maps['processes'])
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--manifest',type=Path);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);args=p.parse_args()
    repo=args.repo.resolve();stage=args.stage.resolve();counts=Counter()
    if args.manifest:
        manifest=document(args.manifest)
        for path,digest in manifest['artifact_sha256'].items():check(sha(repo/path)==digest,'Sealed artifact changed '+path)
        with tarfile.open(repo/manifest['final_review_source_archive'])as archive:
            check(set(archive.getnames())==set(manifest['final_source_sha256']),'Final reviewed source inventory differs')
            for path,digest in manifest['final_source_sha256'].items():check(hashlib.sha256(archive.extractfile(path).read()).hexdigest()==digest,'Final preserved source changed '+path)
        check(manifest['auditor_sha256']==sha(Path(__file__)),'Sealed auditor changed')
        counts['artifact_bindings']=len(manifest['artifact_sha256'])
    baseline=repo/'build/nasa-coverage/coverage95-20261103-matlab-c-interface';ledger=document(baseline/'coverage-snapshot-v1.json');receipt=document(baseline/'stage-receipt-v1.json')
    check(sha(baseline/'coverage-snapshot-v1.json')=='4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6','Canonical ledger changed')
    check(sha(baseline/'stage-receipt-v1.json')=='0ac382c38695626484e02f75ef1fb92539b82aac23affd2af013925295e81314'and sha(baseline/'evidence-manifest-v1.json')=='ed127c5db19b63578dfe5292d1dfbb27ed614527d041d146d1be41b4c2ab37c4','Sealed canonical prefix changed')
    check(ledger['coverage']['requirements']['verified']==1050 and ledger['coverage']['requirements']['total']==1158 and ledger['coverage']['historical_cases']['verified']==1816,'Coverage changed')
    check(len(receipt['original_hold_keys_retained'])==59,'Original hold identities lost')
    # Capsule contents, rather than current mutable test sources, bind execution.
    for version in range(1,10):
        identities=document(stage/f'executed-source-sha256-v{version}.json')
        with tarfile.open(stage/f'executed-sources-v{version}.tar.gz')as archive:
            check(set(archive.getnames())==set(identities),'Capsule inventory differs')
            for path,digest in identities.items():check(hashlib.sha256(archive.extractfile(path).read()).hexdigest()==digest,'Archived source differs '+path)
        counts['source_capsules']+=1
    old=repo/'build/nasa-coverage/coverage95-20261103-plot-frame-dependencies';old_manifest=document(old/'evidence-manifest-v1.json')['artifact_sha256']
    potential_digest=sha(stage/'harmonic-headless-v1/QtGui.NasaHarmonicGravityEarth/JGM2.cof')
    for mode in ('headless','visible'):
        root=stage/f'final-{mode}-v1';case=root/'QtGui.PlotIntegrationSteps';custody(stage,root.name,2,counts);invocation(root,mode,case.name,counts)
        proof=document(case/'proof.json');logged=[json.loads(line.split('NASA_CHECK ',1)[1])for line in(case/'test.log').read_text().splitlines()if'NASA_CHECK 'in line]
        check(proof['cases']==logged and proof['new_requirement_credit']==0 and {p['case']for p in logged}==set(NEW+REGRESSIONS),'Native plot case inventory differs')
        for row in logged:
            name=row['case'];check(document(case/(name+'-proof.json'))==row and document(case/(name+'-points.json'))==row['outputs'],'Raw plot/proof/log differs')
            if name in NEW:
                check_plot(row,document(case/(name+'-gravity-runtime.json')),counts,potential_digest)
                warnings=2 if'Pair'in name and'Thinned'not in name else 0
                check((case/(name+'-run-messages.txt')).read_text().count("has no coherent state for frame 'RotatingFrame'")==warnings,'Missing frame warnings differ')
                counts['new_plot_contexts']+=1
            else:
                prior=old/(('headless-v4'if mode=='headless'else'visible-v1')if name.startswith('LAC')else'regressions-'+mode+'-v1')/case.name/(name+'-proof.json')
                check(sha(prior)==old_manifest[str(prior.relative_to(repo))]and document(prior)['outputs']==row['outputs'],'Prior plot regression changed '+name)
                if name=='RotatingOriginForward':check(row['rotating_context_checks']==['coeval-generic-force-preserved','stale-epoch-force-rejected','same-epoch-changed-state-force-rejected'],'Generic unknown-force guards changed')
                counts['reused_equal_regression_contexts']+=1
        root=stage/f'harmonic-{mode}-v1';custody(stage,root.name,5,counts)
        for body in ('Earth','Ceres'):
            name='QtGui.NasaHarmonicGravity'+body;case=root/name;invocation(root,mode,name,counts)
            proof=document(case/'proof.json');check(proof['outcome']=='passed'and proof['trial_count']==10 and len(proof['observations'])==10,'Harmonic original proof differs')
            check(document(case/'independent.json')['missions']==10,'Original complex-step/DP5 audit missing')
            models={False:field_from_cof(case/'synthetic.cof')}
            if body=='Earth':models[True]=field_from_cof(case/'JGM2.cof')
            for trial in proof['observations']:
                check_sampled(document(case/(trial['trial']+'-sampled-gravity.json')),models[trial['shipped_model']],trial['degree'],trial['order'],counts)
                counts['sampled_clock_exception_recoveries']+=1
            counts['harmonic_GUI_contexts']+=1
        # Cache repair affects the sampled force entry. Keep the earlier passes
        # and compare final-clock executions with their exact earlier outputs.
        current=stage/('final-harmonic-headless-v3'if mode=='headless'else'final-harmonic-visible-v2');custody(stage,current.name,9,counts)
        for body in ('Earth','Ceres'):
            name='QtGui.NasaHarmonicGravity'+body;case=current/name;invocation(current,mode,name,counts)
            previous=stage/f'harmonic-{mode}-v1'/name
            proof=document(case/'proof.json');check(proof==document(previous/'proof.json'),'Final ordinary harmonic proof changed')
            models={False:field_from_cof(case/'synthetic.cof')}
            if body=='Earth':models[True]=field_from_cof(case/'JGM2.cof')
            for trial in proof['observations']:
                filename=trial['trial']+'-sampled-gravity.json';rows=document(case/filename)
                check(rows==document(previous/filename),'Untided sampled force changed after cache repair')
                check_sampled(rows,models[trial['shipped_model']],trial['degree'],trial['order'],counts)
            counts['final_harmonic_GUI_contexts']+=1
            if body=='Earth':
                tides=document(case/'sampled-tides.json');check(len(tides)==6,'Tidal matrix incomplete')
                check([(r['tide_model'],r['epoch_offset'])for r in tides]==[(model,t)for model in ('Solid','SolidAndPole')for t in (-172800.25,.125,345600.75)],'Tidal model/epoch inventory differs')
                for r in tides:
                    check(r['stale_cached_mjd']==21546 and abs((r['sample_epoch_mjd']-21545)*86400-r['epoch_offset'])<1e-6,'Tidal sample/cache epochs differ')
                    check(r['force_clock_restored']is True and r['tide_clock_restored']is True,'Tidal clocks not restored')
                    check(all(len(r[k])==3 for k in ('sampled_acceleration','coeval_reference_acceleration','untided_acceleration','errors')),'Tidal component inventory differs')
                    errors=[abs(a-b)for a,b in zip(r['sampled_acceleration'],r['coeval_reference_acceleration'])]
                    effect=max(abs(a-b)for a,b in zip(r['coeval_reference_acceleration'],r['untided_acceleration']))
                    check(errors==r['errors']and max(errors)==r['maximum_error']<5e-15 and effect==r['tidal_effect']>1e-12,'Tidal clock coherence differs')
                    counts['tidal_clock_contracts']+=1
        current=stage/f'cache-plot-{mode}-v1';case=current/'QtGui.PlotIntegrationSteps';custody(stage,current.name,9,counts);invocation(current,mode,case.name,counts)
        proof=document(case/'RotatingVelocityGravityForward-proof.json');previous=stage/f'final-{mode}-v1'/case.name/'RotatingVelocityGravityForward-proof.json'
        check(proof==document(previous),'Affected rotating velocity plot changed after cache repair')
        check_plot(proof,document(case/'RotatingVelocityGravityForward-gravity-runtime.json'),Counter(),potential_digest);counts['final_cache_plot_contexts']+=1
    custody(stage,'support-headless-v3',4,counts,allow_suite_failure=True)
    invocation(stage/'support-headless-v3','headless','QtGui.ObjectAccelerationDerivativeContract',counts)
    tests=ElementTree.parse(stage/'support-ctest-v1.xml').findall('.//testcase')
    check({t.get('name')for t in tests}=={'QtAcceptance.AccelerationContextForce','QtAcceptance.AccelerationContextLifetime','QtAcceptance.AccelerationHandoff'}and all(t.find(tag)is None for t in tests for tag in('failure','error','skipped')),'Core acceleration CLI support differs')
    counts['CLI_support_cases']=len(tests)
    custody(stage,'support-cli-v1',5,counts)
    for name in ('before-headless-v1','before-visible-v1'):
        root=stage/name;case=root/'QtGui.PlotIntegrationSteps';r=document(case/'result.json');check(r['status']=='failed'and r['returncode']!=0,'Original failure relabeled')
        check('Sampled object-frame acceleration is unavailable for force GravityField'in(case/'RotatingOriginGravityForward-messages.txt').read_text(),'Original force failure missing');counts['native_failures_retained']+=1
    for name in ('support-headless-v3','support-visible-before-v1'):
        for body in ('Earth','Ceres'):
            r=document(stage/name/('QtGui.NasaHarmonicGravity'+body)/'result.json');check(r['status']=='failed'and r['returncode']!=0,'Probe failure relabeled');counts['native_failures_retained']+=1
    for name in ('tides-before-headless-v1','tides-before-visible-v1'):
        case=stage/name/'QtGui.NasaHarmonicGravityEarth';r=document(case/'result.json');observed=document(case/'sampled-tides.json')
        check(r['status']=='failed'and r['returncode']!=0 and len(observed)==1 and abs(observed[0]['maximum_error']-9.598031223967407e-11)<1e-25,'Original stale-tide failure differs');counts['native_failures_retained']+=1
    for name in ('support-headless-v1','support-headless-v2','final-harmonic-headless-v2'):
        r=document(stage/(name+'-loaded-images.json'));check(r['returncode']!=0 and r['inputs_unchanged']and not r['processes'],'Preflight misclassified');counts['preflight_failures_retained']+=1
    build=document(stage/'build-receipt-v6.json');check(build['status']=='failed'and build['returncode']!=0 and build['inputs_unchanged']and not build['binary_sha256'],'Failed build misclassified');counts['failed_builds_retained']=1
    remaining=document(stage/'remaining-requirement-audit-v2.json');original=ledger['remaining_original_rows']['requirements']
    check(len(remaining['rows'])==108 and {r['key']:r['fingerprint']for r in remaining['rows']}=={r['key']:r['fingerprint']for r in original},'Remaining exact obligations changed')
    check(remaining['original_hold_keys']==receipt['original_hold_keys_retained']and all(r['current_increment']['new_requirement_credit']==0 and r['current_increment']['whole_row_qualified']is False for r in remaining['rows']),'Hold or whole-row qualification changed')
    # Independently damaged observations must fail even when their shape survives.
    root=stage/'final-headless-v1/QtGui.PlotIntegrationSteps';base=document(root/(NEW[0]+'-proof.json'));runtime=document(root/(NEW[0]+'-gravity-runtime.json'))
    def reject(callback):
        try:callback()
        except(ValueError,KeyError):counts['damaged_proofs_rejected']+=1;return
        raise ValueError('Damaged proof accepted')
    for key,delta in (('x',.01),('elapsed',1)):
        bad=copy.deepcopy(base);bad['outputs'][0]['points'][0][key]+=delta;reject(lambda:check_plot(bad,runtime,Counter(),potential_digest))
    bad=copy.deepcopy(base);bad['outputs'][0]['points'][0]['velocity'][0]+=.001;reject(lambda:check_plot(bad,runtime,Counter(),potential_digest))
    bad=copy.deepcopy(base);bad['outputs'][0]['points'][0]['connect']=True;reject(lambda:check_plot(bad,runtime,Counter(),potential_digest))
    bad=copy.deepcopy(runtime);bad[0]['degree']=2;reject(lambda:check_plot(base,bad,Counter(),potential_digest))
    case=stage/'harmonic-headless-v1/QtGui.NasaHarmonicGravityEarth';rows=document(case/'Sectoral-sampled-gravity.json');field=field_from_cof(case/'synthetic.cof')
    for key in ('native_acceleration','clock_restored'):
        bad=copy.deepcopy(rows)
        if key=='native_acceleration':bad[0][key][0]+=.001
        else:bad[0][key]=False
        reject(lambda:check_sampled(bad,field,2,2,Counter()))
    result=dict(status='passed',new_requirement_credit=0,coverage=ledger['coverage'],requirements_still_needed=51,counts=dict(counts),original_hold_keys=receipt['original_hold_keys_retained'],whole_plot_rows_still_unqualified=sorted(PLOT_ROWS),auditor_sha256=sha(Path(__file__)),manifest_sha256=sha(args.manifest)if args.manifest else None,independent_tidal_physics_qualified=False,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)
    args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k not in ['original_hold_keys','coverage']}))
if __name__=='__main__':main()
