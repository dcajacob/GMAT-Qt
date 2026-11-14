#include "SolarPowerSystem.hpp"
#include "BaseException.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(){
 try{
  SolarPowerSystem solar("UnitMetadataProbe");
  const std::array<double,5> defaults{1.32077,-0.10848,-0.11665,0.10843,-0.01279};
  for(int i=0;i<5;++i){const auto name="SolarCoeff"+std::to_string(i+1);const auto id=solar.GetParameterID(name);
   require(solar.GetParameterText(id)==name&&solar.GetParameterType(id)==Gmat::REAL_TYPE,"Solar coefficient identity/type changed");
   require(solar.GetParameterUnit(id)=="dimensionless","Solar coefficient unit must describe the AU-normalized dimensionless model, not kW?");
   require(solar.GetRealParameter(id)==defaults[i],"Solar coefficient default changed");
  }
  // Independent check of the unchanged published default coefficients at r=1.
  const double numerator=defaults[0]+defaults[1]+defaults[2],denominator=1+defaults[3]+defaults[4];
  require(std::abs(numerator/denominator-1)<1e-14,"Default coefficient data no longer normalize the r=1 factor");
  const auto initialPower=solar.GetParameterID("InitialMaxPower");
  require(solar.GetParameterUnit(initialPower)=="kW","Inherited power unit changed");
  for(int i=0;i<5;++i){const auto id=solar.GetParameterID("SolarCoeff"+std::to_string(i+1));const double value=(i-2)*.125;
   require(solar.SetRealParameter(id,value)==value&&solar.GetRealParameter(id)==value,"Unit metadata changed coefficient read/write behavior");
  }
  std::unique_ptr<GmatBase> cloned(solar.Clone());
  for(int i=0;i<5;++i){const auto name="SolarCoeff"+std::to_string(i+1);const auto id=cloned->GetParameterID(name);require(cloned->GetParameterUnit(id)=="dimensionless"&&cloned->GetRealParameter(id)==(i-2)*.125,"Clone changed coefficient values or units");}
  std::cout<<"PASS five AU-normalized coefficient labels, unchanged defaults/read-write/clone values and inherited kW power unit; metadata/data check only\n";return 0;
 }catch(BaseException&e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
