"""Replay automatic user-function/reference identities and the exact core increment."""
import argparse,ast,copy,hashlib,json,re,subprocess,sys,tarfile,tempfile
from collections import Counter
from pathlib import Path
from xml.etree import ElementTree

STAGE='build/nasa-coverage/coverage95-20261104-function-automatic-resources'
PRIOR='build/nasa-coverage/coverage95-20261104-function-builtin-automatic-global'
def check(ok,message):
 if not ok:raise ValueError(message)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text())
def source(archive,name):
 with tarfile.open(archive)as tar:return tar.extractfile(name).read()
def functions(archive,name,names):
 tree=ast.parse(source(archive,name),filename=name);nodes=[n for n in tree.body if isinstance(n,ast.FunctionDef)and n.name in names]
 check({n.name for n in nodes}==set(names),'Frozen contract absent')
 ns=dict(require=check,check=check,sha=sha,load=load,hashlib=hashlib,tarfile=tarfile,re=re,copy=copy)
 if 'contract'in names:
  import math
  ns['math']=math
 exec(compile(ast.Module(body=nodes,type_ignores=[]),name,'exec'),ns);return ns

def helper(proof,spec,old):
 check(proof['checkpoint']=='function-automatic-gmat-helper-GmatFunction'and proof['helper_type']=='GmatFunction'and proof['function_calls']==24 and proof['scope_function_calls']==12,'User-function identity/call census changed')
 compatible=copy.deepcopy(proof);compatible['checkpoint']='function-builtin-automatic-global-GmatFunction';compatible['builtin_type']=compatible['helper_type'];compatible['function_calls']=compatible['scope_function_calls']
 for a,b in [('local_builtin_calls','local_helper_calls'),('main_builtin_calls','main_helper_calls'),('local_builtin_execution_blocks','local_helper_execution_blocks'),('main_builtin_execution_blocks','main_helper_execution_blocks')]:compatible[a]=compatible[b]
 for row in compatible['runs']:
  check(row['function_calls']==8 and row['scope_function_calls']==4 and row['main_helper_calls']==1,'Leaf/scope calls changed')
  row['function_calls']=row['scope_function_calls'];row['main_builtin_calls']=row['main_helper_calls']
 recipe=copy.deepcopy(spec);recipe['builtin_type']=recipe['helper_type'];recipe['private_primary_builtin_calls_per_block']=recipe['private_primary_helper_calls_per_block'];recipe['main_primary_builtin_calls_per_block']=recipe['main_primary_helper_calls_per_block']
 old(compatible,recipe)

def reference(proof,spec,old):
 check(proof['checkpoint']=='function-automatic-reference-identity','Automatic reference checkpoint changed')
 compatible=copy.deepcopy(proof);compatible['checkpoint']='function-global-resource-identity';old(compatible,spec)
 for row in proof['phases']:
  check(row['automatic']is True and row['global']is True and row['local']is False and row['decoy_automatic']is True and row['decoy_global']is True and row['decoy_local']is False,'Automatic shared/decoy flags differ')
  for identity in row['identities']:
   pointer=identity['global_object_pointer'];check(re.fullmatch('[0-9a-f]+',pointer)and pointer!='0','Invalid actual object pointer')

def junit(root,name):
 tests=ElementTree.parse(root/'ctest.xml').findall('.//testcase');check(len(tests)==1 and tests[0].get('name')==name and all(tests[0].find(k)is None for k in ['failure','error','skipped']),'Native JUnit differs')

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--repo',type=Path,required=True);parser.add_argument('--manifest',type=Path);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();repo=args.repo.resolve();stage=repo/STAGE;prior=repo/PRIOR;counts=Counter()
 if args.manifest:
  seal=load(args.manifest);check(sha(__file__)==seal['auditor_sha256'],'Auditor identity differs')
  for n,h in seal['regular_artifact_sha256'].items():check(not(repo/n).is_symlink()and sha(repo/n)==h,'Immutable artifact differs: '+n)
  for n,target in seal['symbolic_links'].items():check((repo/n).is_symlink()and str((repo/n).readlink())==target,'Temporal alias route differs')
  with tarfile.open(repo/seal['final_source_archive'])as tar:
   check(set(tar.getnames())==set(seal['final_source_sha256']),'Final review scope differs')
   for n,h in seal['final_source_sha256'].items():check(hashlib.sha256(tar.extractfile(n).read()).hexdigest()==h,'Final reviewed bytes differ')
  counts['immutable_artifact_bindings']=len(seal['regular_artifact_sha256']);counts['temporal_alias_routes']=len(seal['symbolic_links'])
 prep=load(stage/'preparation-v1.json');check(sha(prior/'evidence-manifest-v1.json')==prep['prior_manifest_sha256']and sha(prior/'evidence-audit-v1.json')==prep['prior_audit_sha256'],'Prior seal changed');oldseal=load(prior/'evidence-manifest-v1.json')
 with tempfile.TemporaryDirectory(prefix='gmat-auto-resource-prior-')as folder:
  script=Path(folder)/'prior.py';output=Path(folder)/'audit.json';script.write_bytes(source(repo/oldseal['final_source_archive'],'src/qtgui/tests/AuditFunctionBuiltinAutomaticGlobal.py'))
  done=subprocess.run([sys.executable,str(script),'--repo',str(repo),'--manifest',str(prior/'evidence-manifest-v1.json'),'--output',str(output)],cwd=folder,capture_output=True,text=True)
  check(done.returncode==0,'Frozen prior audit failed: '+done.stderr[-1800:]);check(output.read_bytes()==(prior/'evidence-audit-v1.json').read_bytes(),'Frozen prior replay changed')
 counts['full_predecessor_and_owned_chain_replays']=1
 check(sha(repo/'build/nasa-coverage/coverage95-20261103-matlab-c-interface/coverage-snapshot-v1.json')=='4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6','Canonical exact ledger changed')
 archive=stage/'executed-sources-v2.tar.gz';custody=functions(archive,'src/qtgui/tests/AuditFunctionTextEphemeris.py',{'custody','archive_sources'});versions={};builds={}
 for version in [1,2]:
  versions[version]=custody['archive_sources'](stage,version);builds[version]=load(stage/f'build-receipt-v{version}.json');build=builds[version]
  check(build['status']=='passed'and build['returncode']==0 and build['inputs_unchanged']and build['source_sha256']==versions[version],'Source/build custody differs')
 counts['source_capsules']=2
 baseline=load(prior/'final-reviewed-source-sha256-v1.json');check(all(versions[1][n]==h for n,h in baseline.items()if n.startswith(('src/base/','src/gmatutil/','plugins/'))),'Initial production source differs from predecessor')
 delta=load(stage/'core-increment-v2.json');expected=['src/base/interpreter/Interpreter.cpp','src/gmatutil/util/FileUtil.cpp','src/gmatutil/util/FileUtil.hpp'];check(delta['existing_increments']==expected[:2]and delta['new_pending_paths']==expected[2:]and delta['before_pending_count']==279 and delta['after_pending_count']==280 and delta['previous_pending_paths_unchanged']==277,'Core inventory scope changed')
 before=load(stage/'core-before-v2.json')['pending_core_sha256'];after=delta['pending_core_sha256'];check(set(after)-set(before)=={expected[2]}and all(after[n]==h for n,h in before.items()if n not in expected),'Unrelated pending source changed')
 changed=[n for n,h in versions[2].items()if n.startswith(('src/base/','src/gmatutil/','plugins/'))and versions[1].get(n)!=h];check(changed==expected,'Production increment changed')
 for n in expected:check(sha(stage/'core-preimages-v2'/n)==versions[1][n]==delta['preimage_sha256'][n]and sha(stage/'core-postimages-v2'/n)==versions[2][n]==delta['postimage_sha256'][n],'Exact core pre/postimage differs')
 check(sha(stage/'core-increment-v2.patch')==delta['patch_sha256'],'Exact increment patch changed')
 interpreter=source(archive,expected[0]).decode();utility=source(archive,expected[1]).decode();header=source(archive,expected[2]).decode()
 check('obj->IsAutomaticGlobal() && obj->IsGlobal() && !obj->IsLocal() &&'in interpreter and '!obj->IsOfType(Gmat::PARAMETER)'in interpreter and 'theObjectMap->find(outputArgs[i])'in interpreter and 'obj->GetName() == outputArgs[i]'in interpreter,'Actual-instance output validation guard differs')
 check('outputNames[i].empty() &&'in utility and 'find(inputs.begin(), inputs.end(), outputs[i]) == inputs.end()'in utility and 'outputRows, outputCols, StringArray());'in utility and header.count('GetFunctionOutputTypes(')==2,'Local/input precedence or old exported signature changed')
 for n,h in prep['original_source_sha256'].items():
  if n=='src/qtgui/CMakeLists.txt':continue
  check(sha(stage/'preimages-v1'/n)==h and all(v[n]==h for v in versions.values()),'Original donor driver/catalog changed')
 for n in ['FunctionOutputScopeTests.cpp','NasaFunctionIoTests.cpp','NasaMatlabFunctionIoTests.cpp']:
  path='src/qtgui/tests/'+n;check(versions[1][path]==versions[2][path]==baseline[path],'Original rejection/scope assertion source changed')
 for n in ['NasaFunctionAutomaticGmatHelperTests.cpp','NasaFunctionAutomaticReferenceTests.cpp','NasaFunctionAutomaticGmatHelperCases.json','NasaFunctionAutomaticReferenceCases.json']:
  path='src/qtgui/tests/'+n;check(versions[1][path]==versions[2][path],'New native fixture/assertions changed across the production repair')
 helper_contract=functions(archive,'src/qtgui/tests/AuditFunctionBuiltinAutomaticGlobal.py',{'numeric','contract'})['contract'];reference_contract=functions(archive,'src/qtgui/tests/AuditFunctionGlobalResources.py',{'identities','resource'})['resource']
 spec=json.loads(source(archive,'src/qtgui/tests/NasaFunctionAutomaticGmatHelperCases.json'))['cases'];check(len(spec)==1,'User-function case census changed');spec=spec[0]
 check(spec['key']==spec['helper_type']=='GmatFunction'and spec['expected']==[60,7]and spec['main_expected']==[28,5]and spec['tolerance']==1e-12,'Independent quadratic/input-copy bounds changed')
 check(spec['support_files']==[dict(name='NativeHelper.gmf',contents='function [answer] = NativeHelper(input)\nCreate Variable answer;\nBeginMissionSequence;\nanswer = input * input + 11;\ninput = 999;\n'),dict(name='CallerHelper.gmf',contents='function [answer] = CallerHelper(input)\nCreate Variable answer;\nBeginMissionSequence;\nanswer = input * input + 3;\ninput = 888;\n')],'Actual distinct leaf recipes changed')
 refspecs=json.loads(source(archive,'src/qtgui/tests/NasaFunctionAutomaticReferenceCases.json'))['cases'];oldrefs=json.loads(source(archive,'src/qtgui/tests/NasaFunctionReferenceCases.json'))['cases'];check(len(refspecs)==2 and all(c==next(v for v in oldrefs if v['key']==c['key'])for c in refspecs),'Original reference inputs/oracles changed')
 receipt=load(stage/'stage-receipt-v1.json');previous=load(prior/'stage-receipt-v1.json')
 for k in ['canonical_verified','original_total','target','still_needed','historical_verified','historical_total','original_hold_keys','new_requirement_credit','whole_row_qualified','scientific_approval','technical_approval','full_current_build_qualified','full_NASA_qualified','collective_owned_types','total_owned_types','remaining_owned_types']:check(receipt[k]==previous[k],'Original scope/hold/gate changed: '+k)
 for run,version in receipt['gui_execution_versions'].items():
  mapped=load(stage/(run+'-loaded-images.json'));custody['custody'](mapped,builds[version],stage/f'build-receipt-v{version}.json',counts);check(mapped['returncode']==(1 if version==1 else 0),'Recorded invocation outcome changed')
 def checked(selected):
  root=stage/selected['run']/selected['test'];result=load(root/'result.json');check(sha(root/'result.json')==selected['result_sha256']and result['status']=='passed'and result['returncode']==0 and result['requested_visibility']==selected['mode'],'Native result/version/mode differs');junit(root,selected['test'])
  invocation=load(stage/selected['run']/'results.json')
  for n in result['sources']:check(invocation['source_hashes'][n]==versions[selected['source_version']][n],'Actual native source binding differs')
  check(not result['trace']['screenshots'],'Unexpected screenshot')
  if selected['mode']=='visible':
   desktop=invocation['visible_display'];check(desktop['resizable']and desktop['start_minimized']and 'Iconic'in desktop['initial_host_window_state']and 'FULLSCREEN'not in desktop['initial_host_window_state']and result['trace']['recorded_platforms']==['xcb']and result['trace']['native_exposure_events']>0,'Visible minimized/resizable exposure policy changed');counts['visible_exposures']+=result['trace']['native_exposure_events']
  counts['passed_GUI_contexts']+=1;return root,result
 proofs=[]
 for selected in receipt['selected']:
  root,result=checked(selected);proof=load(root/'proof.json');check(sha(root/'proof.json')==selected['proof_sha256'],'Native proof changed')
  emitted=[json.loads(x)for x in re.findall(r'NASA_CHECK (\{[^\n]+\})',(root/'test.log').read_text())];check(emitted==[proof],'Native emitted proof differs')
  is_helper=selected['case']=='GmatFunction';recipe=spec if is_helper else next(s for s in refspecs if s['key']==selected['case']);contract=lambda p:helper(p,recipe,helper_contract)if is_helper else reference(p,recipe,reference_contract);contract(proof)
  driver='src/qtgui/tests/NasaFunctionAutomatic'+('GmatHelper'if is_helper else'Reference')+'Tests.cpp';catalog='src/qtgui/tests/NasaFunctionAutomatic'+('GmatHelper'if is_helper else'Reference')+'Cases.json'
  check(result['sources']==[driver]and str(repo/catalog)in result['registered_command']and result['registered_command'][-1]==selected['case'],'Actual native registration/recipe route changed')
  check(load(stage/selected['run']/'results.json')['source_hashes'][catalog]==versions[selected['source_version']][catalog],'Actual case catalog binding differs')
  if is_helper:
   for file in spec['support_files']:check((root/file['name']).read_text()==file['contents'],'Actual leaf function bytes differ')
   for row in proof['runs']:
    raw=(root/(row['phase']+'-values.csv')).read_text().strip().splitlines();check(len(raw)==4,'Raw helper report rows omitted')
    for line,name in zip(raw,['main','first','second','third']):check([float(x)for x in line.split(',')if x.strip()]==row[name],'Actual helper report differs')
    check(load(root/(row['phase']+'-prototypes.json'))==load(root/'prototypes-before.json'),'Actual configured prototypes changed')
    counts['helper_store_identities']+=len(row['identities']);counts['counter_store_identities']+=len(row['identities']);counts['independent_typed_results']+=4
   for n in ['initial.script','valid.gmf','nested.gmf']:check('Global CallerHelper;'not in(root/n).read_text(),'Explicit Global substituted for automatic helper identity')
   check('[Measured] = NativeHelper(Input);'in(root/'valid.gmf').read_text()and '[Measured] = CallerHelper(Input);'in(root/'initial.script').read_text(),'Actual distinct main/private calls absent')
  else:
   for row in proof['phases']:
    tag=str(row['phase']);check([float(x)for x in(root/(tag+'-values.txt')).read_text().split()]==row['raw_values'],'Raw reference continuation differs')
    values=[]
    for text in re.findall(r'(?:^|\n)Shared\.'+re.escape(recipe['field'])+r' =\s*\n([^\n]+)',(root/(tag+'-messages.txt')).read_text()):
     text=text.strip()
     if recipe['kind']=='list':
      for c in ['{','}','[',']',"'",'"',' ']:text=text.replace(c,'')
     else:text=text.strip("'")
     values.append(text)
    check(values==row['field_writes'],'Actual main/direct/nested property Writes differ');counts['reference_store_identities']+=len(row['identities']);counts['typed_property_Writes']+=len(values);counts['independent_reference_report_scalars']+=len(row['raw_values'])
   check((root/'initial.gmf').read_text().replace('Keep = 103;','Keep = 41;')==(root/'edited.gmf').read_text(),'Actual file edit escaped sentinel')
   for n in ['caller.script','edited.gmf','nested.gmf']:check('Global Shared;'not in(root/n).read_text(),'Explicit Global substituted for automatic reference identity')
   check('[Shared, Mark] = ReadShared();'in(root/'nested.gmf').read_text(),'Actual whole-object nested return absent')
  counts['new_passed_GUI_contexts']+=1;counts['new_missions']+=proof['missions'];counts['new_function_calls']+=proof['function_calls'];proofs.append((proof,recipe,is_helper))
 for selected in receipt['affected_original_regressions']:
  root,result=checked(selected);counts['affected_original_regressions']+=1
  if selected['test']=='QtGui.NasaFunctionIoMissingOutput':check('Missing output declaration of "out"'in(root/'missing-output-messages.txt').read_text()and [float(x)for x in(root/'states.txt').read_text().split()]==[2.5,6.5],'Original GMAT rejection/recovery differs');counts['original_missing_output_guards']+=1
  if selected['test']=='QtGui.NasaMatlabFunctionIoUndeclaredOutput':check('Missing output declaration of "out"'in(root/'build-messages.txt').read_text()and load(root/'proof.json')==dict(case='UndeclaredOutput',checkpoint='matlab-function-undeclared-output',expected_rejection=True,new_requirement_credit=0,outcome='rejection-checked'),'Original MATLAB undeclared-output rejection differs');counts['original_missing_output_guards']+=1
 for selected in receipt['retained_failed_GUI_contexts']:
  root=stage/selected['run']/selected['test'];result=load(root/'result.json');check(sha(root/'result.json')==selected['result_sha256']and result['status']=='failed'and result['returncode']!=0 and selected['source_version']==1 and 'Missing output declaration of "Shared"'in(root/'build-messages.txt').read_text(),'Original failed counterexample lost');counts['retained_failed_GUI_contexts']+=1
 support_root=stage/receipt['support_execution'];support=load(support_root/'execution.json');check(support['returncode']==0,'Declaration support failed');custody['custody'](support,builds[2],stage/'build-receipt-v2.json',counts)
 tests=ElementTree.parse(support_root/'ctest.xml').findall('.//testcase');check([t.get('name')for t in tests]==['QtAcceptance.FunctionOutputScope','QtAcceptance.FunctionAutomaticOutputScope']and all(t.find(k)is None for t in tests for k in ['failure','error','skipped']),'Original/new declaration support JUnit differs');counts['original_declaration_cases']=9;counts['new_declaration_cases']=12
 for proof,recipe,is_helper in [proofs[0],next(x for x in proofs if not x[2])]:
  damages=([lambda p:p['runs'][0]['identities'][0].update(helper_store_object_pointer='0'),lambda p:p['runs'][0]['identities'][0].update(helper_automatic=False),lambda p:p['runs'][0]['identities'].pop(),lambda p:p['runs'][0]['main'].__setitem__(0,0),lambda p:p['runs'][0]['first'].__setitem__(0,0),lambda p:p['runs'][0].update(continuation=0),lambda p:p['runs'].pop()]if is_helper else[lambda p:p['phases'][0]['identities'][0].update(global_store_object_pointer='dead'),lambda p:p['phases'][0].update(automatic=False),lambda p:p['phases'][0].update(decoy_global=False),lambda p:p['phases'][0]['field_writes'].__setitem__(0,'wrong'),lambda p:p['phases'][0]['identities'].pop(),lambda p:p['phases'][0]['raw_values'].__setitem__(4,0),lambda p:p['phases'].pop()])
  for damage in damages:
   bad=copy.deepcopy(proof);damage(bad);rejected=False
   try:helper(bad,recipe,helper_contract)if is_helper else reference(bad,recipe,reference_contract)
   except(ValueError,KeyError,IndexError):rejected=True
   check(rejected,'Damaged automatic identity/value/scope proof escaped');counts['damaged_proofs_rejected']+=1
 matrix=load(stage/'automatic-resource-scope-matrix-v1.json');old=load(prior/'builtin-auto-scope-matrix-v1.json');check(len(matrix['rows'])==566 and all(matrix[k]==v for k,v in old.items()if k!='rows'),'Prior matrix scope changed')
 for a,b in zip(old['rows'],matrix['rows']):check(all(b[k]==v for k,v in a.items())and b['automatic_resource_identity_witnesses']==[s for s in receipt['selected']if s['case']==a['factory_type']],'Original role/eligibility/whole conjunction changed')
 remaining=load(stage/'remaining-requirement-audit-v1.json');old=load(prior/'remaining-requirement-audit-v1.json');check(len(remaining['rows'])==108 and all(remaining[k]==v for k,v in old.items()),'Exact open rows/fingerprints/closure gates changed')
 processes=load(stage/receipt['process_audit']);check(not processes['matching_native_processes']and not processes['matching_visible_servers']and len(processes['recorded_native_identities'])==counts['mapped_native_processes']==230 and len(processes['recorded_visible_servers'])==3,'Terminal process census differs')
 check(counts['new_passed_GUI_contexts']==6 and counts['passed_GUI_contexts']==34 and counts['affected_original_regressions']==28 and counts['retained_failed_GUI_contexts']==4 and counts['original_missing_output_guards']==4 and counts['helper_store_identities']==18 and counts['reference_store_identities']==36 and len(set(receipt['original_hold_keys']))==59,'Complete new/regression/original hold census differs')
 result=dict(status='passed-bounded-automatic-resource-identity-audit',counts=dict(counts),canonical_verified=1050,original_total=1158,target=1101,still_needed=51,original_open_rows=108,original_holds=59,pending_core_paths=280,core_increment_paths=3,new_requirement_credit=0,whole_row_qualified=False,scientific_technical_current_build_NASA_qualified=False)
 check(not args.output.exists(),'Never overwrite an audit');args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
