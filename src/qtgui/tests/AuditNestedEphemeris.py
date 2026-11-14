"""Audit nested final-only OEM membership and preserve exact failing sources."""
import argparse,copy,csv,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load
from AuditCommandNavigationOperations import compare
PREFIX='QtGui.NasaNestedEphemeris'
NEW=[PREFIX+f for f in ['NestedDC','NestedYukon']]
REG=['QtGui.NasaSolverEphemerisTarget','QtGui.NasaSolverEphemerisOptimize','QtGui.NativeNasaSolverPublicationNested','QtGui.NativeNasaSolverPublicationMixed']
CORE=['src/base/subscriber/EphemerisFile.cpp','src/base/subscriber/EphemerisFile.hpp','src/base/command/SolverBranchCommand.cpp','src/base/command/SolverBranchCommand.hpp']


def circle(radius,t):
    w=math.sqrt(398600.4415/radius**3);v=math.sqrt(398600.4415/radius);a=w*t
    return [radius*math.cos(a),radius*math.sin(a),0,-v*math.sin(a),v*math.cos(a),0]


def check(proof):
    require(proof['outcome']=='passed' and proof['family'] in ['NestedDC','NestedYukon'] and proof['segments']==1 and len(proof['rows'])==25,'Wrong OEM case/arc census')
    accepted=[];local=0
    for row in proof['passes']:
        states=row['states'];require(len(states)==2 and {s['name'] for s in states}==({'DC','Inner'} if proof['family']=='NestedDC' else {'Opt','Inner'}),'Wrong numerical solver census')
        truth=all(s['state']==10014 for s in states);require(row['accepted']==truth,'Accepted state not derived from both numerical solver clones')
        if truth:accepted.append(row['pass'])
        if any(s['name']=='Inner' and s['state']==10014 for s in states) and not truth:local+=1
    require(len(accepted)==proof['accepted_passes']==1 and local==proof['local_accepted_outer_trial']>=2,'Missing nested local accepted/outer trial partition')
    for i,row in enumerate(proof['rows']):compare(row,[5*i]+circle(7230,5*i),[.002]+[2e-5]*3+[2e-8]*3)


def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v2.json')['artifact_sha256']
    for n,h in manifest.items():require(not Path(n).is_symlink() and sha(n)==h,'Changed regular artifact '+n)
    def bound(p):
        p=Path(p);require(str(p) in manifest,'Unbound artifact '+str(p));return p
    def archive(p):
        with tarfile.open(bound(p)) as t:return {m.name:t.extractfile(m).read() for m in t.getmembers()}
    before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')),'Changed visible inputs')
    frozen=archive(stage/'executed-sources-v3.tar.gz');original=archive(stage/'source-v2.tar.gz')
    require({n:hashlib.sha256(v).hexdigest() for n,v in frozen.items()}==before['sources'],'Executed source archive differs')
    require(original['src/qtgui/tests/NasaNestedEphemerisTests.cpp']==frozen['src/qtgui/tests/NasaNestedEphemerisTests.cpp'],'Original failing assertions changed')
    for n in CORE:require(bound(stage/(Path(n).name+'.before')).read_bytes()==original[n] and original[n]!=frozen[n],'Wrong core preimage/delta')
    expected={n:original[n].decode() for n in CORE}
    expected[CORE[0]]=expected[CORE[0]].replace('#include "EphemerisFile.hpp"','#include "EphemerisFile.hpp"\n#include "SolverBranchCommand.hpp"',1)
    insertion='// An inner solution remains a trial while an enclosing solver is iterating.\n// Keep the publisher\'s state intact for reports and numerical execution.\nvoid EphemerisFile::SetRunState(Gmat::RunState state)\n{\n   Subscriber::SetRunState(SolverBranchCommand::GetSubscriberRunState(state));\n}\n\n'
    marker='//------------------------------------------------------------------------------\n// EphemerisFile(const std::string &name, const std::string &type = "EphemerisFile")'
    expected[CORE[0]]=expected[CORE[0]].replace(marker,insertion+marker,1)
    expected[CORE[1]]=expected[CORE[1]].replace('   // methods inherited from Subscriber\n','   // methods inherited from Subscriber\n   virtual void         SetRunState(Gmat::RunState state) override;\n',1)
    expected[CORE[2]]=expected[CORE[2]].replace('Gmat::RunState SolverBranchCommand::GetPlotRunState(Gmat::RunState state)\n{\n   // An inner accepted pass is still a trial until every enclosing solver is\n   // accepted. This affects plot filtering/colors only, never publisher state.','Gmat::RunState SolverBranchCommand::GetPlotRunState(Gmat::RunState state)\n{\n   return GetSubscriberRunState(state);\n}\n\nGmat::RunState SolverBranchCommand::GetSubscriberRunState(Gmat::RunState state)\n{\n   // An inner accepted pass is still a trial until every enclosing solver is\n   // accepted. Subscribers opt in without changing the publisher\'s state.',1)
    expected[CORE[3]]=expected[CORE[3]].replace('   // Plot-only classification: the publisher state remains available unchanged\n   // to propagation, report subscribers and the numerical solver machinery.\n   static Gmat::RunState    GetPlotRunState(Gmat::RunState state);','   // Nested acceptance classification for selected output subscribers. The\n   // publisher state remains unchanged for propagation, reports and solvers.\n   static Gmat::RunState    GetSubscriberRunState(Gmat::RunState state);\n   static Gmat::RunState    GetPlotRunState(Gmat::RunState state);',1)
    require(all(expected[n]==frozen[n].decode() for n in CORE),'Unexpected core change beyond nested acceptance classification')
    old_images=load(bound(stage/'before-core-images.json'));require(any(Path(n).name.startswith('libGmatBase.') and old_images.get(n)!=h for n,h in before['images'].items()),'Core runtime was not rebuilt')
    require(original['src/base/executive/Publisher.cpp']==frozen['src/base/executive/Publisher.cpp'],'Publisher semantics changed')
    launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==0 and launch['mapped_processes']==6 and launch['inputs_unchanged'],'Wrong visible launch')
    mapped=load(bound(stage/'loaded-images.json'))['processes'];selected=[];failures=[];trials_total=0;raw_total=0;last=None
    for mode,v,names in [('headless',1,NEW),('headless',2,NEW),('headless',3,NEW+REG),('visible',1,NEW+REG)]:
        receipt=load(bound(repo/'build/gui-acceptance'/f'nested-ephemeris-{mode}-v{v}'/'results.json'));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal selection')
        version=dict(before['sources'])
        if mode=='headless' and v<3:version.update({n:hashlib.sha256(b).hexdigest() for n,b in archive(stage/f'source-v{v}.tar.gz').items()})
        require(all(version.get(n)==h for n,h in receipt['source_hashes'].items()),'Wrong source version')
        active={r['name']:r for r in receipt['results'] if r['status']!='not-run'};require(set(active)==set(names),'Wrong attempted census')
        if mode=='visible':
            d=receipt['visible_display'];require(receipt['max_screenshots']==0 and d['resizable'] and d['start_minimized'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'],'Wrong desktop policy')
        for name,row in active.items():
            passed=mode=='visible' or v==3;require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Changed native outcome')
            log=bound(row['log']);root=log.parent;tests=ET.parse(bound(root/'ctest.xml')).getroot().findall('testcase');require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None)==passed,'Changed JUnit outcome')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1,'Native executable absent')
            if passed:require(before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Wrong qualified executable')
            if mode=='visible':
                require(row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Native exposure missing');traces=list(root.glob('trace-*.jsonl'));require(traces,'Missing trace')
                for trace in traces:
                    proc=mapped.get(trace.stem.removeprefix('trace-'));require(proc and proc['executable']==exes[0],'No mapped process')
                    for p,h in proc['images'].items():require(before['images'].get(p)==h,'Mapped image differs')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith('libGmat'):require(proc['images'].get(p)==h,'Missing mapped core/test')
            if not passed:
                error='Formation native widget platform required' if v==1 else 'Nested OEM includes nonfinal trajectory';require('FAIL '+error in log.read_text() and not (root/'proof.json').exists(),'Original failure lost')
                if v==2:
                    payload=bound(root/'mission.oem').read_text();rows=[l.split() for l in payload.splitlines() if re.match(r'^\d{4}-\d\d-\d\dT',l.strip())];require(len(rows)==(175 if name.endswith('NestedDC') else 425) and any(abs(math.sqrt(sum(float(x)**2 for x in r[1:4]))-7230)>1 for r in rows),'Old trial trajectory contamination absent')
                failures.append(dict(mode=mode,version=v,name=name,error=error));continue
            proof=load(bound(root/'proof.json'));messages=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(messages)==1 and json.loads(messages[0])==proof and proof['outcome']=='passed','Proof/stdout mismatch')
            if name in NEW:
                check(proof);last=proof;family=name.removeprefix(PREFIX);fixture='src/qtgui/tests/nested-ephemeris/'+family+'.json';spec=json.loads(frozen[fixture]);require(sha(bound(root/'fixture.json'))==hashlib.sha256(frozen[fixture]).hexdigest() and hashlib.sha256(frozen[spec['fixture_origin']]).hexdigest()==spec['fixture_origin_sha256'],'Changed fixture/origin')
                require(load(bound(root/'solver-passes.json'))==proof['passes'],'Pass states/proof mismatch');raw=load(bound(root/'raw-publications.json'));require(raw,'Raw publication grid absent');raw_total+=len(raw)
                csv_rows=[[float(v) for v in row] for row in csv.reader(bound(root/'trials.csv').open())];starts=[r for r in csv_rows if len(r)==2];ends=[r for r in csv_rows if len(r)==9];require(len(starts)==len(ends)==len(proof['passes']),'Incomplete raw control/state trials')
                for i,(start,end,p) in enumerate(zip(starts,ends,proof['passes']),1):
                    require(p['pass']==i,'Pass order changed');compare(end[:2],start,[1e-12]*2);radius=7000+100*start[0]+10*start[1];compare(end[2:],[120]+circle(radius,120),[1e-4]+[2e-5]*3+[2e-8]*3);samples=[r for r in raw if r['pass']==i];require(len(samples)==25 and all(r['accepted']==p['accepted'] for r in samples),'Raw numerical acceptance partition differs')
                    for j,r in enumerate(samples):compare(r['state'],circle(radius,5*j),[2e-5]*3+[2e-8]*3)
                trials_total+=len(starts);require(load(bound(root/'rows.json'))==proof['rows'],'OEM rows/proof mismatch');payload=bound(root/'mission.oem').read_text();require(all(re.search(r'^'+key+r'\s*=\s*'+value+r'\s*$',payload,re.M|re.I) for key,value in [('TIME_SYSTEM','UTC'),('CENTER_NAME','EARTH'),('REF_FRAME','EME2000')]),'OEM frame/time metadata differs')
            selected.append(dict(mode=mode,name=name))
    negatives=[]
    for label,mutate in [('trial-output',lambda p:p['rows'][0].__setitem__(1,7030)),('missing-knot',lambda p:p['rows'].pop()),('wrong-solver-acceptance',lambda p:p['passes'][0].update(accepted=True)),('missing-nested-partition',lambda p:p.update(local_accepted_outer_trial=0))]:
        bad=copy.deepcopy(last);mutate(bad)
        try:check(bad)
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    return dict(status='passed-bounded-nested-final-solution-ephemeris-repair',qualified_keys=[],new_gui_passes=4,affected_regression_passes=8,retained_core_failures=2,retained_registration_failures=2,OEM_rows=100,OEM_components=600,raw_trial_rows=trials_total,raw_publication_rows=raw_total,negative_controls=negatives,selected=selected,failures=failures,production_changed_files=CORE,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();v=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(v,indent=2)+'\n');print(json.dumps({k:x for k,x in v.items() if k not in ['selected','failures']}))
