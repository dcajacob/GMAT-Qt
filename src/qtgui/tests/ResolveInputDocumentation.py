"""Resolve structured DocBook labels without treating matches as qualification."""
import argparse,hashlib,json
from pathlib import Path
from lxml import etree
from AuditInputDocumentation import ALIASES
NS={'d':'http://docbook.org/ns/docbook'}
def words(node):return ' '.join(''.join(node.itertext()).split()) if node is not None else ''
def matching_labels(rows,field):
 exact=[r for r in rows if field in r['labels']]
 if exact:return exact
 folded=[r for r in rows if any(field.casefold()==label.casefold() for label in r['labels'])]
 spellings={label for r in folded for label in r['labels'] if label.casefold()==field.casefold()}
 return folded if len(spellings)==1 else []
def field_index(repo):
 records=[];sources={}
 for path in sorted((repo/'doc/help/src').glob('Resource_*.xml')):
  sources[str(path.relative_to(repo))]=hashlib.sha256(path.read_bytes()).hexdigest();root=etree.parse(str(path))
  for i,row in enumerate(root.findall('.//d:row',NS)):
   entries=row.findall('d:entry',NS)
   if not entries:continue
   metadata={words(v.find('d:term',NS)):words(v.find('d:listitem',NS)) for v in row.findall('.//d:varlistentry',NS)}
   if not metadata.get('Units'):continue
   copy=etree.fromstring(etree.tostring(entries[0]))
   for term in copy.findall('.//d:indexterm',NS):term.getparent().remove(term)
   labels=list(dict.fromkeys(words(v) for v in copy.findall('.//d:guilabel',NS)))
   plain=words(copy)
   if not labels or __import__('re').fullmatch(r'[A-Za-z][A-Za-z0-9_]*',plain):labels=list(dict.fromkeys(labels+[plain]))
   records.append(dict(source=str(path.relative_to(repo)),row_index=i,labels=labels,units=metadata['Units']))
 return records,sources

def resolve(repo,baseline):
 index,sources=field_index(repo);result=[]
 binding_path=repo/'src/qtgui/tests/InputDocumentationBindings.json'
 bindings=json.loads(binding_path.read_text())['bindings'] if binding_path.exists() else []
 rules={}
 for binding in bindings:
  key=(binding['type'],binding['field'])
  if key in rules:raise ValueError('Duplicate explicit documentation binding: '+str(key))
  rules[key]=binding
  for source in binding['source_evidence']:
   path=repo/source
   sources[source]=hashlib.sha256(path.read_bytes()).hexdigest()
 if binding_path.exists():sources[str(binding_path.relative_to(repo))]=hashlib.sha256(binding_path.read_bytes()).hexdigest()
 for original in baseline['fields']:
  r=dict(original);typ=r['type'];owner=ALIASES.get(typ,typ);prefixes=['Resource_'+owner]
  if typ=='Spacecraft':prefixes=['Resource_Spacecraft']
  if typ=='PropSetup':prefixes+=['Resource_NumericalIntegrators','Resource_EphemerisPropagator']
  choices=matching_labels([d for d in index if any(Path(d['source']).stem==p or (typ=='Spacecraft' and Path(d['source']).stem.startswith(p)) for p in prefixes)],r['field'])
  binding=rules.get((typ,r['field']))
  if binding:
   routed=matching_labels([d for d in index if Path(d['source']).stem=='Resource_'+binding['documentation_owner']],binding['documentation_field'])
   if not routed:raise ValueError('Explicit binding has no unit-bearing target: '+str(binding))
   choices=list({(v['source'],v['row_index']):v for v in choices+routed}.values())
   r['explicit_binding']=binding
  r['structured_documentation_candidates']=choices;r['binding_status']='structured-owner-label-found-needs-unit-review' if choices else 'unresolved-owner-label'
  result.append(r)
 return dict(scope='Structured owner/field bindings only, not semantic unit correctness or all-input completion.',summary=dict(records=len(result),structured_owner_matches=sum(bool(r['structured_documentation_candidates']) for r in result),unresolved=sum(not r['structured_documentation_candidates'] for r in result)),fields=result,source_sha256=sources,qualified_keys=[],all_input_documentation_qualified=False)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--baseline',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();raw=a.baseline.read_bytes();report=resolve(a.repo,json.loads(raw));report.update(baseline=str(a.baseline.resolve()),baseline_sha256=hashlib.sha256(raw).hexdigest());a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report['summary']))
