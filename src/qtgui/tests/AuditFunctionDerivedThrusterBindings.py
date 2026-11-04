"""Audit three derived thruster owners, active/stopped lifecycle, units and fuel budgets."""
import argparse,copy,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha
from AuditFunctionParameterBindings import compare,written
UNITS={'ThrustMagnitude':'N','Isp':'s','MassFlowRate':'kg/s'}
TYPES=set(UNITS);PREFIX='QtGui.NasaFunctionDerivedThrusterBinding'


def inputs(shared,inside,calls):
    f=(300 if shared else 200 if inside else 100)+(calls*(20 if shared else 10) if inside else 0)
    k=(500 if shared else 400 if inside else 300)+(calls*(10 if shared else 5) if inside else 0)
    duty=1 if shared else .75 if inside else .5
    scale=.8 if shared else 1.5 if inside else 1.25
    return f,k,duty,scale


def rate(shared,inside,calls):
    f,k,duty,_=inputs(shared,inside,calls)
    return f*duty/(10*k)


def prior_loss(calls):
    return 2*rate(True,False,0)+sum(2*rate(True,True,k) for k in range(1,calls))


def contract(p,spec):
    key=spec['factory_type'];meta=spec['metadata'];unit=UNITS[key]
    require(spec['unit']==unit and spec['absolute_bound']==1e-12 and spec['mass_bound']==1e-7 and spec['clock_bound_seconds']==3e-6,'Derived recipe units/bounds differ')
    require(p['checkpoint']=='live-function-derived-thruster-parameter-binding' and p['outcome']=='passed' and p['case']==p['argument']==key and p['route']=='active-stopped-derived-thruster' and p['family']=='DerivedThruster' and p['unmanaged_factory_unit']==unit and p['full_system_parameter_inventory']==321 and set(p['initial_batch_types'])==TYPES and p['missions']==3 and p['function_calls']==12 and p['live_callback_samples']==24 and p['actual_parameter_owner_checks']==48 and p['caller_root_control_callbacks']==12 and p['executed_begin_end_commands']==48 and p['propagations']==72 and p['file_decisions']==[dict(saved=False),dict(saved=True)],'Derived proof identity/counts differ')
    require([v['phase'] for v in p['phases']]==[0,1,2] and [v['explicit_rebuild'] for v in p['phases']]==[True,True,False],'Derived no-Build repeat lost')
    for phase in p['phases']:
        require(phase['raw_markers']==[42,43,44,103,104] and phase['callback_private_maps_accessed_only_while_executing'] and len(phase['live_samples'])==len(phase['private_writes'])==len(phase['shared_writes'])==8 and len(phase['root_controls'])==4,'Derived callback/Write/control census differs')
        controls=phase['root_controls'];snapshot=phase['caller_snapshot'];require(snapshot==controls[0]['snapshot'] and len(snapshot['state'])==6 and all(isinstance(v,(int,float)) and math.isfinite(v) for v in snapshot['state']),'Caller snapshot shape differs')
        require(snapshot['C1']==100 and snapshot['K1']==300 and snapshot['dry_mass']==1000 and snapshot['duty']==.5 and snapshot['scale']==1.25 and snapshot['firing'] is False and abs((snapshot['epoch']-21545)*86400-3)<=3e-6,'Caller completed-main snapshot differs');compare(snapshot['fuel_mass'],100-2*rate(False,False,0),1e-7);compare(snapshot['total_mass'],1100-2*rate(False,False,0),1e-7)
        for calls,control in enumerate(controls):
            require(control['calls']==calls and control['inside_function']==(calls>0) and control['provider']=='Propagate SharedProp(SharedSC) {SharedSC.ElapsedSecs = 1};' and control['snapshot']==snapshot,'Function changed exact completed-main caller snapshot')
        roots={};engines={}
        for i,sample in enumerate(phase['live_samples']):
            calls=i//2;inside=calls>0;ready=i%2+1;firing=ready==1
            require(sample['calls']==calls and sample['inside_function']==inside and sample['function']==('ReadShared' if inside else '') and sample['ready']==ready and sample['firing']==firing and sample['provider']=='Propagate PrivateProp(PrivateOnly) {PrivateOnly.ElapsedSecs = 1};' and sample['global_map_is_value_snapshot']==(not inside) and len(sample['parameters'])==2,'Derived provider/scope/lifecycle differs')
            for j,row in enumerate(sample['parameters']):
                shared=j==1;f,k,duty,scale=inputs(shared,inside,calls);want=(f*duty*scale if key=='ThrustMagnitude' else k if key=='Isp' else -rate(shared,inside,calls)) if firing else 0
                ref=spec['shared_reference'] if shared else spec['local_private_reference'] if inside else spec['root_private_reference']
                require(row['shared']==shared and row['reference']==ref and row['factory_type']==key and row['native_type']==meta['native_type'] and row['unit']==unit and row['return_type']==meta['return_type']==4 and row['owner_type']==meta['owner_type_id']==101 and row['owner_name']==('SharedSC' if shared else 'PrivateOnly') and row['owner_type_name']=='Spacecraft' and row['shape']==[] and row['absolute_bound']==1e-12,'Derived Parameter metadata differs')
                for field in ['parameter_pointer','reference_owner_pointer','live_owner_pointer','root_owner_pointer','configured_owner_pointer']:require(re.fullmatch('[0-9a-f]+',row[field]) and row[field]!='0','Invalid derived owner identity')
                require(row['reference_owner_pointer']==row['live_owner_pointer']!=row['configured_owner_pointer'] and row['owner_global']==shared and row['owner_local']==(inside and not shared),'Derived reference/owner flags differ')
                if i==0:roots[shared]=row['root_owner_pointer']
                require(row['root_owner_pointer']==roots[shared] and (row['live_owner_pointer']==roots[shared])==(shared or not inside),'Derived owner aliases wrong root')
                if not shared:require(sample['published_owner_pointer']==row['live_owner_pointer'],'Published propagation owner is not actual private Parameter owner')
                for field in ['actual','expected','direct_native_method_value']:compare(row[field],want,1e-12)
                compare((phase['shared_writes'] if shared else phase['private_writes'])[i],want,1e-12)
                a=row['hardware'];engine_name='SharedEngine' if shared else 'LocalEngine' if inside else 'RootEngine';tank_name='SharedFuel' if shared else 'LocalFuel' if inside else 'RootFuel'
                require(a['engine_name']==engine_name and a['tank_name']==tank_name and a['burn_name']==('SharedBurn' if shared else 'PrivateBurn'),'Derived hardware/burn names differ')
                for field in ['engine_pointer','root_engine_pointer','configured_engine_pointer','burn_pointer','tank_pointer','engine_tank_pointer','burn_engine_pointer']:require(re.fullmatch('[0-9a-f]+',a[field]) and a[field]!='0','Invalid derived hardware identity')
                if i==0:engines[shared]=a['root_engine_pointer']
                require(a['root_engine_pointer']==engines[shared] and a['engine_pointer']!=a['configured_engine_pointer'] and (a['engine_pointer']==engines[shared])==(shared or not inside) and a['burn_engine_pointer']==a['engine_pointer'] and a['engine_tank_pointer']==a['tank_pointer'],'Derived actual owner/engine/tank/burn identity differs')
                require(a['C1']==f and a['K1']==k and a['duty']==duty and a['scale']==scale and a['gravity']==10 and a['decrement_mass'] is True and a['engine_firing']==a['burn_firing']==firing and a['burn_has_fired'] is True and a['direction']==[1,2,2] and a['frame_name']=='EarthMJ2000Eq' and a['thruster_frame_pointer_directly_observed'] is False,'Derived constant-coefficient/scale/duty/lifecycle fixture differs')
                elapsed=3*calls+ready if shared and inside else ready;previous=prior_loss(calls) if shared and inside else 0;fuel=100-previous-ready*rate(shared,inside,calls);dry=2000 if shared else 1100 if inside else 1000
                require(a['expected_owner_elapsed']==elapsed and a['expected_fuel_mass']==fuel and a['expected_total_mass']==dry+fuel and a['dry_mass']==dry and a['previous_Global_fuel_loss']==previous,'Independent fuel/clock recipe differs');compare(a['owner_elapsed'],elapsed,3e-6);compare((a['owner_epoch']-21545)*86400,elapsed,3e-6);compare(a['fuel_mass'],fuel,1e-7);compare(a['total_mass'],dry+fuel,1e-7)
                require(meta['settable'] is True and a['runtime_settable'] is True and a['info_settable'] is True,'Original derived writable flags changed')
                if firing:require(a['native_setter_result']=='rejected' and 'SpacecraftData::SetReal() Not settable or unknown item id:' in a['native_setter_error'],'Recorded active setter discrepancy differs')
                else:require(a['native_setter_result']=='not-probed-stopped' and not a['native_setter_error'],'Stopped setter scope changed')


def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,d in bindings.items():require(Path(n).is_file() and not Path(n).is_symlink() and sha(n)==d,'Changed regular binding '+n)
    def bound(p):
        p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def capsule(name):
        with tarfile.open(bound(stage/name)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source member');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible=capsule('visible-executed-sources-v1.tar.gz');before=load(bound(stage/'visible-inputs-before-v1.json'));require(before==load(bound(stage/'visible-inputs-after-v1.json')) and before['sources']=={n:hashlib.sha256(v).hexdigest() for n,v in visible.items()},'Visible inputs changed')
    for n in ['AuditFunctionDerivedThrusterBindings.py','AuditFunctionParameterBindings.py','BuildNasaCoverageExtension.py']:require(hashlib.sha256(visible['src/qtgui/tests/'+n]).hexdigest()==sha(repo/'src/qtgui/tests'/n),'Auditor/helper changed')
    catalog=json.loads(visible['src/qtgui/tests/NasaFunctionDerivedThrusterCases.json']);specs={v['factory_type']:v for v in catalog['derived_binding_cases']};require(len(catalog['cases'])==321 and set(specs)==set(catalog['initial_types'])==set(catalog['selected_arguments'])==TYPES and not catalog['qualified_keys'] and all(not v['qualified'] for v in catalog['cases']),'Complete321 narrowed/promoted')
    baseline=load(bound(repo/catalog['baseline']));require(sha(repo/catalog['baseline'])==catalog['baseline_sha256'] and {v['factory_type']:v['native_parameter_metadata'] for v in baseline['rows'] if v['role']=='owner-bound-system-parameter'}=={v['factory_type']:v['metadata'] for v in catalog['cases']},'Original metadata/roles changed')
    maps=load(bound(stage/'loaded-images-v1.json'))['processes'];launch=load(bound(stage/'visible-launch-v1.json'));require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==3,'Visible map/selection incomplete')
    selected,failures,exposures=[],[],0
    for version in ['headless-v1','headless-v2','visible-v1']:
        inv='function-derived-thruster-bindings-'+version;q=load(bound(repo/'build/gui-acceptance'/inv/'results.json'));frozen=visible if version=='visible-v1' else capsule(version.replace('headless-','headless-executed-sources-')+'.tar.gz');require(q.get('finished_utc') and q['end_reason']=='completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest()==d for n,d in q['source_hashes'].items()),'Executed source/terminal state differs');rows={v['name']:v for v in q['results'] if v['status']!='not-run'};require(set(rows)=={PREFIX+key for key in TYPES},'Exact derived selection differs')
        for name,row in rows.items():
            key=name.removeprefix(PREFIX);case=bound(row['log']).parent;item=dict(invocation=inv,test=name)
            if version=='headless-v1':
                require(row['status']=='failed' and row['returncode']!=0 and 'Native derived-thruster factory unit metadata differs from dimensional contract' in bound(row['log']).read_text() and load(bound(case/'factory-unit.json'))==dict(actual='',case=key,expected=UNITS[key],settable=True),'Retained empty-unit failure lost');failures.append(item);continue
            require(row['status']=='passed' and row['returncode']==0,'Derived selected case failed');tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Derived native JUnit differs');emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',bound(row['log']).read_text())];require(len(emitted)==1 and load(bound(case/'proof.json'))==emitted[0],'Derived emitted/stored proof differs');p=emitted[0];spec=specs[key];contract(p,spec)
            initial,edited=bound(case/'initial.gmf').read_text(),bound(case/'edited.gmf').read_text();require(initial.count('Keep = 103;')==1 and initial.replace('Keep = 103;','Keep = 41;')==edited and 'Global SharedSC SharedBurn Calls BindingValues Ready;' in edited and 'PrivateOnly.LocalEngine.K1 = 400 + Calls * 5;' in edited and 'SharedSC.SharedEngine.K1 = 500 + Calls * 10;' in edited,'Derived exact file edit/input recipe differs')
            for script in [edited,bound(case/'caller.script').read_text()]:
                require(script.count('BeginFiniteBurn ')==script.count('EndFiniteBurn ')==2 and script.count('Propagate ')==6 and script.count('Ready = 3;')==1,'Actual separate-target lifecycle command counts differ')
            require(load(bound(case/'phases.json'))==p['phases'],'Raw derived phases differ')
            for phase in p['phases']:
                tag=str(phase['phase']);messages=bound(case/(tag+'-messages.txt')).read_text();require(load(bound(case/(tag+'-live-parameter-samples.json')))==phase['live_samples'] and load(bound(case/(tag+'-root-controls.json')))==phase['root_controls'] and all(load(bound(case/(tag+'-invariants.json'))).values()) and load(bound(case/(tag+'-prototypes.json')))==load(bound(case/'prototypes-before.json')),'Raw derived samples/control/prototype/invariants differ');require(written(messages,spec,spec['root_private_reference'])+written(messages,spec,spec['local_private_reference'])==phase['private_writes'] and written(messages,spec,spec['shared_reference'])==phase['shared_writes'],'Raw derived Write differs');compare([float(v) for v in bound(case/(tag+'-binding-values.txt')).read_text().split()],[v['actual'] for sample in phase['live_samples'] for v in sample['parameters']],1e-12);require([float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==phase['raw_markers'],'Raw derived marker Report differs')
            if version=='visible-v1':
                d=q['visible_display'];require(d['start_minimized'] and d['resizable'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'] and q['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible policy differs');exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Visible trace absent');mapped=maps[traces[0].stem.removeprefix('trace-')];exes=[n for n in row['registered_command'] if n in q['binary_hashes']];require(len(exes)==1 and mapped['executable']==exes[0] and all(before['images'].get(n)==d for n,d in mapped['images'].items()),'Actual mapped image differs')
                for n,d in before['images'].items():
                    if n==exes[0] or Path(n).name.startswith(('libGmatBase','libGmatUtil')):require(mapped['images'].get(n)==d,'Required image unmapped')
            selected.append(item)
    require(len(selected)==6 and len(failures)==3,'Derived pass/failure census differs')
    template=load(bound(repo/'build/gui-acceptance/function-derived-thruster-bindings-visible-v1'/(PREFIX+'MassFlowRate')/'proof.json'))
    def row(p):return p['phases'][0]['live_samples'][2]['parameters'][1]
    controls=[('wrong-owner-reference',lambda p:row(p).__setitem__('reference_owner_pointer','1')),('private-owner-aliases-root',lambda p:p['phases'][0]['live_samples'][2]['parameters'][0].__setitem__('live_owner_pointer',p['phases'][0]['live_samples'][2]['parameters'][0]['root_owner_pointer'])),('wrong-burn-engine',lambda p:row(p)['hardware'].__setitem__('burn_engine_pointer','1')),('wrong-engine-tank',lambda p:row(p)['hardware'].__setitem__('engine_tank_pointer','1')),('wrong-active-value',lambda p:row(p).__setitem__('actual',999)),('stopped-not-zero',lambda p:p['phases'][0]['live_samples'][3]['parameters'][1].__setitem__('actual',1)),('wrong-unit',lambda p:row(p).__setitem__('unit','')),('wrong-duty',lambda p:row(p)['hardware'].__setitem__('duty',.5)),('wrong-scale',lambda p:row(p)['hardware'].__setitem__('scale',1)),('wrong-Global-fuel-history',lambda p:row(p)['hardware'].__setitem__('previous_Global_fuel_loss',0)),('wrong-fuel-mass',lambda p:row(p)['hardware'].__setitem__('fuel_mass',100)),('wrong-clock',lambda p:row(p)['hardware'].__setitem__('owner_elapsed',1)),('caller-state-mutated',lambda p:p['phases'][0]['root_controls'][1]['snapshot']['state'].__setitem__(0,0)),('missing-no-Build',lambda p:p['phases'].pop()),('wrong-marker',lambda p:p['phases'][0]['raw_markers'].__setitem__(0,999))];negatives=[]
    for label,corrupt in controls:
        p=copy.deepcopy(template);corrupt(p)
        try:contract(p,specs['MassFlowRate'])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt derived proof '+label)
    return dict(status='passed-3-derived-thruster-owner-bindings-and-unit-repair',qualified_keys=[],selected=selected,retained_GUI_failures=failures,retained_compile_failures=1,retained_interrupted_cases=[],new_parameter_types=3,missions=18,function_calls=72,live_parameter_callback_samples=144,caller_root_control_callbacks=72,parameter_owner_checks=288,actual_burn_engine_tank_checks=288,direct_native_method_checks=288,active_nonzero_values=144,stopped_zero_values=144,unit_checks=288,fuel_budget_checks=288,total_mass_checks=288,clock_checks=288,active_native_setter_refusals=144,actual_Write_values=288,actual_Report_values=288,marker_values=90,begin_end_commands=288,propagations=432,visible_mapped_processes=3,visible_exposure_events=exposures,negative_controls=negatives,native_bound_types=150,remaining_without_live_native_binding=171,production_changed_files=['src/base/parameter/HardwareParameters.cpp'],factory_or_readable_role_exclusions=[],writable_metadata_reclassified=False,script_setters_qualified=False,thruster_stored_frame_pointer_directly_observed=False,raw_cache_before_reevaluation_directly_observed=False,arbitrary_polynomial_propulsion_frame_or_time_science_qualified=False,whole_scope_qualified=False,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
