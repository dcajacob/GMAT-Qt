"""Join all retained scope routes to exact original clauses without eligibility waivers."""
import argparse,copy,hashlib,json,re,tarfile
from collections import Counter,defaultdict
from pathlib import Path
from xml.etree import ElementTree

STAGE='build/nasa-coverage/coverage95-20261104-function-scope-clause-join'
PRIOR='build/nasa-coverage/coverage95-20261104-function-automatic-resources'
def check(ok,message):
 if not ok:raise ValueError(message)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def validate_join(value,baseline,repo):
 check(len(value['rows'])==len(baseline['rows'])==566 and [r['factory_type']for r in value['rows']]==[r['factory_type']for r in baseline['rows']],'Exact factory universe differs')
 for old,row in zip(baseline['rows'],value['rows']):
  check(all(row[k]==v for k,v in old.items()),'Original role/evidence/eligibility/gap changed');joined=row['exact_scope_clause_join_v1'];check(joined['whole_obligation_qualified']is False and joined['new_requirement_credit']==0 and joined['eligibility_exclusion_assigned']is False and joined['prior_gaps_preserved']is True,'Unsupported whole-row/eligibility admission')
  if row['role']!='mission-command;separate-command-obligation':check({c['mode']for c in joined['contexts']}=={'headless','visible'},'Exact type/profile omitted')
  if row['role']=='owner-bound-system-parameter':check(joined['scope_inferred_from_owner']is False and joined['independent_named_Parameter_Global_promotion_observed']is False,'Unobserved independent Parameter promotion')
  if row['factory_type']=='Star':check(joined['custom_Star_creation_qualified']is False and joined['existing_Sun_substituted_for_custom_Star']is False,'Existing Sun substituted for custom-Star creation')
  for context in joined['contexts']:
   path=Path(context['proof']);name=str(path.relative_to(repo))if path.is_absolute()else str(path);check(value['artifact_sha256'][name]==context['proof_sha256'],'Joined proof identity differs')
 check(len(value['exact_parent_and_children'])==10,'Exact parent/child omitted')
 for old,row in zip(baseline['exact_parent_and_children'],value['exact_parent_and_children']):check(all(row[k]==v for k,v in old.items())and row['canonical_already_counted']==(row['key']!='FRR!E491')and row['full_conjunction_closed']is False and row['new_requirement_credit']==0,'Original clause/fingerprint/credit/conjunction changed')
 check(value['canonical_verified']==1050 and value['original_total']==1158 and value['target']==1101 and value['still_needed']==51 and value['original_open_rows']==108 and value['original_holds']==59 and value['pending_core_paths']==280 and value['new_native_runs']==0 and value['core_increment_paths']==[],'Original counts/holds/target or execution scope changed')
 for name in ['new_requirement_credit','whole_row_qualified','scientific_approval','technical_approval','full_current_build_qualified','full_NASA_qualified']:check(value[name]==0,'Original qualification gate changed')
 check(value['counts']==dict(bounded_types_preserved=125,separate_command_roles_preserved=53,completed_previously_pending_type_joins=67,owned_type_joins=36,automatic_body_type_joins=5,automatic_added_type_joins=3,user_parameter_type_joins=3,actual_Parameter_types_preserved=321,automatic_owner_resource_joins=2,automatic_builtin_type_joins=18,unique_retained_native_proofs_checked=1260,original_auxiliary_proofs_preserved=4),'Full retained family/proof census differs')

def review(repo,source_archive=None):
 stage=repo/STAGE;prior=repo/PRIOR;bindings={};cache={};parameter_cache={};counts=Counter()
 frozen={}
 if source_archive:
  source_names=[item['path']for item in json.loads((stage/'scope-source-observations-v1.json').read_text())['sources']]
  with tarfile.open(source_archive)as archive:
   for name in source_names:frozen[name]=archive.extractfile(name).read()
 def read(path,expected=None):
  path=Path(path);path=path if path.is_absolute()else repo/path
  name=str(path.relative_to(repo))
  if name in frozen:raw=frozen[name]
  else:check(path.is_file()and not path.is_symlink(),'Missing regular artifact: '+str(path));raw=path.read_bytes()
  digest=hashlib.sha256(raw).hexdigest();check(expected is None or digest==expected,'Changed retained artifact: '+str(path));bindings[name]=digest;return raw
 def load(path,expected=None):return json.loads(read(path,expected))
 def resolve(context):
  if context.get('proof'):
   path=Path(context['proof']);return path if path.is_absolute()else repo/path
  check(context.get('run')and context.get('test')and context.get('proof_sha256'),'Unroutable retained context')
  candidates=[p for p in(repo/'build/nasa-coverage').glob('*/'+context['run']+'/'+context['test']+'/proof.json')if p.is_file()and not p.is_symlink()and sha(p)==context['proof_sha256']]
  check(len(candidates)==1,'Ambiguous source-specific proof route');return candidates[0]
 def native(context,operation):
  path=resolve(context);raw=read(path,context.get('proof_sha256'));key=str(path.relative_to(repo))
  if key not in cache:
   try:value=json.loads(raw)
   except json.JSONDecodeError:
    markers=re.findall(r'NASA_CHECK (\{[^\n]+\})',raw.decode());check(len(markers)==1,'Retained native proof absent');value=json.loads(markers[0])
   check(value.get('outcome')=='passed','Retained native proof did not pass');root=path.parent;result=load(root/'result.json');check(result['status']=='passed'and result['returncode']==0,'Retained native result did not pass')
   tests=ElementTree.fromstring(read(root/'ctest.xml')).findall('.//testcase');check(len(tests)==1 and tests[0].get('name')==result['name']and all(tests[0].find(k)is None for k in ['failure','error','skipped']),'Retained native JUnit differs')
   emitted=[json.loads(x)for x in re.findall(r'NASA_CHECK (\{[^\n]+\})',read(root/'test.log').decode())];check(emitted==[value],'Native emitted proof differs');cache[key]=(value,result)
  value,result=cache[key];check(result['requested_visibility']==context['mode'],'Retained mode differs')
  if context['mode']=='visible':check(result['trace']['native_exposure_events']>0,'Retained visible exposure absent')
  return dict(proof=key,proof_sha256=bindings[key],mode=context['mode'],operation=operation),value
 def pair(contexts,operation):
  check(contexts and {c['mode']for c in contexts}=={'headless','visible'},'Both retained profiles absent: '+operation)
  return [native(c,operation)[0]for c in contexts]
 def parameters(value):
  if isinstance(value,dict):
   if 'factory_type'in value and 'parameter_global'in value:yield value
   for item in value.values():yield from parameters(item)
  elif isinstance(value,list):
   for item in value:yield from parameters(item)
 prep=load(stage/'preparation-v1.json');seal=load(prior/'evidence-manifest-v1.json',prep['prior_manifest_sha256']);load(prior/'evidence-audit-v1.json',prep['prior_audit_sha256']);replay=load(prior/'frozen-audit-replay-v1.json',prep['prior_replay_sha256']);check(replay['byte_identical']and replay['returncode']==0 and replay['canonical_audit_sha256']==prep['prior_audit_sha256'],'Prior frozen replay unbound')
 baseline=load(prior/prep['baseline_matrix'],prep['baseline_sha256']);rows=copy.deepcopy(baseline['rows']);check(len(rows)==566 and len({r['factory_type']for r in rows})==566,'Factory universe changed')
 canonical=load('build/nasa-coverage/coverage95-20261103-matlab-c-interface/coverage-snapshot-v1.json','4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6');check(canonical['coverage']['requirements']['verified']==1050 and canonical['requirements_still_needed']==51,'Canonical counts changed')
 remaining={r['key']for r in canonical['remaining_original_rows']['requirements']};check('FRR!E491'in remaining and not any('FRR!E'+str(n)in remaining for n in range(492,501)),'Original parent/child credit state changed')
 retained_parameters=defaultdict(list)
 for row in rows:
  for context in row['resource_global_join_v1']['contexts']:
   joined,value=native(context,'retained-original-resource-global-join')
   if row['role']=='owner-bound-system-parameter':
    key=joined['proof']
    if key not in parameter_cache:
     index=defaultdict(list)
     for record in parameters(value):index[record['factory_type']].append(record)
     parameter_cache[key]=index
    retained_parameters[row['factory_type']]+=parameter_cache[key][row['factory_type']]
 # The earlier reviewer also checked two navigation and two frame auxiliaries
 # that were not attached to a final per-type context. Keep those exact proofs.
 original_join=load('build/nasa-coverage/coverage95-20261104-function-resource-global-join/retained-route-join-v4.json')
 auxiliary_count=0
 for name,digest in original_join['artifact_sha256'].items():
  if not(name.endswith('/proof.json')or name.endswith('/test.log')):continue
  path=repo/name;result=load(path.parent/'result.json');was_present=name in cache;native(dict(proof=name,proof_sha256=digest,mode=result['requested_visibility']),'preserved-original-auxiliary-proof')
  auxiliary_count+=not was_present
 check(auxiliary_count==4,'Original auxiliary proof census differs')
 helper=next(r for r in rows if r['factory_type']=='GmatFunction')['automatic_resource_identity_witnesses'];check(len(helper)==2,'Variable-counter source-specific contexts missing')
 for row in rows:
  kind=row['factory_type'];old=row['resource_global_join_v1'];joined=dict(contexts=[],observed_clauses=[],whole_obligation_qualified=False,new_requirement_credit=0,eligibility_exclusion_assigned=False,prior_gaps_preserved=True)
  if row['conjunction']['main_private_explicit_global_bounded']:
   joined.update(family='bounded-main-private-explicit-Global',contexts=pair(row['conjunction']['contexts'],'main-private-explicit-Global'),observed_clauses=['FRR!E492','FRR!E493','FRR!E497','FRR!E498','FRR!E499','FRR!E500'],remaining='Retain original operation/domain and full-conjunction limits.');counts['bounded_types_preserved']+=1
  elif row['role']=='owner-bound-system-parameter':
   records=retained_parameters[kind];check(records and any(p['parameter_local']and p['owner_local']for p in records)and any(p['parameter_global']and p['owner_global']and p.get('shared')and p['reference_owner_pointer']==p['root_owner_pointer']!='0'for p in records),'Actual local/shared Parameter records absent: '+kind)
   joined.update(family='actual-owner-bound-Parameter-local-and-global',contexts=old['contexts'],observed_clauses=['FRR!E492','FRR!E493','FRR!E498'],actual_parameter_records=len(records),scope_inferred_from_owner=False,independent_named_Parameter_Global_promotion_observed=False,remaining='Independent promotion of a previously created qualified Parameter name is unproven. Preserve the E497 mapping question,all owner/dependency/domain bounds and ordinary-name grammar;no bare-creation or automatic-global exclusion.');counts['actual_Parameter_types_preserved']+=1
  elif row['role']=='mission-command;separate-command-obligation':
   joined.update(family='separate-command-obligation',remaining='Retain E477 and the SaveMission effect conflict. This role does not assign a resource eligibility exclusion.');counts['separate_command_roles_preserved']+=1
  else:
   counts['completed_previously_pending_type_joins']+=1
   if row['category_id']in {110,152}:
    contexts=old['contexts'];joined.update(family='owned-implementation-through-actual-resource-owner',contexts=pair(contexts,'actual-owned-type-and-owner-scope'),observed_clauses=['FRR!E492','FRR!E493','FRR!E495'if row['category_id']==152 else'FRR!E496'],automatic_scope_object='CoordinateSystem'if row['category_id']==152 else'PropSetup',owned_component_global_flag_inferred=False,remaining='Actual implementation and owner routes are joined. Preserve component/owner distinction,reader/time/frame/source bounds and the independent named-resource eligibility question.');counts['owned_type_joins']+=1
   elif row.get('body_scope_witnesses'):
    contexts=row['body_scope_witnesses'];joined.update(family='actual-shared-solar-model',contexts=pair(contexts,'actual-solar-model-and-body-identities'),observed_clauses=['FRR!E494'],existing_Sun_substituted_for_custom_Star=False)
    for context in contexts:
     _,value=native(context,'actual-solar-model-and-body-identities');check(value['type']==kind and value['checkpoint']=='body-resource-solar-system-scope','Body proof type differs')
     for phase in value['phases']:
      check(phase['main_body']['automatic']and phase['main_body']['global']and not phase['main_body']['local'],'Actual main body flags absent')
      for identity in phase['identities']:check(identity['reader_solar_system_pointer']==identity['runtime_solar_system_pointer']!=identity['configured_solar_system_pointer']and identity['declaration_object_pointer']==identity['runtime_solar_body_pointer'],'Actual shared solar model identity differs')
    if kind=='Star':
     probe=row['custom_Star_probe'];read(probe['log'],probe['log_sha256']);check(probe['status']=='failed'and not probe['eligibility_exclusion'],'Custom Star failure lost');joined.update(custom_Star_creation_qualified=False,remaining='Existing Sun/model access is observed. Custom Star main/function creation remains unverified;retain the advertised factory,failed main creation and original four-class creation authority without assigning an exclusion.')
    else:joined['observed_clauses']+=['FRR!E492','FRR!E493'];joined['remaining']='Preserve original supplied-SPK/field-only body-operation limits and full resource conjunction.'
    counts['automatic_body_type_joins']+=1
   elif row.get('builtin_automatic_global_witnesses'):
    contexts=row['builtin_automatic_global_witnesses'];joined.update(family='actual-automatic-builtin-helper',contexts=pair(contexts,'actual-main-private-and-automatic-helper'),observed_clauses=['FRR!E492','FRR!E493','FRR!E498'],remaining='Keep the original literal,domain,host-bracket,seed or pause contract and its original bounds. No constructor-flag eligibility exception.');counts['automatic_builtin_type_joins']+=1
   elif row.get('automatic_resource_identity_witnesses'):
    contexts=row['automatic_resource_identity_witnesses'];joined.update(family='actual-automatic-user-function-or-reference-resource',contexts=pair(contexts,'actual-automatic-caller-resource-identity'),observed_clauses=['FRR!E492','FRR!E493','FRR!E498'],remaining='Join the independent original local/resource-use routes with these actual automatic identities;selected object returns are not universal operation approval.');counts['automatic_added_type_joins']+=1
   elif row.get('function_owned_scope_witnesses'):
    contexts=row['function_owned_scope_witnesses'];joined.update(family='actual-main-and-function-created-automatic-resource',contexts=pair(contexts,'actual-automatic-owner-declarations-and-identities'),observed_clauses=['FRR!E492','FRR!E493','FRR!E495'if kind=='CoordinateSystem'else'FRR!E496'],remaining='Retain original frame/numerical/endpoint bounds;all later owned-type and precise-clock evidence remains independently joined.');counts['automatic_owner_resource_joins']+=1
    for context in contexts:
     _,value=native(context,'actual-automatic-owner-declarations-and-identities');check(value['native_type']==kind,'Actual owned resource type differs')
     for phase in value['phases']:
      resource=phase['main_resource'];check(resource['automatic']and resource['global']and not resource['local'],'Actual automatic owner flags absent')
      for identity in phase['identities']:check(identity['shared_global_pointer']==identity['runtime_shared_pointer']==resource['pointer']!=identity['configured_shared_pointer'],'Actual implicit owner identity differs')
   elif kind=='Variable':
    joined.update(family='actual-Variable-local-inputs-and-explicit-Global-counter',contexts=pair(helper,'actual-Variable-counter-and-local-copy-isolation'),observed_clauses=['FRR!E492','FRR!E493','FRR!E497','FRR!E498','FRR!E499','FRR!E500'],remaining='Keep original scalar/private-copy and shared-counter limits. Counter/owner use does not replace the separate321 Parameter obligations.')
    for context in helper:
     _,value=native(context,'actual-Variable-counter-and-local-copy-isolation')
     for run in value['runs']:
      check(run['caller_runtime']['Input']==dict(type='Variable',value=5)and run['caller_runtime']['Measured']==dict(type='Variable',value=28)and run['first']==[60,7]and run['continuation']==42,'Actual typed Variable/private-copy values differ')
      for identity in run['identities']:check(identity['global_store_object_pointer']==identity['runtime_object_pointer']!='0','Actual shared counter identity absent')
    counts['user_parameter_type_joins']+=1
   elif kind=='String':
    joined.update(family='actual-String-explicit-Global',contexts=pair(row['String_Global_witnesses'],'actual-String-Global-workflow'),observed_clauses=['FRR!E492','FRR!E493','FRR!E497','FRR!E498','FRR!E499','FRR!E500'],remaining='Preserve Unicode literal/decoy and original local/String I/O routes;no universal character-domain claim.');counts['user_parameter_type_joins']+=1
   elif kind=='Array':
    joined.update(family='actual-Array-explicit-Global',contexts=pair(row['new_global_witnesses'],'actual-Array-Global-element-and-whole-return'),observed_clauses=['FRR!E492','FRR!E493','FRR!E497','FRR!E498','FRR!E499','FRR!E500'],remaining='Preserve original shape/component/return/isolation bounds and independent local Array I/O routes.');counts['user_parameter_type_joins']+=1
   else:raise ValueError('Unjoined exact factory type: '+kind)
  row['exact_scope_clause_join_v1']=joined
 check(counts==Counter(bounded_types_preserved=125,actual_Parameter_types_preserved=321,separate_command_roles_preserved=53,completed_previously_pending_type_joins=67,owned_type_joins=36,automatic_body_type_joins=5,automatic_builtin_type_joins=18,automatic_added_type_joins=3,automatic_owner_resource_joins=2,user_parameter_type_joins=3),'Complete clause-join census differs')
 for old,row in zip(baseline['rows'],rows):check(all(row[k]==v for k,v in old.items()),'Prior role/evidence/gap/eligibility/conjunction changed')
 facts=load(stage/'scope-source-observations-v1.json')
 for item in facts['sources']:
  raw=read(item['path'],item['sha256']).decode();check(item['entry']in raw and item['anchor']in raw,'Owning source anchor absent')
 authority=facts['body_creation_authority'];actual=load(authority['path'],authority['sha256']);check(authority['original_rows']==[r for r in actual['original_rows']if r['key']in {'FRR!E'+str(n)for n in range(213,218)}],'Original body-creation authority differs')
 clauses=[]
 for item in baseline['exact_parent_and_children']:
  key=item['key'];clauses.append(dict(item,canonical_already_counted=key not in remaining,full_conjunction_closed=False,observed_factory_rows=[r['factory_type']for r in rows if key in r['exact_scope_clause_join_v1']['observed_clauses']],all_prior_gaps_preserved=True,source_questions=['qualified-Parameter-independent-Global','custom-Star-creation']if key in {'FRR!E491','FRR!E493','FRR!E497'}else[],new_requirement_credit=0))
 counts['unique_retained_native_proofs_checked']=len(cache)
 counts['original_auxiliary_proofs_preserved']=auxiliary_count
 return dict(status='all67-pending-type-clause-joins-complete;exact-whole-conjunction-open',baseline=str((prior/prep['baseline_matrix']).relative_to(repo)),baseline_sha256=prep['baseline_sha256'],counts=dict(counts),rows=rows,exact_parent_and_children=clauses,source_observations=facts,artifact_sha256=bindings,canonical_verified=1050,original_total=1158,target=1101,still_needed=51,original_open_rows=108,original_holds=59,pending_core_paths=280,new_native_runs=0,core_increment_paths=[],new_requirement_credit=0,whole_row_qualified=False,scientific_approval=False,technical_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,next_action='Resolve the independent qualified-Parameter Global clause mapping and custom-Star creation contract from authority. Retain all original operation/domain gaps and full E492-E500/E491 conjunction. No new generic owner/automatic test is justified by the completed67 joins.')

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--repo',type=Path,required=True);parser.add_argument('--source-archive',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();repo=args.repo.resolve();result=review(repo,args.source_archive);baseline=json.loads((repo/PRIOR/'automatic-resource-scope-matrix-v1.json').read_text());validate_join(result,baseline,repo);check(not args.output.exists(),'Never overwrite a clause review');args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k]for k in ['status','counts','new_native_runs','new_requirement_credit','next_action']}))
if __name__=='__main__':main()
