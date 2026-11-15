"""Replay command unit display witnesses and their native-state isolation."""
from pathlib import Path
import hashlib,json,tarfile
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value,message):
    if not value:raise ValueError(message)
def expected(key,phase='initial'):
    if key=='VaryDistance':
        unit='km'if phase=='initial'else'dimensionless'
        return {f:('1/(km)'if f=='MultiplicativeScaleFactor'and unit=='km'else unit)for f in ['Variable','Initial value','Perturbation','Lower','Upper','MaxStep','AdditiveScaleFactor','MultiplicativeScaleFactor']}
    if key.startswith('Achieve'):
        unit={'AchieveDistance':'km','AchieveTime':'s'}.get(key,'user-defined units')
        return {f:unit for f in ['Goal','Value','Tolerance']}
    if key=='Minimize':return {'Objective':'kg'}
    if key=='Constraint':return {'Left side':'km','Right side':'km'}
    if key=='Assignment':return {'Destination':'kg','Expression':'kg'}
    if key=='For':return {f:'user-defined units'for f in ['Index','Start','Step','End']}
    raise ValueError('Unknown command unit case')
def observations(proof,cases,failures):
    sequence=[]
    for case in cases:
        need(case['units']==expected(case['key']),'Initial command unit definition differs')
        sequence.extend((case['key'],'initial',f,u)for f,u in sorted(case['units'].items()))
        if 'transition'in case:
            transition=case['transition'];need(transition['field']=='Variable'and transition['value']=='Sat.ECC'and transition['units']==expected(case['key'],'changed-control'),'Control unit transition changed')
            sequence.extend((case['key'],'changed-control',f,u)for f,u in sorted(transition['units'].items()))
    need(proof['cases']==[c['key']for c in cases]and len(sequence)==len(proof['observations']),'Incomplete command unit observations')
    bad=0
    for row,(key,phase,field,unit)in zip(proof['observations'],sequence):
        need((row['case'],row['phase'],row['field'],row['expected_unit'])==(key,phase,field,unit),'Command observation identity changed')
        correct='('+unit+')'in row['actual_label'];need(correct==row['passed'],'Command observation flag differs');bad+=not correct
    need(bad==failures==proof['mismatches']and proof['status']==('failed'if bad else'passed'),'Command unit disposition changed')
    need(proof['native_source_history_file_and_parameter_inventory_unchanged']and proof['mission_executions']==0 and proof['new_requirement_credit']==0,'Unit display changed native/parameter/source state')
def audit(stage):
    stage=Path(stage);versions={};paths=['src/qtgui/ParameterReferences.cpp','src/qtgui/CommandForm.cpp','src/qtgui/tests/CommandInputUnitCases.json']
    for v in ['before-v1','after-v1','after-v2','after-v3']:
        ids=load(stage/('source-sha256-'+v+'.json'));build=load(stage/('build-'+v+'.json'));raw={}
        need(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==ids and build['log_sha256']==sha(stage/('build-'+v+'.log')),'Command unit build identity differs')
        with tarfile.open(stage/('sources-'+v+'.tar.gz'))as archive:
            need(set(archive.getnames())==set(ids),'Command source archive incomplete')
            for n,h in ids.items():
                data=archive.extractfile(n).read();need(hashlib.sha256(data).hexdigest()==h,'Command source archive changed')
                if n in paths:raw[n]=data
        versions[v]=(ids,build,raw)
    before,first,second,after=[versions[v]for v in ['before-v1','after-v1','after-v2','after-v3']]
    changed=lambda a,b:{n for n,h in a[0].items()if b[0][n]!=h}
    prod={'src/qtgui/CommandForm.cpp','src/qtgui/CommandForm.hpp','src/qtgui/ParameterReferences.cpp','src/qtgui/ParameterReferences.hpp'}
    need(changed(before,first)==prod,'Initial unit patch changed unrelated source')
    need(changed(first,second)=={'src/qtgui/ResourceProperties.cpp','src/qtgui/CMakeLists.txt','src/qtgui/tests/CommandUnitDisplayTests.cpp'},'Mass follow-up changed unrelated source')
    need(changed(second,after)=={'src/qtgui/CMakeLists.txt'},'Build-target registration changed unrelated source')
    need(load(stage/'implementation-plan-v1.json')['production_files']=={n:before[0][n]for n in prod},'Command unit preimage differs')
    need(before[0][paths[2]]==after[0][paths[2]]==load(stage/'frozen-plan-v1.json')['fixture_sha256'],'Frozen command expectations changed')
    helper=after[2][paths[0]].decode()
    need('CreateParameter(field.toStdString(),"",owner)'in helper and '->Evaluate'not in helper and 'SetRefObject'not in helper,'Unit lookup evaluates or registers a parameter')
    for n,h in before[1]['binary_sha256'].items():
        if Path(n).name.startswith('lib'):need(after[1]['binary_sha256'][n]==h,'Command labels changed native libraries')
    for n,h in load(stage/'definition-source-sha256-v1.json').items():need(sha(stage/'definitions'/n)==h,'Independent command definition changed')
    info=(stage/'definitions/src/base/parameter/ParameterInfo.cpp').read_text()
    need('pos = name.find_last_of(".")'in info and 'if (pos == name.npos)'in info,'Empty prototype name registration contract changed')
    fixture=json.loads(after[2][paths[2]])
    runs=[('headless','before-v1','all',40),('headless','after-v1','all',3),('headless','after-v3','repair',0),('visible','after-v3','all',0)]
    passes=failures=0
    for mode,v,selection,bad in runs:
        key='gui-'+mode+'-'+v+'-'+selection;root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json');ids,build,_=versions[v]
        need(receipt['inputs_unchanged']and receipt['returncode']==int(bool(bad))and all(receipt['source_sha256'].get(n)==h for n,h in ids.items()),'Command runtime provenance differs')
        for n,h in build['binary_sha256'].items():
            if Path(n).name=='GmatQt':continue
            need(any(value==h and Path(p).parent==Path(n).parent and(p==n or Path(p).name.startswith(Path(n).name+'.'))for p,value in receipt['image_sha256'].items()),'Command runtime image differs')
        name='QtGui.NativeCommandInputUnits'+('MassRepair'if selection=='repair'else'')
        need([x['name']for x in batch['results']if x['status']!='not-run']==[name]and batch['mode']==mode and batch['end_reason']=='completed-selection','Command selection changed')
        folder=root/name;result=load(folder/'result.json');proof=load(folder/'proof.json')
        need(result['status']==('failed'if bad else'passed')and result['returncode']==(8 if bad else 0)and result['requested_visibility']==mode,'Command result changed')
        if mode=='visible':need(result['trace']['native_exposure_events']>0,'Command unit widgets not visibly exposed')
        need([json.loads(x.split('NASA_CHECK ',1)[1])for x in(folder/'test.log').read_text().splitlines()if'NASA_CHECK 'in x]==[proof],'Raw command proof differs')
        cases=[c for c in fixture['cases']if selection=='all'or c['key']in ['Minimize','Assignment']]
        observations(proof,cases,bad);passes+=len(proof['observations'])-bad;failures+=bad
    need(passes==80 and failures==43,'Command observation totals changed')
    return dict(status='passed',command_cases=10,passing_gui_observations=passes,preserved_failed_observations=failures,live_unit_transition=True,native_parameter_registry_unchanged=True,native_missions=0,new_requirement_credit=0,whole_row_qualified=False)
