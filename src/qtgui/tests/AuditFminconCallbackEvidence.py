"""Audit real Qt/fmincon constituents and affected regressions; admit no rows."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
import AuditMatlabRowAcceptance as io
from BuildNasaCoverageExtension import load, require, sha

SOLVE_PREFIX='QtGui.NasaFminconSolve'
WRITER_PREFIX='QtGui.NasaMatlabLibraryWriter'
PROTOCOL_CASES=['malformed-json','wrong-root','missing-identity','invalid-identity','wrong-session',
                'script-operation','unexpected-fields','replayed-identity','idle-evaluation','oversized-frame']


def solve(proof, profile, directory=None):
    require(proof['checkpoint']=='real-Qt-fmincon-solve' and proof['profile']==profile
            and proof['new_requirement_credit']==0,'Wrong real optimizer identity/automatic credit')
    scalar=profile=='BoundedScalar'
    expected=dict(Alpha=3 if scalar else 1.5,Beta=0 if scalar else -1.5,
                  Cost=1 if scalar else .5,Constraint=2 if scalar else 0,Keep=41)
    require([p['phase']for p in proof['phases']]==['initial','no-explicit-build-repeat','reopened'],
            'Missing real optimizer repeated/reopened missions')
    total=0
    for phase in proof['phases']:
        native=phase['native']
        require(set(native)==set(expected)|{'Calls'},'Native optimizer field set differs')
        for name,value in expected.items():
            tolerance=1e-8 if name=='Cost'else 1e-6
            require(math.isfinite(native[name]) and (native[name]==value if name=='Keep'else abs(native[name]-value)<tolerance),
                    'Independent analytic optimizer result differs: '+name)
        require(phase['actual_MATLAB_X']==[native['Alpha']] if scalar else
                len(phase['actual_MATLAB_X'])==2 and all(abs(a-native[k])<1e-12 for a,k in zip(phase['actual_MATLAB_X'],['Alpha','Beta'])),
                'Actual MATLAB/native converged controls differ')
        require(abs(phase['actual_MATLAB_fVal']-expected['Cost'])<1e-8
                and phase['actual_MATLAB_exitFlag']>0 and phase['actual_ReportViewer']is True
                and phase['Qt_heartbeat_ticks']>=3,'Actual optimizer convergence/viewer/UI evidence missing')
        require(phase['evaluations']==native['Calls']>=3,'Native optimizer evaluation census differs')
        total+=phase['evaluations']
        if directory:
            name=phase['phase']
            require(load(directory/(name+'-native.json'))==native,'Raw optimizer native state differs')
            rows=(directory/(name+'-evaluations.csv')).read_text().strip().splitlines()
            require(len(rows)==phase['evaluations'],'Raw optimizer branch census differs')
            for index,line in enumerate(rows,1):
                values=[float(x)for x in line.split(',')]
                require(len(values)==5 and all(math.isfinite(x)for x in values),'Invalid raw optimizer evaluation row')
                a,b,c,con,count=values
                cost=(a-4)**2 if scalar else (a-2)**2+(b+1)**2
                constraint=a-1 if scalar else a+b
                require(abs(c-cost)<1e-10 and abs(con-constraint)<1e-12 and count==index,
                        'Independent raw objective/constraint/evaluation sequence differs')
            report=[float(x)for x in(directory/(name+'-report.csv')).read_text().strip().split(',')]
            require(len(report)==6 and all(abs(a-native[k])<1e-12 for a,k in zip(report,['Alpha','Beta','Cost','Constraint','Calls','Keep'])),
                    'Raw final optimizer report differs from native state')
    if profile=='StopRecovery':
        stopped=proof['stop_recovery']
        require(stopped['actual_Stop_action_requests']==1 and stopped['reported_stopped']is True
                and stopped['Engine_recovered']is True and stopped['rejected_statement_absent']is True
                and 'already active'in stopped['reentrant_error'],'Stop/reentrant rejection/same-window recovery missing')
        if directory:
            require(load(directory/'stop-recovery.json')==stopped
                    and 'GMAT execution stopped by user.'in(directory/'stopped-messages.txt').read_text(),
                    'Actual stopped-run diagnostic differs')
    return dict(solves=3,GMAT_branch_evaluations=total,intentional_Stops=int(profile=='StopRecovery'))


def writer(proof, profile, directory=None):
    count=2 if profile=='Records'else 1
    expected={'shape':[count,1]}
    for row in range(count):
        scale=row+1
        expected['record'+str(row)]=dict(numbers=[[scale,2*scale,3*scale],[4*scale,5*scale,6*scale]],
            labels=['row'+str(row)+'-A','row'+str(row)+'-B']if profile=='Records'else['row0-A'])
    require(proof['checkpoint']=='actual-MATLAB-library-writer' and proof['profile']==profile
            and proof['records']==count and proof['expected']==proof['actual']==expected
            and proof['new_requirement_credit']==0,'Independent MATLAB-loaded writer data differs')
    if directory:
        require(load(directory/'actual.json')==load(directory/'expected.json')==expected
                and sha(directory/'records.mat')==proof['mat_file_sha256'],'Actual MAT-file/loaded JSON differs')
        require((directory/'records.mat').read_bytes().startswith(b'MATLAB'),'Actual MAT-file format header absent')


def protocol(proof, directory=None):
    require(proof['checkpoint']=='actual-Qt-fmincon-protocol' and proof['new_requirement_credit']==0
            and proof['caller_unchanged']is True,'Wrong actual protocol/control identity')
    rows=proof['rejections']
    require([r['case']for r in rows]==PROTOCOL_CASES and all(r['response']['ok']is False and r['response']['error']for r in rows),
            'Protocol rejection conjunction missing')
    require(rows[-1]['request_bytes']==4*1024*1024+1,'Oversized frame control missing')
    fragments=['Invalid callback JSON','Invalid callback JSON','Missing callback request identity',
        'Invalid or replayed callback identity','Invalid callback session','Unsupported callback operation',
        'Invalid callback session','Invalid or replayed callback identity','No active uninterrupted GMAT optimizer mission',
        'Callback request exceeds frame capacity']
    require(all(s in r['response']['error']for r,s in zip(rows,fragments)),'Different protocol rejection rule')
    if directory:require(load(directory/'rejections.json')==rows,'Raw protocol responses differ')


def audit(repo, stage):
    before=repo/'build/nasa-coverage/coverage95-20261103-matlab-mission-call'
    ledger=load(before/'coverage-snapshot-v1.json');overlay=load(before/'exact-row-closure-overlay-v1.json')
    require(ledger['coverage']['requirements']['verified']==1022 and ledger['coverage']['requirements']['total']==1158
            and ledger['coverage']['historical_cases']['verified']==1816 and ledger['coverage']['historical_cases']['total']==2367
            and ledger['requirements_still_needed']==79 and ledger['authority_sha256']==nasa.inventory(repo)['sources'],
            'Canonical requirement union/authority changed')
    require(len(overlay['original_hold_keys_retained'])==59 and overlay['unresolved_original_hold_count']==56
            and overlay['unresolved_original_MATLAB_count']==34,'Original hold history changed')
    archives={};runs={};contexts=[];solves=[];writers=[];protocols=[];io_counts=dict(missions=0,MATLAB_calls=0,inside_GMAT_functions=0)
    def archive(version):
        if version not in archives:
            identities=load(stage/f'executed-source-sha256-v{version}.json')
            with tarfile.open(stage/f'executed-sources-v{version}.tar.gz')as tar:
                require(set(tar.getnames())==set(identities),'Source partition differs')
                data={p:tar.extractfile(p).read()for p in identities}
                require(all(hashlib.sha256(data[p]).hexdigest()==h for p,h in identities.items()),'Frozen source differs')
            archives[version]=data
        return archives[version]
    def context(name,version,mode,test,role,argument=None):
        if name not in runs:
            receipt=load(stage/name/'results.json');mapped=load(stage/(name+'-loaded-images.json'))
            require(receipt['mode']==mode and receipt.get('finished_utc')and receipt['end_reason']=='completed-selection'
                    and mapped['returncode']==0 and mapped['inputs_unchanged']is True and mapped['runtime_binary_copies']==0,
                    'Unfinished/failed/mutated selected run')
            frozen=archive(version)
            require(mapped['source_sha256']=={p:hashlib.sha256(b).hexdigest()for p,b in frozen.items()}
                    and all(mapped['source_sha256'].get(p)==h for p,h in receipt['source_hashes'].items()),'Runtime source custody incomplete')
            require(receipt['startup_sha256']==load(stage/'generated-startup-delta-v1.json')['after_sha256'],
                    'Different generated startup configuration')
            union={}
            for process in mapped['processes']:
                for snap in process['snapshots']:
                    require(hashlib.sha256(snap['maps'].encode()).hexdigest()==snap['maps_sha256'],'Raw process map differs')
                    paths=set()
                    for line in snap['maps'].splitlines():
                        parts=line.split(None,5)
                        if len(parts)==6 and(parts[5].startswith(str(repo)+'/')or parts[5].startswith('/usr/local/MATLAB/R2026b/')):paths.add(parts[5])
                    require(paths==set(snap['project_images']),'Mapped project/SDK partition differs');union.update(snap['project_images'])
            require(union==mapped['loaded_project_images'],'Process-image union differs')
            if mode=='visible':
                d=receipt['visible_display'];require(receipt['max_screenshots']==0 and d['resizable']and d['start_minimized']
                    and'Iconic'in d['initial_host_window_state']and'_NET_WM_STATE_FULLSCREEN'not in d['initial_host_window_state'],
                    'Visible minimized/resizable/nonfullscreen policy missing')
            runs[name]=receipt,mapped
        receipt,mapped=runs[name];result=next(r for r in receipt['results']if r['name']==test)
        require(result['status']=='passed'and result['returncode']==0,'Selected case is not a native pass')
        directory=Path(result['log']).parent;xml=list(ET.parse(directory/'ctest.xml').getroot().iter('testcase'))
        require(len(xml)==1 and xml[0].get('name')==test and xml[0].find('failure')is None and xml[0].find('skipped')is None,
                'Native terminal JUnit differs')
        processes=[p for p in mapped['processes']if p['role']==role and(argument is None or p['command'][-1]==argument)]
        require(len(processes)==1,'Native case process absent/ambiguous');process=processes[0];exe=process['executable']
        require(exe in result['registered_command']and mapped['loaded_project_images'][exe]==receipt['binary_hashes'][exe]
                and any(Path(p).name.startswith('libMatlabInterface.so')for p in mapped['loaded_project_images'])
                and any(p.startswith('/usr/local/MATLAB/R2026b/')and Path(p).name=='libeng.so'for p in mapped['loaded_project_images']),
                'Actual native/plugin/SDK mapping absent')
        if role in ['fmincon-startup-test','fmincon-solve-test']:
            require(any(Path(p).name.startswith('libFminconOptimizer.so')for p in mapped['loaded_project_images']),'Actual fmincon plugin mapping absent')
        trace=[json.loads(line)for line in(directory/f'trace-{process["pid"]}.jsonl').read_text().splitlines()]
        require(trace[0]['event']=='start'and trace[-1]['event']=='summary'and trace[-1]['screenshots']==0,'Terminal observer absent')
        exposures=sum(r['event']=='native_exposure'and r.get('exposed',False)for r in trace)
        require(exposures==result['trace']['qt_window_exposure_events']and(mode!='visible'or exposures==result['trace']['native_exposure_events']>0),
                'Native/visible exposure census differs')
        proof_path=directory/('startup-observed.json'if role=='fmincon-startup-test'else'proof.json')
        proof=load(proof_path);emitted=re.findall(r'NASA_CHECK (\{[^\n]+\})',Path(result['log']).read_text())
        require(len(emitted)==1 and json.loads(emitted[0])==proof,'Actual stdout differs from case proof')
        contexts.append(dict(run=name,test=test,mode=mode,source_version=version,pid=process['pid'],
            native_exposures=result['trace']['native_exposure_events']if mode=='visible'else 0,
            project_SDK_image_count=len(mapped['loaded_project_images'])))
        return proof,directory
    for mode,name in [('headless','implemented-headless-v1'),('visible','implemented-visible-v1')]:
        p,d=context(name,3,mode,'QtGui.NasaFminconCallbackStartup','fmincon-startup-test')
        require(p['factory_available']is True and p['factory_build_succeeded']is True and p['server_started']is True
                and p['server_error']==''and p['source_unchanged']is True and p['history_unchanged']is True
                and p['new_requirement_credit']==0 and'FminconOptimizer'in p['native_types'],'Actual startup/factory conjunction differs')
    selections=[('headless','implemented-headless-v1',3,'Constrained'),('headless','scalar-headless-v1',3,'BoundedScalar'),
        ('visible','implemented-visible-v1',3,'Constrained'),('visible','implemented-visible-v1',3,'BoundedScalar'),
        ('headless','stop-headless-v1',4,'StopRecovery'),('visible','stop-visible-v1',4,'StopRecovery')]
    for mode,name,version,profile in selections:
        p,d=context(name,version,mode,SOLVE_PREFIX+profile,'fmincon-solve-test',profile)
        counts=solve(p,profile,d);solves.append((profile,p,counts))
    for mode in ['headless','visible']:
        for case in io.POSITIVES+io.NEGATIVES:
            p,d=context('io-regression-'+mode+'-v1',4,mode,io.PREFIX+case,'matlab-io-test',case)
            io.check(p,case,d)
            if case in io.POSITIVES:
                io_counts['missions']+=len(p['phases']);io_counts['MATLAB_calls']+=sum(r['matlab_calls']for r in p['phases'])
                io_counts['inside_GMAT_functions']+=sum(r['function_calls']for r in p['phases'])
        for profile in ['Dense','Records']:
            name='writer-before-headless-v1'if mode=='headless'else'writer-visible-v1'
            p,d=context(name,5,mode,WRITER_PREFIX+profile,'matlab-writer-test',profile);writer(p,profile,d);writers.append((profile,p))
        p,d=context('protocol-'+mode+'-v1',6,mode,'QtGui.NasaFminconProtocol','fmincon-protocol-test');protocol(p,d);protocols.append(p)
    # Recheck before failures and exact old startup identity without replaying.
    prior_failure=load(stage/'startup-before-audit-v2.json')
    require(prior_failure['new_requirement_credit']==0 and prior_failure['qualified_keys']==[]and len(prior_failure['contexts'])==2,
            'Original native failures reclassified')
    for p,h in load(stage/'before-evidence-manifest-v2.json')['artifact_sha256'].items():require(sha(repo/p)==h,'Changed before evidence '+p)
    delta=load(stage/'generated-startup-delta-v1.json')
    require(sha(stage/'generated-startup-before-v1.source')==delta['before_sha256']
            and sha(stage/'generated-startup-after-v1.source')==delta['after_sha256']
            and(stage/'generated-startup-after-v1.source').read_bytes().replace(b'PLUGIN = ../plugins/libFminconOptimizer\n',b'')
            ==(stage/'generated-startup-before-v1.source').read_bytes(),'Generated startup delta changed')
    old_core=load(before/'core-inventory-v1/inventory.json');new_core=load(stage/'core-inventory-v1/inventory.json')
    a={r['path']:r['working_sha256']for r in old_core['files']};b={r['path']:r['working_sha256']for r in new_core['files']}
    require(len(a)==248 and len(b)==257 and set(a)<=set(b)and sum(a[p]==b[p]for p in a)==245,'Core increment/custody differs')
    qa=load(stage/'source-qa-v1.json');require(qa['original_startup_assertions_preserved']==5 and qa['original_solve_assertions_preserved']==29
        and qa['I_O_test_source_byte_identical']and qa['full_catalogue_test_source_byte_identical']and not qa['relaxed_numerical_bounds'],
        'Original assertion contract changed')
    # Deliberately corrupt numeric, temporal, recovery, writer and protocol claims.
    base=next(p for name,p,_ in solves if name=='Constrained');stopped=next(p for name,p,_ in solves if name=='StopRecovery')
    data=next(p for name,p in writers if name=='Records');wire=protocols[0];negative=[]
    controls=[('wrong-native-control',base,lambda p:p['phases'][0]['native'].update(Alpha=0),lambda p:solve(p,'Constrained')),
        ('wrong-SDK-vector',base,lambda p:p['phases'][0].update(actual_MATLAB_X=[0,0]),lambda p:solve(p,'Constrained')),
        ('wrong-SDK-cost',base,lambda p:p['phases'][0].update(actual_MATLAB_fVal=0),lambda p:solve(p,'Constrained')),
        ('missing-reopen',base,lambda p:p['phases'].pop(),lambda p:solve(p,'Constrained')),
        ('lost-UI-events',base,lambda p:p['phases'][0].update(Qt_heartbeat_ticks=0),lambda p:solve(p,'Constrained')),
        ('unearned-credit',base,lambda p:p.update(new_requirement_credit=1),lambda p:solve(p,'Constrained')),
        ('lost-Stop',stopped,lambda p:p['stop_recovery'].update(actual_Stop_action_requests=0),lambda p:solve(p,'StopRecovery')),
        ('wrong-MAT-record',data,lambda p:p['actual']['record0'].update(labels=[]),lambda p:writer(p,'Records')),
        ('accepted-bad-packet',wire,lambda p:p['rejections'][0]['response'].update(ok=True),protocol),
        ('lost-frame-control',wire,lambda p:p['rejections'].pop(),protocol)]
    for label,proof,mutate,check in controls:
        corrupt=copy.deepcopy(proof);mutate(corrupt)
        try:check(corrupt)
        except(ValueError,AssertionError):negative.append(label)
        else:raise ValueError('Accepted corrupt integration proof '+label)
    require(len(contexts)==42 and io_counts==dict(missions=66,MATLAB_calls=264,inside_GMAT_functions=198)
            and sum(c['solves']for _,_,c in solves)==18 and sum(c['intentional_Stops']for _,_,c in solves)==2,
            'Integration/affected regression census differs')
    return dict(status='passed-component-evidence;original-requirements-open',qualified_keys=[],new_requirement_credit=0,
        coverage=ledger['coverage'],requirements_still_needed=79,GUI_case_contexts=len(contexts),
        successful_real_fmincon_solves=18,intentional_fmincon_Stop_attempts=2,
        recorded_GMAT_optimizer_branch_evaluations=sum(c['GMAT_branch_evaluations']for _,_,c in solves),
        branch_count_limit='Includes native publication/final passes; not a claim of MATLAB callback count or actual optimizer iterations.',
        I_O_regressions=io_counts,MAT_files_written_and_loaded=4,wire_rejection_controls=20,
        visible_native_exposures=sum(c['native_exposures']for c in contexts),case_source_bindings=contexts,
        corrupt_proof_controls=negative,pending_core_paths=257,unchanged_prior_core_paths=245,
        original_hold_identities_retained=59,unresolved_original_hold_count=56,unresolved_original_MATLAB_count=34,
        remaining_optimizer_rows=23,limits=['No optimizer row admission from these constituents.',
            'Actual Resource Tree/command configuration,setting/limit/scale conjunctions and real iteration/progress/report proof remain.',
            'Selected numeric/string MAT records are not general MAT-format/estimation qualification.',
            'Wire controls cover malformed/session/identity/operation/frame/idle rejection; active invalid-shape cases remain separate.',
            'Reentrant Engine evaluation is explicitly rejected; nested MATLAB operations in external optimizer callbacks are not qualified.'],
        scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,
        full_NASA_qualified=False,MATLAB_bundle_qualified=False)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--stage',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();require(not args.output.exists(),'Audit output already exists')
    result=audit(Path(__file__).resolve().parents[3],args.stage.resolve());args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k]for k in ['status','GUI_case_contexts','successful_real_fmincon_solves','new_requirement_credit','visible_native_exposures']}))
