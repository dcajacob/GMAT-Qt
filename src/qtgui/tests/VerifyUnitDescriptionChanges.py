"""Prove a unit-description batch preserves every other source byte."""
import argparse,gzip,hashlib,json,re
from collections import defaultdict
from pathlib import Path
from lxml import etree
from DocumentationTextEdits import apply_edits

def verify(repo,report,originals):
 groups=defaultdict(list)
 for c in report['changes']:groups[c['source']].append(c)
 checked=[]
 for source,changes in groups.items():
  path=repo/source;raw=path.read_bytes();root=etree.fromstring(raw);text=raw.decode('utf-8-sig');rows=list(re.finditer(r'<row(?:\s[^>]*)?>.*?</row>',text,re.S));xmlrows=root.findall('.//{http://docbook.org/ns/docbook}row')
  if len(rows)!=len(xmlrows):raise ValueError('Review nested/comment row structure: '+source)
  edits=[]
  for c in changes:
   row=rows[c['row_index']];m=re.search(r'<term>Units</term>\s*<listitem>\s*<para>(.*?)</para>',row.group(),re.S)
   if m is None or m.group(1)!=c['units']:raise ValueError('Unexpected current unit text: '+source+' '+str(c['row_index']))
   edits.append((row.start()+m.start(1),row.start()+m.end(1),c['old_units']))
  restored=apply_edits(text,edits).encode();restored=(b'\xef\xbb\xbf'+restored) if raw.startswith(b'\xef\xbb\xbf') else restored;original=gzip.decompress((originals/(path.name+'.gz')).read_bytes())
  if restored!=original:raise ValueError('Changes outside the declared Units text: '+source)
  xmlid='{http://www.w3.org/XML/1998/namespace}id';oldroot=etree.fromstring(original)
  if {n.get(xmlid) for n in root.iter() if n.get(xmlid)}!={n.get(xmlid) for n in oldroot.iter() if n.get(xmlid)}:raise ValueError('Changed document identifiers: '+source)
  checked.append(dict(source=source,unit_rows=len(changes),before_sha256=hashlib.sha256(original).hexdigest(),after_sha256=hashlib.sha256(raw).hexdigest(),only_declared_unit_text_changed=True))
 return dict(status='passed',source_pages=len(checked),unit_rows=sum(r['unit_rows'] for r in checked),sources=checked,all_other_source_bytes_preserved=True)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--review',type=Path,required=True);p.add_argument('--originals',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=verify(a.repo.resolve(),json.loads(a.review.read_text()),a.originals);a.output.write_text(json.dumps(r,indent=2)+'\n');print('PASS',r['unit_rows'],'unit rows across',r['source_pages'],'pages; all other bytes preserved.')
