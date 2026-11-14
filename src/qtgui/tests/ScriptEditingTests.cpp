#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatGlobal.hpp"
#include "GmatCommand.hpp"
#include "FileManager.hpp"
#include <memory>
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QPlainTextEdit>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QTimer>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QStatusBar>
#include <QTextBlock>
#include <QRegularExpression>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"File missing"); return QString::fromUtf8(file.readAll()); }
static void write(const QString &path,const QString &text) { QFile file(path); require(file.open(QIODevice::WriteOnly),"Fixture unavailable"); const auto bytes=text.toUtf8(); require(file.write(bytes)==bytes.size(),"Fixture write failed"); }
template<class Open,class Action> static void modal(MainWindow &window,Open open,Action action)
{
   std::exception_ptr failure;
   QTimer::singleShot(0,&window,[&] { auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
      try { require(dialog,"Expected modal missing"); action(dialog); }
      catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   });
   open(); if (failure) std::rethrow_exception(failure);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtScriptEditing");
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid() && argc>1,"Fixture setup failed");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");
      require(editor && messages,"Script/message controls missing");
      auto *saveBuild=window.findChild<QAction *>("saveBuildScript"),*saveRun=window.findChild<QAction *>("saveRunMission"),*go=window.findChild<QAction *>("scriptGoToLine");
      require(saveBuild && saveRun && go,"Script actions missing");
      require(window.availableEngineTypes().contains("CommandEcho"),"ScriptTools command missing");
      const auto directory=files.filePath("document folder"); require(QDir().mkpath(directory),"Document folder unavailable");
      const auto saved=directory+"/mission ü.script",report=files.filePath("script results.txt");
      const QString source="% editable script Δ\nCreate Variable x;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nBeginMissionSequence;\nCommandEcho 'trace on' Off; % keep trace\nx = 2;\nReport Values x;\nCommandEcho 'trace off' Off; % stop trace\nx = 5;\nReport Values x;\n";
      editor->setPlainText(source); editor->document()->setModified(true);
      modal(window,[&] { saveBuild->trigger(); },[](QDialog *dialog) { require(dynamic_cast<QFileDialog *>(dialog),"Save chooser missing"); dialog->reject(); });
      require(editor->toPlainText()==source && editor->document()->isModified() && !QFileInfo::exists(saved) && Moderator::Instance()->GetConfiguredObject("DefaultSC"),"Save Cancel changed source/model/files");
      modal(window,[&] { saveBuild->trigger(); },[&](QDialog *dialog) { auto *picker=dynamic_cast<QFileDialog *>(dialog); require(picker,"Save chooser missing"); picker->selectFile(saved); QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection); });
      require(read(saved)==source && !editor->document()->isModified() && Moderator::Instance()->GetConfiguredObject("x") && !QFileInfo::exists(report),"Save/build did not save and build without running");
      const auto position=editor->textCursor().position();
      modal(window,[&] { go->trigger(); },[](QDialog *dialog) { require(dynamic_cast<QInputDialog *>(dialog),"Line chooser missing"); dialog->reject(); });
      require(editor->textCursor().position()==position,"Line Cancel moved cursor");
      modal(window,[&] { go->trigger(); },[&](QDialog *dialog) { auto *picker=dynamic_cast<QInputDialog *>(dialog); require(picker && picker->intMaximum()==editor->document()->blockCount(),"Line bounds incorrect"); picker->setIntValue(8); picker->accept(); });
      require(editor->textCursor().blockNumber()==7 && editor->toPlainText()==source && !editor->document()->isModified(),"Line navigation altered source or missed line");
      const auto snapshot=window.missionSnapshot(); int index=-1;
      for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="CommandEcho") { index=i; break; }
      require(index>=0,"CommandEcho missing from Mission"); QString error;
      {
         QWidget owner; CommandEditor panel(snapshot.nodes[index].statement,false,{},[&](const auto &text) { error=window.applyMissionChange(snapshot,index,MissionEdit::Replace,text); return error; },{},{},&owner);
         auto *state=panel.findChild<QComboBox *>("commandEchoState"); require(state && state->count()==2 && !state->isEditable(),"Echo selector is not restricted to On/Off"); state->setCurrentText("On");
         require(editor->toPlainText()==source && panel.hasChanges(),"Pending echo changed mission");
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
      }
      const auto configured=editor->toPlainText(); require(configured.contains("CommandEcho 'trace on' On; % keep trace"),"Echo Apply lost name/comment");
      editor->undo(); require(editor->toPlainText()==source,"Echo Undo not exact"); editor->redo(); require(editor->toPlainText()==configured,"Echo Redo not exact");
      messages->clear(); saveRun->trigger();
      require(read(saved)==configured && !editor->document()->isModified(),"Save/run lost source or dirty state");
      const auto values=read(report).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(values.size()==2 && values[0].toDouble()==2 && values[1].toDouble()==5,"Save/run calculations incorrect");
      const auto trace=messages->toPlainText(); require(QRegularExpression("CurrentCommand:[^\\n]*x[^\\n]*2").match(trace).hasMatch() && !QRegularExpression("CurrentCommand:[^\\n]*x[^\\n]*5").match(trace).hasMatch(),"Echo On/Off execution did not bound tracing");
      require(!GmatGlobal::Instance()->EchoCommands(),"Echo setting leaked after RunComplete");
      require(window.loadScript(saved) && window.buildScript(),"Script reopen failed"); messages->clear(); require(window.runMission()==MainWindow::RunResult::Completed && read(report).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)==values,"Reopened script calculations changed");
      require(!GmatGlobal::Instance()->EchoCommands(),"Reopened echo setting leaked");
      GmatGlobal::Instance()->SetCommandEchoMode(true);
      require(window.runMission()==MainWindow::RunResult::Completed && GmatGlobal::Instance()->EchoCommands(),"Echo did not restore an initially enabled setting");
      GmatGlobal::Instance()->SetCommandEchoMode(false);
      const auto valid=editor->toPlainText(); auto invalidSnapshot=window.missionSnapshot(); index=-1;
      for (int i=0;i<invalidSnapshot.nodes.size();++i) if (invalidSnapshot.nodes[i].type=="CommandEcho") { index=i; break; }
      require(!window.applyMissionChange(invalidSnapshot,index,MissionEdit::Replace,"CommandEcho Invalid;").isEmpty() && editor->toPlainText()==valid,"Invalid echo edit did not roll back");
      require(window.runMission()==MainWindow::RunResult::Completed && !GmatGlobal::Instance()->EchoCommands(),"Echo failure recovery failed");
      {
         auto *command=Moderator::Instance()->GetFirstCommand(); while (command && command->GetTypeName()!="CommandEcho") command=command->GetNext(); require(command,"Echo command missing for clone check");
         const auto statement=command->GetGeneratingString(Gmat::NO_COMMENTS); std::unique_ptr<GmatCommand> copy(dynamic_cast<GmatCommand *>(command->Clone())); require(copy && copy->GetGeneratingString(Gmat::NO_COMMENTS)==statement,"Echo clone lost configuration/name");
         GmatGlobal::Instance()->SetCommandEchoMode(true); copy->RunComplete(); require(GmatGlobal::Instance()->EchoCommands(),"Unexecuted echo cleanup changed current setting");
         require(copy->Initialize() && copy->Execute(),"Cloned echo failed to initialize/execute"); copy->RunComplete(); require(GmatGlobal::Instance()->EchoCommands(),"Cloned echo did not restore initial setting");
         GmatGlobal::Instance()->SetCommandEchoMode(false); copy->RunComplete(); require(!GmatGlobal::Instance()->EchoCommands(),"Repeated echo cleanup overwrote a later user setting");
      }
      for (bool initial : {false,true}) {
         GmatGlobal::Instance()->SetCommandEchoMode(initial);
         editor->setPlainText("Create Variable x;\nBeginMissionSequence;\nCommandEcho On;\nx = 2;\nSave x;\n");
         const auto outputPath=FileManager::Instance()->GetAbsPathname("OUTPUT_PATH"); FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",files.filePath("missing/nested").toStdString());
         const auto failed=window.runMission(); FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",outputPath); if (failed!=MainWindow::RunResult::Failed || GmatGlobal::Instance()->EchoCommands()!=initial) { app.processEvents(); throw std::runtime_error("Failed execution lifecycle result="+std::to_string(static_cast<int>(failed))+" initial="+std::to_string(initial)+" actual="+std::to_string(GmatGlobal::Instance()->EchoCommands())+"\n"+messages->toPlainText().right(4000).toStdString()); }
         editor->setPlainText("Create Variable count;\nBeginMissionSequence;\nCommandEcho On;\nWhile count < 1e12;\nIf count == 0;\nCommandEcho Off;\nEndIf;\ncount = count + 1;\nEndWhile;\n"); QTimer::singleShot(80,&window,[&] { window.stopMission(); });
         require(window.runMission()==MainWindow::RunResult::Stopped && GmatGlobal::Instance()->EchoCommands()==initial,"Stopped nested execution leaked echo setting");
      }
      GmatGlobal::Instance()->SetCommandEchoMode(false); require(window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Stopped/failed echo reopen recovery failed");

      // A failed save must prevent both interpretation and execution.
      require(QDir().rename(directory,directory+" backup"),"Save failure setup failed");
      editor->appendPlainText("% retained pending edit"); const auto pending=editor->toPlainText(),title=window.windowTitle(),previousReport=read(report);
      QApplication::processEvents(); messages->clear();
      modal(window,[&] { saveRun->trigger(); },[](QDialog *dialog) { auto *warning=dynamic_cast<QMessageBox *>(dialog); require(warning && warning->windowTitle()=="Save failed","Save failure not explained"); warning->accept(); });
      require(editor->toPlainText()==pending && editor->document()->isModified(),"Failed save lost pending edits");
      require(window.windowTitle()==title,"Failed save changed document identity");
      require(read(report)==previousReport,"Failed save changed report");
      require(!messages->toPlainText().contains("CurrentCommand:"),"Failed save ran mission");
      require(QDir().rename(directory+" backup",directory),"Save recovery setup failed"); saveRun->trigger(); require(read(saved)==pending && !editor->document()->isModified(),"Save/run did not recover");
      const auto good=editor->toPlainText(),goodReport=read(report);
      QString invalid=good; invalid.replace("CommandEcho 'trace on' On","CommandEcho 'trace on' Invalid");
      editor->setPlainText(invalid); saveRun->trigger();
      require(read(saved)==invalid && editor->toPlainText()==invalid && read(report)==goodReport && window.statusBar()->currentMessage().contains("Build failed"),"Invalid saved script executed or lost its diagnostic/source");
      write(saved,good); require(window.loadScript(saved) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Invalid-script reopen recovery failed");
      const auto restored=editor->toPlainText(); editor->setPlainText(" \n\t"); editor->document()->setModified(true); saveRun->trigger();
      require(read(saved)==good && editor->document()->isModified() && window.statusBar()->currentMessage().contains("Enter a mission"),"Empty script overwrote the saved mission");
      editor->setPlainText(restored); editor->document()->setModified(false);
      if (argc>2) window.grab().save(QString::fromLocal8Bit(argv[2]));
      std::cout<<R"(NASA_CHECK {"checkpoint":"script.go-to-line","family":"script-editor","outcome":"passed","facets":["dialog","cancel","line-bound","exact-line","source-unchanged","dirty-state"]})"<<std::endl;
      std::cout<<"PASS: Save/build and Save/build/run with Cancel, Unicode paths, independent 2/5 outputs and failure recovery; bounded Go to line and Cancel; typed CommandEcho On/Off, pending Apply, labels/comments, exact Undo/Redo/save/reopen, trace boundaries, invalid-edit rollback, clone configuration and one-time cleanup, completed/failed/stopped nested RunComplete state restoration.\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
