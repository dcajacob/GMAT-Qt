"""Audit missing command-operation witnesses; any-command parent stays open."""
import argparse,copy,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load,validate_visible
FAMILIES=['CallGmatFunction','CallPythonFunction','CallBuiltinGmatFunction','UpdateDynamicData']
NAMES=['QtGui.NasaCommandOperations'+f+p for f in FAMILIES for p in ['Root','Branch']]
REG='QtGui.NasaScriptEventExtraUpdateDynamicData'
OPERATIONS=['open-context-reuse','close-tree','open-double-click','insert-before','insert-after','delete-cancel-confirm','summary-context','unicode-reopen']
TAILS={'CallGmatFunction':[0,1,4,0],'CallPythonFunction':[0,-7,7,0],'CallBuiltinGmatFunction':[.5,.5,.5,.5],'UpdateDynamicData':[99,1,0,0]}

def expected(family):
    speed=math.sqrt(398600.4415/7000);angle=speed/7000*20
    return [20,7000*math.cos(angle),7000*math.sin(angle),0,-speed*math.sin(angle),speed*math.cos(angle),0,41]+TAILS[family]

def check_values(raw,family):
    truth=expected(family);require(len(raw)==12 and all(math.isfinite(v) for v in raw),'Invalid result row')
    for i,(a,b) in enumerate(zip(raw,truth)):require(abs(a-b)<=(1e-4 if i==0 else 2e-5 if i<4 else 2e-8 if i<7 else 1e-12),'Independent circle/scalar result differs')

def check(proof,family,position):
    require(proof['outcome']=='passed' and proof['family']==family and proof['position']==position,'Wrong/failed operation context')
    require(proof['operations']==OPERATIONS and [r['phase'] for r in proof['results']]==[0,1],'Missing operation/reopen')
    for row in proof['results']:check_values(row['raw'],family);require(all(abs(a-b)<1e-10 for a,b in zip(row['expected'],expected(family))),'Expected values differ from independent geometry')

def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v1.json')
    for p,h in manifest['artifact_sha256'].items():require(not Path(p).is_symlink() and sha(p)==h,'Changed regular artifact '+p)
    def bound(p):require(str(p) in manifest['artifact_sha256'],'Unbound artifact '+str(p));return Path(p)
    before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')),'Changed native inputs')
    launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==0 and launch['inputs_unchanged'],'Visible run failed')
    with tarfile.open(bound(stage/'executed-sources-v2.tar.gz')) as tar:frozen={m.name:tar.extractfile(m).read() for m in tar.getmembers()}
    require({p:hashlib.sha256(v).hexdigest() for p,v in frozen.items()}==before['sources'],'Executed archive differs')
    old=bound(stage/'UpdateDynamicData.before.cpp').read_text();needle='   return dynamicData->UpdateData(dataToUpdate);';replacement='   const bool completed = dynamicData->UpdateData(dataToUpdate);\n   BuildCommandSummary(completed);\n   return completed;';require(old.count(needle)==1 and old.replace(needle,replacement)==frozen['src/base/command/UpdateDynamicData.cpp'].decode(),'Unexpected core delta')
    ids=load(bound(stage/'core-before-identities.json'));require(ids['source_sha256']==hashlib.sha256(old.encode()).hexdigest() and any(Path(p).name.startswith('libGmatBase.') and h!=ids['image_sha256'] for p,h in before['images'].items()),'Core preimage/runtime unchanged or unbound')
    with tarfile.open(bound(stage/'source-v1.tar.gz')) as tar:
        for p in ['src/qtgui/tests/NasaCommandOperationExtensionTests.cpp','src/qtgui/tests/NasaCommandLifecycleTests.cpp']:require(tar.extractfile(p).read()==frozen[p],'Original operation assertions changed')
    first=load(bound(repo/'build/gui-acceptance/command-operations-headless-v1/results.json'));first_active={r['name']:r for r in first['results'] if r['status']!='not-run'};require(set(first_active)==set(NAMES),'Initial selection differs')
    for name,row in first_active.items():require(row['status']==('failed' if 'UpdateDynamicData' in name else 'passed'),'Initial failure set differs')
    mapped=load(bound(stage/'loaded-images.json'))['processes'];selected=[];last=None
    runs=[('headless','command-operations-headless-v1',NAMES[:6]),('headless','command-operations-headless-v2',NAMES[6:]+[REG]),('visible','command-operations-visible-v1',NAMES+[REG])]
    for mode,run,names in runs:
        receipt=load(bound(repo/'build/gui-acceptance'/run/'results.json'));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal GUI run');require(all(before['sources'].get(p)==h for p,h in receipt['source_hashes'].items()),'Wrong executed Qt source')
        active={r['name']:r for r in receipt['results'] if r['status']!='not-run'};require(set(active)==set(NAMES if run.endswith('v1') and mode=='headless' else names),'Wrong run selection');evidence={}
        for name in names:
            row=active[name];log=bound(row['log']);root=log.parent;junit=bound(root/'ctest.xml');require(row['status']=='passed' and row['returncode']==0,'Failed selected case')
            tests=ET.parse(junit).getroot().findall('testcase');require(len(tests)==1 and tests[0].attrib.get('status')=='run' and tests[0].attrib['name']==name and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Missing passing JUnit')
            proof=load(bound(root/'proof.json'));matches=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(matches)==1 and json.loads(matches[0])==proof,'Proof differs from stdout')
            if name==REG:
                require(proof['outcome']=='passed' and proof['family']=='UpdateDynamicData','Affected display regression failed');check_values(proof['observed'],'UpdateDynamicData');require([float(v.split(' = ')[1]) for v in proof['effects']['display_values']]==[41,7],'Display capture refreshed after later assignment')
            else:
                family=next(f for f in FAMILIES if f in name);position='Branch' if name.endswith('Branch') else 'Root';check(proof,family,position);last=(proof,family,position)
                fixture='src/qtgui/tests/script-event-commands/'+family+'.json';require(proof['fixture_sha256']==hashlib.sha256(frozen[fixture]).hexdigest(),'Retained command fixture changed')
                for phase in range(2):
                    raw=[float(v) for v in bound(root/f'{phase}-states.csv').read_text().strip().split(',')];require(raw==proof['results'][phase]['raw'],'Raw output differs');summary=bound(root/f'{phase}-summary.txt').read_text()
                    require('Spacecraft       : Sat' in summary and 'Coordinate System: EarthMJ2000Eq' in summary,'Missing command summary identity')
                    for i,label in enumerate(['X','Y','Z','VX','VY','VZ']):
                        matches=re.findall(r'(?:^|\s)'+label+r'\s*=\s*([-+0-9.eE]+)',summary);require(len(matches)==1 and abs(float(matches[0])-expected(family)[i+1])<=2e-5,'Independent summary state differs')
                script=bound(root/'initial.script').read_text();require(('If Keep == 41; % operation branch begin' in script)==(position=='Branch'),'Wrong actual branch context')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1 and before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Executed test image differs')
            evidence[name]=dict(log=str(log),log_sha256=sha(log),junit=str(junit),junit_sha256=sha(junit));selected.append(dict(mode=mode,name=name,log=str(log)))
            if mode=='visible':
                traces=list(root.glob('trace-*.jsonl'));require(traces,'Missing visible trace')
                for trace in traces:
                    process=mapped.get(trace.stem.removeprefix('trace-'));require(process and process['executable']==exes[0],'Missing process mapping')
                    for p,h in process['images'].items():require(before['images'].get(p)==h,'Mapped runtime image differs')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith('libGmat'):require(process['images'].get(p)==h,'Missing mapped core/test')
        if mode=='visible':validate_visible(receipt,evidence)
    negatives=[]
    for label,mutate in [('wrong-state',lambda p:p['results'][0]['raw'].__setitem__(1,0)),('missing-close',lambda p:p['operations'].remove('close-tree')),('missing-reopen',lambda p:p['results'].pop()),('wrong-position',lambda p:p.update(position='Missing'))]:
        corrupt=copy.deepcopy(last[0]);mutate(corrupt)
        try:check(corrupt,last[1],last[2])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    work=load(bound(stage/'whole-parent-worklist.json'));require(work['factory_census_count']==len(work['rows'])==53 and all(not r['whole_row_qualified'] for r in work['rows']),'Narrowed parent census')
    return dict(status='passed-bounded-command-operations',qualified_keys=[],new_gui_contexts=8,new_native_gui_passes=16,affected_regression_passes=2,raw_operation_rows=32,independent_summary_components=192,negative_controls=negatives,selected=selected,remaining_parent='All Others!E196',technical_core_approval=False,scientific_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='selected'}))
