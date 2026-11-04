"""Audit remaining-resource scope witnesses, Copy controls and exact source versions."""
import argparse,copy,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha

PREFIX='QtGui.NasaFunctionRemainingResource'
META='QtGui.NasaFunctionResourceMetadata'
REG={
 'QtGui.NasaReadonlyTotalMass':'NasaReadonlyTests.cpp',
 'QtGui.NativeNasaFunctionValidationIsolation':'NasaFunctionValidationIsolationTests.cpp',
 'QtGui.NativeNasaBurnObjectLoopImpulse':'NasaBurnObjectLoopTests.cpp',
 'QtGui.NasaFunctionFrameArray':'NasaFunctionFrameTests.cpp',
 'QtGui.NasaFunctionControlRecursion':'NasaFunctionControlTests.cpp',
 'QtGui.NasaFunctionControlMutual':'NasaFunctionControlTests.cpp',
 'QtGui.CustomFovControls':'CustomFovControlsTests.cpp',
 'QtGui.NativeNasaForceControlInventoryBasic':'NasaForceControlInventoryTests.cpp',
}
P2={'ForceModel','ODEModel','PolyhedronGravityModel','NRLMSISE00','BatchEstimatorInv','VF13ad','TextEphemFile','XYPlot','EphemerisFile','OpenGLPlot','Enhanced3DView','GroundTrackPlot','GroundTrack','OwnedPlot','DynamicDataDisplay','EclipseLocator','ContactLocator','IntrusionLocator','Smoother'}
F3={'SolarRadiationPressure','FuelTank','Thruster','TrackingFileSet','ThrustHistoryFile','PlanetographicRegion'}

def same(a,b):
    if isinstance(a,bool) or isinstance(b,bool):return type(a)==type(b) and a==b
    if isinstance(a,(int,float)) and isinstance(b,(int,float)):return math.isfinite(a) and math.isfinite(b) and abs(a-b)<1e-12
    if isinstance(a,list) and isinstance(b,list):return len(a)==len(b) and all(same(x,y) for x,y in zip(a,b))
    return a==b

def value(text,kind):
    text=text.strip()
    if kind in {'real','integer'}:return float(text)
    if kind=='vector':return [float(x) for x in re.sub(r'[\[\]{},]',' ',text).split()]
    if kind=='boolean':require(text.lower() in {'true','false'},'Invalid boolean literal');return text.lower()=='true'
    return text[1:-1] if text.startswith("'") and text.endswith("'") else text

def resource(p,spec,root=''):
    first=value(spec['first'].replace('@ROOT@',root),spec['kind']);second=value(spec['second'].replace('@ROOT@',root),spec['kind'])
    require(p['outcome']=='passed' and p['checkpoint']=='remaining-resource-main-local-global' and
            all(p[k]==spec[k] for k in ['type','native_type','field','kind','read_only']) and
            p['case']==spec['key'] and p.get('caller_same_name_shadow',False)==spec.get('caller_same_name_shadow',False) and
            same(p['first'],first) and same(p['second'],second) and p['missions']==3 and p['function_calls']==12 and
            p['file_decisions']==[{'saved':False},{'saved':True}],'Resource scope/literal/file decisions differ')
    require([v['phase'] for v in p['phases']]==[0,1,2] and [v['explicit_rebuild'] for v in p['phases']]==[True,True,False],'Missing resource repeat')
    for phase in p['phases']:
        require(phase['global'] and not phase['local'] and not phase['private_leaked'] and
                same(phase['shared_value'],first) and same(phase['shadow_value'],second) and phase['raw_values']==[41,41,41,103,104],'Global/private/caller field or continuation differs')
        for key,expected,count in [('shared_writes',first,4),('private_writes',second,3),('shadow_writes',second,1)]:
            require(len(phase[key])==count and all(same(v,expected) for v in phase[key]),'Main/private/global Write literals differ')
        identities=phase['identities'];require([v['function'] for v in identities]==['ReadShared','ReadShared','NestedShared'],'Missing direct/nested identity')
        for row in identities:require(row['global_store_object_pointer']==row['global_object_pointer'] and row['global_object_pointer'] not in {'0','',row['configured_prototype_pointer']},'Global identity/prototype differs')

def metadata(p,cases):
    require(p['outcome']=='passed' and p['candidate_count']==len(p['rows'])==45 and not p['unconstructed'] and p['configured_source_history_unchanged'],'Native metadata incomplete')
    rows={v['factory_type']:v for v in p['rows']};require(set(rows)==set(cases),'Metadata candidate omitted')
    for name,row in rows.items():
        require(row['constructed'] and row['native_type']==cases[name]['native_type'] and not row['automatic_global'] and not row['default_global'] and len(row['fields'])==row['parameter_count'],'Native constructor/metadata partition differs')
    # Raw getter observations and their errors remain; uninitialized model state
    # and suspect display labels are not stable-value or science assertions.

def copy_contract(p,cases):
    expected={k for k,v in cases.items() if v['copy_contract']};rows={v['type']:v for v in p['rows']}
    require(p['outcome']=='passed' and p['class_count']==len(rows)==len(p['rows'])==22 and set(rows)==expected and
            p['invalid_rejections']==44 and p['unsupported_derived_rejections']==2,'Copy class/guard matrix differs')
    for name,row in rows.items():
        spec=cases[name];root=str(Path(row['source_seed']).parent) if isinstance(row['source_seed'],str) and '@ROOT@' in spec['first'] else ''
        first=spec['first'].replace('@ROOT@',root);second=spec['second'].replace('@ROOT@',root)
        if spec['read_only']:first,second=('Mars','Luna') if spec['kind']=='string' else ('3','7')
        require(row['field']==spec['field'] and row['native_type']==spec['native_type'] and same(row['source_seed'],value(first,spec['kind'])) and
                same(row['destination_seed'],value(second,spec['kind'])) and same(row['copy_value'],row['source_seed']) and row['clone_independent'] and
                row['source_unchanged'] and row['destination_identity_preserved'] and row['self_unchanged'] and row['invalid_rejections']==2,'Copy literal/identity/independence differs')

def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for name,digest in bindings.items():require(Path(name).is_file() and not Path(name).is_symlink() and sha(name)==digest,'Changed regular input '+name)
    def bound(p):p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def capsule(p):
        with tarfile.open(bound(p)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source archive');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    frozen_visible=capsule(stage/'visible-executed-sources.tar.gz');before=load(bound(stage/'visible-inputs-before.json'))
    require(before==load(bound(stage/'visible-inputs-after.json')) and {p:hashlib.sha256(b).hexdigest() for p,b in frozen_visible.items()}==before['sources'],'Visible inputs changed')
    launch=load(bound(stage/'visible-launch.json'));maps=load(bound(stage/'loaded-images.json'))['processes']
    require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==54,'Visible selection/maps incomplete')
    catalog=json.loads(frozen_visible['src/qtgui/tests/NasaFunctionRemainingResourceCases.json']);cases={v['key']:v for v in catalog['cases']};require(len(cases)==45,'Candidate omitted')
    base=load(bound(repo/catalog['baseline_matrix']));require(sha(repo/catalog['baseline_matrix'])==catalog['baseline_matrix_sha256'],'Prior role matrix changed')
    expected_candidates={v['factory_type'] for v in base['rows'] if 'resource-candidate' in v['role'] and not v['nominal_category_base_global_default'] and not v['new_global_witnesses']}
    require(set(cases)==expected_candidates and len(base['rows'])==566 and not base['qualified_keys'],'Candidate universe narrowed or qualified')
    originals=load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-global-resources/visible-inputs-before.json'))['sources']
    p2={PREFIX+k for k in P2};f3={PREFIX+k for k in F3};all_resources={PREFIX+k for k in cases};all_final=all_resources|{META}|set(REG)
    versions=[('function-resource-universe-metadata-headless-v1','metadata-executed-sources-v1.tar.gz',{META:True}),
              ('function-resource-universe-pilot-headless-v2','pilot-executed-sources-v2.tar.gz',{PREFIX+'Spacecraft':True,PREFIX+'PointMassForce':False}),
              ('function-resource-universe-headless-v2','headless-executed-sources-v2.tar.gz',{n:n in p2 for n in all_resources-{PREFIX+'Spacecraft',PREFIX+'PointMassForce'}}),
              ('function-resource-universe-recovery-headless-v3','recovery-executed-sources-v3.tar.gz',{n:n not in f3 for n in all_resources-p2-{PREFIX+'Spacecraft'}}),
              ('function-resource-universe-recovery-headless-v4','recovery-executed-sources-v4.tar.gz',{n:True for n in f3}),
              ('function-resource-universe-regression-headless-v5','regression-executed-sources-v5.tar.gz',{n:True for n in {PREFIX+'Imager',PREFIX+'MessageWindow'}|set(REG)}),
              ('function-resource-universe-visible-v1',None,{n:True for n in all_final})]
    selected=[];failed=[];superseded=[];proofs={};exposures=0
    for folder,archive,expected in versions:
        root=repo/'build/gui-acceptance'/folder;r=load(bound(root/'results.json'));require(r.get('finished_utc') and r['end_reason']=='completed-selection','Nonterminal attempt')
        frozen=capsule(stage/archive) if archive else frozen_visible
        for name,digest in r['source_hashes'].items():require(name in frozen and hashlib.sha256(frozen[name]).hexdigest()==digest,'Executed source differs '+name)
        active={v['name']:v for v in r['results'] if v['status']!='not-run'};require(set(active)==set(expected),'Changed selection '+folder)
        for name,passed in expected.items():
            row=active[name];require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Native status differs');log=bound(row['log']);case=log.parent
            tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None and tests[0].find('error') is None)==passed,'Native JUnit differs')
            if not passed:failed.append(dict(invocation=folder,test=name));continue
            emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())]
            if name in REG:
                source='src/qtgui/tests/'+REG[name];require(r['source_hashes'][source]==originals[source],'Original regression assertions changed')
                if name=='QtGui.CustomFovControls':require('PASS' in log.read_text(),'FOV native completion missing')
                else:require(len(emitted)==1 and emitted[0]['outcome']=='passed','Regression native proof missing')
            else:
                require(len(emitted)==1,'Native proof missing');p=emitted[0];proofs[(folder,name)]=p
                if name==META:
                    metadata(p,cases);require(load(bound(case/'native-resource-metadata.json'))==p,'Metadata file differs')
                else:
                    spec=cases[name.removeprefix(PREFIX)];caller=bound(case/'caller.script').read_text();match=re.search(r"ReadShared\.FunctionPath = '([^']+)/ReadShared\.gmf';",caller);require(match is not None,'Function source root missing');directory=match[1]
                    resource(p,spec,directory);require(load(bound(case/'proof.json'))==p and load(bound(case/'phases.json'))==p['phases'],'Native phase/proof files differ')
                    original=bound(case/'initial.gmf').read_text();edited=bound(case/'edited.gmf').read_text();outer=bound(case/'nested.gmf').read_text();require(original.replace('Keep = 103;','Keep = 41;')==edited and 'Create '+spec['type']+' PrivateOnly;' in edited and 'Global Shared;' in edited and 'Global Shared;' in outer and '[Mark] = ReadShared();' in outer,'Private/global/file source differs')
                    require('function [Mark]' in edited and '[First] = ReadShared();' in caller,'Whole-resource output imposed')
                    for filename,contents in spec.get('files',{}).items():require(bound(case/('input-'+filename)).read_text()==contents.replace('@ROOT@',directory)==p['input_files'][filename],'Consumed fixture bytes differ')
                    for phase in p['phases']:
                        tag=str(phase['phase']);require([float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()]==phase['raw_values'],'Raw marker bytes differ');messages=bound(case/(tag+'-messages.txt')).read_text()
                        for object_name,key in [('Shared','shared_writes'),('PrivateOnly','private_writes'),('Shadow','shadow_writes')]:
                            words=re.findall(r'(?:^|\n)'+object_name+r'\.'+re.escape(spec['field'])+r' =\s*\n([^\n]*)',messages);observed=[value(v,spec['kind']) for v in words];require(same(observed,phase[key]),'Raw property Write differs')
            if archive is None:
                display=r['visible_display'];require(display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and r['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible policy differs')
                exposures+=row['trace']['native_exposure_events'];exes=[v for v in row['registered_command'] if v in r['binary_hashes']];require(len(exes)==1,'Executed image missing');traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Process trace missing');process=maps.get(traces[0].stem.removeprefix('trace-'));require(process and process['executable']==exes[0],'Process identity differs')
                for path,digest in process['images'].items():require(before['images'].get(path)==digest,'Mapped image changed')
                for path,digest in before['images'].items():
                    if path==exes[0] or Path(path).name.startswith(('libGmatBase','libGmatUtil','libGmatEstimation','libEKF','libStation','libThrustFile')):require(process['images'].get(path)==digest,'Core/provider unmapped')
            item=dict(invocation=folder,test=name);(superseded if folder=='function-resource-universe-recovery-headless-v3' and name in {PREFIX+'Imager',PREFIX+'MessageWindow'} else selected).append(item)
    require(len(selected)==108 and len(superseded)==2 and len(failed)==31,'GUI totals differ')
    for mode in ['headless','visible']:require({v['test'] for v in selected if mode in v['invocation']}==all_final and sum(mode in v['invocation'] for v in selected)==54,'Selected case/mode omitted')
    final_copy=None
    for version in [4,5]:
        root=stage/('copy-support-v'+str(version));launch=load(bound(root/'launch.json'));support_before=load(bound(root/'inputs-before.json'));require(support_before==load(bound(root/'inputs-after.json')) and launch['inputs_unchanged'] and launch['returncode']==(8 if version==4 else 0),'Copy launch/source status differs')
        frozen=capsule(stage/('recovery-executed-sources-v4.tar.gz' if version==4 else 'copy-support-executed-sources-v5.tar.gz'))
        for name,digest in support_before['sources'].items():require(name in frozen and hashlib.sha256(frozen[name]).hexdigest()==digest,'Copy executed source differs')
        tests=list(ET.parse(bound(root/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')=='QtAcceptance.FunctionResourceCopy' and (tests[0].find('failure') is None)==(version==5),'Copy JUnit differs')
        log=bound(root/'test.log').read_text()
        if version==4:require(len(load(bound(root/'copy-progress.json')))==14 and 'Polymorphic Copy value/identity/source differs' in log,'Original Imager copy failure lost')
        else:
            p=load(bound(root/'proof.json'));copy_contract(p,cases);require([json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log)]==[p],'Copy native proof differs');require('1.234567890123' in bound(root/'message-copy-output.txt').read_text(),'Copied message precision not used');final_copy=p
    matrix=load(bound(stage/'resource-scope-matrix-v1.json'));rows=copy.deepcopy(matrix['rows']);contexts={v['factory_type']:v.pop('remaining_resource_witnesses') for v in rows};require(rows==base['rows'] and not matrix['qualified_keys'] and sum(bool(v) for v in contexts.values())==45,'Complete role table changed/qualified')
    for name,items in contexts.items():
        if not items:continue
        require(len(items)==2 and {v['mode'] for v in items}=={'headless','visible'},'Resource context omitted')
        for v in items:require(dict(invocation=v['invocation'],test=PREFIX+name) in selected and load(bound(v['proof']))==proofs[(v['invocation'],PREFIX+name)] and sha(v['proof'])==v['proof_sha256'],'Context binding differs')
    negatives=[];sample=proofs[('function-resource-universe-visible-v1',PREFIX+'Spacecraft')]
    for label,change in [('wrong-global-pointer',lambda p:p['phases'][0]['identities'][0].__setitem__('global_store_object_pointer','bad')),('wrong-private-field',lambda p:p['phases'][0]['private_writes'].__setitem__(0,-99)),('missing-repeat',lambda p:p['phases'].pop()),('missing-nested-identity',lambda p:p['phases'][0]['identities'].pop())]:
        p=copy.deepcopy(sample);change(p)
        try:resource(p,cases['Spacecraft'])
        except (AssertionError,ValueError):negatives.append(label)
        else:raise ValueError('Accepted corrupt proof '+label)
    p=copy.deepcopy(final_copy);p['rows'][0]['copy_value']='wrong'
    try:copy_contract(p,cases)
    except (AssertionError,ValueError):negatives.append('wrong-copy-value')
    else:raise ValueError('Accepted corrupt Copy proof')
    return dict(status='passed-45-remaining-resource-mechanisms-and-22-copy-contracts',qualified_keys=[],selected=selected,superseded_passes=superseded,retained_GUI_failures=failed,retained_native_support_failures=1,resource_types=45,metadata_contexts=2,new_GUI_passes=92,affected_regression_GUI_passes=16,missions=270,function_calls=1080,property_Write_values=2160,raw_marker_scalars=1350,global_store_identity_checks=810,different_type_caller_shadow_missions=18,copy_classes=22,copy_invalid_rejections=44,copy_unsupported_derived_rejections=2,visible_mapped_processes=54,visible_exposure_events=exposures,negative_controls=negatives,production_changed_files=load(bound(stage/'production-paths-v4.json'))['changed_files'],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
