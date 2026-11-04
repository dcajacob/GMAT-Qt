"""Audit nine rotated-hardware read bindings and retain strict write-contract failures."""
import argparse,copy,hashlib,json,math,re,runpy,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha
from AuditFunctionParameterBindings import compare,written
TYPES={f'R_SB{i}{j}' for i in range(1,4) for j in range(1,4)}
PREFIX='QtGui.NasaFunctionRotationParameterBinding';SUFFIX='WritableProbe';PILOT={'R_SB11'}
GEOMETRY=[([1,2,2],[-2,-1,2],[2/3,-2/3,1/3,2/3,1/3,-2/3,1/3,2/3,2/3]),([6,22,3],[-13,6,-18],[-18/23,3/23,14/23,13/23,-6/23,18/23,6/23,22/23,3/23]),([7,26,2],[-22,7,-14],[-14/27,2/27,23/27,22/27,-7/27,14/27,7/27,26/27,2/27])]


def contract(p,spec):
    key=spec['factory_type'];meta=spec['metadata'];index=(int(key[-2])-1)*3+int(key[-1])-1
    require(spec['matrix_index']==index and spec['step']==0 and spec['absolute_bound']==1e-12 and spec['hardware_kind']=='Tank' and spec['hardware_type']=='ChemicalTank','Rotation recipe identity differs')
    for scope,(z,v,matrix) in zip(['root','private','shared'],GEOMETRY):
        require(spec[scope+'_primary']==z and spec[scope+'_secondary']==v and spec[scope+'_matrix']==matrix and spec[scope+'_value']==matrix[index],'Independent rational geometry recipe differs')
    require(len({spec['root_value'],spec['private_value'],spec['shared_value']})==3,'Selected rotation scalar scopes are not distinguishable')
    require(p['checkpoint']=='live-function-rotation-parameter-owner-binding' and p['outcome']=='passed' and p['case']==p['argument']==key and p['route']=='attached-hardware-rotation-field' and p['family']=='AttachedHardwareRotation' and p['writable_probe'] is False and p['factory_settable'] is True and p['full_system_parameter_inventory']==321 and set(p['initial_batch_types'])==TYPES and p['missions']==3 and p['function_calls']==p['live_callback_samples']==12 and p['actual_parameter_owner_checks']==24 and p['file_decisions']==[dict(saved=False),dict(saved=True)],'Rotation proof identity/counts differ')
    require([v['phase'] for v in p['phases']]==[0,1,2] and [v['explicit_rebuild'] for v in p['phases']]==[True,True,False],'Rotation no-Build repeat lost')
    for phase in p['phases']:
        require(phase['raw_markers']==[42,43,44,103,104] and phase['callback_private_maps_accessed_only_while_executing'] and len(phase['live_samples'])==len(phase['private_writes'])==len(phase['shared_writes'])==4,'Rotation callback/Write/marker census differs');roots={};hardware_roots={}
        for i,sample in enumerate(phase['live_samples']):
            inside=i>0;target='ScopePulse' if inside else 'MainPulse'
            require(sample['inside_function']==inside and sample['function']==('ReadShared' if inside else '') and sample['calls']==i and sample['global_map_is_value_snapshot']==(not inside) and len(sample['parameters'])==2 and sample['provider']==f'Propagate PulseProp({target}) {{{target}.ElapsedSecs = 1}};','Rotation scope/provider differs')
            for j,row in enumerate(sample['parameters']):
                shared=j==1;z,v,matrix=GEOMETRY[2 if shared else 1 if inside else 0];value=matrix[index];reference=spec['shared_reference'] if shared else spec['local_private_reference'] if inside else spec['root_private_reference']
                require(row['shared']==shared and row['reference']==reference and row['factory_type']==key and row['native_type']==meta['native_type'] and row['return_type']==meta['return_type']==4 and row['owner_type']==meta['owner_type_id']==101 and row['owner_type_name']=='Spacecraft' and row['owner_name']==('SharedSC' if shared else 'PrivateOnly'),'Rotation Parameter/owner type differs')
                for field in ['parameter_pointer','reference_owner_pointer','live_owner_pointer','root_owner_pointer','configured_owner_pointer']:require(re.fullmatch('[0-9a-f]+',row[field]) and row[field]!='0','Invalid actual rotation owner identity')
                require(row['reference_owner_pointer']==row['live_owner_pointer']!=row['configured_owner_pointer'] and row['owner_global']==shared and row['owner_local']==(inside and not shared),'Rotation reference/owner flags differ')
                if not inside:roots[shared]=row['root_owner_pointer']
                require(row['root_owner_pointer']==roots[shared] and (row['live_owner_pointer']==roots[shared])==(shared or not inside),'Rotation owner aliases wrong root')
                require(row['shape']==[] and row['absolute_bound']==1e-12,'Rotation scalar shape/bound differs')
                for field in ['actual','expected','direct_owner_property_value']:compare(row[field],value,1e-12)
                compare((phase['shared_writes'] if shared else phase['private_writes'])[i],value,1e-12)
                a=row['attached'];name=('Shared' if shared else 'Local' if inside else 'Root')+'Tank'
                require(a['name']==name and a['type']=='ChemicalTank' and a['property']==key and a['primary']==z and a['secondary']==v and a['location']==[0,0,0] and a['rotation_option']=='Construct','Actual rotation tank/input fixture differs')
                for field in ['pointer','root_pointer','configured_pointer']:require(re.fullmatch('[0-9a-f]+',a[field]) and a[field]!='0','Invalid actual rotation tank identity')
                if not inside:hardware_roots[shared]=a['root_pointer']
                require(a['root_pointer']==hardware_roots[shared] and a['pointer']!=a['configured_pointer'] and (a['pointer']==hardware_roots[shared])==(shared or not inside),'Rotation tank clone aliases wrong root')
                for field in ['R_SB','RotationMatrix','post_matrix']:compare(a[field],matrix,1e-12)
                for field in ['direct_value','post_value']:compare(a[field],value,1e-12)
                compare(a['determinant'],1,1e-12)
                for x in range(3):
                    for y in range(3):require(abs(sum(a['R_SB'][x*3+k]*a['R_SB'][y*3+k] for k in range(3))-(1 if x==y else 0))<=1e-12,'Observed hardware rotation not orthonormal')
                require(meta['settable'] is True and a['runtime_settable'] is True and a['info_settable'] is True and a['native_field_readonly'] is True,'Original writable advertisement/native readonly discrepancy changed')
                # This is an observation of the recorded build, not the desired
                # setter contract. Native read tests accept a future idempotent
                # setter success; the separate strict probes require success.
                require(a['native_setter_result']=='rejected' and 'SpacecraftData::SetReal() Not settable or unknown item id:' in a['native_setter_error'] and a['post_matrix']==a['R_SB'] and a['post_value']==row['actual'],'Recorded unsupported setter or owner/reread preservation differs')


def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,d in bindings.items():require(Path(n).is_file() and not Path(n).is_symlink() and sha(n)==d,'Changed regular binding '+n)
    def bound(p):
        p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def capsule(name):
        with tarfile.open(bound(stage/name)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source member');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible=capsule('visible-executed-sources-v2.tar.gz');before=load(bound(stage/'visible-inputs-before-v2.json'));require(before==load(bound(stage/'visible-inputs-after-v2.json')) and before['sources']=={n:hashlib.sha256(v).hexdigest() for n,v in visible.items()},'Visible inputs changed')
    for n in ['AuditFunctionRotationParameterBindings.py','AuditFunctionParameterBindings.py','BuildNasaCoverageExtension.py']:require(hashlib.sha256(visible['src/qtgui/tests/'+n]).hexdigest()==sha(repo/'src/qtgui/tests'/n),'Auditor/helper changed')
    catalog=json.loads(visible['src/qtgui/tests/NasaFunctionRotationParameterCases.json']);specs={v['factory_type']:v for v in catalog['rotation_binding_cases']}
    require(len(catalog['cases'])==321 and set(specs)==set(catalog['initial_types'])==TYPES and set(catalog['selected_arguments'])==TYPES|{v+SUFFIX for v in TYPES} and not catalog['qualified_keys'] and all(not v['qualified'] for v in catalog['cases']),'Complete321 or strict probe registrations narrowed/promoted')
    baseline=load(bound(repo/catalog['baseline']));require(sha(repo/catalog['baseline'])==catalog['baseline_sha256'] and {v['factory_type']:v['native_parameter_metadata'] for v in baseline['rows'] if v['role']=='owner-bound-system-parameter'}=={v['factory_type']:v['metadata'] for v in catalog['cases']},'Original321 metadata/roles changed')
    maps=load(bound(stage/'loaded-images-v2.json'))['processes'];launch=load(bound(stage/'visible-launch-v2.json'));require(launch['returncode']==1 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==18,'Visible completed selection/maps differ')
    selected,failures,exposures,failed_exposures=[],[],0,0
    for version in ['headless-v3','visible-v2']:
        inv='function-rotation-parameter-bindings-'+version;q=load(bound(repo/'build/gui-acceptance'/inv/'results.json'));frozen=visible if version=='visible-v2' else capsule(version.replace('headless-','headless-executed-sources-')+'.tar.gz')
        require(q.get('finished_utc') and q['end_reason']=='completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest()==d for n,d in q['source_hashes'].items()),'Executed source/terminal state differs')
        rows={v['name']:v for v in q['results'] if v['status']!='not-run'};types=TYPES
        require(set(rows)=={PREFIX+k+suffix for k in types for suffix in ['',SUFFIX]},'Exact rotation read/write selection differs')
        for name,row in rows.items():
            probe=name.endswith(SUFFIX);key=name.removeprefix(PREFIX).removesuffix(SUFFIX);case=bound(row['log']).parent;spec=specs[key];item=dict(invocation=inv,test=name)
            require(row['status']==('failed' if probe else 'passed') and (row['returncode']!=0)==probe,'Rotation native result differs')
            tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is not None)==probe,'Rotation native JUnit differs')
            if probe:
                messages=bound(case/'0-messages.txt').read_text();require('SpacecraftData::SetReal() Not settable or unknown item id:' in messages and 'In PrivateOnly.LocalTank.'+key+' =' in messages and 'Binding scope mission/callback census failed' in bound(row['log']).read_text(),'Strict writable probe rejection lost')
                edited=bound(case/'edited.gmf').read_text()
                for ref,expected in [(spec['local_private_reference'],spec['private_value']),(spec['shared_reference'],spec['shared_value'])]:
                    values=re.findall(re.escape(ref)+r' = ([^;]+);',edited);require(len(values)==1 and float(values[0])==expected,'Idempotent strict write statement changed')
                require(edited.index(spec['local_private_reference']+' =')<edited.index(spec['shared_reference']+' ='),'Failure order/global-unreached scope changed')
                samples=load(bound(case/'0-live-parameter-samples.json'));events=load(bound(case/'0-callback-events.json'));require(len(samples)==len(events)==1 and samples[0]['inside_function'] is False and samples[0]['calls']==0 and len(samples[0]['parameters'])==2,'Probe failed at a different callback boundary');failures.append(item)
            else:
                emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',bound(row['log']).read_text())];require(len(emitted)==1 and load(bound(case/'proof.json'))==emitted[0],'Emitted/stored rotation proof differs');p=emitted[0];contract(p,spec)
                require(load(bound(case/'phases.json'))==p['phases'],'Raw rotation phases differ');initial,edited=bound(case/'initial.gmf').read_text(),bound(case/'edited.gmf').read_text();require(initial.count('Keep = 103;')==1 and initial.replace('Keep = 103;','Keep = 41;')==edited and spec['local_private_reference']+' =' not in edited,'Read function edit or write scope differs')
                for phase in p['phases']:
                    tag=str(phase['phase']);messages=bound(case/(tag+'-messages.txt')).read_text();require(load(bound(case/(tag+'-live-parameter-samples.json')))==phase['live_samples'] and all(load(bound(case/(tag+'-invariants.json'))).values()) and load(bound(case/(tag+'-prototypes.json')))==load(bound(case/'prototypes-before.json')),'Raw rotation samples/invariants/prototypes differ')
                    require(written(messages,spec,spec['root_private_reference'])+written(messages,spec,spec['local_private_reference'])==phase['private_writes'] and written(messages,spec,spec['shared_reference'])==phase['shared_writes'],'Raw rotation Write differs');compare([float(v) for v in bound(case/(tag+'-binding-values.txt')).read_text().split()],[v['actual'] for sample in phase['live_samples'] for v in sample['parameters']],1e-12);require([float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==phase['raw_markers'],'Raw rotation markers differ')
                selected.append(item)
            if version=='visible-v2':
                d=q['visible_display'];require(d['start_minimized'] and d['resizable'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'] and q['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible policy differs');exposures+=row['trace']['native_exposure_events'];failed_exposures+=row['trace']['native_exposure_events'] if probe else 0
                traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Visible trace absent');mapped=maps[traces[0].stem.removeprefix('trace-')];exes=[n for n in row['registered_command'] if n in q['binary_hashes']];require(len(exes)==1 and mapped['executable']==exes[0] and all(before['images'].get(n)==d for n,d in mapped['images'].items()),'Actual mapped image differs')
                for n,d in before['images'].items():
                    if n==exes[0] or Path(n).name.startswith(('libGmatBase','libGmatUtil')):require(mapped['images'].get(n)==d,'Required image unmapped')
    require(len(selected)==len(failures)==18,'Read pass/current strict failure census differs')
    # Earlier passes retain their exact, narrower scalar-stamp fixture. They
    # are checked with their archived contract and never promoted to the new
    # pairwise-distinct scalar guarantee.
    legacy_visible=capsule('visible-executed-sources-v1.tar.gz');legacy_before=load(bound(stage/'visible-inputs-before-v1.json'));require(legacy_before==load(bound(stage/'visible-inputs-after-v1.json')) and legacy_before['sources']=={n:hashlib.sha256(v).hexdigest() for n,v in legacy_visible.items()},'Historical visible custody changed')
    legacy_source=bound(stage/'auditor-before-distinct-stamps-v1.py');require(sha(legacy_source)==hashlib.sha256(legacy_visible['src/qtgui/tests/AuditFunctionRotationParameterBindings.py']).hexdigest(),'Archived historical contract changed');legacy_contract=runpy.run_path(str(legacy_source))['contract'];legacy_catalog=json.loads(legacy_visible['src/qtgui/tests/NasaFunctionRotationParameterCases.json']);legacy_specs={v['factory_type']:v for v in legacy_catalog['rotation_binding_cases']}
    superseded=[];historical_failures=[];historical_maps=load(bound(stage/'loaded-images-v1.json'))['processes'];historical_launch=load(bound(stage/'visible-launch-v1.json'));require(historical_launch['returncode']==1 and historical_launch['inputs_unchanged'] and historical_launch['mapped_processes']==len(historical_maps)==18,'Historical visible map/selection differs')
    for version in ['headless-v1','headless-v2','visible-v1']:
        inv='function-rotation-parameter-bindings-'+version;q=load(bound(repo/'build/gui-acceptance'/inv/'results.json'));frozen=legacy_visible if version=='visible-v1' else capsule(version.replace('headless-','headless-executed-sources-')+'.tar.gz')
        require(q.get('finished_utc') and q['end_reason']=='completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest()==d for n,d in q['source_hashes'].items()),'Historical executed sources/terminal state changed')
        keys=PILOT if version=='headless-v1' else TYPES-PILOT if version=='headless-v2' else TYPES;rows={v['name']:v for v in q['results'] if v['status']!='not-run'};require(set(rows)=={PREFIX+k+suffix for k in keys for suffix in ['',SUFFIX]},'Historical exact selection differs')
        for name,row in rows.items():
            probe=name.endswith(SUFFIX);key=name.removeprefix(PREFIX).removesuffix(SUFFIX);case=bound(row['log']).parent;spec=legacy_specs[key];item=dict(invocation=inv,test=name)
            require(row['status']==('failed' if probe else 'passed') and (row['returncode']!=0)==probe,'Historical result relabelled');tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is not None)==probe,'Historical JUnit relabelled')
            if probe:
                messages=bound(case/'0-messages.txt').read_text();require('SpacecraftData::SetReal() Not settable or unknown item id:' in messages and 'In PrivateOnly.LocalTank.'+key+' =' in messages and len(load(bound(case/'0-live-parameter-samples.json')))==1,'Historical strict failure lost');historical_failures.append(item)
            else:
                p=load(bound(case/'proof.json'));emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',bound(row['log']).read_text())];require(emitted==[p],'Historical emitted/stored proof differs');legacy_contract(p,spec)
                for phase in p['phases']:
                    tag=str(phase['phase']);compare([float(v) for v in bound(case/(tag+'-binding-values.txt')).read_text().split()],[v['actual'] for sample in phase['live_samples'] for v in sample['parameters']],1e-12)
                superseded.append(item)
            if version=='visible-v1':
                traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Historical visible trace differs');mapped=historical_maps[traces[0].stem.removeprefix('trace-')];exes=[n for n in row['registered_command'] if n in q['binary_hashes']];require(len(exes)==1 and mapped['executable']==exes[0] and all(legacy_before['images'].get(n)==d for n,d in mapped['images'].items()),'Historical mapped image changed')
    require(len(superseded)==len(historical_failures)==18,'Historical pass/failure census differs')
    failures=historical_failures+failures
    template=load(bound(repo/'build/gui-acceptance/function-rotation-parameter-bindings-visible-v2'/(PREFIX+'R_SB11')/'proof.json'))
    def row(p):return p['phases'][0]['live_samples'][1]['parameters'][0]
    controls=[('wrong-reference-owner',lambda p:row(p).__setitem__('reference_owner_pointer','1')),('private-tank-aliases-root',lambda p:row(p)['attached'].__setitem__('pointer',row(p)['attached']['root_pointer'])),('wrong-class',lambda p:row(p)['attached'].__setitem__('type','ChemicalThruster')),('wrong-matrix-component',lambda p:row(p)['attached']['R_SB'].__setitem__(0,0)),('wrong-primary',lambda p:row(p)['attached']['primary'].__setitem__(0,99)),('wrong-rational-value',lambda p:row(p).__setitem__('actual',999)),('writable-flag-reclassified',lambda p:row(p)['attached'].__setitem__('runtime_settable',False)),('fake-setter-success',lambda p:row(p)['attached'].__setitem__('native_setter_result','accepted')),('changed-reread-value',lambda p:row(p)['attached'].__setitem__('post_value',999)),('changed-owner-matrix',lambda p:row(p)['attached']['post_matrix'].__setitem__(0,0)),('missing-no-Build',lambda p:p['phases'].pop()),('wrong-marker',lambda p:p['phases'][0]['raw_markers'].__setitem__(0,999))];negatives=[]
    for label,corrupt in controls:
        p=copy.deepcopy(template);corrupt(p)
        try:contract(p,specs['R_SB11'])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt rotation proof '+label)
    return dict(status='passed-9-rotation-read-bindings-with-unresolved-writable-probes',qualified_keys=[],selected=selected,superseded_read_passes=superseded,retained_writable_probe_failures=failures,current_writable_probe_failures=18,historical_writable_probe_failures=18,retained_compile_failures=0,retained_interrupted_cases=[],new_parameter_types=9,missions=54,function_calls=216,live_callback_samples=216,parameter_owner_checks=432,actual_attached_tank_checks=432,direct_rotation_scalar_checks=432,full_R_SB_scalar_components=3888,full_RotationMatrix_scalar_components=3888,literal_direction_components=2592,determinant_checks=432,native_setter_observations_in_read_proofs=432,observed_native_setter_refusals_in_read_proofs=432,actual_Write_values=432,actual_Report_values=432,marker_values=270,visible_mapped_processes=18,historical_visible_mapped_processes=18,visible_read_processes=9,visible_failed_write_processes=9,visible_exposure_events=exposures,visible_failed_write_exposure_events=failed_exposures,negative_controls=negatives,native_bound_types=147,remaining_without_live_native_binding=174,factory_or_readable_role_exclusions=[],production_changed_files=[],metadata_reclassified=False,script_write_contract_qualified=False,Global_script_write_statements_reached=False,raw_parameter_cache_preservation_directly_observed=False,direction_change_refresh_qualified=False,arbitrary_degenerate_orientation_or_translation_science_qualified=False,whole_scope_qualified=False,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
