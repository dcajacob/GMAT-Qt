"""Replay the bounded unit-label evidence, preserving the original blank-unit failure."""
from pathlib import Path
import argparse,copy,hashlib,json,re,tarfile


def require(value,message):
    if not value:raise ValueError(message)


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):return json.loads(Path(path).read_text())


def check(proof,fixture,group):
    cases=[c for c in fixture['cases']if c['group']==group]
    expected={c['key']:c for c in cases}
    require(proof['status']=='passed'and proof['mismatches']==0,'Unit group did not pass')
    require(proof['group']==group and proof['cases']==list(expected),'Unit case census/order changed')
    require(proof['mission_executions']==0 and proof['source_native_history_file_isolation'],'Unit viewing changed mission/state scope')
    require(proof['new_requirement_credit']==0,'Bounded unit evidence awarded a whole row')
    wanted=set()
    for key,c in expected.items():
        phases=['initial','selector-roundtrip']if c.get('roundtrip_representation')else['initial']
        for phase in phases:
            for field in c['units']:wanted.add((key,phase,field))
    actual=set()
    for row in proof['observations']:
        key=row['case'];require(key in expected,'Unknown unit case')
        c=expected[key];field=row['field'];phase=row.get('phase','initial');identity=(key,phase,field)
        require(identity in wanted and identity not in actual,'Extra/duplicate unit observation');actual.add(identity)
        unit=c['units'][field]
        require(unit=='dimensionless'and row['expected_unit']==unit and row['passed'],'Wrong independent unit expectation/result')
        if c['route']in ['body','attitude']:
            require(row['route']==c['route']and unit in row['actual_label'],'Primary body/attitude unit absent')
        else:
            require(row['route']in ['advanced-table','primary+backing-metadata'],'Unknown property display route')
            require(row['actual_unit']==unit and unit in row['visible_unit_or_caption'],'Backing/visible unit mismatch')
    require(actual==wanted,'A frozen unit obligation is missing')
    return dict(group=group,cases=len(cases),unit_observations=len(actual))


def audit(repo,stage):
    repo,stage=Path(repo),Path(stage);plan=load(stage/'frozen-plan-v1.json')
    fixture=load(repo/'src/qtgui/tests/DimensionlessInputUnitCases.json')
    require(fixture==plan['expectations']and sha(repo/'src/qtgui/tests/DimensionlessInputUnitCases.json')==plan['fixture_sha256'],'Frozen quantity definitions changed')
    groups=plan['groups'];results=[];last=None
    headless={'Scalars':'scalars-after-v1','Directions':'directions-after-v2','Orbit':'orbit-after-v1','Attitude':'attitude-after-v1','Bodies':'bodies-after-v1'}
    for group,name in headless.items():
        root=stage/name;r=load(root/'receipt.json')
        require(r['status']=='passed'and r['returncode']==0 and r['profile']=='headless'and r['inputs_unchanged'],'Headless unit receipt failed')
        require(r['mission_executions']==0 and not r['full_regression'],'Unit execution scope changed')
        require(all(sha(root/n)==h for n,h in r['artifacts'].items()),'Changed headless unit artifact')
        with tarfile.open(root/'sources.tar.gz')as archive:
            sources={m.name:hashlib.sha256(archive.extractfile(m).read()).hexdigest()for m in archive.getmembers()if m.isfile()}
        require(sources==r['source_sha256'],'Headless executed source archive differs')
        require(sources['src/qtgui/tests/DimensionlessInputUnitCases.json']==plan['fixture_sha256'],'Wrong executed unit fixture')
        proof=load(root/'proof.json');records=re.findall(r'NASA_CHECK (\{[^\n]+\})',(root/'test.log').read_text())
        require(len(records)==1 and json.loads(records[0])==proof and load(root/'unit-observations.json')==proof['observations'],'Unit proof/raw/stdout mismatch')
        results.append(dict(mode='headless',**check(proof,fixture,group)));last=proof
    receipt=load(stage/'visible-v2/results.json');launch=load(stage/'visible-launch-receipt-v2.json')
    require(launch['returncode']==0 and launch['inputs_unchanged'],'Visible launch failed')
    with tarfile.open(stage/'visible-owned-sources-v2.tar.gz')as archive:
        sources={m.name:hashlib.sha256(archive.extractfile(m).read()).hexdigest()for m in archive.getmembers()if m.isfile()}
    require(sources==launch['source_sha256'],'Visible executed source archive differs')
    require(sources['src/qtgui/tests/DimensionlessInputUnitCases.json']==plan['fixture_sha256'],'Visible fixture changed')
    require(receipt['mode']=='visible'and receipt['end_reason']=='completed-selection'and receipt['finished_utc'],'Visible selection not terminal')
    display=receipt['visible_display']
    require(receipt['display_route']=='nested'and display['route']=='nested Xephyr window on existing desktop'and display['resizable']and display['start_minimized']and'Iconic'in display['initial_host_window_state'],'Not the established minimized watchable desktop')
    require(receipt['max_screenshots']==0,'Unexpected screenshot run')
    active=[r for r in receipt['results']if r['status']!='not-run']
    require({r['name']for r in active}=={'QtGui.NativeDimensionlessUnits'+g for g in groups},'Unrelated or missing visible case')
    for r in active:
        require(r['status']=='passed'and r['returncode']==0 and r['trace']['native_exposure_events']>0 and r['trace']['recorded_platforms']==['xcb'],'Visible native unit case failed/exposure absent')
        require(not r['trace']['screenshots'],'Unexpected unit screenshots')
        root=Path(r['log']).parent;proof=load(root/'proof.json')
        records=re.findall(r'NASA_CHECK (\{[^\n]+\})',Path(r['log']).read_text())
        require(len(records)==1 and json.loads(records[0])==proof and load(root/'unit-observations.json')==proof['observations'],'Visible unit proof/stdout/raw mismatch')
        group=r['name'].removeprefix('QtGui.NativeDimensionlessUnits')
        results.append(dict(mode='visible',**check(proof,fixture,group)))
    negatives=[]
    for label,mutate in [
        ('blank-unit',lambda p:p['observations'][0].update(actual_label='')),
        ('missing-case',lambda p:p['cases'].pop()),
        ('lost-isolation',lambda p:p.update(source_native_history_file_isolation=False)),
        ('missing-observation',lambda p:p['observations'].pop())]:
        bad=copy.deepcopy(last);mutate(bad)
        try:check(bad,fixture,last['group'])
        except ValueError:negatives.append(label)
        else:raise ValueError('Accepted corrupt unit evidence '+label)
    original=stage/'original-orbit-dependent-v1';r=load(original/'receipt.json');field=load(original/'latest-field.json')
    require(r['status']=='failed'and r['returncode']==1 and r['original_assertions_unchanged'],'Original blank-unit failure was relabeled')
    require(all(sha(original/n)==h for n,h in r['artifacts'].items()),'Original failure artifact changed')
    authorized=load(repo/'build/nasa-coverage/coverage95-20261110-aggregate-regression-v1/authorized-test-input-hashes-v1.json')
    for n in ['src/qtgui/tests/NasaOrbitConversionTests.cpp','src/qtgui/tests/NasaOrbitConversionCases.json']:
        require(r['source_sha256'][n]==authorized[n],'Original protected orbit test changed')
    bounds=load(repo/'src/qtgui/tests/NasaOrbitConversionCases.json')['displayed_field_tolerance']
    require('FAIL Displayed conversion units differ'in(original/'test.log').read_text()and field['field']=='ECC'and field['expected_unit']==''and field['unit']=='dimensionless','Original failure cause differs')
    require(abs(field['actual']-field['expected'])<=bounds,'Original field also failed its numerical gate')
    require(sum(x['unit_observations']for x in results)==294 and sum(x['cases']for x in results)==82,'Wrong total unit census')
    return dict(status='passed-bounded-explicit-dimensionless-unit-evidence',results=results,unit_observations=294,GUI_contexts=10,
        distinct_cases=41,original_protected_unit_string_failure_retained=True,original_failure_field=field,
        original_numeric_field_bound=bounds,negative_controls=negatives,native_tests_launched_by_audit=0,
        mission_executions=0,new_requirement_credit=0,qualified_keys=[],full_current_build_qualified=False,
        remaining='Complete non-dimensional input/constructor census, count/identifier meanings, conditional coefficients, solver/expression and array units remain to reconcile; no whole All Others!E13 admission.')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve())
    with a.output.open('x')as f:json.dump(result,f,indent=2);f.write('\n')
    print(json.dumps({k:v for k,v in result.items()if k!='results'}))
