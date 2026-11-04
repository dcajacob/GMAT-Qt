"""Audit function-local navigation, sandbox context repair and retained failures."""
import argparse,copy,csv,datetime,difflib,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load
from AuditCommandNavigationOperations import compare
from AuditScriptEventBackendAcceptance import backend_reference,navigation_report,oracle,check_values

NAME='QtGui.NasaFunctionNavigationScope'
NEW=[NAME,NAME+'NestedBranch',NAME+'Expression']
REG=['QtGui.NasaFunctionControlMutual','QtGui.NativeNasaFunctionPropagatorGlobalExisting','QtGui.NasaScriptEventBackendNavigation']
CORE='src/base/executive/Sandbox.cpp'


def returned_reference():
    result=[]
    expected,_=backend_reference('Navigation')
    for seconds in [20,600,600]:
        w=math.sqrt(398600.4415/7000**3);v=math.sqrt(398600.4415/7000);angle=w*seconds
        result += [21545+20/86400 if seconds==20 else expected[0][1],7000*math.cos(angle),7000*math.sin(angle),0,-v*math.sin(angle),v*math.cos(angle),0,41]
    return result+[103]


def check(proof):
    require(proof['outcome']=='passed' and proof['native_missions']==2 and len(proof['phases'])==2,'Missing function mission/repeat')
    require(proof['file_decisions']==[{'saved':False},{'saved':True}],'Missing actual file Save/Cancel')
    expected,bounds=backend_reference('Navigation');reference=returned_reference()
    for i,phase in enumerate(proof['phases']):
        require(phase['phase']==i and phase['rebuild']==(i==0) and phase['observations']==11,'Wrong phase/observation/no-Build contract')
        require(len(phase['navigation'])==3,'Missing navigation stage')
        for row,target in zip(phase['navigation'],expected):compare(row,target,bounds)
        compare(phase['returned'],reference,([2e-10]+[2e-5]*3+[2e-8]*3+[0])*3+[0])
        require(set(phase['caller_prototypes'])=={'Sat','SimSat','EstSat'},'Missing caller shadow')
        for name,text in phase['caller_prototypes'].items():
            require(name+'.X = 8100;' in text and name+'.NAIFId = '+str(-110100-['Sat','SimSat','EstSat'].index(name))+';' in text,'Caller shadow/fixture ID differs')
    require(proof['phases'][0]['caller_prototypes']==proof['phases'][1]['caller_prototypes'],'Caller prototypes changed across runs')


def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for name,digest in manifest.items():
        p=Path(name);require(p.is_file() and not p.is_symlink() and sha(p)==digest,'Changed regular input '+name)
    def bound(p):
        p=Path(p);require(str(p) in manifest,'Unbound input '+str(p));return p
    def archive(p):
        with tarfile.open(bound(p)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source capsule')
            return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    versions={v:archive(stage/f'executed-sources-v{v}.tar.gz') for v in range(2,8)}
    frozen=archive(stage/'visible-executed-sources-v7.tar.gz');before=load(bound(stage/'visible-inputs-before.json'))
    for name in ['BuildNasaCoverageExtension.py','AuditCommandNavigationOperations.py','AuditScriptEventBackendAcceptance.py']:
        path='src/qtgui/tests/'+name;require(sha(repo/path)==hashlib.sha256(frozen[path]).hexdigest(),'Changed auditor dependency '+path)
    require(before==load(bound(stage/'visible-inputs-after.json')) and {p:hashlib.sha256(b).hexdigest() for p,b in frozen.items()}==before['sources'],'Visible source/input identity differs')
    maps=load(bound(stage/'loaded-images.json'))['processes'];launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==6,'Visible launch incomplete')
    old=bound(stage/'Sandbox.before.cpp').read_text();addition='''         // Function commands use this sandbox's event-manager clones just as
         // main-sequence commands do. Refresh existing FCS bindings on reuse;
         // branch commands forward the managers to their children.
         for (GmatCommand *functionCmd = udf->GetFunctionControlSequence();
              functionCmd; functionCmd = functionCmd->GetNext())
            functionCmd->SetTriggerManagers(&triggerManagers);
''';needle='''               fcsCmd = fcsCmd->GetNext();
            }
         }
''';require(old.count(needle)==1,'Ambiguous core preimage');new=old.replace(needle,needle+addition);require(frozen[CORE].decode()==new and (repo/CORE).read_text()==new,'Unexpected Sandbox increment')
    patch=''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='before/'+CORE,tofile='after/'+CORE));require(bound(stage/'core-v6.patch').read_text()==patch,'Incremental patch differs')
    selected=[];failures=[];last_new_proof=None;metrics=dict(native_function_missions=0,GPS_observation_rows=0,filter_smoother_component_rows=0,raw_report_scalars=0)
    runs=[('headless',v) for v in range(2,8)]+[('visible',1)]
    for mode,version in runs:
        folder=repo/'build/gui-acceptance'/f'function-navigation-{mode}-v{version}';receipt=load(bound(folder/'results.json'));require(receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal native invocation')
        source=frozen if mode=='visible' else versions[version]
        for p,h in receipt['source_hashes'].items():require(p in source and hashlib.sha256(source[p]).hexdigest()==h,'Wrong executed source '+p)
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};expected=set(NEW+REG) if mode=='visible' else {NAME} if version<7 else set(NEW[1:]+REG);require(set(active)==expected,'Wrong selected native census')
        if mode=='visible':
            display=receipt['visible_display'];require(display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots']==0,'Wrong visible window policy')
        for name,row in active.items():
            passed=mode=='visible' or version>=6;require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Pass/failure changed')
            log=bound(row['log']);root=log.parent;tests=list(ET.parse(bound(root/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None)==passed,'JUnit differs')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1,'Missing executable identity')
            if mode=='visible':
                require(before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]] and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Missing mapped/exposed case')
                for trace in root.glob('trace-*.jsonl'):
                    process=maps.get(trace.stem.removeprefix('trace-'));require(process and process['executable']==exes[0],'Missing visible process map')
                    for p,h in process['images'].items():require(before['images'].get(p)==h,'Mapped image differs')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith(('libGmat','libVF13ad')):require(process['images'].get(p)==h,'Core/provider/test image unmapped')
            if not passed:
                diagnostic={2:'Function navigation file action unavailable',3:'Function navigation Unicode reopen changed caller',4:'Function navigation Unicode reopen changed caller',5:'The Event Manager pointer was not set on the RunSimulator command'}[version]
                require(diagnostic in log.read_text() and not (root/'proof.json').exists(),'Original failure lost');failures.append(dict(version=version,diagnostic=diagnostic));continue
            emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())];require(len(emitted)==1 and emitted[0]['outcome']=='passed','Missing native process proof');proof=emitted[0]
            if name in REG:selected.append(dict(mode=mode,name=name,version=version));continue
            require(load(bound(root/'proof.json'))==proof,'Proof/process output differs');check(proof);last_new_proof=proof
            require(bound(root/'initial.gmf').read_text().replace('Original function navigation source','Saved function navigation source')==bound(root/'edited.gmf').read_text(),'Function edit changed executable source')
            require(load(bound(root/'caller-prototypes-before.json'))==load(bound(root/'caller-prototypes-reopened.json'))==proof['phases'][0]['caller_prototypes'],'Full caller identity changed')
            caller=bound(root/'caller.script').read_text();function=bound(root/'edited.gmf').read_text();family=proof.get('family','Direct');require(family==('Direct' if name==NAME else name.removeprefix(NAME)),'Wrong function family')
            if family=='NestedBranch':require('[Result] = OuterNavigation();' in caller and '[out] = LocalNavigation();' in bound(root/'outer.gmf').read_text() and 'If Keep == 41;' in function,'Nested/branch context missing')
            elif family=='Expression':require('Result = LocalNavigation();' in caller and '[Result] = LocalNavigation();' not in caller,'Expression context missing')
            else:require('[Result] = LocalNavigation();' in caller,'Direct context missing')
            spec=load(bound(root/'fixture.json'));require(hashlib.sha256(source['src/qtgui/tests/script-event-backends/Navigation.json']).hexdigest()==sha(root/'fixture.json'),'Fixture identity differs')
            for phase in proof['phases']:
                prefix=str(phase['phase'])+'-';read_csv=lambda n:[[float(x) for x in row] for row in csv.reader(bound(root/(prefix+n)).open())]
                require(read_csv('local-extra.csv')==phase['navigation'] and read_csv('Caller results Ω.csv')==[phase['returned']],'Raw navigation/return differs')
                values=read_csv('local-values.csv');require(len(values)==1,'Missing local continuation');check_values(values[0],oracle(spec));metrics['raw_report_scalars']+=len(values[0])+len(phase['returned'])+sum(len(v) for v in phase['navigation'])
                for filename,kind in [('local-filter.txt','filter.txt'),('local-smoother.txt','smoother.txt')]:
                    records=navigation_report(bound(root/(prefix+filename)).read_text(),kind);require(len(records)==11,'Missing residual records');metrics['filter_smoother_component_rows']+=sum(len(v['observed']) for v in records)
                records=[v.split() for v in bound(root/(prefix+'local-navigation.gmd')).read_text().splitlines() if v.strip() and not v.startswith('%')];require(len(records)==11,'Missing observation rows')
                utc=datetime.datetime(2010,6,10,tzinfo=datetime.timezone.utc);tai=10587.5+utc.timestamp()/86400+34/86400
                for i,v in enumerate(records):
                    require(len(v)==7 and v[1]=='GPS_PosVec' and v[3]=='800' and abs(float(v[0])-(tai+i*60/86400))<2e-11,'GPS identity/epoch differs');require(abs(math.sqrt(sum(float(x)**2 for x in v[-3:]))-7000)<2e-5,'GPS circle norm differs')
                metrics['GPS_observation_rows']+=len(records);metrics['native_function_missions']+=1
            selected.append(dict(mode=mode,name=name,version=version))
    negatives=[]
    for label,change in [('wrong-returned-state',lambda p:p['phases'][0]['returned'].__setitem__(1,8000)),('wrong-local-sentinel',lambda p:p['phases'][0]['returned'].__setitem__(7,103)),('lost-repeat',lambda p:p['phases'].pop()),('lost-observation',lambda p:p['phases'][0].update(observations=10)),('wrong-navigation-state',lambda p:p['phases'][0]['navigation'][1].__setitem__(2,8100))]:
        bad=copy.deepcopy(last_new_proof);change(bad)
        try:check(bad)
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted damaged proof '+label)
    require(metrics==dict(native_function_missions=12,GPS_observation_rows=132,filter_smoother_component_rows=792,raw_report_scalars=732),'Wrong full audit counts')
    return dict(status='passed-bounded-function-local-navigation-and-sandbox-context-repair',qualified_keys=[],selected=selected,failures=failures,metrics=metrics,negative_controls=negatives,production_changed_files=[CORE],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
