"""Audit all remaining factory dispositions, preserving positive-operation failures."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load
from AuditCommandNavigationOperations import compare,circle,OPERATIONS
PREP='QtGui.NasaCommandPrepareRestrictionsOriginal';HIDDEN='QtGui.NasaCommandHiddenDispositions'
BEFORE='QtGui.NasaCommandPrepareOperationsOriginal';DELETE='QtGui.NasaCommandPrepareProtectedOperationsOriginal'
REASON='GMAT currently requires that all objects are created before the mission sequence begins'


def prepared(proof):
    require(proof['outcome']=='passed' and proof['family']=='PrepareMissionSequence' and proof['position']=='Original' and proof['original_parent']==proof['selected_parent']=='Root','Wrong preparation context')
    operations=[('insert-before-native-rejected' if n=='insert-before' else 'delete-cancel-native-rejected' if n=='delete-cancel-confirm' else n) for n in OPERATIONS]
    require(proof['operations']==operations and [r['phase'] for r in proof['results']]==[0,1],'Missing restriction or reopen')
    for row in proof['results']:
        require(row['prepared']['epoch']==21545 and row['prepared']['Count']==0 and row['prepared']['report_exists'] is False,'Preparation executed body')
        compare(row['prepared']['cartesian'],circle(0),[1e-10]*6)
        compare(row['normal'],[20]+circle(20)+[41,21,1,0,0],[1e-4]+[2e-5]*3+[2e-8]*3+[1e-12]*5)


def hidden(proof):
    require(proof['outcome']=='passed' and proof['factory_results']==[dict(requested=n,created=n!='CallFunction') for n in ['NoOp','EndScript','Create','CallFunction']],'Wrong factory constructibility')
    require([r['family'] for r in proof['cases']]==['NoOp','EndScript','Create'],'Wrong hidden census')
    for row in proof['cases']:
        f=row['family']
        if f=='Create':require(row==dict(family=f,built=False,reason=REASON),'Wrong definition restriction');continue
        require(row['built'] and row['graph_count']==row['native_types'].count(f)==(2 if f=='NoOp' else 1) and row['tree_count']==row['actual_tree_count']==0 and row['expanded_count']==(0 if f=='NoOp' else 1),'Hidden graph/tree mismatch')
        compare(row['raw'],[20]+circle(20)+[41,21 if f=='NoOp' else 3,1,0,0],[1e-4]+[2e-5]*3+[2e-8]*3+[1e-12]*5)


def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,h in manifest.items():require(not Path(n).is_symlink() and sha(n)==h,'Changed regular artifact '+n)
    def bound(p):
        p=Path(p);require(str(p) in manifest,'Unbound artifact '+str(p));return p
    def archive(p):
        with tarfile.open(bound(p)) as t:return {m.name:t.extractfile(m).read() for m in t.getmembers()}
    before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')),'Changed visible inputs')
    frozen=archive(stage/'executed-sources-v5.tar.gz');require({n:hashlib.sha256(v).hexdigest() for n,v in frozen.items()}==before['sources'],'Executed sources differ')
    originals=archive(stage/'source-v1.tar.gz');require(originals['src/qtgui/tests/NasaCommandPrepareOperationTests.cpp']==frozen['src/qtgui/tests/NasaCommandPrepareOperationTests.cpp'],'Original full-operation assertions changed')
    old=originals['src/qtgui/tests/NasaCommandHiddenDispositionTests.cpp'].decode();require(old.replace('const bool built=test.window.buildScript();acceptanceView','const bool built=test.window.buildScript();test.events();acceptanceView')==frozen['src/qtgui/tests/NasaCommandHiddenDispositionTests.cpp'].decode(),'Hidden assertions changed beyond event delivery')
    require('Execute only the PrepareMissionSequence command' in frozen['src/base/executive/Moderator.cpp'].decode(),'Native preparation dispatch absent')
    sources=load(bound(stage/'scope-continuation.json'));require(sources['factory_census_count']==len(sources['factory_types'])==53 and sources['qualified_keys']==[] and sha(bound(sources['prior_worklist']))==sources['prior_worklist_sha256'],'Narrowed original census')
    for f in ['PrepareMissionSequence','NoOp','EndScript','Create']:
        spec=json.loads(frozen['src/qtgui/tests/command-operation-dispositions/'+f+'.json']);origin=frozen[spec['operation_fixture_origin']]
        require(hashlib.sha256(origin).hexdigest()==spec['operation_fixture_origin_sha256'] and spec['script']==json.loads(origin)['script'],'Changed retained fixture derivation')
    mapped=load(bound(stage/'loaded-images.json'))['processes'];launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==1 and launch['mapped_processes']==4 and launch['inputs_unchanged'],'Wrong visible outcomes')
    runs=[('headless',1,{BEFORE:'Insertion absent/duplicated',HIDDEN:'Mission-local Create rejection differs'}),('headless',2,{HIDDEN:None,DELETE:'Native pre-starter insertion rejection differs'}),('headless',3,{DELETE:'Delete changed wrong source interval'}),('headless',4,{PREP:'Preparation summary incorrectly contains executed state'}),('headless',5,{PREP:None}),('visible',1,{HIDDEN:None,PREP:None,BEFORE:'Insertion absent/duplicated',DELETE:'Delete changed wrong source interval'})]
    passes=[];failures=[];last={}
    for mode,v,names in runs:
        receipt=load(bound(repo/'build/gui-acceptance'/f'command-dispositions-{mode}-v{v}'/'results.json'));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal selection')
        version=dict(before['sources'])
        if mode=='headless':version.update({n:hashlib.sha256(b).hexdigest() for n,b in archive(stage/f'source-v{v}.tar.gz').items()})
        require(all(version.get(n)==h for n,h in receipt['source_hashes'].items()),'Wrong executed source version')
        active={r['name']:r for r in receipt['results'] if r['status']!='not-run'};require(set(active)==set(names),'Wrong attempted census')
        if mode=='visible':
            d=receipt['visible_display'];require(receipt['max_screenshots']==0 and d['resizable'] and d['start_minimized'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'],'Wrong desktop policy')
        for name,row in active.items():
            error=names[name];passed=error is None;require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Changed native outcome')
            log=bound(row['log']);root=log.parent;tests=ET.parse(bound(root/'ctest.xml')).getroot().findall('testcase');require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None)==passed,'Changed JUnit outcome')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1,'No unique native image')
            if passed or mode=='visible':require(before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Wrong qualified native image')
            if mode=='visible':
                require(row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'No native exposure');traces=list(root.glob('trace-*.jsonl'));require(traces,'No trace')
                for trace in traces:
                    proc=mapped.get(trace.stem.removeprefix('trace-'));require(proc and proc['executable']==exes[0],'Missing mapped process')
                    for p,h in proc['images'].items():require(before['images'].get(p)==h,'Changed mapped image')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith('libGmat'):require(proc['images'].get(p)==h,'Missing core/test mapping')
            if not passed:
                require('FAIL '+error in log.read_text() and not (root/'proof.json').exists(),'Lost retained failure')
                if name==BEFORE:require('Cannot find LHS object named' in bound(root/'runtime/output/GmatLog.txt').read_text(),'Lost before parser restriction')
                if name==DELETE and error=='Delete changed wrong source interval':require('is not allowed before BeginMissionSequence' in bound(root/'runtime/output/GmatLog.txt').read_text(),'Lost Delete parser restriction')
                failures.append(dict(mode=mode,version=v,name=name,error=error,log=str(log)));continue
            proof=load(bound(root/'proof.json'));matches=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(matches)==1 and json.loads(matches[0])==proof,'Proof/stdout mismatch')
            require(load(bound(root/'factory-commands.json'))==sources['factory_types'],'Factory census changed')
            if name==PREP:
                prepared(proof);initial=bound(root/'initial.script').read_text();require(bound(root/'deleted.script').read_text()==initial,'Rejected Delete changed source')
                require(bound(root/'before-rejected.script').read_text()=="'OperationProbe' Keep = Keep + 0;" and bound(root/'before-rejection.txt').read_text()=='The mission rejected these changes. See Message Window.','Wrong rejected draft/status')
                require('Cannot find LHS object named' in bound(root/'before-rejection-messages.txt').read_text() and 'is not allowed before BeginMissionSequence' in bound(root/'delete-rejection-messages.txt').read_text(),'Wrong native diagnostics')
                fixture='src/qtgui/tests/command-operation-dispositions/PrepareMissionSequence.json';require(proof['fixture_sha256']==sha(bound(root/'fixture.json'))==hashlib.sha256(frozen[fixture]).hexdigest(),'Wrong preparation fixture')
                for phase in range(2):
                    require(load(bound(root/f'{phase}-prepared.json'))==proof['results'][phase]['prepared'],'Prepared raw/proof mismatch')
                    raw=[float(v) for v in bound(root/f'{phase}-states.csv').read_text().strip().split(',')];require(raw==proof['results'][phase]['normal'],'Normal raw/proof mismatch')
                    require(bound(root/f'{phase}-normal.script').read_text()==initial.replace('PrepareMissionSequence;','BeginMissionSequence;'),'Transition changed other source')
                    summary=bound(root/f'{phase}-summary.txt').read_text();require('Spacecraft       : Sat' in summary and 'Coordinate System: EarthMJ2000Eq' in summary,'Missing preparation frame')
                    for i,label in enumerate(['X','Y','Z','VX','VY','VZ']):
                        values=re.findall(r'(?:^|\s)'+label+r'\s*=\s*([-+0-9.eE]+)',summary);require(len(values)==1 and abs(float(values[0])-circle(0)[i])<=2e-5,'Initial summary differs')
            else:
                hidden(proof)
                for f in ['NoOp','EndScript','Create']:
                    fixture='src/qtgui/tests/command-operation-dispositions/'+f+'.json';require(sha(bound(root/(f+'-fixture.json')))==hashlib.sha256(frozen[fixture]).hexdigest(),'Wrong hidden fixture')
                    if f=='Create':require(REASON in bound(root/'Create-messages.txt').read_text(),'Missing Create error');continue
                    row=next(v for v in proof['cases'] if v['family']==f);require(load(bound(root/(f+'-native-types.json')))==row['native_types'],'Graph raw/proof mismatch');raw=[float(v) for v in bound(root/(f+'-states.csv')).read_text().strip().split(',')];require(raw==row['raw'],'Hidden raw/proof mismatch')
            last[name]=proof;passes.append(dict(mode=mode,version=v,name=name,log=str(log)))
    negatives=[]
    for name,label,mutate in [(PREP,'body-executed',lambda p:p['results'][0]['prepared'].update(Count=21)),(PREP,'wrong-transition-state',lambda p:p['results'][0]['normal'].__setitem__(1,0)),(PREP,'missing-reopen',lambda p:p['results'].pop()),(HIDDEN,'visible-NoOp',lambda p:p['cases'][0].update(actual_tree_count=1)),(HIDDEN,'constructible-CallFunction',lambda p:p['factory_results'][-1].update(created=True)),(HIDDEN,'accepted-Create',lambda p:p['cases'][-1].update(built=True))]:
        bad=copy.deepcopy(last[name]);mutate(bad)
        try:(prepared if name==PREP else hidden)(bad)
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    return dict(status='passed-bounded-native-dispositions-with-retained-preparation-conflicts',qualified_keys=[],new_native_gui_passes=4,retained_failures=len(failures),normal_numerical_rows=8,prepared_state_rows=4,initial_summary_components=24,negative_controls=negatives,selected=passes,failures=failures,reviewed_factory_entries=53,full_six_operation_entries=41,technical_core_approval=False,scientific_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();v=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(v,indent=2)+'\n');print(json.dumps({k:x for k,x in v.items() if k not in ['selected','failures']}))
