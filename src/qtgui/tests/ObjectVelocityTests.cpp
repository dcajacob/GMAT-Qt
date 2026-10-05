#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "ResourceEditor.hpp"
#include "OrbitVelocity.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "PlotWidget.hpp"
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
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <iomanip>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static const PlotCurve &curve(const PlotModel &model,const QString &name) {
   for (const auto &item:model.curves) if (item.name==name) return item;
   throw std::runtime_error("Velocity curve is missing");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("ObjectVelocity");
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","Velocity regression requires offscreen and a startup path");
      TestSettings isolated; QTemporaryDir files; require(files.isValid(),"Velocity temporary folder missing");
      const QString legacy="% preserve velocity source α\nCreate OpenFramesInterface Display;\nDisplay.Add = {Sat, Earth};\nDisplay.DrawVelocity = [true];\nDisplay.DrawTrajectory = [false false];\n";
      const auto converted=convertOpenFramesViews(legacy); require(converted.error.isEmpty(),"DrawVelocity conversion failed");
      const auto setting=qtCameraSettings(converted.script).value("Display");
      require(setting.objectVelocities==QMap<QString,bool>{{"Sat",true},{"Earth",false}},"Velocity prefix/default semantics lost");
      require(converted.script.contains("% Qt conversion: Display.DrawVelocity = [true];") && !converted.notes.join('\n').contains("DrawVelocity: retained as a comment"),"Velocity source/warning disposition incorrect");
      require(qtCameraSettings(qtCameraDirective("Display",setting)).value("Display").objectVelocities==setting.objectVelocities,"Named velocities did not round trip");
      const auto reset=convertOpenFramesViews(legacy+"Display.Add = {Earth, Sat};\nDisplay.DrawVelocity = [true false true];\n");
      require(reset.error.isEmpty() && qtCameraSettings(reset.script).value("Display").objectVelocities==QMap<QString,bool>{{"Sat",true},{"Earth",false}},"Repeated scripted Add changed velocity order/excess-prefix behavior");
      const auto malformed=legacy+"Display.DrawVelocity = [maybe];\n"; const auto rejected=convertOpenFramesViews(malformed);
      require(!rejected.error.isEmpty() && rejected.script==malformed,"Malformed velocity flags changed source");
      for (const auto type:{"Relative Velocity","Thrust Vector"}) {
         const QString unsupported="Create OpenFramesVector Arrow;\nArrow.VectorType = '"+QString(type)+"';\n"; const auto result=convertOpenFramesViews(unsupported);
         require(!result.error.isEmpty() && result.script==unsupported,"Unsupported vector mode was silently mapped");
      }
      bool invalid=false; try { qtCameraSettings("% GMAT-Qt-Camera {\"plot\":\"Display\",\"perspective\":false,\"fieldOfView\":50,\"objectVelocities\":{\"Sat\":1}}\n"); } catch (std::exception &) { invalid=true; }
      require(invalid,"Nonboolean velocity metadata accepted");
      PlotPoint sample; sample.x=10; sample.y=-20; sample.z=30; sample.viewVelocity={1.2,-2.5,.75}; sample.hasVelocity=true; sample.color=Qt::blue; sample.solver=true;
      sample.bodyToView={0,1,0,-1,0,0,0,0,1}; // This must not rotate view-frame velocity a second time.
      const auto segment=orbitVelocitySegment(sample);
      require(segment && (segment->start-osg::Vec3d(10,-20,30)).length()==0 && (segment->end-osg::Vec3d(1210,-2520,780)).length()<1e-12 && segment->color==Qt::blue,"Velocity geometry differs from independent OF 1000-second endpoint/color convention");
      sample.hasVelocity=false; require(!orbitVelocitySegment(sample),"Absent velocity reused a stale estimate"); sample.hasVelocity=true;
      sample.viewVelocity[0]=std::numeric_limits<double>::max(); require(!orbitVelocitySegment(sample),"Unavailable velocity marker accepted");
      sample.viewVelocity={0,0,0}; const auto stationary=orbitVelocitySegment(sample); require(stationary && stationary->start==stationary->end,"Zero velocity acquired an invented direction");
      auto drawing=std::make_shared<PlotModel>(PlotModel::Kind::Orbit); drawing->axes=false; drawing->grid=false; drawing->legend=false; drawing->labels=false;
      drawing->scriptedCamera=true; drawing->perspective=true; drawing->fieldOfView=45; drawing->cameras.push_back({0,{0,-1000,600},{0,0,0},{0,0,1},false});
      auto &probe=drawing->curves[0]; probe.name="Probe"; probe.lines=false; probe.showObject=false; probe.color=Qt::yellow;
      drawing->frame=1; drawing->append(0,-100,0,0); probe.points.back().viewVelocity={.25,0,.1}; probe.points.back().hasVelocity=true;
      drawing->frame=2; drawing->append(0,100,0,0); probe.points.back().viewVelocity={0,0,.3}; probe.points.back().hasVelocity=true;
      PlotCanvas canvas(drawing); canvas.resize(640,480); canvas.show(); canvas.setFrame(1); QApplication::processEvents(); const auto blank=canvas.captureImage();
      probe.objectVelocity=true; canvas.refresh(); const auto first=canvas.captureImage(); require(first!=blank,"Velocity-only hidden object/trajectory fallback was blank");
      canvas.setFrame(2); require(canvas.captureImage()!=first,"Velocity history did not advance"); canvas.setFrame(1); require(canvas.captureImage()==first,"Velocity replay used future samples");
      probe.visible=false; canvas.refresh(); require(canvas.captureImage()==blank,"Hidden curve kept velocity pixels"); probe.visible=true;
      drawing->maxPoints=1; drawing->trim(); require(probe.points.front().hasVelocity && probe.points.front().viewVelocity==std::array<double,3>{0,0,.3},"Trimming lost/mixed retained velocity");
      {
         QMdiArea workspace; QtPlotReceiver callbacks(&workspace);
         require(callbacks.CreateGlPlotWindow("Samples","",0,0,500,350,false,0),"Callback fixture window unavailable");
         callbacks.SetGlObject("Samples",StringArray{"Sat"},std::vector<SpacePoint *>{nullptr});
         require(callbacks.UpdateGlPlot("Samples","",{"Sat"},1,{10},{20},{30},{1},{2},{3},{},{},false,0,false,true,false),"Complete velocity callback rejected");
         const auto data=callbacks.model("Samples"); require(data && curve(*data,"Sat").points.back().hasVelocity && curve(*data,"Sat").points.back().viewVelocity==std::array<double,3>{1,2,3},"Callback velocity arrays were not retained");
         require(callbacks.UpdateGlPlot("Samples","",{"Sat"},2,{11},{21},{31},{},{},{},{},{},false,0,false,true,false),"Position with absent velocity rejected");
         require(curve(*data,"Sat").points.size()==2 && curve(*data,"Sat").points.back().x==11 && !curve(*data,"Sat").points.back().hasVelocity && !orbitVelocitySegment(curve(*data,"Sat").points.back()),"Absent velocity dropped valid position or reused previous velocity");
         require(callbacks.UpdateGlPlot("Samples","",{"Sat"},3,{12},{22},{32},{std::numeric_limits<double>::max()},{2},{3},{},{},false,0,false,true,false),"Position with unavailable velocity rejected");
         require(curve(*data,"Sat").points.size()==3 && !curve(*data,"Sat").points.back().hasVelocity,"Finite unavailable velocity marker leaked into geometry");
      }
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto report=files.filePath("velocity state Δ.txt"),saved=files.filePath("velocity mission Δ.script");
      // OrbitData forces nutation recomputation for Report parameters; OrbitPlot
      // honors Earth's cache interval. Zero makes both use the same high-fidelity
      // rotation, instead of requiring sub-nanometer agreement with cached data.
      const QString resources="Earth.NutationUpdateInterval = 0;\nCreate Spacecraft Sat;\nCreate CoordinateSystem Rotating;\nRotating.Origin = Earth;\nRotating.Axes = BodyFixed;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 10;\nProp.MaxStep = 10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 17;\nValues.WriteHeaders = false;\n";
      const QString views="Create OpenFramesInterface Display;\nDisplay.Add = {Sat, Earth};\nDisplay.CoordinateSystem = Rotating;\nDisplay.DrawObject = [false false];\nDisplay.DrawTrajectory = [false false];\nDisplay.DrawLabel = [false false];\nDisplay.DrawCenterPoint = [false false];\nDisplay.DrawEndPoints = [false false];\nDisplay.DrawVelocity = [true false];\nDisplay.EnableStars = Off;\nDisplay.EnableConstellations = Off;\n";
      const QString mission="BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 60};\nReport Values Sat.A1ModJulian Sat.Rotating.X Sat.Rotating.Y Sat.Rotating.Z Sat.Rotating.VX Sat.Rotating.VY Sat.Rotating.VZ Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";
      const auto translated=convertOpenFramesViews(resources+views+mission); require(translated.error.isEmpty() && translated.script.endsWith(mission),"Velocity conversion changed the numerical mission");
      MainWindow window; window.show(); require(window.initialize(startup),"Velocity runtime initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); editor->setPlainText(translated.script);
      require(window.runMission()==MainWindow::RunResult::Completed,"Velocity publisher mission failed");
      const auto retained=window.plotReceiver()->model("Display"); require(retained && curve(*retained,"Sat").points.size()>2,"Actual callback history missing");
      const auto &last=curve(*retained,"Sat").points.back(); require(last.hasVelocity && curve(*retained,"Sat").objectVelocity && !curve(*retained,"Sat").lines && !curve(*retained,"Sat").showObject,"Velocity callback/independent visibility flags missing");
      QFile output(report); require(output.open(QIODevice::ReadOnly),"Independent velocity report missing");
      const auto values=QString::fromUtf8(output.readAll()).trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(values.size()==10,"Unexpected velocity report shape");
      const auto reportEpoch=values[0].toDouble(),epochError=last.epoch-reportEpoch;
      const std::array<double,3> position{last.x,last.y,last.z}; double frameDifference=0; bool statesAgree=true;
      for (int axis=0;axis<3;++axis)
         statesAgree=statesAgree && std::abs(position[axis]-values[axis+1].toDouble())<1e-7 && std::abs(last.viewVelocity[axis]-values[axis+4].toDouble())<1e-10;
      if (std::abs(epochError)>=1e-9 || !statesAgree) {
         std::cerr<<std::setprecision(17)<<"Velocity diagnostic: frame="<<last.frame<<" samples="<<curve(*retained,"Sat").points.size()<<" view="<<retained->coordinates.toStdString()<<" callback_epoch="<<last.epoch<<" report_epoch="<<reportEpoch<<" delta_days="<<epochError<<'\n';
         for (int axis=0;axis<3;++axis)
            std::cerr<<"axis="<<axis<<" callback_position="<<position[axis]<<" report_position="<<values[axis+1].toDouble()<<" delta_km="<<position[axis]-values[axis+1].toDouble()<<" callback_velocity="<<last.viewVelocity[axis]<<" report_velocity="<<values[axis+4].toDouble()<<" delta_km_per_s="<<last.viewVelocity[axis]-values[axis+4].toDouble()<<" report_inertial_velocity="<<values[axis+7].toDouble()<<'\n';
      }
      require(std::abs(epochError)<1e-9,"Velocity callback/report epoch mismatch");
      require(statesAgree,"Retained view-frame publisher position/velocity disagrees with independent Report parameters");
      for (int axis=0;axis<3;++axis) frameDifference+=std::abs(values[axis+4].toDouble()-values[axis+7].toDouble());
      require(frameDifference>.1,"Rotating-frame fixture does not distinguish inertial velocity");
      const auto recorded=orbitVelocitySegment(last); require(recorded.has_value(),"Actual rotating velocity geometry missing");
      const auto source=editor->toPlainText(); const auto count=curve(*retained,"Sat").points.size();
      auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems("Display",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Velocity resource missing");
      tree->itemDoubleClicked(items.front(),0); auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Velocity resource panel missing");
      auto dialog=[&](bool accept) {
         std::exception_ptr failure; QTimer::singleShot(0,panel,[&] {
            auto *view=panel->findChild<QDialog *>("orbitObjectDrawingDialog");
            try {
               require(view,"Velocity dialog missing"); auto *sat=view->findChild<QComboBox *>("orbitDrawing_velocity_Sat"),*earth=view->findChild<QComboBox *>("orbitDrawing_velocity_Earth");
               require(sat && earth && sat->isEnabled() && earth->isEnabled() && sat->currentText()=="On" && earth->currentText()=="Off","Velocity controls did not reflect named imported settings");
               sat->setCurrentText("Off"); earth->setCurrentText("On"); view->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
            } catch (...) { failure=std::current_exception(); if (view) view->reject(); }
         }); panel->findChild<QPushButton *>("editOrbitDrawing")->click(); if (failure) std::rethrow_exception(failure);
      };
      dialog(false); require(!panel->hasChanges() && editor->toPlainText()==source,"Velocity Cancel changed source");
      dialog(true); require(panel->hasChanges() && editor->toPlainText()==source && curve(*retained,"Sat").points.size()==count,"Velocity choices did not remain pending or damaged recorded data");
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto edited=editor->toPlainText(); require(qtCameraSettings(edited).value("Display").objectVelocities==QMap<QString,bool>{{"Sat",false},{"Earth",true}} && removeQtCameraSetting(edited,"Display")==removeQtCameraSetting(source,"Display"),"Velocity Apply changed unrelated source or lost independent flags");
      editor->undo(); require(editor->toPlainText()==source,"Velocity Undo not exact"); editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Velocity Redo/build failed");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==edited,"Velocity Unicode save/reopen failed");
      require(window.renameResource("Sat","VelocitySat",edited).isEmpty() && qtCameraSettings(editor->toPlainText()).value("Display").objectVelocities==QMap<QString,bool>{{"VelocitySat",false},{"Earth",true}},"Rename lost named velocity flags");
      editor->undo(); require(editor->toPlainText()==edited && window.buildScript(),"Velocity Rename Undo failed");
      require(window.applyResourceChanges("Display",{{"Add","Earth"}},edited).isEmpty() && qtCameraSettings(editor->toPlainText()).value("Display").objectVelocities==QMap<QString,bool>{{"Earth",true}},"Removed object kept stale velocity setting");
      const auto replay=orbitVelocitySegment(curve(*retained,"Sat").points[count-1]); require(replay && replay->start==recorded->start && replay->end==recorded->end,"Velocity geometry depended on discarded engine objects");
      auto invalidSetting=qtCameraSettings(editor->toPlainText()); invalidSetting["Display"].objectVelocities["Missing"]=true; invalid=false;
      try { QtPlotReceiver::validateCameraReferences(invalidSetting); } catch (std::exception &) { invalid=true; } require(invalid,"Velocity membership validation was weakened");
      std::cout<<"PASS retained DrawVelocity source defaults/prefix/reset, independent 1000-second geometry and precise unsupported vectors, fallback replay/visibility, real rotating-frame publisher velocities versus Report, pending controls/Cancel/Apply/source/Undo/reopen/rename/pruning and engine-independent retained history; offscreen only.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
