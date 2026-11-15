"""Conjoin executed state-conversion routes with their original frame conditions.

The row requires conversion of the represented physical state. The retained
Brouwer model has nonunique inverse roots near critical inclination; literal
mean-element identity is kept as a separate failed diagnostic, not asserted.
"""
from pathlib import Path
import copy,hashlib,json,math,re,tarfile,tempfile
from AuditBrouwerTransportEvidence import audit as transport_audit
from BrouwerPrototypeOracle import adapt,cartesian

KEYS=['FRR!E13']
KIND='complete-source-versioned-state-conversion-route-family'
TYPES=['Cartesian','Keplerian','ModifiedKeplerian','SphericalAZFPA','SphericalRADEC','Equinoctial','ModifiedEquinoctial','AlternateEquinoctial','Delaunay','Planetodetic','OutgoingAsymptote','IncomingAsymptote','BrouwerMeanShort','BrouwerMeanLong']
HYPER=['Cartesian','Keplerian','ModifiedKeplerian','SphericalAZFPA','SphericalRADEC','ModifiedEquinoctial','OutgoingAsymptote','IncomingAsymptote']
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','independent_physical_averaging_qualified']
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def require(value,message):
 if not value:raise ValueError(message)
def bound(actual,expected,limits,relative=0):
 require(len(actual)==len(expected)==len(limits),'Numerical shape changed')
 require(all(math.isfinite(a)and abs(a-b)<=tol+relative*abs(b)for a,b,tol in zip(actual,expected,limits)),'Independent state bound failed')
def direct(matrix,proof,forward):
 require(matrix['types']==TYPES and proof['outcome']=='passed'and proof['directed_contexts']==238 and proof['identity_contexts']==22 and proof['failed_contexts']==0,'Incomplete direct dispatch')
 require(matrix['position_tolerance_km']==1e-6 and matrix['velocity_tolerance_km_s']==1e-9 and matrix['field_tolerance']==1e-6,'Original dispatch bounds changed')
 cases={c['key']:c for c in matrix['cases']};expected={(c['key'],t['type'])for c in cases.values()for t in c['targets']};seen=set();mean_count=0
 require(len(cases)==22 and len(expected)==238,'Missing scalar source/target case')
 for row in proof['results']:
  spec=cases[row['case']];target=next(t for t in spec['targets']if t['type']==row['target']);seen.add((row['case'],row['target']))
  require(row['passed']and row['domain']==spec['domain']and row['source']==spec['source']and row['input']==spec['input']and row['expected_cartesian']==spec['cartesian'],'Wrong/failed executed dispatch cell')
  require(set(t['type']for t in spec['targets'])==set(TYPES if spec['domain']=='elliptic'else HYPER)-{spec['source']},'Target omitted from a supported domain')
  bound(row['reconstructed'],spec['cartesian'],[1e-6]*3+[1e-9]*3)
  if row['target'].startswith('Brouwer'):
   truth=cartesian(forward[row['target']](row['actual']),spec['mu']);bound(truth,spec['cartesian'],[1e-6]*3+[1e-9]*3);mean_count+=1
  elif target.get('values'):
   errors=[]
   for i,(actual,wanted)in enumerate(zip(row['actual'],target['values'])):
    angular=(i<3 if row['target']=='Delaunay'else i==5 if row['target']in ['Equinoctial','ModifiedEquinoctial','AlternateEquinoctial']else i in [1,2,4,5]if row['target'].startswith('Spherical')or row['target']=='Planetodetic'else i>=2 if row['target']!='Cartesian'else False)
    errors.append(math.remainder(actual-wanted,360)if angular else actual-wanted)
   bound(errors,[0]*6,[1e-6]*6)
 require(seen==expected and len(proof['results'])==238 and mean_count==26,'Incomplete/duplicate actual dispatch effects')
 return dict(elliptic_routes=182,hyperbolic_routes=56,identity_routes=22,independent_mean_reconstructions=26)

def gui_context(context,artifacts):
 for key,hashkey in [('receipt','receipt_sha256'),('snapshot_manifest','snapshot_manifest_sha256'),('proof','proof_sha256')]:require(sha(context[key])==context[hashkey]and context[key]in artifacts,'Unbound GUI execution/snapshot/proof')
 receipt=load(context['receipt']);result=next(v for v in receipt['results']if v['name']==context['test']);proof=load(context['proof']);snapshot=load(context['snapshot_manifest'])
 require(result['status']=='passed'and result['returncode']==0 and proof['outcome']=='passed'and receipt['mode']==('headless'if context['mode']=='unattended'else context['mode']),'Failed/wrong-mode GUI context')
 require(result['snapshot_id']==context['snapshot_id']==snapshot['snapshot_id']and result['log']in artifacts and sha(result['log'])==result['log_sha256'],'Unbound executed GUI source/log')
 for name in snapshot['source_files']:
  archived=(Path(snapshot['root'])/name).resolve()
  require(str(archived)in artifacts and artifacts[str(archived)]==snapshot['artifact_hashes'][name],'GUI source is not bound to its executed archived snapshot: '+name)
 emitted=[json.loads(v)for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',Path(result['log']).read_text())]
 require(proof in emitted,'GUI proof differs from actual execution output')
 if 'pairs'in proof:
  require(proof['numeric_sentinel']==9 and len(proof['pairs'])==proof['directed_pairs']and proof['displayed_fields']==24*proof['directed_pairs']and proof['unit_checks']==proof['displayed_fields'],'Missing GUI conversion transactions')
  for pair in proof['pairs']:
   require(pair['source']==proof['source_representation']and pair['sentinel']==9 and all(len(pair[k])==6 for k in ['pending_cancel','pending_apply','committed','reopened','reported']),'Missing GUI Cancel/Apply/reopen/sentinel result')
 else:require(proof['numeric_sentinel']==9 and len(proof['inputs'])==6 and proof['rejections'],'Missing GUI definition/error transaction')
 return proof

def qualify(repo,path,inventory,admitted):
 q=load(path);require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Wrong/extra state-conversion admission')
 require(q['authority_sha256']==inventory['sources']and all(q[g]is False for g in GATES),'Authority or scope overclaim')
 known={r['key']:r for r in inventory['requirements']};require(q['candidate_rows']==[known['FRR!E13']]and {'FRR!E5','FRR!E9'}<=set(admitted),'Original row or coordinate-condition parents missing')
 require(q['coordinate_conditions']==[known[k]for k in ['FRR!E5','FRR!E9']],'Original coordinate conditions changed')
 require(q['generic_composition_review']['complete_route_inventory']==14 and q['generic_composition_review']['all_frame_parameter_cross_products_executed']is False,'Unsupported Cartesian-product claim')
 require(q['generic_composition_review']['argument']=='Each selected-frame representation converts through a six-component Cartesian state. The inherited coordinate-condition families establish body/axis/origin bindings; executed direct dispatch exhausts source/target kernels. Frame transforms compose with those kernels. Brouwer uses the fixed Earth pole through the context helper, tested with independent tilted R/Rdot and both epoch overloads. Planetodetic uses its selected body-fixed spheroid. Physical-state equivalence handles nonunique element conventions.','Different/narrowed conversion argument')
 for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed conversion evidence: '+n)
 for n,h in q['prerequisite_qualifications'].items():require(sha(n)==h and n in q['artifact_sha256'],'Unbound accepted parent qualification')
 protected=load(q['protected_input_manifest']);require(len(protected)==1334 and q['protected_inputs_unchanged']==1334,'Protected input denominator changed')
 # Restore only archived mathematical inputs. Historical source is never replaced
 # by a currently modified fixture or oracle during prefix replay.
 with tempfile.TemporaryDirectory(prefix='gmat-conversion-frozen-math-')as temporary:
  root=Path(temporary)
  with tarfile.open(q['math_archive'])as bundle:
   require(set(bundle.getnames())==set(q['math_sha256']),'Frozen mathematical input inventory changed')
   for n,h in q['math_sha256'].items():
    raw=bundle.extractfile(n).read();require(hashlib.sha256(raw).hexdigest()==h,'Changed frozen math input');p=root/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(raw)
  direct_matrix=load(root/'src/qtgui/tests/NasaDirectOrbitConversionCases.json');receipt=load(q['direct_receipt']);proof=load(q['direct_proof'])
  require(receipt['status']=='passed'and receipt['returncode']==0 and receipt['inputs_unchanged']and receipt['loaded_project_images'],'Unbound native dispatch execution')
  forward={kind:adapt(root/'prototype/StateConv'/name)[0]for kind,name in [('BrouwerMeanShort','BROLYD_short.m'),('BrouwerMeanLong','BROLYD.m')]}
  counts=direct(direct_matrix,proof,forward)
  transport=transport_audit(root,Path(q['transport_directory']));require(transport==load(Path(q['transport_directory'])/'independent-equivalence.json'),'Independent transport audit did not replay')
  require(transport['mean_reads_checked']==648 and transport['cartesian_write_assertions']==576 and transport['contexts']==72 and transport['strict_field_disagreements_retained']==10,'Missing general mean transport/ambiguity facets')
 strict=load(q['strict_trial_receipt']);require(strict['status']=='failed'and strict['returncode']==1 and q['strict_trial_unchanged']and q['strict_trial_receipt']in q['artifact_sha256'],'Original failed strict trial removed/relabelled')
 engineering=Path(q['strict_trial_receipt']).parent.parent
 original=load(engineering/'frozen-plan-v1.json')['source_sha256']['src/qtgui/tests/BrouwerTransportTests.cpp']
 owned=load(engineering/'owned-source-sha256-v1.json')['src/qtgui/tests/BrouwerTransportTests.cpp']
 require(strict['source_sha256']['src/qtgui/tests/BrouwerTransportTests.cpp']==original==owned,'Strict original source/expectations changed')
 with tarfile.open(engineering/'owned-sources-v1.tar.gz')as bundle:
  require(hashlib.sha256(bundle.extractfile('src/qtgui/tests/BrouwerTransportTests.cpp').read()).hexdigest()==original,'Strict trial source archive changed')
 pairs={mode:set()for mode in ['unattended','visible']};hyper={mode:set()for mode in pairs};displayed=set();contexts=0
 for group,entries in q['gui_contexts'].items():
  for c in entries:
   p=gui_context(c,q['artifact_sha256']);contexts+=1
   if group in ['orbit-conversion-review','orbit-extended-conversion-review']:
    pairs[c['mode']].update((v['source'],v['target'])for v in p['pairs']);displayed.add(p['source_representation'])
   elif group=='hyperbolic-conversion-review':hyper[c['mode']].update((v['source'],v['target'])for v in p['pairs']);displayed.add(p['source_representation'])
   elif group=='planetodetic-review':require(p['representation']=='Planetodetic'and p['frame']=='PlanetodeticFixed','Spheroid GUI route missing');displayed.add('Planetodetic')
   else:displayed.add(p['family'])
 nine=set(TYPES)-{'Planetodetic','IncomingAsymptote','OutgoingAsymptote','BrouwerMeanShort','BrouwerMeanLong'}
 require(all(v=={(a,b)for a in nine for b in nine if a!=b}for v in pairs.values()),'Incomplete original nine-representation GUI pair matrix')
 require(all(v=={(a,b)for a in HYPER for b in HYPER if a!=b}for v in hyper.values()),'Incomplete supported hyperbolic GUI pair matrix')
 for key,paths in q['mean_gui_definitions'].items():
  require(key in ['BrouwerMeanShort','BrouwerMeanLong'],'Unknown mean GUI family')
  receipt=load(paths['receipt']);proof=load(paths['proof']);require(receipt['status']=='passed'and proof['outcome']=='passed','Missing positive mean GUI definition')
  require(key in json.dumps(proof)and paths['proof']in q['artifact_sha256'],'Wrong/unbound mean GUI definition');displayed.add(key)
 require(displayed==set(TYPES),'An actual GUI representation family is missing')
 # The dispatch test also proves exact identity and unknown-type rejection.
 # Its executed source is bound to this independent archive/receipt.
 with tarfile.open(q['direct_source_archive'])as bundle:
  source=bundle.extractfile('src/qtgui/tests/NasaDirectOrbitConversionTests.cpp').read();require(hashlib.sha256(source).hexdigest()==receipt_hash(q['direct_receipt'],'src/qtgui/tests/NasaDirectOrbitConversionTests.cpp'),'Native dispatch worker changed')
  require(b'badSource&&badTarget'in source and b'identity[i]==input[i]'in source,'Native type/identity guards absent')
 for n,tokens in q['generic_implementation_guards'].items():
  raw=Path(q['generic_implementation_sources'][n]).read_text();require(all(token in raw for token in tokens),'Missing executed composition owner: '+n)
 summary=dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,original_row_fingerprint=known['FRR!E13']['fingerprint'],original_coordinate_condition_parents=['FRR!E5','FRR!E9'],native_counts=counts,GUI_contexts_reused=contexts,actual_GUI_representation_families=14,independent_mean_transport_contexts=72,independent_mean_reads=648,original_strict_failure_retained=True,nonunique_mean_reads_retained=10,generic_frame_composition=True,all_frame_parameter_cross_products_executed=False,native_tests_run=0,source_versioned_only=True,new_requirement_credit=1,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,independent_physical_averaging_qualified=False)
 return q,KEYS,summary

def receipt_hash(path,name):return load(path)['source_sha256'][name]
