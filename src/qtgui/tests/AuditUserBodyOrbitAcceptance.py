"""Admit only the original user-body TwoBody/orbit-property conjunction."""
import argparse
import copy
import functools
import hashlib
import json
import math
from pathlib import Path
import tarfile

import NasaGuiCoverage as nasa
import BuildNasaCoverage95 as builder
import BuildNasaCoverageExtension as common

load,sha,require=common.load,common.sha,common.require
KEYS=['FRR!E220']+['FRR!E'+str(i) for i in range(228,237)]+['FRR!E238']
TYPES=['Moon','Asteroid','Comet','Planet']
KIND='source-versioned-complete-user-body-two-body-orbit-properties'
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified']
CLAUSES={'original-types-models-and-full-property-context','all-seven-GUI-orbit-fields',
         'both-modes-and-committed-SPICE-restoration','independent-Kepler-states-both-time-interfaces',
         'invalid-edit-atomicity-and-persistence','SPK-kernel-and-central-parent-conjunction',
         'nested-parent-composition','paired-native-visible-profile','implemented-restoration-and-input-fidelity'}


def structure(q,scope,inventory):
    known={r['key']:r for r in inventory['requirements']}
    require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Extra/partial user-body credit')
    require(q['authority_sha256']==inventory['sources'],'Original authority changed')
    require(scope['candidate_rows']==[known[k] for k in KEYS],'Original body row text/fingerprint changed')
    require(scope['original_body_types']==[known['FRR!E'+str(i)] for i in range(213,218)],'Original body universe narrowed')
    require(scope['original_model_context']==[known['FRR!E'+str(i)] for i in range(218,221)],'Original model parent omitted')
    require(scope['original_property_context']==[known['FRR!E'+str(i)] for i in range(228,241)],'SPICE/kernel/central-body parent clauses omitted')
    require(scope['mandatory_types']==TYPES and scope['orbit_fields']==['InitialEpoch','SMA','ECC','INC','RAAN','AOP','TA']
            and scope['modes']==['SPICE','TwoBodyPropagation'],'Required type/field/mode omitted')
    require(scope['newly_resolved_hold_keys']==KEYS and scope['implemented_restoration'] and not scope['administrative_retirement'],
            'Held body requirements retired instead of implemented')
    require(set(scope['clauses'])==CLAUSES and all(v['satisfied'] and v['evidence'] for v in scope['clauses'].values()),
            'Incomplete body conjunction')
    require(scope['original_assertions_changed']==0 and scope['failed_runs_retained'] and scope['runtime_binary_copies']==0
            and scope['whole_row_qualified'] and scope['other_row_credit']==0,'Body input/failure/credit scope changed')
    require(all(scope[k] is False and q[k] is False for k in GATES),'Wider body approval promoted')


def negative_controls(q,scope,inventory):
    rejected=[]
    for fault in ['extra-row','fingerprint','missing-type','missing-parent-property','missing-mode',
                  'missing-field','administrative-retirement','missing-clause','changed-assertion','wider-approval']:
        bq,bs=copy.deepcopy(q),copy.deepcopy(scope)
        if fault=='extra-row':bq['qualified_keys'].append('FRR!E227')
        elif fault=='fingerprint':bs['candidate_rows'][0]['fingerprint']='0'*64
        elif fault=='missing-type':bs['mandatory_types'].pop()
        elif fault=='missing-parent-property':bs['original_property_context'].pop()
        elif fault=='missing-mode':bs['modes'].pop()
        elif fault=='missing-field':bs['orbit_fields'].pop()
        elif fault=='administrative-retirement':bs['administrative_retirement']=True
        elif fault=='missing-clause':bs['clauses'].pop(next(iter(CLAUSES)))
        elif fault=='changed-assertion':bs['original_assertions_changed']=1
        else:bq['technical_core_approval']=True
        try:structure(bq,bs,inventory)
        except (ValueError,AssertionError):rejected.append(fault)
        else:raise ValueError('Damaged body admission accepted: '+fault)
    return rejected


def initial(elements,mu,advance=0):
    a,e,inc,node,peri,ta=elements
    inc,node,peri,ta=map(math.radians,[inc,node,peri,ta+advance])
    p=a*(1-e*e);radius=p/(1+e*math.cos(ta));speed=math.sqrt(mu/p)
    raw=[radius*math.cos(ta),radius*math.sin(ta),0.,-speed*math.sin(ta),speed*(e+math.cos(ta)),0.]
    c,s,w,v,ci,si=math.cos(node),math.sin(node),math.cos(peri),math.sin(peri),math.cos(inc),math.sin(inc)
    rotation=[[c*w-s*v*ci,-c*v-s*w*ci,s*si],[s*w+c*v*ci,-s*v+c*w*ci,-c*si],[v*si,w*si,ci]]
    return [sum(rotation[row][col]*raw[block*3+col] for col in range(3)) for block in range(2) for row in range(3)]


@functools.lru_cache(maxsize=None)
def oracle(elements,mu,duration):
    if elements[1]==0:
        period=2*math.pi*math.sqrt(elements[0]**3/mu)
        return initial(elements,mu,360*duration/period)
    state=initial(elements,mu);steps=max(1,math.ceil(abs(duration)/.25));h=duration/steps
    def derivative(y):
        radius=math.hypot(*y[:3]);scale=-mu/radius**3
        return y[3:]+[scale*x for x in y[:3]]
    def add(y,k,scale):return [a+scale*b for a,b in zip(y,k)]
    for _ in range(steps):
        a=derivative(state);b=derivative(add(state,a,h/2));c=derivative(add(state,b,h/2));d=derivative(add(state,c,h))
        state=[x+h*(aa+2*bb+2*cc+dd)/6 for x,aa,bb,cc,dd in zip(state,a,b,c,d)]
    return state


def component(q):
    stage=Path(q['component_stage']);publication=load(stage/'publication-v1.json')
    for n,h in publication['artifacts'].items():require(sha(n)==h,'Changed body component artifact: '+n)
    profiles=[load(stage/'headless-v5/results.json'),load(stage/'visible-v5/results.json')]
    require(profiles[0]['source_hashes']==profiles[1]['source_hashes'] and profiles[0]['binary_hashes']==profiles[1]['binary_hashes'],
            'Body profile sources/runtime differ')
    body_cases=TYPES+['NestedMoon'];parents=['EarthToMoon','SunToPlanet','SunToAsteroid','SunToComet','UserMoonToMoon']
    names={'QtGui.UserBodyOrbit'+x for x in body_cases}|{'QtGui.NasaUserBodySpk'+x for x in TYPES}|{'QtGui.NativeNasaBodyParentCreation'+x for x in parents}
    queries=0;exposures=0;max_position=0.;max_velocity=0.
    for index,profile in enumerate(profiles):
        actual=[x for x in profile['results'] if x['status']!='not-run']
        require(len(actual)==14 and {x['name'] for x in actual}==names and
                all(x['status']=='passed' and x['returncode']==0 for x in actual),'Body profile incomplete')
        require(profile['end_reason']=='completed-selection','Body profile still running')
        if index:
            display=profile['visible_display']
            require(display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and
                    'FULLSCREEN' not in display['initial_host_window_state'] and display['resizable'] and not display['resize_errors'],
                    'Visible body profile not minimized/resizable/non-fullscreen')
        for case in actual:
            directory=Path(case['log']).parent;name=case['name']
            require(case['requested_visibility']==('visible' if index else 'headless') and
                    case['trace']['instrumented_processes']==1 and not case['trace']['screenshots'],'Body trace/profile lost')
            if index:
                require(case['trace']['native_exposure_events']>0,'Visible body case did not expose natively')
                exposures+=case['trace']['native_exposure_events']
            proof=load(directory/'proof.json')
            if name.startswith('QtGui.UserBodyOrbit'):
                selected=name.removeprefix('QtGui.UserBodyOrbit');nested=selected=='NestedMoon';kind='Moon' if nested else selected
                require(proof['case']==selected and proof['type']==kind and proof['status']=='passed' and proof['orbit_fields']==7
                        and proof['invalid_GUI_rejections']==7 and proof['mission_witness']==42 and proof['nested_parent']==nested,
                        'Body type/field/rejection/witness partition changed')
                require(all(proof[k] for k in ['actual_tree_editor','required_modes_available','committed_SPICE_return',
                        'native_SPICE_state_restored','draft_mode_round_trip','cancel_discard_atomic','apply_undo_redo_idempotent',
                        'unicode_persistence','precision_and_Real_queries']) and not proof['native_OS_input_qualified'],
                        'Missing actual body GUI/state lifecycle proof')
                require({'Resources','body_PosVelSource','body_InitialEpoch','body_SMA','body_ECC','body_INC','body_RAAN','body_AOP','body_TA','body_CentralBody'} <=
                        set(case['trace']['named_objects']),'Body widgets were not actually observed')
                phases=load(directory/'states.json');require(len(phases)==3,'Missing conic phase')
                for phase in phases:
                    label=phase['phase'];mu=126. if nested else 398600.4415+63
                    if label=='ellipse':elements=(24000.,.4,67.,123.,87.,55.);epoch=21546.;times=[0.,600.,-400.,1200.]
                    elif label=='hyperbola':elements=(-30000.,1.4,47.,112.,38.,-20.);epoch=21545.;times=[0.,600.,-400.,1200.]
                    else:
                        require(label=='circular-multiple-revolution','Unknown conic phase')
                        inclination={'Moon':0.,'Asteroid':180.,'Comet':90.,'Planet':31.}[kind]
                        elements=(20000.,0.,inclination,233.,17.,40.);epoch=21545.
                        period=2*math.pi*math.sqrt(elements[0]**3/mu);times=[0.,period/4,-period/4,30*period+37.25,-30*period-37.25]
                    require(tuple(phase['elements'])==elements and phase['epoch']==epoch and phase['mu']==mu and
                            len(phase['states'])==len(times),'Conic input universe or sample count narrowed')
                    for row,dt in zip(phase['states'],times):
                        require(abs(row['seconds']-dt)<=1e-6,'Query epoch omitted/replaced')
                        expected=oracle(elements,mu,row['seconds'])
                        require(len(row['expected'])==6 and all(abs(a-b)<=1e-9 for a,b in zip(row['expected'],expected)),
                                'Retained expected states differ from offline Cartesian/circular oracle')
                        for key in ['actual','actual_A1']:
                            require(len(row[key])==6,'Missing time-interface state')
                            for c,(a,b) in enumerate(zip(row[key],expected)):
                                error=abs(a-b);require(math.isfinite(a) and error<=(2e-6 if c<3 else 2e-9),'Body state fails independent offline oracle')
                                if c<3:max_position=max(max_position,error)
                                else:max_velocity=max(max_velocity,error)
                        queries+=1
                restoration=load(directory/'mode-restoration.json')
                require(len(restoration['initial_SPICE'])==len(restoration['restored_SPICE'])==6 and
                        all(abs(a-b)<=1e-9 for a,b in zip(restoration['initial_SPICE'],restoration['restored_SPICE'])),
                        'Committed SPICE return changed kernel state')
            elif name.startswith('QtGui.NasaUserBodySpk'):
                require(proof['outcome']=='passed' and proof['type']==name.removeprefix('QtGui.NasaUserBodySpk') and
                        proof['spk_type']==9 and proof['report_values']==144 and proof['analytic_state_components']==108 and
                        proof['invalid_rejections']==2 and proof['max_position_error_km']<1e-7 and
                        proof['max_velocity_error_km_per_s']<1e-10,'Original SPK/kernel numerical conjunction lost')
            else:
                require(proof['outcome']=='passed' and proof['case']==name.removeprefix('QtGui.NativeNasaBodyParentCreation') and
                        proof['kernel_picker_visits']==2 and proof['sentinel']==42,'Original selected-parent creation proof lost')
    require(queries==130,'Body numerical query count differs')
    failures=load(stage/'retained-failures-v1.json')
    require(len(failures['registered_failures'])==4 and failures['original_assertion_changes']==0 and not failures['tolerances_changed'],
            'Original-input fidelity or failed body runs omitted')
    return dict(passed_profile_cases=28,original_types=4,new_body_profile_cases=10,original_regression_profile_cases=18,
                numerical_queries=queries,time_interface_vectors=queries*2,Cartesian_scalar_comparisons=queries*12,
                max_position_error_km=max_position,max_velocity_error_km_s=max_velocity,native_visible_exposures=exposures)


def qualify(repo,path,inventory):
    q=load(path)
    for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed body admission artifact: '+n)
    scope=load(q['scope_review']);structure(q,scope,inventory)
    for archive_key,ids_key in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
        require(q[archive_key] in q['artifact_sha256'],'Unbound body source archive')
        with tarfile.open(q[archive_key]) as bundle:
            require(set(bundle.getnames())==set(q[ids_key]),'Body frozen source partition differs')
            for n,h in q[ids_key].items():require(hashlib.sha256(bundle.extractfile(n).read()).hexdigest()==h,'Frozen body source changed')
    original=load(q['protected_input_baseline']);require(len(original)==1334 and all(q['proof_source_sha256'].get(n)==h for n,h in original.items()),'Protected original tests omitted')
    previous=load(q['prior_overlay']);require(len(previous['original_hold_keys_retained'])==59 and set(KEYS)<=set(previous['unresolved_original_hold_keys']),
            'Original body hold identities/dispositions were already dropped or resolved')
    full=load(q['prior_full_audit']);rows={x['original_source']['key']:x for x in full['rows']}
    require(all(rows[k]['status']=='verified-source-versioned' for k in ['FRR!E237','FRR!E239','FRR!E240']),
            'Existing parent SPICE/kernel/central-body dependency is unqualified')
    proof=component(q);require(len(negative_controls(q,scope,inventory))==10,'Damaged body controls incomplete')
    result=dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,component=proof,
                newly_resolved_original_hold_keys=KEYS,new_requirement_credit=11,damaged_admission_controls_rejected=10,
                native_tests_rerun=0,runtime_binary_copies=0,**{k:False for k in GATES})
    return q,KEYS,result


def build(repo,baseline,path,output):
    seal,inventory,keys=builder.baseline(repo,baseline);original={k:set(v) for k,v in keys.items()}
    _,incoming,audit=qualify(repo,path,inventory);known={k:{r['key']:r for r in inventory[k]} for k in keys}
    require(not set(incoming)&keys['requirements'],'Duplicate user-body credit')
    keys['requirements']=common.merge_keys(keys['requirements'],incoming,known['requirements'])
    coverage={k:dict(verified=len(keys[k]),total=len(known[k]),percent=round(100*len(keys[k])/len(known[k]),1),new_verified=len(keys[k])-len(original[k])) for k in keys}
    remaining={k:[r for r in seal['remaining_original_rows'][k] if r['key'] not in keys[k]] for k in keys}
    result=dict(status='passed',scope='95-percent campaign: sealed source-versioned prefix plus complete original user-body TwoBody/orbit-property conjunction.',
        baseline=str(baseline),baseline_sha256=sha(baseline),authority_sha256=inventory['sources'],coverage=coverage,
        target_requirements=1101,target_reached=len(keys['requirements'])>=1101,requirements_still_needed=max(0,1101-len(keys['requirements'])),
        additions=[dict(key=k,kind='requirements',fingerprint=known['requirements'][k]['fingerprint'],source_text=known['requirements'][k]['text'],qualification=str(path),qualification_sha256=sha(path),status='verified-source-versioned') for k in incoming],
        remaining_counts={k:len(v) for k,v in remaining.items()},remaining_original_rows=remaining,
        technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,
        limits=['Original 1158/2367 denominators and source row identities are unchanged.','Eleven original held rows are implemented and proved; their original hold identities remain and only their current resolution changes.','Scientific, physical ephemeris/frame, external ABI, platform/native OS input and full current-build approvals remain separate.'])
    with Path(output).open('x') as f:json.dump(result,f,indent=2);f.write('\n')
    return result,audit


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);p.add_argument('--baseline',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();repo=a.repo.resolve()
    if a.baseline:
        result,_=build(repo,a.baseline,a.qualification,a.output);print(json.dumps({'coverage':result['coverage'],'requirements_still_needed':result['requirements_still_needed']}))
    else:
        _,_,result=qualify(repo,a.qualification,nasa.inventory(repo))
        with a.output.open('x') as f:json.dump(result,f,indent=2);f.write('\n')
        print(json.dumps(result))
