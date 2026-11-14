#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "CommandEditor.hpp"
#include "FindReplaceDialog.hpp"
#include <QApplication>
#include <QAction>
#include <QLabel>
#include <QKeyEvent>
#include <QDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QPlainTextEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPointer>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QTableWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTreeWidget>
#include <QComboBox>
#include <QTextDocument>
#include <QTextCursor>
#include <QStatusBar>
#include <QEventLoop>
#include <QWindow>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Document report/file unreadable"); return QString::fromUtf8(file.readAll()); }
static void write(const QString &path,const QString &text) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(text.toUtf8())==text.toUtf8().size(),"Document fixture write failed"); }
static void modal(MainWindow &window,std::function<void()> action,std::function<void(QDialog *)> inspect)
{
   std::exception_ptr failure;
   QTimer::singleShot(0,&window,[&] { auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); try { require(dialog,"Expected document dialog missing"); inspect(dialog); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } });
   action(); if (failure) std::rethrow_exception(failure);
}
static void choice(MainWindow &window,std::function<void()> action,QMessageBox::StandardButton value)
{
   modal(window,action,[value](QDialog *dialog) { auto *box=dynamic_cast<QMessageBox *>(dialog); require(box && box->button(value),"Document choice missing"); box->button(value)->click(); });
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtScriptDocuments");
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid() && argc>1,"Document fixture setup failed");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(),capture=argc>2 ? QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto aDir=files.filePath("first ü"),bDir=files.filePath("second Δ"); require(QDir().mkpath(aDir) && QDir().mkpath(bDir),"Document folders unavailable");
      write(aDir+"/value.inc","Create Variable V;\nV = 3;\n"); write(bDir+"/value.inc","Create Variable V;\nV = 10;\n");
      const auto a=aDir+"/mission.script",b=bDir+"/mission.script",reportA=files.filePath("first result.txt"),reportB=files.filePath("second result.txt");
      const auto source=[](const QString &report) { return "% document comments α\n#Include 'value.inc'\nCreate ReportFile R;\nR.Filename = '"+report+"';\nR.WriteHeaders = false;\nBeginMissionSequence;\nV = V + 4; % preserve arithmetic label/comment\nReport R V;\n"; };
      const auto originalA=source(reportA),originalB=source(reportB); write(a,originalA); write(b,originalB);
      MainWindow window; window.show(); require(window.initialize(startup) && window.loadScript(a),"Primary document open failed");
      auto *area=window.findChild<QMdiArea *>("workspace"); auto *editorA=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editorA,"Active script editor missing");
      auto record=[&](const QString &path) { for (auto *child:area->subWindowList()) if (child->property("scriptDocument").toBool() && child->property("scriptPath").toString()==path) return child; throw std::runtime_error("Script window identity unavailable"); };
      auto *childA=record(a);
      auto action=[&](const char *name) { auto *item=window.findChild<QAction *>(name); require(item,"Document action unavailable"); return item; };
      auto focus=[&](QMdiSubWindow *child) { child->showNormal(); area->setActiveSubWindow(child); child->widget()->setFocus(); app.processEvents(); };
      auto key=[&](QWidget *widget,int code,Qt::KeyboardModifiers modifiers) { QKeyEvent down(QEvent::KeyPress,code,modifiers),up(QEvent::KeyRelease,code,modifiers); QApplication::sendEvent(widget,&down); QApplication::sendEvent(widget,&up); };
      auto run=[&](const QString &report,const QString &expected) { const auto result=window.runMission(); require(result==MainWindow::RunResult::Completed,qPrintable(window.statusBar()->currentMessage())); require(read(report).trimmed()==expected,"Document calculations/relative include changed"); };
      run(reportA,"7");
      editorA->moveCursor(QTextCursor::End); editorA->insertPlainText("% unsaved first document\n"); const auto dirtyA=editorA->toPlainText();
      modal(window,[&] { action("openScriptDocument")->trigger(); },[&](QDialog *dialog) { auto *picker=dynamic_cast<QFileDialog *>(dialog); require(picker,"Additional script picker missing"); picker->selectFile(b); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection); });
      auto *childB=record(b); auto *editorB=qobject_cast<QPlainTextEdit *>(childB->widget()); require(editorB && !childB->property("activeScript").toBool() && childA->property("activeScript").toBool(),"Opening another document changed active identity");
      require(editorA->toPlainText()==dirtyA && editorA->document()->isModified() && Moderator::Instance()->GetConfiguredObject("V")->GetRealParameter("Value")==3,"Opening inactive script changed source/model");
      require(childA->windowTitle().contains("first ü/mission.script") && childB->windowTitle().contains("second Δ/mission.script"),"Equal basenames are ambiguous");
      editorB->moveCursor(QTextCursor::End); editorB->insertPlainText("% unsaved second document\n"); const auto dirtyB=editorB->toPlainText();
      require(window.openScriptDocument(b) && record(b)==childB && editorB->toPlainText()==dirtyB,"Reopening same document lost edits or duplicated window");
      focus(childB); window.activateWindow(); app.processEvents(); if (capture.isEmpty()) key(editorB,Qt::Key_S,Qt::ControlModifier); else action("saveScript")->trigger(); require(read(b)==dirtyB && !editorB->document()->isModified() && editorA->document()->isModified() && window.findChild<QPlainTextEdit *>("scriptEditor")==editorA,"Inactive Save changed active mission or dirty sibling");
      if (capture.isEmpty()) key(editorB,Qt::Key_Z,Qt::ControlModifier); else editorB->undo(); require(editorB->toPlainText()==originalB,"Inactive Save lost Undo"); if (capture.isEmpty()) key(editorB,Qt::Key_Z,Qt::ControlModifier|Qt::ShiftModifier); else editorB->redo(); require(editorB->toPlainText()==dirtyB,"Inactive Redo changed source");
      // Find/Replace and line navigation route to the selected document.
      focus(childB); action("scriptFind")->trigger(); auto *search=dynamic_cast<FindReplaceDialog *>(editorB->findChild<QDialog *>("findReplaceDialog")); require(search && search->isVisible(),"Find routed to another script");
      search->findChild<QComboBox *>("findText")->setEditText("V + 4"); search->findChild<QComboBox *>("replaceText")->setEditText("V + 5"); search->findChild<QPushButton *>("replaceAll")->click(); search->hide();
      const auto editedB=editorB->toPlainText(); require(editedB.contains("V + 5") && editorA->toPlainText()==dirtyA,"Inactive Replace changed another script");
      const QString exactReplacedB="% document comments α\n#Include 'value.inc'\nCreate ReportFile R;\nR.Filename = '"+reportB+"';\nR.WriteHeaders = false;\nBeginMissionSequence;\nV = V + 5; % preserve arithmetic label/comment\nReport R V;\n% unsaved second document\n";
      require(editedB==exactReplacedB && editorB->document()->isModified(),"Replace All did not preserve every unrelated source byte or mark the document dirty");
      focus(childB); modal(window,[&] { action("scriptGoToLine")->trigger(); },[&](QDialog *dialog) { auto *line=dynamic_cast<QInputDialog *>(dialog); require(line && line->intMaximum()==editorB->document()->blockCount(),"Line navigation uses another document"); line->setIntValue(7); line->accept(); }); require(editorB->textCursor().blockNumber()==6,"Inactive line navigation failed");
      // Save As owns only its selected document; cancellation, write failures
      // and collisions leave identities, source and Undo history intact.
      focus(childB); modal(window,[&] { action("saveScriptAs")->trigger(); },[](QDialog *dialog) { dialog->reject(); }); require(childB->property("scriptPath").toString()==b && editorB->toPlainText()==editedB,"Save As Cancel changed identity/source");
      choice(window,[&] { require(!window.saveScriptTo(b),"Saved over an independently open document"); },QMessageBox::Ok); require(read(b)==dirtyB && editorA->toPlainText()==dirtyA,"Save collision changed a file or buffer");
      const auto copy=bDir+"/saved Δ.script"; focus(childB);
      modal(window,[&] { action("saveScriptAs")->trigger(); },[&](QDialog *dialog) { auto *picker=dynamic_cast<QFileDialog *>(dialog); require(picker,"Save As picker missing"); picker->selectFile(copy); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection); });
      require(childB->property("scriptPath").toString()==copy && read(copy)==editedB && window.findChild<QPlainTextEdit *>("scriptEditor")==editorA,"Inactive Save As changed active identity");
      editorB->undo(); require(editorB->toPlainText()==dirtyB,"Inactive Save As lost Undo"); editorB->redo(); require(editorB->toPlainText()==editedB,"Inactive Save As lost Redo");
      focus(childB); editorB->moveCursor(QTextCursor::End); editorB->insertPlainText("% failed save retains history\n"); const auto failedSave=editorB->toPlainText();
      const auto moved=bDir+" moved"; require(QDir().rename(bDir,moved),"Write failure folder move failed"); choice(window,[&] { action("saveScript")->trigger(); },QMessageBox::Ok);
      require(editorB->document()->isModified() && editorB->toPlainText()==failedSave && childB->property("scriptPath").toString()==copy && read(moved+"/saved Δ.script")==editedB,"Failed inactive Save changed source/identity/file");
      editorB->undo(); require(editorB->toPlainText()==editedB,"Failed inactive Save lost Undo"); editorB->redo(); require(editorB->toPlainText()==failedSave,"Failed inactive Save lost Redo"); editorB->undo(); require(QDir().rename(moved,bDir),"Write failure folder restore failed");
      focus(childB); action("activateScriptDocument")->trigger(); require(window.findChild<QPlainTextEdit *>("scriptEditor")==editorB && childB->property("activeScript").toBool() && !childA->property("activeScript").toBool() && editorA->toPlainText()==dirtyA,"Explicit activation lost inactive source/history"); run(reportB,"15");
      focus(childA); editorA->undo(); require(editorA->toPlainText()==originalA,"Activation destroyed former active Undo"); editorA->redo(); require(editorA->toPlainText()==dirtyA,"Activation destroyed former active Redo"); action("activateScriptDocument")->trigger(); run(reportA,"7");
      // Main command edits target the active model even while the other
      // script is being read. The second source and its Undo remain independent.
      focus(childB); auto snapshot=window.missionSnapshot(); int assignment=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("V = V + 4")) assignment=i; require(assignment>=0,"Active assignment unavailable");
      const auto editError=window.applyMissionChange(snapshot,assignment,MissionEdit::Replace,"V = V + 7; % preserve arithmetic label/comment"); require(editError.isEmpty(),qPrintable(editError)); const auto configuredA=editorA->toPlainText(); require(configuredA.contains("V + 7") && editorB->toPlainText()==editedB,"GUI command edited inactive source"); run(reportA,"10"); editorA->undo(); require(editorA->toPlainText()==dirtyA,"Active GUI Undo not exact"); editorA->redo(); require(editorA->toPlainText()==configuredA,"Active GUI Redo not exact");
      // Pending resource panels offer Apply/Discard/Cancel before activation.
      auto *tree=window.findChild<QTreeWidget *>("Resources"); auto matches=tree->findItems("R",Qt::MatchExactly|Qt::MatchRecursive); require(matches.size()==1,"Report resource missing"); tree->itemDoubleClicked(matches.first(),0);
      auto *panelWindow=area->activeSubWindow(); auto *table=panelWindow->widget()->findChild<QTableWidget *>(); require(table,"Report settings unavailable"); for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="Precision") table->item(row,1)->setText("15");
      focus(childB); choice(window,[&] { action("activateScriptDocument")->trigger(); },QMessageBox::Cancel); require(window.findChild<QPlainTextEdit *>("scriptEditor")==editorA && editorA->toPlainText()==configuredA,"Cancelled activation changed active source/model");
      focus(childB); choice(window,[&] { action("activateScriptDocument")->trigger(); },QMessageBox::Apply); require(window.findChild<QPlainTextEdit *>("scriptEditor")==editorB && editorA->toPlainText().contains("R.Precision = 15"),qPrintable("Pending Apply failed: "+window.statusBar()->currentMessage()+"\n"+(panelWindow->widget()->findChild<QLabel *>("resourceStatus") ? panelWindow->widget()->findChild<QLabel *>("resourceStatus")->text() : QString())+"\n"+editorA->toPlainText())); run(reportB,"15");
      // A bad inactive script fails without taking the active mission away.
      focus(childA); editorA->moveCursor(QTextCursor::End); editorA->insertPlainText("UnknownDocumentCommand;\n"); const auto invalidA=editorA->toPlainText(); action("activateScriptDocument")->trigger(); require(window.findChild<QPlainTextEdit *>("scriptEditor")==editorB && editorA->toPlainText()==invalidA,"Failed activation changed active or lost invalid buffer"); run(reportB,"15"); editorA->undo(); const auto repairedA=editorA->toPlainText(); focus(childA); action("activateScriptDocument")->trigger(); run(reportA,"10"); require(editorA->toPlainText()==repairedA,"Recovered activation rewrote source");
      // Inactive Reload preserves the active model and has explicit Cancel;
      // invalid UTF-8/missing reads preserve the buffer before asking to replace.
      focus(childB); editorB->moveCursor(QTextCursor::End); editorB->insertPlainText("% retain reload undo\n"); const auto beforeReload=editorB->toPlainText(); auto onDisk=editedB; onDisk.replace("V + 5","V + 6"); write(copy,onDisk);
      choice(window,[&] { action("reloadScriptDocument")->trigger(); },QMessageBox::Cancel); require(editorB->toPlainText()==beforeReload && editorB->document()->isModified(),"Reload Cancel lost edits");
      choice(window,[&] { action("reloadScriptDocument")->trigger(); },QMessageBox::Yes); require(editorB->toPlainText()==onDisk && !editorB->document()->isModified() && window.findChild<QPlainTextEdit *>("scriptEditor")==editorA,"Inactive Reload changed active mission"); run(reportA,"10");
      QFile bad(copy); require(bad.open(QIODevice::WriteOnly),"Encoding fixture unavailable"); bad.write("bad\xff"); bad.close(); focus(childB); choice(window,[&] { action("reloadScriptDocument")->trigger(); },QMessageBox::Ok); require(editorB->toPlainText()==onDisk && !editorB->document()->isModified(),"Malformed Reload changed buffer"); write(copy,onDisk);
      focus(childB); action("saveRunMission")->trigger(); require(window.findChild<QPlainTextEdit *>("scriptEditor")==editorB && !editorB->document()->isModified(),"Save/build/run did not activate selected document"); require(read(reportB).trimmed()=="16","Selected Save/build/run used wrong include/calculation");
      if (!capture.isEmpty()) { area->tileSubWindows(); QEventLoop loop; QTimer::singleShot(250,&loop,&QEventLoop::quit); loop.exec(); require(window.windowHandle() && window.windowHandle()->isExposed() && window.grab().save(capture),"Native document workspace not exposed or capture failed"); }
      // Runtime editing and document close/switch are protected for all editors.
      const auto goodB=editorB->toPlainText(); editorB->setPlainText("Create Variable N;\nBeginMissionSequence;\nWhile N < 1e12;\nN = N + 1;\nEndWhile;\n"); bool protectedRun=false;
      QTimer::singleShot(60,&window,[&] { protectedRun=editorA->isReadOnly() && editorB->isReadOnly() && !window.openScriptDocument(a) && !window.saveScriptTo(copy) && !childA->close() && !action("activateScriptDocument")->isEnabled(); window.stopMission(); }); require(window.runMission()==MainWindow::RunResult::Stopped && protectedRun,"Runtime document protection/Stop failed"); editorB->setPlainText(goodB); editorB->document()->setModified(false); run(reportB,"16");
      // Closing the active editor leaves the loaded mission usable; reopening
      // its path restores that same buffer. Inactive close honors dirty choices.
      focus(childB); action("closeScriptDocument")->trigger(); require(!childB->isVisible() && window.findChild<QPlainTextEdit *>("scriptEditor")==editorB,"Closing active editor changed active mission"); run(reportB,"16"); require(window.openScriptDocument(copy) && record(copy)==childB && childB->isVisible(),"Active editor reopen lost identity");
      focus(childA); choice(window,[&] { action("closeScriptDocument")->trigger(); },QMessageBox::Cancel); require(childA->isVisible() && editorA->toPlainText()==repairedA,"Dirty inactive close Cancel lost state"); QPointer<QMdiSubWindow> closed=childA; choice(window,[&] { action("closeScriptDocument")->trigger(); },QMessageBox::Discard); app.sendPostedEvents(nullptr,QEvent::DeferredDelete); require(!closed,"Discard did not close inactive document"); run(reportB,"16");
      action("newScriptDocument")->trigger(); auto *blank=area->activeSubWindow(); require(blank && blank->property("scriptDocument").toBool() && !blank->property("activeScript").toBool(),"New script changed active mission"); auto *blankEditor=qobject_cast<QPlainTextEdit *>(blank->widget()); blankEditor->insertPlainText("% unsaved final document\n");
      choice(window,[&] { window.close(); },QMessageBox::Cancel); require(window.isVisible() && blankEditor->document()->isModified(),"Main close ignored inactive dirty document");
      focus(blank); choice(window,[&] { action("closeScriptDocument")->trigger(); },QMessageBox::Discard); app.sendPostedEvents(nullptr,QEvent::DeferredDelete);
      // Rejected pending Apply cannot switch the active model, and Discard
      // closes the obsolete panel without modifying the former active source.
      require(window.openScriptDocument(a),"Pending recovery document missing"); auto *reopenedA=record(a);
      tree->itemDoubleClicked(tree->findItems("R",Qt::MatchExactly|Qt::MatchRecursive).first(),0); auto *pendingWindow=area->activeSubWindow(); auto *pendingTable=pendingWindow->widget()->findChild<QTableWidget *>(); require(pendingTable,"Pending recovery panel missing");
      int precisionRow=-1; for (int row=0;row<pendingTable->rowCount();++row) if (pendingTable->item(row,0)->text()=="Precision") precisionRow=row; require(precisionRow>=0,"Pending recovery field missing"); pendingTable->item(precisionRow,1)->setText("-1");
      focus(reopenedA); choice(window,[&] { action("activateScriptDocument")->trigger(); },QMessageBox::Apply); require(window.findChild<QPlainTextEdit *>("scriptEditor")==editorB && editorB->toPlainText()==goodB && pendingWindow->isVisible() && pendingTable->item(precisionRow,1)->text()=="-1","Rejected pending Apply lost source/pending settings or switched mission");
      focus(reopenedA); choice(window,[&] { action("activateScriptDocument")->trigger(); },QMessageBox::Discard); require(editorB->toPlainText()==goodB && window.findChild<QPlainTextEdit *>("scriptEditor")==reopenedA->widget(),"Pending Discard changed former active source"); run(reportA,"7");
      focus(childB); action("activateScriptDocument")->trigger(); run(reportB,"16"); focus(reopenedA); action("closeScriptDocument")->trigger(); app.sendPostedEvents(nullptr,QEvent::DeferredDelete);
      // Active Reload invalidates the old model; explicit Cancel preserves
      // source/history, and the next normal Run rebuilds the saved document.
      focus(childB); editorB->moveCursor(QTextCursor::End); editorB->insertPlainText("% active reload pending\n"); const auto activeReload=editorB->toPlainText();
      choice(window,[&] { action("reloadScriptDocument")->trigger(); },QMessageBox::Cancel); require(editorB->toPlainText()==activeReload && editorB->document()->isModified(),"Active Reload Cancel lost source");
      choice(window,[&] { action("reloadScriptDocument")->trigger(); },QMessageBox::Yes); require(editorB->toPlainText()==goodB && !editorB->document()->isModified() && window.missionSnapshot().nodes.isEmpty(),"Active Reload left stale mission commands"); run(reportB,"16");
      // The serializer's duplicate configuration directive is ignored above,
      // but commands owned by an actual mission include remain a text workflow.
      write(bDir+"/commands.inc","V = V + 2;\n"); auto expanded=goodB; expanded.replace("Report R V;","#Include 'commands.inc'\nReport R V;"); editorB->setPlainText(expanded); require(window.buildScript(),"Mission include fixture failed");
      auto included=window.missionSnapshot(); int includedAssignment=-1; for (int i=0;i<included.nodes.size();++i) if (included.nodes[i].type=="GMAT") { includedAssignment=i; break; }
      require(includedAssignment>=0 && !included.nodes[includedAssignment].editable && !window.applyMissionChange(included,includedAssignment,MissionEdit::Replace,"V = 100;").isEmpty() && editorB->toPlainText()==expanded,"Cross-file mission command was patched into the wrong source"); run(reportB,"18");
      editorB->setPlainText(goodB); editorB->document()->setModified(false); run(reportB,"16");
      std::cout<<R"(NASA_CHECK {"checkpoint":"script.replace","family":"script-editor","outcome":"passed","facets":["dialog","replace-all","exact-source","undo-redo","save","document-isolation","numeric-report"]})"<<std::endl;
      std::cout<<"PASS: independent active/inactive script windows, equal basename identity and duplicate-open protection, Unicode/relative includes, per-document Save/Save As/Undo/Redo/Find/Replace/line navigation and synthetic shortcut routing, collision, write failure and Cancel protection, explicit activation with independent 7/10/15/16 reports, active GUI command edits and pending Apply/Discard/Cancel and rejected-Apply recovery, failed activation restoration and correction, active/inactive Reload/UTF-8 recovery, configuration-include source mapping and cross-file command protection, selected Save/build/run, runtime Stop and all-document protection, active hidden/reopen and inactive/main close choices.\n"; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
