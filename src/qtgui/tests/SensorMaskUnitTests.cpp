#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "ScriptCompatibility.hpp"
#include "OrbitSensorMasks.hpp"
#include "Moderator.hpp"
#include "Hardware.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static void near(const osg::Vec3d &a,const std::array<double,3>&b){for(int i=0;i<3;++i)if(!std::isfinite(a[i])||std::abs(a[i]-b[i])>1e-10)throw std::runtime_error("Hardware metres were not converted to view kilometres before body rotation/translation");}
static QJsonArray values(const osg::Vec3d &v){return {v.x(),v.y(),v.z()};}
int main(int argc,char **argv){
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("SensorMaskUnits");
 try{
  require(argc==2&&app.platformName()=="offscreen","Supply startup on offscreen platform");TestSettings settings;QTemporaryDir files;const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;require(window.initialize(startup),"Unit boundary runtime unavailable");
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");require(source,"Unit boundary source missing");const auto output=files.filePath("units.txt");
  const auto views=convertOpenFramesViews("Create OpenFramesInterface Orb;\nOrb.Add = {Sat};\nCreate OpenFramesSensorMask Sensor;\nSensor.Source = Sat;\nSensor.Hardware = Device;\nSensor.LengthType = Manual;\nSensor.Length = 5;\nOrb.SensorMask = Sensor;\n");require(views.error.isEmpty(),"Unit boundary view conversion failed");
  const QString script="Create Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 0;\nCreate ConicalFOV Shape;\nShape.FieldOfViewAngle = 10;\nCreate Imager Device;\nDevice.FieldOfView = Shape;\nDevice.HWOriginInBCSX = 1000;\nDevice.HWOriginInBCSY = -2000;\nDevice.HWOriginInBCSZ = 500;\nDevice.DirectionX = 1;\nDevice.DirectionY = 0;\nDevice.DirectionZ = 0;\nDevice.SecondDirectionX = 0;\nDevice.SecondDirectionY = 1;\nDevice.SecondDirectionZ = 0;\nSat.AddHardware = {Device};\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator P;\nP.FM = FM;\nCreate Variable Keep;\nKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+output+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 20;\n"+views.script+"\nBeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 1};\nReport Values Keep;\n";
  source->setPlainText(script);const auto status=window.runMission();if(status!=MainWindow::RunResult::Completed)for(auto *text:window.findChildren<QPlainTextEdit *>())if(text!=source)std::cerr<<text->objectName().toStdString()<<" "<<text->toPlainText().toStdString()<<std::endl;require(status==MainWindow::RunResult::Completed,"Unit boundary mission failed");require(source->toPlainText()==script,"Unit rendering changed authored source");QFile file(output);require(file.open(QIODevice::ReadOnly),"Unit sentinel output missing");require(std::abs(QString::fromUtf8(file.readAll()).trimmed().toDouble()-41)<1e-12,"Unit boundary changed sentinel");
  const auto model=window.plotReceiver()->model("Orb");require(model&&model->sensorMasks.size()==1,"Unit boundary mask missing");const auto mask=model->sensorMasks[0];require(mask.originInMetres&&mask.origin==std::array<double,3>{1000,-2000,500},"Native raw hardware offset/unit tag changed");
  const auto location=dynamic_cast<Hardware *>(Moderator::Instance()->GetConfiguredObject("Device"))->GetLocation();require(location[0]==1000&&location[1]==-2000&&location[2]==500,"Unit conversion mutated configured hardware");
  // Independent literal SI conversion and two prescribed body-to-view poses.
  PlotPoint pose;pose.x=7000;pose.y=10;pose.z=20;pose.bodyToView={1,0,0,0,1,0,0,0,1};
  auto a=orbitSensorMaskGeometry(*model,mask,pose);require(bool(a),"Identity mask geometry missing");near(a->origin,{7001,8,20.5});near(a->end,{7006,8,20.5});
  pose.bodyToView={0,-1,0,1,0,0,0,0,1};auto b=orbitSensorMaskGeometry(*model,mask,pose);require(bool(b),"Rotated mask geometry missing");near(b->origin,{7002,11,20.5});near(b->end,{7002,16,20.5});
  // Direct plot geometry already in km keeps its unit contract.
  auto km=mask;km.originInMetres=false;km.origin={1,-2,.5};auto c=orbitSensorMaskGeometry(*model,km,pose);require(bool(c),"Explicit kilometre geometry missing");near(c->origin,{7002,11,20.5});near(c->end,{7002,16,20.5});
  const QJsonObject proof{{"checkpoint","hardware-metres-to-view-kilometres"},{"outcome","passed"},{"raw_metres",QJsonArray{1000,-2000,500}},{"identity_origin_km",values(a->origin)},{"identity_end_km",values(a->end)},{"rotated_origin_km",values(b->origin)},{"rotated_end_km",values(b->end)},{"configured_hardware_source_and_sentinel_preserved",true},{"facets",QJsonArray{"actual-native-hardware-receiver-freeze","literal-si-conversion-before-body-transform","manual-kilometre-geometry-contract","unchanged-source-configured-hardware-and-sentinel"}}};
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!evidence.isEmpty()){QFile report(QDir(evidence).filePath("unit-proof.json"));require(report.open(QIODevice::WriteOnly),"Unit proof write failed");report.write(QJsonDocument(proof).toJson());}
  std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException&e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
