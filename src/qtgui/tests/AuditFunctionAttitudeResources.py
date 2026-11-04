"""Audit all eight named attitude scope repairs without provider/scientific credit."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha
from AuditFunctionResourceUniverse import resource

PREFIX='QtGui.NasaFunctionAttitude'
NAMED='QtGui.NasaFunctionOwnedNamed'
FIVE={'CoordinateSystemFixed','Spinner','PrecessingSpinner','CCSDSAEM','SpiceAttitude'}
THREE={'NadirPointing','ThreeAxisKinematic','CommandableNadirPointing'}
REPAIRED={'SpiceAttitude','ThreeAxisKinematic','CommandableNadirPointing'}
REG={'QtGui.NativeNasaAttitudeFixedFrameInertial':'NasaAttitudeFixedFrameTests.cpp','QtGui.NativeNasaFunctionValidationIsolation':'NasaFunctionValidationIsolationTests.cpp','QtGui.NasaFunctionFrameArray':'NasaFunctionFrameTests.cpp'}

def copy_contract(p,cases,formats):
    rows={v['type']:v for v in p['rows']};require(p['outcome']=='passed' and p['class_count']==len(rows)==len(p['rows'])==8 and not p['failures'] and set(rows)=={v['type'] for v in cases.values()} and p['invalid_rejections']==16 and p['unsupported_derived_rejections']==2,'Attitude Copy matrix differs')
    for spec in cases.values():
        row=rows[spec['type']];require(row['field']==spec['field'] and row['native_type']==spec['native_type'] and row['source_seed']==row['copy_value']==spec['first'] and row['destination_seed']==spec['second'] and row['copy_passed'] and row['source_unchanged'] and row['destination_identity_preserved'] and row['clone_independent'] and row['self_unchanged'] and row['invalid_rejections']==2 and row['factory_source_name']=='FactorySource' and row['factory_destination_name']=='FactoryDestination','Copy literal/name/independence differs')
    if formats:
        transitions=p['format_controls'];require(len(transitions)==12 and len({(v['quoted_mode'],v['script_mode']) for v in transitions})==12 and len({v['quoted_mode'] for v in transitions})==2 and len({v['script_mode'] for v in transitions})==6,'Format transition omitted')
        for v in transitions:require("AttitudeReferenceBody = 'Earth';" in v['quoted'] and 'AttitudeReferenceBody = Earth;' in v['unquoted'] and "AttitudeReferenceBody = 'Earth';" not in v['unquoted'],'Serialization mode contamination')

def named_repair(p,key):
    require(p['checkpoint']=='owned-expression-main-named-probe' and p['outcome']=='passed' and not p['qualified'] and p['case']==key and p['factory_type']==key,'Named repair identity differs')
    require(p['factory_constructor']['present'] and p['factory_constructor']['name']=='FactoryOnly','Factory still drops requested name')
    for phase in ['create','write_build']:
        row=p[phase];require(row['built'] and row['source_history_unchanged'] and row['configured']['present'] and row['configured']['name']=='NamedWitness' and row['configured']['native_type']==key and 'serialization_error' not in row['configured'],'Configured attitude prototype remains unusable')
    for phase in ['write_run','recovery_run']:
        row=p[phase];require(row['completed'] and row['run_result']==0 and row['source_history_unchanged'] and float(row['marker_bytes'].strip())==42,'Named main/recovery failed')
    require(p['write_run']['runtime']['present'] and p['write_run']['runtime']['name']=='NamedWitness' and p['write_run']['runtime']['native_type']==key and 'serialization_error' not in p['write_run']['runtime'] and p['recovery_build']['built'] and not p['recovery_build']['configured']['present'] and not p['recovery_run']['runtime']['present'],'Named runtime/reference/recovery differs')

def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for path,digest in bindings.items():require(Path(path).is_file() and not Path(path).is_symlink() and sha(path)==digest,'Changed regular input '+path)
    def bound(p):p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def capsule(p):
        with tarfile.open(bound(p)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source capsule');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible=capsule(stage/'visible-executed-sources-v1.tar.gz');before=load(bound(stage/'visible-inputs-before-v1.json'))
    require(before==load(bound(stage/'visible-inputs-after-v1.json')) and {n:hashlib.sha256(b).hexdigest() for n,b in visible.items()}==before['sources'],'Visible inputs changed')
    catalog=json.loads(visible['src/qtgui/tests/NasaFunctionAttitudeResourceCases.json']);cases={v['key']:v for v in catalog['cases']};base=load(bound(repo/catalog['baseline_matrix']));require(sha(repo/catalog['baseline_matrix'])==catalog['baseline_matrix_sha256'] and len(cases)==8 and {v['type'] for v in cases.values()}=={v['factory_type'] for v in base['rows'] if v['category_id']==153} and len(base['rows'])==566 and not base['qualified_keys'],'Attitude universe narrowed')
    all_scope={PREFIX+k for k in cases};all_named={NAMED+k for k in REPAIRED};all_final=all_scope|all_named|set(REG)
    versions=[('function-attitude-resources-headless-v1','headless-executed-sources-v1.tar.gz',{PREFIX+k:False for k in cases if k!='SpiceAttitude' and k not in {'ThreeAxisKinematic','CommandableNadirPointing'}}),
              ('function-attitude-resources-headless-v2','headless-executed-sources-v2.tar.gz',{PREFIX+k:k in FIVE for k in cases}),
              ('function-attitude-resources-recovery-headless-v3','recovery-executed-sources-v3.tar.gz',{n:True for n in {PREFIX+k for k in THREE}|all_named|set(REG)}),
              ('function-attitude-resources-regression-headless-v4','regression-executed-sources-v4.tar.gz',{'QtGui.NativeNasaAttitudeFixedFrameInertial':True}),
              ('function-attitude-resources-visible-v1',None,{n:True for n in all_final})]
    old=load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-owned-eligibility/visible-inputs-before-v2.json'))['sources'];launch=load(bound(stage/'visible-launch-v1.json'));maps=load(bound(stage/'loaded-images-v1.json'))['processes'];require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==14,'Visible process matrix incomplete')
    selected=[];failed=[];superseded=[];proofs={};exposures=0
    for folder,archive,expected in versions:
        root=repo/'build/gui-acceptance'/folder;receipt=load(bound(root/'results.json'));require(receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal native attempt');frozen=capsule(stage/archive) if archive else visible
        for path,digest in receipt['source_hashes'].items():require(path in frozen and hashlib.sha256(frozen[path]).hexdigest()==digest,'Executed source differs '+path)
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};require(set(active)==set(expected),'Native selection differs')
        for name,passed in expected.items():
            row=active[name];require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Native CTest status differs');log=bound(row['log']);case=log.parent;text=log.read_text();tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'))
            require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None and tests[0].find('error') is None)==passed,'Native JUnit differs');item=dict(invocation=folder,test=name)
            if not passed:
                if folder.endswith('v1'):require('Cannot copy objects of type' in text,'Original Copy failure lost')
                elif name==PREFIX+'NadirPointing':
                    a=load(bound(case/'prototypes-before.json'));b=load(bound(case/'2-prototypes.json'));require(a['Shared']==b['Shared'] and a['Shadow'].replace("AttitudeReferenceBody = 'Earth';",'AttitudeReferenceBody = Earth;')==b['Shadow'] and not load(bound(case/'2-invariants.json'))['prototypes'],'Original exact serialization drift lost')
                else:require('Reference coordinate system EarthMJ2000Eq not defined' in text,'Prototype reference failure lost')
                failed.append(item);continue
            emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',text)];require(len(emitted)==1 and emitted[0]['outcome']=='passed','Native proof absent');p=emitted[0]
            if name in REG:
                source='src/qtgui/tests/'+REG[name];require(receipt['source_hashes'][source]==old[source],'Original regression assertions changed')
                if name=='QtGui.NativeNasaAttitudeFixedFrameInertial' and folder!='function-attitude-resources-recovery-headless-v3':
                    fixture='src/qtgui/tests/AttitudeFixedFrameCases.json';require(fixture in receipt['source_hashes'] and load(bound(case/'matrix.json'))==json.loads(frozen[fixture]),'Consumed fixed-frame fixture unbound or different')
            elif name.startswith(NAMED):
                key=name.removeprefix(NAMED);named_repair(p,key);require(load(bound(case/'proof.json'))==p,'Named native proof differs');proofs[(folder,name)]=p
                for phase in ['write_run','recovery_run']:require(bound(case/(phase+'.values.txt')).read_text()==p[phase]['marker_bytes'] and 'Mission run completed.' in bound(case/(phase+'.messages.txt')).read_text(),'Raw named operation/marker differs')
                require('Create '+key+' NamedWitness;' in bound(case/'create.script').read_text() and 'Write NamedWitness;' in bound(case/'write_build.script').read_text(),'Actual named creation/use missing')
            else:
                key=name.removeprefix(PREFIX);spec=cases[key];resource(p,spec);require(load(bound(case/'proof.json'))==p and load(bound(case/'phases.json'))==p['phases'],'Scope native phase/proof differs');proofs[(folder,name)]=p
                caller=bound(case/'caller.script').read_text();original=bound(case/'initial.gmf').read_text();edited=bound(case/'edited.gmf').read_text();outer=bound(case/'nested.gmf').read_text();require('Create '+spec['type']+' Shared Shadow;' in caller and 'Create '+spec['type']+' PrivateOnly;' in original and original.replace('Keep = 103;','Keep = 41;')==edited and 'Global Shared;' in edited and 'Global Shared;' in outer and '[Mark] = ReadShared();' in outer,'Attitude actual caller/private/global/edit source differs')
                prototypes=load(bound(case/'prototypes-before.json'))
                for phase in p['phases']:
                    tag=str(phase['phase']);require([float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==phase['raw_values'] and load(bound(case/(tag+'-prototypes.json')))==prototypes and all(load(bound(case/(tag+'-invariants.json'))).values()),'Raw continuation/prototypes/invariants differ');messages=bound(case/(tag+'-messages.txt')).read_text()
                    for object_name,key in [('Shared','shared_writes'),('PrivateOnly','private_writes'),('Shadow','shadow_writes')]:
                        words=re.findall(r'(?:^|\n)'+object_name+r'\.'+re.escape(spec['field'])+r' =\s*\n([^\n]*)',messages);require([v.strip().strip("'") for v in words]==phase[key],'Actual string-field Write differs')
            if archive is None:
                display=receipt['visible_display'];require(display['start_minimized'] and display['resizable'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible policy differs');exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Process trace missing');process=maps.get(traces[0].stem.removeprefix('trace-'));exes=[v for v in row['registered_command'] if v in receipt['binary_hashes']];require(len(exes)==1 and process and process['executable']==exes[0],'Mapped process identity differs')
                for path,digest in process['images'].items():require(before['images'].get(path)==digest,'Mapped image changed')
                for path,digest in before['images'].items():
                    if path==exes[0] or Path(path).name.startswith(('libGmatBase','libGmatUtil')):require(process['images'].get(path)==digest,'Core image unmapped')
            (superseded if folder=='function-attitude-resources-recovery-headless-v3' and name=='QtGui.NativeNasaAttitudeFixedFrameInertial' else selected).append(item)
    require(len(selected)==28 and len(failed)==8 and len(superseded)==1,'GUI totals differ')
    for mode in ['headless','visible']:require({v['test'] for v in selected if mode in v['invocation']}==all_final and sum(mode in v['invocation'] for v in selected)==14,'Selected mode/context omitted')
    final_copy=None
    for version in [1,2,4]:
        root=stage/('copy-support-v'+str(version));launch=load(bound(root/'launch.json'));inputs=load(bound(root/'inputs-before.json'));require(inputs==load(bound(root/'inputs-after.json')) and launch['inputs_unchanged'] and launch['returncode']==(8 if version==1 else 0),'Support source/status differs');frozen=capsule(stage/('copy-support-executed-sources-v'+str(version)+'.tar.gz'))
        for path,digest in inputs['sources'].items():require(path in frozen and hashlib.sha256(frozen[path]).hexdigest()==digest,'Support source differs')
        tests=list(ET.parse(bound(root/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')=='QtAcceptance.AttitudeResourceCopy' and (tests[0].find('failure') is None)==(version!=1),'Support JUnit differs');p=load(bound(root/'proof.json'));require([json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',bound(root/'test.log').read_text())]==[p],'Support emitted proof differs')
        if version==1:require(p['outcome']=='failed' and len(p['rows'])==len(p['failures'])==8 and all(not v['copy_passed'] and 'Cannot copy objects of type' in v['copy_error'] for v in p['rows']),'Eight original Copy diagnostics lost')
        else:copy_contract(p,cases,version==4)
        if version==4:require(load(bound(root/'format-controls.json'))==p['format_controls'],'Format evidence differs');final_copy=p
    matrix=load(bound(stage/'attitude-scope-matrix-v1.json'));rows=copy.deepcopy(matrix['rows']);scope={v['factory_type']:v.pop('attitude_scope_witnesses') for v in rows};named={v['factory_type']:v.pop('named_attitude_repairs') for v in rows};require(rows==base['rows'] and not matrix['qualified_keys'] and sum(bool(v) for v in scope.values())==8 and sum(bool(v) for v in named.values())==3,'Complete role table changed/qualified')
    for table,prefix in [(scope,PREFIX),(named,NAMED)]:
        for name,items in table.items():
            if not items:continue
            key=next(v['key'] for v in cases.values() if v['type']==name);require(len(items)==2 and {v['mode'] for v in items}=={'headless','visible'},'Attitude context omitted')
            for v in items:require(dict(invocation=v['invocation'],test=prefix+key) in selected and load(bound(v['proof']))==proofs[(v['invocation'],prefix+key)] and sha(v['proof'])==v['proof_sha256'],'Attitude context binding differs')
    negatives=[];sample=proofs[('function-attitude-resources-visible-v1',PREFIX+'Spinner')]
    for label,change in [('wrong-private-field',lambda p:p['phases'][0]['private_writes'].__setitem__(0,'Wrong')),('missing-repeat',lambda p:p['phases'].pop()),('wrong-global-identity',lambda p:p['phases'][0]['identities'][0].__setitem__('global_store_object_pointer','wrong'))]:
        p=copy.deepcopy(sample);change(p)
        try:resource(p,cases['Spinner'])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt scope '+label)
    p=copy.deepcopy(final_copy);p['format_controls'][0]['unquoted']=p['format_controls'][0]['quoted']
    try:copy_contract(p,cases,True)
    except (ValueError,AssertionError):negatives.append('retained-format-mode')
    else:raise ValueError('Accepted contaminated serialization')
    p=copy.deepcopy(proofs[('function-attitude-resources-visible-v1',NAMED+'SpiceAttitude')]);p['factory_constructor']['name']=''
    try:named_repair(p,'SpiceAttitude')
    except (ValueError,AssertionError):negatives.append('lost-factory-name')
    else:raise ValueError('Accepted unnamed factory result')
    return dict(status='passed-eight-attitude-scope-copy-name-reference-format-repairs',qualified_keys=[],selected=selected,superseded_GUI_passes=superseded,retained_GUI_failures=failed,retained_support_CTest_failures=1,superseded_support_passes=1,new_scope_GUI_passes=16,affected_named_probe_GUI_passes=6,original_regression_GUI_passes=6,scope_missions=48,function_calls=192,property_Write_values=384,raw_marker_scalars=240,global_store_identity_checks=144,repaired_named_main_missions=6,named_recovery_controls=6,copy_classes=8,copy_invalid_rejections=16,copy_unsupported_derived_rejections=2,serialization_format_transitions=12,visible_mapped_processes=14,visible_exposure_events=exposures,negative_controls=negatives,production_changed_files=load(bound(stage/'production-paths-v3.json'))['paths'],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
