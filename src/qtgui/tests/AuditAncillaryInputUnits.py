"""Replay propagation, condition and comparison unit evidence only."""
from pathlib import Path
import hashlib,json,tarfile
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,message):
    if not ok:raise ValueError(message)
def expected():
    rows={}
    for key,unit in [('time','s'),('days-transition','days'),('distance','km'),('user','user-defined units'),('apsis','dimensionless')]:
        for control in ['propagationParameterUnit','propagationValueUnit','propagationToleranceUnit']:
            rows[key,control]='(' + ('no goal'if key=='apsis'and control=='propagationValueUnit'else unit) + ')'
    for i,text in enumerate(['Goal and StopTolerance units: s','Goal and StopTolerance units: km','Event residual units: dimensionless; no goal']):rows['stop-row-'+str(i),'propagationStopUnits']=text
    for i,text in enumerate(['Left: km; Right: km','Left: s; Right: s','Left: user-defined units; Right: user-defined units']):rows['condition-row-'+str(i),'conditionUnits']=text
    rows['condition-changed','conditionUnits']='Left: dimensionless; Right: dimensionless'
    rows['comparison','comparisonToleranceUnit']='(column units)'
    rows['folder-comparison','folderToleranceUnit']='(column units)'
    return rows
def observations(proof,bad):
    wanted=expected();seen=set();failures=0
    for row in proof['observations']:
        key=row['key'],row['control'];need(key not in seen and key in wanted,'Duplicate or unknown ancillary control');seen.add(key)
        need(row['expected']==wanted[key],'Ancillary definition changed')
        passed=wanted[key]in row['actual'];need(row['passed']==passed,'Ancillary result flag differs');failures+=not passed
    need(seen==set(wanted)and len(seen)==24,'Ancillary unit inventory incomplete')
    need(failures==bad==proof['mismatches']and proof['status']==('failed'if bad else'passed'),'Ancillary unit disposition differs')
    need(proof['source_native_history_file_and_parameter_inventory_unchanged']and proof['mission_executions']==0 and proof['new_requirement_credit']==0,'Ancillary UI changed native/source state')
def audit(stage):
    stage=Path(stage);versions={}
    for v in ['before-v1','before-v2','after-v1']:
        ids=load(stage/('source-sha256-'+v+'.json'));build=load(stage/('build-'+v+'.json'))
        need(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==ids and build['log_sha256']==sha(stage/('build-'+v+'.log')),'Ancillary build identity changed')
        with tarfile.open(stage/('sources-'+v+'.tar.gz'))as archive:
            need(set(archive.getnames())==set(ids),'Ancillary source archive incomplete')
            for n,h in ids.items():need(hashlib.sha256(archive.extractfile(n).read()).hexdigest()==h,'Ancillary archived source changed')
        versions[v]=(ids,build)
    early,before,after=[versions[v]for v in ['before-v1','before-v2','after-v1']]
    need({n for n,h in early[0].items()if before[0][n]!=h}=={'src/qtgui/tests/AncillaryUnitDisplayTests.cpp'},'Interaction correction changed other source')
    production=load(stage/'implementation-plan-v1.json')['production_files']
    need({n for n,h in before[0].items()if after[0][n]!=h}==set(production)and len(production)==7,'Ancillary unit changes exceed planned files')
    need(all(before[0][n]==h for n,h in production.items()),'Ancillary preimage differs')
    for n,h in before[1]['binary_sha256'].items():
        if Path(n).name.startswith('lib'):need(after[1]['binary_sha256'][n]==h,'Ancillary unit changes altered native libraries')
    for n,h in load(stage/'definition-source-sha256-v1.json').items():need(sha(stage/'definitions'/n)==h,'Ancillary definition changed')
    orbital=(stage/'definitions/src/base/parameter/OrbitalParameters.cpp').read_text()
    need(orbital.count('pos.GetUnitVector()')>=2 and orbital.count('vel.GetUnitVector()')>=2 and 'mRealValue = R*V;'in orbital,'Apsis residual is not the normalized dot product')
    stop=(stage/'definitions/src/base/stopcond/StopCondition.cpp').read_text();prop=(stage/'definitions/src/base/command/Propagate.cpp').read_text()
    need('const Real difference = goalValue - achievedValue;'in stop and 'case DAY_PARAM:'in stop and 'timeAccuracy = value;'in prop,'Stopping tolerance convention changed')
    compare=(stage/'definitions/src/qtgui/FileComparison.cpp').read_text();need('std::abs(a[column]-b[column])'in compare,'Comparison no longer uses stored numerical differences')
    passes=failures=0
    for mode,v,bad in [('headless','before-v2',24),('headless','after-v1',0),('visible','after-v1',0)]:
        key='gui-'+mode+'-'+v+'-all';root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json');ids,build=versions[v]
        need(receipt['inputs_unchanged']and receipt['returncode']==int(bool(bad))and all(receipt['source_sha256'].get(n)==h for n,h in ids.items()),'Ancillary runtime source changed')
        for n,h in build['binary_sha256'].items():
            if Path(n).name=='GmatQt':continue
            need(any(value==h and Path(p).parent==Path(n).parent and(p==n or Path(p).name.startswith(Path(n).name+'.'))for p,value in receipt['image_sha256'].items()),'Ancillary runtime image differs')
        need([r['name']for r in batch['results']if r['status']!='not-run']==['QtGui.NativeAncillaryInputUnits']and batch['end_reason']=='completed-selection','Ancillary selection differs')
        folder=root/'QtGui.NativeAncillaryInputUnits';result=load(folder/'result.json');proof=load(folder/'proof.json')
        need(result['status']==('failed'if bad else'passed')and result['returncode']==(8 if bad else 0)and result['requested_visibility']==mode,'Ancillary case result differs')
        if mode=='visible':need(result['trace']['native_exposure_events']>0,'Ancillary units not visibly exposed')
        need([json.loads(x.split('NASA_CHECK ',1)[1])for x in(folder/'test.log').read_text().splitlines()if'NASA_CHECK 'in x]==[proof]and proof['observations']==load(folder/'unit-observations.json'),'Ancillary raw observations changed')
        observations(proof,bad);passes+=24-bad;failures+=bad
    pilot=stage/'gui-headless-before-v1-all/QtGui.NativeAncillaryInputUnits'
    need(load(pilot/'result.json')['status']=='failed'and 'FAIL Condition cell editor missing'in(pilot/'test.log').read_text(),'Original ancillary interaction failure missing')
    return dict(status='passed',passing_gui_observations=passes,preserved_unit_failures=failures,preserved_interaction_failures=1,apsis_units='dimensionless normalized position/velocity dot product',live_time_and_operand_units=True,source_native_history_file_and_parameter_inventory_unchanged=True,native_missions=0,new_requirement_credit=0,whole_row_qualified=False)
