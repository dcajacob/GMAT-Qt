"""Check field routing boundaries; matches never confer unit qualification."""
import json,tempfile,unittest
from pathlib import Path
from ResolveInputDocumentation import resolve

class ResolverTests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.repo=Path(self.tmp.name);(self.repo/'doc/help/src').mkdir(parents=True);(self.repo/'src/qtgui/tests').mkdir(parents=True)
 def page(self,owner,label,units='m'):
  unit='<variablelist><varlistentry><term>Units</term><listitem><para>'+units+'</para></listitem></varlistentry></variablelist>' if units else ''
  (self.repo/'doc/help/src'/('Resource_'+owner+'.xml')).write_text('<refentry xmlns="http://docbook.org/ns/docbook"><informaltable><tgroup cols="2"><tbody><row><entry>'+label+'</entry><entry>'+unit+'</entry></row></tbody></tgroup></informaltable></refentry>')
 def audit(self,typ,field):return resolve(self.repo,{'fields':[{'type':typ,'field':field}]})
 def bind(self,owner='Parent',target='Offset'):
  (self.repo/'src/qtgui/tests/InputDocumentationBindings.json').write_text(json.dumps({'bindings':[dict(type='Child',field='NativeOffset',documentation_owner=owner,documentation_field=target,source_evidence=['proof.cpp'],reason='Reviewed inheritance')]}));(self.repo/'proof.cpp').write_text('source proof placeholder')
 def test_index_terms_are_not_field_names(self):
  self.page('Thing','<guilabel>X</guilabel><indexterm><primary><guilabel>Wrong</guilabel></primary></indexterm>');self.assertEqual(self.audit('Thing','X')['summary']['structured_owner_matches'],1);self.assertEqual(self.audit('Thing','Wrong')['summary']['unresolved'],1)
 def test_multiple_labels_and_case(self):
  self.page('Thing','<guilabel>Alpha</guilabel> and <guilabel>Beta</guilabel>');self.assertEqual(self.audit('Thing','beta')['summary']['structured_owner_matches'],1)
 def test_other_owner_and_prefix_collision_do_not_match(self):
  self.page('Other','<guilabel>X</guilabel>');self.page('ThingExtra','<guilabel>X</guilabel>');self.assertEqual(self.audit('Thing','X')['summary']['unresolved'],1)
 def test_units_required(self):
  self.page('Thing','<guilabel>X</guilabel>',units='');self.assertEqual(self.audit('Thing','X')['summary']['unresolved'],1)
 def test_explicit_inheritance_is_bound_to_sources(self):
  self.page('Parent','<guilabel>Offset</guilabel>');self.bind();r=self.audit('Child','NativeOffset');self.assertEqual(r['summary']['structured_owner_matches'],1);self.assertIn('proof.cpp',r['source_sha256']);self.assertEqual(r['fields'][0]['explicit_binding']['documentation_owner'],'Parent');self.assertFalse(r['all_input_documentation_qualified']);self.assertEqual(r['qualified_keys'],[])
 def test_missing_target_rejected(self):
  self.page('Parent','<guilabel>Unrelated</guilabel>');self.bind();self.assertRaises(ValueError,self.audit,'Child','NativeOffset')
 def test_duplicate_binding_rejected(self):
  self.bind();p=self.repo/'src/qtgui/tests/InputDocumentationBindings.json';d=json.loads(p.read_text());d['bindings']*=2;p.write_text(json.dumps(d));self.assertRaises(ValueError,self.audit,'Child','NativeOffset')
 def test_records_preserved_without_matching(self):
  r=resolve(self.repo,{'fields':[{'type':'Thing','field':'A'},{'type':'Thing','field':'B'}]});self.assertEqual([f['field'] for f in r['fields']],['A','B']);self.assertEqual(r['summary']['unresolved'],2);self.assertEqual(r['qualified_keys'],[])

 def case_rows(self,upper,lower):
  self.page('Thing','<guilabel>'+upper+'</guilabel>','km2/s');p=self.repo/'doc/help/src/Resource_Thing.xml';row='<row><entry><guilabel>'+lower+'</guilabel></entry><entry><variablelist><varlistentry><term>Units</term><listitem><para>deg</para></listitem></varlistentry></variablelist></entry></row>';p.write_text(p.read_text().replace('</tbody>',row+'</tbody>'))
 def test_case_sensitive_actions_and_angles_keep_distinct_units(self):
  self.case_rows('L','l');a=self.audit('Thing','L');b=self.audit('Thing','l');self.assertEqual([r['units'] for r in a['fields'][0]['structured_documentation_candidates']],['km2/s']);self.assertEqual([r['units'] for r in b['fields'][0]['structured_documentation_candidates']],['deg'])
 def test_ambiguous_case_fallback_does_not_bind(self):
  self.case_rows('ECC','ecc');self.assertEqual(self.audit('Thing','eCc')['summary']['unresolved'],1)
 def test_unique_case_fallback_still_binds(self):
  self.page('Thing','<guilabel>Zaxis</guilabel>');self.assertEqual(self.audit('Thing','ZAxis')['summary']['structured_owner_matches'],1)

if __name__=='__main__':unittest.main()
