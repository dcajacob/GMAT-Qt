"""Replay bounded time/angle/station/hardware unit evidence without tests."""
from pathlib import Path
import hashlib,json,re,tarfile,xml.etree.ElementTree as ET
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value,message):
    if not value:raise ValueError(message)
UNITS={
 'ContactLocator':{'StepSize':'s','IntervalStepSize':'s'},'EclipseLocator':{'StepSize':'s'},'IntrusionLocator':{'StepSize':'s'},
 'EstimatedParameter':{'HalfLife':'s'},'ExtendedKalmanFilter':{'DelayRectifyTimeSpan':'s','PredictTimeSpan':'s'},
 'Smoother':{'DelayRectifyTimeSpan':'s','PredictTimeSpan':'s'},'ProcessNoiseModel':{'UpdateTimeStep':'s','AccelNoiseSigma':'km/s^(3/2)'},
 'Simulator':{'MeasurementTimeStep':'s'},'Transponder':{'HardwareDelay':'s'},
 'TrackingFileSet':{'TimeGapForPassBreak':'s','SimDopplerCountInterval':'sec','MinHeightOfRayPath':'km','MaxCentralAngleOfRayPath':'deg','SimRangeModuloConstant':'RU'},
 'ConicalFOV':{'FieldOfViewAngle':'deg'},'RectangularFOV':{'AngleHeight':'deg','AngleWidth':'deg'},
 'GroundStation':{'Humidity':'percent','Pressure':'hPa','Temperature':'K','MinimumElevationAngle':'deg','Location1':'conditional','Location2':'conditional','Location3':'km'},
 'Transmitter':{'Frequency':'MHz'}}
def kind(case):return re.search(r'Create (\w+) Probe;',case['fixture']).group(1)
def canonical(unit):
    value=unit.strip()
    return {'Seconds':'s','seconds':'s','sec':'s','Degrees':'deg','degrees':'deg','percentage':'percent','Kelvin':'K','Kilometers':'km',
            'Range Units (RU)':'RU','km/s^3/2 for State Noise Compensation (SNC)':'km/s^(3/2)'}.get(value,value)
def expected(case):
    typ=kind(case);value=UNITS[typ]
    if case['key']=='StationAmbient':value={f:value[f]for f in ['Humidity','Pressure','Temperature']}
    elif case['key']=='StationElevation':value={'MinimumElevationAngle':'deg'}
    elif typ=='GroundStation':
        match=re.fullmatch(r'Station(Cartesian|Spherical)(Sphere|Ellipsoid)',case['key']);need(match,'Unknown station configuration')
        state,horizon=match.groups();need('Probe.StateType = '+state+';'in case['fixture']and'Probe.HorizonReference = '+horizon+';'in case['fixture'],'Station interpretation differs')
        value={'Location'+str(i+1):'km'if state=='Cartesian'or i==2 else'deg'for i in range(3)}
    return value
def definitions(stage):
    ids=load(stage/'definitions-before-sha256-v1.json')
    for name,h in ids.items():need(sha(stage/'definitions-before'/name)==h,'Physical unit definition changed')
    def render(n):return (n.text or '')+''.join(('^'if c.tag.rsplit('}',1)[-1]=='superscript'else'')+render(c)+(c.tail or '')for c in n)
    N={'d':'http://docbook.org/ns/docbook'};documents={}
    for name in ids:
        if not name.endswith('.xml'):continue
        rows={}
        for row in ET.parse(stage/'definitions-before'/name).getroot().findall('.//d:row',N):
            cell=row.find('d:entry',N)
            if cell is None:continue
            labels=[''.join(x.itertext()).strip()for x in cell.findall('.//d:guilabel',N)]
            for v in row.findall('.//d:varlistentry',N):
                term=v.find('d:term',N)
                if term is not None and ''.join(term.itertext()).strip()=='Units':
                    value=canonical(' '.join(render(v.find('d:listitem',N)).split()))
                    for label in labels:rows.setdefault(label,set()).add(value)
        documents[Path(name).stem.removeprefix('Resource_')]=rows
    for typ,fields in UNITS.items():
        doc=documents['FieldOfView'if typ in ['ConicalFOV','RectangularFOV']else typ]
        for field,unit in fields.items():
            if typ=='GroundStation'and field.startswith('Location'):
                alternatives=doc[field];need(any('km (Cartesian'in x and('km (Spherical'if field=='Location3'else'degrees (Spherical')in x for x in alternatives),'Station conditional units incomplete')
            else:need(canonical(unit)in doc.get(field,set()),'Documented unit differs: '+typ+'.'+field)
    rect=(stage/'definitions-before/src/base/hardware/RectangularFOV.cpp').read_text()
    need('angleWidth = value * GmatMathConstants::RAD_PER_DEG'in rect and 'angleHeight = value * GmatMathConstants::RAD_PER_DEG'in rect,'FOV input angular convention changed')
    process=(stage/'definitions-before/plugins/ExtendedKalmanFilterPlugin/src/base/noise/ProcessNoiseModel.cpp').read_text()
    need('noiseModel = new SNCProcessNoise'in process,'Process-noise default model changed')
def observations(proof,cases,bad):
    sequence=[(c,f,u)for c in cases for f,u in sorted(c['units'].items())]
    need(proof['cases']==[c['key']for c in cases]and len(proof['observations'])==len(sequence),'Physical unit observation inventory differs')
    failures=0;passed=set()
    for row,(case,field,unit)in zip(proof['observations'],sequence):
        need(expected(case)[field]==unit and row['field']==field and row['case']==case['key']and row['expected_unit']==unit,'Physical unit meaning differs')
        if case['route']=='primary':
            need(row['route']=='primary','Wrong primary observation route');correct=bool(re.search(r'(?<!\w)'+re.escape(unit)+r'(?!\w)',row['actual_label']))
        else:
            need(row['route']in ['advanced-table','primary+backing-metadata'],'Wrong physical unit route')
            correct=row['actual_unit']==unit and unit in row['visible_unit_or_caption']
        need(correct==row['passed'],'Physical unit outcome changed');failures+=not correct
        if correct:passed.add((case['key'],field))
    need(failures==bad==proof['mismatches']and proof['status']==('failed'if bad else'passed'),'Original physical unit pass or failure changed')
    need(proof['source_native_history_file_isolation']and proof['mission_executions']==0 and proof['new_requirement_credit']==0,'Physical unit observation altered mission/state')
    return passed
def audit(stage):
    stage=Path(stage);definitions(stage);versions={};wanted=['src/qtgui/ResourceProperties.cpp','src/qtgui/tests/TimingGeometryUnitCases.json','src/qtgui/tests/TimingRepairUnitCases.json']
    for version in ['before-v1','after-v1']:
        ids=load(stage/('source-sha256-'+version+'.json'));build=load(stage/('build-'+version+'.json'));raw={}
        need(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==ids and build['log_sha256']==sha(stage/('build-'+version+'.log')),'Physical unit build differs')
        with tarfile.open(stage/('sources-'+version+'.tar.gz'))as bundle:
            need(set(bundle.getnames())==set(ids),'Physical unit source archive incomplete')
            for name,h in ids.items():
                data=bundle.extractfile(name).read();need(hashlib.sha256(data).hexdigest()==h,'Physical unit archived source differs')
                if name in wanted:raw[name]=data
        versions[version]=(ids,build,raw)
    before,after=versions['before-v1'],versions['after-v1'];props=wanted[0]
    need(set(after[0])-set(before[0])=={wanted[2]}and set(before[0])-set(after[0])==set(),'Unexpected physical unit source additions')
    need({n for n,h in before[0].items()if after[0][n]!=h}=={props,'src/qtgui/CMakeLists.txt'},'Physical unit source change scope differs')
    plan=load(stage/'implementation-plan-v1.json');need(plan['production_files'][props]==before[0][props],'Wrong physical unit preimage')
    start=b'         // Physical inputs whose native classes omit display-unit metadata.\n';end=b'         if (field.unit.isEmpty()) field.unit=physicalUnits.value(QString::fromStdString(object.GetTypeName())+"."+field.name);\n'
    output=after[2][props];a=output.index(start);b=output.index(end,a)+len(end);delta=output[a:b]
    need(output[:a]+output[b:]==before[2][props],'Physical unit patch changed unrelated behavior')
    table=dict((t+b'.'+f,u)for t,f,u in re.findall(rb'\{"(\w+)\.(\w+)","([^"]+)"\}',delta))
    baseline=load(stage/'gui-headless-before-v1-all/QtGui.NativeTimingGeometryUnitsTimingHardware/proof.json')
    fixture=json.loads(after[2][wanted[1]]);repair=json.loads(after[2][wanted[2]])
    need(hashlib.sha256(after[2][wanted[1]]).hexdigest()==load(stage/'frozen-plan-v1.json')['fixture_sha256']and hashlib.sha256(after[2][wanted[2]]).hexdigest()==plan['repair_fixture_sha256'],'Physical unit fixture expectations changed')
    expected_delta={}
    for group in ['TimingHardware','FovGeometry','StationAmbient']:
        proof=load(stage/('gui-headless-before-v1-all/QtGui.NativeTimingGeometryUnits'+group+'/proof.json'))
        for row in proof['observations']:
            if not row['passed']:
                case=next(c for c in fixture['cases']if c['key']==row['case']);expected_delta[(kind(case)+'.'+row['field']).encode()]=row['expected_unit'].encode()
    need(table==expected_delta and len(table)==19,'Physical unit fallback keys differ from demonstrated gaps')
    expected_repair={(c['key'],f):u for c in repair['cases']for f,u in c['units'].items()}
    failed_timing={(r['case'],r['field']):r['expected_unit']for r in baseline['observations']if not r['passed']}
    need(expected_repair==failed_timing and len(expected_repair)==13,'Repair subset altered original expectations')
    need(all(after[1]['binary_sha256'][n]==h for n,h in before[1]['binary_sha256'].items()if Path(n).name.startswith('lib')),'Physical labels changed native engines')
    runs=[('headless','before-v1','all',{'TimingHardware':13,'FovGeometry':3,'StationAmbient':3,'StationGeometry':0,'RadioFrequency':0}),('headless','after-v1','repair',{'TimingHardwareRepair':0,'FovGeometry':0,'StationAmbient':0}),('visible','after-v1','all',{'TimingHardware':0,'FovGeometry':0,'StationAmbient':0,'StationGeometry':0,'RadioFrequency':0})]
    totals={'headless':set(),'visible':set()};passed=failures=0
    for mode,version,selection,groups in runs:
        key='gui-'+mode+'-'+version+'-'+selection;root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json');ids,build,_=versions[version]
        need(receipt['inputs_unchanged']and receipt['returncode']==(1 if any(groups.values())else 0),'Physical unit execution outcome changed')
        need(all(receipt['source_sha256'].get(n)==h for n,h in ids.items()),'Physical unit runtime sources differ')
        for n,h in build['binary_sha256'].items():
            if Path(n).name=='GmatQt':continue
            need(any(v==h and Path(p).parent==Path(n).parent and(p==n or Path(p).name.startswith(Path(n).name+'.'))for p,v in receipt['image_sha256'].items()),'Physical unit runtime images differ')
        actual=[x for x in batch['results']if x['status']!='not-run'];need(batch['mode']==mode and batch['end_reason']=='completed-selection'and{x['name']for x in actual}=={'QtGui.NativeTimingGeometryUnits'+g for g in groups},'Unexpected physical unit test selection')
        for group,bad in groups.items():
            path=root/('QtGui.NativeTimingGeometryUnits'+group);result=load(path/'result.json');proof=load(path/'proof.json')
            need(result in actual and result['status']==('failed'if bad else'passed')and result['returncode']==(8 if bad else 0)and result['requested_visibility']==mode,'Physical unit case result changed')
            if mode=='visible':need(result['trace']['native_exposure_events']>0,'Physical unit native visibility not observed')
            raw=[json.loads(x.split('NASA_CHECK ',1)[1])for x in(path/'test.log').read_text().splitlines()if'NASA_CHECK 'in x];need(raw==[proof]and proof['observations']==load(path/'unit-observations.json'),'Physical unit raw evidence differs')
            cases=[c for c in(repair if group=='TimingHardwareRepair'else fixture)['cases']if c['group']==group]
            totals[mode]|=observations(proof,cases,bad);passed+=len(proof['observations'])-bad;failures+=bad
    all_cases={(c['key'],f)for c in fixture['cases']for f in c['units']};need(len(all_cases)==38 and all(v==all_cases for v in totals.values()),'Missing conditional physical unit/profile evidence')
    need(passed==76 and failures==19,'Physical observation count differs')
    return dict(status='passed',completed_field_pairs=29,conditional_case_field_pairs=38,passing_GUI_observations=76,retained_unit_inconsistencies=19,existing_primary_seconds_labels_reconciled=4,reused_headless_observations=19,mission_executions=0,native_engine_changes=False,new_requirement_credit=0,whole_row_qualified=False,new_native_tests_run_by_replay=0)
