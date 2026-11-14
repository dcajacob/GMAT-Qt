"""Check source inventory boundaries and reject stale or incomplete bindings."""
import copy,hashlib,json,tempfile,unittest
from pathlib import Path
from InputConstructionInventory import inventory
from AuditInputConstructionBindings import audit as bindings_audit
from AuditShippedInputDocumentation import audit as shipped_audit,active_sources

class Fixture(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.repo=Path(self.temp.name);self.qt=self.repo/'src/qtgui';self.qt.mkdir(parents=True);self.docs=self.repo/'doc/help/src';self.docs.mkdir(parents=True);(self.qt/'CMakeLists.txt').write_text('add_library(GmatQtFrontend STATIC Input.cpp)\n');(self.qt/'main.cpp').write_text('');(self.qt/'Input.cpp').write_text('auto *x=new QLineEdit;\n')
 def page(self,name,label='X',condition=''):
  p=self.docs/name;p.write_text('<refentry xmlns="http://docbook.org/ns/docbook" '+condition+'><informaltable><tgroup cols="2"><tbody><row><entry><guilabel>'+label+'</guilabel></entry><entry><variablelist><varlistentry><term>Units</term><listitem><para>km</para></listitem></varlistentry></variablelist></entry></row></tbody></tgroup></informaltable></refentry>');return p
 def book(self,includes):
  (self.docs/'help.xml').write_text('<book xmlns="http://docbook.org/ns/docbook" xmlns:xi="http://www.w3.org/2001/XInclude">'+includes+'</book>')

class InventoryTests(Fixture):
 def test_comments_and_literals_do_not_create_fields(self):
  (self.qt/'Input.cpp').write_text('''// new QLineEdit;
/* new QComboBox; */
const char *s="new QSpinBox";
auto raw=R"tag(new QSlider;\nnew QDial;)tag";
auto *real=new QLineEdit;
''');r=inventory(self.repo);self.assertEqual(len(r['records']),1);self.assertEqual(r['records'][0]['line'],6)
 def test_derived_editors_and_local_headers(self):
  (self.qt/'Input.cpp').write_text('#include "Local.hpp"\nauto *a=new Child;\n');(self.qt/'Local.hpp').write_text('class Parent: public QPlainTextEdit {};\nclass Child: public Parent {};\n');r=inventory(self.repo);self.assertEqual(r['records'][0]['constructor'],'Child');self.assertEqual(r['summary']['local_control_subclasses'],2)
 def test_direct_static_and_stack_dialogs(self):
  (self.qt/'Input.cpp').write_text('QFileDialog file(parent);\nauto *color=new QColorDialog;\nQFileDialog::getOpenFileName(parent);\nQLineEdit text;\n');self.assertEqual([r['allocation'] for r in inventory(self.repo)['records']],['stack-dialog','heap','static-dialog','stack-dialog'])
 def test_duplicate_constructors_keep_distinct_identities(self):
  (self.qt/'Input.cpp').write_text('auto *a=new QLineEdit,*b=new QLineEdit;');r=inventory(self.repo)['records'];self.assertEqual([x['constructor_ordinal'] for x in r],[1,2]);self.assertEqual(len({x['key'] for x in r}),2)
 def test_uncompiled_source_is_not_scanned(self):
  (self.qt/'Unused.cpp').write_text('new QSpinBox;');self.assertEqual(inventory(self.repo)['summary']['construction_templates'],1)
 def test_unknown_qt_input_type_requires_review(self):
  (self.qt/'Input.cpp').write_text('new QFontComboBox;');self.assertRaises(ValueError,inventory,self.repo)
 def test_dynamic_source_list_requires_review(self):
  (self.qt/'CMakeLists.txt').write_text('add_library(GmatQtFrontend STATIC ${files})\n');self.assertRaises(ValueError,inventory,self.repo)
 def test_extra_target_sources_require_review(self):
  with (self.qt/'CMakeLists.txt').open('a') as f:f.write('target_sources(GmatQtFrontend PRIVATE Extra.cpp)\n')
  self.assertRaises(ValueError,inventory,self.repo)

class BindingTests(Fixture):
 def setUp(self):
  super().setUp();self.measured=inventory(self.repo);record=self.measured['records'][0];self.units='Numeric values use the selected parameter units.';p=self.docs/'Topic.xml';p.write_text('<refentry xmlns="http://docbook.org/ns/docbook"><refsection xml:id="UnitPolicy"><variablelist><varlistentry><term>Units</term><listitem><para>'+self.units+'</para></listitem></varlistentry></variablelist></refsection></refentry>');self.manifest=dict(all_input_documentation_qualified=False,bindings=[dict(**{k:record[k] for k in ('key','source','source_sha256','source_line_sha256')},documentation='doc/help/src/Topic.xml',section='UnitPolicy',unit_description_sha256=hashlib.sha256(self.units.encode()).hexdigest(),reference_topics=[],reference_topic_sha256={})])
 def run_audit(self):return bindings_audit(self.repo,self.measured,self.manifest)
 def test_complete_binding_preserves_no_qualification(self):
  r=self.run_audit();self.assertEqual(r['summary']['bound_templates'],1);self.assertFalse(r['all_input_documentation_qualified']);self.assertEqual(r['qualified_keys'],[])
 def test_missing_binding_rejected(self):
  self.manifest['bindings']=[];self.assertRaises(ValueError,self.run_audit)
 def test_duplicate_binding_rejected(self):
  self.manifest['bindings']*=2;self.assertRaises(ValueError,self.run_audit)
 def test_stale_source_rejected(self):
  self.manifest['bindings'][0]['source_sha256']='wrong';self.assertRaises(ValueError,self.run_audit)
 def test_live_source_changed_after_inventory_rejected(self):
  (self.qt/'Input.cpp').write_text('new QComboBox;');self.assertRaises(ValueError,self.run_audit)
 def test_changed_unit_text_rejected(self):
  p=self.docs/'Topic.xml';p.write_text(p.read_text().replace(self.units,'Changed without review'));self.assertRaises(ValueError,self.run_audit)
 def test_missing_section_rejected(self):
  self.manifest['bindings'][0]['section']='Absent';self.assertRaises(ValueError,self.run_audit)
 def test_forged_completion_rejected(self):
  self.manifest['all_input_documentation_qualified']=True;self.assertRaises(ValueError,self.run_audit)

class ActiveGuideTests(Fixture):
 def test_inactive_page_does_not_supply_units(self):
  self.page('Resource_Thing.xml');self.book('<!-- ignored --><para>User guide</para>');r=shipped_audit(self.repo,dict(fields=[dict(type='Thing',field='X')]));self.assertEqual(r['summary']['all_source_route_candidates'],1);self.assertEqual(r['summary']['active_guide_route_candidates'],0)
 def test_training_only_include_excluded(self):
  self.page('Resource_Thing.xml');self.book('<xi:include condition="tm" href="Resource_Thing.xml"/>');self.assertEqual(len(active_sources(self.repo)),1)
 def test_active_root_fragment_include_accepted(self):
  self.page('Resource_Thing.xml');self.book('<xi:include href="Resource_Thing.xml" xpointer="element(/1)"/>');r=shipped_audit(self.repo,dict(fields=[dict(type='Thing',field='X')]));self.assertEqual(r['summary']['active_guide_route_candidates'],1);self.assertEqual(r['qualified_keys'],[])
 def test_inactive_row_excluded(self):
  p=self.page('Resource_Thing.xml');p.write_text(p.read_text().replace('<row>','<row condition="tm">'));self.book('<xi:include href="Resource_Thing.xml"/>');self.assertEqual(shipped_audit(self.repo,dict(fields=[dict(type='Thing',field='X')]))['summary']['active_guide_route_candidates'],0)
 def test_unknown_fragment_rejected(self):
  self.page('Resource_Thing.xml');self.book('<xi:include href="Resource_Thing.xml" xpointer="element(/1/2)"/>');self.assertRaises(ValueError,active_sources,self.repo)
 def test_cycle_rejected(self):
  self.book('<xi:include href="help.xml"/>');self.assertRaises(ValueError,active_sources,self.repo)

if __name__=='__main__':unittest.main()
