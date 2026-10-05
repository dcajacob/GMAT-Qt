#pragma once
#include "PlotModel.hpp"
#include <osg/Vec3d>
#include <array>

struct OrbitObjectAxis
{
   osg::Vec3d start,end;
   std::array<osg::Vec3d,4> arrow;
};
// Use the retained pose only: replay never consults an engine object.
inline const PlotPoint *orbitObjectPose(const PlotCurve &curve,quint64 frame)
{
   if (curve.simulationActive && frame==std::numeric_limits<quint64>::max()) return curve.simulationPose ? &*curve.simulationPose : nullptr;
   if (curve.replayPose && curve.replayPose->frame==frame) return &*curve.replayPose;
   const PlotPoint *pose=nullptr;
   for (const auto &point:curve.points) if (point.frame<=frame) pose=&point;
   return pose;
}
inline double orbitObjectAxisRadius(const PlotCurve &curve,double modelRadius=0)
{
   return curve.showObject && modelRadius>0 ? modelRadius : curve.radius>0 ? curve.radius : 1;
}
inline std::array<OrbitObjectAxis,3> orbitObjectAxes(const PlotPoint &pose,double radius)
{
   const osg::Vec3d origin(pose.x,pose.y,pose.z);
   std::array<osg::Vec3d,3> directions;
   for (int axis=0;axis<3;++axis)
      directions[axis]={pose.bodyToView[axis],pose.bodyToView[3+axis],pose.bodyToView[6+axis]};
   std::array<OrbitObjectAxis,3> axes;
   for (int axis=0;axis<3;++axis) {
      auto &value=axes[axis]; const auto direction=directions[axis]*radius;
      // OF places each axis at one radius, then extends it by one radius.
      value.start=origin+direction; value.end=origin+direction*2;
      for (int wing=0;wing<4;++wing)
         value.arrow[wing]=value.end-direction*.2+directions[(axis+1+wing/2)%3]*(radius*.07*(wing%2 ? -1 : 1));
   }
   return axes;
}
