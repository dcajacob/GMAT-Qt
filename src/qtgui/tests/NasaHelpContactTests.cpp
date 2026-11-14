#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QDesktopServices>
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
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>
#include <QUrl>
#include <QUrlQuery>
#include <QWindow>
#include <cmath>
#include <iostream>
#include <stdexcept>

class ContactDispatch final : public QObject
{
   Q_OBJECT
public:
   ContactDispatch() { for (const auto *scheme:{"http","https","mailto"}) QDesktopServices::setUrlHandler(scheme,this,"receive"); }
   ~ContactDispatch() override { for (const auto *scheme:{"http","https","mailto"}) QDesktopServices::unsetUrlHandler(scheme); }
   QList<QUrl> urls;
public slots:
   void receive(const QUrl &url) { urls.append(url); }
};
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Help contact artifact missing"); return file.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Help contact artifact write failed"); }

int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("NasaHelpContacts");
   try {
      require(argc==2&&acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen","Help contacts require native startup");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Help contact fixtures unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto report=files.filePath("contact values Ω.csv"),saved=files.filePath("contact mission Ω.script"),otherPath=files.filePath("independent Δ.script");
      const QString otherText="% independent contact document Δ\n";
      const QString mission="% preserve contact mission Ω\nCreate Variable Keep Offset;\nKeep = 17;\nOffset = -2.75;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nBeginMissionSequence;\nReport Values Keep Offset;\n";
      const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &bytes) { if (!directory.isEmpty()) write(directory+"/"+name,bytes); };
      MainWindow window; window.resize(1200,850); window.show(); require(window.initialize(startup),"Help contact runtime unavailable"); acceptanceView(window);
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *area=window.findChild<QMdiArea *>("workspace"); auto *menu=window.findChild<QMenu *>("helpMenu");
      require(source&&area&&menu&&window.menuBar()->actions().contains(menu->menuAction()),"Help contact actual menu/workspace unavailable");
      source->setPlainText(mission); save("initial.script",mission.toUtf8()); QJsonArray reports,requests;
      auto run=[&](const QString &phase) {
         require(window.runMission()==MainWindow::RunResult::Completed,"Contact witness mission failed");
         const auto raw=read(report); const auto fields=QString::fromUtf8(raw).trimmed().split(','); require(fields.size()==2,"Contact report cardinality differs"); QJsonArray values;
         for (const auto &field:fields) { bool ok=false; const auto value=field.toDouble(&ok); require(ok&&std::isfinite(value),"Contact report nonfinite"); values.append(value); }
         require(values[0].toDouble()==17&&values[1].toDouble()==-2.75,"Help contacts changed independent literal results");
         reports.append(QJsonObject{{"phase",phase},{"values",values}}); save(phase+".csv",raw); save("reports.json",QJsonDocument(reports).toJson()); return raw;
      };
      const auto baseline=run("baseline"); require(window.saveScriptTo(saved),"Contact clean source save failed"); write(otherPath,otherText.toUtf8());
      QPlainTextEdit *other=nullptr; QMdiSubWindow *selected=nullptr;
      auto selectOther=[&] {
         require(window.openScriptDocument(otherPath),"Contact independent document missing"); selected=area->currentSubWindow(); other=qobject_cast<QPlainTextEdit *>(selected->widget());
         require(other&&other!=source&&other->toPlainText()==otherText,"Contact selected wrong document"); auto cursor=other->textCursor(); cursor.setPosition(2); cursor.setPosition(13,QTextCursor::KeepAnchor); other->setTextCursor(cursor); other->setFocus(); events();
      };
      selectOther(); ContactDispatch dispatch;
      const QList<QPair<QString,QString>> links{
         {"gmatIssueHelp","https://gmat.atlassian.net/browse/GMT/issues/?filter=allissues"},
         {"gmatFeedbackHelp","mailto:gmat-developers@lists.sourceforge.net?subject=GMAT%20Feedback"}
      };
      auto exercise=[&](const QString &phase) {
         selected->showNormal(); area->setActiveSubWindow(selected); other->setFocus(); events();
         const auto before=source->toPlainText(),otherBefore=other->toPlainText(); const auto disk=read(saved),otherDisk=read(otherPath); const auto working=QDir::currentPath();
         const int undo=source->document()->availableUndoSteps(),otherUndo=other->document()->availableUndoSteps(),windows=area->subWindowList().size();
         const bool dirty=source->document()->isModified(),otherDirty=other->document()->isModified(); const auto cursor=source->textCursor(),otherCursor=other->textCursor();
         auto invariant=[&] {
            auto *keep=Moderator::Instance()->GetConfiguredObject("Keep"); auto *offset=Moderator::Instance()->GetConfiguredObject("Offset");
            require(keep&&offset&&keep->GetRealParameter("Value")==17&&offset->GetRealParameter("Value")==-2.75,"Contact dispatch changed configured literals");
            require(source->toPlainText()==before&&other->toPlainText()==otherBefore&&read(saved)==disk&&read(otherPath)==otherDisk,"Contact dispatch changed source/disk bytes");
            require(source->document()->availableUndoSteps()==undo&&other->document()->availableUndoSteps()==otherUndo&&source->document()->isModified()==dirty&&other->document()->isModified()==otherDirty,"Contact dispatch changed pending history/dirty state");
            require(source->textCursor().position()==cursor.position()&&source->textCursor().anchor()==cursor.anchor()&&other->textCursor().position()==otherCursor.position()&&other->textCursor().anchor()==otherCursor.anchor(),"Contact dispatch changed selections");
            require(area->subWindowList().size()==windows&&area->activeSubWindow()==selected&&QDir::currentPath()==working,"Contact dispatch changed selected window/working directory");
         };
         for (const auto &link:links) {
            auto *action=window.findChild<QAction *>(link.first); require(action&&menu->actions().contains(action)&&action->isVisible()&&action->isEnabled(),"Contact action absent/disabled");
            auto open=[&] {
               acceptanceView(window); QTest::mouseClick(window.menuBar(),Qt::LeftButton,Qt::NoModifier,window.menuBar()->actionGeometry(menu->menuAction()).center()); events();
               require(menu->isVisible()&&menu->windowHandle()&&QTest::qWaitForWindowExposed(menu->windowHandle()),"Contact popup native surface unavailable");
               require(menu->rect().contains(menu->actionGeometry(action)),"Contact action outside popup"); acceptanceView(*menu);
            };
            const int count=dispatch.urls.size(); open(); QTest::keyClick(menu,Qt::Key_Escape); events(); require(!menu->isVisible()&&dispatch.urls.size()==count,"Canceled contact menu dispatched a request"); invariant();
            open(); QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center()); events();
            require(!menu->isVisible()&&dispatch.urls.size()==count+1&&dispatch.urls.last()==QUrl(link.second),"Contact dispatched wrong URL or duplicate request");
            const auto url=dispatch.urls.last();
            if (link.first=="gmatFeedbackHelp") {
               const auto query=QUrlQuery(url).queryItems(QUrl::FullyDecoded);
               require(url.scheme()=="mailto"&&url.path()=="gmat-developers@lists.sourceforge.net"&&url.authority().isEmpty()&&url.fragment().isEmpty()&&query==QList<QPair<QString,QString>>{{"subject","GMAT Feedback"}},"Feedback recipient/subject or extra payload differs");
            }
            invariant(); requests.append(QJsonObject{{"phase",phase},{"action",link.first},{"url",url.toString(QUrl::FullyEncoded)},{"native_popup",true},{"canceled_requests",0},{"accepted_requests",1},{"source_history_selection_disk_unchanged",true}}); save("dispatches.json",QJsonDocument(requests).toJson());
         }
      };
      exercise("clean"); require(run("after-clean")==baseline,"Clean contact action changed report bytes");
      source->moveCursor(QTextCursor::End); source->setFocus(); QTest::keyClicks(source,"% pending contact edit"); const auto pending=source->toPlainText();
      other->moveCursor(QTextCursor::End); other->setFocus(); QTest::keyClicks(other,"% pending independent edit"); const auto otherPending=other->toPlainText();
      require(source->document()->isModified()&&other->document()->isModified(),"Contact dirty fixtures unavailable");
      exercise("dirty"); require(run("after-dirty")==baseline,"Dirty contact action changed report bytes");
      source->undo(); require(source->toPlainText()==mission,"Contact main Undo changed"); source->redo(); require(source->toPlainText()==pending,"Contact main Redo changed");
      other->undo(); require(other->toPlainText()==otherText,"Contact selected Undo changed"); other->redo(); require(other->toPlainText()==otherPending,"Contact selected Redo changed");
      other->document()->setModified(false); selected->close(); events(); require(window.saveScriptTo(saved)&&read(saved)==pending.toUtf8()&&window.loadScript(saved)&&source->toPlainText()==pending,"Contact Unicode save/reopen changed source");
      selectOther(); exercise("reopened"); require(run("reopened")==baseline,"Reopened contact action changed report bytes"); save("applied.script",pending.toUtf8());
      const QJsonObject proof{{"checkpoint","native-help-contacts"},{"outcome","passed"},{"family","help-contacts"},{"dispatches",dispatch.urls.size()},{"cancellations",6},{"native_popup_visits",12},{"raw_report_values",8},{"facets",QJsonArray{"actual-native-help-issue-and-feedback-pointer-routes","exact-tracker-url-and-mailto-recipient-subject-without-extra-payload","cancel-clean-dirty-reopened-source-history-window-disk-isolation","independent-literal-reports-and-unicode-reopen"}},{"scope","Current guide Report An Issue/Provide Feedback: six exact intercepted HTTPS/mailto requests and six Escape cancellations, clean/dirty/reopened native Help popups, literal/state/source/history/selection/disk/window invariants. No browser/mail client launch, message delivery, remote content/redirect/authentication, failure handling, OS input/platform/hardware/stress or complete NASA/current-build qualification."}};
      save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      source->document()->setModified(false); other->document()->setModified(false); return 0;
   } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
     catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
   return 1;
}
#include "NasaHelpContactTests.moc"
