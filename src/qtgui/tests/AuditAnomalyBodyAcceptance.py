"""Exact EA/MA read-write leaves with real native bodies and controlled inputs."""
import argparse,copy,hashlib,json,math,struct,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
import BuildNasaCoverageExtension as common
import AuditNasaCoveragePrefix as prefix
from AuditWorksheetOrbitBodyAcceptance import trace,proof_from_log
from OrbitalParameterOracle import conic,anomaly_from_eccentric,eccentric_from_mean
sha,load,require=common.sha,common.load,common.require
KEYS=['FRR!E506','FRR!E513']
KIND='source-versioned-complete-body-relative-anomaly-access'
CONTEXT=[f'FRR!E{i}' for i in [*range(183,195),*range(213,221),*range(501,526),536]]
BODIES=['Sun','Mercury','Venus','Earth','Luna','Mars','Jupiter','Saturn','Uranus','Neptune','Pluto','UserMoon','UserAsteroid','UserComet','UserPlanet']
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','native_OS_input_qualified']


def kernel(raw,literal):
 require(raw[:8]==b'DAF/SPK ' and raw[88:96]==b'LTL-IEEE' and len(raw)%1024==0,'Invalid controlled SPK header')
 require(struct.unpack_from('<2i',raw,8)==(2,6),'Wrong SPK descriptor shape');record,back,free=struct.unpack_from('<3i',raw,76);seen=set();got={};previous=0
 while record:
  require(record not in seen and 0<record<=len(raw)//1024,'Invalid SPK record chain');seen.add(record);off=(record-1)*1024;nxt,prev,count=struct.unpack_from('<3d',raw,off);require(prev==previous and count==int(count) and 0<count<=25,'Invalid SPK summary')
  for i in range(int(count)):
   at=off+24+40*i;first,last=struct.unpack_from('<2d',raw,at);target,center,frame,kind,a,b=struct.unpack_from('<6i',raw,at+16);require((center,frame,kind)==(0,1,8) and [first,last]==[-1e11,1e11] and 1<=a<=b<free,'Wrong controlled segment')
   words=list(struct.unpack_from('<'+str(b-a+1)+'d',raw,(a-1)*8));require(len(words)==16 and words[-4:]==[-1e11,2e11,1.,2.] and words[:6]==words[6:12] and target not in got,'Wrong two-knot constant input');got[target]=words[:6]
  previous,record=record,int(nxt)
 require(previous==back and len(got)==20 and got=={int(k):v for k,v in literal['constant_states'].items()} and all(all(math.isfinite(x) for x in v) and v[3:]==[0,0,0] for v in got.values()),'Controlled input states differ from independent literal table')
 return len(got)


def truth(spec):
 elements=spec['initial_elements'].copy();e=elements[1];value=float(spec['expression']);p=spec['parameter']
 if e<1:
  eccentric=value if p=='EA' else eccentric_from_mean(e,value);elements[5]=anomaly_from_eccentric(e,eccentric);angle=value%360
 else:
  require(p=='MA' and e>1,'Undefined conic admitted');mean=math.radians(value);lo,hi=-5.,5.
  for _ in range(110):
   h=(lo+hi)/2
   if e*math.sinh(h)-h<mean:lo=h
   else:hi=h
  h=(lo+hi)/2;elements[5]=math.degrees(2*math.atan2(math.sqrt(e+1)*math.sinh(h/2),math.sqrt(e-1)*math.cosh(h/2)));angle=value
 return [angle]+conic(elements,spec['mu'])


def case_contract(spec,proof,raw,script):
 require(proof['outcome']=='passed' and proof['family']==spec['key'] and proof['destination']==spec['destination'] and proof['expression']==spec['expression'],'Wrong actual anomaly assignment')
 require(proof['origin']==spec['origin'] and proof['body_case']==spec['body_case'] and proof['reference_mu']==spec['mu'],'Wrong actual origin/Mu')
 if 'expected_origin_type' in spec:require(proof['origin_type']==spec['expected_origin_type'],'Actual body factory class differs')
 require(proof['numeric_sentinel']==9 and len(proof['rejections'])==3 and all(x['diagnostic'] for x in proof['rejections']),'Missing invalid Apply/OK or successor assertions')
 require(spec['absolute_tolerance']==2e-8 and spec['relative_tolerance']==2e-12 and spec['column_absolute_tolerances']==[1e-9]+[2e-8]*3+[2e-11]*3,'Original numerical bounds changed')
 expected=truth(spec);require(len(raw)==8 and raw[-1]==9 and raw[:-1]==proof['actual'] and len(proof['expected'])==7,'Wrong raw result shape/sentinel')
 for i,(x,y) in enumerate(zip(raw[:-1],expected)):
  require(math.isfinite(x) and abs(x-y)<=spec['column_absolute_tolerances'][i]+spec['relative_tolerance']*abs(y) and abs(proof['expected'][i]-y)<=1e-10,'Independent full Cartesian/anomaly reconstruction failed')
 require('Sat.'+spec['origin']+'.'+spec['parameter']+' = '+spec['expression']+';' in script,'Desired setter not actually persisted')
 return max(abs(x-y) for x,y in zip(raw[:-1],expected))


def component(stage,fixtures,aggregate):
 runtime_hashes={}
 def runtime_sha(name):
  key=str(Path(name).resolve())
  if key not in runtime_hashes:runtime_hashes[key]=sha(key)
  return runtime_hashes[key]
 total=exposures=0;maximum=0.;controlled=fixtures['controlled'];old=fixtures['strict'];circle=fixtures['routing'];boundary=fixtures['boundary'];literal=load(stage/'controlled-body-inputs-v1.json');plan=load(stage/'plan-v4.json');build=load(stage/'build-worker-v2.json');physical=plan['physical_inputs'];require(len(physical)==1,'Missing controlled input identity');path=Path(next(iter(physical)));require(sha(path)==physical[str(path)],'Controlled input changed');segments=kernel(path.read_bytes(),literal)
 require({(c['body_case'],c['parameter']) for c in controlled['cases']}=={(b,p) for b in BODIES for p in ['EA','MA']},'Missing original body/property case')
 oldby={c['key']:c for c in old['cases']}
 for c in controlled['cases']:
  before=oldby[c['key']]
  for k in ['mu','parameter','initial_elements','expected','expression','before','destination','report','absolute_tolerance','relative_tolerance','column_absolute_tolerances','body_case','origin']:require(c[k]==before[k],'Controlled inputs altered the conic oracle/bounds')
 for version,fx in [('v4',controlled),('v2',circle),('v3',boundary)]:
  for profile in ['headless','visible']:
   for spec in fx['cases']:
    folder=stage/(profile+'-'+version)/(spec['parameter']+'-'+spec['body_case'])
    if version=='v3' and spec['key'].startswith('Hyper'):folder=stage/(profile+'-'+version)/'HyperMA-Earth'
    rec=load(folder/'result.json');require(rec['status']=='passed' and rec['returncode']==0 and rec['profile']==profile and rec['timeout_seconds']==60,'Unsuccessful actual body witness');proof=proof_from_log(folder/'test.log');require(rec['proofs']==[proof],'Stored proof differs from executed log');raw=[float(x) for x in (folder/'values.txt').read_text().strip().split(',')]
    if version=='v4':
     sig=rec['signature'];require(sig['source_sha256']==plan['source_sha256']==build['source_sha256'] and sig['fixture_sha256']==plan['fixture_sha256']==build['fixture_sha256'] and sig['binary_sha256']==build['binary_sha256'] and sig['physical_inputs']==physical,'Wrong controlled executed identities')
     base_loaded=[l.strip() for f in folder.glob('loader.*') for l in f.read_text().splitlines() if 'calling init:' in l and 'libGmatBase.so' in l];require(base_loaded==rec['actual_loaded_GmatBase'] and base_loaded and all(runtime_sha(l.split('calling init:',1)[1].strip())==sig['GmatBase_sha256'] for l in base_loaded),'Actual base library differs from recorded identity')
     loaded=[l.strip().split('calling init:',1)[1].strip() for f in folder.glob('loader.*') for l in f.read_text().splitlines() if 'calling init:' in l and 'libEKF.so' in l];require(loaded and all(Path(x).resolve()==(stage.parent/'coverage95-20261110-snc-state-v1/runtime-v2/lib/libEKF.so.R2026a').resolve() and runtime_sha(x)==sig['EKF_sha256'] for x in loaded),'Wrong actual repaired EKF version');require('SolarSystem.EphemerisSource = SPICE;' in (folder/'mission.script').read_text(),'Controlled source not actually selected')
    maximum=max(maximum,case_contract(spec,proof,raw,(folder/'mission.script').read_text()));exposures+=trace(folder,profile);total+=1
 # Retain the strict noncircular Earth controls and eight actual atomic API rejections.
 guards=0
 for prop in ['EA','MA']:
  spec=oldby[prop+'Earth'];folder=stage/'headless-v1'/(prop+'-Earth');rec=load(folder/'result.json');require(rec['status']=='passed' and rec['returncode']==0,'Strict Earth control did not pass');proof=proof_from_log(folder/'test.log');raw=[float(x) for x in (folder/'values.txt').read_text().strip().split(',')];case_contract(spec,proof,raw,(folder/'mission.script').read_text());require(len(proof['native_guards'])==4 and all(x['rejected'] is True and x['state_unchanged'] is True and x['diagnostic'] for x in proof['native_guards']),'Missing native nonfinite/conic atomicity');guards+=4
 failed=stage/'headless-v1/EA-Mercury';rec=load(failed/'result.json');require(rec['status']=='failed' and rec['returncode']==1 and 'Assignment result differs' in (failed/'test.log').read_text(),'Strict DE precision failure lost/waived')
 # Reuse the original signed-angle controls at their actual source version.
 saved=aggregate/'headless-all-v2/QtGui.ImplementationAnomalyWrites';v=load(saved/'verification.json');require(v['status']=='passed' and v['case']=='AnomalyWrites' and {(x['anomaly'],x['assigned_degrees']) for x in v['observations'] if x['anomaly'] in ['EA','MA']}=={(p,a) for p in ['EA','MA'] for a in [-40,70]},'Signed native controls missing');tests=list(ET.parse(saved/'ctest.xml').iter('testcase'));require(len(tests)==1 and all(tests[0].find(x) is None for x in ['failure','error','skipped']),'Signed original CTest failed')
 require(all(sha(n)==h for n,h in runtime_hashes.items()),'Runtime changed during offline audit')
 return {'controlled_noncircular_GUI_passes':60,'reused_default_DE_circular_GUI_passes':60,'reused_wrapping_hyperbolic_GUI_passes':6,'reused_strict_Earth_GUI_passes':2,'reused_original_signed_EA_MA_cases':4,'native_atomic_rejections':guards,'native_body_cases':15,'native_body_factory_types':5,'independent_constant_SPK_segments':segments,'checked_raw_scalars':total*8+16,'maximum_controlled_or_routing_error':maximum,'visible_native_exposures':exposures,'strict_DE_precision_failure_retained':True,'strict_DE_uniform_precision_qualified':False,'original_assertions_changed':0,'native_tests_rerun_for_admission':0}


def structure(q,scope,inventory):
 known={r['key']:r for r in inventory['requirements']};require(q['contract_kind']==KIND and q['qualified_keys']==KEYS and q['authority_sha256']==inventory['sources'],'Different/extra anomaly credit');require(scope['candidate_rows']==[known[k] for k in KEYS] and scope['complete_original_context']==[known[k] for k in CONTEXT],'Original body/parent context changed');require(scope['bodies']==BODIES and scope['newly_resolved_hold_keys']==[] and scope['other_row_credit']==0 and scope['original_assertions_changed']==0 and scope['failures_waived'] is False and scope['strict_DE_uniform_precision_qualified'] is False and scope['generic_source_argument'],'Incomplete/unsupported scope');require(all(q[k] is False and scope[k] is False for k in GATES),'Broader qualification promoted')


def qualify(repo,path,inventory):
 q=load(path)
 for p,h in q['artifact_sha256'].items():require(sha(p)==h,'Changed anomaly evidence: '+p)
 scope=load(q['scope_review']);structure(q,scope,inventory);frozen={}
 for archive,identities in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
  with tarfile.open(q[archive]) as bundle:
   require(set(bundle.getnames())==set(q[identities]),'Changed source partition')
   for n,h in q[identities].items():
    raw=bundle.extractfile(n).read();require(not raw.startswith(b'\x7fELF') and hashlib.sha256(raw).hexdigest()==h,'Changed/non-source capsule member')
    if identities=='proof_source_sha256':frozen[n]=raw
 original=load(q['protected_input_baseline']);authorized=load(q['authorized_input_manifest']);guard=load(q['component_input_integrity']);require(len(original)==len(authorized)==1334 and all(q['proof_source_sha256'][n]==h for n,h in original.items()),'Original input preimages lost');require(sorted(n for n in original if original[n]!=authorized[n])==['src/qtgui/tests/PlotIntegrationStepTests.cpp','src/qtgui/tests/SharedPlaybackTests.cpp'] and guard['authorized_inputs_final']==authorized and guard['changed_by_this_component']==[],'Unapproved original amendment')
 previous=load(q['prior_full_audit']);require(len(previous['original_hold_keys'])==59 and not set(KEYS)&set(previous['original_hold_keys']),'Formal hold scope changed')
 names={'controlled':'AnomalyBodyControlledCases.json','strict':'AnomalyBodyWriteCases.json','routing':'AnomalyBodyRoutingCases.json','boundary':'AnomalyWriteBoundaryCases.json'};fixtures={k:json.loads(frozen['src/qtgui/tests/'+n]) for k,n in names.items()};worker=frozen['src/qtgui/tests/AnomalyBodyWriteTests.cpp'];require(hashlib.sha256(worker).hexdigest()==load(Path(q['component_stage'])/'build-worker-v2.json')['source_sha256'],'Wrong controlled source version')
 for token in [b'GetGravitationalConstant()',b'GetTypeName()',b'source->undo()',b'source->redo()',b'window.saveScriptTo',b'window.loadScript',b'column_absolute_tolerances',b'Rejected anomaly write partially mutated state']:require(token in worker,'Missing actual calculation/transaction/atomicity assertion')
 facts=component(Path(q['component_stage']),fixtures,Path(q['aggregate_stage']));rejected=0
 def rejects(call):
  nonlocal rejected
  try:call()
  except (ValueError,KeyError):rejected+=1
  else:raise ValueError('Damaged anomaly admission accepted')
 for field,value in [('qualified_keys',KEYS+['FRR!E501']),('scientific_approval',True)]:
  changed=copy.deepcopy(q);changed[field]=value;rejects(lambda:structure(changed,scope,inventory))
 for field,value in [('bodies',BODIES[:-1]),('strict_DE_uniform_precision_qualified',True),('failures_waived',True)]:
  changed=copy.deepcopy(scope);changed[field]=value;rejects(lambda:structure(q,changed,inventory))
 spec=fixtures['controlled']['cases'][0];folder=Path(q['component_stage'])/'headless-v4'/(spec['parameter']+'-'+spec['body_case']);proof=proof_from_log(folder/'test.log');raw=[float(x) for x in (folder/'values.txt').read_text().strip().split(',')];script=(folder/'mission.script').read_text()
 for field,value in [('origin','Earth'),('origin_type','Comet'),('numeric_sentinel',0),('expression','0')]:
  changed=copy.deepcopy(proof);changed[field]=value;rejects(lambda:case_contract(spec,changed,raw,script))
 return q,KEYS,{'status':'passed','qualified_keys':KEYS,'whole_row_conjunction':True,'component':facts,'new_requirement_credit':2,'newly_resolved_original_hold_keys':[],'damaged_admission_controls_rejected':rejected,'native_tests_rerun':0,'runtime_binary_copies':0,**{k:False for k in GATES}}


def build(repo, baseline, path, output):
    seal, inventory, keys = prefix.baseline(repo, baseline)
    original = {kind:set(values) for kind,values in keys.items()}
    _, incoming, audit = qualify(repo, path, inventory)
    known = {kind:{row['key']:row for row in inventory[kind]} for kind in keys}
    require(not set(incoming) & keys['requirements'], 'Duplicate body-access credit')
    keys['requirements'] = common.merge_keys(keys['requirements'], incoming, known['requirements'])
    coverage = {kind:{'verified':len(keys[kind]), 'total':len(known[kind]),
        'percent':round(100*len(keys[kind])/len(known[kind]),1),
        'new_verified':len(keys[kind])-len(original[kind])} for kind in keys}
    remaining = {kind:[row for row in seal['remaining_original_rows'][kind]
        if row['key'] not in keys[kind]] for kind in keys}
    result = {'status':'passed', 'scope':'Sealed source-versioned prefix plus complete original body-relative EA/MA read-write capability',
        'baseline':str(baseline), 'baseline_sha256':sha(baseline), 'authority_sha256':inventory['sources'],
        'coverage':coverage, 'target_requirements':1101, 'target_reached':len(keys['requirements'])>=1101,
        'requirements_still_needed':max(0,1101-len(keys['requirements'])),
        'additions':[{'key':key, 'kind':'requirements', 'fingerprint':known['requirements'][key]['fingerprint'],
            'source_text':known['requirements'][key]['text'], 'qualification':str(path),
            'qualification_sha256':sha(path), 'status':'verified-source-versioned'} for key in incoming],
        'remaining_counts':{kind:len(rows) for kind,rows in remaining.items()},
        'remaining_original_rows':remaining, 'technical_core_approval':False,
        'full_current_build_qualified':False, 'full_NASA_qualified':False,
        'limits':['Original 1158/2367 denominators, texts and fingerprints are unchanged.',
                  'Only FRR E506/E513 are added; broader body/frame parents and all59 formal hold statuses remain open/unchanged.',
                  'Valid-conic EA/MA body-relative capability is qualified with controlled native inputs. Strict DE uniform precision, physical ephemeris, any-coordinate-system, native OS input, platform and full-build approval remain separate.']}
    with Path(output).open('x') as file:
        json.dump(result, file, indent=2); file.write('\n')
    return result, audit


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.baseline:
        result, _ = build(args.repo, args.baseline, args.qualification, args.output)
        print(json.dumps({'coverage':result['coverage'], 'still_needed':result['requirements_still_needed']}))
    else:
        _, _, result = qualify(args.repo, args.qualification, nasa.inventory(args.repo))
        with args.output.open('x') as file:
            json.dump(result, file, indent=2); file.write('\n')
        print(json.dumps(result))
