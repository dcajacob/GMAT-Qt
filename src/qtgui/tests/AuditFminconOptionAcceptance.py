"""Independent physical/runtime checks for exact original fmincon option clauses."""
import argparse
import copy
import hashlib
import json
import math
import re
from pathlib import Path
import sys
import NasaGuiCoverage as nasa
import AuditFminconIterationAcceptance as iteration
import BuildNasaCoverageExtension as common
import AuditNasaCoveragePrefix as prefix
sha,load,require=common.sha,common.load,common.require
KIND='source-versioned-complete-fmincon-property-conjunction'
KEYS=['FRR!E272']+['FRR!E'+str(i) for i in range(275,280)]
PROFILES=['TolCon','TolFun','TolX','DiffMaxChange','DiffMinChange','DiffRangeRecovery']
NAMES=['DiffMaxChange','DiffMinChange','MaxFunEvals','TolX','TolFun','TolCon']


def specification(profile,original):
    difference=profile.startswith('Diff');con=profile=='TolCon'
    initial=-1 if difference else 0;target=4 if difference else 0 if con else .05;minimum=.05 if con else -2
    baseline='1e-12' if profile=='DiffMinChange' else '0.1'
    edited='0.02' if profile=='DiffMinChange' else '1e-8' if profile=='DiffMaxChange' else '1e-9' if profile=='TolX' else '1e-10'
    settings=dict(DiffMaxChange='0.1',DiffMinChange='1e-12',MaxFunEvals='600',TolX='1e-9',TolFun='1e-6' if con else '1e-10',TolCon='1e-10')
    settings[profile]=baseline if original else edited
    return dict(initial=initial,target=target,minimum=minimum,optimum=3 if difference else 0 if original else .05,settings=settings)


def check(proof,profile,directory=None):
    require(proof['checkpoint']=='discriminating-fmincon-option' and proof['profile']==profile and proof['new_requirement_credit']==0,'Different option identity/automatic credit')
    range_recovery=profile=='DiffRangeRecovery'
    if range_recovery:profile='DiffMinChange'
    require([p['phase'] for p in proof['phases']]==['baseline','edited','reopened','repeat','restored'],'Missing loose/tight/reopen/repeat/restoration conjunction')
    edits=proof['GUI_edits'];require([p['phase'] for p in edits]==(['invalid-range-equal','invalid-range-greater'] if range_recovery else [])+['baseline','edited','restored'],'Missing actual GUI edits')
    previous=specification(profile,False)['settings']
    for e in edits:
        expected=specification(profile,e['phase']!='edited')['settings']
        if e['phase'].startswith('invalid-range'):
            expected=specification(profile,False)['settings'];expected[profile]='0.1' if e['phase']=='invalid-range-equal' else '0.2'
        require(e['option']==profile and e['value']==expected[profile] and e['before']==previous and e['after']==expected
                and e['draft_isolated'] is True and e['exact_undo_redo'] is True,'Actual GUI draft/Apply/history changed other options')
        previous=expected
        if directory:
            text=(directory/(e['phase']+'-caller.script')).read_text()
            require('% Preserve option caller tail Ω' in text and re.search(r'Opt\.'+profile+r"\s*=\s*'"+re.escape(e['value'])+r"'\s*;",text),
                'Actual GUI option serialization/Unicode caller tail differs')
    recovery=proof.get('range_recovery',[])
    require(len(recovery)==(2 if range_recovery else 0),'Range error/recovery conjunction differs')
    for event,value in zip(recovery,['0.1','0.2']):
        require(event['DiffMinChange']==value and event['DiffMaxChange']=='0.1' and event['reported_failed'] is True
            and event['Engine_recovered'] is True and event['caller_unchanged'] is True
            and all(s in event['diagnostic'] for s in ['DiffMinChange','DiffMaxChange']),'Actual MATLAB range rejection/recovery missing')
        if directory:require((directory/(event['phase']+'-messages.txt')).read_text()==event['diagnostic'],'Raw range diagnostic differs')
    if directory and range_recovery:require(load(directory/'range-recovery.json')==recovery,'Raw range recovery proof differs')
    notifications=evaluations=0
    for p in proof['phases']:
        phase=p['phase'];original=phase in ['baseline','restored'];s=specification(profile,original)
        require(p['settings']==s['settings'] and p['ReportViewer'] is True,'Native option set/viewer differs')
        values=p['MATLAB_data'];require(len(values)==17 and all(type(x) in [float,int] and math.isfinite(x) for x in values),'Actual MATLAB data shape/nonfinite')
        for i,name in enumerate(NAMES):require(abs(values[i]-float(s['settings'][name]))<=abs(float(s['settings'][name]))*1e-14,'Actual MATLAB option differs from GUI/native')
        require(values[6]==80 and values[8]>0 and values[13:16]==[2.5*(s['initial']+3),-5,15],'Actual initial/bounds/status differs')
        n=p['native'];require(set(n)=={'Alpha','Cost','Constraint','Calls','Keep'} and all(math.isfinite(x) for x in n.values()),'Native result partition differs')
        require(n['Keep']==41 and n['Calls']>0 and n['Calls']==int(n['Calls']) and abs(n['Alpha']-s['optimum'])<1e-6
                and abs(n['Cost']-(s['optimum']-s['target'])**2)<1e-8,'Independent physical control/cost/sentinel differs')
        require(abs(n['Cost']-(n['Alpha']-s['target'])**2)<1e-10 and abs(n['Constraint']-(n['Alpha']-s['minimum']))<1e-12
                and abs(values[7]-n['Cost'])<1e-10 and abs(values[16]-2.5*(n['Alpha']+3))<1e-10,'Actual physical/MATLAB cost/constraint/scaling differs')
        require(abs(values[12]-max(0,s['minimum']-n['Alpha']))<1e-9,'Actual MATLAB constraint violation differs from physical residual')
        if profile=='TolX' and original:require(values[8]==4 and abs(n['Alpha'])<1e-12 and values[11]>1e-3,'Loose state tolerance not discriminating')
        if profile=='TolFun' and original:require(values[8]==1 and abs(n['Alpha'])<1e-12 and values[11]>1e-3,'Loose cost tolerance not discriminating')
        if profile=='TolCon':require(.049<values[12]<.051 if original else values[12]<1e-9,'Constraint tolerance did not distinguish physical violation')
        nominal=math.sqrt(sys.float_info.epsilon)*max(1,abs(2.5*(s['initial']+3)))
        probe=min(float(s['settings']['DiffMaxChange']),max(float(s['settings']['DiffMinChange']),nominal))
        require(abs(p['first_scaled_probe']-probe)<1e-13 and p['independent_probe']==probe,'Actual scaled finite-difference clamp differs')
        records=p['MATLAB_iterations'];require(len(records)>=2 and records[0]['state']=='init' and records[-1]['state']=='done','Actual OutputFcn history missing')
        last_iteration=last_count=-1
        for event in records:
            require(event['state'] in ['init','iter','done'] and len(event['x'])==1 and event['iteration']>=last_iteration and event['funccount']>=last_count,'Actual notification sequence differs')
            require(event['iteration']>=0 and event['iteration']==int(event['iteration']) and event['funccount']==int(event['funccount']),'Noninteger actual notification counter')
            physical=event['x'][0]/2.5-3
            require(abs(event['fval']-(physical-s['target'])**2)<1e-8 and abs(event['native_fval']-event['fval'])<1e-8,'Actual notification cost differs from physical formula')
            last_iteration,last_count=event['iteration'],event['funccount']
        require(last_iteration==values[9] and last_count==values[10],'Actual notification/fmincon counters differ')
        if directory:
            require(load(directory/(phase+'-MATLAB-iterations.json'))==records,'Raw actual MATLAB history differs')
            rows=[[float(v) for v in row.split(',')] for row in (directory/(phase+'-evaluations.csv')).read_text().strip().splitlines()]
            require(len(rows)==n['Calls'] and rows[0][0]==s['initial'],'Raw initial/count differs')
            first=None
            for i,row in enumerate(rows,1):
                require(len(row)==4 and all(math.isfinite(v) for v in row) and abs(row[1]-(row[0]-s['target'])**2)<1e-10
                        and abs(row[2]-(row[0]-s['minimum']))<1e-12 and row[3]==i,'Raw physical branch formula/sequence differs')
                if first is None and abs(row[0]-s['initial'])>1e-14:first=2.5*(row[0]-s['initial'])
            require(first==p['first_scaled_probe'],'First real GMAT perturbation differs from proof')
            final=[float(x) for x in (directory/(phase+'-report.csv')).read_text().strip().split(',')]
            require(len(final)==5 and all(abs(x-n[k])<1e-12 for x,k in zip(final,['Alpha','Cost','Constraint','Calls','Keep'])),'Raw/native final result differs')
        notifications+=len(records);evaluations+=int(n['Calls'])
    if directory:require(load(directory/'phase-results.json')==proof['phases'] and load(directory/'GUI-edits.json')==edits,'Raw phase/editor proof differs')
    return notifications,evaluations


def qualify(repo,path,inventory):
    q=load(path);require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Different/partial property-parent scope')
    require(all(q[k] is False for k in ['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','MATLAB_bundle_qualified']),'Global gate falsely closed')
    for p,h in q['artifact_sha256'].items():require(Path(p).is_file() and not Path(p).is_symlink() and sha(p)==h,'Changed artifact '+p)
    iteration.frozen.archive(q['reader_source_archive'],q['reader_source_sha256'])
    for p,h in q['reader_source_sha256'].items():require(sha(repo/p)==h,'Changed executing offline reader '+p)
    known={r['key']:r for r in inventory['requirements']};row=lambda k:dict(key=k,declared_id=known[k]['declared_id'],fingerprint=known[k]['fingerprint'],source_text=known[k]['text'])
    stage=Path(q['stage']);old=Path(q['reused_stage']);scope=load(q['scope_review']);before=load(q['prior_coverage']);overlay=load(q['prior_overlay'])
    require(q['authority_sha256']==inventory['sources']==scope['authority_sha256'] and len(known)==1158 and len(inventory['historical_cases'])==2367,'Original authority/denominators differ')
    require(scope['candidate_rows']==[row(k) for k in KEYS] and scope['property_children']==[row('FRR!E'+str(i)) for i in range(273,280)]
        and scope['prior_qualified_property_children']==[row('FRR!E273'),row('FRR!E274')] and scope['required_profiles']==PROFILES,'Original seven-property conjunction changed')
    require(before['coverage']['requirements']['verified']==1039 and before['coverage']['requirements']['total']==1158
        and before['coverage']['historical_cases']['verified']==1816 and before['requirements_still_needed']==62,'Prior exact union changed')
    remaining={r['key'] for r in before['remaining_original_rows']['requirements']}
    require(set(KEYS)<=remaining and not {'FRR!E273','FRR!E274','All Others!E139'}&remaining,'Missing/duplicate child credit')
    original={r['key']:r['original_row'] for r in overlay['reviewed_source_rows'] if r['key'] in KEYS}
    require(scope['retained_original_closure_records']==original and scope['original_bundle_gates_retained']=={k:r['closure_gate'] for k,r in original.items()}
        and scope['MATLAB_bundle_qualified'] is False and len(overlay['original_hold_keys_retained'])==59,'Original bundle/hold records lost')
    for p,h in load(old/'evidence-manifest-v1.json')['artifact_sha256'].items():require(sha(repo/p)==h,'Prior sealed evidence changed')
    e=iteration.Evidence(repo,stage,q['generated_startup_sha256']);proofs={};records=evaluations=recovery_attempts=0
    require(len(q['case_bindings'])==12 and {(b['mode'],b['profile']) for b in q['case_bindings']}==
        {(m,p) for m in ['headless','visible'] for p in PROFILES},'New profile/mode matrix incomplete')
    for b in q['case_bindings']:
        require(b['test']=='QtGui.NasaFminconOption'+b['profile'] and b['role']=='fmincon-option-test' and b['argument']==b['profile'],'Different registered option case')
        proof,d=e.context(b['run'],b['version'],b['mode'],b['test'],b['role'],b['argument']);a,c=check(proof,b['profile'],d)
        records+=a;evaluations+=c;recovery_attempts+=len(proof.get('range_recovery',[]));proofs[b['mode']+'-'+b['profile']]=proof
    reused=iteration.Evidence(repo,old,q['generated_startup_sha256']);reused_missions=reused_invalid=0
    require(len(q['reused_case_bindings'])==6 and {(b['mode'],b['component']) for b in q['reused_case_bindings']}==
        {(m,p) for m in ['headless','visible'] for p in ['creation-and-all-fields','IterationLimit','EvaluationLimit']},'Creation/all-seven-property join incomplete')
    for b in q['reused_case_bindings']:
        proof,d=reused.context(b['run'],b['version'],b['mode'],b['test'],b['role'],b.get('argument'))
        if b['component']=='creation-and-all-fields':
            require(b['role']=='fmincon-resource-test' and b['test']=='QtGui.NasaFminconResource','Different creator/field component')
            iteration.resource.check(proof,d);reused_invalid+=proof['invalid_rejections']
        else:
            require(b['role']=='fmincon-iteration-test' and b['argument']==b['component'] and b['test']=='QtGui.NasaFminconIteration'+b['component'],'Different existing limit child')
            iteration.check(proof,b['component'],d)
        reused_missions+=len(proof['phases'])
    qa=load(stage/'source-qa-v1.json');require(qa['core_paths']==259 and qa['core_source_changes']==0 and qa['unchanged_prior_core_paths']==259
        and not qa['public_API_or_layout_changed'] and not qa['numerical_oracle_bounds_relaxed'],'Unreviewed production/oracle increment')
    core=load(qa['core_inventory']);require(len(core['files'])==259 and sha(qa['core_inventory'])==qa['core_inventory_sha256'],'Different core inventory')
    for r in core['files']:require(sha(repo/r['path'])==r['working_sha256'],'Prior core source changed')
    for p,h in qa['production_sha256'].items():
        require(sha(repo/p)==h and all(hashlib.sha256(e.source(v)[p]).hexdigest()==h for v in [1,2])
            and hashlib.sha256(reused.source(4)[p]).hexdigest()==h,'Reused production source changed')
    for p,h in qa['old_test_sha256'].items():
        require(sha(repo/p)==h and all(hashlib.sha256(e.source(v)[p]).hexdigest()==h for v in [1,2])
            and hashlib.sha256(reused.source(4)[p]).hexdigest()==h,'Prior test/assertion source changed')
    require(qa['new_test_require_expressions_before']==43 and qa['new_test_require_expressions_after']==46
        and qa['all_original_new_test_require_expressions_preserved'] is True,'Initial new-test assertions lost')
    controls=[]
    mutations=[('missing-phase',lambda p:p['phases'].pop()),('missing-GUI-edit',lambda p:p['GUI_edits'].pop()),
        ('draft-not-isolated',lambda p:p['GUI_edits'][0].update(draft_isolated=False)),('Undo-Redo-lost',lambda p:p['GUI_edits'][0].update(exact_undo_redo=False)),
        ('unrelated-option-changed',lambda p:p['GUI_edits'][0]['after'].update(TolX='1')),
        ('wrong-native-settings',lambda p:p['phases'][1]['settings'].update(TolCon='0.1')),('wrong-MATLAB-option',lambda p:p['phases'][1]['MATLAB_data'].__setitem__(5,.1)),
        ('wrong-control',lambda p:p['phases'][1]['native'].update(Alpha=0)),('wrong-cost',lambda p:p['phases'][1]['native'].update(Cost=1)),
        ('wrong-constraint',lambda p:p['phases'][1]['native'].update(Constraint=1)),('wrong-sentinel',lambda p:p['phases'][1]['native'].update(Keep=40)),
        ('wrong-violation',lambda p:p['phases'][0]['MATLAB_data'].__setitem__(12,0)),('wrong-actual-counter',lambda p:p['phases'][1]['MATLAB_data'].__setitem__(10,99)),
        ('wrong-iteration-cost',lambda p:p['phases'][1]['MATLAB_iterations'][0].update(fval=1)),('wrong-native-iteration-cost',lambda p:p['phases'][1]['MATLAB_iterations'][0].update(native_fval=1)),
        ('missing-viewer',lambda p:p['phases'][1].update(ReportViewer=False))]
    for name,mutate in mutations:
        p=copy.deepcopy(proofs['headless-TolCon']);mutate(p)
        try:check(p,'TolCon')
        except (ValueError,KeyError,TypeError,IndexError):controls.append(name)
        else:raise ValueError('Corrupt option proof accepted: '+name)
    for profile in ['DiffMaxChange','DiffMinChange']:
        p=copy.deepcopy(proofs['headless-'+profile]);p['phases'][1]['first_scaled_probe']=p['phases'][0]['first_scaled_probe']
        try:check(p,profile)
        except ValueError:controls.append('ignored-'+profile)
        else:raise ValueError('Ignored actual perturbation option accepted')
    for field in ['reported_failed','Engine_recovered','caller_unchanged']:
        p=copy.deepcopy(proofs['headless-DiffRangeRecovery']);p['range_recovery'][0][field]=False
        try:check(p,'DiffRangeRecovery')
        except ValueError:controls.append('lost-range-'+field)
        else:raise ValueError('Lost range rejection/recovery accepted')
    require(records==270 and evaluations==990 and recovery_attempts==4 and reused_missions==18 and reused_invalid==98 and len(controls)==21,'Exact runtime/reuse/control census differs')
    q['audit_summary']=dict(qualified_keys=KEYS,new_GUI_case_contexts=12,new_positive_fmincon_missions=60,expected_failed_range_attempts=4,
        actual_MATLAB_notifications=records,new_GMAT_branch_evaluations=evaluations,reused_GUI_contexts=6,reused_positive_missions=18,reused_atomic_invalid_inputs=reused_invalid,
        case_source_bindings=e.contexts,reused_case_source_bindings=reused.contexts,corrupt_proof_controls=controls,
        visible_native_exposures=sum(c['visible_exposures'] for c in e.contexts),all_prior_core_paths_unchanged=259,remaining_optimizer_rows=0,
        retained_original_hold_identities=59,MATLAB_bundle_qualified=False)
    return q,KEYS


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--qualification',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    repo=Path(__file__).resolve().parents[3];q,keys=qualify(repo,a.qualification,nasa.inventory(repo));require(not a.output.exists(),'Audit output exists')
    a.output.write_text(json.dumps(dict(status='passed',qualified_keys=keys,summary=q['audit_summary']),indent=2)+'\n');print(json.dumps(q['audit_summary']))
