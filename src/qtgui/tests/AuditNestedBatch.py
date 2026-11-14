"""Audit nested batch publication membership, simulator reentry and preserved failures."""
import argparse,copy,csv,difflib,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load
from AuditCommandNavigationOperations import compare
from AuditBatchOutputEdges import oem
from AuditFilterStk import check as check_filter
PREFIX='QtGui.NasaNestedBatch';FAMILIES=['TargetDC','OptimizeYukon','OptimizeVF13ad'];NEW=[PREFIX+x for x in FAMILIES];REG=['QtGui.NativeNasaBatchModeReentry','QtGui.NasaFilterStkFilter'];CORE='plugins/EstimationPlugin/src/base/command/RunSimulator.cpp'


def circle(radius,t):
    w=math.sqrt(398600.4415/radius**3);v=math.sqrt(398600.4415/radius);a=w*t;c=math.sqrt(.5)
    return [radius*math.cos(a),radius*math.sin(a)*c,radius*math.sin(a)*c,-v*math.sin(a),v*math.cos(a)*c,v*math.cos(a)*c]


def check(p):
    require(p['outcome']=='passed' and p['family'] in FAMILIES and [v['phase'] for v in p['phases']]==[0,1],'Wrong nested context/replay')
    for phase in p['phases']:
        require(phase['rebuild']==(phase['phase']==0) and phase['accepted_passes']==1 and phase['local_final_outer_trials']>=2 and len(phase['rows'])==1441,'Missing local/global partition or final grid')
        passes=phase['passes'];require(len(passes)==dict(TargetDC=6,OptimizeYukon=17,OptimizeVF13ad=13)[p['family']],'Outer pass census differs');accepted=[]
        for i,v in enumerate(passes,1):
            require(v['pass']==i and v['batch_status'].startswith('Meet ') and v['accepted']==(v['outer_state']==10014),'Native acceptance/convergence differs');compare([v['radius']],[7000+100*v['U']],[1e-9])
            if v['accepted']:accepted.append(i)
        require(accepted==[len(passes)] and len(passes)-1==phase['local_final_outer_trials'] and min(v['radius'] for v in passes)<7001 and max(v['radius'] for v in passes)>7199,'Distinguishable outer trial/final regimes absent')
        require(abs(passes[-1]['U']-2)<1e-7,'Independent outer solution differs')
        for i,row in enumerate(phase['rows']):compare(row,[20*i]+circle(7200,20*i),[1e-4]+[2e-5]*3+[2e-8]*3)


def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,h in manifest.items():
        p=Path(n);require(p.is_file() and not p.is_symlink() and sha(p)==h,'Changed regular artifact '+n)
        for parent in p.parents:
            require(not parent.is_symlink(),'Symlink ancestor')
            if parent==repo:break
    def bound(p):
        p=Path(p);require(str(p) in manifest,'Unbound artifact '+str(p));return p
    def archive(p):
        with tarfile.open(bound(p)) as t:
            require(all(m.isfile() for m in t.getmembers()),'Nonregular archived source');return {m.name:t.extractfile(m).read() for m in t.getmembers()}
    versions={v:archive(stage/f'source-v{v}.tar.gz') for v in range(1,10)};frozen=archive(stage/'executed-sources-v9.tar.gz');before=load(bound(stage/'visible-inputs-v9-before.json'));require(before==load(bound(stage/'visible-inputs-v9-after.json')) and {n:hashlib.sha256(b).hexdigest() for n,b in frozen.items()}==before['sources'],'Executed source/visible input identity differs')
    for name in ['AuditBatchOutputEdges.py','AuditFilterStk.py','BuildNasaCoverageExtension.py','AuditCommandNavigationOperations.py']:
        key='src/qtgui/tests/'+name;require(sha(repo/key)==hashlib.sha256(frozen[key]).hexdigest(),'Audit dependency changed '+key)
    original=bound(stage/'RunSimulator.cpp.before').read_text();needle='''   // Reset the command if called after it has completed execution
   // todo: Debug this piece; reentrance in a For loop doesn't work yet
//   if (commandComplete)
//      TakeAction("Reset");''';replacement='''   // A containing loop can invoke this command again after FINISHED. Rebuild
   // the command-local simulator and propagators before resetting its state;
   // resetting only the state machine retains stale measurement adapters.
   if (commandComplete)
   {
      if (!Initialize())
         throw CommandException("Cannot reinitialize RunSimulator on reentry.");
      TakeAction("Reset");
   }''';require(original.count(needle)==1 and original.replace(needle,replacement,1).encode()==frozen[CORE],'Unexpected core delta');require(bound(stage/'core-v5.patch').read_text()==''.join(difflib.unified_diff(original.splitlines(True),frozen[CORE].decode().splitlines(True),fromfile='a/'+CORE,tofile='b/'+CORE)),'Core patch differs')
    driver='src/qtgui/tests/NasaNestedBatchTests.cpp';require(versions[3][driver].replace(b'rows.size()==1435',b'rows.size()==1441')==frozen[driver] and versions[5][driver]==versions[3][driver],'Numerical oracle/bounds or original failure assertions changed')
    require(versions[6][driver]==versions[7][driver]==versions[8][driver]==versions[9][driver],'VF failure assertion changed')
    earlier=load(bound(repo/'build/nasa-coverage/coverage95-20261103-batch-output-edges/source-v8-review.json'))['reference_sources']
    for n in ['plugins/EstimationPlugin/src/base/command/RunEstimator.cpp','plugins/EstimationPlugin/src/base/estimator/BatchEstimator.cpp','src/base/subscriber/EphemerisFile.cpp','src/base/command/SolverBranchCommand.cpp']:require(hashlib.sha256(frozen[n]).hexdigest()==earlier[n],'Earlier core repair changed')
    vf='src/qtgui/tests/nested-batch/OptimizeVF13ad.json';unc='src/qtgui/tests/nested-batch/OptimizeVF13adUnconstrained.json';require(json.loads(frozen[unc])['script']==json.loads(versions[6][vf])['script'],'Original unconstrained profile changed');require('NonlinearConstraint Outer(U >= 2);' in json.loads(frozen[vf])['script'],'Explicit constrained profile absent')
    require('MeasurementManager::CalculateMeasurements' in bound(stage/'gdb-v4.log').read_text() and 'SIGSEGV' in bound(stage/'gdb-v4.log').read_text(),'Partial-repair crash diagnosis lost')
    launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==0 and launch['mapped_processes']==5 and launch['inputs_unchanged'],'Wrong visible launch');mapped=load(bound(stage/'loaded-images.json'))['processes'];old_images=load(bound(stage/'before-core-images.json'));require(any(Path(n).name.startswith('libGmatEstimation.') and old_images.get(n)!=h for n,h in before['images'].items()),'Estimation module not rebuilt')
    failures=[];selected=[];last=None;census=None;raw_total=knots=trial_final=trial_streams=0
    selections=[('headless',2,NEW),('headless',3,NEW),('headless',4,NEW+REG),('headless',5,NEW[:1]),('headless',6,NEW),('headless',7,[NEW[2]]),('headless',8,[NEW[2]]),('headless',9,[PREFIX+'OptimizeVF13adUnconstrained']),('visible',1,NEW+REG)]
    for mode,v,names in selections:
        receipt=load(bound(repo/'build/gui-acceptance'/f'nested-batch-{mode}-v{v}'/'results.json'));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal selection');expected=dict(before['sources'])
        if mode=='headless':expected.update({n:hashlib.sha256(b).hexdigest() for n,b in versions[v].items()})
        require(all(expected.get(n)==h for n,h in receipt['source_hashes'].items()),'Wrong source version');active={x['name']:x for x in receipt['results'] if x['status']!='not-run'};require(set(active)==set(names),'Attempted case census differs')
        if mode=='visible':
            d=receipt['visible_display'];require(receipt['max_screenshots']==0 and d['resizable'] and d['start_minimized'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'],'Wrong visible policy')
        for name,row in active.items():
            passed=mode=='visible' or (v==4 and name in REG) or (v==6 and name in NEW[:2]) or v==8
            require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Native result changed');log=bound(row['log']);root=log.parent;tests=ET.parse(bound(root/'ctest.xml')).getroot().findall('testcase');require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None)==passed,'JUnit differs')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1,'Missing executable identity')
            if passed:require(before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Qualified executable differs')
            if mode=='visible':
                require(row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'No native exposure');traces=list(root.glob('trace-*.jsonl'));require(traces,'Missing trace')
                for trace in traces:
                    proc=mapped.get(trace.stem.removeprefix('trace-'));require(proc and proc['executable']==exes[0],'Missing mapped process')
                    for p,h in proc['images'].items():require(before['images'].get(p)==h,'Mapped image differs')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith(('libGmat','libVF13ad')):require(proc['images'].get(p)==h,'Unmapped core/provider/test')
            if not passed:
                error='WriteEphemeris' if v==2 else 'Nested batch mission failed' if v==3 else 'Segmentation fault' if v==4 else 'Nested batch output includes trial trajectories or lost final knots' if v==5 else 'Nested batch output differs from independent final orbit';require(error in log.read_text() and not (root/'proof.json').exists(),'Original failure lost')
                if v==3:require('0 steps taken' in bound(root/'0-messages.txt').read_text() and 'only 1 valid observable records' in bound(root/'0-messages.txt').read_text(),'Stale simulator data failure lost')
                if v==5:require(len(oem(bound(root/'0-mission.oem').read_text()))==1441,'Complete native final grid lost')
                if v in [6,7,9]:
                    p=load(bound(root/'0-passes.json'))[-1];require(p['accepted'] and abs(p['U']-2)>(1e-5 if v in [6,9] else 1e-9),'Original unconstrained accuracy discrepancy absent')
                failures.append(dict(mode=mode,version=v,name=name,error=error));continue
            proof=load(bound(root/'proof.json'));records=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(records)==1 and json.loads(records[0])==proof and proof['outcome']=='passed','Proof/stdout differs')
            if name in REG:
                if name.endswith('Reentry'):
                    require(proof['invocations']==proof['rows']==2 and proof['numeric_sentinel']==9,'Reentry regression differs');rows=list(csv.reader(bound(root/'states.csv').open()));require(len(rows)==2,'Reentry rows absent')
                    for i,r in enumerate(rows,1):require(float(r[0])==i and float(r[-1])==9,'Reentry sentinel differs');compare([float(x) for x in r[2:8]],circle(7000,0),[2e-5]*3+[2e-8]*3)
                else:check_filter(proof)
                selected.append(dict(mode=mode,name=name));continue
            check(proof);last=proof;family=name.removeprefix(PREFIX);fixture='src/qtgui/tests/nested-batch/'+family+'.json';spec=json.loads(frozen[fixture]);require(sha(bound(root/'fixture.json'))==hashlib.sha256(frozen[fixture]).hexdigest() and hashlib.sha256(frozen[spec['origin']]).hexdigest()==spec['origin_sha256'],'Fixture/origin identity differs')
            c=load(bound(root/'runtime-census.json'));require(set(c['solvers'])=={'DifferentialCorrector','ExtendedKalmanFilter','Simulator','BatchEstimator','BatchEstimatorInv','Yukon','VF13ad'} and c['smoothers']==['Smoother'] and set(c['ephemeris_formats'])=={'CCSDS-OEM','SPK','CK','Code-500','STK-TimePosVel'},'Live factory census differs');require(census is None or c==census,'Inconsistent census');census=c
            initial=bound(root/'initial.script').read_text();applied=bound(root/'applied.script').read_text();without=lambda s:re.sub(r'^Export\.WriteEphemeris = (?:false|true);\n','',s,flags=re.M);require(without(initial)==without(applied) and initial.count('Export.WriteEphemeris = false;')==applied.count('Export.WriteEphemeris = true;')==1,'Apply changed unrelated source')
            for p in proof['phases']:
                i=p['phase'];prefix=str(i)+'-';require(bound(root/(prefix+'source.script')).read_text()==applied and load(bound(root/(prefix+'phase.json')))==p and load(bound(root/(prefix+'passes.json')))==p['passes'] and bound(root/(prefix+'observer-error.txt')).read_text()=='','Source/phase/observer differs');raw=load(bound(root/(prefix+'raw-publications.json')));require(len(raw)==p['raw_count'],'Raw count differs');raw_total+=len(raw);require(oem(bound(root/(prefix+'mission.oem')).read_text())==p['rows'],'Raw OEM/proof differs');knots+=len(p['rows']);final=[]
                for record in raw:
                    q=p['passes'][record['pass']-1];require(record['outer_state']==q['outer_state'] and record['global_accepted']==q['accepted'] and record['run_state']==(10007 if record['final_pass'] else 10006),'Local/global/native partition differs')
                    if record['final_pass']:
                        require(record['native_state']==10009,'Not a native batch final propagation');t=(record['epoch']-spec['a1_epoch'])*86400;require(abs(t/20-round(t/20))<1e-5 and -.0001<=t<=28800.0001,'Final grid clock differs');compare(record['state'],circle(q['radius'],t),[2e-5]*3+[2e-8]*3)
                        if record['global_accepted']:final.append(record)
                        else:trial_final+=1
                require(len(final)==1441,'Accepted publication epochs missing from OEM')
                for record,row in zip(final,p['rows']):compare([(record['epoch']-spec['a1_epoch'])*86400]+record['state'],row,[1e-4]+[2e-5]*3+[2e-8]*3)
                trial_streams+=p['local_final_outer_trials'];data=bound(root/(prefix+'mission.oem')).read_text();require(data.count('META_START')==1 and 'REF_FRAME            = EME2000' in data and 'TIME_SYSTEM          = UTC' in data,'OEM metadata differs')
                observations=[line.split() for line in bound(root/(prefix+'Measurements Δ.gmd')).read_text().splitlines() if line.strip() and not line.startswith('%')];require(len(observations)==431 and abs((max(float(x[0]) for x in observations)-26088.50039351852)*86400-28800)<1e-4,'Regenerated final observations do not cover full arc')
            selected.append(dict(mode=mode,name=name))
    negatives=[]
    for label,mutate in [('wrong-state',lambda p:p['phases'][0]['rows'][0].__setitem__(1,0)),('missing-knot',lambda p:p['phases'][0]['rows'].pop()),('trial-accepted',lambda p:p['phases'][0]['passes'][0].update(accepted=True)),('lost-replay',lambda p:p['phases'].pop()),('wrong-radius',lambda p:p['phases'][0]['passes'][0].update(radius=7200))]:
        bad=copy.deepcopy(last);mutate(bad)
        try:check(bad)
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    require(knots==17292 and raw_total==440136 and trial_streams==132 and len(selected)==10 and len(failures)==13,'Aggregate census differs')
    return dict(status='passed-bounded-nested-batch-membership-and-simulator-reentry-repair;unconstrained-VF-accuracy-open',qualified_keys=[],new_gui_passes=6,affected_regression_passes=4,native_phases=12,OEM_rows=knots,OEM_components=6*knots,raw_publication_rows=raw_total,local_final_outer_trial_streams=trial_streams,local_final_outer_trial_publications=trial_final,retained_native_failed_attempts=len(failures),retained_compile_failures=1,runtime_census=census,selected=selected,failures=failures,negative_controls=negatives,production_changed_files=[CORE],open_gate=PREFIX+'OptimizeVF13adUnconstrained',scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();v=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(v,indent=2)+'\n');print(json.dumps({k:x for k,x in v.items() if k not in ['selected','failures']}))
