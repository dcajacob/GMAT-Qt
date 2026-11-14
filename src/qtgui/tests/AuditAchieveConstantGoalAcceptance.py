"""Admit only finite numeric measured Achieve Goal support under FRC!E97."""
import argparse,copy,hashlib,json,math,tarfile,xml.etree.ElementTree as ET
from pathlib import Path
import NasaGuiCoverage as nasa
import BuildNasaCoverage95 as builder
import BuildNasaCoverageExtension as common
load,sha,require=common.load,common.sha,common.require
KEYS=['FRC!E97'];CONTEXT=['FRC!E'+str(i) for i in range(90,98)]
KIND='source-versioned-complete-finite-numeric-Achieve-measured-Goal'
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','native_OS_input_qualified']
CASES=['VariableDefault','VariableFalse','LiteralDefault','LiteralFalse','TrueFraction','TrueScientific','TrueInconsistent','TrueOverflow']
CLAUSES={'exact-original-Achieve-control-context','generic-finite-number-wrapper-path','default-and-explicit-false-compatibility','literal-measured-Goal-and-distinct-desired-value','satisfied-and-inconsistent-target-results','overflow-rejection','policy-copy-and-serialization','actual-option-and-Goal-selector','invalid-Apply-OK-and-Cancel-history','exact-source-UndoRedo-Unicode-reopen','paired-visible-runtime','protected-originals-and-open-parent'}

def structure(q,s,inventory):
    known={r['key']:r for r in inventory['requirements']}
    require(q['contract_kind']==KIND and q['qualified_keys']==KEYS and q['authority_sha256']==inventory['sources'],'Extra/partial/changed numeric Achieve scope')
    require(s['candidate_rows']==[known[k] for k in KEYS] and s['complete_original_context']==[known[k] for k in CONTEXT],'Changed original Achieve context')
    require(s['native_cases']==CASES and s['profiles']==['headless','visible'] and s['operand']=='measured Goal (left side)' and s['default_allow_constant_goal'] is False,'Incomplete numeric measured-Goal policy matrix')
    require(set(s['clauses'])==CLAUSES and all(v['satisfied'] and v['evidence'] for v in s['clauses'].values()),'Incomplete whole numeric Achieve conjunction')
    require(s['result_bound']==1e-8 and s['goal_tolerance']==1e-10 and s['original_assertions_changed']==s['original_fixture_or_tolerance_changes']==s['other_row_credit']==s['runtime_binary_copies']==0,'Changed original assertion/bound or extra numeric credit')
    require(not s['all_Achieve_types_qualified'] and not s['literal_Vary_controls_qualified'] and not s['original_failures_waived'] and s['newly_resolved_hold_keys']==[],'Broader control type or formal holds promoted')
    require(all(q[k] is False and s[k] is False for k in GATES),'Broader numeric Achieve qualification promoted')

def damaged(q,s,inventory):
    rejected=[]
    for fault in ['extra-row','fingerprint','missing-context','missing-native-case','missing-profile','RHS-substitution','changed-default','missing-clause','changed-bound','extra-credit','waived-failures','broader-gate']:
        a,b=copy.deepcopy(q),copy.deepcopy(s)
        if fault=='extra-row':a['qualified_keys'].append('FRC!E93')
        elif fault=='fingerprint':b['candidate_rows'][0]['fingerprint']='0'*64
        elif fault=='missing-context':b['complete_original_context'].pop()
        elif fault=='missing-native-case':b['native_cases'].pop()
        elif fault=='missing-profile':b['profiles'].pop()
        elif fault=='RHS-substitution':b['operand']='desired value (right side)'
        elif fault=='changed-default':b['default_allow_constant_goal']=True
        elif fault=='missing-clause':b['clauses'].pop(next(iter(b['clauses'])))
        elif fault=='changed-bound':b['result_bound']=1
        elif fault=='extra-credit':b['other_row_credit']=1
        elif fault=='waived-failures':b['original_failures_waived']=True
        else:a['full_NASA_qualified']=True
        try:structure(a,b,inventory)
        except ValueError:rejected.append(fault)
        else:raise ValueError('Damaged numeric Achieve admission accepted: '+fault)
    return rejected

def component(stage):
    audit=load(stage/'component-audit-v1.json');require(audit['status']=='passed','Numeric Achieve component failed')
    source=load(stage/'source-final-before-v1.json');require(source==load(stage/'source-final-after-v1.json'),'Numeric Achieve owning sources changed during execution')
    binaries=load(stage/'binary-final-before-v1.json');require(binaries==load(stage/'binary-final-after-v1.json'),'Numeric Achieve runtime changed during execution')
    head=load(stage/'gui-headless-v1/results.json');visible=load(stage/'gui-visible-v1/results.json');require(head['source_hashes']==visible['source_hashes'] and head['binary_hashes']==visible['binary_hashes'],'Numeric Achieve GUI source/runtime mismatch')
    versions=load(stage/'source-versions-v1.json');recorded={}
    with tarfile.open(stage/'sources-v1.tar.gz') as bundle:
        require(set(bundle.getnames())=={'sha256/'+h for h in versions},'Changed numeric Achieve source partition')
        for h,names in sorted(versions.items()):
            raw=bundle.extractfile('sha256/'+h).read();require(not raw.startswith(b'\x7fELF') and hashlib.sha256(raw).hexdigest()==h,'Changed/non-source numeric capsule member')
            for n in names:recorded.setdefault(n,{})[h]=raw
    for inventory in [source,head['source_hashes']]:
        for n,h in inventory.items():require(h in recorded.get(n,{}) or audit['ELF_identity_only'].get(n)==h,'Missing executed numeric Achieve source version')
    def text(n):
        h=source.get(n,head['source_hashes'].get(n));return recorded[n][h].decode()
    code=text('src/base/command/Achieve.cpp')
    for token in ['allowConstantGoal       (false)','toWrapper->GetWrapperType() == Gmat::NUMBER_WT && !allowConstantGoal','goal->GetWrapperType() == Gmat::NUMBER_WT','std::isfinite(goal->EvaluateReal())','const Real measuredValue = goal->EvaluateRealForCalculation();','if (allowConstantGoal || constantGoalOptionSpecified)','if (id == achievedValueID)\n      return achievedValue;']:
        require(token in code,'Missing generic finite numeric Goal/policy/getter path: '+token)
    form=text('src/qtgui/CommandForm.cpp');dialog=text('src/qtgui/ReportParameterDialog.cpp')
    for token in ['"commandCheck_"+name','"AllowConstantGoal"','selectionMode=ReportParameterDialog::Mode::NumericSingle;']:require(token in form,'Missing actual numeric Goal option/selector path')
    require('(literal && number && std::isfinite(real))' in dialog,'Numeric Goal selector lost finite-real guard')
    rows=load(stage/'native-final-v3/results.json')['cases'];require([r['case'] for r in rows]==CASES,'Missing/reordered numeric Goal native cases')
    baseline=load(stage/'native-diagnostic-v2/results.json')['cases'];first=load(stage/'native-v1/results.json')['cases'];require(sum(r['passed'] for r in baseline)==sum(r['passed'] for r in first)==3,'Getter baseline failures lost')
    native_scalars=0;images=None
    for r in rows:
        name=r['case'];case=stage/'native-final-v3'/name;p=load(case/'proof.json');require(r['passed'] and r['returncode']==0 and r['proof']==p and p['passed'] and p['built']==p['expected_build'],'Failed actual numeric Goal native case')
        current=load(case/'native-image-identities.json')
        if images is None:images=current
        else:require(images==current,'Numeric Goal loaded native images differ')
        require(r['binary_sha256']==current[r['command'][0]]['sha256'] and r['startup_sha256']==sha(r['command'][1]),'Unbound actual native numeric Goal command')
        for previous in ['native-v1','native-diagnostic-v2']:
            old=stage/previous/name;require((old/'mission.script').read_text().replace(str(old),'@CASE@')==(case/'mission.script').read_text().replace(str(case),'@CASE@'),'Native numeric Goal inputs/bounds changed')
        if not p['expected_build']:
            require(not p['built'] and p['run_status']==-99 and not p['values'] and ('must be finite' if name=='TrueOverflow' else 'excluding numbers') in '\n'.join(p['errors']),'Invalid/default-false numeric Goal was accepted or rejected for another reason')
            continue
        variable=name.startswith('Variable');inconsistent=name=='TrueInconsistent';policy=p['policy'];measured=9. if variable else -.25 if name=='TrueScientific' else 1.25
        require(policy['achieved_read_only'] and policy['achieved_type_real'] and policy['clone_and_assignment'] and not policy['GetRealParameter_error'] and policy['GetNumber']==p['measured'],'Native numeric Goal readout/copy contract failed')
        require(policy['allow']==name.startswith('True') and policy['explicit_option']==(name!='VariableDefault'),'Default/explicit numeric Goal policy changed')
        require(('AllowConstantGoal' in policy['generated'])==policy['explicit_option'],'Omitted/explicit policy serialization changed')
        if policy['explicit_option']:require(('AllowConstantGoal = true' if policy['allow'] else 'AllowConstantGoal = false') in policy['generated'],'Wrong serialized numeric Goal policy')
        goal='Quantity' if variable else '-2.5e-1' if name=='TrueScientific' else '1.25';desired='9' if variable else '-0.25' if name=='TrueScientific' else '2.25' if inconsistent else '1.25'
        require(policy['measured_goal']==goal and policy['desired']==desired,'Measured Goal was substituted with desired RHS')
        truth={'U':1.01 if inconsistent else 4. if variable else 1.,'Quantity':3.02 if inconsistent else 9. if variable else 3.,'Keep':41.,'After':0. if inconsistent else 7.};require(set(p['values'])==set(truth),'Incomplete native numeric Goal result')
        for k,w in truth.items():require(math.isfinite(p['values'][k]) and abs(p['values'][k]-w)<=(0 if k in ['Keep','After'] else 1e-8),'Independent native numeric Goal scalar failed');native_scalars+=1
        require(abs(p['measured']-measured)<1e-8,'Independent measured numeric Goal failed');native_scalars+=1
        if inconsistent:require(p['run_status']==-5 and p['solver_status']==4 and not p['converged'],'Inconsistent constant target falsely converged')
        else:require(p['run_status']==1 and p['solver_status']==1 and p['converged'],'Satisfied numeric/reference target did not converge')
        old=next(r['proof'] for r in baseline if r['case']==name);require(old['policy']['GetNumber']==p['measured'] and old['values']==p['values'] and 'Cannot get real parameter' in old['policy']['GetRealParameter_error'],'Getter repair changed actual numerical behavior or discarded failure evidence')
    roots=[rows[0]['command'][0][:-len(k)-1] for k in binaries if rows[0]['command'][0].endswith('/'+k)];require(len(roots)==1,'Native numeric Goal executable not in guard');root=roots[0]+'/'
    for n,v in images.items():
        if n.startswith(root):require(binaries.get(n.removeprefix(root))==v['sha256'],'Loaded native numeric Goal project image differs from guard')
    junit=list(ET.parse(stage/'native-final-v3/ctest.xml').iter('testcase'));require(len(junit)==1 and junit[0].get('name')=='QtAcceptance.AchieveConstantGoal' and all(junit[0].find(k) is None for k in ['failure','error','skipped']),'Native numeric Goal CTest failed')
    selected=lambda d:{r['name']:r for r in d['results'] if r['status']!='not-run'}
    require(set(selected(head))==set(selected(visible))=={'QtGui.AchieveConstantGoal','QtGui.NasaAchieveParameter'} and visible['display_route']=='nested' and visible['resizable'] and visible['max_screenshots']==0,'Incomplete paired numeric Goal GUI delivery')
    GUI_scalars=original_scalars=exposures=0
    phases=['default-reference','literal-0','reopened-0','literal-1','reopened-1','explicit-false-reference']
    for profile,d in [('gui-headless-v1',head),('gui-visible-v1',visible)]:
        for name,r in selected(d).items():
            case=stage/profile/name;p=load(case/'proof.json');require(p['outcome']=='passed' and r['status']=='passed' and r['returncode']==0 and not r['trace']['screenshots'],'Failed actual numeric Goal GUI execution')
            if profile=='gui-visible-v1':require(r['trace']['native_exposure_events']>0 and r['trace']['recorded_platforms']==['xcb'],'Numeric Goal GUI not exposed natively')
            else:require(r['trace']['native_exposure_events']==0,'Numeric Goal unattended mode changed')
            exposures+=r['trace']['native_exposure_events'];junit=list(ET.parse(case/'ctest.xml').iter('testcase'));require(len(junit)==1 and junit[0].get('name')==name and all(junit[0].find(k) is None for k in ['failure','error','skipped']),'Numeric Goal GUI CTest did not pass')
            if name=='QtGui.AchieveConstantGoal':
                require(p['checkpoint']=='numeric-measured-Achieve-Goal' and p['original_assertion_changes']==0 and [x['phase'] for x in p['runs']]==phases,'Missing actual numeric Goal mission phases')
                require(p['operations']==['default-false-and-draft-cancel','literal-0-edit-history-reopen','literal-1-edit-history-reopen','explicit-false-reference-compatible'],'Missing numeric Goal editor transaction coverage')
                expected=[('1.25',False,False),('nan',False,False),('inf',False,False),('1e309',False,False),('1.25',True,True),('1.25',False,False),('-2.5e-1',True,True),('-2.5e-1',False,False)]
                require([(x['value'],x['allowed'],x['accepted']) for x in p['picker_checks']]==expected,'Missing/wrong actual measured Goal selector policy')
                require([(x['value'],x['action']) for x in p['rejections']]==[(v,a) for v in ['1.25','1e309','1.25','-2.5e-1'] for a in ['Apply','OK']] and all(x['status'] for x in p['rejections']),'Invalid Goal Apply/OK was not observed')
                initial=(case/'initial.script').read_text();old="Achieve 'Goal Δ' DC(Quantity = 9, {Tolerance = 0.0000000001});";require(initial.count(old)==1,'Wrong original reference Goal')
                for index,run in enumerate(p['runs']):
                    constant=index in [1,2,3,4];measured=-.25 if index in [3,4] else 1.25 if constant else 9.;truth=[1. if constant else 4.,3. if constant else 9.,41.,7.];raw=[float(x) for x in (case/(run['phase']+'-values.csv')).read_text().strip().split(',')]
                    require(run['constant']==constant and run['expected']==truth and raw==run['values'] and len(raw)==4,'Wrong actual numeric Goal report')
                    for i,(v,w) in enumerate(zip(raw,truth)):require(math.isfinite(v) and abs(v-w)<=(1e-8 if i<2 else 0),'Independent GUI numeric Goal report failed');GUI_scalars+=1
                    require(abs(run['measured']-measured)<1e-8,'Independent GUI measured numeric Goal failed');GUI_scalars+=1
                    if constant:
                        goal='-2.5e-1' if index in [3,4] else '1.25';desired='-0.25' if index in [3,4] else '1.25';statement="Achieve 'Goal Δ' DC("+goal+' = '+desired+', {Tolerance = 0.0000000001, AllowConstantGoal = true});';require(run['source']==initial.replace(old,statement),'Wrong measured Goal source or Unicode round trip')
                    elif index==0:require(run['source']==initial,'Default Goal source changed')
                    else:require(run['source']==initial.replace(old,"Achieve 'Goal Δ' DC(Quantity = 9, {Tolerance = 0.0000000001, AllowConstantGoal = false});"),'Explicit false reference compatibility source changed')
            else:
                require(p['checkpoint']=='achieve-scalar-parameter' and p['numeric_sentinel']==42 and p['requested_tolerance']==1e-9 and len(p['reports'])==2,'Original reference regression scope changed')
                for run in p['reports']:
                    variable=run['phase']=='variable';truth=[4.,9.,1012.,42.] if variable else [2.,5.,1006.,42.];raw=[float(x) for x in (case/(run['phase']+'.csv')).read_text().strip().split(',')];require(raw==run['values'] and len(raw)==4,'Original raw Achieve report differs')
                    for i,(v,w) in enumerate(zip(raw,truth)):require(math.isfinite(v) and abs(v-w)<=(1e-8 if i<3 else 0),'Independent original Achieve regression failed');original_scalars+=1
                    desired=9. if variable else 1006.;require(run['desired']==desired and run['achieved']==raw[1 if variable else 2] and abs(run['achieved']-desired)<=1e-9,'Original requested Achieve residual changed')
    require((native_scalars,GUI_scalars,original_scalars,exposures)==(25,60,16,58),'Incomplete numeric Goal result census')
    require(audit['native_cases']==8 and audit['native_successful_solves']==4 and audit['native_required_build_rejections']==3 and audit['native_required_inconsistent_failure']==1 and audit['GUI_passes']==4 and audit['original_protected_inputs']==1334 and audit['original_assertion_changes']==audit['numerical_bound_changes']==0,'Numeric Goal component fidelity changed')
    return dict(native_cases=8,successful_native_solves=4,required_native_rejections=3,required_inconsistent_failure=1,GUI_passes=4,native_scalars=25,GUI_scalars=60,original_GUI_scalars=16,visible_native_exposures=58)

def qualify(repo,path,inventory):
    q=load(path)
    for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed numeric Achieve artifact: '+n)
    s=load(q['scope_review']);structure(q,s,inventory)
    for archive,ids in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
        require(q[archive] in q['artifact_sha256'],'Unbound numeric Achieve source archive')
        with tarfile.open(q[archive]) as bundle:
            require(set(bundle.getnames())==set(q[ids]),'Numeric Achieve admission source partition differs')
            for n,h in sorted(q[ids].items()):require(hashlib.sha256(bundle.extractfile(n).read()).hexdigest()==h,'Changed numeric Achieve admission source')
    protected=load(q['protected_input_baseline']);require(len(protected)==1334 and all(q['proof_source_sha256'].get(n)==h for n,h in protected.items()),'Original protected numeric Achieve inputs omitted')
    prior=load(q['prior_overlay']);rows={r['key']:r for r in prior['reviewed_source_rows']};require(len(prior['original_hold_keys_retained'])==59 and not set(KEYS)&set(prior['original_hold_keys_retained']) and rows[KEYS[0]]['whole_row_status']=='open','Original numeric Goal gap or formal holds changed')
    full={r['original_source']['key']:r for r in load(q['prior_full_audit'])['rows']};require(full['FRC!E93']['status']=='open-original-scope','Whole Achieve type parent substituted')
    proof=component(Path(q['component_stage']));require(len(damaged(q,s,inventory))==12,'Incomplete damaged numeric Goal admission controls')
    return q,KEYS,dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,component=proof,new_requirement_credit=1,newly_resolved_original_hold_keys=[],damaged_admission_controls_rejected=12,native_tests_rerun=0,runtime_binary_copies=0,**{k:False for k in GATES})

def build(repo,baseline,path,output):
    seal,inventory,keys=builder.baseline(repo,baseline);original={k:set(v) for k,v in keys.items()};_,incoming,audit=qualify(repo,path,inventory);known={k:{r['key']:r for r in inventory[k]} for k in keys}
    require(not set(incoming)&keys['requirements'],'Duplicate numeric Achieve credit');keys['requirements']=common.merge_keys(keys['requirements'],incoming,known['requirements'])
    coverage={k:dict(verified=len(keys[k]),total=len(known[k]),percent=round(100*len(keys[k])/len(known[k]),1),new_verified=len(keys[k])-len(original[k])) for k in keys};remaining={k:[r for r in seal['remaining_original_rows'][k] if r['key'] not in keys[k]] for k in keys}
    result=dict(status='passed',scope='95-percent campaign: sealed source-versioned prefix plus complete original finite numeric measured Achieve Goal rule',baseline=str(baseline),baseline_sha256=sha(baseline),authority_sha256=inventory['sources'],coverage=coverage,target_requirements=1101,target_reached=len(keys['requirements'])>=1101,requirements_still_needed=max(0,1101-len(keys['requirements'])),additions=[dict(key=k,kind='requirements',fingerprint=known['requirements'][k]['fingerprint'],source_text=known['requirements'][k]['text'],qualification=str(path),qualification_sha256=sha(path),status='verified-source-versioned') for k in incoming],remaining_counts={k:len(v) for k,v in remaining.items()},remaining_original_rows=remaining,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,limits=['Original 1158/2367 denominators and exact source rows remain unchanged.','Only FRC!E97 is newly admitted; FRC!E93 broad Achieve type scope and all59 separate formal holds retain their prior status.','Scientific, physical frame/ephemeris, external ABI, platform/native OS input and full current-build approvals remain separate.'])
    with Path(output).open('x') as f:json.dump(result,f,indent=2);f.write('\n')
    return result,audit

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);p.add_argument('--baseline',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();repo=a.repo.resolve()
    if a.baseline:
        result,_=build(repo,a.baseline,a.qualification,a.output);print(json.dumps({'coverage':result['coverage'],'requirements_still_needed':result['requirements_still_needed']}))
    else:
        _,_,result=qualify(repo,a.qualification,nasa.inventory(repo))
        with a.output.open('x') as f:json.dump(result,f,indent=2);f.write('\n')
        print(json.dumps(result))
