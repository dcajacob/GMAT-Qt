"""Audit active-state Clone/storage repair and all 44 named main math probes."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha

PREFIX='QtGui.NasaFunctionOwnedNamed'
KINDS={'default-clone','scalar-clone','matrix-clone','scalar-base-assignment','matrix-base-assignment','matrix-reshape'}
REG={'QtGui.NasaFunctionFrameArray':'NasaFunctionFrameTests.cpp','QtGui.NasaFunctionControlRecursion':'NasaFunctionControlTests.cpp','QtGui.NasaFunctionControlMutual':'NasaFunctionControlTests.cpp'}

def clone_contract(p,cases):
    require(p['checkpoint']=='math-node-active-state-clone-base-assignment' and p['outcome']=='passed' and p['factory_spellings']==44 and not p['failed_controls'] and len(p['rows'])==264 and {(v['factory_type'],v['case']) for v in p['rows']}=={(k,kind) for k in cases for kind in KINDS},'Native Clone control omitted')
    for row in p['rows']:
        spec=cases[row['factory_type']];kind=row['case'];function=spec['type']!='MathElement';source=dict(name='CloneSource',type=spec['native_type'],element_type=4,number='scalar' in kind,function=function,function_input=not function,value=17.25 if 'scalar' in kind else -987654321.0123e-45)
        if 'matrix' in kind:source.update(element_type=12,rows=1 if kind=='matrix-reshape' else 2,columns=2 if kind=='matrix-reshape' else 3,value=[-11,17] if kind=='matrix-reshape' else [2,-3,5.5,7,11,13])
        copied=dict(source);copied['name']='AssignDestination' if 'base-assignment' in kind else 'CloneSource'
        require(row['native_type']==spec['native_type'] and row['passed'] and row['source_unchanged'] and row['source']==row['source_after']==source and row['copied']==copied,'Active state/literal/flag/identity differs')
        if 'base-assignment' in kind:require(row['destination_after']==copied,'Assigned active state differs')
    strings=p['stored_string_controls'];require(len(strings)==2 and {v['base_assignment'] for v in strings}=={False,True},'Stored string control omitted')
    for row in strings:
        source=dict(name='StringSource',type='MathElement',element_type=7,number=False,function=False,function_input=True,value='orbit Ω string');copied=dict(source);copied['name']='StringDestination' if row['base_assignment'] else 'StringSource';require(row['passed'] and row['source']==source and row['copied']==copied,'Stored string/flag/identity differs')

def main_probe(p,spec):
    require(p['checkpoint']=='owned-expression-main-named-probe' and p['outcome']=='passed' and p['qualified'] is False and p['case']==spec['key'] and p['factory_type']==spec['type'] and p['category_id']==154,'Named math identity differs')
    require(p['factory_constructor']['present'] and p['factory_constructor']['name']=='FactoryOnly' and p['factory_constructor']['native_type']==spec['native_type'],'Native math factory differs')
    for phase in ['create','write_build']:
        row=p[phase];require(row['built'] and row['source_history_unchanged'] and row['configured']['present'] and row['configured']['name']=='NamedWitness' and row['configured']['native_type']==spec['native_type'],'Named math creation/lookup differs')
    for phase in ['write_run','recovery_run']:
        row=p[phase];require(row['completed'] and row['run_result']==0 and row['source_history_unchanged'] and float(row['marker_bytes'].strip())==42,'Named math/recovery run failed')
    require(p['write_run']['runtime']['present'] and p['write_run']['runtime']['name']=='NamedWitness' and p['write_run']['runtime']['native_type']==spec['native_type'] and p['recovery_build']['built'] and not p['recovery_build']['configured']['present'] and not p['recovery_run']['runtime']['present'],'Runtime math identity/recovery differs')

def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for path,digest in bindings.items():require(Path(path).is_file() and not Path(path).is_symlink() and sha(path)==digest,'Changed regular input '+path)
    def bound(p):p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def capsule(p):
        with tarfile.open(bound(p)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source capsule');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible=capsule(stage/'visible-executed-sources-v1.tar.gz');before=load(bound(stage/'visible-inputs-before-v1.json'));require(before==load(bound(stage/'visible-inputs-after-v1.json')) and {n:hashlib.sha256(b).hexdigest() for n,b in visible.items()}==before['sources'],'Visible inputs changed')
    catalog=json.loads(visible['src/qtgui/tests/NasaMathNodeCloneCases.json']);cases={v['type']:v for v in catalog['cases']};base=load(bound(repo/catalog['baseline_matrix']));require(sha(repo/catalog['baseline_matrix'])==catalog['baseline_matrix_sha256'] and len(cases)==44 and len({v['cpp_class'] for v in cases.values()})==42 and set(cases)=={v['factory_type'] for v in base['rows'] if v['category_id']==154} and len(base['rows'])==566 and not base['qualified_keys'],'Math universe narrowed')
    for version in [1,3]:
        root=stage/('clone-support-v'+str(version));launch=load(bound(root/'launch.json'));inputs=load(bound(root/'inputs-before.json'));require(inputs==load(bound(root/'inputs-after.json')) and launch['inputs_unchanged'] and launch['returncode']==(8 if version==1 else 0),'Native support source/status differs');frozen=capsule(stage/('clone-support-executed-sources-v'+str(version)+'.tar.gz'))
        for path,digest in inputs['sources'].items():require(path in frozen and hashlib.sha256(frozen[path]).hexdigest()==digest,'Support executed source differs')
        tests=list(ET.parse(bound(root/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')=='QtAcceptance.MathNodeClone' and (tests[0].find('failure') is None)==(version==3),'Support JUnit differs');p=load(bound(root/'proof.json'));require([json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',bound(root/'test.log').read_text())]==[p],'Support emitted proof differs')
        if version==1:
            require(p['outcome']=='failed' and len(p['rows'])==264 and len(p['failed_controls'])==266 and len(p['stored_string_controls'])==2 and all(not v['passed'] for v in p['rows']+p['stored_string_controls']) and {(v['factory_type'],v['case']) for v in p['rows']}=={(k,kind) for k in cases for kind in KINDS},'Original 266 control failures lost')
            errors=[v['error'] for v in p['rows']];require(sum('unsized table' in v for v in errors)==132 and sum('dimension error' in v for v in errors)==88 and sum('lost active value or flags' in v for v in errors)==44,'Original failure classes differ')
        else:clone_contract(p,cases);final_clone=p
    expected_main={PREFIX+v['key'] for v in cases.values()};all_final=expected_main|set(REG);versions=[('function-math-clone-headless-v3','headless-executed-sources-v3.tar.gz',expected_main),('function-math-clone-regression-headless-v3','regression-executed-sources-v3.tar.gz',set(REG)),('function-math-clone-visible-v1',None,all_final)]
    old=load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-attitude-resources/visible-inputs-before-v1.json'))['sources'];maps=load(bound(stage/'loaded-images-v1.json'))['processes'];launch=load(bound(stage/'visible-launch-v1.json'));require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==47,'Visible process selection incomplete');selected=[];proofs={};exposures=0
    for folder,archive,expected in versions:
        root=repo/'build/gui-acceptance'/folder;receipt=load(bound(root/'results.json'));require(receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal GUI attempt');frozen=capsule(stage/archive) if archive else visible
        for path,digest in receipt['source_hashes'].items():require(path in frozen and hashlib.sha256(frozen[path]).hexdigest()==digest,'GUI executed source differs')
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};require(set(active)==expected,'GUI selection differs')
        for name,row in active.items():
            require(row['status']=='passed' and row['returncode']==0,'GUI native case failed');log=bound(row['log']);case=log.parent;tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and not any(tests[0].find(v) is not None for v in ['failure','error','skipped']),'GUI JUnit differs');emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())];require(len(emitted)==1 and emitted[0]['outcome']=='passed','GUI native proof missing');p=emitted[0]
            if name in REG:require(receipt['source_hashes']['src/qtgui/tests/'+REG[name]]==old['src/qtgui/tests/'+REG[name]],'Original regression assertions changed')
            else:
                spec=cases[name.removeprefix(PREFIX)];main_probe(p,spec);require(load(bound(case/'proof.json'))==p,'Named math proof file differs');proofs[(folder,spec['type'])]=p
                for phase in ['create','write_build','recovery_build']:
                    text=bound(case/(phase+'.script')).read_text();require('Keep = 41;' in text and 'Keep = Keep + 1;' in text,'Independent control source differs')
                    if phase!='recovery_build':require('Create '+spec['type']+' NamedWitness;' in text,'Actual math Create missing')
                    else:require('NamedWitness' not in text,'Recovery retained named declaration')
                    require(('Write NamedWitness;' in text)==(phase=='write_build'),'Actual named Write source differs')
                for phase in ['write_run','recovery_run']:require(bound(case/(phase+'.values.txt')).read_text()==p[phase]['marker_bytes'] and 'Mission run completed.' in bound(case/(phase+'.messages.txt')).read_text(),'Raw main/control completion differs')
            if archive is None:
                display=receipt['visible_display'];require(display['start_minimized'] and display['resizable'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible policy differs');exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Process trace missing');process=maps.get(traces[0].stem.removeprefix('trace-'));exes=[v for v in row['registered_command'] if v in receipt['binary_hashes']];require(len(exes)==1 and process and process['executable']==exes[0],'Mapped process identity differs')
                for path,digest in process['images'].items():require(before['images'].get(path)==digest,'Mapped image changed')
                for path,digest in before['images'].items():
                    if path==exes[0] or Path(path).name.startswith(('libGmatBase','libGmatUtil')):require(process['images'].get(path)==digest,'Core image unmapped')
            selected.append(dict(invocation=folder,test=name))
    require(len(selected)==94 and all(sum(mode in v['invocation'] for v in selected)==47 for mode in ['headless','visible']),'GUI totals differ')
    matrix=load(bound(stage/'math-main-matrix-v1.json'));rows=copy.deepcopy(matrix['rows']);contexts={v['factory_type']:v.pop('repaired_math_main_witnesses') for v in rows};require(rows==base['rows'] and not matrix['qualified_keys'] and sum(bool(v) for v in contexts.values())==44,'Complete role table changed/qualified')
    for name in cases:
        items=contexts[name];require(len(items)==2 and {v['mode'] for v in items}=={'headless','visible'},'Math mode omitted')
        for v in items:require(dict(invocation=v['invocation'],test=PREFIX+cases[name]['key']) in selected and load(bound(v['proof']))==proofs[(v['invocation'],name)] and sha(v['proof'])==v['proof_sha256'],'Math context binding differs')
    negatives=[]
    for label,change in [('lost-input-flag',lambda p:p['rows'][2]['copied'].__setitem__('function_input',False)),('wrong-matrix-literal',lambda p:p['rows'][2]['source']['value'].__setitem__(0,99)),('wrong-destination-name',lambda p:p['rows'][3]['copied'].__setitem__('name','Wrong')),('missing-spelling',lambda p:p['rows'].pop())]:
        p=copy.deepcopy(final_clone);change(p)
        try:clone_contract(p,cases)
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt active state '+label)
    p=copy.deepcopy(proofs[('function-math-clone-visible-v1','Add')]);p['write_run']['completed']=False
    try:main_probe(p,cases['Add'])
    except (ValueError,AssertionError):negatives.append('failed-named-main')
    else:raise ValueError('Accepted failed main mission')
    return dict(status='passed-math-active-state-clone-and-44-named-main-routes',qualified_keys=[],selected=selected,retained_native_support_failures=1,retained_failed_active_state_controls=266,passed_active_state_controls=266,factory_spellings=44,concrete_classes=42,named_main_GUI_passes=88,affected_original_regression_GUI_passes=6,completed_named_Write_missions=88,completed_independent_recovery_missions=88,visible_mapped_processes=47,visible_exposure_events=exposures,negative_controls=negatives,production_changed_files=['src/base/math/MathNode.cpp'],derived_polymorphic_Copy_qualified=False,function_private_global_qualified=False,expression_graph_ownership_qualified=False,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
