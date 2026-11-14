"""Audit the complete declared NASA scalar NLC operand family and its parent.

Both execution modes and every enumerated member are mandatory. Source versions,
physical inputs, independent numeric truth and the original three-category
constraint witnesses remain explicit. No mission runs during admission.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import tarfile
import tempfile
import subprocess
import sys

from AuditCapiAcceptance import load, require, sha
from BuildNasaCoverageExtension import validate_visible
import NasaGuiCoverage as nasa
from RunHarnessAcceptance import passed_junit

KEYS=['FRC!E104','FRC!E105']
SOURCE='src/qtgui/tests/NasaConstraintScalarTests.cpp'
FIXTURE='src/qtgui/tests/NasaConstraintScalarCases.json'
FACETS=['complete-declared-scalar-family-member','actual-typed-picker-and-transaction','independent-physical-operand-and-active-optimum','both-operand-roles','actual-evaluated-optimizer-progress','raw-results-and-exact-source']


def near(actual, expected, tolerance):
    return math.isfinite(actual) and abs(actual-expected)<=tolerance


def read_run(repo, spec, inventory, artifacts):
    before,after=load(spec['before']),load(spec['after'])
    require(before==after,'Scalar source/runtime changed during execution')
    reuse=load(spec['reuse']);receipt=load(reuse['receipt'])
    require(sha(reuse['receipt'])==reuse['receipt_sha256'] and receipt.get('finished_utc') and receipt.get('end_reason')=='completed-selection','Nonterminal/changed scalar receipt')
    sources={}; hashes={}
    for field,path in [('reused_source_sha256',reuse['reference']),('changed_source_sha256',reuse['delta'])]:
        require(sha(path)==reuse['reference_sha256' if field.startswith('reused') else 'delta_sha256'],'Changed scalar source archive')
        with tarfile.open(path) as bundle:
            for name,digest in reuse[field].items():
                require(not Path(name).is_absolute() and '..' not in Path(name).parts and name not in hashes,'Unsafe/duplicate scalar source member')
                data=bundle.extractfile(name).read()
                require(hashlib.sha256(data).hexdigest()==digest and not data.startswith(b'\x7fELF'),'Changed/non-source scalar archive member')
                require(before['sources'].get(name,digest)==digest,'Scalar source disagrees with wider snapshot')
                sources[name]=data;hashes[name]=digest
    other=reuse['recorded_authority_ELF_or_existing_physical_inputs_not_copied']
    require(not set(other)&set(hashes) and {**hashes,**other}==receipt['source_hashes'],'Incomplete scalar source partition')
    require({p:other[p] for p in inventory['sources']}==inventory['sources'],'Scalar NASA authority differs')
    physical=json.loads(sources[FIXTURE])['physical_inputs'] if FIXTURE in sources else {}
    for name,digest in other.items():
        if name in inventory['sources']:continue
        if name.startswith('application/data/'):
            if name in physical:require(before['sources'].get(name)==digest==physical[name],'Unbound existing scalar physical input')
            else:require(name=='application/data/planetary_ephem/spk/ceres_1900_2100.bsp' and name not in json.loads(sources[FIXTURE])['base_setup'],'Unexplained nonexecuted physical review reference')
        else:require(before['binaries'].get(str(repo/name))==digest,'Unbound noncopied scalar executable')
    results={r['name']:r for r in receipt['results'] if r['status']!='not-run'}
    require(len(results)==sum(r['status']!='not-run' for r in receipt['results']),'Duplicate scalar result')
    for name,result in results.items():
        executables=[p for p in result['registered_command'] if p in receipt['binary_hashes']]
        require(len(executables)==1 and before['binaries'].get(executables[0])==receipt['binary_hashes'][executables[0]],'Scalar executable not bound before execution')
        if result['status']!='passed':continue
        junit=Path(result['log']).parent/'ctest.xml'
        require(result['returncode']==0 and passed_junit(junit,name,0),'Scalar successful result lacks exact JUnit')
        require(all(str(p) in artifacts for p in [Path(result['log']),junit]),'Scalar actual log/JUnit omitted from artifact seal')
    if receipt['mode']=='visible':
        validate_visible(receipt,{name:dict(log=result['log'],log_sha256=sha(result['log']),junit=str(Path(result['log']).parent/'ctest.xml'),junit_sha256=sha(Path(result['log']).parent/'ctest.xml')) for name,result in results.items()})
    else:require(receipt['mode']=='headless','Unknown scalar execution mode')
    return receipt,results,sources


def check_case(case,result,sources,artifacts):
    require(result['status']=='passed' and result['returncode']==0,'Unsuccessful scalar witness')
    path=Path(result['log']).parent
    require(all(str(path/name) in artifacts for name in ['proof.json','results.csv','baseline.script','accepted.script','run.messages.txt','test.log','ctest.xml']),'Scalar essential result artifact omitted')
    proof=load(path/'proof.json')
    require([p for p in nasa.proof_records(result) if p.get('checkpoint')=='constraint-scalar-family']==[proof],'Scalar proof is not the actual logged checkpoint')
    require(proof['outcome']=='passed' and proof['case']==case['key'] and proof['parameter_type']==case['parameter_type'] and proof['reference']==case['reference'],'Scalar member/reference changed')
    require(proof['facets']==FACETS and proof['roles']==['left','right'] and proof['source_rows']==case['source_rows'] and proof['assertions']>=103,'Scalar assertions/roles/obligations omitted')
    for key in ['expected','absolute_tolerance','solver_control_tolerance']:require(proof[key]==case[key],'Scalar oracle/tolerance changed')
    executed=json.loads(sources[FIXTURE]); matches=[c for c in executed['cases'] if c['key']==case['key']]
    require(matches==[case],'Scalar witness executed another fixture')
    code=sources[SOURCE].decode()
    for token in ['near(v[0],expected,controlTolerance','near(v[1],expected,tolerance','near(v[2],4,','v[3]==42','leftSeen&&rightSeen&&progress.size()==2','source->toPlainText()==applied','source->undo()','source->redo()','availableUndoSteps()==undo','Grid(2,1)','MissingScalar.Unknown','QDialogButtonBox::Apply','Scalar family Unicode save/reopen not exact']:
        require(token in code,'Scalar original assertion omitted: '+token)
    rows=[[float(v) for v in row] for row in csv.reader((path/'results.csv').open())]
    require(rows==proof['results'] and len(rows)==2 and all(len(r)==4 for r in rows),'Scalar raw left/right results differ')
    expected=case['expected'];tol=case['absolute_tolerance'];ctol=case['solver_control_tolerance'];scale=max(1,abs(expected)*.001)
    for row in rows:
        require(near(row[0],expected,ctol) and near(row[1],expected,tol) and near(row[2],4,max(2e-6,8*ctol/scale)) and row[3]==42,'Independent active scalar optimum/physical operand failed')
    require(proof['picks']==[dict(role=r,reference=case['reference']) for r in ('Right','Left')],'Actual typed picker role missing')
    progress=proof['progress'];require(len(progress)==2 and {p['name'] for p in progress}=={'Control',case['reference']},'Actual scalar left/right progress missing')
    for p in progress:
        left=p['name']==case['reference'];require(p['kind']==('Goal ≥' if left else 'Goal ≤') and near(p['current'],expected,tol if left else ctol) and near(p['desired'],expected,ctol if left else tol),'Actual scalar accepted progress differs from independent truth')
    baseline=(path/'baseline.script').read_text();accepted=(path/'accepted.script').read_text()
    baseline=baseline.replace("NonlinearConstraint 'RightBound' Opt(Control <= Bound);","NonlinearConstraint 'RightBound' Opt(Control <= "+case['reference']+");").replace("NonlinearConstraint 'LeftBound' Opt(Bound >= Control);","NonlinearConstraint 'LeftBound' Opt("+case['reference']+" >= Control);")
    require(baseline==accepted,'Scalar GUI transaction changed unrelated source')


def qualify(repo,path,inventory):
    q=load(path)
    require(q['contract_kind']=='source-versioned-full-scalar-constraint-family' and q['qualified_keys']==KEYS,'Incomplete/extra scalar credit')
    require(all(q.get(k) is False for k in ('full_current_build_qualified','full_NASA_qualified','technical_core_approval')),'Scalar qualification exceeds scope')
    for p,digest in q['artifact_sha256'].items():require(sha(p)==digest,'Changed scalar artifact: '+p)
    require(q['authority_sha256']==inventory['sources'],'Scalar source baseline changed')
    review=load(q['review']);known={r['key']:r for r in inventory['requirements']}
    require([r['key'] for r in review['reviews']]==KEYS,'Incomplete scalar semantic review')
    for row in review['reviews']:require(row['fingerprint']==known[row['key']]['fingerprint'] and row['source_text']==known[row['key']]['text'] and row['rationale'],'Scalar original row/fingerprint changed')
    fixture=load(q['fixture']);cases={c['key']:c for c in fixture['cases']}
    regeneration=load(q['regeneration']);toolroot=Path(q['fixture']).parent
    require(regeneration['status']=='passed' and regeneration['fixture_sha256']==sha(q['fixture']),'Independent scalar regeneration missing')
    # Replay pure mathematical fixture generation from the frozen oracle inputs.
    # Existing authorities/data are linked only after their content is checked;
    # no engine, native optimizer, GUI or runtime binary is copied or executed.
    with tempfile.TemporaryDirectory(prefix='gmat-scalar-oracle-') as folder:
        frozen=Path(folder)
        for name,digest in regeneration['oracle_input_sha256'].items():
            source=toolroot/Path(name).name;require(sha(source)==digest,'Frozen independent oracle changed')
            target=frozen/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(source.read_bytes())
        for name,digest in {**inventory['sources'],**fixture['physical_inputs']}.items():
            target=frozen/name;target.parent.mkdir(parents=True,exist_ok=True)
            source=toolroot/'constraint-scalars-sun.bsp' if name.endswith('/constraint-scalars-sun.bsp') else repo/name
            require(sha(source)==digest,'Frozen oracle authority/physical input changed');target.symlink_to(source.resolve())
        target=frozen/'build/nasa-coverage/coverage85-20261101/scalar-inventory2/inventory.json';target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((toolroot/'inventory.json').read_bytes())
        generated=frozen/'generated.json'
        process=subprocess.run([sys.executable,str(frozen/'src/qtgui/tests/NasaConstraintScalarOracle.py'),'--repo',str(frozen),'--output',str(generated)],capture_output=True,text=True)
        require(process.returncode==0 and load(generated)==fixture,'Independent frozen mathematical scalar oracle differs: '+process.stderr)

    require(len(cases)==162 and len(fixture['source_rows'])==80 and len(fixture['unique_scalar_types'])==97 and fixture['literal_reference']=='FRR-388','Incomplete original scalar inventory')
    required=set()
    for row in fixture['source_rows']:
        require(row['fingerprint']==known[row['key']]['fingerprint'] and row['source_text']==known[row['key']]['text'],'Changed original scalar family source row')
        required.update(row['scalar_types'])
        require(not set(row['scalar_types'])&set(row['non_scalar_types']),'Overlapping scalar/non-scalar inventory')
    require(required==set(fixture['unique_scalar_types'])=={c['parameter_type'] for c in cases.values()}|{'X'},'Missing declared scalar type')
    require({c['partition'][6:] for c in cases.values() if c['partition'].startswith('euler-')}=={'123','132','213','231','312','321','121','131','212','232','313','323'},'Incomplete twelve Euler sequences')
    runs={name:read_run(repo,spec,inventory,q['artifact_sha256']) for name,spec in q['runs'].items()}
    require(set(q['selected'])=={'headless','visible'},'Scalar mode missing')
    for mode,selections in q['selected'].items():
        require(set(selections)==set(cases),'Scalar mode omits/duplicates a required member')
        for key,selection in selections.items():
            receipt,results,sources=runs[selection['run']];name='QtGui.NasaConstraintScalar'+key
            require(receipt['mode']==mode and selection['test']==name,'Scalar mode/test substituted')
            check_case(cases[key],results[name],sources,q['artifact_sha256'])
    # Original X, user-variable and array-element clauses are retained at their
    # source versions; this mandatory audit adds the missing family, not a new
    # attribution of those receipts to today's binary.
    legacy=load(q['legacy_category_audit'])
    require([c['cohort'] for c in legacy['cohorts']]==['unattended','visible'],'Original constraint cohort omitted')
    for cohort in legacy['cohorts']:
        mapping=load(cohort['mapping']);row=next(r for r in mapping['reviews'] if r['key']=='FRC!E104')
        require(sha(cohort['mapping'])==cohort['mapping_sha256'] and len(row['evidence'])==3 and len(cohort['rows'])==1 and len(cohort['rows'][0]['checks'])==3,'Original parent clause omitted')
        from AuditAcceptanceRecovery import checked_json,verify_claim
        for claim,check in zip(row['evidence'],cohort['rows'][0]['checks']):
            receipt=checked_json(check['receipt'],check['receipt_sha256']);manifest=load(receipt['snapshot_manifest']);result=next(r for r in receipt['results'] if r['name']==claim['test'])
            verify_claim(Path(manifest['root']),manifest,receipt,result,claim)
            if cohort['cohort']=='visible':require(result['trace']['native_exposure_events']>0,'Original X/category GUI witness unexposed')
    return q,KEYS


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args();_,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,GUI_or_engine_executions=0)))
