"""Audit all 44 math-node main/private/Global witnesses and guarded typed Copy."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha

PREFIX='QtGui.NasaFunctionMathResource'
REG={'QtGui.NasaFunctionFrameArray':'NasaFunctionFrameTests.cpp','QtGui.NasaFunctionControlRecursion':'NasaFunctionControlTests.cpp','QtGui.NasaFunctionControlMutual':'NasaFunctionControlTests.cpp','QtGui.NasaFunctionBuiltinQuaternionProduct':'NasaFunctionBuiltinTests.cpp','QtGui.NasaFunctionBuiltinNumberString':'NasaFunctionBuiltinTests.cpp'}

def scope(p,spec):
    require(p['checkpoint']=='math-resource-main-local-global' and p['outcome']=='passed' and p['case']==spec['key'] and p['type']==spec['type'] and p['native_type']==spec['native_type'] and p['caller_same_name_shadow'] and p['missions']==3 and p['function_calls']==12 and p['file_decisions']==[dict(saved=False),dict(saved=True)],'Math scope identity/file decisions differ')
    require([v['phase'] for v in p['phases']]==[0,1,2] and [v['explicit_rebuild'] for v in p['phases']]==[True,True,False],'Math scope repeat missing')
    for phase in p['phases']:
        require(phase['global'] and not phase['local'] and not phase['private_leaked'] and phase['raw_values']==[41,41,41,103,104],'Math global/private/continuation differs')
        for name,key,count in [('Shared','shared_writes',4),('PrivateOnly','private_writes',3),('Shadow','shadow_writes',1)]:require(phase[key]==['Create '+spec['native_type']+' '+name+';']*count,'Actual native math object Write differs')
        require([v['function'] for v in phase['identities']]==['ReadShared','ReadShared','NestedShared'],'Math direct/nested identity missing')
        for row in phase['identities']:require(row['global_store_object_pointer']==row['global_object_pointer'] and row['global_object_pointer'] not in {'','0',row['configured_prototype_pointer']} and all(re.fullmatch('[0-9a-f]+',row[k]) for k in ['global_store_object_pointer','global_object_pointer','configured_prototype_pointer']),'Math shared-store identity differs')

def copy_contract(p,cases):
    require(p['checkpoint']=='math-resource-polymorphic-copy-active-state' and p['outcome']=='passed' and p['factory_spellings']==44 and not p['failed_controls'] and len(p['rows'])==88 and {(v['factory_type'],v['matrix']) for v in p['rows']}=={(k,m) for k in cases for m in [False,True]} and p['invalid_rejections']==264 and p['unsupported_derived_rejections']==2,'Typed math Copy matrix differs')
    for row in p['rows']:
        spec=cases[row['factory_type']];matrix=row['matrix'];function=spec['type']!='MathElement';source=dict(name='CopySource',type=spec['native_type'],element_type=12 if matrix else 4,number=not matrix,function=function,function_input=not function,local=False,value=[2,-3,5.5,7,11,13] if matrix else 17.25);source['global']=False
        if matrix:source.update(rows=2,columns=3)
        copied=dict(source);copied.update(name='CopyDestination');copied['global']=True
        require(row['native_type']==spec['native_type'] and row['passed'] and row['source']==row['source_after']==source and row['copied']==row['destination_after']==copied and row['source_unchanged'] and row['clone_independent'] and row['self_unchanged'] and row['invalid_rejections']==3,'Typed math Copy literal/flags/identity/independence differs')

def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for path,digest in bindings.items():require(Path(path).is_file() and not Path(path).is_symlink() and sha(path)==digest,'Changed regular input '+path)
    def bound(p):p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def capsule(p):
        with tarfile.open(bound(p)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source capsule');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible=capsule(stage/'visible-executed-sources-v1.tar.gz');before=load(bound(stage/'visible-inputs-before-v1.json'));require(before==load(bound(stage/'visible-inputs-after-v1.json')) and {n:hashlib.sha256(b).hexdigest() for n,b in visible.items()}==before['sources'],'Visible inputs changed')
    catalog=json.loads(visible['src/qtgui/tests/NasaFunctionMathResourceCases.json']);cases={v['type']:v for v in catalog['cases']};base=load(bound(repo/catalog['baseline_matrix']));require(sha(repo/catalog['baseline_matrix'])==catalog['baseline_matrix_sha256'] and len(cases)==44 and len({v['cpp_class'] for v in cases.values()})==42 and set(cases)=={v['factory_type'] for v in base['rows'] if v['category_id']==154} and len(base['rows'])==566 and not base['qualified_keys'],'Complete math universe narrowed')
    dispatch=load(bound(stage/'copy-dispatch-classes-v3.json'));require(set(dispatch)=={v['cpp_class'] for v in cases.values()} and len(dispatch)==42 and set(re.findall(r'CopyExactMathNode<([A-Za-z0-9_]+)>',visible['src/base/math/MathNode.cpp'].decode()))==set(dispatch),'Concrete Copy dispatch omits a class')
    for version in [2,3,4]:
        root=stage/('copy-support-v'+str(version));launch=load(bound(root/'launch.json'));inputs=load(bound(root/'inputs-before.json'));require(inputs==load(bound(root/'inputs-after.json')) and launch['inputs_unchanged'] and launch['returncode']==(0 if version==4 else 8),'Support source/status differs');frozen=capsule(stage/('copy-support-executed-sources-v'+str(version)+'.tar.gz'))
        for path,digest in inputs['sources'].items():require(path in frozen and hashlib.sha256(frozen[path]).hexdigest()==digest,'Support executed source differs')
        tests=list(ET.parse(bound(root/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')=='QtAcceptance.MathResourceCopy' and (tests[0].find('failure') is None)==(version==4),'Support JUnit differs');p=load(bound(root/'proof.json'));require([json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',bound(root/'test.log').read_text())]==[p],'Support emitted proof differs')
        if version==2:require(p['outcome']=='failed' and len(p['rows'])==len(p['failed_controls'])==88 and all(not v['passed'] and 'Cannot copy objects of type' in v['error'] for v in p['rows']),'Original missing-Copy diagnostics lost')
        elif version==3:
            require(p['outcome']=='failed' and len(p['rows'])==88 and len(p['failed_controls'])==86 and sum(v['passed'] for v in p['rows'])==2 and sum('lost active state' in v.get('error','') for v in p['rows'])==84 and sum('unsized table' in v.get('error','') for v in p['rows'])==2,'Intermediate typed-assignment diagnostics lost')
        else:copy_contract(p,cases);final_copy=p
    all_new={PREFIX+v['key'] for v in cases.values()};all_final=all_new|set(REG);versions=[('function-math-resources-pilot-headless-v1','pilot-executed-sources-v1.tar.gz',{PREFIX+k for k in ['MathElement','Add','Strcat','FunctionRunner']},False),('function-math-resources-headless-v4','headless-executed-sources-v4.tar.gz',all_final,True),('function-math-resources-visible-v1',None,all_final,True)]
    old=load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-math-clone/visible-inputs-before-v1.json'))['sources'];maps=load(bound(stage/'loaded-images-v1.json'))['processes'];launch=load(bound(stage/'visible-launch-v1.json'));require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==49,'Visible selection/maps incomplete');selected=[];failed=[];proofs={};exposures=0
    for folder,archive,expected,passed in versions:
        root=repo/'build/gui-acceptance'/folder;receipt=load(bound(root/'results.json'));require(receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal GUI attempt');frozen=capsule(stage/archive) if archive else visible
        for path,digest in receipt['source_hashes'].items():require(path in frozen and hashlib.sha256(frozen[path]).hexdigest()==digest,'GUI executed source differs')
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};require(set(active)==expected,'GUI selection differs')
        for name,row in active.items():
            require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Native GUI status differs');log=bound(row['log']);case=log.parent;tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None and tests[0].find('error') is None)==passed,'GUI JUnit differs');item=dict(invocation=folder,test=name)
            if not passed:require('Cannot copy objects of type' in log.read_text(),'Original private reset failure lost');failed.append(item);continue
            emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())];require(len(emitted)==1 and emitted[0]['outcome']=='passed','Native GUI proof missing');p=emitted[0]
            if name in REG:
                require(receipt['source_hashes']['src/qtgui/tests/'+REG[name]]==old['src/qtgui/tests/'+REG[name]],'Original regression assertions changed')
                if name.startswith('QtGui.NasaFunctionBuiltin'):require('src/qtgui/tests/NasaFunctionBuiltinCases.json' in receipt['source_hashes'],'Consumed built-in fixture unbound')
            else:
                spec=cases[name.removeprefix(PREFIX)];scope(p,spec);require(load(bound(case/'proof.json'))==p and load(bound(case/'phases.json'))==p['phases'],'Native scope proof/phases differ');proofs[(folder,spec['type'])]=p
                caller=bound(case/'caller.script').read_text();original=bound(case/'initial.gmf').read_text();edited=bound(case/'edited.gmf').read_text();outer=bound(case/'nested.gmf').read_text();require('Create '+spec['type']+' Shared Shadow;' in caller and 'Create Spacecraft PrivateOnly;' in caller and 'PrivateOnly.DryMass = 987;' in caller and 'Create '+spec['type']+' PrivateOnly;' in original and original.replace('Keep = 103;','Keep = 41;')==edited and 'Global Shared;' in edited and 'Global Shared;' in outer and '[Mark] = ReadShared();' in outer,'Actual caller/private/global/file-edit source differs')
                prototypes=load(bound(case/'prototypes-before.json'));require('PrivateOnly.DryMass = 987;' in prototypes['PrivateOnly'] and 'Create Spacecraft PrivateOnly;' in prototypes['PrivateOnly'],'Different-type caller prototype missing')
                for phase in p['phases']:
                    tag=str(phase['phase']);require([float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==phase['raw_values'] and load(bound(case/(tag+'-prototypes.json')))==prototypes and all(load(bound(case/(tag+'-invariants.json'))).values()),'Raw markers/prototypes/invariants differ');messages=bound(case/(tag+'-messages.txt')).read_text()
                    for object_name,key in [('Shared','shared_writes'),('PrivateOnly','private_writes'),('Shadow','shadow_writes')]:
                        observed=[v.strip() for v in re.findall(r'(?:^|\n)(Create '+re.escape(spec['native_type'])+' '+object_name+r';)',messages)];require(observed==phase[key],'Raw Script-style object Write differs')
            if archive is None:
                display=receipt['visible_display'];require(display['start_minimized'] and display['resizable'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible policy differs');exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Process trace missing');process=maps.get(traces[0].stem.removeprefix('trace-'));exes=[v for v in row['registered_command'] if v in receipt['binary_hashes']];require(len(exes)==1 and process and process['executable']==exes[0],'Mapped process identity differs')
                for path,digest in process['images'].items():require(before['images'].get(path)==digest,'Mapped image changed')
                for path,digest in before['images'].items():
                    if path==exes[0] or Path(path).name.startswith(('libGmatBase','libGmatUtil')):require(process['images'].get(path)==digest,'Core image unmapped')
            selected.append(item)
    require(len(selected)==98 and len(failed)==4 and all(sum(mode in v['invocation'] for v in selected)==49 for mode in ['headless','visible']),'GUI totals differ')
    matrix=load(bound(stage/'math-scope-matrix-v1.json'));rows=copy.deepcopy(matrix['rows']);contexts={v['factory_type']:v.pop('math_private_global_witnesses') for v in rows};require(rows==base['rows'] and not matrix['qualified_keys'] and sum(bool(v) for v in contexts.values())==44,'Complete role table changed/qualified')
    for name in cases:
        items=contexts[name];require(len(items)==2 and {v['mode'] for v in items}=={'headless','visible'},'Math context omitted')
        for v in items:require(dict(invocation=v['invocation'],test=PREFIX+cases[name]['key']) in selected and load(bound(v['proof']))==proofs[(v['invocation'],name)] and sha(v['proof'])==v['proof_sha256'],'Math context binding differs')
    negatives=[];sample=proofs[('function-math-resources-visible-v1','Add')]
    for label,change in [('wrong-private-type',lambda p:p['phases'][0]['private_writes'].__setitem__(0,'Create Spacecraft PrivateOnly;')),('missing-repeat',lambda p:p['phases'].pop()),('wrong-global-identity',lambda p:p['phases'][0]['identities'][0].__setitem__('global_store_object_pointer','wrong'))]:
        p=copy.deepcopy(sample);change(p)
        try:scope(p,cases['Add'])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt scope '+label)
    p=copy.deepcopy(final_copy);p['rows'][0]['copied']['value']=99
    try:copy_contract(p,cases)
    except (ValueError,AssertionError):negatives.append('wrong-Copy-value')
    else:raise ValueError('Accepted wrong Copy value')
    p=copy.deepcopy(final_copy);p['rows'].pop()
    try:copy_contract(p,cases)
    except (ValueError,AssertionError):negatives.append('omitted-Copy-spelling')
    else:raise ValueError('Accepted missing Copy spelling')
    return dict(status='passed-44-math-main-private-global-and-typed-Copy',qualified_keys=[],selected=selected,retained_GUI_failures=failed,retained_support_CTest_failures=2,retained_failed_Copy_controls=[88,86],new_scope_GUI_passes=88,affected_original_regression_GUI_passes=10,scope_missions=264,function_calls=1056,Script_object_Writes=2112,raw_marker_scalars=1320,global_store_identity_checks=792,different_type_caller_shadow_missions=264,factory_spellings=44,concrete_classes=42,passed_Copy_controls=88,atomic_invalid_rejections=264,unsupported_derived_rejections=2,visible_mapped_processes=49,visible_exposure_events=exposures,negative_controls=negatives,production_changed_files=load(bound(stage/'production-candidates-v1.json'))['paths'],operand_graph_evaluation_qualified=False,initialized_graph_manager_ownership_qualified=False,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
