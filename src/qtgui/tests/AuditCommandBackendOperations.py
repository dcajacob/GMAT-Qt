"""Audit missing backend command-operation witnesses; any-command parent stays open."""
import argparse,copy,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load,validate_visible
FAMILIES=['Set','BeginFileThrust','EndFileThrust','FindEvents']
NAMES=['QtGui.NasaCommandBackendOperations'+f+p for f in FAMILIES for p in ['Root','Branch']]
REG='QtGui.NasaScriptEventBackendFindEvents'
OPERATIONS=['open-context-reuse','close-tree','open-double-click','insert-before','insert-after','delete-cancel-confirm','summary-context','unicode-reopen']
TAILS={'Set':[0,1,1.4,0],'BeginFileThrust':[0,1,95,0],'EndFileThrust':[0,1,95,0],'FindEvents':[0,1,0,0]}

def cartesian(family,seconds):
    speed=math.sqrt(398600.4415/7000);angle=speed/7000*seconds;inc=math.pi/4 if family=='FindEvents' else 0
    return [7000*math.cos(angle),7000*math.sin(angle)*math.cos(inc),7000*math.sin(angle)*math.sin(inc),-speed*math.sin(angle),speed*math.cos(angle)*math.cos(inc),speed*math.cos(angle)*math.sin(inc)]

def expected(family):
    seconds=6500 if family=='FindEvents' else 20
    return [seconds]+cartesian(family,seconds)+[41]+TAILS[family]

def summary_expected(family):
    return cartesian(family,{'Set':0,'BeginFileThrust':0,'EndFileThrust':10,'FindEvents':6500}[family])

def contact(text):
    import datetime
    pattern=r'^(\d{2} [A-Za-z]{3} \d{4} \d{2}:\d{2}:\d{2}\.\d+)\s+(\d{2} [A-Za-z]{3} \d{4} \d{2}:\d{2}:\d{2}\.\d+)\s+([0-9.]+)\s*$'
    rows=re.findall(pattern,text,re.M);require(len(rows)==1 and 'Target: Sat' in text and 'Observer: East' in text,'Missing/extra contact interval')
    a,b,d=rows[0];epoch=datetime.datetime(2000,1,1,12);times=[(datetime.datetime.strptime(v,'%d %b %Y %H:%M:%S.%f')-epoch).total_seconds() for v in [a,b]]+[float(d)]
    require(all(abs(a-b)<=.002 for a,b in zip(times,[2773.493088543415,3226.2079913020134,452.7149027585983])),'Independent contact interval differs')
    return times


def check_values(raw,family):
    truth=expected(family);require(len(raw)==12 and all(math.isfinite(v) for v in raw),'Invalid result row')
    for i,(a,b) in enumerate(zip(raw,truth)):require(abs(a-b)<=(1e-4 if i==0 else 2e-5 if i<4 else 2e-8 if i<7 else 1e-12),'Independent circle/scalar result differs')

def check(proof,family,position):
    require(proof['outcome']=='passed' and proof['family']==family and proof['position']==position,'Wrong/failed operation context')
    require(proof['operations']==OPERATIONS and [r['phase'] for r in proof['results']]==[0,1],'Missing operation/reopen')
    for row in proof['results']:check_values(row['raw'],family);require(all(abs(a-b)<1e-10 for a,b in zip(row['expected'],expected(family))),'Expected values differ from independent geometry')

def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v2.json')
    for p,h in manifest['artifact_sha256'].items():require(not Path(p).is_symlink() and sha(p)==h,'Changed regular artifact '+p)
    def bound(p):require(str(p) in manifest['artifact_sha256'],'Unbound artifact '+str(p));return Path(p)
    before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')),'Changed native inputs')
    launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==0 and launch['inputs_unchanged'],'Visible run failed')
    with tarfile.open(bound(stage/'executed-sources-v2.tar.gz')) as tar:frozen={m.name:tar.extractfile(m).read() for m in tar.getmembers()}
    require({p:hashlib.sha256(v).hexdigest() for p,v in frozen.items()}==before['sources'],'Executed archive differs')
    old=bound(stage/'FindEvents.before.cpp').read_text();needle='//   BuildCommandSummary(true);  // do I need this?';replacement='   BuildCommandSummary(true);';require(old.count(needle)==1 and old.replace(needle,replacement)==frozen['src/base/command/FindEvents.cpp'].decode(),'Unexpected core delta')
    ids=load(bound(stage/'before-core-images.json'));require(any(Path(p).name.startswith('libGmatBase.') and h!=ids.get(p) for p,h in before['images'].items()),'Core runtime unchanged')
    with tarfile.open(bound(stage/'source-v1.tar.gz')) as tar:
        for p in ['src/qtgui/tests/NasaCommandBackendOperationTests.cpp','src/qtgui/tests/NasaCommandLifecycleTests.cpp']:require(tar.extractfile(p).read()==frozen[p],'Original operation assertions changed')
    first=load(bound(repo/'build/gui-acceptance/command-backends-headless-v1/results.json'));first_active={r['name']:r for r in first['results'] if r['status']!='not-run'};require(set(first_active)==set(NAMES),'Initial selection differs')
    for name,row in first_active.items():require(row['status']==('failed' if 'FindEvents' in name else 'passed'),'Initial failure set differs')
    mapped=load(bound(stage/'loaded-images.json'))['processes'];selected=[];last=None
    runs=[('headless','command-backends-headless-v1',NAMES[:6]),('headless','command-backends-headless-v2',NAMES[6:]+[REG]),('visible','command-backends-visible-v1',NAMES+[REG])]
    for mode,run,names in runs:
        receipt=load(bound(repo/'build/gui-acceptance'/run/'results.json'));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal GUI run');version_sources=dict(before['sources']);version_sources.update({'src/base/command/FindEvents.cpp':hashlib.sha256(old.encode()).hexdigest()} if mode=='headless' and run.endswith('v1') else {});require(all(version_sources.get(p)==h for p,h in receipt['source_hashes'].items()),'Wrong executed source version')
        active={r['name']:r for r in receipt['results'] if r['status']!='not-run'};require(set(active)==set(NAMES if run.endswith('v1') and mode=='headless' else names),'Wrong run selection');evidence={}
        for name in names:
            row=active[name];log=bound(row['log']);root=log.parent;junit=bound(root/'ctest.xml');require(row['status']=='passed' and row['returncode']==0,'Failed selected case')
            tests=ET.parse(junit).getroot().findall('testcase');require(len(tests)==1 and tests[0].attrib.get('status')=='run' and tests[0].attrib['name']==name and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Missing passing JUnit')
            proof=load(bound(root/'proof.json'));matches=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(matches)==1 and json.loads(matches[0])==proof,'Proof differs from stdout')
            if name==REG:
                require(proof['outcome']=='passed' and proof['family']=='FindEvents','Affected contact regression failed');check_values(proof['observed'],'FindEvents');events=proof['effects']['contact_intervals'];require(len(events)==1 and all(abs(events[0][k]-v)<=.002 for k,v in zip(['start','end','duration'],[2773.493088543415,3226.2079913020134,452.7149027585983])),'Contact regression values differ')
            else:
                family=next(f for f in FAMILIES if f in name);position='Branch' if name.endswith('Branch') else 'Root';check(proof,family,position);last=(proof,family,position)
                fixture='src/qtgui/tests/command-operation-backends/'+family+'.json';spec=json.loads(frozen[fixture]);origin=spec['operation_fixture_origin'];require(hashlib.sha256(frozen[origin]).hexdigest()==spec['operation_fixture_origin_sha256'] and spec['script']==json.loads(frozen[origin])['script'],'Original executable fixture changed');require(proof['fixture_sha256']==hashlib.sha256(frozen[fixture]).hexdigest(),'Retained command fixture changed')
                for phase in range(2):
                    raw=[float(v) for v in bound(root/f'{phase}-states.csv').read_text().strip().split(',')];require(raw==proof['results'][phase]['raw'],'Raw output differs');summary=bound(root/f'{phase}-summary.txt').read_text()
                    require('Spacecraft       : Sat' in summary and 'Coordinate System: EarthMJ2000Eq' in summary,'Missing command summary identity')
                    for i,label in enumerate(['X','Y','Z','VX','VY','VZ']):
                        matches=re.findall(r'(?:^|\s)'+label+r'\s*=\s*([-+0-9.eE]+)',summary);require(len(matches)==1 and abs(float(matches[0])-summary_expected(family)[i])<=2e-5,'Independent summary state differs')
                for phase in range(2):
                    side=proof['results'][phase]['side_effects'];require(proof['results'][phase]['summary_expected']==summary_expected(family) or all(abs(a-b)<1e-10 for a,b in zip(proof['results'][phase]['summary_expected'],summary_expected(family))),'Wrong summary epoch geometry')
                    if family=='FindEvents':contact(bound(root/f'{phase}-events.txt').read_text());require(side==[],'Unexpected contact side rows')
                    else:
                        truth=[[21545,1.4,7000,0,0,0,math.sqrt(398600.4415/7000),0]] if family=='Set' else [[10,100-.5*10],[20,95]]
                        tolerances=[1e-11,1e-12,1e-8,1e-8,1e-8,1e-10,1e-10,1e-10] if family=='Set' else [1e-4,1e-8]
                        raw=[[float(v) for v in re.split(r'[,\s]+',line.strip())] for line in bound(root/f'{phase}-extra.csv').read_text().splitlines() if line.strip()];require(raw==side and len(raw)==len(truth),'Missing backend side effect')
                        for a,b in zip(raw,truth):require(len(a)==len(b) and all(abs(v-w)<=t for v,w,t in zip(a,b,tolerances)),'Selective import/mass consumption differs')
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
    work=load(bound(stage/'scope-continuation.json'));require(work['factory_census_count']==len(work['factory_types'])==53 and work['qualified_keys']==[],'Narrowed parent census')
    return dict(status='passed-bounded-backend-operations',qualified_keys=[],new_gui_contexts=8,new_native_gui_passes=16,affected_regression_passes=2,raw_operation_rows=32,independent_summary_components=192,negative_controls=negatives,selected=selected,remaining_parent='All Others!E196',technical_core_approval=False,scientific_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='selected'}))
