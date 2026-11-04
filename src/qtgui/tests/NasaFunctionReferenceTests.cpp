#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "SandboxException.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
static void events(){QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static QByteArray read(const QString &path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Function resource input missing");return f.readAll();}
static void write(const QString &path,const QByteArray &bytes){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Function resource artifact write failed");}
static void reveal(QWidget *widget){
   require(widget,"Function resource control missing");
   for(auto *p=widget;p;p=p->parentWidget()){
      for(auto *tabs:widget->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(widget);
   }
   events();require(widget->isVisible()&&widget->isEnabled(),"Function resource control hidden/disabled");acceptanceView(*widget);
}
int main(int argc,char **argv){
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FunctionReferenceMatrix");
   try{
      require(argc==4&&acceptanceWidgetPlatform(),"Supply startup,resource catalog and case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Function resource fixture directory missing");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();const QString key=argv[3];QJsonObject spec;
      for(auto value:catalog["cases"].toArray())if(value.toObject()["key"].toString()==key)spec=value.toObject();require(!spec.isEmpty(),"Unknown function resource class");
      const QString type=spec["type"].toString(),field=spec["field"].toString(),kind=spec["kind"].toString(),first=spec["first"].toString(),second=spec["second"].toString();require(kind=="string"||kind=="list","Function reference matrix needs a string/list witness");
      const auto directory=files.filePath("Function resources Ω");require(QDir().mkpath(directory),"Function resource Unicode directory missing");const auto function=directory+"/MakeLocal.gmf",saved=files.filePath("Resource ownership Ω.script"),report=files.filePath("Continuation Ω.csv");
      const QString dependencies=spec["dependencies"].toString(),globalDependencies=spec["globals"].toString();
      auto literal=[&](const QString &v){return kind=="list"?"{"+v+"}":"'"+v+"'";};
      const auto otherFunction=directory+"/MakeOther.gmf";
      auto seed=[&](const QString &name){return spec["main_seeds"].toObject().value(name).toString(second);};
      const auto secondOutput=spec.value("second_output").toString(first),privateSeed=spec.value("private_seed").toString(second);
      auto settingsFor=[&](const QString &name){QString result;for(const auto &line:spec["object_settings"].toString().split('\n',Qt::SkipEmptyParts))result+=name+"."+line+"\n";return result;};
      auto body=[&](const QString &v){return "function [out] = MakeLocal()\nCreate "+type+" out LocalOnly;\nout."+field+" = "+literal(v)+";\nLocalOnly."+field+" = "+literal(privateSeed)+";\n"+settingsFor("out")+settingsFor("LocalOnly")+globalDependencies+"BeginMissionSequence;\n% Function-local referenced resource ownership Ω\n";};
      const auto originalBody=body(second),editedBody=body(first);write(function,originalBody.toUtf8());const auto otherBody=body(secondOutput).replace("MakeLocal()","MakeOther()");if(type=="Formation")write(otherFunction,otherBody.toUtf8());
      QString initial=dependencies+"% Function resource matrix Δ\nCreate "+type+" out First Second;\nout."+field+" = "+literal(seed("out"))+";\nFirst."+field+" = "+literal(seed("First"))+";\nSecond."+field+" = "+literal(seed("Second"))+";\n"+settingsFor("out")+settingsFor("First")+settingsFor("Second")+(type=="Formation"?"Create GmatFunction MakeOther;\nMakeOther.FunctionPath = '"+otherFunction+"';\n":QString())+"Create GmatFunction MakeLocal;\nMakeLocal.FunctionPath = '"+function+"';\nCreate Variable Keep;\nKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\nBeginMissionSequence;\n"+globalDependencies+"[First] = MakeLocal();\n[Second] = MakeLocal();\nReport Values Keep;\nKeep = Keep + 1;\nReport Values Keep;\n% Preserve function caller Ω\n";
      if(type=="Formation")initial.replace("[Second] = MakeLocal();","[Second] = MakeOther();");
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),bytes);};save("initial.script",initial.toUtf8());save("initial.gmf",originalBody.toUtf8());if(type=="Formation")save("second.gmf",otherBody.toUtf8());if(!dependencies.isEmpty())save("shared-dependencies.script",dependencies.toUtf8());
      MainWindow window;require(window.initialize(startup),"Function resource runtime initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *resources=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&resources&&area,"Function resource workspace missing");
      source->setPlainText(initial);if(!window.buildScript()){save("build-messages.txt",messages->toPlainText().toUtf8());save("build-status.txt",window.statusBar()->currentMessage().toUtf8());std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("Function resource initial script did not build");}
      auto value=[&](GmatBase *object){
         require(object&&object->GetTypeName()==type.toStdString(),"Function reference returned wrong/missing native type");
         if(kind=="list"){QStringList names;for(const auto &name:object->GetStringArrayParameter(field.toStdString()))names.append(QString::fromStdString(name));return names.join(',');}
         return QString::fromStdString(object->GetStringParameter(field.toStdString()));
      };
      for(auto name:{"out","First","Second"})require(value(Moderator::Instance()->GetConfiguredObject(name))==seed(name),"Function resource main prototype seed differs");
      auto items=resources->findItems("MakeLocal",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Function resource tree entry missing");resources->expandAll();resources->scrollToItem(items.first());resources->setCurrentItem(items.first());reveal(resources);resources->setFocus();QTest::keyClick(resources,Qt::Key_Return);events();auto *owner=area->activeSubWindow();auto *panel=owner?dynamic_cast<ResourceEditor *>(owner->widget()):nullptr;require(panel&&!panel->hasChanges(),"Function resource actual editor missing/dirty");
      auto *edit=panel->findChild<QPushButton *>("editFunctionFile");reveal(edit);const int undo=source->document()->availableUndoSteps();QJsonArray fileDecisions;
      for(bool commit:{false,true}){
         bool visited=false;std::exception_ptr failure;QTimer::singleShot(0,panel,[&]{
            auto *dialog=panel->findChild<QDialog *>("functionFileDialog");try{
               require(dialog,"Function resource file editor did not open");auto *text=dialog->findChild<QPlainTextEdit *>("functionFileText");require(text&&!text->isReadOnly()&&text->toPlainText()==originalBody,"Function resource initial file content differs");reveal(text);visited=true;
               QTest::mouseClick(text,Qt::LeftButton);QTest::keyClick(text,Qt::Key_A,Qt::ControlModifier);QApplication::clipboard()->setText(editedBody);QTest::keyClick(text,Qt::Key_V,Qt::ControlModifier);events();require(text->toPlainText()==editedBody&&read(function)==originalBody.toUtf8()&&source->toPlainText()==initial&&source->document()->availableUndoSteps()==undo,"Function resource edit escaped file Save/main isolation");acceptanceView(*dialog);
               auto *button=dialog->findChild<QDialogButtonBox *>()->button(commit?QDialogButtonBox::Save:QDialogButtonBox::Cancel);reveal(button);QTest::mouseClick(button,Qt::LeftButton);require(dialog->result()==(commit?QDialog::Accepted:QDialog::Rejected),"Function resource file decision differs");
            }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}
         });QTest::mouseClick(edit,Qt::LeftButton);if(failure)std::rethrow_exception(failure);require(visited,"Function resource file decision not exercised");require(read(function)==(commit?editedBody:originalBody).toUtf8()&&source->toPlainText()==initial&&source->document()->availableUndoSteps()==undo,"Function resource Save/Cancel changed wrong source/history");fileDecisions.append(QJsonObject{{"saved",commit}});
      }
      require(owner->close(),"Function resource owner did not close");events();save("edited.gmf",editedBody.toUtf8());require(window.buildScript()&&window.saveScriptTo(saved),"Function resource saved caller did not build");QJsonArray runs;
      for(const auto &phase:QStringList{"initial","reopened"}){
         if(phase=="reopened")require(window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==initial&&read(function)==editedBody.toUtf8(),"Function resource Unicode reload lost sources");
         const auto before=source->toPlainText();const int history=source->document()->availableUndoSteps();acceptanceView(window);messages->clear();const auto outcome=window.runMission();events();save(phase+"-messages.txt",messages->toPlainText().toUtf8());if(outcome!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(outcome==MainWindow::RunResult::Completed,"Function-local resource creation/output mission failed");
         auto *one=Moderator::Instance()->GetInternalObject("First"),*two=Moderator::Instance()->GetInternalObject("Second"),*shadow=Moderator::Instance()->GetInternalObject("out");require(one&&two&&shadow&&one!=two&&one!=shadow&&two!=shadow,"Function outputs alias caller objects");
         require(value(one)==first&&value(two)==secondOutput&&value(shadow)==seed("out"),"Function-local creation/output changed values or caller shadow");require(one->GetName()=="First"&&two->GetName()=="Second"&&shadow->GetName()=="out","Function object return changed caller identities");
         bool privateAbsent=false;
         try{privateAbsent=Moderator::Instance()->GetInternalObject("LocalOnly")==nullptr;}
         catch(SandboxException &error){require(QString::fromStdString(error.GetFullMessage()).contains("Could not find \"LocalOnly\""),"Unexpected sandbox error during private-resource lookup");privateAbsent=true;}
         require(privateAbsent&&!Moderator::Instance()->GetConfiguredObject("LocalOnly"),"Function-local resource leaked into caller scope");
         for(auto name:{"out","First","Second"})require(value(Moderator::Instance()->GetConfiguredObject(name))==seed(name),"Function resource run changed configured prototypes");
         QJsonArray referenceChecks;
         for(auto *object:{one,two,shadow}){
            QJsonArray names;const auto expected=value(object).split(',');
            if(type=="Antenna"){
               const auto name=expected.first();auto *ref=object->GetRefObject(Gmat::FIELD_OF_VIEW,name.toStdString());
               std::cerr<<"FOV_REF "<<object->GetName()<<" expected "<<name.toStdString()<<" actual="<<ref<<" shared="<<Moderator::Instance()->GetInternalObject(name.toStdString())<<std::endl;require(ref&&ref==Moderator::Instance()->GetInternalObject(name.toStdString())&&ref->GetTypeName()=="ConicalFOV","Returned Antenna lost its shared FOV reference");names.append(name);
            }else if(type=="Formation"){
               const auto members=object->GetRefObjectArray(Gmat::SPACECRAFT);require(int(members.size())==expected.size(),"Returned Formation native member count differs");
               for(int i=0;i<expected.size();++i){const auto name=expected[i];require(members[i]&&members[i]==Moderator::Instance()->GetInternalObject(name.toStdString())&&members[i]->GetName()==name.toStdString(),"Returned Formation lost shared member order/identity");names.append(name);}
            }else if(type=="Barycenter"||type=="LibrationPoint"){
               const auto bodies=type=="LibrationPoint"?QStringList{"Earth",expected.first()}:expected;
               auto *canonical=Moderator::Instance()->GetInternalObject("MainBodyIdentity");
               for(const auto &name:bodies){GmatBase *ref=nullptr,*mainRef=nullptr;
                  for(auto *candidate:{object,canonical})for(int j=0;j<candidate->GetIntegerParameter("NumberOfBodies");++j){auto *body=candidate->GetRefObject(Gmat::SPACE_POINT,"",j);if(body&&body->GetName()==name.toStdString()){if(candidate==object)ref=body;else mainRef=body;}}
                  require(ref&&mainRef&&ref==mainRef,"Returned calculated point lost caller automatic-global body reference");names.append(name);
               }
            }
            referenceChecks.append(QJsonObject{{"object",QString::fromStdString(object->GetName())},{"checked_reference_names",names},{"note",type=="FiniteBurn"?"Thruster name list only;native map is populated during firing,which this case does not execute.":"Native pointer identity checked for declared references;Receiver has no dependency witness."}});
         }
         const auto raw=read(report);save(phase+"-values.csv",raw);const auto rows=QString::fromUtf8(raw).trimmed().split('\n');require(rows.size()==2&&rows[0].trimmed()=="41"&&rows[1].trimmed()=="42","Function resource continuation report differs");require(source->toPlainText()==before&&source->document()->availableUndoSteps()==history&&read(function)==editedBody.toUtf8()&&!window.isRunning()&&window.findChild<QAction *>("runMission")->isEnabled(),"Function resource mission changed source/history/file/lock");
         runs.append(QJsonObject{{"phase",phase},{"reference_checks",referenceChecks},{"type",type},{"field",field},{"first",value(one)},{"second",value(two)},{"caller_shadow",value(shadow)},{"first_name",QString::fromStdString(one->GetName())},{"second_name",QString::fromStdString(two->GetName())},{"private_resource_leaked",false},{"configured_prototypes_unchanged",true}});save("native-results.json",QJsonDocument(runs).toJson());
      }
      auto *output=window.findChild<QTreeWidget *>("Output");auto reports=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"Function resource continuation output missing");output->itemDoubleClicked(reports.first(),0);events();auto *viewer=area->activeSubWindow();require(viewer&&dynamic_cast<ReportViewer *>(viewer->widget()),"Function resource report viewer missing");acceptanceView(*viewer);auto *shown=viewer->findChild<QPlainTextEdit *>();require(shown&&shown->isReadOnly()&&shown->toPlainText().trimmed()==QString::fromUtf8(read(report)).trimmed(),"Function resource viewer differs from checked bytes");
      const QJsonObject proof{{"checkpoint","function-local-reference-resource-"+key},{"outcome","passed"},{"case",key},{"type",type},{"field",field},{"function_calls",4},{"missions",2},{"file_decisions",fileDecisions},{"runs",runs},{"facets",QJsonArray{"actual-function-file-keyboard-edit-cancel-save","function-local-resource-creation-and-whole-object-outputs","caller-shadow-and-private-resource-isolation","distinct-output-identities-and-prototype-preservation","sequential-calls-and-Unicode-reload","actual-continuation-report-viewer","ordered-string-list-values-and-declared-shared-reference-identities"}},{"scope","Selected string/list/reference-dependent resource class;two private local resources,whole-object output to two distinct caller objects,unchanged same-name caller shadow and configured prototypes,private-name isolation,two sequential calls repeated after Unicode reload. FiniteBurn checks names without firing or initialized thruster-map proof. Shared dependencies are explicitly Global;no deep private graph/heap-lifetime proof. No universal all-resource/property/global-type matrix,dynamics,provider execution,OS input/GPU/platform/stress/current-private NASA or complete newest-build claim."}};save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
