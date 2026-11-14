"""Verify explicit count/index units without physical-unit assumptions."""
from pathlib import Path
import hashlib,json,tarfile,xml.etree.ElementTree as ET
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value,message):
    if not value:raise ValueError(message)
def expected():
    batch=dict(FreezeIteration='iteration number',ILSEMaximumIterations='iterations',MaxConsecutiveDivergences='iterations',MaximumIterations='iterations')
    plot=dict(DataCollectFrequency='samples',MaxPlotPoints='points',NumPointsToRedraw='segments',UpdatePlotFrequency='samples')
    return {'BatchEstimator':batch,'BatchEstimatorInv':batch,'DifferentialCorrector':{'MaximumIterations':'iterations'},
      'PropSetup':{'MaxStepAttempts':'attempts'},'EphemerisFile':{'InterpolationOrder':'order'},
      'VF13ad':{'MaximumIterations':'iterations','MaximumLineSearches':'searches'},'Yukon':{'MaximumFunctionEvals':'evaluations','MaximumIterations':'iterations'},
      'GroundTrack':plot,'GroundTrackPlot':plot,'OrbitView':plot|{'StarCount':'stars'},'ContactLocator':{'ReportPrecision':'digits'},
      'ReportFile':{'ColumnWidth':'characters','Precision':'digits'},'TextEphemFile':{'ColumnWidth':'characters','Precision':'digits'}}
def definitions(stage):
    ids=load(stage/'definition-source-sha256-v1.json')
    for n,h in ids.items():need(sha(stage/'definitions'/n)==h,'Count definition changed')
    N={'d':'http://docbook.org/ns/docbook'};docs={}
    for name in ids:
        if not name.endswith('.xml'):continue
        rows={}
        for row in ET.parse(stage/'definitions'/name).getroot().findall('.//d:row',N):
            first=row.find('d:entry',N)
            if first is None:continue
            fields=[''.join(x.itertext()).strip()for x in first.findall('.//d:guilabel',N)]
            for v in row.findall('.//d:varlistentry',N):
                if ''.join(v.find('d:term',N).itertext()).strip()=='Units':
                    value=' '.join(''.join(v.find('d:listitem',N).itertext()).split())
                    for field in fields:rows.setdefault(field,set()).add(value)
        docs[Path(name).stem.removeprefix('Resource_')]=rows
    aliases={'BatchEstimatorInv':'BatchEstimator','PropSetup':'Propagator','GroundTrackPlot':'GroundTrack','TextEphemFile':'ReportFile'}
    for typ,fields in expected().items():
        for field in fields:
            doc=docs['QtInputConventions']if field=='MaxPlotPoints'else docs[aliases.get(typ,typ)]
            values=doc.get(field,set())
            need(values and all('count'in v.lower()or v=='Characters'for v in values),'Reference does not define an integer count/order: '+typ+'.'+field)
    plot=(stage/'definitions/src/base/subscriber/OrbitPlot.cpp').read_text()
    need('(mNumData % mDataCollectFrequency)'in plot,'Collection count is not sample based')
    model=(stage/'definitions/src/qtgui/PlotModel.cpp').read_text();need('if (count++==redrawPoints) break;'in model and 'requested number of recent segments'in model,'Redraw segment count meaning changed')
    report=(stage/'definitions/src/base/subscriber/ReportFile.cpp').read_text();need('ToString(rval, precision, zeroFill)'in report and 'length() > columnWidth'in report,'Report count meaning changed')
def observations(proof,cases,bad):
    sequence=[(c,f,u)for c in cases for f,u in sorted(c['units'].items())];need(len(proof['observations'])==len(sequence)and proof['cases']==[c['key']for c in cases],'Count observation inventory differs')
    failures=0
    for row,(case,field,unit)in zip(proof['observations'],sequence):
        need(expected()[case['key']][field]==unit and row['case']==case['key']and row['field']==field and row['expected_unit']==unit,'Count unit meaning differs')
        token=case['visible_units'][field];correct=row['actual_unit']==unit and token.lower()in row['visible_unit_or_caption'].lower()
        need(row['passed']==correct,'Count observation result differs');failures+=not correct
    need(failures==bad==proof['mismatches']and proof['status']==('failed'if bad else'passed'),'Original count failures/passes changed')
    need(proof['mission_executions']==0 and proof['source_native_history_file_isolation']and proof['new_requirement_credit']==0,'Count viewing changed source/state')
def audit(stage):
    stage=Path(stage);definitions(stage);versions={}
    for version in ['before-v1','before-v2','after-v1']:
        build=load(stage/('build-'+version+'.json'));ids=load(stage/('source-sha256-'+version+'.json'));raw={}
        need(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==ids and build['log_sha256']==sha(stage/('build-'+version+'.log')),'Count source/build mismatch')
        with tarfile.open(stage/('sources-'+version+'.tar.gz'))as archive:
            need(set(archive.getnames())==set(ids),'Count archive incomplete')
            for n,h in ids.items():
                data=archive.extractfile(n).read();need(hashlib.sha256(data).hexdigest()==h,'Count source archive changed')
                if n in ['src/qtgui/ResourceProperties.cpp','src/qtgui/tests/CountInputUnitCases.json','src/qtgui/tests/DimensionlessInputUnitTests.cpp']:raw[n]=data
        versions[version]=(ids,build,raw)
    original,before,after=[versions[n]for n in ['before-v1','before-v2','after-v1']];props='src/qtgui/ResourceProperties.cpp';fixture='src/qtgui/tests/CountInputUnitCases.json'
    need({n for n,h in original[0].items()if before[0][n]!=h}=={fixture,'src/qtgui/tests/DimensionlessInputUnitTests.cpp'},'Fixture correction changed unrelated sources')
    need({n for n,h in before[0].items()if after[0][n]!=h}=={props},'Count metadata change affected unrelated sources')
    call=b'         if(field.unit.isEmpty())field.unit=countResourceUnit(object,field.name);\n'
    a=after[2][props].index(b'static QString countResourceUnit(');b=after[2][props].index(b'QString electricThrusterCoefficientUnit(',a);helper=after[2][props][a:b]
    need(after[2][props].count(call)==1 and (after[2][props][:a]+after[2][props][b:]).replace(call,b'')==before[2][props],'Count repair changed existing property behavior')
    need(b'Set'not in helper and b'object.IsOfType('in helper and all(('"'+u+'"').encode()in helper for fields in expected().values()for u in fields.values()),'Count helper mutates state or omits reviewed meanings')
    need(before[0][props]==load(stage/'implementation-plan-v1.json')['production_files'][props],'Wrong count repair preimage')
    need(all(after[1]['binary_sha256'][n]==h for n,h in before[1]['binary_sha256'].items()if Path(n).name.startswith('lib')),'Count metadata changed native libraries')
    fixtures={v:json.loads(data[2][fixture])for v,data in versions.items()}
    need(hashlib.sha256(before[2][fixture]).hexdigest()==load(stage/'fixture-correction-v1.json')['fixture_sha256'],'Corrected count fixture changed')
    need([(c['key'],c['units'],c['visible_units'])for c in fixtures['before-v1']['cases']]==[(c['key'],c['units'],c['visible_units'])for c in fixtures['after-v1']['cases']],'Count expectations changed during fixture repair')
    runs=[('headless','before-v1','all',{'Presentation':18},True),('headless','before-v2','limits',{'Limits':15},False),('headless','after-v1','all',{'Limits':0,'Presentation':0},False),('visible','after-v1','all',{'Limits':0,'Presentation':0},False)]
    passed=failed=0
    for mode,version,selection,groups,incomplete in runs:
        key='gui-'+mode+'-'+version+'-'+selection;root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json');ids,build,_=versions[version]
        need(receipt['inputs_unchanged']and receipt['returncode']==(1 if any(groups.values())else 0)and all(receipt['source_sha256'].get(n)==h for n,h in ids.items()),'Count execution source/result differs')
        for n,h in build['binary_sha256'].items():
            if Path(n).name=='GmatQt':continue
            need(any(v==h and Path(p).parent==Path(n).parent and(p==n or Path(p).name.startswith(Path(n).name+'.'))for p,v in receipt['image_sha256'].items()),'Count execution binary differs')
        selected={r['name']for r in batch['results']if r['status']!='not-run'}
        need(selected=={'QtGui.NativeCountInputUnits'+g for g in (['Limits','Presentation']if incomplete else groups)}and batch['mode']==mode and batch['end_reason']=='completed-selection','Count test selection changed')
        for group,bad in groups.items():
            run=root/('QtGui.NativeCountInputUnits'+group);result=load(run/'result.json');proof=load(run/'proof.json')
            need(result['status']==('failed'if bad else'passed')and result['returncode']==(8 if bad else 0)and result['requested_visibility']==mode,'Count case result changed')
            if mode=='visible':need(result['trace']['native_exposure_events']>0,'No visible count controls')
            raw=[json.loads(x.split('NASA_CHECK ',1)[1])for x in(run/'test.log').read_text().splitlines()if'NASA_CHECK 'in x];need(raw==[proof]and proof['observations']==load(run/'unit-observations.json'),'Count raw observations changed')
            cases=[c for c in fixtures[version]['cases']if c['group']=='Count'+group];observations(proof,cases,bad);passed+=len(proof['observations'])-bad;failed+=bad
    early=stage/'gui-headless-before-v1-all/QtGui.NativeCountInputUnitsLimits';need(load(early/'result.json')['status']=='failed'and 'The Spacecraft was not set'in(early/'EphemerisFile-build.txt').read_text(),'Initial fixture failure lost')
    need(passed==66 and failed==33,'Count observation totals differ')
    return dict(status='passed',completed_saved_count_records=33,passing_GUI_observations=66,retained_unit_metadata_failures=33,retained_fixture_failure_cases=1,identifier_or_tolerance_credit=0,mission_executions=0,native_engine_changes=False,new_requirement_credit=0,whole_row_qualified=False,new_native_tests_run_by_replay=0)
