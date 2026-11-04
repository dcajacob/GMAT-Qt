"""Independently audit actual MATLAB iteration, optional output and limit clauses."""
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
import AuditFminconVaryAcceptance as vary
import AuditFminconCallbackEvidence as callback
import AuditFminconResourceAcceptance as resource
import BuildNasaCoverageExtension as common

KIND = 'source-versioned-fmincon-actual-iterations-output-and-limits'
KEYS = ['FRR!E273', 'FRR!E274'] + ['FRR!E'+str(i) for i in range(280,286)]
PROFILES = ['ConstrainedOn','ConstrainedOff','ScalarOn','UnconstrainedOn','IterationLimit','EvaluationLimit']
sha, load, require = common.sha, common.load, common.require


def number(value):
    require(type(value) in (int,float) and math.isfinite(value),'Missing/nonfinite numeric datum')
    return value


def check(proof, profile, directory=None):
    require(proof['checkpoint']=='actual-fmincon-iteration-progress' and proof['profile']==profile
            and proof['new_requirement_credit']==0,'Wrong iteration proof')
    phases=proof['phases']
    require([p['phase'] for p in phases]==['initial','no-explicit-build-repeat','reopened'],'Missing repeat/reopen conjunction')
    pair=profile.startswith('Constrained') or profile=='IterationLimit'
    scalar=profile in ['ScalarOn','EvaluationLimit']; limit=profile=='IterationLimit'; evaluation=profile=='EvaluationLimit'
    enabled=profile!='ConstrainedOff'; unconstrained=profile=='UnconstrainedOn'
    additive,multiplier=(-3,.25) if pair else (3,2.5)
    def cost(a,b):
        return (1-a)**2+100*(b-a*a)**2 if limit else (a-2)**2+(b+1)**2 if pair else (a-(4 if scalar else 2))**2
    total_records=total_evaluations=0
    for p in phases:
        require(p['messages_enabled'] is enabled and p['report_enabled'] is enabled and p['actual_ReportViewer'] is True,'Optional GUI output/viewer differs')
        stats=p['MATLAB_summary']; require(len(stats)==6 and all(math.isfinite(number(v)) for v in stats),'MATLAB output shape differs')
        require(stats[4:]==[1 if limit else 80,2 if evaluation else 600],'Actual MATLAB limits differ from GUI settings')
        records=p['MATLAB_records']; expected=[]; previous_iteration=previous_count=-1
        require(len(records)>=2 and records[0]['state']=='init' and records[-1]['state']=='done','Initial/final OutputFcn records missing')
        for index,event in enumerate(records):
            state=event['state']; iteration=number(event['iteration']); count=number(event['funccount']); f=number(event['fval'])
            require(state in ['init','iter','done'] and (index==0 or state!='init') and (index==len(records)-1 or state!='done')
                    and iteration>=previous_iteration and count>=previous_count and iteration>=0 and count>=0
                    and iteration==int(iteration) and count==int(count),'Invalid actual iteration sequence')
            if index==0: require(iteration==0,'Initial iteration is not zero')
            x=event['x'];require(len(x)==(2 if pair else 1),'Actual control dimension differs')
            a=number(x[0])/multiplier-additive; b=number(x[1])/2-4 if pair else 2
            require(abs(f-cost(a,b))<1e-8 and abs(number(event['native_fval'])-f)<1e-8,'MATLAB/native objective differs from physical formula')
            expected.append(dict(state=state,iteration=iteration,funccount=count,Alpha=a,Beta=b,Cost=f,Balance=a+b))
            previous_iteration,previous_count=iteration,count
        require(previous_iteration==stats[0] and previous_count==stats[1] and abs(records[-1]['fval']-stats[2])<1e-8,'OutputFcn/fmincon summary differs')
        require(expected==p['expected_records'],'Native test expected-record payload differs from independent formula')
        n=p['native'];require(set(n)=={'Alpha','Beta','Cost','Balance','Calls','Keep'} and all(math.isfinite(number(v)) for v in n.values()),'Native final result partition differs')
        a,b=n['Alpha'],n['Beta'];require(n['Keep']==41 and n['Calls']>0 and n['Calls']==int(n['Calls'])
                and abs(n['Cost']-cost(a,b))<1e-8 and abs(n['Balance']-(a+b))<1e-12,'Native objective/constraint/sentinel differs')
        require(abs(expected[-1]['Alpha']-a)<1e-8 and abs(expected[-1]['Beta']-b)<1e-8,'Final notified/native controls differ')
        if limit:
            directional=2*(a-1)-200*(1-a-a*a)*(1+2*a)
            require(stats[3]==0 and stats[0]==1 and abs(directional)>1e-3,'Iteration cap did not halt a nonstationary problem')
        elif evaluation:
            require(stats[3]==0 and stats[1]==3 and stats[0]==1 and records[0]['funccount']==2 and abs(a+1)<1e-8 and n['Cost']>20,'Evaluation cap did not halt before convergence')
        else:
            truth=(1.5,-.5,.5) if pair else (3,2,1) if scalar else (2,2,0)
            require(stats[3]>0 and abs(a-truth[0])<1e-6 and abs(b-truth[1])<1e-6 and abs(n['Cost']-truth[2])<1e-8,'Independent active-bound optimum differs')
        if directory:
            phase=p['phase'];require(load(directory/(phase+'-MATLAB-iterations.json'))==records,'Raw MATLAB history differs')
            raw=[[float(v) for v in line.split(',')] for line in (directory/(phase+'-evaluations.csv')).read_text().strip().splitlines()]
            require(len(raw)==n['Calls'],'Raw branch evaluation count differs')
            for i,row in enumerate(raw,1):
                require(len(row)==5 and all(math.isfinite(v) for v in row) and abs(row[2]-cost(row[0],row[1]))<1e-8
                        and abs(row[3]-row[0]-row[1])<1e-12 and row[4]==i,'Raw branch physical formula/sequence differs')
            final=[float(v) for v in (directory/(phase+'-report.csv')).read_text().strip().split(',')]
            require(len(final)==6 and all(abs(v-n[k])<1e-12 for v,k in zip(final,['Alpha','Beta','Cost','Balance','Calls','Keep'])),'Raw/native final report differs')
            text=(directory/(phase+'-messages.txt')).read_text(); report=directory/(phase+'-solver.txt')
            if enabled:
                require(report.is_file() and 'Fmincon Text File' in report.read_text(),'Optional solver report absent')
                for output in [text,report.read_text()]: check_output(output,expected,pair,unconstrained,evaluation,additive,multiplier)
            else:require(not report.exists() and 'MATLAB iteration' not in text,'Disabled iteration messages/report emitted')
        total_records+=len(records);total_evaluations+=n['Calls']
    if directory:
        require(load(directory/'phase-results.json')==phases,'Raw phase proof differs')
        source=(directory/'caller.script').read_text()
        for field,value in [('MaximumIterations',str(1 if limit else 80)),('MaxFunEvals',"'2'" if evaluation else "'600'"),
                            ('ShowProgress','true' if enabled else 'false'),('ReportStyle','Verbose')]:
            require(re.search(r'Opt\.'+field+r'\s*=\s*'+re.escape(value)+r'\s*;',source),'Actual GUI-configured caller property missing: '+field)
        require('% Keep actual iteration caller tail Ω' in source,'Unicode caller tail missing')
    return total_records,int(total_evaluations)


def check_output(text,expected,pair,unconstrained,evaluation,additive,multiplier):
    pattern=r'Fmincon "Opt": MATLAB iteration ([0-9]+) \[(init|iter|done)\], function evaluations ([0-9]+)\n([\s\S]*?)End fmincon iteration\n'
    blocks=list(re.finditer(pattern,text))
    require(len(blocks)==len(expected)==text.count('MATLAB iteration ') and 'Control Sequence Pass' not in text,'Missing/extra/stale native iteration records')
    for match,e in zip(blocks,expected):
        require(int(match[1])==e['iteration'] and match[2]==e['state'] and int(match[3])==e['funccount'],'Native iteration header differs from actual MATLAB')
        lines=match[4].splitlines(); index=0
        controls=[('Alpha',e['Alpha'],multiplier*(e['Alpha']+additive))]
        if pair: controls.append(('Beta',e['Beta'],2*(e['Beta']+4)))
        for name,physical,scaled in controls:
            m=re.fullmatch(r'  Control "'+name+r'": physical ([-+0-9.eE]+), optimizer scaled value: ([-+0-9.eE]+)',lines[index]);index+=1
            require(m is not None and abs(float(m[1])-physical)<1e-9 and abs(float(m[2])-scaled)<1e-9,'Native physical/scaled control differs')
        constraints=[('Balance',e['Balance'],1,'='),('Alpha',e['Alpha'],1.5,'<='),('Beta',e['Beta'],-2,'>=')] if pair else [] if unconstrained else [('Alpha',e['Alpha'],-2 if evaluation else 1,'>=')]
        for name,achieved,desired,op in constraints:
            m=re.fullmatch(r'  Constraint "'+name+r'": achieved ([-+0-9.eE]+), desired ([-+0-9.eE]+) \('+re.escape(op)+r'\)',lines[index]);index+=1
            require(m is not None and abs(float(m[1])-achieved)<1e-9 and abs(float(m[2])-desired)<1e-12,'Native achieved/desired constraint/sense differs')
        m=re.fullmatch(r'  Cost "Cost": ([-+0-9.eE]+)',lines[index]);index+=1
        require(m is not None and abs(float(m[1])-e['Cost'])<1e-8 and index==len(lines),'Native cost or field census differs')


class Evidence:
    def __init__(self,repo,stage,startup):
        self.repo,self.stage,self.startup=repo,stage,startup
        self.cache={};self.sources={};self.contexts=[]
        self.supplements=load(stage/'source-custody-supplements-v1.json')

    def source(self,version):
        if version not in self.sources:
            self.sources[version]=frozen.archive(self.stage/f'executed-sources-v{version}.tar.gz',load(self.stage/f'executed-source-sha256-v{version}.json'))
        return self.sources[version]

    def context(self,name,version,mode,test,role,argument=None):
        if name not in self.cache:
            receipt=load(self.stage/name/'results.json');mapped=load(self.stage/(name+'-loaded-images.json'))
            ids=load(self.stage/f'executed-source-sha256-v{version}.json');self.source(version)
            declared=dict(ids)
            if name in self.supplements:
                supplement=self.supplements[name]
                require(sha(supplement['archive'])==supplement['archive_sha256'],'Changed supplemental source archive')
                require(set(supplement['source_sha256'])=={'src/qtgui/tests/AuditNasaCoveragePrefix.py'}
                    and not set(supplement['source_sha256'])&set(ids),'Unexpected supplemental source scope')
                frozen.archive(supplement['archive'],supplement['source_sha256']);declared.update(supplement['source_sha256'])
            require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection'
                    and mapped['returncode'] in [0,1] and mapped['inputs_unchanged'] is True and mapped['runtime_binary_copies']==0,'Nonterminal/mutated run')
            failures=[r for r in receipt['results'] if r['status'] not in ['passed','not-run']]
            if mapped['returncode']:
                require(name=='controls-headless-v1' and len(failures)==1 and failures[0]['name']=='QtGui.NasaFminconIterationEvaluationLimit'
                    and 'Actual function-evaluation limit did not stop before convergence' in Path(failures[0]['log']).read_text(),
                    'Unexpected mixed-batch failure;selected passing cases cannot be reused')
            else:require(not failures,'Failed result with successful run code')
            require(mapped['source_sha256']==ids and all(declared.get(p)==h for p,h in receipt['source_hashes'].items())
                    and receipt['startup_sha256']==self.startup,'Source/startup custody differs')
            union={}
            for process in mapped['processes']:
                for snap in process['snapshots']:
                    require(hashlib.sha256(snap['maps'].encode()).hexdigest()==snap['maps_sha256'],'Raw map digest differs')
                    paths=set()
                    for line in snap['maps'].splitlines():
                        fields=line.split(None,5)
                        if len(fields)==6 and (fields[5].startswith(str(self.repo)+'/') or fields[5].startswith('/usr/local/MATLAB/R2026b/')):paths.add(fields[5])
                    require(paths==set(snap['project_images']),'Raw mapped image partition differs');union.update(snap['project_images'])
            require(union==mapped['loaded_project_images'],'Loaded image union differs')
            if mode=='visible':
                d=receipt['visible_display'];require(receipt['max_screenshots']==0 and d['resizable'] and d['start_minimized']
                    and 'Iconic' in d['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in d['initial_host_window_state'],'Visible window policy missing')
            self.cache[name]=receipt,mapped
        receipt,mapped=self.cache[name];result=next(r for r in receipt['results'] if r['name']==test)
        require(result['status']=='passed' and result['returncode']==0,'Selected native case failed')
        directory=Path(result['log']).parent;xml=list(ET.parse(directory/'ctest.xml').getroot().iter('testcase'))
        require(len(xml)==1 and xml[0].get('name')==test and xml[0].find('failure') is None and xml[0].find('skipped') is None,'JUnit differs')
        processes=[p for p in mapped['processes'] if p['role']==role and (argument is None or p['command'][-1]==argument)]
        require(len(processes)==1,'Missing/ambiguous actual native case');process=processes[0];exe=process['executable']
        require(exe in result['registered_command'] and mapped['loaded_project_images'][exe]==receipt['binary_hashes'][exe],'Native executable identity differs')
        if 'protocol' not in role:
            require(all(any(f in path for path in mapped['loaded_project_images']) for f in ['libFminconOptimizer.so','libMatlabInterface.so','/R2026b/bin/glnxa64/libeng.so']),'Actual fmincon/SDK images missing')
        trace=[json.loads(line) for line in (directory/f'trace-{process["pid"]}.jsonl').read_text().splitlines()]
        require(trace[0]['event']=='start' and trace[-1]['event']=='summary' and trace[-1]['screenshots']==0,'Terminal observer missing')
        exposures=sum(r['event']=='native_exposure' and r.get('exposed',False) for r in trace)
        require(exposures==result['trace']['qt_window_exposure_events'] and (mode!='visible' or exposures==result['trace']['native_exposure_events']>0),'Exposure census differs')
        proof=load(directory/'proof.json');emitted=re.findall(r'NASA_CHECK (\{[^\n]+\})',Path(result['log']).read_text())
        require(len(emitted)==1 and json.loads(emitted[0])==proof,'Stdout/proof differs')
        self.contexts.append(dict(run=name,source_version=version,test=test,mode=mode,pid=process['pid'],visible_exposures=exposures if mode=='visible' else 0))
        return proof,directory


def qualify(repo,path,inventory):
    q=load(path);require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Different/partial original scope')
    require(all(q[k] is False for k in ['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','MATLAB_bundle_qualified']),'Global gate falsely closed')
    for p,h in q['artifact_sha256'].items():require(Path(p).is_file() and not Path(p).is_symlink() and sha(p)==h,'Changed artifact '+p)
    frozen.archive(q['reader_source_archive'],q['reader_source_sha256'])
    for p,h in q['reader_source_sha256'].items():require(sha(repo/p)==h,'Current offline reader differs: '+p)
    for module in [sys.modules[__name__],nasa,frozen,vary,callback,resource,common]:
        p='src/qtgui/tests/'+Path(module.__file__).name;require(sha(repo/p)==q['reader_source_sha256'][p],'Executing reader source differs')
    scope=load(q['scope_review']);before=load(q['prior_coverage']);overlay=load(q['prior_overlay']);stage=Path(q['stage'])
    known={r['key']:r for r in inventory['requirements']}; row=lambda k:dict(key=k,declared_id=known[k]['declared_id'],fingerprint=known[k]['fingerprint'],source_text=known[k]['text'])
    require(q['authority_sha256']==inventory['sources']==scope['authority_sha256'] and len(known)==1158 and len(inventory['historical_cases'])==2367,'Original authority differs')
    require(scope['candidate_rows']==[row(k) for k in KEYS] and scope['progress_children']==[row('FRR!E'+str(i)) for i in range(281,285)]
            and scope['required_profiles']==PROFILES,'Original conjunction changed')
    require(before['coverage']['requirements']['verified']==1031 and before['coverage']['requirements']['total']==1158
            and before['coverage']['historical_cases']['verified']==1816 and before['requirements_still_needed']==70,'Prior exact union changed')
    require(set(KEYS)<={r['key'] for r in before['remaining_original_rows']['requirements']},'Duplicate/missing credit')
    original={r['key']:r['original_row'] for r in overlay['reviewed_source_rows'] if r['key'] in KEYS}
    require(scope['retained_original_closure_records']==original and scope['original_bundle_gates_retained']=={k:r['closure_gate'] for k,r in original.items()}
            and scope['MATLAB_bundle_qualified'] is False and len(overlay['original_hold_keys_retained'])==59,'Original holds/bundle gates lost')
    evidence=Evidence(repo,stage,q['generated_startup_sha256']);records=evaluations=0;proofs={};legacy_solves=legacy_evaluations=stops=0
    expected_tests={'QtGui.NasaFminconIteration'+p:'fmincon-iteration-test' for p in PROFILES}
    expected_tests.update({'QtGui.NasaFminconIterationProtocol':'fmincon-iteration-protocol-test',
        'QtGui.NasaFminconResource':'fmincon-resource-test','QtGui.NasaFminconProtocol':'fmincon-protocol-test'})
    expected_tests.update({'QtGui.NasaFminconSolve'+p:'fmincon-solve-test' for p in ['Constrained','BoundedScalar','StopRecovery']})
    expected_tests.update({'QtGui.NasaFminconVary'+p:'fmincon-vary-test' for p in ['Combined','References']})
    require(len(q['case_bindings'])==28 and {(b['mode'],b['test']) for b in q['case_bindings']}==
        {(m,t) for m in ['headless','visible'] for t in expected_tests} and
        all(b['role']==expected_tests[b['test']] for b in q['case_bindings']),'Required new/affected case matrix differs')
    for binding in q['case_bindings']:
        p,d=evidence.context(binding['run'],binding['version'],binding['mode'],binding['test'],binding['role'],binding.get('argument'))
        profile=binding.get('profile')
        if profile:
            a,b=check(p,profile,d);records+=a;evaluations+=b;proofs[binding['mode']+'-'+profile]=p
        elif binding['role']=='fmincon-iteration-protocol-test':
            require(p['checkpoint']=='actual-Qt-fmincon-iteration-protocol' and len(p['rejections'])==20
                    and load(d/'rejections.json')==p['rejections'] and all(r['response']['ok'] is False for r in p['rejections'])
                    and p['caller_unchanged'] is True,'New wire rejection controls missing')
            extra=['iteration-invalid-state','iteration-negative-number','iteration-fractional-count','iteration-string-number',
                'iteration-overflow-count','iteration-null-objective','iteration-extra-script','iteration-missing-objective','iteration-string-vector','idle-iteration']
            require([r['case'] for r in p['rejections']]==callback.PROTOCOL_CASES[:-1]+extra+callback.PROTOCOL_CASES[-1:],'New wire control identity differs')
        elif binding['role']=='fmincon-vary-test':
            legacy_evaluations+=vary.check(p,binding['argument'],d);legacy_solves+=len(p['phases'])
        elif binding['role']=='fmincon-solve-test':
            c=callback.solve(p,binding['argument'],d);legacy_solves+=c['solves'];legacy_evaluations+=c['GMAT_branch_evaluations'];stops+=c['intentional_Stops']
        elif binding['role']=='fmincon-resource-test':legacy_evaluations+=resource.check(p,d);legacy_solves+=len(p['phases'])
        elif binding['role']=='fmincon-protocol-test':callback.protocol(p,d)
        else:raise ValueError('Unreviewed regression role')
    require(set(proofs)=={m+'-'+p for m in ['headless','visible'] for p in PROFILES},'Required profile/mode matrix incomplete')
    controls=[]
    mutations=[('missing-phase',lambda p:p['phases'].pop()),('wrong-summary-iteration',lambda p:p['phases'][0]['MATLAB_summary'].__setitem__(0,99)),
        ('wrong-function-count',lambda p:p['phases'][0]['MATLAB_summary'].__setitem__(1,99)),('wrong-output-function-cost',lambda p:p['phases'][0]['MATLAB_records'][0].update(fval=17)),
        ('wrong-native-cost',lambda p:p['phases'][0]['MATLAB_records'][0].update(native_fval=17)),('wrong-scaled-control',lambda p:p['phases'][0]['MATLAB_records'][0]['x'].__setitem__(0,17)),
        ('wrong-state',lambda p:p['phases'][0]['MATLAB_records'][0].update(state='interrupt')),('wrong-control-dimension',lambda p:p['phases'][0]['MATLAB_records'][0]['x'].pop()),
        ('wrong-constraint',lambda p:p['phases'][0]['native'].update(Balance=0)),('wrong-final-cost',lambda p:p['phases'][0]['native'].update(Cost=0)),
        ('wrong-sentinel',lambda p:p['phases'][0]['native'].update(Keep=0)),('viewer-missing',lambda p:p['phases'][0].update(actual_ReportViewer=False)),
        ('wrong-message-toggle',lambda p:p['phases'][0].update(messages_enabled=False)),('wrong-report-toggle',lambda p:p['phases'][0].update(report_enabled=False))]
    for name,mutate in mutations:
        p=copy.deepcopy(proofs['headless-ConstrainedOn']);mutate(p)
        try:check(p,'ConstrainedOn')
        except (ValueError,KeyError,TypeError,IndexError):controls.append(name)
        else:raise ValueError('Corrupt proof accepted: '+name)
    for profile,index in [('IterationLimit',0),('EvaluationLimit',1)]:
        p=copy.deepcopy(proofs['headless-'+profile]);p['phases'][0]['MATLAB_summary'][index]=80
        try:check(p,profile)
        except ValueError:controls.append('ignored-'+profile)
        else:raise ValueError('Ignored real limit accepted')
    original=(stage/'initial-headless-v1/QtGui.NasaFminconIterationConstrainedOn/initial-solver.txt').read_text()
    for name,old,new in [('wrong-output-iteration','MATLAB iteration 0 [init]','MATLAB iteration 7 [init]'),
        ('wrong-output-physical','physical -1,','physical 99,'),('wrong-output-desired','desired 1 (=)','desired 2 (=)'),
        ('wrong-output-sense','desired 1.5 (<=)','desired 1.5 (>=)'),('wrong-output-cost','Cost "Cost": 18','Cost "Cost": 19'),
        ('lost-output-constraint','  Constraint "Balance": achieved 1, desired 1 (=)\n','')]:
        corrupt=original.replace(old,new,1);require(corrupt!=original,'Raw mutation did not apply')
        try:check_output(corrupt,proofs['headless-ConstrainedOn']['phases'][0]['expected_records'],True,False,False,-3,.25)
        except (ValueError,IndexError):controls.append(name)
        else:raise ValueError('Corrupt raw iteration output accepted: '+name)
    qa=load(stage/'source-qa-v1.json');require(not qa['numerical_oracle_bounds_relaxed'],'Numerical bounds relaxed')
    require(qa['core_paths']==259 and qa['core_source_changes']==6 and len(qa['unchanged_prior_core_paths'])==253
        and len(qa['incremented_core_paths'])==4 and len(qa['new_core_paths'])==2,'Core increment scope differs')
    core=load(qa['core_inventory']);require(len(core['files'])==259 and sha(qa['core_inventory'])==qa['core_inventory_sha256'],'Core inventory differs')
    for r in core['files']:require(sha(repo/r['path'])==r['working_sha256'],'Core source changed after native execution')
    for p in qa['production_paths']:
        require(evidence.source(3)[p]==evidence.source(4)[p] and sha(repo/p)==hashlib.sha256(evidence.source(4)[p]).hexdigest(),
            'Production changed across admitted native versions')
    for p in qa['old_test_sources_byte_identical']:
        require(evidence.source(1)[p]==evidence.source(3)[p] and sha(repo/p)==hashlib.sha256(evidence.source(3)[p]).hexdigest(),'Old test source changed')
    require(evidence.source(1)['src/qtgui/tests/NasaFminconIterationTests.cpp']==evidence.source(3)['src/qtgui/tests/NasaFminconIterationTests.cpp'],'Baseline failure assertions changed')
    before_test=evidence.source(3)['src/qtgui/tests/NasaFminconIterationTests.cpp'].decode()
    after_test=evidence.source(4)['src/qtgui/tests/NasaFminconIterationTests.cpp'].decode()
    correction='stats[3]==0&&stats[1]==maxEval+1&&stats[0]==1&&records.first().toObject()[\"funccount\"].toInt()==2&&std::abs(native[\"Alpha\"].toDouble()+1)'
    require(before_test.replace('stats[3]==0&&stats[1]==maxEval&&std::abs(native[\"Alpha\"].toDouble()+1)',correction)==after_test,'Additional native oracle changes')
    review=load(stage/'evaluation-limit-oracle-review-v1.json')
    require(review['configured_MaxFunEvals']==2 and review['expected_count_before']==2 and review['expected_count_after']==3
        and sha(review['MATLAB_source'])==review['MATLAB_source_sha256'] and not review['numerical_tolerance_bounds_relaxed'],
        'Installed strict-count oracle review differs')
    failure=load(stage/'controls-headless-v1/results.json');failed=next(r for r in failure['results'] if r['name']=='QtGui.NasaFminconIterationEvaluationLimit')
    history=load(Path(failed['log']).parent/'initial-MATLAB-iterations.json')
    require(failed['status']=='failed' and history[0]['funccount']==2 and history[-1]['funccount']==3 and history[-1]['iteration']==1,
        'Retained original exact-count failure differs')
    for name in ['before-headless-v1','before-visible-v1']:
        failed=load(stage/name/'results.json');r=next(r for r in failed['results'] if r['name']=='QtGui.NasaFminconIterationConstrainedOn')
        require(r['status']=='failed' and r['returncode']!=0 and 'Actual MATLAB output-function iteration history missing' in Path(r['log']).read_text(),'Original failure hidden')
    q['audit_summary']=dict(qualified_keys=KEYS,GUI_case_contexts=len(evidence.contexts),new_iteration_contexts=12,real_iteration_missions=36,
        actual_MATLAB_notification_records=records,iteration_GMAT_branch_evaluations=evaluations,case_source_bindings=evidence.contexts,
        affected_legacy_solves=legacy_solves,affected_legacy_evaluations=legacy_evaluations,intentional_Stops=stops,
        visible_native_exposures=sum(c['visible_exposures'] for c in evidence.contexts),corrupt_proof_controls=controls,
        retained_original_hold_identities=59,remaining_optimizer_rows=6,retained_bundle_gate_status='open')
    require(legacy_solves==40 and stops==2 and records==156 and evaluations==618 and len(controls)==22,'Exact mission/notification/control census differs')
    return q,KEYS


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--qualification',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    repo=Path(__file__).resolve().parents[3];q,keys=qualify(repo,a.qualification,nasa.inventory(repo));require(not a.output.exists(),'Audit output exists')
    a.output.write_text(json.dumps(dict(status='passed',qualified_keys=keys,summary=q['audit_summary']),indent=2)+'\n');print(json.dumps(q['audit_summary']))
