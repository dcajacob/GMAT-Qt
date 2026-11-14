"""Replay the unchanged Global-resource contract for the new String witness."""
import argparse,ast,copy,hashlib,json,re,subprocess,sys,tarfile,tempfile
from collections import Counter
from pathlib import Path
from xml.etree import ElementTree

STAGE='build/nasa-coverage/coverage95-20261104-function-global-string'
PRIOR='build/nasa-coverage/coverage95-20261104-function-resource-global-join'
def check(ok,message):
 if not ok:raise ValueError(message)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text())
def source(archive,name):
 with tarfile.open(archive)as tar:return tar.extractfile(name).read()
def functions(archive,name,names):
 tree=ast.parse(source(archive,name),filename=name)
 nodes=[n for n in tree.body if isinstance(n,ast.FunctionDef)and n.name in names]
 check({n.name for n in nodes}==set(names),'Frozen function contract absent')
 namespace=dict(require=check,check=check,sha=sha,load=load,hashlib=hashlib,tarfile=tarfile)
 exec(compile(ast.Module(body=nodes,type_ignores=[]),name,'exec'),namespace)
 return namespace
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,required=True);p.add_argument('--manifest',type=Path);p.add_argument('--output',type=Path,required=True);args=p.parse_args();repo=args.repo.resolve();stage=repo/STAGE;prior=repo/PRIOR;counts=Counter()
 if args.manifest:
  seal=load(args.manifest);check(sha(__file__)==seal['auditor_sha256'],'Auditor identity changed')
  for n,h in seal['regular_artifact_sha256'].items():check(not(repo/n).is_symlink()and sha(repo/n)==h,'Immutable artifact changed: '+n)
  for n,target in seal['symbolic_links'].items():check((repo/n).is_symlink()and str((repo/n).readlink())==target,'Temporal alias route changed')
  with tarfile.open(repo/seal['final_source_archive'])as tar:
   check(set(tar.getnames())==set(seal['final_source_sha256']),'Final source scope changed')
   for n,h in seal['final_source_sha256'].items():check(hashlib.sha256(tar.extractfile(n).read()).hexdigest()==h,'Final source bytes changed')
  counts['immutable_artifact_bindings']=len(seal['regular_artifact_sha256']);counts['temporal_alias_routes']=len(seal['symbolic_links'])
 prep=load(stage/'preparation-v1.json');check(sha(prior/'evidence-manifest-v1.json')==prep['prior_manifest_sha256']and sha(prior/'evidence-audit-v1.json')==prep['prior_audit_sha256'],'Prior final seal changed')
 previous_seal=load(prior/'evidence-manifest-v1.json')
 with tempfile.TemporaryDirectory(prefix='gmat-string-prior-audit-')as folder:
  script=Path(folder)/'prior.py';output=Path(folder)/'audit.json';script.write_bytes(source(repo/previous_seal['final_source_archive'],PRIOR+'/audit-join-v1.py'))
  completed=subprocess.run([sys.executable,str(script),'--repo',str(repo),'--manifest',str(prior/'evidence-manifest-v1.json'),'--output',str(output)],cwd=folder,capture_output=True,text=True)
  check(completed.returncode==0,'Frozen prior audit failed: '+completed.stderr[-1500:]);check(output.read_bytes()==(prior/'evidence-audit-v1.json').read_bytes(),'Frozen prior audit differs')
 check(sha(repo/'build/nasa-coverage/coverage95-20261103-matlab-c-interface/coverage-snapshot-v1.json')=='4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6','Canonical ledger changed')
 archive=stage/'executed-sources-v1.tar.gz';custody=functions(archive,'src/qtgui/tests/AuditFunctionTextEphemeris.py',{'custody','archive_sources'});identities=custody['archive_sources'](stage,1);build=load(stage/'build-receipt-v1.json')
 check(build['status']=='passed'and build['returncode']==0 and build['inputs_unchanged']and build['source_sha256']==identities,'Build/source custody changed');counts['source_capsules']=1
 driver='src/qtgui/tests/NasaFunctionGlobalResourceTests.cpp';old_catalog='src/qtgui/tests/NasaFunctionGlobalResourceCases.json'
 for n in [driver,old_catalog]:check(sha(stage/'preimages-v1'/n)==identities[n],'Original native assertions or28-case catalog changed')
 base=load(prior/'final-reviewed-source-sha256-v1.json');core={n:h for n,h in base.items()if n.startswith(('src/base/','src/gmatutil/','plugins/'))};check(all(identities[n]==h for n,h in core.items()),'Production core source changed')
 contract=functions(archive,'src/qtgui/tests/AuditFunctionGlobalResources.py',{'identities','resource'})
 catalog=json.loads(source(archive,'src/qtgui/tests/NasaFunctionGlobalUserResourceCases.json'));check(len(catalog['cases'])==1,'Supplemental String scope changed');spec=catalog['cases'][0]
 check((spec['key'],spec['type'],spec['field'],spec['kind'],spec['first'],spec['second'])==('String','String','Value','string','shared caller Ω value','decoy caller Δ value'),'Independent String literals changed')
 receipt=load(stage/'stage-receipt-v1.json');previous=load(prior/'stage-receipt-v1.json')
 for k in ['canonical_verified','original_total','target','still_needed','historical_verified','historical_total','original_hold_keys','new_requirement_credit','whole_row_qualified','scientific_approval','technical_approval','full_current_build_qualified','full_NASA_qualified','collective_owned_types','total_owned_types','remaining_owned_types']:check(receipt[k]==previous[k],'Original scope/gate changed: '+k)
 proofs=[]
 for selected in receipt['selected']:
  run=selected['run'];root=stage/run/selected['test'];result=load(root/'result.json');proof=load(root/'proof.json')
  check(result['status']=='passed'and result['returncode']==0 and result['requested_visibility']==selected['mode']and sha(root/'proof.json')==selected['proof_sha256'],'Native result/proof/mode changed')
  check(result['sources']==[driver]and 'src/qtgui/tests/NasaFunctionGlobalUserResourceCases.json'in load(stage/run/'results.json')['source_hashes'],'Native source/catalog unbound')
  tests=ElementTree.parse(root/'ctest.xml').findall('.//testcase');check(len(tests)==1 and tests[0].get('name')==selected['test']and all(tests[0].find(k)is None for k in ['error','failure','skipped']),'Native JUnit changed')
  emitted=[json.loads(s)for s in re.findall(r'NASA_CHECK (\{[^\n]+\})',(root/'test.log').read_text())];check(emitted==[proof],'Native process proof differs')
  contract['resource'](proof,spec)
  for phase in proof['phases']:
   tag=str(phase['phase']);check([float(x)for x in(root/(tag+'-values.txt')).read_text().split()]==phase['raw_values'],'Raw continuation report differs')
   values=[s.strip().strip("'")for s in re.findall(r'(?:^|\n)Shared\.Value =\s*\n([^\n]+)',(root/(tag+'-messages.txt')).read_text())];check(values==phase['field_writes'],'Actual Unicode property Write differs')
   counts['global_store_identities']+=len(phase['identities']);counts['independent_String_Write_values']+=len(values);counts['independent_report_scalars']+=len(phase['raw_values'])
  check((root/'initial.gmf').read_text().replace('Keep = 103;','Keep = 41;')==(root/'edited.gmf').read_text(),'Actual file edit changed beyond the sentinel')
  for n in ['caller.script','edited.gmf','nested.gmf']:check('Global Shared;'in(root/n).read_text(),'Explicit Global declaration omitted from a used scope')
  check('[Shared, Mark] = ReadShared();'in(root/'nested.gmf').read_text(),'Actual nested return absent')
  custody['custody'](load(stage/(run+'-loaded-images.json')),build,stage/'build-receipt-v1.json',counts)
  if selected['mode']=='visible':
   desktop=load(stage/run/'results.json')['visible_display'];check(desktop['resizable']and desktop['start_minimized']and 'Iconic'in desktop['initial_host_window_state']and 'FULLSCREEN'not in desktop['initial_host_window_state']and result['trace']['recorded_platforms']==['xcb']and result['trace']['native_exposure_events']>0,'Visible minimized/resizable exposure policy changed');counts['visible_exposures']+=result['trace']['native_exposure_events']
  check(not result['trace']['screenshots'],'Unexpected screenshot');counts['passed_native_contexts']+=1;counts['missions']+=proof['missions'];counts['function_calls']+=proof['function_calls'];proofs.append(proof)
 for damage in [lambda p:p['phases'][0]['identities'][0].update(global_store_object_pointer='dead'),lambda p:p['phases'][0].update({'global':False}),lambda p:p['phases'][0]['field_writes'].__setitem__(0,'wrong'),lambda p:p['phases'][0].update(decoy_value='wrong'),lambda p:p['phases'][0]['identities'].pop(),lambda p:p['phases'][0]['raw_values'].__setitem__(4,0),lambda p:p['phases'].pop()]:
  bad=copy.deepcopy(proofs[0]);damage(bad);rejected=False
  try:contract['resource'](bad,spec)
  except(ValueError,KeyError,IndexError):rejected=True
  check(rejected,'Damaged Global String proof escaped');counts['damaged_proofs_rejected']+=1
 matrix=load(stage/'string-global-scope-matrix-v1.json');old=load(prior/'retained-route-join-v4.json');check(len(matrix['rows'])==566 and all(matrix[k]==v for k,v in old.items()if k!='rows'),'Full retained matrix scope changed')
 for a,b in zip(old['rows'],matrix['rows']):check(all(b[k]==v for k,v in a.items())and b['String_Global_witnesses']==(receipt['selected']if a['factory_type']=='String'else[]),'Prior role/eligibility/conjunction changed')
 remaining=load(stage/'remaining-requirement-audit-v1.json');old=load(prior/'remaining-requirement-audit-v1.json');check(len(remaining['rows'])==108 and all(remaining[k]==v for k,v in old.items()),'Original exact requirements/fingerprints/gaps changed')
 processes=load(stage/receipt['process_audit']);check(not processes['matching_native_processes']and not processes['matching_visible_servers']and len(processes['recorded_native_identities'])==counts['mapped_native_processes']==12,'Recorded terminal native process census changed')
 check(len(set(receipt['original_hold_keys']))==59 and receipt['core_increment_paths']==[],'Original holds/core scope changed')
 result=dict(status='passed-bounded-String-Global-identity-audit',counts=dict(counts),canonical_verified=1050,original_total=1158,target=1101,still_needed=51,original_open_rows=108,original_holds=59,collective_owned_types=36,pending_core_paths=279,core_increment_paths=0,new_requirement_credit=0,whole_row_qualified=False,scientific_technical_current_build_NASA_qualified=False)
 check(not args.output.exists(),'Never overwrite an audit');args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
