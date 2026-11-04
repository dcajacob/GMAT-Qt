#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "ObjectManagedFunction.hpp"
#include "Parameter.hpp"
#include "ParameterInfo.hpp"
#include "TimeData.hpp"
#include "TimeSystemConverter.hpp"
#include "SpaceObject.hpp"
#include "CoordinateSystem.hpp"
#include "FileManager.hpp"
#include <QCryptographicHash>
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
static double bound(const QString &,const QString &type){return type=="ElapsedSecs"?2e-5:2e-10;}
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
static QJsonObject observation(const QJsonObject &spec,bool shared,bool inside,int calls){return spec["observations"].toObject()[shared?"shared":inside?"private":"root"].toArray()[calls].toObject();}
static void epochEquals(SpacePoint *owner,double expected){require(std::abs(owner->GetEpoch()-expected)<=2e-10,"Real owner epoch differs");require(std::abs(owner->GetEpochGT().GetMjd()-expected)<=2e-10,"GmatTime owner epoch differs");}
static QJsonObject clock(SpacePoint *owner){return QJsonObject{{"real",owner->GetEpoch()},{"gmat_time_mjd",owner->GetEpochGT().GetMjd()},{"gmat_time_text",QString::fromStdString(owner->GetEpochGT().ToString())}};}
class BindingObserver final:public Subscriber
{
public:
   BindingObserver(Rig &rig,const QJsonObject &spec,const QString &prefix):Subscriber("TimeBindingObserver","TimeBindingObserver"),rig(&rig),spec(spec),prefix(prefix){}
   ~BindingObserver() override{Publisher::Instance()->Unsubscribe(this);}
   GmatBase *Clone() const override{return new BindingObserver(*this);}
   bool ReceiveData(const Real *data,Integer count=0) override
   {
      bool initial=false;if(!Publisher::GetPublishedPropagationObjects(data,count,initial)||!initial)return true;
      try
      {
         auto *command=dynamic_cast<GmatCommand*>(currentProvider);require(command,"Time provider is not a command");auto *function=dynamic_cast<ObjectManagedFunction*>(command->GetCurrentFunction());const bool inside=function;
         const auto *locals=inside?function->GetFunctionObjectStore():command->GetObjectMap();const auto rootGlobals=inside?ObjectMap{}:Moderator::Instance()->GetSandbox()->GetGlobalObjectMap();const auto *globals=inside?function->GetGlobalObjectStore():&rootGlobals;require(locals&&globals,"Time live maps absent");
         auto lookup=[&](const std::string &name){auto a=locals->find(name);if(a!=locals->end())return a->second;auto b=globals->find(name);return b==globals->end()?nullptr:b->second;};auto *counter=lookup("Calls");require(counter,"Time counter absent");const int calls=int(counter->GetRealParameter("Value"));events.append(QJsonObject{{"calls",calls},{"inside_function",inside},{"function",inside?QString::fromStdString(function->GetName()):QString()},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))}});rig->save(prefix+"-callback-events.json",QJsonDocument(events).toJson());require(calls==events.size()-1&&inside==(calls>0)&&(!inside||function->GetName()=="ReadShared"),"Time callback order/scope differs");
         const auto type=spec["factory_type"].toString();const bool elapsed=type.startsWith("Elapsed"),string=type.endsWith("Gregorian");const auto meta=spec["metadata"].toObject();QJsonArray rows;
         for(bool shared:{false,true})
         {
            const QString ownerName=shared?"SharedSC":"PrivateOnly",full=reference(spec,shared,inside);auto *owner=dynamic_cast<Spacecraft*>(lookup(ownerName.toStdString()));auto *root=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject(ownerName.toStdString()));auto *configured=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetConfiguredObject(ownerName.toStdString()));require(owner&&root&&configured&&owner!=configured&&owner->IsOfType(meta["owner_type_id"].toInt())&&owner->IsGlobal()==shared&&owner->IsLocal()==(inside&&!shared)&&(owner==root)==(shared||!inside),"Time actual owner/class/flags differ");
            auto *parameter=dynamic_cast<Parameter*>(lookup(full.toStdString()));auto *time=dynamic_cast<TimeData*>(parameter);require(parameter&&time&&parameter->IsSystemParameter()&&parameter->GetName()==full.toStdString()&&parameter->GetTypeName()==meta["native_type"].toString().toStdString()&&int(parameter->GetReturnType())==meta["return_type"].toInt()&&int(parameter->GetOwnerType())==meta["owner_type_id"].toInt()&&parameter->GetRefObject(parameter->GetOwnerType(),ownerName.toStdString())==owner&&parameter->IsGlobal()==shared,"Time native Parameter owner/reference/metadata differs");
            const auto expected=observation(spec,shared,inside,calls);QJsonValue actual=string?QJsonValue(QString::fromStdString(parameter->EvaluateString())):QJsonValue(parameter->EvaluateReal());const double bound=expected["value_bound"].toDouble(),a1=expected["a1_epoch"].toDouble();
            diagnostics.append(QJsonObject{{"stage","before"},{"calls",calls},{"shared",shared},{"reference",full},{"actual",actual},{"expected",expected["expected"]},{"clock",clock(owner)},{"expected_a1",a1}});rig->save(prefix+"-clock-diagnostics.json",QJsonDocument(diagnostics).toJson());compareValue(actual,expected["expected"],bound);epochEquals(owner,a1);require(time->IsInitialEpochSet(),"Time initial epoch flag unset");const double initialEpoch=time->GetInitialEpoch();if(elapsed){require(std::abs(initialEpoch-expected["initial_a1_epoch"].toDouble())<=2e-10,"Elapsed initial epoch changed across scopes/calls");require(parameter->GetRealParameter("InitialEpoch")==initialEpoch,"Elapsed public initial epoch differs");}
            QJsonValue direct=elapsed?QJsonValue((owner->GetEpoch()-initialEpoch)*(type=="ElapsedSecs"?86400:1)):string?QJsonValue(QString::fromStdString(TimeSystemConverter::Instance()->ConvertMjdToGregorian(TimeSystemConverter::Instance()->Convert(a1,TimeSystemConverter::A1MJD,TimeSystemConverter::Instance()->GetTimeTypeID(expected["time_system"].toString().toStdString()),GmatTimeConstants::JD_JAN_5_1941)))):QJsonValue(TimeSystemConverter::Instance()->Convert(a1,TimeSystemConverter::A1MJD,TimeSystemConverter::Instance()->GetTimeTypeID(expected["time_system"].toString().toStdString()),GmatTimeConstants::JD_JAN_5_1941));compareValue(direct,expected["expected"],bound);
            const auto beforeState=stateJson(owner);QJsonObject formation;
            if(type=="CurrA1MJD")
            {
               std::unique_ptr<Spacecraft> peer(dynamic_cast<Spacecraft*>(owner->Clone()));require(bool(peer),"Native Formation peer clone absent");peer->SetName("ClockPeerNative");std::unique_ptr<GmatBase> group(FactoryManager::Instance()->CreateObject(Gmat::FORMATION,"Formation","ClockGroupNative"));auto *spaceGroup=dynamic_cast<SpaceObject*>(group.get());require(spaceGroup&&group->GetTypeName()=="Formation","Native Formation factory control absent");auto *contextFrame=dynamic_cast<CoordinateSystem*>(Moderator::Instance()->GetInternalObject("EarthMJ2000Eq"));require(contextFrame&&contextFrame->GetSolarSystem(),"Native Formation live context absent");spaceGroup->SetSolarSystem(contextFrame->GetSolarSystem());spaceGroup->SetJ2000Body(contextFrame->GetSolarSystem()->GetBody("Earth"));group->SetStringParameter("Add",ownerName.toStdString());group->SetStringParameter("Add","ClockPeerNative");require(group->SetRefObject(owner,Gmat::SPACECRAFT,ownerName.toStdString())&&group->SetRefObject(peer.get(),Gmat::SPACECRAFT,"ClockPeerNative")&&group->Initialize(),"Native Formation member setup failed");const int epochId=group->GetParameterID("A1Epoch");require(group->IsParameterReadOnly(epochId),"Formation script epoch read-only advertisement changed");epochEquals(spaceGroup,a1);epochEquals(peer.get(),a1);const auto peerState=stateJson(peer.get());const double target=a1+4/86400.;group->SetRealParameter(epochId,target);const auto changedGroup=clock(spaceGroup),changedOwner=clock(owner),changedPeer=clock(peer.get());diagnostics.append(QJsonObject{{"stage","formation-changed"},{"calls",calls},{"shared",shared},{"group",changedGroup},{"owner",changedOwner},{"peer",changedPeer},{"target",target}});rig->save(prefix+"-clock-diagnostics.json",QJsonDocument(diagnostics).toJson());epochEquals(spaceGroup,target);epochEquals(owner,target);epochEquals(peer.get(),target);compareValue(parameter->EvaluateReal(),target,bound);group->SetRealParameter(epochId,a1);epochEquals(spaceGroup,a1);epochEquals(owner,a1);epochEquals(peer.get(),a1);require(stateJson(owner)==beforeState&&stateJson(peer.get())==peerState,"Native Formation epoch control changed Cartesian states");formation=QJsonObject{{"factory_type","Formation"},{"native_api_only",true},{"script_epoch_readonly",true},{"group_pointer",pointer(group.get())},{"member_pointer",pointer(owner)},{"peer_pointer",pointer(peer.get())},{"target",target},{"changed_group",changedGroup},{"changed_owner",changedOwner},{"changed_peer",changedPeer},{"restored_group",clock(spaceGroup)},{"restored_owner",clock(owner)},{"restored_peer",clock(peer.get())},{"member_states_preserved",true}};
            }
            QJsonObject mutation;
            if(!elapsed)
            {
               const auto target=expected["native_target"];if(string)parameter->SetString(target.toString().toStdString());else parameter->SetReal(target.toDouble());const QJsonValue changed=string?QJsonValue(QString::fromStdString(parameter->EvaluateString())):QJsonValue(parameter->EvaluateReal());const auto changedClock=clock(owner);diagnostics.append(QJsonObject{{"stage","parameter-changed"},{"calls",calls},{"shared",shared},{"reference",full},{"target",target},{"actual",changed},{"clock",changedClock},{"expected_a1",expected["native_a1_epoch"]}});rig->save(prefix+"-clock-diagnostics.json",QJsonDocument(diagnostics).toJson());compareValue(changed,target,bound);epochEquals(owner,expected["native_a1_epoch"].toDouble());require(time->GetInitialEpoch()==initialEpoch,"Native time mutation changed InitialEpoch");if(string)parameter->SetString(actual.toString().toStdString());else parameter->SetReal(actual.toDouble());const QJsonValue restored=string?QJsonValue(QString::fromStdString(parameter->EvaluateString())):QJsonValue(parameter->EvaluateReal());compareValue(restored,actual,bound);epochEquals(owner,a1);require(time->GetInitialEpoch()==initialEpoch&&stateJson(owner)==beforeState&&parameter->GetRefObject(parameter->GetOwnerType(),ownerName.toStdString())==owner,"Time restore changed initial epoch,state or owner reference");mutation=QJsonObject{{"delta_seconds",2},{"target",target},{"changed_value",changed},{"changed_clock",changedClock},{"restored_value",restored},{"restored_clock",clock(owner)},{"initial_epoch_preserved",true},{"state_and_owner_preserved",true}};
            }
            require(parameter->IsSettable()==meta["settable"].toBool()&&ParameterInfo::Instance()->IsSettable(type.toStdString())==meta["settable"].toBool(),"Original time writable metadata changed");const double liveOffset=TimeSystemConverter::Instance()->NumberOfLeapSecondsFrom(expected["selected_mjd"].toDouble(),GmatTimeConstants::JD_JAN_5_1941);require(liveOffset==32,"Live January2000 leap offset differs");
            rows.append(QJsonObject{{"shared",shared},{"reference",full},{"factory_type",type},{"native_type",QString::fromStdString(parameter->GetTypeName())},{"return_type",int(parameter->GetReturnType())},{"owner_type",int(parameter->GetOwnerType())},{"parameter_pointer",pointer(parameter)},{"reference_owner_pointer",pointer(parameter->GetRefObject(parameter->GetOwnerType(),ownerName.toStdString()))},{"live_owner_pointer",pointer(owner)},{"root_owner_pointer",pointer(root)},{"configured_owner_pointer",pointer(configured)},{"owner_name",ownerName},{"owner_type_name",QString::fromStdString(owner->GetTypeName())},{"owner_global",owner->IsGlobal()},{"owner_local",owner->IsLocal()},{"parameter_global",parameter->IsGlobal()},{"parameter_local",parameter->IsLocal()},{"actual",actual},{"expected",expected["expected"]},{"direct_time_value",direct},{"clock",clock(owner)},{"initial_epoch",initialEpoch},{"initial_epoch_set",time->IsInitialEpochSet()},{"cartesian_state",beforeState},{"absolute_bound",bound},{"shape",QJsonArray{}},{"runtime_settable",parameter->IsSettable()},{"info_settable",meta["settable"]},{"legacy_unit",QString::fromStdString(parameter->GetStringParameter("Unit"))},{"live_leap_offset",liveOffset},{"native_mutation",mutation},{"native_formation_control",formation}});
         }
         records.append(QJsonObject{{"calls",calls},{"inside_function",inside},{"function",inside?QString::fromStdString(function->GetName()):QString()},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))},{"global_map_is_value_snapshot",!inside},{"parameters",rows}});rig->save(prefix+"-live-parameter-samples.json",QJsonDocument(records).toJson());
      }
      catch(BaseException &e){if(!failure){error=QString::fromStdString(e.GetFullMessage());failure=std::current_exception();}}
      catch(std::exception &e){if(!failure){error=e.what();failure=std::current_exception();}}
      return true;
   }
   QJsonArray records,events,diagnostics;std::exception_ptr failure;QString error;
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
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FunctionTimeParameterBinding");
   try
   {
      require(argc==4&&acceptanceWidgetPlatform(),"Supply parameter-binding startup,catalog and type");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();const QString argument=argv[3],key=argument;QJsonObject spec;QStringList inventory;
      for(const auto &v:catalog["cases"].toArray())inventory.append(v.toObject()["factory_type"].toString());for(const auto &v:catalog["time_binding_cases"].toArray())if(v.toObject()["factory_type"]==key)spec=v.toObject();
      require(catalog["factory_rows"]==566&&inventory.size()==321&&!spec.isEmpty()&&catalog["initial_types"].toArray().contains(key)&&spec["family"]=="Time","Full parameter inventory or selected owner fixture differs");
      QStringList actual;for(const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::PARAMETER))actual.append(QString::fromStdString(name));inventory+=QStringList{"Array","String","Variable"};inventory.sort();actual.sort();require(actual==inventory,"Advertised complete ParameterFactory universe changed");
      const auto family=spec["family"].toString(),type=spec["factory_type"].toString();std::unique_ptr<Parameter> prototype(FactoryManager::Instance()->CreateParameter(type.toStdString()));
      require(prototype&&prototype->IsSystemParameter()&&prototype->GetTypeName()==spec["metadata"].toObject()["native_type"].toString().toStdString(),"Selected unmanaged native factory control differs");
      const auto leapPath=QString::fromStdString(FileManager::Instance()->GetFullPathname("LEAP_SECS_FILE"));const auto leapCanonical=QFileInfo(leapPath).canonicalFilePath(),leapHash=QString::fromLatin1(QCryptographicHash::hash(read(leapPath),QCryptographicHash::Sha256).toHex());const auto runtime=catalog["runtime_leap_table"].toObject();require(leapCanonical==runtime["canonical_path"].toString()&&leapHash==runtime["sha256"].toString()&&TimeSystemConverter::Instance()->NumberOfLeapSecondsFrom(21545)==32,"Configured/runtime leap table identity or live offset differs");const QJsonObject leapEvidence{{"configured_path",leapPath},{"canonical_path",leapCanonical},{"sha256",leapHash},{"live_offset_at_21545",32},{"reader_pointer_or_internal_filename_directly_observed",false},{"content_copied",false}};rig.save("runtime-leap-table.json",QJsonDocument(leapEvidence).toJson());
      const auto directory=rig.files.filePath("Live parameter owner Ω");require(QDir().mkpath(directory),"Binding Unicode directory absent");
      const auto function=directory+"/ReadShared.gmf",outer=directory+"/NestedShared.gmf",saved=directory+"/Caller.script",report=directory+"/Values.txt";
      auto literal=[](double v){return QString::number(v,'g',17);};
      auto epochSetup=[&](const QString &name,const QString &scope){const auto rc=observation(spec,scope=="shared",scope=="private",0);const auto input=rc["input_value"];return name+".DateFormat = "+rc["date_format"].toString()+";\n"+name+".Epoch = '"+(input.isString()?input.toString():literal(input.toDouble()))+"';\n";};
      QString declaration=spacecraft("PrivateOnly",7500,275,-127101)+epochSetup("PrivateOnly","private")+spacecraft("ScopePulse",9000,701,-127102);
      QString mutate;for(int call=1;call<=3;++call){mutate+="If Calls == "+QString::number(call)+";\n";for(bool shared:{false,true}){const auto rc=observation(spec,shared,true,call);const auto value=rc["input_value"];const QString ref=type.startsWith("Elapsed")?(shared?"SharedSC.A1ModJulian":"PrivateOnly.A1ModJulian"):reference(spec,shared,true);mutate+=ref+" = "+(value.isString()?"'"+value.toString()+"'":literal(value.toDouble()))+";\n";}mutate+="EndIf;\n";}
      const QString original="function [Mark] = ReadShared()\n"+declaration+"Create Variable Mark Keep ScopeEnd;\nKeep = 103;\nScopeEnd = 12345;\nBeginMissionSequence;\nGlobal SharedSC Calls BindingValues;\nCalls = Calls + 1;\n"+mutate+"Write "+reference(spec,false,true)+" "+reference(spec,true,true)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\nReport BindingValues "+reference(spec,false,true)+" "+reference(spec,true,true)+";\nPropagate PulseProp(ScopePulse) {ScopePulse.ElapsedSecs = 1};\nMark = Keep + Calls;\n% Live time binding Ω\n";
      const auto edited=QString(original).replace("Keep = 103;","Keep = 41;");
      const QString outerBody="function [Mark] = NestedShared()\nCreate Variable Mark;\nCreate GmatFunction ReadShared;\nReadShared.FunctionPath = '"+function+"';\nBeginMissionSequence;\n[Mark] = ReadShared();\n";
      write(function,original.toUtf8());write(outer,outerBody.toUtf8());rig.save("initial.gmf",original.toUtf8());rig.save("nested.gmf",outerBody.toUtf8());
      QString caller=spacecraft("PrivateOnly",7000,987,-127001)+epochSetup("PrivateOnly","root")+spacecraft("SharedSC",8000,150,-127002)+epochSetup("SharedSC","shared")+spacecraft("MainPulse",9000,702,-127003);
      caller+="Create ForceModel PulseFM;\nPulseFM.PrimaryBodies = {};\nPulseFM.PointMasses = {Earth};\nPulseFM.Drag = None;\nPulseFM.SRP = Off;\nCreate Propagator PulseProp;\nPulseProp.FM = PulseFM;\nPulseProp.Type = RungeKutta89;\nPulseProp.InitialStepSize = 1;\nPulseProp.MinStep = 0;\nPulseProp.MaxStep = 1;\nCreate Variable First Second Third Keep Calls ScopeEnd;\nKeep = 103;\nCalls = 0;\nScopeEnd = 24680;\nCreate GmatFunction ReadShared NestedShared;\nReadShared.FunctionPath = '"+function+"';\nNestedShared.FunctionPath = '"+outer+"';\nCreate ReportFile BindingValues;\nBindingValues.Filename = '"+directory+"/Bindings.txt';\nBindingValues.WriteHeaders = false;\nBindingValues.FixedWidth = false;\nBindingValues.Delimiter = ',';\nBindingValues.Precision = 17;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\nBeginMissionSequence;\nGlobal SharedSC Calls BindingValues;\nWrite 'Arm bindings' Keep {Style = Verbose, MessageWindow = true, LogFile = false};\nWrite "+reference(spec,false,false)+" "+reference(spec,true,false)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\nReport BindingValues "+reference(spec,false,false)+" "+reference(spec,true,false)+";\nPropagate PulseProp(MainPulse) {MainPulse.ElapsedSecs = 1};\n[First] = ReadShared();\n[Second] = ReadShared();\n[Third] = NestedShared();\nReport Values First Second Third Keep;\nKeep = Keep + 1;\nReport Values Keep;\n";
      rig.source->setPlainText(caller);rig.save("caller.script",caller.toUtf8());const bool built=rig.window.buildScript();rig.save("build-messages.txt",rig.messages->toPlainText().toUtf8());if(!built){std::cerr<<rig.messages->toPlainText().toStdString();QJsonArray errors;for(const auto &e:Moderator::GetScriptInterpreter()->GetErrorList()){errors.append(QString::fromStdString(e));std::cerr<<e<<std::endl;}rig.save("build-errors.json",QJsonDocument(errors).toJson());}require(built,"Binding main parameter caller Build failed");
      auto prototypes=[&]{QJsonObject values;for(const auto &name:QStringList{"PrivateOnly","SharedSC","Calls","Keep","ScopeEnd"}){auto *p=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(p,"Binding prototype absent");values[name]=QString::fromStdString(p->GetGeneratingString(Gmat::NO_COMMENTS));}return values;};const auto before=prototypes();rig.save("prototypes-before.json",QJsonDocument(before).toJson());
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
         const auto rootEpoch=root->GetEpoch(),rootEpochGT=root->GetEpochGT().GetMjd();const double wanted=observation(spec,false,false,0)["a1_epoch"].toDouble();require(std::abs(rootEpoch-wanted)<=2e-10&&std::abs(rootEpochGT-wanted)<=2e-10,"Function changed caller Real/GmatTime epoch");
         const auto after=prototypes();rig.save(tag+"-prototypes.json",QJsonDocument(after).toJson());const QJsonObject invariants{{"root_selected_property",true},{"prototypes",after==before},{"source",rig.source->toPlainText()==caller},{"history",rig.source->document()->availableUndoSteps()==undo},{"function_file",read(function)==edited.toUtf8()},{"nested_file",read(outer)==outerBody.toUtf8()},{"not_running",!rig.window.isRunning()}};rig.save(tag+"-invariants.json",QJsonDocument(invariants).toJson());for(const auto &v:invariants)require(v.toBool(),"Binding run changed source/history/prototypes/files/lock");
         phases.append(QJsonObject{{"phase",phase},{"explicit_rebuild",phase<2},{"live_samples",observer.records},{"private_writes",privateWrites},{"shared_writes",sharedWrites},{"raw_markers",markers},{"callback_private_maps_accessed_only_while_executing",true}});rig.save("phases.json",QJsonDocument(phases).toJson());
      }
      auto *tree=rig.window.findChild<QTreeWidget*>("Output"); const auto outputs=tree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(outputs.size()==1,"Binding report output absent"); tree->itemDoubleClicked(outputs[0],0); events(); auto *viewer=rig.area->activeSubWindow(); require(viewer && dynamic_cast<ReportViewer*>(viewer->widget()),"Binding actual report viewer absent"); auto *shown=viewer->findChild<QPlainTextEdit*>(); require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(read(report)).trimmed(),"Binding viewer differs from checked bytes"); acceptanceView(*viewer);

      const QJsonObject proof{{"checkpoint","live-function-time-parameter-owner-binding"},{"outcome","passed"},{"case",key},{"argument",argument},{"route","selected-time-system-and-dual-clock"},{"family",family},{"full_system_parameter_inventory",321},{"initial_batch_types",catalog["initial_types"]},{"missions",3},{"function_calls",12},{"live_callback_samples",12},{"actual_parameter_owner_checks",24},{"file_decisions",decisions},{"runtime_leap_table",leapEvidence},{"phases",phases},{"scope","All321 original types retained. Ten new time/elapsed types plus A1ModJulian,CurrA1MJD,UTCGregorian controls have actual native owner references,selected-system literals,Real/GmatTime clocks,initial epoch persistence,native epoch mutation/restore and raw Write/Report. CurrA1MJD adds temporary native Formation parent/member/peer clock regression while script epoch remains read-only. January2000 noon fixtures use offset32 and a bounded TDB periodic approximation;no leap boundary,arbitrary time/provider/non-Spacecraft/propagation science or whole-row qualification. Configured/canonical leap-table hash and live offset are measured;internal reader pointer/filename is not directly exposed. No copied runtime table/kernel or expired private-pointer access."}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }
   catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
   catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
   return 1;
}
