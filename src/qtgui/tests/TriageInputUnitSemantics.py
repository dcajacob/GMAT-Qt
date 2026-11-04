"""Retain every measured field in a unit-review worklist without credit."""
import argparse,gzip,hashlib,json
from collections import Counter,defaultdict
from pathlib import Path

def triage(review,census):
 receipt=json.loads((census/'receipt.json').read_text());types=defaultdict(set);inputs={}
 for r in receipt['results']:
  p=Path(r['compressed_output']);raw=p.read_bytes();digest=hashlib.sha256(raw).hexdigest()
  if digest!=r['compressed_sha256']:raise ValueError('Changed census output: '+str(p))
  inputs[str(p)]=digest;data=json.loads(gzip.decompress(raw));typ=data['requested']
  if typ=='--catalog':continue
  for state in [data]+data['single_selector_variants']:
   for f in state.get('native_parameters',[]):types[(typ,f.get('field'))].add(f.get('parameter_type'))
 rows=[]
 for field in review['fields']:
  r=dict(field);native_types=sorted(t for t in types[(r['type'],r['field'])] if t);docs=r['shipped_documentation_candidates'];units=[d['units'] for d in docs];simple={u.casefold().strip() for u in units};identity=(r['type'],r['field']);numeric=bool(set(native_types)&{'Real','Rvector','Rmatrix','Integer'})
  if identity==('MessageWindow','Dec'):status='preserved-corrupt-metadata-current-Precision-reviewed-separately'
  elif identity in {('String','Object'),('Variable','Object')}:status='metadata-not-rendered-by-scalar-editor'
  elif not docs:status='missing-active-unit-documentation'
  elif '?' in r['observed_GUI_unit']:status='ambiguous-native-unit-string-review'
  elif numeric and simple and simple<={'n/a','none','(none)','na','not applicable'}:status='numeric-no-physical-unit-entry-needs-meaning-review'
  elif any(u.casefold().strip().startswith(('see','depends')) or u.casefold().strip()=='unit' for u in units):status='dependent-or-narrative-unit-description-needs-review'
  elif r['observed_GUI_unit']:status='explicit-native-and-guide-unit-strings-need-consistency-review'
  else:status='guide-unit-description-needs-field-meaning-review'
  r.update(native_parameter_types=native_types,unit_review_bucket=status,semantic_unit_qualified=False);rows.append(r)
 return dict(scope='Review prioritization only. Native/guide strings and matching routes are not semantic or NASA qualification.',summary=dict(records=len(rows),buckets=dict(Counter(r['unit_review_bucket'] for r in rows))),fields=rows,census_output_sha256=inputs,qualified_keys=[],all_input_documentation_qualified=False)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--review',type=Path,required=True);p.add_argument('--census',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();raw=a.review.read_bytes();r=triage(json.loads(raw),a.census);r['review_sha256']=hashlib.sha256(raw).hexdigest();a.output.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r['summary']))
