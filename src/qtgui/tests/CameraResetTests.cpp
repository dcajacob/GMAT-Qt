#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "OrbitCamera.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "CoordinateSystem.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QShortcut>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static bool near(const std::array<double,3> &value,osg::Vec3d expected) { return (osg::Vec3d(value[0],value[1],value[2])-expected).length()<1e-8; }
static osg::Vec3d eye(const PlotModel &model,quint64 frame) {
   const auto basis=orbitCamera(model,frame,model.userView->yaw,model.userView->pitch,10000);
   return basis.target+basis.outward*basis.distance;
}
static QString input()
{
   return "% retain physics Δ\nCreate Variable Total;\nTotal = 123.5; % expression\nCreate Spacecraft Scout;\nCreate Propagator Prop;\n"
      "Create OpenFramesInterface Display;\nDisplay.Add = {Scout, Earth};\n"
      "Create OpenFramesView Primary Body AutoOnly Trajectory Segment;\nDisplay.View = {Primary, Body, AutoOnly, Trajectory, Segment};\n"
      "Primary.ViewFrame = CoordinateSystem;\nPrimary.SetCameraLocation = On;\nPrimary.CameraEye = [7001 -8002 9003];\nPrimary.CameraCenter = [11 12 13];\nPrimary.CameraUp = [0 0 1];\n"
      "Primary.SetCurrentLocation = On;\nPrimary.CurrentEye = [101 -202 303];\nPrimary.CurrentCenter = [1 2 3];\nPrimary.CurrentUp = [0 1 0];\n"
      "Body.ViewFrame = Scout;\nBody.InertialFrame = On;\nBody.SetDefaultLocation = On;\nBody.DefaultEye = [4001 -5002 6003];\nBody.DefaultCenter = [4 5 6];\nBody.SetCurrentLocation = On;\nBody.CurrentEye = [401 -502 603];\n"
      "AutoOnly.ViewFrame = Scout;\nAutoOnly.InertialFrame = On;\nAutoOnly.SetDefaultLocation = Off;\nAutoOnly.SetCurrentLocation = On;\nAutoOnly.CurrentEye = [301 -302 303];\n"
      "Trajectory.ViewFrame = Scout;\nTrajectory.ViewTrajectory = On;\nTrajectory.SetCurrentLocation = On;\nTrajectory.CurrentEye = [901 -902 903];\n"
      "Segment.ViewFrame = Scout.Arc;\nSegment.InertialFrame = On;\nSegment.SetDefaultLocation = On;\nSegment.DefaultEye = [21 -22 23];\nSegment.SetCurrentLocation = On;\nSegment.CurrentEye = [41 -42 43];\n"
      "BeginMissionSequence;\nPropagate 'Arc' Prop(Scout) {Scout.ElapsedSecs = 60}; % retained, not run\nTotal = Total + 1;\n";
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   try {
      require(argc==2,"Expected a startup path"); TestSettings isolated;
      const QString source=input(); const auto converted=convertOpenFramesViews(source);
      require(converted.error.isEmpty(),qPrintable(converted.error));
      const auto setting=qtCameraSettings(converted.script).value("Display");
      require(setting.reset && setting.reset->eye==std::array<double,3>{7001,-8002,9003} && setting.reset->center==std::array<double,3>{11,12,13} && setting.reset->up==std::array<double,3>{0,0,1},"Primary Default/alias reset pose was discarded");
      require(converted.script.contains("Display.ViewPointVector = [101 -202 303]") && setting.views.size()==4 && setting.views[0].eye==std::array<double,3>{401,-502,603},"Current no longer takes precedence for initial primary/named pose");
      require(setting.views[0].reset && setting.views[0].reset->eye==std::array<double,3>{4001,-5002,6003} && setting.views[1].reset && setting.views[1].reset->automaticBody=="Scout" && setting.views[2].reset && setting.views[2].reset->automaticTrajectory=="Scout","Named stored/automatic default modes changed");
      require(converted.script.contains("% retain physics Δ\nCreate Variable Total;\nTotal = 123.5; % expression\n"),"Conversion lost unrelated source");
      require(converted.script.endsWith(source.mid(source.indexOf("BeginMissionSequence"))) && converted.script.contains("% Qt conversion: Primary.CameraEye = [7001 -8002 9003];"),"Conversion changed mission calculations or original camera aliases");
      const auto roundtrip=qtCameraSettings(qtCameraDirective("Display",setting)).value("Display");
      require(roundtrip.reset && roundtrip.reset->eye==setting.reset->eye && roundtrip.views[3].reset && roundtrip.views[3].reset->segmentFrame=="Scout.Arc","Default pose metadata roundtrip lost named/segment inputs");
      const auto lastWrite=convertOpenFramesViews(QString(source).replace("Primary.SetCurrentLocation = On;","Primary.DefaultEye = [71 -82 93];\nPrimary.SetCurrentLocation = On;"));
      require(lastWrite.error.isEmpty() && qtCameraSettings(lastWrite.script).value("Display").reset->eye==std::array<double,3>{71,-82,93},"Default/alias source-order precedence changed");
      const auto invalidSource=QString(source).replace("Primary.CameraUp = [0 0 1]","Primary.CameraUp = [0 0 0]");
      const auto invalid=convertOpenFramesViews(invalidSource); require(!invalid.error.isEmpty() && invalid.script==invalidSource,"Invalid active Default reset inputs lost original source");
      QTemporaryDir files; require(files.isValid(),"Temporary files unavailable"); QFile saved(files.filePath("camera reset Δ.script"));
      require(saved.open(QIODevice::WriteOnly) && saved.write(converted.script.toUtf8())==converted.script.toUtf8().size(),"Reset source save failed"); saved.close(); require(saved.open(QIODevice::ReadOnly),"Reset source reopen failed");
      require(QString::fromUtf8(saved.readAll())==converted.script,"Unicode camera source roundtrip changed bytes");

      MainWindow window; require(window.initialize(QString::fromLocal8Bit(argv[1])),"Initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Source editor missing"); editor->setPlainText(converted.script); require(window.buildScript(),"Converted reset configuration failed to build");
      auto *moderator=Moderator::Instance(); auto *scout=dynamic_cast<Spacecraft *>(moderator->GetConfiguredObject("Scout"));
      require(scout && scout->Initialize(),"Spacecraft reference could not initialize");
      auto *earth=moderator->GetSolarSystemInUse()->GetBody("Earth");
      auto *frame=dynamic_cast<CoordinateSystem *>(moderator->GetConfiguredObject("EarthMJ2000Eq"));
      QMdiArea area; area.resize(1000,750); area.show(); QtPlotReceiver receiver(&area); receiver.cameraSettings=qtCameraSettings(converted.script);
      require(receiver.CreateGlPlotWindow("Display","",0,0,.8,.8,false,0),"Receiver plot creation failed");
      receiver.SetGlSolarSystem("Display",moderator->GetSolarSystemInUse()); receiver.SetGlObject("Display",{"Scout","Earth"},{scout,earth}); receiver.SetGlCoordSystem("Display",frame,frame,frame);
      receiver.SetGl3dViewOption("Display",nullptr,nullptr,nullptr,1,Rvector3(0,0,0),Rvector3(101,-202,303),Rvector3(1,2,3),"Y",true,true,true);
      auto publish=[&](double time,double offset,bool solver=false) { require(receiver.UpdateGlPlot("Display","",{"Scout","Earth"},time,{1000+offset,0},{2000+offset,0},{3000+offset,0},{0,0},{0,0},{0,0},{},{},solver,0,true,true,false),"Synthetic camera publication rejected"); };
      publish(21545,0); publish(21545.001,100);
      const auto model=receiver.model("Display"); require(model && model->resetView && model->resetView->cameras.size()==2 && near(model->resetView->cameras.front().eye,{7001,-8002,9003}),"Primary reset history not retained independently");
      PlotWidget *widget=nullptr; for (auto *child:area.subWindowList()) if (auto *plot=dynamic_cast<PlotWidget *>(child->widget())) widget=plot;
      require(widget,"Plot widget unavailable"); auto *reset=widget->findChild<QAction *>("orbitResetView");
      acceptanceView(*widget);
      auto *shortcut=widget->canvas()->findChild<QShortcut *>("orbitResetShortcut");
      require(reset && shortcut && shortcut->key()==QKeySequence(Qt::Key_Space) && shortcut->context()==Qt::WidgetWithChildrenShortcut && shortcut->parent()==widget->canvas(),"Reset operation or plot-scoped Space shortcut missing");
      widget->canvas()->setViewAngles(.7,-.3); widget->canvas()->zoomBy(4); const auto before=converted.script;
      const double fov=model->fieldOfView; reset->trigger();
      require(model->resetCamera && model->userView->yaw==0 && model->userView->pitch==0 && model->userView->zoom==1 && !model->fitCamera && model->fieldOfView==fov && (eye(*model,1)-osg::Vec3d(7001,-8002,9003)).length()<1e-8,"Reset did not restore Default pose/scale or changed projection");
      PlotWidget reopened(std::const_pointer_cast<PlotModel>(model)); require(model->resetCamera && (eye(*model,2)-osg::Vec3d(7001,-8002,9003)).length()<1e-8,"Close/reopen state lost selected Default reset history");
      QAction *script=nullptr; for (auto *action:widget->findChildren<QAction *>()) if (action->text()=="Script view") script=action;
      require(script,"Initial Script view operation lost"); script->trigger(); require(!model->resetCamera && (eye(*model,2)-osg::Vec3d(101,-202,303)).length()<1e-8,"Script view no longer restores initial Current");
      auto *views=widget->findChild<QComboBox *>("orbitCameraView"); require(views,"Named camera selection missing"); views->setCurrentIndex(1); reset->trigger();
      widget->canvas()->setFrame(1); require((eye(*model,1)-osg::Vec3d(5001,-3002,9003)).length()<1e-8,"Named body Default did not follow first replay pose");
      widget->canvas()->setFrame(2); require((eye(*model,2)-osg::Vec3d(5101,-2902,9103)).length()<1e-8,"Named body Default did not follow latest replay pose");
      views->setCurrentIndex(2); reset->trigger(); const auto automatic=orbitCamera(*model,2,0,0,10000);
      require(model->cameraViews[2].reset->automaticBody=="Scout" && (automatic.target-osg::Vec3d(1100,2100,3100)).length()<1e-8 && automatic.distance<100,"Current-only reset did not use automatic body framing");
      views->setCurrentIndex(3); reset->trigger(); require(model->cameraViews[3].reset->automaticTrajectory=="Scout","Trajectory automatic reset metadata missing");
      require(receiver.TakeGlAction("Display","SetOrbitProvider:99 0 1 Arc") && receiver.TakeGlAction("Display","BeginOrbitArcSample") && receiver.TakeGlAction("Display","OrbitArcPose:Scout 21545 10 20 30 1 0 0 0 1 0 0 0 1 1 0 0 0 1 0 0 0 1") && receiver.TakeGlAction("Display","PrepareOrbitArcCameras") && receiver.TakeGlAction("Display","FinalizeOrbitArc"),"Named segment reset capture failed");
      views->setCurrentIndex(4); reset->trigger(); require(model->cameraViews[4].reset && !model->cameraViews[4].reset->cameras.empty() && near(model->cameraViews[4].reset->cameras.back().eye,{31,-2,53}),"Segment Default reset lost retained first arc pose");
      receiver.SetMaxGlDataPoints("Display",1); require(model->resetView->cameras.size()==1 && model->cameraViews[1].reset->cameras.size()==1,"Reset histories ignored retention limit");
      publish(21545.002,200,true); receiver.TakeGlAction("Display","ClearSolverData");
      for (const auto &view:model->cameraViews) if (view.reset) for (const auto &camera:view.reset->cameras) require(!camera.solver,"Reset history retained cleared solver samples");
      require(editor->toPlainText()==before,"Viewer Reset mutated mission source");
      receiver.clear();
      require(window.renameResource("Scout","RenamedScout",editor->toPlainText()).isEmpty(),"Reset-reference Rename failed");
      auto renamed=qtCameraSettings(editor->toPlainText()).value("Display"); require(renamed.views[0].reset->reference=="RenamedScout" && renamed.views[3].reset->segmentFrame=="RenamedScout.Arc","Rename stranded reset metadata references");
      editor->undo(); require(editor->toPlainText()==before && window.buildScript(),"Rename Undo lost exact source");
      require(window.applyResourceChanges("Display",{{"ViewPointVector","[90 -100 110]"}},before).isEmpty(),"Explicit primary camera edit failed");
      const auto edited=qtCameraSettings(editor->toPlainText()).value("Display"); require(!edited.reset && edited.views[0].reset,"Explicit primary edit kept stale reset pose or removed named defaults");
      editor->undo(); require(editor->toPlainText()==before && window.buildScript(),"Explicit camera Undo lost reset source");
      require(window.deleteResource("Display",before).isEmpty() && !qtCameraSettings(editor->toPlainText()).contains("Display"),"Plot Delete retained reset metadata");
      editor->undo(); require(editor->toPlainText()==before && window.buildScript() && qtCameraSettings(editor->toPlainText()).value("Display").reset.has_value(),"Plot Delete Undo lost reset metadata/source");
      std::cout<<"PASS separate Default/Current and alias precedence, exact source/Unicode roundtrip, primary/named stored and automatic resets, replay/segment/trim/solver lifecycle, initial Script view, projection retention, scoped action, retained reopening, Rename and explicit-edit/Undo. Synthetic publisher values; no mission propagation or native/host acceptance.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
