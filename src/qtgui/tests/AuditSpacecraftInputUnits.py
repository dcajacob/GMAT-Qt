"""Replay source-versioned spacecraft unit evidence without running missions."""
from pathlib import Path
import hashlib,json,re,tarfile,xml.etree.ElementTree as ET

load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value,message):
    if not value:raise ValueError(message)
def canonical(unit):
    unit=unit.strip().replace('²','^2').replace('³','^3').replace('sec','s')
    return {'deg.':'deg','Deg.':'deg'}.get(unit,unit)
def expectations():
    result={n:'deg'for n in ['AOP','AZI','DEC','DECV','FPA','INC','MLONG','RA','RAAN','RAV','TA','TLONG']}
    result.update({n:'km'for n in ['RMagnitude','RadApo','RadPer','SMA','SemilatusRectum','X','Y','Z']})
    result.update({n:'km/s'for n in ['VMagnitude','VX','VY','VZ']})
    for p in ['BrouwerShort','BrouwerLong']:
        result.update({p+n:'km'if n=='SMA'else'deg'for n in ['SMA','INC','RAAN','AOP','MA']})
    result.update({'Delaunay'+n:'deg'if n.islower()else'km^2/s'for n in ['l','g','h','L','G','H']})
    for p in ['Incoming','Outgoing']:
        result.update({p+n:'km'if n=='RadPer'else'km^2/s^2'if n=='C3Energy'else'deg'for n in ['RadPer','C3Energy','RHA','DHA','BVAZI']})
    result.update(DryMass='kg',DragArea='m^2',SRPArea='m^2')
    for a in 'XYZ':result['ModelRotation'+a]='deg';result['ModelOffset'+a]='model units'
    need(len(result)==59,'Independent spacecraft field inventory differs')
    return result
def definitions(stage):
    plan=load(stage/'frozen-plan-v1.json');ids=load(stage/'definition-source-sha256-v1.json')
    extra=load(stage/'additional-definition-source-v1.json');expected_sources=dict(plan['meaning_sources']);expected_sources[extra['source']]=extra['sha256']
    need(ids==expected_sources,'Independent reference inventory changed')
    for name,h in ids.items():need(sha(stage/'definitions'/name)==h,'Independent spacecraft definition changed')
    def render(n):return (n.text or '')+''.join(('^'if c.tag.rsplit('}',1)[-1]=='superscript'else'')+render(c)+(c.tail or '')for c in n)
    N={'d':'http://docbook.org/ns/docbook'};fields={}
    for name in ['SpacecraftOrbitState','SpacecraftBallisticMass','SpacecraftVisualizationProperties','SpacecraftProperties']:
        root=ET.parse(stage/'definitions/doc/help/src'/('Resource_'+name+'.xml')).getroot()
        for row in root.findall('.//d:row',N):
            cell=row.find('d:entry',N)
            if cell is None:continue
            labels=[''.join(v.itertext()).strip()for v in cell.findall('.//d:guilabel',N)]
            for v in row.findall('.//d:varlistentry',N):
                term=v.find('d:term',N)
                if term is None or ''.join(term.itertext()).strip()!='Units':continue
                unit=' '.join(render(v.find('d:listitem',N)).split())
                unit=canonical(unit)
                if unit.startswith('Local model-coordinate units, before ModelScale and rotation'):unit='model units'
                for label in labels:fields.setdefault(label,set()).add(unit)
    for field,unit in expectations().items():need(unit in fields.get(field,set()),'Guide unit differs for '+field)
    renderer=(stage/'definitions/src/qtgui/OrbitRenderer.cpp').read_text()
    need('osg::Matrixd::translate(offset[0],offset[1],offset[2])*osg::Matrixd::scale(scale,scale,scale)*rotation'in renderer,'Local model transform order differs')

def retained(stage):
    expected=expectations();index=load(stage/'retained-evidence-index-v1.json');profiles={'headless':set(),'visible':set()};count=0
    need(len(index['records'])==5,'Retained spacecraft run count differs')
    for record in index['records']:
        for file,h in record['evidence_sha256'].items():need(sha(file)==h,'Retained spacecraft artifact changed')
        manifest=load(record['source_manifest']);member=record['source_manifest_member']
        if member:manifest=manifest[member]
        need(manifest[record['source']]==record['source_sha256']==sha(stage/'definitions'/record['source']),'Executed spacecraft assertion source differs')
        root=Path(record['run']);result=load(root/'result.json');proof=load(root/'proof.json');profile=record['profile']
        need(result['status']=='passed'and result['returncode']==0 and result['requested_visibility']==profile,'Retained spacecraft run did not pass')
        if profile=='visible':need(result['trace']['native_exposure_events']>0,'Retained spacecraft visibility unobserved')
        raw=[json.loads(line.split('NASA_CHECK ',1)[1])for line in(root/'test.log').read_text().splitlines()if'NASA_CHECK 'in line]
        need(raw==[proof]and proof['outcome']=='passed','Retained raw spacecraft result differs')
        if record['kind']=='unit-input':
            need(proof['input_count']==15 and proof['phase_count']==4,'Retained numeric input facets incomplete')
            rows=[r for r in proof['inputs']if r['phase']in ['Cartesian','Keplerian']]
            need(len(rows)==12 and all(r['visible_label']for r in rows),'Retained primary labels incomplete')
            for row in rows:
                if row['field']in expected:
                    need(canonical(row['unit'])==expected[row['field']],'Retained primary unit meaning differs');profiles[profile].add(row['field'])
            count+=15
        else:
            need(proof['pairs']==load(root/'pairs.json'),'Retained orbit pairs changed')
            observed=0
            for pair in proof['pairs']:
                for phase in ['pending_cancel','pending_apply','committed','reopened']:
                    need(len(pair[phase])==6,'Retained orbit unit phase incomplete')
                    for row in pair[phase]:
                        observed+=1
                        if row['name']in expected:
                            need(canonical(row['unit'])==expected[row['name']],'Retained orbit unit meaning differs');profiles[profile].add(row['name'])
            need(observed==proof['unit_checks']==proof['displayed_fields'],'Retained orbit count differs');count+=observed
    need(len(profiles['visible'])==22 and len(profiles['headless'])==30,'Retained spacecraft field/profile coverage differs')
    return profiles,count

def observations(proof,cases,bad):
    expected=expectations();sequence=[(case,k,unit)for case in cases for k,unit in sorted(case['units'].items())]
    need(proof['cases']==[c['key']for c in cases]and len(proof['observations'])==len(sequence),'Spacecraft field observation inventory differs')
    failures=0
    for row,(case,field,unit)in zip(proof['observations'],sequence):
        need(row['case']==case['key']and row['route']=='primary'and row['field']==field and row['expected_unit']==unit,'Spacecraft observation identity differs')
        need(canonical(unit)==expected[field],'Spacecraft expected unit is not independent')
        # Word boundaries prevent a bare g or m from matching kg or km.
        correct=bool(re.search(r'(?<![\w])'+re.escape(unit)+r'(?![\w])',row['actual_label']))
        need(row['passed']==correct,'Spacecraft observed label outcome changed');failures+=not correct
    need(failures==bad==proof['mismatches']and proof['status']==('failed'if bad else'passed'),'Original spacecraft label failures or pass changed')
    need(proof['source_native_history_file_isolation']and proof['mission_executions']==0 and proof['new_requirement_credit']==0,'Unit viewing changed state or ran a mission')

def audit(stage):
    stage=Path(stage);definitions(stage);profiles,reused=retained(stage);versions={}
    wanted=['src/qtgui/ResourceProperties.cpp','src/qtgui/VisualModelDialog.cpp','src/qtgui/tests/SpacecraftPhysicalUnitCases.json']
    for version in ['before-v1','after-v1']:
        ids=load(stage/('source-sha256-'+version+'.json'));build=load(stage/('build-'+version+'.json'));raw={}
        need(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==ids and sha(stage/('build-'+version+'.log'))==build['log_sha256'],'Spacecraft unit build mismatch')
        with tarfile.open(stage/('sources-'+version+'.tar.gz'))as bundle:
            need(set(bundle.getnames())==set(ids),'Spacecraft source archive incomplete')
            for name,h in ids.items():
                data=bundle.extractfile(name).read();need(hashlib.sha256(data).hexdigest()==h,'Spacecraft frozen build source changed')
                if name in wanted:raw[name]=data
        versions[version]=(ids,build,raw)
    before,after=versions['before-v1'],versions['after-v1'];props,visual=wanted[:2]
    need({n for n,h in before[0].items()if after[0][n]!=h}=={props,visual},'Spacecraft label patch touched unrelated sources')
    for name,h in load(stage/'implementation-plan-v1.json')['production_files'].items():need(before[0][name]==h,'Wrong spacecraft label preimage')
    addition=b'''         if (field.unit.isEmpty() && object.IsOfType("Spacecraft") &&
             QStringList{"ModelOffsetX","ModelOffsetY","ModelOffsetZ"}.contains(field.name))
            field.unit="model units";
'''
    caption=b'''         else if(group=="Offset") {
            auto *unit=new QLabel("model units",row);
            unit->setToolTip("Local model coordinates, before scale and rotation.");line->addWidget(unit);
         }
'''
    for file,delta in [(props,addition),(visual,caption)]:need(after[2][file].count(delta)==1 and after[2][file].replace(delta,b'')==before[2][file],'Spacecraft label patch is broader than reviewed')
    need(all(after[1]['binary_sha256'][name]==h for name,h in before[1]['binary_sha256'].items()if Path(name).name.startswith('lib')),'Spacecraft labels changed native engines')
    fixture=json.loads(after[2][wanted[2]]);need(hashlib.sha256(after[2][wanted[2]]).hexdigest()==load(stage/'frozen-plan-v1.json')['fixture_sha256'],'Spacecraft fixture expectations changed')
    new_fields={f for case in fixture['cases']for f in case['units']};need(len(new_fields)==37,'New spacecraft field count differs')
    runs=[('headless','before-v1','initial',{'OrbitMissing':0,'MassRotation':0,'LocalOffsets':3}),('headless','after-v1','offsets',{'LocalOffsets':0}),('visible','after-v1','visible',{'OrbitMissing':0,'OrbitVisible':0,'MassRotation':0,'LocalOffsets':0})]
    passed=failed=0
    for mode,version,selection,groups in runs:
        key='gui-'+mode+'-'+version+'-'+selection;root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json');build=versions[version][1]
        need(receipt['returncode']==(1 if any(groups.values())else 0)and receipt['inputs_unchanged'],'Spacecraft GUI batch status changed')
        need(all(receipt['source_sha256'].get(n)==h for n,h in versions[version][0].items()),'Spacecraft GUI ran different sources')
        for n,h in build['binary_sha256'].items():
            if Path(n).name=='GmatQt':continue
            need(any(v==h and Path(p).parent==Path(n).parent and(p==n or Path(p).name.startswith(Path(n).name+'.'))for p,v in receipt['image_sha256'].items()),'Spacecraft GUI ran different images')
        actual=[r for r in batch['results']if r['status']!='not-run'];names={'QtGui.NativeSpacecraftUnits'+g for g in groups}
        need(batch['end_reason']=='completed-selection'and batch['mode']==mode and {r['name']for r in actual}==names,'Unexpected spacecraft cases executed')
        for group,bad in groups.items():
            run=root/('QtGui.NativeSpacecraftUnits'+group);result=load(run/'result.json');proof=load(run/'proof.json')
            need(result in actual and result['status']==('failed'if bad else'passed')and result['returncode']==(8 if bad else 0),'Spacecraft case status changed')
            need(result['requested_visibility']==mode and 'QT_QPA_PLATFORM=xcb'in result['registered_command'],'Wrong spacecraft GUI profile')
            if mode=='visible':need(result['trace']['native_exposure_events']>0,'No visible spacecraft unit exposure')
            raw=[json.loads(line.split('NASA_CHECK ',1)[1])for line in(run/'test.log').read_text().splitlines()if'NASA_CHECK 'in line]
            need(raw==[proof]and proof['observations']==load(run/'unit-observations.json'),'Spacecraft raw observations differ')
            cases=[c for c in fixture['cases']if c['group']=='Spacecraft'+group];observations(proof,cases,bad)
            passed+=len(proof['observations'])-bad;failed+=bad
            if not bad:profiles[mode].update(f for c in cases for f in c['units'])
    need(passed==66 and failed==3,'Spacecraft unit observation counts differ')
    need(all(v==set(expectations())for v in profiles.values()),'Some spacecraft unit lacks paired GUI evidence')
    return dict(status='passed',completed_spacecraft_field_pairs=59,new_passing_GUI_observations=66,retained_missing_label_failures=3,reused_GUI_runs=5,reused_GUI_unit_observations=reused,reused_pre_repair_headless_observations=26,profiles=['headless','visible'],native_engine_changes=False,mission_executions=0,new_native_tests_run_by_replay=0,new_requirement_credit=0,whole_row_qualified=False)
