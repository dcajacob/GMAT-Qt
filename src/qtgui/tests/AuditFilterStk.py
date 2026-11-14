"""Audit every supported filter/smoother STK row against truth and accepted publications."""
import argparse,copy,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load
from AuditNestedEphemeris import circle
from AuditCommandNavigationOperations import compare
PREFIX='QtGui.NasaFilterStk';NAMES=[PREFIX+f for f in ['Filter','Smoother']]
CORE=['src/base/subscriber/EphemerisFile.cpp','src/base/subscriber/EphemerisFile.hpp','src/base/command/SolverBranchCommand.cpp','src/base/command/SolverBranchCommand.hpp']


def check(proof):
    require(proof['outcome']=='passed' and proof['family'] in ['Filter','Smoother'] and [p['phase'] for p in proof['phases']]==[0,1],'Wrong STK/reopen context')
    for phase in proof['phases']:
        require(phase['selected_phase']==('forward' if proof['family']=='Filter' else 'smooth') and phase['eligible_count']==(23 if proof['family']=='Filter' else 24) and phase['raw_count']==69 and len(phase['rows'])==22,'Wrong posterior/publication census')
        for i,row in enumerate(phase['rows']):compare(row,[60*(i//2)]+circle(7000,60*(i//2)),[1e-4]+[2e-5]*3+[2e-8]*3)


def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,h in manifest.items():require(not Path(n).is_symlink() and sha(n)==h,'Changed regular artifact '+n)
    def bound(p):
        p=Path(p);require(str(p) in manifest,'Unbound artifact '+str(p));return p
    def archive(p):
        with tarfile.open(bound(p)) as t:return {m.name:t.extractfile(m).read() for m in t.getmembers()}
    before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')),'Changed native inputs');frozen=archive(stage/'executed-sources-v4.tar.gz');require({n:hashlib.sha256(v).hexdigest() for n,v in frozen.items()}==before['sources'],'Executed source archive differs')
    prior=load(bound(repo/'build/nasa-coverage/coverage95-20261103-nested-ephemeris/source-v3-review.json'))['reference_sources'];require(all(hashlib.sha256(frozen[n]).hexdigest()==prior[n] for n in CORE),'Core changed after nested repair')
    old=archive(stage/'source-v1.tar.gz')['src/qtgui/tests/NasaFilterStkTests.cpp'].decode();current=frozen['src/qtgui/tests/NasaFilterStkTests.cpp'].decode();start='const double epoch0=';end='for(auto v:eligible)';require(old[old.index(start):old.index(end)]==current[current.index(start):current.index(end)],'Independent numerical oracle or bounds changed')
    require('if (theDataCoordSystem == NULL)' in frozen['src/base/subscriber/Subscriber.cpp'].decode() and 'observer.SetInternalCoordSystem(Publisher::Instance()->GetInternalCoordSystem())' in current,'Observer did not initialize through native frame contract')
    require('if (runstate == Gmat::SOLVING)' in frozen['src/base/subscriber/EphemerisFile.cpp'].decode(),'Trial output guard absent')
    launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==0 and launch['mapped_processes']==2 and launch['inputs_unchanged'],'Wrong visible launch');mapped=load(bound(stage/'loaded-images.json'))['processes'];selected=[];failures=[];last=None;raw_total=0;eligible_total=0;trial_total=0
    for mode,v,names in [('headless',1,NAMES),('headless',2,NAMES[:1]),('headless',3,NAMES),('headless',4,NAMES),('visible',1,NAMES)]:
        receipt=load(bound(repo/'build/gui-acceptance'/f'filter-stk-{mode}-v{v}'/'results.json'));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal selection');version=dict(before['sources'])
        if mode=='headless' and v<4:version.update({n:hashlib.sha256(b).hexdigest() for n,b in archive(stage/f'source-v{v}.tar.gz').items()})
        require(all(version.get(n)==h for n,h in receipt['source_hashes'].items()),'Wrong source version');active={r['name']:r for r in receipt['results'] if r['status']!='not-run'};require(set(active)==set(names),'Wrong attempted cases')
        if mode=='visible':
            d=receipt['visible_display'];require(receipt['max_screenshots']==0 and d['resizable'] and d['start_minimized'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'],'Wrong desktop policy')
        for name,row in active.items():
            passed=mode=='visible' or v==4;require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Native outcome changed');log=bound(row['log']);root=log.parent;tests=ET.parse(bound(root/'ctest.xml')).getroot().findall('testcase');require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None)==passed,'Wrong JUnit outcome')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1,'Native executable absent')
            if passed:require(before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Wrong qualified native executable')
            if mode=='visible':
                require(row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'No native exposure');traces=list(root.glob('trace-*.jsonl'));require(traces,'Missing trace')
                for trace in traces:
                    proc=mapped.get(trace.stem.removeprefix('trace-'));require(proc and proc['executable']==exes[0],'Missing mapped process')
                    for p,h in proc['images'].items():require(before['images'].get(p)==h,'Mapped image differs')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith('libGmat'):require(proc['images'].get(p)==h,'Missing mapped core/test')
            if not passed:
                error={1:'STK filter/smoother run failed',2:'STK observer failed; inspect observer-error.txt',3:'STK complete trajectory/frame metadata absent'}[v];require('FAIL '+error in log.read_text() and not (root/'proof.json').exists(),'Development failure lost')
                if v==2:require(bound(root/'0-observer-error.txt').read_text()=='STK stream coordinate context absent','Observer diagnostic differs')
                if v==3:require(len(load(bound(root/'0-rows.json')))==22,'Wrong provisional cadence failure')
                failures.append(dict(version=v,name=name,error=error));continue
            proof=load(bound(root/'proof.json'));check(proof);last=proof;records=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(records)==1 and json.loads(records[0])==proof,'Proof/stdout mismatch');family=name.removeprefix(PREFIX);fixture='src/qtgui/tests/filter-stk/'+family+'.json';spec=json.loads(frozen[fixture]);require(sha(bound(root/'fixture.json'))==hashlib.sha256(frozen[fixture]).hexdigest(),'Wrong fixture');origin=frozen[spec['origin']];require(hashlib.sha256(origin).hexdigest()==spec['origin_sha256'],'Origin changed');script=json.loads(origin)['script'].replace('BeginMissionSequence;',spec['setup']+'BeginMissionSequence;\nToggle Export Off;',1).replace('RunSimulator Sim;',"Report 'NavArm' NavPhases Keep;\nRunSimulator Sim;",1)
            script=script.replace('RunEstimator EKF;',"Report 'ForwardStart' NavPhases Keep;\n"+('Toggle Export On;\n' if family=='Filter' else '')+'RunEstimator EKF;\n'+('Toggle Export Off;\n' if family=='Filter' else ''),1).replace('RunSmoother FPS;',"Report 'SmoothStart' NavPhases Keep;\n"+('Toggle Export On;\n' if family=='Smoother' else '')+'RunSmoother FPS;\n'+('Toggle Export Off;\n' if family=='Smoother' else '')+"Report 'NavDone' NavPhases Keep;",1);require(spec['script']==script,'Fixture mission derivation changed')
            initial=bound(root/'initial.script').read_text();applied=bound(root/'applied.script').read_text();without=lambda s:re.sub(r'^Export\.WriteEphemeris = (?:false|true);\n','',s,flags=re.M);require(without(initial)==without(applied) and initial.count('Export.WriteEphemeris = false;')==applied.count('Export.WriteEphemeris = true;')==1,'GUI Apply changed unrelated bytes')
            for p in proof['phases']:
                phase=p['phase'];prefix=str(phase)+'-';require(bound(root/(prefix+'observer-error.txt')).read_text()=='','Observer failed');raw=load(bound(root/(prefix+'raw-publications.json')));require(len(raw)==69 and load(bound(root/(prefix+'rows.json')))==p['rows'],'Raw proof mismatch');raw_total+=len(raw);eligible=[r for r in raw if r['phase']==p['selected_phase'] and r['run_state']==10007];trials=[r for r in raw if r['phase']=='smooth' and r['run_state']==10006];require(len(trials)==22 and len(eligible)==p['eligible_count'],'Missing accepted/trial stream');trial_total+=len(trials);eligible_total+=len(eligible);require(all(r['provider']==('RunEstimator' if family=='Filter' else 'RunSmoother') for r in eligible),'Wrong selected provider')
                for r in eligible:
                    t=(r['epoch']-spec['a1_epoch'])*86400;compare(r['state'],circle(7000,t),[2e-5]*3+[2e-8]*3);require(any(abs(row[0]-t)<1e-4 for row in p['rows']),'Missing accepted publication epoch')
                for row in p['rows']:
                    matches=[r for r in eligible if abs((r['epoch']-spec['a1_epoch'])*86400-row[0])<1e-4];require(matches,'No selected publication for STK row');require(any(all(abs(a-b)<=tol for a,b,tol in zip(r['state'],row[1:],[2e-5]*3+[2e-8]*3)) for r in matches),'STK state lacks accepted publication')
                payload=bound(root/(prefix+'mission.e')).read_text();require('NumberOfEphemerisPoints 22' in payload and 'ScenarioEpoch           10 Jun 2010 00:00:00.000' in payload and 'CoordinateSystem        J2000' in payload and 'DistanceUnit            Kilometers' in payload,'STK metadata differs');data=payload.split('EphemerisTimePosVel\n',1)[1].split('END Ephemeris',1)[0];rows=[[float(v) for v in line.split()] for line in data.splitlines() if line.strip()];require(rows==p['rows'],'Raw STK rows/proof mismatch')
            selected.append(dict(mode=mode,name=name))
    negatives=[]
    for label,mutate in [('wrong-state',lambda p:p['phases'][0]['rows'][0].__setitem__(1,0)),('missing-row',lambda p:p['phases'][0]['rows'].pop()),('wrong-phase',lambda p:p['phases'][0].update(selected_phase='simulator')),('wrong-accepted-count',lambda p:p['phases'][0].update(eligible_count=22)),('missing-reopen',lambda p:p['phases'].pop())]:
        bad=copy.deepcopy(last);mutate(bad)
        try:check(bad)
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    return dict(status='passed-bounded-independent-filter-smoother-STK-membership',qualified_keys=[],new_gui_passes=4,native_phases=8,STK_rows=176,STK_components=1056,raw_publication_rows=raw_total,selected_accepted_publications=eligible_total,observed_smoother_trial_publications=trial_total,retained_development_failures=5,negative_controls=negatives,selected=selected,failures=failures,production_changed_files=[],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();v=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(v,indent=2)+'\n');print(json.dumps({k:x for k,x in v.items() if k not in ['selected','failures']}))
