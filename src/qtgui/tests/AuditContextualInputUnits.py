"""Replay context-dependent unit descriptions and selected GUI paths."""
from pathlib import Path
import hashlib,json,tarfile,xml.etree.ElementTree as ET
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value,message):
    if not value:raise ValueError(message)
NUMERIC=['RungeKutta89','PrinceDormand78','PrinceDormand45','RungeKutta68','RungeKutta56','PrinceDormand853','BulirschStoer','AdamsBashforthMoulton']
FILES=['SPK','Code500','STK','CCSDS-OEM','SPICESGP4']
FIXED={'BatchEstimator':{f:'weighted-residual units'for f in ['AbsoluteTol','OLSEAdditiveConstant','OLSEInitialRMSSigma']},
 'BatchEstimatorInv':{f:'weighted-residual units'for f in ['AbsoluteTol','OLSEAdditiveConstant','OLSEInitialRMSSigma']},
 'ExtendedKalmanFilter':{'MeasDeweightingSigmaThreshold':'km'},'Smoother':{'MeasDeweightingSigmaThreshold':'km'},
 'VF13ad':{'FeasibilityTolerance':'constraint units','Tolerance':'objective units'},
 'Yukon':{'FeasibilityTolerance':'constraint units','FunctionTolerance':'dimensionless or objective units','MaximumElasticWeight':'objective / residual units','OptimalityTolerance':'Lagrangian-gradient units'}}
def expected(key):
    if key in FIXED:return FIXED[key]
    if key in NUMERIC:
        fields={'Accuracy':'error-metric units','InitialStepSize':'s','MinStep':'s','MaxStep':'s'}
        if key=='AdamsBashforthMoulton':fields.update(LowerError='error-metric units',TargetError='error-metric units')
        return fields
    if key in FILES:return {'InitialStepSize'if key=='SPICESGP4'else'StepSize':'s'}
    if key=='Thinning':return {'ThinningFrequency':'records'}
    raise ValueError('Unknown contextual unit case')
def covered():
    result={(t,f)for t,fields in FIXED.items()for f in fields}
    result|={('PropSetup',f)for key in NUMERIC+FILES for f in expected(key)}
    result.add(('AcceptFilter','ThinningFrequency'));return result
def observations(proof,cases,bad):
    sequence=[]
    for case in cases:
        need(case['units']==expected(case['key']),'Contextual expectation is not independent')
        sequence.extend((case['key'],'initial',f,u)for f,u in sorted(case['units'].items()))
        if case['key']=='Thinning':
            selector={'field':'ThinMode','transitions':[{'value':'Time','units':{'ThinningFrequency':'s'}},{'value':'Frequency','units':{'ThinningFrequency':'records'}}]}
            need(case['unit_selector']==selector,'Thinning unit transition contract changed')
            sequence.extend([('Thinning','ThinMode=Time','ThinningFrequency','s'),('Thinning','ThinMode=Frequency','ThinningFrequency','records')])
    need(proof['cases']==[c['key']for c in cases]and len(proof['observations'])==len(sequence),'Contextual observation inventory differs')
    failures=0
    for row,(key,phase,field,unit)in zip(proof['observations'],sequence):
        need((row['case'],row['phase'],row['field'],row['expected_unit'])==(key,phase,field,unit),'Contextual unit identity changed')
        correct=row['actual_unit']==unit and unit in row['visible_unit_or_caption'];need(row['passed']==correct,'Contextual result changed');failures+=not correct
    need(failures==bad==proof['mismatches']and proof['status']==('failed'if bad else'passed'),'Contextual pass/failure changed')
    need(proof['source_native_history_file_isolation']and proof['mission_executions']==0 and proof['new_requirement_credit']==0,'Contextual inspection changed source/state')
def definitions(stage):
    ids=load(stage/'definition-source-sha256-v1.json')
    for n,h in ids.items():need(sha(stage/'definitions'/n)==h,'Contextual definition changed')
    N={'d':'http://docbook.org/ns/docbook'};docs={}
    for n in ids:
        if not n.endswith('.xml'):continue
        rows={}
        for row in ET.parse(stage/'definitions'/n).getroot().findall('.//d:row',N):
            entry=row.find('d:entry',N)
            if entry is None:continue
            names=[''.join(x.itertext()).strip()for x in entry.findall('.//d:guilabel',N)]
            for v in row.findall('.//d:varlistentry',N):
                if ''.join(v.find('d:term',N).itertext()).strip()=='Units':
                    value=' '.join(''.join(v.find('d:listitem',N).itertext()).split())
                    for name in names:rows.setdefault(name,set()).add(value)
        docs[Path(n).stem.removeprefix('Resource_')]=rows
    fragments={'weighted-residual units':'Weighted-residual','km':'km','constraint units':'Constraint-residual','objective units':'objective-value',
      'dimensionless or objective units':'Dimensionless relative objective change','objective / residual units':'per elastic-residual','Lagrangian-gradient units':'Lagrangian-gradient'}
    for typ,fields in FIXED.items():
        doc=docs['BatchEstimator'if typ=='BatchEstimatorInv'else typ]
        for f,u in fields.items():need(any(fragments[u]in v for v in doc[f]),'Documented contextual meaning differs: '+typ+'.'+f)
    for f in ['Accuracy','LowerError','TargetError']:need(any('normalized branches' in v and 'absolute fallbacks'in v for v in docs['Propagator'][f]),'Integrator error scale is not conditional')
    need(any('ThinMode=Frequency'in v and 'seconds for ThinMode=Time'in v for v in docs['AcceptFilter']['ThinningFrequency']),'Thinning unit dependence lost')
    root=stage/'definitions'
    weight=(root/'plugins/EstimationPlugin/src/base/estimator/Estimator.cpp').read_text();need('if (cov == 0.0)\n      return 1.0;'in weight and 'return 1.0 / cov;'in weight,'Covariance weighting branches changed')
    batch=(root/'plugins/EstimationPlugin/src/base/estimator/BatchEstimator.cpp').read_text();need('value = sqrt(value / count);'in batch and 'constMult*sigmaVal + additiveConst'in batch,'Weighted residual dimensions changed')
    yukon=(root/'plugins/YukonOptimizerPlugin/src/base/solver/Yukon.cpp').read_text();need('if (std::abs(f) > 1e-7)'in yukon and 'std::abs((fnew - f) / f)'in yukon and 'std::abs(fnew - f)'in yukon and 'InfNorm(gradLag) < options.tolGrad'in yukon,'Optimizer unit branches changed')
    ekf=(root/'plugins/ExtendedKalmanFilterPlugin/src/base/EKF/ExtendedKalmanFilter.cpp').read_text();need('Sqrt(pBar(0, 0) + pBar(1, 1) + pBar(2, 2))'in ekf and 'posCovTraceSqrt > deweightThreshold'in ekf,'Position covariance threshold meaning changed')
    thinning=(root/'plugins/EstimationPlugin/src/base/datafilter/AcceptFilter.cpp').read_text();need('recCountMap[trackingConfig] == (thinningFrequency - 1)'in thinning and 'GmatTimeConstants::SECS_PER_DAY / thinningFrequency'in thinning,'Native thinning dimensions changed')
def audit(stage):
    stage=Path(stage);definitions(stage);versions={};paths=['src/qtgui/ResourceProperties.cpp','src/qtgui/ResourceEditor.cpp','src/qtgui/tests/ContextualUnitCases.json']
    for v in ['before-v1','before-v2','after-v1']:
        ids=load(stage/('source-sha256-'+v+'.json'));build=load(stage/('build-'+v+'.json'));raw={}
        need(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==ids and build['log_sha256']==sha(stage/('build-'+v+'.log')),'Contextual build identity changed')
        with tarfile.open(stage/('sources-'+v+'.tar.gz'))as archive:
            need(set(archive.getnames())==set(ids),'Contextual source archive incomplete')
            for n,h in ids.items():
                data=archive.extractfile(n).read();need(hashlib.sha256(data).hexdigest()==h,'Contextual archived source changed')
                if n in paths:raw[n]=data
        versions[v]=(ids,build,raw)
    early,before,after=[versions[v]for v in ['before-v1','before-v2','after-v1']];props,editor,fixture=paths
    need({n for n,h in early[0].items()if before[0][n]!=h}=={fixture},'Propagator fixture correction changed unrelated source')
    need({n for n,h in before[0].items()if after[0][n]!=h}=={props,editor},'Contextual repair changed unrelated source')
    a=after[2][props].index(b'         if (field.unit.isEmpty() && object.IsOfType("BatchEstimator") &&');b=after[2][props].index(b'         // Physical inputs whose native classes',a)
    delta=after[2][props][a:b];need(after[2][props][:a]+after[2][props][b:]==before[2][props]and b'SetRealParameter'not in delta,'Contextual metadata repair changes values')
    a=after[2][editor].index(b'   if (object.IsOfType("AcceptFilter")) {');b=after[2][editor].index(b'   if (object.IsOfType("EventLocator")) {',a)
    delta=after[2][editor][a:b];need(after[2][editor][:a]+after[2][editor][b:]==before[2][editor]and b'QtResourcePreview copy(initial->Clone());'in delta and b'copy->SetStringParameter("ThinMode"'in delta,'Thinning refresh is not detached')
    for n,h in load(stage/'implementation-plan-v1.json')['production_files'].items():need(before[0][n]==h,'Contextual preimage differs')
    need(all(after[1]['binary_sha256'][n]==h for n,h in before[1]['binary_sha256'].items()if Path(n).name.startswith('lib')),'Contextual labels changed native libraries')
    fixtures={v:json.loads(data[2][fixture])for v,data in versions.items()}
    need(hashlib.sha256(before[2][fixture]).hexdigest()==load(stage/'fixture-correction-v1.json')['fixture_sha256'],'Corrected contextual fixture changed')
    need(len(covered())==22,'Contextual field mapping differs')
    runs=[('headless','before-v1','all',{'ResidualUnits':8,'OptimizerUnits':6,'ThinningUnits':3},True),('headless','before-v2','propagators',{'PropagatorUnits':39},False),('headless','after-v1','all',{'ResidualUnits':0,'OptimizerUnits':0,'PropagatorUnits':0,'ThinningUnits':0},False),('visible','after-v1','all',{'ResidualUnits':0,'OptimizerUnits':0,'PropagatorUnits':0,'ThinningUnits':0},False)]
    passes=failures=0
    for mode,v,selection,groups,incomplete in runs:
        key='gui-'+mode+'-'+v+'-'+selection;root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json');ids,build,_=versions[v]
        need(receipt['inputs_unchanged']and receipt['returncode']==(1 if any(groups.values())else 0)and all(receipt['source_sha256'].get(n)==h for n,h in ids.items()),'Contextual runtime identity changed')
        for n,h in build['binary_sha256'].items():
            if Path(n).name=='GmatQt':continue
            need(any(value==h and Path(p).parent==Path(n).parent and(p==n or Path(p).name.startswith(Path(n).name+'.'))for p,value in receipt['image_sha256'].items()),'Contextual runtime image changed')
        names=set(groups)|({'PropagatorUnits'}if incomplete else set());actual={r['name']for r in batch['results']if r['status']!='not-run'}
        need(actual=={'QtGui.NativeContextualUnits'+g for g in names}and batch['mode']==mode and batch['end_reason']=='completed-selection','Contextual selection changed')
        for group,bad in groups.items():
            run=root/('QtGui.NativeContextualUnits'+group);result=load(run/'result.json');proof=load(run/'proof.json')
            need(result['status']==('failed'if bad else'passed')and result['returncode']==(8 if bad else 0)and result['requested_visibility']==mode,'Contextual case status changed')
            if mode=='visible':need(result['trace']['native_exposure_events']>0,'Contextual fields not visibly exposed')
            rows=[json.loads(x.split('NASA_CHECK ',1)[1])for x in(run/'test.log').read_text().splitlines()if'NASA_CHECK 'in x];need(rows==[proof]and proof['observations']==load(run/'unit-observations.json'),'Contextual raw observations changed')
            cases=[c for c in fixtures[v]['cases']if c['group']==group];observations(proof,cases,bad);passes+=len(proof['observations'])-bad;failures+=bad
    old=stage/'gui-headless-before-v1-all/QtGui.NativeContextualUnitsPropagatorUnits';missing=load(old/'SPICESGP4-missing-field.json')
    need(load(old/'result.json')['status']=='failed'and missing['field']=='StepSize'and len(load(old/'unit-observations.json'))==38,'Original SGP4 field-identity failure lost')
    need(passes==112 and failures==56,'Contextual observation count differs')
    return dict(status='passed',completed_saved_unit_records=22,passing_GUI_observations=112,retained_unit_failures=56,retained_fixture_failure_cases=1,propagator_types=13,thinning_modes=2,native_engine_changes=False,mission_executions=0,new_requirement_credit=0,whole_row_qualified=False,new_native_tests_run_by_replay=0)
