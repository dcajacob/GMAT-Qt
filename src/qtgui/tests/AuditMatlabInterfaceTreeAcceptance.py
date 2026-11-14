"""Audit the exact runtime Resource Tree MATLAB interface parent and child."""
import argparse,copy,hashlib,json,math,re,sys
from pathlib import Path
import NasaGuiCoverage as nasa
import AuditFminconIterationAcceptance as native
import BuildNasaCoverageExtension as common
sha,load,require=common.sha,common.load,common.require
KIND='source-versioned-Resource-Tree-MATLAB-interface-open-close'
KEYS=['All Others!E154','All Others!E155']
OPERATIONS=['Cancel','Close','Open','Open','Open','Close','Close','Open','Open','Close']

def check(proof,profile,directory=None):
 require(proof['new_requirement_credit']==0,'Automatic interface credit')
 if profile=='Unavailable':
  require(proof['checkpoint']=='matlab-interface-tree-unavailable' and proof['actual_provider_unregistered'] is True
   and proof['runtime_node_absent'] is True and proof['ordinary_mission_result']==42,'Unavailable actual-provider guard missing')
 else:
  require(proof['checkpoint']=='matlab-interface-tree-available' and [a['operation'] for a in proof['actions']]==OPERATIONS
   and all(a['actual_popup'] is True and a['source_unchanged'] is True and a['history_unchanged'] is True for a in proof['actions']),'Actual Open/Close/cancel/lifetime conjunction differs')
  pids=proof['actual_MATLAB_pids'];require(len(pids)==2 and pids[0]!=pids[1] and all(type(p) in [int,float] and math.isfinite(p) and p>1 and p==int(p) for p in pids),'Actual closed/reopened MATLAB processes differ')
  require(proof['busy_reentry_checks']>=3 and proof['actual_MATLAB_call_result']==130 and all(proof[k] is True for k in
   ['same_engine_workspace_used','actual_ReportViewer','Unicode_reopen','Engine_closed']),'Real Engine/mission/busy/history/reopen proof missing')
  if directory:require(load(directory/'actions.json')==proof['actions'] and (directory/'report.csv').read_text()=='130\n','Raw interface actions/report differ')
 if directory:
  cap=load(directory/'capability.json');require(cap['Matlab_available'] is (profile=='Available') and cap['native_interface_present'] is (profile=='Available'),'Actual provider capability differs')
  entries=[e for e in cap['children'] if e['runtime_kind']=='matlab-interface'];require(len(entries)==int(profile=='Available') and all(not e['configured_name'] for e in entries),'Runtime interface/configured namespace partition differs')
  require('% Preserve interface caller tail Ω' in (directory/'caller.script').read_text(),'Unicode interface caller tail missing')

class Evidence(native.Evidence):
 def context(self,name,version,mode,test,role,argument=None):
  # Preserve the individually passing Available case in its original mixed
  # batch; independently validate the entire receipt/maps/source preamble.
  if name=='implemented-headless-v1' and name not in self.cache:
   receipt=load(self.stage/name/'results.json');mapped=load(self.stage/(name+'-loaded-images.json'))
   ids=load(self.stage/f'executed-source-sha256-v{version}.json');self.source(version)
   failures=[r for r in receipt['results'] if r['status'] not in ['not-run','passed']]
   require(mode=='headless' and receipt['mode']==mode and version==3 and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection'
    and mapped['returncode']==1 and mapped['inputs_unchanged'] is True and mapped['runtime_binary_copies']==0,'Different mixed-batch custody')
   require(len(failures)==1 and failures[0]['name']=='QtGui.NasaMatlabInterfaceTreeUnavailable' and
    'Formation runtime unavailable' in Path(failures[0]['log']).read_text(),'Original negative-fixture failure hidden/different')
   require(mapped['source_sha256']==ids and all(ids.get(p)==h for p,h in receipt['source_hashes'].items()) and receipt['startup_sha256']==self.startup,'Mixed-batch source/startup identity differs')
   union={}
   for process in mapped['processes']:
    for snap in process['snapshots']:
     require(hashlib.sha256(snap['maps'].encode()).hexdigest()==snap['maps_sha256'],'Mixed raw map differs')
     paths=set()
     for line in snap['maps'].splitlines():
      fields=line.split(None,5)
      if len(fields)==6 and (fields[5].startswith(str(self.repo)+'/') or fields[5].startswith('/usr/local/MATLAB/R2026b/')):paths.add(fields[5])
     require(paths==set(snap['project_images']),'Mixed mapped image partition differs');union.update(snap['project_images'])
   require(union==mapped['loaded_project_images'],'Mixed mapped image union differs')
   self.cache[name]=receipt,mapped
  return super().context(name,version,mode,test,role,argument)

def qualify(repo,path,inventory):
 q=load(path);require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Different original interface conjunction')
 require(all(q[k] is False for k in ['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','MATLAB_bundle_qualified']),'Global gate falsely closed')
 for p,h in q['artifact_sha256'].items():require(Path(p).is_file() and not Path(p).is_symlink() and sha(p)==h,'Changed artifact '+p)
 native.frozen.archive(q['reader_source_archive'],q['reader_source_sha256'])
 for p,h in q['reader_source_sha256'].items():require(sha(repo/p)==h,'Executing reader differs '+p)
 stage=Path(q['stage']);scope=load(q['scope_review']);before=load(q['prior_coverage']);overlay=load(q['prior_overlay'])
 known={r['key']:r for r in inventory['requirements']};row=lambda k:dict(key=k,declared_id=known[k]['declared_id'],fingerprint=known[k]['fingerprint'],source_text=known[k]['text'])
 require(q['authority_sha256']==inventory['sources']==scope['authority_sha256'] and len(known)==1158 and len(inventory['historical_cases'])==2367,'Original authority/denominator differs')
 require(scope['candidate_rows']==[row(k) for k in KEYS] and scope['parent_children']==[row('All Others!E155')] and scope['required_profiles']==['Available','Unavailable'] and scope['required_operations']==OPERATIONS,'Original parent/sole-child scope changed')
 require(before['coverage']['requirements']['verified']==1045 and before['coverage']['requirements']['total']==1158 and before['coverage']['historical_cases']['verified']==1816
  and before['requirements_still_needed']==56 and set(KEYS)<={r['key'] for r in before['remaining_original_rows']['requirements']},'Prior exact union changed/duplicate credit')
 original={r['key']:r['original_row'] for r in overlay['reviewed_source_rows'] if r['key'] in KEYS}
 require(scope['retained_original_closure_records']==original and scope['original_bundle_gates_retained']=={k:r['closure_gate'] for k,r in original.items()}
  and scope['MATLAB_bundle_qualified'] is False and len(overlay['original_hold_keys_retained'])==59,'Original holds/bundle records lost')
 e=Evidence(repo,stage,q['generated_startup_sha256']);proofs={};regression_missions=0
 expected={('QtGui.NasaMatlabInterfaceTree'+p,'matlab-interface-tree-test',p) for p in ['Available','Unavailable']}|{
  ('QtGui.NasaMatlabFunctionCreation','matlab-creation-regression',None),('QtGui.NasaFminconResource','fmincon-resource-test',None),('QtGui.NasaFminconSolveConstrained','fmincon-solve-test','Constrained')}
 require(len(q['case_bindings'])==10 and {(b['mode'],b['test'],b['role'],b.get('argument')) for b in q['case_bindings']}=={(m,*t) for m in ['headless','visible'] for t in expected},'New/affected mode matrix incomplete')
 for b in q['case_bindings']:
  p,d=e.context(b['run'],b['version'],b['mode'],b['test'],b['role'],b.get('argument'))
  if b['role']=='matlab-interface-tree-test':check(p,b['argument'],d);proofs[b['mode']+'-'+b['argument']]=p
  elif b['role']=='matlab-creation-regression':native.frozen.check_proof(p,d);regression_missions+=len(p['phases'])
  elif b['role']=='fmincon-resource-test':native.resource.check(p,d);regression_missions+=len(p['phases'])
  else:native.callback.solve(p,'Constrained',d);regression_missions+=len(p['phases'])
 qa=load(stage/'source-qa-v1.json');require(qa['core_paths']==259 and qa['core_source_changes']==0 and not qa['numerical_oracle_bounds_relaxed'] and not qa['new_core_API_or_layout'],'Unreviewed core/oracle change')
 for r in load(qa['core_inventory'])['files']:require(sha(repo/r['path'])==r['working_sha256'],'Core source changed')
 for p,h in qa['production_sha256'].items():require(sha(repo/p)==h and e.source(3)[p]==e.source(4)[p] and hashlib.sha256(e.source(4)[p]).hexdigest()==h,'Admitted frontend versions differ')
 for p,h in qa['old_test_sha256'].items():require(sha(repo/p)==h and all(hashlib.sha256(e.source(v)[p]).hexdigest()==h for v in [1,3,4]),'Original test source/assertion changed')
 member='src/qtgui/tests/NasaMatlabInterfaceTreeTests.cpp';a=e.source(3)[member].decode();b=e.source(4)[member].decode()
 require(e.source(1)[member]==e.source(3)[member],'Before failure assertion changed')
 expected_source=a.replace('text.replace(plugin,"% MATLAB provider disabled for this actual startup")','text.replace(plugin,"# MATLAB provider disabled for this actual startup")').replace('  Rig rig(startup);rig.save("startup.txt",read(startup));','  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!evidence.isEmpty())write(evidence+"/startup-before-initialize.txt",read(startup));\n  Rig rig(startup);rig.save("startup.txt",read(startup));')
 require(b==expected_source,'Additional new test/oracle changes')
 for name in ['before-headless-v1','before-visible-v1']:
  r=next(r for r in load(stage/name/'results.json')['results'] if r['name']=='QtGui.NasaMatlabInterfaceTreeAvailable')
  require(r['status']=='failed' and 'Actual Resource Tree MATLAB interface node missing' in Path(r['log']).read_text(),'Before route failure hidden')
  cap=load(Path(r['log']).parent/'capability.json');require(cap==dict(Matlab_available=True,children=[],native_interface_present=True),'Original actual route gap differs')
 controls=[]
 mutations=[('lost-action',lambda p:p['actions'].pop()),('wrong-operation',lambda p:p['actions'][0].update(operation='Open')),('not-popup',lambda p:p['actions'][0].update(actual_popup=False)),
  ('source-mutation',lambda p:p['actions'][0].update(source_unchanged=False)),('history-mutation',lambda p:p['actions'][0].update(history_unchanged=False)),
  ('same-terminated-PID',lambda p:p['actual_MATLAB_pids'].__setitem__(1,p['actual_MATLAB_pids'][0])),('no-busy-guard',lambda p:p.update(busy_reentry_checks=0)),
  ('wrong-mission-result',lambda p:p.update(actual_MATLAB_call_result=129)),('different-workspace',lambda p:p.update(same_engine_workspace_used=False)),
  ('missing-viewer',lambda p:p.update(actual_ReportViewer=False)),('lost-Unicode-reopen',lambda p:p.update(Unicode_reopen=False)),('Engine-left-open',lambda p:p.update(Engine_closed=False))]
 for name,mutate in mutations:
  p=copy.deepcopy(proofs['headless-Available']);mutate(p)
  try:check(p,'Available')
  except (ValueError,KeyError,TypeError,IndexError):controls.append(name)
  else:raise ValueError('Corrupt interface proof accepted: '+name)
 p=copy.deepcopy(proofs['headless-Unavailable']);p['runtime_node_absent']=False
 try:check(p,'Unavailable')
 except ValueError:controls.append('unavailable-provider-exposed')
 else:raise ValueError('Unavailable-provider route accepted')
 q['audit_summary']=dict(qualified_keys=KEYS,GUI_case_contexts=10,new_interface_contexts=4,affected_regression_contexts=6,affected_regression_missions=regression_missions,
  actual_interface_MATLAB_function_calls=2,actual_distinct_processes_per_available_context=2,actual_popup_actions=20,busy_reentry_checks=sum(proofs[m+'-Available']['busy_reentry_checks'] for m in ['headless','visible']),
  case_source_bindings=e.contexts,visible_native_exposures=sum(c['visible_exposures'] for c in e.contexts),corrupt_proof_controls=controls,all_prior_core_paths_unchanged=259,
  retained_original_hold_identities=59,retained_missing_reference='FRR-36',retained_source_conflict_probes=['Object','Literals'],MATLAB_bundle_qualified=False)
 return q,KEYS

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--qualification',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 repo=Path(__file__).resolve().parents[3];q,keys=qualify(repo,a.qualification,nasa.inventory(repo));require(not a.output.exists(),'Audit output exists')
 a.output.write_text(json.dumps(dict(status='passed',qualified_keys=keys,summary=q['audit_summary']),indent=2)+'\n');print(json.dumps(q['audit_summary']))
