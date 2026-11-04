"""Audit structural operation witnesses without waiving disabled operations or failed summaries."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load
from AuditCommandNavigationOperations import compare,circle
PREFIX='QtGui.NasaCommandStructuralOperations'
FAMILIES=['EndFor','EndIf','EndWhile','EndTarget','EndOptimize','Else']
SUPPORTED=[PREFIX+f+p for f in FAMILIES for p in ['Original','OuterIf']]
FAILED=PREFIX+'BeginMissionSequenceOriginal'
PARENTS=dict(EndFor='For',EndIf='If',EndWhile='While',EndTarget='Target',EndOptimize='Optimize',Else='If',BeginMissionSequence='Root')
TAILS=dict(EndFor=[3,1,0,0],EndIf=[7,1,0,0],EndWhile=[3,1,0,0],EndTarget=[0,4,0,9],EndOptimize=[0,2,0,0],Else=[7,1,0,0],BeginMissionSequence=[0,1,0,0])
OPERATIONS=['context-protection','doubleclick-protection','close-no-owner','insert-before','insert-after','summary-context','unicode-reopen']
MENU={'Edit command…':False,'missionCloseCommand':True,'Insert before…':True,'Insert after…':True,'Delete command':False,'missionCommandSummary':True}


def numeric(raw,family):
    compare(raw,[20]+circle(20)+[41]+TAILS[family],[1e-4]+[2e-5]*3+[2e-8]*3+[2e-5 if family in ['EndTarget','EndOptimize'] else 1e-12]*5)


def check(proof,family,position):
    require(proof['outcome']=='passed' and proof['family']==family and proof['position']==position,'Wrong structural context')
    require(proof['operations']==OPERATIONS and proof['menu_states']==MENU,'Changed structural operation policy')
    require(proof['original_parent']==PARENTS[family],'Wrong original parent')
    expected=[dict(side='before',parent_type=PARENTS[family],parent_label='Probe'),dict(side='after',parent_type=('If' if position=='OuterIf' else 'Root') if family!='Else' else 'If',parent_label='' if family!='Else' else 'Probe')]
    require(proof['insertions']==expected,'Wrong insertion parent or order')
    require([v['phase'] for v in proof['results']]==[0,1],'Missing Unicode reopen')
    for row in proof['results']:
        numeric(row['raw'],family);compare(row['expected'],[20]+circle(20)+[41]+TAILS[family],[1e-10]*12);compare(row['summary_expected'],circle(20),[1e-10]*6)


def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for name,h in manifest.items():require(not Path(name).is_symlink() and sha(name)==h,'Changed regular artifact '+name)
    def bound(path):
        p=Path(path);require(str(p) in manifest,'Unbound artifact '+str(p));return p
    before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')),'Changed native inputs')
    launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==1 and launch['inputs_unchanged'] and launch['mapped_processes']==13,'Lost failed-summary launch outcome')
    with tarfile.open(bound(stage/'executed-sources-v1.tar.gz')) as tar:frozen={m.name:tar.extractfile(m).read() for m in tar.getmembers()}
    require({n:hashlib.sha256(v).hexdigest() for n,v in frozen.items()}==before['sources'],'Executed source archive differs')
    driver=frozen['src/qtgui/tests/NasaCommandStructuralOperationTests.cpp'].decode()
    for text in ['Structural replace/remove protection absent','Structural double click opened editor or changed source','Structural summary lacks spacecraft/frame','Structural insertion crossed parent or order']:
        require(text in driver,'Lost native assertion')
    require('includeInSummary = false;' in frozen['src/base/command/BeginMissionSequence.cpp'].decode(),'Native starter suppression changed')
    require('No summary data for ' in frozen['src/base/command/GmatCommand.cpp'].decode(),'Summary suppression branch absent')
    disposition=load(bound(stage/'starter-summary-conflict-v1.json'));require(disposition['qualified_keys']==[] and disposition['test_assertions_changed'] is False and disposition['production_changed_files']==[],'Conflict was waived')
    require(sha(bound(disposition['c_api_assertion_source']))==disposition['c_api_assertion_sha256'],'C-API assertion identity changed')
    require('CHECK(total==23 && strstr(summary,"No summary data for BeginMissionSequence command ")!=NULL);' in bound(disposition['c_api_assertion_source']).read_text(),'Prior C-API suppression assertion absent')
    census=load(bound(stage/'scope-continuation.json'));require(census['factory_census_count']==len(census['factory_types'])==53 and census['qualified_keys']==[],'Narrowed full census')
    require(sha(bound(census['prior_worklist']))==census['prior_worklist_sha256'],'Prior census changed')
    mapped=load(bound(stage/'loaded-images.json'))['processes'];selected=[];failures=[];last=None
    for mode in ['headless','visible']:
        receipt=load(bound(repo/'build/gui-acceptance'/('command-structural-'+mode+'-v1')/'results.json'));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal run')
        require(all(before['sources'].get(n)==h for n,h in receipt['source_hashes'].items()),'Wrong source version')
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};require(set(active)==set(SUPPORTED+[FAILED]),'Wrong attempted census')
        if mode=='visible':
            display=receipt['visible_display'];require(receipt['max_screenshots']==0 and display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'],'Wrong window policy')
        for name,row in active.items():
            passed=name!=FAILED;require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Lost outcome classification')
            log=bound(row['log']);root=log.parent;junit=ET.parse(bound(root/'ctest.xml')).getroot().findall('testcase')
            require(len(junit)==1 and junit[0].attrib['name']==name and junit[0].find('skipped') is None and junit[0].find('error') is None and (junit[0].find('failure') is None)==passed,'Wrong JUnit outcome')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1 and before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Wrong test image')
            if mode=='visible':
                require(row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'No native exposure')
                traces=list(root.glob('trace-*.jsonl'));require(traces,'Missing visible trace')
                for trace in traces:
                    proc=mapped.get(trace.stem.removeprefix('trace-'));require(proc and proc['executable']==exes[0],'Missing process mapping')
                    for p,h in proc['images'].items():require(before['images'].get(p)==h,'Mapped image differs')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith('libGmat'):require(proc['images'].get(p)==h,'Missing mapped core/test')
            require(load(bound(root/'factory-commands.json'))==census['factory_types'],'Factory census changed')
            if not passed:
                require('FAIL Structural summary lacks spacecraft/frame' in log.read_text() and not (root/'proof.json').exists(),'Original summary failure lost')
                require(bound(root/'0-summary.txt').read_text()=='No summary data for BeginMissionSequence command BeginMissionSequence1\n','Unexpected starter summary')
                raw=[float(v) for v in bound(root/'0-states.csv').read_text().strip().split(',')];numeric(raw,'BeginMissionSequence');require(not (root/'1-states.csv').exists(),'Failure incorrectly completed reopen')
                failures.append(dict(mode=mode,name=name,log=str(log)));continue
            proof=load(bound(root/'proof.json'));messages=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(messages)==1 and json.loads(messages[0])==proof,'Proof differs from stdout')
            position=next(p for p in ['OuterIf','Original'] if name.endswith(p));family=name.removeprefix(PREFIX).removesuffix(position);check(proof,family,position);last=(proof,family,position)
            fixture='src/qtgui/tests/command-operation-structural/'+family+'.json';spec=json.loads(frozen[fixture]);origin=frozen[spec['operation_fixture_origin']];require(hashlib.sha256(origin).hexdigest()==spec['operation_fixture_origin_sha256'],'Origin changed')
            expected=json.loads(origin)['script']
            if family=='Else':expected=expected.replace("If 'Probe' 1 > 2;","If 'Probe' 1 < 2;")
            require(spec['script']==expected and proof['fixture_sha256']==hashlib.sha256(frozen[fixture]).hexdigest()==sha(bound(root/'fixture.json')),'Native fixture differs')
            initial=bound(root/'initial.script').read_text();require(('If Keep == 41; % outer structural guard begin' in initial)==(position=='OuterIf'),'Wrong outer guard')
            for phase in range(2):
                raw=[float(v) for v in bound(root/f'{phase}-states.csv').read_text().strip().split(',')];require(raw==proof['results'][phase]['raw'],'Raw state differs')
                summary=bound(root/f'{phase}-summary.txt').read_text();require('Spacecraft       : Sat' in summary and 'Coordinate System: EarthMJ2000Eq' in summary,'Missing spacecraft/frame')
                for i,label in enumerate(['X','Y','Z','VX','VY','VZ']):
                    values=re.findall(r'(?:^|\s)'+label+r'\s*=\s*([-+0-9.eE]+)',summary);require(len(values)==1 and abs(float(values[0])-circle(20)[i])<=2e-5,'Independent summary differs')
            for side in ['before','after']:
                changed=bound(root/(side+'-inserted.script')).read_text();insertion="'StructuralProbe' Keep = Keep + 0;";require(changed.count(insertion)==1 and initial in [changed.replace(insertion+'\n',''),changed.replace('\n'+insertion,'')],'Insertion changed original bytes')
            selected.append(dict(mode=mode,name=name,log=str(log)))
    negatives=[]
    for label,mutate in [('wrong-state',lambda p:p['results'][0]['raw'].__setitem__(1,0)),('enabled-delete',lambda p:p['menu_states'].update({'Delete command':True})),('missing-reopen',lambda p:p['results'].pop()),('wrong-insert-parent',lambda p:p['insertions'][1].update(parent_type='Missing')),('missing-protection',lambda p:p['operations'].remove('context-protection'))]:
        bad=copy.deepcopy(last[0]);mutate(bad)
        try:check(bad,last[1],last[2])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    return dict(status='passed-bounded-structural-operations-with-retained-starter-summary-conflict',qualified_keys=[],new_native_gui_passes=24,retained_native_failures=2,raw_passing_operation_rows=48,summary_components=288,additional_pre_failure_rows=2,negative_controls=negatives,selected=selected,failures=failures,remaining_parent='All Others!E196',technical_core_approval=False,scientific_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();v=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(v,indent=2)+'\n');print(json.dumps({k:x for k,x in v.items() if k not in ['selected','failures']}))
