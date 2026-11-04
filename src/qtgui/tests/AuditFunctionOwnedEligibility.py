"""Verify complete named main probes; retain all failed operations without exclusions."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha

PREFIX='QtGui.NasaFunctionOwnedNamed'
USABLE={'CoordinateSystemFixed','Spinner','PrecessingSpinner','NadirPointing','CCSDS-AEM'}

def observed(p,spec):
    category=spec['category_id'];name=spec['type'];stored=category in {153,154} and name!='SpiceAttitude';usable=name in USABLE
    require(p['checkpoint']=='owned-expression-main-named-probe' and p['outcome']=='passed' and p['qualified'] is False and
            p['case']==spec['key'] and p['factory_type']==name and p['category_id']==category,'Probe identity/qualification differs')
    constructor=p['factory_constructor'];require(constructor['present'] and constructor['native_type'] and constructor['name']==('' if name=='SpiceAttitude' else 'FactoryOnly'),'Constructor observation differs')
    for row in [constructor,p['create']['configured'],p['write_build']['configured']]:
        if row['present']:require(len(row['fields'])==row['parameter_count'] and not row['local'],'Metadata row/count differs')
    require(p['create']['built'] and p['create']['configured']['present']==stored and p['create']['source_history_unchanged'] and
            p['write_build']['built']==stored and p['write_build']['configured']['present']==stored and p['write_build']['source_history_unchanged'],'Creation/lookup/Write Build differs')
    if stored:
        for phase in ['create','write_build']:
            row=p[phase]['configured'];require(row['name']=='NamedWitness' and row['native_type']==constructor['native_type'] and not row['global'] and not row['automatic_global'],'Named native identity/flags differ')
        run=p['write_run'];require(run['completed']==usable and run['run_result']==(0 if usable else 2) and run['runtime']['present']==(category==153) and run['source_history_unchanged'],'Write runtime observation differs')
        require((float(run['marker_bytes'].strip())==42) if usable else not run['marker_bytes'].strip(),'Write continuation marker differs')
        if category==153:
            require(run['runtime']['name']=='NamedWitness' and run['runtime']['native_type']==constructor['native_type'],'Runtime type/name differs')
            if not usable:require('Reference coordinate system EarthMJ2000Eq not defined' in run['runtime']['serialization_error'],'Uninitialized attitude serialization error lost')
        else:require('Could not find "NamedWitness"' in run['runtime']['lookup_error'],'Failed-run lookup error lost')
    else:
        require(p['write_run']==dict(attempted=False,reason='Write mission did not Build') and any('Nonexistent object "NamedWitness"' in e for e in p['write_build']['errors']),'Unstored-name error lost')
    require(p['recovery_build']['built'] and not p['recovery_build']['configured']['present'] and p['recovery_build']['source_history_unchanged'] and
            p['recovery_run']['completed'] and p['recovery_run']['run_result']==0 and not p['recovery_run']['runtime']['present'] and
            float(p['recovery_run']['marker_bytes'].strip())==42 and p['recovery_run']['source_history_unchanged'],'Recovery/absence/independent marker differs')

def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for name,digest in bindings.items():require(Path(name).is_file() and not Path(name).is_symlink() and sha(name)==digest,'Changed regular input '+name)
    def bound(p):p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def capsule(p):
        with tarfile.open(bound(p)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source capsule');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    frozen_visible=capsule(stage/'visible-executed-sources-v2.tar.gz');before=load(bound(stage/'visible-inputs-before-v2.json'))
    require(before==load(bound(stage/'visible-inputs-after-v2.json')) and {p:hashlib.sha256(b).hexdigest() for p,b in frozen_visible.items()}==before['sources'],'Visible inputs changed')
    catalog=json.loads(frozen_visible['src/qtgui/tests/NasaFunctionOwnedNamedCases.json']);cases={v['key']:v for v in catalog['cases']};base=load(bound(repo/catalog['baseline_matrix']))
    require(sha(repo/catalog['baseline_matrix'])==catalog['baseline_matrix_sha256'] and len(base['rows'])==566 and not base['qualified_keys'] and len(cases)==88 and
            {v['type'] for v in cases.values()}=={v['factory_type'] for v in base['rows'] if v['category_id'] in {110,152,153,154}},'Full candidate universe changed')
    launch=load(bound(stage/'visible-launch-v2.json'));maps=load(bound(stage/'loaded-images-v2.json'))['processes'];require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==len(maps)==88,'Visible launch/maps incomplete')
    versions=[('function-owned-eligibility-pilot-headless-v1','pilot-executed-sources-v1.tar.gz',{'RungeKutta89','MJ2000Eq','Spinner','Add'},False),
              ('function-owned-eligibility-headless-v2','headless-executed-sources-v2.tar.gz',set(cases),True),
              ('function-owned-eligibility-visible-v2',None,set(cases),True)]
    selected=[];failed=[];proofs={};exposures=0
    for folder,archive,keys,passed in versions:
        root=repo/'build/gui-acceptance'/folder;receipt=load(bound(root/'results.json'));require(receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal native attempt')
        frozen=capsule(stage/archive) if archive else frozen_visible
        for path,digest in receipt['source_hashes'].items():require(path in frozen and hashlib.sha256(frozen[path]).hexdigest()==digest,'Executed source differs '+path)
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};require(set(active)=={PREFIX+k for k in keys},'Native selection differs')
        for name,row in active.items():
            require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Native CTest status differs')
            log=bound(row['log']);case=log.parent;tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'))
            require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None and tests[0].find('error') is None)==passed,'Native JUnit differs')
            item=dict(invocation=folder,test=name)
            if not passed:
                require('Sandbox::GetInternalObject(NamedWitness' in log.read_text() and bound(case/'progress.json').is_file(),'Initial unhandled absence diagnostic lost');failed.append(item);continue
            emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())];require(len(emitted)==1,'Native proof missing');p=emitted[0];spec=cases[name.removeprefix(PREFIX)];observed(p,spec)
            require(load(bound(case/'proof.json'))==p,'Native proof file differs');proofs[(folder,spec['key'])]=p
            for phase in ['create','write_build','recovery_build']:
                source=bound(case/(phase+'.script')).read_text();require('Keep = 41;' in source and 'Keep = Keep + 1;' in source,'Probe/control source marker differs')
                if phase!='recovery_build':require('Create '+spec['type']+' NamedWitness;' in source,'Named Create type differs')
                else:require('NamedWitness' not in source,'Recovery retained named declaration')
                require(('Write NamedWitness;' in source)==(phase=='write_build'),'Write probe source differs')
                messages=bound(case/(phase+'.messages.txt')).read_text()
                if phase=='write_build' and not p[phase]['built']:require('Nonexistent object "NamedWitness"' in messages,'Raw failed-Write Build error lost')
            require(bound(case/'create.script').read_text().split('BeginMissionSequence;')[0] in bound(case/'write_build.script').read_text(),'Create and use declarations differ')
            for phase in ['write_run','recovery_run']:
                run=p[phase]
                if run.get('attempted') is False:continue
                require(bound(case/(phase+'.values.txt')).read_text()==run['marker_bytes'],'Raw report marker differs');messages=bound(case/(phase+'.messages.txt')).read_text()
                if run['completed']:
                    require('Mission run completed.' in messages,'Raw completion absent')
                    if phase=='write_run':require('NamedWitness.' in messages,'Actual named object Write missing')
                else:
                    require('Mission run failed.' in messages,'Raw failed mission absent')
                    expected='TableTemplate error : unsized table.' if spec['category_id']==154 else 'Reference coordinate system EarthMJ2000Eq not defined for attitude of type "'+spec['type']+'"'
                    require(expected in messages,'Observed failure diagnostic differs')
            if archive is None:
                display=receipt['visible_display'];require(display['start_minimized'] and display['resizable'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Visible policy differs')
                exposures+=row['trace']['native_exposure_events'];traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Native process trace missing');process=maps.get(traces[0].stem.removeprefix('trace-'));executables=[v for v in row['registered_command'] if v in receipt['binary_hashes']];require(len(executables)==1 and process and process['executable']==executables[0],'Mapped process identity differs')
                for path,digest in process['images'].items():require(before['images'].get(path)==digest,'Mapped image changed')
                for path,digest in before['images'].items():
                    if path==executables[0] or Path(path).name.startswith(('libGmatBase','libGmatUtil')):require(process['images'].get(path)==digest,'Core image unmapped')
            selected.append(item)
    require(len(selected)==176 and len(failed)==4,'Probe CTest totals differ')
    logical=['create','write_build','write_run','recovery_build','recovery_run']
    for key in cases:
        a=proofs[('function-owned-eligibility-headless-v2',key)];b=proofs[('function-owned-eligibility-visible-v2',key)]
        for phase in logical:
            for field in ['built','completed','run_result','attempted','source_history_unchanged']:require(a[phase].get(field)==b[phase].get(field),'Mode outcome differs')
    matrix=load(bound(stage/'named-main-matrix-v1.json'));rows=copy.deepcopy(matrix['rows']);contexts={v['factory_type']:v.pop('named_main_observations') for v in rows}
    require(rows==base['rows'] and not matrix['qualified_keys'] and sum(bool(v) for v in contexts.values())==88,'Complete 566-row role table changed/qualified')
    for row in cases.values():
        items=contexts[row['type']];require(len(items)==2 and {v['mode'] for v in items}=={'headless','visible'},'Named context omitted')
        for v in items:require(dict(invocation=v['invocation'],test=PREFIX+row['key']) in selected and load(bound(v['proof']))==proofs[(v['invocation'],row['key'])] and sha(v['proof'])==v['proof_sha256'],'Named context binding differs')
    negatives=[];sample=proofs[('function-owned-eligibility-visible-v2','Spinner')]
    for label,change in [('false-qualified',lambda p:p.__setitem__('qualified',True)),('missing-configured-name',lambda p:p['create']['configured'].__setitem__('name','Wrong')),('wrong-continuation',lambda p:p['write_run'].__setitem__('marker_bytes','99')),('failed-recovery',lambda p:p['recovery_run'].__setitem__('completed',False))]:
        p=copy.deepcopy(sample);change(p)
        try:observed(p,cases['Spinner'])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt probe '+label)
    p=copy.deepcopy(proofs[('function-owned-eligibility-visible-v2','Add')]);p['write_run']['completed']=True
    try:observed(p,cases['Add'])
    except (ValueError,AssertionError):negatives.append('failed-operation-promoted')
    else:raise ValueError('Accepted failed math operation as usable')
    return dict(status='passed-complete-88-named-main-observations;eligibility-unresolved',qualified_keys=[],selected=selected,retained_native_CTest_failures=failed,factory_rows=566,probed_types=88,constructor_observations=176,Create_only_Builds=176,named_configured_observations=102,Write_mission_Builds=102,completed_named_Write_missions=10,retained_failed_Write_Builds=74,retained_failed_named_missions=92,completed_independent_recovery_missions=176,visible_mapped_processes=88,visible_exposure_events=exposures,negative_controls=negatives,production_changed_files=[],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
