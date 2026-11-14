#pragma once
// Test-only direct potential differentiation. No GMAT gravity implementation.
// Fully normalized real harmonics, no Condon-Shortley phase. Cartesian complex
// powers avoid longitude/latitude singularities at either pole.
#include <array>
#include <algorithm>
#include <cmath>
#include <vector>
namespace HarmonicOracle {
using V=std::array<double,3>;
using S=std::array<double,6>;
using M=std::array<V,3>;
struct Dual {double v=0;V d{};Dual(double value=0):v(value){};};
inline Dual operator+(Dual a,Dual b) {Dual c(a.v+b.v);for(int i=0;i<3;++i)c.d[i]=a.d[i]+b.d[i];return c;}
inline Dual operator-(Dual a,Dual b) {Dual c(a.v-b.v);for(int i=0;i<3;++i)c.d[i]=a.d[i]-b.d[i];return c;}
inline Dual operator*(Dual a,Dual b) {Dual c(a.v*b.v);for(int i=0;i<3;++i)c.d[i]=a.d[i]*b.v+a.v*b.d[i];return c;}
inline Dual operator/(Dual a,Dual b) {Dual c(a.v/b.v);for(int i=0;i<3;++i)c.d[i]=(a.d[i]-c.v*b.d[i])/b.v;return c;}
inline Dual squareRoot(Dual a) {Dual c(std::sqrt(a.v));for(int i=0;i<3;++i)c.d[i]=a.d[i]/(2*c.v);return c;}
struct Coefficient {int n,m;double c,s;};
struct Field {double mu=0,radius=0;int maximum=0;std::vector<Coefficient> coefficients;};
inline V bodyForce(const Field &field,int degree,int order,const V &position) {
   Dual x(position[0]),y(position[1]),z(position[2]);x.d[0]=y.d[1]=z.d[2]=1;
   const Dual r=squareRoot(x*x+y*y+z*z),u=z/r,a=x/r,b=y/r;
   std::vector<Dual> re(degree+1),im(degree+1),radial(degree+1);
   std::vector<std::vector<Dual>> p(degree+1,std::vector<Dual>(degree+1));
   re[0]=radial[0]=p[0][0]=1;
   for(int m=1;m<=degree;++m) {
      re[m]=a*re[m-1]-b*im[m-1];im[m]=b*re[m-1]+a*im[m-1];
      p[m][m]=Dual(2*m-1)*p[m-1][m-1];
   }
   for(int m=0;m<=degree;++m) {
      if(m<degree)p[m+1][m]=Dual(2*m+1)*u*p[m][m];
      for(int n=m+2;n<=degree;++n)
         p[n][m]=(Dual(2*n-1)*u*p[n-1][m]-Dual(n+m-1)*p[n-2][m])/Dual(n-m);
   }
   for(int n=1;n<=degree;++n)radial[n]=radial[n-1]*Dual(field.radius)/r;
   Dual sum(1);
   for(auto coefficient:field.coefficients) {
      const int n=coefficient.n,m=coefficient.m;if(n>degree||m>order)continue;
      const double normalization=std::sqrt((m?2.:1.)*(2*n+1)*std::exp(std::lgamma(n-m+1.)-std::lgamma(n+m+1.)));
      sum=sum+radial[n]*p[n][m]*Dual(normalization)*(Dual(coefficient.c)*re[m]+Dual(coefficient.s)*im[m]);
   }
   return (Dual(field.mu)*sum/r).d;
}
inline V rotate(const M &m,const V &v,bool transpose=false) {
   V out{};for(int i=0;i<3;++i)for(int j=0;j<3;++j)out[i]+=(transpose?m[j][i]:m[i][j])*v[j];return out;
}
inline M orientation(const std::vector<M> &samples,double fraction) {
   M out{};const int count=int(samples.size());
   for(int i=0;i<count;++i) {
      double weight=1;for(int j=0;j<count;++j)if(i!=j)weight*=(fraction*(count-1)-j)/double(i-j);
      for(int row=0;row<3;++row)for(int col=0;col<3;++col)out[row][col]+=weight*samples[i][row][col];
   }
   return out;
}
inline V force(const Field &field,int degree,int order,const V &r,const M &rotation) {
   return rotate(rotation,bodyForce(field,degree,order,rotate(rotation,r)),true);
}
inline S integrate(S state,double seconds,const Field &field,int degree,int order,const std::vector<M> &rotations,int steps) {
   const double h=seconds/steps;
   auto rate=[&](const S &s,double t) {const auto a=force(field,degree,order,{s[0],s[1],s[2]},orientation(rotations,t/seconds));return S{s[3],s[4],s[5],a[0],a[1],a[2]};};
   for(int n=0;n<steps;++n) {
      const auto k1=rate(state,n*h);S s2{},s3{},s4{};
      for(int i=0;i<6;++i)s2[i]=state[i]+h*k1[i]/2;const auto k2=rate(s2,(n+.5)*h);
      for(int i=0;i<6;++i)s3[i]=state[i]+h*k2[i]/2;const auto k3=rate(s3,(n+.5)*h);
      for(int i=0;i<6;++i)s4[i]=state[i]+h*k3[i];const auto k4=rate(s4,(n+1)*h);
      for(int i=0;i<6;++i)state[i]+=h*(k1[i]+2*k2[i]+2*k3[i]+k4[i])/6;
   }
   return state;
}
}
