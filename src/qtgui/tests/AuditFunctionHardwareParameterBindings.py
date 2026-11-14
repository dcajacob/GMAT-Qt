"""Audit live attached tank/thruster Parameter owners and independent literal fields."""
import argparse,copy,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha
from AuditFunctionParameterBindings import compare,written
TANK={f'FuelCenterOfMass{a}{s}' for s in ['', '_BCS'] for a in 'XYZ'}|{f'FuelMomentOfInertia{a}{s}' for s in ['', '_BCS'] for a in ['XX','XY','XZ','YY','YZ','ZZ']}|{'Pressure','Temperature','RefTemperature','Volume','FuelDensity'}
THRUSTER={'DutyCycle','ThrustScaleFactor','GravitationalAccel'}|{f'{a}{i}' for a in 'CK' for i in range(1,17)}|{f'ThrustDirection{i}' for i in range(1,4)}
TYPES=TANK|THRUSTER;PILOT={'DutyCycle','C16','FuelCenterOfMassX_BCS','FuelMomentOfInertiaXY_BCS'};PREFIX='QtGui.NasaFunctionHardwareParameterBinding'


def recipe(key):
    if 'CenterOfMass' in key:return [-.5,.75,1.25],.0625
    if 'MomentOfInertia' in key:return ([10,20,30],.25) if key.removesuffix('_BCS')[-2:] in {'XX','YY','ZZ'} else ([.125,.25,.5],.015625)
    if key=='Pressure':return [1000,1500,2000],1
    if key in {'Temperature','RefTemperature'}:return [15,20,25],.125
    if key=='Volume':return [1,1.5,2],.015625
    if key=='FuelDensity':return [900,1000,1100],1
    if key=='DutyCycle':return [.25,.5,.75],.015625
    if key=='GravitationalAccel':return [9,10,11],.0625
    if key=='C1':return [100,200,300],1
    if key=='K1':return [300,400,500],1
    if key[0] in 'CK' and key[1:].isdigit():return [.125,.25,.5],.015625
    require(key=='ThrustScaleFactor' or key.startswith('ThrustDirection'),'Unknown hardware recipe');return [1,2,3],.0625


def contract(p,spec):
    key=spec['factory_type'];meta=spec['metadata'];tank=key in TANK;kind='Tank' if tank else 'Engine';values,step=recipe(key)
    require(spec['hardware_kind']==kind and spec['hardware_type']==('ChemicalTank' if tank else 'ChemicalThruster') and spec['setter']==key.removesuffix('_BCS') and [spec[n] for n in ['root_value','private_value','shared_value']]==values and spec['step']==step and spec['absolute_bound']==1e-12,'Authored independent hardware recipe differs')
    require(p['checkpoint']=='live-function-hardware-parameter-owner-binding' and p['outcome']=='passed' and p['case']==p['argument']==key and p['family']=='AttachedHardware' and p['route']=='attached-hardware-field' and p['full_system_parameter_inventory']==321 and set(p['initial_batch_types'])==TYPES and p['missions']==3 and p['function_calls']==12 and p['live_callback_samples']==12 and p['actual_parameter_owner_checks']==24 and p['file_decisions']==[dict(saved=False),dict(saved=True)],'Hardware proof identity/counts differ')
    require([v['phase'] for v in p['phases']]==[0,1,2] and [v['explicit_rebuild'] for v in p['phases']]==[True,True,False],'Hardware no-Build repeat lost')
    for phase in p['phases']:
        require(phase['raw_markers']==[42,43,44,103,104] and phase['callback_private_maps_accessed_only_while_executing'] and len(phase['live_samples'])==4 and len(phase['private_writes'])==len(phase['shared_writes'])==4,'Hardware callbacks/Write/markers differ');roots={};hardware_roots={}
        for i,sample in enumerate(phase['live_samples']):
            inside=i>0;calls=i;target='ScopePulse' if inside else 'MainPulse'
            require(sample['inside_function']==inside and sample['calls']==calls and sample['function']==('ReadShared' if inside else '') and sample['global_map_is_value_snapshot']==(not inside) and len(sample['parameters'])==2 and sample['provider']==f'Propagate PulseProp({target}) {{{target}.ElapsedSecs = 1}};','Hardware callback scope/provider differs')
            for j,row in enumerate(sample['parameters']):
                shared=j==1;ref=spec['shared_reference'] if shared else spec['local_private_reference'] if inside else spec['root_private_reference'];hardware_name=('Shared' if shared else 'Local' if inside else 'Root')+kind;expected=values[2 if shared else 1 if inside else 0]+(calls*step if inside else 0)
                require(row['shared']==shared and row['reference']==ref and row['factory_type']==key and row['native_type']==meta['native_type'] and row['return_type']==meta['return_type']==4 and row['owner_type']==meta['owner_type_id'] and row['owner_type_name']=='Spacecraft' and row['owner_name']==('SharedSC' if shared else 'PrivateOnly'),'Hardware Parameter/Spacecraft type differs')
                for k in ['parameter_pointer','reference_owner_pointer','live_owner_pointer','root_owner_pointer','configured_owner_pointer']:require(re.fullmatch('[0-9a-f]+',row[k]) and row[k]!='0','Invalid hardware owner identity')
                require(row['reference_owner_pointer']==row['live_owner_pointer']!=row['configured_owner_pointer'] and row['owner_global']==shared and row['owner_local']==(inside and not shared),'Hardware Parameter reference/owner flags differ')
                if not inside:roots[shared]=row['root_owner_pointer']
                require(row['root_owner_pointer']==roots[shared] and (row['live_owner_pointer']==roots[shared])==(shared or not inside),'Hardware owner aliases root incorrectly')
                require(row['shape']==[] and row['absolute_bound']==1e-12,'Hardware return shape/bound changed')
                for n in ['actual','expected','direct_owner_property_value']:compare(row[n],expected,1e-12)
                compare((phase['shared_writes'] if shared else phase['private_writes'])[i],expected,1e-12)
                a=row['attached'];require(a['name']==hardware_name and a['type']==spec['hardware_type'] and a['property']==key and a['setter']==spec['setter'],'Actual attached hardware type/property differs')
                for k in ['pointer','root_pointer','configured_pointer']:require(re.fullmatch('[0-9a-f]+',a[k]) and a[k]!='0','Invalid attached hardware identity')
                if not inside:hardware_roots[shared]=a['root_pointer']
                require(a['root_pointer']==hardware_roots[shared] and a['pointer']!=a['configured_pointer'] and (a['pointer']==a['root_pointer'])==(shared or not inside),'Actual private/Global hardware clone differs')
                compare(a['direct_value'],expected,1e-12);compare(a['setter_value'],expected,1e-12);require(a['location']==[0,0,0] and len(a['R_SB'])==9,'Hardware zero-offset/rotation shape differs')
                matrix=a['R_SB'];require(all(isinstance(v,(float,int)) and math.isfinite(v) for v in matrix),'Nonfinite hardware rotation')
                for x in range(3):
                    for y in range(3):require(abs(sum(matrix[x*3+k]*matrix[y*3+k] for k in range(3))-(1 if x==y else 0))<=1e-12,'Hardware rotation not orthonormal')
                if tank:
                    require(matrix==[1,0,0,0,1,0,0,0,1],'Tank BCS oracle identity lost');center=[0,0,0]
                    if 'CenterOfMass' in key:center['XYZ'.index(key.removesuffix('_BCS')[-1])]=expected
                    compare(a['fuel_CM'],center,1e-12)
                else:require(a['fuel_CM']==[],'Unexpected thruster CM claim')


def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,d in bindings.items():require(Path(n).is_file() and not Path(n).is_symlink() and sha(n)==d,'Changed regular binding '+n)
    def bound(p):
        p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def capsule(n):
        with tarfile.open(bound(stage/n)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source member');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible=capsule('visible-executed-sources-v1.tar.gz');before=load(bound(stage/'visible-inputs-before-v1.json'));require(before==load(bound(stage/'visible-inputs-after-v1.json')) and before['sources']=={n:hashlib.sha256(v).hexdigest() for n,v in visible.items()},'Visible inputs changed')
    for n in ['AuditFunctionParameterBindings.py','BuildNasaCoverageExtension.py']:require(hashlib.sha256(visible['src/qtgui/tests/'+n]).hexdigest()==sha(repo/'src/qtgui/tests'/n),'Imported audit helper changed')
    c=json.loads(visible['src/qtgui/tests/NasaFunctionHardwareParameterCases.json']);specs={v['factory_type']:v for v in c['hardware_binding_cases']};require(len(c['cases'])==321 and set(c['initial_types'])==set(c['selected_arguments'])==set(specs)==TYPES and len(TYPES)==61 and not c['qualified_keys'] and all(not v['qualified'] for v in c['cases']),'Complete321 inventory narrowed/promoted')
    m=load(bound(repo/c['baseline']));require(sha(repo/c['baseline'])==c['baseline_sha256'] and {v['factory_type']:v['native_parameter_metadata'] for v in m['rows'] if v['role']=='owner-bound-system-parameter'}=={v['factory_type']:v['metadata'] for v in c['cases']},'Original metadata changed')
    maps=load(bound(stage/'loaded-images-v1.json'))['processes'];launch=load(bound(stage/'visible-launch-v1.json'));require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==61,'Visible mapping incomplete')
    selected=[];failures=[];exposures=0
    for version in ['headless-v1','headless-v2','headless-v3','visible-v1']:
        inv='function-hardware-parameter-bindings-'+version;q=load(bound(repo/'build/gui-acceptance'/inv/'results.json'));frozen=visible if version=='visible-v1' else capsule(version.replace('headless-','headless-executed-sources-')+'.tar.gz');require(q.get('finished_utc') and q['end_reason']=='completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest()==d for n,d in q['source_hashes'].items()),'Executed source/terminal state differs')
        rows={v['name']:v for v in q['results'] if v['status']!='not-run'};keys=PILOT if version in ['headless-v1','headless-v2'] else TYPES-PILOT if version=='headless-v3' else TYPES;require(set(rows)=={PREFIX+k for k in keys},'Exact hardware selection differs')
        for name,row in rows.items():
            case=bound(row['log']).parent;key=name.removeprefix(PREFIX);item=dict(invocation=inv,test=name)
            if version=='headless-v1':
                text=bound(row['log']).read_text();require(row['status']=='failed' and row['returncode']!=0 and (('has no parameter defined with' in text and 'Spacecraft' in text) if key in TANK else 'Identity/zero-offset hardware fixture differs' in text),'Retained hardware fixture failure lost');failures.append(item);continue
            require(row['status']=='passed' and row['returncode']==0,'Selected hardware case failed');tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Hardware native JUnit differs');emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',bound(row['log']).read_text())];require(len(emitted)==1 and load(bound(case/'proof.json'))==emitted[0],'Hardware emitted/stored proof differs');p=emitted[0];spec=specs[key];contract(p,spec)
            require(load(bound(case/'phases.json'))==p['phases'],'Raw hardware phases differ');initial=bound(case/'initial.gmf').read_text();edited=bound(case/'edited.gmf').read_text();require(initial.count('Keep = 103;')==1 and initial.replace('Keep = 103;','Keep = 41;')==edited,'Hardware file edit differs')
            for phase in p['phases']:
                tag=str(phase['phase']);messages=bound(case/(tag+'-messages.txt')).read_text();require(load(bound(case/(tag+'-live-parameter-samples.json')))==phase['live_samples'] and all(load(bound(case/(tag+'-invariants.json'))).values()) and load(bound(case/(tag+'-prototypes.json')))==load(bound(case/'prototypes-before.json')),'Raw hardware samples/invariants/prototypes differ');private=written(messages,spec,spec['root_private_reference'])+written(messages,spec,spec['local_private_reference']);require(private==phase['private_writes'] and written(messages,spec,spec['shared_reference'])==phase['shared_writes'],'Raw hardware Write differs');compare([float(v) for v in bound(case/(tag+'-binding-values.txt')).read_text().split()],[v['actual'] for sample in phase['live_samples'] for v in sample['parameters']],1e-12);require([float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==phase['raw_markers'],'Raw hardware markers differ')
            if version=='visible-v1':
                d=q['visible_display'];require(d['start_minimized'] and d['resizable'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'] and q['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible policy differs');exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Visible trace missing');mapped=maps[traces[0].stem.removeprefix('trace-')];exes=[n for n in row['registered_command'] if n in q['binary_hashes']];require(len(exes)==1 and mapped['executable']==exes[0] and all(before['images'].get(n)==d for n,d in mapped['images'].items()),'Actual mapped image differs')
                for n,d in before['images'].items():
                    if n==exes[0] or Path(n).name.startswith(('libGmatBase','libGmatUtil')):require(mapped['images'].get(n)==d,'Required image unmapped')
            selected.append(item)
    require(len(selected)==122 and len(failures)==4,'Hardware selection/failure count differs')
    template=load(bound(repo/'build/gui-acceptance/function-hardware-parameter-bindings-visible-v1'/(PREFIX+'FuelCenterOfMassX_BCS')/'proof.json'));controls=[('wrong-owner-reference',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('reference_owner_pointer','1')),('private-hardware-aliases-root',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['attached'].__setitem__('pointer',p['phases'][0]['live_samples'][1]['parameters'][0]['attached']['root_pointer'])),('global-hardware-is-clone',lambda p:p['phases'][0]['live_samples'][1]['parameters'][1]['attached'].__setitem__('pointer','1')),('wrong-attached-type',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['attached'].__setitem__('type','ChemicalThruster')),('wrong-direct-field',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['attached'].__setitem__('direct_value',999)),('wrong-literal',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0].__setitem__('actual',999)),('nonidentity-tank',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['attached']['R_SB'].__setitem__(0,2)),('nonzero-tank-offset',lambda p:p['phases'][0]['live_samples'][1]['parameters'][0]['attached']['location'].__setitem__(0,1)),('missing-no-Build',lambda p:p['phases'].pop()),('wrong-marker',lambda p:p['phases'][0]['raw_markers'].__setitem__(0,999))];negatives=[]
    for label,corrupt in controls:
        p=copy.deepcopy(template);corrupt(p)
        try:contract(p,specs['FuelCenterOfMassX_BCS'])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt hardware proof '+label)
    return dict(status='passed-61-attached-hardware-parameter-owner-bindings',qualified_keys=[],selected=selected,retained_GUI_failures=failures,retained_compile_failures=0,retained_interrupted_cases=[],new_parameter_types=61,tank_parameter_types=23,thruster_parameter_types=38,missions=366,function_calls=1464,live_callback_samples=1464,parameter_owner_checks=2928,actual_attached_hardware_checks=2928,direct_hardware_field_checks=2928,actual_Write_values=2928,actual_Report_values=2928,marker_values=1830,visible_mapped_processes=61,visible_exposure_events=exposures,negative_controls=negatives,native_bound_types=120,remaining_without_live_native_binding=201,factory_or_readable_role_exclusions=[],production_changed_files=[],tank_BCS_identity_fixture_only=True,general_hardware_transform_science_qualified=False,thruster_firing_science_qualified=False,whole_scope_qualified=False,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();d=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(d,indent=2)+'\n');print(json.dumps(d,indent=2))
