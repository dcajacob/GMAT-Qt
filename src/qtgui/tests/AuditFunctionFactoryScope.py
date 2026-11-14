"""Audit the full native census and additional function resource Copy evidence."""
import argparse,copy,difflib,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load

CENSUS='QtGui.NasaFunctionResourceCensus';PLATE='QtGui.NasaFunctionFactoryPlate';FOGM='QtGui.NasaFunctionFactoryFirstOrderGaussMarkov';REG='QtGui.NasaFunctionResourceExtendedEstimatedParameter'


def census_check(proof):
    require(proof['outcome']=='passed' and proof['configured_source_history_unchanged'],'Census changed configuration')
    require(proof['category_count']==len(proof['categories'])==36,'Missing native category')
    flat=[name for row in proof['categories'] for name in row['factory_types']]
    require(len(flat)==proof['factory_entry_count']==566 and len(set(flat))==proof['unique_factory_type_count']==566,'Incomplete native factory inventory')
    require(sorted(flat)==sorted(proof['all_factory_items'])==proof['unique_factory_types'],'Factory map/list differ')
    require(set(proof['interpreter_first_category_by_name'])==set(flat) and not proof['duplicate_name_categories'],'Interpreter name-resolution map differs')
    for row in proof['categories']:
        require([v['factory_type'] for v in row['entries']]==row['factory_types'],'Missing categorized entry')
        for v in row['entries']:require(v['viewable']==(v['factory_type'] in row['viewable_types']) and v['explicit_unviewable']==(v['factory_type'] in row['unviewable_types']),'Visibility flags differ')


def resource_check(proof):
    require(proof['outcome']=='passed' and proof['missions']==2 and proof['function_calls']==4,'Missing local resource executions')
    values={'Plate':('Area',2.5,7.25),'FirstOrderGaussMarkov':('HalfLife',300,600),'EstimatedParameter':('HalfLife',300,600)}
    field,first,second=values[proof['type']];require(proof['field']==field and proof['file_decisions']==[{'saved':False},{'saved':True}],'Wrong field or file Save/Cancel')
    require([v['phase'] for v in proof['runs']]==['initial','reopened'],'Missing Unicode reopen')
    for row in proof['runs']:
        require(row['first']==row['second']==first and row['caller_shadow']==second,'Wrong output/caller values')
        require(row['first_name']=='First' and row['second_name']=='Second' and not row['private_resource_leaked'] and row['configured_prototypes_unchanged'],'Local ownership/identity escaped')


def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for name,h in manifest.items():require(Path(name).is_file() and not Path(name).is_symlink() and sha(name)==h,'Changed regular artifact '+name)
    def bound(p):
        p=Path(p);require(str(p) in manifest,'Unbound artifact '+str(p));return p
    def archive(p):
        with tarfile.open(bound(p)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source capsule');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    frozen=archive(stage/'visible-executed-sources.tar.gz');before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')) and {p:hashlib.sha256(b).hexdigest() for p,b in frozen.items()}==before['sources'],'Visible inputs changed')
    versions={('census',1):archive(stage/'headless-executed-sources-v1.tar.gz')};versions.update({('factory',v):archive(stage/f'factory-executed-sources-v{v}.tar.gz') for v in [1,2,3]});maps=load(bound(stage/'loaded-images.json'))['processes'];launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==0 and launch['mapped_processes']==4 and launch['inputs_unchanged'],'Visible invocation incomplete')
    selected=[];failed=[];catalog=None;last_resource=None;source_limited=[]
    for mode,kind,version,expected in [('headless','census',1,{CENSUS}),('headless','factory',1,{PLATE,FOGM}),('headless','factory',2,{REG}),('headless','factory',3,{FOGM}),('visible','combined',1,{CENSUS,PLATE,FOGM,REG})]:
        folder=repo/'build/gui-acceptance'/f'function-{("resource-census" if kind=="census" else "factory" if kind=="factory" else "factory-scope")}-{mode}-v{version}';receipt=load(bound(folder/'results.json'));require(receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal invocation');source=frozen if mode=='visible' else versions[kind,version]
        for p,h in receipt['source_hashes'].items():require(p in source and hashlib.sha256(source[p]).hexdigest()==h,'Executed source identity differs '+p)
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};require(set(active)==expected,'Wrong actual selected census')
        if mode=='visible':
            display=receipt['visible_display'];require(display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots']==0,'Wrong visible window policy')
        for name,row in active.items():
            passed=not(mode=='headless' and kind=='factory' and version==1 and name==FOGM);require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Pass/failure changed');root=Path(row['log']).parent;log=bound(row['log']).read_text();tests=list(ET.parse(bound(root/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None)==passed,'JUnit differs')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1,'Missing executed image identity')
            if mode=='visible':
                require(before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]] and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Case not exposed/bound')
                for trace in root.glob('trace-*.jsonl'):
                    process=maps.get(trace.stem.removeprefix('trace-'));require(process and process['executable']==exes[0],'Missing native process map')
                    for p,h in process['images'].items():require(before['images'].get(p)==h,'Mapped image differs')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith(('libGmatBase','libGmatUtil','libEKF')):require(process['images'].get(p)==h,'Required core/provider image unmapped')
            if not passed:require('Cannot copy objects of type FirstOrderGaussMarkov' in log,'Original Copy failure lost');failed.append(name);continue
            proofs=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log)];require(len(proofs)==1,'Native proof missing');proof=proofs[0]
            if name==CENSUS:
                census_check(proof);require(load(bound(root/'native-factory-census.json'))==proof,'Census/process output differs');require(catalog is None or catalog==proof,'Mode-specific factory census differs');catalog=proof
            else:
                resource_check(proof);last_resource=proof;require(load(bound(root/'proof.json'))==proof and load(bound(root/'native-results.json'))==proof['runs'],'Resource/process/raw proof differs')
                for phase in ['initial','reopened']:require([float(v) for v in bound(root/(phase+'-values.csv')).read_text().split()]==[41,42],'Independent continuation differs')
            if name==REG:source_limited.append(dict(mode=mode,name=name,version=version,reason='fixture JSON absent from original source hash list;replaced by focused fully bound reruns'))
            else:selected.append(dict(mode=mode,name=name,version=version))
    regression_before=load(bound(stage/'regression-inputs-before.json'));require(regression_before==load(bound(stage/'regression-inputs-after.json')),'Regression inputs changed');regression_frozen=archive(stage/'regression-visible-sources.tar.gz');require({p:hashlib.sha256(b).hexdigest() for p,b in regression_frozen.items()}==regression_before['sources'],'Regression source capsule differs');regression_maps=load(bound(stage/'regression-loaded-images.json'))['processes'];regression_launch=load(bound(stage/'regression-visible-launch.json'));require(regression_launch['returncode']==0 and regression_launch['mapped_processes']==1,'Focused visible regression incomplete')
    for mode,version,source in [('headless',4,archive(stage/'factory-executed-sources-v4.tar.gz')),('visible',2,regression_frozen)]:
        folder=repo/'build/gui-acceptance'/('function-factory-headless-v4' if mode=='headless' else 'function-factory-scope-visible-v2');receipt=load(bound(folder/'results.json'));require(receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Regression invocation nonterminal');active=[v for v in receipt['results'] if v['status']!='not-run'];require(len(active)==1 and active[0]['name']==REG and active[0]['status']=='passed' and active[0]['returncode']==0,'Focused regression not passed')
        for p,h in receipt['source_hashes'].items():require(p in source and hashlib.sha256(source[p]).hexdigest()==h,'Regression source differs')
        fixture='src/qtgui/tests/NasaFunctionExtendedResourceCases.json';require(fixture in receipt['source_hashes'],'Regression fixture still unbound');row=active[0];root=Path(row['log']).parent;log=bound(row['log']).read_text();emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log)];require(len(emitted)==1,'Focused regression native proof absent');resource_check(emitted[0]);require(load(bound(root/'proof.json'))==emitted[0] and load(bound(root/'native-results.json'))==emitted[0]['runs'],'Focused regression proof/raw differs');tests=list(ET.parse(bound(root/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==REG and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Focused regression JUnit failed')
        for phase in ['initial','reopened']:require([float(v) for v in bound(root/(phase+'-values.csv')).read_text().split()]==[41,42],'Focused regression continuation differs')
        if mode=='visible':
            display=receipt['visible_display'];require(display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Focused regression visibility differs')
            exe=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exe)==1 and regression_before['images'].get(exe[0])==receipt['binary_hashes'][exe[0]],'Focused regression image differs')
            for trace in root.glob('trace-*.jsonl'):
                process=regression_maps.get(trace.stem.removeprefix('trace-'));require(process and process['executable']==exe[0],'Focused regression process map absent')
                for p,h in process['images'].items():require(regression_before['images'].get(p)==h,'Focused regression mapped image differs')
                for p,h in regression_before['images'].items():
                    if p==exe[0] or Path(p).name.startswith(('libGmatBase','libGmatUtil','libEKF')):require(process['images'].get(p)==h,'Focused regression core/provider unmapped')
        selected.append(dict(mode=mode,name=REG,version=version))
    require(len(selected)==8 and len(source_limited)==2 and failed==[FOGM],'Wrong final execution totals')
    original=versions['factory',1]['src/qtgui/tests/NasaFunctionResourceTests.cpp'];require(original==frozen['src/qtgui/tests/NasaFunctionResourceTests.cpp'],'Original driver/assertions changed')
    fixture='src/qtgui/tests/NasaFunctionAdditionalFactoryCases.json';old_cases=json.loads(versions['factory',1][fixture]);new_cases=json.loads(frozen[fixture]);normalized=copy.deepcopy(new_cases)
    require(len(old_cases['cases'])==len(new_cases['cases'])==2,'Additional factory case set changed')
    for prior,current in zip(old_cases['cases'],normalized['cases']):
        if current['key']=='FirstOrderGaussMarkov':
            require(prior['native_source_sha256']==sha(bound(stage/'FirstOrderGaussMarkov.cpp.before')) and current['native_source_sha256']==hashlib.sha256(frozen[current['native_source']]).hexdigest(),'FOGM source transition not tied to exact model bytes');current['native_source_sha256']=prior['native_source_sha256']
        require(prior==current,'Factory fixture assertion or reference changed')
    require(old_cases==normalized,'Factory fixture scope changed')
    prefix='plugins/ExtendedKalmanFilterPlugin/src/base/estimatedparam/FirstOrderGaussMarkov';old=bound(stage/'FirstOrderGaussMarkov.cpp.before').read_text();new=frozen[prefix+'.cpp'].decode();delta=list(difflib.ndiff(old.splitlines(),new.splitlines()));require(not any(v.startswith('- ') for v in delta),'An original model line changed');added='\n'.join(v[2:] for v in delta if v.startswith('+ ')).strip();expected='''// Whole-object assignment and function outputs dispatch through this API.
// Reuse the class assignment operator, preserving the destination identity.
void FirstOrderGaussMarkov::Copy(const GmatBase* source)
{
   const auto* typed = dynamic_cast<const FirstOrderGaussMarkov*>(source);
   if (!typed || typed->GetTypeName() != GetTypeName())
      throw GmatBaseException("Cannot copy into " + GetTypeName() +
                             " from a null or incompatible object");
   operator=(*typed);
}''';require(added==expected,'Unexpected native Copy increment');header=bound(stage/'FirstOrderGaussMarkov.hpp.before').read_text();require(frozen[prefix+'.hpp'].decode()==header.replace('   virtual GmatBase*    Clone(void) const;','   virtual GmatBase*    Clone(void) const;\n   virtual void         Copy(const GmatBase* source);'),'Unexpected header/layout change')
    unit=list(ET.parse(bound(stage/'copy-contract-junit-v2.xml')).getroot().iter('testcase'));require(len(unit)==1 and unit[0].get('name')=='QtAcceptance.FirstOrderGaussMarkovCopy' and unit[0].find('failure') is None and unit[0].find('error') is None,'Copy API support test not run/passed');require('392 exact covariance-component comparisons' in bound(stage/'copy-contract-v2.log').read_text(),'Copy response/guard contract missing')
    matrix=load(bound(stage/'factory-scope-matrix-v1.json'));require(len(matrix['rows'])==566 and {v['factory_type'] for v in matrix['rows']}==set(catalog['unique_factory_types']) and all(not v['qualified'] for v in matrix['rows']),'Planning matrix drops entries or invents credit')
    negatives=[]
    for label,change in [('missing-type',lambda p:p['categories'][0]['factory_types'].pop()),('wrong-output',lambda p:p['runs'][0].update(first=17)),('leaked-private-object',lambda p:p['runs'][0].update(private_resource_leaked=True))]:
        bad=copy.deepcopy(catalog if label=='missing-type' else last_resource);change(bad)
        try:(census_check if label=='missing-type' else resource_check)(bad)
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt scope proof '+label)
    return dict(status='passed-full-native-census-and-bounded-factory-resource-copy-repair',qualified_keys=[],factory_types=566,factory_categories=36,selected=selected,superseded_source_limited_passes=source_limited,retained_native_failures=failed,support_covariance_copy_comparisons=392,invalid_Copy_sources_rejected=2,original_driver_unchanged=True,negative_controls=negatives,production_changed_files=[prefix+'.cpp',prefix+'.hpp'],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
