"""Audit source-versioned body-unit labels without assigning whole-row credit."""
from pathlib import Path
import hashlib,json,tarfile

load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()

def need(value,message):
    if not value:raise ValueError(message)

PHYSICAL={'Mu':'km³/s²','EquatorialRadius':'km','OrientationEpoch':'A1ModJulian',
 'RotationConstant':'deg','RotationRate':'deg/day','SpinAxisDECConstant':'deg',
 'SpinAxisDECRate':'deg/century','SpinAxisRAConstant':'deg','SpinAxisRARate':'deg/century',
 **{'3DModelRotation'+a:'deg'for a in 'XYZ'}}
OFFSETS={'3DModelOffset'+a:'km'for a in 'XYZ'}
OBJECTS=['Sun','Mercury','Venus','Earth','Luna','Mars','Jupiter','Saturn','Uranus','Neptune','Pluto',
 'UnitsMoon','UnitsPlanet','UnitsComet','UnitsAsteroid']

def audit(stage):
    stage=Path(stage);plan=load(stage/'frozen-plan-v1.json');repair=load(stage/'implementation-plan-v1.json')
    versions={}
    for version in ['before-v1','after-v1']:
        ids=load(stage/('source-sha256-'+version+'.json'));build=load(stage/('build-'+version+'.json'))
        need(build['status']=='passed' and build['inputs_unchanged'] and build['returncode']==0 and
             build['source_sha256']==ids and build['log_sha256']==sha(stage/('build-'+version+'.log')),'Body-unit build/source mismatch')
        with tarfile.open(stage/('sources-'+version+'.tar.gz'))as bundle:
            need(set(bundle.getnames())==set(ids),'Incomplete body-unit source archive')
            for name,h in ids.items():need(hashlib.sha256(bundle.extractfile(name).read()).hexdigest()==h,'Changed body-unit source')
            fixture=json.loads(bundle.extractfile('src/qtgui/tests/BodyPhysicalUnitCases.json').read())
            panel=bundle.extractfile('src/qtgui/CelestialBodyPanel.cpp').read()
        need(ids['src/qtgui/tests/BodyPhysicalUnitCases.json']==plan['fixture_sha256'],'Body unit expectations changed')
        versions[version]=dict(ids=ids,build=build,panel=panel,fixture=fixture)
    before,after=[versions[v]for v in ['before-v1','after-v1']]
    need(set(before['ids'])==set(after['ids']) and {n for n,h in before['ids'].items()if after['ids'][n]!=h}=={'src/qtgui/CelestialBodyPanel.cpp'},'Body-unit patch widened')
    need(before['ids']['src/qtgui/CelestialBodyPanel.cpp']==repair['preimage_sha256'],'Wrong body-unit preimage')
    old=b'add("Visualization","3DModelOffset"+axis,"Model offset "+axis);'
    new=b'add("Visualization","3DModelOffset"+axis,"Model offset "+axis+" (km)");'
    need(before['panel'].count(old)==1 and before['panel'].replace(old,new)==after['panel'],'Body-unit patch changed more than offset captions')
    need(all(after['build']['binary_sha256'][n]==h for n,h in before['build']['binary_sha256'].items()if Path(n).name.startswith('lib')),'Native engine libraries changed')
    fixture=after['fixture'];groups={g:[c for c in fixture['cases']if c['group']==g]for g in ['BodyOffsetPilot','BodyOffsets','BodyPhysical']}
    need([c['object']for c in groups['BodyOffsets']]==OBJECTS and [c['object']for c in groups['BodyPhysical']]==OBJECTS,'Body universe changed')
    need(len(groups['BodyOffsetPilot'])==1 and groups['BodyOffsetPilot'][0]['object']=='Sun','Pilot changed')
    for c in fixture['cases']:need(c['route']=='body' and c['units']==(PHYSICAL if c['group']=='BodyPhysical'else OFFSETS),'Changed independent unit expectation')
    passes=failures=0
    runs=[('headless','before-v1','pilot',['BodyOffsetPilot'],3),
          ('headless','before-v1','physical',['BodyPhysical'],0),
          ('headless','after-v1','offsets',['BodyOffsets'],0),
          ('visible','after-v1','all',['BodyOffsets','BodyPhysical'],0)]
    for profile,version,selection,selected,expected_failures in runs:
        key='gui-'+profile+'-'+version+'-'+selection;root=stage/key
        receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json')
        need(receipt['inputs_unchanged'] and receipt['returncode']==(1 if expected_failures else 0),'Changed body-unit run result')
        need(all(receipt['source_sha256'].get(n)==h for n,h in versions[version]['ids'].items()),'Run source differs from build')
        for n,h in versions[version]['build']['binary_sha256'].items():
            matches=[k for k,v in receipt['image_sha256'].items()if h==v and Path(k).parent==Path(n).parent and (k==n or Path(k).name.startswith(Path(n).name+'.'))]
            # GmatQt is a build dependency; the unit worker is the executed GUI.
            if Path(n).name!='GmatQt':need(len(matches)==1,'Run image differs from body-unit build')
        rows=[x for x in batch['results']if x['status']!='not-run']
        need(batch['mode']==profile and batch['end_reason']=='completed-selection' and len(rows)==len(selected),'Body-unit profile or selection differs')
        need({x['name']for x in rows}=={'QtGui.NativeBodyUnits'+g for g in selected},'Body-unit selected groups differ')
        for result in rows:
            group=result['name'].removeprefix('QtGui.NativeBodyUnits');run=root/result['name'];proof=load(run/'proof.json')
            need(result==load(run/'result.json') and result['requested_visibility']==profile,'Body-unit result binding differs')
            emitted=[json.loads(line.split('NASA_CHECK ',1)[1])for line in (run/'test.log').read_text().splitlines()if 'NASA_CHECK 'in line]
            need(emitted==[proof] and proof['observations']==load(run/'unit-observations.json'),'Body-unit raw observation differs')
            need('QT_QPA_PLATFORM=xcb' in result['registered_command'] and result['trace']['instrumented_processes']==1,'Body-unit native widget route missing')
            if profile=='visible':need(result['trace']['native_exposure_events']>0,'Body-unit visible exposure missing')
            want=[(c['key'],f,u)for c in groups[group]for f,u in sorted(c['units'].items())]
            need(proof['cases']==[c['key']for c in groups[group]] and len(proof['observations'])==len(want),'Body-unit field universe differs')
            bad=0
            for actual,(case,field,unit) in zip(proof['observations'],want):
                need((actual['case'],actual['field'],actual['expected_unit'],actual['route'])==(case,field,unit,'body'),'Body-unit observation identity differs')
                ok=actual['actual_label'].endswith('('+unit+')')
                need(actual['passed']==ok,'Body-unit caption does not meet exact suffix rule')
                bad+=not ok
            need(bad==expected_failures and proof['mismatches']==bad and proof['status']==('failed'if bad else'passed'),'Body-unit failures changed')
            need(proof['mission_executions']==0 and proof['source_native_history_file_isolation'] and proof['new_requirement_credit']==0,'Body-unit check changed state or claimed credit')
            need(result['status']==('failed'if bad else'passed') and result['returncode']==(8 if bad else 0),'Original body-unit process result changed')
            failures+=bad;passes+=len(want)-bad
    return dict(status='passed',body_objects=15,default_bodies=11,user_body_types=4,
                passing_GUI_observations=passes,retained_failed_GUI_observations=failures,
                physical_fields_per_body=12,offset_fields_per_body=3,reused_headless_observations=180,
                native_engine_changed=False,mission_executions=0,native_tests_run_by_replay=0,
                original_assertions_changed=0,new_requirement_credit=0,whole_row_qualified=False,
                full_current_build_qualified=False,full_regression=False)
