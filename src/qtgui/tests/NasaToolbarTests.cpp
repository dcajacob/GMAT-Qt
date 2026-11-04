#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QInputMethodEvent>
#include <QLineEdit>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <functional>
#include <iostream>
#include <stdexcept>

class WikiDispatch final : public QObject {
 Q_OBJECT
public:
 QList<QUrl> urls;
public slots:
 void receive(const QUrl &url) { urls.append(url); }
};
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Toolbar artifact missing"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Toolbar artifact write failed"); }

int main(int argc,char **argv)
{
 QApplication app(argc,argv); QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
 app.setOrganizationName("GMATTests"); app.setApplicationName("NasaToolbar");
 try {
  require(argc==3&&acceptanceWidgetPlatform(),"Expected startup and toolbar case");
  const QString mode=argv[2],startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Toolbar fixture directory unavailable");
  const auto output=files.filePath("Toolbar Δ.txt"),saved=files.filePath("Active Δ.script");
  auto mission=[&](int value) { return QString("% Toolbar source Δ\nCreate Variable Keep;\nKeep = %1;\nCreate ReportFile Values;\nValues.Filename = '%2';\nValues.WriteHeaders = false;\nBeginMissionSequence;\nReport 'Literal witness' Values Keep; % preserve Ω\n").arg(value).arg(output); };
  const auto initial=mission(9);
  MainWindow window; require(window.initialize(startup),"Toolbar runtime initialization failed"); window.show(); acceptanceView(window); events();
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *area=window.findChild<QMdiArea *>("workspace"); require(source&&area,"Toolbar workspace missing");
  const auto defaultSource=source->toPlainText(); auto *initialSC=Moderator::Instance()->GetConfiguredObject("DefaultSC"); require(initialSC,"Initial default spacecraft missing"); const int initialNaif=initialSC->GetIntegerParameter("NAIFId"),initialNaifFrame=initialSC->GetIntegerParameter("NAIFIdReferenceFrame"); source->setPlainText(initial); require(window.buildScript()&&window.saveScriptTo(saved),"Toolbar literal fixture failed");
  const int initialUndo=source->document()->availableUndoSteps(); QJsonArray observations,actions; int dialogs=0;
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"); auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); }; save("initial.script",initial.toUtf8());
  auto invariant=[&] { auto *keep=Moderator::Instance()->GetConfiguredObject("Keep"); require(keep&&keep->GetRealParameter("Value")==9&&source->toPlainText()==initial&&source->document()->availableUndoSteps()==initialUndo&&!source->document()->isModified(),"Toolbar route changed active source/model/history"); };
  auto button=[&](const QString &name) {
   auto *action=window.findChild<QAction *>(name); require(action,"Toolbar action missing");
   for (auto *bar:window.findChildren<QToolBar *>()) if (bar->actions().contains(action)) {
    auto *widget=qobject_cast<QToolButton *>(bar->widgetForAction(action)); require(widget&&widget->isVisible()&&widget->isEnabled()&&bar->isVisible(),"Toolbar button not visibly reachable");
    require(bar->rect().contains(QRect(widget->mapTo(bar,QPoint()),widget->size())),"Toolbar button outside its bar");
    return widget;
   }
   throw std::runtime_error("Action is only in a menu, not a toolbar");
  };
  auto press=[&](const QString &name) { auto *widget=button(name); acceptanceView(*widget); QTest::mouseClick(widget,Qt::LeftButton); events(); actions.append(name); };
  auto modal=[&](const QString &name,const std::function<void(QWidget *)> &answer) {
   std::exception_ptr failure; bool answered=false; QTimer timer; timer.setInterval(10);
   QObject::connect(&timer,&QTimer::timeout,&window,[&] { auto *dialog=QApplication::activeModalWidget(); if (!dialog) return; timer.stop(); try { acceptanceView(*dialog); ++dialogs; answered=true; answer(dialog); } catch (...) { failure=std::current_exception(); if (auto *box=qobject_cast<QDialog *>(dialog)) box->reject(); } });
   timer.start(); press(name); timer.stop(); if (failure) std::rethrow_exception(failure); require(answered,"Toolbar omitted its expected dialog");
  };
  auto decision=[&](const QString &name,QMessageBox::StandardButton answer) { modal(name,[&](QWidget *dialog) { auto *box=qobject_cast<QMessageBox *>(dialog); require(box&&box->button(answer),"Toolbar close decision unavailable"); QTest::mouseClick(box->button(answer),Qt::LeftButton); }); };
  auto activate=[&](QMdiSubWindow *child) { require(child,"Toolbar document target missing"); child->showNormal(); area->setActiveSubWindow(child); child->widget()->setFocus(); events(); };
  auto *activeChild=area->currentSubWindow();
  if (mode=="Clipboard") {
   const auto oldClipboard=app.clipboard()->text();
   const auto extra=files.filePath("Clipboard Δ.script"),text=QString("% Clipboard Ω\n"); write(extra,text.toUtf8()); require(window.openScriptDocument(extra),"Clipboard independent document unavailable");
   auto *child=area->currentSubWindow(); auto *edit=qobject_cast<QPlainTextEdit *>(child->widget()); require(edit&&edit!=source,"Clipboard selected wrong editor"); edit->setFocus(); auto cursor=edit->textCursor(); cursor.setPosition(2); cursor.setPosition(11,QTextCursor::KeepAnchor); edit->setTextCursor(cursor); const auto selected=cursor.selectedText();
   press("edit_copy"); require(app.clipboard()->text()==selected&&edit->toPlainText()==text&&!edit->document()->isModified(),"Toolbar Copy changed source or copied wrong selection"); invariant();
   edit->setFocus(); press("edit_cut"); const auto cut=edit->toPlainText(); require(cut==QString(text).remove(2,9)&&app.clipboard()->text()==selected&&edit->document()->isModified(),"Toolbar Cut removed wrong text or lost clipboard"); invariant();
   edit->setFocus(); press("edit_paste"); require(edit->toPlainText()==text,"Toolbar Paste did not restore exact selected text"); invariant();
   edit->undo(); require(edit->toPlainText()==cut,"Clipboard Paste did not form an undoable edit"); edit->undo(); require(edit->toPlainText()==text,"Clipboard Cut Undo failed"); edit->redo(); require(edit->toPlainText()==cut,"Clipboard Cut Redo failed"); edit->redo(); require(edit->toPlainText()==text,"Clipboard Paste Redo failed");
   observations.append(QJsonObject{{"selection",selected},{"original",text},{"cut",cut},{"restored",edit->toPlainText()}}); save("clipboard.json",QJsonDocument(observations).toJson()); app.clipboard()->setText(oldClipboard); edit->document()->setModified(false); child->close(); events(); activate(activeChild);
  } else if (mode=="NewBlank") {
   const auto count=area->subWindowList().size(); press("newScriptDocument"); auto *child=area->currentSubWindow(); auto *edit=qobject_cast<QPlainTextEdit *>(child->widget()); require(edit&&edit!=source&&edit->toPlainText().isEmpty()&&!edit->document()->isModified()&&area->subWindowList().size()==count+1,"Toolbar blank editor replaced active mission or contained default source"); invariant(); observations.append(QJsonObject{{"blank",true},{"windows_added",1}}); child->close(); events(); activate(activeChild);
  } else if (mode=="Open") {
   const auto path=files.filePath("Open Ω.script"),replacement=mission(42); write(path,replacement.toUtf8());
   modal("openMissionScript",[&](QWidget *dialog) { auto *picker=qobject_cast<QFileDialog *>(dialog); require(picker&&picker->fileMode()==QFileDialog::ExistingFile,"Toolbar file browser has wrong contract"); picker->reject(); }); invariant();
   modal("openMissionScript",[&](QWidget *dialog) { auto *picker=qobject_cast<QFileDialog *>(dialog); require(picker,"Toolbar file browser missing"); auto *filename=picker->findChild<QLineEdit *>("fileNameEdit"); require(filename&&filename->isVisible(),"Toolbar browser filename input missing"); filename->setFocus(); filename->selectAll(); QInputMethodEvent input; input.setCommitString(path); QApplication::sendEvent(filename,&input); events(); require(QTest::qWaitFor([&] { return picker->selectedFiles()==QStringList{path}; },1500),"Toolbar file browser did not select the intended Unicode path"); auto *buttons=picker->findChild<QDialogButtonBox *>(); require(buttons&&buttons->button(QDialogButtonBox::Open)&&buttons->button(QDialogButtonBox::Open)->isEnabled(),"Toolbar browser Open unavailable"); QTest::mouseClick(buttons->button(QDialogButtonBox::Open),Qt::LeftButton); require(picker->result()==QDialog::Accepted&&!picker->isVisible(),"Toolbar file browser did not finish accepting"); }); save("opened-actual.script",source->toPlainText().toUtf8()); save("opened-expected.script",replacement.toUtf8()); require(source->toPlainText()==replacement&&!source->document()->isModified()&&read(path)==replacement.toUtf8(),"Toolbar Open lost selected Unicode file bytes");
   require(window.buildScript()&&window.runMission()==MainWindow::RunResult::Completed&&QString::fromUtf8(read(output)).trimmed()=="42","Opened mission did not execute its independent literal"); save("opened-values.txt",read(output)); source->setPlainText(initial); require(window.buildScript()&&window.saveScriptTo(saved),"Toolbar Open cleanup failed");
  } else if (mode=="Save") {
   const auto path=files.filePath("Selected Ω.script"),text=QString("% independent Ω\n"); write(path,text.toUtf8()); require(window.openScriptDocument(path),"Toolbar Save document missing"); auto *child=area->currentSubWindow(); auto *edit=qobject_cast<QPlainTextEdit *>(child->widget()); edit->appendPlainText("% exact additional Δ"); const auto changed=edit->toPlainText(); press("saveScript"); require(read(path)==changed.toUtf8()&&!edit->document()->isModified()&&read(saved)==initial.toUtf8(),"Toolbar Save wrote wrong document or bytes"); invariant(); save("saved-selected.script",read(path)); child->close(); events(); activate(activeChild);
  } else if (mode=="Default") {
   source->appendPlainText("% pending Δ"); const auto dirty=source->toPlainText(); const int undo=source->document()->availableUndoSteps(); decision("loadDefaultMission",QMessageBox::Cancel); require(source->toPlainText()==dirty&&source->document()->availableUndoSteps()==undo&&source->document()->isModified()&&read(saved)==initial.toUtf8(),"Default mission Cancel lost unsaved source/history/disk");
   decision("loadDefaultMission",QMessageBox::Discard);
   auto defaultPayload=[&](QString text) { const QRegularExpression created("^%Created: (\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2})\\n",QRegularExpression::MultilineOption); auto matches=created.globalMatch(text); require(matches.hasNext(),"Default mission creation metadata missing"); const auto match=matches.next(); require(!matches.hasNext(),"Default mission creation metadata duplicated"); const auto time=QDateTime::fromString(match.captured(1),"yyyy-MM-dd HH:mm:ss"); require(time.isValid()&&std::abs(time.secsTo(QDateTime::currentDateTime()))<120,"Default mission creation timestamp invalid"); text.remove(match.capturedStart(),match.capturedLength()); return text; };
   auto *freshSC=Moderator::Instance()->GetConfiguredObject("DefaultSC"); require(freshSC,"Fresh default spacecraft missing"); const int naif=freshSC->GetIntegerParameter("NAIFId"),frame=freshSC->GetIntegerParameter("NAIFIdReferenceFrame"); require(naif<=-10000001&&(-(naif+1))%1000==0&&frame==naif+1000000&&naif!=initialNaif,"Default spacecraft dynamic NAIF identity invalid");
   auto expectedDefault=defaultSource; expectedDefault.replace("DefaultSC.NAIFId = "+QString::number(initialNaif)+";","DefaultSC.NAIFId = "+QString::number(naif)+";"); expectedDefault.replace("DefaultSC.NAIFIdReferenceFrame = "+QString::number(initialNaifFrame)+";","DefaultSC.NAIFIdReferenceFrame = "+QString::number(frame)+";");
   save("default-identity.json",QJsonDocument(QJsonObject{{"initial_naif",initialNaif},{"initial_frame",initialNaifFrame},{"fresh_naif",naif},{"fresh_frame",frame}}).toJson()); save("default-initial.script",defaultSource.toUtf8()); save("default-expected.script",expectedDefault.toUtf8()); save("default.script",source->toPlainText().toUtf8()); require(defaultPayload(source->toPlainText())==defaultPayload(expectedDefault)&&!source->document()->isModified()&&Moderator::Instance()->GetConfiguredObject("DefaultSC")&&Moderator::Instance()->GetConfiguredObject("DefaultProp")&&source->toPlainText().contains("Propagate"),"Toolbar did not load the native default mission"); save("default.script",source->toPlainText().toUtf8()); source->setPlainText(initial); require(window.buildScript()&&window.saveScriptTo(saved),"Default mission cleanup failed");
  } else if (mode=="CloseCurrent"||mode=="CloseAll") {
   QList<QPointer<QMdiSubWindow>> children; QStringList paths,texts;
   for (int i=0;i<2;++i) { const auto path=files.filePath(QString("Close %1 Ω.script").arg(i)),text=QString("% close buffer %1 Ω\n").arg(i); write(path,text.toUtf8()); require(window.openScriptDocument(path),"Toolbar close fixture unavailable"); children.append(area->currentSubWindow()); paths.append(path); texts.append(text); }
   auto *edit=qobject_cast<QPlainTextEdit *>(children.last()->widget()); edit->appendPlainText("% pending Δ"); const auto dirty=edit->toPlainText(); const int undo=edit->document()->availableUndoSteps(); const auto action=mode=="CloseCurrent" ? "toolbarCloseCurrent" : "toolbarCloseAll";
   decision(action,QMessageBox::Cancel); require(children.last()&&children.last()->isVisible()&&edit->toPlainText()==dirty&&edit->document()->availableUndoSteps()==undo&&read(paths.last())==texts.last().toUtf8(),"Toolbar close Cancel lost dirty text/history/disk"); invariant();
   decision(action,QMessageBox::Save); require(!children.last()&&read(paths.last())==dirty.toUtf8(),"Toolbar close Save did not persist selected source"); if (mode=="CloseCurrent") require(children.first()&&children.first()->isVisible(),"Close current closed another document"); else { for (auto *child:area->subWindowList()) require(!child->isVisible(),"Close all retained a visible window"); require(!children.first(),"Close all retained another independent script"); } invariant();
   if (children.first()) { children.first()->close(); events(); } activate(activeChild);
  } else if (mode=="About") {
   modal("aboutGMAT",[&](QWidget *dialog) { require(dialog->objectName()=="aboutDialog","Toolbar opened wrong About panel"); auto *info=dialog->findChild<QLabel *>("aboutInfo"); auto *credits=dialog->findChild<QPlainTextEdit *>("aboutCredits"); require(info&&info->text().contains("General Mission Analysis Tool")&&info->text().contains(QString::fromLatin1(qVersion()))&&credits&&credits->isReadOnly()&&credits->toPlainText().contains("NASA/GSFC"),"Toolbar About content incomplete"); save("about.txt",(info->text()+"\n"+credits->toPlainText()).toUtf8()); auto *buttons=dialog->findChild<QDialogButtonBox *>(); require(buttons&&buttons->button(QDialogButtonBox::Close),"Toolbar About Close missing"); QTest::mouseClick(buttons->button(QDialogButtonBox::Close),Qt::LeftButton); }); invariant();
  } else if (mode=="Wiki") {
   WikiDispatch dispatch; QDesktopServices::setUrlHandler("https",&dispatch,"receive"); press("gmatWikiHelp"); QDesktopServices::unsetUrlHandler("https"); require(dispatch.urls==QList<QUrl>{QUrl("https://gmat.atlassian.net/wiki/spaces/GW/overview")},"Toolbar wiki route did not dispatch the NASA GMAT wiki URL exactly once"); observations.append(dispatch.urls.first().toString()); invariant();
  } else if (mode=="RunControl") {
   const QString loop="Create Variable Count;\nCount = 0;\nBeginMissionSequence;\nWhile Count < 1e12;\nCount = Count + 1;\nEndWhile;\n"; source->setPlainText(loop); require(window.buildScript(),"Toolbar run-control loop build failed"); source->document()->setModified(false); const int undo=source->document()->availableUndoSteps();
   require(!window.findChild<QAction *>("pauseMission")->isEnabled()&&!window.findChild<QAction *>("stopMission")->isEnabled(),"Idle toolbar Pause/Stop enabled");
   int phase=0; double paused=0; bool held=false; std::exception_ptr failure; QTimer poll; poll.setInterval(10);
   auto count=[&] { auto *object=Moderator::Instance()->GetInternalObject("Count"); require(object,"Runtime counter missing"); return object->GetRealParameter("Value"); };
   QObject::connect(&poll,&QTimer::timeout,&window,[&] { try {
    if (phase==0&&window.isRunning()&&count()>0) { phase=1; require(source->isReadOnly()&&!window.findChild<QAction *>("newScriptDocument")->isEnabled()&&!window.findChild<QAction *>("toolbarCloseAll")->isEnabled(),"Toolbar permits editing/closing while executing"); press("pauseMission"); require(Moderator::Instance()->GetRunState()==Gmat::PAUSED,"Toolbar Pause did not pause engine"); paused=count(); observations.append(QJsonObject{{"action","pause"},{"count",paused}}); QTimer::singleShot(120,&window,[&] { try { held=count()==paused&&window.isRunning()&&Moderator::Instance()->GetRunState()==Gmat::PAUSED; require(held,"Mission counter advanced while toolbar paused"); phase=2; press("runMission"); require(Moderator::Instance()->GetRunState()==Gmat::RUNNING,"Toolbar Run did not resume paused engine"); } catch (...) { failure=std::current_exception(); window.stopMission(); } }); }
    else if (phase==2&&count()>paused) { phase=3; observations.append(QJsonObject{{"action","stop"},{"count",count()}}); poll.stop(); press("stopMission"); }
   } catch (...) { failure=std::current_exception(); poll.stop(); window.stopMission(); } });
   QTimer watchdog; watchdog.setSingleShot(true); QObject::connect(&watchdog,&QTimer::timeout,&window,[&] { failure=std::make_exception_ptr(std::runtime_error("Toolbar run-control sequence timed out")); window.stopMission(); }); watchdog.start(15000); poll.start(); press("runMission"); poll.stop(); watchdog.stop(); if (failure) std::rethrow_exception(failure);
   require(phase==3&&held&&!window.isRunning()&&!source->isReadOnly()&&source->toPlainText()==loop&&source->document()->availableUndoSteps()==undo&&!window.findChild<QAction *>("pauseMission")->isEnabled()&&!window.findChild<QAction *>("stopMission")->isEnabled(),"Toolbar Run/Pause/Resume/Stop failed to restore idle/source/history"); source->setPlainText(initial); require(window.buildScript()&&window.saveScriptTo(saved),"Stopped toolbar run did not recover");
  } else throw std::runtime_error("Unknown toolbar acceptance case");
  require(window.runMission()==MainWindow::RunResult::Completed&&QString::fromUtf8(read(output)).trimmed()=="9","Toolbar route damaged independent literal mission result");
  save("values.txt",read(output)); save("observations.json",QJsonDocument(observations).toJson());
  const QJsonArray facets{"actual-visible-toolbar-mouse-input","case-specific-document-clipboard-dialog-or-run-oracle","source-model-history-or-exact-replacement-contract","independent-literal-mission-result"};
  const QJsonObject proof{{"checkpoint","toolbar-"+mode},{"outcome","passed"},{"mode",mode},{"actions",actions},{"dialogs",dialogs},{"observations",observations},{"literal_result",9},{"facets",facets},{"scope","Selected visible toolbar routes with exact source/file/clipboard, dirty Cancel/Save/Discard, default mission, About content, wiki URL dispatch and actual Run/Pause/Resume/Stop counter checks. URL dispatch interception does not qualify external browser/network content; arbitrary windows, compact overflow, native OS input, hardware/platform/stress and full current-build qualification remain open."}};
  save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
#include "NasaToolbarTests.moc"
