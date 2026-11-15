"""Admit only the complete original Bulirsch additional-setting/floor conjunction."""
import argparse,copy,hashlib,json,math,tarfile
from pathlib import Path
import NasaGuiCoverage as nasa
import BuildNasaCoverage95 as builder
import BuildNasaCoverageExtension as common
load,sha,require=common.load,common.sha,common.require
KEYS=['FRR!E170','FRR!E173'];CONTEXT=['FRR!E'+str(i) for i in range(170,174)]
FIELDS=['MinimumReduction','MaximumReduction','MinimumTolerance']
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified']
KIND='source-versioned-complete-Bulirsch-additional-settings-and-tolerance-floor'
CLAUSES={'complete-original-parent-child-context','all-three-actual-GUI-settings','draft-history-persistence-and-invalid-atomicity',
 'owned-nested-direct-and-runtime-clones','disabled-below-request-cold-and-initialized-floor',
 'independent-forward-backward-native-endpoints','paired-visible-profile','preserved-original-conflict-inputs-and-source-versions'}
NUMERIC=['disabled','below-request','raised-initialized','effective-initialized','cold-floor','cold-effective']

def structure(q,s,inventory):
 known={x['key']:x for x in inventory['requirements']}
 require(q['contract_kind']==KIND and q['qualified_keys']==KEYS and q['authority_sha256']==inventory['sources'],'Extra/partial/unbound Bulirsch credit')
 require(s['candidate_rows']==[known[k] for k in KEYS] and s['complete_original_context']==[known[k] for k in CONTEXT],'Original Bulirsch row/context changed')
 require(s['required_fields']==FIELDS and s['directions']==['Forward','Backward'] and s['profiles']==['headless','visible'],'Bulirsch field/direction/profile omitted')
 require(s['effective_tolerance']=='max(Accuracy,MinimumTolerance)' and s['zero_disables_floor'] and s['Accuracy_retains_requested_value'],'Operational floor semantics changed')
 require(s['numerical_cases']==NUMERIC and s['newly_resolved_hold_keys']==[],'Numerical scope or separate hold status changed')
 require(set(s['clauses'])==CLAUSES and all(x['satisfied'] and x['evidence'] for x in s['clauses'].values()),'Bulirsch conjunction incomplete')
 require(s['original_assertions_changed']==0 and not s['original_BulirschControls_passed'] and not s['original_failure_waived']
  and s['original_failure_retained'] and s['runtime_binary_copies']==0 and s['other_row_credit']==0,'Original conflict/input fidelity lost')
 require(all(q[k] is False and s[k] is False for k in GATES),'Wider Bulirsch approval promoted')

def damaged(q,s,inventory):
 rejected=[]
 for fault in ['extra-row','fingerprint','missing-child','missing-field','missing-direction','missing-profile','wrong-effective-rule','missing-cold-case','waived-failure','wider-approval']:
  a,b=copy.deepcopy(q),copy.deepcopy(s)
  if fault=='extra-row':a['qualified_keys'].append('FRR!E171')
  elif fault=='fingerprint':b['candidate_rows'][0]['fingerprint']='0'*64
  elif fault=='missing-child':b['complete_original_context'].pop()
  elif fault=='missing-field':b['required_fields'].pop()
  elif fault=='missing-direction':b['directions'].pop()
  elif fault=='missing-profile':b['profiles'].pop()
  elif fault=='wrong-effective-rule':b['effective_tolerance']='min(Accuracy,MinimumTolerance)'
  elif fault=='missing-cold-case':b['numerical_cases'].pop()
  elif fault=='waived-failure':b['original_failure_waived']=True
  else:a['technical_core_approval']=True
  try:structure(a,b,inventory)
  except (ValueError,AssertionError):rejected.append(fault)
  else:raise ValueError('Damaged Bulirsch admission accepted: '+fault)
 return rejected

def component(q):
 stage=Path(q['component_stage']);pub=load(stage/'publication-v1.json')
 for n,h in pub['artifacts'].items():require(sha(stage/n)==h,'Changed Bulirsch artifact: '+n)
 initial=load(stage/'initial-review-v1.json')
 for n,h in initial['existing_artifacts_sha256'].items():require(sha(n)==h,'Changed prior Bulirsch support: '+n)
 require(initial['existing_proof']['status']=='passed' and not initial['existing_proof']['whole_row_qualified'],'Prior component promoted to whole row')
 profiles=[load(stage/'headless-v1/results.json'),load(stage/'visible-v1/results.json')]
 require(profiles[0]['source_hashes']==profiles[1]['source_hashes'] and profiles[0]['binary_hashes']==profiles[1]['binary_hashes'],'Bulirsch profile sources/runtime differ')
 names={'QtGui.ImplementationBulirschFloor','QtGui.BulirschControls','QtGui.BulirschFloorAcceptanceForward','QtGui.BulirschFloorAcceptanceBackward'}
 vectors=0;maxpos=0.;maxvel=0.;exposures=0
 for index,profile in enumerate(profiles):
  actual=[x for x in profile['results'] if x['status']!='not-run']
  require(profile['end_reason']=='completed-selection' and len(actual)==4 and {x['name'] for x in actual}==names,'Incomplete Bulirsch profile')
  if index:
   d=profile['visible_display'];require(d['start_minimized'] and d['resizable'] and not d['resize_errors'] and 'Iconic' in d['initial_host_window_state']
    and 'FULLSCREEN' not in d['initial_host_window_state'],'Visible Bulirsch desktop policy changed')
  for case in actual:
   directory=Path(case['log']).parent;name=case['name']
   require(case['requested_visibility']==('visible' if index else 'headless') and not case['trace']['screenshots'] and case['trace']['instrumented_processes']==1,'Bulirsch visibility/trace lost')
   if index:require(case['trace']['native_exposure_events']>0,'Bulirsch visible case never exposed');exposures+=case['trace']['native_exposure_events']
   if name=='QtGui.BulirschControls':
    text=Path(case['log']).read_text();require(case['status']=='failed' and case['returncode']==8 and 'Test process exit: 1 (0x00000001)' in text
     and 'Writable reduction fields missing or deprecated MinimumTolerance exposed' in text,'Original restored-field conflict concealed/changed');continue
   require(case['status']=='passed' and case['returncode']==0,'Required Bulirsch positive case failed')
   if name=='QtGui.ImplementationBulirschFloor':
    d=load(directory/'verification.json');require(d['status']=='passed' and d['case']=='BulirschFloor' and d['new_requirement_credit']==0
     and d['observations']==[{'accuracy_requested':1e-12,'cloned_and_executed':True,'minimum_tolerance':1e-8}],'Original implementation probe lost');continue
   d=load(directory/'proof.json');direction=-1 if name.endswith('Backward') else 1
   require(d['outcome']=='passed' and d['case']==('Backward' if direction<0 else 'Forward') and d['required_fields']==FIELDS
    and d['invalid_GUI_rejections']==3 and d['numerical_cases']==6,'Required Bulirsch field/direction/invalid partition missing')
   require(all(d[k] for k in ['GUI_type_draft_discard','GUI_apply_undo_redo_unicode','nested_direct_clones','initialized_and_cold_equivalence',
    'strict_below_floor_equivalence','observable_operational_floor_effect']),'GUI/lifecycle/operational floor conjunction missing')
   require({'Resources','resource_MinimumReduction','resource_MaximumReduction','resource_MinimumTolerance','resource_Type'}<=set(case['trace']['named_objects']),'Actual Bulirsch editor fields unobserved')
   matrix=load(directory/'numerical-matrix.json');require([x['case'] for x in matrix]==NUMERIC,'Cold/live/disabled/below-request universe narrowed')
   for cell,(request,floor) in zip(matrix,[(1e-12,0),(1e-12,1e-15),(1e-12,1e-8),(1e-8,0),(1e-12,1e-8),(1e-8,0)]):
    require(cell['requested_accuracy']==request and cell['floor']==floor and cell['configured_before_initialization']==cell['case'].startswith('cold'),'Floor input profile changed')
    steps=cell['steps'];require(len(steps)==(3 if cell['case'] in NUMERIC[:2] else 2) and abs(steps[0]['start']-direction*10)<1e-6
     and abs(steps[-1]['end']-direction*1810)<1e-6,'Primitive integration interval/scope changed')
    for i,row in enumerate(steps):
     require(direction*(row['end']-row['start'])>0 and (i==0 or abs(row['start']-steps[i-1]['end'])<1e-8),'Primitive endpoints disconnected/reordered')
     w=math.sqrt(398600.4415/7000.**3);a=w*row['end'];expected=[7000*math.cos(a),7000*math.sin(a),0.,-7000*w*math.sin(a),7000*w*math.cos(a),0.]
     require(len(row['state'])==len(row['expected'])==6,'Bulirsch Cartesian vector narrowed')
     for c,(actual,truth) in enumerate(zip(row['state'],expected)):
      require(abs(row['expected'][c]-truth)<1e-9 and math.isfinite(actual) and abs(actual-truth)<(1e-3 if c<3 else 1e-6),'Independent offline Bulirsch orbit failed')
      if c<3:maxpos=max(maxpos,abs(actual-truth))
      else:maxvel=max(maxvel,abs(actual-truth))
     vectors+=1
   require(matrix[0]['steps']==matrix[1]['steps'] and matrix[2]['steps']==matrix[3]['steps'] and matrix[4]['steps']==matrix[5]['steps']
    and matrix[0]['steps']!=matrix[2]['steps'],'Operational max rule or independent control effect failed')
 require(vectors==56,'Bulirsch independent state universe narrowed')
 conflict=load(stage/'retained-conflicts-v1.json');require(not conflict['failure_waived'] and not conflict['full_original_regression_passed']
  and conflict['original_assertions_changed']==0 and conflict['CTest_returncodes']==[8,8] and conflict['child_returncodes']==[1,1],'Original conflict fidelity lost')
 return dict(registered_passes=6,retained_original_conflicts=2,numerical_scenarios=24,independent_state_vectors=vectors,independent_scalar_comparisons=vectors*6,
  max_position_error_km=maxpos,max_velocity_error_km_s=maxvel,native_visible_exposures=exposures,original_BulirschControls_passed=False)

def qualify(repo,path,inventory):
 q=load(path)
 for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed Bulirsch admission artifact: '+n)
 s=load(q['scope_review']);structure(q,s,inventory)
 for a,i in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
  require(q[a] in q['artifact_sha256'],'Unbound Bulirsch source archive')
  with tarfile.open(q[a]) as b:
   require(set(b.getnames())==set(q[i]),'Bulirsch source partition differs')
   for n,h in q[i].items():require(hashlib.sha256(b.extractfile(n).read()).hexdigest()==h,'Bulirsch source bytes differ')
 original=load(q['protected_input_baseline']);require(len(original)==1334 and all(q['proof_source_sha256'].get(n)==h for n,h in original.items()),'Protected original inputs omitted')
 prior=load(q['prior_overlay']);rows={x['key']:x for x in prior['reviewed_source_rows']};require(len(prior['original_hold_keys_retained'])==59
  and not set(KEYS)&set(prior['original_hold_keys_retained']) and all(rows[k]['whole_row_status']=='open' for k in KEYS),'Original Bulirsch gap/hold inventory changed')
 full={x['original_source']['key']:x for x in load(q['prior_full_audit'])['rows']};require(all(full[k]['status']=='verified-source-versioned' for k in ['FRR!E171','FRR!E172']),'Reduction child dependencies unqualified')
 proof=component(q);require(len(damaged(q,s,inventory))==10,'Damaged Bulirsch controls incomplete')
 return q,KEYS,dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,component=proof,newly_resolved_original_hold_keys=[],new_requirement_credit=2,
  damaged_admission_controls_rejected=10,native_tests_rerun=0,runtime_binary_copies=0,**{k:False for k in GATES})

def build(repo,baseline,path,output):
    seal,inventory,keys=builder.baseline(repo,baseline);original={k:set(v) for k,v in keys.items()}
    _,incoming,audit=qualify(repo,path,inventory);known={k:{r['key']:r for r in inventory[k]} for k in keys}
    require(not set(incoming)&keys['requirements'],'Duplicate Bulirsch credit')
    keys['requirements']=common.merge_keys(keys['requirements'],incoming,known['requirements'])
    coverage={k:dict(verified=len(keys[k]),total=len(known[k]),percent=round(100*len(keys[k])/len(known[k]),1),new_verified=len(keys[k])-len(original[k])) for k in keys}
    remaining={k:[r for r in seal['remaining_original_rows'][k] if r['key'] not in keys[k]] for k in keys}
    result=dict(status='passed',scope='95-percent campaign: sealed source-versioned prefix plus complete original Bulirsch additional-setting/floor conjunction.',
        baseline=str(baseline),baseline_sha256=sha(baseline),authority_sha256=inventory['sources'],coverage=coverage,
        target_requirements=1101,target_reached=len(keys['requirements'])>=1101,requirements_still_needed=max(0,1101-len(keys['requirements'])),
        additions=[dict(key=k,kind='requirements',fingerprint=known['requirements'][k]['fingerprint'],source_text=known['requirements'][k]['text'],qualification=str(path),qualification_sha256=sha(path),status='verified-source-versioned') for k in incoming],
        remaining_counts={k:len(v) for k,v in remaining.items()},remaining_original_rows=remaining,
        technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,
        limits=['Original 1158/2367 denominators and source row identities are unchanged.','Two original open Bulirsch rows are implemented and proved; all59 separate original hold identities and statuses remain unchanged.','Scientific, physical ephemeris/frame, external ABI, platform/native OS input and full current-build approvals remain separate.'])
    with Path(output).open('x') as f:json.dump(result,f,indent=2);f.write('\n')
    return result,audit


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);p.add_argument('--baseline',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();repo=a.repo.resolve()
    if a.baseline:
        result,_=build(repo,a.baseline,a.qualification,a.output);print(json.dumps({'coverage':result['coverage'],'requirements_still_needed':result['requirements_still_needed']}))
    else:
        _,_,result=qualify(repo,a.qualification,nasa.inventory(repo))
        with a.output.open('x') as f:json.dump(result,f,indent=2);f.write('\n')
        print(json.dumps(result))
