"""Audit finite-burn readonly metadata,actual choosers and unaffected reads."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha
from AuditFunctionFiniteBurnBindings import contract as finite_contract
from AuditFunctionParameterBindings import compare,written
TYPES={'TotalMassFlowRate','TotalAcceleration1','TotalAcceleration2','TotalAcceleration3','TotalThrust1','TotalThrust2','TotalThrust3'}
PREFIX='QtGui.NasaFiniteBurnReadOnly'
FINITE='QtGui.NasaFunctionFiniteBurnBinding'
CANON='QtGui.NativeNasaBurnCanonicalParameters'


def contract(proof,spec):
    key=spec['factory_type'];reference='Derived.'+key
    require(proof['checkpoint']=='finite-burn-readonly-metadata-and-actual-choosers' and proof['outcome']=='passed' and proof['case']==key and proof['reference']==reference and proof['unit']==spec['unit'] and proof['full_system_parameter_inventory']==321 and proof['factory_is_settable'] is False and proof['info_is_settable'] is False and proof['raw_write_build_rejected'] and proof['raw_source_recovery'] and proof['missions']==3 and proof['native_setter_atomic_rejections']==3 and proof['raw_values']==9,'Readonly proof identity/count/flags differs')
    left=proof['left_picker'];require(all(left[k] is False for k in ['factory_settable','info_settable','runtime_settable','finite_owner_offered','property_offered','typed_readonly_accepted']) and left['writable_variable_accepted'] and left['source_isolated'],'Readonly left picker accepted derived write or rejected mutable control')
    right=proof['readable_pickers'];require(len(right)==2 and [v['commit'] for v in right]==[False,True] and all(v['owner']=='Derived' and v['property']==key and v['reference']==reference and v['existing_selected_count']==1 and v['ok_enabled'] for v in right),'Readonly readable Report picker omitted/rejected value')
    require(proof['invalid_apply_diagnostic'] and [v['phase'] for v in proof['runs']]==[0,1,2] and [v['explicit_build'] for v in proof['runs']]==[True,True,False],'Readonly invalid Apply/no-Build repeat lost')
    for run in proof['runs']:
        require(run['values']==[0,0,42],'Readonly inactive values or continuation differ');native=run['native']
        require(native['phase']==run['phase'] and native['native_type']==key and native['unit']==spec['unit'] and native['is_settable'] is False and native['before']==native['after']==0 and native['owner_source_unchanged'] and 'Unknown ELEMENT id' in native['diagnostic'],'Readonly native rejected setter altered value/owner or wrong metadata')
        for k in ['parameter_pointer','owner_pointer','reference_owner_pointer']:require(re.fullmatch('[0-9a-f]+',native[k]) and native[k]!='0','Invalid readonly live identity')
        require(native['reference_owner_pointer']==native['owner_pointer'],'Readonly actual native owner reference differs')


def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,d in bindings.items():require(Path(n).is_file() and not Path(n).is_symlink() and sha(n)==d,'Changed regular input '+n)
    def bound(p):
        p=Path(p);require(str(p) in bindings,'Unbound artifact '+str(p));return p
    def capsule(name):
        with tarfile.open(bound(stage/name)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source member');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible=capsule('visible-executed-sources-v1.tar.gz');before=load(bound(stage/'visible-inputs-before-v1.json'));require(before==load(bound(stage/'visible-inputs-after-v1.json')) and before['sources']=={n:hashlib.sha256(v).hexdigest() for n,v in visible.items()},'Visible inputs changed')
    for n in ['AuditFunctionFiniteBurnBindings.py','AuditFunctionParameterBindings.py','BuildNasaCoverageExtension.py']:require(hashlib.sha256(visible['src/qtgui/tests/'+n]).hexdigest()==sha(repo/'src/qtgui/tests'/n),'Imported auditor helper source changed')
    catalog=json.loads(visible['src/qtgui/tests/NasaFiniteBurnReadOnlyCases.json']);metadata={v['factory_type']:v['metadata'] for v in catalog['cases']};specs={v['factory_type']:v for v in catalog['readonly_cases']};finite_specs={v['factory_type']:v for v in catalog['binding_cases']}
    require(len(metadata)==321 and set(catalog['initial_types'])==set(specs)==TYPES and not catalog['qualified_keys'] and all(not v['qualified'] for v in catalog['cases']+catalog['readonly_cases']),'Complete readonly321 catalog narrowed/promoted');matrix=load(bound(repo/catalog['baseline']));require(sha(repo/catalog['baseline'])==catalog['baseline_sha256'] and {v['factory_type']:v['native_parameter_metadata'] for v in matrix['rows'] if v['role']=='owner-bound-system-parameter'}==metadata,'Original metadata history changed')
    for key,spec in specs.items():require(spec['expected_settable'] is False and spec['reference_settable']=='N' and spec['metadata']==metadata[key] and spec['metadata']['settable'] is True and spec['unit']==finite_specs[key]['unit'],'Readonly transition/history or unit changed')
    native_contract=load(bound(stage/'native-contract-v1.json'));doc=bound(repo/native_contract['reference']);require(sha(doc)==native_contract['reference_sha256'],'Readonly calculation reference changed')
    for key in TYPES:
        text=doc.read_text();a=text.index('<entry><guilabel>'+key+'</guilabel></entry>');block=text[a:text.index('</row>',a)];require(re.search(r'</guilabel></entry>\s*<entry>N</entry>',block),'Guide readonly declaration changed')
    prior=load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-finite-burn-bindings/visible-inputs-before-v1.json'))['sources']
    for n in ['NasaFunctionFiniteBurnBindingTests.cpp','NasaBurnCanonicalParameterTests.cpp','NasaFunctionBurnParameterBindingTests.cpp']:
        require(hashlib.sha256(visible['src/qtgui/tests/'+n]).hexdigest()==prior['src/qtgui/tests/'+n],'Original affected/writable control assertion source changed')
    maps=load(bound(stage/'loaded-images-v1.json'))['processes'];launch=load(bound(stage/'visible-launch-v1.json'));require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==15,'Visible readonly mapping incomplete')
    selected=[];failures=[];interrupted=[];exposures=0
    new={PREFIX+k for k in TYPES};regs={FINITE+k for k in TYPES}|{CANON};versions=['headless-v1','headless-v2','headless-v3','headless-v4','headless-v5','headless-v6','visible-v1']
    for version in versions:
        invocation='finite-burn-readonly-'+version;receipt=load(bound(repo/'build/gui-acceptance'/invocation/'results.json'));frozen=visible if version=='visible-v1' else capsule(version.replace('headless-','headless-executed-sources-')+'.tar.gz')
        require(receipt.get('finished_utc') and receipt['end_reason']==('interrupted' if version=='headless-v3' else 'completed-selection') and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest()==d for n,d in receipt['source_hashes'].items()),'Readonly executed source/terminal state differs')
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};expected_names=new if version in ['headless-v1','headless-v4'] else {PREFIX+'TotalMassFlowRate'} if version in ['headless-v2','headless-v5'] else {PREFIX+'TotalMassFlowRate',PREFIX+'TotalAcceleration1'} if version=='headless-v3' else (new-{PREFIX+'TotalMassFlowRate'})|regs if version=='headless-v6' else new|regs;require(set(active)==expected_names,'Exact readonly selection differs')
        for name,row in active.items():
            log=bound(row['log']);case=log.parent;item=dict(invocation=invocation,test=name)
            if version in ['headless-v1','headless-v2','headless-v3','headless-v4']:
                if version=='headless-v3' and name.endswith('TotalAcceleration1'):
                    require(row['status']=='interrupted','Interrupted modal diagnostic relabeled');interrupted.append(item);continue
                require(row['status']=='failed' and row['returncode']!=0,'Retained readonly failure status lost')
                if version in ['headless-v1','headless-v2']:require('Readonly read/rejected native setter changed source/history/prototypes/file' in log.read_text(),'Prototype fixture failure lost')
                if version=='headless-v2':
                    invariants=load(bound(case/'0-invariants.json'));a=load(bound(case/'prototypes-before.json'));b=load(bound(case/'0-prototypes.json'));require(invariants==dict(history=True,not_running=True,prototypes=False,saved=True,source=True) and {k for k in a if a[k]!=b[k]}=={'Sat'} and a['Sat'].replace('Sat.NAIFId = -10001001;','Sat.NAIFId = -10002001;').replace('Sat.NAIFIdReferenceFrame = -9001001;','Sat.NAIFIdReferenceFrame = -9002001;')==b['Sat'],'Exact default-ID fixture diagnostic changed')
                if version=='headless-v3':require('Timeout' in log.read_text() and not (case/'left-picker.json').exists(),'Modal timeout fixture history lost')
                if version=='headless-v4':
                    left=load(bound(case/'left-picker.json'));require('Derived finite-burn parameter is incorrectly advertised as writable' in log.read_text() and all(left[k] is True for k in ['factory_settable','info_settable','runtime_settable','finite_owner_offered','property_offered','typed_readonly_accepted','writable_variable_accepted','source_isolated']) and all(load(bound(case/'0-invariants.json')).values()),'Actual metadata/chooser writable bug evidence lost')
                failures.append(item);continue
            require(row['status']=='passed' and row['returncode']==0,'Readonly selected case failed');tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and tests[0].find('failure') is None,'Readonly native JUnit differs')
            emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())];require(len(emitted)==1 and load(bound(case/'proof.json'))==emitted[0],'Readonly emitted/stored proof differs');proof=emitted[0]
            if name in new:
                spec=specs[name.removeprefix(PREFIX)];contract(proof,spec);initial=bound(case/'initial.script').read_text();accepted=bound(case/'accepted.script').read_text();reference=proof['reference'];old="'Read derived' Out = 0; % keep readonly context";new_statement="'Read derived' Out = "+reference+'; % keep readonly context';require(initial.count(old)==1 and initial.replace(old,new_statement)==accepted and 'Sat.NAIFId = -116001;' in initial and 'Sat.NAIFIdReferenceFrame = -8116001;' in initial,'Readonly exact editor edit/explicit fixture IDs differ')
                require(load(bound(case/'left-picker.json'))==proof['left_picker'] and load(bound(case/'readable-pickers.json'))==proof['readable_pickers'] and load(bound(case/'runs.json'))==proof['runs'] and load(bound(case/'native-setter-rejections.json'))==[v['native'] for v in proof['runs']],'Readonly raw metadata/selector/setter records differ')
                invalid=load(bound(case/'invalid-apply.json'));require(invalid['destination']==invalid['expression']==reference and invalid['diagnostic']==proof['invalid_apply_diagnostic'] and invalid['source_history_file_prototypes_isolated'],'Readonly invalid Apply evidence differs')
                bad=bound(case/'invalid-source.script').read_text();require(accepted.replace(new_statement,"'Read derived' "+reference+' = 17.25; % keep readonly context')==bad and any(reference in v and 'Failed to set ElementWrapper' in v for v in load(bound(case/'invalid-build-errors.json'))),'Readonly raw write Build rejection lost')
                prototypes=load(bound(case/'prototypes-before.json'))
                for run in proof['runs']:
                    tag=str(run['phase']);require(run['source']==(initial if run['phase']==0 else accepted) and [float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==run['values'] and all(load(bound(case/(tag+'-invariants.json'))).values()) and load(bound(case/(tag+'-prototypes.json')))==prototypes,'Readonly raw values/source/history/prototypes differ')
            elif name.startswith(FINITE):
                spec=finite_specs[name.removeprefix(FINITE)];finite_contract(proof,spec)
                for phase in proof['phases']:
                    tag=str(phase['phase']);require(load(bound(case/(tag+'-live-parameter-samples.json')))==phase['live_samples'] and all(load(bound(case/(tag+'-invariants.json'))).values()),'Affected original finite raw samples/invariants differ');messages=bound(case/(tag+'-messages.txt')).read_text();require(written(messages,spec,spec['root_private_reference'])==phase['private_writes'] and written(messages,spec,spec['shared_reference'])==phase['shared_writes'],'Affected original finite Write bytes differ');compare([float(v) for v in bound(case/(tag+'-binding-values.txt')).read_text().split()],[p['actual'] for sample in phase['live_samples'] for p in sample['parameters']],1e-12)
            else:
                require(proof['checkpoint']=='burn-canonical-parameters' and proof['components']==['Element1','Element2','Element3'] and proof['rows']==6 and proof['raw_report_values']==102 and len(proof['observed'])==6 and all(len(v)==17 for v in proof['observed']) and proof['max_error']<1e-10,'Original writable-impulse control scope differs');compare(proof['observed'],proof['expected'],1e-10);raw=[[float(v) for v in line.split(',')] for line in bound(case/'values.csv').read_text().splitlines() if line.strip()];compare(raw,proof['observed'],1e-10)
            if version=='visible-v1':
                display=receipt['visible_display'];require(display['start_minimized'] and display['resizable'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible readonly policy differs');exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Visible readonly trace missing');process=maps[traces[0].stem.removeprefix('trace-')];exes=[n for n in row['registered_command'] if n in receipt['binary_hashes']];require(len(exes)==1 and process['executable']==exes[0] and all(before['images'].get(n)==d for n,d in process['images'].items()),'Actual readonly mapped image differs')
                for n,d in before['images'].items():
                    if n==exes[0] or Path(n).name.startswith(('libGmatBase','libGmatUtil')):require(process['images'].get(n)==d,'Actual readonly executable/core image unmapped')
            selected.append(item)
    require(len(selected)==30 and len(failures)==16 and len(interrupted)==1,'Readonly selected/failure/interruption count differs')
    key='TotalMassFlowRate';template=load(bound(repo/'build/gui-acceptance/finite-burn-readonly-visible-v1'/(PREFIX+key)/'proof.json'));controls=[('factory-writable',lambda p:p.__setitem__('factory_is_settable',True)),('cache-writable',lambda p:p.__setitem__('info_is_settable',True)),('offered-left-property',lambda p:p['left_picker'].__setitem__('property_offered',True)),('typed-left-accepted',lambda p:p['left_picker'].__setitem__('typed_readonly_accepted',True)),('blocked-variable-control',lambda p:p['left_picker'].__setitem__('writable_variable_accepted',False)),('readable-reference-omitted',lambda p:p['readable_pickers'][1].__setitem__('ok_enabled',False)),('runtime-writable',lambda p:p['runs'][0]['native'].__setitem__('is_settable',True)),('setter-mutated-value',lambda p:p['runs'][0]['native'].__setitem__('after',17.25)),('raw-write-build-accepted',lambda p:p.__setitem__('raw_write_build_rejected',False)),('omitted-no-Build-repeat',lambda p:p['runs'].pop())];negatives=[]
    for label,corrupt in controls:
        bad=copy.deepcopy(template);corrupt(bad)
        try:contract(bad,specs[key])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt readonly proof '+label)
    return dict(status='passed-seven-finite-burn-readonly-metadata-and-actual-GUI-write-rejection',qualified_keys=[],selected=selected,new_readonly_GUI_passes=14,affected_finite_GUI_passes=14,unchanged_writable_impulse_GUI_passes=2,retained_GUI_failures=failures,retained_interrupted_cases=interrupted,retained_timeout_failures=1,retained_compile_failures=0,metadata_constructor_types_corrected=7,system_parameters_retained=321,native_bound_types=57,remaining_without_live_native_binding=264,readonly_missions=42,native_setter_atomic_rejections=42,readonly_raw_values=126,unmanaged_factory_readonly_checks=14,cache_readonly_checks=14,runtime_readonly_checks=42,actual_left_picker_rejections=14,actual_readable_Report_pickers=28,typed_invalid_Apply_rejections=14,raw_write_Build_rejections=14,positive_RHS_Apply_recoveries=14,affected_finite_missions=42,affected_finite_parameter_owner_checks=672,affected_finite_live_unit_checks=672,affected_finite_Write_values=672,affected_finite_Report_values=672,writable_impulse_maneuvers=6,writable_impulse_raw_values=204,visible_mapped_processes=15,visible_exposure_events=exposures,negative_controls=negatives,production_changed_files=['src/base/parameter/BurnParameters.cpp'],factory_or_readable_role_exclusions=[],whole_scope_qualified=False,function_readonly_flags_directly_observed=False,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--stage',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();result=audit(Path(__file__).resolve().parents[3],args.stage.resolve());args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
