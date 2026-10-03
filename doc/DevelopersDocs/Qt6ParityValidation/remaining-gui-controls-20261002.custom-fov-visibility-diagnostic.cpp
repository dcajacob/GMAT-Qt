#include "CustomFOV.hpp"
#include "GmatConstants.hpp"
#include "BaseException.hpp"
#include <iostream>
#include <memory>

void diagnose(const char *name,CustomFOV &fov)
{
   fov.Initialize();
   const double rad=GmatMathConstants::RAD_PER_DEG;
   std::cout<<name<<" paired_count="<<fov.GetRvectorParameter("ConeAngles").GetSize()<<'\n';
   for (const auto &query:{std::pair<double,double>{0,0},{5,45},{5,20},{1,30},{40,0},{40,45}})
      std::cout<<name<<" cone_deg="<<query.first<<" clock_deg="<<query.second<<" visibility="<<fov.CheckTargetVisibility(query.first*rad,query.second*rad)<<'\n';
}
int main()
{
   try {
      CustomFOV direct("LegacyCardinal");
      for (int i=0;i<4;++i) { direct.SetRealParameter("ConeAngles",20,i); direct.SetRealParameter("ClockAngles",90*i,i); }
      diagnose("direct indexed cardinal",direct);
      CustomFOV offset("LegacyOffset");
      for (int i=0;i<4;++i) { offset.SetRealParameter("ConeAngles",20,i); offset.SetRealParameter("ClockAngles",15+90*i,i); }
      diagnose("direct indexed offset",offset);
      CustomFOV azel("LegacyAzEl"); azel.SetRealParameter("InterpolationStepSize",30);
      for (int i=0;i<4;++i) { azel.SetRealParameter("Azimuth",90*i,i); azel.SetRealParameter("Elevation",70,i); }
      diagnose("direct indexed azel",azel);
   } catch (BaseException &failure) { std::cerr<<failure.GetFullMessage()<<'\n'; return 1; }
}
