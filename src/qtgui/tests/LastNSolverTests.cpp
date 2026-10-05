#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "ResourceEditor.hpp"
#include "ResourceForm.hpp"
#include "PlotWidget.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "OrbitTrialHistoryChecks.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QScrollArea>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <sstream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read report"); return file.readAll(); }
static void preserve(const QString &name,const QByteArray &data) {
   const auto root=qEnvironmentVariable("GMAT_QT_LASTN_EVIDENCE"); if (root.isEmpty()) return;
   QDir().mkpath(root); QFile file(root+"/"+name); require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"Cannot preserve LastN evidence");
}
static const PlotCurve &curve(const PlotModel &model,const QString &name="Sat") {
   for (const auto &c:model.curves) if (c.name==name) return c;
   throw std::runtime_error("Expected plotted object missing");
}
static size_t trials(const PlotModel &model,const QString &name="Sat") {
   size_t count=0; for (const auto &p:curve(model,name).points) count+=p.solver; return count;
}
static QVector<std::array<double,4>> accepted(const PlotModel &model) {
   QVector<std::array<double,4>> result; for (const auto &p:curve(model).points) if (!p.solver) result.append({p.epoch,p.x,p.y,p.z}); return result;
}
static QString convert(const QString &settings) {
   const auto result=convertOpenFramesViews("Create OpenFramesInterface Orb;\nOrb.Add = {Sat, Earth};\n"+settings);
   require(result.error.isEmpty(),qPrintable(result.error)); return result.script+"\n";
}
static void conversion() {
   const auto count=[](const QString &s) { return qtCameraSettings(convert(s)).value("Orb").solverLastN; };
   require(!count(""),"Default All unexpectedly selects LastN");
   require(count("Orb.SolverIterations = LastN;\n")==1,"Constructor backing count not retained");
   require(count("Orb.SolverIterLastN = 5;\nOrb.SolverIterations = LastN;\n")==1,"Pre-LastN count was not ignored");
   require(count("Orb.SolverIterations = All;\nOrb.SolverIterations = LastN;\n")==0,"Explicit All backing reset lost");
   require(count("Orb.SolverIterations = Current;\nOrb.SolverIterations = LastN;\n")==1,"Current backing reset lost");
   require(count("Orb.SolverIterations = LastN;\nOrb.SolverIterLastN = 2;\nOrb.Add = {Earth, Sat};\n")==2,"LastN/Add source order lost");
   require(count("Orb.SolverIterations = LastN;\nOrb.SolverIterLastN = 0;\n")==0,"Unlimited count lost");
   require(!count("Orb.SolverIterations = LastN;\nOrb.SolverIterations = None;\n"),"Later None retained LastN");
   for (const auto &bad:QStringList{"-1","1.5","2147483648","Alpha"}) {
      const auto source="Create OpenFramesInterface Orb;\nOrb.SolverIterLastN = "+bad+";\n"; const auto result=convertOpenFramesViews(source);
      require(!result.error.isEmpty() && result.script==source,"Invalid legacy count did not preserve source");
   }
   const auto script=convert("Orb.SolverIterations = LastN;\nOrb.SolverIterLastN = 2;\n");
   require(script.contains("Orb.SolverIterations = All;") && !script.contains("\nOrb.SolverIterations = LastN;"),"Conversion emits unsupported engine mode");
   for (const auto &bad:QStringList{"-1","0.5","true","\"2\"","2147483648"}) {
      auto invalid=script; invalid.replace("\"solverLastN\":2","\"solverLastN\":"+bad); bool rejected=false;
      try { qtCameraSettings(invalid); } catch (const std::exception &) { rejected=true; } require(rejected,"Invalid LastN metadata accepted");
   }
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("LastNSolver");
   if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   try {
      TestSettings settings; conversion(); runOrbitTrialHistoryChecks();
      QTemporaryDir files; require(files.isValid(),"Temporary fixture directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto report=files.filePath("values.txt");
      const QString resources="% independently authored short LastN fixture α\nCreate Spacecraft Sat;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 0;\nCreate Variable Alpha;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 5;\nP.MinStep = 5;\nP.MaxStep = 5;\nCreate DifferentialCorrector DC;\nDC.MaximumIterations = 25;\nDC.ReportFile = '"+files.filePath("dc.txt")+"';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.WriteReport = false;\nValues.Precision = 17;\nValues.SolverIterations = All;\n";
      const QString mission="BeginMissionSequence;\nPropagate 'Before' P(Sat) {Sat.ElapsedSecs = 10};\nTarget DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary DC(Alpha = 1, {Perturbation = 0.001, Lower = 0, Upper = 5, MaxStep = 0.5});\nSat.X = 7000 + 100 * Alpha;\nPropagate 'First trial piece' P(Sat) {Sat.ElapsedSecs = 10};\nPropagate 'Second trial piece' P(Sat) {Sat.ElapsedSecs = 10};\nAchieve DC(Alpha = 4, {Tolerance = 0.000001});\nReport Values Alpha Sat.A1ModJulian Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z;\nEndTarget;\nPropagate 'After' P(Sat) {Sat.ElapsedSecs = 10};\nReport Values Alpha Sat.A1ModJulian Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z;\n";
      const auto viewSettings=QString("Orb.DrawUsePropLabel = [true false];\nOrb.Axes = Off;\nOrb.UpdatePlotFrequency = 1;\n");
      const auto baseline=resources+convert("Orb.SolverIterations = All;\n"+viewSettings)+mission;
      editor->setPlainText(baseline); require(window.runMission()==MainWindow::RunResult::Completed,"New All reference failed");
      const auto expected=read(report); require(!expected.isEmpty(),"New reference is empty"); preserve("all.report",expected); preserve("all.script",baseline.toUtf8());
      std::istringstream rows(expected.toStdString()); std::array<double,5> row{}; int n=0;
      while (rows>>row[0]>>row[1]>>row[2]>>row[3]>>row[4]) ++n;
      require(n>5 && std::abs(row[0]-4)<1e-6,"Reference did not perform multiple trials and reach Alpha=4");
      const auto allModel=window.plotReceiver()->model("Orb"); require(bool(allModel),"All viewer missing");
      const auto allTrials=trials(*allModel); const auto allAccepted=accepted(*allModel);
      const auto source=resources+convert("Orb.SolverIterations = LastN;\nOrb.SolverIterLastN = 2;\n"+viewSettings)+mission;
      editor->setPlainText(source); require(window.runMission()==MainWindow::RunResult::Completed,"LastN mission failed");
      require(read(report)==expected && editor->toPlainText()==source,"LastN changed complete report or authored source"); preserve("lastn.report",read(report)); preserve("lastn.script",source.toUtf8());
      const auto model=window.plotReceiver()->model("Orb"); require(bool(model),"LastN viewer missing");
      std::cout<<"All trial points="<<allTrials<<"; LastN2="<<trials(*model)<<"; accepted="<<allAccepted.size()<<"; report rows="<<n<<'\n';
      require(trials(*model)>0 && trials(*model)<allTrials,"LastN2 did not retain a bounded trial subset");
      require(accepted(*model)==allAccepted && !model->regularArcs.value("Sat").isEmpty(),"LastN altered accepted points or regular labels");
      require(trials(*model,"Earth")==0,"Accepted-pass body cleanup retained trials");
      QSet<quint64> frames; for (const auto &c:model->curves) for (const auto &p:c.points) frames.insert(p.frame);
      for (const auto &camera:model->cameras) require(!camera.solver || frames.contains(camera.frame),"Discarded trial camera remains");
      auto *area=window.findChild<QMdiArea *>("workspace"); PlotWidget *view=nullptr;
      for (auto *child:area->subWindowList()) if (child->property("plotName")=="Orb") view=dynamic_cast<PlotWidget *>(child->widget());
      require(view && !view->canvas()->captureImage().isNull(),"LastN viewer cannot render"); const auto retained=curve(*model).points.size();
      auto *timeline=view->findChild<QSlider *>("plotTimeline"); require(timeline,"Replay control missing"); timeline->setValue(0); app.processEvents(); timeline->setValue(timeline->maximum()); app.processEvents();
      require(curve(*model).points.size()==retained,"Replay changed LastN history");
      for (auto *child:area->subWindowList()) if (child->property("plotName")=="Orb") child->close();
      require(window.plotReceiver()->show("Orb") && curve(*window.plotReceiver()->model("Orb")).points.size()==retained,"Output reopen lost LastN history");
      auto *tree=window.findChild<QTreeWidget *>("Resources");
      auto open=[&] { const auto items=tree->findItems("Orb",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Orb resource missing"); tree->itemDoubleClicked(items.front(),0); auto *panel=dynamic_cast<ResourceEditor *>(area->activeSubWindow()->widget()); require(panel,"Orbit resource form missing"); return panel; };
      auto *panel=open(); auto *mode=panel->findChild<QComboBox *>("orbit_SolverIterations"); auto *count=panel->findChild<QSpinBox *>("orbit_SolverIterLastN");
      require(mode && count && mode->currentText()=="LastN" && count->value()==2 && count->isEnabled(),"LastN did not reopen in primary controls");
      std::exception_ptr cancelled;
      QTimer::singleShot(0,panel,[&] {
         QDialog *setup=nullptr; for (auto *dialog:panel->findChildren<QDialog *>("orbitViewDialog")) if (dialog->isWindow() && dialog->isVisible()) setup=dialog;
         try {
            require(setup,"Pending setup dialog missing"); auto *pending=setup->findChild<QSpinBox *>("orbit_SolverIterLastN"); require(pending && pending->value()==2,"Setup did not read pending LastN"); pending->setValue(9);
            setup->reject();
         } catch (...) { cancelled=std::current_exception(); if (setup) setup->reject(); }
      }); panel->findChild<QPushButton *>("editOrbitView")->click(); if (cancelled) std::rethrow_exception(cancelled);
      require(count->value()==2 && !panel->hasChanges() && editor->toPlainText()==source,"Setup Cancel changed mode/count/source");
      for (auto *scroll:panel->findChildren<QScrollArea *>("orbitViewScroll")) scroll->ensureWidgetVisible(count);
      const auto folder=qEnvironmentVariable("GMAT_QT_LASTN_EVIDENCE"); if (!folder.isEmpty()) require(panel->grab().save(folder+"/controls.png"),"Control capture failed");
      count->setValue(3); require(panel->hasChanges() && editor->toPlainText()==source,"Count changed source before Apply");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto edited=editor->toPlainText(); require(qtCameraSettings(edited).value("Orb").solverLastN==3 && removeQtCameraSetting(edited,"Orb")==removeQtCameraSetting(source,"Orb"),"Count Apply changed unrelated source");
      auto expectedSettings=qtCameraSettings(source).value("Orb"); expectedSettings.solverLastN=3;
      require(edited==setQtCameraSetting(source,"Orb",expectedSettings),"Count edit changed other imported camera metadata");
      editor->undo(); require(editor->toPlainText()==source,"Count Undo not exact"); editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Count Redo failed");
      require(!window.applyResourceChanges("Orb",{{"@QtSolverLastN","-1"}},edited).isEmpty() && editor->toPlainText()==edited,"Invalid count failed atomic rollback");
      const auto saved=files.filePath("LastN α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==edited,"Unicode Save/reopen failed");
      panel=open(); mode=panel->findChild<QComboBox *>("orbit_SolverIterations"); count=panel->findChild<QSpinBox *>("orbit_SolverIterLastN");
      require(mode->currentText()=="LastN" && count->value()==3,"Saved count did not reopen"); mode->setCurrentText("All"); require(!count->isEnabled(),"Count enabled outside LastN"); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto all=editor->toPlainText(); require(!qtCameraSettings(all).value("Orb").solverLastN,"All selection did not clear LastN"); editor->undo(); require(editor->toPlainText()==edited && window.buildScript(),"Mode Undo failed");
      require(window.renameResource("Orb","RenamedOrb",edited).isEmpty() && qtCameraSettings(editor->toPlainText()).value("RenamedOrb").solverLastN==3,"Rename lost LastN metadata"); editor->undo(); require(editor->toPlainText()==edited,"Rename Undo failed");
      std::cout<<"PASS source-ordered LastN conversion/count validation, grouped history lifecycle, actual pending controls/Apply/rollback/Undo/Redo/Unicode reopen/Rename, replay/Output reopening and identical complete new DC report.\n";
   } catch (BaseException &e) { std::cerr<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
   return 0;
}
