#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ScriptEventDialog.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); QApplication::processEvents(); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Script-event artifact unavailable"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Script-event artifact write failed"); }
int main(int argc,char **argv) {
 QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("NasaScriptEvent");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Supply startup, event cases and id"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); const QString id=argv[3];QJsonObject spec;
  for (const auto &v:QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object()["cases"].toArray()) if (v.toObject()["id"].toString()==id) spec=v.toObject();require(!spec.isEmpty(),"Unknown script-event case");
  QDir::setCurrent(QFileInfo(startup).absolutePath());TestSettings settings;QTemporaryDir files;require(files.isValid(),"Script-event fixture unavailable");
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes) {if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes);};
  MainWindow window;require(window.initialize(startup),"Script-event runtime unavailable");window.show();events();acceptanceView(window);
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&area,"Script-event workspace missing");
  const auto report=files.filePath("event results Δ.csv");
  const QString prefix="% Retain source sentinel Ω\nCreate Variable V I X Keep;\nV = 0;\nI = -99;\nX = 0;\nKeep = 41;\nCreate DifferentialCorrector DC;\nCreate Yukon Opt;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\n";
  const QString before="BeginScript 'Event'; % fixed begin\nV = 1; % original body\nEndScript; % fixed end\n";
  const QString after="% updated comments Δ\n% second comment\nBeginScript 'Event'; % fixed begin\n"+spec["body"].toString()+"EndScript; % fixed end\n";
  const QString tail="Keep = Keep + 1; % outside event\nReport Values V I X Keep;\n% Retain tail Ω\n",baseline=prefix+before+tail,wanted=prefix+after+tail;
  source->setPlainText(baseline);require(window.buildScript(),"Script-event baseline failed");save("baseline.script",baseline.toUtf8());save("case.json",QJsonDocument(spec).toJson());
  auto model=[&] {QJsonArray values;for (const auto &name:QStringList{"V","I","X","Keep"}) {auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(object,"Script-event configured object lost");values.append(object->GetRealParameter("Value"));}return values;};const auto originalModel=model();
  auto open=[&] {
   auto *tabs=window.findChild<QTabWidget *>();for (int i=0;tabs&&i<tabs->count();++i) if (tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
   const auto snapshot=window.missionSnapshot();int index=-1;for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="BeginScript") {require(index<0,"More than one ScriptEvent icon");index=i;require(snapshot.nodes[i].children.isEmpty(),"Script-event body leaked into child commands");}
   require(index>=0,"ScriptEvent icon absent");QTreeWidgetItem *item=nullptr;for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index) item=*it;
   require(item&&item->childCount()==0,"Script-event GUI grouping differs from one command icon");tree->setCurrentItem(item);tree->scrollToItem(item);events();acceptanceView(window);tree->itemDoubleClicked(item,0);events();
   auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Actual event command editor missing");child->showNormal();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);child->raise();events();acceptanceView(*child);return child;
  };
  auto *child=open();auto *panel=dynamic_cast<CommandEditor *>(child->widget());auto *preview=panel->findChild<QPlainTextEdit *>("commandSource");auto *buttons=panel->findChild<QDialogButtonBox *>();require(preview&&buttons,"Event command controls missing");
  auto edit=[&](const QString &body,const QString &comments,bool accept) {
   auto *button=panel->findChild<QPushButton *>("editScriptEvent");require(button&&button->isVisible()&&button->isEnabled(),"Actual Script event button unavailable");bool visited=false;std::exception_ptr failure;
   QTimer::singleShot(0,child,[&] {auto *dialog=dynamic_cast<ScriptEventDialog *>(QApplication::activeModalWidget());try {require(dialog,"Actual ScriptEvent dialog missing");visited=true;
    require(dialog->findChild<QLabel *>("scriptEventBegin")->text()=="BeginScript 'Event'; % fixed begin"&&dialog->findChild<QLabel *>("scriptEventEnd")->text()=="EndScript; % fixed end","Fixed event boundaries changed");
    dialog->findChild<QPlainTextEdit *>("scriptEventBody")->setPlainText(body);dialog->findChild<QPlainTextEdit *>("scriptEventComments")->setPlainText(comments);events();acceptanceView(*dialog);dialog->findChild<QDialogButtonBox *>()->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel)->click();
   }catch (...) {failure=std::current_exception();if(dialog)dialog->reject();}});button->click();if(failure)std::rethrow_exception(failure);require(visited,"Script event dialog unvisited");events();acceptanceView(*child);
  };
  const auto originalDraft=preview->toPlainText();const int originalUndo=source->document()->availableUndoSteps();
  edit("V = -999;","discarded",false);require(preview->toPlainText()==originalDraft&&!panel->hasChanges()&&source->toPlainText()==baseline,"Event Cancel changed pending or applied source");
  QJsonArray rejected;
  auto reject=[&](const QString &kind) {
   const auto draft=preview->toPlainText();const auto hint=panel->findChild<QLabel *>("commandStatus")->text();buttons->button(QDialogButtonBox::Apply)->click();events();const auto diagnostic=panel->findChild<QLabel *>("commandStatus")->text();save(kind+".draft.txt",draft.toUtf8());save(kind+".diagnostic.txt",diagnostic.toUtf8());
   require(panel->hasChanges()&&preview->toPlainText()==draft&&source->toPlainText()==baseline&&source->document()->availableUndoSteps()==originalUndo&&window.missionSnapshot().sourceScript==baseline&&model()==originalModel,"Invalid event accepted or rejection changed draft/source/history/model");require(!diagnostic.isEmpty()&&diagnostic!=hint,"Invalid event has no diagnostic");rejected.append(QJsonObject{{"kind",kind},{"diagnostic",diagnostic},{"draft_retained",true},{"source_history_model_retained",true}});
  };
  edit(spec["unclosed"].toString(),"incomplete branch",true);reject("unclosed");
  preview->setPlainText(spec["crossed"].toString());events();acceptanceView(*child);reject("crossed");
  preview->setPlainText(originalDraft);events();edit(spec["body"].toString(),"updated comments Δ\nsecond comment",true);
  require(panel->hasChanges()&&source->toPlainText()==baseline&&preview->toPlainText().trimmed()==after.trimmed(),"Corrected event escaped pending state or lost exact boundaries/comments");
  buttons->button(QDialogButtonBox::Apply)->click();events();panel=dynamic_cast<CommandEditor *>(child->widget());require(panel,"Applied event editor missing");preview=panel->findChild<QPlainTextEdit *>("commandSource");buttons=panel->findChild<QDialogButtonBox *>();require(preview&&buttons,"Applied event controls missing");require(!panel->hasChanges()&&source->toPlainText()==wanted,"Corrected event Apply changed unrelated source");const auto appliedUndo=source->document()->availableUndoSteps();buttons->button(QDialogButtonBox::Apply)->click();events();require(source->document()->availableUndoSteps()==appliedUndo&&source->toPlainText()==wanted,"Repeated event Apply changed source/history");require(child->close(),"Clean event did not close");events();
  source->undo();require(source->toPlainText()==baseline&&window.buildScript()&&model()==originalModel,"Event Undo did not restore source/model");source->redo();require(source->toPlainText()==wanted&&window.buildScript(),"Event Redo did not restore source/model");
  const auto saved=files.filePath(id+" Δ.script"),copy=files.filePath(id+" copy ü.script");require(window.saveScriptTo(saved)&&read(saved)==wanted.toUtf8()&&window.saveScriptTo(copy)&&read(copy)==wanted.toUtf8()&&window.loadScript(copy)&&window.buildScript()&&source->toPlainText()==wanted,"Event Unicode Save/SaveAs/reopen changed exact source");save("accepted.script",source->toPlainText().toUtf8());child=open();require(child->close(),"Reopened clean event did not close");events();
  const int runUndo=source->document()->availableUndoSteps();QTimer watchdog;watchdog.setSingleShot(true);QObject::connect(&watchdog,&QTimer::timeout,&window,&MainWindow::stopMission);watchdog.start(10000);acceptanceView(window);const auto status=window.runMission();watchdog.stop();events();save("messages.txt",messages->toPlainText().toUtf8());if(status!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(status==MainWindow::RunResult::Completed,"Script event mission failed or watchdog stopped it");
  const auto raw=read(report);save("results.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==1,"Event result row count differs");const auto fields=lines.front().split(',');const auto oracle=spec["oracle"].toArray();require(fields.size()==oracle.size(),"Event result column count differs");QJsonArray observed;
  for(int i=0;i<fields.size();++i){bool ok=false;double v=fields[i].toDouble(&ok);require(ok&&std::isfinite(v)&&std::abs(v-oracle[i].toDouble())<1e-12,"Event branch result differs from independent oracle");observed.append(v);}
  require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==runUndo,"Event execution changed source/history");
  const QJsonObject proof{{"checkpoint","script-event-boundary"},{"outcome","passed"},{"case",id},{"observed",observed},{"rejected",rejected},{"single_icon",true},{"facets",QJsonArray{"actual-mission-tree-event-and-body-comment-dialog","fixed-boundaries-cancel-and-pending-apply","unclosed-and-crossed-branch-rejection-with-draft-source-history-model-retention","exact-idempotence-undo-redo-unicode-save-reopen","single-command-icon-grouping","independent-finite-branch-result"}},{"scope","Selected six contained branch families with scalar literal result checks; Target/Optimize use RunInitialGuess and do not establish solver convergence. Arbitrary command combinations, nested-event save normalization, native OS input, complete editor tooling, science/hardware/platform/stress/current private NASA provenance and complete current-build qualification remain open."}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
