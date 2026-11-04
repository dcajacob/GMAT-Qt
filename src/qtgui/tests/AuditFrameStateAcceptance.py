"""Admit the original FRR-1.2 definition family; never generalize to FRR-1.1/1.3."""
import argparse,csv,hashlib,json,math,re
from pathlib import Path
import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load,require,sha
from AuditCentralBodyPropagationAcceptance import read_run
from BuildNasaCoverage95 import baseline
from SphericalStateOracle import TYPES,METADATA,definitions,cartesian
from AuditNamedFrameAcceptance import AXES
KEY='FRR!E9'
SOURCE='src/qtgui/tests/NasaFrameStateDefinitionTests.cpp'
FIXTURE='src/qtgui/tests/NasaFrameStateDefinitionCases.json'
FACETS=['actual-frame-representation-popup-and-six-typed-fields','independent-local-spherical-Cartesian-equations-and-units','atomic-invalid-Apply-OK-pending-Cancel-and-recovery','exact-source-and-complete-native-graph-Undo-Redo','Unicode-save-reopen-and-read-only-output-viewer','native-frame-axis-origin-and-runtime-Cartesian-result','three-epochs-octants-and-source-history-sentinel-isolation']
STATES=[[7000,-1200,900,1.5,6.75,-2],[-5100,4300,-1800,-4,5,1.25],[-3300,-6100,2600,6,-2,-1]]

GENERIC_GUARDS={
 'src/qtgui/SpacecraftOrbit.cpp':['StateConversionUtil::Convert(input,representation.toStdString(),"Cartesian"','converter.Convert(spacecraft->GetEpoch(),cartesian,frame,converted,internal)','spacecraft->SetState(converted)'],
 'src/base/spacecraft/Spacecraft.cpp':['coordConverter.Convert(GetEpoch(), inState, internalCoordSystem, csState,','finalState = csState;'],
 'src/base/coordsystem/CoordinateConverter.cpp':['inCoord->ToBaseSystem(epoch, inState, intState, coincident, forceComputation);','outCoord->FromBaseSystem(epoch, baseState, outState, coincident, forceComputation);'],
 'src/base/coordsystem/CoordinateSystem.cpp':['axes->RotateToBaseSystem(epoch, inState, internalState, forceComputation)','axes->RotateFromBaseSystem(epoch,internalState,outState,']}

def structure(q,review,matrix,inventory):
 require(q['contract_kind']=='source-versioned-complete-frame-state-definition-family' and q['qualified_keys']==[KEY],'Partial or extra frame-state credit')
 require(q['authority_sha256']==inventory['sources'] and all(q.get(k) is False for k in ['technical_core_approval','full_current_build_qualified','full_NASA_qualified']),'Frame-state authority or qualification overclaim')
 known={r['key']:r for r in inventory['requirements']};row=review['requirement'];require(row==known[KEY],'Original frame-state obligation changed')
 require(review['children']=={k:known[k] for k in ['FRR!E10','FRR!E11','FRR!E12']} and review['axis_rows']=={k:known[k] for k in ['FRR!E'+str(i) for i in range(434,450)]} and review['origin_rows']=={k:known[k] for k in ['FRR!E'+str(i) for i in range(428,433)]},'Original definition/frame inventory omitted')
 require(review['all_parameter_cross_products_executed'] is False and review['generic_rigid_state_argument'] and review['independent_astronomical_frame_truth_claimed'] is False,'Generic state argument or scope distinction omitted')
 require(len(matrix['cases'])==57 and len({c['key'] for c in matrix['cases']})==57,'Incomplete/duplicate frame-state matrix')
 require({c['axis'] for c in matrix['cases']}==set(AXES) and {c['origin_class'] for c in matrix['cases']}=={'CelestialBody','Spacecraft','GroundStation','Barycenter','LibrationPoint'},'Missing declared frame category')
 frames={c['key'].rsplit('_',1)[0] for c in matrix['cases']};require(len(frames)==19 and {(c['key'].rsplit('_',1)[0],c['representation']) for c in matrix['cases']}=={(f,t) for f in frames for t in TYPES},'Missing frame/representation cell')
 require(matrix['position_tolerance_km']==1e-6 and matrix['velocity_tolerance_km_s']==1e-9 and matrix['display_tolerance']==1e-6,'Original tolerances changed')
 for c in matrix['cases']:
  require({k:c[k] for k in ['names','units']}==METADATA[c['representation']] and len(c['samples'])==3,'Field/unit or sample inventory changed')
  for sample,state in zip(c['samples'],STATES):require(sample['cartesian']==state and sample['values']==definitions(state)[c['representation']] and math.isfinite(sample['epoch']),'Independent geometry fixture changed')
 require(set(q['selected'])=={'headless','visible'},'Missing frame-state execution mode')
 for mode,selections in q['selected'].items():require(len(selections)==57 and {x['case'] for x in selections}=={c['key'] for c in matrix['cases']},'Incomplete/duplicate frame-state mode')


def case_check(matrix,c,result,sources,artifacts,binding_sources):
 work=Path(result['log']).parent;p=load(work/'proof.json');require([r for r in nasa.proof_records(result) if r.get('checkpoint')=='frame-state-definition']==[p],'Proof differs from actual frame-state log')
 require(result['status']=='passed' and result['returncode']==0 and p['outcome']=='passed' and p['case']==c['key'] and all(p[k]==c[k] for k in ['frame','axis','representation','origin_class']) and p['facets']==FACETS and len(p['groups'])==3,'Substituted frame-state execution or assertion contract')
 require(json.loads(sources[FIXTURE])==matrix,'Executed definition fixture differs')
 for path,tokens in GENERIC_GUARDS.items():
  require(path in binding_sources and all(t in binding_sources[path].decode() for t in tokens),'Missing archived generic state-binding implementation: '+path)
 require(all(str(work/n) in artifacts for n in ['proof.json','groups.json','test.log','ctest.xml']),'Missing frame-state terminal evidence')
 require(load(work/'groups.json')==p['groups'],'Native frame-state groups differ')
 for index,(sample,group) in enumerate(zip(c['samples'],p['groups'])):
  require(group['sample']==sample and group['sentinel']==9 and len(group['fields'])==6,'Sample/field/sentinel omitted')
  expected=cartesian(c['representation'],sample['values']);raw=list(csv.reader((work/(str(index)+'-results.csv')).open()));require(len(raw)==1 and len(raw[0])==7,'Raw frame-state row cardinality differs');reported=[float(v) for v in raw[0]];require(reported[-1]==9 and reported[:6]==group['reported_cartesian'],'Raw frame-state output differs from proof')
  for actual in [group['native_cartesian'],group['reported_cartesian']]:
   require(len(actual)==6,'Frame-state component omitted')
   for i,(v,w) in enumerate(zip(actual,expected)):require(math.isfinite(v) and abs(v-w)<=(matrix['position_tolerance_km'] if i<3 else matrix['velocity_tolerance_km_s']),'Independent frame-local geometry differs')
  for i,f in enumerate(group['fields']):
   error=f['value']-sample['values'][i];error=math.remainder(error,360) if c['units'][i]=='deg' else error
   require(f['name']==c['names'][i] and f['unit']==c['units'][i] and math.isfinite(error) and abs(error)<=matrix['display_tolerance'],'Reopened field/units differ')
  rejected=group['rejected'];required=[(c['names'][0],'nan','Apply'),(c['names'][0],'not-a-number','OK')]
  if c['representation']!='Cartesian':required += [('RMagnitude','1e-11','Apply'),('VMagnitude','1e-11','Apply'),('DEC','91','Apply')]
  require([(r['field'],r['input'],r['action']) for r in rejected]==required and all(r['diagnostic'] for r in rejected),'Invalid input recovery assertions omitted')
  for suffix in ['baseline.script','accepted.script','build.messages.txt','run.messages.txt','results.csv']:require(str(work/(str(index)+'-'+suffix)) in artifacts,'Raw sample artifact omitted')
  before=(work/(str(index)+'-baseline.script')).read_text();after=(work/(str(index)+'-accepted.script')).read_text();strip=lambda s:re.sub(r'^[ \t]*(?:GMAT[ \t]+)?Sat\.[^\n]*\n','',s,flags=re.M)
  require(before!=after and strip(before)==strip(after) and after.split('BeginMissionSequence;')[1]==before.split('BeginMissionSequence;')[1],'Unrelated configuration/mission source changed')
 code=sources[SOURCE].decode()
 for anchor in ['combo->showPopup()','QTest::keyClicks','graph()==baseline','graph()==accepted','graph()==beforeRun','source->undo()','source->redo()','read(saved)==applied.toUtf8()','text->isReadOnly()','Native frame-state selectors differ','Native frame-state axis/origin binding differs','Rejected circular frame did not restore both selectors and explain the error']:
  # The original three pilots predate the added negative circular-frame clause.
  if anchor.startswith('Rejected circular') and c['axis']!='ObjectReferenced':continue
  require(anchor in code,'Original frame-state assertion omitted: '+anchor)


def qualify(repo,path,inventory):
 q=load(path);review=load(q['review']);matrix=load(q['fixture']);structure(q,review,matrix,inventory)
 for p,h in q['artifact_sha256'].items():require(sha(p)==h,'Changed frame-state artifact: '+p)
 required={q['review'],q['fixture'],q['baseline_seal']}
 for spec in q['runs'].values():
  required.update(spec.values());reuse=load(spec['reuse']);required.update(reuse[k] for k in ['receipt','reference','delta'])
 for spec in q['generic_binding_sources'].values():
  required.add(spec['artifact']);require(sha(spec['artifact'])==spec['sha256'],'Generic binding source digest differs')
 require(required<=set(q['artifact_sha256']),'Execution metadata or generic binding source is unsealed')
 _,_,keys=baseline(repo,Path(q['baseline_seal']));require({'FRR!E10','FRR!E11','FRR!E12','FRR!E427','FRR!E433'}<=keys['requirements'],'Missing original definition children/frame prerequisites')
 runs={n:read_run(repo,s,inventory,q['artifact_sha256'],matrix['physical_inputs']) for n,s in q['runs'].items()}
 binding_sources={name:Path(spec['artifact']).read_bytes() for name,spec in q['generic_binding_sources'].items()};require(set(binding_sources)==set(GENERIC_GUARDS),'Generic binding source omitted')
 for name,spec in q['runs'].items():
  before=load(spec['before'])
  for path,data in binding_sources.items():require(hashlib.sha256(data).hexdigest()==before['sources'].get(path),'Generic binding source does not match execution snapshot')
 cases={c['key']:c for c in matrix['cases']}
 for mode,selections in q['selected'].items():
  for s in selections:
   receipt,results,sources=runs[s['run']];name='QtGui.NativeNasaFrameState'+s['case'];require(receipt['mode']==mode and s['test']==name,'State definition test/mode substituted');case_check(matrix,cases[s['case']],results[name],sources,q['artifact_sha256'],binding_sources)
   for p,h in matrix['physical_inputs'].items():require(load(q['runs'][s['run']]['before'])['sources'].get(p)==receipt['source_hashes'].get(p)==h,'Unbound frame-state physical fixture')
 return q,[KEY]

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args();_,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,new_GUI_or_engine_executions=0)))
