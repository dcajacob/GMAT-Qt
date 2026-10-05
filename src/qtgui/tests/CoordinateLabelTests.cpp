#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "ResourceEditor.hpp"
#include "PlotWidget.hpp"
#include "OrbitRenderer.hpp"
#include "OrbitCoordinateLabels.hpp"
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
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read fixture report"); return file.readAll(); }
static void preserve(const QString &name,const QByteArray &data) {
   const auto path=qEnvironmentVariable("GMAT_QT_COORDINATE_LABEL_EVIDENCE"); if (path.isEmpty()) return;
   QDir().mkpath(path); QFile file(path+"/"+name); require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"Cannot preserve evidence");
}
static int pixels(const QImage &a,const QImage &b) {
   require(!a.isNull() && a.size()==b.size(),"Incompatible captures"); int n=0;
   for (int y=0;y<a.height();++y) for (int x=0;x<a.width();++x) n+=a.pixel(x,y)!=b.pixel(x,y);
   return n;
}
static void render(QApplication &app,bool native) {
   auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit); model->grid=false; model->legend=false; model->labels=false;
   model->coordinates="EarthMJ2000Eq"; model->scriptedCamera=true;
   model->cameras.push_back({0,{24000,-26000,21000},{0,0,0},{0,0,1},false});
   auto &curve=model->curves[0]; curve.name="Sat"; curve.lines=true; curve.showObject=false; model->append(0,7000,0,0);
   auto labels=orbitCoordinateLabels(*model,7000); require(labels.size()==3 && labels[0].text=="X" && labels[1].pose.y==7000 && labels[2].pose.z==7000,"Default XYZ geometry wrong");
   model->frameLabel=true; require(orbitCoordinateLabels(*model,7000).size()==4,"Object label master hid coordinate labels");
   model->axes=false; labels=orbitCoordinateLabels(*model,7000); require(labels.size()==1 && labels[0].text==model->coordinates && labels[0].pose.x==0 && labels[0].pose.y==0 && labels[0].pose.z==0,"Frame label needs independent origin placement");
   model->coordinates.clear(); require(orbitCoordinateLabels(*model,7000).isEmpty(),"Empty coordinate name created text"); model->coordinates="EarthMJ2000Eq";
   model->axes=true; model->frameLabel=false; PlotWidget widget(model); widget.resize(720,540); widget.show(); app.processEvents();
   if (native) {
      QEventLoop loop; QTimer::singleShot(150,&loop,&QEventLoop::quit); loop.exec();
      auto *renderer=dynamic_cast<OrbitRenderer *>(widget.findChild<QOpenGLWidget *>()); require(renderer && renderer->isValid(),"Native software renderer unavailable");
   }
   auto capture=[&] { widget.canvas()->refresh(); return widget.canvas()->captureImage(); };
   const auto axesText=capture(); model->axesLabels=false; const auto lines=capture(); require(pixels(axesText,lines)>30,"XYZ labels did not independently render");
   model->axes=false; const auto blank=capture(); require(pixels(lines,blank)>100,"Axes label switch removed axis lines");
   model->frameLabel=true; const auto frame=capture(); require(pixels(frame,blank)>100,"Frame name absent with axes and object labels Off");
   model->labels=true; require(capture()==frame,"Coordinate text incorrectly depends on object label master");
   model->labels=false; model->cameras[0].eye={0,0,20000}; model->cameras[0].target={0,0,30000}; model->perspective=true;
   const auto behind=capture(); model->frameLabel=false; require(capture()==behind,"Behind-camera origin text was not clipped");
   const auto folder=qEnvironmentVariable("GMAT_QT_COORDINATE_LABEL_EVIDENCE"); if (!folder.isEmpty()) {
      QDir().mkpath(folder); const auto prefix=folder+(native ? "/native" : "/fallback");
      require(axesText.save(prefix+"-axes.png") && lines.save(prefix+"-lines.png") && frame.save(prefix+"-frame.png"),"Coordinate captures failed");
   }
   require(curve.points.size()==1 && curve.points.front().x==7000,"Coordinate rendering mutated raw samples");
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("CoordinateLabels");
   const bool native=argc==2 && QString::fromLocal8Bit(argv[1])=="--native";
   if (argc!=2 || QGuiApplication::platformName()!=(native ? "xcb" : "offscreen")) return 2;
   try {
      TestSettings settings; render(app,native);
      if (native) { std::cout<<"PASS private native XYZ/frame pixels, independent lines/master and clipping.\n"; return 0; }
      const QString legacy="Create OpenFramesInterface Display;\nDisplay.Add = {Sat, Earth};\nDisplay.AxesLabels = Off;\nDisplay.FrameLabel = On;\nDisplay.Add = {Earth, Sat};\n";
      const auto converted=convertOpenFramesViews(legacy); require(converted.error.isEmpty(),qPrintable(converted.error));
      const auto imported=qtCameraSettings(converted.script).value("Display"); require(imported.axesLabels==false && imported.frameLabel==true,"Conversion/Add reset lost frame flags");
      const auto override=convertOpenFramesViews(legacy+"Display.AxesLabels = On;\nDisplay.FrameLabel = Off;\n");
      require(qtCameraSettings(override.script).value("Display").axesLabels==true && qtCameraSettings(override.script).value("Display").frameLabel==false,"Frame setting source order lost");
      const auto bad=legacy+"Display.FrameLabel = maybe;\n"; require(!convertOpenFramesViews(bad).error.isEmpty() && convertOpenFramesViews(bad).script==bad,"Invalid conversion changed source");
      auto malformed=qtCameraDirective("Display",imported); malformed.replace("\"frameLabel\":true","\"frameLabel\":1"); bool rejected=false;
      try { qtCameraSettings(malformed); } catch (const std::exception &) { rejected=true; } require(rejected,"Nonboolean frame metadata accepted");
      QTemporaryDir files; require(files.isValid(),"Fixture directory unavailable"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Initialization failed"); window.show(); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("states.txt");
      const QString resources="% preserve authored source α\nCreate Spacecraft Sat;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.WriteReport = false;\nValues.Precision = 17;\n";
      const QString mission="BeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 10};\nReport Values Sat.A1ModJulian Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z;\n";
      editor->setPlainText(resources+mission); require(window.runMission()==MainWindow::RunResult::Completed,"Independent reference failed"); const auto reference=read(report); preserve("reference.report",reference);
      const QString source=resources+"Create OrbitView Display;\nDisplay.Add = {Sat, Earth};\nDisplay.CoordinateSystem = EarthMJ2000Eq;\nDisplay.Axes = Off;\n"+qtCameraDirective("Display",imported)+"\n"+mission;
      editor->setPlainText(source); require(window.runMission()==MainWindow::RunResult::Completed && read(report)==reference && editor->toPlainText()==source,"Coordinate labels changed complete report/source"); preserve("labels.report",read(report)); preserve("labels.script",source.toUtf8());
      const auto model=window.plotReceiver()->model("Display"); require(model && !model->axes && !model->axesLabels && model->frameLabel && model->coordinates=="EarthMJ2000Eq","Runtime frame settings absent");
      auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace");
      auto open=[&] { const auto items=tree->findItems("Display",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Display missing"); tree->itemDoubleClicked(items.front(),0); auto *panel=dynamic_cast<ResourceEditor *>(area->activeSubWindow()->widget()); require(panel,"Resource form missing"); return panel; };
      auto *panel=open();
      auto choose=[&](bool accept,const QString &axes,const QString &frame) {
         std::exception_ptr failure; QTimer::singleShot(0,panel,[&] {
            auto *dialog=panel->findChild<QDialog *>("orbitObjectDrawingDialog");
            try {
               require(dialog,"Drawing controls missing"); auto *tabs=dialog->findChild<QTabWidget *>("orbitDrawingTabs"); tabs->setCurrentIndex(tabs->count()-1);
               auto *a=dialog->findChild<QComboBox *>("orbitDrawing_axesLabels"),*f=dialog->findChild<QComboBox *>("orbitDrawing_frameLabel");
               require(a && f && a->currentText()=="Off" && f->currentText()=="On","Imported coordinate flags did not reopen"); a->setCurrentText(axes); f->setCurrentText(frame);
               dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
            } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
         }); panel->findChild<QPushButton *>("editOrbitDrawing")->click(); if (failure) std::rethrow_exception(failure);
      };
      choose(false,"On","Off"); require(!panel->hasChanges() && editor->toPlainText()==source,"Cancel changed frame flags");
      choose(true,"On","Off"); require(panel->hasChanges() && editor->toPlainText()==source,"Frame flags did not remain pending"); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto edited=editor->toPlainText(); const auto flags=qtCameraSettings(edited).value("Display"); require(flags.axesLabels==true && flags.frameLabel==false && removeQtCameraSetting(edited,"Display")==removeQtCameraSetting(source,"Display"),"Apply lost flags or unrelated source");
      editor->undo(); require(editor->toPlainText()==source,"Undo not exact"); editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Redo failed");
      require(!window.applyResourceChanges("Display",{{"@QtAxesLabels","invalid"}},edited).isEmpty() && editor->toPlainText()==edited,"Invalid pending flag did not roll back");
      const auto saved=files.filePath("frame labels α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==edited,"Unicode Save/reopen failed");
      require(window.applyResourceChanges("Display",{{"@QtAxesLabels","Default"},{"@QtFrameLabel","Default"}},edited).isEmpty(),"Default flags Apply failed");
      const auto defaults=qtCameraSettings(editor->toPlainText()).value("Display"); require(!defaults.axesLabels && !defaults.frameLabel,"Default materialized coordinate overrides"); editor->undo(); require(editor->toPlainText()==edited && window.buildScript(),"Default Undo lost source");
      require(window.renameResource("Display","RenamedDisplay",edited).isEmpty() && qtCameraSettings(editor->toPlainText()).value("RenamedDisplay").axesLabels==true,"Plot Rename lost coordinate flags"); editor->undo(); require(editor->toPlainText()==edited,"Rename Undo lost source");
      std::cout<<"PASS coordinate-label conversion/order/defaults/invalid metadata, native-independent fallback pixels, pending Cancel/Apply/source rollback/Undo/Redo/Unicode reopen/Rename and identical complete report.\n";
   } catch (BaseException &e) { std::cerr<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
   return 0;
}
