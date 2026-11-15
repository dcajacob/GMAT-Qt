"""Admit the NonlinearConstraint command with the user's editorial interpretation.

Replay accepted scalar/category evidence at its executed versions. No native
mission or GUI is launched by this reader. Original wording/fingerprint survives.
"""
import argparse,copy,hashlib,json,tarfile
from pathlib import Path
import NasaGuiCoverage as nasa
import BuildNasaCoverageExtension as common
import AuditNasaCoveragePrefix as prefix
import AuditConstraintScalarAcceptance as scalar
sha,load,require=common.sha,common.load,common.require
KEYS=['FRC!E103']
KIND='source-versioned-nonlinear-constraint-command-editorial-interpretation'
CONTEXT=[f'FRC!E{i}' for i in range(98,108)]
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','native_OS_input_qualified']

def structure(q,scope,inventory):
 known={r['key']:r for r in inventory['requirements']}
 require(q['contract_kind']==KIND and q['qualified_keys']==KEYS and q['authority_sha256']==inventory['sources'],'Different command/original authority')
 require(scope['candidate_rows']==[known[k] for k in KEYS] and scope['complete_original_context']==[known[k] for k in CONTEXT],'Changed original command/fingerprint/context')
 require(scope['required_children']==['FRC!E104','FRC!E105','FRC!E106','FRC!E107'] and scope['interpretation']=='constraints-in-Optimize' and scope['original_assertions_changed']==0 and scope['failures_waived'] is False and scope['other_row_credit']==0,'Incomplete or altered command scope')
 require(all(q[k] is False and scope[k] is False for k in GATES),'Broader qualification promoted')

def qualify(repo,path,inventory):
 q=load(path)
 for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed command proof: '+n)
 scope=load(q['scope_review']);structure(q,scope,inventory)
 decision=load(q['decision']);require(decision['FRC!E103']['user_answer']=='Treat it as an editorial error' and decision['FRC!E103']['original_source_fingerprint_preserved'] is True,'Editorial decision missing/changed')
 for af,identities in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
  with tarfile.open(q[af]) as bundle:
   require(set(bundle.getnames())==set(q[identities]),'Changed source partition')
   for n,h in q[identities].items():
    raw=bundle.extractfile(n).read();require(hashlib.sha256(raw).hexdigest()==h and not raw.startswith(b'\x7fELF'),'Changed/non-source capsule member')
 original=load(q['protected_input_baseline']);authorized=load(q['authorized_input_manifest']);guard=load(q['component_input_integrity']);require(len(original)==len(authorized)==1334 and all(q['proof_source_sha256'][n]==h for n,h in original.items()),'Original preimages lost')
 require(sorted(n for n in original if original[n]!=authorized[n])==['src/qtgui/tests/PlotIntegrationStepTests.cpp','src/qtgui/tests/SharedPlaybackTests.cpp'] and guard['authorized_inputs_final']==authorized and guard['changed_by_this_component']==[],'Unapproved protected amendment')
 previous=load(q['prior_full_audit']);require(len(previous['original_hold_keys'])==59 and not set(KEYS)&set(previous['original_hold_keys']),'Formal hold scope changed')
 children={x['original_source']['key']:x for x in previous['rows']}
 require(all(children[k]['status']=='verified-source-versioned' for k in scope['required_children']),'Unqualified operand prerequisite')
 sq,keys=scalar.qualify(repo,Path(q['scalar_child_qualification']),inventory)
 require(keys==['FRC!E104','FRC!E105'] and all(n in q['artifact_sha256'] and q['artifact_sha256'][n]==h for n,h in sq['artifact_sha256'].items()),'Scalar child evidence omitted from new immutable stage')
 # The accepted full-family reader checks actual comparison strings, both GUI
 # operand roles, independent physical values and the active constrained optimum.
 # Its original three-category cohorts also replay Variable/Array/X witnesses.
 fixture=load(sq['fixture']);required_types=set(fixture['unique_scalar_types']);require(len(required_types)==97,'Scalar operand family incomplete')
 rejected=0
 for field,value in [('qualified_keys',KEYS+['FRC!E98']),('scientific_approval',True)]:
  damaged=copy.deepcopy(q);damaged[field]=value
  try:structure(damaged,scope,inventory)
  except ValueError:rejected+=1
  else:raise ValueError('Damaged command admission accepted')
 for field,value in [('interpretation','cost-function'),('failures_waived',True),('required_children',['FRC!E104'])]:
  damaged=copy.deepcopy(scope);damaged[field]=value
  try:structure(q,damaged,inventory)
  except ValueError:rejected+=1
  else:raise ValueError('Incomplete command scope accepted')
 return q,KEYS,{'status':'passed','qualified_keys':KEYS,'whole_row_conjunction':True,'component':{'editorial_interpretation':'constraints-in-Optimize','reused_GUI_scalar_executions':324,'scalar_types':97,'active_left_right_constrained_optima':324,'reused_original_X_Variable_Array_cohorts':2,'required_children':scope['required_children'],'original_assertions_changed':0,'native_tests_rerun_for_admission':0},'new_requirement_credit':1,'newly_resolved_original_hold_keys':[],'damaged_admission_controls_rejected':rejected,'native_tests_rerun':0,'runtime_binary_copies':0,**{k:False for k in GATES}}

def build(repo, baseline, path, output):
    seal, inventory, keys = prefix.baseline(repo, baseline)
    original = {kind:set(values) for kind,values in keys.items()}
    _, incoming, audit = qualify(repo, path, inventory)
    known = {kind:{row['key']:row for row in inventory[kind]} for kind in keys}
    require(not set(incoming) & keys['requirements'], 'Duplicate command credit')
    keys['requirements'] = common.merge_keys(keys['requirements'], incoming, known['requirements'])
    coverage = {kind:{'verified':len(keys[kind]), 'total':len(known[kind]),
        'percent':round(100*len(keys[kind])/len(known[kind]),1),
        'new_verified':len(keys[kind])-len(original[kind])} for kind in keys}
    remaining = {kind:[row for row in seal['remaining_original_rows'][kind]
        if row['key'] not in keys[kind]] for kind in keys}
    result = {'status':'passed', 'scope':'Sealed source-versioned prefix plus complete original NonlinearConstraint command under the user-approved editorial interpretation',
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
                  'Only FRC E103 is added. All other row records and all59 formal hold statuses remain unchanged.',
                  'Existing independent scalar/category passes prove actual constraints in Optimize. Original historical wording remains verbatim, with the user-approved editorial interpretation recorded separately. Scientific, native OS input, platform and full-build approval remain separate.']}
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
