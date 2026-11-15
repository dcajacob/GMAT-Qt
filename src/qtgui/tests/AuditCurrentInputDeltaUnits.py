"""Replay the current-census numeric delta without launching native tests."""
from pathlib import Path
import hashlib,json,tarfile
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,message):
    if not ok:raise ValueError(message)
EXPECTED={t:{'RootTolerance':'s'}for t in ['ContactLocator','EclipseLocator','IntrusionLocator']}
EXPECTED.update({t:{'IntegrationStepsToSkip':'steps'}for t in ['OrbitView','GroundTrack','GroundTrackPlot']})
EXPECTED.update({t:{'InitialEpoch':'A1ModJulian','SMA':'km','ECC':'dimensionless','INC':'deg','RAAN':'deg','AOP':'deg','TA':'deg'}for t in ['Asteroid','Comet','Moon','Planet']})
EXPECTED.update(TextEphemFile={'Interval':'s'},BulirschStoer={'MinimumTolerance':'error-metric units'},OffsetQuaternion={'OffsetQuaternion':'dimensionless'},FminconOptimizer={'MaximumIterations':'iterations'})
def covered():
    return {(('PropSetup'if key=='BulirschStoer'else'Spacecraft'if key=='OffsetQuaternion'else key),field)
            for key,fields in EXPECTED.items()for field in fields}
def observations(proof,cases,bad,initial=False):
    expected=[]
    for c in cases:
        units=dict(EXPECTED[c['key']])
        if initial and c['key']=='BulirschStoer':units['MinimumTolerance']='dimensionless'
        need(c['units']==units,'Independent delta unit definition changed')
        expected.extend((c['key'],f,u)for f,u in sorted(units.items()))
    need(len(proof['observations'])==len(expected)and proof['cases']==[c['key']for c in cases],'Delta inventory incomplete')
    failures=0
    for row,(key,field,unit)in zip(proof['observations'],expected):
        need((row['case'],row['field'],row['expected_unit'])==(key,field,unit),'Delta field identity changed')
        passed=unit in row['actual_label']if row['route']=='body'else row['actual_unit']==unit and unit in row['visible_unit_or_caption']
        need(passed==row['passed'],'Delta observation flag differs');failures+=not passed
    need(failures==bad==proof['mismatches']and proof['status']==('failed'if bad else'passed'),'Delta disposition changed')
    need(proof['source_native_history_file_isolation']and proof['mission_executions']==0 and proof['new_requirement_credit']==0,'Delta inspection changed state')

def audit(stage):
    stage=Path(stage);versions={};paths=['src/qtgui/ResourceProperties.cpp','doc/help/src/Resource_Propagator.xml','src/qtgui/tests/CurrentInputDeltaUnitCases.json']
    for v in ['before-v1','before-v2','before-v3','after-v1']:
        ids=load(stage/('source-sha256-'+v+'.json'));build=load(stage/('build-'+v+'.json'));raw={}
        need(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==ids and build['log_sha256']==sha(stage/('build-'+v+'.log')),'Delta build identity changed')
        with tarfile.open(stage/('sources-'+v+'.tar.gz'))as archive:
            need(set(archive.getnames())==set(ids),'Delta source archive incomplete')
            for n,h in ids.items():
                data=archive.extractfile(n).read();need(hashlib.sha256(data).hexdigest()==h,'Archived delta source changed')
                if n in paths:raw[n]=data
        versions[v]=(ids,build,raw)
    first,initial,before,after=[versions[v]for v in ['before-v1','before-v2','before-v3','after-v1']]
    need({n for n,h in first[0].items()if initial[0][n]!=h}=={'src/qtgui/CMakeLists.txt'},'Recipe correction exceeded new registrations')
    need({n for n,h in initial[0].items()if before[0][n]!=h}=={'src/qtgui/CMakeLists.txt',paths[2]},'Oracle correction changed other source')
    need({n for n,h in before[0].items()if after[0][n]!=h}==set(paths[:2]),'Unit repair changed unrelated source')
    need(load(stage/'implementation-plan-v1.json')['production_files']=={n:before[0][n]for n in paths[:2]},'Unit repair preimage changed')
    for n,h in before[1]['binary_sha256'].items():
        if Path(n).name.startswith('lib'):need(after[1]['binary_sha256'][n]==h,'Unit repair changed native code')
    oracle=load(stage/'corrected-unit-oracle-v1.json')
    need(oracle['initial_case_sha256']==sha(stage/'initial-unit-cases-v1.json')and oracle['corrected_fixture_sha256']==before[0][paths[2]],'Oracle versions changed')
    defs=stage/'definitions'
    for n,h in load(stage/'definition-source-sha256-v1.json').items():need(sha(defs/n)==h,'Independent delta definition changed')
    bs=(defs/'plugins/ExtraPropagatorsPlugin/src/base/propagator/BulirschStoer.cpp').read_text()
    need('tolerance=(std::max)(requestedTolerance,mintolerance);'in bs and 'physicalModel->EstimateError(errorEstimates, estimatedState)'in bs,'MinimumTolerance no longer shares Accuracy metric')
    ode=(defs/'src/base/forcemodel/ODEModel.cpp').read_text()
    need('err = sqrt(err / mag);'in ode and 'err = sqrt(err);'in ode,'Normalized/absolute error branches changed')
    ephem=(defs/'src/base/subscriber/TextEphemFile.cpp').read_text()
    need('mIntervalInSec/GmatTimeConstants::SECS_PER_DAY'in ephem,'Text interval time unit changed')
    for typ in ['ContactLocator','EclipseLocator','IntrusionLocator']:
        need('RootTolerance must be finite and positive in seconds'in(defs/('plugins/EventLocatorPlugin/src/base/locator/'+typ+'.cpp')).read_text(),'Event root unit changed')
    need('same error-metric units'in after[2][paths[1]].decode(),'Guide correction omitted')
    runs=[('headless','before-v2','all',{'NewScalarUnits':7,'UserBodyOrbitUnits':0,'NewDimensionlessUnits':0,'OptionalSolverUnits':0},True),
          ('headless','before-v3','minimum',{'MinimumToleranceUnits':1},False),
          ('headless','after-v1','repair',{'NewScalarUnits':0,'MinimumToleranceUnits':0},False),
          ('visible','after-v1','all',{'NewScalarUnits':0,'UserBodyOrbitUnits':0,'NewDimensionlessUnits':0,'OptionalSolverUnits':0,'MinimumToleranceUnits':0},False)]
    passes=failures=invalid=0
    for mode,v,selection,groups,initial_oracle in runs:
        key='gui-'+mode+'-'+v+'-'+selection;root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json');ids,build,raw=versions[v]
        need(receipt['inputs_unchanged']and receipt['returncode']==int(any(groups.values()))and all(receipt['source_sha256'].get(n)==h for n,h in ids.items()),'Delta runtime source identity changed')
        for n,h in build['binary_sha256'].items():
            if Path(n).name=='GmatQt':continue
            need(any(value==h and Path(p).parent==Path(n).parent and(p==n or Path(p).name.startswith(Path(n).name+'.'))for p,value in receipt['image_sha256'].items()),'Delta runtime image changed')
        need({x['name']for x in batch['results']if x['status']!='not-run'}=={'QtGui.NativeCurrentInputDeltaUnits'+g for g in groups}and batch['end_reason']=='completed-selection','Delta selection changed')
        fixture=json.loads(raw[paths[2]])
        for group,bad in groups.items():
            folder=root/('QtGui.NativeCurrentInputDeltaUnits'+group);result=load(folder/'result.json');proof=load(folder/'proof.json')
            need(result['status']==('failed'if bad else'passed')and result['returncode']==(8 if bad else 0)and result['requested_visibility']==mode,'Delta case result changed')
            if mode=='visible':need(result['trace']['native_exposure_events']>0,'Delta GUI not visibly exposed')
            need([json.loads(x.split('NASA_CHECK ',1)[1])for x in(folder/'test.log').read_text().splitlines()if'NASA_CHECK 'in x]==[proof]and proof['observations']==load(folder/'unit-observations.json'),'Delta raw proof changed')
            observations(proof,[c for c in fixture['cases']if c['group']==group],bad,initial_oracle)
            exclusion=int(initial_oracle and group=='NewDimensionlessUnits')
            invalid+=exclusion;passes+=len(proof['observations'])-bad-exclusion;failures+=bad
    need(passes==76 and failures==8 and invalid==1,'Delta observation totals changed')
    return dict(status='passed',new_numeric_fields_reviewed=38,passing_GUI_observations=passes,preserved_unit_failures=failures,excluded_initial_incorrect_oracle_passes=invalid,original_case_and_result_retained=True,native_missions=0,new_requirement_credit=0,whole_row_qualified=False,full_regression=False)
