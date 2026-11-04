"""Admit only the original fmincon Resource Tree leaf from complete native evidence."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
import AuditMatlabFunctionCreation as frozen
import AuditFminconCallbackEvidence as callback
import BuildNasaCoverageExtension as common

KIND='source-versioned-fmincon-Resource-Tree-creation-and-configuration'
KEY='All Others!E139'
TEST='QtGui.NasaFminconResource'
OPTIONS=['DiffMaxChange','DiffMinChange','MaxFunEvals','TolX','TolFun','TolCon']
VALUES=[.2,2e-7,400,2e-9,3e-10,4e-10,64]
PHASES=['gui-configured','reopened','progress-enabled']
sha,load,require=common.sha,common.load,common.require


def check(proof,directory=None):
    require(proof['family']=='fmincon-resource'and proof['checkpoint']=='fmincon-resource-creation-and-configuration'
        and proof['outcome']=='passed'and proof['new_requirement_credit']==0,'Different resource identity/automatic credit')
    require(proof['creation_dialogs']==2 and proof['invalid_rejections']==49 and proof['configured_options']==6,
        'Incomplete actual creator/configuration controls')
    require([p['phase']for p in proof['phases']]==PHASES,'Missing resource run/reopen/progress phase')
    calls=0
    for phase in proof['phases']:
        for n,v in dict(Alpha=3,Cost=1,Constraint=2,Keep=41).items():
            require(math.isfinite(phase[n])and abs(phase[n]-v)<(1e-8 if n=='Cost'else 1e-6),
                'Resource analytic optimum/sentinel differs: '+n)
        require(phase['MATLAB_options']==VALUES and abs(phase['MATLAB_X']-3)<1e-6
            and abs(phase['MATLAB_fVal']-1)<1e-8 and phase['MATLAB_exitFlag']>0,'Actual MATLAB option/solve readback differs')
        require(phase['Calls']>=3 and phase['Calls']==int(phase['Calls']),'Invalid branch evaluation census')
        calls+=phase['Calls']
        if directory:
            name=phase['phase'];rows=(directory/(name+'-evaluations.csv')).read_text().strip().splitlines()
            require(len(rows)==phase['Calls'],'Raw evaluation count differs')
            for index,row in enumerate(rows,1):
                v=[float(x)for x in row.split(',')]
                require(len(v)==4 and all(math.isfinite(x)for x in v),'Raw evaluation shape/nonfinite')
                require(abs(v[1]-(v[0]-4)**2)<1e-10 and abs(v[2]-(v[0]-1))<1e-12 and v[3]==index,
                    'Raw branch differs from independent objective/constraint/sequence')
                if index==1:require(v[0]==-1.25,'Different physical initial guess')
            raw=[float(x)for x in(directory/(name+'-report.csv')).read_text().strip().split(',')]
            require(len(raw)==5 and all(abs(a-phase[n])<1e-12 for a,n in zip(raw,['Alpha','Cost','Constraint','Calls','Keep'])),
                'Raw final report differs from native result')
            files=load(directory/(name+'-file-census.json'));enabled=name=='progress-enabled'
            require(files['final_exists']is True and files['evaluations_exist']is True
                and files['solver_report_exists']is enabled,'Documented report enable/disable behavior differs')
            if enabled:
                report=(directory/(name+'-solver.txt')).read_text()
                require('Fmincon Text File'in report and'optimizer scaled value'in report,'Actual enabled Verbose report missing')
            else:require(not(directory/(name+'-solver.txt')).exists(),'Disabled solver report falsely supplied')
    if directory:
        require(load(directory/'results-values.json')==proof['phases'],'Raw phase proof differs')
        require(load(directory/'creation.json')==[dict(accepted=False,menu=True,dialog=True),dict(accepted=True,menu=True,dialog=True)],
            'Actual Add/creator Cancel/accept missing')
        census=load(directory/'field-census.json')
        expected=set(OPTIONS+['MaximumIterations','ShowProgress','ReportStyle','ReportFile','FunctionPath'])
        require({f['name']for f in census['fields']}==expected and all(f['named_input']is True for f in census['fields']),
            'Incomplete actual native editor field census')
        require([(r['name'],r['id'],r['input_present'])for r in census['native_option_aliases']]
            ==[(n,1000+i,True)for i,n in enumerate(OPTIONS)],'Actual option alias identities/inputs differ')
        rejected=load(directory/'rejections.json')
        require([(r['field'],r['input'])for r in rejected]==[(n,v)for n in OPTIONS+['MaximumIterations']
            for v in['','not-a-number','nan','inf','0','-1','1.5junk']]
            and all(r['rejected_atomically']is True and r['diagnostic']for r in rejected),'Atomic invalid input conjunction incomplete')
        accepted=load(directory/'native-settings.json')['after']
        require([float(accepted[n])for n in OPTIONS]+[accepted['MaximumIterations']]==VALUES
            and accepted['ShowProgress']is False and accepted['ReportStyle']=='Verbose'
            and Path(accepted['ReportFile']).name=='Resource solver Ω.txt'
            and Path(accepted['FunctionPath']).name=='gmat_fmincon','Actual GUI native configuration differs')
        require(all(float(census['baseline'][n])!=float(accepted[n])for n in OPTIONS)
            and census['baseline']['MaximumIterations']!=64,'Changed options not distinguished from defaults')
        applied=(directory/'applied.script').read_text();caller=(directory/'caller.script').read_text()
        require('Create FminconOptimizer Opt;'in applied and caller.startswith(applied.split('BeginMissionSequence;')[0])
            and'Vary Opt(Alpha = -1.25'in caller and'Keep = 41;'in caller,'GUI configuration/caller identity differs')
    return int(calls)


def qualify(repo,path,inventory):
    q=load(path)
    require(q['contract_kind']==KIND and q['qualified_keys']==[KEY],'Different original row scope')
    require(all(q[k]is False for k in['technical_core_approval','full_current_build_qualified','full_NASA_qualified',
        'scientific_approval','MATLAB_bundle_qualified']),'Global gate falsely closed')
    for p,h in q['artifact_sha256'].items():require(Path(p).is_file()and not Path(p).is_symlink()and sha(p)==h,'Changed/unbound artifact '+p)
    frozen.archive(q['reader_source_archive'],q['reader_source_sha256'])
    for module in[sys.modules[__name__],nasa,frozen,callback,common]:
        p='src/qtgui/tests/'+Path(module.__file__).name
        require(sha(repo/p)==q['reader_source_sha256'][p],'Executing admission reader differs')
    known={r['key']:r for r in inventory['requirements']}
    row=lambda k:dict(key=k,declared_id=known[k]['declared_id'],fingerprint=known[k]['fingerprint'],source_text=known[k]['text'])
    scope=load(q['scope_review']);before=load(q['prior_coverage']);overlay=load(q['prior_overlay'])
    require(q['authority_sha256']==inventory['sources']==scope['authority_sha256']and len(known)==1158
        and len(inventory['historical_cases'])==2367,'Original authority/denominator changed')
    require(scope['candidate_rows']==[row(KEY)]and scope['inherited_parents']==[row('All Others!E120')]
        and scope['excluded_optimizer_rows']==[r for r in load(q['optimizer_gap_audit'])['rows']if r['key']!=KEY],
        'Original resource scope/inheritance/sibling gaps changed')
    original=next(r['original_row']for r in overlay['reviewed_source_rows']if r['key']==KEY)
    require(scope['original_closure_record']==original and scope['retained_bundle_gate']==original['closure_gate']
        and scope['MATLAB_bundle_qualified']is False,'Original copied gate record lost')
    require(before['coverage']['requirements']['verified']==1022 and before['coverage']['requirements']['total']==1158
        and before['coverage']['historical_cases']['verified']==1816 and before['requirements_still_needed']==79
        and KEY in{r['key']for r in before['remaining_original_rows']['requirements']},'Prior source union changed')
    require(len(overlay['original_hold_keys_retained'])==59 and overlay['unresolved_original_hold_count']==56,'Original hold history lost')
    stage=Path(q['stage']);identities=load(stage/'executed-source-sha256-v6.json')
    frozen.archive(stage/'executed-sources-v6.tar.gz',identities)
    supplement=load(q['source_custody_supplement'])
    member='src/qtgui/tests/AuditFminconCallbackEvidence.py'
    require(supplement['member']==member and sha(supplement['archive'])==supplement['archive_sha256'],
        'Different source custody supplement')
    supplemented=frozen.archive(supplement['archive'],supplement['archive_members'])
    require(hashlib.sha256(supplemented[member]).hexdigest()==supplement['source_sha256'],
        'Sealed supplemental source differs')
    executed_inputs=dict(identities,**{member:supplement['source_sha256']})
    contexts=[];proofs=[];total_calls=0;regression_calls=0
    for mode in['headless','visible']:
        for role,name,test in[('fmincon-resource-test','implemented-'+mode+('-v4'if mode=='headless'else'-v1'),TEST),
            ('fmincon-solve-test','regression-'+mode+'-v1','QtGui.NasaFminconSolveConstrained')]:
            receipt=load(stage/name/'results.json');mapped=load(stage/(name+'-loaded-images.json'))
            require(receipt['mode']==mode and receipt.get('finished_utc')and receipt['end_reason']=='completed-selection'
                and mapped['returncode']==0 and mapped['inputs_unchanged']is True and mapped['runtime_binary_copies']==0,
                'Nonterminal/failed/mutated native selection')
            require(mapped['source_sha256']==identities and all(executed_inputs.get(p)==h for p,h in receipt['source_hashes'].items()),
                'Declared executed input missing from frozen source')
            active=[r for r in receipt['results']if r['status']!='not-run'];require(len(active)==1 and active[0]['name']==test,'Different native selection')
            result=active[0];require(result['status']=='passed'and result['returncode']==0,'Native case failed')
            directory=Path(result['log']).parent;xml=list(ET.parse(directory/'ctest.xml').getroot().iter('testcase'))
            require(len(xml)==1 and xml[0].get('name')==test and xml[0].find('failure')is None and xml[0].find('skipped')is None,'Terminal JUnit differs')
            require(receipt['startup_sha256']==q['generated_startup_sha256'],'Different generated startup')
            union={}
            for process in mapped['processes']:
                for snap in process['snapshots']:
                    require(hashlib.sha256(snap['maps'].encode()).hexdigest()==snap['maps_sha256'],'Raw process map differs')
                    paths=set()
                    for line in snap['maps'].splitlines():
                        parts=line.split(None,5)
                        if len(parts)==6 and(parts[5].startswith(str(repo)+'/')or parts[5].startswith('/usr/local/MATLAB/R2026b/')):paths.add(parts[5])
                    require(paths==set(snap['project_images']),'Process image partition differs');union.update(snap['project_images'])
            require(union==mapped['loaded_project_images'],'Mapped image union differs')
            require(all(any(fragment in p for p in union)for fragment in['libFminconOptimizer.so','libMatlabInterface.so','/R2026b/bin/glnxa64/libeng.so']),
                'Actual fmincon/plugin/SDK images absent')
            process=[p for p in mapped['processes']if p['role']==role];require(len(process)==1,'Missing/ambiguous actual native process');process=process[0]
            exe=process['executable'];require(exe in result['registered_command']and union[exe]==receipt['binary_hashes'][exe],'Actual executable identity differs')
            trace=[json.loads(line)for line in(directory/f'trace-{process["pid"]}.jsonl').read_text().splitlines()]
            require(trace[0]['event']=='start'and trace[-1]['event']=='summary'and trace[-1]['screenshots']==0,'Terminal observer missing')
            exposures=sum(r['event']=='native_exposure'and r.get('exposed',False)for r in trace)
            require(exposures==result['trace']['qt_window_exposure_events'],'Native exposure census differs')
            if mode=='visible':
                d=receipt['visible_display'];require(receipt['max_screenshots']==0 and d['resizable']and d['start_minimized']and'Iconic'in d['initial_host_window_state']
                    and'_NET_WM_STATE_FULLSCREEN'not in d['initial_host_window_state']and exposures==result['trace']['native_exposure_events']>0,'Visible window policy missing')
            proof=load(directory/'proof.json');emitted=re.findall(r'NASA_CHECK (\{[^\n]+\})',Path(result['log']).read_text())
            require(len(emitted)==1 and json.loads(emitted[0])==proof,'Stdout/proof differs')
            if role=='fmincon-resource-test':total_calls+=check(proof,directory);proofs.append(proof)
            else:regression_calls+=callback.solve(proof,'Constrained',directory)['GMAT_branch_evaluations']
            contexts.append(dict(run=name,test=test,mode=mode,pid=process['pid'],source_version=6,visible_exposures=exposures if mode=='visible'else 0))
    core=load(q['current_core_inventory']);old=load(q['prior_core_inventory']);old_by={r['path']:r for r in old['files']}
    require({r['path']for r in core['files']}==set(old_by)and len(core['files'])==257,'Core source set changed')
    changed=[r['path']for r in core['files']if r['working_sha256']!=old_by[r['path']]['working_sha256']]
    require(changed==['plugins/FminconOptimizerPlugin/src/base/solver/FminconOptimizer.cpp'],'Unreviewed core increment')
    for r in core['files']:require(sha(repo/r['path'])==r['working_sha256'],'Core worktree changed after audit snapshot')
    controls=[]
    mutations=[('creator',lambda p:p.update(creation_dialogs=1)),('rejections',lambda p:p.update(invalid_rejections=48)),
        ('fields',lambda p:p.update(configured_options=5)),('missing-phase',lambda p:p['phases'].pop()),
        ('native-control',lambda p:p['phases'][0].update(Alpha=2)),('native-cost',lambda p:p['phases'][0].update(Cost=2)),
        ('native-constraint',lambda p:p['phases'][0].update(Constraint=0)),('sentinel',lambda p:p['phases'][0].update(Keep=40)),
        ('MATLAB-options',lambda p:p['phases'][0]['MATLAB_options'].__setitem__(0,.1)),('MATLAB-exit',lambda p:p['phases'][0].update(MATLAB_exitFlag=-1))]
    for name,mutate in mutations:
        p=copy.deepcopy(proofs[0]);mutate(p)
        try:check(p)
        except(ValueError,KeyError,TypeError,IndexError):controls.append(name)
        else:raise ValueError('Corrupt proof accepted: '+name)
    q['audit_summary']=dict(qualified_keys=[KEY],GUI_case_contexts=4,resource_contexts=2,real_resource_solves=6,
        affected_regression_solves=6,real_fmincon_solves=12,atomic_invalid_controls=98,resource_GMAT_branch_evaluations=total_calls,
        regression_GMAT_branch_evaluations=regression_calls,case_source_bindings=contexts,negative_controls=controls,
        visible_native_exposures=sum(c['visible_exposures']for c in contexts),remaining_optimizer_rows=22,
        retained_original_holds=59,newly_resolved_original_holds=[KEY],retained_bundle_gate_status='open')
    return q,[KEY]


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--qualification',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    repo=Path(__file__).resolve().parents[3];q,keys=qualify(repo,a.qualification,nasa.inventory(repo));require(not a.output.exists(),'Audit output exists')
    a.output.write_text(json.dumps(dict(status='passed',qualified_keys=keys,summary=q['audit_summary']),indent=2)+'\n')
    print(json.dumps(q['audit_summary']))
