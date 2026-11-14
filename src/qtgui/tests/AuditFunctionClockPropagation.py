"""Replay GT/Real start-selection controls without granting exact-row credit."""
import argparse
from collections import Counter
import copy
from decimal import Decimal, localcontext
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile
import tempfile
from xml.etree import ElementTree

STAGE='build/nasa-coverage/coverage95-20261104-function-clock-propagation'
PRIOR='build/nasa-coverage/coverage95-20261104-function-text-ephemeris'
CANON='build/nasa-coverage/coverage95-20261103-matlab-c-interface'
LEDGER='4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6'
PROFILES={'FromSpacecraft':('FromSpacecraft',30,True),'EphemStart':('EphemStart',0,True),'ScriptStart':('01 Jan 2000 12:00:30.000',30,True),'RealScriptStart':('01 Jan 2000 12:00:30.000',30,False),'FractionLittle':('EphemStart',.123456789,True),'FractionBig':('EphemStart',.123456789,True)}
def check(ok,message):
    if not ok:raise ValueError(message)
def load(p):return json.loads(Path(p).read_text())
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def source(archive,name):
    with tarfile.open(archive)as tar:return tar.extractfile(name).read()
def module(archive,name):
    ns={'__name__':'frozen_contract','__file__':name};exec(compile(source(archive,name),name,'exec'),ns);return ns
def calls(text):
    result=[]
    for match in re.finditer(r'\brequire\s*\(',text):
        index=match.end();depth=1;quote=None
        while depth:
            c=text[index]
            if quote:
                if c=='\\':index+=2;continue
                if c==quote:quote=None
            elif c in ['"',"'"]:quote=c
            elif c=='(':depth+=1
            elif c==')':depth-=1
            index+=1
        result.append(text[match.start():index])
    return result
def subsequence(needles,haystack):
    index=0
    for item in haystack:
        if index<len(needles)and item==needles[index]:index+=1
    return index==len(needles)
def normalized(root,name):
    match=re.search(r"ReadShared\.FunctionPath = '([^']+/Owned type scope Ω)/ReadShared.gmf';",(root/'caller.script').read_text())
    check(match is not None,'Actual owned fixture prefix absent')
    return (root/name).read_text().replace(match.group(1),'${OWNED_FIXTURE_DIR}')
def raw_input(root,kind,counts,shift=0,endian=1):
    for scope in ['root','private']:
        manifest=load(root/f'{scope}-{kind}-input-manifest.json')
        check(manifest['kind']==kind and manifest['binary_copies']==manifest['kernel_copies']==0 and len(manifest['samples'])==9,'Input recipe census differs')
        r=[7000,-200,450]if scope=='private'else[9000,100,300];v=[.2,6.8,-.3]if scope=='private'else[-.1,7.1,.4]
        for i,item in enumerate(manifest['samples']):
            expected=[r[j]+v[j]*(30*i+shift) for j in range(3)]+v
            check(item['utc_seconds']==30*i+shift and len(item['state'])==6 and all(abs(a-b)<=1e-11 for a,b in zip(item['state'],expected)),'Actual recorded affine input differs')
        if shift:
            import math
            recipe=load(root/f'{scope}-{kind}-clock-shift.json')
            check(recipe['utc_shift_seconds']==shift and recipe['file_endian']==endian,'Authored fractional file clock differs')
            expected_fields={40:1,64:1,336:1/864,384:1000,1536:1/864,1544:1/864,5616:1,8032:1/864}
            check({f['byte_offset']:f['units_per_second']for f in recipe['fields']}==expected_fields,'Exact generated clock-field recipe differs')
            for field in recipe['fields']:
                target=field['before']+shift*expected_fields[field['byte_offset']]
                check(abs(field['after']-target)<=math.ulp(target),'Generated clock field lost its exact shift')
            fields={f['byte_offset']:f for f in recipe['fields']}
            check(fields[40]['before']==fields[5616]['before']==43200 and fields[64]['before']==43440,'Physical UTC file clock differs')
            counts['fractional_clock_field_recipes']+=1
        counts['input_recipe_bindings']+=1
def script_clock(root,spec,profile):
    _,_,precise=PROFILES[profile]
    for filename,prop,owner,seconds in [('caller.script','Shared','MainSC',15),('initial.gmf','FunctionOwned','FunctionSC',30)]:
        text=(root/filename).read_text()
        pattern=r'Propagate '+prop+r'\('+owner+r'\) \{'+owner+r'\.ElapsedSecs = (\d+)(?:, StopTolerance = ([^;}]+))?\};'
        match=re.search(pattern,text);check(match is not None and int(match.group(1))==seconds,'Supplemental command duration differs')
        check((match.group(2)is not None)==precise,'Supplemental command clock mode differs')
        if precise:check(float(match.group(2))==1e-7,'Supplemental GT activation differs')
        check(spec['supplemental_'+('root'if owner=='MainSC'else'private')+'_spacecraft_configuration']in text,'Authored supplemental spacecraft clock missing')

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--repo',type=Path,default=Path.cwd());parser.add_argument('--stage',type=Path);parser.add_argument('--manifest',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    repo=args.repo.resolve();stage=args.stage.resolve()if args.stage else repo/STAGE;prior=repo/PRIOR;counts=Counter()
    if args.manifest:
        seal=load(args.manifest);check(sha(__file__)==seal['auditor_sha256'],'Auditor identity differs')
        for name,digest in seal['regular_artifact_sha256'].items():check(not(repo/name).is_symlink()and sha(repo/name)==digest,'Immutable artifact changed: '+name)
        for name,target in seal['symbolic_links'].items():check((repo/name).is_symlink()and str((repo/name).readlink())==target,'Temporal runtime route changed')
        archive=repo/seal['final_source_archive']
        with tarfile.open(archive)as tar:
            check(set(tar.getnames())==set(seal['final_source_sha256']),'Final reviewed archive scope differs')
            for name,digest in seal['final_source_sha256'].items():check(hashlib.sha256(tar.extractfile(name).read()).hexdigest()==digest,'Final reviewed source changed')
        counts['immutable_artifact_bindings']=len(seal['regular_artifact_sha256']);counts['temporal_alias_routes']=len(seal['symbolic_links'])
    check(sha(repo/CANON/'coverage-snapshot-v1.json')==LEDGER,'Canonical exact scopes/fingerprints changed')
    check(sha(prior/'evidence-manifest-v1.json')=='42040ed6c92eeb86929da2fdfad3b193ceee9146364ec40bcfe0057d7265524d'and sha(prior/'evidence-audit-v1.json')=='172a084ed24e45d4e8f74a1e0b68745f9bc0c2788fdec5374ef202c493e9f63a','Prior final evidence changed')
    old_seal=load(prior/'evidence-manifest-v1.json');old_archive=repo/old_seal['final_source_archive'];prior_contract=module(old_archive,'src/qtgui/tests/AuditFunctionTextEphemeris.py')
    with tempfile.TemporaryDirectory(prefix='gmat-clock-prior-audit-')as folder:
        script=Path(folder)/'prior.py';output=Path(folder)/'audit.json';script.write_bytes(source(old_archive,'src/qtgui/tests/AuditFunctionTextEphemeris.py'))
        result=subprocess.run([sys.executable,str(script),'--repo',str(repo),'--stage',str(prior),'--manifest',str(prior/'evidence-manifest-v1.json'),'--output',str(output)],capture_output=True,text=True)
        check(result.returncode==0,'Frozen prior audit failed: '+result.stderr[-1500:]);check(load(output)==load(stage/'prior-audit-replay-v1.json'),'Frozen prior audit changed')
    receipt=load(stage/'stage-receipt-v2.json');previous=load(prior/'stage-receipt-v2.json')
    for key in ['canonical_verified','original_total','target','still_needed','historical_verified','historical_total','original_hold_keys','new_requirement_credit','whole_row_qualified','scientific_approval','technical_approval','full_current_build_qualified','full_NASA_qualified','collective_owned_types','total_owned_types','remaining_owned_types']:check(receipt[key]==previous[key],'Original scope/gate changed: '+key)
    check(receipt['canonical_verified']==1050 and receipt['original_total']==1158 and receipt['target']==1101 and receipt['still_needed']==51 and len(set(receipt['original_hold_keys']))==59,'Original target/denominator/holds changed')
    for old_name,new_name in [('remaining-requirement-audit-v1.json','remaining-requirement-audit-v2.json'),('text-owned-type-matrix-v1.json','clock-owned-type-matrix-v2.json')]:
        a,b=load(prior/old_name),load(stage/new_name)
        if 'matrix'in old_name:
            check(len(b['rows'])==566,'Full resource universe changed')
            for x,y in zip(a['rows'],b['rows']):check(all(y[k]==v for k,v in x.items()),'Original role/eligibility/Global obligation changed')
        else:check(len(b['rows'])==108 and all(b[k]==v for k,v in a.items()),'Original exact row/fingerprint/gap/conjunction changed')
    driver='src/qtgui/tests/NasaFunctionOwnedTypeTests.cpp';catalog='src/qtgui/tests/NasaFunctionOwnedTypeCases.json';base=load(stage/'baseline-source-sha256-v1.json');original_calls=calls(source(old_archive,driver).decode());first_calls=calls(source(stage/'executed-sources-v1.tar.gz',driver).decode())
    check(len(original_calls)==43 and len(first_calls)==46 and subsequence(original_calls,first_calls),'Original native assertions changed')
    helper='src/qtgui/tests/FunctionOwnedInputFixtures.hpp';helper_calls=calls(source(old_archive,helper).decode())
    check(len(helper_calls)==3,'Original input-helper assertion census differs')

    for version in range(1,receipt['source_versions']+1):
        identities=prior_contract['archive_sources'](stage,version);build=load(stage/f'build-receipt-v{version}.json')
        check(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==identities,'Build/source custody changed')
        native_calls=calls(source(stage/f'executed-sources-v{version}.tar.gz',driver).decode())
        seen_helper_calls=calls(source(stage/f'executed-sources-v{version}.tar.gz',helper).decode())
        check(subsequence(helper_calls,seen_helper_calls),'Original input-helper assertion changed')

        check(subsequence(first_calls,native_calls)and len(native_calls)==(46 if version==1 else 49)and identities[catalog]==base[catalog],'Original/additional assertions or32-case catalog changed');counts['source_capsules']+=1
    check(all(load(stage/'build-receipt-v4.json')['binary_sha256'][n]==h for n,h in load(stage/'build-receipt-v3.json')['binary_sha256'].items()),'Regression target build changed already selected native images')
    for run,version in receipt['gui_execution_versions'].items():prior_contract['custody'](load(stage/(run+'-loaded-images.json')),load(stage/f'build-receipt-v{version}.json'),stage/f'build-receipt-v{version}.json',counts)
    for mode in ['headless','visible']:
        observed=load(stage/('precise-clock-'+mode+'-v1')/'results.json');statuses=Counter(x['status']for x in observed['results']if x['status']!='not-run')
        check(statuses=={'passed':3,'failed':6},'Initial clock failure census changed')
        for item in observed['results']:
            if item['status']in ['passed','failed']:check((item['status']=='passed')==item['name'].endswith('FromSpacecraft'),'Initial selector failure partition changed')
    counts['retained_initial_failures']=12
    for run in ['fraction-clock-headless-v1','fraction-clock-headless-v2','fraction-clock-visible-v1']:
        observed=load(stage/run/'results.json');statuses=Counter(x['status']for x in observed['results']if x['status']!='not-run')
        check(statuses=={'failed':2},'Fractional input/runtime failure census changed')
    counts['retained_fraction_input_failures']=2;counts['retained_fraction_runtime_failures']=4
    binary=str(repo/'build/linux-gui/src/qtgui/GmatQtNasaFunctionOwnedTypeTests')
    check(load(stage/'build-receipt-v6.json')['binary_sha256'][binary]==load(stage/'build-receipt-v7.json')['binary_sha256'][binary],'Fractional before/after native test program changed')
    for mode in ['headless','visible']:
        before=stage/('fraction-clock-'+mode+('-v2'if mode=='headless'else'-v1'))
        after=stage/('code500-clock-'+mode+'-v3')
        for profile in ['FractionLittle','FractionBig']:
            name='QtGui.NasaFunctionEphemerisClockCode500'+profile
            for scope in ['root','private']:
                a,b=before/name,after/name
                check(load(a/f'{scope}-Code500-clock-shift.json')==load(b/f'{scope}-Code500-clock-shift.json'),'Fractional before/after file clock changed')
                check(load(a/f'{scope}-Code500-input-manifest.json')['samples']==load(b/f'{scope}-Code500-input-manifest.json')['samples'],'Fractional before/after physical trajectory changed')

    # Rebuild each supplemental catalog from frozen canonical input in a fresh
    # offline directory; no current mutable case file is used as historical input.
    executed=stage/'executed-sources-v7.tar.gz';builder=module(executed,'src/qtgui/tests/PrepareFunctionEphemerisClock.py');catalogs={}
    with tempfile.TemporaryDirectory(prefix='gmat-clock-catalog-audit-')as folder:
        root=Path(folder);path=root/catalog;path.parent.mkdir(parents=True);path.write_bytes(source(executed,catalog))
        for profile in PROFILES:
            name=f'src/qtgui/tests/NasaFunctionEphemerisClock{profile}Cases.json';data=json.loads(source(executed,name));check(builder['generate'](root,profile)==data,'Supplemental catalog recipe differs');catalogs[profile]=data
    original=module(old_archive,'src/qtgui/tests/AuditFunctionOwnedTypes.py');old_expected=original['expected_state']
    def expected(sample,spec,root):
        kind=spec['factory_type']
        if kind not in {'Code500','STK','CCSDS-OEM'}:return old_expected(sample,spec,root)
        inside=sample['inside_function'];resource=sample['resource'];execution=resource['executed'];inputs=sample['oracle_inputs'];elapsed=execution['elapsed_seconds']
        check(abs(elapsed-spec['requested_private_seconds'if inside else'requested_root_seconds'])<=spec['stop_time_bound_seconds'],'Original elapsed stop bound differs')
        check(inputs['file_time_system']=='UTC'and abs(float((Decimal(inputs['query_gt'])-Decimal(inputs['start_gt']))*86400)-inputs['seconds_gt'])<1e-10,'Precise oracle input differs')
        profile=spec.get('clock_profile',spec.get('real_clock_profile'))
        if profile:
            selector,offset,precise=PROFILES[profile]
            check(execution['has_precision_time']is precise and resource['owner_has_precision_time']is precise and execution['start_epoch_source']==selector,'Actual selected clock/selector differs')
            check(spec['expected_start_selector']==selector and spec['expected_start_offset_seconds']==offset,'Independent authored start expectation differs')
            check(abs(inputs['seconds_gt']-elapsed-offset)<=(1e-9 if precise else 1e-6),'A correct state at the wrong selected start escaped')
        else:check(execution['has_precision_time']is False and resource['owner_has_precision_time']is False,'Original default clock fixture changed')
        r=[7000,-200,450]if inside else[9000,100,300];v=[.2,6.8,-.3]if inside else[-.1,7.1,.4];return[r[i]+v[i]*inputs['seconds_gt']for i in range(3)]+v
    original['expected_state']=expected;original['OPEN_TYPES']-={'Code500','STK','CCSDS-OEM'}
    proofs=[];new_counts=Counter()
    for selected in receipt['selected']:
        root=original['invocation'](stage,selected['run'],selected['test'],selected['mode'],counts);proof=load(root/'proof.json');data=load(root/'executed-catalog.json');profile=selected['profile'];check(data==catalogs[profile]and sha(root/'proof.json')==selected['proof_sha256'],'Selected supplemental input/proof changed')
        spec=next(c for c in data['cases']if c['key']==selected['case']);check((spec['position_bound_km'],spec['velocity_bound_km_s'],spec['stop_time_bound_seconds'])==(1e-7,1e-10,1e-6),'Original numerical bounds weakened')
        original['contract'](proof,spec,new_counts);raw_input(root,selected['case'],counts,spec.get('file_clock_shift_seconds',0),spec.get('file_output_endian',1));script_clock(root,spec,profile)
        if selected['case']in ['STK','CCSDS-OEM']:prior_contract['fixture'](root,selected['case'],counts)
        with localcontext()as context:
            context.prec=60
            configured=load(root/'configured-state-before.json');authored=Decimal(21545)+(Decimal('32.0343817')+Decimal(PROFILES[profile][1]))/86400
            check(abs((Decimal(configured['epoch_gt'])-authored)*86400)<=Decimal('1e-9'),'Configured supplemental spacecraft clock differs')
        check(load(root/'reopened-prototypes.json')==load(root/'prototypes-before.json'),'Exact prototype reopen invariant changed')
        for phase in range(3):
            check(all(load(root/f'{phase}-invariants.json').values()),'Original source/history/prototype/disk invariant differs')
            check([float(x)for x in(root/f'{phase}-values.txt').read_text().split()]==proof['phases'][phase]['raw_values'],'Actual raw Report bytes differ')
        proofs.append((proof,spec));counts['precise_contexts'if PROFILES[profile][2]else'real_explicit_contexts']+=1
    counts.update({'new_'+k:v for k,v in new_counts.items()})
    default_counts=Counter();baseline_catalog=json.loads(source(old_archive,catalog))
    for selected in receipt['defaults']:
        root=original['invocation'](stage,selected['run'],selected['test'],selected['mode'],counts);proof=load(root/'proof.json');check(load(root/'executed-catalog.json')==baseline_catalog,'Original default catalog changed');spec=next(c for c in baseline_catalog['cases']if c['key']==selected['case']);original['contract'](proof,spec,default_counts)
    for record in load(stage/'original-script-preservation-v2.json')['records']:
        before,after=repo/record['prior'],repo/record['current'];a=normalized(before.parent,before.name);b=normalized(after.parent,after.name)
        check(a==b and hashlib.sha256(b.encode()).hexdigest()==record['normalized_sha256'],'Default script source changed beyond its exact fixture-directory prefix');counts['unchanged_default_script_bindings']+=1
    for reg in receipt['regressions']:original['invocation'](stage,reg['run'],reg['test'],reg['mode'],counts)
    check(counts['passed_native_contexts']==66 and counts['precise_contexts']==22 and counts['real_explicit_contexts']==6,'Selected new/original native census differs')
    support=stage/receipt['support_run'];execution=load(support/'execution.json');version=receipt['support_source_version'];build=load(stage/f'build-receipt-v{version}.json');prior_contract['custody'](execution,build,stage/f'build-receipt-v{version}.json',counts);check(execution['returncode']==0,'Registered retained Code500 contract failed')
    tests=ElementTree.parse(support/'ctest.xml').findall('.//testcase');check(len(tests)==1 and tests[0].get('name')=='QtAcceptance.Code500EpochInterpolation'and all(tests[0].find(k)is None for k in ['failure','error','skipped']),'Retained registered contract JUnit differs')
    spline=module(executed,'src/qtgui/tests/AuditCode500EpochInterpolation.py');support_proof=load(support/'proof.json');result=spline['audit'](support_proof);check(result==load(support/'independent-audit.json'),'Retained Code500 arithmetic replay differs');counts.update({'support_'+k:v for k,v in result['counts'].items()})
    damages=[lambda p:p['phases'][0]['live_samples'].pop(),lambda p:p['phases'][0]['live_samples'][1].update(shared_pointer='0'),lambda p:p['phases'][0]['live_samples'][0]['resource']['executed'].update(has_precision_time=False),lambda p:p['phases'][0]['live_samples'][0]['resource']['executed'].update(start_epoch_source='Wrong'),lambda p:p['phases'][0]['live_samples'][0]['oracle_inputs'].update(seconds_gt=15),lambda p:p['phases'][0]['live_samples'][0]['actual'].__setitem__(0,0),lambda p:p.update(whole_row_qualified=True)]
    for damage in damages:
        proof,spec=proofs[0];bad=copy.deepcopy(proof);damage(bad);rejected=False
        try:original['contract'](bad,spec,Counter())
        except(ValueError,KeyError,IndexError):rejected=True
        check(rejected,'Damaged clock/scope evidence escaped');counts['damaged_proofs_rejected']+=1
    proof,spec=next((p,s)for p,s in proofs if 'real_clock_profile'in s);bad=copy.deepcopy(proof);bad['phases'][0]['live_samples'][0]['resource']['executed']['has_precision_time']=True;rejected=False
    try:original['contract'](bad,spec,Counter())
    except(ValueError,KeyError,IndexError):rejected=True
    check(rejected,'Real clock substitution escaped');counts['damaged_proofs_rejected']+=1
    for damage in [lambda p:p['files'].pop(),lambda p:p['observations'][0]['actual'].__setitem__(0,0),lambda p:p['observations'][0]['spline_inputs'][1].update(x_seconds=31),lambda p:p.update(whole_row_qualified=True)]:
        bad=copy.deepcopy(support_proof);damage(bad);rejected=False
        try:spline['audit'](bad)
        except(ValueError,KeyError,IndexError):rejected=True
        check(rejected,'Damaged retained interpolation evidence escaped');counts['damaged_proofs_rejected']+=1
    core=load(stage/'core-increment-v2.json');check(core['pending_core_paths']==279 and core['prior_paths_unchanged']==274 and core['existing_pending_path_increments']==4 and core['new_pending_paths']==1 and len(core['increments'])==5,'Core increment scope differs')
    for change in core['increments']:check(sha(repo/change['before'])==base[change['path']]==change['before_sha256']and sha(repo/change['after'])==load(stage/'executed-source-sha256-v7.json')[change['path']]==change['after_sha256'],'Exact core pre/postimage changed')
    check(sha(stage/'core-increment-v2.patch')==core['patch_sha256'],'Exact core patch changed')
    processes=load(stage/receipt['process_audit']);check(not processes['matching_native_processes']and not processes['matching_visible_servers']and len(processes['recorded_native_identities'])==counts['mapped_native_processes'],'Recorded terminal identity census differs')
    result=dict(status='passed-bounded-ephemeris-clock-audit',counts=dict(counts),canonical_verified=1050,original_total=1158,target=1101,still_needed=51,open_exact_rows=108,original_holds=59,collective_owned_types=35,total_owned_types=36,remaining_owned_types=['Topocentric'],pending_core_paths=279,new_requirement_credit=0,whole_row_qualified=False,scientific_technical_current_build_NASA_qualified=False)
    check(not args.output.exists(),'Never overwrite an audit');args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
