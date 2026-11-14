"""Replay all18 builtin helper routes and preserve original positive contracts."""
import argparse,ast,copy,hashlib,json,math,re,subprocess,sys,tarfile,tempfile
from collections import Counter
from pathlib import Path
from xml.etree import ElementTree

STAGE='build/nasa-coverage/coverage95-20261104-function-builtin-automatic-global'
PRIOR='build/nasa-coverage/coverage95-20261104-function-global-string'
def check(ok,message):
 if not ok:raise ValueError(message)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text())
def source(archive,name):
 with tarfile.open(archive)as tar:return tar.extractfile(name).read()
def functions(archive,name,names):
 tree=ast.parse(source(archive,name),filename=name);nodes=[n for n in tree.body if isinstance(n,ast.FunctionDef)and n.name in names]
 check({n.name for n in nodes}==set(names),'Frozen contract absent')
 ns=dict(require=check,check=check,sha=sha,load=load,hashlib=hashlib,tarfile=tarfile,re=re)
 exec(compile(ast.Module(body=nodes,type_ignores=[]),name,'exec'),ns);return ns
def numeric(actual,expected,bound):
 check(len(actual)==len(expected)and all(math.isfinite(a)and math.isfinite(b)and abs(a-b)<=bound for a,b in zip(actual,expected)),'Independent literal differs')
def normalized(root,name):
 match=re.search(r"Exercise\.FunctionPath = '([^']+)/Built-in functions Ω/Exercise.gmf';",(root/'initial.script').read_text())
 check(match is not None,'Actual builtin fixture root absent')
 return(root/name).read_text().replace(match[1],'${BUILTIN_TEMP_ROOT}')
def contract(proof,spec):
 check(proof['checkpoint']=='function-builtin-automatic-global-'+spec['key']and proof['outcome']=='passed'and proof['builtin_type']==spec['builtin_type']and proof['missions']==3 and proof['function_calls']==12,'Builtin route identity/census differs')
 check(proof['file_decisions']==[dict(saved=False,recovery=False),dict(saved=True,recovery=False)]and [r['phase']for r in proof['runs']]==['initial','reopened','repeat-no-build'],'Actual edit/reopen/no-Build sequence differs')
 check(proof['local_builtin_execution_blocks']==9 and proof['main_builtin_execution_blocks']==3 and proof['local_builtin_calls']==9*spec['private_primary_builtin_calls_per_block']and proof['main_builtin_calls']==3*spec['main_primary_builtin_calls_per_block'],'Authored straight-line primary-call census differs')
 for row in proof['runs']:
  for name in ['main','first','second','third']:
   value=row[name];expected=spec['main_expected'if name=='main'else'expected'];oracle=spec['oracle']
   if oracle=='host-utc-bracket':
    stamp=float(value);lower=10587.5+row['host_utc_before_ms']/86400000.;upper=10587.5+row['host_utc_after_ms']/86400000.;check(math.isfinite(stamp)and lower-1/86400<=stamp<=upper+1/86400,'Independent recorded host UTC bracket differs')
   elif oracle=='seed-repeatability':check(len(value)==6 and value[0]==value[2]and value[1]==value[3]and(value[0]!=value[4]or value[1]!=value[5])and all(0<=x<=1 for x in value),'Seed repeat/distinction/unit interval differs')
   elif oracle=='elapsed-pause':check(len(value)==2 and .015<=value[0]<30 and value[1]==42 and row['elapsed_ms']>=30,'Original independent pause bounds differ')
   elif spec['kind']=='string':check(value==expected,'Independent typed String literal differs')
   else:numeric(value,expected,spec['tolerance'])
  if spec['original_scope_case']or spec['oracle']in {'literal','seed-repeatability'}:check(row['main']!=row['first']and row['third']==row['first'],'Contrasting root/private or nested result differs')
  if spec['oracle']not in {'host-utc-bracket','elapsed-pause'}:check(row['first']==row['second'],'Original deterministic repeat differs')
  check(row['function_calls']==4 and row['exercise_calls']==3 and row['main_builtin_calls']==1 and row['continuation']==42 and row['private_name_absent']and row['configured_prototypes_unchanged'],'Original call/continuation/isolation differs')
  check(set(row['caller_runtime'])==set(spec['main_runtime_expected']),'Caller input binding omitted')
  for name,expected in spec['main_runtime_expected'].items():
   actual=dict(row['caller_runtime'][name]);wanted=dict(expected)
   if 'values'in wanted:numeric(actual.pop('values'),wanted.pop('values'),spec['tolerance'])
   if wanted.get('type')=='Variable':check(abs(actual.pop('value')-wanted.pop('value'))<=spec['tolerance'],'Caller scalar changed')
   check(actual==wanted,'Caller state/shape/string metadata differs')
  check([i['function']for i in row['identities']]==['Exercise','Exercise','NestedExercise'],'Actual direct/direct/nested stores omitted')
  helper=None
  for identity in row['identities']:
   pointer=identity['runtime_object_pointer'];check(pointer==identity['global_store_object_pointer']and re.fullmatch('[0-9a-f]+',pointer)and pointer!='0','Shared counter pointer differs')
   pointer=identity['helper_runtime_object_pointer'];check(pointer==identity['helper_store_object_pointer']and re.fullmatch('[0-9a-f]+',pointer)and pointer!='0'and identity['helper_type']==spec['builtin_type']and identity['helper_automatic']is True and identity['helper_global']is True and identity['helper_local']is False,'Actual automatic caller-helper identity/flags differ')
   if helper is None:helper=pointer
   check(helper==pointer,'Direct/nested helpers do not share the actual caller identity')

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,required=True);p.add_argument('--manifest',type=Path);p.add_argument('--output',type=Path,required=True);args=p.parse_args();repo=args.repo.resolve();stage=repo/STAGE;prior=repo/PRIOR;counts=Counter()
 if args.manifest:
  seal=load(args.manifest);check(sha(__file__)==seal['auditor_sha256'],'Auditor identity changed')
  for n,h in seal['regular_artifact_sha256'].items():check(not(repo/n).is_symlink()and sha(repo/n)==h,'Immutable artifact changed: '+n)
  for n,target in seal['symbolic_links'].items():check((repo/n).is_symlink()and str((repo/n).readlink())==target,'Temporal route changed')
  with tarfile.open(repo/seal['final_source_archive'])as tar:
   check(set(tar.getnames())==set(seal['final_source_sha256']),'Final reviewed scope changed')
   for n,h in seal['final_source_sha256'].items():check(hashlib.sha256(tar.extractfile(n).read()).hexdigest()==h,'Final reviewed source bytes changed')
  counts['immutable_artifact_bindings']=len(seal['regular_artifact_sha256']);counts['temporal_alias_routes']=len(seal['symbolic_links'])
 prep=load(stage/'preparation-v1.json');check(sha(prior/'evidence-manifest-v1.json')==prep['prior_manifest_sha256']and sha(prior/'evidence-audit-v1.json')==prep['prior_audit_sha256'],'Prior final seal changed');oldseal=load(prior/'evidence-manifest-v1.json')
 with tempfile.TemporaryDirectory(prefix='gmat-builtin-auto-prior-')as folder:
  script=Path(folder)/'prior.py';output=Path(folder)/'audit.json';script.write_bytes(source(repo/oldseal['final_source_archive'],'src/qtgui/tests/AuditFunctionGlobalString.py'))
  done=subprocess.run([sys.executable,str(script),'--repo',str(repo),'--manifest',str(prior/'evidence-manifest-v1.json'),'--output',str(output)],cwd=folder,capture_output=True,text=True)
  check(done.returncode==0,'Frozen prior audit failed: '+done.stderr[-1500:]);check(output.read_bytes()==(prior/'evidence-audit-v1.json').read_bytes(),'Frozen prior audit changed')
 # The retained-route predecessor checks its selected records. Independently
 # replay the earlier owned-type chain as well, including its source capsules,
 # loaded-image maps,reader arithmetic and temporal-alias correction.
 join=repo/'build/nasa-coverage/coverage95-20261104-function-resource-global-join';join_prep=load(join/'preparation-v1.json');earlier=repo/join_prep['prior_stage']
 check(sha(earlier/'evidence-manifest-v1.json')==join_prep['prior_manifest_sha256']and sha(earlier/'evidence-audit-v1.json')==join_prep['prior_audit_sha256'],'Earlier owned-chain seal changed')
 earlier_seal=load(earlier/'evidence-manifest-v1.json')
 with tempfile.TemporaryDirectory(prefix='gmat-builtin-auto-owned-chain-')as folder:
  script=Path(folder)/'earlier.py';output=Path(folder)/'audit.json';script.write_bytes(source(repo/earlier_seal['final_source_archive'],'src/qtgui/tests/AuditFunctionTopocentricReopen.py'))
  done=subprocess.run([sys.executable,str(script),'--repo',str(repo),'--stage',str(earlier),'--manifest',str(earlier/'evidence-manifest-v1.json'),'--output',str(output)],cwd=folder,capture_output=True,text=True)
  check(done.returncode==0,'Frozen earlier owned chain failed: '+done.stderr[-1500:]);check(output.read_bytes()==(earlier/'evidence-audit-v1.json').read_bytes(),'Frozen earlier owned chain changed')
 counts['earlier_owned_chain_replays']=1
 check(sha(repo/'build/nasa-coverage/coverage95-20261103-matlab-c-interface/coverage-snapshot-v1.json')=='4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6','Canonical ledger changed')
 archive=stage/'executed-sources-v1.tar.gz';custody=functions(archive,'src/qtgui/tests/AuditFunctionTextEphemeris.py',{'custody','archive_sources'});identities=custody['archive_sources'](stage,1);build=load(stage/'build-receipt-v1.json');check(build['status']=='passed'and build['returncode']==0 and build['inputs_unchanged']and build['source_sha256']==identities,'Build/source custody changed')
 template_record=load(stage/'additional-original-template-custody-v1.json');template_name=template_record['path']
 check(template_name=='src/qtgui/tests/NasaFunctionBuiltinScopeCases.json.in'and template_name not in identities and sha(repo/template_record['pre_run_preimage'])==template_record['sha256']==prep['original_source_sha256'][template_name]and not template_record['executed_archive_modified'],'Original-template preimage custody differs')
 for binding in template_record['invocation_reference_bindings']:
  check(sha(repo/binding['receipt'])==binding['receipt_sha256']and load(repo/binding['receipt'])['source_hashes'][template_name]==template_record['sha256'],'Original-template native reference binding differs')
 def input_source(name):
  return (repo/template_record['pre_run_preimage']).read_bytes()if name==template_name else source(archive,name)
 for n,h in prep['original_source_sha256'].items():
  if n=='src/qtgui/CMakeLists.txt':continue
  check(hashlib.sha256(input_source(n)).hexdigest()==h==sha(stage/'preimages-v1'/n),'Original driver/catalog changed')
 baseline=load(prior/'final-reviewed-source-sha256-v1.json');check(all(identities[n]==h for n,h in baseline.items()if n.startswith(('src/base/','src/gmatutil/','plugins/'))),'Production core source changed')
 call_parser=functions(archive,'src/qtgui/tests/AuditFunctionClockPropagation.py',{'calls','subsequence'});original=source(archive,'src/qtgui/tests/NasaFunctionBuiltinScopeTests.cpp').decode();extended=source(archive,'src/qtgui/tests/NasaFunctionBuiltinAutomaticGlobalTests.cpp').decode();oldcalls=call_parser['calls'](original);newcalls=call_parser['calls'](extended);check(len(oldcalls)==71 and len(newcalls)==76 and call_parser['subsequence'](oldcalls,newcalls),'Original71 assertions changed')
 counts['original_scope_assertions_preserved']=71;counts['additional_native_assertions']=5
 generator_name='src/qtgui/tests/PrepareFunctionBuiltinAutomaticGlobal.py';ns={'__name__':'frozen_generator','__file__':generator_name};exec(compile(source(archive,generator_name),generator_name,'exec'),ns)
 with tempfile.TemporaryDirectory(prefix='gmat-builtin-auto-recipe-')as folder:
  root=Path(folder)
  for n in ['NasaFunctionBuiltinScopeCases.json.in','NasaFunctionBuiltinCases.json']:
   path=root/'src/qtgui/tests'/n;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(input_source('src/qtgui/tests/'+n))
  generated=ns['generate'](root)
 authored=json.loads(source(archive,'src/qtgui/tests/NasaFunctionBuiltinAutomaticGlobalCases.json.in'));check(generated==authored and len(authored['cases'])==18,'Independent recipe regeneration differs')
 configured=json.loads(source(archive,'build/linux-gui/src/qtgui/NasaFunctionBuiltinAutomaticGlobalCases.json'))
 text=json.dumps(authored).replace('@GMAT_BUILTIN_SCOPE_PRIVATE_OEM@',str(repo/'src/qtgui/tests/function-builtin-scope/private-endpoints.oem')).replace('@GMAT_BUILTIN_SCOPE_CALLER_OEM@',str(repo/'src/qtgui/tests/function-builtin-scope/caller-endpoints.oem'));check(json.loads(text)==configured,'Configured catalog differs from actual authored recipes')
 oldscope=json.loads(input_source('src/qtgui/tests/NasaFunctionBuiltinScopeCases.json.in'))['cases'];oldpositive=[s for s in json.loads(source(archive,'src/qtgui/tests/NasaFunctionBuiltinCases.json'))['cases']if 'expected_failure'not in s]
 for old in oldscope+oldpositive:
  case=next(c for c in authored['cases']if c['key']==old['key']);check(all(case[k]==v for k,v in old.items()),'Original positive inputs/oracles/bounds changed')
 oldcontract=functions(archive,'src/qtgui/tests/AuditFunctionBuiltinScope.py',{'contract'})['contract'];receipt=load(stage/'stage-receipt-v1.json');previous=load(prior/'stage-receipt-v1.json')
 for k in ['canonical_verified','original_total','target','still_needed','historical_verified','historical_total','original_hold_keys','new_requirement_credit','whole_row_qualified','scientific_approval','technical_approval','full_current_build_qualified','full_NASA_qualified','collective_owned_types','total_owned_types','remaining_owned_types']:check(receipt[k]==previous[k],'Original scope/gate changed: '+k)
 for run,version in receipt['gui_execution_versions'].items():check(version==1,'Execution version changed');custody['custody'](load(stage/(run+'-loaded-images.json')),build,stage/'build-receipt-v1.json',counts)
 proofs=[]
 for selected in receipt['selected']:
  root=stage/selected['run']/selected['test'];result=load(root/'result.json');proof=load(root/'proof.json');spec=next(c for c in configured['cases']if c['key']==selected['case'])
  check(result['status']=='passed'and result['returncode']==0 and result['requested_visibility']==selected['mode']and sha(root/'proof.json')==selected['proof_sha256'],'Native result/proof/mode changed')
  catalog_name='build/linux-gui/src/qtgui/NasaFunctionBuiltinAutomaticGlobalCases.json';driver_name='src/qtgui/tests/NasaFunctionBuiltinAutomaticGlobalTests.cpp';invocation=load(stage/selected['run']/'results.json')
  check(result['sources']==[driver_name]and str(repo/catalog_name)in result['registered_command']and result['registered_command'][-1]==selected['case'],'Actual native driver/catalog/case route differs')
  check(all(invocation['source_hashes'][n]==identities[n]for n in [catalog_name,driver_name,generator_name]),'Actual native source/recipe bindings differ')
  valid=(root/'valid.gmf').read_text();caller=(root/'initial.script').read_text();check(spec['declarations']in valid and spec['execution']in valid and re.sub(r'\bout\b','Main',spec['main_execution'])in caller,'Authored private/main executable recipe absent')
  check(len(re.findall(r'\bNativeHelper\(',valid))==spec['private_primary_builtin_calls_per_block']and len(re.findall(r'\bCallerHelper\(',caller))==spec['main_primary_builtin_calls_per_block'],'Actual straight-line primary-call statements differ')
  tests=ElementTree.parse(root/'ctest.xml').findall('.//testcase');check(len(tests)==1 and tests[0].get('name')==selected['test']and all(tests[0].find(k)is None for k in ['failure','error','skipped']),'Native JUnit differs')
  emitted=[json.loads(x)for x in re.findall(r'NASA_CHECK (\{[^\n]+\})',(root/'test.log').read_text())];check(emitted==[proof],'Actual process output differs');contract(proof,spec)
  if spec['original_scope_case']:
   compatible=copy.deepcopy(proof);compatible['checkpoint']='function-builtin-scope-'+spec['key'];oldcontract(compatible,spec);counts['original_scope_contracts_replayed']+=1
  for row in proof['runs']:
   raw=(root/(row['phase']+'-values.csv')).read_text().strip().splitlines();check(len(raw)==4,'Raw report rows omitted')
   for words,name in zip(raw,['main','first','second','third']):
    if spec['kind']=='string':check(words.strip()==row[name],'Raw typed String differs')
    else:numeric([float(x)for x in words.split(',')if x.strip()],row[name],spec['tolerance'])
   check(load(root/(row['phase']+'-prototypes.json'))==load(root/'prototypes-before.json'),'Actual configured prototype snapshots differ')
   counts['actual_helper_store_identities']+=len(row['identities']);counts['actual_counter_store_identities']+=len(row['identities']);counts['independent_typed_results']+=4
  for n in ['initial.script','valid.gmf','nested.gmf']:check('Global CallerHelper;'not in(root/n).read_text(),'Explicit Global substituted for automatic helper sharing')
  if selected['mode']=='visible':
   desktop=load(stage/selected['run']/'results.json')['visible_display'];check(desktop['start_minimized']and desktop['resizable']and 'Iconic'in desktop['initial_host_window_state']and 'FULLSCREEN'not in desktop['initial_host_window_state']and result['trace']['recorded_platforms']==['xcb']and result['trace']['native_exposure_events']>0,'Visible minimized/resizable exposure policy changed');counts['visible_exposures']+=result['trace']['native_exposure_events']
  check(not result['trace']['screenshots'],'Unexpected screenshot');counts['passed_native_contexts']+=1;counts['missions']+=proof['missions'];counts['function_calls']+=proof['function_calls'];counts['primary_builtin_calls']+=proof['local_builtin_calls']+proof['main_builtin_calls'];proofs.append((proof,spec))
 for damage in [lambda p:p['runs'][0]['identities'][0].update(helper_store_object_pointer='0'),lambda p:p['runs'][0]['identities'][0].update(helper_automatic=False),lambda p:p['runs'][0]['identities'].pop(),lambda p:p['runs'][0]['main'].__setitem__(0,0),lambda p:p['runs'][0]['first'].__setitem__(0,0),lambda p:p['runs'][0].update(continuation=0),lambda p:p['runs'].pop()]:
  proof,spec=proofs[0];bad=copy.deepcopy(proof);damage(bad);rejected=False
  try:contract(bad,spec)
  except(ValueError,KeyError,IndexError):rejected=True
  check(rejected,'Damaged helper/value/scope proof escaped');counts['damaged_proofs_rejected']+=1
 for binding in load(stage/'original-script-preservation-v1.json')['records']:
  a=repo/binding['prior'];b=repo/binding['current'];before=normalized(a.parent,a.name);after=normalized(b.parent,b.name)
  check(before==after and hashlib.sha256(after.encode()).hexdigest()==binding['normalized_sha256'],'Original scope scripts changed beyond exact temporary root')
  counts['unchanged_original_script_bindings']+=1
 matrix=load(stage/'builtin-auto-scope-matrix-v1.json');old=load(prior/'string-global-scope-matrix-v1.json');check(len(matrix['rows'])==566 and all(matrix[k]==v for k,v in old.items()if k!='rows'),'Full resource matrix changed')
 for a,b in zip(old['rows'],matrix['rows']):check(all(b[k]==v for k,v in a.items())and b['builtin_automatic_global_witnesses']==[x for x in receipt['selected']if x['builtin_type']==a['factory_type']],'Prior role/eligibility/conjunction changed')
 remaining=load(stage/'remaining-requirement-audit-v1.json');old=load(prior/'remaining-requirement-audit-v1.json');check(len(remaining['rows'])==108 and all(remaining[k]==v for k,v in old.items()),'Original exact rows/fingerprints/gaps changed')
 processes=load(stage/receipt['process_audit']);check(not processes['matching_native_processes']and not processes['matching_visible_servers']and len(processes['recorded_native_identities'])==counts['mapped_native_processes']==252,'Terminal process census changed')
 check(counts['passed_native_contexts']==36 and counts['original_scope_contracts_replayed']==6 and counts['actual_helper_store_identities']==324 and counts['primary_builtin_calls']==912 and len(set(receipt['original_hold_keys']))==59,'Full18-type/original-scope/hold census differs')
 result=dict(status='passed-bounded18-builtin-automatic-helper-audit',counts=dict(counts),builtin_types=18,canonical_verified=1050,original_total=1158,target=1101,still_needed=51,original_open_rows=108,original_holds=59,pending_core_paths=279,core_increment_paths=0,new_requirement_credit=0,whole_row_qualified=False,scientific_technical_current_build_NASA_qualified=False)
 check(not args.output.exists(),'Never overwrite an audit');args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
