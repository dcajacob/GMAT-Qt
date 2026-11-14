"""Audit full declared scalar Minimize support without repeating prior missions.

The original parent has three source categories and four mapped evidence claims.
All remain mandatory. New static scalar-objective fixtures prove typed value
support; retained changing-optimum witnesses prove effective objective selection.
"""
import argparse
import copy
import csv
import json
import math
from pathlib import Path

import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load,require,sha
from AuditConstraintScalarAcceptance import read_run
from BuildNasaCoverage95 import baseline
import AuditAcceptanceRecovery as recovery

KEYS=['FRC!E99']
SOURCE='src/qtgui/tests/NasaMinimizeScalarTests.cpp'
FIXTURE='src/qtgui/tests/NasaConstraintScalarCases.json'
FACETS=['complete-declared-scalar-objective-member','actual-objective-picker-and-transaction','independent-physical-objective-value','unique-feasible-control','actual-native-objective-progress','raw-results-and-exact-source']


def near(a,b,t):return math.isfinite(a) and abs(a-b)<=t


def check_case(case,result,sources,artifacts):
    work=Path(result['log']).parent
    require(all(str(work/name) in artifacts for name in ['proof.json','results.csv','baseline.script','accepted.script','run.messages.txt','test.log','ctest.xml']),'Minimize essential raw artifact omitted')
    p=load(work/'proof.json');require([r for r in nasa.proof_records(result) if r.get('checkpoint')=='minimize-scalar-family']==[p],'Minimize checkpoint differs from actual native log')
    require(result['status']=='passed' and result['returncode']==0 and p['outcome']=='passed' and p['case']==case['key'] and p['reference']==p['native_objective']==case['reference'] and p['parameter_type']==case['parameter_type'],'Wrong/unsuccessful actual native objective')
    require(p['facets']==FACETS and p['source_rows']==case['source_rows'] and p['assertions']>=75+(case['reference'].count('.')==2),'Minimize original contract facets/obligations omitted')
    for k in ['expected','absolute_tolerance','solver_control_tolerance']:require(p[k]==case[k],'Minimize independent oracle/tolerance changed')
    require([c for c in json.loads(sources[FIXTURE])['cases'] if c['key']==case['key']]==[case],'Minimize case executed another oracle fixture')
    expected=case['expected'];tol=case['absolute_tolerance'];ctol=case['solver_control_tolerance'];dummy=-(abs(expected)+987.25)
    rows=[[float(x) for x in row] for row in csv.reader((work/'results.csv').open())]
    require(rows==p['results'] and len(rows)==1 and len(rows[0])==4 and p['dummy_cost']==dummy,'Incomplete/altered raw Minimize fields')
    row=rows[0];require(near(row[0],expected,ctol) and near(row[1],expected,tol) and near(row[2],dummy,tol) and row[3]==42,'Independent unique feasible control, physical value or sentinel differs')
    require(p['picks']==[dict(role='Objective',reference=case['reference'])],'Actual Minimize objective picker not exercised')
    progress=p['progress'];objectives=[v for v in progress if v['kind']=='Objective'];goals=[v for v in progress if v['kind']=='Goal =']
    require(len(progress)==2 and len(objectives)==len(goals)==1,'Actual objective/feasibility progress omitted')
    require(objectives[0]['name']=='Cost' and near(objectives[0]['current'],expected,tol) and abs(objectives[0]['current']-dummy)>100,'Actual native objective cost differs from independent physical value')
    require(goals[0]['name']=='Control' and near(goals[0]['current'],expected,ctol),'Actual feasible control progress differs')
    before=(work/'baseline.script').read_text();after=(work/'accepted.script').read_text();old="Minimize 'Objective' Opt(Cost);";new="Minimize 'Objective' Opt("+case['reference']+");"
    require(before.count(old)==1 and before.replace(old,new)==after,'Objective transaction changed unrelated source')
    code=sources[SOURCE].decode()
    for token in ['commandField_Objective','commandChoose_Objective','"17","nan","inf","Label"','source->undo()','source->redo()','GetClone(0)','GetParameterID("ObjectiveFunction")','nativeObjective==reference','IntegerSolverStatus','Gmat::CONVERGED','near(current,expected,tolerance','near(v[0],expected,controlTolerance','near(v[1],expected,tolerance','near(v[2],dummy,tolerance','near(desired,expected,tolerance','objectives==1&&goals==1&&progress.size()==2','availableUndoSteps()==undo']:
        require(token in code,'Missing original Minimize native assertion: '+token)


def check_legacy(q,inventory):
    audit=recovery.checked_json(q['legacy_audit'],q['legacy_audit_sha256']);require(audit['new_test_executions']==0,'Legacy reuse silently reran tests')
    cohorts=audit['cohorts'];require({c['cohort'] for c in cohorts}=={'unattended','visible'},'Original Minimize cohort missing')
    for cohort in cohorts:
        rows=[r for r in cohort['rows'] if r['key']=='FRC!E99'];require(len(rows)==1,'Original Minimize mapping row missing');row=rows[0]
        mapping=recovery.checked_json(row['original_mapping'],row['original_mapping_sha256']);mapped=next(r for r in mapping['reviews'] if r['key']=='FRC!E99')
        require(len(mapped['evidence'])==row['original_required_clauses']==len(row['checks'])==4,'Original four mapped clauses omitted')
        for claim,check in zip(mapped['evidence'],row['checks']):
            require(check['test']==claim['test'] and check['source']==claim['source'] and check['source_sha256']==claim['source_sha256'] and check['assertion_fields_unchanged'],'Legacy assertion source changed')
            require(check['original_reference_sources']==claim.get('reference_sources',{}),'Unreviewed original reference rewrite')
            corrected=copy.deepcopy(claim);corrected['reference_sources']=check['reference_sources']
            receipt=recovery.checked_json(check['receipt'],check['receipt_sha256']);manifest=json.loads(recovery.payload(receipt['snapshot_manifest']));result=next(r for r in receipt['results'] if r['name']==claim['test'])
            recovery.verify_claim(Path(manifest['root']),manifest,receipt,result,corrected)
            if cohort['cohort']=='visible':require(result['trace']['native_exposure_events']>0,'Legacy Minimize witness unexposed')


def qualify(repo,path,inventory):
    q=load(path);require(q['contract_kind']=='source-versioned-complete-Minimize-scalar-family' and q['qualified_keys']==KEYS,'Partial/extra Minimize credit')
    require(all(q.get(k) is False for k in ('technical_core_approval','full_current_build_qualified','full_NASA_qualified')),'Minimize scope overclaim')
    require(q['authority_sha256']==inventory['sources'],'Minimize source authority changed')
    for path,digest in q['artifact_sha256'].items():require(sha(path)==digest,'Changed Minimize artifact: '+path)
    review=load(q['review']);known={r['key']:r for r in inventory['requirements']};require(len(review['reviews'])==1,'Missing parent review');r=review['reviews'][0]
    require(r['key']==KEYS[0] and r['fingerprint']==known[KEYS[0]]['fingerprint'] and r['source_text']==known[KEYS[0]]['text'] and r['rationale'],'Original parent changed')
    children={r['key'] for r in inventory['requirements'] if r['declared_id'].startswith('FRC-6.2.') and r['key']!=KEYS[0]};require(children=={'FRC!E100','FRC!E101','FRC!E102'} and set(review['original_source_category_keys'])==children,'Original three-category hierarchy changed')
    _,original,keys=baseline(repo,Path(q['baseline_seal']));require(original['sources']==inventory['sources'] and children<=keys['requirements'],'Original child evidence absent')
    fixture=load(q['fixture']);cases={c['key']:c for c in fixture['cases']};require(len(cases)==162 and len(fixture['unique_scalar_types'])==97 and {c['parameter_type'] for c in cases.values()}|{'X'}==set(fixture['unique_scalar_types']),'Incomplete declared scalar family')
    # The frozen independent scalar generator and original complete source
    # chapter audit remain mandatory; new objective execution cannot replace it.
    from AuditConstraintScalarAcceptance import qualify as scalar_admission
    scalar_admission(repo,Path(q['scalar_family_qualification']),inventory)
    require(set(q['selected'])=={'headless','visible'},'Missing Minimize GUI execution mode')
    runs={name:read_run(repo,spec,inventory,q['artifact_sha256']) for name,spec in q['runs'].items()}
    for mode,selections in q['selected'].items():
        require(set(selections)==set(cases),'Missing/extra declared scalar objective')
        for key,selection in selections.items():
            receipt,results,sources=runs[selection['run']];name='QtGui.NasaMinimizeScalar'+key
            require(receipt['mode']==mode and selection['test']==name,'Substituted objective case/cohort');check_case(cases[key],results[name],sources,q['artifact_sha256'])
    check_legacy(q,inventory)
    require(review['derivative_accuracy_for_every_parameter_claimed'] is False,'Static objective support promoted to full derivative science')
    return q,KEYS


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args();_,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,new_GUI_or_engine_executions=0)))
