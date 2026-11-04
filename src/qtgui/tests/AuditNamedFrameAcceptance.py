"""Audit complete declared named-frame consumer support and finite parents.

Independent rotation tables come from frozen CSPICE/geometric axis proofs.
Actual consumer results, all declared types/origin classes, inherited clauses
and the generic coincident-vector implementation are separate obligations.
"""
import argparse,csv,json,math,re
from pathlib import Path
import NasaGuiCoverage as nasa
import AuditAcceptanceRecovery as recovery
from AuditCapiAcceptance import load,require,sha
from AuditCentralBodyPropagationAcceptance import read_run
from BuildNasaCoverage95 import baseline
KEYS=['FRR!E68','FRR!E69','FRR!E88','FRR!E89']
SOURCE='src/qtgui/tests/NasaNamedFrameConsumerTests.cpp'
FIXTURE='src/qtgui/tests/NasaNamedFrameConsumerCases.json'
AXES=['MJ2000Eq','MJ2000Ec','TOEEq','TOEEc','MOEEq','MOEEc','MODEq','MODEc','BodyFixed','BodyInertial','GSM','GSE','ObjectReferenced','Topocentric','TODEq','TODEc']
FACETS=['actual-named-frame-popup-and-local-dependencies','pending-Cancel-Apply-exact-graph-Undo-Redo','Unicode-save-reopen-and-read-only-viewer','independent-rotation-three-spanning-directions-three-epochs','actual-native-consumer-frame-and-force-configuration','free-vector-normalization-and-zero-translation','following-sentinel-and-source-native-isolation']
GENERIC_GUARDS={'src/base/burn/Burn.cpp':['coordSystem->ToBaseSystem(A1Mjd(epoch), inDeltaV, outDeltaV, true);','dvInertial[0] = outDeltaV[0];'],'src/base/hardware/Thruster.cpp':['coordSystem->ToBaseSystem(A1Mjd(epoch), inDir, outDir, true);'],'src/base/coordsystem/CoordinateSystem.cpp':['axes->RotateToBaseSystem(epoch, inState, internalState, forceComputation)','if (!coincident)','outState[i] = internalState[i]'],'src/base/coordsystem/AxisSystem.cpp':['outState[p] = rotData[p3]   * pos[0]','rotData[p3+1] * pos[1]','rotData[p3+2] * pos[2]']}

def check_case(matrix,case,consumer,result,sources,artifacts):
 work=Path(result['log']).parent;p=load(work/'proof.json');require([r for r in nasa.proof_records(result) if r.get('checkpoint')=='named-frame-consumer']==[p],'Frame proof differs from native log')
 require(result['status']=='passed' and result['returncode']==0 and p['outcome']=='passed' and p['case']==case['key'] and p['consumer']==consumer and p['axis']==case['axis'] and p['origin_class']==case['origin_class'],'Substituted frame/consumer/native result')
 require(p['facets']==FACETS and p['selections']==2,'Original frame GUI/physics contract omitted')
 require(json.loads(sources[FIXTURE])==matrix,'Different frame/oracle fixture executed')
 for n in ['proof.json','results.csv','baseline.script','accepted.script','build.messages.txt','run.messages.txt','test.log','ctest.xml']:require(str(work/n) in artifacts,'Essential frame artifact omitted: '+n)
 raw=list(csv.reader((work/'results.csv').open()));require(len(raw)==28 and raw[-1]==['42'],'Incomplete frame rows/following sentinel')
 rows=[[float(x) for x in r] for r in raw[:-1]];require(rows==p['actual'] and len(p['expected'])==27,'Frame raw/proof cardinality differs')
 thrust=consumer=='Thruster';state=[7000,-1200,900,1.5,6.75,-2]
 for rowIndex,row in enumerate(rows):
  group,phase=rowIndex//3,rowIndex%3;s=case['samples'][group//3];direction=matrix['directions'][group%3];norm=math.sqrt(sum(v*v for v in direction));l1=sum(abs(v) for v in direction);rotation=s['rows'];world=[math.fsum(rotation[j][i]*direction[j] for j in range(3)) for i in range(3)]
  expected=[group,phase,s['epoch'],41,1000,1400]+[v+(world[c-3]*matrix['impulse_scale_km_s'] if not thrust and phase==1 and c>=3 else 0) for c,v in enumerate(state)]
  if thrust:
   firing=phase==1;force=[6*v/norm if firing else 0 for v in world];expected += [6 if firing else 0,300 if firing else 0,-.4*10/(8*300) if firing else 0]+force+[v/(1400*1000) for v in force]+[-.4*10/(8*300) if firing else 0]
  require(len(row)==len(expected)==len(p['expected'][rowIndex])==(22 if thrust else 12),'Frame independent output fields omitted')
  for c,(observed,wanted,claimed) in enumerate(zip(row,expected,p['expected'][rowIndex])):
   tolerance=1e-10 if c==2 else 1e-9
   if thrust and 15<=c<=17:tolerance=6*l1/norm*case['matrix_bound']+1e-9
   if thrust and 18<=c<=20:tolerance=6*l1/norm*case['matrix_bound']/(1400*1000)+1e-14
   if thrust and c in [14,21]:tolerance=1e-12
   if not thrust and 9<=c<=11:tolerance=l1*matrix['impulse_scale_km_s']*case['matrix_bound']+1e-9
   require(math.isfinite(observed) and abs(observed-wanted)<=tolerance and abs(claimed-wanted)<=tolerance,'Independent force/delta-V/normalization/clock/sentinel result differs')
 before=(work/'baseline.script').read_text();after=(work/'accepted.script').read_text();actor='TestEngine' if thrust else 'TestKick';strip=lambda s:re.sub(r'^[ \t]*(?:GMAT[ \t]+)?'+actor+r'\.[^\n]*\n','',s,flags=re.M)
 require(before!=after and strip(before)==strip(after) and before.count(actor+'.CoordinateSystem = EarthMJ2000Eq;')==1 and after.count(actor+'.CoordinateSystem = CreatedFrame;')==1,'Frame Apply altered unrelated source or another binding')
 code=sources[SOURCE].decode()
 for token in ['combo->showPopup()','panel->discardChanges()','source->undo()','source->redo()','graph()==baseline','graph()==accepted','read(saved)==acceptedSource.toUtf8()','native->GetStringParameter("CoordinateSystem")=="CreatedFrame"','graph()==beforeRun','text->isReadOnly()','require(error<=tolerance']:
  require(token in code,'Original frame native/GUI assertion omitted: '+token)


def check_oracle(record):
 receipt=load(record['receipt']);require(sha(record['receipt'])==record['receipt_sha256'],'Frozen oracle receipt changed');manifest=json.loads(recovery.payload(receipt['snapshot_manifest']));require(recovery.digest(recovery.payload(receipt['snapshot_manifest']))==record['manifest_sha256'],'Frozen oracle manifest changed');result=next(r for r in receipt['results'] if r['name']==record['test']);proof=next(r for r in nasa.proof_records(result) if r['checkpoint']==record['checkpoint'])
 claim=dict(test=record['test'],source=record['source'],source_sha256=record['source_sha256'],anchor='"samples",samples',checkpoint=record['checkpoint'],facets=record['facets'],scope='Retained independent frame-model oracle inputs only; no new axis mission or new axis requirement credit.')
 recovery.verify_claim(Path(manifest['root']),manifest,receipt,result,claim)
 require(sha(record['log'])==record['log_sha256'] and recovery.digest(recovery.payload(record['script']))==record['script_sha256'],'Oracle log/setup changed')
 require([s for s in proof['samples'] if s['phase']==record['phase']]==record['samples'],'Oracle matrices/physical inputs rewritten')
 source=recovery.payload(Path(manifest['root'])/record['source']).decode();require(('pxform_c' in source and 'EARTH_IAU_1976' in source) or ('basis_rows' in source and 'oracle_inputs' in source) or ('basis_rows' in source and 'Never query the GSE/GSM rotation being tested' in source),'Unreviewed independent oracle producer')


def check_legacy(spec,runs):
 if spec['mode']=='visible':
  receipt=load(spec['receipt']);require(sha(spec['receipt'])==spec['receipt_sha256'],'Legacy visible receipt changed');manifest=json.loads(recovery.payload(receipt['snapshot_manifest']));result=next(r for r in receipt['results'] if r['name']==spec['test']);recovery.verify_claim(Path(manifest['root']),manifest,receipt,result,spec['claim']);require(result['trace']['native_exposure_events']>0,'Legacy named consumer not exposed')
 else:
  receipt,results,sources=runs[spec['run']];result=results[spec['test']];require(receipt['mode']=='headless' and result['status']=='passed' and result['returncode']==0 and spec['claim']['source_sha256']==__import__('hashlib').sha256(sources[spec['claim']['source']]).hexdigest(),'Inherited headless source/result substituted')
 proofs=[p for p in nasa.proof_records(result) if p['checkpoint']==spec['claim']['checkpoint']];require(len(proofs)==1 and proofs[0]['mode']==spec['kind'] and proofs[0]['outcome']=='passed','Inherited named ecliptic/object contract missing');p=proofs[0]
 require(len(p['actual'])==len(p['expected'])==(195 if spec['consumer']=='Thruster' else 252),'Inherited independent output family incomplete')
 # The unchanged original test sources compare every independent component,
 # exact transactions and native commands before emitting this terminal proof.
 return spec['consumer'],spec['kind'],spec['mode']


def structure(q,review,matrix,inventory):
 require(q['contract_kind']=='source-versioned-complete-named-frame-consumer-conjunction' and q['qualified_keys']==KEYS,'Partial/extra frame credit')
 require(q['authority_sha256']==inventory['sources'] and all(q.get(k) is False for k in ['technical_core_approval','full_current_build_qualified','full_NASA_qualified']),'Frame source/scope overclaim')
 known={r['key']:r for r in inventory['requirements']};require({r['key'] for r in review['reviews']}==set(KEYS),'Original frame row omitted')
 for r in review['reviews']:require(r['fingerprint']==known[r['key']]['fingerprint'] and r['source_text']==known[r['key']]['text'] and r['rationale'],'Original frame obligation changed')
 require({r['key'] for r in review['original_reviews']}==set(KEYS) and all(r['fingerprint']==known[r['key']]['fingerprint'] and not r['evidence'] for r in review['original_reviews']),'Original mapped frame reviews omitted/changed')
 require(review['axis_source_rows']=={k:known[k] for k in ['FRR!E'+str(i) for i in range(434,450)]} and review['origin_source_rows']=={k:known[k] for k in ['FRR!E'+str(i) for i in range(428,433)]},'Declared frame/axis inventory changed')
 require(len(matrix['cases'])==18 and len({c['key'] for c in matrix['cases']})==18 and matrix['directions']==[[2,-3,6],[-5,4,2],[0,7,0]] and matrix['thrust_newtons']==6 and matrix['impulse_scale_km_s']==.001,'Incomplete original frame fixtures/directions')
 require({c['axis'] for c in matrix['cases']}|{'MJ2000Ec','ObjectReferenced'}==set(AXES),'Missing declared axis family')
 require({c['origin_class'] for c in matrix['cases']}|{'Spacecraft'}=={'CelestialBody','Spacecraft','GroundStation','Barycenter','LibrationPoint'},'Missing original origin class')
 require(set(q['selected'])=={'headless','visible'},'Missing named-frame execution mode');expected={(c['key'],consumer) for c in matrix['cases'] for consumer in ['Thruster','Impulse']}
 for mode,selected in q['selected'].items():require({(s['case'],s['consumer']) for s in selected}==expected and len(selected)==36,'Missing/duplicate frame-consumer cell')
 require(len(q['legacy_consumers'])==8 and {(s['consumer'],s['kind'],s['mode']) for s in q['legacy_consumers']}=={(c,k,m) for c in ['Thruster','Impulse'] for k in ['NamedEcliptic','NamedObject'] for m in ['headless','visible']},'Missing inherited consumer/mode')
 require(review['all_origin_axis_parameter_cross_products_executed'] is False and review['generic_binding_argument_required'] is True and review['full_astronomical_frame_truth_claimed'] is False,'Frame factorization/scope misrepresented')
 require(review['generic_binding_source_guards']==GENERIC_GUARDS and review['generic_binding_argument'],'Generic coincident-vector proof omitted')


def independent_fixture(matrix,reuse):
 records=reuse['records'];require(len(records)==15,'Independent oracle producer omitted')
 require({c['oracle'] for c in matrix['cases'] if isinstance(c['oracle'],int)}==set(range(15)),'Independent oracle/family link omitted')
 for case in matrix['cases']:
  if isinstance(case['oracle'],int):
   record=records[case['oracle']];samples=record['samples'];require(len(samples)==len(case['samples'])==3,'Independent epochs omitted')
   for source,declared in zip(samples,case['samples']):
    epoch=source.get('mission_epoch_a1mjd',source.get('epoch_a1mjd'))
    if case['key']=='BodyFixed':
     angle=source['oracle_inputs']['rotation_angle_rad'];rows=[[math.cos(angle),math.sin(angle),0],[-math.sin(angle),math.cos(angle),0],[0,0,1]]
    else:rows=source.get('oracle_matrix',source.get('basis_rows'))
    require(declared==dict(epoch=epoch,rows=rows),'Independent frame matrix/epoch changed')
  else:
   require(case['key'] in ['Barycenter','Libration','BodyInertialEarth'] and case['samples']==[dict(epoch=e,rows=[[1,0,0],[0,1,0],[0,0,1]]) for e in [21545,25300.25,30000.75]],'Unreviewed literal inertial oracle')


def qualify(repo,path,inventory):
 q=load(path);review=load(q['review']);matrix=load(q['fixture']);structure(q,review,matrix,inventory)
 for p,digest in q['artifact_sha256'].items():require(sha(p)==digest,'Changed named-frame artifact: '+p)
 seal,original,keys=baseline(repo,Path(q['baseline_seal']));needed={'FRR!E427','FRR!E433'}|{'FRR!E'+str(i) for i in list(range(70,75))+list(range(90,95))};require(needed<=keys['requirements'] and review['required_inherited_source_keys']==sorted(needed),'Missing original parent/local/body/frame clauses')
 runs={name:read_run(repo,spec,inventory,q['artifact_sha256'],matrix['physical_inputs']) for name,spec in q['runs'].items()}
 for name,spec in q['runs'].items():
  before=load(spec['before']);receipt=runs[name][0]
  for p,digest in matrix['physical_inputs'].items():require(before['sources'].get(p)==digest==receipt['source_hashes'].get(p),'Unbound frame physical input')
 cases={c['key']:c for c in matrix['cases']}
 for mode,selections in q['selected'].items():
  for s in selections:
   receipt,results,sources=runs[s['run']];name='QtGui.NativeNasaNamedFrame'+s['case']+s['consumer'];require(receipt['mode']==mode and s['test']==name,'Frame test/mode substituted');check_case(matrix,cases[s['case']],s['consumer'],results[name],sources,q['artifact_sha256'])
 reuse=load(q['oracle_reuse']);require(reuse['fixture_sha256']==sha(q['fixture']),'Independent oracle/fixture identity differs')
 independent_fixture(matrix,reuse)
 for record in reuse['records']:check_oracle(record)
 for spec in q['legacy_consumers']:check_legacy(spec,runs)
 for source,tokens in review['generic_binding_source_guards'].items():
  code=(repo/source).read_text()
  for token in tokens:require(token in code,'Missing generic free-vector binding implementation: '+source)
 require(review['generic_binding_argument'] and all(not r['evidence'] for r in review['original_reviews']),'Original mapped clause omitted')
 return q,KEYS


if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args();_,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,new_GUI_or_engine_executions=0)))
