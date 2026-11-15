"""Check source-versioned electric and power display units without missions."""
from pathlib import Path
import hashlib,json,tarfile,xml.etree.ElementTree as ET
load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value,message):
    if not value:raise ValueError(message)
def coefficient_units():
    return {prefix+str(i):base+(''if i==1 else'/kW'if i==2 else'/kW^'+str(i-1))
      for prefix,base in [('ThrustCoeff','mN'),('MassFlowCoeff','mg/s')]for i in range(1,6)}
POWER={'AnnualDecayRate':'percent/year','BusCoeff1':'kW','BusCoeff2':'kW*AU','BusCoeff3':'kW*AU^2','InitialMaxPower':'kW','Margin':'percent'}

def definitions(stage):
    plan=load(stage/'frozen-plan-v1.json');ids=load(stage/'definitions-sha256-v1.json')
    for name,h in ids.items():
        need(h==plan['source_sha256'][name] and sha(stage/'definitions'/name)==h,'Independent unit definition changed')
    def rendered(node):
        return (node.text or '')+''.join(('^'if c.tag.rsplit('}',1)[-1]=='superscript'else'')+rendered(c)+(c.tail or '')for c in node)
    def units(typ):
        result={}
        root=ET.parse(stage/'definitions/doc/help/src'/('Resource_'+typ+'.xml')).getroot()
        for row in root.iter():
            if row.tag.rsplit('}',1)[-1]!='row':continue
            if not len(row):continue
            name=''.join(row[0].itertext()).strip()
            for entry in row.iter():
                if entry.tag.rsplit('}',1)[-1]!='varlistentry':continue
                children={c.tag.rsplit('}',1)[-1]:c for c in entry}
                term=children.get('term')
                if term is not None and ''.join(term.itertext()).strip()=='Units':result[name]=' '.join(rendered(children['listitem']).split())
        return result
    electric=units('ElectricThruster')
    for prefix,base in [('ThrustCoeff','mN'),('MassFlowCoeff','mg/s')]:
        for i in range(1,6):
            power=''if i==2 else'^'+str(i-1)
            expected=base if i==1 else ('mN/kW'+power if prefix=='ThrustCoeff'else'mg/(s kW'+power+')')
            need(electric[prefix+str(i)]==expected+' (ThrustMassPolynomial; input power in kW)','Coefficient guide definition differs')
    for typ in ['NuclearPowerSystem','SolarPowerSystem']:
        guide=units(typ)
        need(all(guide[f].lower()==unit.lower()for f,unit in POWER.items()),'Power guide definition differs')
    engine=(stage/'definitions/src/base/hardware/ElectricThruster.cpp').read_text()
    need('thrustCoeff[0]) / 1.0e3;'in engine and 'massFlowCoeff[0]) / 1.0e6;'in engine,'Polynomial output scales differ')
    source=(stage/'definitions/src/base/hardware/PowerSystem.cpp').read_text()
    need('busCoeff1 + (busCoeff2/sunDist)'in source and '(busCoeff3/(sunDist*sunDist))'in source and 'GmatPhysicalConstants::ASTRONOMICAL_UNIT'in source and 'annualDecayRate/100.00'in source,'Power dimensions differ')

def coefficients(proof,switched,bad):
    units=coefficient_units()
    sequence=[('advanced-table',f)for f in sorted(units)]+[('coefficient-dialog',p+str(i))for p in ['ThrustCoeff','MassFlowCoeff']for i in range(1,6)]
    need(len(proof['observations'])==20,'Missing coefficient unit surface')
    failures=0
    for row,(route,field) in zip(proof['observations'],sequence):
        need((row['route'],row['field'],row['expected_unit'])==(route,field,units[field]),'Wrong independently derived coefficient units')
        correct=row['actual_unit']==units[field];need(row['passed']==correct,'Coefficient unit outcome changed');failures+=not correct
    need(failures==bad==proof['mismatches'] and proof['status']==('failed'if bad else'passed'),'Original coefficient failure or pass changed')
    need(proof.get('model_switched',False)==switched and (not switched or proof['pending_switch_discarded']),'Missing pending model-switch witness')
    if switched:need(proof['model_availability_transitions']==5,'Model availability transitions incomplete')
    need(proof['native_thrust_units']==proof['native_mass_units']==['See Docs']*5,'Native metadata was modified')
    need(proof['native_source_history_file_unchanged'] and proof['mission_executions']==0 and proof['new_requirement_credit']==0,'Coefficient viewing changed state or assigned credit')

def power(proof):
    expected=[(t,f,u)for t in ['NuclearPowerSystem','SolarPowerSystem']for f,u in sorted(POWER.items())]
    need(len(proof['observations'])==12,'Power unit field missing')
    for row,(typ,field,unit)in zip(proof['observations'],expected):
        need((row['case'],row['field'],row['expected_unit'],row['actual_unit'])==(typ,field,unit,unit),'Power unit differs')
        shown=row['visible_unit_or_caption'].strip()
        need(shown in [unit,unit+' '+field]or'('+unit+')'in shown,'Power unit not displayed with its input')
        need(row['passed'],'Power observation failed')
    need(proof['status']=='passed'and proof['mismatches']==0 and proof['source_native_history_file_isolation']and proof['mission_executions']==0,'Power view violated state/isolation')

def audit(stage):
    stage=Path(stage);definitions(stage);versions={}
    for version in ['before-v1','before-v2','after-v1','after-v2']:
        build=load(stage/('build-'+version+'.json'));ids=load(stage/('source-sha256-'+version+'.json'));raw={}
        need(build['status']=='passed'and build['inputs_unchanged']and build['source_sha256']==ids and build['log_sha256']==sha(stage/('build-'+version+'.log')),'Electric unit build/source mismatch')
        with tarfile.open(stage/('sources-'+version+'.tar.gz'))as bundle:
            need(set(bundle.getnames())==set(ids),'Electric unit source archive incomplete')
            for name,h in ids.items():
                data=bundle.extractfile(name).read();need(hashlib.sha256(data).hexdigest()==h,'Electric unit source changed')
                if name in ['src/qtgui/ResourceProperties.cpp','src/qtgui/ResourceProperties.hpp','src/qtgui/ResourceEditor.cpp','src/qtgui/tests/ElectricPowerUnitCases.json']:raw[name]=data
        versions[version]=(build,ids,raw)
    before,after=versions['before-v2'],versions['after-v1']
    allowed={'src/qtgui/ResourceProperties.cpp','src/qtgui/ResourceProperties.hpp','src/qtgui/ResourceEditor.cpp','src/qtgui/tests/ElectricCoefficientUnitTests.cpp','src/qtgui/CMakeLists.txt'}
    need({n for n,h in before[1].items()if after[1][n]!=h}==allowed,'Electric unit change scope differs')
    need(all(after[0]['binary_sha256'][n]==h for n,h in before[0]['binary_sha256'].items()if Path(n).name.startswith('lib')),'Native engine libraries changed')
    plan=load(stage/'implementation-plan-v1.json')
    for name,h in plan['production_files'].items():need(before[1][name]==h,'Wrong electric unit production preimage')
    helper=b'''QString electricThrusterCoefficientUnit(const QString &name)
{
   static const QRegularExpression coefficient("^(ThrustCoeff|MassFlowCoeff)([1-5])$");
   const auto match=coefficient.match(name);
   if (!match.hasMatch()) return {};
   const int power=match.captured(2).toInt()-1;
   const QString base=match.captured(1)=="ThrustCoeff" ? "mN" : "mg/s";
   return base+(power==0 ? QString() : power==1 ? "/kW" : "/kW^"+QString::number(power));
}

'''
    fallback=b'         if (field.unit.isEmpty() && object.IsOfType("ElectricThruster"))\n            field.unit=electricThrusterCoefficientUnit(field.name);\n'
    props='src/qtgui/ResourceProperties.cpp';head='src/qtgui/ResourceProperties.hpp';editor='src/qtgui/ResourceEditor.cpp'
    need(after[2][props].count(helper)==1 and after[2][props].count(fallback)==1 and
         after[2][props].replace(helper,b'').replace(fallback,b'')==before[2][props],'Property repair changed unrelated source')
    declaration=b'// Display units for the native five-term electric-thruster polynomials.\nQString electricThrusterCoefficientUnit(const QString &name);\n'
    need(after[2][head].count(declaration)==1 and after[2][head].replace(declaration,b'')==before[2][head],'Header repair changed unrelated source')
    old=b'            names[family].append(prefix+QString::number(i+1)); units[family].append(QString::fromStdString(values[i]));\n'
    new=b'''            const auto name=prefix+QString::number(i+1);
            const auto displayUnit=electric ? electricThrusterCoefficientUnit(name) : QString();
            names[family].append(name);
            units[family].append(displayUnit.isEmpty() ? QString::fromStdString(values[i]) : displayUnit);
'''
    need(after[2][editor].count(new)==1 and after[2][editor].replace(new,old)==before[2][editor],'Dialog repair changed unrelated source')
    final=versions['after-v2']
    need({n for n,h in after[1].items()if final[1][n]!=h}=={editor,'src/qtgui/tests/ElectricCoefficientUnitTests.cpp'},'Model availability repair changed unrelated sources')
    removed=b'''   if (object.IsOfType("ElectricThruster")) {
      auto *coefficients=findChild<QPushButton *>("thrusterCoefficients");
      for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="ThrustModel") {
         if (auto *model=qobject_cast<QComboBox *>(table->cellWidget(row,1))) {
            auto update=[model,coefficients] { coefficients->setEnabled(model->currentText()=="ThrustMassPolynomial"); };
            connect(model,&QComboBox::currentTextChanged,this,update); update();
         }
      }
   }
'''
    added=b'''   if (object.IsOfType("ElectricThruster")) {
      auto *coefficients=findChild<QPushButton *>("thrusterCoefficients");
      // Setup owns this field when present; its pending model can differ from
      // the hidden Advanced snapshot until Apply.
      auto *model=primary->findChild<QComboBox *>("thruster_ThrustModel");
      if (!model) for (int row=0;row<table->rowCount();++row)
         if (table->item(row,0)->text()=="ThrustModel") model=qobject_cast<QComboBox *>(table->cellWidget(row,1));
      if (model && coefficients) {
         auto update=[model,coefficients] { coefficients->setEnabled(model->currentText()=="ThrustMassPolynomial"); };
         connect(model,&QComboBox::currentTextChanged,this,update); update();
      }
   }
'''
    need(final[2][editor].count(added)==1 and after[2][editor].count(removed)==1 and
         final[2][editor].replace(added,b'')==after[2][editor].replace(removed,b''),'Model availability patch scope differs')
    switch_plan=load(stage/'model-switch-repair-plan-v1.json')
    need(all(after[1][n]==h for n,h in switch_plan['preimage_sha256'].items()),'Model availability repair preimage differs')
    need(all(final[0]['binary_sha256'][n]==h for n,h in after[0]['binary_sha256'].items()if Path(n).name.startswith('lib')),'Model availability repair changed native libraries')
    fixture=json.loads(after[2]['src/qtgui/tests/ElectricPowerUnitCases.json'])
    need(fixture['cases'][0]['units']==coefficient_units()and all(c['units']==POWER for c in fixture['power_cases']),'Electric unit fixture differs from independent dimensions')
    runs=[('headless','before-v2','coefficients',{'QtGui.NativeElectricCoefficientUnits':20}),
          ('headless','before-v2','power',{'QtGui.NativePowerSystemUnits':0}),
          ('headless','after-v1','both-coefficients',{'QtGui.NativeElectricCoefficientUnits':0,'QtGui.NativeElectricCoefficientUnitsSwitch':None}),
          ('headless','after-v2','switched',{'QtGui.NativeElectricCoefficientUnitsSwitch':0}),
          ('visible','after-v2','all',{'QtGui.NativeElectricCoefficientUnits':0,'QtGui.NativeElectricCoefficientUnitsSwitch':0,'QtGui.NativePowerSystemUnits':0})]
    passed=failed=0
    for mode,version,selection,names in runs:
        key='gui-'+mode+'-'+version+'-'+selection;root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json')
        need(receipt['inputs_unchanged']and receipt['returncode']==(1 if any(v!=0 for v in names.values())else 0),'Electric unit execution result changed')
        need(all(receipt['source_sha256'].get(n)==h for n,h in versions[version][1].items()),'Executed unit source differs from build')
        for name,h in versions[version][0]['binary_sha256'].items():
            if Path(name).name=='GmatQt':continue
            matches=[p for p,v in receipt['image_sha256'].items()if v==h and Path(p).parent==Path(name).parent and(p==name or Path(p).name.startswith(Path(name).name+'.'))]
            need(len(matches)==1,'Electric unit runtime image differs')
        rows=[x for x in batch['results']if x['status']!='not-run'];need(batch['mode']==mode and batch['end_reason']=='completed-selection'and{r['name']for r in rows}==set(names),'Electric unit selection changed')
        for name,bad in names.items():
            run=root/name;result=load(run/'result.json');need(result in rows,'Electric unit result not in batch')
            if bad is None:
                need(result['status']=='failed'and result['returncode']==8 and 'FAIL Electric polynomial editor disabled'in(run/'test.log').read_text(),'Original pending-model failure lost')
                partial=load(run/'unit-observations.json');units=coefficient_units()
                need(len(partial)==10 and all(p['route']=='advanced-table'and p['actual_unit']==p['expected_unit']==units[p['field']]and p['passed']for p in partial),'Pending-model partial observations changed')
                continue
            proof=load(run/'proof.json')
            need(result['status']==('failed'if bad else'passed')and result['returncode']==(8 if bad else 0),'Original electric unit case status changed')
            need(result['requested_visibility']==mode and 'QT_QPA_PLATFORM=xcb'in result['registered_command'],'Wrong electric unit GUI route')
            if mode=='visible':need(result['trace']['native_exposure_events']>0,'No visible electric unit exposure')
            lines=[json.loads(x.split('NASA_CHECK ',1)[1])for x in(run/'test.log').read_text().splitlines()if'NASA_CHECK 'in x]
            need(lines==[proof] and proof['observations']==load(run/'unit-observations.json'),'Electric unit raw evidence differs')
            if name=='QtGui.NativePowerSystemUnits':power(proof)
            else:coefficients(proof,name.endswith('Switch'),bad)
            failed+=bad;passed+=len(proof['observations'])-bad
    early=stage/'gui-headless-before-v1-coefficients/QtGui.NativeElectricCoefficientUnits'
    need(load(early/'result.json')['status']=='failed'and'Electric unit editor missing'in(early/'test.log').read_text(),'Original selection failure lost')
    need(passed==104 and failed==20,'Electric/power observation count changed')
    return dict(status='passed',passing_GUI_observations=104,retained_unit_label_failures=20,retained_interaction_failure_cases=1,retained_model_availability_failure_cases=1,
      coefficient_fields=10,power_fields=12,reused_headless_power_observations=12,reused_headless_coefficient_observations=20,model_switch_profiles=2,
      native_metadata_unchanged=True,native_engine_changes=False,original_assertions_changed=0,
      new_requirement_credit=0,whole_row_qualified=False,mission_executions=0,native_tests_run_by_replay=0)
