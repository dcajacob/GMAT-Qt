// A zonal gravity model is invariant under rotation about the Earth's pole.
// This contract uses only that symmetry, not a second copy of Brouwer formulas.
#include "StateConversionUtil.hpp"
#include "BaseException.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>
int main() {
 int checks=0,failures=0;
 std::cout<<std::setprecision(17);
 for(double inclination:{47.,60.,63.2,64.,65.,118.}) {
  const Rvector6 mean(9800.,.3,inclination,88.,18.,67.);
  try {
   const auto base=StateConversionUtil::BrouwerMeanLongToCartesian(398600.4418,mean);
   for(double angle:{13.,90.,-31.}) {
    auto shifted=mean;shifted[3]+=angle;
    const auto actual=StateConversionUtil::BrouwerMeanLongToCartesian(398600.4418,shifted);
    const double rad=angle*std::acos(-1.)/180.,c=std::cos(rad),s=std::sin(rad);
    double maxp=0,maxv=0;
    for(int offset:{0,3}) {
     const double expected[]={c*base[offset]-s*base[offset+1],s*base[offset]+c*base[offset+1],base[offset+2]};
     for(int i=0;i<3;++i) {
      const double error=std::abs(actual[offset+i]-expected[i]);
      if(offset==0)maxp=std::max(maxp,error);else maxv=std::max(maxv,error);
     }
    }
    const bool ok=std::isfinite(maxp)&&std::isfinite(maxv)&&maxp<=1e-8&&maxv<=1e-11;
    ++checks;if(!ok)++failures;
    std::cout<<"BROUWER_ROTATION inclination="<<inclination<<" rotation="<<angle<<" position_error_km="<<maxp<<" velocity_error_km_s="<<maxv<<" passed="<<ok<<'\n';
   }
  }catch(BaseException &e){++failures;std::cerr<<e.GetFullMessage()<<'\n';}
 }
 std::cout<<"BROUWER_ROTATION_RESULT checks="<<checks<<" failures="<<failures<<'\n';
 return checks==18&&failures==0?0:1;
}
