#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "ObjectManagedFunction.hpp"
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
   require(widget,"Remaining resource control absent");
   for (auto *page=widget;page;page=page->parentWidget())
   {
      for (auto *tabs:widget->window()->findChildren<QTabWidget*>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
      if (auto *scroll=qobject_cast<QScrollArea*>(page)) scroll->ensureWidgetVisible(widget);
   }
   events(); require(widget->isVisible() && widget->isEnabled(),"Remaining resource control unreachable"); acceptanceView(*widget);
}
static bool same(const QJsonValue &a,const QJsonValue &b)
{
   if (a.isDouble() && b.isDouble()) return std::abs(a.toDouble()-b.toDouble())<1e-12;
   if (a.isArray() && b.isArray()) { const auto x=a.toArray(),y=b.toArray(); if (x.size()!=y.size()) return false; for (int i=0;i<x.size();++i) if (!same(x[i],y[i])) return false; return true; }
   return a==b;
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("FunctionRemainingResource");
   try
   {
      require(argc==4 && acceptanceWidgetPlatform(),"Supply remaining resource startup,catalog and key");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath()); const QString key=argv[3];
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object(); QJsonObject spec;
      for (const auto &v:catalog["cases"].toArray()) if (v.toObject()["key"]==key) spec=v.toObject();
      require(!spec.isEmpty(),"Unknown remaining resource candidate");
      const QString type=spec["type"].toString(),nativeType=spec["native_type"].toString(),field=spec["field"].toString(),kind=spec["kind"].toString();
      const bool readOnly=spec["read_only"].toBool(),sameNameShadow=spec["caller_same_name_shadow"].toBool();
      const auto directory=rig.files.filePath("Remaining resource Ω"); require(QDir().mkpath(directory),"Remaining resource directory absent");
      auto expand=[&](QString text){return text.replace("@ROOT@",directory);};
      const auto files=spec["files"].toObject(); QJsonObject inputFiles;
      for (auto i=files.begin();i!=files.end();++i) { const auto bytes=expand(i.value().toString()).toUtf8(); write(directory+"/"+i.key(),bytes); rig.save("input-"+i.key(),bytes); inputFiles[i.key()]=QString::fromUtf8(bytes); }
      const QString first=expand(spec["first"].toString()),second=expand(spec["second"].toString());
      auto parse=[&](QString text)->QJsonValue {
         text=text.trimmed();
         if (kind=="real" || kind=="integer") { bool ok; const double value=text.toDouble(&ok); require(ok && std::isfinite(value),"Remaining property is not finite numeric data"); return value; }
         if (kind=="boolean") { require(text.toLower()=="true" || text.toLower()=="false","Remaining boolean data invalid"); return text.toLower()=="true"; }
         if (kind=="vector") { text.replace(QRegularExpression("[\\[\\]{},]")," "); QJsonArray values; for (const auto &word:text.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) { bool ok; const double v=word.toDouble(&ok); require(ok && std::isfinite(v),"Remaining vector contains invalid data"); values.append(v); } require(!values.isEmpty(),"Remaining vector empty"); return values; }
         require(kind=="string" || kind=="onoff","Remaining field kind unsupported");
         if (text.startsWith('\'') && text.endsWith('\'')) text=text.mid(1,text.size()-2); return text;
      };
      const auto expectedFirst=parse(first),expectedSecond=parse(second);
      auto literal=[&](const QString &text){return kind=="string"?"'"+text+"'":kind=="vector"?"["+text+"]":text;};
      auto settings=[&](const QString &name){QString text; for (const auto &line:expand(spec["object_settings"].toString()).replace("@NAME@",name).replace("@NAIF@",QString::number(name=="Shared"?-113101:name=="Shadow"?-113102:-113103)).replace("@FRAMEID@",QString::number(name=="Shared"?-9113101:name=="Shadow"?-9113102:-9113103)).split('\n',Qt::SkipEmptyParts)) text+=name+"."+line+"\n"; return text;};
      auto setting=[&](const QString &name,const QString &seed){return readOnly?QString():name+"."+field+" = "+literal(seed)+";\n";};
      const QString dependencies=expand(spec["dependencies"].toString())+(sameNameShadow?"Create Spacecraft PrivateOnly;\nPrivateOnly.DryMass = 987;\nPrivateOnly.NAIFId = -113104;\nPrivateOnly.NAIFIdReferenceFrame = -9113104;\n":QString()),globals=spec["globals"].toString();
      const auto function=directory+"/ReadShared.gmf",outer=directory+"/NestedShared.gmf",saved=directory+"/Caller.script",report=directory+"/Values.txt";
      const QString original="function [Mark] = ReadShared()\nCreate "+type+" PrivateOnly;\n"+setting("PrivateOnly",second)+settings("PrivateOnly")+"Create Variable Mark Keep;\nKeep = 103;\n"+globals+"BeginMissionSequence;\nGlobal Shared;\nWrite Shared."+field+" {Style = Verbose, MessageWindow = true, LogFile = false};\nWrite PrivateOnly."+field+" {Style = Verbose, MessageWindow = true, LogFile = false};\nMark = Keep;\n% Private resource Ω\n";
      const auto edited=QString(original).replace("Keep = 103;","Keep = 41;");
      const QString outerBody="function [Mark] = NestedShared()\nCreate Variable Mark;\nCreate GmatFunction ReadShared;\nReadShared.FunctionPath = '"+function+"';\n"+globals+"BeginMissionSequence;\nGlobal Shared;\n[Mark] = ReadShared();\n";
      write(function,original.toUtf8()); write(outer,outerBody.toUtf8()); rig.save("initial.gmf",original.toUtf8()); rig.save("nested.gmf",outerBody.toUtf8());
      const QString caller=dependencies+"Create "+type+" Shared Shadow;\n"+setting("Shared",first)+setting("Shadow",second)+settings("Shared")+settings("Shadow")+"Create Variable First Second Third Keep;\nKeep = 103;\nCreate GmatFunction ReadShared NestedShared;\nReadShared.FunctionPath = '"+function+"';\nNestedShared.FunctionPath = '"+outer+"';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\nBeginMissionSequence;\n"+globals+"Global Shared;\nWrite Shared."+field+" {Style = Verbose, MessageWindow = true, LogFile = false};\nWrite Shadow."+field+" {Style = Verbose, MessageWindow = true, LogFile = false};\n[First] = ReadShared();\n[Second] = ReadShared();\n[Third] = NestedShared();\nReport Values First Second Third Keep;\nKeep = Keep + 1;\nReport Values Keep;\n";
      rig.source->setPlainText(caller); rig.save("caller.script",caller.toUtf8()); const bool built=rig.window.buildScript(); rig.save("build-messages.txt",rig.messages->toPlainText().toUtf8());
      if (!built) { std::cerr<<rig.messages->toPlainText().toStdString(); QJsonArray errors; for (const auto &e:Moderator::GetScriptInterpreter()->GetErrorList()) { errors.append(QString::fromStdString(e)); std::cerr<<e<<std::endl; } rig.save("build-diagnostics.json",QJsonDocument(QJsonObject{{"errors",errors},{"status",rig.window.statusBar()->currentMessage()}}).toJson()); } require(built,"Remaining resource caller Build failed");
      auto native=[&](GmatBase *object)->QJsonValue {
         require(object && object->GetTypeName()==nativeType.toStdString(),"Remaining resource native type differs from its source-backed factory route");
         if (kind=="real") return object->GetRealParameter(field.toStdString());
         if (kind=="integer") return object->GetIntegerParameter(field.toStdString());
         if (kind=="boolean") return object->GetBooleanParameter(field.toStdString());
         if (kind=="vector") { const auto &values=object->GetRvectorParameter(field.toStdString()); QJsonArray v; for (int i=0;i<values.GetSize();++i) v.append(values[i]); return v; }
         return QString::fromStdString(kind=="onoff"?object->GetOnOffParameter(field.toStdString()):object->GetStringParameter(field.toStdString()));
      };
      auto prototypes=[&]{QJsonObject values; for (const auto &name:QStringList(sameNameShadow?QStringList{"Shared","Shadow","PrivateOnly"}:QStringList{"Shared","Shadow"})) { auto *p=Moderator::Instance()->GetConfiguredObject(name.toStdString()); require(p,"Remaining prototype absent"); values[name]=QString::fromStdString(p->GetGeneratingString(Gmat::NO_COMMENTS)); } return values;};
      const auto before=prototypes(); rig.save("prototypes-before.json",QJsonDocument(before).toJson());
      require(same(native(Moderator::Instance()->GetConfiguredObject("Shared")),expectedFirst) && same(native(Moderator::Instance()->GetConfiguredObject("Shadow")),expectedSecond),"Remaining main literal seeds differ");
      for (auto *tabs:rig.window.findChildren<QTabWidget*>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==rig.resources || tabs->widget(i)->isAncestorOf(rig.resources)) tabs->setCurrentIndex(i);
      const auto entries=rig.resources->findItems("ReadShared",Qt::MatchExactly|Qt::MatchRecursive); require(entries.size()==1,"Remaining function resource absent"); rig.resources->scrollToItem(entries[0]); rig.resources->itemDoubleClicked(entries[0],0); events();
      auto *owner=rig.area->activeSubWindow(); auto *panel=owner?dynamic_cast<ResourceEditor*>(owner->widget()):nullptr; require(panel,"Remaining function editor absent"); owner->showNormal(); owner->move(0,0); owner->resize(rig.area->viewport()->size()); events(); auto *edit=panel->findChild<QPushButton*>("editFunctionFile"); reveal(edit);
      const int history=rig.source->document()->availableUndoSteps(); QJsonArray decisions;
      for (bool commit:{false,true})
      {
         bool visited=false; std::exception_ptr failure;
         QTimer::singleShot(0,panel,[&]{auto *dialog=panel->findChild<QDialog*>("functionFileDialog"); try { require(dialog,"Remaining file dialog absent"); auto *text=dialog->findChild<QPlainTextEdit*>("functionFileText"); require(text && !text->isReadOnly() && text->toPlainText()==original,"Remaining initial file differs"); reveal(text); QTest::mouseClick(text->viewport(),Qt::LeftButton); QTest::keyClick(text,Qt::Key_A,Qt::ControlModifier); QApplication::clipboard()->setText(edited); QTest::keyClick(text,Qt::Key_V,Qt::ControlModifier); events(); require(text->toPlainText()==edited && read(function)==original.toUtf8() && rig.source->toPlainText()==caller && rig.source->document()->availableUndoSteps()==history && prototypes()==before,"Remaining draft changed file/caller/history/prototypes"); auto *button=dialog->findChild<QDialogButtonBox*>()->button(commit?QDialogButtonBox::Save:QDialogButtonBox::Cancel); reveal(button); QTest::mouseClick(button,Qt::LeftButton); visited=true; } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }});
         QTest::mouseClick(edit,Qt::LeftButton); if (failure) std::rethrow_exception(failure);
         require(visited && read(function)==(commit?edited:original).toUtf8() && rig.source->toPlainText()==caller,"Remaining file Save/Cancel differs"); decisions.append(QJsonObject{{"saved",commit}});
      }
      require(owner->close(),"Remaining editor failed to close"); events(); rig.save("edited.gmf",edited.toUtf8()); require(rig.window.saveScriptTo(saved),"Remaining Unicode caller Save failed"); QJsonArray phases;
      for (int phase=0;phase<3;++phase)
      {
         if (phase==1) require(rig.window.loadScript(saved) && rig.window.buildScript() && rig.source->toPlainText()==caller && prototypes()==before,"Remaining Unicode reopen changed source/prototypes");
         const int undo=rig.source->document()->availableUndoSteps(); rig.messages->clear(); acceptanceView(rig.window); const auto outcome=rig.window.runMission(); events(); const auto tag=QString::number(phase); const auto messages=rig.messages->toPlainText(); rig.save(tag+"-messages.txt",messages.toUtf8());
         if (outcome!=MainWindow::RunResult::Completed) std::cerr<<messages.toStdString(); require(outcome==MainWindow::RunResult::Completed,"Remaining main/local/global mission failed");
         auto *shared=Moderator::Instance()->GetInternalObject("Shared"),*shadow=Moderator::Instance()->GetInternalObject("Shadow"),*configured=Moderator::Instance()->GetConfiguredObject("Shared");
         require(shared && shadow && shared!=shadow && shared!=configured && shared->IsGlobal() && !shared->IsLocal() && !shadow->IsGlobal() && same(native(shared),expectedFirst) && same(native(shadow),expectedSecond),"Remaining global/main native identity/flags/literal field differs");
         bool privateAbsent=false; try { privateAbsent=Moderator::Instance()->GetInternalObject("PrivateOnly")==nullptr; } catch (SandboxException &e) { require(QString::fromStdString(e.GetFullMessage()).contains("Could not find \"PrivateOnly\""),"Unexpected private-resource lookup error"); privateAbsent=true; }
         if (sameNameShadow) { auto *shadowed=Moderator::Instance()->GetInternalObject("PrivateOnly"); auto *prototype=Moderator::Instance()->GetConfiguredObject("PrivateOnly"); require(shadowed && prototype && shadowed->GetTypeName()=="Spacecraft" && shadowed->GetRealParameter("DryMass")==987 && prototype->GetRealParameter("DryMass")==987,"Function-private resource changed its different-type caller shadow"); }
         else require(privateAbsent && !Moderator::Instance()->GetConfiguredObject("PrivateOnly"),"Function private resource leaked into caller configuration/runtime");
         auto checkedWrites=[&](const QString &name,const QJsonValue &wanted,int count){QJsonArray values; auto matches=QRegularExpression("(?:^|\\n)"+name+"\\."+QRegularExpression::escape(field)+" =\\s*\\n([^\\n]*)").globalMatch(messages); while (matches.hasNext()) { auto value=parse(matches.next().captured(1)); require(same(value,wanted),"Remaining Write differs from independent literal field"); values.append(value); } require(values.size()==count,"Remaining main/global/private Write count differs"); return values;};
         const auto sharedWrites=checkedWrites("Shared",expectedFirst,4),privateWrites=checkedWrites("PrivateOnly",expectedSecond,3),shadowWrites=checkedWrites("Shadow",expectedSecond,1);
         QJsonArray identities;
         for (auto *command=Moderator::Instance()->GetFirstCommand();command;command=command->GetNext())
         {
            if (!command->IsOfType("CallFunction")) continue; auto *functionObject=dynamic_cast<ObjectManagedFunction*>(command->GetRefObject(Gmat::FUNCTION,"")); require(functionObject && functionObject->GetGlobalObjectStore(),"Remaining actual function global store absent"); const auto *store=functionObject->GetGlobalObjectStore(); const auto found=store->find("Shared"); require(found!=store->end() && found->second==shared,"Remaining function global identity differs"); identities.append(QJsonObject{{"function",QString::fromStdString(functionObject->GetName())},{"global_store_object_pointer",pointer(found->second)},{"global_object_pointer",pointer(shared)},{"configured_prototype_pointer",pointer(configured)}});
         }
         require(identities.size()==3,"Remaining direct/direct/nested function identity absent");
         const auto raw=read(report); rig.save(tag+"-values.txt",raw); const auto words=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(words.size()==5,"Remaining raw continuation width differs"); QJsonArray values; for (const auto &word:words) { bool ok; const double v=word.toDouble(&ok); require(ok && std::isfinite(v),"Remaining raw marker invalid"); values.append(v); }
         require(values==QJsonArray{41,41,41,103,104},"Remaining private/caller sentinel or continuation differs");
         const auto after=prototypes(); rig.save(tag+"-prototypes.json",QJsonDocument(after).toJson()); rig.save(tag+"-invariants.json",QJsonDocument(QJsonObject{{"prototypes",after==before},{"source",rig.source->toPlainText()==caller},{"history",rig.source->document()->availableUndoSteps()==undo},{"function_file",read(function)==edited.toUtf8()},{"nested_file",read(outer)==outerBody.toUtf8()},{"not_running",!rig.window.isRunning()}}).toJson());
         require(after==before && rig.source->toPlainText()==caller && rig.source->document()->availableUndoSteps()==undo && read(function)==edited.toUtf8() && read(outer)==outerBody.toUtf8() && !rig.window.isRunning(),"Remaining execution changed source/history/prototypes/function files/lock");
         for (auto i=inputFiles.begin();i!=inputFiles.end();++i) require(read(directory+"/"+i.key())==i.value().toString().toUtf8(),"Remaining input file changed");
         phases.append(QJsonObject{{"phase",phase},{"explicit_rebuild",phase<2},{"shared_value",native(shared)},{"shadow_value",native(shadow)},{"shared_writes",sharedWrites},{"private_writes",privateWrites},{"shadow_writes",shadowWrites},{"global",shared->IsGlobal()},{"local",shared->IsLocal()},{"private_leaked",false},{"identities",identities},{"raw_values",values}}); rig.save("phases.json",QJsonDocument(phases).toJson());
      }
      auto *tree=rig.window.findChild<QTreeWidget*>("Output"); const auto outputs=tree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(outputs.size()==1,"Remaining report output absent"); tree->itemDoubleClicked(outputs[0],0); events(); auto *viewer=rig.area->activeSubWindow(); require(viewer && dynamic_cast<ReportViewer*>(viewer->widget()),"Remaining actual report viewer absent"); auto *shown=viewer->findChild<QPlainTextEdit*>(); require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(read(report)).trimmed(),"Remaining viewer differs from checked bytes"); acceptanceView(*viewer);
      const QJsonObject proof{{"checkpoint","remaining-resource-main-local-global"},{"outcome","passed"},{"case",key},{"type",type},{"native_type",nativeType},{"field",field},{"kind",kind},{"read_only",readOnly},{"caller_same_name_shadow",sameNameShadow},{"first",expectedFirst},{"second",expectedSecond},{"input_files",inputFiles},{"missions",3},{"function_calls",12},{"file_decisions",decisions},{"phases",phases},{"scope","Selected remaining factory candidate with actual named main and function-private creation/use plus explicit Global in main/direct/nested functions. Independent typed fields and Write literals,private-name/caller/prototype isolation,global-store identity,actual file Cancel/Save,Unicode reopen,no-explicit-Build repeat and report viewer. No mandatory whole-resource output,all properties/operations,complete resource universe,provider execution/science or exact requirement qualification."}};
      rig.save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
   }
   catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
   catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
   return 1;
}
