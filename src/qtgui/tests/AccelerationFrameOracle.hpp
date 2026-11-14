#pragma once
#include <array>
#include <cmath>
namespace accelerationOracle {
using V=std::array<double,3>;
using M=std::array<V,3>;
inline V cross(const V&a,const V&b) {return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
inline V unit(V a) {double n=0;for(auto x:a)n+=x*x;n=std::sqrt(n);for(auto &x:a)x/=n;return a;}
// Position/velocity-only V/N/B basis on a local quadratic trajectory.
// Neither a native rotation nor a native derivative is used by this oracle.
inline M basis(const V&r,const V&v,const V&a,double t)
{
   V p{},w{};for(int i=0;i<3;++i){p[i]=r[i]+t*v[i]+.5*t*t*a[i];w[i]=v[i]+t*a[i];}
   const auto x=unit(w),y=unit(cross(p,w)),z=cross(x,y);M result{};
   for(int i=0;i<3;++i){result[i][0]=x[i];result[i][1]=y[i];result[i][2]=z[i];}return result;
}
inline M derivative(const V&r,const V&v,const V&a)
{
   constexpr double h=.02;const auto mm=basis(r,v,a,-2*h),m1=basis(r,v,a,-h),p1=basis(r,v,a,h),pp=basis(r,v,a,2*h);M result{};
   for(int i=0;i<3;++i)for(int j=0;j<3;++j)result[i][j]=(mm[i][j]-8*m1[i][j]+8*p1[i][j]-pp[i][j])/(12*h);return result;
}
inline V rateDegrees(const V&r,const V&v,const V&a)
{
   const auto R=basis(r,v,a,0),D=derivative(r,v,a);V rate{};
   const int row[3]={2,0,1},column[3]={1,2,0};
   for(int i=0;i<3;++i)for(int k=0;k<3;++k)rate[i]+=R[k][row[i]]*D[k][column[i]]*180/std::acos(-1.);
   return rate;
}
}
