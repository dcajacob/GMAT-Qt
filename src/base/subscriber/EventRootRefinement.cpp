// SPDX-License-Identifier: Apache-2.0
#include "EventRootRefinement.hpp"
#ifdef __USE_SPICE__
#include "SubscriberException.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

namespace {
using Number = long double;
using Vector = std::array<Number,3>;
using Polynomial = std::vector<Number>;
using Trig = std::array<Number,5>; // 1, cos(t), sin(t), cos(2t), sin(2t)
constexpr Number pi = 3.141592653589793238462643383279502884L;

void spiceError()
{
   if (!failed_c()) return;
   SpiceChar message[4096]; getmsg_c("LONG",sizeof(message),message); reset_c();
   throw SubscriberException(std::string("Event root geometry: ")+message);
}
std::string keyword(const std::string &text)
{
   std::string result;
   for (const unsigned char c:text)
      if (!std::isspace(c)) result+=static_cast<char>(std::toupper(c));
   return result;
}
Number dot(const Vector &a,const Vector &b)
{ return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
Vector cross(const Vector &a,const Vector &b)
{ return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}; }
Vector scaled(Vector v,Number scale)
{ for (auto &x:v) x*=scale; return v; }
Number norm(const Vector &v) { return std::sqrt(dot(v,v)); }
Vector difference(Vector a,const Vector &b)
{ for (int i=0;i<3;++i) a[i]-=b[i]; return a; }
Vector unit(const Vector &v)
{
   const Number length=norm(v);
   if (!(length>0) || !std::isfinite(length))
      throw SubscriberException("Event direction must be finite and nonzero.");
   return scaled(v,1/length);
}
Number angle(const Vector &a,const Vector &b)
{ return std::atan2(norm(cross(a,b)),dot(a,b)); }
void finite(Number value)
{
   if (!std::isfinite(value))
      throw SubscriberException("Event root geometry produced a nonfinite value.");
}
Real real(Number value)
{
   finite(value); const Real result=static_cast<Real>(value);
   if (!std::isfinite(result) || (result==0 && value!=0))
      throw SubscriberException("Event root geometry exceeds residual precision.");
   return result;
}
Number evaluate(const Polynomial &p,Number x)
{
   Number result=0;
   for (auto i=p.rbegin();i!=p.rend();++i) result=result*x+*i;
   return result;
}

// Isolate every real root on a bounded chart by derivative subdivision.
// Trigonometric polynomials below have degree at most four on each chart.
// No angular sampling grid can miss a close stationary-point pair.
Polynomial roots(Polynomial p)
{
   while (p.size()>1 && p.back()==0) p.pop_back();
   Number scale=0;
   for (const auto x:p) { finite(x); scale=std::max(scale,std::abs(x)); }
   if (p.size()<2 || scale==0) return {};
   for (auto &x:p) x/=scale;
   if (p.size()==2)
   {
      const Number root=-p[0]/p[1];
      return root>=-1 && root<=1 ? Polynomial{root} : Polynomial{};
   }
   Polynomial derivative;
   for (size_t i=1;i<p.size();++i) derivative.push_back(i*p[i]);
   auto divisions=roots(derivative);
   divisions.push_back(-1); divisions.push_back(1);
   std::sort(divisions.begin(),divisions.end());
   divisions.erase(std::unique(divisions.begin(),divisions.end()),divisions.end());
   Polynomial result;
   Number roundoff=0;
   for (const auto x:p) roundoff+=std::abs(x);
   roundoff*=64*std::numeric_limits<Number>::epsilon();
   for (const auto x:divisions)
      if (std::abs(evaluate(p,x))<=roundoff) result.push_back(x);
   for (size_t i=1;i<divisions.size();++i)
   {
      Number a=divisions[i-1],b=divisions[i];
      Number fa=evaluate(p,a),fb=evaluate(p,b);
      if (fa==0 || fb==0 || std::signbit(fa)==std::signbit(fb)) continue;
      for (int iteration=0;iteration<192;++iteration)
      {
         const Number mid=a+(b-a)/2;
         if (mid==a || mid==b) break;
         const Number fm=evaluate(p,mid);
         if (fm==0) {a=b=mid;break;}
         if (std::signbit(fm)==std::signbit(fa)) {a=mid;fa=fm;}
         else b=mid;
      }
      result.push_back(a+(b-a)/2);
   }
   return result;
}
void angles(const Trig &f,bool derivative,std::vector<Number> &result)
{
   for (int chart=0;chart<2;++chart)
   {
      const Number a=f[0],b=chart ? -f[1] : f[1],c=chart ? -f[2] : f[2];
      const Number d=f[3],e=f[4];
      const Polynomial p=derivative ? Polynomial{c+2*e,-2*b-8*d,-12*e,-2*b+8*d,-c+2*e} :
         Polynomial{a+b+d,2*c+4*e,2*a-6*d,2*c-4*e,a-b+d};
      for (const auto t:roots(p)) result.push_back(2*std::atan(t)+chart*pi);
   }
}
Number evaluate(const Trig &f,Number angle)
{
   const Number c=std::cos(angle),s=std::sin(angle);
   return f[0]+f[1]*c+f[2]*s+f[3]*(c*c-s*s)+f[4]*2*c*s;
}
std::pair<Number,Number> extremaOfMinimum(const std::vector<Trig> &functions)
{
   std::vector<Number> candidates{0,pi/2,pi,3*pi/2};
   for (size_t i=0;i<functions.size();++i)
   {
      angles(functions[i],true,candidates);
      for (size_t j=0;j<i;++j)
      {
         Trig difference;
         for (size_t k=0;k<5;++k) difference[k]=functions[i][k]-functions[j][k];
         angles(difference,false,candidates);
      }
   }
   Number minimum=std::numeric_limits<Number>::infinity(),maximum=-minimum;
   for (const auto angle:candidates)
   {
      Number value=std::numeric_limits<Number>::infinity();
      for (const auto &function:functions) value=std::min(value,evaluate(function,angle));
      finite(value); minimum=std::min(minimum,value); maximum=std::max(maximum,value);
   }
   return {minimum,maximum};
}

struct Appearance
{
   bool point=false;
   Vector center{},limb{},major{},minor{},radii{};
   Number rotation[3][3]{}; // body-fixed to J2000 at the body epoch
   Vector bodyVector(const Vector &v) const
   {
      Vector result{};
      for (int i=0;i<3;++i) for (int j=0;j<3;++j) result[i]+=rotation[j][i]*v[j];
      return result;
   }
};
Appearance appearance(const std::string &body,const std::string &shape,
      const std::string &frame,const std::string &correction,
      const std::string &observer,Real epoch)
{
   Appearance result; result.point=keyword(shape)=="POINT";
   if (!result.point && keyword(shape)!="ELLIPSOID")
      throw SubscriberException("Brent event refinement requires POINT/ELLIPSOID geometry.");
   SpiceDouble position[3],lightTime;
   spkpos_c(body.c_str(),epoch,"J2000",correction.c_str(),observer.c_str(),position,&lightTime);
   spiceError();
   for (int i=0;i<3;++i) {finite(position[i]);result.center[i]=position[i];}
   if (result.point) return result;
   SpiceDouble radii[3],rotation[3][3]; SpiceInt count;
   bodvrd_c(body.c_str(),"RADII",3,&count,radii); spiceError();
   if (count!=3) throw SubscriberException("Event body needs three ellipsoid radii.");
   const Real bodyEpoch=correction=="NONE" ? epoch :
      epoch+(correction.front()=='X' ? lightTime : -lightTime);
   pxform_c(frame.c_str(),"J2000",bodyEpoch,rotation); spiceError();
   for (int i=0;i<3;++i)
   {
      finite(radii[i]);
      if (radii[i]<=0) throw SubscriberException("Event ellipsoid radii must be positive.");
      result.radii[i]=radii[i];
      for (int j=0;j<3;++j) {finite(rotation[i][j]);result.rotation[i][j]=rotation[i][j];}
   }
   const Vector viewpoint=scaled(result.bodyVector(result.center),-1);
   SpiceDouble view[3],center[3],major[3],minor[3];
   for (int i=0;i<3;++i) view[i]=static_cast<Real>(viewpoint[i]);
   SpiceEllipse limb;
   edlimb_c(radii[0],radii[1],radii[2],view,&limb); spiceError();
   el2cgv_c(&limb,center,major,minor); spiceError();
   result.limb=result.center;
   for (int i=0;i<3;++i) for (int j=0;j<3;++j)
   {
      result.limb[i]+=rotation[i][j]*static_cast<Number>(center[j]);
      result.major[i]+=rotation[i][j]*static_cast<Number>(major[j]);
      result.minor[i]+=rotation[i][j]*static_cast<Number>(minor[j]);
   }
   return result;
}
struct Cone
{
   const Appearance &body;
   Vector observer{}; Number observerNorm=0, radiusScale=0;
   explicit Cone(const Appearance &a) : body(a)
   {
      observer=scaled(body.bodyVector(body.center),-1);
      for (int i=0;i<3;++i) observer[i]/=body.radii[i];
      observerNorm=norm(observer); radiusScale=*std::max_element(body.radii.begin(),body.radii.end());
      finite(observerNorm); finite(radiusScale);
      if (!(observerNorm>1)) throw SubscriberException("Event observer must be outside the ellipsoid.");
   }
   Vector direction(const Vector &ray) const
   {
      Vector result=body.bodyVector(ray);
      for (int i=0;i<3;++i) result[i]*=radiusScale/body.radii[i];
      return result;
   }
   Number quadratic(const Vector &a,const Vector &b) const
   {
      const Vector x=direction(a),y=direction(b);
      // The cross-product form avoids subtracting two terms proportional
      // to observer distance squared when the rays are nearly aligned.
      return (dot(x,y)-dot(cross(observer,x),cross(observer,y)))/
         (observerNorm*observerNorm);
   }
   Number forward(const Vector &ray) const
   { return -dot(observer,direction(ray))/observerNorm; }
   Number value(const Vector &ray) const
   {
      const Number length=norm(ray);
      if (!(length>0)) throw SubscriberException("Event direction is undefined at the observer.");
      const Vector d=scaled(ray,1/length);
      return std::min(quadratic(d,d),forward(d));
   }
   std::pair<Number,Number> limbRange(const Appearance &target) const
   {
      const Number scale=std::max({norm(target.limb),norm(target.major),norm(target.minor)});
      if (!(scale>0)) throw SubscriberException("Event limb is degenerate.");
      const Vector p=scaled(target.limb,1/scale),u=scaled(target.major,1/scale),v=scaled(target.minor,1/scale);
      const Number uu=quadratic(u,u),vv=quadratic(v,v);
      const Trig q{quadratic(p,p)+(uu+vv)/2,2*quadratic(p,u),2*quadratic(p,v),(uu-vv)/2,quadratic(u,v)};
      const Trig f{forward(p),forward(u),forward(v),0,0};
      return extremaOfMinimum({q,f});
   }
};
struct Occultation
{
   std::string front,frontShape,frontFrame,back,backShape,backFrame,correction,observer;
   Real lastEpoch=std::numeric_limits<Real>::quiet_NaN();
   std::array<Real,3> margins{};
   const std::array<Real,3> &at(Real epoch)
   {
      if (epoch==lastEpoch) return margins;
      const auto f=appearance(front,frontShape,frontFrame,correction,observer,epoch);
      const auto b=appearance(back,backShape,backFrame,correction,observer,epoch);
      std::array<Real,3> values;
      if (f.point && b.point) throw SubscriberException("Two point targets have no occultation limb.");
      if (b.point) values.fill(real(Cone(f).value(b.center)));
      else if (f.point) values.fill(real(Cone(b).value(f.center)));
      else
      {
         const Cone fc(f),bc(b);
         const auto onBack=fc.limbRange(b),onFront=bc.limbRange(f);
         // Boundary intersection, or containment of the front axis in back.
         values[0]=real(std::max(onBack.second,bc.value(f.center)));
         values[1]=real(onBack.first); // back entirely within front's cone
         values[2]=real(onFront.first); // front entirely within back's cone
      }
      margins=values;lastEpoch=epoch;return margins;
   }
};

// All rays are expressed in the instrument frame at the observer epoch.
// The body's orientation within Appearance retains its light-time epoch.
void instrumentFrame(Appearance &body,const std::string &frame,Real epoch)
{
   SpiceDouble rotation[3][3]; pxform_c("J2000",frame.c_str(),epoch,rotation); spiceError();
   auto rotate=[&](const Vector &v) {
      Vector result{};
      for (int i=0;i<3;++i) for (int j=0;j<3;++j) result[i]+=rotation[i][j]*v[j];
      return result;
   };
   body.center=rotate(body.center);body.limb=rotate(body.limb);
   body.major=rotate(body.major);body.minor=rotate(body.minor);
   Number composed[3][3]{};
   for (int i=0;i<3;++i) for (int j=0;j<3;++j) for (int k=0;k<3;++k)
      composed[i][j]+=rotation[i][k]*body.rotation[k][j];
   for (int i=0;i<3;++i) for (int j=0;j<3;++j) body.rotation[i][j]=composed[i][j];
}

struct FieldOfView
{
   std::string shape,frame;
   Vector axis{},x{},y{},pointY{};
   Number a=0,b=0,orientation=1;
   std::vector<Vector> vertices;
   std::vector<std::array<Vector,3>> triangles;

   explicit FieldOfView(const std::string &instrument)
   {
      SpiceInt id=0,count=0;SpiceBoolean found=SPICEFALSE;
      bods2c_c(instrument.c_str(),&id,&found);spiceError();
      if (!found) throw SubscriberException("Cannot resolve the event instrument ID.");
      std::unique_ptr<SpiceDouble[][3]> bounds(new SpiceDouble[SPICE_GF_MAXVRT][3]);
      SpiceChar kind[128],reference[128];SpiceDouble bore[3];
      getfov_c(id,SPICE_GF_MAXVRT,sizeof(kind),sizeof(reference),kind,reference,bore,&count,bounds.get());
      spiceError();shape=keyword(kind);frame=reference;axis=unit({bore[0],bore[1],bore[2]});
      for (SpiceInt i=0;i<count;++i) vertices.push_back(unit({bounds[i][0],bounds[i][1],bounds[i][2]}));
      if (shape=="CIRCLE" || shape=="ELLIPSE")
      {
         if (count!=(shape=="CIRCLE" ? 1 : 2)) throw SubscriberException("Invalid event FOV axes.");
         auto projection=[&](const Vector &v) {
            const Number forward=dot(axis,v);
            if (!(forward>0)) throw SubscriberException("Event FOV axes must lie in the forward hemisphere.");
            return difference(scaled(v,1/forward),axis);
         };
         const Vector first=projection(vertices[0]);a=norm(first);x=unit(first);y=unit(cross(axis,x));
         b=shape=="CIRCLE" ? a : norm(projection(vertices[1]));
         pointY=shape=="CIRCLE" ? y : unit(projection(vertices[1]));
         if (!(a>0 && b>0)) throw SubscriberException("Event FOV axes must be nonzero.");
      }
      else if (shape=="POLYGON" || shape=="RECTANGLE") triangulate();
      else throw SubscriberException("Unsupported event field-of-view shape.");
   }

   void triangulate()
   {
      if (vertices.size()<3) throw SubscriberException("An event polygon FOV needs at least three vertices.");
      // Simple SPICE polygons can be concave. A spherical signed area chooses
      // the authored interior without selecting the antipodal winding region.
      Number area=0;
      for (size_t i=1;i+1<vertices.size();++i)
      {
         const auto &p=vertices[0],&q=vertices[i],&r=vertices[i+1];
         area+=2*std::atan2(dot(p,cross(q,r)),1+dot(p,q)+dot(q,r)+dot(r,p));
      }
      area=std::remainder(area,4*pi);
      if (area==0) throw SubscriberException("Event polygon FOV has no interior.");
      orientation=area>0 ? 1 : -1;
      std::vector<size_t> remaining;
      for (size_t i=0;i<vertices.size();++i)
         if (i==0 || norm(difference(vertices[i],vertices[i-1]))!=0) remaining.push_back(i);
      if (remaining.size()>1 && norm(difference(vertices[remaining.front()],vertices[remaining.back()]))==0)
         remaining.pop_back();
      while (remaining.size()>=3)
      {
         bool removed=false;
         for (size_t i=0;i<remaining.size();++i)
         {
            const size_t previous=remaining[(i+remaining.size()-1)%remaining.size()];
            const size_t current=remaining[i],next=remaining[(i+1)%remaining.size()];
            const auto &p=vertices[previous],&q=vertices[current],&r=vertices[next];
            const Number turn=orientation*dot(cross(p,q),r);
            // Remove redundant vertices on a minor arc, never a reflex turn.
            if (turn==0 && dot(cross(p,q),cross(q,r))>=0)
            {remaining.erase(remaining.begin()+i);removed=true;break;}
            if (!(turn>0)) continue;
            const std::array<Vector,3> edges{scaled(unit(cross(p,q)),orientation),
               scaled(unit(cross(q,r)),orientation),scaled(unit(cross(r,p)),orientation)};
            bool occupied=false;
            for (const auto index:remaining)
            {
               if (index==previous || index==current || index==next) continue;
               const auto &v=vertices[index];
               if (dot(edges[0],v)>=0 && dot(edges[1],v)>=0 && dot(edges[2],v)>=0)
               {occupied=true;break;}
            }
            if (occupied) continue;
            triangles.push_back(edges);remaining.erase(remaining.begin()+i);removed=true;break;
         }
         if (!removed) throw SubscriberException("Cannot triangulate the simple event polygon FOV.");
      }
      if (triangles.empty()) throw SubscriberException("Event polygon FOV has no triangles.");
   }

   Number polygonPoint(const Vector &ray) const
   {
      const Vector direction=unit(ray);
      Number winding=0,distance=pi;
      for (size_t i=0;i<vertices.size();++i)
      {
         const auto &p=vertices[i],&q=vertices[(i+1)%vertices.size()];
         const Vector edge=cross(p,q);
         if (norm(edge)==0) continue;
         winding+=std::atan2(dot(direction,edge),dot(p,q)-dot(p,direction)*dot(q,direction));
         distance=std::min({distance,angle(direction,p),angle(direction,q)});
         const Vector normal=unit(edge);
         const Number height=dot(direction,normal);
         const Vector projected=difference(direction,scaled(normal,height));
         if (norm(projected)>0)
         {
            const Vector closest=unit(projected);
            if (dot(cross(p,closest),normal)>=0 && dot(cross(closest,q),normal)>=0)
               distance=std::min(distance,angle(direction,closest));
         }
      }
      // Distance is to the real polygon boundary; triangulation diagonals
      // must not create false zeros for point targets inside a concave FOV.
      return orientation*winding>pi ? distance : -distance;
   }

   Number quadratic(const Vector &p,const Vector &q) const
   {return dot(axis,p)*dot(axis,q)-dot(x,p)*dot(x,q)/(a*a)-dot(y,p)*dot(y,q)/(b*b);}

   Number value(const Appearance &target) const
   {
      if (target.point)
      {
         if (shape=="POLYGON" || shape=="RECTANGLE") return polygonPoint(target.center);
         const Vector direction=unit(target.center);
         if (shape=="CIRCLE") return std::atan(a)-angle(axis,direction);
         // Match the point-FOV projection onto both authored semi-axis
         // directions. Extended FOVs use the orthogonal cone basis above.
         const Number forward=dot(axis,direction),px=dot(x,direction)/a,py=dot(pointY,direction)/b;
         return std::min(forward*forward-px*px-py*py,forward);
      }
      const Number scale=std::max({norm(target.limb),norm(target.major),norm(target.minor)});
      if (!(scale>0)) throw SubscriberException("Event FOV target limb is degenerate.");
      const Vector p=scaled(target.limb,1/scale),u=scaled(target.major,1/scale),v=scaled(target.minor,1/scale);
      const Cone cone(target);
      if (shape=="CIRCLE" || shape=="ELLIPSE")
      {
         const Number uu=quadratic(u,u),vv=quadratic(v,v);
         const Trig q{quadratic(p,p)+(uu+vv)/2,2*quadratic(p,u),2*quadratic(p,v),(uu-vv)/2,quadratic(u,v)};
         const Trig forward{dot(axis,p),dot(axis,u),dot(axis,v),0,0};
         return std::max(extremaOfMinimum({q,forward}).second,cone.value(axis));
      }
      Number result=-std::numeric_limits<Number>::infinity();
      for (const auto &triangle:triangles)
      {
         std::vector<Trig> constraints;
         for (const auto &normal:triangle)
            constraints.push_back({dot(normal,p),dot(normal,u),dot(normal,v),0,0});
         result=std::max(result,extremaOfMinimum(constraints).second);
      }
      // Boundary intersection or containment: if the whole polygon is inside
      // the target disk, no target-limb point need lie inside a triangle.
      for (const auto &vertex:vertices) result=std::max(result,cone.value(vertex));
      return result;
   }
};
}

namespace GmatEventRoot {
Real Refine(const Function &value,Real a,Real b,Real tolerance)
{
   if (!std::isfinite(tolerance) || tolerance<=0 ||
       !std::isfinite(a) || !std::isfinite(b) || !(a<=b))
      throw SubscriberException("Brent refinement needs a finite positive tolerance and ordered bracket.");
   Real fa=value(a),fb=value(b);
   if (!std::isfinite(fa) || !std::isfinite(fb)) throw SubscriberException("Brent residual is not finite.");
   if (fa==0) return a;
   if (fb==0) return b;
   if (std::signbit(fa)==std::signbit(fb)) throw SubscriberException("Brent refinement needs a sign-changing bracket.");
   Real c=a,fc=fa,d=b-a,e=d;
   for (Integer iteration=0;iteration<256;++iteration)
   {
      if (std::signbit(fb)==std::signbit(fc)) {c=a;fc=fa;d=e=b-a;}
      if (std::abs(fc)<std::abs(fb)) {const Real oldB=b,oldFb=fb;a=b;fa=fb;b=c;fb=fc;c=oldB;fc=oldFb;}
      const Real tol=0.5*tolerance+2*std::numeric_limits<Real>::epsilon()*std::abs(b);
      const Real half=(c-b)*0.5;
      if (fb==0 || std::abs(half)<=tol) return b;
      if (std::abs(e)>=tol && std::abs(fa)>std::abs(fb))
      {
         const Real ratio=fb/fa;Real numerator,denominator;
         if (a==c) {numerator=2*half*ratio;denominator=1-ratio;}
         else {const Real q=fa/fc,r=fb/fc;numerator=ratio*(2*half*q*(q-r)-(b-a)*(r-1));denominator=(q-1)*(r-1)*(ratio-1);}
         if (numerator>0) denominator=-denominator;else numerator=-numerator;
         const Real oldE=e;e=d;
         if (denominator!=0 && std::isfinite(numerator/denominator) &&
             2*numerator<std::min(3*half*denominator-std::abs(tol*denominator),std::abs(oldE*denominator))) d=numerator/denominator;
         else d=e=half;
      }
      else d=e=half;
      a=b;fa=fb;b+=std::abs(d)>tol ? d : std::copysign(tol,half);fb=value(b);
      if (!std::isfinite(fb)) throw SubscriberException("Brent residual is not finite.");
   }
   throw SubscriberException("Brent event root refinement did not converge.");
}

void RefineWindow(SpiceCell &result,SpiceCell &confinement,
                  const std::vector<Function> &functions,Real tolerance)
{
   if (!std::isfinite(tolerance) || tolerance<=0 || functions.empty())
      throw SubscriberException("Event root refinement needs geometry functions and a positive finite tolerance.");
   std::vector<std::pair<Real,Real>> domains,intervals;
   for (SpiceInt i=0;i<wncard_c(&confinement);++i) {Real a,b;wnfetd_c(&confinement,i,&a,&b);domains.emplace_back(a,b);}
   for (SpiceInt i=0;i<wncard_c(&result);++i) {Real a,b;wnfetd_c(&result,i,&a,&b);intervals.emplace_back(a,b);}
   spiceError();
   std::vector<Real> boundaries;
   for (const auto &interval:intervals) {boundaries.push_back(interval.first);boundaries.push_back(interval.second);}
   auto endpoint=[&](Real center,size_t index) {
      Real left=0,right=0;bool found=false;
      for (const auto &domain:domains)
      {
         if (center==domain.first || center==domain.second) return center; // clipped, not an event root
         if (center>domain.first && center<domain.second) {left=domain.first;right=domain.second;found=true;break;}
      }
      if (!found) throw SubscriberException("Event root lies outside its confinement window.");
      const Real spacing=std::nextafter(center,std::numeric_limits<Real>::infinity())-center;
      if (tolerance<2*spacing) throw SubscriberException("Event root tolerance is below SPICE epoch resolution.");
      const Real margin=std::max(64*static_cast<Real>(SPICE_GF_CNVTOL),64*spacing);
      left=std::max(left,center-margin);right=std::min(right,center+margin);
      if (index>0 && boundaries[index-1]<center) left=std::max(left,center-(center-boundaries[index-1])*0.5);
      if (index+1<boundaries.size() && boundaries[index+1]>center) right=std::min(right,center+(boundaries[index+1]-center)*0.5);
      bool refined=false;Real best=0;
      for (const auto &function:functions)
      {
         auto offsetValue=[&](Real offset) {const Real v=function(center+offset);if(!std::isfinite(v)) throw SubscriberException("Event residual is not finite.");return v;};
         if (offsetValue(0)==0) return center;
         const Real a=left-center,b=right-center,fa=offsetValue(a),fb=offsetValue(b);
         if (fa!=0 && fb!=0 && std::signbit(fa)==std::signbit(fb)) continue;
         const Real root=Refine(offsetValue,a,b,tolerance);
         if (!refined || std::abs(root)<std::abs(best)) {refined=true;best=root;}
      }
      if (!refined) throw SubscriberException("No continuous sign-changing geometry bracket for a Brent event boundary.");
      return center+best;
   };
   for (size_t i=0;i<intervals.size();++i)
   {
      intervals[i].first=endpoint(boundaries[2*i],2*i);
      intervals[i].second=endpoint(boundaries[2*i+1],2*i+1);
      if (intervals[i].first>intervals[i].second) throw SubscriberException("Brent refinement inverted an event interval.");
   }
   scard_c(0,&result);
   for (const auto &interval:intervals) {wninsd_c(interval.first,interval.second,&result);spiceError();}
}

void RefineOccultation(SpiceCell &result,SpiceCell &confinement,
      const std::string &type,const std::string &front,const std::string &frontShape,
      const std::string &frontFrame,const std::string &back,const std::string &backShape,
      const std::string &backFrame,const std::string &correction,const std::string &observer,Real tolerance)
{
   auto context=std::make_shared<Occultation>();
   context->front=front;context->frontShape=frontShape;context->frontFrame=frontFrame;
   context->back=back;context->backShape=backShape;context->backFrame=backFrame;context->observer=observer;
   context->correction=keyword(correction);
   const auto stellar=context->correction.find("+S");
   if (stellar!=std::string::npos) context->correction.erase(stellar); // gfoclt ignores stellar aberration
   const auto kind=keyword(type);
   std::vector<Function> functions;
   auto margin=[&](size_t i) {functions.push_back([context,i](Real epoch) {return context->at(epoch)[i];});};
   if (kind=="ANY") margin(0);
   else if (kind=="FULL") margin(1);
   else if (kind=="ANNULAR") margin(2);
   else if (kind=="PARTIAL") {margin(0);margin(1);margin(2);}
   else throw SubscriberException("Unknown occultation type for Brent refinement.");
   RefineWindow(result,confinement,functions,tolerance);
}

void RefineLatitude(SpiceCell &result,SpiceCell &confinement,
      const std::string &target,const std::string &frame,const std::string &correction,
      const std::string &observer,Real latitude,Real tolerance,bool maximum)
{
   auto value=[&](Real epoch) {
      SpiceDouble state[6],lightTime;
      if (maximum)
         spkezr_c(target.c_str(),epoch,frame.c_str(),correction.c_str(),observer.c_str(),state,&lightTime);
      else
         spkpos_c(target.c_str(),epoch,frame.c_str(),correction.c_str(),observer.c_str(),state,&lightTime);
      spiceError();
      const Number scale=std::max({std::abs(state[0]),std::abs(state[1]),std::abs(state[2])});
      if (!(scale>0)) throw SubscriberException("Event latitude is undefined at the observer.");
      const Number x=state[0]/scale,y=state[1]/scale,z=state[2]/scale;
      if (maximum)
         return real((x*x+y*y)*(state[5]/scale)-z*(x*(state[3]/scale)+y*(state[4]/scale)));
      return real(std::atan2(z,std::hypot(x,y))-latitude);
   };
   RefineWindow(result,confinement,{value},tolerance);
}

void RefineFieldOfView(SpiceCell &result,SpiceCell &confinement,
      const std::string &instrument,const std::string &target,const std::string &shape,
      const std::string &frame,const std::string &correction,const std::string &observer,Real tolerance)
{
   if (wncard_c(&result)==0) {spiceError();return;}
   const FieldOfView fov(instrument);
   const std::string aberration=keyword(correction);
   auto value=[&](Real epoch) {
      auto body=appearance(target,shape,frame,aberration,observer,epoch);
      instrumentFrame(body,fov.frame,epoch);
      return real(fov.value(body));
   };
   RefineWindow(result,confinement,{value},tolerance);
}

void RefinePhase(SpiceCell &result,SpiceCell &confinement,
      const std::string &target,const std::string &illuminator,const std::string &observer,
      const std::string &correction,Real phase,Real tolerance)
{
   auto value=[&](Real epoch) {
      const Real angle=phaseq_c(epoch,target.c_str(),illuminator.c_str(),observer.c_str(),correction.c_str());
      spiceError();return angle-phase;
   };
   RefineWindow(result,confinement,{value},tolerance);
}
}
#endif
