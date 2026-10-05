#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ScriptCompatibility.hpp"
#include "OrbitObjectGuides.hpp"
#include "OrbitObjectDrawingDialog.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTreeWidget>
#include <QTimer>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("ObjectGuides");
   if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   try {
      TestSettings isolated; QTemporaryDir files;
      const QString legacy="% guide settings α\nCreate OpenFramesInterface Display;\nGMAT Display.Add = {Sat, Earth};\nGMAT Display.DrawGrid = [true];\nGMAT Display.DrawXYPlane = [false true];\nGMAT Display.XYPlane = Off;\nGMAT Display.DrawUsePropLabel = [true false];\n";
      const auto converted=convertOpenFramesViews(legacy); require(converted.error.isEmpty(),"Guide conversion failed");
      const auto setting=qtCameraSettings(converted.script).value("Display");
      require(setting.objectGrids==QMap<QString,bool>{{"Sat",true},{"Earth",false}} && setting.objectXYPlanes==QMap<QString,bool>{{"Sat",false},{"Earth",true}},"Per-object guide prefix/defaults lost");
      require(!converted.script.contains("Display.Grid =") && converted.script.contains("GMAT Display.XYPlane = Off;") && converted.script.contains("% Qt conversion: GMAT Display.DrawGrid = [true];"),"Body guides changed global guide settings or lost original source");
      require(!converted.notes.join('\n').contains("DrawUsePropLabel: retained as a comment") && qtCameraSettings(converted.script).value("Display").objectSegmentLabels.value("Sat"),"Propagation labels were not retained as active metadata");
      const auto retained=qtCameraSettings(qtCameraDirective("Display",setting)).value("Display");
      require(retained.objectGrids==setting.objectGrids && retained.objectXYPlanes==setting.objectXYPlanes,"Named guide metadata round trip failed");
      const auto reset=convertOpenFramesViews(legacy+"GMAT Display.Add = {Earth, Sat};\nGMAT Display.DrawGrid = [false true true];\nGMAT Display.DrawXYPlane = [true];\n");
      const auto after=qtCameraSettings(reset.script).value("Display");
      require(reset.error.isEmpty() && after.objectGrids==QMap<QString,bool>{{"Sat",false},{"Earth",true}} && after.objectXYPlanes==QMap<QString,bool>{{"Sat",true},{"Earth",true}},"Repeated scripted Add changed guide order/prefix retention");
      for (const QString field:{"DrawGrid","DrawXYPlane"}) {
         const auto bad=QString(legacy)+"GMAT Display."+field+" = [maybe];\n"; const auto rejected=convertOpenFramesViews(bad);
         require(!rejected.error.isEmpty() && rejected.script==bad,"Malformed guide flags changed original source");
      }
      for (const QString field:{"DrawYZPlane","DrawXZPlane","DrawPlanes"}) {
         const auto bad=QString(legacy)+"GMAT Display."+field+" = [true];\n"; const auto rejected=convertOpenFramesViews(bad);
         require(!rejected.error.isEmpty() && rejected.script==bad,"Nonexistent OF plane field was silently converted");
      }
      bool invalid=false; try { qtCameraSettings("% GMAT-Qt-Camera {\"plot\":\"Display\",\"perspective\":false,\"fieldOfView\":50,\"objectGrids\":{\"Sat\":1}}\n"); } catch (std::exception &) { invalid=true; }
      require(invalid,"Non-boolean guide metadata accepted");
      PlotCurve curve; curve.objectGrid=true; curve.objectXYPlane=true; curve.showObject=false; curve.radius=10;
      PlotPoint pose; pose.x=100; pose.y=200; pose.z=300; pose.color=Qt::yellow; pose.bodyToView={0,0,1,1,0,0,0,1,0};
      const auto geometry=orbitObjectGuides(curve,pose,10); require(!geometry.lines.empty() && !geometry.triangles.empty(),"Guide geometry missing");
      const osg::Vec3d center(100,200,300);
      for (const auto &edge:geometry.lines) if (edge.color.red()==0 && edge.color.green()==0 && edge.color.blue()==0) {
         require(std::abs((edge.start-center).length()-10)<1e-10 && std::abs((edge.end-center).length()-10)<1e-10,"Latitude/longitude grid is not on the sphere");
         require(std::abs(edge.color.alphaF()-.9)<1e-3,"Grid alpha does not match OF");
      }
      require((geometry.triangles.front().points[1]-osg::Vec3d(100,350,300)).length()<1e-10,"Plane did not use object attitude and 15-radius extent");
      for (const auto &triangle:geometry.triangles) {
         require(std::abs(triangle.points[1].x()-100)<1e-10 && std::abs((triangle.points[1]-center).length()-150)<1e-10,"Local XY plane used inertial coordinates or wrong radius");
         require(std::abs(triangle.color.alphaF()-.2)<1e-3,"Plane fill alpha does not match OF");
      }
      auto invalidPose=pose; invalidPose.bodyToView[0]=std::numeric_limits<double>::quiet_NaN();
      const auto invalidGeometry=orbitObjectGuides(curve,invalidPose,10);
      require(invalidGeometry.lines.empty() && invalidGeometry.triangles.empty(),"Invalid latest attitude published guide vertices");
      invalidPose=pose; invalidPose.x=std::numeric_limits<double>::infinity();
      require(orbitObjectGuides(curve,invalidPose,10).lines.empty(),"Invalid guide position was accepted");
      require(orbitObjectGuideRadius(curve,25)==10,"Hidden model changed guide radius"); curve.radius=0;
      require(orbitObjectGuideRadius(curve,25)==100,"Plain frame guide radius is not OF's 100 km"); curve.showObject=true;
      require(orbitObjectGuideRadius(curve,25)==25,"Visible model bound is not used for guide radius");
      auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit); model->axes=false; model->grid=false; model->legend=false; model->labels=false;
      model->scriptedCamera=true; model->perspective=true; model->fieldOfView=45; model->cameras.push_back({0,{0,-1000,600},{0,0,0},{0,0,1},false});
      auto &body=model->curves[0]; body.name="Probe"; body.color=Qt::yellow; body.lines=false; body.showObject=false; body.radius=50;
      model->append(0,0,0,0); const auto first=model->frame;
      PlotCanvas canvas(model); canvas.resize(640,480); canvas.show(); canvas.setFrame(first); QApplication::processEvents(); const auto blank=canvas.captureImage();
      body.objectGrid=true; canvas.refresh(); const auto grid=canvas.captureImage(); require(grid!=blank,"Grid-only hidden-object fallback was blank");
      body.objectGrid=false; body.objectXYPlane=true; canvas.refresh(); const auto plane=canvas.captureImage(); require(plane!=blank && plane!=grid,"Filled local XY plane fallback was missing");
      ++model->frame; model->append(0,100,200,0); body.points.back().bodyToView=pose.bodyToView;
      canvas.setFrame(first); require(canvas.captureImage()==plane,"Guide replay used future position or attitude");
      canvas.setFrame(model->frame); require(canvas.captureImage()!=plane,"Guide replay did not follow retained position/attitude");
      body.visible=false; canvas.refresh(); require(canvas.captureImage()==blank,"Hidden curve retained guide pixels"); body.visible=true;
      ++model->frame; model->append(0,0,-4000,2400); canvas.setFrame(model->frame); require(canvas.captureImage()==blank,"Guide behind camera reused earlier pose");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Guide UI initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString source="% preserve unrelated mission source α\nCreate Spacecraft Sat;\nCreate OrbitView Display;\nGMAT Display.Add = {Sat, Earth};\nGMAT Display.Grid = On;\nGMAT Display.XYPlane = On;\nBeginMissionSequence;\n";
      editor->setPlainText(source); require(window.buildScript(),"Guide UI fixture failed");
      auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems("Display",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Guide resource missing");
      tree->itemDoubleClicked(items.front(),0); auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Guide resource panel missing");
      auto dialog=[&](bool change,bool accept) {
         std::exception_ptr failure;
         QTimer::singleShot(0,panel,[&] {
            auto *view=panel->findChild<QDialog *>("orbitObjectDrawingDialog");
            try {
               require(view,"Guide dialog missing"); auto *grid=view->findChild<QComboBox *>("orbitDrawing_grid_Sat"),*plane=view->findChild<QComboBox *>("orbitDrawing_xyPlane_Earth");
               require(grid && plane,"Body grid/plane controls missing"); if (change) { grid->setCurrentText("On"); plane->setCurrentText("On"); }
               view->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
            } catch (...) { failure=std::current_exception(); if (view) view->reject(); }
         }); panel->findChild<QPushButton *>("editOrbitDrawing")->click(); if (failure) std::rethrow_exception(failure);
      };
      dialog(true,false); require(!panel->hasChanges() && editor->toPlainText()==source,"Guide Cancel changed source");
      dialog(false,true); require(!panel->hasChanges(),"Untouched guide controls made implicit defaults explicit");
      dialog(true,true); require(panel->hasChanges() && editor->toPlainText()==source,"Guide controls did not stay pending");
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto edited=editor->toPlainText(); const auto applied=qtCameraSettings(edited).value("Display");
      require(applied.objectGrids.value("Sat") && applied.objectXYPlanes.value("Earth") && removeQtCameraSetting(edited,"Display")==source,"Guide Apply changed unrelated source or global options");
      require(child->isVisible() && !dynamic_cast<ResourceEditor *>(child->widget())->hasChanges(),"Guide Apply did not retain clean panel");
      editor->undo(); require(editor->toPlainText()==source,"Guide Undo not exact"); editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Guide Redo/build not exact");
      require(!window.applyResourceChanges("Display",{{"@QtObjectGrids","{\"Missing\":true}"}},edited).isEmpty() && editor->toPlainText()==edited,"Invalid guide membership did not roll back");
      require(window.saveScriptTo(files.filePath("guides-α.script")) && window.loadScript(files.filePath("guides-α.script")) && window.buildScript() && editor->toPlainText()==edited,"Guide Unicode round trip failed");
      require(window.renameResource("Sat","GuideSat",edited).isEmpty(),"Guide resource Rename failed");
      require(qtCameraSettings(editor->toPlainText()).value("Display").objectGrids==QMap<QString,bool>{{"GuideSat",true}},"Rename lost named guide reference");
      editor->undo(); require(editor->toPlainText()==edited && window.buildScript(),"Guide Rename Undo not exact");
      require(window.applyResourceChanges("Display",{{"Add","Earth"}},edited).isEmpty() && !qtCameraSettings(editor->toPlainText()).value("Display").objectGrids.contains("Sat") && qtCameraSettings(editor->toPlainText()).value("Display").objectXYPlanes.value("Earth"),"Removed object kept stale guides or pruned other body's plane");
      std::cout<<"PASS per-object grid/XY plane conversion and source-ordered defaults, precise rejection, independent spherical/body-space/fill geometry, fallback visibility/replay/clipping, actual pending controls/Cancel/retained Apply, exact source/Undo/Redo/Unicode reopen/rename/pruning; no numerical mission or native desktop test.\n";
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
