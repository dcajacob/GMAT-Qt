"""Audit supported control/dynamics/solver operations and retain rejected placements."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load
from AuditCommandNavigationOperations import compare,circle,OPERATIONS

FAMILIES=['Propagate','Maneuver','BeginFiniteBurn','EndFiniteBurn','For','If','IfElse','While','Target','Achieve','VaryTarget','Optimize','VaryOptimize','Minimize','NonlinearConstraint','Assignment','Stop']
CHILDREN=['Achieve','VaryTarget','VaryOptimize','Minimize','NonlinearConstraint']
PREFIX='QtGui.NasaCommandControlOperations'
INITIAL=[PREFIX+f+p for f in FAMILIES for p in ['Original','OuterIf']]
REJECTED=[PREFIX+f+'OuterIf' for f in CHILDREN]
NEW=[PREFIX+f+'SolverOuterIf' for f in CHILDREN]
STOP=[PREFIX+'Stop'+p for p in ['Original','OuterIf']]
REG='QtGui.NasaScriptEventExtraStop'
SUPPORTED=[n for n in INITIAL if n not in REJECTED]+NEW
TAILS={f:[0,1,0,0] for f in FAMILIES}
TAILS.update(For=[3,1,0,0],If=[7,1,0,0],IfElse=[-7,1,0,0],While=[3,1,0,0],Assignment=[3,1,0,0])
for f in ['Target','Achieve','VaryTarget']:TAILS[f]=[0,4,0,9]
for f in ['Optimize','VaryOptimize','Minimize']:TAILS[f]=[0,2,0,0]
TAILS['NonlinearConstraint']=[0,3,1,0]
SOLVERS=['Target','Achieve','VaryTarget','Optimize','VaryOptimize','Minimize','NonlinearConstraint']


def parent(family):
    return 'Target' if family in ['Achieve','VaryTarget'] else 'Optimize' if family in ['VaryOptimize','Minimize','NonlinearConstraint'] else 'Root'


def state(family,seconds=20):
    v=circle(seconds)
    if family=='Maneuver':v[3]+=.01
    return v


def numeric(raw,family):
    compare(raw,[20]+state(family)+[41]+TAILS[family],[1e-4]+[2e-5]*3+[2e-8]*3+[2e-5 if family in SOLVERS else 1e-12]*5)


def check(proof,family,position):
    require(proof['outcome']=='passed' and proof['family']==family and proof['position']==position,'Wrong operation context')
    require(proof['operations']==OPERATIONS and [v['phase'] for v in proof['results']]==[0,1],'Missing operation/reopen')
    require(proof['original_parent']==parent(family) and proof['selected_parent']==('If' if position=='OuterIf' else parent(family)),'Wrong native parent')
    require((position=='SolverOuterIf')==(family in CHILDREN and position!='Original'),'Wrong solver placement')
    for row in proof['results']:
        numeric(row['raw'],family)
        compare(row['expected'],[20]+state(family)+[41]+TAILS[family],[1e-10]*12)
        phases=row['effects']['phase_observations']
        if family in ['BeginFiniteBurn','EndFiniteBurn']:
            require(phases==[dict(before_command='BeginFiniteBurn',burn_firing=False),dict(before_command='EndFiniteBurn',burn_firing=True),dict(before_command='Report',burn_firing=False)] and row['effects']['burn_active_after_pair'] is False,'Burn pair runtime transitions differ')
        else:require(phases==[],'Unexpected runtime phases')
        if family=='Stop':require(row['effects']['following_report_absent'] is True,'Stop ran the following report')


def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for name,h in manifest.items():require(not Path(name).is_symlink() and sha(name)==h,'Changed regular artifact '+name)
    def bound(path):
        p=Path(path);require(str(p) in manifest,'Unbound artifact '+str(p));return p
    def archive(path):
        with tarfile.open(bound(path)) as tar:return {m.name:tar.extractfile(m).read() for m in tar.getmembers()}
    before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')),'Changed native inputs')
    launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==1 and launch['inputs_unchanged'] and launch['mapped_processes']==40,'Lost rejected-placement launch result')
    frozen=archive(stage/'executed-sources-v2.tar.gz');original=archive(stage/'source-v1.tar.gz')
    require({n:hashlib.sha256(v).hexdigest() for n,v in frozen.items()}==before['sources'],'Executed source archive differs')
    old=bound(stage/'Stop.before.cpp').read_text();needle='bool Stop::Execute()\n{\n   throw CommandException('
    require(original['src/base/command/Stop.cpp'].decode()==old and old.count(needle)==1 and old.replace(needle,'bool Stop::Execute()\n{\n   BuildCommandSummary(true);\n   throw CommandException(')==frozen['src/base/command/Stop.cpp'].decode(),'Unexpected Stop delta')
    for n in ['src/qtgui/tests/NasaCommandControlOperationTests.cpp','src/qtgui/tests/NasaCommandLifecycleTests.cpp','src/qtgui/tests/NasaScriptEventCommandTests.cpp']:
        require(original[n]==frozen[n],'Original native assertions changed')
    old_images=load(bound(stage/'before-core-images.json'));require(any(Path(n).name.startswith('libGmatBase.') and old_images.get(n)!=h for n,h in before['images'].items()),'Stop runtime was not rebuilt')
    disposition=load(bound(stage/'solver-placement-disposition.json'))
    require(disposition['families']==CHILDREN and disposition['qualified_keys']==[] and hashlib.sha256(frozen[disposition['source']]).hexdigest()==disposition['source_sha256'],'Solver placement source differs')
    require('cannot be nested inside' in frozen[disposition['source']].decode(),'Native placement restriction absent')
    mapped=load(bound(stage/'loaded-images.json'))['processes'];selected=[];rejected=[];last=None
    runs=[('headless','command-control-headless-v1',[n for n in INITIAL if n not in REJECTED+STOP],INITIAL),('headless','command-control-headless-v2',NEW+STOP+[REG],NEW+STOP+[REG]),('visible','command-control-visible-v1',SUPPORTED+[REG],INITIAL+NEW+[REG])]
    for mode,run,names,all_names in runs:
        receipt=load(bound(repo/'build/gui-acceptance'/run/'results.json'));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal run')
        version=dict(before['sources'])
        if run=='command-control-headless-v1':version.update({n:hashlib.sha256(v).hexdigest() for n,v in original.items()})
        require(all(version.get(n)==h for n,h in receipt['source_hashes'].items()),'Wrong executed source version')
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};require(set(active)==set(all_names),'Wrong attempted census')
        if mode=='visible':
            display=receipt['visible_display'];require(receipt['max_screenshots']==0 and display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'],'Wrong window policy')
        for n,row in active.items():
            passed=n in names;require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Outcome classification changed')
            log=bound(row['log']);root=log.parent;junit=ET.parse(bound(root/'ctest.xml')).getroot().findall('testcase')
            require(len(junit)==1 and junit[0].attrib['name']==n and junit[0].find('skipped') is None and (junit[0].find('failure') is None)==passed,'Wrong JUnit outcome')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1 and before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Wrong test image')
            if mode=='visible':
                require(row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Missing native exposure')
                traces=list(root.glob('trace-*.jsonl'));require(traces,'Missing visible trace')
                for trace in traces:
                    proc=mapped.get(trace.stem.removeprefix('trace-'));require(proc and proc['executable']==exes[0],'Missing process mapping')
                    for p,h in proc['images'].items():require(before['images'].get(p)==h,'Mapped image differs')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith('libGmat'):require(proc['images'].get(p)==h,'Missing mapped core/test')
            if not passed:
                if n in REJECTED:
                    require('FAIL Nested operation Build failed' in log.read_text() and 'Solver command, and cannot be nested inside a "If" Control Logic command' in bound(root/'runtime/output/GmatLog.txt').read_text(),'Native placement rejection differs')
                    rejected.append(dict(mode=mode,name=n,log=str(log)))
                else:require(n in STOP and 'FAIL Context summary lacks selected spacecraft/frame' in log.read_text(),'Lost original Stop summary failure')
                require(not (root/'proof.json').exists(),'Failure was replaced by a pass proof');continue
            proof=load(bound(root/'proof.json'));messages=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(messages)==1 and json.loads(messages[0])==proof,'Proof differs from stdout')
            if n==REG:
                require(proof['outcome']=='passed' and proof['family']=='Stop' and proof['native_outcome']=='Stopped','Stop regression no longer interrupts');numeric(proof['observed'],'Stop')
            else:
                position=next(p for p in ['SolverOuterIf','OuterIf','Original'] if n.endswith(p));family=n.removeprefix(PREFIX).removesuffix(position);check(proof,family,position);last=(proof,family,position)
                fixture='src/qtgui/tests/command-operation-control/'+family+'.json';spec=json.loads(frozen[fixture]);origin=frozen[spec['operation_fixture_origin']];require(hashlib.sha256(origin).hexdigest()==spec['operation_fixture_origin_sha256'],'Retained fixture origin changed')
                expected_script=json.loads(origin)['script']
                if family=='Assignment':expected_script=expected_script.replace("Report 'Witness'","'AssignmentProbe' Count = 2 * U + 1;\nReport 'Witness'")
                require(spec['script']==expected_script and proof['fixture_sha256']==hashlib.sha256(frozen[fixture]).hexdigest()==sha(bound(root/'fixture.json')),'Executed mission fixture differs')
                require(('If Keep == 41; % operation branch begin' in bound(root/'initial.script').read_text())==(position!='Original'),'Wrong outer guard')
                for phase in range(2):
                    raw=[float(v) for v in bound(root/f'{phase}-states.csv').read_text().strip().split(',')];require(raw==proof['results'][phase]['raw'],'Raw state differs')
                    require(load(bound(root/f'{phase}-phase-observations.json'))==proof['results'][phase]['effects']['phase_observations'],'Raw burn phases differ')
                    summary=bound(root/f'{phase}-summary.txt').read_text();require('Spacecraft       : Sat' in summary and 'Coordinate System: EarthMJ2000Eq' in summary,'Missing selected spacecraft/frame')
                    expected=state(family,10 if family=='Propagate' else 20)
                    for i,label in enumerate(['X','Y','Z','VX','VY','VZ']):
                        values=re.findall(r'(?:^|\s)'+label+r'\s*=\s*([-+0-9.eE]+)',summary);require(len(values)==1 and abs(float(values[0])-expected[i])<=2e-5,'Independent command-epoch summary differs')
            selected.append(dict(mode=mode,name=n,log=str(log)))
    negatives=[]
    for label,mutate in [('wrong-state',lambda p:p['results'][0]['raw'].__setitem__(1,0)),('missing-close',lambda p:p['operations'].remove('close-tree')),('missing-reopen',lambda p:p['results'].pop()),('wrong-parent',lambda p:p.update(selected_parent='Missing'))]:
        bad=copy.deepcopy(last[0]);mutate(bad)
        try:check(bad,last[1],last[2])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    work=load(bound(stage/'scope-continuation.json'));require(work['factory_census_count']==len(work['factory_types'])==53 and work['qualified_keys']==[],'Narrowed full parent census')
    require(sha(bound(work['prior_worklist']))==work['prior_worklist_sha256'],'Prior census changed')
    return dict(status='passed-bounded-supported-control-operations-with-placement-rejections',qualified_keys=[],supported_gui_contexts=34,new_native_gui_passes=68,affected_regression_passes=2,
                original_stop_failures=2,retained_unsupported_placement_failures=10,raw_operation_rows=136,summary_components=816,negative_controls=negatives,selected=selected,rejected=rejected,
                remaining_parent='All Others!E196',technical_core_approval=False,scientific_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();v=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(v,indent=2)+'\n');print(json.dumps({k:x for k,x in v.items() if k not in ['selected','rejected']}))
