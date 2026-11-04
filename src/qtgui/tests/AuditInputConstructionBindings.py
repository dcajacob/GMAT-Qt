"""Check complete, unchanged source-template routes to unit conventions."""
import argparse,hashlib,json
from pathlib import Path
from lxml import etree
from InputConstructionInventory import inventory
NS={'d':'http://docbook.org/ns/docbook'}
def audit(repo,measured,manifest):
 if manifest.get('all_input_documentation_qualified') is not False:raise ValueError('Template routes cannot claim all-input qualification')
 for source,digest in measured['source_sha256'].items():
  if hashlib.sha256((repo/source).read_bytes()).hexdigest()!=digest:raise ValueError('Inventory source changed before binding audit: '+source)
 actual={r['key']:r for r in measured['records']};routes={};documents={}
 for binding in manifest['bindings']:
  key=binding['key']
  if key in routes:raise ValueError('Duplicate source-template route: '+key)
  if key not in actual:raise ValueError('Unknown source-template route: '+key)
  r=actual[key]
  for field in ('source','source_sha256','source_line_sha256'):
   if binding[field]!=r[field]:raise ValueError('Stale source-template route: '+key+' '+field)
  path=(repo/binding['documentation']).resolve()
  if not path.is_relative_to(repo.resolve()):raise ValueError('Unit topic outside repository')
  raw=path.read_bytes();documents[str(path.relative_to(repo))]=hashlib.sha256(raw).hexdigest();root=etree.fromstring(raw)
  sections=root.xpath('//*[@xml:id=$key]',key=binding['section'],namespaces={'xml':'http://www.w3.org/XML/1998/namespace'})
  if len(sections)!=1:raise ValueError('Missing/duplicate unit topic section: '+binding['section'])
  units=sections[0].xpath('./d:variablelist/d:varlistentry[d:term="Units"]/d:listitem/d:para',namespaces=NS)
  if len(units)!=1:raise ValueError('Missing unit description: '+key)
  text=''.join(units[0].itertext())
  if not text.strip() or hashlib.sha256(text.encode()).hexdigest()!=binding['unit_description_sha256']:raise ValueError('Changed unit description requires review: '+key)
  references=binding['reference_topic_sha256']
  if set(references)!=set(binding['reference_topics']):raise ValueError('Missing reference-topic identity: '+key)
  for topic,digest in references.items():
   p=repo/'doc/help/src'/topic
   if hashlib.sha256(p.read_bytes()).hexdigest()!=digest:raise ValueError('Changed field reference requires review: '+topic)
   documents[str(p.relative_to(repo))]=digest
  routes[key]=binding
 if set(actual)!=set(routes):raise ValueError('Unbound source templates: '+str(sorted(set(actual)-set(routes))))
 return dict(scope='Source-template unit-convention bindings only; concrete field expansion and semantic/native-mode review remain separate.',summary=dict(construction_templates=len(actual),bound_templates=len(routes),source_files=len({r['source'] for r in actual.values()}),unit_sections=len({r['section'] for r in routes.values()})),bindings=list(routes.values()),documentation_sha256=documents,qualified_keys=[],all_input_documentation_qualified=False)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--bindings',type=Path,default=Path(__file__).with_name('InputConstructionBindings.json'));p.add_argument('--output',type=Path,required=True);a=p.parse_args();repo=a.repo.resolve();r=audit(repo,inventory(repo),json.loads(a.bindings.read_text()));a.output.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r['summary']))
