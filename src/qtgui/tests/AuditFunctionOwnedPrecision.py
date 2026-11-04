"""Audit the Code500 increment, original scopes and temporal runtime-link custody."""
import argparse
from collections import Counter
import copy
from decimal import Decimal
import hashlib
import json
from pathlib import Path
import tarfile
from xml.etree import ElementTree

STAGE='build/nasa-coverage/coverage95-20261104-function-owned-precision'
PRIOR='build/nasa-coverage/coverage95-20261104-function-owned-type-matrix'
ENV='build/nasa-coverage/coverage95-20261103-function-environment-bindings'
CANON='build/nasa-coverage/coverage95-20261103-matlab-c-interface'
LEDGER='4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6'

def check(ok,message):
    if not ok:raise ValueError(message)
def load(p):return json.loads(Path(p).read_text())
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def module_from_archive(archive,name):
    with tarfile.open(archive)as tar:source=tar.extractfile(name).read()
    namespace={'__name__':'frozen_contract','__file__':name}
    exec(compile(source,name,'exec'),namespace)
    return namespace

def maps(stage,run,version,counts):
    row=load(stage/(run+'-loaded-images.json'));receipt=stage/f'build-receipt-v{version}.json';build=load(receipt)
    check(row['inputs_unchanged']and row['runtime_binary_copies']==0 and row['processes']and
          row['build_receipt_sha256']==sha(receipt)and row['source_sha256']==build['source_sha256']and
          row['binary_sha256']==build['binary_sha256'],'GUI execution custody differs')
    for process in row['processes']:
        check(any(s['project_images'].get(process['executable'])==build['binary_sha256'][process['executable']]
                  for s in process['snapshots']),'Actual executable image absent')
        for snapshot in process['snapshots']:
            check(hashlib.sha256(snapshot['maps'].encode()).hexdigest()==snapshot['maps_sha256'],'Raw maps changed')
            for name,digest in snapshot['project_images'].items():
                check(row['loaded_project_images'][name]==digest,'Loaded partition differs')
    counts['mapped_gui_processes']+=len(row['processes'])

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path.cwd());p.add_argument('--stage',type=Path);p.add_argument('--manifest',type=Path);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    repo=args.repo.resolve();stage=args.stage.resolve()if args.stage else repo/STAGE;counts=Counter()
    if args.manifest:
        seal=load(args.manifest);check(sha(__file__)==seal['auditor_sha256'],'Auditor identity differs')
        for name,digest in seal['regular_artifact_sha256'].items():
            check(not(repo/name).is_symlink()and sha(repo/name)==digest,'Immutable artifact changed: '+name)
        for name,target in seal['symbolic_links'].items():
            check((repo/name).is_symlink()and str((repo/name).readlink())==target,'Runtime alias route changed: '+name)
        with tarfile.open(repo/seal['final_source_archive'])as tar:
            check(set(tar.getnames())==set(seal['final_source_sha256']),'Final source set differs')
            for name,digest in seal['final_source_sha256'].items():
                check(hashlib.sha256(tar.extractfile(name).read()).hexdigest()==digest,'Final frozen source changed')
        counts['immutable_artifact_bindings']=len(seal['regular_artifact_sha256']);counts['temporal_alias_routes']=len(seal['symbolic_links'])
        final_archive=repo/seal['final_source_archive']
    else:final_archive=None
    check(sha(repo/CANON/'coverage-snapshot-v1.json')==LEDGER,'Canonical denominator/fingerprints changed')
    ledger=load(repo/CANON/'coverage-snapshot-v1.json')
    check(ledger['coverage']['requirements']['verified']==1050 and ledger['coverage']['requirements']['total']==1158 and ledger['coverage']['historical_cases']['verified']==1816,'Canonical coverage changed')
    prior=repo/PRIOR;old_seal=load(prior/'evidence-manifest-v1.json')
    check(sha(prior/'evidence-manifest-v1.json')=='d862d7ddb1b64cbefa4af4d2427d98658df12726d12983e5baebda25ea478fa7'and
          sha(prior/'evidence-audit-v3.json')=='ef016d572547fc1c02a54f644d2f365fd63a261ec224d45213db406010411926','Original owned prefix changed')
    if final_archive:
        alias_module=module_from_archive(final_archive,'src/qtgui/tests/AuditTemporalArtifactLinks.py')
    else:
        ns={'__name__':'alias_contract'};exec(compile((repo/'src/qtgui/tests/AuditTemporalArtifactLinks.py').read_bytes(),'alias_contract','exec'),ns);alias_module=ns
    for folder,manifest,receipt in [(PRIOR,'evidence-manifest-v1.json','prior-owned-link-custody-v1.json'),(ENV,'evidence-manifest-v2.json','prior-environment-link-custody-v1.json')]:
        verified=alias_module['verify_legacy_manifest'](repo,repo/folder/manifest)
        check(verified==load(stage/receipt),'Legacy immutable/temporal link inventory differs')
        counts['retained_regular_artifact_bindings']+=verified['regular_artifact_bindings']
        counts['retained_temporal_aliases']+=verified['symbolic_aliases']
        counts['historical_mapped_alias_hashes']+=verified['aliases_with_historical_loaded_image_hash']
    old=module_from_archive(repo/old_seal['final_source_archive'],'src/qtgui/tests/AuditFunctionOwnedTypes.py')
    retained_maps=Counter()
    for run in ['owned-context-headless-v2','owned-context-visible-v2','original-regressions-headless-v1','original-regressions-visible-v1']:
        maps(prior,run,4,retained_maps)
    counts['retained_mapped_processes']=retained_maps['mapped_gui_processes']
    # Reuse the actual old contracts on the preserved 56 selected proofs and
    # 30 original regressions. This is independent contract revalidation, not
    # a claim that the dereferencing legacy manifest now replays literally.
    for selected in load(prior/'selected-evidence-v1.json'):
        root=old['invocation'](prior,selected['run'],'QtGui.NasaFunctionOwnedType'+selected['case'],selected['mode'],Counter())
        check(sha(root/'proof.json')==selected['proof_sha256'],'Retained selected proof changed')
        spec=next(c for c in load(root/'executed-catalog.json')['cases']if c['key']==selected['case'])
        old['contract'](load(root/'proof.json'),spec,Counter());counts['retained_owned_contract_contexts']+=1
    for reg in load(prior/'stage-receipt-v1.json')['regressions']:
        old['invocation'](prior,reg['run'],reg['test'],reg['mode'],Counter());counts['retained_original_contexts']+=1
    receipt=load(stage/'stage-receipt-v2.json')
    processes=load(stage/receipt['process_audit'])
    check(processes['matching_native_processes']==[]and processes['matching_visible_servers']==[]and
          processes['gui_execution_runs']==len(receipt['gui_execution_versions']),
          'Recorded terminal process audit differs')
    check(len(receipt['original_hold_keys'])==len(set(receipt['original_hold_keys']))==59 and
          receipt['original_hold_keys']==load(prior/'stage-receipt-v1.json')['original_hold_keys'],'Original holds changed')
    catalogs={}
    for version in range(1,13):
        identities=load(stage/f'executed-source-sha256-v{version}.json');build=load(stage/f'build-receipt-v{version}.json')
        check(build['status']==('failed'if version==11 else'passed')and build['inputs_unchanged']and build['source_sha256']==identities,'Build/source history differs')
        if version==11:check(build['returncode']!=0 and not build['binary_sha256'],'Failed build authorized an image')
        with tarfile.open(stage/f'executed-sources-v{version}.tar.gz')as tar:
            check(set(tar.getnames())==set(identities),'Execution source set differs')
            for name,digest in identities.items():check(hashlib.sha256(tar.extractfile(name).read()).hexdigest()==digest,'Execution source changed')
            catalogs[version]=json.loads(tar.extractfile('src/qtgui/tests/NasaFunctionOwnedTypeCases.json').read())
        counts['source_capsules']+=1
    check(all(catalogs[v]==catalogs[1]for v in catalogs),'Original complete catalog/bounds changed')
    old_matrix=load(prior/'owned-type-binding-matrix-v1.json');new_matrix=load(stage/'precision-owned-type-matrix-v1.json')
    check(len(new_matrix['rows'])==566,'Complete factory matrix changed')
    for a,b in zip(old_matrix['rows'],new_matrix['rows']):check(all(b[k]==v for k,v in a.items()),'Original factory role/eligibility changed')
    previous=load(prior/'remaining-requirement-audit-v1.json');remaining=load(stage/'remaining-requirement-audit-v1.json')
    check(len(remaining['rows'])==len(previous['rows'])==108 and remaining['original_hold_keys']==previous['original_hold_keys'],'Exact requirement inventory changed')
    for a,b in zip(previous['rows'],remaining['rows']):check(all(b[k]==v for k,v in a.items()),'Exact fingerprint/gap/conjunction changed')
    for run,version in receipt['gui_execution_versions'].items():maps(stage,run,version,counts)
    # Extend the frozen contract only with the now-executed Code500 UTC affine
    # oracle. All scope, raw Report, phase, prototype and owner checks survive.
    old_expected=old['expected_state']
    def precision_expected(sample,spec,root):
        if spec['factory_type']!='Code500':return old_expected(sample,spec,root)
        request=sample['oracle_inputs'];inside=sample['inside_function'];t=request['seconds_gt']
        check(request['file_time_system']=='UTC'and abs(float((Decimal(request['query_gt'])-Decimal(request['start_gt']))*86400)-t)<1e-10,'Code500 precise time input differs')
        check(sample['resource']['executed']['has_precision_time']is False and sample['resource']['owner_has_precision_time']is False,'Original default Real clock scenario changed')
        r=[7000,-200,450]if inside else[9000,100,300];v=[.2,6.8,-.3]if inside else[-.1,7.1,.4]
        return[r[i]+v[i]*t for i in range(3)]+v
    old['expected_state']=precision_expected;old['OPEN_TYPES']=old['OPEN_TYPES']-{'Code500'}
    spec=next(c for c in catalogs[10]['cases']if c['key']=='Code500');proofs=[]
    for mode in ['headless','visible']:
        run='code500-affected-'+mode+'-v1';root=old['invocation'](stage,run,'QtGui.NasaFunctionOwnedTypeCode500',mode,counts);proof=load(root/'proof.json');old['contract'](proof,spec,counts)
        check(load(root/'reopened-prototypes.json')==load(root/'prototypes-before.json'),'Exact reopened prototype equality changed')
        for phase in range(3):
            check(all(load(root/f'{phase}-invariants.json').values()),'Source/history/prototype/disk invariant differs')
            check([float(x)for x in(root/f'{phase}-values.txt').read_text().split()]==proof['phases'][phase]['raw_values'],'Raw Report bytes differ')
        proofs.append(proof)
        for name in receipt['regression_names']:old['invocation'](stage,run,name,mode,counts)
    check(counts['passed_native_contexts']==14,'New/affected GUI census differs')
    support=stage/receipt['support_run'];execution=load(support/'execution.json');build=load(stage/f"build-receipt-v{receipt['support_source_version']}.json")
    check(build['binary_sha256']==load(stage/'build-receipt-v10.json')['binary_sha256'],'Registration-only change altered selected native images')
    check(execution['returncode']==0 and execution['inputs_unchanged']and execution['source_sha256']==build['source_sha256']and execution['binary_sha256']==build['binary_sha256']and execution['runtime_binary_copies']==0,'Registered support custody differs')
    for process in execution['processes']:
        check(any(s['project_images'].get(process['executable'])==build['binary_sha256'][process['executable']]for s in process['snapshots']),'Registered native image absent')
        for s in process['snapshots']:check(hashlib.sha256(s['maps'].encode()).hexdigest()==s['maps_sha256'],'Registered raw maps changed')
    tests=ElementTree.parse(support/'ctest.xml').findall('.//testcase')
    check(len(tests)==1 and tests[0].get('name')=='QtAcceptance.Code500EpochInterpolation'and all(tests[0].find(k)is None for k in ['error','failure','skipped']),'Registered native-plus-oracle JUnit differs')
    spline=module_from_archive(stage/f"executed-sources-v{receipt['support_source_version']}.tar.gz",'src/qtgui/tests/AuditCode500EpochInterpolation.py')
    support_proof=load(support/'proof.json');independent=spline['audit'](support_proof)
    check(independent==load(support/'independent-audit.json')==load(stage/'code500-contract-audit-v3.json'),'Independent 16-equation spline replay differs')
    counts.update({'support_'+k:v for k,v in independent['counts'].items()})
    # Deliberate damage must fail actual contracts, not merely hash checks.
    mutations=[lambda p:p['phases'][0]['live_samples'].pop(),lambda p:p['phases'][0]['live_samples'][1].update(shared_pointer='0'),lambda p:p['phases'][0]['live_samples'][1]['resource'].update(spacecraft_j2000_body='0'),lambda p:p['phases'][0]['live_samples'][1]['actual'].__setitem__(0,123.),lambda p:p.update(whole_row_qualified=True)]
    for mutate in mutations:
        bad=copy.deepcopy(proofs[0]);mutate(bad);rejected=False
        try:old['contract'](bad,spec,Counter())
        except(ValueError,IndexError,KeyError):rejected=True
        check(rejected,'Damaged function proof escaped');counts['damaged_proofs_rejected']+=1
    support_mutations=[lambda p:p['observations'][0]['actual'].__setitem__(0,0.),lambda p:p['observations'][0]['spline_inputs'][1].update(x_seconds=31.),lambda p:p['files'].pop(),lambda p:p.update(whole_row_qualified=True)]
    for mutate in support_mutations:
        bad=copy.deepcopy(support_proof);mutate(bad);rejected=False
        try:spline['audit'](bad)
        except(ValueError,IndexError,KeyError):rejected=True
        check(rejected,'Damaged spline proof escaped');counts['damaged_proofs_rejected']+=1
    core=load(stage/'core-increment-v1.json');check(core['pending_core_paths']==267 and core['prior_paths_unchanged']==263 and core['new_pending_paths']==4 and len(core['increments'])==4,'Exact core increment scope differs')
    for change in core['increments']:
        check(sha(repo/change['before'])==change['before_sha256']and sha(repo/change['after'])==change['after_sha256']and load(stage/'executed-source-sha256-v10.json')[change['path']]==change['after_sha256'],'Exact core pre/postimage differs')
    check(sha(stage/'core-increment-v1.patch')==core['patch_sha256'],'Core patch changed')
    check(receipt['remaining_owned_types']==['CCSDS-OEM','STK','Topocentric']and receipt['new_requirement_credit']==0 and receipt['whole_row_qualified']is False,'Unresolved scope inflated')
    result=dict(status='passed-bounded-code500-precision-audit',counts=dict(counts),canonical_verified=1050,original_total=1158,target=1101,still_needed=51,open_exact_rows=108,original_holds=59,new_requirement_credit=0,whole_row_qualified=False,collective_owned_types=33,total_owned_types=36,remaining_owned_types=receipt['remaining_owned_types'],literal_legacy_manifest_replay_after_rebuild=False,scientific_technical_current_build_NASA_qualified=False)
    check(not args.output.exists(),'Never overwrite an audit');args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

if __name__=='__main__':main()
