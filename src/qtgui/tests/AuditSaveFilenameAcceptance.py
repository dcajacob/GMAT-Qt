"""Admit only the original default ObjectName.ObjectType Save filename rule."""
import argparse, copy, hashlib, json, math, re, tarfile
from pathlib import Path
import NasaGuiCoverage as nasa
import BuildNasaCoverage95 as builder
import BuildNasaCoverageExtension as common
load,sha,require=common.load,common.sha,common.require
KEYS=['FRC!E135'];CONTEXT=['FRC!E'+str(i) for i in range(132,136)]
STYLES=['Worksheet','LegacyData'];SIZES=['Single','Multi']
KIND='source-versioned-complete-default-Save-filename-rule'
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified']
CLAUSES={'exact-original-filename-condition-and-context','worksheet-native-and-GUI-default',
 'actual-command-selection-mode-and-blank-filename','draft-discard-history-Unicode-and-clone',
 'single-multiple-per-object-and-legacy-filenames','actual-snapshot-values-and-successor',
 'paired-visible-profile-and-stable-runtime','preserved-original-failures-and-universal-Save-gap'}

def structure(q,s,inventory):
    known={x['key']:x for x in inventory['requirements']}
    require(q['contract_kind']==KIND and q['qualified_keys']==KEYS and q['authority_sha256']==inventory['sources'],'Extra/partial/unbound Save credit')
    require(s['candidate_rows']==[known[k] for k in KEYS] and s['complete_original_context']==[known[k] for k in CONTEXT],'Original Save obligation/context changed')
    require(s['styles']==STYLES and s['selection_sizes']==SIZES and s['profiles']==['headless','visible'] and s['default_style']=='Worksheet' and s['explicit_filename']=='','Save default/mode/condition narrowed')
    require(set(s['clauses'])==CLAUSES and all(x['satisfied'] and x['evidence'] for x in s['clauses'].values()),'Save filename conjunction incomplete')
    require(s['original_assertions_changed']==s['original_fixture_or_tolerance_changes']==s['other_row_credit']==s['runtime_binary_copies']==0
     and s['retained_original_conflicts']==5 and not s['original_failures_waived'] and not s['full_original_regression_passed']
     and not s['universal_any_Resource_Save_qualified'] and s['newly_resolved_hold_keys']==[],'Save fidelity/scope changed')
    require(all(q[k] is False and s[k] is False for k in GATES),'Wider Save approval promoted')

def damaged(q,s,inventory):
    rejected=[]
    for fault in ['extra-row','fingerprint','missing-context','missing-style','missing-selection','missing-profile','wrong-default','explicit-name','waived-failure','universal-Save']:
        a,b=copy.deepcopy(q),copy.deepcopy(s)
        if fault=='extra-row':a['qualified_keys'].append('FRC!E132')
        elif fault=='fingerprint':b['candidate_rows'][0]['fingerprint']='0'*64
        elif fault=='missing-context':b['complete_original_context'].pop()
        elif fault=='missing-style':b['styles'].pop()
        elif fault=='missing-selection':b['selection_sizes'].pop()
        elif fault=='missing-profile':b['profiles'].pop()
        elif fault=='wrong-default':b['default_style']='LegacyData'
        elif fault=='explicit-name':b['explicit_filename']='snapshot.txt'
        elif fault=='waived-failure':b['original_failures_waived']=True
        else:b['universal_any_Resource_Save_qualified']=True
        try:structure(a,b,inventory)
        except (ValueError,AssertionError):rejected.append(fault)
        else:raise ValueError('Damaged Save admission accepted: '+fault)
    return rejected

def component(stage):
    for n,h in load(stage/'publication-v1.json')['artifacts'].items():require(sha(stage/n)==h,'Changed Save artifact: '+n)
    guard=load(stage/'matrix-inputs-before-v2.json');require(guard==load(stage/'matrix-inputs-after-v2.json') and load(stage/'matrix-input-guard-v2.json')['identical'],'Save source/runtime drifted')
    require(any('SaveCommand' in n and '.so.' in n for n in guard['binaries']),'Save plugin identity absent')
    profiles=[load(stage/(mode+'-v2/results.json')) for mode in ['headless','visible']]
    require(profiles[0]['source_hashes']==profiles[1]['source_hashes'] and profiles[0]['binary_hashes']==profiles[1]['binary_hashes'],'Save profiles differ')
    cases={'QtGui.SaveFilenameAcceptance'+style+size:(style,size) for style in STYLES for size in SIZES};exposures=0;outputs=0;selected=0
    for mode,run in zip(['headless','visible'],profiles):
        rows=[x for x in run['results'] if x['status']!='not-run'];require(run['end_reason']=='completed-selection' and len(rows)==4 and {x['name'] for x in rows}==set(cases),'Save case universe incomplete')
        if mode=='visible':
            d=run['visible_display'];require(d['start_minimized'] and d['resizable'] and not d['resize_errors'] and 'Iconic' in d['initial_host_window_state'] and 'FULLSCREEN' not in d['initial_host_window_state'],'Visible Save desktop policy changed')
        for row in rows:
            require(row['status']=='passed' and row['returncode']==0 and row['requested_visibility']==mode and row['trace']['instrumented_processes']==1 and not row['trace']['screenshots'],'Required Save case failed/trace lost')
            if mode=='visible':require(row['trace']['native_exposure_events']>0,'Visible Save case never exposed');exposures+=row['trace']['native_exposure_events']
            work=Path(row['log']).parent;d=load(work/'proof.json');style,size=cases[row['name']];objects=['Saved'] if size=='Single' else ['Saved','Grid','Craft'];types={'Saved':'Variable','Grid':'Array','Craft':'Spacecraft'}
            names=sorted([name+'.'+types[name] for name in objects] if style=='Worksheet' else ['Saved.Variable.data' if size=='Single' else 'Saved_Grid_Craft.data'])
            require(d['outcome']=='passed' and d['filename_style']==style and d['selection_size']==size and d['selected_objects']==objects and d['explicit_filename']==''
             and d['saved_scalar']==19 and d['successor_scalar']==42 and d['new_requirement_credit']==0,'Save style/selection/value/condition changed')
            require(all(d[k] for k in ['worksheet_default','selector_and_typed_order','explicit_filename_disables_style_and_retains_draft','draft_Discard_Apply_Undo_Redo_Unicode','native_clone_settings']),'Actual Save lifecycle conjunction incomplete')
            require({'Mission','commandField_Objects','commandChoose_Objects','commandSaveFilenameStyle','commandSaveFilename','commandObjectList'}<=set(row['trace']['named_objects']),'Actual Save controls unobserved')
            require([x['filename'] for x in d['outputs']]==names,'Wrong default/legacy filename projection')
            text=''
            for output in d['outputs']:
                p=work/output['filename'];require(p.stat().st_size==output['bytes'] and output['bytes']>0,'Save payload size/content binding differs');text+=p.read_text();outputs+=1
            m=re.search(r'(?:^|\n)(?:GMAT )?Saved\s*=\s*([^;\n]+);',text);require(m and float(m[1])==19 and not re.search(r'(?:^|\n)(?:GMAT )?Unused\s*=',text),'Saved scalar/selected membership differs')
            if size=='Multi':
                require('Create Array Grid' in text and '1.25' in text and 'Create Spacecraft Craft' in text,'Selected resource types/array value missing')
                m=re.search(r'(?:^|\n)(?:GMAT )?Craft\.X\s*=\s*([^;\n]+);',text);require(m and math.isfinite(float(m[1])) and abs(float(m[1])-7000)<1e-8,'Selected spacecraft scalar differs')
            require((work/'initial.script').read_text().endswith('% Preserve filename tail Δ\n') and (work/'applied.script').read_text().endswith('% Preserve filename tail Δ\n'),'Save source/tail evidence lost');selected+=len(objects)
    require(outputs==12 and selected==16,'Save output/selection universe narrowed')
    original=load(stage/'retained-original-five-conflicts-v1.json');require(original['original_failures_recovered']==5 and original['current_native_reruns']==0 and original['original_assertion_changes']==0 and not original['failure_waived'] and not original['full_original_regression_passed'],'Original Save conflict scope changed')
    for n,h in original['artifact_sha256'].items():require(sha(n)==h,'Changed retained original Save conflict: '+n)
    expected={'QtGui.NativeNasaCommandTreeSaveRoot','QtGui.NativeNasaCommandTreeSaveBoundary','QtGui.NativeNasaCommandTreeSaveInterior','QtGui.NasaCommandOutputOperationsSaveRoot','QtGui.NasaCommandOutputOperationsSaveBranch'}
    require({x['name'] for x in original['records']}==expected,'Original Save failure identities lost')
    for r in original['records']:
        text=Path(r['test_log']).read_text();require(('saved_quantities' in text or 'Operation input absent' in text) and r['historical_signature_is_not_the_authoritative_failure_text'] and not r['current_rerun'],'Actual original Save failure log lost')
    failed=[x for x in load(stage/'visible-v1/results.json')['results'] if x['status']=='failed'];require(len(failed)==4 and all('Pending Save window absent' in Path(x['log']).read_text() for x in failed),'New witness owner failures concealed')
    correction=load(stage/'retained-visible-owner-failures-v1.json');require(correction['product_changes']==correction['original_assertion_or_fixture_changes']==correction['filename_expectation_changes']==0,'Save witness correction changed scope')
    return dict(registered_profile_passes=8,native_output_files=outputs,selected_root_occurrences=selected,independent_saved_scalar_checks=8,successor_scalar_checks=8,native_visible_exposures=exposures,retained_new_witness_owner_failures=4,prior_original_failures_reused=5,original_conflict_native_reruns=0,universal_any_Resource_Save_qualified=False,full_original_regression_passed=False)

def qualify(repo,path,inventory):
    q=load(path)
    for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed Save admission artifact: '+n)
    s=load(q['scope_review']);structure(q,s,inventory)
    for archive,ids in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
        require(q[archive] in q['artifact_sha256'],'Unbound Save source archive')
        with tarfile.open(q[archive]) as bundle:
            require(set(bundle.getnames())==set(q[ids]),'Save source partition differs')
            for n,h in q[ids].items():require(hashlib.sha256(bundle.extractfile(n).read()).hexdigest()==h,'Save source bytes differ')
    original=load(q['protected_input_baseline']);require(len(original)==1334 and all(q['proof_source_sha256'].get(n)==h for n,h in original.items()),'Protected original Save inputs omitted')
    prior=load(q['prior_overlay']);rows={x['key']:x for x in prior['reviewed_source_rows']};require(len(prior['original_hold_keys_retained'])==59 and not set(KEYS)&set(prior['original_hold_keys_retained']) and rows[KEYS[0]]['whole_row_status']=='open','Original Save gap/hold inventory changed')
    full={x['original_source']['key']:x for x in load(q['prior_full_audit'])['rows']};require(full['FRC!E132']['status']=='open-original-scope','Universal Save gap already substituted/changed')
    proof=component(Path(q['component_stage']));require(len(damaged(q,s,inventory))==10,'Damaged Save controls incomplete')
    return q,KEYS,dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,component=proof,newly_resolved_original_hold_keys=[],new_requirement_credit=1,damaged_admission_controls_rejected=10,native_tests_rerun=0,runtime_binary_copies=0,**{k:False for k in GATES})

def build(repo,baseline,path,output):
    seal,inventory,keys=builder.baseline(repo,baseline);original={k:set(v) for k,v in keys.items()}
    _,incoming,audit=qualify(repo,path,inventory);known={k:{r['key']:r for r in inventory[k]} for k in keys}
    require(not set(incoming)&keys['requirements'],'Duplicate Save filename credit')
    keys['requirements']=common.merge_keys(keys['requirements'],incoming,known['requirements'])
    coverage={k:dict(verified=len(keys[k]),total=len(known[k]),percent=round(100*len(keys[k])/len(known[k]),1),new_verified=len(keys[k])-len(original[k])) for k in keys}
    remaining={k:[r for r in seal['remaining_original_rows'][k] if r['key'] not in keys[k]] for k in keys}
    result=dict(status='passed',scope='95-percent campaign: sealed source-versioned prefix plus complete original default Save filename rule conjunction.',
        baseline=str(baseline),baseline_sha256=sha(baseline),authority_sha256=inventory['sources'],coverage=coverage,
        target_requirements=1101,target_reached=len(keys['requirements'])>=1101,requirements_still_needed=max(0,1101-len(keys['requirements'])),
        additions=[dict(key=k,kind='requirements',fingerprint=known['requirements'][k]['fingerprint'],source_text=known['requirements'][k]['text'],qualification=str(path),qualification_sha256=sha(path),status='verified-source-versioned') for k in incoming],
        remaining_counts={k:len(v) for k,v in remaining.items()},remaining_original_rows=remaining,
        technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,
        limits=['Original 1158/2367 denominators and source row identities are unchanged.','One original open Save filename row is implemented and proved; all59 separate original hold identities and statuses remain unchanged.','Scientific, physical ephemeris/frame, external ABI, platform/native OS input and full current-build approvals remain separate.'])
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
