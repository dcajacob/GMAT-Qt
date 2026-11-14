#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "ObjectManagedFunction.hpp"
#include "Parameter.hpp"
#include "Burn.hpp"
#include "Thruster.hpp"
#include "CoordinateSystem.hpp"
#include "FactoryManager.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include "UserDefinedFunction.hpp"
#include "Sandbox.hpp"
#include "Debugger.hpp"
#include <memory>
#include "SandboxException.hpp"
#include "ScriptInterpreter.hpp"
#include <QStatusBar>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
using namespace formaccept;

static QString pointer(const void *p) { return QString::number(quintptr(p),16); }
static void reveal(QWidget *widget)
{
   require(widget,"Binding resource control absent");
   for (auto *page=widget;page;page=page->parentWidget())
   {
      for (auto *tabs:widget->window()->findChildren<QTabWidget*>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
      if (auto *scroll=qobject_cast<QScrollArea*>(page)) scroll->ensureWidgetVisible(widget);
   }
   events(); require(widget->isVisible() && widget->isEnabled(),"Binding resource control unreachable"); acceptanceView(*widget);
}


static QString reference(const QJsonObject &spec,bool shared,bool function)
{
   return spec[shared?"shared_reference":function?"local_private_reference":"root_private_reference"].toString();
}
static double force(bool shared,bool inside,int calls){return (shared?300:inside?200:100)+(inside?calls*(shared?20:10):0);}
static QJsonValue expectedValue(const QJsonObject &spec,bool shared,bool inside,int calls,bool firing)
{
   if(!firing)return 0.;const double f=force(shared,inside,calls);const QString type=spec["factory_type"].toString();
   if(type=="TotalMassFlowRate")return -f/3000.;const double component=f*(spec["axis"].toInt()==0?1:2)/3.;return type.startsWith("TotalAcceleration")?component/(1000.*(inside?1000:2000)):component;
}
static double nativeValue(Burn *burn,const QString &type)
{
   if(type=="TotalMassFlowRate")return burn->GetTotalMassFlowRate();const int axis=type.right(1).toInt()-1;return type.startsWith("TotalAcceleration")?burn->GetTotalAcceleration()[axis]:burn->GetTotalThrust()[axis];
}
static double bound(const QString &,const QString &) {return 1e-12;}
static void compareValue(const QJsonValue &actual,const QJsonValue &expected,double tolerance)
{
   if(expected.isString()){require(actual==expected,"Binding string value differs from authored stamp");return;}
   if(expected.isArray())
   {
      require(actual.isArray()&&actual.toArray().size()==expected.toArray().size(),"Binding array shape differs");const auto a=actual.toArray(),e=expected.toArray();
      for(int i=0;i<a.size();++i)compareValue(a[i],e[i],tolerance);return;
   }
   require(actual.isDouble()&&std::isfinite(actual.toDouble())&&std::abs(actual.toDouble()-expected.toDouble())<=tolerance,"Binding numeric value differs from selected literal/formula");
}
class BindingObserver final:public Subscriber
{
public:
   BindingObserver(Rig &rig,const QJsonObject &spec,const QString &prefix):Subscriber("FiniteBurnBindingObserver","FiniteBurnBindingObserver"),rig(&rig),spec(spec),prefix(prefix){}
   ~BindingObserver() override {Publisher::Instance()->Unsubscribe(this);}
   GmatBase *Clone() const override{return new BindingObserver(*this);}
   bool ReceiveData(const Real *data,Integer count=0) override
   {
      bool initial=false;if(!Publisher::GetPublishedPropagationObjects(data,count,initial)||!initial)return true;
      try
      {
         auto *command=dynamic_cast<GmatCommand*>(currentProvider);require(command,"Finite binding provider is not a command");auto *function=dynamic_cast<ObjectManagedFunction*>(command->GetCurrentFunction());const bool inside=function!=nullptr;
         const auto *locals=inside?function->GetFunctionObjectStore():command->GetObjectMap();const auto rootGlobals=inside?ObjectMap{}:Moderator::Instance()->GetSandbox()->GetGlobalObjectMap();const auto *globals=inside?function->GetGlobalObjectStore():&rootGlobals;require(locals&&globals,"Live finite scope map absent");
         auto lookup=[&](const std::string &name){auto a=locals->find(name);if(a!=locals->end())return a->second;auto b=globals->find(name);return b==globals->end()?nullptr:b->second;};
         auto *gate=lookup("Ready");require(gate,"Finite observer gate absent");const int ready=int(gate->GetRealParameter("Value"));if(ready==0)return true;require(ready==1||ready==2,"Finite observer gate invalid");const bool firing=ready==1;
         auto *counter=lookup("Calls");require(counter,"Finite call counter absent");const int calls=int(counter->GetRealParameter("Value"));QJsonArray localKeys,globalKeys;for(const auto &v:*locals)localKeys.append(QString::fromStdString(v.first));for(const auto &v:*globals)globalKeys.append(QString::fromStdString(v.first));
         events.append(QJsonObject{{"inside_function",inside},{"function",inside?QString::fromStdString(function->GetName()):QString()},{"calls",calls},{"ready",ready},{"completed_samples",records.size()},{"local_keys",localKeys},{"global_keys",globalKeys},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))}});rig->save(prefix+"-callback-events.json",QJsonDocument(events).toJson());
         require(calls==(events.size()-1)/2&&ready==(events.size()%2?1:2)&&inside==(calls>0)&&(!inside||function->GetName()=="ReadShared"),"Finite callback order/function/lifecycle differs");
         auto *target=dynamic_cast<Spacecraft*>(lookup(inside?"ScopePulse":"MainPulse"));auto *mainTarget=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("MainPulse"));require(target&&mainTarget&&(inside?target!=mainTarget:target==mainTarget),"Actual finite target scope differs");
         const bool massFlow=spec["mass_flow"].toBool();const double elapsed=(target->GetEpoch()-21545)*86400,baseMass=inside?1000:2000;require(std::abs(elapsed-ready)<=3e-6,"Finite observation elapsed domain differs");
         const double mass=target->GetRealParameter("TotalMass"),expectedMass=baseMass-(massFlow?(force(false,inside,calls)+force(true,inside,calls))*elapsed/3000:0);require(std::abs(mass-expectedMass)<=1e-7,"Finite actual target mass budget differs");
         const QJsonObject targetState{{"name",inside?"ScopePulse":"MainPulse"},{"pointer",pointer(target)},{"main_target_pointer",pointer(mainTarget)},{"epoch",target->GetEpoch()},{"elapsed",elapsed},{"total_mass",mass},{"expected_total_mass",expectedMass},{"mass_bound",1e-7},{"dry_mass",target->GetRealParameter("DryMass")},{"ready",ready},{"firing",firing}};
         const QString type=spec["factory_type"].toString();const auto meta=spec["metadata"].toObject();QJsonArray parameters;
         for(bool shared:{false,true})
         {
            const QString ownerName=shared?"SharedBurn":"PrivateBurn",full=reference(spec,shared,inside);auto *owner=dynamic_cast<Burn*>(lookup(ownerName.toStdString()));auto *root=dynamic_cast<Burn*>(Moderator::Instance()->GetInternalObject(ownerName.toStdString()));auto *configured=Moderator::Instance()->GetConfiguredObject(ownerName.toStdString());
            require(owner&&root&&configured&&owner!=configured&&owner->IsOfType(meta["owner_type_id"].toInt())&&owner->GetTypeName()=="FiniteBurn","Live finite owner/class/prototype differs");require(owner->IsGlobal()==shared&&owner->IsLocal()==(inside&&!shared)&&(shared?owner==root:inside?owner!=root:owner==root),"Finite private/global owner identity or flags differ");
            auto *parameter=dynamic_cast<Parameter*>(lookup(full.toStdString()));require(parameter,"Live finite Parameter missing from scope map");require(parameter->IsSystemParameter()&&parameter->GetName()==full.toStdString()&&parameter->GetTypeName()==meta["native_type"].toString().toStdString()&&int(parameter->GetReturnType())==meta["return_type"].toInt()&&int(parameter->GetOwnerType())==meta["owner_type_id"].toInt(),"Actual finite Parameter metadata differs");auto *referent=parameter->GetRefObject(parameter->GetOwnerType(),ownerName.toStdString());require(referent==owner,"Finite Parameter reference does not bind live owner");
            const QString unit=QString::fromStdString(parameter->GetStringParameter(parameter->GetParameterID("Unit")));require(unit==spec["unit"].toString(),"Live finite Parameter unit metadata differs");const double actual=parameter->EvaluateReal(),direct=nativeValue(owner,type);const auto expected=expectedValue(spec,shared,inside,calls,firing);rig->save(prefix+"-numeric-observation.json",QJsonDocument(QJsonObject{{"reference",full},{"calls",calls},{"ready",ready},{"actual",actual},{"direct",direct},{"expected",expected},{"target",targetState}}).toJson());compareValue(actual,expected,1e-12);compareValue(direct,expected,1e-12);require(owner->HasFired()&&owner->IsFiring()==firing,"Actual finite burn fired/lifecycle state differs");
            const std::string engineName=shared?"SharedEngine":inside?"LocalEngine":"RootEngine",tankName=shared?"SharedFuel":inside?"LocalFuel":"RootFuel";auto *engine=target->GetRefObject(Gmat::THRUSTER,engineName);auto *nativeEngine=dynamic_cast<Thruster*>(engine);auto *burnEngine=owner->GetRefObject(Gmat::THRUSTER,engineName);auto *tank=target->GetRefObject(Gmat::FUEL_TANK,tankName);
            require(engine&&nativeEngine&&engine->GetTypeName()=="ChemicalThruster"&&burnEngine==engine&&tank&&tank->GetTypeName()=="ChemicalTank","Actual finite burn/target hardware graph differs");const auto tanks=nativeEngine->GetRefObjectArray(Gmat::HARDWARE);require(tanks.size()==1&&tanks[0]==tank,"Actual finite thruster-to-target tank identity differs");
            const double thrust=force(shared,inside,calls),tankMass=tank->GetRealParameter("FuelMass"),expectedTankMass=100-(massFlow?thrust*elapsed/3000:0);require(std::abs(tankMass-expectedTankMass)<=1e-7,"Actual finite tank mass budget differs");require(engine->GetRealParameter("C1")==thrust&&engine->GetRealParameter("K1")==300&&engine->GetRealParameter("GravitationalAccel")==10&&engine->GetRealParameter("DutyCycle")==1&&engine->GetRealParameter("ThrustScaleFactor")==1&&engine->GetBooleanParameter("DecrementMass")==massFlow&&engine->GetBooleanParameter("IsFiring")==firing&&engine->GetStringParameter("CoordinateSystem")=="EarthMJ2000Eq","Actual finite attached hardware fields/lifecycle differ");for(int i=2;i<=16;++i)require(engine->GetRealParameter("C"+std::to_string(i))==0&&engine->GetRealParameter("K"+std::to_string(i))==0,"Finite constant coefficient assumptions differ");for(int i=1;i<=3;++i)require(engine->GetRealParameter("ThrustDirection"+std::to_string(i))==double(i==1?1:2),"Finite authored direction differs");
            auto *internal=parameter->GetInternalCoordSystem();require(internal&&internal==Moderator::Instance()->GetInternalCoordinateSystem()&&internal->GetName()=="InternalEarthMJ2000Eq","Finite Parameter internal frame identity differs");
            QJsonArray actualThrust,actualAcceleration;for(int i=0;i<3;++i){actualThrust.append(owner->GetTotalThrust()[i]);actualAcceleration.append(owner->GetTotalAcceleration()[i]);const double want=firing?thrust*(i==0?1:2)/3:0;require(std::abs(actualThrust[i].toDouble()-want)<=1e-12,"Actual finite thrust vector differs");if(!massFlow)require(std::abs(actualAcceleration[i].toDouble()-want/(baseMass*1000))<=1e-12,"Actual finite acceleration vector differs");}
            const QJsonObject hardware{{"engine_name",QString::fromStdString(engineName)},{"engine_type",QString::fromStdString(engine->GetTypeName())},{"burn_engine_pointer",pointer(burnEngine)},{"target_engine_pointer",pointer(engine)},{"tank_name",QString::fromStdString(tankName)},{"tank_type",QString::fromStdString(tank->GetTypeName())},{"engine_tank_pointer",pointer(tanks[0])},{"target_tank_pointer",pointer(tank)},{"tank_mass",tankMass},{"expected_tank_mass",expectedTankMass},{"thrust_C1",thrust},{"isp_K1",300},{"gravity_accel",10},{"duty",1},{"scale",1},{"direction",QJsonArray{1,2,2}},{"decrement_mass",massFlow},{"engine_firing",firing},{"frame_name",QString::fromStdString(engine->GetStringParameter("CoordinateSystem"))},{"thruster_frame_pointer_directly_observed",false}};
            const QJsonObject burnState{{"has_fired",owner->HasFired()},{"is_firing",owner->IsFiring()},{"last_fire_epoch",owner->GetEpochAtLastFire()},{"total_mass_flow",owner->GetTotalMassFlowRate()},{"total_thrust",actualThrust},{"total_acceleration",actualAcceleration},{"internal_frame_pointer",pointer(internal)},{"moderator_internal_frame_pointer",pointer(Moderator::Instance()->GetInternalCoordinateSystem())},{"internal_frame_name",QString::fromStdString(internal->GetName())}};
            parameters.append(QJsonObject{{"reference",full},{"factory_type",type},{"native_type",QString::fromStdString(parameter->GetTypeName())},{"unit",unit},{"return_type",int(parameter->GetReturnType())},{"owner_type",int(parameter->GetOwnerType())},{"parameter_pointer",pointer(parameter)},{"reference_owner_pointer",pointer(referent)},{"live_owner_pointer",pointer(owner)},{"root_owner_pointer",pointer(root)},{"configured_owner_pointer",pointer(configured)},{"direct_owner_property_value",direct},{"base_bookkeeping_owner_pointer",pointer(parameter->GetOwner())},{"owner_name",ownerName},{"owner_type_name",QString::fromStdString(owner->GetTypeName())},{"owner_global",owner->IsGlobal()},{"owner_local",owner->IsLocal()},{"parameter_global",parameter->IsGlobal()},{"parameter_local",parameter->IsLocal()},{"shared",shared},{"shape",QJsonArray{}},{"actual",actual},{"expected",expected},{"absolute_bound",1e-12},{"hardware",hardware},{"burn_state",burnState}});
         }
         records.append(QJsonObject{{"inside_function",inside},{"function",inside?QString::fromStdString(function->GetName()):QString()},{"calls",calls},{"ready",ready},{"firing",firing},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))},{"locals_pointer",pointer(locals)},{"globals_pointer",pointer(globals)},{"global_map_is_value_snapshot",!inside},{"parameters",parameters},{"target_state",targetState}});rig->save(prefix+"-live-parameter-samples.json",QJsonDocument(records).toJson());
      }
      catch(BaseException &e){if(!failure){error=QString::fromStdString(e.GetFullMessage());failure=std::current_exception();}}
      catch(std::exception &e){if(!failure){error=e.what();failure=std::current_exception();}}
      return true;
   }
   QJsonArray records,events;std::exception_ptr failure;QString error;
protected:
   bool Distribute(Integer) override{return true;}
   bool Distribute(const Real *,Integer) override{return true;}
private:
   Rig *rig;QJsonObject spec;QString prefix;
};

static QString spacecraft(const QString &name,int x,int mass,int id,const QString &stamp=QString())
{
   QString code="Create Spacecraft "+name+";\n"+name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DateFormat = "+(stamp.isEmpty()?"A1ModJulian":"UTCGregorian")+";\n"+name+".Epoch = '"+(stamp.isEmpty()?"21545":stamp)+"';\n"+name+".DisplayStateType = Cartesian;\n"+name+".X = "+QString::number(x)+";\n"+name+".Y = 100;\n"+name+".Z = 200;\n"+name+".VX = -0.1;\n"+name+".VY = 7.5;\n"+name+".VZ = 0.8;\n"+name+".DryMass = "+QString::number(mass)+";\n"+name+".NAIFId = "+QString::number(id)+";\n"+name+".NAIFIdReferenceFrame = "+QString::number(id-8000000)+";\n";return code;
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FunctionFiniteBurnBinding");
   try
   {
      require(argc==4&&acceptanceWidgetPlatform(),"Supply parameter-binding startup,catalog and type");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();const QString argument=argv[3];QJsonObject spec;QStringList inventory;
      for(const auto &v:catalog["cases"].toArray())inventory.append(v.toObject()["factory_type"].toString());for(const auto &v:catalog["binding_cases"].toArray())if(v.toObject()["argument"]==argument)spec=v.toObject();
      require(catalog["factory_rows"]==566&&inventory.size()==321&&catalog["initial_types"].toArray().size()==7&&!spec.isEmpty()&&spec["family"]=="FiniteBurn","Full parameter inventory or selected burn fixture differs");const QString key=spec["factory_type"].toString();
      QStringList actual;for(const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::PARAMETER))actual.append(QString::fromStdString(name));inventory+=QStringList{"Array","String","Variable"};inventory.sort();actual.sort();require(actual==inventory,"Advertised complete ParameterFactory universe changed");
      const auto family=spec["family"].toString(),type=spec["factory_type"].toString();std::unique_ptr<Parameter> prototype(FactoryManager::Instance()->CreateParameter(type.toStdString()));
      require(prototype&&prototype->IsSystemParameter()&&prototype->GetTypeName()==spec["metadata"].toObject()["native_type"].toString().toStdString(),"Selected unmanaged native factory control differs");
      const QString factoryUnit=QString::fromStdString(prototype->GetStringParameter(prototype->GetParameterID("Unit")));if(factoryUnit!=spec["unit"].toString())std::cerr<<"FINITE_UNIT actual="<<factoryUnit.toStdString()<<" expected="<<spec["unit"].toString().toStdString()<<std::endl;require(factoryUnit==spec["unit"].toString(),"Native finite-burn factory unit metadata differs from dimensional contract");
      const auto directory=rig.files.filePath("Live parameter owner Ω");require(QDir().mkpath(directory),"Binding Unicode directory absent");
      const auto function=directory+"/ReadShared.gmf",outer=directory+"/NestedShared.gmf",saved=directory+"/Caller.script",report=directory+"/Values.txt";
      auto literal=[](double v){return QString::number(v,'g',17);};
      const bool massFlow=spec["mass_flow"].toBool();
      auto hardware=[&](const QString &engine,const QString &tank,int thrust){QString code="Create ChemicalTank "+tank+";\n"+tank+".FuelMass = 100;\n"+tank+".Pressure = 2000;\n"+tank+".Volume = 1;\n"+tank+".FuelDensity = 1000;\n"+tank+".PressureModel = PressureRegulated;\nCreate ChemicalThruster "+engine+";\n"+engine+".Tank = {"+tank+"};\n"+engine+".DecrementMass = "+(massFlow?"true":"false")+";\n"+engine+".CoordinateSystem = EarthMJ2000Eq;\n"+engine+".ThrustDirection1 = 1;\n"+engine+".ThrustDirection2 = 2;\n"+engine+".ThrustDirection3 = 2;\n"+engine+".DutyCycle = 1;\n"+engine+".ThrustScaleFactor = 1;\n"+engine+".GravitationalAccel = 10;\n";for(int i=1;i<=16;++i)code+=engine+".C"+QString::number(i)+" = "+QString::number(i==1?thrust:0)+";\n"+engine+".K"+QString::number(i)+" = "+QString::number(i==1?300:0)+";\n";return code;};
      auto burn=[](const QString &name,const QString &engine){return "Create FiniteBurn "+name+";\n"+name+".Thrusters = {"+engine+"};\n";};
      const QString declaration=spacecraft("Anchor",7500,275,-115101)+hardware("LocalEngine","LocalFuel",200)+hardware("SharedEngine","SharedFuel",300)+spacecraft("ScopePulse",9000,800,-115102)+"ScopePulse.Tanks = {LocalFuel, SharedFuel};\nScopePulse.Thrusters = {LocalEngine, SharedEngine};\n"+burn("PrivateBurn","LocalEngine");
      const QString mutate="ScopePulse.LocalEngine.C1 = 200 + Calls * 10;\nScopePulse.SharedEngine.C1 = 300 + Calls * 20;\n";
      auto prime=[](const QString &target){return "Ready = 0;\nBeginFiniteBurn PrivateBurn("+target+");\nBeginFiniteBurn SharedBurn("+target+");\nPropagate PulseProp("+target+") {"+target+".ElapsedSecs = 1};\nReady = 1;\n";};
      auto stop=[](const QString &target){return "EndFiniteBurn PrivateBurn("+target+");\nEndFiniteBurn SharedBurn("+target+");\nReady = 2;\n";};
      const bool vectorBridge=type=="Cartesian";const QString vectorDeclarations=vectorBridge?"Create Array PrivateVector[6,1] SharedVector[6,1];\n":QString();
      auto actualReport=[&](bool inside){if(!vectorBridge)return "Report BindingValues "+reference(spec,false,inside)+" "+reference(spec,true,inside)+";\n";QString code="PrivateVector = "+reference(spec,false,inside)+";\nSharedVector = "+reference(spec,true,inside)+";\nReport BindingValues";for(const auto &name:QStringList{"PrivateVector","SharedVector"})for(int i=1;i<=6;++i)code+=" "+name+"("+QString::number(i)+",1)";return code+";\n";};
      const QString original="function [Mark] = ReadShared()\n"+declaration+vectorDeclarations+"Create Variable Mark Keep ScopeEnd;\nKeep = 103;\nScopeEnd = 12345;\nBeginMissionSequence;\nGlobal SharedBurn Calls BindingValues Ready;\nCalls = Calls + 1;\n"+mutate+prime("ScopePulse")+"Write "+reference(spec,false,true)+" "+reference(spec,true,true)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\n"+actualReport(true)+"Propagate PulseProp(ScopePulse) {ScopePulse.ElapsedSecs = 1};\n"+stop("ScopePulse")+"Write "+reference(spec,false,true)+" "+reference(spec,true,true)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\n"+actualReport(true)+"Propagate PulseProp(ScopePulse) {ScopePulse.ElapsedSecs = 1};\nReady = 0;\nMark = Keep + Calls;\n% Live binding ownership Ω\n";
      const auto edited=QString(original).replace("Keep = 103;","Keep = 41;");
      const QString outerBody="function [Mark] = NestedShared()\nCreate Variable Mark;\nCreate GmatFunction ReadShared;\nReadShared.FunctionPath = '"+function+"';\nBeginMissionSequence;\n[Mark] = ReadShared();\n";
      write(function,original.toUtf8());write(outer,outerBody.toUtf8());rig.save("initial.gmf",original.toUtf8());rig.save("nested.gmf",outerBody.toUtf8());
      QString caller=spacecraft("Anchor",7000,987,-115001)+hardware("RootEngine","RootFuel",100)+hardware("SharedEngine","SharedFuel",300)+spacecraft("MainPulse",9000,1800,-115003)+"MainPulse.Tanks = {RootFuel, SharedFuel};\nMainPulse.Thrusters = {RootEngine, SharedEngine};\n"+burn("PrivateBurn","RootEngine")+burn("SharedBurn","SharedEngine");
      caller+=vectorDeclarations+"Create ForceModel PulseFM;\nPulseFM.PrimaryBodies = {};\nPulseFM.PointMasses = {Earth};\nPulseFM.Drag = None;\nPulseFM.SRP = Off;\nCreate Propagator PulseProp;\nPulseProp.FM = PulseFM;\nPulseProp.Type = RungeKutta89;\nPulseProp.InitialStepSize = 1;\nPulseProp.MinStep = 0;\nPulseProp.MaxStep = 1;\nCreate Variable First Second Third Keep Calls ScopeEnd Ready;\nKeep = 103;\nCalls = 0;\nReady = 0;\nScopeEnd = 24680;\nCreate GmatFunction ReadShared NestedShared;\nReadShared.FunctionPath = '"+function+"';\nNestedShared.FunctionPath = '"+outer+"';\nCreate ReportFile BindingValues;\nBindingValues.Filename = '"+directory+"/Bindings.txt';\nBindingValues.WriteHeaders = false;\nBindingValues.FixedWidth = false;\nBindingValues.Precision = 17;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\nBeginMissionSequence;\nGlobal SharedBurn Calls BindingValues Ready;\nWrite 'Arm bindings' Keep {Style = Verbose, MessageWindow = true, LogFile = false};\n"+prime("MainPulse")+"Write "+reference(spec,false,false)+" "+reference(spec,true,false)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\n"+actualReport(false)+"Propagate PulseProp(MainPulse) {MainPulse.ElapsedSecs = 1};\n"+stop("MainPulse")+"Write "+reference(spec,false,false)+" "+reference(spec,true,false)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\n"+actualReport(false)+"Propagate PulseProp(MainPulse) {MainPulse.ElapsedSecs = 1};\nReady = 0;\n[First] = ReadShared();\n[Second] = ReadShared();\n[Third] = NestedShared();\nReport Values First Second Third Keep;\nKeep = Keep + 1;\nReport Values Keep;\n";
      rig.source->setPlainText(caller);rig.save("caller.script",caller.toUtf8());const bool built=rig.window.buildScript();rig.save("build-messages.txt",rig.messages->toPlainText().toUtf8());if(!built){std::cerr<<rig.messages->toPlainText().toStdString();QJsonArray errors;for(const auto &e:Moderator::GetScriptInterpreter()->GetErrorList()){errors.append(QString::fromStdString(e));std::cerr<<e<<std::endl;}rig.save("build-errors.json",QJsonDocument(errors).toJson());}require(built,"Binding main parameter caller Build failed");
      auto prototypes=[&]{QJsonObject values;for(const auto &name:QStringList{"PrivateBurn","SharedBurn","Anchor","MainPulse","RootEngine","SharedEngine","RootFuel","SharedFuel","Calls","Keep","ScopeEnd","Ready"}){auto *p=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(p,"Binding prototype absent");values[name]=QString::fromStdString(p->GetGeneratingString(Gmat::NO_COMMENTS));}return values;};const auto before=prototypes();rig.save("prototypes-before.json",QJsonDocument(before).toJson());
      for (auto *tabs:rig.window.findChildren<QTabWidget*>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==rig.resources || tabs->widget(i)->isAncestorOf(rig.resources)) tabs->setCurrentIndex(i);
      const auto entries=rig.resources->findItems("ReadShared",Qt::MatchExactly|Qt::MatchRecursive); require(entries.size()==1,"Binding function resource absent"); rig.resources->scrollToItem(entries[0]); rig.resources->itemDoubleClicked(entries[0],0); events();
      auto *owner=rig.area->activeSubWindow(); auto *panel=owner?dynamic_cast<ResourceEditor*>(owner->widget()):nullptr; require(panel,"Binding function editor absent"); owner->showNormal(); owner->move(0,0); owner->resize(rig.area->viewport()->size()); events(); auto *edit=panel->findChild<QPushButton*>("editFunctionFile"); reveal(edit);
      const int history=rig.source->document()->availableUndoSteps(); QJsonArray decisions;
      for (bool commit:{false,true})
      {
         bool visited=false; std::exception_ptr failure;
         QTimer::singleShot(0,panel,[&]{auto *dialog=panel->findChild<QDialog*>("functionFileDialog"); try { require(dialog,"Binding file dialog absent"); auto *text=dialog->findChild<QPlainTextEdit*>("functionFileText"); require(text && !text->isReadOnly() && text->toPlainText()==original,"Binding initial file differs"); reveal(text); QTest::mouseClick(text->viewport(),Qt::LeftButton); QTest::keyClick(text,Qt::Key_A,Qt::ControlModifier); QApplication::clipboard()->setText(edited); QTest::keyClick(text,Qt::Key_V,Qt::ControlModifier); events(); require(text->toPlainText()==edited && read(function)==original.toUtf8() && rig.source->toPlainText()==caller && rig.source->document()->availableUndoSteps()==history && prototypes()==before,"Binding draft changed file/caller/history/prototypes"); auto *button=dialog->findChild<QDialogButtonBox*>()->button(commit?QDialogButtonBox::Save:QDialogButtonBox::Cancel); reveal(button); QTest::mouseClick(button,Qt::LeftButton); visited=true; } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }});
         QTest::mouseClick(edit,Qt::LeftButton); if (failure) std::rethrow_exception(failure);
         require(visited && read(function)==(commit?edited:original).toUtf8() && rig.source->toPlainText()==caller,"Binding file Save/Cancel differs"); decisions.append(QJsonObject{{"saved",commit}});
      }
      require(owner->close(),"Binding editor failed to close"); events(); rig.save("edited.gmf",edited.toUtf8()); require(rig.window.saveScriptTo(saved),"Binding Unicode caller Save failed"); QJsonArray phases;

      for(int phase=0;phase<3;++phase)
      {
         if(phase==1)require(rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==caller&&prototypes()==before,"Binding Unicode reopen changed source/prototypes");
         const auto tag=QString::number(phase);BindingObserver observer(rig,spec,tag);Debugger *debug=nullptr;for(auto *child:rig.window.children())if(auto *candidate=dynamic_cast<Debugger*>(child))debug=candidate;
         int arm=-1;const auto snapshot=rig.window.missionSnapshot();for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].statement.contains("Arm bindings"))arm=i;
         require(debug&&arm>=0&&rig.window.setBreakpoint(arm,true),"Binding actual observer arm boundary absent");bool armed=false;std::exception_ptr armError;auto previous=debug->paused;
         debug->paused=[&](const QString &kind){if(previous)previous(kind);try{require(!armed&&kind=="Write","Binding arm command/order differs");observer.SetInternalCoordSystem(Publisher::Instance()->GetInternalCoordSystem());if(auto *frame=Publisher::Instance()->GetDataCoordSystem())observer.SetDataCoordSystem(frame);require(Publisher::Instance()->Subscribe(&observer),"Binding observer subscription failed");armed=true;if(auto *dialog=rig.window.findChild<QDialog*>("debuggerDialog"))acceptanceView(*dialog);}catch(...){armError=std::current_exception();rig.window.stopMission();}QTimer::singleShot(0,&rig.window,[&]{debug->resume();});};
         const int undo=rig.source->document()->availableUndoSteps();rig.messages->clear();acceptanceView(rig.window);const auto outcome=rig.window.runMission();debug->paused=previous;Publisher::Instance()->Unsubscribe(&observer);events();
         const auto messages=rig.messages->toPlainText();rig.save(tag+"-messages.txt",messages.toUtf8());rig.save(tag+"-observer-error.txt",observer.error.toUtf8());if(armError)std::rethrow_exception(armError);if(observer.failure)std::rethrow_exception(observer.failure);
         if(outcome!=MainWindow::RunResult::Completed)std::cerr<<messages.toStdString();require(outcome==MainWindow::RunResult::Completed&&armed&&observer.records.size()==8,"Binding scope mission/callback census failed");
         QJsonArray privateWrites,sharedWrites;
         auto parse=[&](const QString &ref,const QJsonValue &expected){QJsonArray values;const QString delimiters="(?:"+QRegularExpression::escape(reference(spec,false,false))+"|"+QRegularExpression::escape(reference(spec,false,true))+"|"+QRegularExpression::escape(reference(spec,true,true))+"|ScopeEnd)";auto matches=QRegularExpression("(?:^|\\n)"+QRegularExpression::escape(ref)+" =\\s*\\n([\\s\\S]*?)(?=\\n"+delimiters+" =)").globalMatch(messages);while(matches.hasNext()){auto raw=matches.next().captured(1).trimmed();if(expected.isString()){if(raw.startsWith('\'')&&raw.endsWith('\''))raw=raw.mid(1,raw.size()-2);values.append(raw);}else{for(const auto &c:QStringList{"[","]",";",","})raw.replace(c," ");const auto words=raw.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);if(expected.isArray()){QJsonArray array;for(const auto &word:words){bool ok;const double v=word.toDouble(&ok);require(ok&&std::isfinite(v),"Actual binding array Write invalid");array.append(v);}values.append(array);}else{require(words.size()==1,"Actual scalar Write width differs");bool ok;const double v=words[0].toDouble(&ok);require(ok&&std::isfinite(v),"Actual binding scalar Write invalid");values.append(v);}}}return values;};
         const auto first=observer.records[0].toObject()["parameters"].toArray()[0].toObject()["actual"];
         if(reference(spec,false,false)==reference(spec,false,true))privateWrites=parse(reference(spec,false,false),first);
         else{privateWrites=parse(reference(spec,false,false),first);const auto local=parse(reference(spec,false,true),first);for(const auto &v:local)privateWrites.append(v);}
         sharedWrites=parse(reference(spec,true,true),first);require(privateWrites.size()==8&&sharedWrites.size()==8,"Actual main/private/global parameter Write count differs");
         for(int i=0;i<8;++i){const auto parameters=observer.records[i].toObject()["parameters"].toArray();compareValue(privateWrites[i],parameters[0].toObject()["actual"],bound(family,type));compareValue(sharedWrites[i],parameters[1].toObject()["actual"],bound(family,type));}
         const auto bindingBytes=read(directory+"/Bindings.txt");require(!bindingBytes.isEmpty(),"Actual binding parameter Report produced no bytes");rig.save(tag+"-binding-values.txt",bindingBytes);
         const auto raw=read(report);rig.save(tag+"-values.txt",raw);QJsonArray markers;for(const auto &word:QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)){bool ok;const double v=word.toDouble(&ok);require(ok&&std::isfinite(v),"Binding marker report invalid");markers.append(v);}require(markers==QJsonArray{42,43,44,103,104},"Binding file/private/global/caller marker differs");
         auto *root=Moderator::Instance()->GetInternalObject("PrivateBurn");auto *anchor=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("Anchor"));require(root&&anchor&&anchor->GetRealParameter("DryMass")==987&&anchor->GetState()[0]==7000&&Moderator::Instance()->GetInternalObject("ScopeEnd")->GetRealParameter("Value")==24680,"Binding private use changed caller shadow/sentinel");
         const auto after=prototypes();rig.save(tag+"-prototypes.json",QJsonDocument(after).toJson());const QJsonObject invariants{{"root_selected_property",root->GetStringArrayParameter("Thrusters")==StringArray{"RootEngine"}},{"prototypes",after==before},{"source",rig.source->toPlainText()==caller},{"history",rig.source->document()->availableUndoSteps()==undo},{"function_file",read(function)==edited.toUtf8()},{"nested_file",read(outer)==outerBody.toUtf8()},{"not_running",!rig.window.isRunning()}};rig.save(tag+"-invariants.json",QJsonDocument(invariants).toJson());for(const auto &v:invariants)require(v.toBool(),"Binding run changed source/history/prototypes/files/lock");
         phases.append(QJsonObject{{"phase",phase},{"explicit_rebuild",phase<2},{"live_samples",observer.records},{"private_writes",privateWrites},{"shared_writes",sharedWrites},{"raw_markers",markers},{"callback_private_maps_accessed_only_while_executing",true}});rig.save("phases.json",QJsonDocument(phases).toJson());
      }
      auto *tree=rig.window.findChild<QTreeWidget*>("Output"); const auto outputs=tree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(outputs.size()==1,"Binding report output absent"); tree->itemDoubleClicked(outputs[0],0); events(); auto *viewer=rig.area->activeSubWindow(); require(viewer && dynamic_cast<ReportViewer*>(viewer->widget()),"Binding actual report viewer absent"); auto *shown=viewer->findChild<QPlainTextEdit*>(); require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(read(report)).trimmed(),"Binding viewer differs from checked bytes"); acceptanceView(*viewer);

      const QJsonObject proof{{"checkpoint","live-function-finite-burn-parameter-binding"},{"outcome","passed"},{"case",key},{"argument",argument},{"route","active-stopped-finite-burn"},{"unmanaged_factory_unit",factoryUnit},{"executed_begin_end_commands",48},{"propagations",36},{"family",family},{"full_system_parameter_inventory",321},{"initial_batch_types",catalog["initial_types"]},{"missions",3},{"function_calls",12},{"live_callback_samples",24},{"actual_parameter_owner_checks",48},{"file_decisions",decisions},{"phases",phases},{"scope","Actual live main/private/Global FiniteBurn Parameter owners,active/stopped values,nonzero constant-thrust or mass-flow literals,actual burn-to-target-attached thruster/tank and frame graph,shared burn retarget through direct/direct/nested calls. Three missions keep file Cancel/Save,source/history/prototypes,caller anchor,Unicode/no-Build/report viewing. All321 types and prior50 native types retained;no general propulsion/force/integration/time/whole-scope/science qualification. All private maps and hardware references observed synchronously before function return;main GOS is a value snapshot."}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }
   catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
   catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
   return 1;
}
