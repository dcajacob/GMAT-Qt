"""Audit live finite-burn owners,active/stopped hardware and dimensional units."""
import argparse,copy,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha
from AuditFunctionParameterBindings import compare,written
PREFIX='QtGui.NasaFunctionFiniteBurnBinding'
TYPES={'TotalMassFlowRate','TotalAcceleration1','TotalAcceleration2','TotalAcceleration3','TotalThrust1','TotalThrust2','TotalThrust3'}


def identity(v):return isinstance(v,str) and re.fullmatch('[0-9a-f]+',v) and v!='0'


def recipe(spec):
    key=spec['factory_type'];flow=key=='TotalMassFlowRate';axis=-1 if flow else int(key[-1])-1;unit='kg/s' if flow else 'km/s^2' if key.startswith('TotalAcceleration') else 'N'
    require(key in TYPES and spec['argument']==key and spec['family']=='FiniteBurn' and spec['mass_flow']==flow and spec['axis']==axis and spec['unit']==unit and [spec[n] for n in ['root_force','private_force','shared_force','private_increment','shared_increment','isp','gravity_accel','direction_norm']]==[100,200,300,10,20,300,10,3] and spec['direction']==[1,2,2] and spec['absolute_bound']==1e-12 and spec['mass_bound']==1e-7,'Finite authored recipe/units/bounds changed')
    return flow,axis,unit


def force(shared,inside,calls):return (300 if shared else 200 if inside else 100)+(calls*(20 if shared else 10) if inside else 0)


def expected(spec,shared,inside,calls,firing):
    flow,axis,unit=recipe(spec)
    if not firing:return 0
    f=force(shared,inside,calls)
    if flow:return -f/3000
    value=f*[1,2,2][axis]/3
    return value/(1000*(1000 if inside else 2000)) if spec['factory_type'].startswith('TotalAcceleration') else value


def contract(proof,spec,units=True):
    flow,axis,unit=recipe(spec);meta=spec['metadata']
    require(proof['checkpoint']=='live-function-finite-burn-parameter-binding' and proof['outcome']=='passed' and proof['case']==proof['argument']==spec['factory_type'] and proof['family']=='FiniteBurn' and proof['route']=='active-stopped-finite-burn' and proof['full_system_parameter_inventory']==321 and set(proof['initial_batch_types'])==TYPES and proof['missions']==3 and proof['function_calls']==12 and proof['live_callback_samples']==24 and proof['actual_parameter_owner_checks']==48 and proof['executed_begin_end_commands']==48 and proof['propagations']==36 and proof['file_decisions']==[dict(saved=False),dict(saved=True)],'Finite native proof identity/count/workflow differs')
    if units:require(proof['unmanaged_factory_unit']==unit,'Unmanaged factory unit differs')
    require([v['phase'] for v in proof['phases']]==[0,1,2] and [v['explicit_rebuild'] for v in proof['phases']]==[True,True,False],'Finite no-Build repeat omitted')
    for phase in proof['phases']:
        require(phase['raw_markers']==[42,43,44,103,104] and phase['callback_private_maps_accessed_only_while_executing'] and len(phase['live_samples'])==8 and len(phase['private_writes'])==len(phase['shared_writes'])==8,'Finite sample/Write/marker contract differs')
        root_owners={};targets={}
        for index,sample in enumerate(phase['live_samples']):
            calls=index//2;inside=calls>0;ready=index%2+1;firing=ready==1
            require(sample['inside_function']==inside and sample['function']==('ReadShared' if inside else '') and sample['calls']==calls and sample['ready']==ready and sample['firing']==firing and sample['global_map_is_value_snapshot']==(not inside) and len(sample['parameters'])==2,'Finite callback order/lifecycle/map provenance differs')
            require(sample['provider']==('Propagate PulseProp(ScopePulse) {ScopePulse.ElapsedSecs = 1};' if inside else 'Propagate PulseProp(MainPulse) {MainPulse.ElapsedSecs = 1};'),'Actual finite publication provider differs')
            target=sample['target_state'];elapsed=(target['epoch']-21545)*86400;mass0=1000 if inside else 2000;mass=mass0-((force(False,inside,calls)+force(True,inside,calls))*elapsed/3000 if flow else 0)
            require(target['name']==('ScopePulse' if inside else 'MainPulse') and identity(target['pointer']) and identity(target['main_target_pointer']) and (target['pointer']==target['main_target_pointer'])==(not inside) and target['ready']==ready and target['firing']==firing and target['dry_mass']==(800 if inside else 1800) and target['mass_bound']==1e-7 and abs(elapsed-ready)<=3e-6,'Actual finite target identity/elapsed domain differs')
            compare(target['elapsed'],elapsed,1e-9);compare(target['total_mass'],mass,1e-7);compare(target['expected_total_mass'],mass,1e-7)
            if firing:targets[calls]=target['pointer']
            require(target['pointer']==targets[calls],'Active/stopped target changed within call')
            for position,row in enumerate(sample['parameters']):
                shared=position==1;reference=spec['shared_reference'] if shared else spec['local_private_reference'] if inside else spec['root_private_reference']
                require(row['shared']==shared and row['reference']==reference and row['factory_type']==spec['factory_type'] and row['native_type']==meta['native_type'] and row['return_type']==meta['return_type']==4 and row['owner_type']==meta['owner_type_id']==108 and row['owner_name']==('SharedBurn' if shared else 'PrivateBurn') and row['owner_type_name']=='FiniteBurn','Actual finite Parameter/owner type differs')
                for key in ['parameter_pointer','reference_owner_pointer','live_owner_pointer','root_owner_pointer','configured_owner_pointer']:require(identity(row[key]),'Invalid actual finite identity')
                require(row['reference_owner_pointer']==row['live_owner_pointer']!=row['configured_owner_pointer'] and row['owner_global']==shared and row['owner_local']==(inside and not shared),'Finite Parameter reference/owner flags differ')
                if not inside:root_owners[shared]=row['root_owner_pointer']
                require(row['root_owner_pointer']==root_owners[shared] and (row['live_owner_pointer']==row['root_owner_pointer'])==(shared or not inside),'Finite owner aliases caller incorrectly')
                if units:require(row['unit']==unit,'Actual live finite unit differs')
                value=expected(spec,shared,inside,calls,firing);require(row['shape']==[] and row['absolute_bound']==1e-12,'Finite scalar shape/bounds changed')
                for key in ['actual','expected','direct_owner_property_value']:compare(row[key],value,1e-12)
                compare(phase['shared_writes'][index] if shared else phase['private_writes'][index],value,1e-12)
                engine=row['hardware'];f=force(shared,inside,calls);tank_mass=100-(f*elapsed/3000 if flow else 0)
                require(engine['engine_name']==('SharedEngine' if shared else 'LocalEngine' if inside else 'RootEngine') and engine['tank_name']==('SharedFuel' if shared else 'LocalFuel' if inside else 'RootFuel') and engine['engine_type']=='ChemicalThruster' and engine['tank_type']=='ChemicalTank' and identity(engine['burn_engine_pointer']) and engine['burn_engine_pointer']==engine['target_engine_pointer'] and identity(engine['engine_tank_pointer']) and engine['engine_tank_pointer']==engine['target_tank_pointer'],'Actual finite burn/target thruster/tank reference differs')
                require([engine[k] for k in ['thrust_C1','isp_K1','gravity_accel','duty','scale']]==[f,300,10,1,1] and engine['direction']==[1,2,2] and engine['decrement_mass']==flow and engine['engine_firing']==firing and engine['frame_name']=='EarthMJ2000Eq' and not engine['thruster_frame_pointer_directly_observed'],'Actual attached hardware fields/frame limits differ')
                compare(engine['tank_mass'],tank_mass,1e-7);compare(engine['expected_tank_mass'],tank_mass,1e-7)
                burn=row['burn_state'];require(burn['has_fired'] and burn['is_firing']==firing and math.isfinite(burn['last_fire_epoch']) and burn['internal_frame_name']=='InternalEarthMJ2000Eq' and identity(burn['internal_frame_pointer']) and burn['internal_frame_pointer']==burn['moderator_internal_frame_pointer'],'Finite firing/internal-frame identity differs')
                compare(burn['total_mass_flow'],-f/3000 if flow and firing else 0,1e-12)
                thrust=[f*v/3 if firing else 0 for v in [1,2,2]];compare(burn['total_thrust'],thrust,1e-12)
                if not flow:compare(burn['total_acceleration'],[v/(mass0*1000) for v in thrust],1e-12)
                elif not firing:compare(burn['total_acceleration'],[0,0,0],1e-12)
                else:require(len(burn['total_acceleration'])==3 and all(math.isfinite(v) for v in burn['total_acceleration']),'Nonfinite recorded mass-varying acceleration')


def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,d in bindings.items():require(Path(n).is_file() and not Path(n).is_symlink() and sha(n)==d,'Changed regular input '+n)
    def bound(p):
        p=Path(p);require(str(p) in bindings,'Unbound artifact '+str(p));return p
    def capsule(name):
        with tarfile.open(bound(stage/name)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source member');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible=capsule('visible-executed-sources-v1.tar.gz');before=load(bound(stage/'visible-inputs-before-v1.json'))
    require(before==load(bound(stage/'visible-inputs-after-v1.json')) and before['sources']=={n:hashlib.sha256(v).hexdigest() for n,v in visible.items()},'Visible inputs changed')
    for n in ['src/qtgui/tests/AuditFunctionParameterBindings.py','src/qtgui/tests/BuildNasaCoverageExtension.py']:require(hashlib.sha256(visible[n]).hexdigest()==sha(repo/n),'Imported audit helper source changed')
    catalog=json.loads(visible['src/qtgui/tests/NasaFunctionFiniteBurnCases.json']);metadata={v['factory_type']:v['metadata'] for v in catalog['cases']};specs={v['argument']:v for v in catalog['binding_cases']}
    require(len(metadata)==321 and set(catalog['initial_types'])==set(catalog['selected_arguments'])==set(specs)==TYPES and not catalog['qualified_keys'] and all(not v['qualified'] for v in catalog['cases']+catalog['binding_cases']),'Complete321 catalog narrowed/promoted')
    matrix=load(bound(repo/catalog['baseline']));require(sha(repo/catalog['baseline'])==catalog['baseline_sha256'] and {v['factory_type']:v['native_parameter_metadata'] for v in matrix['rows'] if v['role']=='owner-bound-system-parameter'}==metadata,'Original321 metadata changed')
    unit_contract=load(bound(stage/'native-contract-v2-units.json'));doc=bound(repo/unit_contract['source']);require(sha(doc)==unit_contract['source_sha256'],'Calculation-parameter unit reference changed');text=doc.read_text();unit_rows=[]
    for spec in specs.values():
        flow,axis,unit=recipe(spec);require(spec['metadata']==metadata[spec['factory_type']] and spec['metadata']['owner_type_name']=='FiniteBurn','Finite owner metadata differs');key=spec['factory_type'];a=text.index('<entry><guilabel>'+key+'</guilabel></entry>');block=text[a:text.index('</row>',a)];original=re.search(r'<term>Units</term>\s*<listitem>\s*<para>(.*?)</para>',block,re.S).group(1);require(original==('Kg/s' if flow else 'Km/s<superscript>2</superscript>' if key.startswith('TotalAcceleration') else 'Newtons') and unit_contract['expected_units'][key]==unit,'Native normalized unit/reference differs');unit_rows.append(dict(factory_type=key,reference_units=original,native_unit=unit,original_settable_metadata_retained=spec['metadata']['settable'],reference_settable='N',read_only_conflict_qualified=False))
    prior=load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-burn-parameter-bindings/visible-inputs-before-v1.json'))['sources']
    for n in ['NasaFunctionParameterBindingTests.cpp','NasaFunctionSpacecraftFieldBindingTests.cpp','NasaFunctionBurnParameterBindingTests.cpp','NasaFunctionVectorResourceTests.cpp','NasaSubscriberGlobalLoopTests.cpp']:
        require(hashlib.sha256(visible['src/qtgui/tests/'+n]).hexdigest()==prior['src/qtgui/tests/'+n],'Prior assertion source changed')
    maps=load(bound(stage/'loaded-images-v1.json'))['processes'];launch=load(bound(stage/'visible-launch-v1.json'));require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==7,'Visible mapping incomplete')
    selected=[];superseded=[];failures=[];exposures=0
    for version in ['headless-v1','headless-v2','headless-v3','visible-v1']:
        invocation='function-finite-burn-bindings-'+version;receipt=load(bound(repo/'build/gui-acceptance'/invocation/'results.json'));frozen=visible if version=='visible-v1' else capsule(version.replace('headless-','headless-executed-sources-')+'.tar.gz')
        require(receipt.get('finished_utc') and receipt['end_reason']=='completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest()==d for n,d in receipt['source_hashes'].items()),'Executed source/terminal state differs')
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};require(set(active)=={PREFIX+k for k in TYPES},'Exact native selection differs')
        for name,row in active.items():
            spec=specs[name.removeprefix(PREFIX)];log=bound(row['log']);case=log.parent;tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'))
            if version=='headless-v2':
                require(row['status']=='failed' and row['returncode']!=0 and len(tests)==1 and tests[0].find('failure') is not None and tests[0].find('skipped') is None and 'Native finite-burn factory unit metadata differs from dimensional contract' in log.read_text() and 'FINITE_UNIT actual=Km/s expected='+spec['unit'] in log.read_text(),'Retained native wrong-unit evidence lost');failures.append(dict(invocation=invocation,test=name));continue
            require(row['status']=='passed' and row['returncode']==0 and len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and tests[0].find('failure') is None,'Native result/JUnit differs')
            emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())];require(len(emitted)==1 and load(bound(case/'proof.json'))==emitted[0],'Native emitted/stored proof differs');proof=emitted[0];contract(proof,spec,units=version!='headless-v1')
            original=bound(case/'initial.gmf').read_text();edited=bound(case/'edited.gmf').read_text();caller=bound(case/'caller.script').read_text();nested=bound(case/'nested.gmf').read_text()
            require(original.replace('Keep = 103;','Keep = 41;')==edited and 'Mark = Keep + Calls;' in edited and '[Mark] = ReadShared();' in nested and 'Create FiniteBurn PrivateBurn;' in caller+edited and all(v in edited for v in ['ScopePulse.LocalEngine.C1 = 200 + Calls * 10;','ScopePulse.SharedEngine.C1 = 300 + Calls * 20;']) and all((script.count('BeginFiniteBurn '),script.count('EndFiniteBurn '),script.count('Propagate PulseProp('))==(2,2,3) for script in [caller,edited]),'Exact finite edit/mutation/lifecycle script differs')
            prototypes=load(bound(case/'prototypes-before.json'))
            for phase in proof['phases']:
                tag=str(phase['phase']);invariants=load(bound(case/(tag+'-invariants.json')));require(load(bound(case/(tag+'-live-parameter-samples.json')))==phase['live_samples'] and load(bound(case/(tag+'-prototypes.json')))==prototypes and all(invariants.values()) and [float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==phase['raw_markers'] and not bound(case/(tag+'-observer-error.txt')).read_text(),'Raw finite samples/root/source invariants differ')
                messages=bound(case/(tag+'-messages.txt')).read_text();require(written(messages,spec,spec['root_private_reference'])==phase['private_writes'] and written(messages,spec,spec['shared_reference'])==phase['shared_writes'],'Raw finite Write bytes differ');compare([float(v) for v in bound(case/(tag+'-binding-values.txt')).read_text().split()],[p['actual'] for sample in phase['live_samples'] for p in sample['parameters']],1e-12)
            item=dict(invocation=invocation,test=name)
            if version=='headless-v1':superseded.append(item);continue
            if version=='visible-v1':
                display=receipt['visible_display'];require(display['start_minimized'] and display['resizable'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible window policy differs');exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Visible trace absent');process=maps[traces[0].stem.removeprefix('trace-')];exes=[n for n in row['registered_command'] if n in receipt['binary_hashes']];require(len(exes)==1 and process['executable']==exes[0] and all(before['images'].get(n)==d for n,d in process['images'].items()),'Mapped image identity differs')
                for n,d in before['images'].items():
                    if n==exes[0] or Path(n).name.startswith(('libGmatBase','libGmatUtil')):require(process['images'].get(n)==d,'Actual executable/core image unmapped')
            selected.append(item)
    require(len(selected)==14 and len(failures)==len(superseded)==7,'Finite selected/failure/superseded count differs')
    key='TotalMassFlowRate';template=load(bound(repo/'build/gui-acceptance/function-finite-burn-bindings-visible-v1'/(PREFIX+key)/'proof.json'));controls=[('configured-owner',lambda p:p['phases'][0]['live_samples'][2]['parameters'][0].__setitem__('reference_owner_pointer',p['phases'][0]['live_samples'][2]['parameters'][0]['configured_owner_pointer'])),('caller-owner',lambda p:p['phases'][0]['live_samples'][2]['parameters'][0].__setitem__('live_owner_pointer',p['phases'][0]['live_samples'][2]['parameters'][0]['root_owner_pointer'])),('native-unit',lambda p:p['phases'][0]['live_samples'][2]['parameters'][0].__setitem__('unit','Km/s')),('wrong-thruster',lambda p:p['phases'][0]['live_samples'][2]['parameters'][0]['hardware'].__setitem__('burn_engine_pointer','0')),('wrong-tank',lambda p:p['phases'][0]['live_samples'][2]['parameters'][0]['hardware'].__setitem__('engine_tank_pointer','0')),('wrong-mass-flow',lambda p:p['phases'][0]['live_samples'][2]['parameters'][0].__setitem__('actual',0)),('nonzero-stopped-value',lambda p:p['phases'][0]['live_samples'][3]['parameters'][0].__setitem__('actual',1)),('stopped-engine-firing',lambda p:p['phases'][0]['live_samples'][3]['parameters'][0]['hardware'].__setitem__('engine_firing',True)),('wrong-target-mass',lambda p:p['phases'][0]['live_samples'][2]['target_state'].__setitem__('total_mass',1000)),('omitted-no-Build-repeat',lambda p:p['phases'].pop())];negatives=[]
    for label,corrupt in controls:
        bad=copy.deepcopy(template);corrupt(bad)
        try:contract(bad,specs[key])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt finite binding '+label)
    return dict(status='passed-seven-live-finite-burn-bindings-and-corrected-native-unit-labels',qualified_keys=[],selected=selected,new_binding_GUI_passes=14,retained_GUI_failures=failures,superseded_value_only_passes=superseded,value_only_passes_do_not_prove_units=True,retained_compile_failures=0,system_parameters_retained=321,additional_native_bound_types=7,cumulative_native_bound_types=57,remaining_without_live_native_binding=264,binding_missions=42,function_calls=168,live_callback_samples=336,function_callback_samples=252,active_samples=168,stopped_samples=168,actual_parameter_owner_checks=672,direct_owner_property_checks=672,actual_typed_Write_values=672,actual_Report_values=672,actual_begin_end_commands=672,actual_propagations=504,unmanaged_factory_unit_checks=14,live_native_unit_checks=672,actual_burn_thruster_identity_checks=672,actual_thruster_tank_identity_checks=672,nonzero_mass_flow_checks=48,marker_scalars=210,visible_mapped_processes=7,visible_exposure_events=exposures,negative_controls=negatives,unit_reference_rows=sorted(unit_rows,key=lambda v:v['factory_type']),read_only_metadata_conflict_qualified=False,thruster_coordinate_frame_pointer_directly_observed=False,production_changed_files=['src/base/parameter/BurnParameters.cpp'],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--stage',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();result=audit(Path(__file__).resolve().parents[3],args.stage.resolve());args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
