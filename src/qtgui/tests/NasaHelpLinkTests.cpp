#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMenuBar>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>
#include <QWindow>
#include <cmath>
#include <iostream>
#include <stdexcept>

class HelpDispatch final : public QObject
{
   Q_OBJECT
public:
   HelpDispatch()
   {
      QDesktopServices::setUrlHandler("https",this,"receive");
      QDesktopServices::setUrlHandler("http",this,"receive");
   }
   ~HelpDispatch() override
   {
      QDesktopServices::unsetUrlHandler("https");
      QDesktopServices::unsetUrlHandler("http");
   }
   QList<QUrl> urls;
public slots:
   void receive(const QUrl &url) { urls.append(url); }
};

static void require(bool ok,const char *message)
{
   if (!ok) throw std::runtime_error(message);
}
static void events()
{
   QApplication::processEvents();
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static QByteArray read(const QString &path)
{
   QFile f(path); require(f.open(QIODevice::ReadOnly),"Help link artifact missing"); return f.readAll();
}
static void write(const QString &path,const QByteArray &data)
{
   QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"Help link artifact write failed");
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaHelpLinks");
   try {
      require(argc==2&&acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen","Help links require native startup");
      TestSettings settings; QTemporaryDir files;
      require(files.isValid(),"Help link fixture unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto report=files.filePath("help values Ω.csv"),saved=files.filePath("help mission Ω.script");
      const auto inactivePath=files.filePath("independent Δ.script");
      const QString inactiveText="% independent selected document Ω\n";
      const QString mission="% preserve help mission Ω\nCreate Variable Keep Offset;\nKeep = 41;\nOffset = -6.125;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nBeginMissionSequence;\nReport Values Keep Offset;\n";
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &data) { if (!evidence.isEmpty()) write(evidence+"/"+name,data); };
      MainWindow window; window.resize(1200,850); window.show();
      require(window.initialize(startup),"Help runtime unavailable"); acceptanceView(window);
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto *area=window.findChild<QMdiArea *>("workspace");
      auto *menu=window.findChild<QMenu *>("helpMenu");
      require(source&&area&&menu&&window.menuBar()->actions().contains(menu->menuAction()),"Actual Help menu/workspace unavailable");
      source->setPlainText(mission); save("initial.script",mission.toUtf8());
      QJsonArray reports,requests; int aboutDialogs=0;
      auto run=[&](const QString &phase) {
         require(window.runMission()==MainWindow::RunResult::Completed,"Help witness mission failed");
         const auto raw=read(report); const auto fields=QString::fromUtf8(raw).trimmed().split(',');
         require(fields.size()==2,"Help report cardinality differs");
         QJsonArray values;
         for (const auto &field:fields) {
            bool ok=false; const auto value=field.toDouble(&ok);
            require(ok&&std::isfinite(value),"Help report value nonfinite"); values.append(value);
         }
         require(values[0].toDouble()==41&&values[1].toDouble()==-6.125,"Help actions changed independent literal results");
         reports.append(QJsonObject{{"phase",phase},{"values",values}});
         save(phase+".csv",raw); save("reports.json",QJsonDocument(reports).toJson()); return raw;
      };
      const auto baseline=run("baseline");
      require(window.saveScriptTo(saved),"Help clean source save failed");
      write(inactivePath,inactiveText.toUtf8());
      QPlainTextEdit *inactive=nullptr; QMdiSubWindow *selected=nullptr;
      auto openInactive=[&] {
         require(window.openScriptDocument(inactivePath),"Help independent document unavailable");
         selected=area->currentSubWindow(); inactive=qobject_cast<QPlainTextEdit *>(selected->widget());
         require(inactive&&inactive!=source&&inactive->toPlainText()==inactiveText,"Help selected wrong document");
         auto cursor=inactive->textCursor(); cursor.setPosition(2); cursor.setPosition(13,QTextCursor::KeepAnchor); inactive->setTextCursor(cursor);
         inactive->setFocus(); events();
      };
      openInactive(); HelpDispatch dispatch;
      const QList<QPair<QString,QString>> links{
         {"gmatWikiHelp","https://gmat.atlassian.net/wiki/spaces/GW/overview"},
         {"gmatForumHelp","https://github.com/nasa/GMAT/discussions"},
         {"gmatProjectHelp","https://sourceforge.net/projects/gmat/"}
      };
      auto exercise=[&](const QString &phase) {
         selected->showNormal(); area->setActiveSubWindow(selected); inactive->setFocus(); events();
         const auto mainText=source->toPlainText(),otherText=inactive->toPlainText();
         const auto disk=read(saved),otherDisk=read(inactivePath);
         const auto working=QDir::currentPath();
         const int mainUndo=source->document()->availableUndoSteps(),otherUndo=inactive->document()->availableUndoSteps();
         const bool mainDirty=source->document()->isModified(),otherDirty=inactive->document()->isModified();
         const auto mainCursor=source->textCursor(),otherCursor=inactive->textCursor();
         const int windows=area->subWindowList().size();
         auto invariant=[&] {
            auto *keep=Moderator::Instance()->GetConfiguredObject("Keep");
            auto *offset=Moderator::Instance()->GetConfiguredObject("Offset");
            require(keep&&offset&&keep->GetRealParameter("Value")==41&&offset->GetRealParameter("Value")==-6.125,"Help dispatch changed native variables");
            require(source->toPlainText()==mainText&&inactive->toPlainText()==otherText&&read(saved)==disk&&read(inactivePath)==otherDisk,"Help dispatch changed source or disk contents");
            require(source->document()->availableUndoSteps()==mainUndo&&inactive->document()->availableUndoSteps()==otherUndo&&source->document()->isModified()==mainDirty&&inactive->document()->isModified()==otherDirty,"Help dispatch changed history or dirty state");
            require(source->textCursor().position()==mainCursor.position()&&source->textCursor().anchor()==mainCursor.anchor()&&inactive->textCursor().position()==otherCursor.position()&&inactive->textCursor().anchor()==otherCursor.anchor(),"Help dispatch changed source selections");
            require(area->subWindowList().size()==windows&&area->activeSubWindow()==selected&&QDir::currentPath()==working,"Help dispatch changed selected window or working directory");
         };
         for (const auto &link:links) {
            auto *action=window.findChild<QAction *>(link.first);
            require(action&&menu->actions().contains(action)&&action->isEnabled()&&action->isVisible(),"Help link action missing/disabled");
            auto open=[&] {
               acceptanceView(window);
               QTest::mouseClick(window.menuBar(),Qt::LeftButton,Qt::NoModifier,window.menuBar()->actionGeometry(menu->menuAction()).center()); events();
               require(menu->isVisible()&&menu->windowHandle()&&QTest::qWaitForWindowExposed(menu->windowHandle()),"Help popup native surface unavailable");
               require(menu->rect().contains(menu->actionGeometry(action)),"Help link outside visible popup");
               acceptanceView(*menu);
            };
            const int before=dispatch.urls.size(); open(); QTest::keyClick(menu,Qt::Key_Escape); events();
            require(!menu->isVisible()&&dispatch.urls.size()==before,"Canceled Help popup launched a destination"); invariant();
            open(); QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center()); events();
            require(!menu->isVisible()&&dispatch.urls.size()==before+1&&dispatch.urls.last()==QUrl(link.second),"Help link dispatched wrong URL or duplicate request");
            invariant(); requests.append(QJsonObject{{"phase",phase},{"action",link.first},{"url",dispatch.urls.last().toString(QUrl::FullyEncoded)},{"native_popup",true},{"cancel_requests",0},{"accepted_requests",1},{"source_history_selection_disk_unchanged",true}});
            save("dispatches.json",QJsonDocument(requests).toJson());
         }
         if (phase=="dirty") {
            auto *about=window.findChild<QAction *>("aboutGMAT");
            require(about&&menu->actions().contains(about)&&about->isEnabled(),"Help About action unavailable");
            bool seen=false; std::exception_ptr failure; QTimer timer; timer.setInterval(20);
            QObject::connect(&timer,&QTimer::timeout,&window,[&] {
               auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
               if (!dialog||dialog->objectName()!="aboutDialog") return;
               timer.stop(); seen=true;
               try {
                  require(dialog->windowHandle()&&QTest::qWaitForWindowExposed(dialog->windowHandle()),"Help About native surface unavailable");
                  acceptanceView(*dialog);
                  auto *buttons=dialog->findChild<QDialogButtonBox *>();
                  auto *close=buttons ? buttons->button(QDialogButtonBox::Close) : nullptr;
                  require(close&&close->isVisible()&&close->isEnabled(),"Help About Close unavailable");
                  QTest::mouseClick(close,Qt::LeftButton); require(!dialog->isVisible(),"Help About Close failed");
                  ++aboutDialogs;
               } catch (...) { failure=std::current_exception(); dialog->reject(); }
            });
            const int before=dispatch.urls.size();
            QTest::mouseClick(window.menuBar(),Qt::LeftButton,Qt::NoModifier,window.menuBar()->actionGeometry(menu->menuAction()).center()); events();
            require(menu->isVisible(),"Help About popup did not open"); acceptanceView(*menu); timer.start();
            QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(about).center());
            timer.stop(); if (failure) std::rethrow_exception(failure);
            require(seen&&aboutDialogs==1&&dispatch.urls.size()==before&&!menu->isVisible(),"Help About pointer route or URL isolation failed");
            invariant(); save("about-route.json",QJsonDocument(QJsonObject{{"native_exposed",true},{"actual_menu_pointer",true},{"pending_source_history_preserved",true},{"closed",true}}).toJson());
         }
      };
      exercise("clean"); require(run("after-clean")==baseline,"Clean Help links changed report bytes");
      source->moveCursor(QTextCursor::End); source->setFocus(); QTest::keyClicks(source,"% pending main edit");
      const auto pending=source->toPlainText();
      inactive->moveCursor(QTextCursor::End); inactive->setFocus(); QTest::keyClicks(inactive,"% pending inactive edit");
      const auto otherPending=inactive->toPlainText();
      require(source->document()->isModified()&&inactive->document()->isModified(),"Help pending fixtures not dirty");
      exercise("dirty"); require(run("after-dirty")==baseline,"Dirty Help links changed report bytes");
      // A complete typed comment is a single Qt undo block; exact restoration is required.
      source->undo(); require(source->toPlainText()==mission,"Help changed main pending Undo");
      source->redo(); require(source->toPlainText()==pending,"Help changed main pending Redo");
      inactive->undo(); require(inactive->toPlainText()==inactiveText,"Help changed inactive pending Undo");
      inactive->redo(); require(inactive->toPlainText()==otherPending,"Help changed inactive pending Redo");
      inactive->document()->setModified(false); selected->close(); events();
      require(window.saveScriptTo(saved)&&read(saved)==pending.toUtf8()&&window.loadScript(saved)&&source->toPlainText()==pending,"Help Unicode source save/reopen failed");
      openInactive(); exercise("reopened"); require(run("reopened")==baseline,"Reopened Help links changed report bytes");
      save("applied.script",pending.toUtf8());
      const QJsonObject proof{{"checkpoint","native-help-links"},{"outcome","passed"},{"family","help-links"},{"link_kinds",3},{"dispatches",dispatch.urls.size()},{"cancellations",9},{"native_popup_visits",19},{"about_dialogs",aboutDialogs},{"raw_report_values",8},{"facets",QJsonArray{"actual-help-menu-pointer-routes-and-native-popups","exact-current-wiki-forum-project-url-dispatch","escape-cancel-clean-dirty-reopened-source-history-and-window-isolation","independent-literal-reports-and-unicode-reopen","actual-help-about-menu-pointer-and-native-close"}},{"scope","Three exact HTTPS destinations via actual native Qt Help menu clicks, Escape cancellation and independent source/history/window/disk/literal-result invariants in clean/dirty/reopened states; actual About pointer/native Close route without repeating existing content tests. QDesktopServices dispatch is intercepted; OS browser launch/failure, network content/redirects/authentication, real OS input/platform/hardware/stress and complete NASA/current-build qualification remain open."}};
      save("proof.json",QJsonDocument(proof).toJson());
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      inactive->document()->setModified(false); source->document()->setModified(false); return 0;
   } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
     catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
   return 1;
}
#include "NasaHelpLinkTests.moc"
