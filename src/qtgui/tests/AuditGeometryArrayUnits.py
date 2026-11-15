"""Replay angle-array unit display evidence without launching native tests."""
from pathlib import Path
import hashlib,json,tarfile,xml.etree.ElementTree as ET
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value,message):
    if not value:raise ValueError(message)
FIELDS={'CustomFOV':['ConeAngles','ClockAngles','InterpolationStepSize','Azimuth','Elevation'],
        'PlanetographicRegion':['Latitude','Longitude']}
def sequence():
    return [('CustomFOV','ConeClock','ConeAngles','Cone angle (deg)',3),('CustomFOV','ConeClock','ClockAngles','Clock angle (deg)',3),
            ('CustomFOV','Both','InterpolationStepSize','Interpolation step (deg)',1),('CustomFOV','AzimuthElevation','Azimuth','Azimuth (deg)',3),
            ('CustomFOV','AzimuthElevation','Elevation','Elevation (deg)',3),('PlanetographicRegion','Vertices','Latitude','Latitude (deg)',4),
            ('PlanetographicRegion','Vertices','Longitude','Longitude (deg)',4)]
def observations(proof):
    need(len(proof['observations'])==7,'Missing angle-array field observation')
    for row,(typ,mode,field,label,count)in zip(proof['observations'],sequence()):
        need((row['type'],row['mode'],row['field'],row['unit'],row['expected_header'],row['actual_header'],row['rows'])==(typ,mode,field,'deg',label,label,count)and row['passed'],'Angle-array unit/shape differs')
    need(proof['absolute_bound']==1e-12 and proof['status']=='passed'and proof['mismatches']==0 and proof['source_native_history_file_unchanged']and proof['mission_executions']==0 and proof['new_requirement_credit']==0,'Geometry unit inspection changed its bounds or state')
def definitions(stage):
    ids=load(stage/'definition-source-sha256-v1.json')
    for name,h in ids.items():need(sha(stage/'definitions'/name)==h,'Angle-array definition source changed')
    N={'d':'http://docbook.org/ns/docbook'}
    for kind,names in FIELDS.items():
        file='Resource_FieldOfView.xml'if kind=='CustomFOV'else'Resource_PlanetographicRegion.xml';doc={}
        for row in ET.parse(stage/'definitions/doc/help/src'/file).getroot().findall('.//d:row',N):
            cell=row.find('d:entry',N)
            if cell is None:continue
            labels=[''.join(x.itertext()).strip()for x in cell.findall('.//d:guilabel',N)]
            for v in row.findall('.//d:varlistentry',N):
                if ''.join(v.find('d:term',N).itertext()).strip()=='Units':
                    unit=' '.join(''.join(v.find('d:listitem',N).itertext()).split())
                    for label in labels:doc[label]=unit
        need(all(doc[n]in ['Degrees','Angles are in degrees.']for n in names),'Angle-array guide units differ')
    fov=(stage/'definitions/src/base/hardware/CustomFOV.cpp').read_text();need('interpolationSize = value * GmatMathConstants::RAD_PER_DEG;'in fov and 'clockAngleVec[index] = value * GmatMathConstants::RAD_PER_DEG;'in fov,'Native FOV input dimensions changed')
    region=next(stage/'definitions'/n for n in ids if n.endswith('/PlanetographicRegion.cpp'))
    code=region.read_text();need('latitudeList[i] * GmatMathConstants::RAD_PER_DEG'in code and 'longitudeList[i] * GmatMathConstants::RAD_PER_DEG'in code,'Native region input dimensions changed')
def audit(stage):
    stage=Path(stage);definitions(stage);build=load(stage/'build-before-v1.json');ids=load(stage/'source-sha256-before-v1.json');raw={}
    need(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==ids and build['log_sha256']==sha(stage/'build-before-v1.log'),'Angle-array build mismatch')
    with tarfile.open(stage/'sources-before-v1.tar.gz')as archive:
        need(set(archive.getnames())==set(ids),'Angle-array source archive incomplete')
        for name,h in ids.items():
            data=archive.extractfile(name).read();need(hashlib.sha256(data).hexdigest()==h,'Angle-array archived source changed')
            if name in ['src/qtgui/tests/GeometryArrayUnitTests.cpp','src/qtgui/tests/GeometryArrayUnitCases.json']:raw[name]=data
    prior=load(stage.parent/'complete18-20261112-error-model-units-v1/source-sha256-before-v1.json')
    need({n for n,h in prior.items()if ids.get(n)!=h}=={'src/qtgui/CMakeLists.txt'},'Existing product/test source changed during angle-unit verification')
    need(hashlib.sha256(raw['src/qtgui/tests/GeometryArrayUnitCases.json']).hexdigest()==load(stage/'frozen-plan-v1.json')['fixture_sha256'],'Geometry fixture changed')
    worker=raw['src/qtgui/tests/GeometryArrayUnitTests.cpp']
    need(b'{{10,0},{10,120},{10,240}}'in worker and b'{{0,80},{120,80},{240,80}}'in worker and b'{{-10,-10},{20,-10},{20,20},{-10,20}}'in worker and b'<=1e-12'in worker,'Degree-valued draft controls changed')
    for mode in ['headless','visible']:
        key='gui-'+mode+'-before-v1-all';root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json')
        need(receipt['inputs_unchanged']and receipt['returncode']==0 and all(receipt['source_sha256'].get(n)==h for n,h in ids.items()),'Angle-array runtime/source mismatch')
        for n,h in build['binary_sha256'].items():
            if Path(n).name=='GmatQt':continue
            need(any(v==h and Path(p).parent==Path(n).parent and(p==n or Path(p).name.startswith(Path(n).name+'.'))for p,v in receipt['image_sha256'].items()),'Angle-array runtime image mismatch')
        actual=[r for r in batch['results']if r['status']!='not-run'];need(len(actual)==1 and actual[0]['name']=='QtGui.NativeGeometryArrayUnits'and batch['mode']==mode and batch['end_reason']=='completed-selection','Unexpected angle-array execution')
        run=root/'QtGui.NativeGeometryArrayUnits';result=load(run/'result.json');proof=load(run/'proof.json')
        need(result==actual[0]and result['status']=='passed'and result['returncode']==0 and result['requested_visibility']==mode,'Angle-array case did not pass')
        if mode=='visible':need(result['trace']['native_exposure_events']>0,'Angle arrays had no native window exposure')
        checks=[json.loads(x.split('NASA_CHECK ',1)[1])for x in(run/'test.log').read_text().splitlines()if'NASA_CHECK 'in x]
        need(checks==[proof]and proof['observations']==load(run/'unit-observations.json'),'Angle-array raw observations differ');observations(proof)
    return dict(status='passed',completed_saved_unit_records=7,passing_GUI_unit_observations=14,production_changes=0,degree_draft_checks=True,mission_executions=0,new_requirement_credit=0,whole_row_qualified=False,new_native_tests_run_by_replay=0)
