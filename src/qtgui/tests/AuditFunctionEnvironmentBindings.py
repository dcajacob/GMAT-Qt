"""Audit versioned environment Parameter-owner evidence; assign no whole-row credit."""
import argparse
from collections import Counter
import copy
import hashlib
import json
import math
from pathlib import Path
import tarfile
from xml.etree import ElementTree

PREFIX='QtGui.NasaFunctionEnvironmentBinding'
STAGE='build/nasa-coverage/coverage95-20261103-function-environment-bindings'
CANON='build/nasa-coverage/coverage95-20261103-matlab-c-interface'
LEDGER_SHA='4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6'
REGRESSIONS=['QtGui.NasaFunctionRemainingResourceForceModel','QtGui.NasaFunctionRemainingResourceODEModel','QtGui.NasaFunctionMathResourceDiag','QtGui.NativeNasaFunctionPropagatorGlobalExisting','QtGui.NativeNasaFunctionPropagatorGlobalCreated','QtGui.NativeNasaFilterFormatFilterOem','QtGui.Covariance','QtGui.NasaFunctionTimeParameterBindingA1ModJulian','QtGui.NasaFunctionTimeParameterBindingUTCGregorian']

def check(value,message):
    if not value:raise ValueError(message)
def load(path):return json.loads(Path(path).read_text())
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def compare(actual,expected,bound,counts):
    if isinstance(expected,list):
        check(isinstance(actual,list)and len(actual)==len(expected),'Numeric shape differs')
        for a,e in zip(actual,expected):compare(a,e,bound,counts)
    else:
        check(isinstance(actual,(int,float))and math.isfinite(actual)and abs(actual-expected)<=bound,'Independent numeric result differs')
        counts['independent_components']+=1

def contract(proof,spec,oracle,counts):
    kind=spec['factory_type'];meta=spec['metadata']
    check(proof['checkpoint']=='live-function-environment-parameter-owner-binding'and proof['case']==kind and proof['outcome']=='passed'and proof['argument']==kind+'Read','Proof identity differs')
    check(proof['family']=='Environment'and proof['full_system_parameter_inventory']==321 and set(proof['initial_batch_types'])==set(oracle['TYPES']),'Factory universe differs')
    check(proof['missions']==3 and proof['function_calls']==proof['live_callback_samples']==12 and proof['actual_parameter_owner_checks']==24,'Mission/owner census differs')
    check(proof['file_decisions']==[dict(saved=False),dict(saved=True)]and not proof['writable_probe']and not proof['report_via_scalar_variables'],'File decisions or read route differs')
    check([p['phase']for p in proof['phases']]==[0,1,2]and[p['explicit_rebuild']for p in proof['phases']]==[True,True,False],'Reopen/no-Build phase differs')
    for phase in proof['phases']:
        check(phase['raw_markers']==[42,43,44,103,104]and phase['callback_private_maps_accessed_only_while_executing'],'Caller/global/private markers differ')
        samples=phase['live_samples'];check(len(samples)==len(phase['private_writes'])==len(phase['shared_writes'])==4,'Publication/write census differs')
        roots={}
        for calls,sample in enumerate(samples):
            inside=calls>0;target='ScopePulse'if inside else'MainPulse'
            check(sample['calls']==calls and sample['inside_function']==inside and sample['function']==('ReadShared'if inside else'')and sample['global_map_is_value_snapshot']==(not inside),'Callback scope/order differs')
            check(sample['provider']==f'Propagate PulseProp({target}) {{{target}.ElapsedSecs = 1}};'and len(sample['parameters'])==2,'Actual publication provider differs')
            for shared,row in enumerate(sample['parameters']):
                scope='shared'if shared else'private'if inside else'root'
                owner='SharedSC'if shared else'PrivateOnly';bound=oracle['bound'](kind,0)
                check(row['factory_type']==kind and row['native_type']==meta['native_type']and row['return_type']==meta['return_type']and row['owner_type']==meta['owner_type_id'],'Native factory metadata differs')
                check(row['owner_name']==owner and row['shared']==bool(shared)and row['owner_global']==bool(shared)and row['owner_local']==(inside and not shared)and row['parameter_global']==bool(shared),'Native scope flags differ')
                check(row['reference_owner_pointer']==row['live_owner_pointer']!='0'and row['live_owner_pointer']!=row['configured_owner_pointer'],'Parameter owner identity differs')
                if not inside:roots[shared]=row['live_owner_pointer']
                check(row['root_owner_pointer']==roots[shared]and(row['live_owner_pointer']==roots[shared])==(bool(shared)or not inside),'Private/root/Global lifetime differs')
                check(row['runtime_settable']==row['info_settable']==meta['settable']and row['runtime_reportable']==meta['reportable']and row['runtime_plottable']==meta['plottable'],'Advertised metadata was reclassified')
                check(row['legacy_unit']==row['factory_prototype_unit'],'Unit metadata differs')
                check(row['frame_pointer']==row['expected_frame_pointer']!='0'and row['origin_pointer']==row['expected_origin_pointer']!='0'and row['frame_name']=='EarthMJ2000Eq'and row['frame_axes']=='MJ2000Eq','Frame/origin binding differs')
                if inside:check(row['function_solar_system_pointer']==row['solar_system_pointer']!='0','Function SolarSystem differs')
                if spec['dependency_kind']=='OdeModel':check(row['force_model_reference_pointer']==row['expected_force_model_pointer']!='0','ForceModel binding differs')
                if meta['origin_dependent']:check(row['parameter_origin_pointer']==row['origin_pointer'],'Origin-dependent Parameter differs')
                check(abs(row['owner_epoch']-21545)<2e-10 and abs(row['owner_gmat_time_mjd']-21545)<2e-10 and row['origin_mu']==oracle['MU'],'Epoch/Mu differs')
                frozen=row['independent_oracle'];request=frozen['input']
                check(request['type']==kind and request['scope']==scope and request['calls']==calls and frozen['independent_orientation_ephemeris_qualified']is False,'Oracle inputs or scope differ')
                expected=oracle['expected'](kind,scope,calls,request['inputs'])
                if kind=='OrbitErrorCovariance':expected=[x for r in expected for x in r]
                check(row['absolute_bound']==frozen['absolute_bound']==bound,'Original numeric bound differs')
                compare(row['actual'],expected,bound,counts);compare(row['expected'],expected,0,Counter());compare(frozen['expected'],expected,0,Counter())
                compare(phase['shared_writes'if shared else'private_writes'][calls],row['actual'],bound,Counter())
                expected_state=oracle['state'](scope,calls)
                for i in range(6):
                    compare(row['reference_state'][i],expected_state[i],1e-6 if i<3 else 1e-9,Counter())
                    compare(row['inertial_state'][i],expected_state[i],1e-6 if i<3 else 1e-9,Counter())
                check(row['shape']==([]if meta['return_type']==4 else spec['shape']),'Dynamic vector/matrix shape differs')
                counts['actual_parameter_owner_checks']+=1
            counts['live_callbacks']+=1
        counts['missions']+=1

def invocation(stage,run,test,mode,counts):
    root=stage/run/test;r=load(root/'result.json')
    check(r['status']=='passed'and r['returncode']==0 and r['requested_visibility']==mode,'Native result/mode differs')
    tests=ElementTree.parse(root/'ctest.xml').findall('.//testcase')
    check(len(tests)==1 and tests[0].get('name')==test and all(tests[0].find(k)is None for k in ['error','failure','skipped']),'JUnit differs')
    check(not r['trace']['screenshots'],'Unexpected screenshot')
    if mode=='visible':
        d=load(stage/run/'results.json')['visible_display']
        check(d['resizable']and d['start_minimized']and'Iconic'in d['initial_host_window_state']and'FULLSCREEN'not in d['initial_host_window_state'],'Visible desktop policy differs')
        check(r['trace']['recorded_platforms']==['xcb']and r['trace']['native_exposure_events']>0,'Actual native exposure absent')
        counts['visible_exposures']+=r['trace']['native_exposure_events']
    counts['passed_native_contexts']+=1
    return root

def custody(stage,run,version,counts):
    record=load(stage/(run+'-loaded-images.json'));build=load(stage/f'build-receipt-v{version}.json')
    check(record['inputs_unchanged']and record['processes']and record['runtime_binary_copies']==0,'Loaded process custody absent')
    check(build['status']=='passed'and build['returncode']==0 and build['inputs_unchanged']and record['build_receipt_sha256']==sha(stage/f'build-receipt-v{version}.json'),'Successful matching build absent')
    check(record['source_sha256']==build['source_sha256']==load(stage/f'executed-source-sha256-v{version}.json')and record['binary_sha256']==build['binary_sha256'],'Source/binary version differs')
    for process in record['processes']:
        check(process['snapshots'],'Raw native maps absent')
        for snapshot in process['snapshots']:
            check(hashlib.sha256(snapshot['maps'].encode()).hexdigest()==snapshot['maps_sha256'],'Raw native maps changed')
            for path,digest in snapshot['project_images'].items():check(record['loaded_project_images'][path]==digest,'Mapped partition differs')
        executable=process['executable']
        check(executable in build['binary_sha256']and any(s['project_images'].get(executable)==build['binary_sha256'][executable]for s in process['snapshots']),'Executed binary identity absent')
    counts['mapped_processes']+=len(record['processes'])

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--repo',type=Path,default=Path.cwd());p.add_argument('--output',type=Path,required=True);p.add_argument('--manifest',type=Path);args=p.parse_args()
    stage=args.stage.resolve();repo=args.repo.resolve();counts=Counter();capsules={};oracles={}
    if args.manifest:
        manifest=load(args.manifest)
        check(manifest['auditor_sha256']==sha(__file__),'Auditor identity differs')
        for path,digest in manifest['artifact_sha256'].items():check(sha(repo/path)==digest,'Sealed artifact differs '+path)
        with tarfile.open(repo/manifest['final_review_source_archive'])as archive:
            check(set(archive.getnames())==set(manifest['final_source_sha256']),'Final reviewed source set differs')
            for path,digest in manifest['final_source_sha256'].items():check(hashlib.sha256(archive.extractfile(path).read()).hexdigest()==digest,'Final preserved source differs')
        counts['artifact_bindings']=len(manifest['artifact_sha256'])
    check(sha(repo/CANON/'coverage-snapshot-v1.json')==LEDGER_SHA,'Canonical coverage changed')
    ledger=load(repo/CANON/'coverage-snapshot-v1.json');holds=load(repo/CANON/'stage-receipt-v1.json')['original_hold_keys_retained']
    check(ledger['coverage']['requirements']['verified']==1050 and ledger['coverage']['requirements']['total']==1158 and ledger['coverage']['historical_cases']['verified']==1816 and len(holds)==len(set(holds))==59,'Original counts/holds changed')
    prior=repo/'build/nasa-coverage/coverage95-20261103-function-special-orbit-bindings'
    check(sha(prior/'stage-receipt-v1.json')=='7aaaf6571930091f3e8ea63a11782c52af084678364ec0ce5a7f61df4c7f2963'and sha(prior/'evidence-inputs-v1.json')=='d7f86418af915cc98e9219956fdb2e5e7be787aed901b1ec9b668bb7075d25b9','Previous 298-type prefix identity differs')
    prior_receipt=load(prior/'stage-receipt-v1.json');check(sha(prior_receipt['source_archive'])==prior_receipt['source_archive_sha256'],'Previous source archive changed')
    with tarfile.open(prior_receipt['source_archive'])as archive:
        names=set(archive.getnames());check(names==set(prior_receipt['source_identities']),'Previous preserved source inventory differs')
        for name,digest in prior_receipt['source_identities'].items():check(hashlib.sha256(archive.extractfile(name).read()).hexdigest()==digest,'Previous preserved source differs')
        for path,digest in load(prior/'evidence-inputs-v1.json')['artifact_sha256'].items():
            absolute=Path(path);relative=str(absolute.relative_to(repo))if absolute.is_relative_to(repo)else path
            observed=hashlib.sha256(archive.extractfile(relative).read()).hexdigest()if relative in names else sha(absolute)
            check(observed==digest,'Previous frozen evidence differs '+relative)
    counts['retained_prefix_artifact_bindings']=len(load(prior/'evidence-inputs-v1.json')['artifact_sha256'])
    before=load(prior/'special-orbit-binding-matrix-v1.json');after=load(stage/'environment-parameter-binding-matrix-v1.json')
    check(len(before['rows'])==len(after['rows'])==566 and sum(r['role']=='owner-bound-system-parameter'for r in after['rows'])==321,'Original factory roles lost')
    for old,new in zip(before['rows'],after['rows']):
        check(all(new[k]==v for k,v in old.items()),'Original factory role/eligibility/conjunction changed')
    prior_work=load(prior/'remaining-parameter-binding-worklist-v1.json');work=load(stage/'remaining-parameter-binding-worklist-v1.json')
    check(len(prior_work['native_bound_types'])==298 and work['remaining_count']==0 and not work['rows']and work['previous_remaining_rows_historical']==prior_work['rows']and work['all_system_parameters_whole_scope_open'],'Remaining-work scope changed')
    old_remaining=load(repo/'build/nasa-coverage/coverage95-20261103-plot-gravity-samples/remaining-requirement-audit-v2.json');new_remaining=load(stage/'remaining-requirement-audit-v1.json')
    check(len(old_remaining['rows'])==len(new_remaining['rows'])==108 and new_remaining['original_hold_keys']==old_remaining['original_hold_keys'],'Requirement/hold inventory changed')
    for old,new in zip(old_remaining['rows'],new_remaining['rows']):check(all(new[k]==v for k,v in old.items()),'Original exact requirement/fingerprint/gap changed')
    for version in range(1,11):
        source=load(stage/f'executed-source-sha256-v{version}.json');build=load(stage/f'build-receipt-v{version}.json')
        check(build['source_sha256']==source and build['inputs_unchanged']and build['status']==('failed'if version==3 else'passed'),'Build history differs')
        with tarfile.open(stage/f'executed-sources-v{version}.tar.gz')as archive:
            check(len(archive.getnames())==len(source)and set(archive.getnames())==set(source),'Execution capsule inventory differs')
            for name,digest in source.items():check(hashlib.sha256(archive.extractfile(name).read()).hexdigest()==digest,'Execution capsule source differs')
            namespace={'__name__':'frozen_environment_oracle'};exec(compile(archive.extractfile('src/qtgui/tests/FunctionEnvironmentBindingOracle.py').read(),'frozen_environment_oracle','exec'),namespace);oracles[version]=namespace
        # Early capsules omit the new JSON. Its exact bytes were preserved
        # separately and the native invocation records their launch-time hash.
        # Do not claim that JSON participated in the parent's closing guard.
        catalog_version=2 if version<=5 else 3 if version==6 else 4 if version==7 else 5
        capsules[version]=load(stage/f'prepared-catalog-v{catalog_version}.json')
        counts['executed_source_capsules']+=1
    types=oracles[9]['TYPES'];selection=load(stage/'selected-evidence-v1.json');check(len(selection)==46 and{(s['case'],s['mode'])for s in selection}=={(kind,mode)for kind in types for mode in ['headless','visible']},'Missing/duplicate selected type/profile')
    check(set(work['native_bound_types'])==set(prior_work['native_bound_types'])|set(types)and len(work['native_bound_types'])==321,'Collective binding union differs')
    seen=set();proofs=[]
    for selected in selection:
        version=selected['source_version'];run=selected['run'];kind=selected['case'];mode=selected['mode'];test=PREFIX+kind+'Read';key=(run,version)
        if key not in seen:custody(stage,run,version,counts);seen.add(key)
        catalog=capsules[version];check(len(catalog['cases'])==321 and len(catalog['environment_binding_cases'])==23 and catalog['new_requirement_credit']==0,'Catalog scope differs')
        catalog_version=2 if version<=5 else 3 if version==6 else 4 if version==7 else 5
        check(load(stage/run/'results.json')['source_hashes']['src/qtgui/tests/NasaFunctionEnvironmentCases.json']==sha(stage/f'prepared-catalog-v{catalog_version}.json'),'Separately preserved native catalog differs')
        spec=next(x for x in catalog['environment_binding_cases']if x['factory_type']==kind)
        root=invocation(stage,run,test,mode,counts);proof=load(root/'proof.json');contract(proof,spec,oracles[version],counts);proofs.append((proof,spec,oracles[version]))
        check(selected['proof_sha256']==sha(root/'proof.json'),'Selected proof digest differs')
        check(load(root/'phases.json')==proof['phases'],'Phase sidecar differs')
        initial=(root/'initial.gmf').read_bytes();edited=(root/'edited.gmf').read_bytes();check(initial.replace(b'Keep = 103;',b'Keep = 41;')==edited and initial!=edited,'Function edit bytes differ')
        for phase in range(3):
            check(all(load(root/f'{phase}-invariants.json').values()),'Source/history/prototype/disk invariant failed')
            check(load(root/f'{phase}-prototypes.json')==load(root/'prototypes-before.json'),'Configured prototypes changed')
            raw=[float(x)for x in(root/f'{phase}-values.txt').read_text().split()];check(raw==[42,43,44,103,104],'Raw caller report differs')
            observed=[list(map(float,line.split()))for line in(root/f'{phase}-binding-values.txt').read_text().splitlines()if line.strip()]
            check(len(observed)==4,'Raw binding Report row count differs')
            for i,row in enumerate(observed):
                expected=[]
                for parameter in proof['phases'][phase]['live_samples'][i]['parameters']:expected+=parameter['actual']if isinstance(parameter['actual'],list)else[parameter['actual']]
                compare(row,expected,oracles[version]['bound'](kind,0),Counter())
            if kind in oracles[version]['TORQUE']:
                for scope,level in [('root',1),('private',2),('shared',3)]:
                    inputs=load(root/f'{phase}-{scope}-torque-inputs.json')
                    compare(inputs['system_moi'],[100*level*(i+1)if i==j else 0 for i in range(3)for j in range(3)],1e-10,Counter())
                    compare(inputs['system_cm'],[0,0,0],0,Counter());compare(inputs['body_to_inertial'],[int(i==j)for i in range(3)for j in range(3)],1e-12,Counter())
                    check(inputs['mass_properties_modeled']=='CenterOfMassAndMomentOfInertia','Actual mass-property mode differs')
                    if kind in ['SRPTorque','TotalTorque']:
                        authored=oracles[version]['plates'](scope);check(len(inputs['plates'])==len(authored)==6,'Actual plate count differs')
                        for a,e in zip(inputs['plates'],authored):
                            for field in ['area','specular','diffuse','lit_fraction','normal','location']:compare(a[field],e[field],0,Counter())
                    counts['actual_torque_input_sets']+=1
    for mode in ['headless','visible']:
        run='affected-'+mode+'-v2';custody(stage,run,10,counts)
        for test in REGRESSIONS:invocation(stage,run,test,mode,counts);counts['affected_original_contexts']+=1
    damaged=0
    for field in ['actual','owner','metadata','shape','calls','markers','inventory','bound']:
        original,spec,oracle=next(v for v in proofs if v[0]['case']=='OrbitErrorCovariance');proof=copy.deepcopy(original)
        row=proof['phases'][0]['live_samples'][0]['parameters'][0]
        if field=='actual':row['actual'][0]+=1
        elif field=='owner':row['reference_owner_pointer']='0'
        elif field=='metadata':row['runtime_settable']=not row['runtime_settable']
        elif field=='shape':row['shape']=[1,36]
        elif field=='calls':proof['phases'][0]['live_samples'][1]['calls']=99
        elif field=='markers':proof['phases'][0]['raw_markers'][0]=0
        elif field=='inventory':proof['full_system_parameter_inventory']=320
        else:row['absolute_bound']=1
        rejected=False
        try:contract(proof,spec,oracle,Counter())
        except(ValueError,KeyError,IndexError,TypeError):rejected=True
        check(rejected,'Damaged '+field+' proof accepted');damaged+=1
    support=load(stage/'matrix-contract-receipt-v1.json');check(support['status']=='passed'and support['source_version']==10 and support['returncode']==0 and support['inputs_unchanged']and support['accepted_shapes']==3 and support['rejected_transfers']==21,'Matrix support contract differs')
    check(support['build_receipt_sha256']==sha(stage/'build-receipt-v10.json')and support['source_sha256']==load(stage/'executed-source-sha256-v10.json')and support['binary_sha256']==load(stage/'build-receipt-v10.json')['binary_sha256'],'Matrix support build custody differs')
    check('Invalid matrix literal was silently accepted'in(stage/'matrix-contract-v2-rejected-inf.log').read_text(),'Original nonfinite transfer failure absent')
    check(support['maps']and all(support['loaded_project_images'][p]==h for m in support['maps']for p,h in m['project_images'].items()),'Matrix support raw maps absent')
    failures=load(stage/'retained-native-failures-v1.json');check(len(failures)==25,'Retained native failure census differs')
    for failure in failures:
        r=load(repo/failure['result']);check(r['status']=='failed'and r['returncode']==failure['returncode'],'Original native failure lost')
    result=dict(status='passed-bounded-environment-owner-audit',coverage=ledger['coverage'],requirements_still_needed=51,new_qualified_keys=[],original_hold_keys_retained=holds,selected_types=23,collective_versioned_live_bound_types=321,all_system_parameter_whole_obligations_open=True,metrics=dict(counts),damaged_proofs_rejected=damaged,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,independent_orientation_ephemeris_qualified=False)
    args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k]for k in ['status','selected_types','collective_versioned_live_bound_types','metrics','damaged_proofs_rejected','requirements_still_needed']}))

if __name__=='__main__':main()
