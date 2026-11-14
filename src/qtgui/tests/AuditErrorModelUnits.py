"""Replay measurement-type-dependent input-unit observations without tests."""
from pathlib import Path
import hashlib,json,re,tarfile,xml.etree.ElementTree as ET
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value,message):
    if not value:raise ValueError(message)
def meanings():
    groups={'RU':['DSN_SeqRange','DSN_PNRange'],'Hz':['DSN_TCP','SN_Doppler','SN_Doppler_Rtn','SN_DOWD','BRTS_Doppler'],
            'km':['GPS_PosVec','Range','Range_Skin','SN_Range','BRTS_Range'],'km/s':['RangeRate'],
            'deg':['Azimuth','Elevation','XEast','YNorth','XSouth','YEast','RightAscension','Declination']}
    return {name:unit for unit,names in groups.items()for name in names}
FIELDS=['Bias','BiasSigma','NoiseSigma']
STATIC=['Range','DSN_SeqRange','DSN_TCP','RangeRate','Azimuth']
TESTING=['RightAscension','Declination']
def definitions(stage):
    for name,h in load(stage/'definition-source-sha256-v1.json').items():need(sha(stage/'definitions'/name)==h,'Error unit reference changed')
    source=(stage/'definitions/plugins/EstimationPlugin/src/base/errormodel/ErrorModel.cpp').read_text()
    a=source.index('std::string ErrorModel::GetUnit(');b=source.index('return unit;',a)
    native=dict(re.findall(r'type == "([^"]+)"\)\s*unit = "([^"]+)"',source[a:b]))
    need(native==meanings(),'Native measurement unit assignment differs from independent meanings')
    a=source.index('StringArray ErrorModel::GetAllAvailableTypes()');b=source.index('return sa;',a)
    types=re.findall(r'sa.push_back\("([^"]+)"\)',source[a:b]);need(len(types)==21 and types[-2:]==TESTING and set(types)==set(meanings()),'Native measurement type inventory changed')
    N={'d':'http://docbook.org/ns/docbook'};root=ET.parse(stage/'definitions/doc/help/src/System_TrackingDataTypes.xml').getroot();table=root.find('.//d:informaltable',N);documented={}
    normal={'Degrees':'deg','Hertz':'Hz','Kilometers':'km','Kilometers/sec':'km/s','Range Units':'RU'}
    for row in table.findall('.//d:tbody/d:row',N):
        cells=[' '.join(''.join(c.itertext()).split())for c in row.findall('d:entry',N)]
        for name in cells[0].split(','):documented[name.strip()]=normal[cells[-1]]
    need(documented=={t:meanings()[t]for t in types[:-2]},'Documented normal measurement units differ')
    for name,angle in [('RightAscAdapter.cpp','rightAscension'),('DeclinationAdapter.cpp','declination')]:
        adapter=(stage/'definitions/plugins/EstimationPlugin/src/base/adapter'/name).read_text()
        need('return '+angle+' * GmatMathConstants::DEG_PER_RAD;'in adapter,'Testing-only adapter angular units differ')
    picker=(stage/'definitions/src/qtgui/TrackingConfigDialog.cpp').read_text();need('if (GmatGlobal::Instance()->InTestingMode()) names.append({"RightAscension","Declination"});'in picker,'Testing-only choice gating changed')
    return types
def observations(proof,mode,types):
    selected=STATIC if mode=='Static'else TESTING if mode=='Testing'else types[:-2]
    need(proof['mode']==mode and proof['selected_types']==selected,'Measurement type selection differs')
    expected=[(t,f,s,meanings()[t])for t in selected for f in FIELDS for s in ['metadata','primary']]
    need(len(proof['observations'])==len(expected),'Error unit observation count differs')
    for row,(typ,field,surface,unit)in zip(proof['observations'],expected):
        need((row['type'],row['field'],row['surface'],row['expected'],row['actual'])==(typ,field,surface,unit,unit)and row['passed'],'Error unit or dimensional scale differs')
    need(proof['status']=='passed'and proof['mismatches']==0 and proof['source_native_history_file_unchanged']and proof['values_not_converted']and proof['mission_executions']==0 and proof['new_requirement_credit']==0,'Type inspection altered original state or values')
def audit(stage):
    stage=Path(stage);types=definitions(stage);build=load(stage/'build-before-v1.json');ids=load(stage/'source-sha256-before-v1.json');raw={}
    need(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==ids and build['log_sha256']==sha(stage/'build-before-v1.log'),'Error unit build/source mismatch')
    with tarfile.open(stage/'sources-before-v1.tar.gz')as archive:
        need(set(archive.getnames())==set(ids),'Error unit source archive incomplete')
        for name,h in ids.items():
            data=archive.extractfile(name).read();need(hashlib.sha256(data).hexdigest()==h,'Error unit source changed')
            if name in ['src/qtgui/tests/ErrorModelUnitCases.json','src/qtgui/ResourceEditor.cpp']:raw[name]=data
    prior=load(stage.parent/'complete18-20261112-polynomial-array-units-v1/source-sha256-after-v1.json')
    need({n for n,h in prior.items()if ids.get(n)!=h}=={'src/qtgui/CMakeLists.txt'},'Existing product or test sources changed during unit verification')
    fixture=json.loads(raw['src/qtgui/tests/ErrorModelUnitCases.json'])
    need(fixture==dict(units=meanings(),static_types=STATIC,testing_only=TESTING,fields=FIELDS),'Error unit frozen expectations differ')
    need(hashlib.sha256(raw['src/qtgui/tests/ErrorModelUnitCases.json']).hexdigest()==load(stage/'frozen-plan-v1.json')['fixture_sha256'],'Error unit fixture changed')
    editor=raw['src/qtgui/ResourceEditor.cpp'];need(b'QtResourcePreview copy(initial->Clone()); copy->SetStringParameter("Type",selected.toStdString());'in editor and b'QStringList{"NoiseSigma","Bias","BiasSigma","PassBiases"}'in editor,'Missing detached unit refresh implementation')
    count=0
    for mode in ['headless','visible']:
        key='gui-'+mode+'-before-v1-all';root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json')
        need(receipt['inputs_unchanged']and receipt['returncode']==0 and all(receipt['source_sha256'].get(n)==h for n,h in ids.items()),'Error unit runtime/source differs')
        for n,h in build['binary_sha256'].items():
            if Path(n).name=='GmatQt':continue
            need(any(v==h and Path(p).parent==Path(n).parent and(p==n or Path(p).name.startswith(Path(n).name+'.'))for p,v in receipt['image_sha256'].items()),'Error unit runtime binary differs')
        results=[r for r in batch['results']if r['status']!='not-run'];need({r['name']for r in results}=={'QtGui.NativeErrorModelUnits'+m for m in ['Static','Switch','Testing']}and batch['end_reason']=='completed-selection'and batch['mode']==mode,'Error unit selection differs')
        for label in ['Static','Switch','Testing']:
            run=root/('QtGui.NativeErrorModelUnits'+label);result=load(run/'result.json');proof=load(run/'proof.json')
            need(result in results and result['status']=='passed'and result['returncode']==0 and result['requested_visibility']==mode,'Error unit case did not pass')
            if mode=='visible':need(result['trace']['native_exposure_events']>0,'No native error unit window exposure')
            rows=[json.loads(x.split('NASA_CHECK ',1)[1])for x in(run/'test.log').read_text().splitlines()if'NASA_CHECK 'in x]
            need(rows==[proof]and proof['observations']==load(run/'unit-observations.json'),'Error unit raw evidence differs');observations(proof,label,types);count+=len(proof['observations'])
    need(count==312,'Error unit total observations differ')
    return dict(status='passed',passing_GUI_observations=312,normal_measurement_types=19,testing_only_measurement_types=2,input_fields=3,completed_saved_unit_records=15,production_changes=0,mission_executions=0,new_requirement_credit=0,whole_row_qualified=False,new_native_tests_run_by_replay=0)
