"""Audit ThrustAngle owner/rebinding, dynamic vectors and native dispatch protocols."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha
from AuditFunctionParameterBindings import compare,written
TYPES={'ThrustAngle1','ThrustAngle2'};ARGS={k+'N'+str(n) for k in TYPES for n in [1,3,4]};PREFIX='QtGui.NasaFunctionThrustAngleParameterBinding';REG={'QtGui.NasaFunctionRemainingResource'+n for n in ['ThrustSegment','ThrustHistoryFile']}
NATIVE={'QtAcceptance.'+k+'Native'+m for k in TYPES for m in ['Binding','Full']}

def vector(n,base):return [base+.125*i for i in range(n)]

def contract(p,spec):
    key=spec['factory_type'];n=spec['length'];meta=spec['metadata']
    require(p['checkpoint']=='live-function-thrust-angle-parameter-owner-binding' and p['outcome']=='passed' and p['case']==key and p['argument']==spec['argument'] and p['length']==n and p['family']=='ThrustAngle' and p['full_system_parameter_inventory']==321 and set(p['initial_batch_types'])==TYPES and p['missions']==3 and p['function_calls']==12 and p['live_callback_samples']==12 and p['actual_parameter_owner_checks']==24 and p['file_decisions']==[dict(saved=False),dict(saved=True)],'Thrust-angle proof identity/counts differ')
    require([v['phase'] for v in p['phases']]==[0,1,2] and [v['explicit_rebuild'] for v in p['phases']]==[True,True,False],'Thrust-angle no-Build repeat lost')
    for phase in p['phases']:
        require(phase['raw_markers']==[42,43,44,103,104] and phase['callback_private_maps_accessed_only_while_executing'] and len(phase['live_samples'])==4 and len(phase['private_writes'])==len(phase['shared_writes'])==4,'Thrust-angle callbacks/Write/markers differ');roots={}
        for i,sample in enumerate(phase['live_samples']):
            inside=i>0;target='ScopePulse' if inside else 'MainPulse';require(sample['inside_function']==inside and sample['calls']==i and sample['function']==('ReadShared' if inside else '') and sample['global_map_is_value_snapshot']==(not inside) and len(sample['parameters'])==2 and sample['provider']==f'Propagate PulseProp({target}) {{{target}.ElapsedSecs = 1}};','Thrust-angle callback scope/provider differs')
            for j,row in enumerate(sample['parameters']):
                shared=j==1;ref=spec['shared_reference'] if shared else spec['local_private_reference'] if inside else spec['root_private_reference'];value=vector(n,(30 if shared else 20 if inside else 10)+(i*(2 if shared else 1) if inside else 0))
                require(row['shared']==shared and row['reference']==ref and row['factory_type']==key and row['native_type']==meta['native_type'] and row['return_type']==meta['return_type']==11 and row['owner_type']==meta['owner_type_id']==172 and row['owner_type_name']=='ThrustSegment' and row['owner_name']==('SharedSC' if shared else 'PrivateOnly'),'Actual ThrustAngle/owner class differs')
                for k in ['parameter_pointer','reference_owner_pointer','live_owner_pointer','root_owner_pointer','configured_owner_pointer']:require(re.fullmatch('[0-9a-f]+',row[k]) and row[k]!='0','Invalid native angle identity')
                require(row['reference_owner_pointer']==row['live_owner_pointer']!=row['configured_owner_pointer'] and row['owner_global']==shared and row['owner_local']==(inside and not shared),'Thrust-angle actual owner reference/flags differ')
                if not inside:roots[shared]=row['root_owner_pointer']
                require(row['root_owner_pointer']==roots[shared] and (row['live_owner_pointer']==roots[shared])==(shared or not inside),'Thrust-angle private/Global alias differs');require(row['shape']==[n,1] and row['absolute_bound']==1e-12,'Thrust-angle shape/bound changed');compare(row['actual'],value,1e-12);compare(row['expected'],value,1e-12);compare((phase['shared_writes'] if shared else phase['private_writes'])[i],value,1e-12)
                a=row['attached'];require(a['declared_length']==n and a['native_primary_category']==166 and a['native_thrust_segment_type'],'Native Interface/ThrustSegment supported-type distinction lost');compare(a['direct_vector'],value,1e-12)

def native(p,key,binding_only):
    require(p['checkpoint']=='thrust-angle-native-parameter-protocol' and p['outcome']=='passed' and p['type']==key and p['binding_only']==binding_only,'Native angle protocol identity differs')
    require([v['length'] for v in p['rows']]==([3] if binding_only else [1,3,4]),'Native angle protocol arities differ')
    for row in p['rows']:
        n=row['length'];require(row['type']==key,'Native protocol type differs')
        if binding_only:require(row['added'] and row['owner_primary_type']==166 and row['owner_supports_thrust_segment'] and row['parameter_owner_type']==172 and row['native_vector']==vector(3,10),'Incremental native owner control differs');continue
        require(row['owner_reference'] and row['repeated_reads']==64,'Native protocol owner/repeated read control lost')
        for k,base in [('before',10),('polymorphic',20),('wrapper',30),('copied',30),('assigned',30),('other',90)]:compare(row[k],vector(n,base),1e-12)
        compare(row['resized'],vector(1 if n==4 else 4,40),1e-12)

def regression(p,key):
    require(p['checkpoint']=='remaining-resource-main-local-global' and p['outcome']=='passed' and p['case']==p['type']==p['native_type']==key and p['missions']==3 and p['function_calls']==12 and p['file_decisions']==[dict(saved=False),dict(saved=True)] and not p['read_only'] and not p['caller_same_name_shadow'],'Original related resource scope differs')
    require([v['phase'] for v in p['phases']]==[0,1,2] and [v['explicit_rebuild'] for v in p['phases']]==[True,True,False],'Related resource no-Build repeat lost')
    for v in p['phases']:
        require(v['global'] and not v['local'] and not v['private_leaked'] and v['raw_values']==[41,41,41,103,104] and v['shared_value']==p['first'] and v['shadow_value']==p['second'] and v['private_writes']==[p['second']]*3 and v['shadow_writes']==[p['second']] and v['shared_writes']==[p['first']]*4,'Related resource values/flags differ');require([a['function'] for a in v['identities']]==['ReadShared','ReadShared','NestedShared'] and all(a['global_store_object_pointer']==a['global_object_pointer'] not in {'0',a['configured_prototype_pointer']} for a in v['identities']),'Related resource live Global identity differs')
    if key=='ThrustSegment':require(p['field']=='ThrustScaleFactor' and p['first']==.5 and p['second']==1.5,'Original scalar resource recipe differs')
    else:require(p['field']=='FileName' and p['first'].endswith('/first.thf') and p['second'].endswith('/second.thf') and set(p['input_files'])=={'first.thf','second.thf'},'Original thrust-file dependencies differ')

def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,d in bindings.items():require(Path(n).is_file() and not Path(n).is_symlink() and sha(n)==d,'Changed regular binding '+n)
    def bound(p):
        p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def capsule(n):
        with tarfile.open(bound(stage/n)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source member');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible=capsule('visible-executed-sources-v1.tar.gz');before=load(bound(stage/'visible-inputs-before-v1.json'));require(before==load(bound(stage/'visible-inputs-after-v1.json')) and before['sources']=={n:hashlib.sha256(v).hexdigest() for n,v in visible.items()},'Visible inputs changed')
    for n in ['AuditFunctionParameterBindings.py','BuildNasaCoverageExtension.py']:require(hashlib.sha256(visible['src/qtgui/tests/'+n]).hexdigest()==sha(repo/'src/qtgui/tests'/n),'Imported helper changed')
    c=json.loads(visible['src/qtgui/tests/NasaFunctionThrustAngleParameterCases.json']);specs={v['argument']:v for v in c['thrust_angle_binding_cases']};require(len(c['cases'])==321 and set(c['initial_types'])==TYPES and set(c['selected_arguments'])==set(specs)==ARGS and not c['qualified_keys'] and all(not v['qualified'] for v in c['cases']),'Complete321 parameter catalog narrowed/promoted')
    m=load(bound(repo/c['baseline']));require(sha(repo/c['baseline'])==c['baseline_sha256'] and {v['factory_type']:v['native_parameter_metadata'] for v in m['rows'] if v['role']=='owner-bound-system-parameter'}=={v['factory_type']:v['metadata'] for v in c['cases']},'Original321 metadata changed')
    require(all(v['metadata']['owner_type_name']=='UnknownObject' and v['metadata']['owner_type_id']==172 for v in specs.values()),'Historical unknown display metadata reclassified')
    maps=load(bound(stage/'loaded-images-v1.json'))['processes'];launch=load(bound(stage/'visible-launch-v1.json'));require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==8,'Visible image mapping incomplete')
    selected=[];failed=[];superseded=[];exposures=0;gui_passes=0;support_passes=0
    new={PREFIX+a for a in ARGS};n3={PREFIX+k+'N3' for k in TYPES};binding={'QtAcceptance.'+k+'NativeBinding' for k in TYPES};full={'QtAcceptance.'+k+'NativeFull' for k in TYPES}
    for version in ['headless-v1','headless-v2','headless-v3','headless-v4','headless-v5','visible-v1']:
        inv='function-thrust-angle-parameter-bindings-'+version;q=load(bound(repo/'build/gui-acceptance'/inv/'results.json'));frozen=visible if version=='visible-v1' else capsule(version.replace('headless-','headless-executed-sources-')+'.tar.gz');require(q.get('finished_utc') and q['end_reason']=='completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest()==d for n,d in q['source_hashes'].items()),'Thrust-angle executed source/terminal state differs')
        active={v['name']:v for v in q['results'] if v['status']!='not-run'};expected=n3 if version=='headless-v1' else n3|binding if version in ['headless-v2','headless-v3'] else new|REG|full if version=='headless-v4' else NATIVE if version=='headless-v5' else new|REG;require(set(active)==expected,'Thrust-angle exact selection differs')
        for name,row in active.items():
            case=bound(row['log']).parent;item=dict(invocation=inv,test=name);passed=version in ['headless-v4','headless-v5','visible-v1'] or (version=='headless-v3' and name in binding)
            if not passed:
                require(row['status']=='failed' and row['returncode']!=0,'Retained failure relabeled');text=bound(row['log']).read_text()
                if version=='headless-v1':require('"ElseIf" is not yet supported' in text,'Fixture syntax failure lost')
                elif name in binding:
                    d=load(bound(case/'binding-diagnostic.json'));require(not d['added'] and d['owner_primary_type']==166 and d['owner_supports_thrust_segment'] and d['parameter_owner_type']==172,'Actual native binding defect diagnostic lost')
                else:require('Actual Parameter reference does not bind live scoped owner' in text,'Actual owner mismatch lost')
                if version=='headless-v3':
                    d=load(bound(case/'0-vector-diagnostic.json'));require(d['calls']==3 and d['direct']==d['expected'] and d['actual']!=d['expected'],'Observed temporary-vector corruption lost')
                failed.append(item);continue
            require(row['status']=='passed' and row['returncode']==0,'Selected ThrustAngle/regression failed');tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Native JUnit differs');emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',bound(row['log']).read_text())];require(len(emitted)==1 and load(bound(case/'proof.json'))==emitted[0],'Emitted/stored ThrustAngle proof differs');p=emitted[0]
            historical_support=(version=='headless-v3' and name in binding) or (version=='headless-v4' and name in full)
            if historical_support:
                require('std::initializer_list<int>' in frozen['src/qtgui/tests/ThrustAngleParameterNativeTests.cpp'].decode(),'Original unsafe support loop source lost')
                if version=='headless-v3':require(p['binding_only'] and len(p['rows'])==1 and p['rows'][0]['length']==0 and p['rows'][0]['native_vector']==[] and p['rows'][0]['added'],'Actual superseded empty owner control changed')
                else:native(p,p['type'],False)
                superseded.append(item);continue
            if name in NATIVE:
                key=p['type'];native(p,key,name in binding);require(any(Path(n).name=='GmatQtThrustAngleParameterNativeTests' for n in q['binary_hashes']),'Passing support binary identity unbound');support_passes+=1
            elif name in new:
                spec=specs[name.removeprefix(PREFIX)];contract(p,spec);require(load(bound(case/'phases.json'))==p['phases'],'Raw angle phases differ');initial=bound(case/'initial.gmf').read_text();edited=bound(case/'edited.gmf').read_text();require(initial.count('Keep = 103;')==1 and initial.replace('Keep = 103;','Keep = 41;')==edited and 'ElseIf' not in initial and initial.count('If Calls == ')==3,'Exact corrected file edit/branch recipe differs')
                for phase in p['phases']:
                    tag=str(phase['phase']);messages=bound(case/(tag+'-messages.txt')).read_text();require(load(bound(case/(tag+'-live-parameter-samples.json')))==phase['live_samples'] and all(load(bound(case/(tag+'-invariants.json'))).values()) and load(bound(case/(tag+'-prototypes.json')))==load(bound(case/'prototypes-before.json')),'Raw angle samples/invariants/prototypes differ');require(written(messages,spec,spec['root_private_reference'])==phase['private_writes'] and written(messages,spec,spec['shared_reference'])==phase['shared_writes'],'Actual vector Write differs');compare([float(v) for v in bound(case/(tag+'-binding-values.txt')).read_text().split()],[x for sample in phase['live_samples'] for v in sample['parameters'] for x in v['actual']],1e-12);require([float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==phase['raw_markers'],'Raw marker report differs')
                gui_passes+=1
            else:
                regression(p,name.removeprefix('QtGui.NasaFunctionRemainingResource'));gui_passes+=1
                for v in p['phases']:require([float(x) for x in bound(case/(str(v['phase'])+'-values.txt')).read_text().split()]==v['raw_values'],'Related resource raw markers differ')
            if version=='visible-v1':
                d=q['visible_display'];require(d['start_minimized'] and d['resizable'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'] and q['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible policy differs');exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Visible trace missing');mapped=maps[traces[0].stem.removeprefix('trace-')];exes=[n for n in row['registered_command'] if n in q['binary_hashes']];require(len(exes)==1 and mapped['executable']==exes[0] and all(before['images'].get(n)==d for n,d in mapped['images'].items()),'Actual mapped image differs')
                for n,d in before['images'].items():
                    if n==exes[0] or Path(n).name.startswith(('libGmatBase','libGmatUtil','libThrustFile')):require(mapped['images'].get(n)==d,'Required current image unmapped')
            selected.append(item)
    require(len(selected)==20 and gui_passes==16 and support_passes==4 and len(failed)==8 and len(superseded)==4,'Selected GUI/support/failure census differs')
    template=load(bound(repo/'build/gui-acceptance/function-thrust-angle-parameter-bindings-visible-v1'/(PREFIX+'ThrustAngle1N3')/'proof.json'));controls=[('wrong-owner-reference',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('reference_owner_pointer','1')),('private-aliases-root',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('live_owner_pointer',p['phases'][0]['live_samples'][1]['parameters'][0]['root_owner_pointer'])),('wrong-vector-width',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('shape',[1,1])),('corrupt-vector',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['actual'].__setitem__(0,999)),('wrong-native-class',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('owner_type_name','Interface')),('missing-no-Build',lambda p:p['phases'].pop()),('wrong-marker',lambda p:p['phases'][0]['raw_markers'].__setitem__(0,999))];negative=[]
    for label,corrupt in controls:
        p=copy.deepcopy(template);corrupt(p)
        try:contract(p,specs['ThrustAngle1N3'])
        except (ValueError,AssertionError):negative.append(label)
        else:raise ValueError('Accepted corrupt angle proof '+label)
    p=load(bound(repo/'build/gui-acceptance/function-thrust-angle-parameter-bindings-headless-v4'/'QtAcceptance.ThrustAngle1NativeFull'/'proof.json'))
    for label,change in [('const-setter-cache-only',lambda x:x['rows'][0].__setitem__('polymorphic',[10])),('wrapper-cache-only',lambda x:x['rows'][0].__setitem__('wrapper',[10])),('assignment-not-copied',lambda x:x['rows'][0].__setitem__('assigned',[90])),('resize-lost',lambda x:x['rows'][0].__setitem__('resized',[40]))]:
        bad=copy.deepcopy(p);change(bad)
        try:native(bad,'ThrustAngle1',False)
        except (ValueError,AssertionError):negative.append(label)
        else:raise ValueError('Accepted corrupt native angle proof '+label)
    return dict(status='passed-two-thrust-angle-parameter-owner-and-vector-protocols',qualified_keys=[],selected=selected,selected_GUI_passes=16,selected_support_passes=4,incremental_owner_controls_reused=0,current_binding_controls=2,current_full_native_protocols=2,retained_superseded_support_passes=superseded,current_native_protocol_source_fixed=True,audit_authoring_failures=1,retained_failures=failed,retained_GUI_failures=6,retained_support_failures=2,retained_compile_failures=0,retained_interrupted_cases=[],new_GUI_passes=12,related_regression_GUI_passes=4,arity_cases=6,new_parameter_types=2,missions=36,function_calls=144,live_callback_samples=144,parameter_owner_checks=288,vector_Write_observations=288,vector_Report_observations=288,scalar_Write_components=768,scalar_Report_components=768,marker_values=180,native_protocol_rows=6,native_repeated_reads=384,native_const_setter_checks=6,native_wrapper_setter_checks=6,native_copy_assignment_checks=6,native_resize_checks=6,visible_mapped_processes=8,visible_exposure_events=exposures,negative_controls=negative,native_bound_types=122,remaining_without_live_native_binding=199,factory_or_readable_role_exclusions=[],historical_unknown_display_metadata_preserved=True,thrust_history_force_or_estimation_science_qualified=False,production_changed_files=['plugins/ThrustFilePlugin/src/base/parameter/ThrustSegmentRvector.cpp','plugins/ThrustFilePlugin/src/base/parameter/ThrustSegmentParameters.cpp','plugins/ThrustFilePlugin/src/base/parameter/ThrustSegmentParameters.hpp'],whole_scope_qualified=False,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();d=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d,indent=2))
