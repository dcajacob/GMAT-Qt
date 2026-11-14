"""Replay hardware-origin and tank display-unit checks without native runs."""
from pathlib import Path
import hashlib,json,tarfile

load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value,message):
    if not value:raise ValueError(message)
ORIGINS={'HWOriginInBCS'+a:'m'for a in 'XYZ'}
TANK={'FuelMass':'kg',**{'FuelCenterOfMass'+a:'m'for a in 'XYZ'},
      **{'FuelMomentOfInertia'+a:'kg-m^2'for a in ['XX','XY','XZ','YY','YZ','ZZ']}}
CHEMICAL={'FuelDensity':'kg/m^3','Pressure':'kPa','RefTemperature':'C','Temperature':'C','Volume':'m^3'}
TYPES={'Antenna','FuelTank','ChemicalTank','ElectricTank','Imager','Receiver','Transmitter','Transponder','ChemicalThruster','ElectricThruster','Thruster'}

def check_observations(proof,cases,bad):
    expected=[(c['key'],field,unit)for c in cases for field,unit in sorted(c['units'].items())]
    need(proof['cases']==[c['key']for c in cases] and len(proof['observations'])==len(expected),'Hardware unit case/field census differs')
    failures=0
    for row,(case,field,unit) in zip(proof['observations'],expected):
        need((row['case'],row['field'],row['expected_unit'],row['phase'])==(case,field,unit,'initial'),'Hardware unit observation identity differs')
        shown=row['visible_unit_or_caption'].strip()
        presentation=(shown==unit if row['route']=='advanced-table' else
                      shown==unit+' '+field or '('+unit+')'in shown)
        correct=row['actual_unit']==unit and presentation
        need(row['passed']==correct,'Hardware unit assertion disagrees with its displayed caption')
        need(row['route']in ['advanced-table','primary+backing-metadata'],'Unexpected hardware unit route')
        failures+=not correct
    need(failures==bad==proof['mismatches'] and proof['status']==('failed'if bad else'passed'),'Historical hardware label result was relabelled')
    need(proof['mission_executions']==0 and proof['source_native_history_file_isolation'] and proof['new_requirement_credit']==0,'Hardware unit viewing changed state or claimed coverage')

def audit(stage):
    stage=Path(stage);plan=load(stage/'frozen-plan-v1.json');repair=load(stage/'implementation-plan-v1.json');versions={}
    definitions=load(stage/'independent-definition-sources-v1.json')
    need(set(definitions)=={n for n in plan['source_sha256']if n.startswith('doc/')},'Independent unit definitions missing')
    for name,row in definitions.items():
        need(sha(row['path'])==row['sha256']==plan['source_sha256'][name],'Independent unit definition changed')
    for version in ['before-v1','after-v1']:
        build=load(stage/('build-'+version+'.json'));ids=load(stage/('source-sha256-'+version+'.json'))
        need(build['status']=='passed' and build['returncode']==0 and build['inputs_unchanged'] and build['source_sha256']==ids,'Hardware unit build/source guard failed')
        need(build['log_sha256']==sha(stage/('build-'+version+'.log')),'Hardware unit build log changed')
        with tarfile.open(stage/('sources-'+version+'.tar.gz'))as bundle:
            need(set(bundle.getnames())==set(ids),'Hardware unit source archive incomplete')
            for name,h in ids.items():need(hashlib.sha256(bundle.extractfile(name).read()).hexdigest()==h,'Hardware unit source bytes changed')
            fixture=json.loads(bundle.extractfile('src/qtgui/tests/HardwareOriginUnitCases.json').read())
            source=bundle.extractfile('src/qtgui/ResourceProperties.cpp').read()
        need(ids['src/qtgui/tests/HardwareOriginUnitCases.json']==plan['fixture_sha256'],'Hardware unit fixture changed')
        versions[version]=(build,ids,fixture,source)
    before,after=[versions[v]for v in ['before-v1','after-v1']]
    need({n for n,h in before[1].items()if after[1][n]!=h}=={'src/qtgui/ResourceProperties.cpp'},'Hardware unit repair widened')
    need(before[1]['src/qtgui/ResourceProperties.cpp']==repair['preimage_sha256'],'Hardware unit preimage differs')
    addition=b'         // Native hardware metadata omits the documented metre offsets.\n         if (field.unit.isEmpty() && object.IsOfType(Gmat::HARDWARE) &&\n             QStringList{"HWOriginInBCSX","HWOriginInBCSY","HWOriginInBCSZ"}.contains(field.name))\n            field.unit="m";\n'
    need(after[3].count(addition)==1 and after[3].replace(addition,b'')==before[3],'Repair changed more than the three missing-unit fallbacks')
    need(all(after[0]['binary_sha256'][p]==h for p,h in before[0]['binary_sha256'].items()if Path(p).name.startswith('lib')),'Native libraries changed during GUI unit repair')
    fixture=after[2];cases=fixture['cases'];origins=[c for c in cases if c['group']=='HardwareOrigins'];tanks=[c for c in cases if c['group']=='TankPhysical'];pilot=[c for c in cases if c['group']=='HardwareOriginPilot']
    need(len(origins)==11 and {c['key'].removesuffix('Origins')for c in origins}==TYPES and all(c['units']==ORIGINS for c in origins),'Hardware origin type/field universe changed')
    need(len(tanks)==3 and {c['key'].removesuffix('Physical')for c in tanks}=={'FuelTank','ChemicalTank','ElectricTank'},'Tank type universe changed')
    for c in tanks:need(c['units']==TANK|(CHEMICAL if c['key']!='ElectricTankPhysical'else{}),'Tank dimensional expectations differ')
    need(len(pilot)==1 and pilot[0]['units']==ORIGINS and pilot[0]['fixture']=='Create Antenna Probe;\n','Hardware origin pilot changed')
    groups=[('headless','before-v1','pilot',{'HardwareOriginPilot':3}),('headless','before-v1','tanks',{'TankPhysical':0}),
            ('headless','after-v1','origins',{'HardwareOrigins':0}),('visible','after-v1','all',{'HardwareOrigins':0,'TankPhysical':0})]
    count=failed=0
    for profile,version,selection,selected in groups:
        key='gui-'+profile+'-'+version+'-'+selection;root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json')
        need(receipt['returncode']==(1 if sum(selected.values())else 0) and receipt['inputs_unchanged'],'Hardware unit execution guard differs')
        need(all(receipt['source_sha256'].get(n)==h for n,h in versions[version][1].items()),'Hardware unit execution source differs')
        for name,h in versions[version][0]['binary_sha256'].items():
            if Path(name).name=='GmatQt':continue
            matches=[p for p,d in receipt['image_sha256'].items()if d==h and Path(p).parent==Path(name).parent and (p==name or Path(p).name.startswith(Path(name).name+'.'))]
            need(len(matches)==1,'Hardware unit execution image differs')
        rows=[x for x in batch['results']if x['status']!='not-run'];need(batch['mode']==profile and batch['end_reason']=='completed-selection'and len(rows)==len(selected),'Hardware unit selection/profile differs')
        for group,bad in selected.items():
            run=root/('QtGui.NativeHardwareUnits'+group);result=load(run/'result.json');proof=load(run/'proof.json');check_observations(proof,[c for c in cases if c['group']==group],bad)
            need(result in rows and result['status']==('failed'if bad else'passed')and result['returncode']==(8 if bad else 0),'Original hardware unit process result changed')
            need(result['requested_visibility']==profile and 'QT_QPA_PLATFORM=xcb'in result['registered_command'],'Hardware unit native route differs')
            if profile=='visible':need(result['trace']['native_exposure_events']>0,'Hardware unit window never exposed')
            emitted=[json.loads(x.split('NASA_CHECK ',1)[1])for x in (run/'test.log').read_text().splitlines()if'NASA_CHECK 'in x]
            need(emitted==[proof] and proof['observations']==load(run/'unit-observations.json'),'Hardware raw observation differs')
            count+=len(proof['observations'])-bad;failed+=bad
    need(count==146 and failed==3,'Hardware unit observation count changed')
    return dict(status='passed',passing_GUI_observations=count,retained_failed_GUI_observations=failed,hardware_origin_types=11,origin_fields=33,tank_fields=40,reused_headless_tank_observations=40,production_files_modified=1,native_value_changes=False,original_assertions_changed=0,new_requirement_credit=0,whole_row_qualified=False,mission_executions=0,native_tests_run_by_replay=0,full_regression=False)
