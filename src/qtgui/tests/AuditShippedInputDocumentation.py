"""Limit input-unit routes to the active ug-profile user-guide source graph."""
import argparse,hashlib,json,re
from pathlib import Path
from lxml import etree
from ResolveInputDocumentation import resolve
NS={'d':'http://docbook.org/ns/docbook','xi':'http://www.w3.org/2001/XInclude'}
XI='{'+NS['xi']+'}include'
def enabled(node):
 if not isinstance(node.tag,str):return False
 values=re.split(r'[\s;,]+',(node.get('condition') or '').strip())
 return values==[''] or 'ug' in values

def active_sources(repo):
 base=(repo/'doc/help/src').resolve();active={};pending=set()
 def walk(path):
  path=path.resolve()
  if path in pending:raise ValueError('Cyclic user-guide include: '+str(path))
  if path in active:return
  if not path.is_relative_to(base):raise ValueError('User-guide include outside source directory: '+str(path))
  root=etree.parse(str(path)).getroot()
  if not enabled(root):return
  pending.add(path);active[path]=root
  def visit(node):
   if not enabled(node):return
   if node.tag==XI:
    if node.get('xpointer') not in (None,'','element(/1)') or node.get('parse','xml')!='xml':raise ValueError('Review fragment/text user-guide include: '+str(path))
    walk(path.parent/node.get('href'));return
   for child in node:visit(child)
  visit(root);pending.remove(path)
 walk(base/'help.xml')
 return active

def audit(repo,baseline):
 active=active_sources(repo);base=resolve(repo,baseline);eligible=set();documents={}
 for path,root in active.items():
  documents[str(path.relative_to(repo))]=hashlib.sha256(path.read_bytes()).hexdigest()
  for index,row in enumerate(root.findall('.//d:row',NS)):
   if all(enabled(n) for n in [row,*row.iterancestors()]):eligible.add((str(path.relative_to(repo)),index))
 fields=[]
 for row in base['fields']:
  r=dict(row);r['shipped_documentation_candidates']=[d for d in r['structured_documentation_candidates'] if (d['source'],d['row_index']) in eligible];r['binding_status']='active-guide-route-found-needs-semantic-unit-review' if r['shipped_documentation_candidates'] else 'no-active-guide-unit-route';fields.append(r)
 return dict(scope='Active user-guide ug-profile source routes only; matching Units text is not semantic or all-input qualification.',summary=dict(records=len(fields),all_source_route_candidates=sum(bool(r['structured_documentation_candidates']) for r in fields),active_guide_route_candidates=sum(bool(r['shipped_documentation_candidates']) for r in fields),without_active_guide_route=sum(not r['shipped_documentation_candidates'] for r in fields),active_guide_source_files=len(active)),fields=fields,active_guide_source_sha256=documents,qualified_keys=[],all_input_documentation_qualified=False)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--baseline',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();raw=a.baseline.read_bytes();r=audit(a.repo.resolve(),json.loads(raw));r.update(baseline_sha256=hashlib.sha256(raw).hexdigest());a.output.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r['summary']))
