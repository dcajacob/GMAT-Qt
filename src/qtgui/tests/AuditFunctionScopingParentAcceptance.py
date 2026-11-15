"""Join the nine original scoping clauses and their missing provider contexts."""
import argparse,copy,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
import BuildNasaCoverageExtension as common
import AuditNasaCoveragePrefix as prefix
sha,load,require=common.sha,common.load,common.require
KEYS=['FRR!E491'];CHILDREN=[f'FRR!E{i}' for i in range(492,501)];CONTEXT=KEYS+CHILDREN
KIND='source-versioned-complete-function-scoping-conjunction'
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','native_OS_input_qualified']

def structure(q,scope,inventory):
 known={r['key']:r for r in inventory['requirements']}
 require(q['contract_kind']==KIND and q['qualified_keys']==KEYS and q['authority_sha256']==inventory['sources'],'Changed scoping credit/authority')
 require(scope['candidate_rows']==[known[k] for k in KEYS] and scope['complete_original_context']==[known[k] for k in CONTEXT] and scope['required_children']==CHILDREN,'Missing original scoping clause')
 require(known[KEYS[0]]['text'].strip().rstrip(':')=='GMAT functions shall support the following scoping requirements' and scope['original_assertions_changed']==0 and scope['failures_waived'] is False and scope['other_row_credit']==0 and scope['newly_resolved_hold_keys']==[],'Overbroad scoping claim')
 require(all(q[k] is False and scope[k] is False for k in GATES),'Broader qualification promoted')

def observed(folder):
 proof=load(folder/'proof.json');logs=[json.loads(l.split('NASA_CHECK ',1)[1]) for l in (folder/'test.log').read_text().splitlines() if re.match(r'^(?:\d+:\s+)?NASA_CHECK ',l)];require(proof in logs and proof['outcome']=='passed','Missing executed scoping proof');return proof

def qualify(repo,path,inventory):
 q=load(path)
 for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed scoping artifact: '+n)
 scope=load(q['scope_review']);structure(q,scope,inventory);frozen={}
 for af,ids in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
  with tarfile.open(q[af]) as bundle:
   require(set(bundle.getnames())==set(q[ids]),'Changed source partition')
   for n,h in q[ids].items():
    raw=bundle.extractfile(n).read();require(hashlib.sha256(raw).hexdigest()==h and not raw.startswith(b'\x7fELF'),'Changed/non-source capsule member')
    if ids=='proof_source_sha256':frozen[n]=raw
 original=load(q['protected_input_baseline']);authorized=load(q['authorized_input_manifest']);guard=load(q['component_input_integrity']);require(len(original)==len(authorized)==1334 and all(q['proof_source_sha256'][n]==h for n,h in original.items()),'Original preimages lost')
 require(sorted(n for n in original if original[n]!=authorized[n])==['src/qtgui/tests/PlotIntegrationStepTests.cpp','src/qtgui/tests/SharedPlaybackTests.cpp'] and guard['authorized_inputs_final']==authorized and guard['changed_by_this_component']==[],'Unapproved protected amendment')
 previous=load(q['prior_full_audit']);require(len(previous['original_hold_keys'])==59 and not set(KEYS)&set(previous['original_hold_keys']),'Formal hold scope changed');children={x['original_source']['key']:x for x in previous['rows']};require(all(children[k]['status']=='verified-source-versioned' for k in CHILDREN),'Unqualified scoping prerequisite')
 expanded=load(q['expanded_scope_summary']);counts={}
 for n,expected in [('expanded-headless-v3',131),('report-truth-headless-v4',1),('expanded-visible-v1',132),('final-detach-headless-v6',7),('final-detach-visible-v2',7)]:
  item=expanded['runs'][n];receipt=repo/item['receipt'];require(sha(receipt)==item['receipt_sha256'],'Wrong actual scope run');d=load(receipt);require(d['end_reason']=='completed-selection' and d['mode']==item['mode'] and len([x for x in d['results'] if x['status']=='passed' and x['returncode']==0])==expected,'Incomplete saved scope case cohort');counts[n]=expected
 antenna=load(q['antenna_component']);require(antenna['passing_GUI_executions']==31 and antenna['passing_native_controls']==1 and antenna['original_inputs_unchanged']==1334,'Missing actual ownership regression evidence')
 agg=Path(q['aggregate_stage']);source=load(agg/'source-hashes-v1.json');require(b'RetireFunctionObjectStore' in frozen['src/base/function/FunctionManager.cpp'] and hashlib.sha256(frozen['src/base/function/FunctionManager.cpp']).hexdigest()==source['src/base/function/FunctionManager.cpp'],'Unbound function retirement owner')
 for name in ['Scalar','ScalarGlobal']:
  p=agg/'headless-all-v2'/('QtGui.NasaMatlabFunctionIo'+name);proof=observed(p);j=list(ET.parse(p/'ctest.xml').iter('testcase'));require(len(j)==1 and all(j[0].find(x) is None for x in ['failure','error','skipped']),'Saved real MATLAB scope did not pass');require(len(proof['phases'])==3 and all(x['inputs']==[2.5,-3,8,4] and x['outputs']==[3*y+1 for y in x['inputs']] and x['function_calls']==3 and x['matlab_calls']==4 for x in proof['phases']),'Saved MATLAB scope independent I/O differs')
  function=(p/'Exercise.gmf').read_text();require(('Global MatIo;' if name=='ScalarGlobal' else 'Create MatlabFunction MatIo;') in function and '[out] = MatIo(input);' in function and '[out] = Exercise(input);' in (p/'NestedExercise.gmf').read_text(),'Wrong actual MATLAB private/shared/nested route')
 s=Path(q['fmincon_scope_stage']);a=s/'headless-v5/PrivateAndShared';plan=load(s/'plan-v5.json');res=load(a/'result.json');proof=observed(a);spec=json.loads(frozen['src/qtgui/tests/FunctionFminconScopeWriteCase.json']);worker=frozen['src/qtgui/tests/FunctionFminconScopeTests.cpp'];require(res['status']=='passed' and res['returncode']==0 and res['signature']['binary_sha256']==plan['binary_sha256'] and res['signature']['source_sha256']==plan['driver_sha256']==hashlib.sha256(worker).hexdigest(),'Wrong actual fmincon scope source/runtime')
 raw=load(a/'native-values.json');require(raw==proof['native'] and raw['PrivateCalls']>=3 and raw['SharedCalls']>=3,'Native optimizer scope evaluations absent');require(spec['expected']=={'PrivateValue':3,'PrivateCost':1,'SharedValue':4,'SharedCost':1,'ObservedLimit':61,'Keep':42} and all(math.isfinite(raw[k]) and abs(raw[k]-v)<=spec['absolute_bounds'][k] for k,v in spec['expected'].items()),'Independent scoped optimizer result differs')
 require(all(proof[k] is True for k in ['private_resource_did_not_escape','shared_resource_retained','callback_retired']) and 'Could not find "LocalOptimizer" in the Sandbox.' in (a/'private-lookup.txt').read_text(),'Optimizer scope/cleanup not proved')
 for name,text in spec['functions'].items():require((a/name).read_text()==text,'Executed function source changed')
 require('Create FminconOptimizer LocalOptimizer;' in spec['functions']['PrivateSolve.gmf'] and 'Global SharedOptimizer;' in spec['functions']['SharedSolve.gmf'] and 'Write SharedOptimizer.MaximumIterations' in spec['functions']['SharedSolve.gmf'],'Missing actual private/shared construction/use')
 require(all(load(s/f'headless-v{i}/PrivateAndShared/result.json')['status']=='failed' for i in range(1,5)),'Earlier new fixture/observer failure waived');identity=load(s/'provider-identity-v1.json');require(identity['status']=='passed' and set(identity['required_libraries'])=={'libeng.so','libMatlabInterface.so.R2026a','libFminconOptimizer.so.R2026a'},'Unbound actual optimizer/provider libraries')
 rejected=0
 for field,value in [('required_children',CHILDREN[:-1]),('other_row_credit',1),('failures_waived',True)]:
  damaged=copy.deepcopy(scope);damaged[field]=value
  try:structure(q,damaged,inventory)
  except ValueError:rejected+=1
  else:raise ValueError('Damaged scoping admission accepted')
 return q,KEYS,{'status':'passed','qualified_keys':KEYS,'whole_row_conjunction':True,'component':{'complete_original_clauses':CHILDREN,'reused_expanded_GUI_passes':264,'reused_affected_detach_passes':14,'reused_antenna_GUI_passes':31,'reused_antenna_native_passes':1,'reused_real_MATLAB_scope_cases':2,'new_real_fmincon_scope_contexts':2,'retained_new_scope_attempt_failures':4,'native_tests_rerun_for_admission':0},'new_requirement_credit':1,'newly_resolved_original_hold_keys':[],'damaged_admission_controls_rejected':rejected,'native_tests_rerun':0,'runtime_binary_copies':0,**{k:False for k in GATES}}

def build(repo, baseline, path, output):
    seal, inventory, keys = prefix.baseline(repo, baseline)
    original = {kind:set(values) for kind,values in keys.items()}
    _, incoming, audit = qualify(repo, path, inventory)
    known = {kind:{row['key']:row for row in inventory[kind]} for kind in keys}
    require(not set(incoming) & keys['requirements'], 'Duplicate scope credit')
    keys['requirements'] = common.merge_keys(keys['requirements'], incoming, known['requirements'])
    coverage = {kind:{'verified':len(keys[kind]), 'total':len(known[kind]),
        'percent':round(100*len(keys[kind])/len(known[kind]),1),
        'new_verified':len(keys[kind])-len(original[kind])} for kind in keys}
    remaining = {kind:[row for row in seal['remaining_original_rows'][kind]
        if row['key'] not in keys[kind]] for kind in keys}
    result = {'status':'passed', 'scope':'Sealed source-versioned prefix plus complete original GMAT function scoping conjunction',
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
                  'Only FRR E491 is added. All other row records and all59 formal hold statuses remain unchanged.',
                  'All nine admitted scoping clauses plus retained lifecycle/resource and real MATLAB/optimizer scope witnesses establish this parent conjunction. All-command, I/O syntax, platform, scientific and full-build scope remain separate.']}
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
