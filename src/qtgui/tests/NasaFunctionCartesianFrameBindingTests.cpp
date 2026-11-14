#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "ObjectManagedFunction.hpp"
#include "Parameter.hpp"
#include "ParameterInfo.hpp"
#include "CoordinateSystem.hpp"
#include "AxisSystem.hpp"
#include "ODEModel.hpp"
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
static QJsonValue expectedValue(const QJsonObject &spec,bool shared,bool inside,int calls){return spec["observations"].toObject()[shared?"shared":inside?"private":"root"].toArray()[calls].toObject()["expected"];}
static double bound(const QString &,const QString &) { return 1e-9; }
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
static QJsonArray stateJson(Spacecraft *sc){QJsonArray a;for(int i=0;i<6;++i)a.append(sc->GetState()[i]);return a;}
static QJsonArray flat(const QJsonArray &rows){QJsonArray a;for(const auto &row:rows)for(const auto &v:row.toArray())a.append(v);return a;}
static QJsonArray matrixJson(const Rmatrix33 &m){QJsonArray a;for(int i=0;i<3;++i)for(int j=0;j<3;++j)a.append(m(i,j));return a;}
static QJsonArray vectorJson(const Rvector &v){QJsonArray a;for(int i=0;i<v.GetSize();++i)a.append(v[i]);return a;}
static QJsonObject observation(const QJsonObject &spec,bool shared,bool inside,int calls){return spec["observations"].toObject()[shared?"shared":inside?"private":"root"].toArray()[calls].toObject();}
class BindingObserver final:public Subscriber
{
public:
   BindingObserver(Rig &rig,const QJsonObject &spec,const QString &prefix):Subscriber("CartesianFrameObserver","CartesianFrameObserver"),rig(&rig),spec(spec),prefix(prefix){}
   ~BindingObserver() override{Publisher::Instance()->Unsubscribe(this);}
   GmatBase *Clone() const override{return new BindingObserver(*this);}
   bool ReceiveData(const Real *data,Integer count=0) override
   {
      bool initial=false;if(!Publisher::GetPublishedPropagationObjects(data,count,initial)||!initial)return true;
      try
      {
         auto *command=dynamic_cast<GmatCommand*>(currentProvider);require(command,"Cartesian frame provider is not a command");auto *function=dynamic_cast<ObjectManagedFunction*>(command->GetCurrentFunction());const bool inside=function;
         const auto *locals=inside?function->GetFunctionObjectStore():command->GetObjectMap();const auto rootGlobals=inside?ObjectMap{}:Moderator::Instance()->GetSandbox()->GetGlobalObjectMap();const auto *globals=inside?function->GetGlobalObjectStore():&rootGlobals;require(locals&&globals,"Cartesian live maps absent");
         auto lookup=[&](const std::string &name){auto a=locals->find(name);if(a!=locals->end())return a->second;auto b=globals->find(name);return b==globals->end()?nullptr:b->second;};auto *counter=lookup("Calls");require(counter,"Cartesian counter absent");const int calls=int(counter->GetRealParameter("Value"));
         events.append(QJsonObject{{"calls",calls},{"inside_function",inside},{"function",inside?QString::fromStdString(function->GetName()):QString()},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))}});rig->save(prefix+"-callback-events.json",QJsonDocument(events).toJson());require(calls==events.size()-1&&inside==(calls>0)&&(!inside||function->GetName()=="ReadShared"),"Cartesian callback order/scope differs");
         const auto type=spec["factory_type"].toString(),mode=spec["mode"].toString();const auto meta=spec["metadata"].toObject();QJsonArray rows;
         for(bool shared:{false,true})
         {
            const QString ownerName=shared?"SharedSC":"PrivateOnly",full=reference(spec,shared,inside),frameName=mode=="Bare"?"EarthMJ2000Eq":shared?"SharedFrame":"ScopeFrame";auto *owner=dynamic_cast<Spacecraft*>(lookup(ownerName.toStdString()));auto *root=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject(ownerName.toStdString()));auto *configured=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetConfiguredObject(ownerName.toStdString()));require(owner&&root&&configured&&owner!=configured&&owner->IsOfType(meta["owner_type_id"].toInt())&&owner->IsGlobal()==shared&&owner->IsLocal()==(inside&&!shared)&&(owner==root)==(shared||!inside),"Cartesian actual SpacePoint/Spacecraft owner differs");
            auto *parameter=dynamic_cast<Parameter*>(lookup(full.toStdString()));require(parameter&&parameter->IsSystemParameter()&&parameter->GetName()==full.toStdString()&&parameter->GetTypeName()==meta["native_type"].toString().toStdString()&&int(parameter->GetReturnType())==meta["return_type"].toInt()&&int(parameter->GetOwnerType())==meta["owner_type_id"].toInt()&&parameter->GetRefObject(parameter->GetOwnerType(),ownerName.toStdString())==owner,"Cartesian native Parameter owner/reference/type differs");
            auto *frame=dynamic_cast<CoordinateSystem*>(parameter->GetRefObject(Gmat::COORDINATE_SYSTEM,frameName.toStdString()));auto *wanted=dynamic_cast<CoordinateSystem*>(mode=="Bare"?Moderator::Instance()->GetInternalObject("EarthMJ2000Eq"):lookup(frameName.toStdString()));auto *rootFrame=dynamic_cast<CoordinateSystem*>(Moderator::Instance()->GetInternalObject(frameName.toStdString()));require(frame&&wanted&&rootFrame&&frame==wanted&&(frame==rootFrame)==(mode=="Bare"||shared||!inside),"Actual Parameter coordinate-frame reference aliases wrong scope");
            auto *solar=frame->GetSolarSystem();require(solar&&frame->GetOrigin()==solar->GetBody("Earth")&&(!inside||function->GetSolarSystem()==solar),"Cartesian frame origin/live SolarSystem differs");const auto expected=observation(spec,shared,inside,calls);
            QJsonValue actual;QJsonArray shape;if(type=="Cartesian"){const auto &v=parameter->EvaluateRvector();actual=vectorJson(v);shape=QJsonArray{v.GetSize(),1};require(shape==QJsonArray{6,1},"Cartesian Rvector width differs");}else actual=parameter->EvaluateReal();
            const auto inertial=stateJson(owner);compareValue(inertial,expected["after_script_inertial"],1e-9);compareValue(actual,expected["expected"],1e-9);require(owner->GetEpoch()==21545,"Cartesian owner epoch changed");
            Rvector6 state;for(int i=0;i<6;++i)state[i]=owner->GetState()[i];const auto direct=vectorJson(frame->FromBaseSystem(A1Mjd(owner->GetEpoch()),state,true,true));const auto rotation=matrixJson(frame->GetLastRotationMatrix()),rotationDot=matrixJson(frame->GetLastRotationDotMatrix());compareValue(direct,expected["after_script_framed"],1e-9);compareValue(rotation,flat(expected["frame_to_base"].toArray()),1e-12);compareValue(rotationDot,flat(expected["frame_to_base_dot"].toArray()),1e-12);
            QJsonObject graph;
            if(mode=="Frame")
            {
               const QString primaryName=shared?"SharedPrimary":"ScopePrimary",guideName=shared?"SharedGuide":"ScopeGuide";auto *primary=dynamic_cast<Spacecraft*>(lookup(primaryName.toStdString()));auto *guide=dynamic_cast<Spacecraft*>(lookup(guideName.toStdString()));require(primary&&guide&&frame->GetPrimaryObject()==primary&&frame->GetSecondaryObject()==guide&&primary->IsGlobal()==shared&&guide->IsGlobal()==shared&&primary->IsLocal()==(inside&&!shared)&&guide->IsLocal()==(inside&&!shared)&&frame->GetAxisSystem()->GetTypeName()=="ObjectReferenced","Actual frame primary/secondary graph differs");compareValue(stateJson(primary),expected["primary_state"],0);compareValue(stateJson(guide),expected["guide_state"],0);const auto relative=vectorJson(guide->GetMJ2000State(A1Mjd(21545))-primary->GetMJ2000State(A1Mjd(21545)));compareValue(relative,expected["relative_guide_state"],1e-9);Rvector3 acceleration;const bool bound=ODEModel::GetBoundRelativeAcceleration(guide,primary,A1Mjd(21545),acceleration);require(!bound,"Unpropagated frame guides unexpectedly have a bound force provider");const auto a=guide->GetMJ2000Acceleration(A1Mjd(21545))-primary->GetMJ2000Acceleration(A1Mjd(21545));QJsonArray relativeAcceleration;for(int i=0;i<3;++i)relativeAcceleration.append(a[i]);compareValue(relativeAcceleration,QJsonArray{0,0,0},0);
               graph=QJsonObject{{"primary_name",primaryName},{"guide_name",guideName},{"primary_pointer",pointer(primary)},{"guide_pointer",pointer(guide)},{"frame_primary_pointer",pointer(frame->GetPrimaryObject())},{"frame_secondary_pointer",pointer(frame->GetSecondaryObject())},{"primary_state",stateJson(primary)},{"guide_state",stateJson(guide)},{"relative_guide_state",relative},{"relative_acceleration",relativeAcceleration},{"bound_force_provider",bound},{"primary_global",primary->IsGlobal()},{"guide_global",guide->IsGlobal()},{"primary_local",primary->IsLocal()},{"guide_local",guide->IsLocal()},{"axes","ObjectReferenced"},{"X","R"},{"Y",""},{"Z","N"}};
            }
            require(parameter->IsSettable()==meta["settable"].toBool()&&ParameterInfo::Instance()->IsSettable(type.toStdString())==meta["settable"].toBool(),"Original Cartesian writable metadata differs");QJsonObject mutation;
            if(type!="Cartesian")
            {
               const double target=actual.toDouble()+expected["native_delta"].toDouble();parameter->SetReal(target);const double changed=parameter->EvaluateReal();const auto changedState=stateJson(owner);compareValue(changed,target,1e-9);compareValue(changedState,expected["changed_inertial"],1e-9);parameter->SetReal(actual.toDouble());const double restored=parameter->EvaluateReal();const auto restoredState=stateJson(owner);compareValue(restored,actual,1e-9);compareValue(restoredState,inertial,1e-9);require(parameter->GetRefObject(parameter->GetOwnerType(),ownerName.toStdString())==owner&&parameter->GetRefObject(Gmat::COORDINATE_SYSTEM,frameName.toStdString())==frame,"Cartesian native setter changed owner/frame binding");mutation=QJsonObject{{"delta",expected["native_delta"]},{"target",target},{"changed_value",changed},{"changed_inertial",changedState},{"restored_value",restored},{"restored_inertial",restoredState},{"owner_and_frame_preserved",true}};
            }
            rows.append(QJsonObject{{"shared",shared},{"reference",full},{"factory_type",type},{"native_type",QString::fromStdString(parameter->GetTypeName())},{"return_type",int(parameter->GetReturnType())},{"owner_type",int(parameter->GetOwnerType())},{"parameter_pointer",pointer(parameter)},{"reference_owner_pointer",pointer(parameter->GetRefObject(parameter->GetOwnerType(),ownerName.toStdString()))},{"live_owner_pointer",pointer(owner)},{"root_owner_pointer",pointer(root)},{"configured_owner_pointer",pointer(configured)},{"owner_name",ownerName},{"owner_type_name",QString::fromStdString(owner->GetTypeName())},{"owner_global",owner->IsGlobal()},{"owner_local",owner->IsLocal()},{"parameter_global",parameter->IsGlobal()},{"parameter_local",parameter->IsLocal()},{"shape",shape},{"actual",actual},{"expected",expected["expected"]},{"absolute_bound",1e-9},{"inertial_state",inertial},{"direct_frame_state",direct},{"owner_epoch",owner->GetEpoch()},{"frame_name",frameName},{"frame_pointer",pointer(frame)},{"expected_frame_pointer",pointer(wanted)},{"root_frame_pointer",pointer(rootFrame)},{"origin_pointer",pointer(frame->GetOrigin())},{"expected_origin_pointer",pointer(solar->GetBody("Earth"))},{"solar_system_pointer",pointer(solar)},{"function_solar_system_pointer",inside?pointer(function->GetSolarSystem()):QString("0")},{"frame_to_base_rotation",rotation},{"frame_to_base_rotation_dot",rotationDot},{"graph",graph},{"native_mutation",mutation},{"runtime_settable",parameter->IsSettable()},{"info_settable",meta["settable"]},{"legacy_unit",QString::fromStdString(parameter->GetStringParameter("Unit"))}});
         }
         records.append(QJsonObject{{"calls",calls},{"inside_function",inside},{"function",inside?QString::fromStdString(function->GetName()):QString()},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))},{"global_map_is_value_snapshot",!inside},{"parameters",rows}});rig->save(prefix+"-live-parameter-samples.json",QJsonDocument(records).toJson());
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
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FunctionCartesianFrameBinding");
   try
   {
      require(argc==4&&acceptanceWidgetPlatform(),"Supply parameter-binding startup,catalog and type");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();const QString argument=argv[3];QJsonObject spec;QStringList inventory;
      for(const auto &v:catalog["cases"].toArray())inventory.append(v.toObject()["factory_type"].toString());for(const auto &v:catalog["cartesian_binding_cases"].toArray())if(v.toObject()["argument"]==argument)spec=v.toObject();const QString key=spec["factory_type"].toString();
      require(catalog["factory_rows"]==566&&inventory.size()==321&&!spec.isEmpty()&&catalog["initial_types"].toArray().contains(key)&&spec["family"]=="CartesianFrame","Full parameter inventory or selected owner fixture differs");
      QStringList actual;for(const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::PARAMETER))actual.append(QString::fromStdString(name));inventory+=QStringList{"Array","String","Variable"};inventory.sort();actual.sort();require(actual==inventory,"Advertised complete ParameterFactory universe changed");
      const auto family=spec["family"].toString(),type=spec["factory_type"].toString();std::unique_ptr<Parameter> prototype(FactoryManager::Instance()->CreateParameter(type.toStdString()));
      require(prototype&&prototype->IsSystemParameter()&&prototype->GetTypeName()==spec["metadata"].toObject()["native_type"].toString().toStdString(),"Selected unmanaged native factory control differs");
      const auto directory=rig.files.filePath("Live parameter owner Ω");require(QDir().mkpath(directory),"Binding Unicode directory absent");
      const auto function=directory+"/ReadShared.gmf",outer=directory+"/NestedShared.gmf",saved=directory+"/Caller.script",report=directory+"/Values.txt";
      auto literal=[](double v){return QString::number(v,'g',17);};
      auto stateCode=[&](const QString &name,const QJsonArray &values){QString code;const QStringList fields{"X","Y","Z","VX","VY","VZ"};for(int i=0;i<6;++i)code+=name+"."+fields[i]+" = "+literal(values[i].toDouble())+";\n";return code;};
      auto base=[&](const QString &scope){return spec["base_states"].toObject()[scope].toArray();};
      auto frameSetup=[&](bool shared,bool inside){const QString scope=shared?"shared":inside?"private":"root",frame=shared?"SharedFrame":"ScopeFrame",primary=shared?"SharedPrimary":"ScopePrimary",guide=shared?"SharedGuide":"ScopeGuide";const auto rc=observation(spec,shared,inside,0);const int id=shared?-126020:inside?-126120:-126010;QString code=spacecraft(primary,131072,101,id)+stateCode(primary,rc["primary_state"].toArray())+spacecraft(guide,131072,102,id-1)+stateCode(guide,rc["guide_state"].toArray());return code+"Create CoordinateSystem "+frame+";\n"+frame+".Origin = Earth;\n"+frame+".Axes = ObjectReferenced;\n"+frame+".Primary = "+primary+";\n"+frame+".Secondary = "+guide+";\n"+frame+".XAxis = R;\n"+frame+".YAxis = '';\n"+frame+".ZAxis = N;\n";};
      QString declaration=spacecraft("PrivateOnly",7500,275,-126101)+stateCode("PrivateOnly",base("private"))+spacecraft("ScopePulse",9000,701,-126102)+frameSetup(false,true);
      QString mutate;const QStringList fields{"X","Y","Z","VX","VY","VZ"};for(bool shared:{false,true}){const QString name=shared?"SharedSC":"PrivateOnly",scope=shared?"shared":"private";const auto values=base(scope),steps=spec["state_steps"].toObject()[scope].toArray();for(int i=0;i<6;++i)mutate+=name+"."+fields[i]+" = "+literal(values[i].toDouble())+" + Calls * "+literal(steps[i].toDouble())+";\n";if(type!="Cartesian")mutate+=reference(spec,shared,true)+" = "+reference(spec,shared,true)+" + "+literal(observation(spec,shared,true,1)["script_delta"].toDouble())+";\n";}
      const bool vectorBridge=type=="Cartesian";const QString vectorDeclarations=vectorBridge?"Create Array PrivateVector[6,1] SharedVector[6,1];\n":QString();
      auto actualReport=[&](bool inside){if(!vectorBridge)return "Report BindingValues "+reference(spec,false,inside)+" "+reference(spec,true,inside)+";\n";QString code="PrivateVector = "+reference(spec,false,inside)+";\nSharedVector = "+reference(spec,true,inside)+";\nReport BindingValues";for(const auto &name:QStringList{"PrivateVector","SharedVector"})for(int i=1;i<=6;++i)code+=" "+name+"("+QString::number(i)+",1)";return code+";\n";};
      const QString global="Global SharedSC SharedFrame SharedPrimary SharedGuide Calls BindingValues;\n";
      const QString original="function [Mark] = ReadShared()\n"+declaration+vectorDeclarations+"Create Variable Mark Keep ScopeEnd;\nKeep = 103;\nScopeEnd = 12345;\nBeginMissionSequence;\n"+global+"Calls = Calls + 1;\n"+mutate+"Write "+reference(spec,false,true)+" "+reference(spec,true,true)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\n"+actualReport(true)+"Propagate PulseProp(ScopePulse) {ScopePulse.ElapsedSecs = 1};\nMark = Keep + Calls;\n% Live Cartesian frame binding Ω\n";
      const auto edited=QString(original).replace("Keep = 103;","Keep = 41;");
      const QString outerBody="function [Mark] = NestedShared()\nCreate Variable Mark;\nCreate GmatFunction ReadShared;\nReadShared.FunctionPath = '"+function+"';\nBeginMissionSequence;\n[Mark] = ReadShared();\n";
      write(function,original.toUtf8());write(outer,outerBody.toUtf8());rig.save("initial.gmf",original.toUtf8());rig.save("nested.gmf",outerBody.toUtf8());
      QString caller=spacecraft("PrivateOnly",7000,987,-126001)+stateCode("PrivateOnly",base("root"))+spacecraft("SharedSC",8000,150,-126002)+stateCode("SharedSC",base("shared"))+spacecraft("MainPulse",9000,702,-126003)+frameSetup(false,false)+frameSetup(true,false);
      caller+=vectorDeclarations+"Create ForceModel PulseFM;\nPulseFM.PrimaryBodies = {};\nPulseFM.PointMasses = {Earth};\nPulseFM.Drag = None;\nPulseFM.SRP = Off;\nCreate Propagator PulseProp;\nPulseProp.FM = PulseFM;\nPulseProp.Type = RungeKutta89;\nPulseProp.InitialStepSize = 1;\nPulseProp.MinStep = 0;\nPulseProp.MaxStep = 1;\nCreate Variable First Second Third Keep Calls ScopeEnd;\nKeep = 103;\nCalls = 0;\nScopeEnd = 24680;\nCreate GmatFunction ReadShared NestedShared;\nReadShared.FunctionPath = '"+function+"';\nNestedShared.FunctionPath = '"+outer+"';\nCreate ReportFile BindingValues;\nBindingValues.Filename = '"+directory+"/Bindings.txt';\nBindingValues.WriteHeaders = false;\nBindingValues.FixedWidth = false;\nBindingValues.Precision = 17;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\nBeginMissionSequence;\n"+global+"Write 'Arm bindings' Keep {Style = Verbose, MessageWindow = true, LogFile = false};\nWrite "+reference(spec,false,false)+" "+reference(spec,true,false)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\n"+actualReport(false)+"Propagate PulseProp(MainPulse) {MainPulse.ElapsedSecs = 1};\n[First] = ReadShared();\n[Second] = ReadShared();\n[Third] = NestedShared();\nReport Values First Second Third Keep;\nKeep = Keep + 1;\nReport Values Keep;\n";
      rig.source->setPlainText(caller);rig.save("caller.script",caller.toUtf8());const bool built=rig.window.buildScript();rig.save("build-messages.txt",rig.messages->toPlainText().toUtf8());if(!built){std::cerr<<rig.messages->toPlainText().toStdString();QJsonArray errors;for(const auto &e:Moderator::GetScriptInterpreter()->GetErrorList()){errors.append(QString::fromStdString(e));std::cerr<<e<<std::endl;}rig.save("build-errors.json",QJsonDocument(errors).toJson());}require(built,"Binding main parameter caller Build failed");
      auto prototypes=[&]{QJsonObject values;for(const auto &name:QStringList{"PrivateOnly","SharedSC","Calls","Keep","ScopeEnd","ScopeFrame","SharedFrame","ScopePrimary","ScopeGuide","SharedPrimary","SharedGuide"}){auto *p=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(p,"Binding prototype absent");values[name]=QString::fromStdString(p->GetGeneratingString(Gmat::NO_COMMENTS));}return values;};const auto before=prototypes();rig.save("prototypes-before.json",QJsonDocument(before).toJson());
      for (auto *tabs:rig.window.findChildren<QTabWidget*>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==rig.resources || tabs->widget(i)->isAncestorOf(rig.resources)) tabs->setCurrentIndex(i);
      const auto entries=rig.resources->findItems("ReadShared",Qt::MatchExactly|Qt::MatchRecursive); require(entries.size()==1,"Binding function resource absent"); rig.resources->scrollToItem(entries[0]); rig.resources->itemDoubleClicked(entries[0],0); events();
      auto *owner=rig.area->activeSubWindow(); auto *panel=owner?dynamic_cast<ResourceEditor*>(owner->widget()):nullptr; require(panel,"Binding function editor absent"); owner->showNormal(); owner->move(0,0); owner->resize(rig.area->viewport()->size()); events(); auto *edit=panel->findChild<QPushButton*>("editFunctionFile"); reveal(edit);
      const int history=rig.source->document()->availableUndoSteps(); QJsonArray decisions;
      for (bool commit:{false,true})
      {
         bool visited=false; std::exception_ptr failure;
         QTimer modal;modal.setInterval(20);QObject::connect(&modal,&QTimer::timeout,panel,[&]{auto *dialog=panel->findChild<QDialog*>("functionFileDialog");if(!dialog)return;modal.stop();try { require(dialog,"Binding file dialog absent"); auto *text=dialog->findChild<QPlainTextEdit*>("functionFileText"); require(text && !text->isReadOnly() && text->toPlainText()==original,"Binding initial file differs"); reveal(text); QTest::mouseClick(text->viewport(),Qt::LeftButton); QTest::keyClick(text,Qt::Key_A,Qt::ControlModifier); QApplication::clipboard()->setText(edited); QTest::keyClick(text,Qt::Key_V,Qt::ControlModifier); events(); require(text->toPlainText()==edited && read(function)==original.toUtf8() && rig.source->toPlainText()==caller && rig.source->document()->availableUndoSteps()==history && prototypes()==before,"Binding draft changed file/caller/history/prototypes"); auto *button=dialog->findChild<QDialogButtonBox*>()->button(commit?QDialogButtonBox::Save:QDialogButtonBox::Cancel); reveal(button); QTest::mouseClick(button,Qt::LeftButton); visited=true; } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }});
         reveal(edit);modal.start();QTest::mouseClick(edit,Qt::LeftButton);modal.stop(); if (failure) std::rethrow_exception(failure);
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
         if(outcome!=MainWindow::RunResult::Completed)std::cerr<<messages.toStdString();require(outcome==MainWindow::RunResult::Completed&&armed&&observer.records.size()==4,"Binding scope mission/callback census failed");
         QJsonArray privateWrites,sharedWrites;
         auto parse=[&](const QString &ref,const QJsonValue &expected){QJsonArray values;const QString delimiters="(?:"+QRegularExpression::escape(reference(spec,false,false))+"|"+QRegularExpression::escape(reference(spec,false,true))+"|"+QRegularExpression::escape(reference(spec,true,true))+"|ScopeEnd)";auto matches=QRegularExpression("(?:^|\\n)"+QRegularExpression::escape(ref)+" =\\s*\\n([\\s\\S]*?)(?=\\n"+delimiters+" =)").globalMatch(messages);while(matches.hasNext()){auto raw=matches.next().captured(1).trimmed();if(expected.isString()){if(raw.startsWith('\'')&&raw.endsWith('\''))raw=raw.mid(1,raw.size()-2);values.append(raw);}else{for(const auto &c:QStringList{"[","]",";",","})raw.replace(c," ");const auto words=raw.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);if(expected.isArray()){QJsonArray array;for(const auto &word:words){bool ok;const double v=word.toDouble(&ok);require(ok&&std::isfinite(v),"Actual binding array Write invalid");array.append(v);}values.append(array);}else{require(words.size()==1,"Actual scalar Write width differs");bool ok;const double v=words[0].toDouble(&ok);require(ok&&std::isfinite(v),"Actual binding scalar Write invalid");values.append(v);}}}return values;};
         const auto first=observer.records[0].toObject()["parameters"].toArray()[0].toObject()["actual"];
         if(reference(spec,false,false)==reference(spec,false,true))privateWrites=parse(reference(spec,false,false),first);
         else{privateWrites=parse(reference(spec,false,false),first);const auto local=parse(reference(spec,false,true),first);for(const auto &v:local)privateWrites.append(v);}
         sharedWrites=parse(reference(spec,true,true),first);require(privateWrites.size()==4&&sharedWrites.size()==4,"Actual main/private/global parameter Write count differs");
         for(int i=0;i<4;++i){const auto parameters=observer.records[i].toObject()["parameters"].toArray();compareValue(privateWrites[i],parameters[0].toObject()["actual"],bound(family,type));compareValue(sharedWrites[i],parameters[1].toObject()["actual"],bound(family,type));}
         const auto bindingBytes=read(directory+"/Bindings.txt");require(!bindingBytes.isEmpty(),"Actual binding parameter Report produced no bytes");rig.save(tag+"-binding-values.txt",bindingBytes);
         const auto raw=read(report);rig.save(tag+"-values.txt",raw);QJsonArray markers;for(const auto &word:QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)){bool ok;const double v=word.toDouble(&ok);require(ok&&std::isfinite(v),"Binding marker report invalid");markers.append(v);}require(markers==QJsonArray{42,43,44,103,104},"Binding file/private/global/caller marker differs");
         auto *root=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("PrivateOnly"));require(root&&root->GetRealParameter("DryMass")==987&&root->GetState()[0]==7000&&Moderator::Instance()->GetInternalObject("ScopeEnd")->GetRealParameter("Value")==24680,"Binding private use changed caller shadow/sentinel");
         const auto rootValue=stateJson(root);compareValue(rootValue,observation(spec,false,false,0)["after_script_inertial"],1e-9);require(root->GetEpoch()==21545,"Cartesian caller root epoch changed");
         const auto after=prototypes();rig.save(tag+"-prototypes.json",QJsonDocument(after).toJson());const QJsonObject invariants{{"root_selected_property",true},{"prototypes",after==before},{"source",rig.source->toPlainText()==caller},{"history",rig.source->document()->availableUndoSteps()==undo},{"function_file",read(function)==edited.toUtf8()},{"nested_file",read(outer)==outerBody.toUtf8()},{"not_running",!rig.window.isRunning()}};rig.save(tag+"-invariants.json",QJsonDocument(invariants).toJson());for(const auto &v:invariants)require(v.toBool(),"Binding run changed source/history/prototypes/files/lock");
         phases.append(QJsonObject{{"phase",phase},{"explicit_rebuild",phase<2},{"live_samples",observer.records},{"private_writes",privateWrites},{"shared_writes",sharedWrites},{"raw_markers",markers},{"callback_private_maps_accessed_only_while_executing",true}});rig.save("phases.json",QJsonDocument(phases).toJson());
      }
      auto *tree=rig.window.findChild<QTreeWidget*>("Output"); const auto outputs=tree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(outputs.size()==1,"Binding report output absent"); tree->itemDoubleClicked(outputs[0],0); events(); auto *viewer=rig.area->activeSubWindow(); require(viewer && dynamic_cast<ReportViewer*>(viewer->widget()),"Binding actual report viewer absent"); auto *shown=viewer->findChild<QPlainTextEdit*>(); require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(read(report)).trimmed(),"Binding viewer differs from checked bytes"); acceptanceView(*viewer);

      const QJsonObject proof{{"checkpoint","live-function-cartesian-frame-parameter-binding"},{"outcome","passed"},{"case",key},{"argument",argument},{"mode",spec["mode"]},{"route","instantaneous-rotating-Cartesian-frame"},{"family",family},{"full_system_parameter_inventory",321},{"initial_batch_types",catalog["initial_types"]},{"missions",3},{"function_calls",12},{"live_callback_samples",12},{"actual_parameter_owner_checks",24},{"file_decisions",decisions},{"phases",phases},{"scope","All321 original types retained. Six Cartesian component types and existing Cartesian vector have actual main/private/Global SpacePoint-to-Spacecraft owner and default/custom frame references. Independent nonidentity instantaneous rotation/rate transport,actual primary/guide graph and zero relative acceleration,script component mutations and native scalar mutation/full-state restore are checked. Owners/guides stay at21545 while separate pulse spacecraft provide synchronous scope boundaries. Legacy Unit strings are retained without mixed-vector unit qualification. No arbitrary frame/translation/provider/propagation science or whole-row credit;no expired private-pointer access."}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }
   catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
   catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
   return 1;
}
