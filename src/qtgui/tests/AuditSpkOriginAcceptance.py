"""Admit only the complete original ephemeris-propagator settings conjunction."""
import argparse, copy, hashlib, json, math, tarfile
from pathlib import Path
import NasaGuiCoverage as nasa
import BuildNasaCoverage95 as builder
import BuildNasaCoverageExtension as common
load,sha,require=common.load,common.sha,common.require
KEYS=['FRR!E303','FRR!E305'];CONTEXT=['FRR!E'+str(i) for i in range(303,309)]
FIELDS=['StepSize','CentralBody','EpochFormat','StartEpoch']
ORIGINS=['Earth','Mars','Probe'];EPOCHS=['Spacecraft','Explicit','KernelStart']
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified']
KIND='source-versioned-complete-SPK-origin-step-start-settings'
CLAUSES={'complete-original-parent-and-setting-children','actual-origin-step-epoch-editor',
 'draft-type-discard-invalid-and-history-atomicity','owned-direct-and-runtime-clones',
 'independent-affine-ICRF-results-and-distinct-start-epochs','FK5-native-origin-operational-coherence',
 'paired-visible-profile-and-unmodified-family-regression','preserved-original-restoration-conflicts-and-witness-failures'}

def structure(q,s,inventory):
    known={x['key']:x for x in inventory['requirements']}
    require(q['contract_kind']==KIND and q['qualified_keys']==KEYS and q['authority_sha256']==inventory['sources'],'Extra/partial/unbound SPK credit')
    require(s['candidate_rows']==[known[k] for k in KEYS] and s['complete_original_context']==[known[k] for k in CONTEXT],'Original SPK parent/child context changed')
    require(s['required_fields']==FIELDS and s['origins']==ORIGINS and s['epoch_profiles']==EPOCHS and s['profiles']==['headless','visible'],'SPK setting universe narrowed')
    require(s['step_seconds']==75 and s['expected_end_seconds']=={'Spacecraft':195,'Explicit':315,'KernelStart':75},'SPK operational step/start semantics changed')
    require(set(s['clauses'])==CLAUSES and all(x['satisfied'] and x['evidence'] for x in s['clauses'].values()),'SPK conjunction incomplete')
    require(s['original_assertions_changed']==s['original_fixture_or_tolerance_changes']==s['other_row_credit']==s['runtime_binary_copies']==0
     and not s['original_readonly_conflicts_waived'] and not s['full_original_regression_passed'] and s['newly_resolved_hold_keys']==[],'SPK fidelity/hold scope changed')
    require(all(q[k] is False and s[k] is False for k in GATES),'Wider SPK approval promoted')

def damaged(q,s,inventory):
    rejected=[]
    for fault in ['extra-row','fingerprint','missing-child','missing-field','missing-origin','missing-epoch','missing-profile','wrong-step','waived-failure','wider-approval']:
        a,b=copy.deepcopy(q),copy.deepcopy(s)
        if fault=='extra-row':a['qualified_keys'].append('FRR!E304')
        elif fault=='fingerprint':b['candidate_rows'][0]['fingerprint']='0'*64
        elif fault=='missing-child':b['complete_original_context'].pop()
        elif fault=='missing-field':b['required_fields'].pop()
        elif fault=='missing-origin':b['origins'].pop()
        elif fault=='missing-epoch':b['epoch_profiles'].pop()
        elif fault=='missing-profile':b['profiles'].pop()
        elif fault=='wrong-step':b['step_seconds']=60
        elif fault=='waived-failure':b['original_readonly_conflicts_waived']=True
        else:a['full_NASA_qualified']=True
        try:structure(a,b,inventory)
        except (ValueError,AssertionError):rejected.append(fault)
        else:raise ValueError('Damaged SPK admission accepted: '+fault)
    return rejected

def component(stage):
    pub=load(stage/'publication-v1.json')
    for n,h in pub['artifacts'].items():require(sha(stage/n)==h,'Changed SPK artifact: '+n)
    guard=load(stage/'matrix-inputs-before-v1.json')
    require(guard==load(stage/'matrix-inputs-after-v1.json')==load(stage/'pilot-correct-frame-v4-inputs-before.json')==load(stage/'pilot-correct-frame-v4-inputs-after.json')
     and load(stage/'matrix-input-guard-v1.json')['identical'],'SPK source/runtime drifted')
    require(any('libGmatBase.so.' in n for n in guard['binaries']) and any('libEphemPropagator.so.' in n for n in guard['binaries']),'SPK core/plugin identity missing')
    runs=[load(stage/(n+'/results.json')) for n in ['pilot-correct-frame-v4','remaining-headless-v4','visible-matrix-v4']]
    require(all(x['source_hashes']==runs[0]['source_hashes'] and x['binary_hashes']==runs[0]['binary_hashes'] for x in runs),'SPK profile inventories differ')
    census={'QtGui.SpkOriginAcceptance'+body+epoch:(body,epoch) for body in ORIGINS for epoch in EPOCHS};family='QtGui.NasaSpkDraftFamilies';conflicts={'QtGui.NasaSpkDraftCreate','QtGui.NasaSpkDraftEdit'}
    pilot='QtGui.SpkOriginAcceptanceMarsExplicit';selections=[{pilot},(set(census)-{pilot})|{family}|conflicts,set(census)|{family}|conflicts]
    vectors=0;coherence=0;effect=0;exposures=0;max_position=0.;max_velocity=0.;seen={'headless':set(),'visible':set()}
    for index,(run,expected) in enumerate(zip(runs,selections)):
        mode='visible' if index==2 else 'headless';rows=[x for x in run['results'] if x['status']!='not-run']
        require(run['end_reason']=='completed-selection' and len(rows)==len(expected) and {x['name'] for x in rows}==expected,'SPK case selection incomplete')
        if mode=='visible':
            d=run['visible_display'];require(d['start_minimized'] and d['resizable'] and not d['resize_errors'] and 'Iconic' in d['initial_host_window_state'] and 'FULLSCREEN' not in d['initial_host_window_state'],'Visible SPK desktop policy changed')
        for row in rows:
            name=row['name'];work=Path(row['log']).parent
            require(row['requested_visibility']==mode and row['trace']['instrumented_processes']==1 and not row['trace']['screenshots'],'SPK visibility/trace lost')
            if name in conflicts:
                text=Path(row['log']).read_text();require(row['status']=='failed' and row['returncode']==8 and 'Test process exit: 1 (0x00000001)' in text and 'Inactive SPK Origin is exposed as an effective editable setting' in text,'Original SPK readonly conflict concealed/changed');continue
            require(row['status']=='passed' and row['returncode']==0,'Required SPK case failed')
            if mode=='visible':require(row['trace']['native_exposure_events']>0,'Visible SPK case never exposed');exposures+=row['trace']['native_exposure_events']
            if name==family:
                proof=nasa.proof_records(row);require(len(proof)==1 and proof[0]['outcome']=='passed' and proof[0]['checkpoint']=='spk-draft.Families','Original numerical-family regression lost');continue
            require(name not in seen[mode],'Duplicate substituted for SPK case');seen[mode].add(name);origin,epoch=census[name];d=load(work/'proof.json')
            require(d['outcome']=='passed' and d['origin']==origin and d['epoch_profile']==epoch and d['required_fields']==FIELDS and d['step_seconds']==75
             and d['invalid_origin_rejections']==3 and d['new_requirement_credit']==0,'SPK field/origin/epoch/invalid partition differs')
            require(all(d[k] for k in ['default_Earth_retained','type_draft_Discard','Apply_Undo_Redo_Unicode','nested_direct_runtime_clones']),'SPK lifecycle/owner conjunction incomplete')
            require({'Resources','resource_Type','resource_CentralBody','resource_StepSize','resource_EpochFormat','resource_StartEpoch'}<=set(row['trace']['named_objects']),'Actual SPK editor fields unobserved')
            t={'Spacecraft':195,'Explicit':315,'KernelStart':75}[epoch];require(d['expected_seconds']==t and abs(d['observed_TDB_seconds']-t)<2e-5,'Distinct operational step/start epoch failed')
            require(d['start_epoch']==('FromSpacecraft' if epoch=='Spacecraft' else 'EphemStart' if epoch=='KernelStart' else format(21545+240/86400,'.17g')),'Wrong SPK epoch spelling/mode')
            values=[float(x) for x in (work/'state.csv').read_text().strip().split(',')];require(len(values)==7 and values[0]==d['observed_A1Mjd'] and values[1:]==d['state'],'SPK raw CSV/proof differs')
            truth=[7000+t,-100+2*t,200-.5*t,1,2,-.5]
            require(len(d['state'])==len(d['expected_state'])==len(d['native_relative_state'])==len(d['native_J2000_FK5_state'])==len(d['shared_native_origin_offset'])==6,'SPK Cartesian state narrowed')
            for i,(actual,target) in enumerate(zip(d['state'],truth)):
                error=abs(actual-target);require(math.isfinite(actual) and error<(2e-5 if i<3 else 2e-9),'Independent nominal affine ICRF result differs')
                require(abs(d['native_relative_state'][i]+d['shared_native_origin_offset'][i]-d['native_J2000_FK5_state'][i])<(2e-5 if i<3 else 2e-9),'Same-frame native origin coherence failed')
                if i<3:max_position=max(max_position,error)
                else:max_velocity=max(max_velocity,error)
                coherence+=1
            if origin!='Earth':require(sum(abs(x-y) for x,y in zip(d['native_relative_state'][:3],d['native_J2000_FK5_state'][:3]))>1,'Non-Earth origin had no operational effect');effect+=1
            require((work/'affine-origin.bsp').stat().st_size>0 and (work/'initial.script').read_text().endswith('% Preserve tail Ω\n') and (work/'applied.script').read_text().endswith('% Preserve tail Ω\n'),'SPK kernel/input/tail evidence lost')
            vectors+=1
    require(seen=={'headless':set(census),'visible':set(census)} and vectors==18 and coherence==108 and effect==12,'SPK matrix narrowed')
    failed=load(stage/'retained-new-witness-frame-failures-v1.json');require(failed['product_changes_for_correction']==failed['original_assertion_or_fixture_changes']==failed['numerical_bound_changes']==0,'Witness frame correction scope changed')
    for name in failed['retained_failed_runs']:
        r=load(stage/(name+'/results.json'));rows=[x for x in r['results'] if x['status']!='not-run'];require(len(rows)==1 and rows[0]['status']=='failed' and 'Native SPK vector' in Path(rows[0]['log']).read_text(),'Earlier witness failure concealed')
    return dict(registered_positive_passes=20,original_readonly_conflicts=4,new_GUI_profile_passes=18,unchanged_original_family_passes=2,independent_state_vectors=vectors,independent_state_scalar_comparisons=108,native_same_frame_coherence_comparisons=coherence,nonEarth_operational_effect_cases=effect,native_visible_exposures=exposures,max_position_error_km=max_position,max_velocity_error_km_s=max_velocity,retained_new_witness_frame_failures=3,full_original_regression_passed=False)

def qualify(repo,path,inventory):
    q=load(path)
    for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed SPK admission artifact: '+n)
    s=load(q['scope_review']);structure(q,s,inventory)
    for archive,ids in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
        require(q[archive] in q['artifact_sha256'],'Unbound SPK source archive')
        with tarfile.open(q[archive]) as bundle:
            require(set(bundle.getnames())==set(q[ids]),'SPK source partition differs')
            for n,h in q[ids].items():require(hashlib.sha256(bundle.extractfile(n).read()).hexdigest()==h,'SPK source bytes differ')
    original=load(q['protected_input_baseline']);require(len(original)==1334 and all(q['proof_source_sha256'].get(n)==h for n,h in original.items()),'Protected original inputs omitted')
    prior=load(q['prior_overlay']);rows={x['key']:x for x in prior['reviewed_source_rows']};require(len(prior['original_hold_keys_retained'])==59 and not set(KEYS)&set(prior['original_hold_keys_retained']) and all(rows[k]['whole_row_status']=='open' for k in KEYS),'SPK gap/hold inventory changed')
    full={x['original_source']['key']:x for x in load(q['prior_full_audit'])['rows']};require(all(full[k]['status']=='verified-source-versioned' for k in ['FRR!E304','FRR!E306','FRR!E307','FRR!E308']),'SPK step/epoch child dependencies unqualified')
    proof=component(Path(q['component_stage']));require(len(damaged(q,s,inventory))==10,'Damaged SPK controls incomplete')
    return q,KEYS,dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,component=proof,newly_resolved_original_hold_keys=[],new_requirement_credit=2,damaged_admission_controls_rejected=10,native_tests_rerun=0,runtime_binary_copies=0,**{k:False for k in GATES})

def build(repo,baseline,path,output):
    seal,inventory,keys=builder.baseline(repo,baseline);original={k:set(v) for k,v in keys.items()}
    _,incoming,audit=qualify(repo,path,inventory);known={k:{r['key']:r for r in inventory[k]} for k in keys}
    require(not set(incoming)&keys['requirements'],'Duplicate SPK settings credit')
    keys['requirements']=common.merge_keys(keys['requirements'],incoming,known['requirements'])
    coverage={k:dict(verified=len(keys[k]),total=len(known[k]),percent=round(100*len(keys[k])/len(known[k]),1),new_verified=len(keys[k])-len(original[k])) for k in keys}
    remaining={k:[r for r in seal['remaining_original_rows'][k] if r['key'] not in keys[k]] for k in keys}
    result=dict(status='passed',scope='95-percent campaign: sealed source-versioned prefix plus complete original SPK origin/step/start settings conjunction.',
        baseline=str(baseline),baseline_sha256=sha(baseline),authority_sha256=inventory['sources'],coverage=coverage,
        target_requirements=1101,target_reached=len(keys['requirements'])>=1101,requirements_still_needed=max(0,1101-len(keys['requirements'])),
        additions=[dict(key=k,kind='requirements',fingerprint=known['requirements'][k]['fingerprint'],source_text=known['requirements'][k]['text'],qualification=str(path),qualification_sha256=sha(path),status='verified-source-versioned') for k in incoming],
        remaining_counts={k:len(v) for k,v in remaining.items()},remaining_original_rows=remaining,
        technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,
        limits=['Original 1158/2367 denominators and source row identities are unchanged.','Two original open SPK setting rows are implemented and proved; all59 separate original hold identities and statuses remain unchanged.','Scientific, physical ephemeris/frame, external ABI, platform/native OS input and full current-build approvals remain separate.'])
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
