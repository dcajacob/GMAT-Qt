#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "OrbitCamera.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <iomanip>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Segment-camera report missing"); return file.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Segment-camera evidence write failed"); }
static osg::Vec3d position(const PlotPoint &point) { return {point.x,point.y,point.z}; }
static osg::Vec3d body(const PlotPoint &point,const osg::Vec3d &vector) { const auto &r=point.bodyToView; return {r[0]*vector.x()+r[1]*vector.y()+r[2]*vector.z(),r[3]*vector.x()+r[4]*vector.y()+r[5]*vector.z(),r[6]*vector.x()+r[7]*vector.y()+r[8]*vector.z()}; }
static osg::Vec3d xyz(const std::array<double,3> &value) { return {value[0],value[1],value[2]}; }
static void syntaxTests(const QString &startup)
{
   const QString prefix="% preserve physics α\nCreate Variable Value;\nValue = 3;\n";
   const QString mission="BeginMissionSequence;\nValue = Value + 2; % retain calculation\n";
   const QString declarations="Create OpenFramesView First Close Far; % preserve grouped views\n"
      "First.ViewFrame = CoordinateSystem;\nFirst.SetDefaultLocation = On;\nFirst.DefaultEye = [1 2 3];\n"
      "Close.ViewFrame = CoordinateSystem;\nFar.ViewFrame = CoordinateSystem;\n"
      "Create OpenFramesInterface Display Other; % preserve grouped displays\n"
      "Display.Add = {Earth};\nOther.Add = {Earth};\nOther.View = First;\n";
   const auto input=prefix+declarations+"Display.View = {First, , Close, Far}; % empty entry accepted by OF\n"+mission;
   const auto converted=convertOpenFramesViews(input);
   require(converted.error.isEmpty(),qPrintable(converted.error));
   const auto settings=qtCameraSettings(converted.script);
   require(converted.plots==2 && settings.size()==2 && settings.value("Display").primaryName=="First" && settings.value("Display").views.size()==2 && settings.value("Display").views[0].name=="Close" && settings.value("Display").views[1].name=="Far","Grouped/blank-entry conversion lost displays or ordered cameras");
   require(settings.value("Other").primaryName=="First" && settings.value("Other").views.isEmpty(),"Scalar camera selection lost its configured view");
   require(converted.script.contains(prefix) && converted.script.endsWith(mission) && converted.script.contains("Create OrbitView Display Other;") && converted.script.contains("% Qt conversion: Create OpenFramesInterface Display Other; % preserve grouped displays") && converted.script.contains("% Qt conversion: Create OpenFramesView First Close Far; % preserve grouped views") && converted.script.contains("% empty entry accepted by OF"),"Camera syntax conversion changed physics/source comments or grouped object identity");
   auto ordered=prefix+declarations+"Display.View = Far;\nDisplay.View = Close;\nDisplay.View = {First, , Close, First,};\nDisplay.View = Far;\nDisplay.View = Close;\n"+mission;
   const auto combined=convertOpenFramesViews(ordered); require(combined.error.isEmpty(),qPrintable(combined.error));
   const auto selection=qtCameraSettings(combined.script).value("Display");
   require(selection.primaryName=="Far" && selection.views.size()==2 && selection.views[0].name=="Close" && selection.views[1].name=="First","Script array/scalar append/dedup semantics differ from OF AddView");
   const auto cleared=convertOpenFramesViews(prefix+declarations+"Display.View = First;\nDisplay.View = {};\n"+mission);
   require(cleared.error.isEmpty() && qtCameraSettings(cleared.script).value("Display").primaryName=="First","Empty script View changed the existing camera selection");
   const auto noSemicolons=convertOpenFramesViews(QString(input).replace("Create OpenFramesView First Close Far;","Create OpenFramesView First Close Far").replace("Create OpenFramesInterface Display Other;","Create OpenFramesInterface Display Other"));
   require(noSemicolons.error.isEmpty() && noSemicolons.plots==2,"Optional declaration semicolon changed accepted grouped syntax");
   const auto repeatedInput=prefix+QString(declarations).replace(";","; ;")+"Display.View = {First, , Close, Far};; % duplicate terminators\n"+mission;
   const auto repeated=convertOpenFramesViews(repeatedInput); require(repeated.error.isEmpty(),qPrintable(repeated.error));
   const auto repeatedSettings=qtCameraSettings(repeated.script);
   require(repeated.plots==2 && repeatedSettings.value("Display").primaryName=="First" && repeatedSettings.value("Display").views.size()==2 && repeatedSettings.value("Other").primaryName=="First","Trailing empty statements changed grouped/view metadata");
   require(repeated.script.contains("Display.Add = {Earth}; ;\n") && repeated.script.contains("% Qt conversion: First.SetDefaultLocation = On; ;\n") && repeated.script.contains("% duplicate terminators") && repeated.script.contains(prefix) && repeated.script.endsWith(mission),"Empty terminator parsing changed original statements/comments or mission source");
   for (const auto &bad:QStringList{QString(input).replace("Display.Add = {Earth};","Display.Add = {Earth};; Value = 4;"),QString(input).replace("Display.View = {First, , Close, Far};","Display.View = 'First;';;"),QString(input).replace(mission,"BeginMissionSequence;\nDisplay.View = First;;\nValue = Value + 2;\n")}) {
      const auto rejected=convertOpenFramesViews(bad); require(!rejected.error.isEmpty() && rejected.script==bad,"Empty terminator support stripped a quoted/compound or dynamic viewer statement");
   }
   for (const auto &bad:QStringList{QString(input).replace("First Close Far;","First First;"),QString(input).replace("First Close Far;","First, Close;"),QString(input).replace("{First, , Close, Far}","{First, Missing}"),QString(input).replace("{First, , Close, Far}","[First]")}) {
      const auto rejected=convertOpenFramesViews(bad); require(!rejected.error.isEmpty() && rejected.script==bad,"Invalid grouped/view syntax silently changed original source");
   }
   const auto spaced=convertOpenFramesViews(QString(input).replace("{First, , Close, Far}","{First Close}"));
   require(spaced.error.isEmpty() && qtCameraSettings(spaced.script).value("Display").primaryName=="First" && qtCameraSettings(spaced.script).value("Display").views.size()==1 && qtCameraSettings(spaced.script).value("Display").views.front().name=="Close","Interpreter whitespace-separated view list was rejected or lost its order");
   const auto samples=QDir(QFileInfo(startup).absolutePath()).absoluteFilePath("../samples");
   const auto geo=QString::fromUtf8(read(QDir(samples).filePath("Ex_GEOTransfer.script")));
   const auto geoConverted=convertOpenFramesViews(geo); require(geoConverted.error.isEmpty(),qPrintable(geoConverted.error));
   const auto geoCameras=qtCameraSettings(geoConverted.script);
   require(geoConverted.plots==2 && geoCameras.value("OFI_inertialView").primaryName=="inertial_View" && geoCameras.value("OFI_inertialView").views.size()==2 && geoCameras.value("OFI_fixedView").views.size()==2 && geoConverted.script.endsWith(geo.mid(geo.indexOf("BeginMissionSequence;"))),"Actual GEOTransfer conversion changed ordered views or mission source");
   const auto flyby=QString::fromUtf8(read(QDir(samples).filePath("OptimalControl/Ex_IntegratedFlyby_MarsFlyby.script")));
   const auto flybyConverted=convertOpenFramesViews(flyby); require(flybyConverted.error.isEmpty(),qPrintable(flybyConverted.error));
   const auto flybyCameras=qtCameraSettings(flybyConverted.script);
   const QString flybyAdd="OFI_OrbitView1.Add              = {emsat, Earth, Mars};;";
   require(flyby.contains(flybyAdd) && flybyConverted.script.contains(flybyAdd) && flybyConverted.plots==2 && flybyCameras.value("OFI_OrbitView1").primaryName=="Orbit_View" && flybyCameras.value("OFI_OrbitView2").primaryName=="Orbit_View" && flybyConverted.script.endsWith(flyby.mid(flyby.indexOf("BeginMissionSequence;"))),"Actual IntegratedFlyby conversion rejected empty terminator or changed Add/cameras/mission source");
   std::cout<<"PASS grouped OF display/view identities, scalar/list append, empty no-op and duplicate entries, trailing empty statements with original lines/comments and compound/dynamic rejection, actual GEOTransfer and IntegratedFlyby static conversion. Required vector sample conversion is covered by OpenFramesVectors. No engine initialization, mission, pixels or native desktop test.\n";

}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtSegmentCameras");
   try {
      if (argc==3 && QString::fromLocal8Bit(argv[2])=="--syntax") {
         require(QApplication::platformName()=="offscreen","OF syntax mode requires offscreen"); syntaxTests(QFileInfo(argv[1]).absoluteFilePath()); return 0;
      }
      TestSettings settings; QTemporaryDir files; require((argc==2 || argc==3) && files.isValid(),"Segment-camera setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath()); const auto report=files.filePath("segment state Δ.txt"),saved=files.filePath("segment cameras Δ.script");
      const QString resources="% preserve segment calculations α\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.54605329010754;\nSat.VZ = 0;\nSat.Attitude = Spinner;\nSat.AttitudeDisplayStateType = EulerAngles;\nSat.EulerAngle1 = 30;\nSat.EulerAngle2 = 20;\nSat.EulerAngle3 = 10;\nSat.AttitudeRateDisplayStateType = AngularVelocity;\nSat.AngularVelocityX = 0;\nSat.AngularVelocityY = 0;\nSat.AngularVelocityZ = 0.2;\nCreate ForceModel Forces;\nForces.PrimaryBodies = {};\nForces.PointMasses = {Earth};\nForces.Drag = None;\nForces.SRP = Off;\nCreate Propagator Prop;\nProp.FM = Forces;\nProp.InitialStepSize = 10;\nProp.MaxStep = 10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 17;\n";
      const QString state="Report Values Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Sat.A1ModJulian Sat.DCM11 Sat.DCM12 Sat.DCM13 Sat.DCM21 Sat.DCM22 Sat.DCM23 Sat.DCM31 Sat.DCM32 Sat.DCM33;\n";
      // A stop on an exact 10-second step skips TakeFinalStep's flush. Use
      // non-step endpoints so these are two actual OF regular arcs, rather
      // than inferring an arc boundary from a changed command summary name.
      const QString mission="BeginMissionSequence;\nPropagate 'FirstArc' Prop(Sat) {Sat.ElapsedSecs = 61}; % retain first named command\n"+state+"Propagate 'SecondArc' Prop(Sat) {Sat.ElapsedSecs = 119}; % retain second named command\n"+state;
      const QString views="Create OpenFramesView Stored AutoFirst AlignedLate AutoLate;\nStored.ViewFrame = Sat.FirstArc;\nStored.ViewTrajectory = On;\nStored.InertialFrame = Off;\nStored.SetDefaultLocation = On;\nStored.DefaultEye = [0 -10000 1000];\nStored.DefaultCenter = [1 2 3];\nStored.DefaultUp = [1 0 1];\nStored.FOVy = 60;\nAutoFirst.ViewFrame = Sat.FirstArc;\nAutoFirst.ViewTrajectory = On;\nAutoFirst.InertialFrame = Off;\nAlignedLate.ViewFrame = Sat.SecondArc;\nAlignedLate.ViewTrajectory = Off;\nAlignedLate.InertialFrame = On;\nAlignedLate.LookAtFrame = Earth;\nAlignedLate.ShortestAngle = On;\nAlignedLate.SetCurrentLocation = On;\nAlignedLate.CurrentEye = [0 -12000 0];\nAlignedLate.CurrentUp = [0 0 1];\nAutoLate.ViewFrame = Sat.SecondArc;\nAutoLate.ViewTrajectory = Off;\nAutoLate.InertialFrame = On;\nCreate OpenFramesInterface Display;\nDisplay.Add = {Sat, Earth};\nDisplay.View = {Stored, AutoFirst, AlignedLate, AutoLate};\nDisplay.CoordinateSystem = EarthMJ2000Eq;\nDisplay.NumPointsToRedraw = 1;\n";
      // Converter accepts one declaration per line; grouped resources below remain engine syntax.
      const auto legacy=resources+QString(views).replace("Create OpenFramesView Stored AutoFirst AlignedLate AutoLate;","Create OpenFramesView Stored;\nCreate OpenFramesView AutoFirst;\nCreate OpenFramesView AlignedLate;\nCreate OpenFramesView AutoLate;")+mission;
      const auto converted=convertOpenFramesViews(legacy); if (!converted.error.isEmpty()) std::cerr<<converted.error.toStdString()<<'\n'; require(converted.error.isEmpty(),"Named propagation-segment camera conversion rejected"); require(converted.script.endsWith(mission),"Segment conversion changed mission calculations/comments");
      MainWindow window; window.show(); require(window.initialize(startup),"Segment-camera runtime initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); editor->setPlainText(resources+mission); require(window.runMission()==MainWindow::RunResult::Completed,"Independent segment mission failed"); const auto expected=read(report); require(QString::fromUtf8(expected).trimmed().split('\n').size()==2,"Independent complete report missing rows");
      editor->setPlainText(legacy); bool offered=false; QTimer::singleShot(0,[&] { auto *prompt=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); if (prompt && prompt->objectName()=="openFramesConversionPrompt") { offered=true; prompt->done(QMessageBox::Yes); } else if (prompt) prompt->reject(); }); require(window.buildScript() && offered,"Actual segment Build did not automatically offer conversion"); const auto source=editor->toPlainText(); require(source==converted.script,"Actual conversion differed from source conversion"); editor->undo(); require(editor->toPlainText()==legacy,"Segment conversion Undo lost original OF source"); editor->redo(); require(editor->toPlainText()==source && window.saveScriptTo(saved) && window.loadScript(saved),"Segment conversion Redo/Unicode save/reopen failed"); require(window.runMission()==MainWindow::RunResult::Completed && read(report)==expected,"Segment camera display changed complete numerical report");
      auto model=window.plotReceiver()->model("Display"); require(model && model->cameraViews.size()==4,"Segment named camera views missing"); const PlotCurve *curve=nullptr; for (const auto &item:model->curves) if (item.name=="Sat") curve=&item; require(curve && !curve->points.empty(),"Segment spacecraft history missing"); const PlotPoint *first=nullptr,*last=nullptr,*secondBegin=nullptr; for (const auto &point:curve->points) { if (point.provider=="FirstArc") first=&point; if (point.provider=="SecondArc") { if (!secondBegin) secondBegin=&point; last=&point; } } require(first && last && secondBegin && (position(*last)-position(*first)).length()>500,"Named propagation segments missing or indistinguishable");
      const auto arcs=model->regularArcs.value("Sat");
      require(arcs.size()==2 && arcs[0].name=="FirstArc" && arcs[1].name=="SecondArc" &&
         arcs[0].providers.size()==1 && arcs[1].providers.size()==1 && arcs[0].providers!=arcs[1].providers &&
         arcs[0].finalized && arcs[1].finalized,"Fixture did not create two finalized provider-distinct OF regular arcs");
      const auto reportRows=QString::fromUtf8(expected).trimmed().split('\n');
      for (int index=0;index<2;++index) {
         const auto fields=reportRows[index].simplified().split(' '); require(fields.size()==17,"Independent endpoint report columns missing");
         std::array<double,17> row{};
         for (int column=0;column<17;++column) { bool valid=false; row[column]=fields[column].toDouble(&valid); require(valid,"Independent endpoint report contains invalid numeric data"); }
         const auto &pose=arcs[index].last;
         require(std::abs(pose.epoch-row[7])<1e-12 && (position(pose)-osg::Vec3d(row[1],row[2],row[3])).length()<1e-7,"Regular arc endpoint differs from independent numerical report");
         // GMAT reports inertial-to-body DCM; plot poses store its transpose.
         for (int axis=0;axis<3;++axis) for (int column=0;column<3;++column)
            require(std::abs(pose.bodyToView[3*axis+column]-row[8+3*column+axis])<1e-12,"Regular arc attitude differs from independent numerical report");
         const auto &collected=index==0 ? *first : *last;
         require(collected.epoch==pose.epoch && (position(collected)-position(pose)).length()<1e-7,"Collected segment endpoint differs from finalized regular arc");
      }
      const auto &primary=model->cameras.back();
      const auto eyeError=(xyz(primary.eye)-(position(*first)+body(*first,{0,-10000,1000}))).length();
      const auto targetError=(xyz(primary.target)-(position(*first)+body(*first,{1,2,3}))).length();
      const auto upError=(xyz(primary.up)-body(*first,{1,0,1})).length();
      if (!(eyeError<1e-7 && targetError<1e-7 && upError<1e-10)) {
         std::cerr<<std::setprecision(17)<<"Segment endpoint errors eye="<<eyeError<<" target="<<targetError<<" up="<<upError<<" modelFrame="<<model->frame<<" cameraFrame="<<primary.frame<<'\n';
         const auto dumpPoint=[](const char *label,const PlotPoint &point) {
            std::cerr<<label<<" epoch="<<point.epoch<<" frame="<<point.frame<<" provider="<<point.provider.toStdString()<<" xyz="<<point.x<<','<<point.y<<','<<point.z<<" body=";
            for (const auto value:point.bodyToView) std::cerr<<value<<',';
            std::cerr<<'\n';
         };
         dumpPoint("collected first endpoint",*first); dumpPoint("collected last endpoint",*last);
         for (const auto &arc:arcs) {
            std::cerr<<"regular arc id="<<arc.id<<" name="<<arc.name.toStdString()<<" providers="<<arc.providers.size()<<" finalized="<<arc.finalized<<'\n';
            dumpPoint("arc first",arc.first); dumpPoint("arc last",arc.last);
         }
         std::cerr<<"actual eye="<<primary.eye[0]<<','<<primary.eye[1]<<','<<primary.eye[2]<<" target="<<primary.target[0]<<','<<primary.target[1]<<','<<primary.target[2]<<" up="<<primary.up[0]<<','<<primary.up[1]<<','<<primary.up[2]<<"\nIndependent report:\n"<<expected.constData();
      }
      require(eyeError<1e-7 && targetError<1e-7 && upError<1e-10,"Stored first-segment camera followed later position/attitude or lost pose");
      require((body(*first,{1,0,0})-body(*last,{1,0,0})).length()>0.01,"Nonzero attitude fixture did not distinguish endpoint orientation");
      auto &aligned=model->cameraViews[2].cameras; require(!aligned.empty() && aligned.front().frame>=secondBegin->frame,"Late segment camera acquired before its data existed"); const auto &alignedLast=aligned.back(); const auto toward=-position(*last); const auto forward=xyz(alignedLast.target)-xyz(alignedLast.eye); require((xyz(alignedLast.target)-position(*last)).length()<1e-7 && forward*toward/(forward.length()*toward.length())>1-1e-10,"Segment LookAt camera lost inertial alignment");
      for (int index:{1,3}) { PlotModel viewModel=*model; viewModel.selectedCamera=index; viewModel.perspective=model->cameraViews[index].perspective; viewModel.fieldOfView=model->cameraViews[index].fieldOfView; const auto &point=index==1 ? *first : *last; const auto camera=orbitCamera(viewModel,model->frame,0,0,7000,1.5); const auto half=viewModel.fieldOfView*3.14159265358979323846/360.; require((camera.target-position(point)).length()<1e-7 && std::abs(camera.distance-1/std::sin(half))<1e-9,"Automatic segment camera framed a whole trajectory/model instead of the empty segment frame"); require((camera.up-(index==1 ? body(point,{0,0,1}) : osg::Vec3d(0,0,1))).length()<1e-10,"Automatic segment inertial/body orientation wrong"); }
      require(window.plotReceiver()->show("Display"),"Segment viewer cannot activate"); auto *area=window.findChild<QMdiArea *>("workspace"); auto *plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()); require(plot,"Segment viewer widget missing"); const auto latest=plot->canvas()->captureImage(); require(!latest.isNull(),"Segment scene failed to render"); auto *selector=plot->findChild<QComboBox *>("orbitCameraView"); auto *timeline=plot->findChild<QSlider *>("plotTimeline"); require(selector && timeline,"Segment camera/replay controls missing"); selector->setCurrentIndex(2); const auto lateImage=plot->canvas()->captureImage(); require(lateImage!=latest,"Segment named view switching did not change scene"); selector->setCurrentIndex(0); timeline->setValue(0); require(plot->canvas()->captureImage()!=latest,"Segment Earlier did not change pixels"); timeline->setValue(timeline->maximum()); require(plot->canvas()->captureImage()==latest,"Segment Latest did not restore frozen endpoint camera"); const auto count=curve->points.size(); area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(window.plotReceiver()->show("Display") && window.plotReceiver()->model("Display")==model && model->cameras.back().eye==primary.eye,"Segment close/reopen lost retained history");
      require(area->viewport()->rect().contains(area->activeSubWindow()->geometry()),"Reopened segment viewer controls cascade outside workspace");
      const auto viewerGeometry=area->activeSubWindow()->geometry(); require(window.plotReceiver()->show("Display") && area->activeSubWindow()->geometry()==viewerGeometry,"Activating existing segment viewer moved its geometry");
      if (!capture.isEmpty()) { require(latest.save(capture+".Stored.png") && lateImage.save(capture+".AlignedLate.png"),"Segment scene capture failed"); QEventLoop exposed; QTimer::singleShot(200,&exposed,&QEventLoop::quit); exposed.exec(); require(window.grab().save(capture+".png"),"Segment workspace capture failed"); write(capture+".reference.txt",expected); write(capture+".viewer.txt",read(report)); }
      // Explicit primary edits override imported segment behavior without changing secondary views.
      require(window.applyResourceChanges("Display",{{"ViewPointVector","[0 -15000 1000]"}},source).isEmpty(),"Segment primary camera override failed"); const auto edited=editor->toPlainText(); require(qtCameraSettings(edited).value("Display").segmentFrame.isEmpty() && qtCameraSettings(edited).value("Display").views.size()==3 && qtCameraSettings(edited).value("Display").views[0].segmentFrame=="Sat.FirstArc","Primary override did not clear its segment or removed named secondary views"); editor->undo(); require(editor->toPlainText()==source,"Primary override Undo lost segment metadata"); require(window.buildScript(),"Segment source rebuild after Undo failed");
      const auto formation=QString(source).replace("BeginMissionSequence;","Create Formation Pair;\nPair.Add = {Sat};\nBeginMissionSequence;").replace("Prop(Sat)","Prop(Pair)"); editor->setPlainText(formation); require(window.buildScript(),"Named segment validation rejected a propagated formation member");
      for (const auto &invalid:QStringList{QString(source).replace("Sat.FirstArc","Sat.MissingArc"),QString(source).replace("Sat.FirstArc","Earth.FirstArc"),QString(source).replace("Display.Add = {Sat, Earth}","Display.Add = {Earth}"),QString(source).replace("Create Spacecraft Sat;","Create Spacecraft Sat Other;").replace("Prop(Sat) {Sat.ElapsedSecs","Prop(Other) {Other.ElapsedSecs")}) { editor->setPlainText(invalid); require(!window.buildScript() && editor->toPlainText()==invalid,"Unknown/nonspacecraft/unplotted segment silently accepted or source lost"); }
      require(window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && read(report)==expected && editor->toPlainText()==source && window.plotReceiver()->model("Display")->curves[0].points.size()==count,"Segment failure correction/reopen/rerun changed source/calculation/history");
      std::cout<<"PASS actual OF named-segment conversion, stored/automatic body+inertial and LookAt cameras, nonzero-attitude endpoint clamp, late-data acquisition, four-view selector/replay/close-reopen, primary override/Undo, invalid reference recovery and exact complete independent reports/Unicode source; no propagation algorithm changes.\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
