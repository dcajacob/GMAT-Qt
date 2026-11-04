"""Crosswalk the measured GUI field census against owner-specific unit docs.

This reports measured/lexical scope and unresolved bindings; it grants no credit.
"""
import argparse,gzip,hashlib,json,re
from collections import Counter
from pathlib import Path
from AuditInputUnits import inventory
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
text=lambda p:Path(p).read_text()
ALIASES={'ChemicalTank':'FuelTank','ChemicalThruster':'Thruster','FuelTank':'FuelTank','Thruster':'Thruster','PropSetup':'Propagator','ODEModel':'ForceModel','Planet':'CelestialBody','Moon':'CelestialBody','Asteroid':'CelestialBody','Comet':'CelestialBody','ConicalFOV':'FieldOfView','RectangularFOV':'FieldOfView','CustomFOV':'FieldOfView','GroundTrackPlot':'GroundTrack','BatchEstimatorInv':'BatchEstimator'}

def audit(repo,census):
 receipt=json.loads((census/'receipt.json').read_text());records={};states=[];input_sha={};doc=inventory(repo)
 if not receipt.get('inputs_unchanged') or len({r['requested'] for r in receipt['results']})!=len(receipt['results']):raise ValueError('Changed inputs or duplicate metadata process')
 for p,h in receipt['inputs_before'].items():
  if sha(p)!=h:raise ValueError('Census input/source changed: '+p)
 for result in receipt['results']:
  if result['status']!='completed':raise ValueError('Incomplete metadata process: '+result['requested'])
  p=Path(result['compressed_output']);raw=gzip.decompress(p.read_bytes())
  if sha(p)!=result['compressed_sha256'] or hashlib.sha256(raw).hexdigest()!=result['output_sha256'] or sha(result['log'])!=result['log_sha256']:raise ValueError('Census output/log changed')
  input_sha[str(p)]=sha(p);data=json.loads(raw)
  if data.get('requested')!=result['requested'] or data.get('mission_executions')!=0 or data.get('GUI_behavior_qualified') or data.get('all_input_documentation_qualified'):raise ValueError('Census identity/scope exceeds metadata role')
  if result['requested']=='--catalog':catalogue=data;continue
  typ=data['requested'];observed=[('default',data['default_GUI_fields'])]
  for variant in data['single_selector_variants']:
   state=variant['field']+'='+variant['choice']
   states.append(dict(type=typ,state=state,error=variant.get('error'),status='rejected-by-native-setter' if 'error' in variant else 'measured'))
   if 'GUI_fields' in variant:observed.append((state,variant['GUI_fields']))
  for state,fields in observed:
   for field in fields:
    identity=(typ,field['field'],field['unit']);item=records.setdefault(identity,dict(type=typ,native_type=data['native_type'],field=field['field'],observed_GUI_unit=field['unit'],states=[],native_references=field['references'],filename=field['filename'],file_list=field['file_list'],list=field['list'],color=field['color'],source_output=result['output'],source_output_sha256=result['output_sha256']))
    item['states'].append(state)
 if {r['requested'] for r in receipt['results'] if r['requested']!='--catalog'}!=set(catalogue['resource_types']):raise ValueError('Resource census omits an active creation type')
 for item in records.values():
  typ=item['type'];owner=ALIASES.get(typ,typ)
  prefixes=['Resource_'+owner]
  if typ=='Spacecraft':prefixes=['Resource_Spacecraft']
  if typ=='PropSetup':prefixes+=['Resource_NumericalIntegrators','Resource_EphemerisPropagator']
  candidates=[f for f in doc['fields'] if f['field'].casefold()==item['field'].casefold() and any(Path(f['source']).stem.startswith(prefix) for prefix in prefixes)]
  documented=[f for f in candidates if f['explicit_units']]
  item['documentation_candidates']=[dict(source=f['source'],row_index=f['row_index'],units=f['metadata']['Units']) for f in documented]
  item['crosswalk_status']='owner-field-Units-entry-found-needs-semantic-check' if documented else 'no-exact-owner-field-Units-binding'
  if '?' in item['observed_GUI_unit']:item['native_unit_review']='Native unit string contains an unresolved question mark; do not infer a unit.'
 # Record every input-widget construction site in every compiled frontend source,
 # including static Qt dialog APIs. Sites are templates, not instantiated fields.
 cmake=repo/'src/qtgui/CMakeLists.txt';source_files=sorted({p for p in re.findall(r'\b[A-Za-z][A-Za-z0-9_]*\.cpp\b',text(cmake).split('target_link_libraries(GmatQtFrontend',1)[0])})
 lexical=[]
 pattern=re.compile(r'new\s+(Q(?:LineEdit|ComboBox|SpinBox|DoubleSpinBox|CheckBox|PlainTextEdit|TextEdit|TableWidget|DateEdit|DateTimeEdit|TimeEdit|Slider|Dial))\b|((?:QInputDialog|QFileDialog|QColorDialog|QFontDialog)::(?:get\w+))\s*\(')
 for name in source_files:
  path=repo/'src/qtgui'/name;input_sha[str(path)]=sha(path)
  for number,line in enumerate(text(path).splitlines(),1):
   for match in pattern.finditer(line):lexical.append(dict(source=str(path.relative_to(repo)),line=number,constructor=match.group(1) or match.group(2),source_line=line.strip(),documentation_binding=None))
 for source,h in doc['source_sha256'].items():input_sha[str(repo/source)]=h
 input_sha[str(cmake)]=sha(cmake);input_sha[str(census/'receipt.json')]=sha(census/'receipt.json')
 result=dict(scope='Measured active resource/default and single-selector field crosswalk plus compiled input-construction templates. No all-input completeness or NASA credit.',summary=dict(active_resource_types=len(catalogue['resource_types']),metadata_processes=len(receipt['results']),single_selector_variants=len(states),rejected_single_selector_variants=sum(r['error'] is not None for r in states),distinct_type_field_unit_records=len(records),blank_native_GUI_unit_records=sum(not r['observed_GUI_unit'] for r in records.values()),exact_owner_field_unit_doc_candidates=sum(bool(r['documentation_candidates']) for r in records.values()),without_exact_owner_field_unit_doc_binding=sum(not r['documentation_candidates'] for r in records.values()),compiled_frontend_cpp_files=len(source_files),input_construction_sites=len(lexical)),fields=sorted(records.values(),key=lambda r:(r['type'],r['field'],r['observed_GUI_unit'])),selector_variants=states,input_construction_templates=lexical,nested_factory_types=catalogue['nested_factory_types'],remaining_completeness_obligations=['Reconcile every field with semantic unit descriptions, aliases and dependent-unit narratives; a matching label is not independent unit correctness.','Cover nested attitude, axes, propagator, force/atmosphere and provider modes, configured built-ins, commands/expressions and narrative-only fields.','Bind every compiled construction template, including dynamically named and unnamed controls and standard Qt dialogs, to user documentation.','Resolve native rejected combinations and ambiguous unit strings without treating blank labels as dimensionless.','Single-selector sweeps do not prove conditional cross-product completeness; source-level unit dependencies still require review.','Census is metadata only, with no mission, numerical correctness, GUI delivery/exposure or newest-source-to-binary qualification.','Seal full loaded-library identities and frozen source provenance for final qualification; the exploratory receipt binds the probe executable and selected text inputs only.'],qualified_keys=[],all_input_documentation_qualified=False,input_sha256=input_sha)
 return result

if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);parser.add_argument('--census',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();report=audit(args.repo.resolve(),args.census.resolve());args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report['summary']))
