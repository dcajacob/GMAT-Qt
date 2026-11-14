"""Verify polynomial input units from frozen sources and GUI receipts."""
from pathlib import Path
import hashlib,json,re,tarfile,xml.etree.ElementTree as ET
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value,message):
    if not value:raise ValueError(message)
FIELDS={'Transmitter':['FrequencyBias','FrequencyBiasSigma'],
        'ThrustSegment':['ThrustAngle1','ThrustAngle2','ThrustAngle1Sigma','ThrustAngle2Sigma']}
def unit(typ,order):
    base='MHz'if typ=='Transmitter'else'deg'
    return base+(''if order==0 else'/s'if order==1 else'/s^'+str(order))
def sequence():
    rows=[]
    for typ,fields in FIELDS.items():
        base=unit(typ,0);summary=base+', '+base+'/s, '+base+'/s^2, ...'
        for field in fields:
            rows.extend((typ,field,surface,3,-1,summary)for surface in ['metadata','primary'])
            for length in ([3,5,2,3]if typ=='ThrustSegment'else[3]):
                rows.extend((typ,field,'grid',length,k,'c'+str(k)+' ('+unit(typ,k)+')')for k in range(length))
    return rows
def observations(proof,bad):
    expected=sequence();need(len(expected)==len(proof['observations'])==70,'Polynomial observation count differs')
    failures=0
    for row,identity in zip(proof['observations'],expected):
        observed=tuple(row[k]for k in ['type','field','surface','length','order','expected'])
        need(observed==identity,'Independent polynomial dimensions or field order differ')
        correct=row['actual']==identity[-1];need(row['passed']==correct,'Polynomial result changed');failures+=not correct
    need(failures==bad==proof['mismatches']and proof['status']==('failed'if bad else'passed'),'Original polynomial failures or passes changed')
    need(proof['native_source_history_file_unchanged']and proof['mission_executions']==0 and proof['new_requirement_credit']==0,'Polynomial inspection changed native/source state')
def definitions(stage):
    for n,h in load(stage/'definition-source-sha256-v1.json').items():need(sha(stage/'definitions'/n)==h,'Polynomial dimension source changed')
    N={'d':'http://docbook.org/ns/docbook'}
    for typ,fields in FIELDS.items():
        found={}
        for row in ET.parse(stage/'definitions/doc/help/src'/('Resource_'+typ+'.xml')).getroot().findall('.//d:row',N):
            entry=row.find('d:entry',N)
            if entry is None:continue
            labels=[''.join(x.itertext()).strip()for x in entry.findall('.//d:guilabel',N)]
            for v in row.findall('.//d:varlistentry',N):
                if ''.join(v.find('d:term',N).itertext()).strip()=='Units':
                    value=' '.join(''.join(v.find('d:listitem',N).itertext()).split()).replace('sec','s').replace('^2 ,','^2,')
                    for name in labels:found[name]=value
        base=unit(typ,0);prefix=base+', '+base+'/s, '+base+'/s^2, ...'
        need(all(found.get(f,'').startswith(prefix)for f in fields),'Polynomial guide dimensions differ')
    transmitter=(stage/'definitions/plugins/EstimationPlugin/src/base/hardware/Transmitter.cpp').read_text()
    need('Real t = (atTime - initialEpoch).GetMjd() * 86400;'in transmitter and 'freqPolynomialCoeffs[k] * t_powk'in transmitter and 't_powk = t_powk * t;'in transmitter,'Radio polynomial time/output dimensions differ')
    thrust=(stage/'definitions/plugins/ThrustFilePlugin/src/base/forcemodel/FileThrust.cpp').read_text()
    need('Real offsetSec = offset * GmatTimeConstants::SECS_PER_DAY;'in thrust and 'angleVec1[ii] * GmatMathUtil::Pow(offsetSec, ii)'in thrust and 'angleVec2[ii] * GmatMathUtil::Pow(offsetSec, ii)'in thrust and 'angle1 *= GmatMathConstants::RAD_PER_DEG;'in thrust,'Thrust polynomial dimensions differ')
def audit(stage):
    stage=Path(stage);definitions(stage);versions={};paths=['src/qtgui/ResourceEditor.cpp','src/qtgui/ResourceProperties.cpp','src/qtgui/tests/PolynomialArrayUnitCases.json']
    for version in ['before-v1','after-v1']:
        build=load(stage/('build-'+version+'.json'));ids=load(stage/('source-sha256-'+version+'.json'));raw={}
        need(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==ids and build['log_sha256']==sha(stage/('build-'+version+'.log')),'Polynomial source/build mismatch')
        with tarfile.open(stage/('sources-'+version+'.tar.gz'))as archive:
            need(set(archive.getnames())==set(ids),'Polynomial build archive incomplete')
            for name,h in ids.items():
                data=archive.extractfile(name).read();need(hashlib.sha256(data).hexdigest()==h,'Polynomial source archive changed')
                if name in paths:raw[name]=data
        versions[version]=(ids,build,raw)
    before,after=versions['before-v1'],versions['after-v1'];editor,props,fixture=paths
    need(set(before[0])==set(after[0])and {n for n,h in before[0].items()if after[0][n]!=h}=={editor,props,'src/qtgui/CMakeLists.txt'},'Polynomial patch affected unrelated source')
    for n,h in load(stage/'implementation-plan-v1.json')['production_files'].items():need(before[0][n]==h,'Polynomial patch preimage differs')
    meta=b'''         if (field.unit.isEmpty() && object.IsOfType("Transmitter") &&
             QStringList{"FrequencyBias","FrequencyBiasSigma"}.contains(field.name))
            field.unit="MHz, MHz/s, MHz/s^2, ...";
         if (field.unit.isEmpty() && object.IsOfType("ThrustSegment") &&
             QStringList{"ThrustAngle1","ThrustAngle2","ThrustAngle1Sigma","ThrustAngle2Sigma"}.contains(field.name))
            field.unit="deg, deg/s, deg/s^2, ...";
'''
    need(after[2][props].count(meta)==1 and after[2][props].replace(meta,b'')==before[2][props],'Polynomial metadata repair scope differs')
    # The only editor additions label headers and subscribe to dimension changes.
    old=b'   connect(table,&QTableWidget::itemDoubleClicked,this,[this](QTableWidgetItem *item) {\n'
    new=b'   const auto coefficientOwnerType=QString::fromStdString(object.GetTypeName());\n   connect(table,&QTableWidget::itemDoubleClicked,this,[this,coefficientOwnerType](QTableWidgetItem *item) {\n'
    body=after[2][editor];a=body.index(b'      const auto field=table->item(item->row(),0)->text();\n');b=body.index(b'      const bool covariance=',a)
    need(body[:a]+body[b:]==before[2][editor].replace(old,new),'Polynomial editor changed existing validation/resize/value logic')
    need(b'&QAbstractItemModel::columnsInserted'in body[a:b]and b'&QAbstractItemModel::columnsRemoved'in body[a:b]and b'grid->setHorizontalHeaderLabels(labels);'in body[a:b],'Polynomial labels do not track resize')
    need(all(after[1]['binary_sha256'][n]==h for n,h in before[1]['binary_sha256'].items()if Path(n).name.startswith('lib')),'Polynomial labels changed native libraries')
    plan=load(stage/'frozen-plan-v1.json');need(hashlib.sha256(after[2][fixture]).hexdigest()==plan['fixture_sha256'],'Polynomial fixture changed')
    cases=json.loads(after[2][fixture])['cases'];need([(c['type'],c['fields'],c['base_unit'],c['allow_resize'])for c in cases]==[(t,f,unit(t,0),t=='ThrustSegment')for t,f in FIELDS.items()],'Polynomial case contract changed')
    for mode,version,bad in [('headless','before-v1',70),('headless','after-v1',0),('visible','after-v1',0)]:
        key='gui-'+mode+'-'+version+'-all';root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json');ids,build,_=versions[version]
        need(receipt['inputs_unchanged']and receipt['returncode']==(1 if bad else 0)and all(receipt['source_sha256'].get(n)==h for n,h in ids.items()),'Polynomial execution identity changed')
        for n,h in build['binary_sha256'].items():
            if Path(n).name=='GmatQt':continue
            need(any(v==h and Path(p).parent==Path(n).parent and(p==n or Path(p).name.startswith(Path(n).name+'.'))for p,v in receipt['image_sha256'].items()),'Polynomial runtime image differs')
        actual=[r for r in batch['results']if r['status']!='not-run'];need(len(actual)==1 and actual[0]['name']=='QtGui.NativePolynomialArrayUnits'and batch['mode']==mode and batch['end_reason']=='completed-selection','Polynomial selection changed')
        run=root/'QtGui.NativePolynomialArrayUnits';result=load(run/'result.json');proof=load(run/'proof.json')
        need(result==actual[0]and result['status']==('failed'if bad else'passed')and result['returncode']==(8 if bad else 0)and result['requested_visibility']==mode,'Original polynomial result changed')
        if mode=='visible':need(result['trace']['native_exposure_events']>0,'No visible polynomial GUI exposure')
        lines=[json.loads(x.split('NASA_CHECK ',1)[1])for x in(run/'test.log').read_text().splitlines()if'NASA_CHECK 'in x]
        need(lines==[proof]and proof['observations']==load(run/'unit-observations.json'),'Polynomial raw observations changed');observations(proof,bad)
    return dict(status='passed',completed_field_pairs=6,passing_GUI_observations=140,retained_unit_label_failures=70,thrust_vector_lengths=[3,5,2,3],radio_vector_length_preserved=3,mission_executions=0,native_engine_changes=False,new_requirement_credit=0,whole_row_qualified=False,new_native_tests_run_by_replay=0)
