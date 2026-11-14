#pragma once
#include "OrbitObjectAxes.hpp"
#include <vector>
#include <cmath>
#include <limits>

struct OrbitGuideLine { osg::Vec3d start,end; QColor color; double width=1; };
struct OrbitGuideTriangle { std::array<osg::Vec3d,3> points; QColor color; };
struct OrbitObjectGuides { std::vector<OrbitGuideLine> lines; std::vector<OrbitGuideTriangle> triangles; };
// Match OFSpaceObject: a visible model's bound, a celestial equatorial radius,
// or the plain/missing-model reference frame's 100 km radius. No engine reads.
inline double orbitObjectGuideRadius(const PlotCurve &curve,double modelRadius=0)
{
   return curve.showObject && std::isfinite(modelRadius) && modelRadius>0 ? modelRadius :
      std::isfinite(curve.radius) && curve.radius>0 ? curve.radius : 100;
}
inline osg::Vec3d orbitObjectGuidePosition(const PlotPoint &pose,const osg::Vec3d &local)
{
   const auto &r=pose.bodyToView;
   return {pose.x+r[0]*local.x()+r[1]*local.y()+r[2]*local.z(),
      pose.y+r[3]*local.x()+r[4]*local.y()+r[5]*local.z(),
      pose.z+r[6]*local.x()+r[7]*local.y()+r[8]*local.z()};
}
inline OrbitObjectGuides orbitObjectGuides(const PlotCurve &curve,const PlotPoint &pose,double radius,bool celestialGrid=false)
{
   OrbitObjectGuides guides;
   constexpr double pi=3.14159265358979323846;
   if (!std::isfinite(radius) || radius<=0 || radius>std::numeric_limits<float>::max()/15 ||
       !PlotModel::usableOrbitPosition(pose.x,pose.y,pose.z)) return guides;
   for (const auto value:pose.bodyToView) if (!std::isfinite(value)) return guides;
   auto line=[&](const osg::Vec3d &a,const osg::Vec3d &b,QColor color,double width=1) {
      guides.lines.push_back({orbitObjectGuidePosition(pose,a),orbitObjectGuidePosition(pose,b),color,width});
   };
   // The OrbitView Grid option is a body-fixed latitude/longitude overlay.
   // Per-object OpenFrames guides retain their independent selection.
   if (curve.objectGrid || (celestialGrid && curve.showObject && curve.radius>0)) {
      const QColor black=QColor::fromRgbF(0,0,0,.9);
      // Spherical latitudes/longitudes spaced pi/8, independent of the mesh's
      // own model rotation/offset and the planet's flattening.
      auto point=[&](double lat,double lon) { return osg::Vec3d(radius*std::cos(lat)*std::cos(lon),radius*std::cos(lat)*std::sin(lon),radius*std::sin(lat)); };
      for (int lat=-3;lat<=3;++lat) for (int part=0;part<128;++part)
         line(point(lat*pi/8,part*pi/64),point(lat*pi/8,(part+1)*pi/64),black,lat==0 ? 3 : 1);
      for (int lon=0;lon<16;++lon) for (int part=0;part<64;++part)
         line(point(-pi/2+part*pi/64,lon*pi/8),point(-pi/2+(part+1)*pi/64,lon*pi/8),black,lon==0 ? 3 : 1);
   }
   if (curve.objectXYPlane) {
      const QColor color=pose.color; QColor fill=color; fill.setAlphaF(.2);
      auto point=[](double r,double angle) { return osg::Vec3d(r*std::cos(angle),r*std::sin(angle),0); };
      // OF's radial plane has radius 15R, rings every R and 12 spokes.
      for (int ring=1;ring<=15;++ring) for (int part=0;part<128;++part)
         line(point(radius*ring,part*pi/64),point(radius*ring,(part+1)*pi/64),color);
      for (int spoke=0;spoke<12;++spoke) line({},point(15*radius,spoke*pi/6),color,spoke==0 ? 3 : 1);
      const auto origin=orbitObjectGuidePosition(pose,{});
      for (int part=0;part<128;++part) guides.triangles.push_back({{origin,
         orbitObjectGuidePosition(pose,point(15*radius,part*pi/64)),
         orbitObjectGuidePosition(pose,point(15*radius,(part+1)*pi/64))},fill});
   }
   auto usable=[](const osg::Vec3d &p) { return PlotModel::usableOrbitPosition(p.x(),p.y(),p.z()); };
   for (const auto &edge:guides.lines) if (!usable(edge.start) || !usable(edge.end)) return {};
   for (const auto &triangle:guides.triangles) for (const auto &point:triangle.points) if (!usable(point)) return {};
   return guides;
}
