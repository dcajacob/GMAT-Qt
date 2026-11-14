#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ReportViewer.hpp"
#include "FindReplaceDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QThread>
#include <QTextCursor>
#include <QVBoxLayout>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void write(const QString &path,const QByteArray &bytes)
{
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Search fixture write failed");
}
static QByteArray read(const QString &path)
{
   QFile file(path); require(file.open(QIODevice::ReadOnly),"Search fixture read failed"); return file.readAll();
}
static void shortcut(QWidget *target,QAction *action)
{
   const auto sequence=action->shortcut();
   require(sequence.count()==1,"Search routing requires a configured single-chord shortcut");
   const auto chord=sequence[0];
   QKeyEvent down(QEvent::KeyPress,chord.key(),chord.keyboardModifiers()),up(QEvent::KeyRelease,chord.key(),chord.keyboardModifiers());
   QApplication::sendEvent(target,&down); QApplication::sendEvent(target,&up); QApplication::processEvents();
}
static FindReplaceDialog *search(QPlainTextEdit *text,QWidget *owner)
{
   auto *dialog=dynamic_cast<FindReplaceDialog *>(owner->findChild<QDialog *>("findReplaceDialog"));
   require(text && dialog,"Local Find dialog missing"); return dialog;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtSearchRouting");
   try {
      require(argc==2 && acceptanceWidgetPlatform(),"Search routing requires offscreen startup");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Search fixture directory unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Search runtime initialization failed"); window.show();
      auto *area=window.findChild<QMdiArea *>("workspace"); auto *active=window.findChild<QPlainTextEdit *>("scriptEditor");
      require(area && active,"Search workspace controls missing");
      auto *activeWindow=area->activeSubWindow(); require(activeWindow && activeWindow->property("activeScript").toBool(),"Initial active script missing");
      auto *activeSearch=search(active,active);
      const auto missionBefore=window.missionSnapshot();
      auto *configuredSpacecraft=Moderator::Instance()->GetConfiguredObject("DefaultSC");
      const QString activeSource="% active search document α\nCreate Variable ActiveValue;\nActiveValue = 7;\nBeginMissionSequence;\nActiveValue = ActiveValue + 1; % ActiveNeedle\n% ActiveNeedle\n";
      auto activeCursor=active->textCursor(); activeCursor.select(QTextCursor::Document); activeCursor.insertText(activeSource);
      require(active->document()->isModified(),"Unsaved active fixture not retained");
      const auto action=[&](const char *name) {
         auto *value=window.findChild<QAction *>(name); require(value && value->isEnabled(),"Search action unavailable"); return value;
      };
      const auto focus=[&](QMdiSubWindow *child,QWidget *target) {
         child->showNormal(); area->setActiveSubWindow(child); window.activateWindow(); target->setFocus(Qt::OtherFocusReason); app.processEvents();
         // Directly-created fixture windows bypass the application's placement
         // helper. Keep the selected document visible instead of accepting an
         // off-edge MDI cascade merely because its text controls have focus.
         const auto bounds=area->viewport()->rect();
         child->resize(child->size().boundedTo(bounds.size())); child->move(bounds.topLeft());
         app.processEvents();
         require(bounds.contains(child->geometry()),"Selected search document extends beyond the visible workspace");
         const auto selected=[&] { return area->activeSubWindow()==child && (QApplication::focusWidget()==target || target->isAncestorOf(QApplication::focusWidget())); };
         QElapsedTimer settle; settle.start();
         while (!selected() && settle.elapsed()<2000) { app.processEvents(QEventLoop::AllEvents,20); QThread::msleep(2); }
         require(selected(),"Search focus target was not selected");
      };
      const auto selected=[&](QPlainTextEdit *text,const QString &query,int offset,const char *message) {
         require(text->textCursor().selectedText()==query && text->textCursor().selectionStart()==offset,message);
      };
      const auto reportPath=files.filePath("report Δ.txt");
      const QString reportContents="first ReportToken α\nsecond ReportToken β\nthird ReportToken γ\n";
      const auto reportBytes=reportContents.toUtf8(); write(reportPath,reportBytes);
      auto *report=new ReportViewer(reportPath,"Routing"); auto *reportWindow=area->addSubWindow(report);
      reportWindow->resize(850,530); reportWindow->show();
      auto *reportText=report->findChild<QPlainTextEdit *>("report:Routing"); auto *reportSearch=search(reportText,report);
      require(reportText->isReadOnly() && reportText->toPlainText()==reportContents,"Report fixture not loaded read-only");
      std::cout<<"Configured search shortcuts: Find "<<action("scriptFind")->shortcut().toString().toStdString()
         <<", Next "<<action("scriptFindNext")->shortcut().toString().toStdString()
         <<", Previous "<<action("scriptFindPrevious")->shortcut().toString().toStdString()<<'\n';
      // This first assertion reproduces the former MainWindow/ReportViewer
      // duplicate Ctrl+F registrations without running or rebuilding a mission.
      focus(reportWindow,reportText); shortcut(reportText,action("scriptFind"));
      require(reportSearch->isVisible() && !activeSearch->isVisible() && area->activeSubWindow()==reportWindow,
         "Report Ctrl+F did not open local Find; a competing shortcut or script routing took focus");
      reportSearch->hide(); focus(reportWindow,reportText); action("scriptFind")->trigger();
      require(reportSearch->isVisible() && !activeSearch->isVisible() && area->activeSubWindow()==reportWindow,"Edit Find activated the script instead of the report");
      require(!reportSearch->findChild<QPushButton *>("replaceCurrent") && !reportSearch->findChild<QPushButton *>("replaceAll"),"Report search offered mutation controls");
      reportSearch->findChild<QComboBox *>("findText")->setEditText("ReportToken"); reportSearch->hide();
      const int first=reportContents.indexOf("ReportToken"),second=reportContents.indexOf("ReportToken",first+1),third=reportContents.indexOf("ReportToken",second+1);
      focus(reportWindow,reportText); reportText->moveCursor(QTextCursor::Start); action("scriptFindNext")->trigger(); selected(reportText,"ReportToken",first,"Report menu Next did not select its first local match");
      shortcut(reportText,action("scriptFindNext")); selected(reportText,"ReportToken",second,"Report configured Next shortcut did not advance locally");
      shortcut(reportText,action("scriptFindPrevious")); selected(reportText,"ReportToken",first,"Report configured Previous shortcut did not move locally");
      action("scriptFindPrevious")->trigger(); selected(reportText,"ReportToken",third,"Report menu Previous did not wrap locally");
      action("scriptFindNext")->trigger(); selected(reportText,"ReportToken",first,"Report menu Next did not wrap locally");
      report->findChild<QPushButton *>("reportFind")->click();
      require(reportSearch->isVisible() && area->activeSubWindow()==reportWindow,"Report Find button stopped working");
      reportSearch->hide();
      focus(reportWindow,reportText); action("scriptReplace")->trigger();
      require(reportSearch->isVisible() && !activeSearch->isVisible() && area->activeSubWindow()==reportWindow && active->toPlainText()==activeSource,"Read-only Replace left the report or edited the script");
      reportSearch->hide();
      // ComparisonPanel embeds the same report component. Use the same
      // composition here without re-running any comparison or mission.
      auto *wrapper=new QWidget; auto *nestedLayout=new QVBoxLayout(wrapper);
      auto *nestedReport=new ReportViewer(reportPath,"Nested",wrapper); nestedLayout->addWidget(nestedReport);
      auto *nestedWindow=area->addSubWindow(wrapper); nestedWindow->resize(850,530); nestedWindow->show();
      auto *nestedText=nestedReport->findChild<QPlainTextEdit *>("report:Nested"); auto *nestedSearch=search(nestedText,nestedReport);
      focus(nestedWindow,nestedText); shortcut(nestedText,action("scriptFind"));
      require(nestedSearch->isVisible() && !reportSearch->isVisible() && !activeSearch->isVisible() && area->activeSubWindow()==nestedWindow,"Nested report Ctrl+F routed outside its active workspace");
      nestedSearch->hide(); focus(nestedWindow,nestedText); action("scriptFind")->trigger();
      require(nestedSearch->isVisible() && !activeSearch->isVisible() && area->activeSubWindow()==nestedWindow,"Nested report menu Find routed outside its active workspace");
      nestedSearch->findChild<QComboBox *>("findText")->setEditText("ReportToken"); nestedSearch->hide();
      focus(nestedWindow,nestedText); nestedText->moveCursor(QTextCursor::Start); action("scriptFindNext")->trigger(); selected(nestedText,"ReportToken",first,"Nested report menu Next routed elsewhere");
      shortcut(nestedText,action("scriptFindNext")); selected(nestedText,"ReportToken",second,"Nested report configured Next shortcut routed elsewhere");
      action("scriptFindPrevious")->trigger(); selected(nestedText,"ReportToken",first,"Nested report menu Previous routed elsewhere");
      action("scriptReplace")->trigger();
      require(nestedSearch->isVisible() && !activeSearch->isVisible() && area->activeSubWindow()==nestedWindow && !nestedSearch->findChild<QPushButton *>("replaceAll"),"Nested read-only Replace routed to a writable script");
      nestedSearch->hide();
      // A separately opened script remains inactive. Search chooses that
      // document and cannot alter the active mission or report's bytes.
      const auto inactivePath=files.filePath("inactive Δ.script");
      const QString inactiveSource="% independent inactive document\nCreate Variable Other;\nBeginMissionSequence;\nOther = 3; % InactiveNeedle\n% InactiveNeedle\n";
      write(inactivePath,inactiveSource.toUtf8()); require(window.openScriptDocument(inactivePath),"Inactive search document could not open");
      auto *inactiveWindow=area->activeSubWindow(); auto *inactive=inactiveWindow->findChild<QPlainTextEdit *>("inactiveScriptEditor");
      require(inactive && !inactiveWindow->property("activeScript").toBool(),"Opening search document changed active identity"); auto *inactiveSearch=search(inactive,inactive);
      focus(inactiveWindow,inactive); action("scriptFind")->trigger();
      require(inactiveSearch->isVisible() && !activeSearch->isVisible() && !reportSearch->isVisible() && !nestedSearch->isVisible() && area->activeSubWindow()==inactiveWindow,"Inactive menu Find routed elsewhere");
      inactiveSearch->findChild<QComboBox *>("findText")->setEditText("InactiveNeedle"); inactiveSearch->hide();
      const int inactiveFirst=inactiveSource.indexOf("InactiveNeedle"),inactiveSecond=inactiveSource.indexOf("InactiveNeedle",inactiveFirst+1);
      focus(inactiveWindow,inactive); inactive->moveCursor(QTextCursor::Start); action("scriptFindNext")->trigger(); selected(inactive,"InactiveNeedle",inactiveFirst,"Inactive menu Next routed elsewhere");
      shortcut(inactive,action("scriptFindNext")); selected(inactive,"InactiveNeedle",inactiveSecond,"Inactive configured Next shortcut routed elsewhere");
      action("scriptFindPrevious")->trigger(); selected(inactive,"InactiveNeedle",inactiveFirst,"Inactive menu Previous routed elsewhere");
      shortcut(inactive,action("scriptFind"));
      require(inactiveSearch->isVisible() && !activeSearch->isVisible() && !reportSearch->isVisible() && area->activeSubWindow()==inactiveWindow,"Inactive Ctrl+F routed elsewhere");
      inactiveSearch->hide();
      require(active->toPlainText()==activeSource && active->document()->isModified() && inactive->toPlainText()==inactiveSource && !inactive->document()->isModified(),"Search changed independent document source/dirty state");
      require(window.findChild<QPlainTextEdit *>("scriptEditor")==active && activeWindow->property("activeScript").toBool() && !inactiveWindow->property("activeScript").toBool(),"Search changed active mission identity");
      require(window.missionSnapshot().sourceScript==missionBefore.sourceScript && window.missionSnapshot().nodes.size()==missionBefore.nodes.size() && Moderator::Instance()->GetConfiguredObject("DefaultSC")==configuredSpacecraft,"Search rebuilt or changed the loaded mission");
      require(reportText->toPlainText()==reportContents && reportText->isReadOnly() && read(reportPath)==reportBytes && read(inactivePath)==inactiveSource.toUtf8(),"Search changed report or inactive file bytes");
      selected(reportText,"ReportToken",first,"Inactive search changed the report selection");
      selected(nestedText,"ReportToken",first,"Inactive search changed the nested report selection");
      require(nestedText->toPlainText()==reportContents && nestedText->isReadOnly(),"Search changed nested report text");
      std::cout<<R"(NASA_CHECK {"checkpoint":"script.find","family":"script-editor","outcome":"passed","facets":["menu","shortcut","document-isolation","exact-match-offset","read-only-report","source-unchanged","mission-unchanged"]})"<<std::endl;
      std::cout<<R"(NASA_CHECK {"checkpoint":"script.find-next","family":"script-editor","outcome":"passed","facets":["menu","shortcut","exact-match-offset","forward-backward","wrap","document-isolation","source-unchanged"]})"<<std::endl;
      std::cout<<"PASS local/nested report Ctrl+F/menu Find/Next/Previous and configured Next/Previous shortcuts, read-only Replace/wrapping and button Find; independent inactive-script routing with unchanged active mission, sources, dirty states and report/file bytes. No mission/comparison execution or completed-suite repeat.\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<'\n'; return 1; }
}
