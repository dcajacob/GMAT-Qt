#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "ObjectManagedFunction.hpp"
#include "Parameter.hpp"
#include "ParameterInfo.hpp"
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
static double isp(bool shared,bool inside,int calls){return (shared?500:inside?400:300)+(inside?calls*(shared?10:5):0);}
static double duty(bool shared,bool inside){return shared?1:inside?.75:.5;}
static double scale(bool shared,bool inside){return shared?.8:inside?1.5:1.25;}
static double rate(bool shared,bool inside,int calls){return force(shared,inside,calls)*duty(shared,inside)/(10*isp(shared,inside,calls));}
static double previousSharedLoss(int calls){double loss=2*rate(true,false,0);for(int i=1;i<calls;++i)loss+=2*rate(true,true,i);return loss;}
static QJsonValue expectedValue(const QJsonObject &spec,bool shared,bool inside,int calls,bool firing)
{
   if(!firing)return 0.;const auto type=spec["factory_type"].toString();return type=="ThrustMagnitude"?force(shared,inside,calls)*duty(shared,inside)*scale(shared,inside):type=="Isp"?isp(shared,inside,calls):-rate(shared,inside,calls);
}
static double nativeValue(Thruster *engine,const QString &type){return type=="ThrustMagnitude"?engine->GetThrustMagnitude():type=="Isp"?engine->GetIsp():engine->GetMassFlowRate();}
static QJsonObject rootSnapshot()
{
   auto *sc=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("PrivateOnly"));require(sc,"Caller root snapshot owner absent");auto *engine=sc->GetRefObject(Gmat::THRUSTER,"RootEngine"),*tank=sc->GetRefObject(Gmat::FUEL_TANK,"RootFuel");require(engine&&tank,"Caller root snapshot hardware absent");QJsonArray state;for(int i=0;i<6;++i)state.append(sc->GetState()[i]);
   return QJsonObject{{"owner_pointer",pointer(sc)},{"engine_pointer",pointer(engine)},{"tank_pointer",pointer(tank)},{"state",state},{"epoch",sc->GetEpoch()},{"dry_mass",sc->GetRealParameter("DryMass")},{"total_mass",sc->GetRealParameter("TotalMass")},{"fuel_mass",tank->GetRealParameter("FuelMass")},{"C1",engine->GetRealParameter("C1")},{"K1",engine->GetRealParameter("K1")},{"duty",engine->GetRealParameter("DutyCycle")},{"scale",engine->GetRealParameter("ThrustScaleFactor")},{"firing",engine->GetBooleanParameter("IsFiring")}};
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
   BindingObserver(Rig &rig,const QJsonObject &spec,const QString &prefix):Subscriber("DerivedThrusterObserver","DerivedThrusterObserver"),rig(&rig),spec(spec),prefix(prefix){}
   ~BindingObserver() override{Publisher::Instance()->Unsubscribe(this);}
   GmatBase *Clone() const override{return new BindingObserver(*this);}
   bool ReceiveData(const Real *data,Integer count=0) override
   {
      bool initial=false;const auto *published=Publisher::GetPublishedPropagationObjects(data,count,initial);if(!published||!initial)return true;
      try
      {
         auto *command=dynamic_cast<GmatCommand*>(currentProvider);require(command,"Derived thruster provider is not a command");auto *function=dynamic_cast<ObjectManagedFunction*>(command->GetCurrentFunction());const bool inside=function!=nullptr;
         const auto *locals=inside?function->GetFunctionObjectStore():command->GetObjectMap();const auto rootGlobals=inside?ObjectMap{}:Moderator::Instance()->GetSandbox()->GetGlobalObjectMap();const auto *globals=inside?function->GetGlobalObjectStore():&rootGlobals;require(locals&&globals,"Derived live maps absent");
         auto lookup=[&](const std::string &name){auto a=locals->find(name);if(a!=locals->end())return a->second;auto b=globals->find(name);return b==globals->end()?nullptr:b->second;};auto *gate=lookup("Ready"),*counter=lookup("Calls");require(gate&&counter,"Derived gate/counter absent");const int ready=int(gate->GetRealParameter("Value")),calls=int(counter->GetRealParameter("Value"));if(ready==0)return true;
         if(ready==3)
         {
            require(published->size()==1&&(*published)[0]==lookup("SharedSC"),"Caller isolation checkpoint provider differs");const auto current=rootSnapshot();if(!inside)rootAfterMain=current;controls.append(QJsonObject{{"inside_function",inside},{"calls",calls},{"snapshot",current},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))}});rig->save(prefix+"-root-controls.json",QJsonDocument(controls).toJson());require(!rootAfterMain.isEmpty()&&current==rootAfterMain&&calls==controls.size()-1&&inside==(calls>0),"Function use changed caller root state/hardware after completed main burn");return true;
         }
         require(ready==1||ready==2,"Derived gate invalid");const bool firing=ready==1;require(published->size()==1&&(*published)[0]==lookup("PrivateOnly"),"Actual published propagation owner differs from parameter owner");
         events.append(QJsonObject{{"inside_function",inside},{"function",inside?QString::fromStdString(function->GetName()):QString()},{"calls",calls},{"ready",ready},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))}});rig->save(prefix+"-callback-events.json",QJsonDocument(events).toJson());require(calls==(events.size()-1)/2&&ready==(events.size()%2?1:2)&&inside==(calls>0)&&(!inside||function->GetName()=="ReadShared"),"Derived callback scope/order/lifecycle differs");
         const auto type=spec["factory_type"].toString();const auto meta=spec["metadata"].toObject();QJsonArray rows;
         for(bool shared:{false,true})
         {
            const QString ownerName=shared?"SharedSC":"PrivateOnly",full=reference(spec,shared,inside);auto *owner=dynamic_cast<Spacecraft*>(lookup(ownerName.toStdString()));auto *root=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject(ownerName.toStdString()));auto *configured=Moderator::Instance()->GetConfiguredObject(ownerName.toStdString());require(owner&&root&&configured&&owner!=configured&&owner->IsOfType(meta["owner_type_id"].toInt()),"Derived native owner/prototype differs");require(owner->IsGlobal()==shared&&owner->IsLocal()==(inside&&!shared)&&(shared?owner==root:inside?owner!=root:owner==root),"Derived owner scope/flags differ");
            auto *parameter=dynamic_cast<Parameter*>(lookup(full.toStdString()));require(parameter&&parameter->IsSystemParameter()&&parameter->GetName()==full.toStdString()&&parameter->GetTypeName()==meta["native_type"].toString().toStdString()&&int(parameter->GetReturnType())==meta["return_type"].toInt()&&int(parameter->GetOwnerType())==meta["owner_type_id"].toInt(),"Derived Parameter metadata differs");auto *referent=parameter->GetRefObject(parameter->GetOwnerType(),ownerName.toStdString());require(referent==owner,"Derived Parameter does not reference actual scoped Spacecraft");
            const std::string engineName=shared?"SharedEngine":inside?"LocalEngine":"RootEngine",tankName=shared?"SharedFuel":inside?"LocalFuel":"RootFuel",burnName=shared?"SharedBurn":"PrivateBurn";auto *nativeEngine=dynamic_cast<Thruster*>(owner->GetRefObject(Gmat::THRUSTER,engineName));auto *engine=static_cast<GmatBase*>(nativeEngine);auto *tank=owner->GetRefObject(Gmat::FUEL_TANK,tankName);auto *burn=dynamic_cast<Burn*>(lookup(burnName));auto *rootEngine=root->GetRefObject(Gmat::THRUSTER,shared?"SharedEngine":"RootEngine");auto *configuredEngine=Moderator::Instance()->GetConfiguredObject(shared?"SharedEngine":"RootEngine");require(engine&&tank&&burn&&rootEngine&&configuredEngine&&engine->GetTypeName()=="ChemicalThruster"&&tank->GetTypeName()=="ChemicalTank"&&burn->GetTypeName()=="FiniteBurn"&&burn->GetRefObject(Gmat::THRUSTER,engineName)==engine,"Derived burn/owner/engine/tank identity differs");require(engine!=configuredEngine&&(shared?engine==rootEngine:inside?engine!=rootEngine:engine==rootEngine),"Derived private/global engine clone differs");const auto &tanks=engine->GetRefObjectArray(Gmat::HARDWARE);require(tanks.size()==1&&tanks[0]==tank,"Derived engine-to-owner tank identity differs");
            const double elapsed=(owner->GetEpoch()-21545)*86400,expectedElapsed=shared&&inside?3*calls+ready:ready;require(std::abs(elapsed-expectedElapsed)<=3e-6,"Derived owner elapsed interval differs");const double previous=shared&&inside?previousSharedLoss(calls):0,expectedFuel=100-previous-ready*rate(shared,inside,calls),fuel=tank->GetRealParameter("FuelMass"),dry=shared?2000:inside?1100:1000,total=owner->GetRealParameter("TotalMass");require(std::abs(fuel-expectedFuel)<=1e-7&&owner->GetRealParameter("DryMass")==dry&&std::abs(total-(dry+expectedFuel))<=1e-7,"Derived separate/cumulative Global mass budget differs");
            const double f=force(shared,inside,calls),k=isp(shared,inside,calls),d=duty(shared,inside),q=scale(shared,inside);require(engine->GetRealParameter("C1")==f&&engine->GetRealParameter("K1")==k&&engine->GetRealParameter("GravitationalAccel")==10&&engine->GetRealParameter("DutyCycle")==d&&engine->GetRealParameter("ThrustScaleFactor")==q&&engine->GetBooleanParameter("DecrementMass")&&engine->GetBooleanParameter("IsFiring")==firing&&engine->GetStringParameter("CoordinateSystem")=="EarthMJ2000Eq"&&burn->HasFired()&&burn->IsFiring()==firing,"Derived configured hardware/lifecycle differs");for(int i=2;i<=16;++i)require(engine->GetRealParameter("C"+std::to_string(i))==0&&engine->GetRealParameter("K"+std::to_string(i))==0,"Derived constant-polynomial fixture differs");for(int i=1;i<=3;++i)require(engine->GetRealParameter("ThrustDirection"+std::to_string(i))==double(i==1?1:2),"Derived direction fixture differs");
            const QString unit=QString::fromStdString(parameter->GetStringParameter(parameter->GetParameterID("Unit")));require(unit==spec["unit"].toString(),"Derived live unit metadata differs");const bool infoSettable=ParameterInfo::Instance()->IsSettable(type.toStdString());require(parameter->IsSettable()==meta["settable"].toBool()&&infoSettable==meta["settable"].toBool(),"Derived existing writable metadata changed");const double actual=parameter->EvaluateReal(),direct=nativeValue(nativeEngine,type);const auto expected=expectedValue(spec,shared,inside,calls,firing);rig->save(prefix+"-numeric-observation.json",QJsonDocument(QJsonObject{{"reference",full},{"actual",actual},{"direct",direct},{"expected",expected},{"elapsed",elapsed},{"fuel_mass",fuel},{"expected_fuel_mass",expectedFuel}}).toJson());compareValue(actual,expected,1e-12);compareValue(direct,expected,1e-12);
            QString setterResult="not-probed-stopped",setterError;if(firing){setterResult="accepted";try{parameter->SetReal(actual);}catch(BaseException &e){setterResult="rejected";setterError=QString::fromStdString(e.GetFullMessage());}compareValue(parameter->EvaluateReal(),expected,1e-12);}
            const QJsonObject hardware{{"engine_name",QString::fromStdString(engineName)},{"engine_pointer",pointer(engine)},{"root_engine_pointer",pointer(rootEngine)},{"configured_engine_pointer",pointer(configuredEngine)},{"burn_name",QString::fromStdString(burnName)},{"burn_pointer",pointer(burn)},{"burn_engine_pointer",pointer(burn->GetRefObject(Gmat::THRUSTER,engineName))},{"burn_firing",burn->IsFiring()},{"burn_has_fired",burn->HasFired()},{"tank_name",QString::fromStdString(tankName)},{"tank_pointer",pointer(tank)},{"engine_tank_pointer",pointer(tanks[0])},{"C1",f},{"K1",k},{"duty",d},{"scale",q},{"gravity",10},{"decrement_mass",true},{"engine_firing",firing},{"direction",QJsonArray{1,2,2}},{"frame_name","EarthMJ2000Eq"},{"thruster_frame_pointer_directly_observed",false},{"fuel_mass",fuel},{"expected_fuel_mass",expectedFuel},{"previous_Global_fuel_loss",previous},{"owner_epoch",owner->GetEpoch()},{"owner_elapsed",elapsed},{"expected_owner_elapsed",expectedElapsed},{"dry_mass",dry},{"total_mass",total},{"expected_total_mass",dry+expectedFuel},{"runtime_settable",parameter->IsSettable()},{"info_settable",infoSettable},{"native_setter_result",setterResult},{"native_setter_error",setterError}};
            rows.append(QJsonObject{{"reference",full},{"factory_type",type},{"native_type",QString::fromStdString(parameter->GetTypeName())},{"unit",unit},{"return_type",int(parameter->GetReturnType())},{"owner_type",int(parameter->GetOwnerType())},{"parameter_pointer",pointer(parameter)},{"reference_owner_pointer",pointer(referent)},{"live_owner_pointer",pointer(owner)},{"root_owner_pointer",pointer(root)},{"configured_owner_pointer",pointer(configured)},{"owner_name",ownerName},{"owner_type_name",QString::fromStdString(owner->GetTypeName())},{"owner_global",owner->IsGlobal()},{"owner_local",owner->IsLocal()},{"parameter_global",parameter->IsGlobal()},{"parameter_local",parameter->IsLocal()},{"shared",shared},{"shape",QJsonArray{}},{"actual",actual},{"expected",expected},{"direct_native_method_value",direct},{"absolute_bound",1e-12},{"hardware",hardware}});
         }
         records.append(QJsonObject{{"inside_function",inside},{"function",inside?QString::fromStdString(function->GetName()):QString()},{"calls",calls},{"ready",ready},{"firing",firing},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))},{"published_owner_pointer",pointer((*published)[0])},{"parameters",rows},{"global_map_is_value_snapshot",!inside}});rig->save(prefix+"-live-parameter-samples.json",QJsonDocument(records).toJson());
      }
      catch(BaseException &e){if(!failure){error=QString::fromStdString(e.GetFullMessage());failure=std::current_exception();}}
      catch(std::exception &e){if(!failure){error=e.what();failure=std::current_exception();}}
      return true;
   }
   QJsonArray records,events,controls;QJsonObject rootAfterMain;std::exception_ptr failure;QString error;
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
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FunctionDerivedThrusterBinding");
   try
   {
      require(argc==4&&acceptanceWidgetPlatform(),"Supply parameter-binding startup,catalog and type");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();const QString argument=argv[3];QJsonObject spec;QStringList inventory;
      for(const auto &v:catalog["cases"].toArray())inventory.append(v.toObject()["factory_type"].toString());for(const auto &v:catalog["derived_binding_cases"].toArray())if(v.toObject()["argument"]==argument)spec=v.toObject();
      require(catalog["factory_rows"]==566&&inventory.size()==321&&catalog["initial_types"].toArray().size()==3&&!spec.isEmpty()&&spec["family"]=="DerivedThruster","Full parameter inventory or selected burn fixture differs");const QString key=spec["factory_type"].toString();
      QStringList actual;for(const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::PARAMETER))actual.append(QString::fromStdString(name));inventory+=QStringList{"Array","String","Variable"};inventory.sort();actual.sort();require(actual==inventory,"Advertised complete ParameterFactory universe changed");
      const auto family=spec["family"].toString(),type=spec["factory_type"].toString();std::unique_ptr<Parameter> prototype(FactoryManager::Instance()->CreateParameter(type.toStdString()));
      require(prototype&&prototype->IsSystemParameter()&&prototype->GetTypeName()==spec["metadata"].toObject()["native_type"].toString().toStdString(),"Selected unmanaged native factory control differs");
      const QString factoryUnit=QString::fromStdString(prototype->GetStringParameter(prototype->GetParameterID("Unit")));rig.save("factory-unit.json",QJsonDocument(QJsonObject{{"case",key},{"actual",factoryUnit},{"expected",spec["unit"]},{"settable",prototype->IsSettable()}}).toJson());require(factoryUnit==spec["unit"].toString(),"Native derived-thruster factory unit metadata differs from dimensional contract");
      const auto directory=rig.files.filePath("Live parameter owner Ω");require(QDir().mkpath(directory),"Binding Unicode directory absent");
      const auto function=directory+"/ReadShared.gmf",outer=directory+"/NestedShared.gmf",saved=directory+"/Caller.script",report=directory+"/Values.txt";
      auto literal=[](double v){return QString::number(v,'g',17);};
      auto hardware=[&](const QString &engine,const QString &tank,int scope){const bool shared=scope==2,inside=scope==1;QString code="Create ChemicalTank "+tank+";\n"+tank+".FuelMass = 100;\n"+tank+".Pressure = 2000;\n"+tank+".Volume = 1;\n"+tank+".FuelDensity = 1000;\n"+tank+".PressureModel = PressureRegulated;\nCreate ChemicalThruster "+engine+";\n"+engine+".Tank = {"+tank+"};\n"+engine+".DecrementMass = true;\n"+engine+".CoordinateSystem = EarthMJ2000Eq;\n"+engine+".ThrustDirection1 = 1;\n"+engine+".ThrustDirection2 = 2;\n"+engine+".ThrustDirection3 = 2;\n"+engine+".DutyCycle = "+literal(duty(shared,inside))+";\n"+engine+".ThrustScaleFactor = "+literal(scale(shared,inside))+";\n"+engine+".GravitationalAccel = 10;\n";for(int i=1;i<=16;++i)code+=engine+".C"+QString::number(i)+" = "+literal(i==1?force(shared,inside,0):0)+";\n"+engine+".K"+QString::number(i)+" = "+literal(i==1?isp(shared,inside,0):0)+";\n";return code;};
      auto attach=[](const QString &owner,const QString &engine,const QString &tank){return owner+".Tanks = {"+tank+"};\n"+owner+".Thrusters = {"+engine+"};\n";};
      auto burn=[](const QString &name,const QString &engine){return "Create FiniteBurn "+name+";\n"+name+".Thrusters = {"+engine+"};\n";};
      auto prop=[](const QString &name,const QString &owner){return "Propagate "+name+"("+owner+") {"+owner+".ElapsedSecs = 1};\n";};
      auto actualReport=[&](bool inside){return "Report BindingValues "+reference(spec,false,inside)+" "+reference(spec,true,inside)+";\n";};
      auto observe=[&](bool inside){return "Write "+reference(spec,false,inside)+" "+reference(spec,true,inside)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\n"+actualReport(inside);};
      auto lifecycle=[&](bool inside){return "Ready = 0;\nBeginFiniteBurn PrivateBurn(PrivateOnly);\nBeginFiniteBurn SharedBurn(SharedSC);\n"+prop("PrivateProp","PrivateOnly")+prop("SharedProp","SharedSC")+"Ready = 1;\n"+observe(inside)+prop("PrivateProp","PrivateOnly")+"Ready = 0;\n"+prop("SharedProp","SharedSC")+"EndFiniteBurn PrivateBurn(PrivateOnly);\nEndFiniteBurn SharedBurn(SharedSC);\nReady = 2;\n"+observe(inside)+prop("PrivateProp","PrivateOnly")+"Ready = 3;\n"+prop("SharedProp","SharedSC")+"Ready = 0;\n";};
      const QString declaration=spacecraft("Anchor",7500,275,-122104)+hardware("LocalEngine","LocalFuel",1)+spacecraft("PrivateOnly",7500,1100,-122101)+attach("PrivateOnly","LocalEngine","LocalFuel")+burn("PrivateBurn","LocalEngine");
      const QString mutate="PrivateOnly.LocalEngine.C1 = 200 + Calls * 10;\nPrivateOnly.LocalEngine.K1 = 400 + Calls * 5;\nSharedSC.SharedEngine.C1 = 300 + Calls * 20;\nSharedSC.SharedEngine.K1 = 500 + Calls * 10;\n";
      const QString original="function [Mark] = ReadShared()\n"+declaration+"Create Variable Mark Keep ScopeEnd;\nKeep = 103;\nScopeEnd = 12345;\nBeginMissionSequence;\nGlobal SharedSC SharedBurn Calls BindingValues Ready;\nCalls = Calls + 1;\n"+mutate+lifecycle(true)+"Mark = Keep + Calls;\n% Live derived thruster ownership Ω\n";
      const auto edited=QString(original).replace("Keep = 103;","Keep = 41;");
      const QString outerBody="function [Mark] = NestedShared()\nCreate Variable Mark;\nCreate GmatFunction ReadShared;\nReadShared.FunctionPath = '"+function+"';\nBeginMissionSequence;\n[Mark] = ReadShared();\n";
      write(function,original.toUtf8());write(outer,outerBody.toUtf8());rig.save("initial.gmf",original.toUtf8());rig.save("nested.gmf",outerBody.toUtf8());
      QString caller=spacecraft("Anchor",7000,987,-122004)+hardware("RootEngine","RootFuel",0)+hardware("SharedEngine","SharedFuel",2)+spacecraft("PrivateOnly",7000,1000,-122001)+spacecraft("SharedSC",8000,2000,-122002)+attach("PrivateOnly","RootEngine","RootFuel")+attach("SharedSC","SharedEngine","SharedFuel")+burn("PrivateBurn","RootEngine")+burn("SharedBurn","SharedEngine");
      caller+="Create ForceModel PulseFM;\nPulseFM.PrimaryBodies = {};\nPulseFM.PointMasses = {Earth};\nPulseFM.Drag = None;\nPulseFM.SRP = Off;\n";for(const auto &name:QStringList{"PrivateProp","SharedProp"})caller+="Create Propagator "+name+";\n"+name+".FM = PulseFM;\n"+name+".Type = RungeKutta89;\n"+name+".InitialStepSize = 1;\n"+name+".MinStep = 0;\n"+name+".MaxStep = 1;\n";
      caller+="Create Variable First Second Third Keep Calls ScopeEnd Ready;\nKeep = 103;\nCalls = 0;\nReady = 0;\nScopeEnd = 24680;\nCreate GmatFunction ReadShared NestedShared;\nReadShared.FunctionPath = '"+function+"';\nNestedShared.FunctionPath = '"+outer+"';\nCreate ReportFile BindingValues;\nBindingValues.Filename = '"+directory+"/Bindings.txt';\nBindingValues.WriteHeaders = false;\nBindingValues.FixedWidth = false;\nBindingValues.Precision = 17;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\nBeginMissionSequence;\nGlobal SharedSC SharedBurn Calls BindingValues Ready;\nWrite 'Arm bindings' Keep {Style = Verbose, MessageWindow = true, LogFile = false};\n"+lifecycle(false)+"[First] = ReadShared();\n[Second] = ReadShared();\n[Third] = NestedShared();\nReport Values First Second Third Keep;\nKeep = Keep + 1;\nReport Values Keep;\n";
      rig.source->setPlainText(caller);rig.save("caller.script",caller.toUtf8());const bool built=rig.window.buildScript();rig.save("build-messages.txt",rig.messages->toPlainText().toUtf8());if(!built){std::cerr<<rig.messages->toPlainText().toStdString();QJsonArray errors;for(const auto &e:Moderator::GetScriptInterpreter()->GetErrorList()){errors.append(QString::fromStdString(e));std::cerr<<e<<std::endl;}rig.save("build-errors.json",QJsonDocument(errors).toJson());}require(built,"Binding main parameter caller Build failed");
      auto prototypes=[&]{QJsonObject values;for(const auto &name:QStringList{"PrivateBurn","SharedBurn","Anchor","PrivateOnly","SharedSC","RootEngine","SharedEngine","RootFuel","SharedFuel","Calls","Keep","ScopeEnd","Ready"}){auto *p=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(p,"Binding prototype absent");values[name]=QString::fromStdString(p->GetGeneratingString(Gmat::NO_COMMENTS));}return values;};const auto before=prototypes();rig.save("prototypes-before.json",QJsonDocument(before).toJson());
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
         if(outcome!=MainWindow::RunResult::Completed)std::cerr<<messages.toStdString();require(outcome==MainWindow::RunResult::Completed&&armed&&observer.records.size()==8&&observer.controls.size()==4,"Binding scope mission/callback census failed");
         QJsonArray privateWrites,sharedWrites;
         auto parse=[&](const QString &ref,const QJsonValue &expected){QJsonArray values;const QString delimiters="(?:"+QRegularExpression::escape(reference(spec,false,false))+"|"+QRegularExpression::escape(reference(spec,false,true))+"|"+QRegularExpression::escape(reference(spec,true,true))+"|ScopeEnd)";auto matches=QRegularExpression("(?:^|\\n)"+QRegularExpression::escape(ref)+" =\\s*\\n([\\s\\S]*?)(?=\\n"+delimiters+" =)").globalMatch(messages);while(matches.hasNext()){auto raw=matches.next().captured(1).trimmed();if(expected.isString()){if(raw.startsWith('\'')&&raw.endsWith('\''))raw=raw.mid(1,raw.size()-2);values.append(raw);}else{for(const auto &c:QStringList{"[","]",";",","})raw.replace(c," ");const auto words=raw.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);if(expected.isArray()){QJsonArray array;for(const auto &word:words){bool ok;const double v=word.toDouble(&ok);require(ok&&std::isfinite(v),"Actual binding array Write invalid");array.append(v);}values.append(array);}else{require(words.size()==1,"Actual scalar Write width differs");bool ok;const double v=words[0].toDouble(&ok);require(ok&&std::isfinite(v),"Actual binding scalar Write invalid");values.append(v);}}}return values;};
         const auto first=observer.records[0].toObject()["parameters"].toArray()[0].toObject()["actual"];
         if(reference(spec,false,false)==reference(spec,false,true))privateWrites=parse(reference(spec,false,false),first);
         else{privateWrites=parse(reference(spec,false,false),first);const auto local=parse(reference(spec,false,true),first);for(const auto &v:local)privateWrites.append(v);}
         sharedWrites=parse(reference(spec,true,true),first);require(privateWrites.size()==8&&sharedWrites.size()==8,"Actual main/private/global parameter Write count differs");
         for(int i=0;i<8;++i){const auto parameters=observer.records[i].toObject()["parameters"].toArray();compareValue(privateWrites[i],parameters[0].toObject()["actual"],bound(family,type));compareValue(sharedWrites[i],parameters[1].toObject()["actual"],bound(family,type));}
         const auto bindingBytes=read(directory+"/Bindings.txt");require(!bindingBytes.isEmpty(),"Actual binding parameter Report produced no bytes");rig.save(tag+"-binding-values.txt",bindingBytes);
         const auto raw=read(report);rig.save(tag+"-values.txt",raw);QJsonArray markers;for(const auto &word:QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)){bool ok;const double v=word.toDouble(&ok);require(ok&&std::isfinite(v),"Binding marker report invalid");markers.append(v);}require(markers==QJsonArray{42,43,44,103,104},"Binding file/private/global/caller marker differs");
         auto *root=Moderator::Instance()->GetInternalObject("PrivateBurn");auto *anchor=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("Anchor"));require(root&&rootSnapshot()==observer.rootAfterMain&&anchor&&anchor->GetRealParameter("DryMass")==987&&anchor->GetState()[0]==7000&&Moderator::Instance()->GetInternalObject("ScopeEnd")->GetRealParameter("Value")==24680,"Binding private use changed caller shadow/sentinel");
         const auto after=prototypes();rig.save(tag+"-prototypes.json",QJsonDocument(after).toJson());const QJsonObject invariants{{"root_selected_property",root->GetStringArrayParameter("Thrusters")==StringArray{"RootEngine"}&&rootSnapshot()==observer.rootAfterMain},{"prototypes",after==before},{"source",rig.source->toPlainText()==caller},{"history",rig.source->document()->availableUndoSteps()==undo},{"function_file",read(function)==edited.toUtf8()},{"nested_file",read(outer)==outerBody.toUtf8()},{"not_running",!rig.window.isRunning()}};rig.save(tag+"-invariants.json",QJsonDocument(invariants).toJson());for(const auto &v:invariants)require(v.toBool(),"Binding run changed source/history/prototypes/files/lock");
         phases.append(QJsonObject{{"phase",phase},{"explicit_rebuild",phase<2},{"live_samples",observer.records},{"root_controls",observer.controls},{"caller_snapshot",observer.rootAfterMain},{"private_writes",privateWrites},{"shared_writes",sharedWrites},{"raw_markers",markers},{"callback_private_maps_accessed_only_while_executing",true}});rig.save("phases.json",QJsonDocument(phases).toJson());
      }
      auto *tree=rig.window.findChild<QTreeWidget*>("Output"); const auto outputs=tree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(outputs.size()==1,"Binding report output absent"); tree->itemDoubleClicked(outputs[0],0); events(); auto *viewer=rig.area->activeSubWindow(); require(viewer && dynamic_cast<ReportViewer*>(viewer->widget()),"Binding actual report viewer absent"); auto *shown=viewer->findChild<QPlainTextEdit*>(); require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(read(report)).trimmed(),"Binding viewer differs from checked bytes"); acceptanceView(*viewer);

      const QJsonObject proof{{"checkpoint","live-function-derived-thruster-parameter-binding"},{"outcome","passed"},{"case",key},{"argument",argument},{"route","active-stopped-derived-thruster"},{"unmanaged_factory_unit",factoryUnit},{"executed_begin_end_commands",48},{"propagations",72},{"family",family},{"full_system_parameter_inventory",321},{"initial_batch_types",catalog["initial_types"]},{"missions",3},{"function_calls",12},{"live_callback_samples",24},{"actual_parameter_owner_checks",48},{"caller_root_control_callbacks",12},{"file_decisions",decisions},{"phases",phases},{"scope","Actual main/private/Global Spacecraft Parameter references with separate attached ChemicalThruster/tank/FiniteBurn identities,active/stopped derived values,nonunit duty/scale,constant C1/K1,changing function inputs and cumulative Global fuel use. Exact caller-root state/hardware controls follow main coast and each function;source/history/prototypes/file Cancel/Save/Unicode/no-Build/viewer remain. Units N,s,kg/s;writable flags retained and active native setter outcomes recorded without requiring rejection. Thruster frame string is checked;stored frame pointer is not observed. No arbitrary polynomial/frame/force/integration/time/science or whole-scope qualification. Private references are inspected only synchronously during live callbacks."}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }
   catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
   catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
   return 1;
}
