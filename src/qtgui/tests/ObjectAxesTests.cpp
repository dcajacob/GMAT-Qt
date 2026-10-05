#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ScriptCompatibility.hpp"
#include "OrbitObjectAxes.hpp"
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
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("ObjectAxes");
   if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   try {
      TestSettings isolated; QTemporaryDir files;
      const QString legacy="Create OpenFramesInterface Display;\nGMAT Display.Add = {Sat, Earth};\nGMAT Display.DrawAxes = [true];\n";
      const auto converted=convertOpenFramesViews(legacy);
      require(converted.error.isEmpty(),"Axes conversion failed");
      const auto setting=qtCameraSettings(converted.script).value("Display");
      require(setting.objectAxes.value("Sat") && !setting.objectAxes.value("Earth",true),"Axes prefix/default values lost");
      require(qtCameraSettings(qtCameraDirective("Display",setting)).value("Display").objectAxes==setting.objectAxes,"Axes serialization lost named flags");
      const auto reset=convertOpenFramesViews(legacy+"GMAT Display.Add = {Earth, Sat};\nGMAT Display.DrawAxes = [false true true];\n");
      require(reset.error.isEmpty() && qtCameraSettings(reset.script).value("Display").objectAxes==QMap<QString,bool>{{"Sat",false},{"Earth",true}},"Repeated scripted Add changed axes order/excess-prefix behavior");
      const auto bad=QString(legacy).replace("[true]","[maybe]"); const auto rejected=convertOpenFramesViews(bad);
      require(!rejected.error.isEmpty() && rejected.script==bad,"Invalid axes conversion changed source");
      bool invalid=false; try { qtCameraSettings("% GMAT-Qt-Camera {\"plot\":\"Display\",\"perspective\":false,\"fieldOfView\":50,\"objectAxes\":{\"Sat\":1}}\n"); } catch (std::exception &) { invalid=true; }
      require(invalid,"Non-boolean axes metadata accepted");
      PlotPoint pose; pose.x=100; pose.y=200; pose.z=300; pose.bodyToView={0,-1,0,1,0,0,0,0,1};
      const auto geometry=orbitObjectAxes(pose,10);
      require((geometry[0].start-osg::Vec3d(100,210,300)).length()<1e-12 && (geometry[0].end-osg::Vec3d(100,220,300)).length()<1e-12,"Axes did not use body rotation/radius offset");
      auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit); model->axes=false; model->grid=false; model->legend=false; model->labels=false;
      model->scriptedCamera=true; model->perspective=true; model->fieldOfView=45; model->cameras.push_back({0,{0,-1000,0},{0,0,0},{0,0,1},false});
      auto &curve=model->curves[0]; curve.name="Probe"; curve.color=Qt::yellow; curve.lines=false; curve.showObject=false; curve.objectAxes=true; curve.radius=100;
      model->append(0,0,0,0); const auto first=model->frame; ++model->frame; model->append(0,100,0,0); curve.points.back().bodyToView=pose.bodyToView;
      require(orbitObjectPose(curve,first)==&curve.points.front(),"Axes replay used latest pose");
      PlotCanvas canvas(model); canvas.resize(640,480); canvas.show(); canvas.setFrame(first); QApplication::processEvents(); const auto initial=canvas.captureImage();
      curve.objectAxes=false; canvas.refresh(); const auto blank=canvas.captureImage(); require(initial!=blank,"Axes-only hidden-model fallback was blank");
      curve.objectAxes=true; canvas.setFrame(model->frame); require(canvas.captureImage()!=initial,"Axes replay failed to follow position/attitude");
      curve.visible=false; canvas.refresh(); require(canvas.captureImage()==blank,"Live visibility did not hide axes");
      curve.visible=true; ++model->frame; model->append(0,0,-1200,0); canvas.setFrame(model->frame); require(canvas.captureImage()==blank,"Axes behind the camera reused an earlier pose");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Axes UI initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString source="% keep source α\nCreate Spacecraft Sat;\nCreate OrbitView Display;\nGMAT Display.Add = {Sat, Earth};\nBeginMissionSequence;\n";
      editor->setPlainText(source); require(window.buildScript(),"Axes UI fixture failed");
      auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems("Display",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Axes resource missing");
      tree->itemDoubleClicked(items.front(),0); auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Axes resource panel missing");
      auto dialog=[&](bool change,bool accept) {
         std::exception_ptr failure;
         QTimer::singleShot(0,panel,[&] {
            auto *view=panel->findChild<QDialog *>("orbitObjectDrawingDialog");
            try { require(view,"Axes dialog missing"); auto *choice=view->findChild<QComboBox *>("orbitDrawing_axes_Sat"); require(choice,"Body axes control missing"); if (change) choice->setCurrentText("On"); view->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
            catch (...) { failure=std::current_exception(); if (view) view->reject(); }
         }); panel->findChild<QPushButton *>("editOrbitDrawing")->click(); if (failure) std::rethrow_exception(failure);
      };
      dialog(true,false); require(!panel->hasChanges() && editor->toPlainText()==source,"Axes Cancel changed source");
      dialog(false,true); require(!panel->hasChanges(),"Untouched guides made defaults explicit");
      dialog(true,true); require(panel->hasChanges() && editor->toPlainText()==source,"Axes were not pending until Apply");
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto edited=editor->toPlainText(); require(qtCameraSettings(edited).value("Display").objectAxes.value("Sat") && removeQtCameraSetting(edited,"Display")==source,"Axes Apply changed unrelated mission source");
      require(child->isVisible() && !dynamic_cast<ResourceEditor *>(child->widget())->hasChanges(),"Axes Apply did not retain clean panel");
      editor->undo(); require(editor->toPlainText()==source,"Axes Undo not exact"); editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Axes Redo/build not exact");
      require(!window.applyResourceChanges("Display",{{"@QtObjectAxes","{\"Missing\":true}"}},edited).isEmpty() && editor->toPlainText()==edited,"Invalid axes member did not roll back");
      require(window.saveScriptTo(files.filePath("axes-α.script")) && window.loadScript(files.filePath("axes-α.script")) && window.buildScript() && editor->toPlainText()==edited,"Axes Unicode round trip failed");
      require(window.applyResourceChanges("Display",{{"Add","Earth"}},edited).isEmpty() && !qtCameraSettings(editor->toPlainText()).value("Display").objectAxes.contains("Sat"),"Removed object kept stale axes");
      std::cout<<"PASS per-object axes conversion/defaults/scripted Add retention, typed maps, retained pose/radius/rotation, fallback visibility/replay/clipping, actual pending controls/Cancel/retained Apply, exact source/Undo/Redo/Unicode reopen and named pruning; no numerical mission or native desktop test.\n";
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
