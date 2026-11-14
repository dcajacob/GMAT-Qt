#include "FormationAcceptance.hpp"
#include <algorithm>
using namespace formaccept;
int main(int argc,char **argv) {
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaMsise86Boundary");
 try {
  require(argc==2,"Supply MSISE86 startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
  const auto earth=Moderator::Instance()->GetListOfFactoryItems(Gmat::ATMOSPHERE,"Earth"),mars=Moderator::Instance()->GetListOfFactoryItems(Gmat::ATMOSPHERE,"Mars");
  require(std::find(earth.begin(),earth.end(),"MSISE86")!=earth.end()&&std::find(mars.begin(),mars.end(),"MSISE86")==mars.end(),"MSISE86 factory body qualification differs");
  QJsonArray phases;for(double height:{89.0,90.001}) {
   QString script="% MSIS86 lower-domain rejection and recovery\nEarth.Flattening = 0;\nEarth.EquatorialRadius = 6378.1363;\nCreate Spacecraft Sat;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '21 Jun 2000 08:03:20.000';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = "+number(6378.1363+height)+";\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.9;\nSat.VZ = 0;\nSat.DragArea = 10;\nCreate ForceModel Forces;\nForces.PrimaryBodies = {Earth};\nForces.PointMasses = {};\nForces.GravityField.Earth.Degree = 0;\nForces.GravityField.Earth.Order = 0;\nForces.Drag.AtmosphereModel = MSISE86;\nForces.Drag.HistoricWeatherSource = ConstantFluxAndGeoMag;\nForces.Drag.PredictedWeatherSource = ConstantFluxAndGeoMag;\nCreate Propagator P;\nP.FM = Forces;\nP.InitialStepSize = 0.001;\nP.MinStep = 0;\nP.MaxStep = 0.001;\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 0.001};\nKeep = 42;\n";
   rig.source->setPlainText(script);require(rig.window.buildScript(),"MSISE86 boundary script did not build");acceptanceView(rig.window);rig.messages->clear();const auto result=rig.window.runMission();events();const bool negative=height<90;const QString name=negative?"below-domain":"recovered";rig.save(name+".script",script.toUtf8());rig.save(name+"-messages.txt",rig.messages->toPlainText().toUtf8());
   require((result==MainWindow::RunResult::Completed)!=negative,"MSISE86 lower-domain result differs");if(negative)require(rig.messages->toPlainText().contains("at or above 90 km altitude"),"MSISE86 lower-domain diagnostic absent");require(rig.source->toPlainText()==script,"Boundary mission changed source");const auto *keep=Moderator::Instance()->GetInternalObject("Keep");require(keep&&const_cast<GmatBase *>(keep)->GetRealParameter("Value")== (negative?41:42),"MSISE86 failure continuation or recovery differs");acceptanceView(*rig.messages);phases.append(QJsonObject{{"height_km",height},{"expected_failure",negative},{"completed",result==MainWindow::RunResult::Completed}});
  }
  const QJsonObject proof{{"checkpoint","msise86-domain"},{"outcome","passed"},{"earth_only_factory",true},{"phases",phases}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
