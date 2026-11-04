"""Audit Solver Parameter store references, clone values and caller-cache isolation."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha
from AuditFunctionParameterBindings import written
from AuditFunctionGlobalResources import resource

PREFIX='QtGui.NasaFunctionSolverParameterBinding'
TYPES={'SolverState','SolverStatus'}
REG={kind+name for kind in ['QtGui.NasaFunctionResource','QtGui.NasaFunctionGlobalResource'] for name in ['DifferentialCorrector','Yukon']}


def contract(proof,spec):
    key=spec['factory_type'];meta=spec['metadata']
    require(proof['checkpoint']=='live-function-solver-parameter-owner-binding' and proof['outcome']=='passed' and proof['case']==key and proof['argument']==key+'Scoped' and proof['family']=='Solver' and proof['full_system_parameter_inventory']==321 and set(proof['initial_batch_types'])==TYPES and proof['missions']==3 and proof['function_calls']==12 and proof['live_callback_samples']==15 and proof['actual_parameter_owner_checks']==30 and proof['file_decisions']==[dict(saved=False),dict(saved=True)],'Solver proof identity/counts differ')
    require([v['phase'] for v in proof['phases']]==[0,1,2] and [v['explicit_rebuild'] for v in proof['phases']]==[True,True,False],'Solver no-Build repeat lost')
    for phase in proof['phases']:
        require(phase['raw_markers']==[42,43,44,103,104] and phase['callback_private_maps_accessed_only_while_executing'] and len(phase['live_samples'])==5 and len(phase['private_writes'])==len(phase['shared_writes'])==5,'Solver callback/Write/marker census differs')
        roots={}
        for i,sample in enumerate(phase['live_samples']):
            inside=0<i<4;calls=min(i,3)
            require(sample['inside_function']==inside and sample['calls']==calls and sample['function']==('ReadShared' if inside else '') and sample['global_map_is_value_snapshot']==(not inside) and len(sample['parameters'])==2,'Solver callback scope/order differs')
            require(sample['provider']=='Propagate PulseProp('+('ScopePulse' if inside else 'MainPulse')+') {'+('ScopePulse' if inside else 'MainPulse')+'.ElapsedSecs = 1};','Solver publication provider differs')
            for j,row in enumerate(sample['parameters']):
                shared=j==1;reference=spec['shared_reference'] if shared else spec['local_private_reference'] if inside else spec['root_private_reference']
                require(row['shared']==shared and row['reference']==reference and row['factory_type']==key and row['native_type']==meta['native_type'] and row['return_type']==meta['return_type'] and row['owner_type']==meta['owner_type_id'] and row['owner_type_name']=='DifferentialCorrector' and row['owner_name']==('SharedSC' if shared else 'PrivateOnly'),'Solver native type/reference metadata differs')
                for k in ['parameter_pointer','reference_owner_pointer','live_owner_pointer','root_owner_pointer','configured_owner_pointer']:
                    require(re.fullmatch('[0-9a-f]+',row[k]) and row[k]!='0','Invalid Solver native identity')
                require(row['reference_owner_pointer']==row['live_owner_pointer']!=row['configured_owner_pointer'] and row['owner_global']==shared and row['owner_local']==(inside and not shared),'Solver store-owner reference or flags differ')
                if i==0:roots[shared]=row['root_owner_pointer']
                require(row['root_owner_pointer']==roots[shared] and (row['live_owner_pointer']==roots[shared])==(shared or not inside),'Solver private/global aliases caller incorrectly')
                converged=inside or (shared and calls==3);status=int(converged);label='Converged' if converged else 'Created';value=status if key=='SolverState' else label
                require(row['actual']==row['expected']==value and row['shape']==[] and row['absolute_bound']==0 and (phase['shared_writes'] if shared else phase['private_writes'])[i]==value,'Solver status value or bound differs')
                a=row['attached'];require(a['name_cache_status']==status and a['name_cache_string']==label and a['reference_status']==a['store_status'] and a['store_status'] in ({0,2} if inside and not shared else {0}),'Solver cache/store status differs')
                if inside:
                    require(a['execution_clone_status']==1 and a['execution_clone_pointer'] not in {'0',row['live_owner_pointer']} and re.fullmatch('[0-9a-f]+',a['execution_clone_pointer']) and a['target_control_goal']==(-2-calls if shared else 2+calls),'Actual Target clone or independent scalar goal differs')
                else:require(a['execution_clone_pointer']=='0' and a['execution_clone_status']==-999 and a['target_control_goal']==0,'Expired Target clone accessed outside function')
    f=proof['runtime_fault_recovery'];require(f['runtime_failed'] and 'Reject' in f['diagnostic'] and 'index' in f['diagnostic'].lower() and f['private_cache_after_fault']==f['recovered_private_cache']==0 and f['shared_cache_after_fault']==f['recovered_shared_cache']==1 and f['source_history_prototypes_isolated'] and f['explicit_fault_build'] and f['explicit_recovery_build'] and f['recovered'] and f['recovery_markers']==[42,43,44,103,104],'Solver runtime failure/recovery leaked private cache or rolled back Global result')


def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,d in bindings.items():require(Path(n).is_file() and not Path(n).is_symlink() and sha(n)==d,'Changed regular binding '+n)
    def bound(p):
        p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def capsule(name):
        with tarfile.open(bound(stage/name)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source member');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible=capsule('visible-executed-sources-v1.tar.gz');before=load(bound(stage/'visible-inputs-before-v1.json'))
    require(before==load(bound(stage/'visible-inputs-after-v1.json')) and before['sources']=={n:hashlib.sha256(v).hexdigest() for n,v in visible.items()},'Visible inputs changed')
    for n in ['AuditFunctionParameterBindings.py','AuditFunctionGlobalResources.py','BuildNasaCoverageExtension.py']:
        require(hashlib.sha256(visible['src/qtgui/tests/'+n]).hexdigest()==sha(repo/'src/qtgui/tests'/n),'Imported audit helper changed')
    catalog=json.loads(visible['src/qtgui/tests/NasaFunctionSolverParameterCases.json']);specs={v['factory_type']:v for v in catalog['solver_binding_cases']}
    require(len(catalog['cases'])==321 and set(catalog['initial_types'])==set(specs)==TYPES and catalog['selected_arguments']==['SolverStateScoped','SolverStatusScoped'] and not catalog['qualified_keys'] and all(not v['qualified'] for v in catalog['cases']),'Solver321 inventory narrowed/promoted')
    matrix=load(bound(repo/catalog['baseline']));require(sha(repo/catalog['baseline'])==catalog['baseline_sha256'] and {v['factory_type']:v['native_parameter_metadata'] for v in matrix['rows'] if v['role']=='owner-bound-system-parameter'}=={v['factory_type']:v['metadata'] for v in catalog['cases']},'Original parameter metadata changed')
    global_specs={v['key']:v for v in json.loads(visible['src/qtgui/tests/NasaFunctionGlobalResourceCases.json'])['cases']};local_specs={v['key']:v for v in json.loads(visible['src/qtgui/tests/NasaFunctionResourceCases.json'])['cases']}
    maps=load(bound(stage/'loaded-images-v1.json'))['processes'];launch=load(bound(stage/'visible-launch-v1.json'));require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==6,'Visible process mapping incomplete')
    selected=[];failed=[];interrupted=[];exposures=0
    new={PREFIX+k+'Scoped' for k in TYPES};old={PREFIX+k for k in TYPES}
    for version in ['headless-v1','headless-v2','headless-v3','headless-v4','headless-v5','visible-v1']:
        invocation='function-solver-parameter-bindings-'+version;q=load(bound(repo/'build/gui-acceptance'/invocation/'results.json'));frozen=visible if version=='visible-v1' else capsule(version.replace('headless-','headless-executed-sources-')+'.tar.gz')
        require(q.get('finished_utc') and q['end_reason']==('interrupted' if version=='headless-v1' else 'completed-selection') and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest()==d for n,d in q['source_hashes'].items()),'Solver executed-source or terminal state differs')
        active={v['name']:v for v in q['results'] if v['status']!='not-run'};expected={PREFIX+'SolverState'} if version=='headless-v1' else old if version=='headless-v2' else new if version in ['headless-v3','headless-v4'] else REG if version=='headless-v5' else new|REG;require(set(active)==expected,'Solver exact selection differs')
        for name,row in active.items():
            case=bound(row['log']).parent;item=dict(invocation=invocation,test=name)
            if version=='headless-v1':require(row['status']=='interrupted','Interrupted traversal relabeled');interrupted.append(item);continue
            if version in ['headless-v2','headless-v3']:
                require(row['status']=='failed' and row['returncode']!=0,'Solver retained failure relabeled')
                if version=='headless-v2':
                    d=load(bound(case/'0-solver-reference-diagnostic.json'));require(d['execution_clone_pointer']!='0' and d['reference_pointer']==d['store_owner_pointer']!=d['execution_clone_pointer'],'Strict clone probe diagnostic lost')
                else:
                    d=load(bound(case/'0-solver-value-diagnostic.json'));require(d['calls']==3 and not d['inside_function'] and d['reference'].startswith('PrivateOnly.') and d['attached']['store_status']==d['attached']['reference_status']==0 and d['attached']['name_cache_status']==1 and d['actual'] in [1,'Converged'] and d['expected'] in [0,'Created'],'Actual caller cache leak diagnostic lost')
                failed.append(item);continue
            require(row['status']=='passed' and row['returncode']==0,'Selected Solver/regression case failed');tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Native JUnit differs')
            emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',bound(row['log']).read_text())];require(len(emitted)==1 and load(bound(case/'proof.json'))==emitted[0],'Emitted/stored proof differs');p=emitted[0]
            if name in new:
                key=name.removeprefix(PREFIX).removesuffix('Scoped');spec=specs[key];contract(p,spec)
                require(load(bound(case/'phases.json'))==p['phases'] and load(bound(case/'runtime-fault-recovery.json'))==p['runtime_fault_recovery'],'Raw Solver phases/fault differ')
                initial=bound(case/'initial.gmf').read_text();edited=bound(case/'edited.gmf').read_text();require(initial.count('Keep = 103;')==1 and initial.replace('Keep = 103;','Keep = 41;')==edited,'Solver file edit differs')
                failing=bound(case/'runtime-fault.gmf').read_text();require(failing==edited.replace('Create Variable Mark Keep ScopeEnd;','Create Variable Mark Keep ScopeEnd BadIndex;\nCreate Array Reject[1,1];').replace('Mark = Keep + Calls;','BadIndex = 2;\nReject(BadIndex,1) = 1;\nMark = Keep + Calls;'),'Runtime fault fixture changed')
                require(bound(case/'runtime-fault-messages.txt').read_text()==p['runtime_fault_recovery']['diagnostic'],'Fault diagnostic bytes differ')
                for phase in p['phases']:
                    tag=str(phase['phase']);messages=bound(case/(tag+'-messages.txt')).read_text();require(load(bound(case/(tag+'-live-parameter-samples.json')))==phase['live_samples'] and all(load(bound(case/(tag+'-invariants.json'))).values()) and load(bound(case/(tag+'-prototypes.json')))==load(bound(case/'prototypes-before.json')),'Raw Solver samples/invariants/prototypes differ')
                    require(written(messages,spec,spec['root_private_reference'])==phase['private_writes'] and written(messages,spec,spec['shared_reference'])==phase['shared_writes'],'Raw actual Solver Write differs')
                    raw=bound(case/(tag+'-binding-values.txt')).read_text().split();values=raw if key=='SolverStatus' else [float(v) for v in raw];require(values==[v['actual'] for sample in phase['live_samples'] for v in sample['parameters']],'Raw actual Solver Report differs')
                    require([float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==phase['raw_markers'],'Raw markers differ')
                require([float(v) for v in bound(case/'runtime-recovery-values.txt').read_text().split()]==[42,43,44,103,104],'Raw recovery markers differ')
                raw=bound(case/'runtime-recovery-binding-values.txt').read_text().split();require((raw if key=='SolverStatus' else [float(v) for v in raw])==[v['actual'] for sample in p['phases'][0]['live_samples'] for v in sample['parameters']],'Raw recovery Solver Report differs')
            elif name.startswith('QtGui.NasaFunctionGlobalResource'):
                key=name.removeprefix('QtGui.NasaFunctionGlobalResource');resource(p,global_specs[key])
                for phase in p['phases']:require([float(v) for v in bound(case/(str(phase['phase'])+'-values.txt')).read_text().split()]==phase['raw_values'],'Original Global raw report differs')
            else:
                key=name.removeprefix('QtGui.NasaFunctionResource');spec=local_specs[key];require(p['checkpoint']=='function-local-resource-'+key and p['outcome']=='passed' and p['case']==key and p['type']==spec['type'] and p['field']==spec['field'] and p['missions']==2 and p['function_calls']==4 and p['file_decisions']==[dict(saved=False),dict(saved=True)] and len(p['runs'])==2,'Original local Solver resource scope differs')
                for run in p['runs']:
                    require(run['first']==run['second']==float(spec['first']) and run['caller_shadow']==float(spec['second']) and not run['private_resource_leaked'] and run['configured_prototypes_unchanged'] and run['first_name']=='First' and run['second_name']=='Second','Original Solver resource field/identity differs');require([float(v) for v in bound(case/(run['phase']+'-values.csv')).read_text().split()]==[41,42],'Original local raw report differs')
            if version=='visible-v1':
                d=q['visible_display'];require(d['start_minimized'] and d['resizable'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'] and q['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible policy differs');exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Visible trace missing');mapped=maps[traces[0].stem.removeprefix('trace-')];exes=[n for n in row['registered_command'] if n in q['binary_hashes']];require(len(exes)==1 and mapped['executable']==exes[0] and all(before['images'].get(n)==d for n,d in mapped['images'].items()),'Actual mapped image differs')
                for n,d in before['images'].items():
                    if n==exes[0] or Path(n).name.startswith(('libGmatBase','libGmatUtil','libNewParameters')):require(mapped['images'].get(n)==d,'Required current image unmapped')
            selected.append(item)
    require(len(selected)==12 and len(failed)==4 and len(interrupted)==1,'Solver selections/failures/interruption census differs')
    template=load(bound(repo/'build/gui-acceptance/function-solver-parameter-bindings-visible-v1'/(PREFIX+'SolverStateScoped')/'proof.json'))
    controls=[('private-cache-leak',lambda p:p['phases'][0]['live_samples'][-1]['parameters'][0].__setitem__('actual',1)),('global-cache-rolled-back',lambda p:p['phases'][0]['live_samples'][-1]['parameters'][1]['attached'].__setitem__('name_cache_status',0)),('wrong-store-reference',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('reference_owner_pointer','1')),('wrong-clone-status',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['attached'].__setitem__('execution_clone_status',0)),('missing-post-return',lambda p:p['phases'][0]['live_samples'].pop()),('missing-no-Build-repeat',lambda p:p['phases'].pop()),('fault-leaks-private',lambda p:p['runtime_fault_recovery'].__setitem__('private_cache_after_fault',1)),('fault-rolls-back-global',lambda p:p['runtime_fault_recovery'].__setitem__('shared_cache_after_fault',0)),('recovery-missing',lambda p:p['runtime_fault_recovery'].__setitem__('recovered',False))];negatives=[]
    for label,corrupt in controls:
        p=copy.deepcopy(template);corrupt(p)
        try:contract(p,specs['SolverState'])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt Solver proof '+label)
    return dict(status='passed-two-solver-parameter-owner-and-caller-cache-bindings',qualified_keys=[],selected=selected,retained_GUI_failures=failed,retained_interrupted_cases=interrupted,retained_compile_failures=2,new_GUI_passes=4,regression_GUI_passes=8,observed_normal_missions=12,observed_live_callback_samples=60,parameter_store_owner_checks=120,actual_Write_values=120,actual_Report_values=120,actual_live_Target_clone_checks=72,normal_function_calls=48,runtime_failure_recoveries=4,additional_fault_and_recovery_missions=8,visible_mapped_processes=6,visible_exposure_events=exposures,negative_controls=negatives,native_bound_types=59,remaining_without_live_native_binding=262,factory_or_readable_role_exclusions=[],strict_external_clone_probes_qualified=False,parameter_evaluation_uses_name_cache=True,direct_execution_clone_reference_claim=False,production_changed_files=['src/base/function/FunctionManager.cpp'],whole_scope_qualified=False,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();d=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d,indent=2))
