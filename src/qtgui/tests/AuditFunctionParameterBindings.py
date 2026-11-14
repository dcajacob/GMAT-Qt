"""Audit actual live Parameter references and the Global promotion key repair."""
import argparse,copy,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha

PREFIX='QtGui.NasaFunctionParameterBinding'
TYPES={'DryMass','TotalMass','RMAG','C3Energy','Cartesian','A1ModJulian','CurrA1MJD','UTCGregorian','FuelMass','OrbitSTM','OrbitSTMA','OrbitSTMB','OrbitSTMC','OrbitSTMD'}
REG={'QtGui.NasaFunctionVectorResource':'NasaFunctionVectorResourceTests.cpp','QtGui.NativeNasaSubscriberGlobalLoopGlobal':'NasaSubscriberGlobalLoopTests.cpp'}


def compare(actual,expected,bound):
    if isinstance(expected,str):require(actual==expected,'Authored epoch stamp differs');return
    if isinstance(expected,list):
        require(isinstance(actual,list) and len(actual)==len(expected),'Typed value shape differs')
        for a,e in zip(actual,expected):compare(a,e,bound)
        return
    require(isinstance(actual,(int,float)) and math.isfinite(actual) and abs(actual-expected)<=bound,'Selected independent value differs')


def expected(spec,shared,inside,calls):
    family=spec['family'];type=spec['factory_type']
    if family=='Mass':return ((150 if shared else 275)+calls if inside else 150 if shared else 987),0,[]
    if family=='Attached':return ((71 if shared else 52)+calls if inside else 71 if shared else 19),0,[]
    if family=='Epoch':
        if type=='UTCGregorian':return f'0{3 if shared else 2 if inside else 1} Jan 2000 12:00:{calls if inside else 0:02d}.000',2e-10,[]
        # These selected A1 values are also checked against the live parent
        # reference and the authored UTC stamp case. This is a scope/clock
        # consistency witness, not independent time-conversion science.
        offset=2 if shared else 1 if inside else 0
        return 21545.0003707683+offset+(calls if inside else 0)/86400,2e-10,[]
    if family=='Matrix':
        n=6 if type=='OrbitSTM' else 3;identity=type in {'OrbitSTM','OrbitSTMA','OrbitSTMD'}
        return [1 if identity and i==j else 0 for i in range(n) for j in range(n)],1e-12,[n,n]
    x=(8000+20*calls if shared else 7500+10*calls) if inside else 8000 if shared else 7000
    if type=='Cartesian':return [x,100,200,-.1,7.5,.8],1e-9,[6,1]
    radius=math.sqrt(x*x+100**2+200**2)
    return (radius if type=='RMAG' else 56.9-2*398600.4415/radius),1e-8,[]


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
    require(proof['checkpoint']=='live-function-parameter-owner-binding' and proof['outcome']=='passed' and proof['case']==spec['factory_type'] and proof['family']==spec['family'] and proof['full_system_parameter_inventory']==321 and set(proof['initial_batch_types'])==TYPES and proof['missions']==3 and proof['function_calls']==12 and proof['live_callback_samples']==12 and proof['actual_parameter_owner_checks']==24 and proof['file_decisions']==[dict(saved=False),dict(saved=True)],'Live binding identity/count/edit differs')
    require([v['phase'] for v in proof['phases']]==[0,1,2] and [v['explicit_rebuild'] for v in proof['phases']]==[True,True,False],'Binding no-Build repeat omitted')
    meta=spec['metadata']
    for phase in proof['phases']:
        require(phase['raw_markers']==[42,43,44,103,104] and phase['callback_private_maps_accessed_only_while_executing'] and len(phase['live_samples'])==4 and len(phase['private_writes'])==len(phase['shared_writes'])==4,'Binding live callback/Write/marker contract differs')
        root_owners={}
        for index,sample in enumerate(phase['live_samples']):
            inside=index>0;calls=index
            require(sample['inside_function']==inside and sample['function']==('ReadShared' if inside else '') and sample['calls']==calls and sample['global_map_is_value_snapshot']==(not inside) and len(sample['parameters'])==2,'Live callback order/map provenance differs')
            require(sample['provider']==('Propagate PulseProp(ScopePulse) {ScopePulse.ElapsedSecs = 1};' if inside else 'Propagate PulseProp(MainPulse) {MainPulse.ElapsedSecs = 1};'),'Actual publication provider differs')
            for position,row in enumerate(sample['parameters']):
                shared=position==1;reference=spec['shared_reference'] if shared else spec['local_private_reference'] if inside else spec['root_private_reference']
                require(row['shared']==shared and row['reference']==reference and row['factory_type']==spec['factory_type'] and row['native_type']==meta['native_type'] and row['return_type']==meta['return_type'] and row['owner_type']==meta['owner_type_id'] and row['owner_name']==('SharedSC' if shared else 'PrivateOnly') and row['owner_type_name']=='Spacecraft','Actual native Parameter/owner class differs')
                for key in ['parameter_pointer','reference_owner_pointer','live_owner_pointer','root_owner_pointer','configured_owner_pointer']:
                    require(re.fullmatch('[0-9a-f]+',row[key]) and row[key]!='0','Invalid actual native identity')
                require(row['reference_owner_pointer']==row['live_owner_pointer']!=row['configured_owner_pointer'] and row['owner_global']==shared and row['owner_local']==(inside and not shared),'Parameter reference/owner flags differ')
                if not inside:root_owners[shared]=row['root_owner_pointer']
                require(row['root_owner_pointer']==root_owners[shared] and (row['live_owner_pointer']==row['root_owner_pointer'])==(shared or not inside),'Private/global actual owner aliases caller incorrectly')
                value,bound,shape=expected(spec,shared,inside,calls)
                require(row['shape']==shape and row['absolute_bound']==bound,'Typed value shape/bounds changed')
                compare(row['actual'],value,bound);compare(row['expected'],value,bound)
                compare(phase['shared_writes'][index] if shared else phase['private_writes'][index],row['actual'],bound)
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
    catalog=json.loads(visible['src/qtgui/tests/NasaFunctionParameterBindingCases.json']);specs={v['factory_type']:v for v in catalog['cases']}
    require(len(specs)==321 and set(catalog['initial_types'])==TYPES and not catalog['qualified_keys'] and all(not v['qualified'] for v in specs.values()),'Complete parameter inventory narrowed/promoted')
    matrix=load(bound(repo/catalog['baseline']));require(sha(repo/catalog['baseline'])==catalog['baseline_sha256'] and {v['factory_type']:v['native_parameter_metadata'] for v in matrix['rows'] if v['role']=='owner-bound-system-parameter'}=={n:v['metadata'] for n,v in specs.items()},'Original 321 metadata/roles changed')
    prior=load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-owned-scope/visible-inputs-before-v1.json'))['sources']
    for n in REG.values():require(hashlib.sha256(visible['src/qtgui/tests/'+n]).hexdigest()==prior['src/qtgui/tests/'+n],'Original regression assertion changed')
    maps=load(bound(stage/'loaded-images-v1.json'))['processes'];launch=load(bound(stage/'visible-launch-v1.json'))
    require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==16,'Visible process mapping incomplete')
    versions=[('headless-v1',{PREFIX+'DryMass'},set()),('headless-v2',{PREFIX+'DryMass'},set()),('headless-v3',{PREFIX+'DryMass'}|set(REG),{PREFIX+'DryMass'}|set(REG)),('headless-v4',{PREFIX+k for k in TYPES-{'DryMass'}},{PREFIX+k for k in TYPES-{'DryMass','Cartesian'}}),('headless-v5',{PREFIX+'Cartesian'},{PREFIX+'Cartesian'}),('visible-v1',{PREFIX+k for k in TYPES}|set(REG),{PREFIX+k for k in TYPES}|set(REG))]
    selected,failures,exposures=[],[],0
    for version,expected_names,passed in versions:
        invocation='function-parameter-bindings-'+version;receipt=load(bound(repo/'build/gui-acceptance'/invocation/'results.json'));frozen=visible if version=='visible-v1' else capsule(version.replace('headless-','headless-executed-sources-')+'.tar.gz')
        require(receipt.get('finished_utc') and receipt['end_reason']=='completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest()==d for n,d in receipt['source_hashes'].items()),'Executed source/terminal state differs')
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};require(set(active)==expected_names,'Exact native selection differs')
        for name,row in active.items():
            good=name in passed;require(row['status']==('passed' if good else 'failed') and (row['returncode']==0)==good,'Native result status differs')
            log=bound(row['log']);case=log.parent;tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'))
            require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None)==good,'Native JUnit differs')
            item=dict(invocation=invocation,test=name)
            if not good:
                message={'headless-v1':'Binding callback order/function identity differs','headless-v2':'Live actual Parameter instance missing from scope map','headless-v4':'Binding main parameter caller Build failed'}[version]
                require(message in log.read_text(),'Retained failure diagnostic lost')
                if version=='headless-v2':
                    events=load(bound(case/'0-callback-events.json'));require(len(events)==4 and [v['calls'] for v in events]==[0,1,2,3] and all('SharedSC.DryMass' not in v['global_keys']+v['local_keys'] and 'PrivateOnly.DryMass' in v['local_keys'] for v in events),'Missing promoted namespace evidence lost')
                if version=='headless-v4':require(not specs['Cartesian']['metadata']['reportable'] and 'Report BindingValues PrivateOnly.EarthMJ2000Eq.Cartesian SharedSC.EarthMJ2000Eq.Cartesian;' in bound(case/'caller.script').read_text(),'Original non-reportable vector fixture lost')
                failures.append(item);continue
            emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())];require(len(emitted)==1 and emitted[0]['outcome']=='passed','Native emitted proof absent');proof=emitted[0]
            if (case/'proof.json').is_file():require(load(bound(case/'proof.json'))==proof,'Stored native proof differs')
            if name not in REG:
                require((case/'proof.json').is_file(),'Binding proof file absent');spec=specs[name.removeprefix(PREFIX)];contract(proof,spec)
                original=bound(case/'initial.gmf').read_text();edited=bound(case/'edited.gmf').read_text();caller=bound(case/'caller.script').read_text();nested=bound(case/'nested.gmf').read_text()
                require(original.replace('Keep = 103;','Keep = 41;')==edited and 'Mark = Keep + Calls;' in edited and '[Mark] = ReadShared();' in nested and 'Create Spacecraft PrivateOnly;' in caller and 'Create Spacecraft PrivateOnly;' in edited,'Exact edit/private owner construction differs')
                if spec['factory_type']=='Cartesian':require('PrivateVector = PrivateOnly.EarthMJ2000Eq.Cartesian;' in caller+edited and 'SharedVector = SharedSC.EarthMJ2000Eq.Cartesian;' in caller+edited and 'Report BindingValues PrivateVector(1,1)' in caller+edited,'Supported actual vector assignment/report bridge missing')
                prototypes=load(bound(case/'prototypes-before.json'))
                for phase in proof['phases']:
                    tag=str(phase['phase']);require(load(bound(case/(tag+'-live-parameter-samples.json')))==phase['live_samples'] and load(bound(case/(tag+'-prototypes.json')))==prototypes and all(load(bound(case/(tag+'-invariants.json'))).values()) and [float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==phase['raw_markers'] and not bound(case/(tag+'-observer-error.txt')).read_text() and bound(case/(tag+'-binding-values.txt')).stat().st_size>0,'Raw live sample/value/source invariants differ')
                    messages=bound(case/(tag+'-messages.txt')).read_text();private=written(messages,spec,spec['root_private_reference'])
                    if spec['root_private_reference']!=spec['local_private_reference']:private+=written(messages,spec,spec['local_private_reference'])
                    require(private==phase['private_writes'] and written(messages,spec,spec['shared_reference'])==phase['shared_writes'],'Raw typed parameter Write bytes differ')
            else:
                checkpoint='function-vector-parameter-resource-routing' if name=='QtGui.NasaFunctionVectorResource' else 'subscriber-global-loop-Global'
                require(proof['checkpoint']==checkpoint,'Original regression proof differs')
            if version=='visible-v1':
                display=receipt['visible_display'];require(display['start_minimized'] and display['resizable'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible window policy differs')
                exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Visible trace missing');process=maps[traces[0].stem.removeprefix('trace-')];exes=[n for n in row['registered_command'] if n in receipt['binary_hashes']]
                require(len(exes)==1 and process['executable']==exes[0] and all(before['images'].get(n)==d for n,d in process['images'].items()),'Mapped native image identity differs')
                for n,d in before['images'].items():
                    if n==exes[0] or Path(n).name.startswith(('libGmatBase','libGmatUtil')):require(process['images'].get(n)==d,'Actual executable/core image unmapped')
            selected.append(item)
    require(len(selected)==32 and len(failures)==3,'Binding selected/failure count differs')
    require('taking address of rvalue' in bound(stage/'build-v1.log').read_text() and bound(stage/'NasaFunctionParameterBindingTests.compile-v1.cpp').is_file(),'Compile failure/source lost')
    negatives=[];samples={k:load(bound(repo/'build/gui-acceptance/function-parameter-bindings-visible-v1'/(PREFIX+k)/'proof.json')) for k in ['DryMass','Cartesian','UTCGregorian','FuelMass','OrbitSTM']}
    controls=[('configured-owner-substitution','DryMass',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('reference_owner_pointer',p['phases'][0]['live_samples'][1]['parameters'][0]['configured_owner_pointer'])),('caller-owner-substitution','DryMass',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('live_owner_pointer',p['phases'][0]['live_samples'][1]['parameters'][0]['root_owner_pointer'])),('wrong-native-parameter-type','DryMass',lambda p:p['phases'][0]['live_samples'][0]['parameters'][0].__setitem__('native_type','Variable')),('wrong-attached-tank','FuelMass',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['attached'].__setitem__('name','RootTank')),('wrong-vector-component','Cartesian',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['actual'].__setitem__(0,0)),('wrong-epoch-stamp','UTCGregorian',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('actual','bad')),('wrong-matrix-shape','OrbitSTM',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('shape',[3,3])),('omitted-repeat','DryMass',lambda p:p['phases'].pop())]
    for label,key,corrupt in controls:
        bad=copy.deepcopy(samples[key]);corrupt(bad)
        try:contract(bad,specs[key])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt live binding '+label)
    return dict(status='passed-fourteen-live-parameter-owner-routes-and-Global-promotion-key-repair',qualified_keys=[],selected=selected,retained_GUI_failures=failures,retained_compile_failures=1,new_binding_GUI_passes=28,affected_original_regression_GUI_passes=4,system_parameters_retained=321,native_bound_types=14,remaining_without_live_native_binding=307,binding_missions=84,function_calls=336,live_callback_samples=336,function_callback_samples=252,actual_parameter_owner_checks=672,actual_typed_Write_values=672,marker_scalars=420,visible_mapped_processes=16,visible_exposure_events=exposures,negative_controls=negatives,production_changed_files=['src/base/command/ManageObject.cpp'],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--stage',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();result=audit(Path(__file__).resolve().parents[3],args.stage.resolve());args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
