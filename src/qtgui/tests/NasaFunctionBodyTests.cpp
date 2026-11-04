#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "ObjectManagedFunction.hpp"
#include "SolarSystem.hpp"
#include "Spacecraft.hpp"
#include "UserDefinedFunction.hpp"
#include <QCryptographicHash>
#include "SandboxException.hpp"
#include "ScriptInterpreter.hpp"
#include <QStatusBar>
#include <array>
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
   require(widget,"Body scope resource control absent");
   for (auto *page=widget;page;page=page->parentWidget())
   {
      for (auto *tabs:widget->window()->findChildren<QTabWidget*>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
      if (auto *scroll=qobject_cast<QScrollArea*>(page)) scroll->ensureWidgetVisible(widget);
   }
   events(); require(widget->isVisible() && widget->isEnabled(),"Body scope resource control unreachable"); acceptanceView(*widget);
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("FunctionBodyScope");
   try
   {
      require(argc==4 && acceptanceWidgetPlatform(),"Supply body scope startup,catalog and key");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath()); const QString key=argv[3];
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object(); QJsonObject spec;
      for (const auto &v:catalog["cases"].toArray()) if (v.toObject()["key"]==key) spec=v.toObject();
      require(!spec.isEmpty(),"Unknown body scope candidate");
      const QString type=spec["type"].toString(),central=spec["central"].toString();
      const bool existingSun=key=="Sun";const QString mainName=existingSun?"Sun":"MainBody",functionName=existingSun?"Sun":"FunctionBody";const auto expectedSource=existingSun?QString::fromStdString(Moderator::Instance()->GetSolarSystemInUse()->GetBody("Sun")->GetStringParameter(Moderator::Instance()->GetSolarSystemInUse()->GetBody("Sun")->GetParameterID("PosVelSource"))):QString("SPICE");
      const auto kernel=catalog["kernel"].toString(),kernelHash=catalog["kernel_sha256"].toString();
      auto kernelDigest=[&]{return QString::fromLatin1(QCryptographicHash::hash(read(kernel),QCryptographicHash::Sha256).toHex());};
      require(QFileInfo(kernel).isFile()&&kernelDigest()==kernelHash,"Existing body configuration kernel changed");
      const QJsonObject inputFiles{{"path",kernel},{"sha256",kernelHash},{"copied",false}};
      const auto directory=rig.files.filePath("Function body Ω"); require(QDir().mkpath(directory),"Body scope directory absent");
      const auto function=directory+"/ReadShared.gmf",outer=directory+"/NestedShared.gmf",saved=directory+"/Caller.script",report=directory+"/Values.txt";
      auto bodySettings=[&](const QString &name,int mu,int radius){QString code="Create "+type+" "+name+";\n"+name+".CentralBody = "+central+";\n"+name+".NAIFId = 2000001;\n"+name+".OrbitSpiceKernelName = {'"+kernel+"'};\n"+name+".Mu = "+QString::number(mu)+";\n"+name+".EquatorialRadius = "+QString::number(radius)+";\n"+name+".SpinAxisDECConstant = 90;\n";if(type=="Star")code+=name+".RadiantPower = 1000000;\n"+name+".ReferenceDistance = 1000;\n"+name+".PhotosphereRadius = "+QString::number(radius)+";\n";return code;};
      const QString original="function [Mark] = ReadShared()\n"+(existingSun?QString():bodySettings("FunctionBody",73,480))+"Create Variable Mark PrivateOnly;\nPrivateOnly = 103;\nBeginMissionSequence;\nGlobal Calls;\nCalls = Calls + 1;\nWrite "+mainName+(existingSun?QString():" "+functionName)+" {Style = Script, MessageWindow = true, LogFile = false};\nMark = PrivateOnly + Calls;\n% Runtime solar-system body Ω\n";
      const auto edited=QString(original).replace("PrivateOnly = 103;","PrivateOnly = 41;");
      const QString outerBody="function [Mark] = NestedShared()\nCreate Variable Mark;\nCreate GmatFunction ReadShared;\nReadShared.FunctionPath = '"+function+"';\nBeginMissionSequence;\n[Mark] = ReadShared();\n";
      write(function,original.toUtf8()); write(outer,outerBody.toUtf8()); rig.save("initial.gmf",original.toUtf8()); rig.save("nested.gmf",outerBody.toUtf8());
      const QString caller=(existingSun?QString("Sun.Mu = 63;\nSun.EquatorialRadius = 470;\n"):bodySettings("MainBody",63,470))+"Create Spacecraft PrivateOnly;\nPrivateOnly.CoordinateSystem = EarthMJ2000Eq;\nPrivateOnly.DateFormat = A1ModJulian;\nPrivateOnly.Epoch = '21545';\nPrivateOnly.DisplayStateType = Cartesian;\nPrivateOnly.X = 7000;\nPrivateOnly.Y = 100;\nPrivateOnly.Z = 200;\nPrivateOnly.VX = -0.1;\nPrivateOnly.VY = 7.5;\nPrivateOnly.VZ = 0.8;\nPrivateOnly.DryMass = 987;\nPrivateOnly.NAIFId = -114104;\nPrivateOnly.NAIFIdReferenceFrame = -9114104;\nCreate Variable First Second Third Keep Calls;\nKeep = 103;\nCalls = 0;\nCreate GmatFunction ReadShared NestedShared;\nReadShared.FunctionPath = '"+function+"';\nNestedShared.FunctionPath = '"+outer+"';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\nBeginMissionSequence;\nGlobal Calls;\nWrite "+mainName+" {Style = Script, MessageWindow = true, LogFile = false};\n[First] = ReadShared();\n[Second] = ReadShared();\n[Third] = NestedShared();\nReport Values First Second Third Keep;\nKeep = Keep + 1;\nReport Values Keep;\n";
      require(!QRegularExpression("Global[^;]*(?:MainBody|FunctionBody|Sun)").match(caller+original+outerBody).hasMatch(),"Fixture declares an automatic body Global");
      rig.source->setPlainText(caller); rig.save("caller.script",caller.toUtf8()); const bool built=rig.window.buildScript(); rig.save("build-messages.txt",rig.messages->toPlainText().toUtf8());
      if (!built) { std::cerr<<rig.messages->toPlainText().toStdString(); QJsonArray errors; for (const auto &e:Moderator::GetScriptInterpreter()->GetErrorList()) { errors.append(QString::fromStdString(e)); std::cerr<<e<<std::endl; } rig.save("build-diagnostics.json",QJsonDocument(QJsonObject{{"errors",errors},{"status",rig.window.statusBar()->currentMessage()}}).toJson()); } require(built,"Body scope resource caller Build failed");
      auto native=[&](CelestialBody *body,const QString &name,int mu,int radius){require(body&&body->GetName()==name.toStdString()&&body->GetTypeName()==type.toStdString()&&body->IsUserDefined()==!existingSun&&body->IsGlobal()&&body->IsAutomaticGlobal()&&!body->IsLocal()&&body->GetRealParameter(body->GetParameterID("Mu"))==mu&&body->GetRealParameter(body->GetParameterID("EquatorialRadius"))==radius&&body->GetStringParameter(body->GetParameterID("CentralBody"))==central.toStdString()&&body->GetIntegerParameter(body->GetParameterID("NAIFId"))==(existingSun?10:2000001)&&QString::fromStdString(body->GetStringParameter(body->GetParameterID("PosVelSource")))==expectedSource,"Native body type/identity/literal/global metadata differs");return QJsonObject{{"name",name},{"type",type},{"mu",mu},{"radius",radius},{"central",central},{"user",body->IsUserDefined()},{"global",body->IsGlobal()},{"automatic",body->IsAutomaticGlobal()},{"local",body->IsLocal()},{"naif",body->GetIntegerParameter(body->GetParameterID("NAIFId"))},{"source",QString::fromStdString(body->GetStringParameter(body->GetParameterID("PosVelSource")))}};};
      auto prototypes=[&]{QJsonObject values;auto *ss=Moderator::Instance()->GetSolarSystemInUse();require(ss&&(existingSun||!ss->GetBody("FunctionBody")),"Function-created body escaped into configured solar system");for(const auto &name:QStringList{mainName,"PrivateOnly","Calls"}){auto *p=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(p,"Body-scope prototype absent");values[name]=QString::fromStdString(p->GetGeneratingString(Gmat::NO_COMMENTS));}return values;};
      const auto before=prototypes();rig.save("prototypes-before.json",QJsonDocument(before).toJson());
      native(Moderator::Instance()->GetSolarSystemInUse()->GetBody(mainName.toStdString()),mainName,63,470);
      for (auto *tabs:rig.window.findChildren<QTabWidget*>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==rig.resources || tabs->widget(i)->isAncestorOf(rig.resources)) tabs->setCurrentIndex(i);
      const auto entries=rig.resources->findItems("ReadShared",Qt::MatchExactly|Qt::MatchRecursive); require(entries.size()==1,"Body scope function resource absent"); rig.resources->scrollToItem(entries[0]); rig.resources->itemDoubleClicked(entries[0],0); events();
      auto *owner=rig.area->activeSubWindow(); auto *panel=owner?dynamic_cast<ResourceEditor*>(owner->widget()):nullptr; require(panel,"Body scope function editor absent"); owner->showNormal(); owner->move(0,0); owner->resize(rig.area->viewport()->size()); events(); auto *edit=panel->findChild<QPushButton*>("editFunctionFile"); reveal(edit);
      const int history=rig.source->document()->availableUndoSteps(); QJsonArray decisions;
      for (bool commit:{false,true})
      {
         bool visited=false; std::exception_ptr failure;
         QTimer::singleShot(0,panel,[&]{auto *dialog=panel->findChild<QDialog*>("functionFileDialog"); try { require(dialog,"Body scope file dialog absent"); auto *text=dialog->findChild<QPlainTextEdit*>("functionFileText"); require(text && !text->isReadOnly() && text->toPlainText()==original,"Body scope initial file differs"); reveal(text); QTest::mouseClick(text->viewport(),Qt::LeftButton); QTest::keyClick(text,Qt::Key_A,Qt::ControlModifier); QApplication::clipboard()->setText(edited); QTest::keyClick(text,Qt::Key_V,Qt::ControlModifier); events(); require(text->toPlainText()==edited && read(function)==original.toUtf8() && rig.source->toPlainText()==caller && rig.source->document()->availableUndoSteps()==history && prototypes()==before,"Body scope draft changed file/caller/history/prototypes"); auto *button=dialog->findChild<QDialogButtonBox*>()->button(commit?QDialogButtonBox::Save:QDialogButtonBox::Cancel); reveal(button); QTest::mouseClick(button,Qt::LeftButton); visited=true; } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }});
         QTest::mouseClick(edit,Qt::LeftButton); if (failure) std::rethrow_exception(failure);
         require(visited && read(function)==(commit?edited:original).toUtf8() && rig.source->toPlainText()==caller,"Body scope file Save/Cancel differs"); decisions.append(QJsonObject{{"saved",commit}});
      }
      require(owner->close(),"Body scope editor failed to close"); events(); rig.save("edited.gmf",edited.toUtf8()); require(rig.window.saveScriptTo(saved),"Body scope Unicode caller Save failed"); QJsonArray phases;
      for (int phase=0;phase<3;++phase)
      {
         if (phase==1) require(rig.window.loadScript(saved) && rig.window.buildScript() && rig.source->toPlainText()==caller && prototypes()==before,"Body scope Unicode reopen changed source/prototypes");
         const int undo=rig.source->document()->availableUndoSteps(); rig.messages->clear(); acceptanceView(rig.window); const auto outcome=rig.window.runMission(); events(); const auto tag=QString::number(phase); const auto messages=rig.messages->toPlainText(); rig.save(tag+"-messages.txt",messages.toUtf8());
         if (outcome!=MainWindow::RunResult::Completed) std::cerr<<messages.toStdString(); require(outcome==MainWindow::RunResult::Completed,"Body scope main/local/global mission failed");
         auto *anchor=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("PrivateOnly"));auto *configuredSs=Moderator::Instance()->GetSolarSystemInUse();require(anchor&&anchor->GetRealParameter("DryMass")==987&&anchor->GetEpoch()==21545,"Function-private scalar changed same-name caller spacecraft");const std::array<double,6> anchorLiteral{7000,100,200,-.1,7.5,.8};for(int i=0;i<6;++i)require(anchor->GetState()[i]==anchorLiteral[i],"Body use changed caller anchor state");
         auto *runtimeSs=anchor->GetSolarSystem();require(runtimeSs&&runtimeSs!=configuredSs&&(existingSun||!configuredSs->GetBody("FunctionBody")),"Runtime/configured solar-system identity/isolation differs");auto *mainBody=runtimeSs->GetBody(mainName.toStdString()),*functionBody=runtimeSs->GetBody(functionName.toStdString());require(mainBody&&functionBody&&(existingSun||mainBody!=functionBody)&&mainBody!=configuredSs->GetBody(mainName.toStdString()),"Main/function body ownership/clone identity differs");const auto mainState=native(mainBody,mainName,63,470),functionState=native(functionBody,functionName,existingSun?63:73,existingSun?470:480);native(configuredSs->GetBody(mainName.toStdString()),mainName,63,470);require(Moderator::Instance()->GetInternalObject("Calls")->GetRealParameter("Value")==3,"Body reader call count differs");
         auto checkedWrites=[&](const QString &name,int count){QJsonArray result;auto matches=QRegularExpression("(?:^|\\n)Create "+QRegularExpression::escape(type)+" "+name+";").globalMatch(messages);while(matches.hasNext())result.append(matches.next().captured(0).trimmed());require(result.size()==count,"Actual body Script-style Write count/type differs");return result;};const auto mainWrites=checkedWrites(mainName,existingSun?0:4),functionWrites=checkedWrites(functionName,existingSun?0:3);
         auto writtenMu=[&](const QString &name,const QJsonArray &wanted){QJsonArray result;auto matches=QRegularExpression("(?:^|\\n)"+name+"\\.Mu = ([-+0-9.eE]+);").globalMatch(messages);while(matches.hasNext()){bool ok=false;const double value=matches.next().captured(1).toDouble(&ok);require(ok&&std::isfinite(value),"Body Write Mu not finite");result.append(value);}require(result==wanted,"Body Write shared-model/function literal differs");return result;};const auto mainMuWrites=writtenMu(mainName,QJsonArray{63,63,63,63}),functionMuWrites=existingSun?QJsonArray{}:writtenMu(functionName,QJsonArray{73,73,73});
         QJsonArray identities;
         auto readerIdentity=[&](UserDefinedFunction *reader,const QString &path){require(reader&&reader->GetName()=="ReadShared"&&reader->GetFunctionControlSequence()&&reader->GetSolarSystem()==runtimeSs,"Actual body reader FCS/shared model absent");auto *declared=reader->FindFunctionObject(functionName.toStdString());require(declared==functionBody,"Function declaration lookup refers to a different solar-system-owned body");int owned=0;for(int i=0;i<runtimeSs->GetOwnedObjectCount();++i)if(runtimeSs->GetOwnedObject(i)==functionBody)++owned;require(owned==1,"Runtime solar system does not own exactly one body instance");QJsonArray commands;int found=0;for(auto *command=reader->GetFunctionControlSequence();command;command=command->GetNext()){commands.append(QJsonObject{{"type",QString::fromStdString(command->GetTypeName())},{"source",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))}});if(command->GetTypeName()!="Create"||command->GetStringParameter("ObjectType")!=type.toStdString())continue;auto *created=command->GetRefObject(Gmat::CELESTIAL_BODY,"FunctionBody");require(created==functionBody,"Function Create refers to a different solar-system-owned body");++found;}identities.append(QJsonObject{{"path",path},{"function_created",!existingSun},{"declaration_object_pointer",pointer(declared)},{"owned_instances",owned},{"fcs_create_commands",found},{"fcs_commands",commands},{"runtime_solar_body_pointer",pointer(runtimeSs->GetBody(functionName.toStdString()))},{"reader_solar_system_pointer",pointer(reader->GetSolarSystem())},{"runtime_solar_system_pointer",pointer(runtimeSs)},{"configured_solar_system_pointer",pointer(configuredSs)}});};
         int direct=0,nested=0;for(auto *command=Moderator::Instance()->GetFirstCommand();command;command=command->GetNext()){if(!command->IsOfType("CallFunction"))continue;auto *functionObject=dynamic_cast<UserDefinedFunction*>(command->GetRefObject(Gmat::FUNCTION,""));require(functionObject,"Root body call does not reference a GmatFunction");if(functionObject->GetName()=="ReadShared"){readerIdentity(functionObject,"direct-"+QString::number(++direct));}else if(functionObject->GetName()=="NestedShared"){for(auto *inner=functionObject->GetFunctionControlSequence();inner;inner=inner->GetNext()){if(!inner->IsOfType("CallFunction"))continue;readerIdentity(dynamic_cast<UserDefinedFunction*>(inner->GetRefObject(Gmat::FUNCTION,"")),"nested");++nested;}}}require(direct==2&&nested==1&&identities.size()==3,"Direct/direct/nested body ownership identities missing");
         const auto raw=read(report); rig.save(tag+"-values.txt",raw); const auto words=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(words.size()==5,"Body scope raw continuation width differs"); QJsonArray values; for (const auto &word:words) { bool ok; const double v=word.toDouble(&ok); require(ok && std::isfinite(v),"Body scope raw marker invalid"); values.append(v); }
         require(values==QJsonArray{42,43,44,103,104},"Body scope private/caller sentinel or continuation differs");
         const auto after=prototypes(); rig.save(tag+"-prototypes.json",QJsonDocument(after).toJson()); rig.save(tag+"-invariants.json",QJsonDocument(QJsonObject{{"prototypes",after==before},{"source",rig.source->toPlainText()==caller},{"history",rig.source->document()->availableUndoSteps()==undo},{"function_file",read(function)==edited.toUtf8()},{"nested_file",read(outer)==outerBody.toUtf8()},{"not_running",!rig.window.isRunning()}}).toJson());
         require(after==before && rig.source->toPlainText()==caller && rig.source->document()->availableUndoSteps()==undo && read(function)==edited.toUtf8() && read(outer)==outerBody.toUtf8() && !rig.window.isRunning(),"Body scope execution changed source/history/prototypes/function files/lock");
         require(kernelDigest()==kernelHash,"Body configuration kernel input changed");
         phases.append(QJsonObject{{"phase",phase},{"explicit_rebuild",phase<2},{"main_writes",mainWrites},{"function_writes",functionWrites},{"main_mu_writes",mainMuWrites},{"function_mu_writes",functionMuWrites},{"main_body",mainState},{"function_body",functionState},{"function_created",!existingSun},{"configured_function_body_absent",!existingSun},{"same_name_caller_mass",987},{"caller_anchor_state",QJsonArray{7000,100,200,-.1,7.5,.8}},{"identities",identities},{"raw_values",values}});rig.save("phases.json",QJsonDocument(phases).toJson());
      }
      auto *tree=rig.window.findChild<QTreeWidget*>("Output"); const auto outputs=tree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(outputs.size()==1,"Body scope report output absent"); tree->itemDoubleClicked(outputs[0],0); events(); auto *viewer=rig.area->activeSubWindow(); require(viewer && dynamic_cast<ReportViewer*>(viewer->widget()),"Body scope actual report viewer absent"); auto *shown=viewer->findChild<QPlainTextEdit*>(); require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(read(report)).trimmed(),"Body scope viewer differs from checked bytes"); acceptanceView(*viewer);
      const QJsonObject proof{{"checkpoint","body-resource-solar-system-scope"},{"outcome","passed"},{"case",key},{"type",type},{"central",central},{"creation_mode",spec["creation_mode"]},{"data_input",inputFiles},{"missions",3},{"function_calls",12},{"reader_calls",9},{"file_decisions",decisions},{"phases",phases},{"scope","Four original NASA user-body classes retain actual main and function-side creation/use;Star has separate existing-Sun shared-model use and a retained failed custom-create probe. Actual body Script Writes/native Mu/radius literals,independent private-marker/counter arithmetic,shared solar-model identity,creation-to-owned-body identity where applicable,configured-model/source/history isolation,caller shadow,file Cancel/Save,Unicode reopen,no-Build repeat and report viewer. Existing Ceres kernel is a referenced configuration fixture only,never copied; no propagation,ephemeris/physical accuracy or universal requirement approval."}};
      rig.save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
   }
   catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
   catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
   return 1;
}
