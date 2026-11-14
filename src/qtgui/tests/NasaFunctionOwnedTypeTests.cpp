#include "FunctionScopeUi.hpp"
#include "FunctionOwnedInputFixtures.hpp"
#include "ReportViewer.hpp"
#include "UserDefinedFunction.hpp"
#include "ObjectManagedFunction.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "Propagate.hpp"
#include "CoordinateSystem.hpp"
#include "AxisSystem.hpp"
#include "CoordinateConverter.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include "Sandbox.hpp"
#include "Debugger.hpp"
#include "ScriptInterpreter.hpp"
#include <QRegularExpression>
using namespace formaccept;
static QString pointer(const void *p){return QString::number(quintptr(p),16);}
static const QStringList fields{"X","Y","Z","VX","VY","VZ"};
static QJsonArray vector(const std::array<double,6>&v){QJsonArray a;for(double x:v)a.append(x);return a;}
static QJsonArray state(Spacecraft *sc){QJsonArray a;for(int i=0;i<6;++i)a.append(sc->GetState()[i]);return a;}
static QJsonArray matrix(const Rmatrix33 &m){QJsonArray a;for(int i=0;i<3;++i)for(int j=0;j<3;++j)a.append(m(i,j));return a;}
static std::array<double,6> circle(double radius,double seconds)
{
   const double n=std::sqrt(400000./(radius*radius*radius)),angle=n*seconds,v=radius*n;return{radius*std::cos(angle),radius*std::sin(angle),0,-v*std::sin(angle),v*std::cos(angle),0};
}
static QString craft(const QString &name,const QString &frame,const std::array<double,6>&v,int id,const QString &family,const QString &kind,const QString &input={})
{
   QString code="Create Spacecraft "+name+";\n"+name+".CoordinateSystem = "+frame+";\n"+name+".DateFormat = "+(family=="Ephemeris"?"UTCModJulian":"A1ModJulian")+";\n"+name+".Epoch = '21545';\n"+name+".DisplayStateType = Cartesian;\n"+name+".NAIFId = "+QString::number(id)+";\n"+name+".NAIFIdReferenceFrame = "+QString::number(id-8000000)+";\n";
   for(int i=0;i<6;++i)code+=name+"."+fields[i]+" = "+QString::number(v[i],'g',17)+";\n";
   if(family=="Ephemeris")
   {
      if(kind=="SPK")code+=name+".OrbitSpiceKernelName = {'"+input+"'};\n";
      else code+=name+".EphemerisName = '"+input+"';\n";
      if(kind=="SPICESGP4")code+=name+".Id = 'ExampleSat';\n"+name+".DateFormat = UTCGregorian;\n"+name+".Epoch = '07 May 2002 00:00:00.000';\n";
   }
   return code;
}
static void compare(const QJsonArray &a,const QJsonArray &e,const QJsonObject &spec)
{
   require(a.size()==e.size()&&a.size()==6,"Owned type state width differs");for(int i=0;i<6;++i)require(std::isfinite(a[i].toDouble())&&std::abs(a[i].toDouble()-e[i].toDouble())<=spec[i<3?"position_bound_km":"velocity_bound_km_s"].toDouble(),"Owned type independent state differs");
}
static PropSetup *executed(GmatCommand *head,const std::string &name)
{
   for(auto *c=head;c;c=c->GetNext())if(c->GetTypeName()=="Propagate"&&c->GetGeneratingString(Gmat::NO_COMMENTS).find("Propagate "+name+"(")==0){auto *p=dynamic_cast<Propagate*>(c);return dynamic_cast<PropSetup*>(p->GetRefObject(Gmat::PROP_SETUP,name,0));}return nullptr;
}
static QJsonObject native(GmatBase *object,const QJsonObject &spec,bool local)
{
   require(object&&object->IsAutomaticGlobal()&&object->IsGlobal(),"Owned type automatic-global resource absent");const auto type=spec["factory_type"].toString();QJsonObject result{{"name",QString::fromStdString(object->GetName())},{"type",QString::fromStdString(object->GetTypeName())},{"pointer",pointer(object)},{"global",object->IsGlobal()},{"automatic",object->IsAutomaticGlobal()},{"local",object->IsLocal()}};
   if(spec["family"]=="Axes")
   {
      auto *frame=dynamic_cast<CoordinateSystem*>(object);require(frame&&frame->GetAxisSystem()&&frame->GetAxisSystem()->GetTypeName()==type.toStdString(),"Owned actual axis implementation differs");result["owned_type"]=type;result["owned_pointer"]=pointer(frame->GetAxisSystem());result["solar_system"]=pointer(frame->GetSolarSystem());result["origin"]=QString::fromStdString(frame->GetOriginName());
      if(QStringList{"MOEEq","MOEEc","TOEEq","TOEEc"}.contains(type))require(frame->GetRealParameter("Epoch")== (local?25301.75:25300.25),"Owned fixed-epoch axes differ");
      if(type=="SPICE")require(frame->GetStringParameter("SpiceFrameId")=="J2000","Owned SPICE frame selector differs");
   }
   else
   {
      auto *setup=dynamic_cast<PropSetup*>(object);auto *method=setup?setup->GetPropagator():nullptr;require(method&&method->GetTypeName()==type.toStdString(),"Owned actual propagator implementation differs");result["owned_type"]=type;result["owned_pointer"]=pointer(method);result["elapsed_seconds"]=method->GetTime();result["uses_ode"]=method->UsesODEModel();result["has_precision_time"]=method->HasPrecisionTime();if(spec["family"]=="Ephemeris")result["start_epoch_source"]=QString::fromStdString(method->GetStringParameter("StartEpoch"));
      if(spec["family"]=="Numerical")
      {
         require(method->UsesODEModel()&&setup->GetODEModel(),"Numerical owned force absent");require(method->GetRealParameter("Accuracy")== (local?1e-12:1e-11)&&method->GetRealParameter("InitialStepSize")== (local?1.:3.)&&method->GetRealParameter("MinStep")==0&&method->GetRealParameter("MaxStep")==5,"Owned numerical settings differ");result["force_seconds"]=setup->GetODEModel()->GetTime();require(result["force_seconds"]==result["elapsed_seconds"],"Owned force/method clocks differ");
      }
      else require(!method->UsesODEModel()&&method->GetRealParameter("StepSize")==5,"Owned ephemeris settings differ");
   }
   return result;
}

class Observer final:public Subscriber
{
public:
   Observer(Rig &r,QJsonObject s,QString t):Subscriber("OwnedTypeObserver","OwnedTypeObserver"),rig(r),spec(s),tag(t){}
   ~Observer()override{Publisher::Instance()->Unsubscribe(this);}
   GmatBase *Clone()const override{return new Observer(*this);}
   bool ReceiveData(const Real *data,Integer count=0)override
   {
      bool initial=false;const auto *objects=Publisher::GetPublishedPropagationObjects(data,count,initial);if(!objects||!initial)return true;bool pulse=false;for(auto *o:*objects)if(o&&(o->GetName()=="MainPulse"||o->GetName()=="ScopePulse"))pulse=true;if(!pulse)return true;
      try
      {
         auto *command=dynamic_cast<GmatCommand*>(currentProvider);auto *function=command?dynamic_cast<UserDefinedFunction*>(command->GetCurrentFunction()):nullptr;require(command,"Owned live provider absent");const bool inside=function;const auto rootGlobals=Moderator::Instance()->GetSandbox()->GetGlobalObjectMap();const auto *locals=inside?function->GetFunctionObjectStore():command->GetObjectMap();const auto *globals=inside?function->GetGlobalObjectStore():&rootGlobals;
         auto lookup=[&](const std::string &name)->GmatBase*{auto a=locals->find(name);if(a!=locals->end())return a->second;auto b=globals->find(name);return b==globals->end()?nullptr:b->second;};auto *calls=lookup("Calls");require(calls,"Owned live counter absent");const int call=int(calls->GetRealParameter("Value"));require(call==records.size()&&inside==(call>0),"Owned live callback order differs");
         auto *shared=lookup("Shared");require(shared==Moderator::Instance()->GetInternalObject("Shared")&&shared!=Moderator::Instance()->GetConfiguredObject("Shared"),"Owned implicit shared identity differs");auto *owner=dynamic_cast<Spacecraft*>(inside?lookup("FunctionSC"):lookup("MainSC"));require(owner&&owner->IsLocal()==inside&&owner!=Moderator::Instance()->GetConfiguredObject(inside?"FunctionSC":"MainSC"),"Owned live spacecraft identity differs");
         auto *resource=inside?lookup("FunctionOwned"):shared;auto resourceInfo=native(resource,spec,inside);
         auto *solar=owner->GetSolarSystem();require(solar&&owner->GetJ2000Body()==solar->GetBody("Earth")&&(!inside||solar==function->GetSolarSystem()),"Owned actual spacecraft SolarSystem/J2000 context differs");resourceInfo["spacecraft_solar_system"]=pointer(solar);resourceInfo["spacecraft_j2000_body"]=pointer(owner->GetJ2000Body());resourceInfo["expected_j2000_body"]=pointer(solar->GetBody("Earth"));
         QJsonArray actual,expected;QJsonObject inputs;
         if(spec["family"]=="Axes")
         {
            auto *frame=dynamic_cast<CoordinateSystem*>(resource);require(owner->GetRefObject(Gmat::COORDINATE_SYSTEM,inside?"FunctionOwned":"Shared")==frame,"Owned actual spacecraft frame reference differs");auto *reference=dynamic_cast<CoordinateSystem*>(lookup(inside?"PrivateOracle":"RootOracle"));require(reference&&reference!=frame&&reference->GetAxisSystem()->GetTypeName()==spec["factory_type"].toString().toStdString(),"Separate shared frame input absent");
            Rvector6 zero(0,0,0,0,0,0);reference->ToBaseSystem(A1Mjd(owner->GetEpoch()),zero,true,true);auto R=reference->GetLastRotationMatrix(),D=reference->GetLastRotationDotMatrix();Rmatrix33 baseRotation;if(reference->GetBaseSystem()=="ICRF"){CoordinateConverter baseConverter;baseRotation=baseConverter.GetRotationMatrixFromICRFToFK5(A1Mjd(owner->GetEpoch()));R=baseRotation*R;D=baseRotation*D;}const auto origin=reference->GetOrigin()->GetMJ2000State(A1Mjd(owner->GetEpoch())),earth=reference->GetSolarSystem()->GetBody("Earth")->GetMJ2000State(A1Mjd(owner->GetEpoch()));
            const std::array<double,6> authored=inside?std::array<double,6>{1250,-250,450,1.5,-.4,.2}:std::array<double,6>{1000,-200,300,1.25,-.5,.75};
            for(int i=0;i<6;++i){double value=origin[i]-earth[i];for(int j=0;j<3;++j){value+=R(i%3,j)*authored[j+(i<3?0:3)];if(i>=3)value+=D(i%3,j)*authored[j];}expected.append(value);}actual=state(owner);inputs={{"reference_pointer",pointer(reference)},{"reference_base",QString::fromStdString(reference->GetBaseSystem())},{"base_rotation",matrix(baseRotation)},{"rotation",matrix(R)},{"rotation_dot",matrix(D)},{"origin_state",vector({origin[0],origin[1],origin[2],origin[3],origin[4],origin[5]})},{"earth_state",vector({earth[0],earth[1],earth[2],earth[3],earth[4],earth[5]})},{"authored_state",vector(authored)}};
         }
         else
         {
            auto *setup=executed(inside?function->GetFunctionControlSequence():Moderator::Instance()->GetFirstCommand(),inside?"FunctionOwned":"Shared");require(setup,"Owned actual Propagate clone absent");const auto run=native(setup,spec,inside);const double elapsed=setup->GetPropagator()->GetTime(),requested=spec[inside?"requested_private_seconds":"requested_root_seconds"].toDouble();require(std::abs(elapsed-requested)<=spec["stop_time_bound_seconds"].toDouble(),"Owned stopping duration exceeds original microsecond bound");resourceInfo["executed"]=run;resourceInfo["owner_has_precision_time"]=owner->HasPrecisionTime();resourceInfo["owner_epoch_real"]=owner->GetEpoch();resourceInfo["owner_epoch_gt"]=QString::fromStdString(owner->GetEpochGT().ToString());resourceInfo["owner_epoch_gt_as_real"]=owner->GetEpochGT().GetMjd();
            if(spec.contains("clock_profile"))
            {
               require(run["has_precision_time"].toBool()&&owner->HasPrecisionTime(),"Supplemental GT propagator/owner precision flags absent");
               require(run["start_epoch_source"]==spec["expected_start_selector"],"Supplemental GT start selector differs");
            }
            if(spec.contains("real_clock_profile"))
            {
               require(!run["has_precision_time"].toBool()&&!owner->HasPrecisionTime(),"Supplemental Real propagator/owner clock flags differ");
               require(run["start_epoch_source"]==spec["expected_start_selector"],"Supplemental Real start selector differs");
            }
            if(spec["family"]=="Numerical"){actual=state(owner);expected=vector(circle(inside?7000:9000,elapsed));}
            else
            {
               const auto frameName=spec["frame_source"].toString();auto *frame=dynamic_cast<CoordinateSystem*>(Moderator::Instance()->GetInternalObject(frameName.toStdString()));auto *base=dynamic_cast<CoordinateSystem*>(Moderator::Instance()->GetInternalObject("EarthMJ2000Eq"));require(frame&&base,"Owned ephemeris output frame absent");Rvector6 raw;for(int i=0;i<6;++i)raw[i]=owner->GetState()[i];CoordinateConverter converter;Rvector6 out;converter.Convert(owner->GetEpochGT(),raw,base,out,frame,true,true);for(int i=0;i<6;++i)actual.append(out[i]);
               if(spec["factory_type"]=="SPICESGP4"){if(!inside)sgp4Reference=actual;expected=sgp4Reference;require(expected.size()==6,"Separate root SGP4 control absent");}
               else
               {
                  auto *time=TimeSystemConverter::Instance();const auto system=spec["factory_type"]=="SPK"?TimeSystemConverter::TDB:TimeSystemConverter::UTC;
                  const auto nowGT=time->Convert(owner->GetEpochGT(),TimeSystemConverter::A1,system),startGT=time->Convert(GmatTime(21545.),TimeSystemConverter::UTC,system);const double secondsGT=(nowGT-startGT).GetTimeInSec();
                  rig.save(tag+"-clock-selected-diagnostic.json",QJsonDocument(QJsonObject{{"calls",call},{"seconds_gt",secondsGT},{"elapsed_seconds",elapsed},{"expected_start_offset_seconds",spec["expected_start_offset_seconds"]},{"resource",resourceInfo}}).toJson());
                  if(spec.contains("clock_profile"))require(std::abs(secondsGT-elapsed-spec["expected_start_offset_seconds"].toDouble())<=1e-9,"Supplemental GT selected start clock differs from authored epoch");
                  if(spec.contains("real_clock_profile"))require(std::abs(secondsGT-elapsed-spec["expected_start_offset_seconds"].toDouble())<=1e-6,"Supplemental Real selected start clock differs from authored epoch");
                  const double now=time->Convert(owner->GetEpoch(),TimeSystemConverter::A1,system),start=time->Convert(21545.,TimeSystemConverter::UTC,system),secondsReal=(now-start)*86400.;
                  // The temporary SPK recipe explicitly authors its grid using
                  // the legacy double TDB conversion. Text inputs use exact UTC.
                  expected=vector(owned_inputs::affine(inside,spec["factory_type"]=="SPK"?secondsReal:secondsGT));
                  inputs={{"file_time_system",system==TimeSystemConverter::TDB?"TDB":"UTC"},{"query_gt",QString::fromStdString(nowGT.ToString())},{"start_gt",QString::fromStdString(startGT.ToString())},{"seconds_gt",secondsGT},{"seconds_real",secondsReal},{"affine_at_requested_stop",vector(owned_inputs::affine(inside,requested))}};
               }
            }
         }
         rig.save(tag+"-latest-diagnostic.json",QJsonDocument(QJsonObject{{"calls",call},{"actual",actual},{"expected",expected},{"resource",resourceInfo},{"oracle_inputs",inputs}}).toJson());compare(actual,expected,spec);records.append(QJsonObject{{"calls",call},{"inside_function",inside},{"owner",QString::fromStdString(owner->GetName())},{"owner_pointer",pointer(owner)},{"shared_pointer",pointer(shared)},{"configured_shared_pointer",pointer(Moderator::Instance()->GetConfiguredObject("Shared"))},{"resource",resourceInfo},{"actual",actual},{"expected",expected},{"oracle_inputs",inputs}});rig.save(tag+"-live-samples.json",QJsonDocument(records).toJson());
      }
      catch(BaseException &e){if(!failure){failure=std::current_exception();error=QString::fromStdString(e.GetFullMessage());}}
      catch(const std::exception &e){if(!failure){failure=std::current_exception();error=e.what();}}
      return true;
   }
   Rig &rig;QJsonObject spec;QString tag,error;QJsonArray records,sgp4Reference;std::exception_ptr failure;
};

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FunctionOwnedTypes");
   try
   {
      require(argc==4&&acceptanceWidgetPlatform(),"Supply owned-type startup,catalog and case");Rig rig(QFileInfo(argv[1]).absoluteFilePath());const auto catalogBytes=read(QFileInfo(argv[2]).absoluteFilePath());rig.save("executed-catalog.json",catalogBytes);const auto catalog=QJsonDocument::fromJson(catalogBytes).object();QJsonObject spec;const QString key=argv[3];for(const auto &x:catalog["cases"].toArray())if(x.toObject()["key"]==key)spec=x.toObject();require(!spec.isEmpty()&&catalog["factory_rows"]==566&&catalog["selected_missing_count"]==32,"Owned full-universe catalog differs");const QString family=spec["family"].toString(),kind=spec["factory_type"].toString();const bool prop=family!="Axes";
      const QString stopOptions=spec.contains("precision_stop_tolerance_seconds")?", StopTolerance = "+QString::number(spec["precision_stop_tolerance_seconds"].toDouble(),'g',17):QString();
      const auto directory=rig.files.filePath("Owned type scope Ω");require(QDir().mkpath(directory),"Owned type fixture directory absent");const auto function=directory+"/ReadShared.gmf",outer=directory+"/NestedShared.gmf",saved=directory+"/Caller.script",report=directory+"/Values.txt";const auto repo=QDir(QFileInfo(argv[2]).absolutePath()+"/../../..").absolutePath();const auto rootInput=family=="Ephemeris"?owned_inputs::make(rig,directory,kind,false,repo,spec["file_clock_shift_seconds"].toDouble(),spec["file_output_endian"].toInt(1)):QString(),privateInput=family=="Ephemeris"?owned_inputs::make(rig,directory,kind,true,repo,spec["file_clock_shift_seconds"].toDouble(),spec["file_output_endian"].toInt(1)):QString();
      const auto mainInitial=family=="Axes"?std::array<double,6>{1000,-200,300,1.25,-.5,.75}:family=="Numerical"?circle(9000,0):owned_inputs::affine(false,0),privateInitial=family=="Axes"?std::array<double,6>{1250,-250,450,1.5,-.4,.2}:family=="Numerical"?circle(7000,0):owned_inputs::affine(true,0);
      const QString globals="Global Calls;\n"+QString(kind=="Topocentric"?"Global ScopeSite;\n":""),outputFrame=spec["frame_source"].toString();auto bridge=[&](const QString &array,const QString &owner){QString code;for(int i=0;i<6;++i)code+=array+"("+QString::number(i+1)+",1) = "+owner+"."+outputFrame+"."+fields[i]+";\n";return code;};
      QString original="function [Mark, State] = ReadShared()\n"+spec["private_configuration"].toString()+craft("FunctionSC",prop?"EarthMJ2000Eq":"FunctionOwned",privateInitial,-132102,family,kind,privateInput)+spec["supplemental_private_spacecraft_configuration"].toString()+craft("ScopePulse","EarthMJ2000Eq",circle(8000,0),-132104,"Numerical",kind)+"Create Variable Mark PrivateOnly;\nCreate Array State[6,1];\nPrivateOnly = 103;\nBeginMissionSequence;\n"+globals+"Calls = Calls + 1;\nWrite Shared FunctionOwned {Style = Script, MessageWindow = true, LogFile = false};\n";
      if(prop)original+="Propagate FunctionOwned(FunctionSC) {FunctionSC.ElapsedSecs = "+QString::number(spec["requested_private_seconds"].toInt())+stopOptions+"};\n";original+=bridge("State","FunctionSC")+"Propagate PulseProp(ScopePulse) {ScopePulse.ElapsedSecs = 1};\nMark = PrivateOnly + Calls;\n% Complete owned type scoping Ω\n";const auto edited=QString(original).replace("PrivateOnly = 103;","PrivateOnly = 41;");
      const QString outerBody="function [Mark, State] = NestedShared()\nCreate Variable Mark;\nCreate Array State[6,1];\nCreate GmatFunction ReadShared;\nReadShared.FunctionPath = '"+function+"';\nBeginMissionSequence;\n[Mark, State] = ReadShared();\n";write(function,original.toUtf8());write(outer,outerBody.toUtf8());rig.save("initial.gmf",original.toUtf8());rig.save("nested.gmf",outerBody.toUtf8());
      QString caller="Earth.Mu = 400000;\n"+spec["common_configuration"].toString()+spec["reference_configuration"].toString()+spec["root_configuration"].toString()+craft("MainSC",prop?"EarthMJ2000Eq":"Shared",mainInitial,-132101,family,kind,rootInput)+spec["supplemental_root_spacecraft_configuration"].toString()+craft("PrivateOnly","EarthMJ2000Eq",{7000,100,200,-.1,7.5,.8},-132103,"Numerical",kind)+craft("MainPulse","EarthMJ2000Eq",circle(8500,0),-132105,"Numerical",kind);
      caller+="Create ForceModel PulseFM;\nPulseFM.PrimaryBodies = {};\nPulseFM.PointMasses = {Earth};\nPulseFM.Drag = None;\nPulseFM.SRP = Off;\nCreate Propagator PulseProp;\nPulseProp.FM = PulseFM;\nPulseProp.InitialStepSize = 1;\nPulseProp.MinStep = 0;\nPulseProp.MaxStep = 1;\nCreate Variable First Second Third Keep Calls;\nCreate Array MainState[6,1] FirstState[6,1] SecondState[6,1] ThirdState[6,1];\nKeep = 103;\nCalls = 0;\nCreate GmatFunction ReadShared NestedShared;\nReadShared.FunctionPath = '"+function+"';\nNestedShared.FunctionPath = '"+outer+"';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\nBeginMissionSequence;\n"+globals+"Write 'Arm owned types' Keep {Style = Verbose, MessageWindow = true, LogFile = false};\n";
      if(prop)caller+="Propagate Shared(MainSC) {MainSC.ElapsedSecs = "+QString::number(spec["requested_root_seconds"].toInt())+stopOptions+"};\n";caller+=bridge("MainState","MainSC")+"Propagate PulseProp(MainPulse) {MainPulse.ElapsedSecs = 1};\nReport Values";for(int i=1;i<=6;++i)caller+=" MainState("+QString::number(i)+",1)";caller+=";\n";
      for(const auto &word:QStringList{"First","Second","Third"}){caller+="["+word+", "+word+"State] = "+(word=="Third"?"NestedShared":"ReadShared")+"();\nReport Values "+word;for(int i=1;i<=6;++i)caller+=" "+word+"State("+QString::number(i)+",1)";caller+=";\n";}caller+="Report Values Keep;\nKeep = Keep + 1;\nReport Values Keep;\n";
      rig.source->setPlainText(caller);rig.save("caller.script",caller.toUtf8());const bool built=rig.window.buildScript();rig.save("build-messages.txt",rig.messages->toPlainText().toUtf8());if(!built)for(const auto &e:Moderator::GetScriptInterpreter()->GetErrorList())std::cerr<<e<<std::endl;require(built,"Owned type caller Build failed");
      auto prototypes=[&]{QJsonObject values;for(const auto &name:QStringList{"Shared","MainSC","PrivateOnly","Calls","Keep"}){auto *p=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(p,"Owned configured prototype absent");values[name]=QString::fromStdString(p->GetGeneratingString(Gmat::NO_COMMENTS));}return values;};auto configuredState=[&]{auto *sc=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetConfiguredObject("MainSC"));auto *frame=dynamic_cast<CoordinateSystem*>(Moderator::Instance()->GetConfiguredObject("Shared"));QJsonObject data{{"state",state(sc)},{"epoch_gt",QString::fromStdString(sc->GetEpochGT().ToString())},{"epoch_real",sc->GetEpoch()},{"has_precision_time",sc->HasPrecisionTime()}};if(family=="Axes"){data["rotation"]=matrix(frame->GetLastRotationMatrix());data["rotation_dot"]=matrix(frame->GetLastRotationDotMatrix());const auto origin=frame->GetOrigin()->GetMJ2000State(A1Mjd(sc->GetEpoch()));data["origin_state"]=vector({origin[0],origin[1],origin[2],origin[3],origin[4],origin[5]});}return data;};const auto before=prototypes();rig.save("configured-state-before.json",QJsonDocument(configuredState()).toJson());rig.save("prototypes-before.json",QJsonDocument(before).toJson());const auto decisions=function_scope_ui::editFunction(rig,function,original,edited,caller,prototypes);rig.save("edited.gmf",edited.toUtf8());require(rig.window.saveScriptTo(saved),"Owned Unicode caller Save failed");QJsonArray phases;
      for(int phase=0;phase<3;++phase)
      {
         if(phase==1){const bool loaded=rig.window.loadScript(saved),rebuilt=loaded&&rig.window.buildScript();rig.save("reopened-prototypes.json",QJsonDocument(prototypes()).toJson());rig.save("configured-state-reopened.json",QJsonDocument(configuredState()).toJson());rig.save("reopened-diagnostic.json",QJsonDocument(QJsonObject{{"loaded",loaded},{"rebuilt",rebuilt},{"source_unchanged",rig.source->toPlainText()==caller},{"prototypes_unchanged",prototypes()==before}}).toJson());require(loaded&&rebuilt&&rig.source->toPlainText()==caller&&prototypes()==before,"Owned Unicode reopen changed source/prototypes");}const auto tag=QString::number(phase);Observer observer(rig,spec,tag);Debugger *debug=nullptr;for(auto *child:rig.window.children())if(auto *candidate=dynamic_cast<Debugger*>(child))debug=candidate;int arm=-1;const auto snapshot=rig.window.missionSnapshot();for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].statement.contains("Arm owned types"))arm=i;require(debug&&arm>=0&&rig.window.setBreakpoint(arm,true),"Owned observer arm absent");bool armed=false;std::exception_ptr armError;auto previous=debug->paused;
         debug->paused=[&](const QString &kind){if(previous)previous(kind);try{require(!armed&&kind=="Write","Owned arm boundary differs");auto *main=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("MainSC"));rig.save(tag+"-arm-clock-diagnostic.json",QJsonDocument(QJsonObject{{"has_precision_time",main->HasPrecisionTime()},{"epoch_real",main->GetEpoch()},{"epoch_gt",QString::fromStdString(main->GetEpochGT().ToString())},{"state",state(main)}}).toJson());observer.SetInternalCoordSystem(Publisher::Instance()->GetInternalCoordSystem());if(auto *frame=Publisher::Instance()->GetDataCoordSystem())observer.SetDataCoordSystem(frame);require(Publisher::Instance()->Subscribe(&observer),"Owned observer subscription failed");armed=true;}catch(...){armError=std::current_exception();rig.window.stopMission();}QTimer::singleShot(0,&rig.window,[&]{debug->resume();});};
         const int undo=rig.source->document()->availableUndoSteps();rig.messages->clear();acceptanceView(rig.window);const auto outcome=rig.window.runMission();debug->paused=previous;Publisher::Instance()->Unsubscribe(&observer);events();rig.save(tag+"-messages.txt",rig.messages->toPlainText().toUtf8());rig.save(tag+"-observer-error.txt",observer.error.toUtf8());if(armError)std::rethrow_exception(armError);if(observer.failure)std::rethrow_exception(observer.failure);if(outcome!=MainWindow::RunResult::Completed)std::cerr<<rig.messages->toPlainText().toStdString();require(outcome==MainWindow::RunResult::Completed&&armed&&observer.records.size()==4,"Owned mission/live callback census differs");
         const auto raw=read(report);rig.save(tag+"-values.txt",raw);const auto words=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==29,"Owned raw report width differs");QJsonArray values,expected;for(const auto &word:words){bool ok;const auto v=word.toDouble(&ok);require(ok&&std::isfinite(v),"Owned raw report contains invalid number");values.append(v);}for(int call=0;call<4;++call){if(call)expected.append(41+call);for(const auto &v:observer.records[call].toObject()["expected"].toArray())expected.append(v);}expected.append(103);expected.append(104);require(expected.size()==29,"Owned expected width differs");for(int i=0;i<29;++i){const bool marker=i==6||i==13||i==20||i>=27;const int column=i<6?i:(i-7)%7;require(std::abs(values[i].toDouble()-expected[i].toDouble())<= (marker?0:spec[column<3?"position_bound_km":"velocity_bound_km_s"].toDouble()),"Owned actual Report differs from live independent expectation");}
         auto *anchor=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("PrivateOnly"));require(anchor&&anchor->GetEpoch()==21545&&state(anchor)==QJsonArray{7000,100,200,-.1,7.5,.8},"Owned function changed caller shadow");const auto after=prototypes();QJsonObject invariants{{"source",rig.source->toPlainText()==caller},{"history",rig.source->document()->availableUndoSteps()==undo},{"prototypes",after==before},{"function_file",read(function)==edited.toUtf8()},{"nested_file",read(outer)==outerBody.toUtf8()},{"not_running",!rig.window.isRunning()}};for(const auto &v:invariants)require(v.toBool(),"Owned execution isolation failed");rig.save(tag+"-invariants.json",QJsonDocument(invariants).toJson());phases.append(QJsonObject{{"phase",phase},{"explicit_rebuild",phase<2},{"live_samples",observer.records},{"raw_values",values},{"expected_values",expected}});rig.save("phases.json",QJsonDocument(phases).toJson());
      }
      auto *tree=rig.window.findChild<QTreeWidget*>("Output");const auto outputs=tree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(outputs.size()==1,"Owned report output absent");tree->itemDoubleClicked(outputs[0],0);events();auto *viewer=rig.area->activeSubWindow();require(viewer&&dynamic_cast<ReportViewer*>(viewer->widget()),"Owned actual report viewer absent");auto *text=viewer->findChild<QPlainTextEdit*>();require(text&&text->isReadOnly()&&text->toPlainText().trimmed()==QString::fromUtf8(read(report)).trimmed(),"Owned report viewer bytes differ");acceptanceView(*viewer);
      QJsonObject proof{{"checkpoint","complete-owned-type-function-scoping"},{"outcome","passed"},{"case",key},{"family",family},{"factory_rows",566},{"selected_missing_types",32},{"missions",3},{"function_calls",12},{"live_callbacks",12},{"file_decisions",decisions},{"phases",phases},{"whole_row_qualified",false},{"new_requirement_credit",0},{"independent_orientation_provider_truth",false}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }
   catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
   catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
   return 1;
}
