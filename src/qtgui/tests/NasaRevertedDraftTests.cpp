#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QAction>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Reverted draft artifact unavailable"); return file.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Reverted draft artifact write failed"); }
static void reveal(QWidget *widget) {
   require(widget,"Reverted draft input missing");
   for (auto *parent=widget;parent;parent=parent->parentWidget()) {
      for (auto *tabs:widget->window()->findChildren<QTabWidget *>()) if (tabs->indexOf(parent)>=0) tabs->setCurrentWidget(parent);
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(widget);
   }
   events(); require(widget->isVisible(),"Reverted draft input hidden");
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaRevertedDraft");
   try {
      require(argc==3&&acceptanceWidgetPlatform(),"Supply reverted draft startup and subject");
      const QString key=argv[2],startup=QFileInfo(argv[1]).absoluteFilePath(); require(key=="Epoch"||key=="Variable","Unknown reverted draft subject");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Reverted draft directory unavailable");
      const auto report=files.filePath("reverted.csv"),saved=files.filePath("Reverted draft Ω.script"),evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &value) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),value); };
      const QString baseline="% NASA reverted draft Ω\nCreate Spacecraft Sat;\nSat.DateFormat = TAIModJulian;\nSat.Epoch = 21545;\nCreate Variable V Keep;\nV = 2;\nKeep = 7;\nCreate ReportFile R;\nR.Filename = '"+report+"';\nR.WriteHeaders = false;\nR.Precision = 17;\nR.FixedWidth = false;\nR.Delimiter = ',';\nBeginMissionSequence;\nKeep = Keep + 2;\nReport R Sat.A1ModJulian V Keep;\n";
      save("baseline.script",baseline.toUtf8()); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window;
      require(window.initialize(startup),"Reverted draft initialization failed"); window.show(); acceptanceView(window); events();
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace"); require(source&&tree&&area,"Reverted draft workspace unavailable");
      source->setPlainText(baseline); require(window.buildScript()&&window.saveScriptTo(saved),"Reverted draft baseline failed"); const int undo=source->document()->availableUndoSteps();
      const QString name=key=="Epoch"?"Sat":"V",control=key=="Epoch"?"resource_Epoch":"parameterValue",base=key=="Epoch"?"21545":"2",changed=key=="Epoch"?"21546":"9";
      auto unchanged=[&] {
         require(source->toPlainText()==baseline&&source->document()->availableUndoSteps()==undo&&!source->document()->isModified()&&read(saved)==baseline.toUtf8(),"Reverted draft changed source/history/file");
         auto *sat=Moderator::Instance()->GetConfiguredObject("Sat"),*variable=Moderator::Instance()->GetConfiguredObject("V"); require(sat&&variable,"Reverted draft native objects absent");
         require(std::abs(sat->GetRealParameter("A1Epoch")-(21545.+.0343817/86400.))*86400<=.000002&&variable->GetRealParameter("Value")==2,"Reverted draft changed independent native epoch/value");
      };
      auto open=[&] { const auto matches=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(matches.size()==1,"Reverted draft resource missing/ambiguous"); tree->scrollToItem(matches.front()); tree->setCurrentItem(matches.front()); acceptanceView(window); tree->itemDoubleClicked(matches.front(),0); events(); auto *child=area->activeSubWindow(); require(child&&dynamic_cast<ResourceEditor *>(child->widget()),"Reverted draft actual editor unavailable"); child->showNormal(); child->move(0,0); child->resize(area->viewport()->size()); acceptanceView(*child); return child; };
      QPointer<QMdiSubWindow> child; QJsonArray states;
      auto panel=[&] { auto *value=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr; require(value,"Reverted draft owner absent"); return value; };
      auto button=[&](QDialogButtonBox::StandardButton action) { auto *box=panel()->findChild<QDialogButtonBox *>("resourceButtons"); require(box,"Reverted draft buttons unavailable"); auto *value=box->button(action); reveal(value); return value; };
      auto reverted=[&](const QString &phase) {
         child=open(); auto *input=panel()->findChild<QLineEdit *>(control); require(input&&!input->isReadOnly(),"Reverted draft field unavailable"); reveal(input);
         input->setFocus(); input->selectAll(); QTest::keyClicks(input,changed); events(); require(panel()->hasChanges(),"Reverted draft did not first change its value"); unchanged();
         input->selectAll(); QTest::keyClicks(input,base); events(); unchanged();
         const bool enabled=button(QDialogButtonBox::Apply)->isEnabled(),different=panel()->hasChanges();
         states.append(QJsonObject{{"phase",phase},{"input",input->text()},{"values_differ",different},{"apply_enabled",enabled}}); save("states.json",QJsonDocument(states).toJson());
         require(input->text()==base&&!different&&enabled,"Reverted draft did not retain dirty input with baseline values"); acceptanceView(*child);
      };
      auto prompt=[&](const QString &title,QMessageBox::StandardButton choice,const std::function<bool()> &action) {
         bool visited=false; std::exception_ptr failure; QTimer timer; timer.setInterval(20);
         QObject::connect(&timer,&QTimer::timeout,&window,[&] { auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return; timer.stop(); try {
            auto *box=qobject_cast<QMessageBox *>(dialog); require(box&&box->windowTitle()==title,"Unexpected reverted draft prompt"); auto *selected=box->button(choice); require(selected,"Reverted draft choice missing"); acceptanceView(*box); visited=true; QTest::mouseClick(selected,Qt::LeftButton);
         } catch (...) { failure=std::current_exception(); dialog->reject(); } });
         timer.start(); const bool result=action(); timer.stop(); events(); if (failure) std::rethrow_exception(failure); require(visited,"Reverted draft close prompt missing"); return result;
      };
      reverted("panel-close");
      require(!prompt("Unapplied changes",QMessageBox::Cancel,[&] { return child->close(); })&&child&&child->isVisible(),"Reverted draft close refusal lost its owner"); unchanged();
      require(prompt("Unapplied changes",QMessageBox::Discard,[&] { return child->close(); })&&!child,"Reverted draft discard did not close owner"); unchanged();
      reverted("no-op-apply"); QTest::mouseClick(button(QDialogButtonBox::Apply),Qt::LeftButton); events(); unchanged();
      require(!button(QDialogButtonBox::Apply)->isEnabled()&&child->close(),"Reverted no-op Apply did not clear dirty state"); events(); require(!child,"Reverted no-op clean close failed");
      reverted("no-op-ok"); QTest::mouseClick(button(QDialogButtonBox::Ok),Qt::LeftButton); events(); require(!child,"Reverted no-op OK did not close its owner"); unchanged();
      reverted("document-activation");
      QPointer<QMdiSubWindow> originalDocument;
      for (auto *entry:area->subWindowList()) if (entry->isAncestorOf(source)) originalDocument=entry;
      require(originalDocument,"Reverted draft original document missing");
      const auto otherPath=files.filePath("Other mission Ω.script"); const auto otherSource=QString(baseline).replace("V = 2;","V = 3;");
      write(otherPath,otherSource.toUtf8()); require(window.openScriptDocument(otherPath),"Reverted draft second document failed to open");
      QPointer<QMdiSubWindow> otherDocument=area->activeSubWindow(); require(otherDocument&&otherDocument!=originalDocument,"Reverted draft second document identity differs");
      auto *activate=window.findChild<QAction *>("activateScriptDocument"); require(activate&&activate->isEnabled(),"Reverted draft activate action unavailable");
      prompt("Pending active-mission changes",QMessageBox::Cancel,[&] { activate->trigger(); return true; });
      require(window.findChild<QPlainTextEdit *>("scriptEditor")==source&&child&&button(QDialogButtonBox::Apply)->isEnabled(),"Reverted draft cancelled activation changed document or lost input"); unchanged();
      // The prompt's Apply choice is a genuine no-op for these restored values;
      // it must clear dirty input so activation can proceed without a commit.
      area->setActiveSubWindow(otherDocument); acceptanceView(*otherDocument);
      prompt("Pending active-mission changes",QMessageBox::Apply,[&] { activate->trigger(); return true; });
      require(!child&&window.findChild<QPlainTextEdit *>("scriptEditor")!=source&&Moderator::Instance()->GetConfiguredObject("V")->GetRealParameter("Value")==3,"Reverted draft no-op Apply did not activate the selected document");
      require(source->toPlainText()==baseline&&source->document()->availableUndoSteps()==undo&&read(saved)==baseline.toUtf8(),"Reverted activation changed the inactive original source/history/file");
      originalDocument->showNormal(); area->setActiveSubWindow(originalDocument); acceptanceView(*originalDocument); activate->trigger(); events();
      require(window.findChild<QPlainTextEdit *>("scriptEditor")==source,"Reverted activation did not return to original document"); unchanged();
      require(otherDocument&&otherDocument->close(),"Reverted inactive document close failed"); events();
      reverted("load-build-run-guards"); require(!window.loadScript(saved),"Reverted draft bypassed load guard"); require(!window.buildScript(),"Reverted draft bypassed build guard");
      require(window.runMission()==MainWindow::RunResult::Failed&&!QFile::exists(report)&&Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value")==7,"Reverted draft bypassed run guard or executed mission"); unchanged();
      auto *openAction=window.findChild<QAction *>("openMissionScript"); QMenu *fileMenu=nullptr;
      for (auto *action:window.menuBar()->actions()) if (action->menu()&&action->menu()->actions().contains(openAction)) fileMenu=action->menu(); require(openAction&&fileMenu,"Reverted draft File/Open action unavailable");
      auto file=[&](QMessageBox::StandardButton choice) {
         int stage=0; std::exception_ptr failure; QTimer timer; timer.setInterval(20);
         QObject::connect(&timer,&QTimer::timeout,&window,[&] { auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return; try {
            if (stage==0) { auto *picker=qobject_cast<QFileDialog *>(dialog); require(picker,"Reverted draft expected File/Open dialog"); picker->selectFile(saved); acceptanceView(*picker); require(picker->selectedFiles()==QStringList{saved},"Reverted draft File/Open path differs"); auto *box=picker->findChild<QDialogButtonBox *>(); require(box&&box->button(QDialogButtonBox::Open),"Reverted draft File/Open accept missing"); stage=1; QTest::mouseClick(box->button(QDialogButtonBox::Open),Qt::LeftButton); }
            else if (stage==1) { auto *box=qobject_cast<QMessageBox *>(dialog); require(box&&box->windowTitle()=="Unapplied panel changes"&&box->button(choice),"Reverted draft File/Open lost dirty prompt"); timer.stop(); stage=2; acceptanceView(*box); QTest::mouseClick(box->button(choice),Qt::LeftButton); }
         } catch (...) { timer.stop(); failure=std::current_exception(); dialog->reject(); } });
         acceptanceView(window); QTest::mouseClick(window.menuBar(),Qt::LeftButton,Qt::NoModifier,window.menuBar()->actionGeometry(fileMenu->menuAction()).center()); require(QTest::qWaitFor([&] { return fileMenu->isVisible(); },1500),"Reverted draft File menu did not expose"); fileMenu->setActiveAction(openAction); acceptanceView(*fileMenu); timer.start(); QTest::keyClick(fileMenu,Qt::Key_Return); timer.stop(); events(); if (failure) std::rethrow_exception(failure); require(stage==2,"Reverted draft File/Open prompt sequence incomplete");
      };
      file(QMessageBox::Cancel); require(child&&child->isVisible()&&button(QDialogButtonBox::Apply)->isEnabled(),"Reverted File/Open cancel lost draft"); unchanged();
      file(QMessageBox::Discard); require(!child,"Reverted File/Open discard did not retire draft"); unchanged();
      require(window.runMission()==MainWindow::RunResult::Completed,"Reverted draft accepted mission failed"); events(); const auto bytes=read(report); save("values.csv",bytes); const auto values=QString::fromUtf8(bytes).trimmed().split(',');
      require(values.size()==3&&std::abs(values[0].toDouble()-(21545.+.0343817/86400.))*86400<=.000002&&values[1].toDouble()==2&&values[2].toDouble()==9,"Reverted draft independent mission results differ");
      reverted("program-close"); require(!prompt("Unapplied panel changes",QMessageBox::Cancel,[&] { return window.close(); })&&window.isVisible()&&child,"Reverted program-close cancel lost draft/window"); unchanged();
      require(prompt("Unapplied panel changes",QMessageBox::Discard,[&] { return window.close(); })&&!window.isVisible()&&!child,"Reverted program-close discard failed"); unchanged();
      const QJsonObject proof{{"checkpoint","reverted-draft-"+key},{"outcome","passed"},{"family",key},{"states",states},{"numeric_sentinel",9},{"facets",QJsonArray{"reverted-input-panel-close-refusal-and-discard","no-op-apply-and-ok-clear-dirty-without-commit","reverted-load-build-run-guards-and-actual-file-open","reverted-program-close-refusal-and-discard","exact-source-history-file-and-independent-epoch-value-result"}},
         {"scope","Selected primary Epoch and scalar Variable line edits restored to their saved baseline. Dirty-input close/load/build/run policy, actual File/Open modal choices, no-op Apply/OK, unchanged source/history/file/native values and independent epoch/V2/Keep9 report. Not all wrappers/input kinds, unsaved-document Save decisions, current NASA private corpus, native OS input/GPU/platform/stress/general science or full-current-build qualification."}};
      save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl; }
     catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<std::endl; }
   return 1;
}
