#pragma once
#include "OrbitCamera.hpp"
#include <osg/Quat>
#include <optional>

struct OrbitVectorArrow
{
   osg::Vec3d start,end;
   std::array<osg::Vec3d,4> arrow;
};
inline const PlotVectorSample *orbitVectorSample(const PlotVector &vector,quint64 frame)
{
   if (vector.replaySample && vector.replaySample->source.frame==frame)
      return vector.replaySample->valid ? &*vector.replaySample : nullptr;
   const PlotVectorSample *sample=nullptr;
   for (const auto &item:vector.samples) if (item.source.frame<=frame) sample=&item;
   return sample && sample->valid ? sample : nullptr;
}
// Both renderers use only retained samples and loaded asset bounds. No engine
// object survives here; replay remains valid after a script is replaced.
inline std::optional<OrbitVectorArrow> orbitVectorArrow(const PlotModel &model,const PlotVector &vector,
      const PlotVectorSample &sample,const QMap<QString,OrbitObjectBounds> *objects=nullptr)
{
   if (!sample.valid || (!vector.bodyFixed && vector.source==vector.destination)) return {};
   for (const auto &curve:model.curves) if (curve.name==vector.source && !curve.visible) return {};
   const auto &pose=sample.source;
   const osg::Vec3d origin(pose.x,pose.y,pose.z);
   auto body=[&](const osg::Vec3d &value) {
      return osg::Vec3d(pose.bodyToView[0]*value.x()+pose.bodyToView[1]*value.y()+pose.bodyToView[2]*value.z(),
         pose.bodyToView[3]*value.x()+pose.bodyToView[4]*value.y()+pose.bodyToView[5]*value.z(),
         pose.bodyToView[6]*value.x()+pose.bodyToView[7]*value.y()+pose.bodyToView[8]*value.z());
   };
   auto inverseBody=[&](const osg::Vec3d &value) {
      return osg::Vec3d(pose.bodyToView[0]*value.x()+pose.bodyToView[3]*value.y()+pose.bodyToView[6]*value.z(),
         pose.bodyToView[1]*value.x()+pose.bodyToView[4]*value.y()+pose.bodyToView[7]*value.z(),
         pose.bodyToView[2]*value.x()+pose.bodyToView[5]*value.y()+pose.bodyToView[8]*value.z());
   };
   const auto bound=objects && objects->contains(vector.source) ? objects->value(vector.source) : OrbitObjectBounds{{},sample.sourceRadius};
   const double radius=std::isfinite(bound.radius) && bound.radius>0 ? bound.radius : 1;
   const double length=vector.automaticLength ? radius : vector.length;
   osg::Vec3d direction;
   if (vector.bodyFixed) direction.set(vector.direction[0],vector.direction[1],vector.direction[2]);
   else direction=inverseBody(osg::Vec3d(sample.destination[0],sample.destination[1],sample.destination[2])-origin);
   if (!std::isfinite(direction.length()) || direction.normalize()<1e-12 || !std::isfinite(length) || length<=0) return {};
   osg::Quat alignment; alignment.makeRotate(osg::Vec3d(1,0,0),direction);
   const auto along=body(alignment*osg::Vec3d(1,0,0));
   OrbitVectorArrow result;
   // OF's Relative Position vector points along local +X after a shortest
   // rotation in the source body frame. Its shaft starts outside source bounds.
   result.start=origin+(vector.bodyFixed ? body(osg::Vec3d(vector.start[0],vector.start[1],vector.start[2])) : body(alignment*(bound.center+osg::Vec3d(radius,0,0))));
   result.end=result.start+along*length;
   for (int wing=0;wing<4;++wing) {
      const auto transverse=body(alignment*(wing<2 ? osg::Vec3d(0,1,0) : osg::Vec3d(0,0,1)));
      result.arrow[wing]=result.end-along*(length*.2)+transverse*(length*.07*(wing%2 ? -1 : 1));
   }
   auto usable=[](const osg::Vec3d &value) { return PlotModel::usableOrbitPosition(value.x(),value.y(),value.z()); };
   if (!usable(result.start) || !usable(result.end)) return {};
   for (const auto &wing:result.arrow) if (!usable(wing)) return {};
   return result;
}
inline std::optional<OrbitVectorArrow> orbitVectorArrow(const PlotModel &model,const PlotVector &vector,quint64 frame,
      const QMap<QString,OrbitObjectBounds> *objects=nullptr)
{
   const auto *sample=orbitVectorSample(vector,frame);
   return sample ? orbitVectorArrow(model,vector,*sample,objects) : std::optional<OrbitVectorArrow>{};
}
