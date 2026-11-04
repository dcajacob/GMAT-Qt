#!/usr/bin/env python3
"""Reuse exact gravity-boundary receipts without executing their GUI scenarios.

A bounded evidence audit, not a test runner or latest-build qualification.
"""
from pathlib import Path
from collections import Counter
import argparse,copy,datetime,hashlib,json,math,re,sys
import AcceptanceSnapshot as snapshot
import NasaGuiCoverage as nasa

CACHE={}
def sha(path):
 p=Path(path);s=p.stat();key=(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns,s.st_ctime_ns)
 if key not in CACHE:
  with p.open('rb')as stream:CACHE[key]=hashlib.file_digest(stream,'sha256').hexdigest()
 return CACHE[key]
def read(p):return json.loads(Path(p).read_text())
def write(out,name,value):(out/name).write_text(json.dumps(value,indent=2)+'\n')
def integer(text):
 if not re.fullmatch(r'-?[0-9]+',text):return None
 value=int(text);return value if -2147483648<=value<=2147483647 else None

def validate_recipe(recipe,source):
 checks=recipe['checks'];assert len(checks)==52 and len({c['checkpoint']for c in checks})==52
 counts=Counter(c['kind']for c in checks);assert counts=={'coupled-pair':12,'gravity-domain':16,'tide-domain':23,'transaction':1}
 for check in checks:
  if check['kind']=='coupled-pair':
   assert check['cpp_literal']in source;d=integer(check['fields']['Degree']);o=integer(check['fields']['Order']);legal=d is not None and o is not None and 0<=o<=d
   assert legal==check['valid']
  elif check['kind']=='gravity-domain':
   assert '{"'+check['case']+'",'in source
   if 'StmLimit'in check['fields']:
    v=integer(check['fields']['StmLimit']);assert (v is not None and v>=0)==check['valid']
   else:
    d=integer(check['fields']['Degree']);o=integer(check['fields']['Order']);limit=(4,2)if'PotentialFile'in check['fields']else(70,70)
    assert (d is not None and o is not None and 0<=d<=limit[0]and 0<=o<=min(d,limit[1]))==check['valid']
   for f,value in check['fields'].items():
    literal='{"'+f+'",'+(value if value=='lowOrder'else'"'+value+'"')+'}'
    start=source.index('{"'+check['case']+'",');end=source.index('},'+str(check['valid']).lower()+'}',start)+1;assert literal in source[start:end],(check,literal)
  elif check['kind']=='tide-domain':
   start=source.index('{"'+check['case']+'","'+check['body']+'",');end=source.index('}',start)+1
   assert ',"'+check['model']+'",'+str(check['valid']).lower()+'}'in source[start:end]
 assert len(recipe['fields'])==6 and len({f['field']for f in recipe['fields']})==6
 for field in recipe['fields']:
  assert field['checkpoints']and set(field['checkpoints'])<={c['checkpoint']for c in checks}
  assert not field['whole_field_qualified']and not field['science_qualified']and not field['native_OS_input_qualified']
  for anchor in field['assertion_anchors']:assert anchor in source

def validate_proofs(recipe,proofs):
 assert len(proofs)==52 and len({p['checkpoint']for p in proofs})==52
 expected={c['checkpoint']:c for c in recipe['checks']};assert {p['checkpoint']for p in proofs}==set(expected)
 for p in proofs:
  c=expected[p['checkpoint']];assert p['outcome']=='passed'
  if c['kind']=='coupled-pair':assert p['case']==c['checkpoint'].removeprefix('gravity-bounds.')
  if c['kind']in ['gravity-domain','tide-domain']:assert p['case']==c['case']and p['valid']is c['valid']
  if c['kind']=='tide-domain':assert p['body']==c['body']
  facets=set(p['facets'])
  required={'coupled-pair':{'gui-input','atomic-rejection','pending-preserved','exact-undo-redo'},'gravity-domain':{'visible-field','file-boundary','integer-domain','pending-isolation','atomic-apply','rejection-retains-input','exact-undo-redo'},'tide-domain':{'visible-field','tide-file-model','atomic-apply','rejection-retains-input','correct-and-apply','exact-undo-redo'},'transaction':{'frontend-transaction','coupled-degree-order','atomic-rejection','source-history'}}[c['kind']]
  assert required<=facets

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();root=Path(__file__).resolve().parents[3];out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
 if(out/'final-verification.json').exists():raise ValueError('Finalized review is immutable;choose a new output directory')
 recipepath=root/'src/qtgui/tests/GravityBoundaryReuseCases.json';recipe=read(recipepath);source=root/recipe['source'];assert sha(source)==recipe['source_sha256'];validate_recipe(recipe,source.read_text());snapshot.sha=sha;nasa.sha=sha
 for name,h in recipe['current_reference_sha256'].items():assert sha(root/name)==h
 manifest=nasa.load_review(root,nasa.inventory(root));conflict=next(c for c in manifest['source_conflicts']if c['id']==recipe['known_source_conflict_id']);assert 'Degree <= Order'in conflict['detail']and'Order <= Degree'in conflict['detail']
 # The source typo is retained, with the independent wx ordering rule identified.
 guide=(root/'doc/help/src/Resource_ForceModel.xml').read_text();order=guide.split('<replaceable>PrimaryBodyName</replaceable>.Order',1)[1].split('</row>',1)[0];assert re.search(r'Degree</guilabel>\s*&lt;=\s*<guilabel>Order',order)
 wx=(root/'src/gui/propagator/PropagationConfigPanel.cpp').read_text();assert 'Order <= Degree'in wx
 artifacts={str(recipepath):sha(recipepath),str(source):sha(source)};contexts=[];negative=[];metrics=Counter();field_evidence=[]
 for item in recipe['receipts']:
  receiptpath=root/item['receipt'];assert sha(receiptpath)==item['receipt_sha256'];r=read(receiptpath);manifestpath=root/item['snapshot_manifest'];assert sha(manifestpath)==item['snapshot_manifest_sha256'];f=read(manifestpath);assert r['finished_utc']and r['snapshot_unchanged']and r['snapshot_manifest']==str(manifestpath)
  snapshot.verify(f);frozen=Path(f['root']);assert f['original_hashes'][recipe['source']]==recipe['source_sha256']and sha(frozen/recipe['source'])==recipe['source_sha256']
  c=next(c for c in r['results']if c['name']==recipe['test']);assert c['status']=='passed'and c['returncode']==0
  for k in ['log','junit']:assert sha(c[k])==c[k+'_sha256']==item[k+'_sha256'];artifacts[c[k]]=sha(c[k])
  proofs=nasa.proof_records(c);validate_proofs(recipe,proofs)
  assert r['execution_profile']==item['execution_profile']and r['execution_profile']['visible']==(item['mode']=='visible')
  if item['mode']=='visible':assert r['execution_profile']['scale']==3 and r['execution_profile']['width']==3966 and r['execution_profile']['height']==2506 and r['execution_profile']['start_minimized']and not r['execution_profile']['fixed_size']
  events=[json.loads(line)for q in Path(c['log']).parent.glob('trace*.jsonl')for line in q.read_text().splitlines()];eventcounts=Counter(e['event']for e in events);exposed=sum(e['event']=='checkpoint'and bool(e.get('exposed'))for e in events)
  if item['mode']=='visible':assert exposed>0
  def reject(label,values):
   try:validate_proofs(recipe,values)
   except(AssertionError,KeyError,TypeError,ValueError):negative.append(dict(mode=item['mode'],mutation=label,rejected=True))
   else:raise AssertionError('Corrupt legacy evidence accepted:'+label)
  reject('missing-checkpoint',proofs[1:]);reject('duplicate-checkpoint',proofs+[proofs[0]])
  bad=copy.deepcopy(proofs);bad[0]['outcome']='failed';reject('failed-checkpoint',bad)
  bad=copy.deepcopy(proofs);next(p for p in bad if p['checkpoint']=='gravity-domain.stm-zero')['valid']=False;reject('wrong-boundary-validity',bad)
  bad=copy.deepcopy(proofs);next(p for p in bad if p['checkpoint']=='tide-domain.lunar-solid')['body']='Earth';reject('wrong-primary-body',bad)
  bad=copy.deepcopy(proofs);bad[0]['facets']=[];reject('missing-proof-facets',bad)
  bad=copy.deepcopy(proofs);next(p for p in bad if p['checkpoint']=='tide-domain.file-solid')['case']='wrong';reject('wrong-file-case',bad)
  changed_refs=[dict(source=n,frozen_sha256=f['original_hashes'].get(n),current_sha256=sha(root/n))for n in ['src/qtgui/MainWindow.cpp','src/qtgui/ResourceEditor.cpp','src/qtgui/ResourceProperties.cpp']if f['original_hashes'].get(n)!=sha(root/n)]
  unpreserved=[n for n in recipe['current_reference_sha256']if n not in f['original_hashes']]
  contexts.append(dict(test=c['name'],mode=item['mode'],receipt=str(receiptpath),receipt_sha256=sha(receiptpath),snapshot_manifest=str(manifestpath),snapshot_manifest_sha256=sha(manifestpath),snapshot_id=f['snapshot_id'],log=c['log'],log_sha256=sha(c['log']),junit=c['junit'],junit_sha256=sha(c['junit']),assertion_source=recipe['source'],assertion_source_sha256=recipe['source_sha256'],structured_proofs=proofs,trace_event_counts=dict(eventcounts),exposed_checkpoints=exposed,legacy_visible_profile=r['execution_profile'],production_references_changed_since_receipt=changed_refs,production_reference_sources_not_preserved=unpreserved,full_current_build_qualified=False,input_delivery='Programmatic Qt field changes/callbacks;one tide picker Cancel/Open sequence',science_qualified=False,native_OS_input_qualified=False))
  artifacts[str(receiptpath)]=sha(receiptpath);artifacts[str(manifestpath)]=sha(manifestpath);artifacts[str(frozen/recipe['source'])]=sha(frozen/recipe['source'])
  for q in Path(c['log']).parent.glob('trace*.jsonl'):artifacts[str(q)]=sha(q)
  metrics.update(reused_contexts=1,reused_proofs=len(proofs),legacy_exposed_checkpoints=exposed)
 for field in recipe['fields']:
  witnesses=[]
  for c in contexts:
   proofs=[p for p in c['structured_proofs']if p['checkpoint']in field['checkpoints']];assert len(proofs)==len(field['checkpoints']);witnesses.append(dict(mode=c['mode'],receipt=c['receipt'],receipt_sha256=c['receipt_sha256'],snapshot_manifest=c['snapshot_manifest'],snapshot_manifest_sha256=c['snapshot_manifest_sha256'],log=c['log'],log_sha256=c['log_sha256'],selected_proofs=proofs))
  field_evidence.append(dict(**field,source=recipe['source'],source_sha256=recipe['source_sha256'],witnesses=witnesses,status='selected legacy bounds/Apply/history evidence reused;whole field remains open'))
 counts=Counter(c['kind']for c in recipe['checks']);valid=Counter('valid'if c['valid']else'invalid'for c in recipe['checks']if c['kind']!='transaction');assert valid=={'valid':17,'invalid':34}
 metrics.update(unique_checkpoint_types=52,field_records=6,valid_scenario_types=17,invalid_scenario_types=34,direct_transaction_types=1,new_GUI_cases=0,new_GUI_contexts=0,new_missions=0,existing_GUI_scenarios_reexecuted=0,new_screenshots=0,new_requirement_credit=0,new_historical_credit=0)
 write(out,'reused-contexts.json',contexts);write(out,'field-evidence.json',field_evidence);write(out,'negative-checks.json',negative)
 result=dict(status='52existing checkpoint types and6selected gravity fields audited without GUI replay',verified_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),metrics=dict(metrics),case_inventory=str(recipepath),case_inventory_sha256=sha(recipepath),assertion_source=recipe['source'],assertion_source_sha256=recipe['source_sha256'],known_source_conflict=conflict,negative_checks=negative,raw_artifact_sha256=artifacts,whole_requirement_credit=False,historical_credit=False,full_current_build_qualified=False,technical_approval=False,limits=recipe['limits'])
 write(out,'validation-summary.json',result);print(json.dumps(dict(status='passed',metrics=dict(metrics),negative_checks=len(negative))))
if __name__=='__main__':main()
