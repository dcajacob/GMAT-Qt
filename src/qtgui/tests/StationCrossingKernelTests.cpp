// Analytic SPK inputs test isolated roots independently of contact intervals.
#include "FormationAcceptance.hpp"
#include "EphemManager.hpp"
#include "SpiceInterface.hpp"
#include "SpiceOrbitKernelWriter.hpp"
#include "FileManager.hpp"
#include <memory>
using namespace formaccept;

static void makeKernel(const QString &path,Integer id,const QString &name,
      int shape,double moonDistance,SpiceInterface &spice)
{
 std::vector<std::unique_ptr<Rvector6>> stateStorage;
 std::vector<std::unique_ptr<A1Mjd>> epochStorage;StateArray states;EpochArray epochs;
 const double center=spice.A1ToSpiceTime(epoch);
 const int count=shape==7||shape==8 ? 4 : 2;
 for(int i=-count;i<=count;++i){
  const double offset=shape==7||shape==8 ? (std::abs(i)<=2 ? i*.001 : (i<0?-1:1)*(std::abs(i)-2)*50.) : i*50.;
  const double a1=epoch+offset/86400;const double t=spice.A1ToSpiceTime(a1)-center;
  const double shifted=t+(shape==7?2.5e-6:-2.5e-6);
  const double edge=shape==9||shape==10 ? spice.A1ToSpiceTime(epoch+(shape==9?-100:100)/86400.)-center : 0;
  const double z=shape==1?t:shape==-1?-t:shape==2?t*t:shape==3?1e-4*t*t*t:shape==-3?-1e-4*t*t*t:shape==4?10000:shape==5?1e-6*(t+5e-6):shape==6?1e-6*(t-5e-6):shape==7||shape==8?1000*shifted*shifted*shifted:shape==9||shape==10?t-edge:0;
  const double vz=shape==1?1:shape==-1?-1:shape==2?2*t:shape==3?3e-4*t*t:shape==-3?-3e-4*t*t:shape==5||shape==6?1e-6:shape==7||shape==8?3000*shifted*shifted:shape==9||shape==10?1:0;
  stateStorage.emplace_back(new Rvector6(id==301?moonDistance:shape==4?0:10000,0,id==301?0:z,0,0,id==301?0:vz));
  epochStorage.emplace_back(new A1Mjd(a1));states.push_back(stateStorage.back().get());epochs.push_back(epochStorage.back().get());
 }
 SpiceOrbitKernelWriter writer(name.toStdString(),"Earth",id,399,path.toStdString(),3,"ECLIPJ2000");
 writer.WriteSegment(*epochs.front(),*epochs.back(),states,epochs);writer.FinalizeKernel(true,false);
}

int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("StationCrossingKernel");try{
 require(argc==2,"Supply native startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
 rig.source->setPlainText("Create Spacecraft Probe;\nProbe.NAIFId = -160001;\nBeginMissionSequence;\n");require(rig.window.buildScript(),"Crossing synthetic fixture build failed");
 auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("Probe"));require(sat,"Synthetic spacecraft missing");
 auto *system=Moderator::Instance()->GetSolarSystemInUse();auto *moon=system->GetBody("Luna");require(moon,"Synthetic occulting body missing");
 const auto oldFrame=moon->GetStringParameter(moon->GetParameterID("SpiceFrameId"));
 const auto frame=rig.files.filePath("crossing-frame.tf");
 const QByteArray frameText="KPL/FK\n\\begindata\nFRAME_CROSS_MOON = 1400001\nFRAME_1400001_NAME = 'CROSS_MOON'\nFRAME_1400001_CLASS = 4\nFRAME_1400001_CLASS_ID = 1400001\nFRAME_1400001_CENTER = 301\nTKFRAME_1400001_RELATIVE = 'ECLIPJ2000'\nTKFRAME_1400001_SPEC = 'MATRIX'\nTKFRAME_1400001_MATRIX = (1 0 0 0 1 0 0 0 1)\n\\begintext\n";
 write(frame,frameText);rig.save("frame.tf",frameText);SpiceInterface spice;
 spice.LoadKernel(FileManager::Instance()->GetFullPathname("PLANETARY_PCK_FILE"));spice.LoadKernel(frame.toStdString());
 moon->SetStringParameter(moon->GetParameterID("SpiceFrameId"),"CROSS_MOON");
 struct Case{const char *name;int shape;double start,stop;int rises,sets;double moon;double elevation=0;};
 const Case cases[]={
  {"RiseInterior",1,-40,40,1,0,0},{"RiseAtStart",1,0,40,1,0,0},{"RiseAtEnd",1,-40,0,1,0,0},
  {"SetAtStart",-1,0,40,0,1,0},{"SetAtEnd",-1,-40,0,0,1,0},
  {"ClippedAbove",1,10,40,0,0,0},{"Below",1,-40,-10,0,0,0},
  {"Tangent",2,-40,40,0,0,0},{"EqualityPlateau",0,-40,40,0,0,0},
  {"OccultedRise",1,-40,40,0,0,5000},{"FarBodyTransit",1,-40,40,1,0,20000},
  {"CubicRise",3,-40,40,1,0,0},{"CubicSet",-3,-40,40,0,1,0},
  {"LatitudePole",4,-40,40,0,0,0,90},{"SingletonRise",1,0,0,1,0,0},
  {"RootBeforeWindow",1,1e-5,40,0,0,0},{"RootAfterWindow",1,-40,-1e-5,0,0,0},
  {"ShallowRootBeforeWindow",5,0,40,0,0,0},{"ShallowRootAfterWindow",6,-40,0,0,0,0},
  {"ShallowRootInsideWindow",6,0,40,1,0,0},
  {"ShiftedCubicBeforeWindow",7,0,.001,0,0,0},{"ShiftedCubicAfterWindow",8,-.001,0,0,0,0},
  {"ShiftedCubicInsideWindow",8,0,.001,1,0,0},
  {"RiseAtCoverageStart",9,-100,40,1,0,0},{"RiseAtCoverageEnd",10,-40,100,1,0,0}};
 QJsonArray outcomes;
 for(const auto &c:cases){
  const auto target=rig.files.filePath(QString(c.name)+"-target.bsp"),occulter=rig.files.filePath(QString(c.name)+"-moon.bsp");
  makeKernel(target,-160001,QString(c.name)+"Target",c.shape,0,spice);spice.LoadKernel(target.toStdString());
  if(c.moon){makeKernel(occulter,301,QString(c.name)+"Moon",0,c.moon,spice);spice.LoadKernel(occulter.toStdString());}
  require(sat->SetStringParameter("OrbitSpiceKernelName",target.toStdString(),0)&&sat->GetStringArrayParameter("OrbitSpiceKernelName").size()==1,"Synthetic target kernel replacement failed");
  EphemManager manager(false);manager.SetObject(sat);manager.SetSolarSystem(system);
  manager.SetCoordinateSystem(dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("EarthMJ2000Eq")));require(manager.Initialize(),"Synthetic event manager unavailable");
  RealArray rises{123},sets{456};StringArray bodies;if(c.moon)bodies.push_back("Luna");
  const auto centerEt=spice.A1ToSpiceTime(epoch);
  SpiceDouble centerState[6],lightTime;
  spkezr_c("-160001",centerEt,"ECLIPJ2000","NONE","399",centerState,&lightTime);
  require(!failed_c(),"Synthetic root state unavailable");
  SPICEDOUBLE_CELL(directWindow,20);SPICEDOUBLE_CELL(directRoots,20);
  scard_c(0,&directWindow);scard_c(0,&directRoots);
  wninsd_c(spice.A1ToSpiceTime(epoch+c.start/86400),spice.A1ToSpiceTime(epoch+c.stop/86400),&directWindow);
  if(c.shape<9) gfposc_c("-160001","ECLIPJ2000","NONE","399","LATITUDINAL","LATITUDE","=",c.elevation*GmatMathConstants::RAD_PER_DEG,0,1,100,&directWindow,&directRoots);
  if(failed_c()){SpiceChar message[1800];getmsg_c("LONG",1800,message);std::cerr<<message<<std::endl;require(false,"Synthetic direct equality diagnostic failed");}
  QJsonArray directRootOffsets;for(int i=0;i<wncard_c(&directRoots);++i){double first,last;wnfetd_c(&directRoots,i,&first,&last);directRootOffsets.append(first-centerEt);}
  manager.GetStationElevationCrossings("399",c.elevation,"ECLIPJ2000",bodies,"NONE",epoch+c.start/86400,epoch+c.stop/86400,false,false,false,1,rises,sets);
  QJsonArray riseTimes,setTimes;for(double v:rises)riseTimes.append(v);for(double v:sets)setTimes.append(v);
  const QJsonObject row{{"case",c.name},{"shape",c.shape},{"elevation_degrees",c.elevation},{"start_seconds",c.start},{"stop_seconds",c.stop},{"moon_distance",c.moon},{"rises",riseTimes},{"sets",setTimes},{"center_et",centerEt},{"center_z",centerState[2]},{"center_vz",centerState[5]},{"direct_root_offsets",directRootOffsets}};
  outcomes.append(row);rig.save("outcomes.json",QJsonDocument(outcomes).toJson());
  require(int(rises.size())==c.rises&&int(sets.size())==c.sets,"Station crossing count differs from analytic polynomial/occultation truth");
  const double expectedOffset=c.shape==5?-5e-6:c.shape==6?5e-6:c.shape==7?-2.5e-6:c.shape==8?2.5e-6:c.shape==9?-100:c.shape==10?100:0;
  for(const auto *values:{&rises,&sets})for(double v:*values)require(std::isfinite(v)&&std::abs((v-epoch)*86400-expectedOffset)<5e-6,"Station crossing epoch differs from exact analytic root");
  spice.UnloadKernel(target.toStdString());if(c.moon)spice.UnloadKernel(occulter.toStdString());
 }
 moon->SetStringParameter(moon->GetParameterID("SpiceFrameId"),oldFrame);spice.UnloadKernel(frame.toStdString());acceptanceView(rig.window);
 const QJsonObject proof{{"checkpoint","station-crossing-kernel"},{"outcome","passed"},{"cases",outcomes},
  {"scope","Analytic polynomial SPKs test root endpoints, clipped searches, tangent/plateau and occultation ordering. Native helper only; no GUI selection or whole NASA requirement credit."}};
 rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
