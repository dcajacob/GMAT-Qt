"""Conjoin the exact FRR!E5 three-conic inertial/celestial definition family."""
import argparse,csv,hashlib,json,math,re
from pathlib import Path
import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load,require,sha
from AuditCentralBodyPropagationAcceptance import read_run,DEFAULTS,USER_TYPES
from AuditFrameStateAcceptance import qualify as shared_transactions,GENERIC_GUARDS
from BuildNasaCoverage95 import baseline
from InertialDefinitionOracle import TYPES,define,cartesian,conic_cartesian
KEY='FRR!E5'
SOURCE='src/qtgui/tests/NasaInertialDefinitionTests.cpp'
FIXTURE='src/qtgui/tests/NasaInertialDefinitionCases.json'
AXES=['MJ2000Eq','MJ2000Ec','MOEEq','MOEEc','TOEEq','TOEEc','BodyInertial']
FACETS=['actual-frame-roundtrip-representation-popup-and-six-typed-conic-fields','native-inertial-celestial-type-user-GM-and-TA-binding','independent-elliptic-hyperbolic-circular-Cartesian-geometry','actual-Apply-source-rebuild-and-complete-native-resource-isolation','raw-local-state-epoch-sentinel-and-unchanged-mission-source','coupled-conic-validation-with-retained-transaction-conjunction']
META={
 'Keplerian':dict(names=['SMA','ECC','INC','RAAN','AOP','TA'],units=['km','','deg','deg','deg','deg']),
 'ModifiedKeplerian':dict(names=['RadPer','RadApo','INC','RAAN','AOP','TA'],units=['km','km','deg','deg','deg','deg']),
 'Equinoctial':dict(names=['SMA','EquinoctialH','EquinoctialK','EquinoctialP','EquinoctialQ','MLONG'],units=['km','','','','','deg'])}
CONIC_GUARDS={
 'src/qtgui/SpacecraftOrbit.cpp':['auto *body=dynamic_cast<CelestialBody *>(frame->GetOrigin());','body ? body->GetGravitationalConstant() : 0','converter.Convert(spacecraft->GetEpoch(),cartesian,frame,converted,internal)'],
 'src/gmatutil/util/StateConversionUtil.cpp':['outState = KeplerianToCartesian(mu, state, anomalyType);','Rvector6 keplerian = ModKeplerianToKeplerian(state);','outState = KeplerianToCartesian(mu, keplerian, anomalyType);','Rvector6 cartState = EquinoctialToCartesian(state, mu);','Real n    = Sqrt(mu/(sma * sma * sma));']}

def structure(q,r,m,inventory):
 known={x['key']:x for x in inventory['requirements']}
 require(q['contract_kind']=='source-versioned-complete-inertial-conic-definition-family' and q['qualified_keys']==[KEY],'Partial/extra inertial definition credit')
 require(q['authority_sha256']==inventory['sources'] and r['requirement']==known[KEY] and r['children']=={k:known[k] for k in ['FRR!E6','FRR!E7','FRR!E8']},'Changed original conic obligation/children')
 require(all(q.get(k) is False for k in ['technical_core_approval','full_current_build_qualified','full_NASA_qualified']),'Inertial qualification overclaim')
 require(r['original_axis_rows']=={k:known[k] for k in ['FRR!E'+str(i) for i in range(434,450)]},'Original axis obligations changed')
 require(r['inertial_axes']==AXES and r['generic_conic_and_frame_argument'] and not r['all_parameter_cross_products_executed'] and not r['absolute_body_or_frame_science_qualified'],'Missing generic argument or scope boundary')
 bodies={b['key']:b for b in m['body_inventory']};require(len(bodies)==len(m['body_inventory'])==15 and {b['body'] for b in bodies.values() if not b['user']}==set(DEFAULTS) and {b['type'] for b in bodies.values() if b['user']}==set(USER_TYPES),'Declared body/type omitted')
 frames={**{'Body'+k:('MJ2000Eq',k) for k in bodies if k!='Earth'},**{'Axes'+a:(a,'Earth') for a in AXES if a!='MJ2000Eq'},'BodyInertialMars':('BodyInertial','Mars'),'BodyInertialUserPlanet':('BodyInertial','UserPlanet')};expected={f+'_'+kind for f in frames for kind in TYPES}
 require(len(m['cases'])==len(expected)==66 and {c['key'] for c in m['cases']}==expected,'Incomplete/duplicate frame/body/conic matrix')
 require(m['position_tolerance_km']==1e-6 and m['velocity_tolerance_km_s']==1e-9,'Original conic tolerance changed')
 for c in m['cases']:
  f,kind=c['key'].rsplit('_',1);axis,b=frames[f];require(c['representation']==kind and c['axis']==axis and c['body']==bodies[b] and c['frame']=='InertialFrame' and len(c['samples'])==3 and len(c['names'])==len(c['units'])==6,'Substituted inertial definition cell')
  require({k:c[k] for k in ['names','units']}==META[kind],'Original conic field/unit contract changed')
  require([s['domain'] for s in c['samples']]==['inclined-elliptic','inclined-elliptic' if kind=='Equinoctial' else 'hyperbolic','circular-equatorial'],'Missing supported conic domain')
  require(sum(any(abs(v-c['baseline_cartesian'][i])>1e-6 for i,v in enumerate(s['cartesian'])) for s in c['samples'])>=2,'Conic witnesses only reproduce unchanged baseline state')
  for s in c['samples']:
   require(s['values']==define(kind,s['canonical_keplerian']),'Independent definition fixture changed')
   truth=conic_cartesian(s['canonical_keplerian'],c['body']['mu_km3_s2']);inverse=cartesian(kind,s['values'],c['body']['mu_km3_s2'])
   require(len(s['cartesian'])==6 and all(abs(a-b)<(1e-8 if i<3 else 1e-11) and abs(a-inverse[i])<(1e-8 if i<3 else 1e-11) for i,(a,b) in enumerate(zip(truth,s['cartesian']))),'Independent conic/inverse fixture disagreement')
 require(set(q['selected'])=={'headless','visible'},'Missing inertial execution mode')
 for mode,selection in q['selected'].items():require(len(selection)==66 and {s['case'] for s in selection}==expected,'Missing/duplicate inertial definition witness')

def check_case(m,c,result,sources,artifacts):
 work=Path(result['log']).parent;p=load(work/'proof.json');require([x for x in nasa.proof_records(result) if x.get('checkpoint')=='inertial-conic-definition']==[p],'Conic proof differs from actual native log')
 require(result['status']=='passed' and result['returncode']==0 and p['outcome']=='passed' and p['case']==c['key'] and all(p[k]==c[k] for k in ['representation','frame','axis','body']) and p['facets']==FACETS and len(p['groups'])==3,'Conic execution/contract substituted')
 require(json.loads(sources[FIXTURE])==m,'Different definition fixture executed')
 require(all(str(work/n) in artifacts for n in ['proof.json','groups.json','test.log','ctest.xml']) and load(work/'groups.json')==p['groups'],'Essential conic terminal evidence omitted')
 for i,(sample,g) in enumerate(zip(c['samples'],p['groups'])):
  require(g['sample']==sample and len(g['typed'])==6 and g['sentinel']==9 and g['native_GM']==c['body']['mu_km3_s2'],'Conic input/GM/field sample omitted')
  for j,f in enumerate(g['typed']):require(f==dict(name=c['names'][j],unit=c['units'][j],value=sample['values'][j]),'Typed conic field or unit differs')
  expected=cartesian(c['representation'],sample['values'],c['body']['mu_km3_s2']);rows=list(csv.reader((work/(str(i)+'-results.csv')).open()));require(len(rows)==1 and len(rows[0])==8,'Conic raw cardinality differs');raw=[float(v) for v in rows[0]];require(raw[:6]==g['reported_cartesian'] and abs(raw[6]-sample['epoch'])<1e-10 and raw[7]==9,'Conic raw/proof/epoch/sentinel differ')
  for actual in [g['native_cartesian'],g['reported_cartesian']]:
   require(len(actual)==6,'Missing conic state component')
   for j,(v,w) in enumerate(zip(actual,expected)):require(math.isfinite(v) and abs(v-w)<=(m['position_tolerance_km'] if j<3 else m['velocity_tolerance_km_s']),'Independent local conic geometry differs')
  if c['key'].startswith('AxesMJ2000Ec_') and i==0:
   bad=[('ECC','1')] if c['representation']=='Keplerian' else [('RadPer',format(sample['values'][1]*1.1,'.17g'))] if c['representation']=='ModifiedKeplerian' else [('EquinoctialH','0.9'),('EquinoctialK','0.9')]
   require([(r['field'],r['input']) for r in g['rejected']]==bad and all(r['diagnostic'] for r in g['rejected']),'Coupled/parabolic invalid recovery omitted')
  else:require(g['rejected']==[],'Unreviewed conic validation substitution')
  for suffix in ['baseline.script','accepted.script','build.messages.txt','run.messages.txt','results.csv']:require(str(work/(str(i)+'-'+suffix)) in artifacts,'Missing raw conic sample artifact')
  before=(work/(str(i)+'-baseline.script')).read_text();after=(work/(str(i)+'-accepted.script')).read_text();strip=lambda s:re.sub(r'^[ \t]*(?:GMAT[ \t]+)?Sat\.[^\n]*\n','',s,flags=re.M)
  require(before!=after and strip(before)==strip(after) and before.split('BeginMissionSequence;')[1]==after.split('BeginMissionSequence;')[1],'Unrelated conic source/mission mutated')
 code=sources[SOURCE].decode()
 for anchor in ['combo->showPopup()','QTest::keyClicks','cs->IsInertial()','origin->GetGravitationalConstant()==gm','origin->IsUserDefined()==body','stable();','graph()==accepted','source->document()->availableUndoSteps()==beforeRun','Invalid coupled conic/parabolic state accepted or lost diagnostic','Inertial conic native selectors differ']:
  require(anchor in code,'Original conic assertion omitted: '+anchor)
 if c['axis'].startswith(('MOE','TOE')):require('.Epoch = 25300.25;' in code and 'cs->GetRealParameter("Epoch")==25300.25' in code,'Fixed-epoch input/binding contract omitted')

def qualify(repo,path,inventory):
 q=load(path);r=load(q['review']);m=load(q['fixture']);structure(q,r,m,inventory)
 for p,h in q['artifact_sha256'].items():require(sha(p)==h,'Changed inertial definition artifact: '+p)
 needed={q['review'],q['fixture'],q['baseline_seal'],q['transaction_qualification']}
 for spec in q['runs'].values():needed.update(spec.values());reuse=load(spec['reuse']);needed.update(reuse[k] for k in ['receipt','reference','delta'])
 for path,spec in q['generic_sources'].items():
  needed.add(spec['artifact']);data=Path(spec['artifact']).read_bytes();require(hashlib.sha256(data).hexdigest()==spec['sha256'] and all(t in data.decode() for t in CONIC_GUARDS[path]),'Generic conic implementation missing or changed')
  for run in q['runs'].values():require(load(run['before'])['sources'].get(path)==spec['sha256'],'Generic conic source does not match pre-execution snapshot')
 require(set(q['generic_sources'])==set(CONIC_GUARDS),'Generic conic implementation omitted')
 require(needed<=set(q['artifact_sha256']),'Conic execution metadata omitted from seal')
 _,_,keys=baseline(repo,Path(q['baseline_seal']));required={'FRR!E6','FRR!E7','FRR!E8','FRR!E183','FRR!E195','FRR!E213','FRR!E427','FRR!E433'};require(required<=keys['requirements'] and r['inherited_prefix_keys']==sorted(required),'Missing inherited definition/body/frame obligations')
 tq,tk=shared_transactions(repo,Path(q['transaction_qualification']),inventory);require(tk==['FRR!E9'] and sha(q['transaction_qualification'])==q['transaction_qualification_sha256'],'Changed/missing complete transaction conjunction')
 runs={n:read_run(repo,s,inventory,q['artifact_sha256'],m['physical_inputs']) for n,s in q['runs'].items()};cases={c['key']:c for c in m['cases']}
 for mode,selection in q['selected'].items():
  for s in selection:
   receipt,results,sources=runs[s['run']];name='QtGui.NativeNasaInertialDefinition'+s['case'];require(name==s['test'] and receipt['mode']==mode,'Conic mode/test substituted');check_case(m,cases[s['case']],results[name],sources,q['artifact_sha256'])
   for p,h in m['physical_inputs'].items():require(load(q['runs'][s['run']]['before'])['sources'].get(p)==receipt['source_hashes'].get(p)==h,'Unbound conic physical input')
 return q,[KEY]
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args();_,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,new_GUI_or_engine_executions=0)))
