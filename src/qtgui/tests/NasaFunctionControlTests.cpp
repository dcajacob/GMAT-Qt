#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "CommandEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Parameter.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static QByteArray read(const QString &path) {QFile file(path);require(file.open(QIODevice::ReadOnly),"Function control evidence missing");return file.readAll();}
static void write(const QString &path,const QByteArray &bytes) {QFile file(path);require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Function control evidence write failed");}
static double value(const char *name) {auto *object=dynamic_cast<Parameter *>(Moderator::Instance()->GetInternalObject(name));require(object,"Function runtime parameter unavailable");return object->EvaluateReal();}
static void reveal(QWidget *control) {
   require(control,"Function control widget missing");
   for (QWidget *page=control;page;page=page->parentWidget()) {
      for (auto *tabs:control->window()->findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
      if (auto *scroll=qobject_cast<QScrollArea *>(page)) scroll->ensureWidgetVisible(control);
   }
   events();require(control->isVisible() && control->isEnabled(),"Function control widget unreachable");
}
struct Fixture {QString accept,other,mission;QJsonArray summary,trace;};
static Fixture fixture(const QString &mode,const QString &accept,const QString &other)
{
   Fixture f;
   if (mode=="Recursion") {
      f.accept="function [out] = Accept(depth)\nCreate GmatFunction Accept;\nAccept.FunctionPath = '"+accept+"';\nCreate Variable out next child;\nGlobal Count;\nBeginMissionSequence;\nCount = Count + 1;\nIf depth <= 0\n   out = 1;\nElse\n   next = depth - 1;\n   [child] = Accept(next);\n   out = depth * child;\nEndIf;\n";
      int count=0;for (int depth:{0,1,5}) {int factorial=1;for (int i=1;i<=depth;++i) factorial*=i;count+=depth+1;f.summary.append(depth);f.summary.append(factorial);f.summary.append(count);f.mission+="Depth = "+QString::number(depth)+";\n'"+QString(depth==0 ? "Before":"Next")+"' [Result] = Accept(Depth);\nReport Summary Depth Result Count;\n";}
   } else if (mode=="Mutual") {
      f.accept="function [out] = Accept(depth)\nCreate GmatFunction Other;\nOther.FunctionPath = '"+other+"';\nCreate Variable out next child;\nGlobal CountA;\nBeginMissionSequence;\nCountA = CountA + 1;\nIf depth <= 0\n   out = 1;\nElse\n   next = depth - 1;\n   [child] = Other(next);\n   out = child + 2 * depth;\nEndIf;\n";
      f.other="function [out] = Other(depth)\nCreate GmatFunction Accept;\nAccept.FunctionPath = '"+accept+"';\nCreate Variable out next child;\nGlobal CountB;\nBeginMissionSequence;\nCountB = CountB + 1;\nIf depth <= 0\n   out = 10;\nElse\n   next = depth - 1;\n   [child] = Accept(next);\n   out = child + 3 * depth;\nEndIf;\n";
      int ca=0,cb=0;for (int depth:{0,1,6}) {int a=1,b=10;for (int i=1;i<=depth;++i) {const int old=a;a=b+2*i;b=old+3*i;}ca+=depth/2+1;cb+=(depth+1)/2;for (int x:{depth,a,ca,cb}) f.summary.append(x);f.mission+="Depth = "+QString::number(depth)+";\n'"+QString(depth==0 ? "Before":"Next")+"' [Result] = Accept(Depth);\nReport Summary Depth Result CountA CountB;\n";}
   } else if (mode=="Subscribers") {
      f.accept="function [out] = Accept(input)\nCreate Variable k out start;\nGlobal Trace Counter;\nBeginMissionSequence;\nstart = Counter;\nFor k = 1:3\n   Counter = Counter + input * k;\n   Report Trace Counter;\nEndFor;\nout = Counter - start;\n";
      f.mission="Report Trace Counter;\n'Before' [Result] = Accept(Number);\nReport Trace Counter;\nReport Summary Number Result Counter;\n";f.summary={3,18,28};f.trace={10,13,19,28,28};
   } else {
      require(QStringList{"PauseResume","Stop","PauseStop"}.contains(mode),"Unknown function control case");
      f.accept="function [out] = Accept(limit)\nCreate Variable k out;\nGlobal Progress;\nBeginMissionSequence;\nout = 0;\nFor k = 1:limit\n   Progress = Progress + 1;\n   out = out + 1;\nEndFor;\n";
      f.mission="'Before' [Result] = Accept(Limit);\nReport Summary Limit Result Progress;\n";
   }
   f.accept+="% Edited in the actual function-file GUI Δ\n";return f;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaFunctionControl");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and function-control case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Function-control directory unavailable");
      const QString mode=argv[2];const bool control=QStringList{"PauseResume","Stop","PauseStop"}.contains(mode),stop=mode=="Stop" || mode=="PauseStop";
      const auto folder=files.filePath("functions Δ");require(QDir().mkpath(folder),"Function-control Unicode folder failed");const auto accept=QDir(folder).filePath("Accept.gmf"),other=QDir(folder).filePath("Other.gmf"),summary=files.filePath("Summary Δ.txt"),trace=files.filePath("Trace Δ.txt"),path=files.filePath("Control Δ.script");const auto f=fixture(mode,accept,other);
      const QString originalBody=f.accept+"% Initial external revision\n";write(accept,originalBody.toUtf8());if (!f.other.isEmpty()) write(other,f.other.toUtf8());
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      QString initial="% Retain function-control acceptance Δ\nCreate GmatFunction Accept;\nAccept.FunctionPath = '"+accept+"';\nCreate Variable Depth Result Count CountA CountB Number Counter Progress Limit Keep;\nNumber = 3;\nCounter = 10;\nProgress = 0;\nLimit = 40000; % bounded long-function input\nCount = 0;\nCountA = 0;\nCountB = 0;\nKeep = 41; % preserve initializer\nCreate ReportFile Summary Trace;\nSummary.Filename = '"+summary+"';\nTrace.Filename = '"+trace+"';\nSummary.WriteHeaders = false;\nTrace.WriteHeaders = false;\nSummary.Precision = 16;\nTrace.Precision = 16;\nBeginMissionSequence;\nGlobal Count CountA CountB Trace Counter Progress;\n"+f.mission+"Keep = Keep + 1; % preserve mission\n";
      const auto destination=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes) {if (!destination.isEmpty()) write(QDir(destination).filePath(name),bytes);};save("initial.script",initial.toUtf8());save("accept-initial.gmf",originalBody.toUtf8());if (!f.other.isEmpty()) save("other.gmf",f.other.toUtf8());
      MainWindow window;require(window.initialize(startup),"Function-control initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *resources=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source && messages && resources && area,"Function-control workspace missing");source->setPlainText(initial);const bool built=window.buildScript();if (!built) std::cerr<<messages->toPlainText().toStdString();require(built,"Function-control baseline failed");const int initialUndo=source->document()->availableUndoSteps();
      auto view=[&](QMdiSubWindow *child) {if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);child->showNormal();events();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);child->raise();events();}acceptanceView(*child);};
      auto openResource=[&](const QString &name) {const auto items=resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Function-control resource tree item missing");resources->itemDoubleClicked(items.front(),0);events();auto *child=area->activeSubWindow();require(child && dynamic_cast<ResourceEditor *>(child->widget()),"Function-control actual resource editor missing");view(child);return child;};
      auto *child=openResource("Accept");auto *resource=dynamic_cast<ResourceEditor *>(child->widget());auto *edit=resource->findChild<QPushButton *>("editFunctionFile");reveal(edit);int fileCancels=0;
      for (bool commit:{false,true}) {
         acceptanceView(*child);std::exception_ptr error;
         QTimer::singleShot(0,resource,[&] {auto *dialog=resource->findChild<QDialog *>("functionFileDialog");try {require(dialog,"Function-control actual file editor missing");auto *text=dialog->findChild<QPlainTextEdit *>("functionFileText");require(text && text->toPlainText()==originalBody,"Function-control editor read wrong file");text->setPlainText(f.accept);text->document()->setModified(true);acceptanceView(*dialog);require(source->toPlainText()==initial && read(accept)==originalBody.toUtf8() && source->document()->availableUndoSteps()==initialUndo,"Function file draft escaped Save");dialog->findChild<QDialogButtonBox *>()->button(commit ? QDialogButtonBox::Save:QDialogButtonBox::Cancel)->click();require(dialog->result()==(commit ? QDialog::Accepted:QDialog::Rejected),"Function-control file Save/Cancel failed");if (!commit) ++fileCancels;}catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});
         edit->click();if (error) std::rethrow_exception(error);require(read(accept)==(commit ? f.accept:originalBody).toUtf8() && source->toPlainText()==initial && !resource->hasChanges(),"Function file Save/Cancel changed mission/path");
      }
      require(child->close(),"Function-control clean resource panel did not close");events();
      const auto snapshot=window.missionSnapshot();int index=-1;for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("'Before'")) index=i;require(index>=0 && snapshot.nodes[index].editable,"Function-control call source mapping unavailable");auto *tree=window.findChild<QTreeWidget *>("Mission");QTreeWidgetItemIterator it(tree);while (*it && (!(*it)->data(0,Qt::UserRole).isValid() || (*it)->data(0,Qt::UserRole).toInt()!=index)) ++it;require(*it,"Function-control mission item missing");tree->itemDoubleClicked(*it,0);events();child=area->activeSubWindow();auto *panel=dynamic_cast<CommandEditor *>(child->widget());require(panel,"Function-control actual command panel missing");view(child);auto *name=panel->findChild<QLineEdit *>("commandName");reveal(name);name->setText("Acceptance call");require(source->toPlainText()==initial && panel->hasChanges(),"Function-control command draft escaped Apply");acceptanceView(*child);panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();panel=dynamic_cast<CommandEditor *>(child->widget());if (panel && panel->hasChanges()) std::cerr<<panel->findChild<QLabel *>("commandStatus")->text().toStdString();require(panel && !panel->hasChanges(),"Function-control command Apply failed");const auto applied=source->toPlainText();require(applied==QString(initial).replace("'Before'","'Acceptance call'"),"Function-control Apply changed unrelated source");require(child->close(),"Function-control clean command did not close");events();source->undo();require(source->toPlainText()==initial && window.buildScript() && read(accept)==f.accept.toUtf8(),"Function-control exact Undo/file isolation failed");source->redo();require(source->toPlainText()==applied && window.buildScript(),"Function-control exact Redo failed");require(window.saveScriptTo(path) && window.loadScript(path) && window.buildScript() && source->toPlainText()==applied,"Function-control Unicode persistence failed");
      QJsonArray observations,reports;int phase=0,requested=0,held=0;double maximum=0;std::exception_ptr failure;auto *pauseAction=window.findChild<QAction *>("pauseMission"),*stopAction=window.findChild<QAction *>("stopMission"),*runAction=window.findChild<QAction *>("runMission");require(pauseAction && stopAction && runAction,"GUI run controls missing");QTimer timer,deadline;timer.setInterval(5);deadline.setSingleShot(true);deadline.setInterval(15000);QElapsedTimer holdClock;
      if (control) {
         QObject::connect(&timer,&QTimer::timeout,&window,[&] {
            if (!window.isRunning() || phase!=0) return;
            auto *progress=dynamic_cast<Parameter *>(Moderator::Instance()->GetInternalObject("Progress"));if (!progress) return;const double current=progress->EvaluateReal();if (!(current>0 && current<40000)) return;
            timer.stop();try {
               require(source->isReadOnly() && !resources->isEnabled() && pauseAction->isEnabled() && stopAction->isEnabled(),"Function run controls/editor locks inconsistent");requested=current;phase=1;
               if (mode=="Stop") {stopAction->trigger();require(!pauseAction->isEnabled() && !stopAction->isEnabled() && Moderator::Instance()->GetRunState()==Gmat::IDLE,"Function Stop action did not set its state");observations.append(QJsonObject{{"action","stop"},{"progress",current}});acceptanceView(window);return;}
               pauseAction->trigger();require(Moderator::Instance()->GetRunState()==Gmat::PAUSED && runAction->isEnabled() && !pauseAction->isEnabled(),"Function Pause action did not set its state");holdClock.start();observations.append(QJsonObject{{"action","pause"},{"progress",current}});acceptanceView(window);
               QTimer::singleShot(150,&window,[&] {try {held=value("Progress");require(holdClock.elapsed()>=100 && held==requested && window.isRunning() && Moderator::Instance()->GetRunState()==Gmat::PAUSED,"Function continued executing while GUI reported paused");observations.append(QJsonObject{{"action","held"},{"progress",held},{"elapsed_ms",int(holdClock.elapsed())}});acceptanceView(window);if (mode=="PauseStop") {stopAction->trigger();observations.append(QJsonObject{{"action","stop-paused"},{"progress",held}});}else {runAction->trigger();require(Moderator::Instance()->GetRunState()==Gmat::RUNNING && pauseAction->isEnabled() && !runAction->isEnabled(),"Function Resume action did not restore run state");observations.append(QJsonObject{{"action","resume"},{"progress",held}});}phase=2;}catch (...) {failure=std::current_exception();window.stopMission();}});
            }catch (...) {failure=std::current_exception();window.stopMission();}
         });
         QObject::connect(&deadline,&QTimer::timeout,&window,[&] {failure=std::make_exception_ptr(std::runtime_error("Function GUI control did not release execution"));window.stopMission();});
         timer.start();deadline.start();
      }
      messages->clear();const auto result=window.runMission();timer.stop();deadline.stop();events();save("mission.script",source->toPlainText().toUtf8());save("accept.gmf",read(accept));save("messages.txt",messages->toPlainText().toUtf8());if (failure) std::rethrow_exception(failure);
      require(!window.isRunning() && !source->isReadOnly() && resources->isEnabled() && !pauseAction->isEnabled() && !stopAction->isEnabled(),"Function execution did not release GUI controls");
      if (control) {require(requested>0 && requested<40000 && phase>0,"GUI actions were not processed inside the active function");require(result==(stop ? MainWindow::RunResult::Stopped:MainWindow::RunResult::Completed),"Function control returned wrong completion state");require(value("Keep")== (stop ? 41:42),"Stopped function executed later mission commands");if (stop) require(value("Progress")>=requested && value("Progress")<40000,"Function did not stop before its loop completed");else require(phase==2 && held==requested && value("Progress")==40000,"Paused function did not resume to its exact result");}
      else {if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed && value("Keep")==42,"Function-control numerical mission failed");}
      if (control) observations.append(QJsonObject{{"action","finished"},{"result",result==MainWindow::RunResult::Stopped ? "stopped":"completed"},{"progress",value("Progress")},{"keep",value("Keep")}});
      auto checkReport=[&](const QString &name,const QString &filename,const QJsonArray &expected) {
         const auto raw=read(filename);save(name+".txt",raw);const auto words=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==expected.size(),"Function-control report cardinality differs");QJsonArray actual;
         for (int i=0;i<words.size();++i) {bool ok=false;const double number=words[i].toDouble(&ok),error=std::abs(number-expected[i].toDouble());actual.append(number);maximum=std::max(maximum,error);if (!ok || error>1e-9) std::cerr<<"FUNCTION_CONTROL_DIFF "<<mode.toStdString()<<" component="<<i<<" actual="<<number<<" expected="<<expected[i].toDouble()<<std::endl;require(ok && std::isfinite(number) && error<=1e-9,"Function control result differs from independent recurrence/subscriber oracle");}
         auto *output=window.findChild<QTreeWidget *>("Output");const auto items=output->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Function-control output tree entry missing");output->itemDoubleClicked(items.front(),0);events();QMdiSubWindow *viewer=nullptr;for (auto *w:area->subWindowList()) if (dynamic_cast<ReportViewer *>(w->widget())) viewer=w;require(viewer,"Function-control actual report viewer missing");view(viewer);auto *text=viewer->findChild<QPlainTextEdit *>();require(text && text->isReadOnly() && text->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Function-control viewer differs from checked report");text->horizontalScrollBar()->setValue(text->horizontalScrollBar()->maximum());events();acceptanceView(*viewer);require(viewer->close(),"Function-control report did not close");events();reports.append(QJsonObject{{"name",name},{"actual",actual},{"expected",expected}});
      };
      if (control && stop) {
         require(source->toPlainText()==applied,"GUI Stop changed mission source");child=openResource("Limit");resource=dynamic_cast<ResourceEditor *>(child->widget());auto *limit=resource->findChild<QLineEdit *>("parameterValue");reveal(limit);limit->setText("37");require(source->toPlainText()==applied && resource->hasChanges(),"Stopped-function correction escaped Apply");acceptanceView(*child);resource->requestApply();events();resource=dynamic_cast<ResourceEditor *>(child->widget());require(resource && !resource->hasChanges(),"Stopped-function input correction failed");require(child->close(),"Corrected function input panel did not close");events();require(window.runMission()==MainWindow::RunResult::Completed && value("Keep")==42,"Stopped function did not recover after GUI input edit");save("recovered.script",source->toPlainText().toUtf8());checkReport("Summary",summary,QJsonArray{37,37,37});
      } else if (control) checkReport("Summary",summary,QJsonArray{40000,40000,40000});
      else {checkReport("Summary",summary,f.summary);if (mode=="Subscribers") checkReport("Trace",trace,f.trace);}
      const QJsonObject proof{{"checkpoint","function-control-"+mode},{"outcome","passed"},{"mode",mode},{"file_cancels",fileCancels},{"control_observations",observations},{"requested_progress",requested},{"held_progress",held},{"reports",reports},{"max_numeric_error",maximum},{"facets",QJsonArray{"actual-function-file-editor","exact-command-history-and-unicode","real-function-control-flow","independent-recurrence-or-subscriber-results","actual-report-viewer"}},{"scope","Bounded direct/mutual recursion depths, global ReportFile continuity and GUI pause/resume/stop during a finite pure-command function. Global/local variable and per-function paths are observed within these fixtures. No arbitrary recursion depth/stack, all subscriber types, general command/input/physical/native-input/platform/stress/current private NASA corpus or full-regression qualification."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
