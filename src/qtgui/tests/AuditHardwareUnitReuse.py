"""Join saved hardware GUI unit assertions to independent unit meanings."""
from pathlib import Path
import gzip,hashlib,json,re,xml.etree.ElementTree as ET

load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
N={'d':'http://docbook.org/ns/docbook'}

def need(value,message):
    if not value:raise ValueError(message)

def canonical(unit):
    unit=unit.strip().replace('²','^2').replace('³','^3')
    if unit=='None' or unit.casefold().startswith('dimensionless'):return 'dimensionless'
    unit={'seconds':'s','Seconds':'s'}.get(unit,unit)
    unit=re.sub(r'kPa(?:\^)?([CK]\d+)',r'kPa^\1',unit)
    return re.sub(r'(km|kPa|m|s)([23])(?=$|[/ *])',r'\1^\2',unit)

def expected_coefficients(letter,output):
    # Every additive term has the output unit. Pressure is in kPa; an
    # exponent and the base of a dimensionless exponential have no unit.
    pattern={1:output,2:output+'/kPa',3:output,4:output+'/kPa',5:output+'/kPa^2',
      6:output+'/kPa^'+letter+'7',7:'dimensionless',8:output+'/kPa^'+letter+'9',9:'dimensionless',
      10:output+'/kPa^'+letter+'11',11:'dimensionless',12:output,13:'dimensionless',
      14:'1/kPa',15:'dimensionless',16:'1/kPa'}
    return {letter+str(i):value for i,value in pattern.items()}

def expectations():
    physical={'BurnPrimary':{'Element1':'km/s','Element2':'km/s','Element3':'km/s','Isp':'s','GravitationalAccel':'m/s^2'},
      'ChemicalPrimary':{'GravitationalAccel':'m/s^2'},
      'ElectricPrimary':{'GravitationalAccel':'m/s^2','MinimumUsablePower':'kW','MaximumUsablePower':'kW','Isp':'s','ConstantThrust':'N'},
      'ChemicalCoefficients':expected_coefficients('C','N')|expected_coefficients('K','s')}
    direction={f:'dimensionless'for f in ['DutyCycle','ThrustScaleFactor','ThrustDirection1','ThrustDirection2','ThrustDirection3']}
    return {('physical',k):v for k,v in physical.items()}|{
      ('dimensionless','ChemicalPrimary'):direction,
      ('dimensionless','ElectricPrimary'):direction|{'FixedEfficiency':'dimensionless'}}

def audit(repo,index):
    repo=Path(repo);definitions=Path(index['definition_root']);expected=expectations()
    need(len(index['records'])==12,'Missing hardware profile/case')
    observed=set();count=0;pair_profiles={}
    for record in index['records']:
        run=Path(record['run']);result=load(run/'result.json');proof=load(run/'proof.json');case=record['case'];profile=record['profile'];identity=(record['domain'],case)
        need((identity,profile)not in observed,'Duplicated hardware run');observed.add((identity,profile))
        need(result['status']=='passed' and result['returncode']==0 and proof['outcome']=='passed' and proof['case']==case,'Original hardware run did not pass')
        need(result['requested_visibility']==('headless'if profile=='unattended'else'visible'),'Hardware profile differs')
        if profile=='visible':need(result['trace']['native_exposure_events']>0,'Hardware visibility unobserved')
        lines=[json.loads(x.split('NASA_CHECK ',1)[1])for x in (run/'test.log').read_text().splitlines()if 'NASA_CHECK 'in x]
        need(lines==[proof] and proof['observations']==load(run/'unit-observations.json'),'Hardware raw assertion record differs')
        sources=load(record['source_index']);source=repo/sources['src/qtgui/tests/NasaHardwareUnitTests.cpp']['archive']
        need(sha(source)==record['source_sha256']==sources['src/qtgui/tests/NasaHardwareUnitTests.cpp']['sha256'],'Hardware executed source identity differs')
        fixture_name='src/qtgui/tests/'+record['fixture_filename'];fixture_path=repo/sources[fixture_name]['archive'];fixture=load(fixture_path)
        spec=next(v for v in fixture['cases']if v['key']==case)
        need({k:canonical(v)for k,v in spec['units'].items()}==expected[identity],'Frozen fixture disagrees with independent unit dimensions')
        need(sha(fixture_path)==sources[fixture_name]['sha256'],'Hardware frozen fixture changed')
        counts={}
        for row in proof['observations']:
            field=row['field'];need(field in expected[identity] and canonical(row['expected_unit'])==expected[identity][field],'Hardware field meaning differs')
            if row['route']=='primary':need('('+row['expected_unit']+')'in row['label'],'Primary unit caption absent')
            else:
                need(row['route']in ['advanced-table','coefficient-dialog'] and row['displayed_unit']==row['expected_unit'],'Coefficient unit absent')
                if row['route']=='advanced-table':need(row['metadata_unit']==row['expected_unit'],'Coefficient metadata differs')
            counts[field]=counts.get(field,0)+1;count+=1
            pair_profiles.setdefault((spec['type'],field),set()).add(profile)
        multiplicity=2 if case=='ChemicalCoefficients'else 3 if case in ['BurnPrimary','ElectricPrimary']else 1
        need(counts=={k:multiplicity for k in expected[identity]},'Missing conditional/secondary hardware unit surface')
        need(len(proof['observations'])==record['unit_checks'] and proof['numeric_sentinel']==42 and float((run/'values.csv').read_text().strip())==42,'Hardware sentinel/count differs')
        isolation=load(run/'isolation-checks.json')
        need(isolation and all(all(v[k]for k in ['source_equal','history_equal','file_equal','pointer_equal','native_equal'])for v in isolation),'Hardware viewing changed source/state')
    need(observed=={(c,p)for c in expected for p in ['unattended','visible']} and count==236,'Hardware case universe differs')
    need(len(pair_profiles)==54 and all(p=={'unattended','visible'}for p in pair_profiles.values()),'Missing paired field evidence')
    docfields={}
    for file in ['Resource_Thruster.xml','Resource_ElectricThruster.xml','Resource_ImpulsiveBurn.xml']:
        for row in ET.parse(definitions/'doc/help/src'/file).getroot().findall('.//d:row',N):
            entries=row.findall('d:entry',N)
            if not entries:continue
            labels=[''.join(x.itertext()).strip()for x in entries[0].findall('.//d:guilabel',N)]
            md={' '.join(''.join(x.find('d:term',N).itertext()).split()):' '.join(''.join(x.find('d:listitem',N).itertext()).split())for x in row.findall('.//d:varlistentry',N)}
            for label in labels:
                if 'Units'in md:docfields.setdefault((file,label),[]).append(canonical(md['Units']))
    for (_,case),fields in expected.items():
        file='Resource_ImpulsiveBurn.xml'if case=='BurnPrimary'else'Resource_ElectricThruster.xml'if case=='ElectricPrimary'else'Resource_Thruster.xml'
        for field,unit in fields.items():need(unit in docfields.get((file,field),[]),'Current guide lacks matching independent unit: '+field)
    alias=index['alias_census'];data=[]
    for entry in alias:
        raw=Path(entry['compressed']).read_bytes();need(hashlib.sha256(raw).hexdigest()==entry['compressed_sha256'],'Alias census archive changed')
        decoded=gzip.decompress(raw);need(hashlib.sha256(decoded).hexdigest()==entry['uncompressed_sha256'],'Alias census content changed');data.append(json.loads(decoded))
    need({v['requested']for v in data}=={'Thruster','ChemicalThruster'} and all(v['native_type']=='ChemicalThruster'for v in data),'Alias class identity differs')
    need(data[0]['default_GUI_fields']==data[1]['default_GUI_fields'],'Alias exposes different default field metadata')
    factory=(definitions/'src/base/factory/HardwareFactory.cpp').read_text()
    need('if ((ofType == "Thruster") || (ofType == "ChemicalThruster"))\n      return new ChemicalThruster(withName);'in factory,'Legacy alias constructor differs')
    return dict(status='passed',retained_GUI_runs=12,retained_GUI_observations=236,independent_field_pairs=54,
      legacy_alias_field_mappings=38,matched_saved_numeric_records=len(index['completed_saved_records']),
      source_versioned_only=True,new_native_tests_run=0,new_requirement_credit=0,whole_row_qualified=False)
