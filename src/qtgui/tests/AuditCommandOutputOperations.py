"""Audit output/plot Mission Tree operations without narrowing the any-command parent."""
import argparse, copy, hashlib, json, math, re, tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require, sha, load, validate_visible
from AuditCommandNavigationOperations import compare, circle, OPERATIONS

FAMILIES = ['ClearPlot','PenUp','PenDown','MarkPoint','Write','Global','Report','Save','Toggle','SaveMission','CommandEchoOn','CommandEchoOff']
NAMES = ['QtGui.NasaCommandOutputOperations'+f+p for f in FAMILIES for p in ['Root','Branch']]
REG = 'QtGui.NasaScriptEventExtraCommandEcho'
PLOTS = ['ClearPlot','PenUp','PenDown','MarkPoint']


def numeric(raw):
    compare(raw, [20]+circle(20)+[41,0,1,0,0], [1e-4]+[2e-5]*3+[2e-8]*3+[1e-12]*5)


def effects(value, family, position):
    phases = value['phase_observations']
    if family.startswith('CommandEcho'):
        require(phases == [{'before_command':'Report','echo_commands':True}, {'before_command':'Report','echo_commands':False}], 'Echo On/Off runtime transitions differ')
    elif family == 'Global':
        require(len(phases) == 2, 'Missing Global migration phases')
        for index, row in enumerate(phases):
            require(row == dict(before_command='Global' if index == 0 else 'Report', Keep_global=True,
                               Count_global=False, Keep_in_local=index == 0, Keep_in_global=index == 1,
                               Count_in_local=True, Count_in_global=False, same_Keep_pointer=True), 'Global moved the wrong object/store')
    else:
        require(phases == [], 'Unexpected command phase data')
    if family in PLOTS:
        points, control = value['trace_points'], value['control_points']
        require(points and control and any(p[0] < 10-1e-5 for p in control), 'Missing plot interval/control')
        for p in points+control:
            require(len(p) == 4 and all(math.isfinite(v) for v in p[:2]) and abs(p[0]-p[1]) <= 1e-4 and -1e-4 <= p[0] <= 20+1e-4, 'Plot differs from elapsed-time reference')
        later = [p for p in points if p[0] > 10+1e-5]
        if family == 'ClearPlot': require(all(p[0] >= 10-1e-5 for p in points), 'ClearPlot retained earlier data')
        if family == 'PenUp': require(not value['trace_pen_down'] and later and not any(p[2] for p in later), 'PenUp did not break later links')
        if family == 'PenDown': require(value['trace_pen_down'] and later and all(p[2] for p in later) and any(1e-5 < p[0] < 10+1e-5 and not p[2] for p in points), 'PenDown lost initial gap/resumed links')
        if family == 'MarkPoint':
            marks = [p for p in points if p[3]]
            require(len(marks) == 1 and abs(marks[0][0]-10) <= 1e-4 and not any(p[3] for p in control), 'MarkPoint changed wrong point/control')
    if family == 'Toggle': require(value['trace_active'] is False, 'Toggle did not turn selected trace off')
    if family == 'Write': require(value['written_message'] == 'GUI_EVENT_WRITE_41_Ω', 'Write string differs')
    if family == 'Report': require(value['extra_value'] == 41, 'Report selected value differs')
    if family == 'Save': compare(value['saved_cartesian'], circle(20), [2e-5]*6)
    if family == 'SaveMission': require(value['mission_saved'] == (position == 'Root'), 'SaveMission terminal/branch contract differs')


def check(proof, family, position):
    require(proof['outcome'] == 'passed' and proof['family'] == family and proof['position'] == position, 'Wrong operation context')
    require(proof['operations'] == OPERATIONS and [p['phase'] for p in proof['results']] == [0,1], 'Missing operation/reopen')
    for row in proof['results']:
        numeric(row['raw']); effects(row['effects'], family, position)
    if family == 'SaveMission' and position == 'Root': numeric(proof['export_replay'])
    else: require(proof['export_replay'] == [], 'Unexpected mission export replay')


def audit(repo, stage):
    manifest = load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for name, digest in manifest.items(): require(not Path(name).is_symlink() and sha(name) == digest, 'Changed regular artifact '+name)
    def bound(path):
        path = Path(path); require(str(path) in manifest, 'Unbound artifact '+str(path)); return path
    before = load(bound(stage/'visible-inputs-before.json'))
    require(before == load(bound(stage/'visible-inputs-after.json')), 'Native inputs changed')
    launch = load(bound(stage/'visible-launch.json'))
    require(launch['returncode'] == 0 and launch['inputs_unchanged'] and launch['mapped_processes'] == 25, 'Visible run incomplete')
    with tarfile.open(bound(stage/'executed-sources-v2.tar.gz')) as tar: frozen = {m.name:tar.extractfile(m).read() for m in tar.getmembers()}
    require({n:hashlib.sha256(v).hexdigest() for n,v in frozen.items()} == before['sources'], 'Executed source archive differs')
    name = 'plugins/ScriptToolsPlugin/src/base/command/CommandEcho.cpp'
    old = bound(stage/'CommandEcho.before.cpp').read_text()
    needle = '   GmatGlobal::Instance()->SetCommandEchoMode(echoStatus);\n   return true;'
    require(old.count(needle) == 1 and old.replace(needle, needle.replace('   return true;', '   BuildCommandSummary(true);\n   return true;')) == frozen[name].decode(), 'Unexpected production delta')
    old_images = load(bound(stage/'before-core-images.json'))
    require(any(Path(n).name.startswith('libScriptTools.') and old_images.get(n) != h for n,h in before['images'].items()), 'CommandEcho runtime was not rebuilt')
    with tarfile.open(bound(stage/'source-v1.tar.gz')) as tar:
        for n in ['src/qtgui/tests/NasaCommandOutputOperationTests.cpp','src/qtgui/tests/NasaCommandLifecycleTests.cpp','src/qtgui/tests/NasaScriptEventCommandTests.cpp']:
            require(tar.extractfile(n).read() == frozen[n], 'Original assertions changed')
    first = load(bound(repo/'build/gui-acceptance/command-output-headless-v1/results.json'))
    active = {v['name']:v for v in first['results'] if v['status'] != 'not-run'}
    require(set(active) == set(NAMES), 'Original census differs')
    for n,row in active.items():
        failed = 'CommandEcho' in n
        require(row['status'] == ('failed' if failed else 'passed'), 'Original failure set differs')
        if failed: require('FAIL Context summary lacks selected spacecraft/frame' in bound(row['log']).read_text(), 'Lost original summary failure')
    mapped = load(bound(stage/'loaded-images.json'))['processes']; selected=[]; last=None
    runs = [('headless','command-output-headless-v1',NAMES[:20]),('headless','command-output-headless-v2',NAMES[20:]+[REG]),('visible','command-output-visible-v1',NAMES+[REG])]
    for mode, run, names in runs:
        receipt = load(bound(repo/'build/gui-acceptance'/run/'results.json'))
        require(receipt['mode'] == mode and receipt.get('finished_utc') and receipt['end_reason'] == 'completed-selection', 'Nonterminal run')
        require(all(before['sources'].get(n) == h for n,h in receipt['source_hashes'].items()), 'Wrong executed source version')
        active = {v['name']:v for v in receipt['results'] if v['status'] != 'not-run'}
        require(set(active) == set(NAMES if run == 'command-output-headless-v1' else names), 'Wrong selection')
        evidence={}
        for n in names:
            row=active[n]; log=bound(row['log']); root=log.parent; junit=bound(root/'ctest.xml')
            require(row['status'] == 'passed' and row['returncode'] == 0, 'Selected case failed')
            tests=ET.parse(junit).getroot().findall('testcase')
            require(len(tests) == 1 and tests[0].attrib['name'] == n and tests[0].find('failure') is None and tests[0].find('skipped') is None, 'Missing passing JUnit')
            proof=load(bound(root/'proof.json')); messages=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())
            require(len(messages) == 1 and json.loads(messages[0]) == proof, 'Proof differs from stdout')
            if n == REG:
                require(proof['outcome'] == 'passed' and proof['family'] == 'CommandEcho' and proof['native_outcome'] == 'Completed', 'Existing echo regression failed')
                numeric(proof['observed']); effects({'phase_observations':proof['phase_observations']},'CommandEchoOn','Root')
            else:
                position='Branch' if n.endswith('Branch') else 'Root'; family=n.removeprefix('QtGui.NasaCommandOutputOperations').removesuffix(position)
                require(family in FAMILIES, 'Unknown family'); check(proof,family,position); last=(proof,family,position)
                fixture='src/qtgui/tests/command-operation-output/'+family+'.json'; spec=json.loads(frozen[fixture]); origin=frozen[spec['operation_fixture_origin']]
                require(hashlib.sha256(origin).hexdigest() == spec['operation_fixture_origin_sha256'] and spec['script'] == json.loads(origin)['script'], 'Original mission fixture changed')
                require(proof['fixture_sha256'] == hashlib.sha256(frozen[fixture]).hexdigest() == sha(bound(root/'fixture.json')), 'Executed fixture differs')
                require(('If Keep == 41; % operation branch begin' in bound(root/'initial.script').read_text()) == (position == 'Branch'), 'Wrong branch position')
                for phase in range(2):
                    value=proof['results'][phase]; raw=[float(v) for v in bound(root/f'{phase}-states.csv').read_text().strip().split(',')]
                    require(raw == value['raw'], 'Raw state differs'); compare(value['expected'],[20]+circle(20)+[41,0,1,0,0],[1e-10]*12)
                    summary=bound(root/f'{phase}-summary.txt').read_text(); truth=circle(10 if family in PLOTS else 20)
                    require('Spacecraft       : Sat' in summary and 'Coordinate System: EarthMJ2000Eq' in summary, 'Summary identity missing')
                    for i,label in enumerate(['X','Y','Z','VX','VY','VZ']):
                        matches=re.findall(r'(?:^|\s)'+label+r'\s*=\s*([-+0-9.eE]+)',summary)
                        require(len(matches) == 1 and abs(float(matches[0])-truth[i]) <= 2e-5, 'Independent summary state differs')
                    require(load(bound(root/f'{phase}-phase-observations.json')) == value['effects']['phase_observations'], 'Raw runtime phases differ')
                    if family in PLOTS: require(load(bound(root/f'{phase}-plot-observations.json')) == value['effects'], 'Raw plot observations differ')
                    if family == 'Report': require(float(bound(root/f'{phase}-reported-value.txt').read_text()) == 41, 'Raw Report differs')
                    if family == 'Save':
                        text=bound(root/f'{phase}-saved-runtime.txt').read_text()
                        for i,label in enumerate(['X','Y','Z','VX','VY','VZ']):
                            m=re.search(r'(?:^|\n)(?:GMAT )?Sat\.'+label+r' = ([^;\n]+);',text)
                            require(m and abs(float(m[1])-circle(20)[i]) <= 2e-5, 'Saved runtime state differs')
                    if family == 'SaveMission':
                        path=root/f'{phase}-mission-export.script'
                        if position == 'Root': require(all(token in bound(path).read_text() for token in ['Create Spacecraft Sat','BeginMissionSequence','Propagate','SaveMission']), 'Incomplete exported mission')
                        else: require(not path.exists(), 'Nested SaveMission unexpectedly exported')
                if family == 'SaveMission' and position == 'Root':
                    raw=[float(v) for v in bound(root/'export-replayed-state.csv').read_text().strip().split(',')]
                    require(raw == proof['export_replay'], 'Native export replay differs'); numeric(raw)
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']]
            require(len(exes) == 1 and before['images'].get(exes[0]) == receipt['binary_hashes'][exes[0]], 'Executed image differs')
            evidence[n]=dict(log=str(log),log_sha256=sha(log),junit=str(junit),junit_sha256=sha(junit)); selected.append(dict(mode=mode,name=n,log=str(log)))
            if mode == 'visible':
                traces=list(root.glob('trace-*.jsonl')); require(traces,'Missing native trace')
                for trace in traces:
                    process=mapped.get(trace.stem.removeprefix('trace-')); require(process and process['executable'] == exes[0],'Missing process mapping')
                    for p,h in process['images'].items(): require(before['images'].get(p) == h,'Mapped image differs')
                    for p,h in before['images'].items():
                        if p == exes[0] or Path(p).name.startswith(('libGmat','libScriptTools')): require(process['images'].get(p) == h,'Missing mapped core/plugin/test')
        if mode == 'visible': validate_visible(receipt,evidence)
    negatives=[]
    for label,mutate in [('wrong-state',lambda p:p['results'][0]['raw'].__setitem__(1,0)),('missing-close',lambda p:p['operations'].remove('close-tree')),('missing-reopen',lambda p:p['results'].pop()),('wrong-echo-state',lambda p:p['results'][0]['effects']['phase_observations'][0].update(echo_commands=False))]:
        bad=copy.deepcopy(last[0]);mutate(bad)
        try:check(bad,last[1],last[2])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    work=load(bound(stage/'scope-continuation.json')); require(work['factory_census_count'] == len(work['factory_types']) == 53 and work['qualified_keys'] == [], 'Narrowed parent census')
    require(sha(bound(work['prior_worklist'])) == work['prior_worklist_sha256'], 'Prior census changed')
    return dict(status='passed-bounded-output-operations',qualified_keys=[],new_gui_contexts=24,new_native_gui_passes=48,affected_regression_passes=2,
                raw_operation_rows=96,export_replay_rows=2,summary_components=576,negative_controls=negatives,selected=selected,
                remaining_parent='All Others!E196',technical_core_approval=False,scientific_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args(); result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k != 'selected'}))
