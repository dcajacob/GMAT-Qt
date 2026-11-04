"""Audit complete original optimizer selection and five-setting Vary conjunctions."""
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
import BuildNasaCoverageExtension as common

KIND='source-versioned-fmincon-GUI-selection-and-Vary-settings'
KEYS=['FRC!E14','FRC!E16']+['FRC!E'+str(i)for i in range(35,41)]
PROFILES=['Initial','Lower','Upper','Additive','Multiplicative','Combined','References','SelectFmincon','SelectVF13ad']
PREFIX='QtGui.NasaFminconVary'
sha,load,require=common.sha,common.load,common.require


def control(initial=0,lower=-5,upper=5,additive=0,multiplier=1):
    return dict(initial=initial,lower=lower,upper=upper,additive=additive,multiplier=multiplier)


def prescribed(profile,phase):
    a=control();b=control()
    if profile=='Lower':a['initial']=3.5
    if phase!='baseline':
        if profile=='Initial':a['initial']=-1.25
        if profile=='Lower':a['lower']=3
        if profile=='Upper':a['upper']=1.5
        if profile=='Additive':a['additive']=3
        if profile=='Multiplicative':a['multiplier']=2.5
        if profile=='Combined':a=control(initial=-1,additive=-3,multiplier=.25);b=control(initial=2,additive=4,multiplier=2)
        if profile=='References':a=control(initial=-1.25,lower=-4,upper=7000,additive=3,multiplier=.5)
    return[a,b]if profile=='Combined'else[a]


def check(proof,profile,directory=None):
    selection=profile.startswith('Select');vf=profile=='SelectVF13ad';combined=profile=='Combined'
    require(proof['checkpoint']=='fmincon-vary-and-solver-selection'and proof['profile']==profile
        and proof['outcome']=='passed'and proof['new_requirement_credit']==0,'Wrong profile/automatic credit')
    require([p['phase']for p in proof['phases']]==(['gui-selected','reopened','script']if selection else['baseline','gui','reopened','script']),
        'Incomplete baseline/GUI/reopen/script conjunction')
    if selection:
        require(proof['invalid_inputs']==[] and proof['picker_visits']==[dict(accepted=a,chosen='ChosenVF'if vf else'ChosenF',
            pool=['ChosenF','ChosenVF','OldF'])for a in[False,True]],'Actual conditional optimizer picker Cancel/accept/pool differs')
    else:
        field={'Initial':'Initial value','Lower':'Lower','Upper':'Upper','Additive':'AdditiveScaleFactor',
            'Multiplicative':'MultiplicativeScaleFactor','Combined':'MultiplicativeScaleFactor','References':'Initial value'}[profile]
        inputs=['','not-a-number','nan','inf']+(['0','-1']if field=='MultiplicativeScaleFactor'else[])
        require(proof['picker_visits']==[]and[(r['field'],r['input'])for r in proof['invalid_inputs']]==[(field,v)for v in inputs]
            and all(r['rejected']is True and r['diagnostic']for r in proof['invalid_inputs']),'Atomic invalid correction incomplete')
    calls=0
    for p in proof['phases']:
        controls=prescribed(profile,p['phase']);require(p['controls']==controls,'Prescribed GUI control values differ')
        a=controls[0];x=1.5 if combined else min(a['upper'],max(a['lower'],2));y=-1.5 if combined else 0
        truth=dict(Alpha=x,Beta=y,Cost=.5 if combined else(x-2)**2,Constraint=0 if combined else x-1,Keep=41)
        n=p['native'];require(set(n)==set(truth)|{'Calls'},'Native field partition differs')
        for name,value in truth.items():
            require(math.isfinite(n[name])and(n[name]==value if name=='Keep'else abs(n[name]-value)<(1e-8 if name=='Cost'else 1e-6)),
                'Independent physical optimum differs: '+name)
        require(p['solver_type']==('VF13ad'if vf else'FminconOptimizer')and p['solver_name']==('ChosenVF'if vf else'ChosenF')
            and p['report_viewer']is True,'Actual solver binding/viewer missing')
        require(n['Calls']>=3 and n['Calls']==int(n['Calls']),'Invalid actual evaluation census');calls+=n['Calls']
        require(len(p['native_scaled'])==len(controls),'Native control dimension differs')
        for i,c in enumerate(controls):
            physical=n['Alpha'if i==0 else'Beta']
            require(abs(p['native_scaled'][i]-c['multiplier']*(physical+c['additive']))<1e-9,'Native scaling differs from m*(physical+a)')
        if vf:require(p['MATLAB_scaled_arrays']==[]and p['native_solver_status']==1,'Actual selected VF13ad did not converge')
        else:
            require(len(p['MATLAB_scaled_arrays'])==len(controls),'Actual MATLAB dimension differs')
            for i,c in enumerate(controls):
                physical=n['Alpha'if i==0 else'Beta'];expected=[c['multiplier']*(v+c['additive'])for v in[c['initial'],c['lower'],c['upper'],physical]]
                require(len(p['MATLAB_scaled_arrays'][i])==4 and all(math.isfinite(v)and abs(v-e)<1e-9 for v,e in zip(p['MATLAB_scaled_arrays'][i],expected)),
                    'Actual MATLAB initial/bounds/final arrays differ from independent physical transform')
        if directory:
            phase=p['phase'];rows=(directory/(phase+'-evaluations.csv')).read_text().strip().splitlines()
            require(len(rows)==n['Calls'],'Raw branch count differs')
            for index,row in enumerate(rows,1):
                v=[float(x)for x in row.split(',')];require(len(v)==5 and all(math.isfinite(x)for x in v),'Raw branch shape/nonfinite')
                cost=(v[0]-2)**2+((v[1]+1)**2 if combined else 0);constraint=v[0]+v[1]if combined else v[0]-1
                require(abs(v[2]-cost)<1e-10 and abs(v[3]-constraint)<1e-12 and v[4]==index,'Raw objective/constraint/sequence differs')
                if index==1:require(abs(v[0]-controls[0]['initial'])<1e-12 and(not combined or abs(v[1]-controls[1]['initial'])<1e-12),'Raw physical initial controls differ')
            raw=[float(v)for v in(directory/(phase+'-report.csv')).read_text().strip().split(',')]
            require(len(raw)==6 and all(abs(v-n[k])<1e-12 for v,k in zip(raw,['Alpha','Beta','Cost','Constraint','Calls','Keep'])),'Raw/native final report differs')
            source=directory/(phase+'-caller.script')
            if not source.exists():source=directory/('original.script'if phase=='baseline'else'accepted.script')
            text=source.read_text();require("Optimize 'Chosen branch' "+p['solver_name']in text and'% Preserve Vary caller tail Ω'in text,'Actual caller identity/tail differs')
            if profile=='References'and phase in['gui','reopened']:
                require(all(v in text for v in['Alpha = Guess','Lower = Limits(1,1)','Upper = Sat.EarthMJ2000Eq.X',
                    'AdditiveScaleFactor = Addend','MultiplicativeScaleFactor = Factor']),'Actual reference-valued GUI settings missing')
    if directory:
        require(load(directory/'phase-results.json')==proof['phases'],'Raw phase proof differs')
        if selection:require(load(directory/'picker-visits.json')==proof['picker_visits'],'Actual picker visits differ')
        else:
            require(load(directory/'invalid-inputs.json')==proof['invalid_inputs'],'Actual invalid controls differ')
            cap=load(directory/'capability.json');require([r['field']for r in cap]==['Initial value','Lower','Upper','AdditiveScaleFactor','MultiplicativeScaleFactor']
                and all(r['enabled']is True for r in cap),'All five actual Vary inputs not enabled')
    return int(calls)


def legacy(proof,test,directory):
    require(proof['outcome']=='passed','Affected original regression failed')
    property_case=test.endswith('VaryOptimizeProperty');rows=load(directory/'results-values.json')
    expected_phases=['gui','reopened']if property_case else['baseline','gui','undo','redo','reopened','script']
    require([r['phase']for r in rows]==expected_phases and proof['phases']==len(rows),'Original regression phases lost')
    for r in rows:
        require(r['native_status']==1 and len(r['final'])==1 and abs(r['final'][0][0]-2)<1e-6
            and abs(r['final'][0][1]-1)<2e-6 and r['final'][0][2]==41,'Original VF13ad analytic result/status differs')
        for v in r['raw_trials']:require(len(v)==3 and abs(v[1]-(v[0]-3)**2)<1e-8 and v[2]==41,'Original raw VF13ad quadratic/sentinel differs')
        require(abs(r['native_scaled']-r['multiplicative']*(r['final'][0][0]+r['additive']))<1e-7,'Original VF13ad scaling differs')
        actual=[[float(v)for v in line.split(',')]for line in(directory/(r['phase']+'.trials.csv')).read_text().strip().splitlines()]
        require(actual==r['raw_trials'],'Raw VF13ad trace differs from original proof')
    if property_case:
        require(proof['backend']=='VF13ad'and proof['property']=='Sat.EarthMJ2000Eq.X'and proof['invalid_rejections']==10,'Original property picker scope changed')
        for r in load(directory/'native-controls.json'):
            require(r['variable']=='Sat.EarthMJ2000Eq.X'and r['status']==1 and r['configured']==[7000,-4]
                and r['unused_variable']==-4 and abs(r['physical']-2)<1e-6,'Original native property/configured isolation differs')
    return len(rows)


def qualify(repo,path,inventory):
    q=load(path);require(q['contract_kind']==KIND and q['qualified_keys']==KEYS,'Different/partial original conjunction')
    require(all(q[k]is False for k in['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','MATLAB_bundle_qualified']),
        'Global gate falsely closed')
    for p,h in q['artifact_sha256'].items():require(Path(p).is_file()and not Path(p).is_symlink()and sha(p)==h,'Changed artifact '+p)
    frozen.archive(q['reader_source_archive'],q['reader_source_sha256'])
    for module in[sys.modules[__name__],nasa,frozen,common]:
        p='src/qtgui/tests/'+Path(module.__file__).name;require(sha(repo/p)==q['reader_source_sha256'][p],'Executing reader source differs')
    known={r['key']:r for r in inventory['requirements']};row=lambda k:dict(key=k,declared_id=known[k]['declared_id'],fingerprint=known[k]['fingerprint'],source_text=known[k]['text'])
    scope=load(q['scope_review']);before=load(q['prior_coverage']);overlay=load(q['prior_overlay']);stage=Path(q['stage'])
    require(q['authority_sha256']==inventory['sources']==scope['authority_sha256']and len(known)==1158 and len(inventory['historical_cases'])==2367,'Original authority changed')
    require(scope['candidate_rows']==[row(k)for k in KEYS]and scope['selection_children']==[row('FRC!E15'),row('FRC!E16')]
        and scope['vary_children']==[row('FRC!E'+str(i))for i in range(36,41)]and scope['required_profiles']==PROFILES,'Original full parent/child scope changed')
    require(before['coverage']['requirements']['verified']==1023 and before['coverage']['requirements']['total']==1158
        and before['coverage']['historical_cases']['verified']==1816 and before['requirements_still_needed']==78,'Prior exact union changed')
    remaining={r['key']for r in before['remaining_original_rows']['requirements']};require(set(KEYS)<=remaining and'FRC!E15'not in remaining,'Missing/duplicate old child credit')
    original={r['key']:r['original_row']for r in overlay['reviewed_source_rows']if r['key']in KEYS}
    require(scope['retained_original_closure_records']==original and scope['original_bundle_gates_retained']=={k:r['closure_gate']for k,r in original.items()}
        and scope['MATLAB_bundle_qualified']is False and len(overlay['original_hold_keys_retained'])==59,'Original bundle/hold history lost')
    qa=load(stage/'source-qa-v1.json');require(qa['core_source_changes']==0 and qa['core_paths']==257 and not qa['numerical_oracle_bounds_relaxed'],'Core/oracle gate changed')
    core=load(qa['core_inventory']);require(len(core['files'])==257,'Core inventory changed')
    for r in core['files']:require(sha(repo/r['path'])==r['working_sha256'],'Core worktree changed')
    caches={};sources={};contexts=[];proofs={};counts=dict(fmincon=0,VF13ad=0,legacy=0,evaluations=0,invalid=0)
    def source(version):
        if version not in sources:sources[version]=frozen.archive(stage/f'executed-sources-v{version}.tar.gz',load(stage/f'executed-source-sha256-v{version}.json'))
        return sources[version]
    def context(name,version,mode,test,role,argument=None):
        if name not in caches:
            receipt=load(stage/name/'results.json');mapped=load(stage/(name+'-loaded-images.json'));ids=load(stage/f'executed-source-sha256-v{version}.json');source(version)
            require(receipt['mode']==mode and receipt.get('finished_utc')and receipt['end_reason']=='completed-selection'
                and mapped['returncode']==0 and mapped['inputs_unchanged']is True and mapped['runtime_binary_copies']==0,'Nonterminal/failed/mutated selection')
            require(mapped['source_sha256']==ids and all(ids.get(p)==h for p,h in receipt['source_hashes'].items()),'Executed source custody incomplete')
            require(receipt['startup_sha256']==q['generated_startup_sha256'],'Different generated startup')
            union={}
            for process in mapped['processes']:
                for snap in process['snapshots']:
                    require(hashlib.sha256(snap['maps'].encode()).hexdigest()==snap['maps_sha256'],'Raw map differs')
                    paths=set()
                    for line in snap['maps'].splitlines():
                        fields=line.split(None,5)
                        if len(fields)==6 and(fields[5].startswith(str(repo)+'/')or fields[5].startswith('/usr/local/MATLAB/R2026b/')):paths.add(fields[5])
                    require(paths==set(snap['project_images']),'Mapped image partition differs');union.update(snap['project_images'])
            require(union==mapped['loaded_project_images'],'Mapped image union differs')
            if mode=='visible':
                d=receipt['visible_display'];require(receipt['max_screenshots']==0 and d['resizable']and d['start_minimized']and'Iconic'in d['initial_host_window_state']
                    and'_NET_WM_STATE_FULLSCREEN'not in d['initial_host_window_state'],'Visible window policy missing')
            caches[name]=receipt,mapped
        receipt,mapped=caches[name];result=next(r for r in receipt['results']if r['name']==test)
        require(result['status']=='passed'and result['returncode']==0,'Selected native case failed')
        directory=Path(result['log']).parent;xml=list(ET.parse(directory/'ctest.xml').getroot().iter('testcase'))
        require(len(xml)==1 and xml[0].get('name')==test and xml[0].find('failure')is None and xml[0].find('skipped')is None,'Terminal JUnit differs')
        process=[p for p in mapped['processes']if p['role']==role and(argument is None or p['command'][-1]==argument)]
        require(len(process)==1,'Missing/ambiguous actual native case');process=process[0];exe=process['executable']
        require(exe in result['registered_command']and mapped['loaded_project_images'][exe]==receipt['binary_hashes'][exe],'Native executable identity differs')
        require(any('libVF13adOptimizer'in p for p in mapped['loaded_project_images']),'Actual VF13ad library absent')
        if role=='fmincon-vary-test'and argument!='SelectVF13ad':
            require(all(any(f in p for p in mapped['loaded_project_images'])for f in['libFminconOptimizer.so','libMatlabInterface.so','/R2026b/bin/glnxa64/libeng.so']),
                'Actual fmincon/MATLAB SDK libraries absent')
        trace=[json.loads(line)for line in(directory/f'trace-{process["pid"]}.jsonl').read_text().splitlines()]
        require(trace[0]['event']=='start'and trace[-1]['event']=='summary'and trace[-1]['screenshots']==0,'Terminal observer absent')
        exposures=sum(r['event']=='native_exposure'and r.get('exposed',False)for r in trace)
        require(exposures==result['trace']['qt_window_exposure_events']and(mode!='visible'or exposures==result['trace']['native_exposure_events']>0),'Native exposure census differs')
        proof=load(directory/'proof.json');emitted=re.findall(r'NASA_CHECK (\{[^\n]+\})',Path(result['log']).read_text())
        require(len(emitted)==1 and json.loads(emitted[0])==proof,'Stdout/proof differs')
        contexts.append(dict(run=name,source_version=version,test=test,mode=mode,pid=process['pid'],visible_exposures=exposures if mode=='visible'else 0))
        return proof,directory
    for mode in['headless','visible']:
        for profile in PROFILES:
            if mode=='visible':name,version='controls-visible-v1',4
            elif profile=='Initial':name,version='initial-headless-v2',2
            elif profile=='References':name,version='references-headless-v1',4
            else:name,version='controls-headless-v1',3
            p,d=context(name,version,mode,PREFIX+profile,'fmincon-vary-test',profile);counts['evaluations']+=check(p,profile,d);proofs[mode+'-'+profile]=p
            counts['VF13ad'if profile=='SelectVF13ad'else'fmincon']+=len(p['phases']);counts['invalid']+=len(p['invalid_inputs'])
        for test,role,arg in[('QtGui.NativeNasaVF13adAdditive','vf13ad-vary-regression','Additive'),
            ('QtGui.NativeNasaVF13adMultiplicative','vf13ad-vary-regression','Multiplicative'),('QtGui.NativeNasaVaryOptimizeProperty','vary-property-regression',None)]:
            p,d=context('legacy-'+mode+'-v1',4,mode,test,role,arg);counts['legacy']+=legacy(p,test,d)
    for version in[2,3,4]:require(hashlib.sha256(source(version)['src/qtgui/CommandForm.cpp']).hexdigest()==qa['production_sha256'],'Different executed production fix')
    for p in qa['old_test_sources_byte_identical']:require(all(source(v)[p]==source(4)[p]for v in[2,3])and sha(repo/p)==hashlib.sha256(source(4)[p]).hexdigest(),'Original test source changed')
    require(source(1)['src/qtgui/tests/NasaFminconVaryTests.cpp']==source(2)['src/qtgui/tests/NasaFminconVaryTests.cpp'],'Before failure assertion was changed to pass')
    failed=load(stage/'initial-headless-v1/results.json');r=next(r for r in failed['results']if r['name']==PREFIX+'Initial')
    require(r['status']=='failed'and r['returncode']!=0 and load(Path(r['log']).parent/'invalid-inputs.json')[-1]['input']=='inf'
        and load(Path(r['log']).parent/'invalid-inputs.json')[-1]['rejected']is False,'Original nonfinite acceptance failure hidden')
    controls=[]
    mutations=[('missing-phase',lambda p:p['phases'].pop()),('physical-initial',lambda p:p['phases'][1]['controls'][0].update(initial=1)),
        ('physical-control',lambda p:p['phases'][1]['native'].update(Alpha=0)),('physical-cost',lambda p:p['phases'][1]['native'].update(Cost=2)),
        ('constraint',lambda p:p['phases'][1]['native'].update(Constraint=-1)),('sentinel',lambda p:p['phases'][1]['native'].update(Keep=40)),
        ('native-scale',lambda p:p['phases'][1]['native_scaled'].__setitem__(0,99)),('MATLAB-initial',lambda p:p['phases'][1]['MATLAB_scaled_arrays'][0].__setitem__(0,99)),
        ('MATLAB-lower',lambda p:p['phases'][1]['MATLAB_scaled_arrays'][0].__setitem__(1,99)),('MATLAB-upper',lambda p:p['phases'][1]['MATLAB_scaled_arrays'][0].__setitem__(2,99)),
        ('MATLAB-final',lambda p:p['phases'][1]['MATLAB_scaled_arrays'][0].__setitem__(3,99)),('viewer',lambda p:p['phases'][1].update(report_viewer=False)),
        ('invalid-control',lambda p:p['invalid_inputs'][0].update(rejected=False))]
    for name,mutate in mutations:
        p=copy.deepcopy(proofs['headless-Combined']);mutate(p)
        try:check(p,'Combined')
        except(ValueError,KeyError,TypeError,IndexError):controls.append(name)
        else:raise ValueError('Corrupt proof accepted: '+name)
    p=copy.deepcopy(proofs['headless-SelectFmincon']);p['picker_visits'][0]['pool'].append('WrongDC')
    try:check(p,'SelectFmincon')
    except ValueError:controls.append('optimizer-pool')
    else:raise ValueError('Corrupt optimizer pool accepted')
    require(counts['fmincon']==62 and counts['VF13ad']==6 and counts['legacy']==28 and counts['invalid']==64,'Mission/control census differs')
    q['audit_summary']=dict(qualified_keys=KEYS,GUI_case_contexts=24,new_case_contexts=18,legacy_case_contexts=6,actual_fmincon_solves=62,
        new_VF13ad_solves=6,legacy_VF13ad_solves=28,total_solver_missions=96,atomic_new_invalid_controls=64,legacy_property_rejections=20,
        actual_optimizer_picker_visits=8,GMAT_branch_evaluations=counts['evaluations'],case_source_bindings=contexts,corrupt_proof_controls=controls,
        visible_native_exposures=sum(c['visible_exposures']for c in contexts),core_paths_unchanged=257,core_source_changes=0,
        retained_original_hold_identities=59,remaining_optimizer_rows=14,retained_bundle_gate_status='open')
    return q,KEYS


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--qualification',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    repo=Path(__file__).resolve().parents[3];q,keys=qualify(repo,a.qualification,nasa.inventory(repo));require(not a.output.exists(),'Audit output exists')
    a.output.write_text(json.dumps(dict(status='passed',qualified_keys=keys,summary=q['audit_summary']),indent=2)+'\n');print(json.dumps(q['audit_summary']))
