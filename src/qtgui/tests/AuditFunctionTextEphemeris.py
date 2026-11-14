"""Replay bounded text-ephemeris evidence, preserving every exact-row gate."""
import argparse
from collections import Counter
import contextlib
import copy
from decimal import Decimal
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
from xml.etree import ElementTree

STAGE='build/nasa-coverage/coverage95-20261104-function-text-ephemeris'
PRIOR='build/nasa-coverage/coverage95-20261104-function-owned-precision'
CANON='build/nasa-coverage/coverage95-20261103-matlab-c-interface'
LEDGER='4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6'
def check(ok,message):
    if not ok:raise ValueError(message)
def load(p):return json.loads(Path(p).read_text())
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def module(archive,name):
    with tarfile.open(archive)as tar:source=tar.extractfile(name).read()
    ns={'__name__':'frozen_contract','__file__':name};exec(compile(source,name,'exec'),ns);return ns
def archive_sources(stage,version):
    identities=load(stage/f'executed-source-sha256-v{version}.json')
    with tarfile.open(stage/f'executed-sources-v{version}.tar.gz')as tar:
        check(set(tar.getnames())==set(identities),'Executed archive scope differs')
        for name,digest in identities.items():check(hashlib.sha256(tar.extractfile(name).read()).hexdigest()==digest,'Executed source bytes differ')
    return identities
def custody(record,build,receipt,counts):
    check(record['inputs_unchanged'] and record['runtime_binary_copies']==0 and record['processes'] and record['source_sha256']==build['source_sha256'] and record['binary_sha256']==build['binary_sha256'] and record['build_receipt_sha256']==sha(receipt),'Execution/source/build custody differs')
    for process in record['processes']:
        check(any(s['project_images'].get(process['executable'])==build['binary_sha256'][process['executable']]for s in process['snapshots']),'Actual executable image absent')
        for snapshot in process['snapshots']:
            check(hashlib.sha256(snapshot['maps'].encode()).hexdigest()==snapshot['maps_sha256'],'Raw native maps changed')
            for name,digest in snapshot['project_images'].items():
                check(record['loaded_project_images'][name]==digest,'Recorded loaded images disagree')
                if name in build['binary_sha256']:check(build['binary_sha256'][name]==digest,'Mapped image does not match its build')
    counts['mapped_native_processes']+=len(record['processes'])
def fixture(root,kind,counts):
    for scope in ['root','private']:
        raw=(root/f'{scope}-{kind}-input.txt').read_bytes();record=load(root/f'{scope}-{kind}-input-manifest.json')
        check(hashlib.sha256(raw).hexdigest()==record['sha256'] and record['binary_copies']==record['kernel_copies']==0,'Authored/actual input binding differs')
        lines=raw.decode().splitlines();start=lines.index('EphemerisTimePosVel'if kind=='STK'else'META_STOP')+1
        data=[line.split()for line in lines[start:]if line and not line.startswith('END')]
        check(len(data)==9 and all(len(row)==7 for row in data),'Original text input grid differs')
        r=[7000,-200,450]if scope=='private'else[9000,100,300];v=[.2,6.8,-.3]if scope=='private'else[-.1,7.1,.4]
        for i,row in enumerate(data):
            if kind=='STK':check(float(row[0])==30*i,'Original STK offset differs')
            else:check(row[0]==f'2000-01-01T12:{i//2:02d}:{30*(i%2):02d}.000','Original OEM calendar clock differs')
            expected=[r[j]+v[j]*30*i for j in range(3)]+v
            check(all(abs(float(a)-b)<=1e-11 for a,b in zip(row[1:],expected)),'Original affine file state differs')
        counts['independently_checked_gui_input_files']+=1

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--repo',type=Path,default=Path.cwd());parser.add_argument('--stage',type=Path);parser.add_argument('--manifest',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    repo=args.repo.resolve();stage=args.stage.resolve()if args.stage else repo/STAGE;prior=repo/PRIOR;counts=Counter();final_archive=None
    if args.manifest:
        seal=load(args.manifest);check(sha(__file__)==seal['auditor_sha256'],'Auditor identity differs')
        for name,digest in seal['regular_artifact_sha256'].items():check(not(repo/name).is_symlink()and sha(repo/name)==digest,'Immutable artifact changed: '+name)
        for name,target in seal['symbolic_links'].items():check((repo/name).is_symlink()and str((repo/name).readlink())==target,'Temporal runtime route changed')
        final_archive=repo/seal['final_source_archive']
        with tarfile.open(final_archive)as tar:
            check(set(tar.getnames())==set(seal['final_source_sha256']),'Reviewed archive scope differs')
            for name,digest in seal['final_source_sha256'].items():check(hashlib.sha256(tar.extractfile(name).read()).hexdigest()==digest,'Reviewed frozen source changed')
        counts['immutable_artifact_bindings']=len(seal['regular_artifact_sha256']);counts['temporal_alias_routes']=len(seal['symbolic_links'])
    check(sha(repo/CANON/'coverage-snapshot-v1.json')==LEDGER,'Canonical scope/fingerprints changed')
    check(sha(prior/'evidence-manifest-v2.json')=='283fe1ba23c4e87b89cb3f95f0b20db2213d983bc8415000c0780ff565253839'and sha(prior/'evidence-audit-v2.json')=='4048e7c5779c90c60ffa2db9b88a2c3f3ec8b17750eac74a926ff4a99461c22a','Prior final seal changed')
    old_seal=load(prior/'evidence-manifest-v2.json')
    # Reexecute the actual frozen prior audit, including its immutable/temporal
    # legacy-link correction and original contracts. No GMAT process is run.
    with tempfile.TemporaryDirectory(prefix='gmat-text-prior-audit-')as temporary:
        script=Path(temporary)/'prior.py';result=Path(temporary)/'result.json'
        with tarfile.open(repo/old_seal['final_source_archive'])as tar:script.write_bytes(tar.extractfile('src/qtgui/tests/AuditFunctionOwnedPrecision.py').read())
        command=[sys.executable,str(script),'--repo',str(repo),'--stage',str(prior),'--manifest',str(prior/'evidence-manifest-v2.json'),'--output',str(result)]
        completed=subprocess.run(command,capture_output=True,text=True)
        check(completed.returncode==0,'Frozen prior audit failed: '+completed.stderr[-1500:]);check(load(result)==load(stage/'prior-audit-replay-v1.json'),'Frozen prior result changed')
    counts['retained_owned_type_witnesses']=33
    receipt=load(stage/'stage-receipt-v2.json');previous=load(prior/'stage-receipt-v2.json')
    for key in ['canonical_verified','original_total','target','still_needed','historical_verified','historical_total','original_hold_keys','new_requirement_credit','whole_row_qualified','scientific_approval','technical_approval','full_current_build_qualified','full_NASA_qualified']:check(receipt[key]==previous[key],'Original scope/gate changed: '+key)
    check(receipt['canonical_verified']==1050 and receipt['original_total']==1158 and receipt['target']==1101 and receipt['still_needed']==51 and len(set(receipt['original_hold_keys']))==59,'Original denominator/target/holds differ')
    old_remaining=load(prior/'remaining-requirement-audit-v1.json');remaining=load(stage/'remaining-requirement-audit-v1.json')
    check(len(remaining['rows'])==108 and all(remaining[k]==v for k,v in old_remaining.items()),'An exact row, fingerprint, historical gap or conjunction changed')
    matrix=load(stage/'text-owned-type-matrix-v1.json');old_matrix=load(prior/'precision-owned-type-matrix-v1.json')
    check(len(matrix['rows'])==566,'Full factory universe changed')
    for a,b in zip(old_matrix['rows'],matrix['rows']):check(all(b[k]==v for k,v in a.items()),'Old role/eligibility/Global obligation changed')
    driver='src/qtgui/tests/NasaFunctionOwnedTypeTests.cpp';catalog='src/qtgui/tests/NasaFunctionOwnedTypeCases.json';baseline=load(stage/'baseline-source-sha256-v1.json')
    for version in range(1,receipt['source_versions']+1):
        identities=archive_sources(stage,version);build=load(stage/f'build-receipt-v{version}.json')
        check(build['source_sha256']==identities and build['inputs_unchanged']and build['status']==('failed'if version in [1,2]else'passed'),'Build/source history changed')
        check(identities[driver]==baseline[driver]and identities[catalog]==baseline[catalog],'Original native assertions/catalog/bounds changed');counts['executed_source_capsules']+=1
    original=module(repo/old_seal['final_source_archive'],'src/qtgui/tests/AuditFunctionOwnedTypes.py');old_expected=original['expected_state']
    def expected(sample,spec,root):
        if spec['factory_type']not in {'STK','CCSDS-OEM'}:return old_expected(sample,spec,root)
        inputs=sample['oracle_inputs'];resource=sample['resource'];elapsed=resource['executed']['elapsed_seconds'];inside=sample['inside_function']
        check(abs(elapsed-spec['requested_private_seconds'if inside else'requested_root_seconds'])<=spec['stop_time_bound_seconds'],'Original stop bound changed')
        check(inputs['file_time_system']=='UTC' and abs(float((Decimal(inputs['query_gt'])-Decimal(inputs['start_gt']))*86400)-inputs['seconds_gt'])<1e-10,'Precise file/query diagnostic differs')
        check(resource['executed']['has_precision_time']is False and resource['owner_has_precision_time']is False,'Original default Real-clock scenario changed')
        r=[7000,-200,450]if inside else[9000,100,300];v=[.2,6.8,-.3]if inside else[-.1,7.1,.4];return[r[i]+v[i]*inputs['seconds_gt']for i in range(3)]+v
    original['expected_state']=expected;original['OPEN_TYPES']-= {'STK','CCSDS-OEM'}
    with tarfile.open(stage/'executed-sources-v5.tar.gz')as tar:catalog_data=json.loads(tar.extractfile(catalog).read())
    proofs=[]
    for selected in receipt['selected']:
        root=original['invocation'](stage,selected['run'],selected['test'],selected['mode'],counts);proof=load(root/'proof.json');spec=next(c for c in catalog_data['cases']if c['key']==selected['case'])
        check(sha(root/'proof.json')==selected['proof_sha256'],'Selected GUI proof changed');original['contract'](proof,spec,counts);fixture(root,selected['case'],counts)
        check(load(root/'reopened-prototypes.json')==load(root/'prototypes-before.json'),'Exact reopened prototype equality changed')
        for phase in range(3):
            check(all(load(root/f'{phase}-invariants.json').values()),'Original source/history/prototype/disk invariant changed')
            check([float(x)for x in(root/f'{phase}-values.txt').read_text().split()]==proof['phases'][phase]['raw_values'],'Actual raw Report changed')
        proofs.append((proof,spec))
    for regression in receipt['regressions']:original['invocation'](stage,regression['run'],regression['test'],regression['mode'],counts)
    check(counts['passed_native_contexts']==38,'New/affected native census differs')
    for run,version in receipt['gui_execution_versions'].items():custody(load(stage/(run+'-loaded-images.json')),load(stage/f'build-receipt-v{version}.json'),stage/f'build-receipt-v{version}.json',counts)
    for run in receipt['support_runs']:
        version={'text-contract-v1':3,'text-contract-v2':4,'text-contract-v3':5,'text-contract-v4':6,'text-contract-v5':7,'text-contract-v6':8}[run]
        execution=load(stage/run/'execution.json');check(execution['returncode']==(8 if run in ['text-contract-v1','text-contract-v2']else 0),'Recorded support outcome changed');custody(execution,load(stage/f'build-receipt-v{version}.json'),stage/f'build-receipt-v{version}.json',counts)
    support=stage/receipt['support_run'];tests=ElementTree.parse(support/'ctest.xml').findall('.//testcase')
    check(len(tests)==1 and tests[0].get('name')=='QtAcceptance.TextEphemerisEpoch'and all(tests[0].find(k)is None for k in ['failure','error','skipped']),'Registered native-plus-oracle contract differs')
    support_module=module(stage/'executed-sources-v8.tar.gz','src/qtgui/tests/AuditTextEphemerisEpoch.py');support_proof=load(support/'proof.json');independent=support_module['audit'](support_proof,support)
    check(independent==load(support/'independent-audit.json'),'Independent arithmetic replay differs');counts.update({'support_'+k:v for k,v in independent['counts'].items()})
    # Validate the complete request multiset independently of the native loop.
    groups={};outside=Counter()
    files={f['key']:f for f in support_proof['files']}
    for observation in support_proof['observations']:
        key=observation['file']
        if observation.get('outside'):outside[key]+=1;continue
        groups.setdefault((key,observation['direction'],observation['api']),[]).append(observation['requested_seconds'])
    for (key,direction,api),requests in groups.items():
        end=30*(files[key]['count']-1);check(Counter(requests)==Counter([0,.125,14.999999,15,15.000001,30,end/2,end-15,end-.125,end])and outside[key]==2,'Original full request partition changed')
    damages=[lambda p:p['phases'][0]['live_samples'].pop(),lambda p:p['phases'][0]['live_samples'][1].update(shared_pointer='0'),lambda p:p['phases'][0]['live_samples'][1]['resource'].update(spacecraft_j2000_body='0'),lambda p:p['phases'][0]['live_samples'][1]['actual'].__setitem__(0,0),lambda p:p.update(whole_row_qualified=True)]
    for damage in damages:
        proof,spec=proofs[0];bad=copy.deepcopy(proof);damage(bad);rejected=False
        try:original['contract'](bad,spec,Counter())
        except(ValueError,IndexError,KeyError):rejected=True
        check(rejected,'Damaged function evidence escaped');counts['damaged_proofs_rejected']+=1
    for damage in [lambda p:p['files'].pop(),lambda p:p['files'][0]['nodes'][0]['epoch'].update(frac=.5),lambda p:p['observations'][1]['state'].__setitem__(0,0),lambda p:p['subulp_controls'][0]['nodes'].pop(),lambda p:p['controls'][1].update(count=1),lambda p:p.update(whole_row_qualified=True)]:
        bad=copy.deepcopy(support_proof);damage(bad);rejected=False
        try:support_module['audit'](bad,support)
        except(ValueError,IndexError,KeyError):rejected=True
        check(rejected,'Damaged clock evidence escaped');counts['damaged_proofs_rejected']+=1
    core=load(stage/'core-increment-v1.json');check(core['pending_core_paths']==278 and core['prior_paths_unchanged']==267 and core['new_pending_paths']==len(core['increments'])==11,'Core increment scope changed')
    for change in core['increments']:
        check(sha(repo/change['before'])==baseline[change['path']]==change['before_sha256']and sha(repo/change['after'])==change['after_sha256']==load(stage/'executed-source-sha256-v7.json')[change['path']],'Exact incremental pre/postimage changed')
    check(sha(stage/'core-increment-v1.patch')==core['patch_sha256'],'Exact core patch changed')
    processes=load(stage/receipt['process_audit']);check(processes['matching_native_processes']==[]and processes['matching_visible_servers']==[],'A recorded process was not terminal')
    check(receipt['remaining_owned_types']==['Topocentric']and receipt['collective_owned_types']==35 and receipt['new_requirement_credit']==0,'Owned progress/gates inflated')
    result=dict(status='passed-bounded-text-ephemeris-audit',counts=dict(counts),canonical_verified=1050,original_total=1158,target=1101,still_needed=51,open_exact_rows=108,original_holds=59,collective_owned_types=35,total_owned_types=36,remaining_owned_types=['Topocentric'],pending_core_paths=278,prior_core_paths_unchanged=267,new_requirement_credit=0,whole_row_qualified=False,scientific_technical_current_build_NASA_qualified=False)
    check(not args.output.exists(),'Never overwrite an audit');args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
