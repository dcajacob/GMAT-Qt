// Internal full Cartesian differential for owner-dependent object axes.
#ifndef OrbitStmOwnerFrame_hpp
#define OrbitStmOwnerFrame_hpp
#include "OrbitStmContext.hpp"
#include "CoordinateSystem.hpp"
#include "AxisSystem.hpp"
#include <array>
#include <algorithm>
#include <cctype>
#include <limits>

namespace OrbitStmOwnerFrame {
struct Jet {
   long double value=0;
   std::array<long double,6> derivative{};
   Jet()=default;
   Jet(long double v):value(v){}
};
inline Jet operator+(const Jet &a,const Jet &b){Jet r(a.value+b.value);for(int j=0;j<6;++j)r.derivative[j]=a.derivative[j]+b.derivative[j];return r;}
inline Jet operator-(const Jet &a,const Jet &b){Jet r(a.value-b.value);for(int j=0;j<6;++j)r.derivative[j]=a.derivative[j]-b.derivative[j];return r;}
inline Jet operator-(const Jet &a){return Jet(0)-a;}
inline Jet operator*(const Jet &a,const Jet &b){Jet r(a.value*b.value);for(int j=0;j<6;++j)r.derivative[j]=a.derivative[j]*b.value+a.value*b.derivative[j];return r;}
inline Jet operator/(const Jet &a,const Jet &b){Jet r(a.value/b.value);for(int j=0;j<6;++j)r.derivative[j]=(a.derivative[j]-r.value*b.derivative[j])/b.value;return r;}
inline Jet Root(const Jet &a){if(!(a.value>0)||!std::isfinite(a.value))throw ParameterException("Undefined owner-referenced STM frame axis norm.\n");Jet r(std::sqrt(a.value));for(int j=0;j<6;++j)r.derivative[j]=a.derivative[j]/(2*r.value);return r;}
using Vector=std::array<Jet,3>;
using Matrix=std::array<std::array<long double,6>,6>;
inline Real Mjd(const A1Mjd &epoch){return epoch.Get();}
inline Real Mjd(const GmatTime &epoch){return epoch.GetMjd();}
inline Vector operator+(const Vector &a,const Vector &b){return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
inline Vector operator-(const Vector &a,const Vector &b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
inline Vector Scale(const Vector &a,const Jet &s){return {a[0]*s,a[1]*s,a[2]*s};}
inline Jet Dot(const Vector &a,const Vector &b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
inline Vector Cross(const Vector &a,const Vector &b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
inline Vector Unit(const Vector &a){return Scale(a,Jet(1)/Root(Dot(a,a)));}
inline Vector UnitRate(const Vector &a,const Vector &rate){const auto u=Unit(a);return Scale(rate-Scale(u,Dot(u,rate)),Jet(1)/Root(Dot(a,a)));}

template<class Epoch>
inline Rvector6 RelativeState(SpacePoint *point,SpacePoint *anchor,const Epoch &epoch)
{
   if(point==anchor)return Rvector6(0,0,0,0,0,0);
   if(auto *spacecraft=dynamic_cast<Spacecraft*>(point))
      if(spacecraft->GetJ2000Body()==anchor)return OrbitStmContext::Scope::SampleOf(spacecraft).state;
   return point->GetMJ2000State(epoch)-anchor->GetMJ2000State(epoch);
}
template<class Epoch>
inline Matrix Jacobian(Spacecraft *owner,CoordinateSystem *from,CoordinateSystem *to,const Epoch &epoch)
{
   if(from->GetBaseSystem()!="FK5"||!from->AreAxesOfType("MJ2000EqAxes")||from->GetOrigin()!=owner->GetJ2000Body()||
         !to->GetAxisSystem()||to->GetAxisSystem()->GetTypeName()!="ObjectReferenced"||to->GetBaseSystem()!="FK5")
      throw ParameterException("The full owner-dependent STM Jacobian is not available for this frame family/internal chart.\n");
   auto *primary=to->GetPrimaryObject(),*secondary=to->GetSecondaryObject(),*origin=to->GetOrigin();
   if(!primary||!secondary||!origin||(primary!=owner&&secondary!=owner))
      throw ParameterException("Unresolved owner-dependent STM object axes.\n");
   std::array<std::string,3> labels;
   int used=0;bool normal=false,radial=false,velocity=false;
   const char *names[]={"XAxis","YAxis","ZAxis"};
   for(int i=0;i<3;++i){labels[i]=to->GetAxisSystem()->GetStringParameter(to->GetAxisSystem()->GetParameterID(names[i]));std::transform(labels[i].begin(),labels[i].end(),labels[i].begin(),[](unsigned char c){return std::toupper(c);});if(!labels[i].empty())++used;auto label=labels[i];if(!label.empty()&&label[0]=='-')label.erase(0,1);normal=normal||label=="N";radial=radial||label=="R";velocity=velocity||label=="V";}
   if(used!=2||!normal||(radial==velocity))
      throw ParameterException("Owner-dependent object axes need an R/N or V/N pair for an open six-dimensional Cartesian chart.\n");
   // The orbit-normal position is identically zero in this centered chart.
   // This is a mathematical rank deficiency, not an unsupported-frame waiver.
   if((primary==origin&&secondary==owner)||(secondary==origin&&primary==owner))
      throw ParameterException("The owner-referenced STM frame has a singular Cartesian state map: orbit-normal position is identically zero.\n");
   auto *anchor=owner->GetJ2000Body();
   const auto own=OrbitStmContext::Scope::SampleOf(owner).state;
   std::array<Jet,6> variables;
   for(int i=0;i<6;++i){variables[i]=Jet(own[i]);variables[i].derivative[i]=1;}
   auto position=[&](SpacePoint *p){if(p==owner)return Vector{variables[0],variables[1],variables[2]};const auto state=RelativeState(p,anchor,epoch);return Vector{Jet(state[0]),Jet(state[1]),Jet(state[2])};};
   auto speed=[&](SpacePoint *p){if(p==owner)return Vector{variables[3],variables[4],variables[5]};const auto state=RelativeState(p,anchor,epoch);return Vector{Jet(state[3]),Jet(state[4]),Jet(state[5])};};
   const Vector r=position(secondary)-position(primary),v=speed(secondary)-speed(primary);
   Rvector3 acceleration;
   if(!ODEModel::GetBoundRelativeAcceleration(secondary,primary,A1Mjd(Mjd(epoch)),acceleration))
      throw ParameterException("The owner-dependent STM frame acceleration is unavailable.\n");
   const auto &gradient=OrbitStmContext::Scope::GradientOf(owner);
   Vector a;const long double sign=secondary==owner?1:-1;
   for(int i=0;i<3;++i){a[i]=Jet(acceleration[i]);for(int j=0;j<6;++j)a[i].derivative[j]=sign*gradient(i,j);}
   const auto n=Cross(r,v);const auto ru=Unit(r),vu=Unit(v),nu=Unit(n);
   const auto rd=UnitRate(r,v),vd=UnitRate(v,a),nd=UnitRate(n,Cross(r,a));
   std::array<Vector,3> axes,rates;int missing=-1;
   for(int i=0;i<3;++i){auto label=labels[i];long double direction=1;if(!label.empty()&&label[0]=='-'){direction=-1;label.erase(0,1);}if(label.empty()){missing=i;continue;}if(label=="R"){axes[i]=Scale(ru,Jet(direction));rates[i]=Scale(rd,Jet(direction));}else if(label=="V"){axes[i]=Scale(vu,Jet(direction));rates[i]=Scale(vd,Jet(direction));}else if(label=="N"){axes[i]=Scale(nu,Jet(direction));rates[i]=Scale(nd,Jet(direction));}else throw ParameterException("Unsupported owner-dependent STM axis label.\n");}
   if(missing<0)throw ParameterException("Owner-dependent STM axes have no orthogonal completion.\n");
   const int left=(missing+1)%3,right=(missing+2)%3;axes[missing]=Cross(axes[left],axes[right]);rates[missing]=Cross(rates[left],axes[right])+Cross(axes[left],rates[right]);
   const auto p=position(owner)-position(origin),w=speed(owner)-speed(origin);
   std::array<Jet,6> mapped;
   for(int i=0;i<3;++i){mapped[i]=Dot(axes[i],p);mapped[i+3]=Dot(axes[i],w)+Dot(rates[i],p);}
   Matrix result{};for(int i=0;i<6;++i)for(int j=0;j<6;++j){result[i][j]=mapped[i].derivative[j];if(!std::isfinite(result[i][j]))throw ParameterException("Nonfinite owner-dependent STM Jacobian.\n");}
   return result;
}
inline Matrix Multiply(const Matrix &a,const Matrix &b){Matrix r{};for(int i=0;i<6;++i)for(int j=0;j<6;++j)for(int k=0;k<6;++k)r[i][j]+=a[i][k]*b[k][j];return r;}
inline Matrix Inverse(const Matrix &input)
{
   std::array<std::array<long double,12>,6> a{};long double largest=0;
   for(int i=0;i<6;++i){for(int j=0;j<6;++j){a[i][j]=input[i][j];largest=std::max(largest,std::abs(input[i][j]));}a[i][i+6]=1;}
   for(int j=0;j<6;++j){int pivot=j;for(int i=j+1;i<6;++i)if(std::abs(a[i][j])>std::abs(a[pivot][j]))pivot=i;
      if(std::abs(a[pivot][j])<=largest*256*std::numeric_limits<long double>::epsilon())throw ParameterException("STM initial frame Jacobian is numerically rank deficient at native precision.\n");
      std::swap(a[j],a[pivot]);const auto divisor=a[j][j];for(auto &v:a[j])v/=divisor;
      for(int i=0;i<6;++i)if(i!=j){const auto factor=a[i][j];for(int k=0;k<12;++k)a[i][k]-=factor*a[j][k];}
   }Matrix r{};for(int i=0;i<6;++i)for(int j=0;j<6;++j)r[i][j]=a[i][j+6];return r;
}
inline Rmatrix66 Transport(const Matrix &final,const Rmatrix66 &native,const Matrix &initial)
{
   Matrix phi{};for(int i=0;i<6;++i)for(int j=0;j<6;++j)phi[i][j]=native(i,j);
   const auto value=Multiply(final,Multiply(phi,Inverse(initial)));Rmatrix66 result(false);
   for(int i=0;i<6;++i)for(int j=0;j<6;++j){result(i,j)=static_cast<Real>(value[i][j]);if(!std::isfinite(result(i,j)))throw ParameterException("Nonfinite owner-dependent STM result.\n");}
   return result;
}
}
#endif
