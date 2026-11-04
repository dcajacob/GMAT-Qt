#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "CommandEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QScrollArea>
#include <QRegularExpression>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <QTest>
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static void events(){QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static QByteArray read(const QString &path){QFile f(path);require(f.open(QIODevice::ReadOnly),"External plug-in evidence unreadable");return f.readAll();}
static void write(const QString &path,const QByteArray &data){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"External plug-in evidence write failed");}
static bool offered(UnsignedInt type,const std::string &name){const auto &items=Moderator::Instance()->GetListOfFactoryItems(type);return std::find(items.begin(),items.end(),name)!=items.end();}
static void text(QPlainTextEdit *editor,const QString &value){require(editor&&editor->isVisible()&&!editor->isReadOnly(),"External plug-in source editor unavailable");editor->setFocus();QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QApplication::clipboard()->setText(value);QTest::keyClick(editor,Qt::Key_V,Qt::ControlModifier);events();require(editor->toPlainText()==value,"External plug-in keyboard source replacement differs");}
static void cell(QWidget *owner,const QString &name,const QString &value){
 require(owner,"External plug-in property owner missing");auto *entry=owner->findChild<QLineEdit *>("resource_"+name);require(entry,"External plug-in typed property control missing");
 for(QWidget *p=entry;p;p=p->parentWidget()){
  for(auto *tabs:owner->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
  if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(entry);
 }
 events();require(entry->isVisible()&&entry->isEnabled()&&!entry->isReadOnly(),"External plug-in property control not exposed/editable");acceptanceView(*entry);entry->setFocus();QTest::keyClick(entry,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(entry,value);QTest::keyClick(entry,Qt::Key_Tab);events();require(entry->text()==value,"External plug-in keyboard property differs");acceptanceView(*entry);
}

int main(int argc,char **argv){
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaExternalPlugin");
 try{
  require(argc==3&&acceptanceWidgetPlatform(),"Supply external plug-in startup and mode");TestSettings settings;QTemporaryDir files;require(files.isValid(),"External plug-in temporary folder missing");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath();const QString mode=argv[2];require(QStringList{"Enabled","Absent","Missing"}.contains(mode),"Unknown external plug-in mode");QDir::setCurrent(QFileInfo(startup).absolutePath());
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &data){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),data);};
  MainWindow window;window.resize(1100,850);window.show();require(window.initialize(startup),"External plug-in startup initialization failed");acceptanceView(window);
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *resources=window.findChild<QTreeWidget *>("Resources"),*mission=window.findChild<QTreeWidget *>("Mission"),*output=window.findChild<QTreeWidget *>("Output");auto *area=window.findChild<QMdiArea *>("workspace");auto *create=window.findChild<QAction *>("createResource");require(source&&messages&&resources&&mission&&output&&area&&create,"External plug-in GUI controls missing");save("startup-messages.txt",messages->toPlainText().toUtf8());
  const bool enabled=mode=="Enabled";require(offered(Gmat::HARDWARE,"AcceptanceProbe")==enabled&&offered(Gmat::COMMAND,"AcceptanceEvaluate")==enabled,"External factories do not match startup selection");
  const auto report=files.filePath("Plug-in results Ω.csv"),saved=files.filePath("Plug-in mission Ω.script");
  const QString baseline="% Preserve unrelated mission Ω\nCreate Variable Result Keep;\nResult = -901;\nKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nKeep = Keep + 1;\nReport 'PluginWitness' Values Result Keep;\n% Retained tail Ω\n";
  text(source,baseline);require(window.buildScript(),"External baseline build failed");const int baselineUndo=source->document()->availableUndoSteps();QJsonArray transactions,runs;
  auto creation=[&](bool accept){bool visited=false;std::exception_ptr failure;QTimer::singleShot(0,&window,[&]{auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());try{
   require(dialog&&dialog->objectName()=="newResourceDialog","External actual New resource dialog missing");auto *types=dialog->findChild<QComboBox *>("resourceType");require(types&&(types->findText("AcceptanceProbe")>=0)==enabled,"External resource discovery differs in GUI");acceptanceView(*dialog);
   if(!enabled){dialog->reject();visited=true;return;}
   types->setCurrentText("AcceptanceProbe");auto *name=dialog->findChild<QLineEdit *>("resourceName");require(name,"External resource name editor missing");name->setFocus();QTest::keyClick(name,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(name,"Probe");events();
   ResourceEditor *panel=nullptr;for(auto *candidate:dialog->findChildren<QWidget *>())if(auto *form=dynamic_cast<ResourceEditor *>(candidate);form&&form->isVisible()){require(!panel,"Multiple visible external resource forms");panel=form;}if(!panel)std::cerr<<"CREATION_STATUS "<<dialog->findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<std::endl;cell(panel,"Input","4.5");cell(panel,"Gain","2");cell(panel,"Bias","-1");require(source->toPlainText()==baseline&&source->document()->availableUndoSteps()==baselineUndo&&!Moderator::Instance()->GetConfiguredObject("Probe"),"Pending plug-in creation mutated caller/model");acceptanceView(*dialog);
   if(accept){auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons");require(buttons,"External resource creation buttons missing");QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);events();require(Moderator::Instance()->GetConfiguredObject("Probe"),"External resource Apply did not create object");if(dialog->isVisible())dialog->accept();}else dialog->reject();visited=true;
  }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});create->trigger();events();if(failure)std::rethrow_exception(failure);require(visited,"External New resource route was not exercised");};
  creation(false);require(source->toPlainText()==baseline&&!Moderator::Instance()->GetConfiguredObject("Probe"),"External resource Cancel modified source/model");
  auto run=[&](const QString &phase,double result,double keep){const auto accepted=source->toPlainText();const int undo=source->document()->availableUndoSteps();acceptanceView(window);messages->clear();const auto status=window.runMission();events();save(phase+"-messages.txt",messages->toPlainText().toUtf8());if(status!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(status==MainWindow::RunResult::Completed,"External plug-in mission failed");const auto raw=read(report);save(phase+"-values.csv",raw);const auto fields=QString::fromUtf8(raw).trimmed().split(',');require(fields.size()==2&&fields[0].toDouble()==result&&fields[1].toDouble()==keep,"External result/report differs from independent affine oracle");require(Moderator::Instance()->GetInternalObject("Result")->GetRealParameter("Value")==result&&Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==keep,"External native result differs from checked raw report");require(source->toPlainText()==accepted&&source->document()->availableUndoSteps()==undo&&!window.isRunning()&&window.findChild<QAction *>("runMission")->isEnabled(),"External run changed source/history/lock");runs.append(QJsonObject{{"phase",phase},{"result",result},{"keep",keep},{"source_history_preserved",true}});};
  if(!enabled){
   const auto unknownResource="Create AcceptanceProbe Ghost;\n"+baseline;text(source,unknownResource);require(!window.buildScript()&&source->toPlainText()==unknownResource&&!Moderator::Instance()->GetConfiguredObject("Ghost"),"Unavailable external resource was accepted");save("rejected-resource-messages.txt",messages->toPlainText().toUtf8());
   const auto unknownCommand=QString(baseline).replace("Keep = Keep + 1;","AcceptanceEvaluate Ghost Result;\nKeep = Keep + 1;");text(source,unknownCommand);require(!window.buildScript()&&source->toPlainText()==unknownCommand,"Unavailable external command was accepted");save("rejected-command-messages.txt",messages->toPlainText().toUtf8());
   text(source,baseline);require(window.buildScript(),"Unavailable external plug-in did not recover to ordinary mission");run("recovered-baseline",-901,42);transactions.append(QJsonObject{{"unavailable_factories_rejected",true},{"ordinary_mission_recovered",true}});
  }else{
   creation(true);const auto created=source->toPlainText();save("created.script",created.toUtf8());QString unrelated=created;unrelated.remove("Create AcceptanceProbe Probe;\n");unrelated.remove(QRegularExpression("(?m)^(?:GMAT )?Probe\\.(?:Input|Gain|Bias) = [^;\\n]+;\\n"));unrelated.replace("\n\nBeginMissionSequence;","\nBeginMissionSequence;");require(unrelated==baseline&&created.count("Create AcceptanceProbe Probe;")==1&&resources->findItems("Probe",Qt::MatchExactly|Qt::MatchRecursive).size()==1,"External creation changed unrelated source or tree identity");
   auto check=[&](double input,double gain,double bias){auto *p=Moderator::Instance()->GetConfiguredObject("Probe");require(p&&p->IsOfType("AcceptanceProbe")&&p->GetRealParameter("Input")==input&&p->GetRealParameter("Gain")==gain&&p->GetRealParameter("Bias")==bias,"External native metadata values differ");};check(4.5,2,-1);
   source->undo();require(source->toPlainText()==baseline&&window.buildScript()&&!Moderator::Instance()->GetConfiguredObject("Probe"),"External creation Undo failed");source->redo();require(source->toPlainText()==created&&window.buildScript(),"External creation Redo failed");check(4.5,2,-1);transactions.append(QJsonObject{{"Cancel_isolated",true},{"creation_Undo_Redo_exact",true}});
   const auto callable=QString(created).replace("Keep = Keep + 1;","AcceptanceEvaluate Probe Result;\nKeep = Keep + 1;");text(source,callable);require(window.buildScript(),"External command failed to parse/build");
   auto selectTab=[&](const QString &name){for(auto *tabs:window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->tabText(i)==name)tabs->setCurrentIndex(i);events();};
   selectTab("Mission");QTreeWidgetItem *command=nullptr;for(QTreeWidgetItemIterator it(mission);*it;++it)if((*it)->text(0).contains("AcceptanceEvaluate"))command=*it;require(command,"External command absent from Mission Tree");mission->expandAll();mission->setCurrentItem(command);mission->scrollToItem(command);acceptanceView(window);mission->itemDoubleClicked(command,0);events();auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"External command actual editor missing");child->showNormal();child->resize(area->viewport()->size());child->move(0,0);acceptanceView(*child);auto *commandSource=child->findChild<QPlainTextEdit *>("commandSource");require(commandSource&&commandSource->toPlainText().contains("AcceptanceEvaluate Probe Result"),"External generic command source missing");require(child->close(),"External command editor close failed");events();run("initial",8,42);check(4.5,2,-1);
   selectTab("Resources");auto items=resources->findItems("Probe",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"External resource editor target missing");resources->expandAll();resources->setCurrentItem(items[0]);resources->scrollToItem(items[0]);acceptanceView(window);resources->setFocus();QTest::keyClick(resources,Qt::Key_Return);events();child=area->activeSubWindow();require(child&&dynamic_cast<ResourceEditor *>(child->widget()),"External actual resource editor missing");child->showNormal();child->resize(area->viewport()->size());child->move(0,0);acceptanceView(*child);auto *table=child->findChild<QTableWidget *>("resourceProperties");auto *buttons=child->findChild<QDialogButtonBox *>("resourceButtons");require(table&&buttons,"External resource form controls missing");const auto before=source->toPlainText();const int undo=source->document()->availableUndoSteps();cell(child,"Input","101");QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo,"Invalid external property mutated source/history");check(4.5,2,-1);save("invalid-property-status.txt",child->findChild<QLabel *>("resourceStatus")->text().toUtf8());
   cell(child,"Input","-3");cell(child,"Gain","4");cell(child,"Bias","0.5");acceptanceView(*child);QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();check(-3,4,.5);require(child->close(),"External applied resource editor failed to close");events();run("edited",-11.5,42);check(-3,4,.5);
   const auto accepted=source->toPlainText();require(window.saveScriptTo(saved)&&read(saved)==accepted.toUtf8()&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==accepted,"External Unicode save/reopen differs");save("accepted.script",read(saved));check(-3,4,.5);run("reopened",-11.5,42);
   auto reports=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"External report tree item missing");output->itemDoubleClicked(reports[0],0);events();child=area->activeSubWindow();require(child&&dynamic_cast<ReportViewer *>(child->widget()),"External actual report viewer missing");acceptanceView(*child);auto *shown=child->findChild<QPlainTextEdit *>();require(shown&&shown->isReadOnly()&&shown->toPlainText().trimmed()==QString::fromUtf8(read(report)).trimmed(),"External report viewer differs from checked raw result");transactions.append(QJsonObject{{"invalid_native_property_rejected_atomically",true},{"actual_resource_and_command_editors",true},{"Unicode_save_reopen_exact",true},{"configured_prototypes_preserved",true},{"actual_checked_report_viewer",true}});
  }
  const QJsonObject proof{{"checkpoint","external-plugin-"+mode},{"outcome","passed"},{"mode",mode},{"external_factories_registered",enabled},{"plugin_linked_into_gui",false},{"resource_type","AcceptanceProbe"},{"command_type","AcceptanceEvaluate"},{"runs",runs},{"transactions",transactions},{"facets",QJsonArray{"private-startup-external-shared-library","native-factory-and-GUI-discovery","independent-native-result-and-raw-report","source-history-and-configuration-isolation"}},{"scope","Selected test-only external resource and command loaded through documented startup mechanism. Not arbitrary third-party plugins,ABI/compiler/platform compatibility,hot reload,malicious-library isolation,scientific models,native OS input,hardware/GPU/stress or current private NASA provenance."}};save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
