#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "ResourceEditor.hpp"
#include "PlotWidget.hpp"
#include "OrbitRenderer.hpp"
#include "OrbitSegmentLabels.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QEventLoop>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *why) { if (!value) throw std::runtime_error(why); }
static void near(double value,double expected,const char *why) { require(std::abs(value-expected)<1e-7,why); }
static QByteArray bytes(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Cannot read report"); return f.readAll(); }
static void preserve(const QString &name,const QByteArray &data) {
   const auto root=qEnvironmentVariable("GMAT_QT_SEGMENT_LABEL_EVIDENCE"); if (root.isEmpty()) return;
   QDir().mkpath(root); QFile f(root+"/"+name); require(f.open(QIODevice::WriteOnly) && f.write(data)==data.size(),"Cannot preserve evidence");
}
static const PlotCurve &curve(const PlotModel &model,const QString &name) {
   for (const auto &c:model.curves) if (c.name==name) return c;
   throw std::runtime_error("Missing plotted object");
}
static void rendering(QApplication &app,bool native)
{
   auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
   model->axes=false; model->grid=false; model->legend=false; model->scriptedCamera=true;
   model->cameras.push_back({1,{0,0,15000},{0,0,0},{0,1,0},false});
   auto &c=model->curves[0]; c.name="Probe"; c.segmentLabels=true; c.lines=false; c.showObject=false;
   c.importedLabel=false; c.importedFontSize=20;
   auto point=[](double epoch,double x,quint64 frame,quint64 arc) {
      PlotPoint p; p.epoch=epoch; p.x=x; p.frame=frame; p.regularArc=arc; p.connect=true; return p;
   };
   auto first=point(0,-3000,1,1),middle=point(5,-2500,2,3),last=point(10,-1000,3,1);
   auto otherFirst=point(20,1000,4,2),otherLast=point(30,3000,5,2);
   c.points={first,middle,last,otherFirst,otherLast}; model->frame=5;
   PlotRegularArc a; a.id=1; a.name="Departure"; a.first=first; a.last=last; a.members={1,3}; a.finalized=true;
   PlotRegularArc b; b.id=2; b.name="Arrival"; b.first=otherFirst; b.last=otherLast; b.members={2}; b.finalized=true;
   model->regularArcs[c.name]={a,b};
   auto trial=point(2.5,99999,6,1); trial.solver=true; c.points.push_back(trial);
   model->replayCursor=PlotReplayCursor{1,2,.5,2.5};
   auto labels=orbitSegmentLabels(*model,c,1);
   require(labels.size()==2 && labels[0].name=="Departure" && labels[1].name=="Arrival","Independent named segments missing");
   near(labels[0].pose.x,-2750,"Label ignored retained nonlinear point or merged identity");
   near(labels[1].pose.x,1000,"Future segment did not clamp to its first endpoint");
   model->replayCursor=PlotReplayCursor{4,5,.5,25}; labels=orbitSegmentLabels(*model,c,4);
   near(labels[0].pose.x,-1000,"Completed segment did not clamp to endpoint"); near(labels[1].pose.x,2000,"Active label did not follow epoch");
   std::swap(model->regularArcs[c.name][1].first,model->regularArcs[c.name][1].last);
   near(orbitSegmentLabels(*model,c,4)[1].pose.x,2000,"Backward segment epoch lookup failed");
   std::swap(model->regularArcs[c.name][1].first,model->regularArcs[c.name][1].last);
   c.points.pop_back(); const auto original=c.points;
   PlotWidget widget(model); widget.resize(700,500); widget.show(); app.processEvents();
   widget.canvas()->setReplayCursor({1,2,.5,2.5});
   if (native) {
      QEventLoop settle; QTimer::singleShot(150,&settle,&QEventLoop::quit); settle.exec();
      auto *renderer=dynamic_cast<OrbitRenderer *>(widget.findChild<QOpenGLWidget *>());
      require(renderer && renderer->isValid(),"Private native context unavailable");
   }
   const auto visible=widget.canvas()->captureImage(); require(!visible.isNull(),"Label capture unavailable");
   c.segmentLabels=false; widget.canvas()->refresh(); const auto hidden=widget.canvas()->captureImage();
   require(visible!=hidden,"Segment-only labels did not render independently of object/path");
   int changed=0; for (int y=0;y<visible.height();++y) for (int x=0;x<visible.width();++x) if (visible.pixel(x,y)!=hidden.pixel(x,y)) ++changed;
   require(changed>100,"Segment label pixels missing");
   c.segmentLabels=true; c.importedFontSize=0; widget.canvas()->refresh(); require(widget.canvas()->captureImage()==hidden,"Font zero did not hide segment labels");
   c.importedFontSize=20; model->labels=false; widget.canvas()->refresh(); require(widget.canvas()->captureImage()==hidden,"Master labels did not hide segment labels");
   model->labels=true; c.visible=false; require(orbitSegmentLabels(*model,c,1).isEmpty(),"Hidden curve retained segment labels"); c.visible=true;
   const auto capture=qEnvironmentVariable("GMAT_QT_SEGMENT_LABEL_EVIDENCE");
   if (!capture.isEmpty()) { QDir().mkpath(capture); require(visible.save(capture+(native ? "/native-labels.png" : "/fallback-labels.png")),"Cannot preserve label image"); }
   require(c.points.size()==original.size(),"Label rendering mutated retained data");
   for (size_t i=0;i<original.size();++i) require(c.points[i].x==original[i].x && c.points[i].epoch==original[i].epoch && c.points[i].regularArc==original[i].regularArc,"Label rendering changed raw samples");
   model->maxPoints=1; model->trim(); widget.canvas()->setFrame(std::numeric_limits<quint64>::max());
   labels=orbitSegmentLabels(*model,c,widget.canvas()->frame());
   require(labels.size()==2,"Trimming erased compact segment endpoints"); near(labels[0].pose.x,-1000,"Trimmed old segment lost endpoint");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("SegmentLabels");
   const bool native=argc==2 && QString::fromLocal8Bit(argv[1])=="--native";
   if (argc!=2 || QGuiApplication::platformName()!=(native ? "xcb" : "offscreen")) return 2;
   try {
      TestSettings settings; rendering(app,native);
      if (native) { std::cout<<"PASS private native segment-label pixels, epoch following, endpoints, font and master visibility.\n"; return 0; }
      const QString legacy="Create OpenFramesInterface Display;\nDisplay.Add = {Sat, Earth};\nDisplay.DrawUsePropLabel = [true];\n";
      const auto converted=convertOpenFramesViews(legacy); require(converted.error.isEmpty(),"Segment label conversion failed");
      const auto imported=qtCameraSettings(converted.script).value("Display");
      require(imported.objectSegmentLabels==QMap<QString,bool>{{"Sat",true},{"Earth",false}},"Default/prefix segment flags lost");
      require(!converted.notes.join('\n').contains("DrawUsePropLabel: retained as a comment"),"Active labels still warned unsupported");
      const auto reset=convertOpenFramesViews(legacy+"Display.Add = {Earth, Sat};\nDisplay.DrawUsePropLabel = [true];\n");
      require(qtCameraSettings(reset.script).value("Display").objectSegmentLabels==QMap<QString,bool>{{"Earth",true},{"Sat",false}},"Source-ordered Add reset lost");
      const auto invalid=legacy+"Display.DrawUsePropLabel = [maybe];\n";
      require(!convertOpenFramesViews(invalid).error.isEmpty() && convertOpenFramesViews(invalid).script==invalid,"Invalid flag altered source");
      QTemporaryDir files; require(files.isValid(),"Private fixture directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("states.txt");
      const QString resources="% retain authored source α\nCreate Spacecraft Sat;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.WriteReport = false;\nValues.Precision = 17;\n";
      const QString state="Report Values Sat.A1ModJulian Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z;\n";
      const QString mission="BeginMissionSequence;\nPropagate 'Departure' P(Sat) {Sat.ElapsedSecs = 30};\n"+state+"Propagate 'Arrival' P(Sat) {Sat.ElapsedSecs = 30};\n"+state;
      editor->setPlainText(resources+mission); require(window.runMission()==MainWindow::RunResult::Completed,"Independent reference failed");
      const auto expected=bytes(report); preserve("reference.script",(resources+mission).toUtf8()); preserve("reference.report",expected);
      auto drawing=imported; drawing.objectSegmentLabels["Earth"]=true; drawing.objectLabels={{"Sat",false},{"Earth",false}};
      const QString display="Create OrbitView Display;\nDisplay.Add = {Sat, Earth};\nDisplay.CoordinateSystem = EarthMJ2000Eq;\nDisplay.DrawObject = [false false];\nDisplay.SolverIterations = Current;\n"+qtCameraDirective("Display",drawing)+"\n";
      const QString source=resources+display+mission; editor->setPlainText(source);
      require(window.runMission()==MainWindow::RunResult::Completed,"Segment label mission failed");
      require(bytes(report)==expected && editor->toPlainText()==source,"Labels changed complete numerical report/source");
      preserve("labels.script",source.toUtf8()); preserve("labels.report",bytes(report));
      const auto model=window.plotReceiver()->model("Display"); require(model!=nullptr,"Runtime plot missing");
      const auto arcs=model->regularArcs.value("Sat"),bodies=model->regularArcs.value("Earth");
      require(arcs.size()==2 && arcs[0].name=="Departure" && arcs[1].name=="Arrival","Label-only metadata missed named regular spacecraft arcs");
      require(bodies.size()==1 && bodies[0].name=="Departure" && bodies[0].providers.size()==2,"Body labels did not retain first provider and merged trajectory");
      for (const auto &p:curve(*model,"Sat").points) require(!p.solver && p.regularArc,"Accepted sample lacks regular arc identity");
      const auto rows=QString::fromUtf8(expected).trimmed().split('\n'); require(rows.size()==2,"Unexpected report shape");
      for (int i=0;i<2;++i) {
         const auto values=rows[i].simplified().split(' '); require(values.size()==4,"Report columns missing");
         near(arcs[i].last.epoch,values[0].toDouble(),"Arc epoch differs from Report");
         near(arcs[i].last.x,values[1].toDouble(),"Arc X differs from Report"); near(arcs[i].last.y,values[2].toDouble(),"Arc Y differs from Report"); near(arcs[i].last.z,values[3].toDouble(),"Arc Z differs from Report");
      }
      auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems("Display",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Display resource missing");
      tree->itemDoubleClicked(items.front(),0); auto *panel=dynamic_cast<ResourceEditor *>(window.findChild<QMdiArea *>("workspace")->activeSubWindow()->widget()); require(panel,"Resource form missing");
      auto choose=[&](bool accept) {
         std::exception_ptr failure; QTimer::singleShot(0,panel,[&] {
            auto *dialog=panel->findChild<QDialog *>("orbitObjectDrawingDialog");
            try {
               require(dialog,"Object drawing dialog missing");
               auto *sat=dialog->findChild<QComboBox *>("orbitDrawing_segmentLabel_Sat"),*earth=dialog->findChild<QComboBox *>("orbitDrawing_segmentLabel_Earth");
               require(sat && earth && sat->isEnabled() && earth->isEnabled() && sat->currentText()=="On" && earth->currentText()=="On","Segment controls failed readback");
               sat->setCurrentText("Off"); dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
            } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
         }); panel->findChild<QPushButton *>("editOrbitDrawing")->click(); if (failure) std::rethrow_exception(failure);
      };
      choose(false); require(!panel->hasChanges() && editor->toPlainText()==source,"Cancel changed source");
      choose(true); require(panel->hasChanges() && editor->toPlainText()==source,"Labels did not remain pending");
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto edited=editor->toPlainText(); require(qtCameraSettings(edited).value("Display").objectSegmentLabels==QMap<QString,bool>{{"Sat",false},{"Earth",true}} && removeQtCameraSetting(edited,"Display")==removeQtCameraSetting(source,"Display"),"Apply changed unrelated source or flags");
      editor->undo(); require(editor->toPlainText()==source,"Undo not exact"); editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Redo/build failed");
      const auto saved=files.filePath("labels α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==edited,"Unicode Save/reopen failed");
      require(window.renameResource("Sat","RenamedSat",edited).isEmpty() && qtCameraSettings(editor->toPlainText()).value("Display").objectSegmentLabels.contains("RenamedSat"),"Rename lost segment flags");
      editor->undo(); require(editor->toPlainText()==edited && window.buildScript(),"Rename Undo failed");
      require(window.applyResourceChanges("Display",{{"Add","Earth"}},edited).isEmpty() && qtCameraSettings(editor->toPlainText()).value("Display").objectSegmentLabels==QMap<QString,bool>{{"Earth",true}},"Object removal left stale label metadata");
      require(orbitSegmentLabels(*model,curve(*model,"Sat"),model->frame).size()==2,"Retained label display depended on deleted live engine objects");
      std::cout<<"PASS segment labels: conversion defaults/prefix/reset, accepted arc/body identity, independent numerical report, rendered replay/clamp/trim/visibility, pending controls/Cancel/Apply/Undo/Redo/Save/reopen/Rename/removal.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
