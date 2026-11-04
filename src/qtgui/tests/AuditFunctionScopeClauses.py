"""Replay exact scope-clause joins,retaining every earlier proof and gate."""
import argparse,copy,hashlib,json,subprocess,sys,tarfile,tempfile
from pathlib import Path

STAGE='build/nasa-coverage/coverage95-20261104-function-scope-clause-join'
PRIOR='build/nasa-coverage/coverage95-20261104-function-automatic-resources'
def check(ok,message):
 if not ok:raise ValueError(message)
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def load(path):return json.loads(Path(path).read_text())
def source(archive,name):
 with tarfile.open(archive)as tar:return tar.extractfile(name).read()

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,required=True);p.add_argument('--manifest',type=Path);p.add_argument('--output',type=Path,required=True);args=p.parse_args();repo=args.repo.resolve();stage=repo/STAGE;prior=repo/PRIOR;counts={}
 if args.manifest:
  seal=load(args.manifest);check(sha(__file__)==seal['auditor_sha256'],'Auditor identity differs')
  for n,h in seal['regular_artifact_sha256'].items():check(not(repo/n).is_symlink()and sha(repo/n)==h,'Immutable artifact differs: '+n)
  check(seal['symbolic_links']=={},'Offline review acquired runtime aliases');counts.update(immutable_artifact_bindings=len(seal['regular_artifact_sha256']),temporal_alias_routes=0)
 archive=stage/'reviewed-sources-v1.tar.gz';identities=load(stage/'reviewed-source-sha256-v1.json')
 with tarfile.open(archive)as tar:
  check(set(tar.getnames())==set(identities),'Offline reviewed source universe differs')
  for n,h in identities.items():check(hashlib.sha256(tar.extractfile(n).read()).hexdigest()==h,'Offline source bytes differ')
 counts['offline_reviewed_source_paths']=len(identities)
 prep=load(stage/'preparation-v1.json');check(sha(prior/'evidence-manifest-v1.json')==prep['prior_manifest_sha256']and sha(prior/'evidence-audit-v1.json')==prep['prior_audit_sha256']and sha(prior/'frozen-audit-replay-v1.json')==prep['prior_replay_sha256'],'Prior source-versioned seal/replay changed');oldseal=load(prior/'evidence-manifest-v1.json')
 with tempfile.TemporaryDirectory(prefix='gmat-scope-clauses-prior-')as folder:
  root=Path(folder);script=root/'prior.py';output=root/'audit.json';script.write_bytes(source(repo/oldseal['final_source_archive'],'src/qtgui/tests/AuditFunctionAutomaticResources.py'))
  done=subprocess.run([sys.executable,str(script),'--repo',str(repo),'--manifest',str(prior/'evidence-manifest-v1.json'),'--output',str(output)],cwd=root,capture_output=True,text=True)
  check(done.returncode==0,'Frozen prior/owned chain failed: '+done.stderr[-1800:]);check(output.read_bytes()==(prior/'evidence-audit-v1.json').read_bytes(),'Frozen prior/owned chain differs')
 counts['full_predecessor_and_owned_chain_replays']=1
 check(sha(repo/'build/nasa-coverage/coverage95-20261103-matlab-c-interface/coverage-snapshot-v1.json')=='4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6','Canonical authority ledger changed')
 before=load(prior/'final-reviewed-source-sha256-v1.json');check(all(identities[n]==h for n,h in before.items()if n.startswith(('src/base/','src/gmatutil/','plugins/'))),'Production core source changed')
 reviewer='src/qtgui/tests/ReviewFunctionScopeClauses.py';module={'__name__':'frozen_clause_reviewer','__file__':reviewer};exec(compile(source(archive,reviewer),reviewer,'exec'),module)
 with tempfile.TemporaryDirectory(prefix='gmat-scope-clauses-review-')as folder:
  root=Path(folder);script=root/'review.py';output=root/'review.json';script.write_bytes(source(archive,reviewer))
  done=subprocess.run([sys.executable,str(script),'--repo',str(repo),'--source-archive',str(archive),'--output',str(output)],cwd=root,capture_output=True,text=True)
  check(done.returncode==0,'Frozen source-specific reviewer failed: '+done.stderr[-1800:]);check(output.read_bytes()==(stage/'exact-scope-clause-join-v3.json').read_bytes(),'Frozen reviewer output differs')
 counts['frozen_reviewer_replays']=1
 review=load(stage/'exact-scope-clause-join-v3.json');baseline=load(prior/'automatic-resource-scope-matrix-v1.json');module['validate_join'](review,baseline,repo)
 check((stage/'exact-scope-clause-join-v2.json').read_bytes()==(stage/'exact-scope-clause-join-v3.json').read_bytes(),'Source-archive support changed the canonical clause data')
 facts=load(stage/'scope-source-observations-v1.json');check(facts==review['source_observations']and facts['eligibility_exclusions']==[]and facts['production_changes']==[]and not facts['custom_Star_creation_qualified']and not facts['independent_qualified_Parameter_Global_promotion_qualified'],'Source question was waived')
 for item in facts['sources']:check(identities[item['path']]==item['sha256'],'Source observation not frozen')
 receipt=load(stage/'stage-receipt-v1.json');previous=load(prior/'stage-receipt-v1.json')
 for name in ['canonical_verified','original_total','target','still_needed','historical_verified','historical_total','original_hold_keys','new_requirement_credit','whole_row_qualified','scientific_approval','technical_approval','full_current_build_qualified','full_NASA_qualified','collective_owned_types','total_owned_types','remaining_owned_types']:check(receipt[name]==previous[name],'Original scope/hold/gate changed: '+name)
 check(len(set(receipt['original_hold_keys']))==59 and receipt['pending_core_paths']==280 and receipt['core_increment_paths']==[]and receipt['new_native_runs']==receipt['new_builds']==0,'New native/core change or original hold lost')
 remaining=load(stage/'remaining-requirement-audit-v1.json');old=load(prior/'remaining-requirement-audit-v1.json');check(len(remaining['rows'])==108 and all(remaining[k]==v for k,v in old.items()if k!='rows'),'Exact remaining authority/scope changed')
 for a,b in zip(old['rows'],remaining['rows']):check(all(b[k]==v for k,v in a.items()),'Exact original row/fingerprint/gap changed')
 controls=[lambda v:v['rows'].pop(),lambda v:next(r for r in v['rows']if r['factory_type']=='Sign')['exact_scope_clause_join_v1']['contexts'].pop(),lambda v:v['rows'][0].update(role='excluded'),lambda v:next(r for r in v['rows']if r['factory_type']=='Star')['exact_scope_clause_join_v1'].update(custom_Star_creation_qualified=True),lambda v:next(r for r in v['rows']if r['factory_type']=='X')['exact_scope_clause_join_v1'].update(independent_named_Parameter_Global_promotion_observed=True),lambda v:next(r for r in v['rows']if r['factory_type']=='String')['exact_scope_clause_join_v1'].update(eligibility_exclusion_assigned=True),lambda v:v['exact_parent_and_children'][0].update(canonical_already_counted=True),lambda v:v.update(new_requirement_credit=1),lambda v:v['counts'].update(unique_retained_native_proofs_checked=1),lambda v:next(r for r in v['rows']if r['factory_type']=='Array')['exact_scope_clause_join_v1']['contexts'][0].update(proof_sha256='0')]
 for corrupt in controls:
  bad=copy.deepcopy(review);corrupt(bad);rejected=False
  try:module['validate_join'](bad,baseline,repo)
  except(ValueError,KeyError,IndexError):rejected=True
  check(rejected,'Damaged clause/row/proof/eligibility model escaped')
 counts['damaged_clause_models_rejected']=len(controls);counts.update(review['counts'])
 result=dict(status='passed-complete67-type-exact-scope-clause-join-audit;whole-conjunction-open',counts=counts,canonical_verified=1050,original_total=1158,target=1101,still_needed=51,original_open_rows=108,original_holds=59,pending_core_paths=280,core_increment_paths=0,new_native_runs=0,new_requirement_credit=0,whole_row_qualified=False,scientific_technical_current_build_NASA_qualified=False)
 check(not args.output.exists(),'Never overwrite a clause audit');args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
