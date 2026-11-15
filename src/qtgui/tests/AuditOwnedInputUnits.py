"""Replay previously untyped/nested input unit witnesses, without native runs."""
from pathlib import Path
import hashlib,json,tarfile
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,message):
    if not ok:raise ValueError(message)
EXPECTED={p+a:'dimensionless'for p in ['AlignmentVector','ConstraintVector','ConstraintReferenceVector']for a in 'XYZ'}
EXPECTED={'FrameDirections':EXPECTED,'FminconOptions':{'DiffMaxChange':'scaled-variable units','DiffMinChange':'scaled-variable units','MaxFunEvals':'evaluations','TolX':'dimensionless or scaled-variable units','TolFun':'dimensionless or objective/gradient units','TolCon':'constraint units'},'ArrayValues':{'RmatValue':'user-defined per element'},'VariableValue':{'Value':'user-defined units'}}
def observations(proof,cases,failures):
    expected=[]
    for case in cases:
        need(case['units']==EXPECTED[case['key']],'Owned unit expectation differs')
        expected.extend((case['key'],field,unit)for field,unit in sorted(case['units'].items()))
        if case.get('grid_unit_notice'):
            need(case['grid_unit_notice']=='user-defined per element','Array notice expectation changed')
            expected.append((case['key'],'RmatValue',case['grid_unit_notice']))
    need(proof['cases']==[c['key']for c in cases]and len(proof['observations'])==len(expected),'Owned observations incomplete')
    bad=0
    for row,(key,field,unit)in zip(proof['observations'],expected):
        need((row['case'],row['field'],row['expected_unit'])==(key,field,unit),'Owned observation identity changed')
        correct=unit in row['actual_label']if row['route']in ['primary','array-grid']else row['actual_unit']==unit and unit in row['visible_unit_or_caption']
        need(row['passed']==correct,'Owned unit result differs');bad+=not correct
    need(bad==failures==proof['mismatches']and proof['status']==('failed'if bad else'passed'),'Owned pass/fail disposition differs')
    need(proof['mission_executions']==0 and proof['source_native_history_file_isolation']and proof['new_requirement_credit']==0,'Owned inspection changed state')
def covered():
    mapping={'FrameDirections':'CoordinateSystem','FminconOptions':'FminconOptimizer','ArrayValues':'Array','VariableValue':'Variable'}
    return {(mapping[key],field)for key,fields in EXPECTED.items()for field in fields}
def audit(stage):
    stage=Path(stage);versions={};paths=['src/qtgui/ResourceProperties.cpp','src/qtgui/ResourceEditor.cpp','src/qtgui/tests/OwnedInputUnitCases.json']
    for v in ['before-v1','before-v2','before-v3','before-v4','after-v1']:
        ids=load(stage/('source-sha256-'+v+'.json'));build=load(stage/('build-'+v+'.json'));raw={}
        need(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==ids and build['log_sha256']==sha(stage/('build-'+v+'.log')),'Owned build identity changed')
        with tarfile.open(stage/('sources-'+v+'.tar.gz'))as archive:
            need(set(archive.getnames())==set(ids),'Owned source archive incomplete')
            for n,h in ids.items():
                data=archive.extractfile(n).read();need(hashlib.sha256(data).hexdigest()==h,'Owned archive changed')
                if n in paths:raw[n]=data
        versions[v]=(ids,build,raw)
    before,after=versions['before-v4'],versions['after-v1']
    need({n for n,h in before[0].items()if after[0][n]!=h}==set(paths[:2]),'Owned unit repair changed unrelated source')
    need(load(stage/'implementation-plan-v1.json')['production_files']=={n:before[0][n]for n in paths[:2]},'Owned unit preimage differs')
    need(load(stage/'array-route-correction-v1.json')['fixture_sha256']==before[0][paths[2]],'Corrected array route changed')
    for n,h in before[1]['binary_sha256'].items():
        if Path(n).name.startswith('lib'):need(after[1]['binary_sha256'][n]==h,'Owned unit labels changed native libraries')
    for n,h in load(stage/'definition-source-sha256-v1.json').items():need(sha(stage/'definitions'/n)==h,'Owned unit definition changed')
    defs=stage/'definitions'
    axes=(defs/'src/base/coordsystem/LocalAlignedConstrainedAxes.cpp').read_text()
    need('alignmentVector/alignmentVector.GetMagnitude()'in axes and 'av_I/av_I.GetMagnitude()'in axes,'Direction normalization definition changed')
    driver=(defs/'application/matlab/gmat_fmincon/GmatFminconOptimizationDriver.m').read_text()
    need("'Algorithm','active-set'"in driver,'fmincon algorithm changed')
    native=(defs/'plugins/FminconOptimizerPlugin/src/base/solver/FminconOptimizer.cpp').read_text()
    need('variable.at(i)'in native and 'X0 = ['in native and 'GMAToptions = optimset('in native,'fmincon variable/options transport changed')
    refs=load(stage/'provider-unit-references-v1.json')
    need(refs['version']=='R2026b'and len(refs['sources'])==3 and all(x.startswith('https://www.mathworks.com/help/optim/')for x in refs['sources']),'Provider criteria references changed')
    fixture=json.loads(after[2][paths[2]])
    passes=failures=0
    for mode,v in [('headless','before-v4'),('headless','after-v1'),('visible','after-v1')]:
        key='gui-'+mode+'-'+v+'-all';root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json');ids,build,_=versions[v];bad=18 if v=='before-v4'else 0
        need(receipt['inputs_unchanged']and receipt['returncode']==int(bool(bad))and all(receipt['source_sha256'].get(n)==h for n,h in ids.items()),'Owned run provenance changed')
        for n,h in build['binary_sha256'].items():
            if Path(n).name=='GmatQt':continue
            need(any(value==h and Path(p).parent==Path(n).parent and(p==n or Path(p).name.startswith(Path(n).name+'.'))for p,value in receipt['image_sha256'].items()),'Owned runtime image changed')
        need([x['name']for x in batch['results']if x['status']!='not-run']==['QtGui.NativeOwnedInputUnits']and batch['end_reason']=='completed-selection','Owned test selection changed')
        folder=root/'QtGui.NativeOwnedInputUnits';result=load(folder/'result.json');proof=load(folder/'proof.json')
        need(result['returncode']==(8 if bad else 0)and result['status']==('failed'if bad else'passed')and result['requested_visibility']==mode,'Owned result changed')
        if mode=='visible':need(result['trace']['native_exposure_events']>0,'Owned inputs not visibly exposed')
        need([json.loads(x.split('NASA_CHECK ',1)[1])for x in(folder/'test.log').read_text().splitlines()if'NASA_CHECK 'in x]==[proof]and proof['observations']==load(folder/'unit-observations.json'),'Owned raw proof changed')
        observations(proof,fixture['cases'],bad);passes+=len(proof['observations'])-bad;failures+=bad
    for v,count in [('before-v1',15),('before-v2',15),('before-v3',15)]:
        folder=stage/('gui-headless-'+v+'-all')/'QtGui.NativeOwnedInputUnits';result=load(folder/'result.json')
        need(result['status']=='failed'and 'Unit control is hidden'in(folder/'test.log').read_text(),'Original interaction failure missing')
        path=folder/'unit-observations.json';partial=load(path)if path.exists()else[]
        need(len(partial)==count and not any(row['passed']for row in partial),'Partial failure receipt changed')
    need(passes==36 and failures==18 and len(covered())==17,'Owned unit totals changed')
    return dict(status='passed',numeric_fields=17,passing_gui_observations=36,preserved_complete_unit_failures=18,preserved_partial_unit_failures=45,preserved_interaction_failures=3,source_native_history_file_isolation=True,native_missions=0,external_provider_calls=0,new_requirement_credit=0,whole_row_qualified=False)
