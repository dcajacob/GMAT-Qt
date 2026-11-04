#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "SpacePoint.hpp"
#include "GmatTime.hpp"
#include "BaseException.hpp"
#include "ConsoleMessageReceiver.hpp"
#include "MessageInterface.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
int main(int argc,char**argv){
 try{
  if(argc!=5)throw std::runtime_error("startup, geometry-only script, observations, output");
  MessageInterface::SetMessageReceiver(ConsoleMessageReceiver::Instance());
  auto*m=Moderator::Instance();if(!m->Initialize(argv[1]))throw std::runtime_error("Initialize failed");m->CreateDefaultParameters();
  if(!m->InterpretScript(argv[2]))throw std::runtime_error("Geometry script rejected");
  const auto status=m->RunMission();std::cerr<<"geometry-only Stop status "<<status<<"\n";
  auto*earth=m->GetSolarSystemInUse()->GetBody("Earth");std::ifstream input(argv[3]);std::ofstream output(argv[4]);if(!input||!output)throw std::runtime_error("Input/output unavailable");
  output<<"tai_mjd,station,x,y,z,vx,vy,vz\n"<<std::setprecision(17);std::string line;int rows=0;
  while(std::getline(input,line)){
   if(line.empty()||line[0]=='%')continue;std::istringstream row(line);std::string epoch,type,id,station,satellite,value;row>>epoch>>type>>id>>station>>satellite>>value;
   if(type!="Range_Skin"||station.empty())throw std::runtime_error("Observation schema differs");
   auto*point=dynamic_cast<SpacePoint*>(m->GetInternalObject(station));if(!point)throw std::runtime_error("Initialized station missing");
   GmatTime a1;if(!a1.SetMjdString(epoch))throw std::runtime_error("TAI epoch invalid");a1.AddSeconds(.0343817);
   const auto state=point->GetMJ2000State(a1)-earth->GetMJ2000State(a1);
   output<<epoch<<','<<station;for(int i=0;i<6;++i)output<<','<<state[i];output<<'\n';++rows;
  }
  output.close();std::cerr<<"shared station rows "<<rows<<"\n";m->Finalize();return rows==358?0:1;
 }catch(BaseException&e){std::cerr<<e.GetFullMessage()<<'\n';}catch(std::exception&e){std::cerr<<e.what()<<'\n';}return 1;
}
