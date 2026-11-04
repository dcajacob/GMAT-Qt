#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "BaseException.hpp"
#include "Debugger.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *text) { if(!ok)throw std::runtime_error(text); }
static void events() { QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile f(path);if(!f.exists())return {};require(f.open(QIODevice::ReadOnly),"Pen policy output unreadable");return f.readAll(); }
static QJsonArray rows(const QByteArray &bytes) {
 QJsonArray result;for(const auto &line:QString::fromUtf8(bytes).split('\n',Qt::SkipEmptyParts)){const auto fields=line.trimmed().split(',');require(fields.size()==2,"Pen policy report has wrong shape");QJsonArray row;for(const auto &field:fields){bool ok=false;const double value=field.toDouble(&ok);require(ok&&std::isfinite(value),"Pen policy report contains invalid number");row.append(value);}result.append(row);}return result;
}
int main(int argc,char **argv) {
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaPenScriptPolicy");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Supply startup and pen script-policy command");const QString command=argv[2],mode=argv[3];require(command=="PenUp"||command=="PenDown","Unknown pen policy command");require(mode=="Debug"||mode=="Plain","Unknown pen policy observation mode");const bool debugMode=mode=="Debug";
  TestSettings settings;QTemporaryDir files;require(files.isValid(),"Pen policy fixture unavailable");const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"),report=files.filePath("policy values Δ.csv");
  auto save=[&](const QString &name,const QByteArray &data){if(evidence.isEmpty())return;QFile f(QDir(evidence).filePath(name));require(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"Pen policy artifact write failed");};
  const QString invalid="% Preserve pen-policy source Δ\nCreate Variable Phase Keep;\nPhase = 0;\nKeep = 41;\nCreate XYPlot Trace;\nTrace.XVariable = Phase;\nTrace.YVariables = {Keep};\nTrace.ShowPlot = false;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nReport 'Before policy' Values Phase Keep;\n"+command+" 'Policy' Values;\n'After policy' Phase = 1;\nReport 'After witness' Values Phase Keep;\n% Preserve pen-policy tail Ω\n";
  QString valid=invalid;const auto bad=command+" 'Policy' Values;",good=command+" 'Policy' Trace;";require(valid.count(bad)==1,"Pen policy replacement anchor ambiguous");valid.replace(bad,good);
  MainWindow window;require(window.initialize(startup),"Pen policy runtime unavailable");window.show();events();acceptanceView(window);
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");require(source&&messages,"Pen policy GUI source/messages absent");source->setPlainText(invalid);save("invalid.script",invalid.toUtf8());
  auto invoke=[&](const QString &label){auto *bar=window.menuBar();QAction *menuAction=nullptr;for(auto *a:bar->actions())if(QString(a->text()).remove('&')=="Mission")menuAction=a;require(menuAction&&menuAction->menu(),"Pen policy Mission menu absent");acceptanceView(*bar);std::exception_ptr failure;bool visited=false;QTimer::singleShot(0,&window,[&]{auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());try{require(menu==menuAction->menu(),"Pen policy actual Run popup absent");QAction *chosen=nullptr;for(auto *a:menu->actions())if(QString(a->text()).remove('&')==label)chosen=a;require(chosen&&chosen->isEnabled(),"Pen policy menu command unavailable");acceptanceView(*menu);menu->setActiveAction(chosen);visited=true;QTest::keyClick(menu,Qt::Key_Return);}catch(...){failure=std::current_exception();if(menu)menu->close();}});QTest::mouseClick(bar,Qt::LeftButton,Qt::NoModifier,bar->actionGeometry(menuAction).center());events();if(failure)std::rethrow_exception(failure);require(visited,"Pen policy Run menu action did not execute");};
  Debugger *debug=nullptr;for(auto *object:window.children())if(auto *d=dynamic_cast<Debugger *>(object))debug=d;require(debug,"Pen policy debugger absent");
  QJsonArray observed;std::exception_ptr failure;auto previous=debug->paused;
  debug->paused=[&](const QString &kind){if(previous)previous(kind);try{auto *dialog=window.findChild<QDialog *>("debuggerDialog");auto *text=dialog?dialog->findChild<QPlainTextEdit *>("debuggerCommand"):nullptr;require(dialog&&dialog->isVisible()&&text,"Pen policy native command view absent");acceptanceView(*dialog);QJsonObject item{{"node",dialog->property("commandIndex").toInt()},{"type",kind},{"command",text->toPlainText()}};for(const char *name:{"Phase","Keep"}){auto *value=Moderator::Instance()->GetInternalObject(name);require(value,"Pen policy internal scalar absent at boundary");item[name]=value->GetRealParameter("Value");}observed.append(item);}catch(...){failure=std::current_exception();QTimer::singleShot(0,&window,[&]{window.stopMission();});}QTimer::singleShot(0,&window,[&]{debug->resume();});};
  auto breakpoints=[&]{const auto snapshot=window.missionSnapshot();require(snapshot.nodes.size()==5,"Pen policy native mission shape differs");for(int i=1;i<snapshot.nodes.size();++i)require(window.setBreakpoint(i,true),"Pen policy native breakpoint unavailable");};
  const int invalidUndo=source->document()->availableUndoSteps();const bool invalidModified=source->document()->isModified();messages->clear();invoke("Build script");const auto buildStatus=window.statusBar()->currentMessage();const bool built=buildStatus=="Build succeeded";
  require(built||buildStatus.startsWith("Build failed"),"Pen policy Build has no checked completion status");QJsonObject rejected;
  if(built){if(debugMode)breakpoints();invoke("Run mission");if(failure)std::rethrow_exception(failure);const auto status=window.statusBar()->currentMessage();const auto raw=read(report);auto *invalidPhase=Moderator::Instance()->GetInternalObject("Phase"),*invalidKeep=Moderator::Instance()->GetInternalObject("Keep");require(invalidPhase&&invalidKeep&&invalidPhase->GetRealParameter("Value")==0&&invalidKeep->GetRealParameter("Value")==41,"Unsupported pen changed or lost final scalar witnesses");if(!debugMode)require(observed.isEmpty(),"Plain pen policy unexpectedly entered debugger");save("invalid-values.txt",raw);save("invalid-messages.txt",messages->toPlainText().toUtf8());save("invalid-commands.json",QJsonDocument(observed).toJson());save("invalid-run-diagnostic.json",QJsonDocument(QJsonObject{{"build_status",buildStatus},{"run_status",status},{"rows",rows(raw)},{"commands",observed}}).toJson());require(status.startsWith("Mission failed")&&!window.isRunning(),"Unsupported ReportFile pen reference completed or stopped without rejection");for(const auto &v:observed){const auto o=v.toObject();require(o["node"].toInt()<=2&&o["Phase"].toDouble()==0&&o["Keep"].toDouble()==41,"Unsupported pen ran later commands or changed scalar witness");}for(const auto &r:rows(raw))require(r.toArray()==QJsonArray{0,41},"Unsupported pen wrote a post-command witness");require(messages->toPlainText().contains(command)||messages->toPlainText().contains("Values"),"Unsupported pen failure lacks command/reference diagnostic");rejected=QJsonObject{{"phase","run"},{"status",status},{"rows",rows(raw)},{"commands",observed},{"final_phase",invalidPhase->GetRealParameter("Value")},{"final_keep",invalidKeep->GetRealParameter("Value")}};
  }else{save("invalid-messages.txt",messages->toPlainText().toUtf8());rejected=QJsonObject{{"phase","build"},{"status",buildStatus},{"rows",QJsonArray{}},{"commands",QJsonArray{}}};}
  require(source->toPlainText()==invalid&&source->document()->availableUndoSteps()==invalidUndo&&source->document()->isModified()==invalidModified,"Pen policy rejection changed source/history");
  source->setFocus();source->moveCursor(QTextCursor::Start);require(source->find(bad),"Pen policy correction text not found");acceptanceView(*source);QTest::keyClicks(source,good);events();require(source->toPlainText()==valid,"Pen policy typed correction changed unrelated source");source->undo();require(source->toPlainText()==invalid,"Pen policy exact Undo failed");source->redo();require(source->toPlainText()==valid,"Pen policy exact Redo failed");
  const auto saved=files.filePath("Pen policy "+command+" Ω.script");require(window.saveScriptTo(saved)&&window.loadScript(saved)&&source->toPlainText()==valid,"Pen policy Unicode save/reopen failed");save("valid.script",valid.toUtf8());
  messages->clear();observed={};failure=nullptr;invoke("Build script");require(window.statusBar()->currentMessage()=="Build succeeded","Corrected pen script Build failed");if(debugMode)breakpoints();const int validUndo=source->document()->availableUndoSteps();invoke("Run mission");if(failure)std::rethrow_exception(failure);debug->paused=previous;
  save("valid-messages.txt",messages->toPlainText().toUtf8());save("valid-values.txt",read(report));save("valid-commands.json",QJsonDocument(observed).toJson());const auto actual=rows(read(report));
  require(window.statusBar()->currentMessage()=="Mission completed"&&!window.isRunning()&&actual==QJsonArray{QJsonArray{0,41},QJsonArray{1,41}},"Corrected pen run did not recover to independent before/after report");require(observed.size()==(debugMode?4:0),"Corrected pen native execution count differs");
  const QStringList expected{"Report 'Before policy' Values Phase Keep;",good,"Phase = 1;","Report 'After witness' Values Phase Keep;"};for(int i=0;i<observed.size();++i){auto o=observed[i].toObject();require(o["node"].toInt()==i+1&&o["command"]==expected[i]&&o["Phase"].toDouble()==(i==3?1:0)&&o["Keep"].toDouble()==41,"Corrected pen native command/scalar sequence differs");}
  auto *phase=Moderator::Instance()->GetInternalObject("Phase"),*keep=Moderator::Instance()->GetInternalObject("Keep");require(phase&&keep&&phase->GetRealParameter("Value")==1&&keep->GetRealParameter("Value")==41&&source->toPlainText()==valid&&source->document()->availableUndoSteps()==validUndo,"Corrected pen run changed source/history or final native witnesses");
  const QJsonObject proof{{"checkpoint","pen-script-policy-"+command+"-"+mode},{"outcome","passed"},{"family",command},{"mode",mode},{"numeric_sentinel",41},{"invalid_rejected",true},{"rejection",rejected},{"valid_completed",true},{"valid_rows",actual},{"valid_commands",observed},{"exact_history_unicode",true},{"facets",QJsonArray{"actual-gui-script-build-run-menu-actions","unsupported-reportfile-pen-rejection-and-no-later-commands","native-boundary-scalar-and-partial-report-witness","typed-source-correction-exact-undo-redo-unicode-reopen","fresh-run-recovery-to-independent-zero-one-and-sentinel"}},{"scope","Raw GUI script PenUp/PenDown against ReportFile must reject at Build or Run, without later commands or source/history mutation; correction to a configured XYPlot completes through real Run-menu actions with native boundary (Debug mode) or uninterrupted execution (Plain mode) and literal0/1/41 witnesses. This does not impose rejection timing, change core semantics, qualify arbitrary subscriber classes/plot science/native OS input/hardware/platform/stress or complete latest-build NASA coverage."}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
