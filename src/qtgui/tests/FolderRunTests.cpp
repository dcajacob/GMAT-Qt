#include "MainWindow.hpp"
#include "FolderRunDialog.hpp"
#include "QtPlotReceiver.hpp"
#include "EditablePanel.hpp"
#include "TestSettings.hpp"
#include "FileManager.hpp"
#include "MessageInterface.hpp"
#include "GmatGlobal.hpp"
#include "Moderator.hpp"
#include "PlotWidget.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QTemporaryDir>
#include <QEventLoop>
#include <QTabWidget>
#include <QWindow>
#include <QMessageBox>
#include <QTreeWidget>
#include <QTableWidget>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *text) { if (!ok) throw std::runtime_error(text); }
static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Folder fixture write failed"); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),qPrintable("Folder output unavailable: "+path)); return file.readAll(); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtFolderRun");
   if (argc<2 || argc>3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; MainWindow window; window.show(); require(window.initialize(startup),"Folder runtime unavailable");
      QTemporaryDir files; require(files.isValid(),"Folder fixtures unavailable");
      for (const auto &name:QStringList{"scripts","baseline","output","original","copies"}) require(QDir(files.path()).mkdir(name),"Cannot create folder fixture");
      const auto scripts=files.filePath("scripts"),output=files.filePath("output"),baseline=files.filePath("baseline");
      write(QDir(scripts).filePath("value.inc"),"Create Variable V;\nV = 9;\n");
      const QByteArray good="% source bytes stay exact α\n#Include 'value.inc'\nCreate Spacecraft NormalSat;\nNormalSat.OrbitColor = Red;\nCreate ForceModel NormalFM;\nNormalFM.PrimaryBodies = {};\nNormalFM.PointMasses = {Earth};\nCreate Propagator NormalProp;\nNormalProp.FM = NormalFM;\nNormalProp.InitialStepSize = 1;\nNormalProp.MaxStep = 1;\nCreate OrbitView NormalOrbit;\nNormalOrbit.Add = {NormalSat, Earth};\nCreate ReportFile R;\nR.Filename = 'GMATgood.txt';\nR.WriteHeaders = false;\nBeginMissionSequence;\nPropagate NormalProp(NormalSat) {NormalSat.ElapsedSecs = 10};\nV = V + 1;\nReport R V;\n";
      const auto a=QDir(scripts).filePath("A_good.script"),b=QDir(scripts).filePath("B_bad.script"),c=QDir(scripts).filePath("C_good.m");
      write(a,good); write(b,"Create Variable V;\nBeginMissionSequence;\nNoSuchCommand V;\n"); auto second=good; second.replace("GMATgood.txt","GMATsecond.txt"); write(c,second);
      write(QDir(scripts).filePath("function.m"),"% skip function\nfunction out = f(in)\nout = in;\n"); write(QDir(scripts).filePath("backup.script~"),good);
      require(folderRunScripts(scripts)==QStringList{a,b,c},"Folder scan ordering, functions or backup exclusion failed");
      auto *fm=FileManager::Instance(); fm->SetAbsPathname("OUTPUT_PATH",(files.filePath("original")+"/").toStdString());
      require(window.loadScript(a) && window.runMission()==MainWindow::RunResult::Completed,"Independent folder reference failed");
      const auto expected=read(files.filePath("original/GMATgood.txt")); require(expected.trimmed().toDouble()==10,"Independent numeric folder reference incorrect");
      const auto originalPlot=window.plotReceiver()->model("NormalOrbit"); require(originalPlot && !originalPlot->curves.isEmpty(),"Original folder viewer reference missing");
      const auto originalPointCount=originalPlot->curves.first().points.size();
      write(QDir(baseline).filePath("REFgood.txt"),expected); write(QDir(baseline).filePath("REFsecond.txt"),expected);
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto accepted=editor->toPlainText(); editor->appendPlainText("% preserve pending folder source"); const auto pending=editor->toPlainText();
      const auto originalOutput=fm->GetFullPathname("OUTPUT_PATH"),originalEphem=fm->GetFullPathname("VEHICLE_EPHEM_PATH"),originalWorking=fm->GetGmatWorkingDirectory(),originalLog=MessageInterface::GetLogFileName(); const bool originalBatch=GmatGlobal::Instance()->IsBatchMode();
      QMdiArea batchArea; QtPlotReceiver views(&batchArea); std::atomic_bool cancel=false;
      FolderRunOptions options; options.directory=scripts; options.outputDirectory=output; options.copyDirectory=files.filePath("copies"); options.count=3; options.repeats=2; options.saveCopies=true; options.runFolders=true; options.compare=true; options.baselineDirectory=baseline; options.replacement="REF"; options.saveComparison=true; options.comparisonFile=files.filePath("comparisons.txt");
      editor->undo(); auto *resources=window.findChild<QTreeWidget *>("Resources"); const auto variable=resources->findItems("V",Qt::MatchExactly|Qt::MatchRecursive); require(variable.size()==1,"Pending folder resource missing"); resources->itemDoubleClicked(variable[0],0);
      auto *area=window.findChild<QMdiArea *>("workspace"); auto *panel=area->currentSubWindow(); auto *value=panel->widget()->findChild<QLineEdit *>("parameterValue"); require(value,"Pending folder resource editor missing"); value->setText("98");
      const auto guarded=window.runFolderScripts(options,views,cancel); require(guarded.error.contains("pending panel") && value->text()=="98" && dynamic_cast<EditablePanel *>(panel->widget())->hasChanges(),"Folder run discarded or accepted pending panel values");
      dynamic_cast<EditablePanel *>(panel->widget())->discardChanges(); panel->close(); editor->redo(); require(editor->toPlainText()==pending,"Folder pending guard damaged source Undo/Redo");
      window.findChild<QPlainTextEdit *>("messageWindow")->setPlainText(QString("historical message\n").repeated(10000));
      const auto result=window.runFolderScripts(options,views,cancel);
      if (!result.error.isEmpty()) std::cerr<<result.summary().toStdString();
      require(result.error.isEmpty() && result.items.size()==6 && !result.cancelled,"Folder run did not finish all selected repeats");
      for (const auto &item:result.items) {
         if (item.script==b) require(item.category=="Build error" && item.details.contains("NoSuchCommand"),"Build classification or diagnostics were lost after console truncation");
         else { if (item.category!="Completed" || !item.comparison.contains("differences=0 errors=0")) std::cerr<<item.script.toStdString()<<" repeat "<<item.repeat<<" "<<item.category.toStdString()<<"\n"<<item.details.toStdString()<<"\n"<<item.comparison.toStdString(); require(item.category=="Completed" && item.comparison.contains("differences=0 errors=0"),"Good folder mission or numeric comparison failed"); }
      }
      require(read(files.filePath("copies/A_good.script"))==good && read(a)==good,"Saved copy changed source bytes");
      for (const auto &run:QStringList{"Run_1","Run_2"}) for (const auto &report:QStringList{"GMATgood.txt","GMATsecond.txt"}) require(read(QDir(output).filePath(run+"/"+report))==expected,"Repeat output folder changed calculations or missed a report");
      require(read(result.summaryFile).contains("Build error: 2") && read(options.comparisonFile).contains("differences=0 errors=0"),"Folder summary or comparison export missing");
      auto restored=[&] {
         require(fm->GetFullPathname("OUTPUT_PATH")==originalOutput && fm->GetFullPathname("VEHICLE_EPHEM_PATH")==originalEphem && fm->GetGmatWorkingDirectory()==originalWorking && MessageInterface::GetLogFileName()==originalLog && GmatGlobal::Instance()->IsBatchMode()==originalBatch,"Folder run did not restore output/ephemeris/script/log/batch state");
         require(editor->toPlainText()==pending && editor->document()->isUndoAvailable() && !window.isRunning() && Moderator::Instance()->GetConfiguredObject("R"),"Folder run altered the pending document or failed to restore its engine model");
         require(window.plotReceiver()->model("NormalOrbit")==originalPlot && originalPlot->curves.first().points.size()==originalPointCount && window.plotReceiver()->show("NormalOrbit"),"Folder run damaged the ordinary viewer's retained history");
      }; restored(); editor->undo(); require(editor->toPlainText()==accepted,"Folder run damaged original Undo"); editor->redo();
      options.saveCopies=false; options.compare=false; options.saveComparison=false; options.runFolders=false; options.first=2; options.count=2; options.repeats=1; options.filter="good";
      const auto filtered=window.runFolderScripts(options,views,cancel); require(filtered.items.size()==1 && filtered.items[0].script==c,"Range-before-filter behavior differs from the selected mission list"); restored();
      options.first=1; options.count=3; options.filter.clear(); options.secondFilter="bad"; options.secondExclude=true;
      const auto excluded=window.runFolderScripts(options,views,cancel); require(excluded.items.size()==2,"Second exclusion filter failed without a first filter"); restored();
      options.secondFilter.clear();
      const auto stopped=window.runFolderScripts(options,views,cancel,[&](int,int,const FolderRunItem &) { cancel=true; }); require(stopped.cancelled && stopped.items.size()==1,"Between-script Stop executed further missions"); restored(); cancel=false;
      // Initialization and runtime errors must remain separate and must not
      // prevent the following mission from executing.
      // The names are distinct at build time. Sandbox initialization resolves
      // formation members and discovers that Sat would be propagated twice.
      write(b,"Create Spacecraft Sat;\nCreate Formation F;\nF.Add = {Sat};\n"
              "Create Propagator P;\nBeginMissionSequence;\nPropagate P(F, Sat);\n");
      const auto init=window.runFolderScripts(options,views,cancel);
      if (init.items[1].engineStatus!=-2) std::cerr<<init.summary().toStdString();
      require(init.items[1].engineStatus==-2 && init.items[1].category=="Initialization error" &&
            init.items[1].details.contains("Duplicate Spacecraft [Sat]") && init.items[2].category=="Completed",
            "Initialization failure classification/recovery failed");
      restored();
      write(b,"Create Spacecraft Sat;\nSat.DisplayStateType = Cartesian;\nSat.X = 0;\nSat.Y = 0;\nSat.Z = 0;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator P;\nP.FM = FM;\nBeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 1};\n");
      const auto runtime=window.runFolderScripts(options,views,cancel); require(runtime.items[1].category=="Runtime error" && runtime.items[2].category=="Completed","Runtime failure classification/recovery failed"); restored();
      write(b,"Create Variable V;\nV = 0;\nBeginMissionSequence;\nWhile V >= 0;\n   V = V + 1;\nEndWhile;\n"); options.first=2; options.count=1;
      QTimer::singleShot(50,&window,[&] { cancel=true; window.stopMission(); });
      const auto interrupted=window.runFolderScripts(options,views,cancel); require(interrupted.cancelled && interrupted.items.size()==1 && interrupted.items[0].category=="Interrupted","Active mission Stop did not interrupt folder run"); restored(); cancel=false; write(b,"bad script");
      options.repeats=2; write(b,QByteArray("invalid UTF-8 \xff")); const auto invalidText=window.runFolderScripts(options,views,cancel);
      require(invalidText.items.size()==2 && invalidText.items[0].category=="Read error" && invalidText.items[1].category=="Read error","Repeated malformed scripts lost their read-error classification"); restored(); options.repeats=1; write(b,"bad script");
      options.first=1; options.count=1; options.compare=true; options.saveComparison=true; options.comparisonFile=a;
      const auto overwrite=window.runFolderScripts(options,views,cancel); require(overwrite.error.contains("cannot overwrite") && read(a)==good,"Comparison results overwrote a source mission"); restored(); options.compare=false; options.saveComparison=false;
      options.compare=true; options.saveComparison=true; options.comparisonFile=QDir(baseline).filePath("REFgood.txt");
      const auto baselineOverwrite=window.runFolderScripts(options,views,cancel); require(baselineOverwrite.error.contains("cannot overwrite") && read(options.comparisonFile)==expected,"Comparison export overwrote its baseline"); restored(); options.compare=false; options.saveComparison=false;
      // The baseline and candidate can be the same file even with different
      // output folders (absolute report paths) or a mission-time redirect.
      FolderRunOptions guardedOptions=options;
      guardedOptions.compare=true;
      guardedOptions.outputDirectory=baseline;
      guardedOptions.replacement="GMAT";
      const auto reference=QDir(baseline).filePath("GMATgood.txt");
      const QByteArray sentinel="1234\n";
      write(reference,sentinel);
      const auto sameDirectory=window.runFolderScripts(guardedOptions,views,cancel);
      require(sameDirectory.items.size()==1 && sameDirectory.items[0].category=="Output error" &&
            sameDirectory.items[0].comparison.isEmpty() && read(reference)==sentinel,
            "Folder run overwrote and compared its own baseline");

      guardedOptions.outputDirectory=output;
      auto redirected=good;
      redirected.replace("Report R V;", "R.Filename = '"+reference.toUtf8()+"';\nReport R V;");
      write(a,redirected);
      const auto duringMission=window.runFolderScripts(guardedOptions,views,cancel);
      require(duringMission.items.size()==1 && duringMission.items[0].category!="Completed" &&
            duringMission.items[0].details.contains("comparison baseline") && read(reference)==sentinel,
            "Mission-time ReportFile redirect overwrote a comparison baseline");
      guardedOptions.replacement="REF";
      const auto ephemerisReference=QDir(baseline).filePath("REFgood.oem");
      write(ephemerisReference,sentinel);
      auto withEphemeris=good;
      withEphemeris.replace("GMATgood.txt","GMATgood.oem");
      withEphemeris.replace("BeginMissionSequence;", "Create Spacecraft Sat;\nCreate EphemerisFile E;\nE.Spacecraft = Sat;\nE.FileFormat = CCSDS-OEM;\nE.Filename = '"+ephemerisReference.toUtf8()+"';\nBeginMissionSequence;");
      write(a,withEphemeris);
      const auto ephemerisCollision=window.runFolderScripts(guardedOptions,views,cancel);
      require(ephemerisCollision.items.size()==1 && ephemerisCollision.items[0].category!="Completed" &&
            ephemerisCollision.items[0].details.contains("comparison baseline") && read(ephemerisReference)==sentinel,
            "Ephemeris output overwrote a reserved ReportFile baseline");
#ifndef Q_OS_WIN
      FolderRunOptions copyOptions=guardedOptions;
      copyOptions.compare=false; copyOptions.saveCopies=true; copyOptions.copyDirectory=files.filePath("linked copies");
      require(QDir().mkpath(copyOptions.copyDirectory),"Cannot create copy directory");
      const auto originalB=read(b);
      require(QFile::link(b,QDir(copyOptions.copyDirectory).filePath(QFileInfo(a).fileName())),"Cannot create copy alias");
      const auto copyCollision=window.runFolderScripts(copyOptions,views,cancel);
      require(copyCollision.items.size()==1 && copyCollision.items[0].category=="Copy error" && read(b)==originalB,
            "Saving a script copy overwrote a different source mission");
#endif
      auto allowed=good;
      allowed.replace("Report R V;", "R.Filename = 'GMATnew.txt';\nReport R V;");
      write(a,allowed);
      write(QDir(baseline).filePath("REFnew.txt"),expected);
      const auto redirectedReport=window.runFolderScripts(guardedOptions,views,cancel);
      require(redirectedReport.items.size()==1 && redirectedReport.items[0].category=="Completed" &&
            redirectedReport.items[0].reports.value("R")==QDir(output).filePath("GMATnew.txt") &&
            redirectedReport.items[0].comparison.contains("differences=0 errors=0"),
            "Safe report redirect compared the original destination");
      write(a,good); restored();
      auto collisionSource=good; collisionSource.replace("GMATgood.txt","FolderRunSummary.txt"); write(a,collisionSource);
      const auto collision=window.runFolderScripts(options,views,cancel); require(collision.error.isEmpty() && collision.summaryFile!=QDir(output).filePath("FolderRunSummary.txt") && read(QDir(output).filePath("FolderRunSummary.txt"))==expected,"Folder summary overwrote an active report"); restored(); write(a,good);
      options.outputDirectory=files.filePath("not-directory"); write(options.outputDirectory,"retain"); const auto badOutput=window.runFolderScripts(options,views,cancel); require(!badOutput.error.isEmpty() && read(options.outputDirectory)=="retain","Invalid output path was overwritten or accepted"); restored(); options.outputDirectory=output;
      // Open the delivered menu/dialog and run through its actual controls.
      const auto plotDirectory=files.filePath("plot scripts"); require(QDir().mkpath(plotDirectory),"Plot folder missing");
      const QByteArray plotSource="Create Spacecraft Sat;\nSat.OrbitColor = Red;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 1;\nP.MaxStep = 1;\nCreate OrbitView Orb;\nOrb.Add = {Sat, Earth};\nCreate GroundTrack Ground;\nGround.Add = {Sat};\nBeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 60};\n";
      write(QDir(plotDirectory).filePath("Plot.script"),plotSource);
      auto of=plotSource; of.replace("Create OrbitView Orb;","Create OpenFramesInterface Orb;"); write(QDir(plotDirectory).filePath("OF.script"),of);
      auto solver=plotSource; solver.replace("BeginMissionSequence;","Create DifferentialCorrector DC;\nBeginMissionSequence;\nTarget DC {ShowProgressWindow = true, ExitMode = SaveAndContinue};\nVary DC(Sat.X = 7000, {Perturbation = 1, MaxStep = 100});\nAchieve DC(Sat.X = 7100, {Tolerance = 0.000001});\nEndTarget;"); write(QDir(plotDirectory).filePath("Solver.script"),solver);
      std::exception_ptr failure;
      QTimer::singleShot(0,&window,[&] {
         try {
            auto *dialog=dynamic_cast<FolderRunDialog *>(window.findChild<QDialog *>("folderRunDialog")); require(dialog,"Folder menu did not open dialog");
            dialog->findChild<QLineEdit *>("folderDirectory")->setText(scripts); dialog->findChild<QLineEdit *>("folderOutput")->setText(output);
            dialog->findChild<QLineEdit *>("folderFilter")->setText("good"); dialog->findChild<QPushButton *>("folderRun")->click();
            require(dialog->findChild<QPlainTextEdit *>("folderResults")->toPlainText().contains("Completed: 2") && dialog->findChild<QPushButton *>("folderRun")->isEnabled(),"Folder dialog did not display finished summary or recover Run");
            if (!capture.isEmpty()) { QEventLoop loop; QTimer::singleShot(400,&loop,&QEventLoop::quit); loop.exec(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(capture+".results.png"),"Native folder results capture failed"); }
            dialog->findChild<QLineEdit *>("folderDirectory")->setText(plotDirectory); dialog->findChild<QLineEdit *>("folderFilter")->setText("Plot.script"); dialog->findChild<QPushButton *>("folderRun")->click();
            require(dialog->findChild<QPlainTextEdit *>("folderResults")->toPlainText().contains("Completed: 1"),"Folder plot mission failed");
            auto *batch=dialog->findChild<QMdiArea *>("folderPlots"); dialog->findChild<QTabWidget *>("folderRunTabs")->setCurrentIndex(2); batch->tileSubWindows(); QApplication::processEvents();
            int scenes=0;
            for (auto *child:batch->subWindowList()) if (auto *plot=dynamic_cast<PlotWidget *>(child->widget())) {
               const auto frame=plot->canvas()->captureImage(); require(!frame.isNull(),"Folder plot frame missing");
               int ocean=0,trajectory=0;
               for (int y=0;y<frame.height();++y) for (int x=0;x<frame.width();++x) { const auto color=frame.pixelColor(x,y); if (color.blue()>color.red()*2 && color.blue()>50 && color.green()>25) ++ocean; if (color.red()>150 && color.green()<100 && color.blue()<100) ++trajectory; }
               require(ocean>200 && trajectory>5,"Folder viewer lost body/map or its trajectory after normal engine restoration"); ++scenes;
               if (!capture.isEmpty()) require(frame.save(capture+"."+child->property("plotName").toString()+".png"),"Folder scene capture failed");
            }
            require(scenes==2,"Folder OrbitView and GroundTrack were not both retained");
            if (!capture.isEmpty()) require(dialog->grab().save(capture+".plots.png"),"Folder viewers capture failed");
            dialog->findChild<QLineEdit *>("folderFilter")->setText("OF.script");
            
            QTimer conversionTimer; conversionTimer.setInterval(10); QObject::connect(&conversionTimer,&QTimer::timeout,dialog,[&] { if (auto *prompt=dialog->findChild<QMessageBox *>("openFramesFolderConversionPrompt")) { conversionTimer.stop(); prompt->button(QMessageBox::Yes)->click(); } }); conversionTimer.start();
            dialog->findChild<QPushButton *>("folderRun")->click(); require(dialog->findChild<QPlainTextEdit *>("folderResults")->toPlainText().contains("Completed: 1") && read(QDir(plotDirectory).filePath("OF.script"))==of,"Folder OF conversion failed or changed the source file");
            dialog->findChild<QLineEdit *>("folderFilter")->setText("Solver.script"); dialog->findChild<QSpinBox *>("folderRepeats")->setValue(2); dialog->findChild<QPushButton *>("folderRun")->click();
            require(dialog->findChild<QPlainTextEdit *>("folderResults")->toPlainText().contains("Completed: 2"),"Repeated folder solver execution failed");
            const auto tables=batch->findChildren<QTableWidget *>("solverProgress"); require(tables.size()==1 && tables[0]->rowCount()>0 && batch->findChild<QLabel *>("solverStatus")->property("converged").toBool(),"Folder solver progress was lost or is stale");
            require(area->findChildren<QTableWidget *>("solverProgress").isEmpty(),"Folder solver window leaked into the ordinary mission workspace");
            dialog->reject();
         } catch (...) { failure=std::current_exception(); for (auto *dialog:window.findChildren<QDialog *>()) dialog->reject(); }
      }); window.findChild<QAction *>("runScriptFolder")->trigger(); if (failure) std::rethrow_exception(failure); restored();
      std::cout<<"PASS: sorted folder missions/functions/backups, range/two filters, repeats and per-run outputs, exact saved copies with relative includes, independent numeric reports/comparison and exports, build/init/runtime failure recovery, between-run/active Stop, output failure and original document/Undo/engine/path/log/batch restoration; menu/dialog summary and retry controls, retained textured OrbitView/GroundTrack scenes after restoration and automatic OF conversion offer without changing source files\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
