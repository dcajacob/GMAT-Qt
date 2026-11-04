"""Audit frozen global-resource identity, nested return and regression evidence."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha

PREFIX='QtGui.NasaFunctionGlobalResource'
ARRAY='QtGui.NasaFunctionGlobalArray'
REGRESSIONS={
    'QtGui.NativeNasaFunctionValidationIsolation':'NasaFunctionValidationIsolationTests.cpp',
    'QtGui.NativeNasaBurnObjectLoopImpulse':'NasaBurnObjectLoopTests.cpp',
    'QtGui.NasaReadonlyTotalMass':'NasaReadonlyTests.cpp',
    'QtGui.NasaFunctionFrameArray':'NasaFunctionFrameTests.cpp',
    'QtGui.NasaFunctionControlRecursion':'NasaFunctionControlTests.cpp',
    'QtGui.NasaFunctionControlMutual':'NasaFunctionControlTests.cpp',
}
CHANGED=['src/base/interpreter/Validator.cpp','src/base/function/FunctionManager.cpp',
         'src/base/function/ObjectManagedFunction.cpp','src/base/function/ObjectManagedFunction.hpp',
         'src/base/function/UserDefinedFunction.cpp','src/base/parameter/ObjectPropertyWrapper.cpp']


def identities(rows,names):
    require([v['function'] for v in rows]==names,'Missing direct/nested function identity')
    for v in rows:
        require(v['global_store_object_pointer']==v['global_object_pointer'] and
                v['global_object_pointer'] not in {'0','',v['configured_prototype_pointer']},'Global identity/prototype differs')


def resource(p,spec):
    numeric=spec['kind'] in {'real','integer'}
    same=lambda a,b:abs(float(a)-float(b))<1e-12 if numeric else a==b
    marker=(float(spec['first']) if numeric else 0)+41
    require(p['outcome']=='passed' and p['checkpoint']=='function-global-resource-identity' and
            all(p[k]==spec[k] for k in ['type','field','kind','first']) and
            p['case']==spec['key'] and p['missions']==3 and p['function_calls']==12 and
            p['expected_marker']==marker and p['file_decisions']==[{'saved':False},{'saved':True}], 'Global workflow scope/literals differ')
    require([v['phase'] for v in p['phases']]==[0,1,2] and
            [v['explicit_rebuild'] for v in p['phases']]==[True,True,False], 'Global repeat missing')
    for v in p['phases']:
        require(v['global'] and not v['local'] and same(v['shared_value'],spec['first']) and
                same(v['decoy_value'],spec.get('private_seed',spec['second'])), 'Global field/decoy/flags differ')
        require(len(v['field_writes'])==4 and all(same(x,spec['first']) for x in v['field_writes']), 'Main/direct/nested field writes differ')
        require(len(v['raw_values'])==5 and all(abs(x-marker)<1e-12 for x in v['raw_values'][:3]) and
                v['raw_values'][3:]==[103,104], 'Global marker/caller continuation differs')
        identities(v['identities'],['ReadShared','ReadShared','NestedShared'])


def array(p):
    require(p['outcome']=='passed' and p['checkpoint']=='function-global-array-return' and
            p['missions']==3 and p['function_calls']==12, 'Global Array workflow differs')
    require([v['phase'] for v in p['phases']]==[0,1,2] and
            [v['explicit_rebuild'] for v in p['phases']]==[True,True,False], 'Global Array repeat missing')
    for v in p['phases']:
        require(v['values']==[100,59,103,11,13,17,17,11,13,17,17,31,32,33,34,104] and
                v['local_frame']==[41,5,6,7,8] and v['attached_mass_values']==1, 'Global Array component/frame/attached-object result differs')
        identities(v['identities'],['Outer','Whole'])


def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for name,digest in bindings.items():
        require(Path(name).is_file() and not Path(name).is_symlink() and sha(name)==digest,'Changed regular input '+name)
    def bound(p):
        p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def capsule(p):
        with tarfile.open(bound(p)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source capsule')
            return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible=capsule(stage/'visible-executed-sources.tar.gz')
    before=load(bound(stage/'visible-inputs-before.json'))
    require(before==load(bound(stage/'visible-inputs-after.json')) and
            {p:hashlib.sha256(b).hexdigest() for p,b in visible.items()}==before['sources'],'Visible inputs changed')
    launch=load(bound(stage/'visible-launch.json'));maps=load(bound(stage/'loaded-images.json'))['processes']
    require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==35,'Visible selection/maps incomplete')
    simulator_visible=capsule(stage/'simulator-visible-executed-sources.tar.gz')
    simulator_before=load(bound(stage/'simulator-visible-inputs-before.json'))
    require(simulator_before==load(bound(stage/'simulator-visible-inputs-after.json')) and
            {p:hashlib.sha256(b).hexdigest() for p,b in simulator_visible.items()}==simulator_before['sources'],'Simulator visible inputs changed')
    simulator_maps=load(bound(stage/'simulator-loaded-images.json'))['processes']
    simulator_launch=load(bound(stage/'simulator-visible-launch.json'))
    require(simulator_launch['returncode']==0 and simulator_launch['inputs_unchanged'] and simulator_launch['mapped_processes']==len(simulator_maps)==1,'Simulator visible selection/maps incomplete')
    catalog=json.loads(visible['src/qtgui/tests/NasaFunctionGlobalResourceCases.json'])
    specs={PREFIX+v['key']:v for v in catalog['cases']};require(len(specs)==28,'Resource type omitted')
    for original in catalog['source_catalogs']:
        raw=visible[original['path']];require(hashlib.sha256(raw).hexdigest()==original['sha256'],'Original resource catalog changed')
        original_cases=json.loads(raw);original_cases=original_cases['cases'] if isinstance(original_cases,dict) else original_cases
        for spec in (v for v in specs.values() if v['source_catalog']==original['path']):
            prior=next(v for v in original_cases if v['key']==spec['key'])
            require({k:v for k,v in spec.items() if k not in {'source_catalog','source_case_sha256'}}==prior,'Original case/assertion inputs changed')
    pilot=PREFIX+'ChemicalTank';three={PREFIX+v for v in ['ImpulsiveBurn','Formation','FiniteBurn']}
    all_final=set(specs)|{ARRAY}|set(REGRESSIONS)
    versions=[('function-global-pilot-headless-v'+str(n),'pilot-executed-sources-v'+str(n)+'.tar.gz',{pilot:False}) for n in range(2,7)]
    versions += [('function-global-pilot-headless-v8','pilot-executed-sources-v8.tar.gz',{pilot:True}),
                 ('function-global-headless-v8b','headless-executed-sources-v8b.tar.gz',{n:n not in three for n in all_final-{pilot,ARRAY}}),
                 ('function-global-diagnostic-headless-v9','diagnostic-executed-sources-v9.tar.gz',{PREFIX+'ImpulsiveBurn':False}),
                 ('function-global-diagnostic-headless-v10','diagnostic-executed-sources-v10.tar.gz',{PREFIX+'ImpulsiveBurn':False}),
                 ('function-global-recovery-headless-v11','recovery-executed-sources-v11.tar.gz',{n:n!=ARRAY for n in three|{ARRAY}|set(REGRESSIONS)}),
                 ('function-global-array-headless-v12','array-executed-sources-v12.tar.gz',{ARRAY:True}),
                 ('function-global-visible-v1',None,{n:True for n in all_final}),
                 ('function-global-simulator-headless-v13','simulator-headless-executed-sources-v13.tar.gz',{PREFIX+'Simulator':True}),
                 ('function-global-simulator-visible-v13',None,{PREFIX+'Simulator':True})]
    old_assertions=load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-resource-eligibility/visible-inputs-before.json'))['sources']
    selected=[];superseded=[];failed=[];proofs={};exposures=0
    for folder,archive,expected in versions:
        root=repo/'build/gui-acceptance'/folder;r=load(bound(root/'results.json'))
        require(r.get('finished_utc') and r['end_reason']=='completed-selection','Native attempt nonterminal')
        frozen=capsule(stage/archive) if archive else (simulator_visible if folder=='function-global-simulator-visible-v13' else visible)
        active_before=simulator_before if folder=='function-global-simulator-visible-v13' else before
        active_maps=simulator_maps if folder=='function-global-simulator-visible-v13' else maps
        for name,digest in r['source_hashes'].items():require(name in frozen and hashlib.sha256(frozen[name]).hexdigest()==digest,'Executed source differs '+name)
        active={v['name']:v for v in r['results'] if v['status']!='not-run'};require(set(active)==set(expected),'Selection changed '+folder)
        for name,passed in expected.items():
            row=active[name];require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Native status differs')
            log=bound(row['log']);case=log.parent;tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'))
            require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and
                    (tests[0].find('failure') is None and tests[0].find('error') is None)==passed,'Native JUnit differs')
            if not passed:failed.append(dict(invocation=folder,test=name));continue
            emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())]
            require(len(emitted)==1 and emitted[0]['outcome']=='passed','Native proof missing')
            p=emitted[0]
            if name==PREFIX+'Simulator' and folder.endswith('v13'):
                dependency='application/samples/Navigation/Ex_Estimate_RangeSkin.script'
                require(dependency in r['source_hashes'],'Consumed Simulator dependency unbound')
                prefix=re.split(r'(?m)^BeginMissionSequence[^\n]*',frozen[dependency].decode())[0]
                caller=bound(case/'caller.script').read_text()
                filename=re.search(r"'([^']*/Unused simulation Ω\.gmd)'",caller)
                require(filename is not None and caller.startswith(prefix.replace("'Ex_Estimate_RangeSkin.gmd'","'"+filename[1]+"'")),'Consumed Simulator prefix differs')
            if name in REGRESSIONS:
                source='src/qtgui/tests/'+REGRESSIONS[name]
                require(r['source_hashes'][source]==old_assertions[source],'Original regression assertions changed')
            else:
                (resource(p,specs[name]) if name in specs else array(p))
                require(load(bound(case/'proof.json'))==p and load(bound(case/'phases.json'))==p['phases'],'Native proof files differ')
                for phase in p['phases']:
                    tag=str(phase['phase']);values=[float(x) for x in bound(case/(tag+'-values.txt')).read_text().split()]
                    require(values==phase['raw_values' if name in specs else 'values'],'Raw report differs')
                    messages=bound(case/(tag+'-messages.txt')).read_text()
                    if name in specs:
                        spec=specs[name];values=re.findall(r'(?:^|\n)Shared\.'+re.escape(spec['field'])+r' =\s*\n([^\n]+)',messages)
                        if spec['kind']=='list':values=[re.sub(r'[{}\[\]\'" ]','',x.strip()) for x in values]
                        else:values=[x.strip().strip("'") for x in values]
                        require(values==phase['field_writes'],'Actual Write bytes differ')
                    else:
                        require([float(x) for x in bound(case/(tag+'-trace.txt')).read_text().split()]==phase['local_frame'],'Raw local frame differs')
                        require(len(re.findall(r'FuelMass\s*=\s*23(?:\.0*)?;',messages))==1,'Attached tank output differs')
                if name in specs:
                    original=bound(case/'initial.gmf').read_text();edited=bound(case/'edited.gmf').read_text();outer=bound(case/'nested.gmf').read_text()
                    require(original.replace('Keep = 103;','Keep = 41;')==edited and 'Global Shared;' in edited and
                            'Global Shared;' in outer and '[Shared, Mark] = ReadShared();' in outer,'File edit/global/nested witness differs')
                proofs[(folder,name)]=p
            if archive is None:
                display=r['visible_display'];require(display['resizable'] and display['start_minimized'] and
                    'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and
                    r['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible policy differs')
                exposures+=row['trace']['native_exposure_events'];exes=[v for v in row['registered_command'] if v in r['binary_hashes']]
                require(len(exes)==1,'Executed image unbound');traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Missing process trace')
                process=active_maps.get(traces[0].stem.removeprefix('trace-'));require(process and process['executable']==exes[0],'Process identity differs')
                for path,digest in process['images'].items():require(active_before['images'].get(path)==digest,'Mapped image changed')
                for path,digest in active_before['images'].items():
                    if path==exes[0] or Path(path).name.startswith(('libGmatBase','libGmatUtil','libGmatEstimation','libEKF','libStation','libThrustFile')):require(process['images'].get(path)==digest,'Core/provider image unmapped')
            item=dict(invocation=folder,test=name)
            old_regression=folder=='function-global-headless-v8b' and name in REGRESSIONS
            old_simulator=name==PREFIX+'Simulator' and folder in {'function-global-headless-v8b','function-global-visible-v1'}
            (superseded if old_regression or old_simulator else selected).append(item)
    require(len(selected)==70 and len(superseded)==8 and len(failed)==11,'Native totals differ')
    for mode in ['headless','visible']:
        require({v['test'] for v in selected if mode in v['invocation']}==all_final and
                sum(mode in v['invocation'] for v in selected)==35,'Missing selected mode/case')
    for version in [9,10]:
        root=repo/'build/gui-acceptance'/('function-global-diagnostic-headless-v'+str(version));r=load(bound(root/'results.json'));log=bound(next(v['log'] for v in r['results'] if v['status']=='failed')).read_text()
        require('Segmentation fault' in log and 'UDF_FUNCTION Shared.Element1' in log,'Cleanup diagnostic lost')
        if version==10:
            pointer=re.findall(r'UDF_FUNCTION Shared\.Element1 (0x[0-9a-f]+)',log)[-1]
            require('UDF_SANDBOX '+pointer in log,'Borrowed identity diagnosis lost')
    support=list(ET.parse(bound(stage/'function-output-scope.xml')).getroot().iter('testcase'))
    require(len(support)==1 and support[0].get('name')=='QtAcceptance.FunctionOutputScope' and support[0].find('failure') is None,'Output parser support failed')
    negative=[];sample=proofs[('function-global-visible-v1',pilot)];spec=specs[pilot]
    for label,change in [('wrong-global-pointer',lambda p:p['phases'][0]['identities'][0].__setitem__('global_store_object_pointer','dead')),
                         ('wrong-field',lambda p:p['phases'][0]['field_writes'].__setitem__(0,'99')),
                         ('lost-nested-call',lambda p:p['phases'][0]['identities'].pop()),
                         ('lost-repeat',lambda p:p['phases'].pop())]:
        p=copy.deepcopy(sample);change(p)
        try:resource(p,spec)
        except (AssertionError,ValueError):negative.append(label)
        else:raise ValueError('Accepted corrupt proof '+label)
    p=copy.deepcopy(proofs[('function-global-visible-v1',ARRAY)]);p['phases'][0]['values'][4]=-2
    try:array(p)
    except (AssertionError,ValueError):negative.append('lost-global-array-element-return')
    else:raise ValueError('Accepted lost global Array return')
    roles=load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-resource-eligibility/scope-review-v1.json'))
    require(len(roles['rows'])==566 and not roles['qualified_keys'] and all(not v['qualified'] for v in roles['rows']),'Factory denominator/qualification changed')
    matrix=load(bound(stage/'global-scope-matrix-v1.json'))
    rows=copy.deepcopy(matrix['rows']);witnesses={v['factory_type']:v.pop('new_global_witnesses') for v in rows}
    require(rows==roles['rows'] and not matrix['qualified_keys'] and sum(bool(v) for v in witnesses.values())==29,'Factory matrix dropped/qualified rows')
    for factory_type,items in witnesses.items():
        if not items:continue
        name=ARRAY if factory_type=='Array' else PREFIX+factory_type
        require(len(items)==2 and {v['mode'] for v in items}=={'headless','visible'},'Global context omitted')
        for v in items:
            require(dict(invocation=v['invocation'],test=name) in selected and
                    load(bound(v['proof']))==proofs[(v['invocation'],name)] and sha(v['proof'])==v['proof_sha256'],'Global context binding differs')
    return dict(status='passed-bounded-global-resource-and-nested-return',qualified_keys=[],selected=selected,superseded_regression_passes=superseded,retained_native_failures=failed,
                resource_types=28,new_GUI_passes=58,affected_regression_GUI_passes=12,new_missions=174,new_function_calls=696,raw_report_scalars=966,
                property_write_literals=672,global_store_identities=516,attached_tank_output_checks=6,visible_mapped_processes=36,selected_visible_cases=35,visible_exposure_events=exposures,
                negative_controls=negative,production_changed_files=CHANGED,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
