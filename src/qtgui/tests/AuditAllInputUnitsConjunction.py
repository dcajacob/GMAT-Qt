"""Complete unit-display conjunction over field and construction inventories."""
from pathlib import Path
import hashlib
import json
import tarfile

KEY = 'All Others!E13'
FINGERPRINT = 'c2d109c8497529a5e6f07e66ede69ada918084b2173a55d0a321703e5ca23608'
KIND = 'complete-gui-input-unit-conjunction-v1'
FAMILY_STAGES = {
 'Commands':['command-input-units','ancillary-input-units','function-argument-units'],
 'Comparisons':['ancillary-input-units'],
 'ResourceTables':['body-physical-units','unit-evidence-join','owned-input-units','numeric-text-units','record-index-units'],
 'Atmosphere':['force-unit-evidence-join','nested-unit-witnesses'],
 'Attitude':['spacecraft-input-units','owned-input-units'],
 'Mass':['spacecraft-input-units'], 'Burn':['unit-evidence-join'],
 'Bodies':['body-physical-units','current-input-unit-delta'],
 'Ephemeris':['numeric-text-units','epoch-unit-transitions'],
 'Epochs':['numeric-text-units','epoch-unit-transitions'],
 'Gravity':['nested-unit-witnesses'], 'Stations':['timing-geometry-units','unit-evidence-join'],
 'PlotSetup':['camera-vector-units','count-input-units'],
 'Drawing':['display-options-units'], 'Playback':['display-options-units'],
 'Masks':['display-options-units'], 'Stars':['display-options-units'],
 'PlotStyle':['data-display-units','display-options-units'],
 'Polyhedra':['nested-unit-witnesses'],
 'Integrators':['current-input-unit-delta','contextual-input-units'],
 'Dynamic':['data-display-units'], 'Reports':['picker-input-units','function-argument-units'],
 'Regions':['geometry-array-units'], 'SolarSystem':['display-options-units'],
 'Thrusters':['unit-evidence-join','electric-power-units','hardware-origin-units'],
 'Models':['body-physical-units','spacecraft-input-units'],
}
NONQUANTITY_FAMILIES = {'Text','Scripts','External','Find','Paths','Colors','Outputs','Tracking'}
STRING_RULES = {
 'representation-choice':{'EpochFormat','InputEpochFormat','WarmStartEpochFormat','EstimationEpochFormat'},
 'named-mode-or-model':{'ThinMode','PosVelSource','RotationDataSource','InversionAlgorithm','ReportFormat','ReportTimeFormat','Type','SolveFor','ThrottleLogicAlgorithm','IonosphereModel','TroposphereModel','DataSource','ReportCoordinates','ShadowModel','ExtendedMassPropertiesModel','AberrationCorrection','FrequencyBand'},
 'identifier':{'SpiceFrameId','Id','FormatVersion'},
 'file-or-directory':{'InputWarmStartFile','OutputWarmStartFile','FunctionPath','SpiceGridFrameFile','AreaFileName'},
 'text-separator':{'Delimiter'}, 'resource-name':{'EphemerisName','CoordinateSystem'},
 'string-value':{'Value'}, 'symbolic-participant-epoch':{'EstimationEpoch'},
}
LIST_RULES = {
 'file-list':{'FileNames','PlanetarySpiceKernelName','OrbitSpiceKernelName','FrameSpiceKernelName','AttitudeSpiceKernelName','SCClockSpiceKernelName','FileName','RampTable'},
 'mode-list':{'DataTypes','EventTypes','EclipseTypes'},
 'resource-or-parameter-list':{'DataFilters','SolveFors','ShadowBodies','AddThrustSegment','MassSource'},
}

def load(path):return json.loads(Path(path).read_text())
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def need(value,message):
 if not value:raise ValueError(message)

def route_pairs(path,repo):
 p=Path(path);p=p if p.is_absolute()else Path(repo)/p;d=load(p);out=set()
 for key in ['completed_saved_records','completed_saved_numeric_records_this_batch','newly_matched_saved_records','completed_current_records','current_document_definitions']:
  for row in d.get(key,[]):
   if isinstance(row,dict)and 'type'in row and 'field'in row:out.add((row['type'],row['field']))
 out.update(tuple(v)for v in d.get('completed_current_pairs',[]))
 out.update(tuple(v)for v in d.get('completed_this_batch',{}).get('type_field_pairs',[]))
 if 'target_fields'in d:out.update((t,f)for t in ['BatchEstimator','BatchEstimatorInv']for f in d['target_fields'])
 if p.name=='stage-receipt-v1.json':
  name=p.parent.name
  if name.endswith('force-unit-evidence-join-v1'):out.update((t,f)for t in ['ForceModel','ODEModel']for f in ['SRP.Flux','SRP.Nominal_Sun'])
  if name.endswith('numeric-text-units-v1'):
   with tarfile.open(p.parent/'sources-after-v1.tar.gz')as archive:
    cases=json.loads(archive.extractfile('src/qtgui/tests/NumericTextUnitCases.json').read())['cases']
   out.update((v['kind'],f)for v in cases for f in v['units'])
  if name.endswith('record-index-units-v1'):out.update(tuple(v)for v in load(p.parent/'frozen-plan-v1.json')['expected_case_fields'])
  if name.endswith('camera-vector-units-v1'):out.update(('OrbitView',f)for f in load(p.parent/'frozen-plan-v1.json')['fields'])
 if ('MessageWindow','Dec')in out:out.add(('MessageWindow','Precision'))
 return out

def nonquantity(row,owned):
 pair=row['type'],row['field'];field=row['field'];types=set(row['native_parameter_types'])
 if pair in owned:return 'owned-field source review'
 need(pair not in {('OrbitView',f)for f in ['ViewPointReference','ViewPointVector','ViewDirection']},'Camera vector mode lost its numerical obligation')
 need(field!='RecordNumbers','Record-index obligation lost')
 need(types and types<={'Boolean','BooleanArray','OnOff','Filename','Enumeration','Object','ObjectArray','Color','String','StringArray'},'Numerical or unclassified input has no execution evidence')
 if 'String'in types:
  kinds=[kind for kind,fields in STRING_RULES.items()if field in fields];need(len(kinds)==1,'Unclassified string input')
  if kinds[0]=='string-value':need(row['type']=='String','Numeric string mistaken for text')
  if kinds[0]=='symbolic-participant-epoch':need(row['type']in ['BatchEstimator','BatchEstimatorInv'],'Numerical epoch mistaken for symbolic choice')
  return kinds[0]
 if 'StringArray'in types:
  kinds=[kind for kind,fields in LIST_RULES.items()if field in fields];need(len(kinds)==1,'Unclassified string-array input');return kinds[0]
 return {'Boolean':'logical choice','BooleanArray':'logical choices','OnOff':'logical choice','Filename':'file path','Enumeration':'fixed choice','Object':'object or parameter name','ObjectArray':'object or parameter names','Color':'colour encoding'}[next(iter(types))]

def validate_domains(rows,known,owned,repo):
 expected={(v['type'],v['field']):v for v in known};seen=set();quantities=0;routes={}
 for row in rows:
  pair=row['type'],row['field'];need(pair in expected and pair not in seen,'Missing, duplicated or unknown field');seen.add(pair)
  need(row['native_parameter_types']==expected[pair]['native_parameter_types'] and row['states']==expected[pair]['states'],'Field definition or conditional state inventory changed')
  if row['evidence_routes']:
   for route in row['evidence_routes']:
    if route not in routes:routes[route]=route_pairs(route,repo)
   need(any(pair in routes[route]for route in row['evidence_routes']),'Field has no completed execution-evidence binding');quantities+=1
  else:
   kind=nonquantity(row,owned)
   if kind=='owned-field source review':need(row['nonphysical_classification']==owned[pair],'Owned-field classification differs')
   else:need(row['nonphysical_classification']==kind,'Nonquantity classification differs')
 need(seen==set(expected)and len(seen)==1047 and quantities==612,'Full field conjunction incomplete')
 return quantities

def check_stage(stage):
 stage=Path(stage);receipt=load(stage/'stage-receipt-v1.json');manifest=stage/'evidence-manifest-v1.json'
 if manifest.exists():
  need(receipt['manifest_sha256']==sha(manifest),'Stage manifest identity changed');data=load(manifest)
  artifacts=data.get('artifact_sha256',data.get('artifacts_sha256',{}))
 else:artifacts=receipt.get('artifact_sha256',receipt.get('artifacts_sha256',{}))
 need(bool(artifacts),'Missing stage execution artifacts')
 for path,digest in artifacts.items():need(sha(path)==digest,'Changed executed evidence: '+path)
 verification=stage/'stage-verification-v1.json'
 if verification.exists():need(load(verification)['status']=='passed','Component verification is not passing')
 else:need(stage.name.endswith('force-unit-evidence-join-v1')and receipt['summary']['status']=='passed','Missing component verification')
 if 'reader'in receipt:need(sha(receipt['reader'])==receipt['reader_sha256'],'Component reader changed')
 return len(artifacts)

def audit_index(index,repo):
 repo=Path(repo)
 for path,digest in index['artifact_sha256'].items():need(sha(path)==digest,'Full-unit index input changed: '+path)
 fields=load(index['field_review'])['records'];known=load(index['native_field_inventory'])
 owned={(v['type'],v['field']):v['kind']for v in load(index['owned_classification'])['classified_fields']}
 need(len(owned)==13,'Owned-field classification inventory differs')
 validate_domains(fields,known,owned,repo)
 constructions=load(index['construction_inventory']);families=load(index['family_review']);keys=[]
 need(len(families)==34 and {f['family']for f in families}==set(FAMILY_STAGES)|NONQUANTITY_FAMILIES,'Input-family inventory incomplete')
 stages={row['stage']for row in index['execution_stages']}
 for family in families:
  name=family['family'];keys+=family['construction_keys']
  need(family['stages']==FAMILY_STAGES.get(name,[]),'Family execution obligations changed')
  if name in FAMILY_STAGES:
   need(all(any(Path(path).name=='complete18-20261112-'+short+'-v1'for path in stages)for short in family['stages']),'Family has no required executed witness')
  else:need(family['classification']=='names, code, paths, logical choices, colour encodings or outputs; no single numerical quantity','Unqualified nonquantity family')
 need(len(keys)==len(set(keys))==357 and set(keys)=={v['key']for v in constructions['records']},'Construction source-domain coverage incomplete')
 with tarfile.open(index['frontend_archive'])as archive:
  need(set(archive.getnames())==set(constructions['source_sha256']),'Frontend source archive incomplete')
  for name,digest in constructions['source_sha256'].items():need(hashlib.sha256(archive.extractfile(name).read()).hexdigest()==digest,'Frontend source identity changed')
 for row in index['execution_stages']:need(check_stage(row['stage'])==row['verified_artifacts'],'Execution-stage binding changed')
 continuity=load(index['native_continuity']);need(continuity['status']=='passed'and len(continuity['unchanged_native_images'])>=3,'Native type continuity missing')
 census=load(continuity['source_census'])
 expected_images={p:h for p,h in census['image_sha256'].items()if Path(p).is_relative_to(repo/'application')and'.so'in Path(p).name}
 need(continuity['unchanged_native_images']==expected_images,'Native provider image inventory incomplete')
 need(set(continuity['native_image_snapshots'])==set(continuity['unchanged_native_images']),'Native snapshot inventory incomplete')
 for path,digest in continuity['unchanged_native_images'].items():need(sha(continuity['native_image_snapshots'][path])==digest,'Frozen native type owner differs from census')
 return dict(status='passed',resource_field_pairs=1047,unit_bearing_or_count_fields=612,classified_nonphysical_fields=435,construction_templates=357,construction_families=34,execution_stages=len(stages),native_tests_run=0,full_current_build_qualified=False)

def qualify(repo,path,inventory,admitted):
 q=load(path);need(q['contract_kind']==KIND and q['qualified_keys']==[KEY],'Wrong unit conjunction contract')
 for artifact,digest in q['artifact_sha256'].items():need(sha(artifact)==digest,'Qualification artifact changed')
 known={row['key']:row for row in inventory['requirements']}
 need(KEY not in admitted and 'All Others!E12'in admitted,'Unit row duplicate or documentation prerequisite missing')
 need(q['authority_sha256']==inventory['sources']and q['candidate_rows']==[known[KEY]]and known[KEY]['fingerprint']==FINGERPRINT,'Original unit requirement changed')
 result=audit_index(load(q['evidence_index']),repo)
 return q,[KEY],dict(status='passed',requirement=KEY,fingerprint=FINGERPRINT,qualified_keys=[KEY],whole_row_conjunction=True,conjunction=result,native_tests_run=0)
