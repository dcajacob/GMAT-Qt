"""Audit batch terminal/interruption/error OEM membership, repairs and retained failures."""
import argparse,copy,csv,datetime,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load
from AuditCommandNavigationOperations import compare
PREFIX='QtGui.NasaBatchOutputEdge'
FAMILIES=['Stop','Singular','IterationLimit','InvalidSolveFors']
NEW=[PREFIX+n for n in FAMILIES]
REG=['QtGui.NativeNasaBatchModeInitialGuess','QtGui.NativeNasaBatchModeIterationLimit','QtGui.NasaBatchEphemerisPositive']
CORE='plugins/EstimationPlugin/src/base/command/RunEstimator.cpp'


def circle(t,phase=0):
    w=math.sqrt(398600.4415/7000**3);v=math.sqrt(398600.4415/7000);a=w*t+phase;c=math.sqrt(.5)
    return [7000*math.cos(a),7000*math.sin(a)*c,7000*math.sin(a)*c,-v*math.sin(a),v*math.cos(a)*c,v*math.cos(a)*c]


def check(proof):
    require(proof['outcome']=='passed' and proof['family'] in FAMILIES and [p['phase'] for p in proof['phases']]==[0,1,2],'Wrong batch context')
    family=proof['family']
    for p in proof['phases']:
        i=p['phase'];require(p['rebuilt']==(i==0 or (i==1 and family!='Stop')),'Recovery Build policy differs')
        if i==0:
            require(not p['rows'] and p['final_publications']==0 and p['raw_count']==dict(Stop=2,Singular=25,IterationLimit=359,InvalidSolveFors=0)[family],'Trial/terminal output leaked or publication census changed')
            require(p['done']==(family=='IterationLimit') and p['end_clicked']==(family=='Stop') and p['outcome']==('Stopped' if family=='Stop' else 'Completed' if family=='IterationLimit' else 'Failed'),'Abort/continuation contract differs')
            if family=='IterationLimit':require(p['terminal_status']=='Exceed maximum number of iterations','Nonconvergence status lost')
        else:
            require(p['outcome']=='Completed' and p['done'] and not p['end_clicked'] and p['terminal_status'].startswith('Meet ') and p['raw_count']==2868 and p['final_publications']==1435 and len(p['rows'])==1435,'Converged recovery census differs')
            for j,row in enumerate(p['rows']):compare(row,[20*j]+circle(20*j),[1e-4]+[2e-5]*3+[2e-8]*3)


def oem(payload):
    base=datetime.datetime(2012,6,10);rows=[]
    for line in payload.splitlines():
        fields=line.split()
        if fields and re.match(r'^\d{4}-\d\d-\d\dT',fields[0]):rows.append([(datetime.datetime.fromisoformat(fields[0])-base).total_seconds()]+[float(v) for v in fields[1:]])
    return rows


def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v2.json')['artifact_sha256']
    for n,h in manifest.items():
        p=Path(n);require(p.is_file() and not p.is_symlink() and sha(p)==h,'Changed regular artifact '+n)
        for parent in p.parents:
            require(not parent.is_symlink(),'Symlink ancestor '+str(parent))
            if parent==repo:break
    def bound(p):
        p=Path(p);require(str(p) in manifest,'Unbound artifact '+str(p));return p
    def archive(p):
        with tarfile.open(bound(p)) as t:
            require(all(m.isfile() for m in t.getmembers()),'Nonregular archived source')
            return {m.name:t.extractfile(m).read() for m in t.getmembers()}
    before=load(bound(stage/'visible-inputs-v8-before.json'));require(before==load(bound(stage/'visible-inputs-v8-after.json')),'Visible inputs changed');frozen=archive(stage/'executed-sources-v8.tar.gz');older=load(bound(stage/'visible-inputs-before.json'));require(older==load(bound(stage/'visible-inputs-after.json')),'Older visible inputs changed')
    require({n:hashlib.sha256(v).hexdigest() for n,v in frozen.items()}==before['sources'],'Executed source archive differs')
    versions={v:archive(stage/f'source-v{v}.tar.gz') for v in range(1,9)}
    pre=bound(stage/'RunEstimator.cpp.before').read_bytes();require(pre==versions[2][CORE] and pre!=frozen[CORE],'Wrong core preimage')
    # This increment touches only the early size guard and FINISHED packet guard.
    old=pre.decode();expected=old.replace('         if (!(theEstimator->IsOfType("BatchEstimatorBase") &&\n               theEstimator->IsFinalPass()))','         if (!theEstimator->IsOfType("BatchEstimatorBase"))',1)
    expected=expected.replace('// A batch final pass has already published its terminal trajectory\n         // point. Its participants are now back at the fitted initial epoch;\n         // publishing them with the old terminal epoch would corrupt the OEM.','// A batch estimator restores its participants to the fitted initial\n         // epoch, including when it terminates without a final pass. Any\n         // accepted trajectory was already published by the final pass; do\n         // not tag restored initial conditions with the old terminal epoch.',1)
    needle='            if (ps->GetPropagator()->UsesODEModel())\n               sc->TakeAction("UseSTM");'
    replacement='''            if (ps->GetPropagator()->UsesODEModel())
            {
               // Force derivatives always include the six Cartesian rows.
               // Reject a reduced solve-for-only STM before force evaluation;
               // the estimator's later state-solve-for check is too late.
               if (sc->GetIntegerParameter("FullSTMRowCount") < 6)
                  throw CommandException("Cannot initialize RunEstimator: spacecraft \\\"" +
                        sc->GetName() + "\\\" has fewer than six Cartesian STM rows.\\n");
               sc->TakeAction("UseSTM");
            }'''
    expected=expected.replace(needle,replacement,1);require(expected.encode()==frozen[CORE],'Unexpected core delta')
    import difflib
    require(bound(stage/'core-v4.patch').read_text()==''.join(difflib.unified_diff(old.splitlines(True),expected.splitlines(True),fromfile='a/'+CORE,tofile='b/'+CORE)),'Core patch differs')
    batch='plugins/EstimationPlugin/src/base/estimator/BatchEstimator.cpp'
    batch_before=bound(stage/'BatchEstimator.cpp.before').read_bytes();require(batch_before==versions[7][batch],'Wrong inversion preimage')
    start='   else\n   {\n      try\n      {\n         reducedCovMatrix = reducedInfMatrix.Inverse();'
    end='   else\n   {\n      // Internal inversion can return finite values for a rank-deficient\n      // normal matrix. Use the same positive-definiteness check as Schur\n      // before accepting a covariance; retain the requested inversion below.\n      Rmatrix testMatrix = reducedInfMatrix;\n      CholeskyFactorization cf;\n      cf.Invert(testMatrix);\n\n      try\n      {\n         reducedCovMatrix = reducedInfMatrix.Inverse();'
    require(batch_before.decode().replace(start,end,1).encode()==frozen[batch],'Unexpected inversion delta')
    delta=''.join(difflib.unified_diff(batch_before.decode().splitlines(True),frozen[batch].decode().splitlines(True),fromfile='a/'+batch,tofile='b/'+batch))
    require(bound(stage/'core-v8.patch').read_text()==bound(stage/'core-v4.patch').read_text()+delta,'Combined core patch differs')
    fixture='src/qtgui/tests/batch-output-edges/InternalRankDeficient.json';require(versions[7][fixture]==frozen[fixture],'Original Internal rejection fixture changed')
    for n in ['src/base/spacecraft/Spacecraft.cpp','src/base/forcemodel/PointMassForce.cpp','plugins/EstimationPlugin/src/base/estimator/EstimationStateManager.cpp','plugins/EstimationPlugin/src/base/estimator/Estimator.cpp']:
        require(versions[3][n]==frozen[n],'Underlying STM/solve-for contract changed')
    require('Error: No state solvefor parameters are used in estimation.' in frozen['plugins/EstimationPlugin/src/base/estimator/EstimationStateManager.cpp'].decode(),'Native state prerequisite lost')
    driver='src/qtgui/tests/NasaBatchOutputEdgeTests.cpp';oracle=lambda b:b[b.index(b'static std::array<double,6> advance'):b.index(b'int main(')]
    require(oracle(versions[1][driver])==oracle(frozen[driver]),'Original RK4 oracle changed')
    v2=versions[2][driver].decode().replace('mode=="Singular"?MainWindow::RunResult::Failed','(mode=="Singular"||mode=="InvalidSolveFors")?MainWindow::RunResult::Failed').replace('require(!raw.isEmpty(),"Batch edge failed before trial propagation");','require(!raw.isEmpty()||(!phase&&mode=="InvalidSolveFors"),"Batch edge failed before trial propagation");').replace('else require(rig.messages->toPlainText().contains("matrix",Qt::CaseInsensitive),"Singular batch matrix diagnostic absent");','else if(mode=="InvalidSolveFors")require(raw.isEmpty()&&rig.messages->toPlainText().contains("fewer than six Cartesian STM rows"),"Invalid batch STM guard diagnostic absent");else require(rig.messages->toPlainText().contains("matrix",Qt::CaseInsensitive),"Singular batch matrix diagnostic absent");')
    require(v2.encode()==frozen[driver],'Original packet failure assertion or numerical bounds changed')
    require(versions[5][driver]==versions[6][driver]==versions[7][driver]==frozen[driver],'Driver changed to invert Internal failure')
    launch=load(bound(stage/'visible-launch-v2.json'));require(launch['returncode']==0 and launch['mapped_processes']==8 and launch['inputs_unchanged'],'Wrong visible launch');mapped_new=load(bound(stage/'loaded-images-v2.json'))['processes'];mapped_old=load(bound(stage/'loaded-images.json'))['processes'];prior_launch=load(bound(stage/'visible-launch.json'));require(prior_launch['returncode']==0 and prior_launch['mapped_processes']==7 and prior_launch['inputs_unchanged'],'Wrong earlier visible launch')
    old_images=load(bound(stage/'before-core-images.json'));require(any(Path(n).name.startswith('libGmatEstimation.') and old_images.get(n)!=h for n,h in before['images'].items()),'Estimation module not rebuilt')
    require('-O0 -g -fno-omit-frame-pointer' in load(bound(stage/'debug-ninja-block.json'))['diagnostic'] and 'CACHE_SIZE=7 FORCE=0' in bound(stage/'gdb-size.log').read_text() and 'double free or corruption' in bound(stage/'gdb-size.log').read_text(),'Original heap diagnostic lost')
    selected=[];failures=[];last=None;knots=raw_total=final_total=0;prior_passes=0
    selections=[('headless',1,NEW[:3]),('headless',2,NEW[1:3]),('headless',3,[PREFIX+'Singular']),('headless',4,REG+NEW),('headless',5,[PREFIX+'Singular']),('headless',6,[PREFIX+'Singular']),('headless',7,[PREFIX+'InternalRankDeficient']),('visible',1,REG+NEW),('headless',8,REG+NEW+[PREFIX+'InternalRankDeficient']),('visible',2,REG+NEW+[PREFIX+'InternalRankDeficient'])]
    for mode,v,names in selections:
        receipt=load(bound(repo/'build/gui-acceptance'/f'batch-output-edges-{mode}-v{v}'/'results.json'));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal selection')
        source_version=dict(older['sources'] if (mode=='visible' and v==1) or (mode=='headless' and v<8) else before['sources']);inputs=older if (mode=='visible' and v==1) or (mode=='headless' and v<8) else before;mapped=mapped_old if v==1 else mapped_new;primary=(mode=='headless' and v==8) or (mode=='visible' and v==2)
        if mode=='headless':source_version.update({n:hashlib.sha256(b).hexdigest() for n,b in versions[v].items()})
        require(all(source_version.get(n)==h for n,h in receipt['source_hashes'].items()),'Wrong executed source version')
        active={r['name']:r for r in receipt['results'] if r['status']!='not-run'};require(set(active)==set(names),'Wrong attempted census')
        if mode=='visible':
            d=receipt['visible_display'];require(receipt['max_screenshots']==0 and d['resizable'] and d['start_minimized'] and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'],'Wrong desktop policy')
        for name,row in active.items():
            passed=(mode=='visible' or (v==1 and name==PREFIX+'Stop') or (v==4 and name!=PREFIX+'Singular') or v in [6,8])
            require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Outcome changed')
            log=bound(row['log']);root=log.parent;tests=ET.parse(bound(root/'ctest.xml')).getroot().findall('testcase');require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None)==passed,'JUnit differs')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1,'Executable absent')
            if passed and (mode=='visible' or v>=4):require(inputs['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'Qualified executable differs')
            if mode=='visible':
                require(row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Missing exposure');traces=list(root.glob('trace-*.jsonl'));require(traces,'Missing trace')
                for t in traces:
                    proc=mapped.get(t.stem.removeprefix('trace-'));require(proc and proc['executable']==exes[0],'Missing mapped process')
                    for p,h in proc['images'].items():require(inputs['images'].get(p)==h,'Mapped image differs')
                    for p,h in inputs['images'].items():
                        if p==exes[0] or Path(p).name.startswith('libGmat'):require(proc['images'].get(p)==h,'Unmapped core/test')
            if not passed:
                error='Batch edge native outcome differs' if v in [1,5,7] and name.endswith(('Singular','InternalRankDeficient')) else 'Batch edge final trajectory missing' if v==1 else 'double free or corruption' if v==2 and name.endswith('Singular') else 'Batch without final pass emitted a mis-timed fitted-state packet' if v==2 else 'Batch edge failed before trial propagation'
                require(error in log.read_text() and not (root/'proof.json').exists(),'Original failure lost')
                if v==2 and name.endswith('IterationLimit'):
                    rows=oem(bound(root/'0-mission.oem').read_text());require(len(rows)==1 and rows[0][0]==28680,'Mistimed packet missing');spec=json.loads(versions[2]['src/qtgui/tests/batch-output-edges/IterationLimit.json']);compare(rows[0][1:],spec['reference_one_step'],[2e-5]*3+[2e-8]*3)
                if v in [5,7]:require('Final Covariance Matrix:' in bound(root/'0-messages.txt').read_text() and '-nan' in bound(root/'0-messages.txt').read_text(),'Invalid covariance conflict lost')
                if v in [3,4]:require('No observation data is used for estimation' in bound(root/'0-messages.txt').read_text(),'Provisional empty-observation failure lost')
                failures.append(dict(mode=mode,version=v,name=name,error=error));continue
            if not primary:prior_passes+=1
            proof=load(bound(root/'proof.json'));records=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(records)==1 and json.loads(records[0])==proof and proof['outcome']=='passed','Proof/stdout mismatch')
            if name in REG:
                if 'BatchMode' in name:
                    require(proof['invocations']==1 and proof['rows']==1,'Wrong mode invocation count');states=list(csv.reader(bound(root/'states.csv').open()));require(len(states)==1,'Mode raw row count differs');values=[float(v) for v in states[0]];require(len(values)==9 and values[0]==1 and values[-1]==9,'Mode sentinel lost')
                    reference=json.loads(frozen['src/qtgui/tests/NasaBatchIterationCases.json'])
                    want=reference['one_step_reference']['expected_state'] if name.endswith('IterationLimit') else circle((values[1]-reference['a1_epoch'])*86400,.001)
                    compare(values[2:8],want,[2e-5]*3+[2e-8]*3)
                else:require(proof['points']==1435 and proof['convergence_status'].startswith('Meet '),'Converged OEM regression differs')
                
                if primary:selected.append(dict(mode=mode,name=name))
                continue
            check(proof)
            if not primary:continue # Prior source-versioned passes are retained without double counting.
            last=proof;family=proof['family'];fixture='src/qtgui/tests/batch-output-edges/'+name.removeprefix(PREFIX)+'.json';spec=json.loads(frozen[fixture]);require(sha(bound(root/'fixture.json'))==hashlib.sha256(frozen[fixture]).hexdigest(),'Fixture differs');require(sha(bound(repo/spec['origin']))==spec['origin_sha256'] and hashlib.sha256(frozen[spec['reference_source']]).hexdigest()==spec['reference_sha256'],'Inherited origin/reference differs')
            reference=json.loads(frozen[spec['reference_source']]);require(spec['reference_one_step']==reference['one_step_reference']['expected_state'] and spec['mu']==398600.4415,'Independent reference changed')
            initial=bound(root/'initial.script').read_text();applied=bound(root/'applied.script').read_text();without=lambda s:re.sub(r'^Export\.WriteEphemeris = (?:false|true);\n','',s,flags=re.M);require(without(initial)==without(applied) and initial.count('Export.WriteEphemeris = false;')==applied.count('Export.WriteEphemeris = true;')==1,'Apply changed unrelated bytes')
            for p in proof['phases']:
                i=p['phase'];prefix=str(i)+'-';require(load(bound(root/(prefix+'phase.json')))==p and bound(root/(prefix+'observer-error.txt')).read_text()=='','Phase/observer differs');raw=load(bound(root/(prefix+'raw-publications.json')));require(len(raw)==p['raw_count'],'Raw census differs');final=[r for r in raw if r['final_pass']];require(len(final)==p['final_publications'] and all(r['run_state']==(10007 if r['final_pass'] else 10006) for r in raw),'Native final/trial partition differs');raw_total+=len(raw);final_total+=len(final)
                rows=oem(bound(root/(prefix+'mission.oem')).read_text());require(rows==p['rows'],'Raw OEM/proof mismatch');knots+=len(rows)
                if i:
                    payload=bound(root/(prefix+'mission.oem')).read_text();require(payload.count('META_START')==1 and all(re.search(r'^'+k+r'\s*=\s*'+v+r'\s*$',payload,re.M|re.I) for k,v in [('TIME_SYSTEM','UTC'),('REF_FRAME','EME2000'),('CENTER_NAME','EARTH')]),'OEM segment/frame metadata differs')
                    require(len(final)==len(rows),'Accepted epoch missing from output')
                    for r,out in zip(final,rows):
                        require(r['native_state']==10009,'Not a native final propagation');compare([(r['epoch']-spec['a1_epoch'])*86400]+r['state'],out,[1e-4]+[2e-5]*3+[2e-8]*3)
                elif family=='IterationLimit':
                    values=list(csv.reader(bound(root/'0-states.csv').open()))[-1];require(len(values)==9 and float(values[-1])==9,'One-step report sentinel differs');compare([float(x) for x in values[2:8]],spec['reference_one_step'],[2e-5]*3+[2e-8]*3);require(sha(bound(root/'0-observations.gmd'))==reference['one_step_reference']['inputs']['src/qtgui/tests/batch-one-step/observations.gmd'],'One-step observations differ')
                elif family=='Singular':require('Matrix must be positive definite for Cholesky decomposition.' in bound(root/'0-messages.txt').read_text(),'Native singular error differs')
                elif family=='InvalidSolveFors':require('fewer than six Cartesian STM rows' in bound(root/'0-messages.txt').read_text(),'Early invalid STM guard absent')
            selected.append(dict(mode=mode,name=name))
    negatives=[]
    for label,mutate in [('trial-output',lambda p:p['phases'][0]['rows'].append([0]+circle(0))),('wrong-state',lambda p:p['phases'][1]['rows'][0].__setitem__(1,0)),('missing-knot',lambda p:p['phases'][1]['rows'].pop()),('lost-recovery',lambda p:p['phases'].pop()),('wrong-terminal',lambda p:p['phases'][1].update(terminal_status='Exceed maximum number of iterations'))]:
        bad=copy.deepcopy(last);mutate(bad)
        try:check(bad)
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    require((knots,raw_total,final_total)==(28700,58182,28700) and len(selected)==16 and prior_passes==15,'Aggregate evidence census differs')
    release=load(bound(stage/'release-build-identity.json'));require(release['images']==before['images'] and all('-O3' in b and '-O0' not in b for b in release['blocks'].values()) and release['source_review_sha256']==sha(bound(stage/'source-v8-review.json')),'Normal Release identity differs')
    return dict(status='passed-bounded-batch-output-edges-and-three-core-repairs',qualified_keys=[],new_gui_passes=10,affected_regression_passes=6,native_edge_phases=30,OEM_rows=knots,OEM_components=6*knots,raw_publication_rows=raw_total,accepted_final_publications=final_total,retained_failed_attempts=len(failures),negative_controls=negatives,selected=selected,failures=failures,production_changed_files=[CORE,batch],retained_prior_passes=prior_passes,all_three_original_core_failures_repaired=True,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();v=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(v,indent=2)+'\n');print(json.dumps({k:x for k,x in v.items() if k not in ['selected','failures']}))
