"""Audit actual bare/frame burn Parameter ownership and executed maneuvers."""
import argparse,copy,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha

PREFIX='QtGui.NasaFunctionBurnParameterBinding'
TYPES={'Element1','Element2','Element3','V','N','B'}
REG={'QtGui.NasaFunctionVectorResource':'NasaFunctionVectorResourceTests.cpp','QtGui.NativeNasaSubscriberGlobalLoopGlobal':'NasaSubscriberGlobalLoopTests.cpp'}


def compare(actual,expected,bound):
    if isinstance(expected,str):require(actual==expected,'Authored epoch stamp differs');return
    if isinstance(expected,list):
        require(isinstance(actual,list) and len(actual)==len(expected),'Typed value shape differs')
        for a,e in zip(actual,expected):compare(a,e,bound)
        return
    require(isinstance(actual,(int,float)) and math.isfinite(actual) and abs(actual-expected)<=bound,'Selected independent value differs')


def recipe(spec):
    axes={'Element1':0,'Element2':1,'Element3':2,'V':0,'N':1,'B':2};key=spec['factory_type'];axis=axes[key]
    require(spec['axis']==axis and spec['setter']=='Element'+str(axis+1) and [spec[k] for k in ['root_value','private_value','shared_value']]==[.125,.25,.5] and spec['step']==.0625 and spec['absolute_bound']==1e-12,'Authored burn recipe/bounds changed')
    require(spec['argument']==key+('Frame' if spec['frame_reference'] else 'Bare'),'Burn argument/route changed')
    return [.125,.25,.5],.0625


def expected(spec,shared,inside,calls):
    values,step=recipe(spec);return values[2 if shared else 1 if inside else 0]+(calls*step if inside else 0),1e-12,[]


def written(text,spec,reference):
    names={spec['root_private_reference'],spec['local_private_reference'],spec['shared_reference'],'ScopeEnd'}
    delimiters='(?:'+'|'.join(re.escape(v) for v in sorted(names))+')'
    blocks=re.findall(r'(?:^|\n)'+re.escape(reference)+r' =\s*\n([\s\S]*?)(?=\n'+delimiters+' =)',text)
    values=[]
    for block in blocks:
        raw=block.strip()
        if spec['metadata']['return_type']==7:
            if raw.startswith("'") and raw.endswith("'"):raw=raw[1:-1]
            values.append(raw)
        else:
            words=re.sub(r'[\[\];,]',' ',raw).split();numbers=[float(v) for v in words]
            require(all(math.isfinite(v) for v in numbers),'Nonfinite raw parameter Write')
            if spec['metadata']['return_type']==4:require(len(numbers)==1,'Raw scalar width differs');values.append(numbers[0])
            else:values.append(numbers)
    return values


def contract(proof,spec):
    require(proof['checkpoint']=='live-function-burn-parameter-owner-binding' and proof['outcome']=='passed' and proof['case']==spec['factory_type'] and proof['family']==spec['family']=='ImpulsiveBurn' and proof['full_system_parameter_inventory']==321 and set(proof['initial_batch_types'])==TYPES and proof['missions']==3 and proof['function_calls']==12 and proof['live_callback_samples']==12 and proof['actual_parameter_owner_checks']==24 and proof['file_decisions']==[dict(saved=False),dict(saved=True)],'Live binding identity/count/edit differs')
    require([v['phase'] for v in proof['phases']]==[0,1,2] and [v['explicit_rebuild'] for v in proof['phases']]==[True,True,False],'Binding no-Build repeat omitted')
    require(proof['route']==('explicit-frame' if spec['frame_reference'] else 'bare-burn-frame') and proof['argument']==spec['argument'] and proof['executed_maneuver_commands']==24 and proof['target_state_components']==72,'Selected initialized/mutable route differs')
    meta=spec['metadata']
    for phase in proof['phases']:
        require(phase['raw_markers']==[42,43,44,103,104] and phase['callback_private_maps_accessed_only_while_executing'] and len(phase['live_samples'])==4 and len(phase['private_writes'])==len(phase['shared_writes'])==4,'Binding live callback/Write/marker contract differs')
        root_owners={}
        for index,sample in enumerate(phase['live_samples']):
            inside=index>0;calls=index
            require(sample['inside_function']==inside and sample['function']==('ReadShared' if inside else '') and sample['calls']==calls and sample['global_map_is_value_snapshot']==(not inside) and len(sample['parameters'])==2,'Live callback order/map provenance differs')
            require(sample['provider']==('Propagate PulseProp(ScopePulse) {ScopePulse.ElapsedSecs = 1};' if inside else 'Propagate PulseProp(MainPulse) {MainPulse.ElapsedSecs = 1};'),'Actual publication provider differs')
            target=sample['target_state'];state=[9000,100,200,-.1,7.5,.8];state[spec['axis']+3]+=expected(spec,False,inside,calls)[0]+expected(spec,True,inside,calls)[0]
            require(target['name']==('ScopePulse' if inside else 'MainPulse') and target['epoch']==21545 and target['mass']==(701 if inside else 702) and target['absolute_bound']==1e-12 and re.fullmatch('[0-9a-f]+',target['pointer']) and target['pointer']!='0','Actual executed maneuver target identity differs')
            compare(target['actual'],state,1e-12);compare(target['expected'],state,1e-12)
            for position,row in enumerate(sample['parameters']):
                shared=position==1;reference=spec['shared_reference'] if shared else spec['local_private_reference'] if inside else spec['root_private_reference']
                require(row['shared']==shared and row['reference']==reference and row['factory_type']==spec['factory_type'] and row['native_type']==meta['native_type'] and row['return_type']==meta['return_type'] and row['owner_type']==meta['owner_type_id'] and row['owner_name']==('SharedBurn' if shared else 'PrivateBurn') and row['owner_type_name']=='ImpulsiveBurn','Actual native Parameter/owner class differs')
                for key in ['parameter_pointer','reference_owner_pointer','live_owner_pointer','root_owner_pointer','configured_owner_pointer']:
                    require(re.fullmatch('[0-9a-f]+',row[key]) and row[key]!='0','Invalid actual native identity')
                require(row['reference_owner_pointer']==row['live_owner_pointer']!=row['configured_owner_pointer'] and row['owner_global']==shared and row['owner_local']==(inside and not shared),'Parameter reference/owner flags differ')
                if not inside:root_owners[shared]=row['root_owner_pointer']
                require(row['root_owner_pointer']==root_owners[shared] and (row['live_owner_pointer']==row['root_owner_pointer'])==(shared or not inside),'Private/global actual owner aliases caller incorrectly')
                value,bound,shape=expected(spec,shared,inside,calls)
                require(row['shape']==shape and row['absolute_bound']==bound,'Typed value shape/bounds changed')
                compare(row['actual'],value,bound);compare(row['expected'],value,bound);compare(row['direct_owner_property_value'],value,bound)
                compare(phase['shared_writes'][index] if shared else phase['private_writes'][index],row['actual'],bound)
                burn=row['burn_state'];delta=[0,0,0];delta[spec['axis']]=value
                require(burn['has_fired'] and burn['last_fire_epoch']==target['epoch'] and burn['internal_frame_name']=='InternalEarthMJ2000Eq' and burn['internal_frame_pointer']==burn['moderator_internal_frame_pointer'] and re.fullmatch('[0-9a-f]+',burn['internal_frame_pointer']) and burn['internal_frame_pointer']!='0','Actual burn fire/internal frame identity differs')
                compare(burn['delta_v_inertial'],delta,1e-12)
                if spec['frame_reference']:
                    frame=burn['out_frame'];require(frame['name']=='EarthMJ2000Eq' and frame['parameter_pointer']==frame['live_pointer'] and re.fullmatch('[0-9a-f]+',frame['live_pointer']) and frame['live_pointer']!='0','Actual explicit frame identity differs')
                else:require(burn['out_frame']=={},'Bare route gained unobserved output frame')

                if spec['family']=='Attached':
                    attached=row['attached'];require(attached['name']==('SharedTank' if shared else 'LocalTank' if inside else 'RootTank') and attached['type']=='ChemicalTank' and attached['fuel_mass']==value and re.fullmatch('[0-9a-f]+',attached['pointer']) and attached['pointer']!='0','Actual attached dependency differs')
                else:require(row['attached']=={},'Unexpected attached dependency claim')


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
    catalog=json.loads(visible['src/qtgui/tests/NasaFunctionBurnParameterCases.json']);metadata={v['factory_type']:v['metadata'] for v in catalog['cases']};specs={v['argument']:v for v in catalog['binding_cases']}
    require(len(metadata)==321 and set(catalog['initial_types'])==TYPES and len(specs)==12 and set(specs)=={k+v for k in TYPES for v in ['Bare','Frame']} and set(catalog['selected_arguments'])==set(specs) and not catalog['qualified_keys'] and all(not v['qualified'] for v in catalog['cases']+catalog['binding_cases']),'Complete321 catalog/bare-frame cases narrowed/promoted')
    matrix=load(bound(repo/catalog['baseline']));require(sha(repo/catalog['baseline'])==catalog['baseline_sha256'] and {v['factory_type']:v['native_parameter_metadata'] for v in matrix['rows'] if v['role']=='owner-bound-system-parameter'}==metadata,'Original321 metadata changed')
    for spec in specs.values():recipe(spec);require(spec['metadata']==metadata[spec['factory_type']] and spec['metadata']['return_type']==4 and spec['metadata']['owner_type_name']=='ImpulsiveBurn','Actual case owner/metadata changed')
    prior=load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-spacecraft-field-bindings/visible-inputs-before-v1.json'))['sources']
    for n in ['NasaFunctionParameterBindingTests.cpp','NasaFunctionSpacecraftFieldBindingTests.cpp','NasaFunctionVectorResourceTests.cpp','NasaSubscriberGlobalLoopTests.cpp','NasaFunctionMathResourceTests.cpp']:
        require(hashlib.sha256(visible['src/qtgui/tests/'+n]).hexdigest()==prior['src/qtgui/tests/'+n],'Previous assertion source changed')
    maps=load(bound(stage/'loaded-images-v1.json'))['processes'];launch=load(bound(stage/'visible-launch-v1.json'));require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==12,'Visible mapping incomplete')
    selected=[];failures=[];exposures=0
    for version in ['headless-v1','headless-v2','visible-v1']:
        invocation='function-burn-parameter-bindings-'+version;receipt=load(bound(repo/'build/gui-acceptance'/invocation/'results.json'));frozen=visible if version=='visible-v1' else capsule(version.replace('headless-','headless-executed-sources-')+'.tar.gz')
        require(receipt.get('finished_utc') and receipt['end_reason']=='completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest()==d for n,d in receipt['source_hashes'].items()),'Executed source/terminal state differs')
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};require(set(active)=={PREFIX+k for k in specs},'Exact native selection differs')
        for name,row in active.items():
            log=bound(row['log']);case=log.parent;tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'))
            if version=='headless-v1':
                require(row['status']=='failed' and row['returncode']!=0 and len(tests)==1 and tests[0].find('failure') is not None and tests[0].find('skipped') is None and 'Actual burn Parameter internal frame absent' in log.read_text() and 'internal->GetName()=="EarthMJ2000Eq"' in frozen['src/qtgui/tests/NasaFunctionBurnParameterBindingTests.cpp'].decode(),'Retained internal-frame fixture failure lost')
                failures.append(dict(invocation=invocation,test=name));continue
            require(row['status']=='passed' and row['returncode']==0 and len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and tests[0].find('failure') is None,'Native result/JUnit differs')
            emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())];require(len(emitted)==1 and load(bound(case/'proof.json'))==emitted[0],'Native emitted/stored proof differs');proof=emitted[0];spec=specs[name.removeprefix(PREFIX)];contract(proof,spec)
            original=bound(case/'initial.gmf').read_text();edited=bound(case/'edited.gmf').read_text();caller=bound(case/'caller.script').read_text();nested=bound(case/'nested.gmf').read_text()
            require(original.replace('Keep = 103;','Keep = 41;')==edited and 'Mark = Keep + Calls;' in edited and '[Mark] = ReadShared();' in nested and 'Create ImpulsiveBurn PrivateBurn;' in caller and 'Create ImpulsiveBurn PrivateBurn;' in edited and '.DecrementMass = false;' in caller+edited,'Exact editing/owner construction differs')
            require(all(v in edited for v in ['PrivateBurn.'+spec['setter']+' = 0.25 + Calls * 0.0625;','SharedBurn.'+spec['setter']+' = 0.5 + Calls * 0.0625;','Maneuver PrivateBurn(ScopePulse);','Maneuver SharedBurn(ScopePulse);']) and all(v in caller for v in ['Maneuver PrivateBurn(MainPulse);','Maneuver SharedBurn(MainPulse);']) and edited.count('Maneuver ')==caller.count('Maneuver ')==2 and ('.EarthMJ2000Eq.' in spec['shared_reference'])==spec['frame_reference'],'Actual burn mutations/maneuvers/frame route differs')
            prototypes=load(bound(case/'prototypes-before.json'))
            for phase in proof['phases']:
                tag=str(phase['phase']);invariants=load(bound(case/(tag+'-invariants.json')))
                require(load(bound(case/(tag+'-live-parameter-samples.json')))==phase['live_samples'] and load(bound(case/(tag+'-prototypes.json')))==prototypes and invariants['root_selected_property'] and all(invariants.values()) and [float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==phase['raw_markers'] and not bound(case/(tag+'-observer-error.txt')).read_text(),'Raw live/source/root invariants differ')
                messages=bound(case/(tag+'-messages.txt')).read_text();require(written(messages,spec,spec['root_private_reference'])==phase['private_writes'] and written(messages,spec,spec['shared_reference'])==phase['shared_writes'],'Raw scalar Write bytes differ')
                report=[float(v) for v in bound(case/(tag+'-binding-values.txt')).read_text().split()];compare(report,[row['actual'] for sample in phase['live_samples'] for row in sample['parameters']],1e-12)
            if version=='visible-v1':
                display=receipt['visible_display'];require(display['start_minimized'] and display['resizable'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible window policy differs')
                exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Visible trace absent');process=maps[traces[0].stem.removeprefix('trace-')];exes=[n for n in row['registered_command'] if n in receipt['binary_hashes']]
                require(len(exes)==1 and process['executable']==exes[0] and all(before['images'].get(n)==d for n,d in process['images'].items()),'Actual mapped image identity differs')
                for n,d in before['images'].items():
                    if n==exes[0] or Path(n).name.startswith(('libGmatBase','libGmatUtil')):require(process['images'].get(n)==d,'Actual executable/core image unmapped')
            selected.append(dict(invocation=invocation,test=name))
    require(len(selected)==24 and len(failures)==12,'Burn selected/failure count differs')
    require('cannot convert' in bound(stage/'build-v1.log').read_text() and bound(stage/'NasaFunctionBurnParameterBindingTests.compile-v1.cpp').is_file(),'Compile failure/source lost')
    proofs={key:load(bound(repo/'build/gui-acceptance/function-burn-parameter-bindings-visible-v1'/(PREFIX+key)/'proof.json')) for key in ['Element1Bare','Element1Frame']};controls=[('configured-owner','Element1Bare',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('reference_owner_pointer',p['phases'][0]['live_samples'][1]['parameters'][0]['configured_owner_pointer'])),('caller-owner','Element1Bare',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('live_owner_pointer',p['phases'][0]['live_samples'][1]['parameters'][0]['root_owner_pointer'])),('native-class','Element1Bare',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('native_type','Variable')),('output-frame','Element1Frame',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['burn_state']['out_frame'].__setitem__('parameter_pointer','0')),('internal-frame','Element1Bare',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['burn_state'].__setitem__('internal_frame_pointer','0')),('unfired-owner','Element1Bare',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['burn_state'].__setitem__('has_fired',False)),('inertial-delta','Element1Bare',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['burn_state']['delta_v_inertial'].__setitem__(0,0)),('target-velocity','Element1Bare',lambda p:p['phases'][0]['live_samples'][1]['target_state']['actual'].__setitem__(3,0)),('omitted-no-Build-repeat','Element1Bare',lambda p:p['phases'].pop()),('typed-Write','Element1Bare',lambda p:p['phases'][0]['private_writes'].__setitem__(1,0))];negatives=[]
    for label,key,corrupt in controls:
        bad=copy.deepcopy(proofs[key]);corrupt(bad)
        try:contract(bad,specs[key])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt burn binding '+label)
    return dict(status='passed-six-live-burn-parameter-types-twelve-bare-frame-routes',qualified_keys=[],selected=selected,new_binding_GUI_passes=24,retained_GUI_failures=failures,retained_compile_failures=1,system_parameters_retained=321,additional_native_bound_types=6,bare_frame_arguments=12,cumulative_native_bound_types=50,remaining_without_live_native_binding=271,binding_missions=72,function_calls=288,live_callback_samples=288,function_callback_samples=216,actual_parameter_owner_checks=576,direct_owner_property_checks=576,actual_typed_Write_values=576,actual_Report_values=576,actual_maneuver_commands=576,immediate_target_state_components=1728,burn_inertial_delta_components=1728,explicit_output_frame_checks=288,internal_frame_identity_checks=576,root_selected_property_checks=72,marker_scalars=360,visible_mapped_processes=12,visible_exposure_events=exposures,negative_controls=negatives,production_changed_files=[],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--stage',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();result=audit(Path(__file__).resolve().parents[3],args.stage.resolve());args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
