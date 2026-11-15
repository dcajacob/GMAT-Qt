"""Admit four original plot-step/list requirements from complete preserved evidence."""
import argparse,copy,hashlib,json,math,tarfile
from pathlib import Path
import NasaGuiCoverage as nasa
import BuildNasaCoverage95 as builder
import BuildNasaCoverageExtension as common

load,sha,require=common.load,common.sha,common.require
KEYS=['FRR!E329','FRR!E330','FRR!E633','FRR!E634']
CONTEXT=['FRR!E'+str(i) for i in [329,330,331,332,633,634,635,636]]
FIELDS=['IntegrationStepsToSkip','UpdatePlotFrequency','MaxPlotPoints','NumPointsToRedraw']
CASES=['ForwardSplit','BackwardSplit','SingleZero','IntMax','CadenceRetention','RedrawAll','RedrawLastTwo','LegacyEveryPacket']
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified']
KIND='source-versioned-complete-plot-integration-skip-and-data-options'
CLAUSES={'original-complete-parent-child-context','worksheet-skip-versus-compatibility-period',
 'primitive-producer-and-transaction-oracles','all-original-consumer-scenarios',
 'actual-editor-history-and-Unicode-persistence','intermediate-refresh-and-retention',
 'live-final-and-animated-redraw','paired-visible-profiles','preserved-inputs-failures-and-source-versions'}

def structure(q,s,inventory):
 known={x['key']:x for x in inventory['requirements']}
 require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Extra/partial plot credit')
 require(q['authority_sha256']==inventory['sources'],'Original authority changed')
 require(s['candidate_rows']==[known[k] for k in KEYS] and s['complete_original_context']==[known[k] for k in CONTEXT],'Original plot text/context changed')
 require(s['required_fields']==FIELDS and s['plot_types']==['OrbitView','GroundTrack'],'Plot setting/type omitted')
 require(s['worksheet_control']=='IntegrationStepsToSkip' and s['compatibility_control']=='DataCollectFrequency'
  and s['explicit_zero_collects_every_step'] and s['positive_skips_that_many_steps'] and s['legacy_mode']==-1,'Period substituted for skip count')
 require(s['consumer_case_count']==125 and s['paired_gap_cases']==CASES and s['animation_modes']==['PropagationSteps','InterpolatedSteps'],'Consumer/profile/animation universe narrowed')
 require(s['newly_resolved_hold_keys']==[] and s['implemented_restoration'] and not s['administrative_retirement'],'Original holds retired instead of implemented')
 require(set(s['clauses'])==CLAUSES and all(x['satisfied'] and x['evidence'] for x in s['clauses'].values()),'Plot conjunction incomplete')
 require(s['original_assertions_changed']==0 and s['failed_runs_retained'] and not s['unfiltered_registered_test_passed']
  and not s['unfiltered_failure_waived'] and s['runtime_binary_copies']==0 and s['other_row_credit']==0,'Input/failure/credit fidelity lost')
 require(all(q[k] is False and s[k] is False for k in GATES),'Wider plot approval promoted')

def damaged(q,s,inventory):
 rejected=[]
 for fault in ['extra-row','fingerprint','missing-parent-child','missing-field','period-for-skip','missing-consumer','missing-animation','waived-failure','changed-assertion','wider-approval']:
  bq,bs=copy.deepcopy(q),copy.deepcopy(s)
  if fault=='extra-row':bq['qualified_keys'].append('FRR!E636')
  elif fault=='fingerprint':bs['candidate_rows'][0]['fingerprint']='0'*64
  elif fault=='missing-parent-child':bs['complete_original_context'].pop()
  elif fault=='missing-field':bs['required_fields'].pop()
  elif fault=='period-for-skip':bs['worksheet_control']='DataCollectFrequency'
  elif fault=='missing-consumer':bs['consumer_case_count']=124
  elif fault=='missing-animation':bs['animation_modes'].pop()
  elif fault=='waived-failure':bs['unfiltered_failure_waived']=True
  elif fault=='changed-assertion':bs['original_assertions_changed']=1
  else:bq['technical_core_approval']=True
  try:structure(bq,bs,inventory)
  except (ValueError,AssertionError):rejected.append(fault)
  else:raise ValueError('Damaged plot admission accepted: '+fault)
 return rejected

def circle(t):
 w=math.sqrt(398600.4415/7000.**3);a=w*t
 return [7000*math.cos(a),7000*math.sin(a),0.,-7000*w*math.sin(a),7000*w*math.cos(a),0.]

def profile(stage,names,headless,visible):
 profiles=[load(stage/headless/'results.json'),load(stage/visible/'results.json')];exposures=0;selected=[]
 require(profiles[0]['source_hashes']==profiles[1]['source_hashes'] and profiles[0]['binary_hashes']==profiles[1]['binary_hashes'],'Paired plot sources/runtime differ')
 for index,p in enumerate(profiles):
  cases=[x for x in p['results'] if x['status']!='not-run']
  require(p['end_reason']=='completed-selection' and {x['name'] for x in cases}==set(names) and len(cases)==len(names)
   and all(x['status']=='passed' and x['returncode']==0 for x in cases),'Plot profile incomplete')
  if index:
   d=p['visible_display'];require(d['start_minimized'] and d['resizable'] and not d['resize_errors']
    and 'Iconic' in d['initial_host_window_state'] and 'FULLSCREEN' not in d['initial_host_window_state'],'Visible plot desktop policy changed')
  for c in cases:
   require(c['requested_visibility']==('visible' if index else 'headless') and not c['trace']['screenshots'] and c['trace']['instrumented_processes']==1,'Plot trace/visibility lost')
   if index:require(c['trace']['native_exposure_events']>0,'Plot never exposed natively');exposures+=c['trace']['native_exposure_events']
   selected.append((index,c,Path(c['log']).parent))
 return profiles,selected,exposures

def component(q):
 old=Path(q['consumer_stage']);gap=Path(q['profile_gap_stage']);playback=Path(q['playback_stage'])
 for stage,name,field in [(old,'publication-v1.json','artifacts'),(gap,'profile-gap-publication-v1.json','artifact_sha256'),(playback,'publication-v1.json','artifacts')]:
  for n,h in load(stage/name)[field].items():
   path=Path(n);path=path if path.is_absolute() else (Path(q['repo'])/path if field=='artifact_sha256' else stage/path)
   require(sha(path)==h,'Changed plot component artifact: '+str(path))
 matrix=load(old/'consumer-case-results-v1.json');require(matrix['case_proof_count']==125 and len(matrix['current_runtime_case_proofs'])==125
  and not matrix['unfiltered_test_failure_waived'],'Original consumer inventory/failure altered')
 for name,row in matrix['current_runtime_case_proofs'].items():
  p=old/row['proof'];d=load(p);require(sha(p)==row['sha256'] and d['case']==name and d['outcome']=='passed','Consumer case missing: '+name)
 full=load(old/'final-matrix-v6-results.json');require(len(full['results'])==11
  and sum(x['returncode']==0 for x in full['results'])==10
  and next(x for x in full['results'] if x['name']=='QtGui.PlotIntegrationSteps')['returncode']==1,'Original unfiltered failure concealed')
 producer=load(old/'final-matrix-v6/QtGui.StepPublication/proof.json')
 require(producer['outcome']=='passed' and len(producer['cases'])==8 and all(x['unchanged_legacy_bytes'] for x in producer['cases']),'Committed producer/compatibility proof lost')
 transaction=load(old/'final-matrix-v6/QtGui.AcceptedStepTransactions/proof.json')
 require(transaction['outcome']=='passed' and transaction['all_eight_methods_exercised'] and transaction['nested_discard_and_commit']
  and transaction['thread_isolated'] and transaction['no_publication_or_live_spacecraft_mutation'] and len(transaction['native_cases'])==16,'Primitive transaction conjunction lost')
 scalars=0
 for cell in transaction['native_cases']:
  require(len(cell['primitive_steps'])==10 and cell['raw_publications']==0,'Primitive steps replaced by outer-call counts')
  direction=-1 if cell['case'].endswith('Backward') else 1;step=cell['primitive_step_seconds']
  for i,row in enumerate(cell['primitive_steps']):
   require(abs(row['start']-direction*step*(i+1))<1e-8 and abs(row['end']-direction*step*(i+2))<1e-8,'Primitive endpoint grid changed')
   require(len(row['state'])==6 and all(math.isfinite(a) and abs(a-b)<(3e-5 if j<3 else 4e-8)
    for j,(a,b) in enumerate(zip(row['state'],circle(row['end'])))),'Primitive offline circle oracle failed');scalars+=6
 require(scalars==transaction['independent_state_values']==960,'Primitive oracle scope narrowed')
 gp,gcases,exposure=profile(gap,['QtGui.PlotIntegrationSteps'],'headless-gap-v1','visible-gap-v1');samples=0
 expected={'ForwardSplit':[0,20,37],'BackwardSplit':[0,-20,-37],'SingleZero':[0,10,20],'IntMax':[0]}
 for _,case,directory in gcases:
  proof=load(directory/'proof.json');require(proof['outcome']=='passed' and {x['case'] for x in proof['cases']}==set(CASES),'Paired skip case partition missing')
  for name,times in expected.items():
   d=next(x for x in proof['cases'] if x['case']==name)
   for plot in ['Orb','Map']:
    rows=next(x['points'] for x in d['outputs'] if x['object']=='One' and x['plot']==plot)
    require(len(rows)==len(times) and all(abs(x['elapsed']-t)<1e-4 for x,t in zip(rows,times)),'Zero/positive/boundary skip grid differs')
    if plot=='Orb':
     for x in rows:require(all(abs(x[k]-v)<4e-5 for k,v in zip(['x','y','z'],circle(x['elapsed'])[:3])),'Consumer circle oracle failed')
  observation=load(directory/'CadenceRetention-observations.json');raw=[]
  for row in observation['samples']:
   step=row['raw_step'];collected=1+step//2
   require(row['expected_collected']==collected,'Skip count replaced by publication period')
   if not raw or raw[-1]!=step:raw.append(step)
   for model in row['models']:
    points=model['retained'];count=min(10,collected)
    require(model['frame']==collected and len(points)==count and all(abs(x['seconds']-20*(collected-count+i))<1e-4 for i,x in enumerate(points)),'Intermediate selected history/retention differs')
   samples+=1
  require(raw==list(range(61)),'Raw integration-step grid narrowed')
  for plot,frequency in [('Orb',3),('Map',4)]:
   actual=[x['frame'] for x in observation['regular_refreshes'] if x['plot']==plot]
   require(actual==list(range(frequency,32,frequency)),'Forced/final redraw substituted for intermediate cadence')
 pp,pcases,pexposure=profile(playback,['QtGui.PlotPlaybackRedrawOrbitView','QtGui.PlotPlaybackRedrawGroundTrack'],'headless-v5','visible-v5');pixels=0
 for _,case,directory in pcases:
  d=load(directory/'proof.json');require(d['outcome']=='passed' and d['retained_points']==6 and d['redraw_segments']==2
   and d['modes']==['PropagationSteps','InterpolatedSteps'] and d['pixel_observations']==6 and d['actual_transport_buttons'] and d['retained_history_unchanged'],'Playback conjunction missing')
  require({'plotReplayStart','plotReplayPlay','plotReplayLatest','plotReplayMode'}<=set(case['trace']['named_objects']),'Actual playback controls unobserved')
  rows=load(directory/'playback-pixels.json');require(len(rows)==6 and {(x['mode'],x['phase'],x.get('timeline')) for x in rows}==
   {(m,p,t) for m in d['modes'] for p,t in [('paused-history',500),('paused-history',700),('latest-complete',None)]},'Playback mode/position partition narrowed')
  for row in rows:
   wanted=5 if row['phase']=='latest-complete' else 2
   require(row['expected_full_segments']==wanted and row['interpolated']==(row['mode']=='InterpolatedSteps' and row['phase']=='paused-history'),'Live/final/animated clipping conflated')
   controls=row['alternatives'];require(any(x['segments']==wanted and x['different_pixels']==0 for x in controls)
    and any(x['segments']!=wanted for x in controls) and all(x['different_pixels']>=5 for x in controls if x['segments']!=wanted),'Rendered playback count/negative controls differ');pixels+=1
 require(pixels==24 and load(playback/'retained-failures-v1.json')['original_assertions_changed']==0,'Playback observations/failure fidelity lost')
 return dict(reused_consumer_cases=125,producer_cases=8,transaction_cases=16,independent_transaction_scalars=scalars,
  paired_skip_cases=16,intermediate_cadence_observations=samples,playback_registered_passes=4,playback_pixel_observations=pixels,
  native_visible_exposures=exposure+pexposure,unfiltered_original_registered_test_passed=False)

def qualify(repo,path,inventory):
 q=load(path)
 for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed plot admission artifact: '+n)
 scope=load(q['scope_review']);structure(q,scope,inventory)
 for a,i in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
  require(q[a] in q['artifact_sha256'],'Unbound plot source archive')
  with tarfile.open(q[a]) as b:
   require(set(b.getnames())==set(q[i]),'Plot source partition differs')
   for n,h in q[i].items():require(hashlib.sha256(b.extractfile(n).read()).hexdigest()==h,'Plot source bytes differ')
 original=load(q['protected_input_baseline']);require(len(original)==1334 and all(q['proof_source_sha256'].get(n)==h for n,h in original.items()),'Protected original plot inputs omitted')
 for group,bindings in q['executed_source_bindings'].items():
  expected=load(q['executed_source_hash_files'][group])
  if 'source_hashes' in expected:expected=expected['source_hashes']
  expected={n:h for n,h in expected.items() if not n.startswith('build/')}
  require(set(bindings)==set(expected) and all(q['proof_source_sha256'].get(member)==expected[n] for n,member in bindings.items()),'Executed plot source version omitted: '+group)
 prior=load(q['prior_overlay']);prior_rows={x['key']:x for x in prior['reviewed_source_rows']};require(len(prior['original_hold_keys_retained'])==59 and not set(KEYS)&set(prior['original_hold_keys_retained']) and all(prior_rows[k]['whole_row_status']=='open' and prior_rows[k]['closure_track']=='plot-whole-row-conjunction' for k in KEYS),'Original plot gap/hold inventory changed')
 rows={x['original_source']['key']:x for x in load(q['prior_full_audit'])['rows']}
 require(all(rows[k]['status']=='verified-source-versioned' for k in set(CONTEXT)-set(KEYS)),'Original parent child dependency unqualified')
 proof=component(q);require(len(damaged(q,scope,inventory))==10,'Damaged plot controls incomplete')
 return q,KEYS,dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,component=proof,newly_resolved_original_hold_keys=[],
  new_requirement_credit=4,damaged_admission_controls_rejected=10,native_tests_rerun=0,runtime_binary_copies=0,**{k:False for k in GATES})

def build(repo,baseline,path,output):
    seal,inventory,keys=builder.baseline(repo,baseline);original={k:set(v) for k,v in keys.items()}
    _,incoming,audit=qualify(repo,path,inventory);known={k:{r['key']:r for r in inventory[k]} for k in keys}
    require(not set(incoming)&keys['requirements'],'Duplicate plot-step credit')
    keys['requirements']=common.merge_keys(keys['requirements'],incoming,known['requirements'])
    coverage={k:dict(verified=len(keys[k]),total=len(known[k]),percent=round(100*len(keys[k])/len(known[k]),1),new_verified=len(keys[k])-len(original[k])) for k in keys}
    remaining={k:[r for r in seal['remaining_original_rows'][k] if r['key'] not in keys[k]] for k in keys}
    result=dict(status='passed',scope='95-percent campaign: sealed source-versioned prefix plus complete original plot skip-count/data-option conjunction.',
        baseline=str(baseline),baseline_sha256=sha(baseline),authority_sha256=inventory['sources'],coverage=coverage,
        target_requirements=1101,target_reached=len(keys['requirements'])>=1101,requirements_still_needed=max(0,1101-len(keys['requirements'])),
        additions=[dict(key=k,kind='requirements',fingerprint=known['requirements'][k]['fingerprint'],source_text=known['requirements'][k]['text'],qualification=str(path),qualification_sha256=sha(path),status='verified-source-versioned') for k in incoming],
        remaining_counts={k:len(v) for k,v in remaining.items()},remaining_original_rows=remaining,
        technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,
        limits=['Original 1158/2367 denominators and source row identities are unchanged.','Four original open plot rows are implemented and proved; all59 separate original hold identities and statuses remain unchanged.','Scientific, physical ephemeris/frame, external ABI, platform/native OS input and full current-build approvals remain separate.'])
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
