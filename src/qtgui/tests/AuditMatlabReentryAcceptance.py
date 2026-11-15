"""Verify the real licensed MATLAB restriction and reused native guard cases."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
import BuildNasaCoverageExtension as common
import AuditNasaCoveragePrefix as prefix
sha,load,require=common.sha,common.load,common.require
KEYS=['FRR!E479']
KIND='source-versioned-complete-matlab-function-reentry-restriction'
CONTEXT=[f'FRR!E{i}' for i in range(472,481)]
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','native_OS_input_qualified']
OPS=['execute','text-input','real-input','iteration','open-script','run-script','build-script','update-script','clear-script','put-script']

def structure(q,scope,inventory):
 known={r['key']:r for r in inventory['requirements']}
 require(q['contract_kind']==KIND and q['qualified_keys']==KEYS and q['authority_sha256']==inventory['sources'],'Different restriction/original authority')
 require(scope['candidate_rows']==[known[k] for k in KEYS] and scope['complete_original_context']==[known[k] for k in CONTEXT],'Changed original restriction/context')
 require(scope['original_assertions_changed']==0 and scope['failures_waived'] is False and scope['other_row_credit']==0 and scope['newly_resolved_hold_keys']==KEYS and scope['windows_process_modes_qualified'] is False,'Incomplete/overbroad restriction scope')
 require(all(q[k] is False and scope[k] is False for k in GATES),'Broader qualification promoted')

def checkpoint(folder):
 proof=load(folder/'proof.json');logged=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in (folder/'test.log').read_text().splitlines() if re.match(r'^(?:\d+:\s+)?NASA_CHECK ',line)]
 require(logged==[proof] and proof['outcome']=='passed','Proof differs from actual executed check');return proof

def qualify(repo,path,inventory):
 q=load(path)
 for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed restriction proof: '+n)
 scope=load(q['scope_review']);structure(q,scope,inventory);frozen={}
 for af,ids in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
  with tarfile.open(q[af]) as bundle:
   require(set(bundle.getnames())==set(q[ids]),'Changed source partition')
   for n,h in q[ids].items():
    raw=bundle.extractfile(n).read();require(hashlib.sha256(raw).hexdigest()==h and not raw.startswith(b'\x7fELF'),'Changed/non-source capsule member')
    if ids=='proof_source_sha256':frozen[n]=raw
 original=load(q['protected_input_baseline']);authorized=load(q['authorized_input_manifest']);guard=load(q['component_input_integrity']);require(len(original)==len(authorized)==1334 and all(q['proof_source_sha256'][n]==h for n,h in original.items()),'Original preimages lost')
 require(sorted(n for n in original if original[n]!=authorized[n])==['src/qtgui/tests/PlotIntegrationStepTests.cpp','src/qtgui/tests/SharedPlaybackTests.cpp'] and guard['authorized_inputs_final']==authorized and guard['changed_by_this_component']==[],'Unapproved protected amendment')
 previous=load(q['prior_full_audit']);require(len(previous['original_hold_keys'])==59 and len(previous['unresolved_original_hold_keys'])==17 and KEYS[0] in previous['unresolved_original_hold_keys'],'Original hold scope changed')
 s=Path(q['component_stage']);plan=load(s/'plan-v1.json');fixture=json.loads(frozen['src/qtgui/tests/MatlabFunctionReentryCase.json']);worker=frozen['src/qtgui/tests/MatlabFunctionReentryTests.cpp'];require(hashlib.sha256(worker).hexdigest()==plan['driver_sha256'],'Wrong executed worker version')
 require(all(token in worker for token in [b'window.runMission()',b'server.start()',b'probe.calls==0&&probe.writes==0',b'interface->ExecuteCallback()&&probe.calls==1',b'GetRealParameter("Value")==14',b'GetRealParameter("Value")==42']),'Missing actual mission or atomic callback checks')
 a=s/'headless-installed-provider-v3/Guard';res=load(a/'result.json');proof=checkpoint(a);require(res['returncode']==0 and res['status']=='passed' and res['proofs']==[proof] and res['signature']['source_sha256']==plan['driver_sha256'] and res['signature']['fixture_sha256']==plan['fixture_sha256'] and res['signature']['binary_sha256']==plan['binary_sha256'],'Wrong actual execution/inputs')
 actual=(a/'MatGuard.m').read_text();require(actual==fixture['matlab_function'] and all(x in actual for x in ['tcpclient','readline(c)','~reply.ok','GMAT-called MATLAB function cannot','y = 3*x + 2;']),'Actual licensed callback/strict diagnostic missing')
 caller=(a/'caller.script').read_text();require(all(x in caller for x in ['Input = 4;','Keep = 41;','[Out] = MatGuard(Input);','Keep = Keep + 1;']),'Actual call/successor missing')
 require(proof['actual_matlab_result']==3*4+2==fixture['expected_output'] and proof['successor']==41+1==fixture['expected_successor'] and proof['callback_dispatches_during_matlab']==proof['callback_writes_during_matlab']==0 and proof['callback_dispatches_after_return']==1,'Independent output/guard recovery failed')
 identity=load(s/'provider-identity-v1.json');actual_libs={l.split('calling init:',1)[1].strip() for f in a.glob('loader.*') for l in f.read_text().splitlines() if 'calling init:' in l and any(x in l for x in ['libMatlabInterface.so','libeng.so'])};require(actual_libs==set(identity['actual_loaded_libraries_sha256']) and len(actual_libs)==2 and identity['provider_version']=='R2026b' and identity['status']=='passed','Real provider identity missing')
 for stage in ['headless-v1','headless-provider-v2']:
  fail=load(s/stage/'Guard/result.json');require(fail['status']=='failed' and fail['returncode']!=0 and 'ApplicationService' in (s/stage/'Guard/test.log').read_text(),'Initial provider failure lost/waived')
 old=Path(q['prior_native_policy_stage'])
 for profile in ['repaired-current-headless-v6','repaired-current-visible-v1']:
  folder=old/profile/'QtGui.FunctionCommandPolicyMatlabGuard';res=load(folder/'result.json');p=checkpoint(folder);j=list(ET.parse(folder/'ctest.xml').iter('testcase'));require(res['status']=='passed' and res['returncode']==0 and len(j)==1 and all(j[0].find(x) is None for x in ['failure','error','skipped']),'Old guard did not execute/pass')
  require([x['blocked_operation'] for x in p['observations'][:-1]]==OPS and all(p['observations'][-1][k] is True for k in ['cross_thread_rejected','native_callback_resumes','nested_exception_preserves_outer_guard','receipt_guard_rejected']),'Old command/receipt/scope guard conjunction incomplete')
 code={'src/base/interface/GmatInterface.cpp':['activeMatlabFunctions','GMAT-called MATLAB function cannot invoke a GMAT callback','GMAT-called MATLAB function cannot build, change or run a GMAT script'], 'plugins/MatlabInterfacePlugin/src/base/command/CallMatlabFunction.cpp':['GmatInterface::MatlabFunctionScope scope;'], 'src/base/interface/SocketServer.cpp':['CanAcceptMatlabCommand'], 'src/qtgui/QtMatlabCallbackServer.cpp':['PutCallbackData(input)','ExecuteCallback()']}
 aggregate=load(q['aggregate_source_manifest'])
 for n,tokens in code.items():require(hashlib.sha256(frozen[n]).hexdigest()==aggregate[n] and all(t.encode() in frozen[n] for t in tokens),'Unbound guard/transport owner')
 rejected=0
 for field,value in [('newly_resolved_hold_keys',KEYS+['FRR!E480']),('windows_process_modes_qualified',True),('failures_waived',True)]:
  damaged=copy.deepcopy(scope);damaged[field]=value
  try:structure(q,damaged,inventory)
  except ValueError:rejected+=1
  else:raise ValueError('Damaged restriction admission accepted')
 return q,KEYS,{'status':'passed','qualified_keys':KEYS,'whole_row_conjunction':True,'component':{'actual_licensed_MATLAB_loopback_rejection_and_recovery':1,'reused_native_guard_executions':2,'covered_native_guard_operations':10,'cross_thread_nested_and_receipt_guards_reused':True,'initial_vendor_startup_failures_retained':2,'windows_process_modes_qualified':False,'original_assertions_changed':0,'native_tests_rerun_for_admission':0},'new_requirement_credit':1,'newly_resolved_original_hold_keys':KEYS,'damaged_admission_controls_rejected':rejected,'native_tests_rerun':0,'runtime_binary_copies':0,**{k:False for k in GATES}}

def build(repo, baseline, path, output):
    seal, inventory, keys = prefix.baseline(repo, baseline)
    original = {kind:set(values) for kind,values in keys.items()}
    _, incoming, audit = qualify(repo, path, inventory)
    known = {kind:{row['key']:row for row in inventory[kind]} for kind in keys}
    require(not set(incoming) & keys['requirements'], 'Duplicate restriction credit')
    keys['requirements'] = common.merge_keys(keys['requirements'], incoming, known['requirements'])
    coverage = {kind:{'verified':len(keys[kind]), 'total':len(known[kind]),
        'percent':round(100*len(keys[kind])/len(known[kind]),1),
        'new_verified':len(keys[kind])-len(original[kind])} for kind in keys}
    remaining = {kind:[row for row in seal['remaining_original_rows'][kind]
        if row['key'] not in keys[kind]] for kind in keys}
    result = {'status':'passed', 'scope':'Sealed source-versioned prefix plus complete original MATLAB-to-GMAT function restriction',
        'baseline':str(baseline), 'baseline_sha256':sha(baseline), 'authority_sha256':inventory['sources'],
        'coverage':coverage, 'target_requirements':1101, 'target_reached':len(keys['requirements'])>=1101,
        'requirements_still_needed':max(0,1101-len(keys['requirements'])),
        'additions':[{'key':key, 'kind':'requirements', 'fingerprint':known['requirements'][key]['fingerprint'],
            'source_text':known['requirements'][key]['text'], 'qualification':str(path),
            'qualification_sha256':sha(path), 'status':'verified-source-versioned'} for key in incoming],
        'remaining_counts':{kind:len(rows) for kind,rows in remaining.items()},
        'remaining_original_rows':remaining, 'technical_core_approval':False,
        'full_current_build_qualified':False, 'full_NASA_qualified':False,
        'limits':['Original 1158/2367 denominators, texts and fingerprints are unchanged.',
                  'Only FRR E479 is added and its own formal hold is resolved. All59 hold identities and all other row/status records remain unchanged.',
                  'The actual licensed MATLAB function cannot dispatch a callback back into the active GMAT mission. Source and saved native guards cover the shared callback and script-control paths. Broader MATLAB parents, Windows process modes, scientific, native OS input, platform and full-build approval remain separate.']}
    with Path(output).open('x') as file:
        json.dump(result, file, indent=2); file.write('\n')
    return result, audit


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.baseline:
        result, _ = build(args.repo, args.baseline, args.qualification, args.output)
        print(json.dumps({'coverage':result['coverage'], 'still_needed':result['requirements_still_needed']}))
    else:
        _, _, result = qualify(args.repo, args.qualification, nasa.inventory(args.repo))
        with args.output.open('x') as file:
            json.dump(result, file, indent=2); file.write('\n')
        print(json.dumps(result))
