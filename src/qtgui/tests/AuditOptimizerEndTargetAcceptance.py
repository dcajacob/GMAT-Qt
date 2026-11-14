"""Admit only the worksheet's EndTarget optimization-termination obligation."""
import argparse,copy,hashlib,json,math,re,tarfile,xml.etree.ElementTree as ET
from pathlib import Path
import NasaGuiCoverage as nasa
import BuildNasaCoverage95 as builder
import BuildNasaCoverageExtension as common
load,sha,require=common.load,common.sha,common.require
KEYS=['FRC!E54'];CONTEXT=['FRC!E5','FRC!E6','FRC!E7','FRC!E53','FRC!E54','FRC!E55']
KIND='source-versioned-complete-worksheet-optimizer-EndTarget'
GATES=['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','native_OS_input_qualified']
CONTEXTS=['Direct','If','TargetInside','OptimizeInside']
NEGATIVES=['WrongTargetEndOptimize','WrongIfEndTarget','OrphanEndTarget']
NATIVE=[p+c for c in CONTEXTS for p in ['Canonical','Worksheet']]+NEGATIVES
CLAUSES={'exact-original-dual-spelling-context','generic-innermost-optimizer-source-argument','actual-optimization-and-successor','nested-solver-ownership-both-directions','specific-invalid-marker-rejections','actual-Mission-tree-structural-operations','UndoRedo-and-Unicode-reopen','nonempty-command-state-summaries','paired-visible-runtime-identities','protected-originals-and-open-broad-parents'}

def structure(q,s,inventory):
    known={r['key']:r for r in inventory['requirements']}
    require(q['contract_kind']==KIND and q['qualified_keys']==KEYS and q['authority_sha256']==inventory['sources'],'Extra/partial/changed optimizer ending scope')
    require(s['candidate_rows']==[known[k] for k in KEYS] and s['complete_original_context']==[known[k] for k in CONTEXT],'Changed original optimization/target context')
    require(s['spellings']==['EndOptimize','EndTarget'] and s['contexts']==CONTEXTS and s['negative_cases']==NEGATIVES and s['profiles']==['headless','visible'],'Incomplete optimizer spelling/ownership matrix')
    require(set(s['clauses'])==CLAUSES and all(v['satisfied'] and v['evidence'] for v in s['clauses'].values()),'Incomplete exact optimizer ending conjunction')
    require(s['scalar_bound']==2e-5 and s['original_assertions_changed']==s['original_fixture_or_tolerance_changes']==s['other_row_credit']==s['runtime_binary_copies']==0,'Changed original assertion/bound or extra optimizer credit')
    require(not s['all_commands_in_solver_qualified'] and not s['original_failures_waived'] and s['newly_resolved_hold_keys']==[],'Broader solver scope or original holds promoted')
    require(all(q[k] is False and s[k] is False for k in GATES),'Broader optimizer qualification promoted')

def damaged(q,s,inventory):
    rejected=[]
    for fault in ['extra-row','fingerprint','missing-context','missing-spelling','missing-nesting','missing-negative','missing-profile','missing-clause','changed-bound','extra-credit','waived-failures','broader-gate']:
        a,b=copy.deepcopy(q),copy.deepcopy(s)
        if fault=='extra-row':a['qualified_keys'].append('FRC!E7')
        elif fault=='fingerprint':b['candidate_rows'][0]['fingerprint']='0'*64
        elif fault=='missing-context':b['complete_original_context'].pop()
        elif fault=='missing-spelling':b['spellings'].pop()
        elif fault=='missing-nesting':b['contexts'].pop()
        elif fault=='missing-negative':b['negative_cases'].pop()
        elif fault=='missing-profile':b['profiles'].pop()
        elif fault=='missing-clause':b['clauses'].pop(next(iter(b['clauses'])))
        elif fault=='changed-bound':b['scalar_bound']=1
        elif fault=='extra-credit':b['other_row_credit']=1
        elif fault=='waived-failures':b['original_failures_waived']=True
        else:a['full_NASA_qualified']=True
        try:structure(a,b,inventory)
        except ValueError:rejected.append(fault)
        else:raise ValueError('Damaged optimizer admission accepted: '+fault)
    return rejected

def component(stage):
    """Read recorded inputs/images; never substitute current host binaries."""
    audit=load(stage/'component-audit-v1.json');require(audit['status']=='passed','Optimizer component audit failed')
    rows=load(stage/'native-v1/results.json')['cases'];require([r['case'] for r in rows]==NATIVE,'Missing/reordered native optimizer ending cases')
    source=load(stage/'source-before-v1.json');require(source==load(stage/'source-after-v1.json'),'Native owning sources changed during execution')
    binaries=load(stage/'binary-before-v1.json');require(binaries==load(stage/'binary-after-v1.json'),'Native images changed during execution')
    mapping=load(stage/'source-versions-v1.json');recorded={};native_images=None
    with tarfile.open(stage/'sources-v1.tar.gz') as bundle:
        require(set(bundle.getnames())=={'sha256/'+h for h in mapping},'Wrong optimizer source capsule partition')
        for h,names in sorted(mapping.items()):
            raw=bundle.extractfile('sha256/'+h).read();require(not raw.startswith(b'\x7fELF') and hashlib.sha256(raw).hexdigest()==h,'Changed/non-source optimizer capsule member')
            for n in names:recorded.setdefault(n,{})[h]=raw
    def text(n):
        h=source[n];require(h in recorded.get(n,{}),'Missing native owner source: '+n);return recorded[n][h].decode()
    for name,tokens in {
      'src/base/interpreter/Interpreter.cpp':['const bool legacyOptimizerEnd = str1 == "Optimize" && str == "EndTarget";','if (expEndStr != str && !legacyOptimizerEnd)'],
      'src/base/command/Optimize.cpp':['const bool wasFillingBranch = branchToFill >= 0;','cmd->GetTypeName() == "EndTarget" &&','(!cmd->GetNext() || cmd->GetNext()->GetTypeName() == "Optimize")'],
      'src/base/command/Target.cpp':['cmd->GetTypeName() == "EndTarget" &&','(!cmd->GetNext() || cmd->GetNext()->GetTypeName() != "Optimize")'],
      'src/base/command/EndTarget.cpp':['next->GetTypeName() != "Target" && next->GetTypeName() != "Optimize"','BuildCommandSummary(true);','generatingString = prefix + "EndTarget;";']}.items():
        for token in tokens:require(token in text(name),'Missing generic optimizer ownership/spelling path: '+token)
    junit=list(ET.parse(stage/'native-v1/ctest.xml').iter('testcase'));require(len(junit)==1 and junit[0].get('name')=='QtAcceptance.OptimizerEndTarget' and all(junit[0].find(k) is None for k in ['failure','error','skipped']),'Native registered optimizer entry did not pass')
    native_scalars=0;by_name={r['case']:r for r in rows}
    for r in rows:
        name=r['case'];case=stage/'native-v1'/name;p=load(case/'proof.json');require(r['passed'] and r['returncode']==0 and r['proof']==p and p['passed'] and p['built']==p['expected_build'],'Failed optimizer ending case: '+name)
        images=load(case/'native-image-identities.json')
        if native_images is None:native_images=images
        else:require(images==native_images,'Different native optimizer ending loaded images')
        require(r['binary_sha256']==images[r['command'][0]]['sha256'] and r['startup_sha256']==sha(r['command'][1]),'Unbound actual native optimizer command/input')
        if name in NEGATIVES:
            wanted={'WrongTargetEndOptimize':'Expecting "EndTarget" but found "EndOptimize"','WrongIfEndTarget':'Expecting "Else" or "EndIf" but found "EndTarget"','OrphanEndTarget':'Found too many "EndTarget"'}[name]
            require(not p['built'] and p['run_status']==-99 and not p['values'] and wanted in '\n'.join(p['errors']),'Wrong optimizer marker was accepted or rejected for another reason')
        else:
            nested=name.endswith(('TargetInside','OptimizeInside'));expected={'U':2.,'V':4. if nested else 1.,'Cost':0.,'Quantity':5. if nested else 0.,'Keep':41.,'After':7.}
            require(p['built'] and p['run_status']==1 and p['expected']==expected and set(p['values'])==set(expected),'Missing actual optimizer/target execution')
            for k,v in p['values'].items():require(math.isfinite(v) and abs(v-expected[k])<=(0 if k in ['Keep','After'] else 2e-5),'Independent scalar optimizer ending witness failed');native_scalars+=1
    for c in CONTEXTS:
        left=stage/'native-v1'/('Canonical'+c);right=stage/'native-v1'/('Worksheet'+c)
        require(by_name['Canonical'+c]['proof']['values']==by_name['Worksheet'+c]['proof']['values'],'Terminator spelling changed native results')
        a=(left/'mission.script').read_text().replace(str(left),'@CASE@');b=(right/'mission.script').read_text().replace(str(right),'@CASE@')
        require(a.count('EndOptimize;')==1 and a.replace('EndOptimize;','EndTarget;')==b,'Native spelling pair changed another input')
    head=load(stage/'gui-headless-v1/results.json');visible=load(stage/'gui-visible-v1/results.json')
    require(head['source_hashes']==visible['source_hashes'] and head['binary_hashes']==visible['binary_hashes'],'GUI optimizer ending source/runtime mismatch')
    require(visible['display_route']=='nested' and visible['resizable'] and visible['max_screenshots']==0,'Missing native resizable visible delivery')
    for n,h in source.items():require(h in recorded.get(n,{}),'Missing executed native source version')
    for n,h in head['source_hashes'].items():
        require(h in recorded.get(n,{}) or audit['ELF_identity_only'].get(n)==h,'Missing executed GUI source version')
    require('(branch.type=="Optimize" && node.type=="EndTarget")' in recorded['src/qtgui/MissionModel.cpp'][head['source_hashes']['src/qtgui/MissionModel.cpp']].decode(),'Missing Qt optimizer alias branch-end lookup')
    commands=rows[0]['command'];roots=[commands[0][:-len(k)-1] for k in binaries if commands[0].endswith('/'+k)];require(len(roots)==1,'Native command not bound by binary guard');native_root=roots[0]+'/'
    # Bind project loaded identities to the stable before/after guard. System
    # images retain their actual process-map identities, without runtime copies.
    for n,v in native_images.items():
        if n.startswith(native_root):require(binaries.get(n.removeprefix(native_root))==v['sha256'],'Loaded native project image differs from guard')
    fixture_names=['src/qtgui/tests/command-operation-structural/EndOptimize.json','src/qtgui/tests/command-operation-structural/OptimizeEndTarget.json']
    fixture_bytes={n:recorded[n][head['source_hashes'][n]] for n in fixture_names}
    canonical,alias=[json.loads(fixture_bytes[n]) for n in fixture_names];derivation=copy.deepcopy(alias)
    require(alias['alias_fixture_origin_sha256']==hashlib.sha256(fixture_bytes[fixture_names[0]]).hexdigest(),'Unbound original structural fixture')
    for k in ['alias_fixture_origin','alias_fixture_origin_sha256']:derivation.pop(k)
    for k in ['family','command','anonymous_tree_label','derivation']:derivation[k]=canonical[k]
    derivation['script']=derivation['script'].replace('EndTarget;','EndOptimize;');require(derivation==canonical,'New alias fixture changed another original input or bound')
    selected=lambda d:{r['name']:r for r in d['results'] if r['status']!='not-run'}
    names={p+c for p in ['QtGui.OptimizeWorksheetEndTarget','QtGui.NasaCommandStructuralOperationsEndOptimize'] for c in ['Original','OuterIf']}
    require(set(selected(head))==set(selected(visible))==names,'Missing canonical/worksheet GUI operation pair')
    GUI_scalars=summary_scalars=exposures=0
    for profile,d in [('gui-headless-v1',head),('gui-visible-v1',visible)]:
        for name,r in selected(d).items():
            case=stage/profile/name;p=load(case/'proof.json');fixture=load(case/'fixture.json');worksheet=name.startswith('QtGui.OptimizeWorksheet');require(r['status']=='passed' and r['returncode']==0 and p['outcome']=='passed' and p['original_parent']=='Optimize' and p['fixture_sha256']==sha(case/'fixture.json'),'Failed/wrong optimizer GUI fixture')
            require(fixture==(alias if worksheet else canonical) and p['position']==('OuterIf' if name.endswith('OuterIf') else 'Original'),'Wrong source-bound fixture or GUI position')
            require(p['family']==('EndTarget' if worksheet else 'EndOptimize') and p['operations']==['context-protection','doubleclick-protection','close-no-owner','insert-before','insert-after','summary-context','unicode-reopen'],'Missing actual optimizer end operations')
            require(not r['trace']['screenshots'],'Unrequested optimizer screenshots')
            if profile=='gui-visible-v1':require(r['trace']['native_exposure_events']>0 and r['trace']['recorded_platforms']==['xcb'],'Optimizer GUI was not exposed natively')
            else:require(r['trace']['native_exposure_events']==0,'Unattended optimizer GUI profile changed')
            junit=list(ET.parse(case/'ctest.xml').iter('testcase'));require(len(junit)==1 and junit[0].get('name')==name and all(junit[0].find(k) is None for k in ['failure','error','skipped']),'Actual GUI CTest did not pass')
            exposures+=r['trace']['native_exposure_events'];original=(case/'initial.script').read_text();require(original.count('EndTarget;' if worksheet else 'EndOptimize;')==1,'Original ending spelling not preserved')
            for side in ['before','after']:
                changed=(case/(side+'-inserted.script')).read_text();statement="'StructuralProbe' Keep = Keep + 0;";require(changed.count(statement)==1 and (changed.replace(statement+'\n','',1)==original or changed.replace('\n'+statement,'',1)==original),'GUI insertion altered unrelated source')
            require(len(p['results'])==2 and fixture['expected_tail']==[0,2,0,0] and fixture['scalar_tolerance']==2e-5,'Original scalar inputs/bounds changed')
            omega=math.sqrt(398600.4415/7000.**3);speed=math.sqrt(398600.4415/7000.)
            def state(seconds):
                a=omega*seconds;return [7000*math.cos(a),7000*math.sin(a),0,-speed*math.sin(a),speed*math.cos(a),0]
            for phase,result in enumerate(p['results']):
                raw=[float(x) for x in (case/f'{phase}-states.csv').read_text().strip().split(',')];truth=[20.,*state(20.),41.,0,2,0,0];require(raw==result['raw'] and len(raw)==len(truth)==12,'Wrong optimizer GUI report shape')
                for i,(v,w) in enumerate(zip(raw,truth)):require(math.isfinite(v) and abs(v-w)<=(1e-4 if i==0 else 2e-5 if i<4 else 2e-8 if i<7 else 2e-5),'Independent optimizer GUI report failed');GUI_scalars+=1
                summary=(case/f'{phase}-summary.txt').read_text();require('Spacecraft       : Sat' in summary and 'Coordinate System: EarthMJ2000Eq' in summary,'Missing selected command summary')
                for label,truth in zip(['X','Y','Z','VX','VY','VZ'],state(fixture['operation_summary_seconds'])):
                    values=re.findall(r'(?:^|\s)'+label+r'\s*=\s*([-+0-9.eE]+)',summary);require(len(values)==1 and abs(float(values[0])-truth)<=2e-5,'Independent optimizer command summary failed');summary_scalars+=1
    require((native_scalars,GUI_scalars,summary_scalars,exposures)==(48,192,96,102),'Incomplete optimizer ending result census')
    require(audit['native_cases']==11 and audit['actual_optimization_cases']==8 and audit['required_invalid_marker_rejections']==3 and audit['GUI_passes']==8 and audit['original_protected_inputs']==1334 and audit['production_changes']==audit['original_assertion_changes']==audit['numerical_bound_changes']==0,'Component result/fidelity census changed')
    return dict(native_cases=11,actual_optimization_cases=8,invalid_marker_rejections=3,GUI_passes=8,native_scalars=48,GUI_report_scalars=192,GUI_summary_scalars=96,visible_native_exposures=102)

def qualify(repo,path,inventory):
    q=load(path)
    for n,h in q['artifact_sha256'].items():require(sha(n)==h,'Changed optimizer termination artifact: '+n)
    s=load(q['scope_review']);structure(q,s,inventory)
    for archive,ids in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
        require(q[archive] in q['artifact_sha256'],'Unbound optimizer source archive')
        with tarfile.open(q[archive]) as bundle:
            require(set(bundle.getnames())==set(q[ids]),'Optimizer admission source partition differs')
            for n,h in sorted(q[ids].items()):require(hashlib.sha256(bundle.extractfile(n).read()).hexdigest()==h,'Changed optimizer admission source')
    protected=load(q['protected_input_baseline']);require(len(protected)==1334 and all(q['proof_source_sha256'].get(n)==h for n,h in protected.items()),'Protected original inputs omitted')
    prior=load(q['prior_overlay']);rows={r['key']:r for r in prior['reviewed_source_rows']};require(len(prior['original_hold_keys_retained'])==59 and not set(KEYS)&set(prior['original_hold_keys_retained']) and rows[KEYS[0]]['whole_row_status']=='open','Original optimizer gap or holds changed')
    full={r['original_source']['key']:r for r in load(q['prior_full_audit'])['rows']}
    require(all(full[k]['status']=='open-original-scope' for k in ['FRC!E7','FRC!E55']),'Broad all-command solver parents substituted')
    result=component(Path(q['component_stage']));rejected=damaged(q,s,inventory);require(len(rejected)==12,'Incomplete damaged admission controls')
    return q,KEYS,dict(status='passed',qualified_keys=KEYS,whole_row_conjunction=True,component=result,new_requirement_credit=1,newly_resolved_original_hold_keys=[],damaged_admission_controls_rejected=12,native_tests_rerun=0,runtime_binary_copies=0,**{k:False for k in GATES})

def build(repo,baseline,path,output):
    seal,inventory,keys=builder.baseline(repo,baseline);original={k:set(v) for k,v in keys.items()};_,incoming,audit=qualify(repo,path,inventory);known={k:{r['key']:r for r in inventory[k]} for k in keys}
    require(not set(incoming)&keys['requirements'],'Duplicate optimizer ending credit');keys['requirements']=common.merge_keys(keys['requirements'],incoming,known['requirements'])
    coverage={k:dict(verified=len(keys[k]),total=len(known[k]),percent=round(100*len(keys[k])/len(known[k]),1),new_verified=len(keys[k])-len(original[k])) for k in keys};remaining={k:[r for r in seal['remaining_original_rows'][k] if r['key'] not in keys[k]] for k in keys}
    result=dict(status='passed',scope='95-percent campaign: sealed source-versioned prefix plus complete original worksheet optimization EndTarget rule',baseline=str(baseline),baseline_sha256=sha(baseline),authority_sha256=inventory['sources'],coverage=coverage,target_requirements=1101,target_reached=len(keys['requirements'])>=1101,requirements_still_needed=max(0,1101-len(keys['requirements'])),additions=[dict(key=k,kind='requirements',fingerprint=known['requirements'][k]['fingerprint'],source_text=known['requirements'][k]['text'],qualification=str(path),qualification_sha256=sha(path),status='verified-source-versioned') for k in incoming],remaining_counts={k:len(v) for k,v in remaining.items()},remaining_original_rows=remaining,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,limits=['Original 1158/2367 denominators and exact source rows remain unchanged.','Only FRC!E54 is newly admitted; FRC!E7/E55 broad command scopes and all59 separate formal holds retain their prior status.','Scientific, physical frame/ephemeris, external ABI, platform/native OS input and full current-build approvals remain separate.'])
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
