"""Audit native factory aliases and actual Assignment/ScriptEvent tree operations."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load
from AuditCommandNavigationOperations import compare,circle,OPERATIONS

PREFIX='QtGui.NasaCommandAliasOperations'
FAMILIES=['AssignmentAliases','ScriptEvent']
NAMES=[PREFIX+f+p for f in FAMILIES for p in ['Original','OuterIf']]
ALIASES={'AssignmentAliases':[dict(requested=n,native_type='GMAT') for n in ['GMAT','Assignment','Equation']], 'ScriptEvent':[dict(requested='BeginScript',native_type='BeginScript')]}
TEMPLATES={'AssignmentAliases':('Assignment','VariableName = 1;'),'ScriptEvent':('Script event','BeginScript;\n   % Insert commands here.\nEndScript;')}


def check(proof,family,position):
    require(proof['outcome']=='passed' and proof['family']==family and proof['position']==position,'Wrong operation context')
    require(proof['operations']==OPERATIONS and [v['phase'] for v in proof['results']]==[0,1],'Missing operation/reopen')
    require(proof['original_parent']=='Root' and proof['selected_parent']==('If' if position=='OuterIf' else 'Root'),'Wrong parent')
    require(proof['factory_aliases']==ALIASES[family] and (proof['template_name'],proof['template_source'])==TEMPLATES[family],'Wrong native alias/template')
    for row in proof['results']:
        truth=[20]+circle(20)+[41,3,1,0,0]
        compare(row['raw'],truth,[1e-4]+[2e-5]*3+[2e-8]*3+[1e-12]*5)
        compare(row['expected'],truth,[1e-10]*12)
        compare(row['summary_expected'],circle(20),[1e-10]*6)
        require(row['effects']==dict(phase_observations=[]) and row['side_effects']==[],'Unexpected effects')


def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for name,h in manifest.items():require(not Path(name).is_symlink() and sha(name)==h,'Changed regular artifact '+name)
    def bound(path):
        p=Path(path);require(str(p) in manifest,'Unbound artifact '+str(p));return p
    before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')),'Changed native inputs')
    launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==0 and launch['inputs_unchanged'] and launch['mapped_processes']==4,'Wrong visible launch result')
    with tarfile.open(bound(stage/'executed-sources-v1.tar.gz')) as tar:frozen={m.name:tar.extractfile(m).read() for m in tar.getmembers()}
    require({n:hashlib.sha256(v).hexdigest() for n,v in frozen.items()}==before['sources'],'Executed source archive differs')
    factory=frozen['src/base/factory/CommandFactory.cpp'].decode()
    require('ofType == "GMAT" || ofType == "Equation" || ofType == "Assignment"' in factory and 'return new Assignment;' in factory,'Alias implementation differs')
    driver=frozen['src/qtgui/tests/NasaCommandAliasOperationTests.cpp'].decode()
    for assertion in ['require(!menuOnly,','Script event interior leaked into tree','Event selection lost its closing boundary','Alias template emitted wrong syntax or committed draft']:
        require(assertion in driver,'Missing alias/event assertion')
    mapped=load(bound(stage/'loaded-images.json'))['processes'];selected=[];last=None
    census=load(bound(stage/'scope-continuation.json'));require(census['factory_census_count']==len(census['factory_types'])==53 and census['qualified_keys']==[],'Narrowed parent census')
    require(sha(bound(census['prior_worklist']))==census['prior_worklist_sha256'],'Prior census changed')
    for mode in ['headless','visible']:
        receipt=load(bound(repo/'build/gui-acceptance'/('command-aliases-'+mode+'-v1')/'results.json'))
        require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal run')
        require(all(before['sources'].get(n)==h for n,h in receipt['source_hashes'].items()),'Wrong executed source version')
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};require(set(active)==set(NAMES),'Wrong attempted census')
        if mode=='visible':
            display=receipt['visible_display'];require(receipt['max_screenshots']==0 and display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'],'Wrong window policy')
        for name,row in active.items():
            require(row['status']=='passed' and row['returncode']==0,'Failed case')
            log=bound(row['log']);root=log.parent;junit=ET.parse(bound(root/'ctest.xml')).getroot().findall('testcase')
            require(len(junit)==1 and junit[0].attrib['name']==name and all(junit[0].find(n) is None for n in ['skipped','failure','error']),'Wrong JUnit outcome')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1 and before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Wrong test image')
            if mode=='visible':
                require(row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Missing native exposure')
                traces=list(root.glob('trace-*.jsonl'));require(traces,'Missing visible trace')
                for trace in traces:
                    proc=mapped.get(trace.stem.removeprefix('trace-'));require(proc and proc['executable']==exes[0],'Missing process mapping')
                    for p,h in proc['images'].items():require(before['images'].get(p)==h,'Mapped image differs')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith('libGmat'):require(proc['images'].get(p)==h,'Missing mapped core/test')
            proof=load(bound(root/'proof.json'));messages=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(messages)==1 and json.loads(messages[0])==proof,'Proof differs from stdout')
            position=next(p for p in ['OuterIf','Original'] if name.endswith(p));family=name.removeprefix(PREFIX).removesuffix(position);check(proof,family,position);last=(proof,family,position)
            fixture='src/qtgui/tests/command-operation-aliases/'+family+'.json';spec=json.loads(frozen[fixture]);origin=frozen[spec['operation_fixture_origin']]
            require(hashlib.sha256(origin).hexdigest()==spec['operation_fixture_origin_sha256'],'Fixture origin changed')
            expected=json.loads(origin)['script']
            if family=='ScriptEvent':expected=expected.replace('BeginMissionSequence;\n',"BeginMissionSequence;\nBeginScript 'EventProbe';\n").replace("Report 'Witness'","EndScript;\nReport 'Witness'")
            require(spec['script']==expected and proof['fixture_sha256']==hashlib.sha256(frozen[fixture]).hexdigest()==sha(bound(root/'fixture.json')),'Executed fixture differs')
            require(load(bound(root/'factory-commands.json'))==census['factory_types'],'Native factory census differs')
            initial=bound(root/'initial.script').read_text();require(('If Keep == 41; % operation branch begin' in initial)==(position=='OuterIf'),'Wrong outer guard')
            for phase in range(2):
                raw=[float(v) for v in bound(root/f'{phase}-states.csv').read_text().strip().split(',')];require(raw==proof['results'][phase]['raw'],'Raw state differs')
                require(load(bound(root/f'{phase}-phase-observations.json'))==[],'Unexpected raw phases')
                summary=bound(root/f'{phase}-summary.txt').read_text();require('Spacecraft       : Sat' in summary and 'Coordinate System: EarthMJ2000Eq' in summary,'Missing spacecraft/frame')
                for i,label in enumerate(['X','Y','Z','VX','VY','VZ']):
                    values=re.findall(r'(?:^|\s)'+label+r'\s*=\s*([-+0-9.eE]+)',summary);require(len(values)==1 and abs(float(values[0])-circle(20)[i])<=2e-5,'Independent summary differs')
            for side in ['before','after']:
                changed=bound(root/(side+'-inserted.script')).read_text();insertion="'OperationProbe' Keep = Keep + 0;";require(changed.count(insertion)==1 and initial in [changed.replace(insertion+'\n',''),changed.replace('\n'+insertion,'')],'Insertion source corruption')
            deleted=bound(root/'deleted.script').read_text()
            if family=='ScriptEvent':require('BeginScript' not in deleted and 'EndScript' not in deleted and 'AssignmentProbe' not in deleted and "Report 'Witness'" in deleted,'Event Delete lost scope')
            else:require('AssignmentProbe' not in deleted and deleted.count('Propagate')==initial.count('Propagate'),'Assignment Delete lost scope')
            selected.append(dict(mode=mode,name=name,log=str(log)))
    negatives=[]
    for label,mutate in [('wrong-state',lambda p:p['results'][0]['raw'].__setitem__(1,0)),('missing-close',lambda p:p['operations'].remove('close-tree')),('missing-reopen',lambda p:p['results'].pop()),('wrong-parent',lambda p:p.update(selected_parent='Missing')),('wrong-alias',lambda p:p['factory_aliases'][0].update(native_type='NoOp')),('wrong-template',lambda p:p.update(template_source='Stop;'))]:
        bad=copy.deepcopy(last[0]);mutate(bad)
        try:check(bad,last[1],last[2])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    return dict(status='passed-bounded-alias-and-script-event-operations',qualified_keys=[],new_native_gui_passes=8,raw_operation_rows=16,summary_components=96,negative_controls=negatives,selected=selected,remaining_parent='All Others!E196',technical_core_approval=False,scientific_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();v=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(v,indent=2)+'\n');print(json.dumps({k:x for k,x in v.items() if k!='selected'}))
