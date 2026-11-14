"""Admit only the complete FRC93 scalar-input family, reusing sealed witnesses."""
import argparse,copy,hashlib,json,math,tarfile
from pathlib import Path
import NasaGuiCoverage as nasa
import BuildNasaCoverageExtension as common
import AuditNasaCoveragePrefix as prefix
from AuditWorksheetOrbitBodyAcceptance import trace,proof_from_log
sha,load,require=common.sha,common.load,common.require
KEYS=['FRC!E93']
KIND='source-versioned-complete-Achieve-input-family'
CONTEXT=[f'FRC!E{i}' for i in range(90,98)]+[f'FRR!E{i}' for i in range(501,589)]
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','native_OS_input_qualified']


def check_case(spec,proof,raw,script):
 require(proof['outcome']=='passed' and proof['checkpoint']=='achieve-scalar-family' and proof['case']==spec['key'] and proof['parameter_type']==spec['parameter_type'] and proof['reference']==spec['reference'],'Wrong Achieve family member')
 for k in ['expected','absolute_tolerance','solver_control_tolerance','source_rows']:require(proof[k]==spec[k],'Independent scalar oracle/bound changed')
 truth=spec['expected'];tol=spec['absolute_tolerance'];ctol=spec['solver_control_tolerance'];scale=max(1,abs(truth)*.001)
 require(raw==proof['actual'] and len(raw)==3 and all(math.isfinite(v) for v in raw),'Wrong/nonfinite raw Achieve result')
 require(abs(raw[0]-(truth+scale))<=ctol and abs(raw[1]-truth)<=tol and raw[2]==42,'Independent fixed target/control/sentinel failed')
 require(proof['requested_tolerance']==tol and abs(proof['native_achieved']-truth)<=tol and abs(proof['native_achieved']-raw[1])<=tol,'Actual native AchievedValue did not satisfy its independent target')
 require(proof['picker_visited'] is True and proof['invalid_picker_rejections']==6 and proof['independent_control_unchanged'] is True,'Missing actual numeric selector/negative controls')
 require("Achieve 'Family goal' DC("+spec['reference']+' = ' in script and 'Control = ' in script,'Measured goal not actually configured')


def component(stage,reuse,fixture,legacy):
 build=load(stage/'build-worker-v2.json');signatures=[];exposures=0;count=0
 require(len(fixture['cases'])==96 and len(legacy['cases'])==2,'Incomplete scalar spellings')
 for version,fx in [('v2',fixture),('v3',legacy)]:
  plan=load(stage/('plan-'+version+'.json'));require(set(plan['cases'])=={x['key'] for x in fx['cases']},'Changed execution plan')
  for profile in ['headless','visible']:
   for spec in fx['cases']:
    folder=stage/(profile+'-'+version)/spec['key'];rec=load(folder/'result.json');sig=rec['signature']
    require(rec['status']=='passed' and rec['returncode']==0 and rec['case']==spec['key'] and rec['profile']==profile and rec['timeout_seconds']==60,'Incomplete actual scalar execution')
    require(sig['source_sha256']==plan['source_sha256']==build['source_sha256'] and sig['fixture_sha256']==plan['fixture_sha256'] and sig['binary_sha256']==build['binary_sha256'] and sig['profile']==profile,'Wrong executed source/fixture/binary/profile')
    signatures.append({k:v for k,v in sig.items() if k not in ['profile','fixture_sha256']})
    loaded=[l.strip() for p in folder.glob('loader.*') for l in p.read_text().splitlines() if 'calling init:' in l and 'libGmatBase.so' in l]
    require(loaded==rec['actual_loaded_GmatBase'] and loaded and all('coverage95-20261110-de-delta-boundaries-v1/runtime-v1/lib/' in x for x in loaded),'Wrong actual loaded backend')
    proof=proof_from_log(folder/'test.log');require(rec['proofs']==[proof] and load(folder/'proof.json')==proof,'Actual scalar proof mismatch')
    raw=[float(x) for x in (folder/'results.csv').read_text().strip().split(',')];check_case(spec,proof,raw,(folder/'accepted.script').read_text());exposures+=trace(folder,profile);count+=1
 require(all(x==signatures[0] for x in signatures),'Mixed executed worker/backend versions')
 for profile,old in [('headless','GUI-headless-v1'),('visible','GUI-visible-v1')]:
  folder=reuse/old/'QtGui.NasaAchieveParameter';proof=proof_from_log(folder/'test.log');require(proof['checkpoint']=='achieve-scalar-parameter' and proof['outcome']=='passed' and proof['numeric_sentinel']==42,'Reused moving-control X witness failed')
  require({x['phase'] for x in proof['reports']}=={'variable','property'},'Missing reused moving-control type')
  for phase in proof['reports']:
   raw=[float(x) for x in (folder/(phase['phase']+'.csv')).read_text().strip().split(',')];require(raw==phase['values'] and abs(phase['achieved']-phase['desired'])<=phase['requested_tolerance']==1e-9 and raw[3]==42,'Reused independent moving-control residual/sentinel failed')
  exposures+=trace(folder,profile)
 return {'new_required_original_scalar_types':96,'reused_X_scalar_types':1,'complete_original_scalar_type_count':97,'additional_extension_scalar_types':2,'total_native_scalar_spellings':99,'new_GUI_scenarios':count,'reused_moving_control_GUI_scenarios':2,'new_independent_raw_scalars':588,'visible_native_exposures':exposures,'original_assertions_changed':0,'native_tests_rerun_for_admission':0,'new_scenarios_are_independently_satisfied_targets':True,'moving_control_convergence_reused':True}


def structure(q,scope,inventory):
 known={x['key']:x for x in inventory['requirements']}
 require(q['contract_kind']==KIND and q['qualified_keys']==KEYS and q['authority_sha256']==inventory['sources'],'Different/extra Achieve credit')
 require(scope['candidate_rows']==[known[k] for k in KEYS] and scope['complete_original_context']==[known[k] for k in CONTEXT],'Original obligation/context changed')
 require(scope['scalar_names_count']==97 and scope['new_scalar_names_count']==96 and scope['reused_scalar_names']==['X'] and scope['reused_child_keys']==['FRC!E94','FRC!E95','FRC!E96','FRC!E97'],'Incomplete type/child conjunction')
 require(scope['original_assertions_changed']==0 and scope['other_row_credit']==0 and scope['failures_waived'] is False and scope['newly_resolved_hold_keys']==[] and scope['generic_source_argument'],'Unsupported original/failure/hold change')
 require(all(q[k] is False and scope[k] is False for k in GATES),'Broader gate promoted')


def qualify(repo,path,inventory):
 q=load(path)
 for p,h in q['artifact_sha256'].items():require(sha(p)==h,'Changed Achieve evidence: '+p)
 scope=load(q['scope_review']);structure(q,scope,inventory);frozen={}
 for archive,identities in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
  require(q[archive] in q['artifact_sha256'],'Unbound source archive')
  with tarfile.open(q[archive]) as bundle:
   require(set(bundle.getnames())==set(q[identities]),'Changed source partition')
   for n,h in q[identities].items():
    raw=bundle.extractfile(n).read();require(not raw.startswith(b'\x7fELF') and hashlib.sha256(raw).hexdigest()==h,'Changed/non-source capsule member')
    if identities=='proof_source_sha256':frozen[n]=raw
 original=load(q['protected_input_baseline']);authorized=load(q['authorized_input_manifest']);guard=load(q['component_input_integrity']);require(len(original)==len(authorized)==1334 and all(q['proof_source_sha256'][n]==h for n,h in original.items()),'Original preimages lost');require(sorted(n for n in original if original[n]!=authorized[n])==['src/qtgui/tests/PlotIntegrationStepTests.cpp','src/qtgui/tests/SharedPlaybackTests.cpp'] and guard['authorized_inputs_final']==authorized and guard['changed_by_this_component']==[],'Unapproved original-input change')
 prior=load(q['prior_full_audit']);prior_rows={x['original_source']['key']:x for x in prior['rows']};require(len(prior['original_hold_keys'])==59 and not set(KEYS)&set(prior['original_hold_keys']),'Separate formal holds changed')
 for k in scope['reused_child_keys']:require(prior_rows[k]['status']=='verified-source-versioned','Child type not already admitted')
 fixture=json.loads(frozen['src/qtgui/tests/AchieveScalarFamilyCases.json']);original_fixture=json.loads(frozen['src/qtgui/tests/NasaConstraintScalarCases.json']);known_cases={x['key']:x for x in original_fixture['cases']};require(len(fixture['cases'])==96 and all(known_cases.get(x['key'])==x for x in fixture['cases']),'Independent original numerical cases/bounds changed')
 require({x['parameter_type'] for x in fixture['cases']}|{'X'}==set(original_fixture['unique_scalar_types']) and len(original_fixture['unique_scalar_types'])==97,'Missing original numeric factory name')
 legacy=json.loads(frozen['src/qtgui/tests/AchieveScalarLegacyCases.json']);require({x['parameter_type'] for x in legacy['cases']}=={'RLA','DLA'},'Missing legacy angle names')
 oldmap=load(q['scalar_inventory']);required={n for n,v in oldmap['parameter_types'].items() if v['real_var_ancestor']};allnames={x['parameter_type'] for x in fixture['cases']+legacy['cases']}|{'X'}
 require(len(required)==97 and required<=allnames and allnames-required=={'OutgoingRHA','OutgoingDHA'},'Old original97-name inventory not fully covered')
 for spec in legacy['cases']:
  elements=spec['oracle_recipe']['elements'];require(elements==[-14000,1.4,30,40,50,60] and spec['oracle_recipe']['mu']==398600.4415,'Legacy independent conic changed')
  e=elements[1];i,o,w=map(math.radians,elements[2:5]);v=math.acos(-1/e);c,s=math.cos,math.sin
  u=[c(o)*c(w)-s(o)*s(w)*c(i),s(o)*c(w)+c(o)*s(w)*c(i),s(w)*s(i)];z=[-c(o)*s(w)-s(o)*c(w)*c(i),-s(o)*s(w)+c(o)*c(w)*c(i),c(w)*s(i)];direction=[-s(v)*a+(e+c(v))*b for a,b in zip(u,z)];m=math.sqrt(sum(x*x for x in direction));truth=math.degrees(math.atan2(direction[1],direction[0])) if spec['parameter_type']=='RLA' else math.degrees(math.asin(direction[2]/m))
  require(abs(spec['expected']-truth)<=1e-12 and spec['absolute_tolerance']==2e-8 and spec['solver_control_tolerance']==2e-7,'Legacy outgoing-direction oracle/bounds changed')

 for k in ['physical_inputs','base_setup','mission_prefix']:require(fixture[k]==original_fixture[k],'Changed independent physical inputs/setup')
 for n,h in fixture['physical_inputs'].items():require(sha(repo/n)==h,'Changed actual physical input')
 worker=frozen['src/qtgui/tests/AchieveScalarFamilyTests.cpp'];require(hashlib.sha256(worker).hexdigest()==load(Path(q['component_stage'])/'build-worker-v2.json')['source_sha256'],'Wrong worker source version')
 for token in [b'GetRealParameter("AchievedValue")',b'source->undo()',b'source->redo()',b'window.saveScriptTo',b'window.loadScript',b'reportPropertyType',b'Goal boundary ambiguous',b'actual[2].toDouble()==42']:
  require(token in worker,'Missing actual execution/transaction assertion')
 facts=component(Path(q['component_stage']),Path(q['reuse_stage']),fixture,legacy);rejected=0
 def rejects(call):
  nonlocal rejected
  try:call()
  except (ValueError,KeyError):rejected+=1
  else:raise ValueError('Damaged Achieve admission accepted')
 for field,value in [('qualified_keys',KEYS+['FRR!E506']),('full_current_build_qualified',True)]:
  changed=copy.deepcopy(q);changed[field]=value;rejects(lambda:structure(changed,scope,inventory))
 for field,value in [('scalar_names_count',96),('failures_waived',True),('reused_child_keys',['FRC!E97'])]:
  changed=copy.deepcopy(scope);changed[field]=value;rejects(lambda:structure(q,changed,inventory))
 spec=fixture['cases'][0];folder=Path(q['component_stage'])/'headless-v2'/spec['key'];proof=proof_from_log(folder/'test.log');raw=[float(x) for x in (folder/'results.csv').read_text().strip().split(',')];script=(folder/'accepted.script').read_text()
 for field,value in [('native_achieved',proof['expected']+1),('invalid_picker_rejections',0),('parameter_type','MissingType'),('expected',proof['expected']+1)]:
  changed=copy.deepcopy(proof);changed[field]=value;rejects(lambda:check_case(spec,changed,raw,script))
 return q,KEYS,{'status':'passed','qualified_keys':KEYS,'whole_row_conjunction':True,'component':facts,'new_requirement_credit':1,'newly_resolved_original_hold_keys':[],'damaged_admission_controls_rejected':rejected,'native_tests_rerun':0,'runtime_binary_copies':0,**{k:False for k in GATES}}


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
    result = {'status':'passed', 'scope':'Sealed source-versioned prefix plus complete original Achieve input-type family',
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
                  'Only FRC E93 is added; all other rows and59 formal hold statuses remain open/unchanged.',
                  'Achieve scalar inputs, Variable, array element and opted-in numeric families are composed at their recorded source versions. Parameter physical truth, native OS input, platform and full-build approval remain separate.']}
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
