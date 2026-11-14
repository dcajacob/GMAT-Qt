#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "CommandEditor.hpp"
#include "InspectionDialog.hpp"
#include "FindReplaceDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "FileManager.hpp"
#include "GmatCommand.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QClipboard>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QLabel>
#include <QTreeWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <QThread>
#include <iostream>
#include <cmath>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Cannot read fixture"); return QString::fromUtf8(f.readAll()); }
static void write(const QString &path,const QString &text) { QFile f(path); require(f.open(QIODevice::WriteOnly),"Cannot write fixture"); require(f.write(text.toUtf8())==text.toUtf8().size(),"Fixture write incomplete"); }
template<class Open,class Inspect> static void modal(QWidget &owner,Open open,Inspect inspect)
{
   std::exception_ptr failure;
   QTimer::singleShot(0,&owner,[&] {
      auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
      try { require(dialog,"Expected inspection dialog missing"); inspect(dialog); }
      catch (...) { failure=std::current_exception(); }
      if (dialog && dialog->isVisible()) dialog->reject();
   }); open(); if (failure) std::rethrow_exception(failure);
}
static int node(const MainWindow &window,const QString &type,const QString &text={})
{
   const auto &nodes=window.missionSnapshot().nodes;
   for (int i=0;i<nodes.size();++i) if (nodes[i].type==type && (text.isEmpty() || nodes[i].statement.contains(text))) return i;
   throw std::runtime_error("Mission node missing");
}
static bool rejected(const std::function<void()> &operation)
{
   try { operation(); } catch (BaseException &) { return true; } catch (const std::exception &) { return true; } return false;
}
static QList<double> summaryState(const QString &text)
{
   QList<double> result;
   for (const auto *key:{"X","Y","Z","VX","VY","VZ"}) {
      const auto match=QRegularExpression(QString("\\b%1\\s*=\\s*([-+0-9.eE]+)\\s+km").arg(key)).match(text);
      require(match.hasMatch(),qPrintable("Summary missing "+QString(key))); bool ok=false;
      const double number=match.captured(1).toDouble(&ok); require(ok && std::isfinite(number),"Summary contains invalid state"); result.append(number);
   }
   return result;
}
static void compare(const QList<double> &state,const QStringList &numbers,int offset)
{
   for (int i=0;i<6;++i) require(std::abs(state[i]-numbers[offset+i].toDouble())<=std::max(1e-7,std::abs(state[i])*1e-10),"Summary differs from independently reported command state");
}
static QTreeWidgetItem *item(QTreeWidget *tree,const QString &name)
{
   QTreeWidgetItemIterator iterator(tree);
   while (*iterator) { if ((*iterator)->text(0)==name) return *iterator; ++iterator; }
   throw std::runtime_error("Resource item missing");
}
static void screenshot(QWidget *widget,const QString &path)
{
   if (path.isEmpty()) return;
   widget->show(); QElapsedTimer elapsed; elapsed.start();
   while (elapsed.elapsed()<350) { QApplication::processEvents(); QThread::msleep(5); }
   require(widget->grab().save(path),"Screenshot unavailable");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtInspection");
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid() && argc>1,"Fixture setup failed");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const QString capture=argc>2 ? QString::fromLocal8Bit(argv[2]) : QString();
      MainWindow window; window.show(); require(window.initialize(startup),"Initialization failed");
      const auto outputPath=FileManager::Instance()->GetAbsPathname("OUTPUT_PATH");
      struct RestoreOutput { std::string path; ~RestoreOutput() { FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",path); } } restoreOutput{outputPath};
      FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",files.path().toStdString());
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Script editor missing");
      const auto report=files.filePath("states.txt"),saved=files.filePath("mission ü.script");
      QString source="Create Spacecraft Sat;\nCreate ForceModel Forces;\nForces.CentralBody = Earth;\nForces.PrimaryBodies = {};\nForces.PointMasses = {Earth};\nForces.Drag = None;\nForces.SRP = Off;\nCreate Propagator Prop;\nProp.FM = Forces;\n"
         "Create Barycenter Center;\nCenter.BodyNames = {Earth, Luna};\nCreate CoordinateSystem Bary;\nBary.Origin = Center;\nBary.Axes = MJ2000Eq;\n"
         "Create CoordinateSystem MoonEq;\nMoonEq.Origin = Luna;\nMoonEq.Axes = MJ2000Eq;\nCreate CoordinateSystem SatEq;\nSatEq.Origin = Sat;\nSatEq.Axes = MJ2000Eq;\n"
         "Create CoordinateSystem SatAxes;\nSatAxes.Origin = Earth;\nSatAxes.Axes = ObjectReferenced;\nSatAxes.Primary = Earth;\nSatAxes.Secondary = Sat;\nSatAxes.XAxis = R;\nSatAxes.YAxis = V;\n"
         "Create Variable x;\nx = 0;\nCreate ReportFile States;\nStates.WriteHeaders = false;\nStates.Precision = 16;\nStates.Filename = '"+report+"';\n"
         "BeginMissionSequence;\nIf x > 0;\n   Propagate 'Skipped coast' Prop(Sat) {Sat.ElapsedSecs = 30};\nElse;\n   x = 1;\nEndIf;\nPropagate 'First coast' Prop(Sat) {Sat.ElapsedSecs = 600};\nReport States ";
      QStringList parameters;
      for (const auto &frame:QStringList{"EarthMJ2000Eq","EarthFixed","MoonEq","Bary"}) for (const auto *axis:{"X","Y","Z","VX","VY","VZ"}) parameters.append("Sat."+frame+"."+axis);
      source+=parameters.join(' ')+";\nBeginScript 'Script stage';\n   x = 2; % keep pending source\nEndScript;\nPropagate 'Second coast' Prop(Sat) {Sat.ElapsedSecs = 1200};\nReport States "+parameters.join(' ')+";\n";
      editor->setPlainText(source); require(window.saveScriptTo(saved) && window.buildScript(),"Inspection fixture build failed");
      require(rejected([&] { window.summaryText(-1,"EarthMJ2000Eq"); }),"Unexecuted summary accepted");
      require(window.runMission()==MainWindow::RunResult::Completed,"Inspection fixture execution failed");
      const auto baseline=read(report); const auto numbers=baseline.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(numbers.size()==48,"Reference state report incomplete");
      const auto frames=window.summaryFrames(); require(frames.contains("EarthMJ2000Eq") && frames.contains("EarthFixed") && frames.contains("MoonEq") && frames.contains("Bary") && !frames.contains("SatEq") && !frames.contains("SatAxes"),"Summary frame filtering is incorrect");
      const int first=node(window,"Propagate","First coast"),second=node(window,"Propagate","Second coast"),event=node(window,"BeginScript"),skipped=node(window,"Propagate","Skipped coast");
      for (int f=0;f<4;++f) {
         const auto frame=QStringList{"EarthMJ2000Eq","EarthFixed","MoonEq","Bary"}[f];
         compare(summaryState(window.summaryText(first,frame)),numbers,f*6);
         compare(summaryState(window.summaryText(second,frame)),numbers,24+f*6);
         const auto script=window.summaryText(event,frame); compare(summaryState(script),numbers,f*6);
         require(script.contains("ScriptEvent Command: Script stage"),"Script event summarized the wrong command/name");
      }
      const auto skippedText=window.summaryText(skipped,"EarthMJ2000Eq"); require(!skippedText.contains("Cartesian State") && skippedText.contains("did not execute"),"Skipped branch fabricated results");
      require(rejected([&] { window.summaryText(first,"SatEq"); }) && rejected([&] { window.summaryText(first,"SatAxes"); }) && rejected([&] { window.summaryText(999,"EarthMJ2000Eq"); }),"Invalid summary selection accepted");
      const auto all=window.summaryText(-1,"EarthMJ2000Eq"),physics=window.summaryText(-1,"EarthMJ2000Eq",true);
      require(all.contains("First coast") && all.contains("Second coast") && physics.contains("First coast") && physics.contains("Second coast") && all!=physics,"Mission all/physics inspection missing");
      QVector<GmatCommand *> commands; snapshotMission(Moderator::Instance()->GetFirstCommand(),window.missionSnapshot().canonicalScript,source,&commands);
      auto *end=commands[event]->GetNext(); while (end && end->GetTypeName()!="EndScript") end=end->GetNext(); require(end,"Script event end missing"); const auto endName=end->GetSummaryName();
      window.summaryText(event,"EarthMJ2000Eq"); require(end->GetSummaryName()==endName,"Inspection changed EndScript summary name");
      modal(window,[&] { window.showSummary(); },[&](QDialog *base) {
         auto *dialog=dynamic_cast<SummaryDialog *>(base); require(dialog,"Mission summary dialog missing");
         auto *frame=dialog->findChild<QComboBox *>("summaryFrame"); auto *text=dialog->findChild<QPlainTextEdit *>("inspectionText");
         require(text && text->isReadOnly() && frame->currentText()=="EarthMJ2000Eq" && text->toPlainText()==all,"Initial mission summary incorrect");
         dialog->findChild<QCheckBox *>("summaryPhysics")->setChecked(true); require(text->toPlainText()==physics,"Physics checkbox did not refresh summary");
         frame->setCurrentText("MoonEq"); require(text->toPlainText().contains("Coordinate System: MoonEq"),"Coordinate selector did not refresh summary");
         const auto before=text->toPlainText(); QKeyEvent key(QEvent::KeyPress,Qt::Key_A,Qt::NoModifier,"oops"); QApplication::sendEvent(text,&key); require(text->toPlainText()==before,"Summary is editable");
         dialog->findChild<QPushButton *>("inspectionFind")->click(); auto *search=dynamic_cast<FindReplaceDialog *>(dialog->findChild<QDialog *>("findReplaceDialog")); require(search && !search->findChild<QPushButton *>("replaceAll"),"Read-only Find offers replacement");
         search->findChild<QComboBox *>("findText")->setEditText("MoonEq"); require(search->findNext(),"Summary Find failed"); text->copy(); require(QApplication::clipboard()->text()=="MoonEq","Summary Copy failed"); search->hide();
         const auto exported=files.filePath("summary Δ.txt"); require(dialog->exportText(exported).isEmpty() && read(exported)==before,"Unicode summary export failed");
         require(!dialog->exportText(saved).isEmpty() && read(saved)==source,"Summary overwrote mission source");
         const auto alias=files.filePath("source-alias.txt"); require(QFile::link(saved,alias) && !dialog->exportText(alias).isEmpty() && read(saved)==source,"Summary overwrote source through symlink");
         require(!dialog->exportText(files.filePath("missing/summary.txt")).isEmpty() && dialog->exportText(exported).isEmpty(),"Export failure recovery failed");
         modal(*dialog,[&] { dialog->findChild<QPushButton *>("inspectionSave")->click(); },[](QDialog *picker) { require(dynamic_cast<QFileDialog *>(picker),"Summary Save as picker missing"); });
         require(read(exported)==before && read(saved)==source,"Save as Cancel changed files");
         const auto uiExport=files.filePath("summary from chooser ü.txt");
         modal(*dialog,[&] { dialog->findChild<QPushButton *>("inspectionSave")->click(); },[&](QDialog *picker) {
            auto *file=dynamic_cast<QFileDialog *>(picker); require(file,"Summary Save as picker missing"); file->selectFile(uiExport); QMetaObject::invokeMethod(file,"accept",Qt::DirectConnection);
         }); require(read(uiExport)==before && dialog->findChild<QLabel *>("inspectionStatus")->text().contains("saved"),"Save as acceptance failed"); screenshot(dialog,capture);
         const auto previous=frame->currentText(); require(window.buildScript(),"Rebuild from inspection test failed"); frame->setCurrentText("EarthFixed"); require(frame->currentText()==previous && text->toPlainText()==before && dialog->findChild<QLabel *>("inspectionStatus")->text().contains("changed"),"Stale summary did not roll back selector safely");
      });
      require(rejected([&] { window.summaryText(first,"EarthMJ2000Eq"); }),"Rebuild retained old summary results");
      require(window.runMission()==MainWindow::RunResult::Completed && read(report)==baseline,"Inspection changed calculations");
      // Open both preview controls through their actual MDI command/resource panels.
      auto *tree=window.findChild<QTreeWidget *>("Resources"); tree->itemDoubleClicked(item(tree,"Sat"),0); QApplication::processEvents();
      auto *workspace=window.findChild<QMdiArea *>("workspace"); auto *resource=dynamic_cast<ResourceEditor *>(workspace->activeSubWindow()->widget()); require(resource,"Actual resource panel missing");
      auto *table=resource->findChild<QTableWidget *>("resourceProperties"); QTableWidgetItem *value=nullptr; for (int row=0;table && row<table->rowCount();++row) if (table->item(row,0)->text()=="DryMass") { value=table->item(row,1); break; } require(value,"Spacecraft dry mass control missing"); value->setText("999");
      modal(*resource,[&] { resource->findChild<QPushButton *>("showScript")->click(); },[&](QDialog *dialog) { auto *text=dialog->findChild<QPlainTextEdit *>("inspectionText"); require(text && text->isReadOnly() && text->toPlainText().contains("Create Spacecraft Sat") && !text->toPlainText().contains("DryMass = 999"),"Resource preview included pending edits"); });
      require(resource->hasChanges() && value->text()=="999" && editor->toPlainText()==source,"Resource preview disturbed pending edits/source"); resource->discardChanges(); workspace->activeSubWindow()->close(); QApplication::processEvents();
      auto *mission=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItem *commandItem=nullptr; QTreeWidgetItemIterator iterator(mission); while (*iterator) { if ((*iterator)->data(0,Qt::UserRole).isValid() && (*iterator)->data(0,Qt::UserRole).toInt()==first) { commandItem=*iterator; break; } ++iterator; } require(commandItem,"First command tree item missing");
      mission->itemDoubleClicked(commandItem,0); QApplication::processEvents(); auto *command=dynamic_cast<CommandEditor *>(workspace->activeSubWindow()->widget()); require(command,"Actual command panel missing");
      auto *commandText=command->findChild<QPlainTextEdit *>("commandSource"); auto applied=commandText->toPlainText(); commandText->setPlainText(applied.replace("600","750"));
      modal(*command,[&] { command->findChild<QPushButton *>("showScript")->click(); },[](QDialog *dialog) { const auto preview=dialog->findChild<QPlainTextEdit *>("inspectionText")->toPlainText(); require(preview.contains("600") && !preview.contains("750"),"Command preview included pending edits"); });
      modal(*command,[&] { command->findChild<QPushButton *>("showCommandSummary")->click(); },[&](QDialog *dialog) { compare(summaryState(dialog->findChild<QPlainTextEdit *>("inspectionText")->toPlainText()),numbers,0); });
      require(command->hasChanges() && commandText->toPlainText().contains("750") && editor->toPlainText()==source,"Command inspection disturbed pending edits"); command->discardChanges(); workspace->activeSubWindow()->close(); QApplication::processEvents();
      editor->insertPlainText("% pending source\n"); require(rejected([&] { window.summaryText(-1,"EarthMJ2000Eq"); }),"Edited mission exposed stale results"); editor->undo(); require(editor->toPlainText()==source,"Inspection created extra undo entries");
      require(window.loadScript(saved) && rejected([&] { window.summaryText(-1,"EarthMJ2000Eq"); }) && window.runMission()==MainWindow::RunResult::Completed && read(report)==baseline,"Save/reopen results changed or retained stale summaries");
      QString failureSource=source; failureSource.replace("Propagate 'Second coast' Prop(Sat) {Sat.ElapsedSecs = 1200};","Sat.DryMass = -1;"); editor->setPlainText(failureSource); require(window.runMission()==MainWindow::RunResult::Failed,"Runtime failure fixture did not fail"); compare(summaryState(window.summaryText(node(window,"Propagate","First coast"),"EarthMJ2000Eq")),numbers,0);
      editor->setPlainText(source); require(window.runMission()==MainWindow::RunResult::Completed && read(report)==baseline,"Failed-run summary recovery failed");
      QString stopped=source; stopped.replace("Propagate 'Second coast' Prop(Sat) {Sat.ElapsedSecs = 1200};","CommandEcho On;\nWhile x > 0;\n x = x + 1;\nEndWhile;"); editor->setPlainText(stopped);
      auto *messages=window.findChild<QPlainTextEdit *>("messageWindow"); messages->clear(); QTimer stopTimer;
      QObject::connect(&stopTimer,&QTimer::timeout,&window,[&] {
         if (messages->toPlainText().contains(QRegularExpression("CurrentCommand:[^\\n]*While"))) { stopTimer.stop(); window.stopMission(); }
      }); stopTimer.start(10); require(window.runMission()==MainWindow::RunResult::Stopped,"Stopped summary fixture did not stop"); compare(summaryState(window.summaryText(node(window,"Propagate","First coast"),"EarthMJ2000Eq")),numbers,0);
      editor->setPlainText(source); require(window.runMission()==MainWindow::RunResult::Completed && read(report)==baseline,"Stopped-run summary recovery failed");
      // Editable command text retains transactional validation. ClearPlot now
      // also has a typed form, which can reject a bad reference before Apply's
      // backend callback; its visible diagnostic is part of the same contract.
      QString generic=source; generic.replace("BeginMissionSequence;","Create XYPlot Trace;\nTrace.XVariable = Sat.ElapsedSecs;\nTrace.YVariables = {Sat.X};\nTrace.ShowPlot = false;\nBeginMissionSequence;\nClearPlot Trace;");
      editor->setPlainText(generic); require(window.runMission()==MainWindow::RunResult::Completed && read(report)==baseline,"Generic command reference run changed calculations");
      auto snapshot=window.missionSnapshot(); const auto clear=node(window,"ClearPlot"); QString applyError;
      {
         QWidget owner; CommandEditor panel(snapshot.nodes[clear].statement,false,{},[&](const QString &replacement) {
            applyError=window.applyMissionChange(snapshot,clear,MissionEdit::Replace,replacement); return applyError;
         },{},{},&owner);
         auto *text=panel.findChild<QPlainTextEdit *>("commandSource"); text->setPlainText("ClearPlot Missing;");
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         auto *status=panel.findChild<QLabel *>("commandStatus");
         require(status&&status->text().contains("Missing"),"Invalid plot command omitted its attributable visible diagnostic");
         if (applyError.isEmpty()) applyError=status->text();
         require(!applyError.isEmpty() && editor->toPlainText()==generic && panel.hasChanges(),"Generic invalid command changed mission or discarded pending text");
         text->setPlainText("MarkPoint Trace;"); panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         require(applyError.isEmpty() && editor->toPlainText().contains("MarkPoint Trace;"),"Generic command correction did not apply");
      }
      const auto genericSaved=files.filePath("generic commands ü.script"); require(window.saveScriptTo(genericSaved) && window.loadScript(genericSaved) && window.runMission()==MainWindow::RunResult::Completed && read(report)==baseline,"Generic command save/reopen recovery changed calculations");
      std::cout<<"Qt inspections: previews, pending edits, command/mission states, four frames, physics selection, skipped branches, script events, search/copy/export, stale/failure/stop recovery passed\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
