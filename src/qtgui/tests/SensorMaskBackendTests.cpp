#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "ScriptCompatibility.hpp"
#include "OrbitSensorMasks.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QSlider>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read new mask report"); return file.readAll(); }
static void preserve(const QString &name,const QByteArray &data) {
   const auto folder=qEnvironmentVariable("GMAT_QT_SENSOR_MASK_EVIDENCE"); if (folder.isEmpty()) return;
   QDir().mkpath(folder); QFile file(folder+"/"+name); require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"Cannot preserve new mask evidence");
}
static QString masksSource() {
   return "Create OpenFramesSensorMask ConeMask RectMask PolyMask;\nConeMask.Source = Sat;\nConeMask.Hardware = ConeImager;\nConeMask.Label = 'mount α''s cone';\nConeMask.LengthType = Manual;\nConeMask.Length = 90;\nRectMask.Source = Earth;\nRectMask.Hardware = RectImager;\nPolyMask.Source = Sat;\nPolyMask.Hardware = PolyImager;\nPolyMask.LengthType = Manual;\nPolyMask.Length = 60;\n";
}
static QString plotSource(const QString &selection) {
   return "Create OpenFramesInterface Orb;\nOrb.Add = {Sat, Earth};\nOrb.Axes = Off;\nOrb.EnableStars = Off;\nOrb.UpdatePlotFrequency = 1;\n"+masksSource()+selection;
}
static QString converted(const QString &source) {
   const auto result=convertOpenFramesViews(source); require(result.error.isEmpty(),qPrintable(result.error)); return result.script;
}
static QStringList names(const QVector<QtSensorMaskSetting> &masks) {
   QStringList result; for (const auto &mask:masks) result.append(mask.name); return result;
}
static void conversionChecks() {
   const auto source=plotSource("Orb.SensorMask = {ConeMask, RectMask};\nOrb.SensorMask = PolyMask;\nOrb.SensorMask = ConeMask;\n");
   const auto script=converted(source);
   const auto masks=qtCameraSettings(script).value("Orb").sensorMasks;
   require(names(masks)==QStringList{"ConeMask","RectMask","PolyMask"},"Sensor mask ordered append/dedup failed");
   require(masks[0].label=="mount α's cone" && masks[0].length==90 && !masks[0].automaticLength && masks[1].automaticLength && masks[1].length==1 && masks[1].label=="DefaultSensorMask","Sensor mask defaults/quoted label/length lost");
   require(script.contains("% Qt conversion: Create OpenFramesSensorMask ConeMask RectMask PolyMask;") && script.contains("% Qt conversion: ConeMask.Label = 'mount α''s cone';"),"Original mask definitions were not retained");
   const auto replaced=converted(source+"Orb.SensorMask = {PolyMask, ConeMask};\nOrb.SensorMask = RectMask;\n");
   require(names(qtCameraSettings(replaced).value("Orb").sensorMasks)==QStringList{"PolyMask","ConeMask","RectMask"},"Braced mask replacement lost source order");
   require(qtCameraSettings(converted(source+"Orb.SensorMask = {};\n")).value("Orb").sensorMasks.isEmpty(),"Empty mask selection failed to clear");
   const auto json=qtSensorMasksJson(masks); require(qtSensorMasksJson(qtSensorMasks(json))==json,"Mask JSON round trip failed");
   for (const auto &bad:QStringList{"ConeMask.Length = 0;","ConeMask.Length = -1;","ConeMask.Length = NaN;","ConeMask.LengthType = Other;","ConeMask.Unknown = 1;","ConeMask.Source = Sat.NamedPropagate;","Orb.SensorMask = Unknown;","Report Values ConeMask;"}) {
      const auto invalid=source+bad+'\n'; const auto result=convertOpenFramesViews(invalid);
      require(!result.error.isEmpty() && result.script==invalid,"Invalid/unsupported mask conversion changed source");
   }
   const auto original=QJsonDocument::fromJson(json.toUtf8()).array();
   for (int caseNumber=0;caseNumber<7;++caseNumber) {
      auto array=original; auto value=array[0].toObject();
      if (caseNumber==0) value["length"]=0;
      if (caseNumber==1) value["length"]="90";
      if (caseNumber==2) value["automaticLength"]="Auto";
      if (caseNumber==3) value["label"]=true;
      if (caseNumber==4) value["unknown"]=1;
      if (caseNumber==5) value["source"]="Sat.NamedPropagate";
      array[0]=value; if (caseNumber==6) array.append(array[0]); bool rejected=false;
      try { qtSensorMasks(QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact))); } catch (const std::exception &) { rejected=true; }
      require(rejected,"Invalid/duplicate mask metadata accepted");
   }
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("SensorMaskBackend");
   if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   try {
      TestSettings settings; conversionChecks(); QTemporaryDir files; require(files.isValid(),"Temporary mask directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Script editor unavailable"); const auto report=files.filePath("mask-values.txt");
      const QString resources="Create Spacecraft Sat;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 0;\nCreate ConicalFOV Cone;\nCone.FieldOfViewAngle = 10;\nCone.Color = Blue;\nCone.Alpha = 80;\nCreate RectangularFOV Rect;\nRect.AngleWidth = 12;\nRect.AngleHeight = 6;\nCreate CustomFOV Poly;\nPoly.ConeAngles = [5 8 10 12];\nPoly.ClockAngles = [0 90 180 270];\nCreate Imager ConeImager RectImager PolyImager;\nCreate Antenna EmptyHardware;\nConeImager.FieldOfView = Cone;\nRectImager.FieldOfView = Rect;\nPolyImager.FieldOfView = Poly;\nSat.AddHardware = {ConeImager, RectImager, PolyImager};\n";
      QString mount;
      for (const auto &name:QStringList{"ConeImager","RectImager","PolyImager"}) mount+=name+".HWOriginInBCSX = 0.2;\n"+name+".HWOriginInBCSY = -0.3;\n"+name+".HWOriginInBCSZ = 0.4;\n"+name+".DirectionX = 0;\n"+name+".DirectionY = 0;\n"+name+".DirectionZ = 1;\n"+name+".SecondDirectionX = 0;\n"+name+".SecondDirectionY = 1;\n"+name+".SecondDirectionZ = 0;\n";
      const QString propagation="Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 5;\nP.MinStep = 5;\nP.MaxStep = 5;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 17;\nValues.WriteHeaders = false;\nValues.WriteReport = false;\n";
      const QString mission="BeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 20};\nReport Values Sat.ElapsedSecs Sat.A1ModJulian Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z;\n";
      const auto baseline=resources+mount+propagation+converted(plotSource(""))+"\n"+mission;
      editor->setPlainText(baseline); require(window.runMission()==MainWindow::RunResult::Completed,"New mask-free reference failed");
      const auto expected=read(report); std::istringstream row(expected.toStdString()); std::array<double,5> values{};
      require(bool(row>>values[0]>>values[1]>>values[2]>>values[3]>>values[4]) && std::abs(values[0]-20)<1e-5,"New independent elapsed-time reference is not 20 seconds");
      preserve("mask-free.script",baseline.toUtf8()); preserve("mask-free.report",expected);
      const auto source=resources+mount+propagation+converted(plotSource("Orb.SensorMask = {ConeMask, RectMask, PolyMask};\n"))+"\n"+mission;
      editor->setPlainText(source); require(window.runMission()==MainWindow::RunResult::Completed,"New mask-enabled mission failed");
      require(read(report)==expected && editor->toPlainText()==source,"Sensor masks changed complete report or authored source");
      preserve("sensor-masks.script",source.toUtf8()); preserve("sensor-masks.report",read(report));
      const auto model=window.plotReceiver()->model("Orb"); require(model && model->sensorMasks.size()==3,"Receiver did not freeze all three mask shapes");
      const auto &cone=model->sensorMasks[0],&rect=model->sensorMasks[1],&poly=model->sensorMasks[2];
      require(cone.boundary.size()==120 && !cone.drawEdges && rect.boundary.size()==4 && rect.drawEdges && poly.boundary.size()==4 && poly.drawEdges,"Mask shape geometry/default edge modes disagree with OF");
      require(cone.color.red()==0 && cone.color.green()==0 && cone.color.blue()==255 && cone.color.alpha()==80,"FOV RGB/alpha was not retained");
      for (const auto &mask:model->sensorMasks) require(mask.origin==std::array<double,3>{0.2,-0.3,0.4} && std::abs(mask.axis[0])<1e-12 && std::abs(mask.axis[1])<1e-12 && std::abs(mask.axis[2]-1)<1e-12,"Hardware mount location/boresight was not frozen");
      constexpr double radians=0.017453292519943295769;
      require(std::abs(cone.boundary[0][0]+std::tan(10*radians))<1e-12 && std::abs(cone.boundary[0][1])<1e-12 && std::abs(cone.boundary[0][2]-1)<1e-12,"Conical boundary was normalized or cone angle changed");
      for (const auto &vertex:rect.boundary) require(std::abs(std::abs(vertex[0])-std::tan(12*radians))<1e-12 && std::abs(std::abs(vertex[1])-std::tan(6*radians))<1e-12 && std::abs(vertex[2]-1)<1e-12,"Rectangular width/height axial geometry changed");
      const std::array<double,4> angles{5,8,10,12};
      for (int i=0;i<4;++i) require(std::abs(std::hypot(poly.boundary[i][0],poly.boundary[i][1])-std::tan(angles[i]*radians))<1e-12 && std::abs(poly.boundary[i][2]-1)<1e-12,"Custom mask paired angles were not retained");
      require(cone.sourceRadius==1 && rect.sourceRadius>6000 && rect.source=="Earth" && rect.hardware=="RectImager","Auto fallback radius/global Hardware lookup changed");
      auto metadata=qtCameraSettings(source); const auto valid=metadata.value("Orb").sensorMasks;
      for (const auto &missing:QStringList{"MissingHardware","EmptyHardware","Sat"}) {
         auto invalid=metadata; invalid["Orb"].sensorMasks[0].hardware=missing; bool rejected=false;
         try { QtPlotReceiver::validateCameraReferences(invalid); } catch (BaseException &) { rejected=true; } catch (const std::exception &) { rejected=true; }
         require(rejected,"Invalid Hardware/FOV mask reference accepted");
      }
      auto invalid=metadata; invalid["Orb"].sensorMasks[0].source="NotPlotted"; bool rejected=false;
      try { QtPlotReceiver::validateCameraReferences(invalid); } catch (BaseException &) { rejected=true; } catch (const std::exception &) { rejected=true; }
      require(rejected && qtSensorMasksJson(metadata.value("Orb").sensorMasks)==qtSensorMasksJson(valid),"Invalid source mutated valid metadata");
      auto *area=window.findChild<QMdiArea *>("workspace"); PlotWidget *viewer=nullptr;
      for (auto *child:area->subWindowList()) if (child->property("plotName")=="Orb") viewer=dynamic_cast<PlotWidget *>(child->widget());
      require(viewer,"Mask Output viewer unavailable"); auto *timeline=viewer->findChild<QSlider *>("plotTimeline"); require(timeline,"Mask replay control unavailable");
      const auto frozen=cone.boundary; timeline->setValue(0); app.processEvents(); timeline->setValue(timeline->maximum()); app.processEvents();
      require(model->sensorMasks[0].boundary==frozen,"Replay altered frozen FOV geometry");
      for (auto *child:area->subWindowList()) if (child->property("plotName")=="Orb") child->close();
      require(window.plotReceiver()->show("Orb") && window.plotReceiver()->model("Orb")->sensorMasks[0].boundary==frozen,"Output reopening lost sensor masks");
      editor->setPlainText("Create Variable Replacement;\nBeginMissionSequence;\nReplacement = 1;\n"); require(window.buildScript(),"New model replacement failed");
      require(model->sensorMasks[0].boundary==frozen && orbitSensorMaskGeometry(*model,model->sensorMasks[0],model->frame),"Retained mask geometry depends on destroyed engine objects");
      std::cout<<"PASS mask declarations/order/defaults/strict JSON/source rejection, three FOV shapes, axial mount/color/Auto bounds/global references, invalid reference recovery, replay/Output/replaced-engine lifetime and identical new complete 20-second report ("<<expected.size()<<" bytes).\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
