"""Audit live attitude owner references, typed recipes and separate write paths."""
import argparse,copy,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha
from AuditFunctionParameterBindings import compare,written
from FunctionAttitudeBindingOracle import TYPES,WRITABLE_TYPES,SCOPES,recipe
PREFIX='QtGui.NasaFunctionAttitudeParameterBinding'
SUFFIX='WritableProbe';NATIVE_SUFFIX='NativeWritableProbe'


def contract(p,spec,allow_old_MRP_refusals=False):
    key=spec['factory_type'];meta=spec['metadata'];arg=p['argument'];native=arg.endswith(NATIVE_SUFFIX);script=arg.endswith(SUFFIX) and not native
    require(key in TYPES and meta['settable']==(key in WRITABLE_TYPES) and spec['absolute_bound']==1e-10 and spec['step']==0 and {s:{k:v for k,v in rc.items() if k!='native_mutation_expected'} for s,rc in spec['scope_recipes'].items()}=={s:recipe(s,key) for s in SCOPES},'Attitude independent recipe/bounds/metadata differ')
    require(p['checkpoint']=='live-function-attitude-parameter-owner-binding' and p['outcome']=='passed' and p['case']==key and arg==key+(NATIVE_SUFFIX if native else SUFFIX if script else '') and p['writable_probe']==script and p.get('native_writable_probe',False)==native and p['route']=='attached-attitude-field' and p['family']=='AttachedAttitude' and p['full_system_parameter_inventory']==321 and set(p['initial_batch_types'])==set(TYPES) and p['missions']==3 and p['function_calls']==p['live_callback_samples']==12 and p['actual_parameter_owner_checks']==24 and p['file_decisions']==[dict(saved=False),dict(saved=True)],'Attitude proof identity/counts differ')
    require([v['phase'] for v in p['phases']]==[0,1,2] and [v['explicit_rebuild'] for v in p['phases']]==[True,True,False],'Attitude no-Build repeat lost')
    for phase in p['phases']:
        require(phase['raw_markers']==[42,43,44,103,104] and phase['callback_private_maps_accessed_only_while_executing'] and len(phase['live_samples'])==len(phase['private_writes'])==len(phase['shared_writes'])==4,'Attitude callback/output/marker census differs')
        roots={};attitudes={};frame_pointer=None;solar_pointer=None
        for calls,sample in enumerate(phase['live_samples']):
            inside=calls>0;target='ScopePulse' if inside else 'MainPulse'
            require(sample['inside_function']==inside and sample['function']==('ReadShared' if inside else '') and sample['calls']==calls and sample['provider']==f'Propagate PulseProp({target}) {{{target}.ElapsedSecs = 1}};' and sample['global_map_is_value_snapshot']==(not inside) and len(sample['parameters'])==2,'Attitude live provider/scope differs')
            for j,row in enumerate(sample['parameters']):
                shared=j==1;scope='shared' if shared else 'private' if inside else 'root';rc=recipe(scope,key);value=rc['expected'];ref=spec['shared_reference'] if shared else spec['local_private_reference'] if inside else spec['root_private_reference'];shape=[4,1] if key=='Quaternion' else [3,3] if key=='DirectionCosineMatrix' else []
                require(row['shared']==shared and row['reference']==ref and row['factory_type']==key and row['native_type']==meta['native_type'] and row['return_type']==meta['return_type'] and row['owner_type']==meta['owner_type_id']==101 and row['owner_name']==('SharedSC' if shared else 'PrivateOnly') and row['owner_type_name']=='Spacecraft' and row['shape']==shape and row['absolute_bound']==1e-10,'Attitude Parameter/owner metadata differs')
                for field in ['parameter_pointer','reference_owner_pointer','live_owner_pointer','root_owner_pointer','configured_owner_pointer']:require(re.fullmatch('[0-9a-f]+',row[field]) and row[field]!='0','Invalid actual attitude owner identity')
                require(row['reference_owner_pointer']==row['live_owner_pointer']!=row['configured_owner_pointer'] and row['owner_global']==shared and row['owner_local']==(inside and not shared),'Actual Parameter attitude owner reference/flags differ')
                if calls==0:roots[shared]=row['root_owner_pointer']
                require(row['root_owner_pointer']==roots[shared] and (row['live_owner_pointer']==roots[shared])==(shared or not inside),'Attitude owner aliases wrong root')
                for field in ['actual','expected','direct_owner_property_value']:compare(row[field],value,1e-10)
                compare((phase['shared_writes'] if shared else phase['private_writes'])[calls],value,1e-10)
                a=row['attached'];require(a['type']==rc['model'] and a['property']==key and a['owner_epoch']==21545 and a['euler_sequence']=='321' and a['runtime_settable']==a['info_settable']==meta['settable'] and a['owner_orbit_propagated'] is False,'Attached attitude model/epoch/sequence/metadata differs')
                for field in ['pointer','root_pointer','configured_pointer','frame_pointer','root_frame_pointer','solar_system_pointer']:require(re.fullmatch('[0-9a-f]+',a[field]) and a[field]!='0','Invalid attached attitude/frame/SolarSystem identity')
                if calls==0:attitudes[shared]=a['root_pointer']
                require(a['root_pointer']==attitudes[shared] and a['pointer']!=a['configured_pointer'] and (a['pointer']==attitudes[shared])==(shared or not inside) and a['frame_name']=='EarthMJ2000Eq' and a['frame_pointer']==a['root_frame_pointer'],'Actual attitude clone/frame reference differs')
                if frame_pointer is None:frame_pointer=a['frame_pointer'];solar_pointer=a['solar_system_pointer']
                require(a['frame_pointer']==frame_pointer and a['solar_system_pointer']==solar_pointer,'Live default frame/SolarSystem identity changed')
                if 'function_solar_system_pointer' in a:require(a['function_solar_system_pointer']==(solar_pointer if inside else '0'),'Actual Function SolarSystem differs')
                if rc['model']=='NadirPointing':require(a['body_pointer']!='0' and a.get('expected_live_body_pointer',a['body_pointer'])==a['body_pointer'],'NadirPointing body does not bind actual live SolarSystem')
                else:require(a['body_pointer']=='0','Unexpected body identity claim for non-Nadir fixture')
                unit='deg/sec' if key.startswith(('AngularVelocity','EulerAngleRate')) else 'deg' if key.startswith('EulerAngle') else ''
                require(a['unit']==unit and a['native_setter_owner_identity_preserved'] is True,'Attitude unit/owner preservation differs')
                compare(a['direct_value'],value,1e-10);compare(a['post_native_setter_value'],value,1e-10)
                if not meta['settable']:require(a['native_setter_result']=='not-probed-readonly' and not a['native_setter_error'],'Readonly native probe scope changed')
                elif allow_old_MRP_refusals and key in {'MRP1','MRP2','MRP3'}:require(a['native_setter_result']=='rejected' and 'AttitudeData::SetReal() Not settable or unknown item id:' in a['native_setter_error'],'Original native MRP refusal lost')
                else:require(a['native_setter_result']=='accepted' and not a['native_setter_error'],'Current native writable setter not accepted')
                if native:
                    require(key in {'MRP1','MRP2','MRP3'},'Native mutation probe selected unrelated type');change=a['native_mutation'];index=int(key[-1])-1;mrp=[rc['quaternion'][i]/(1+rc['quaternion'][3]) for i in range(3)];mrp[index]+=.0625;n=sum(v*v for v in mrp);q=[2*v/(1+n) for v in mrp]+[(1-n)/(1+n)];x,y,z,w=q;matrix=[1-2*(y*y+z*z),2*(x*y+z*w),2*(x*z-y*w),2*(x*y-z*w),1-2*(x*x+z*z),2*(y*z+x*w),2*(x*z+y*w),2*(y*z-x*w),1-2*(x*x+y*y)]
                    require(change['delta']==.0625 and change['owner_identity_preserved'],'Native MRP mutation recipe/owner differs');compare(change['target'],value+.0625,1e-10);compare(change['parameter_value'],value+.0625,1e-10);compare(change['direct_value'],value+.0625,1e-10);compare(change['changed_matrix'],matrix,1e-10);compare(change['restored_value'],value,1e-10);compare(change['restored_matrix'],rc['matrix'],1e-10)
                elif 'native_mutation' in a:require(a['native_mutation']=={},'Read/script case claims native mutation')


def vector_contract(p,spec):
    from AuditFunctionThrustAngleParameterBindings import contract as prior_contract
    require(p['checkpoint']=='live-function-vector-assignment-owner-binding' and p['route']=='Rvector-to-Rvector','Vector assignment route differs')
    checked=copy.deepcopy(p);checked['checkpoint']='live-function-thrust-angle-parameter-owner-binding';prior_contract(checked,spec)
    for phase in p['phases']:
        for calls,sample in enumerate(phase['live_samples']):
            for j,row in enumerate(sample['parameters']):
                shared=j==1;a=row['vector_source'];value=[(30 if shared else 20 if calls else 10)+(calls*(2 if shared else 1) if calls else 0)+.125*k for k in range(p['length'])]
                require(a['reference']==('SharedSource.' if shared else 'PrivateSource.')+p['case'] and a['reference_owner_pointer']==a['owner_pointer']!=row['live_owner_pointer'] and a['parameter_pointer']!=row['parameter_pointer'] and a['owner_local']==(calls>0) and a['owner_global'] is False and a['return_type']==11 and a['owner_type']==172,'Actual Rvector RHS Parameter/owner differs')
                for field in ['parameter_pointer','reference_owner_pointer','owner_pointer']:require(re.fullmatch('[0-9a-f]+',a[field]) and a[field]!='0','Invalid actual Rvector source identity')
                compare(a['actual'],value,1e-12);compare(a['direct'],value,1e-12)


def historical_contract(p,spec):
    require(p['checkpoint']=='live-function-attitude-parameter-owner-binding' and p['outcome']=='passed' and p['missions']==3 and p['actual_parameter_owner_checks']==24 and len(p['phases'])==3,'Historical pilot identity/census differs')
    for phase in p['phases']:
        require(phase['raw_markers']==[42,43,44,103,104] and len(phase['live_samples'])==4,'Historical pilot callback/markers differ')
        for calls,sample in enumerate(phase['live_samples']):
            for j,row in enumerate(sample['parameters']):
                rc=recipe('shared' if j else 'private' if calls else 'root',p['case']);a=row['attached']
                require(row['reference_owner_pointer']==row['live_owner_pointer']!=row['configured_owner_pointer'] and a['pointer']!=a['configured_pointer'] and a['type']==rc['model'] and a['frame_pointer']==a['root_frame_pointer'],'Historical pilot owner/attitude/frame identity differs')
                compare(row['actual'],rc['expected'],1e-10);compare(a['direct_value'],rc['expected'],1e-10)


def leaf_script_assignments(proof):
    return 2*sum(sample['inside_function'] for phase in proof['phases'] for sample in phase['live_samples']) if proof['writable_probe'] else 0


def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,d in bindings.items():require(Path(n).is_file() and not Path(n).is_symlink() and sha(n)==d,'Changed regular binding '+n)
    def bound(p):
        p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def capsule(name):
        with tarfile.open(bound(stage/name)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source member');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible=capsule('visible-executed-sources-v1.tar.gz');before=load(bound(stage/'visible-inputs-before-v1.json'));require(before==load(bound(stage/'visible-inputs-after-v1.json')) and before['sources']=={n:hashlib.sha256(v).hexdigest() for n,v in visible.items()},'Visible inputs changed')
    revision=load(bound(stage/'auditor-revision-review-v1.json'));require(visible['src/qtgui/tests/AuditFunctionAttitudeParameterBindings.py']==bound(stage/'auditor-visible-preimage-v1.py').read_bytes() and revision['inventoried_preimage_sha256']==hashlib.sha256(visible['src/qtgui/tests/AuditFunctionAttitudeParameterBindings.py']).hexdigest() and revision['final_sha256']==sha(repo/'src/qtgui/tests/AuditFunctionAttitudeParameterBindings.py') and revision['native_driver_or_assertion_changed'] is False,'Offline auditor version custody differs')
    for n in ['FunctionAttitudeBindingOracle.py','AuditFunctionParameterBindings.py','AuditFunctionThrustAngleParameterBindings.py','BuildNasaCoverageExtension.py']:require(hashlib.sha256(visible['src/qtgui/tests/'+n]).hexdigest()==sha(repo/'src/qtgui/tests'/n),'Imported helper changed')
    catalog=json.loads(visible['src/qtgui/tests/NasaFunctionAttitudeParameterCases.json']);specs={v['factory_type']:v for v in catalog['attitude_binding_cases']};arguments=set(TYPES)|{v+SUFFIX for v in WRITABLE_TYPES}|{v+NATIVE_SUFFIX for v in ['MRP1','MRP2','MRP3']};require(len(catalog['cases'])==321 and set(specs)==set(catalog['initial_types'])==set(TYPES) and set(catalog['selected_arguments'])==arguments and len(arguments)==67 and not catalog['qualified_keys'] and all(not v['qualified'] for v in catalog['cases']),'Complete321 narrowed/promoted')
    baseline=load(bound(repo/catalog['baseline']));require(sha(repo/catalog['baseline'])==catalog['baseline_sha256'] and {v['factory_type']:v['native_parameter_metadata'] for v in baseline['rows'] if v['role']=='owner-bound-system-parameter'}=={v['factory_type']:v['metadata'] for v in catalog['cases']},'Original metadata/roles changed')
    vc=json.loads(visible['src/qtgui/tests/NasaFunctionThrustAngleParameterCases.json']);vspecs={v['argument']:v for v in vc['thrust_angle_binding_cases']};vargs=set(vc['selected_arguments']);require(len(vargs)==6 and len(vc['cases'])==321,'Vector catalog narrowed')
    maps=load(bound(stage/'loaded-images-v1.json'))['processes'];launch=load(bound(stage/'visible-launch-v1.json'));require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==73,'Visible exact selection/maps differ')
    selected,superseded,failures=[],[],[];exposures=0;script_assignments=0
    pilot={'DCM11','Quaternion','DirectionCosineMatrix','BodyAlignmentVectorX','Attitude','AttitudeReferenceBody','AttitudeConstraintType','MRP1WritableProbe','DCM11WritableProbe'};affected={v+s for v in ['MRP1','MRP2','MRP3'] for s in ['',SUFFIX,NATIVE_SUFFIX]}|{'QuaternionWritableProbe'}
    versions=[('function-attitude-parameter-bindings-headless-v1','headless-executed-sources-v1.tar.gz',pilot),('function-attitude-parameter-bindings-headless-v2','headless-executed-sources-v2.tar.gz',arguments-{v+NATIVE_SUFFIX for v in ['MRP1','MRP2','MRP3']}),('function-attitude-parameter-bindings-headless-v3','headless-executed-sources-v3.tar.gz',{v+NATIVE_SUFFIX for v in ['MRP1','MRP2','MRP3']}),('function-attitude-parameter-bindings-headless-v4','headless-executed-sources-v4.tar.gz',affected),('function-vector-assignment-headless-v1','vector-headless-executed-sources-v1.tar.gz',vargs),('function-vector-assignment-headless-v2','vector-headless-executed-sources-v2.tar.gz',vargs),('function-attitude-parameter-bindings-visible-v1','visible-executed-sources-v1.tar.gz',arguments|{'Vector:'+v for v in vargs})]
    for inv,cap,expected_args in versions:
        vector_inv=inv.startswith('function-vector-');q=load(bound(repo/'build/gui-acceptance'/inv/'results.json'));frozen=visible if 'visible' in inv else capsule(cap);require(q.get('finished_utc') and q['end_reason']=='completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest()==d for n,d in q['source_hashes'].items()),'Executed source/terminal state differs '+inv);rows={v['name']:v for v in q['results'] if v['status']!='not-run'};wanted={('QtGui.NasaFunctionVectorAssignment'+v[7:]) if v.startswith('Vector:') else ('QtGui.NasaFunctionVectorAssignment' if vector_inv else PREFIX)+v for v in expected_args};require(set(rows)==wanted,'Exact selection differs '+inv)
        for name,row in rows.items():
            vector=name.startswith('QtGui.NasaFunctionVectorAssignment');arg=name.removeprefix('QtGui.NasaFunctionVectorAssignment' if vector else PREFIX);case=bound(row['log']).parent;item=dict(invocation=inv,test=name);key=arg.removesuffix(NATIVE_SUFFIX) if arg.endswith(NATIVE_SUFFIX) else arg.removesuffix(SUFFIX);spec=vspecs[arg] if vector else specs[key];key=spec['factory_type']
            failed=(inv.endswith('headless-v1') and (vector or arg in {'BodyAlignmentVectorX','AttitudeReferenceBody','AttitudeConstraintType'})) or (inv=='function-attitude-parameter-bindings-headless-v2' and arg=='QuaternionWritableProbe') or inv=='function-attitude-parameter-bindings-headless-v3'
            log=bound(row['log']).read_text()
            if failed:
                require(row['status']=='failed' and row['returncode']!=0 and not (case/'proof.json').exists(),'Retained failure relabelled')
                if vector or arg=='QuaternionWritableProbe':require('ElementWrapper::SetValue() Cannot set "PrivateOnly.'+('ThrustAngle' if vector else 'Quaternion') in log and 'FAIL Binding scope mission/callback census failed' in log,'Original strict Rvector assignment failure lost')
                elif inv.endswith('headless-v3'):
                    require('AttitudeData::SetReal() Not settable or unknown item id:' in log,'Original strict native MRP failure lost');attempts=load(bound(case/'0-native-mutation-attempts.json'));require(len(attempts)==4 and [v['calls'] for v in attempts]==[0,1,2,3] and all(v['shared'] is False for v in attempts),'Native MRP failed attempt scope differs')
                else:require('Actual NadirPointing reference body pointer differs' in log,'Original configured-system fixture failure lost')
                failures.append(item);continue
            require(row['status']=='passed' and row['returncode']==0,'Selected case failed '+name);tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Native JUnit differs');emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log)];require(len(emitted)==1 and load(bound(case/'proof.json'))==emitted[0],'Emitted/stored proof differs');p=emitted[0]
            old=inv=='function-attitude-parameter-bindings-headless-v1' or (inv=='function-attitude-parameter-bindings-headless-v2' and key in {'MRP1','MRP2','MRP3'})
            if vector:vector_contract(p,spec)
            elif inv=='function-attitude-parameter-bindings-headless-v1':historical_contract(p,spec)
            else:contract(p,spec,allow_old_MRP_refusals=inv=='function-attitude-parameter-bindings-headless-v2')
            initial,edited=bound(case/'initial.gmf').read_text(),bound(case/'edited.gmf').read_text();require(initial.count('Keep = 103;')==1 and initial.replace('Keep = 103;','Keep = 41;')==edited,'Exact file-edit recipe differs')
            if vector:
                for text in [edited,bound(case/'caller.script').read_text()]:require(f'PrivateOnly.{key} = PrivateSource.{key};' in text and f'SharedSC.{key} = SharedSource.{key};' in text,'Original Rvector source assignments replaced')
            elif arg.endswith(SUFFIX) and not arg.endswith(NATIVE_SUFFIX):require(f'PrivateOnly.{key} = PrivateOnly.{key};' in edited and f'SharedSC.{key} = SharedSC.{key};' in edited,'Strict script statements changed')
            require(load(bound(case/'phases.json'))==p['phases'],'Raw phases differ')
            for phase in p['phases']:
                tag=str(phase['phase']);messages=bound(case/(tag+'-messages.txt')).read_text();require(load(bound(case/(tag+'-live-parameter-samples.json')))==phase['live_samples'] and all(load(bound(case/(tag+'-invariants.json'))).values()) and load(bound(case/(tag+'-prototypes.json')))==load(bound(case/'prototypes-before.json')),'Raw samples/invariants/prototypes differ');require(written(messages,spec,spec['root_private_reference'])+(written(messages,spec,spec['local_private_reference']) if spec['root_private_reference']!=spec['local_private_reference'] else [])==phase['private_writes'] and written(messages,spec,spec['shared_reference'])==phase['shared_writes'],'Raw target Write differs')
                values=[v['actual'] for sample in phase['live_samples'] for v in sample['parameters']];raw=bound(case/(tag+'-binding-values.txt')).read_text().split();flat=[n for v in values for n in (v if isinstance(v,list) else [v])];require(raw==flat if flat and isinstance(flat[0],str) else len(raw)==len(flat),'Raw Report width/string values differ')
                if flat and not isinstance(flat[0],str):compare([float(v) for v in raw],flat,1e-12 if vector else 1e-10)
                require([float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==phase['raw_markers'],'Raw marker Report differs')
                if vector:
                    source_spec=dict(metadata=spec['metadata'],root_private_reference='PrivateSource.'+key,local_private_reference='PrivateSource.'+key,shared_reference='SharedSource.'+key)
                    for j,ref in enumerate([source_spec['root_private_reference'],source_spec['shared_reference']]):require(written(messages,source_spec,ref)==[sample['parameters'][j]['vector_source']['actual'] for sample in phase['live_samples']],'Raw source Write differs')
            if 'visible' in inv:
                d=q['visible_display'];require(d['start_minimized'] and d['resizable'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'] and q['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible policy differs');exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Visible trace absent');mapped=maps[traces[0].stem.removeprefix('trace-')];exes=[n for n in row['registered_command'] if n in q['binary_hashes']];require(len(exes)==1 and mapped['executable']==exes[0] and all(before['images'].get(n)==d for n,d in mapped['images'].items()),'Actual mapped image differs')
                for n,d in before['images'].items():
                    if n==exes[0] or Path(n).name.startswith(('libGmatBase','libGmatUtil')):require(mapped['images'].get(n)==d,'Required image unmapped')
            if not old and not vector:script_assignments+=leaf_script_assignments(p)
            (superseded if old else selected).append(item)
    require(len(selected)==146 and len(superseded)==12 and len(failures)==13,'Current/historical/failure census differs')
    base=repo/'build/gui-acceptance/function-attitude-parameter-bindings-visible-v1';template=load(bound(base/(PREFIX+'MRP1NativeWritableProbe')/'proof.json'))
    def row(p):return p['phases'][0]['live_samples'][1]['parameters'][0]
    controls=[('wrong-owner-reference',lambda p:row(p).__setitem__('reference_owner_pointer','1')),('private-owner-aliases-root',lambda p:row(p).__setitem__('live_owner_pointer',row(p)['root_owner_pointer'])),('wrong-attitude-model',lambda p:row(p)['attached'].__setitem__('type','Spinner')),('wrong-native-attitude-pointer',lambda p:row(p)['attached'].__setitem__('pointer',row(p)['attached']['configured_pointer'])),('wrong-value',lambda p:row(p).__setitem__('actual',999)),('wrong-direct-value',lambda p:row(p).__setitem__('direct_owner_property_value',999)),('wrong-frame',lambda p:row(p)['attached'].__setitem__('frame_pointer','1')),('wrong-SolarSystem',lambda p:row(p)['attached'].__setitem__('solar_system_pointer','1')),('wrong-function-SolarSystem',lambda p:row(p)['attached'].__setitem__('function_solar_system_pointer','1')),('wrong-unit',lambda p:row(p)['attached'].__setitem__('unit','deg')),('writable-reclassified',lambda p:row(p)['attached'].__setitem__('runtime_settable',False)),('native-setter-refused',lambda p:row(p)['attached'].__setitem__('native_setter_result','rejected')),('post-setter-changed',lambda p:row(p)['attached'].__setitem__('post_native_setter_value',999)),('wrong-mutation-target',lambda p:row(p)['attached']['native_mutation'].__setitem__('target',999)),('wrong-mutation-matrix',lambda p:row(p)['attached']['native_mutation']['changed_matrix'].__setitem__(0,999)),('wrong-restored-matrix',lambda p:row(p)['attached']['native_mutation']['restored_matrix'].__setitem__(0,999)),('owner-clock-propagated',lambda p:row(p)['attached'].__setitem__('owner_epoch',21546)),('missing-no-Build',lambda p:p['phases'].pop()),('wrong-marker',lambda p:p['phases'][0]['raw_markers'].__setitem__(0,999))];negatives=[]
    for label,corrupt in controls:
        p=copy.deepcopy(template);corrupt(p)
        try:contract(p,specs['MRP1'])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt attitude proof '+label)
    return dict(status='passed-36-attitude-bindings-MRP-and-Rvector-dispatch-repairs',qualified_keys=[],selected=selected,superseded_passes=superseded,retained_GUI_failures=failures,retained_compile_failures=0,retained_interrupted_cases=[],new_parameter_types=36,missions=438,function_calls=1752,live_parameter_callback_samples=1752,parameter_owner_checks=3504,attitude_clone_frame_and_SolarSystem_checks=3216,typed_attitude_Write_and_Report_values=3216,expanded_attitude_values_per_output_channel=3888,strict_attitude_script_assignments=script_assignments,native_attitude_idempotent_setter_acceptances=2832,native_MRP_mutation_restore_roundtrips=144,native_MRP_changed_and_restored_matrix_components=2592,typed_ThrustAngle_target_and_source_checks=288,expanded_ThrustAngle_values_per_source_or_target_output_channel=768,Rvector_to_Rvector_script_assignments=288,marker_values=2190,visible_mapped_processes=73,visible_exposure_events=exposures,negative_controls=negatives,native_bound_types=186,remaining_without_live_native_binding=135,production_changed_files=['src/base/parameter/AttitudeData.cpp','src/base/foundation/ElementWrapper.cpp'],factory_or_readable_role_exclusions=[],writable_metadata_reclassified=False,arbitrary_script_mutations_qualified=False,raw_cache_before_reevaluation_directly_observed=False,propagated_attitude_or_arbitrary_frame_provider_science_qualified=False,whole_scope_qualified=False,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
