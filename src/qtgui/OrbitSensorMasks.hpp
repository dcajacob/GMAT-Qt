#pragma once
#include "OrbitCamera.hpp"
#include "OrbitObjectAxes.hpp"
#include <optional>

struct OrbitSensorMaskGeometry
{
   osg::Vec3d origin,end;
   QVector<osg::Vec3d> boundary;
};
// The receiver freezes the hardware mount and FOV boundary while engine objects
// exist. Rendering and replay use only that snapshot and the retained body pose.
inline std::optional<OrbitSensorMaskGeometry> orbitSensorMaskGeometry(const PlotModel &model,
      const PlotSensorMask &mask,const PlotPoint &pose,const QMap<QString,OrbitObjectBounds> *objects=nullptr)
{
   const PlotCurve *source=nullptr;
   for (const auto &curve:model.curves) if (curve.name==mask.source) { source=&curve; break; }
   if (!source || !source->visible || mask.boundary.size()<3 ||
       !PlotModel::usableOrbitPosition(pose.x,pose.y,pose.z)) return {};
   for (const double value:pose.bodyToView) if (!std::isfinite(value)) return {};
   const double radius=objects && objects->contains(mask.source) ? objects->value(mask.source).radius : mask.sourceRadius;
   const double length=mask.automaticLength ? (std::isfinite(radius) && radius>0 ? radius : 1) : mask.length;
   if (!std::isfinite(length) || length<=0) return {};
   auto position=[&](const std::array<double,3> &direction,double scale) {
      osg::Vec3d body;
      const double originScale=mask.originInMetres ? .001 : 1.;
      for (int i=0;i<3;++i) body[i]=mask.origin[i]*originScale+direction[i]*scale;
      const auto &r=pose.bodyToView;
      return osg::Vec3d(pose.x+r[0]*body.x()+r[1]*body.y()+r[2]*body.z(),
         pose.y+r[3]*body.x()+r[4]*body.y()+r[5]*body.z(),
         pose.z+r[6]*body.x()+r[7]*body.y()+r[8]*body.z());
   };
   OrbitSensorMaskGeometry result;
   result.origin=position({},0); result.end=position(mask.axis,length);
   for (const auto &vertex:mask.boundary) result.boundary.append(position(vertex,length));
   auto usable=[](const osg::Vec3d &p) { return PlotModel::usableOrbitPosition(p.x(),p.y(),p.z()); };
   if (!usable(result.origin) || !usable(result.end)) return {};
   for (const auto &point:result.boundary) if (!usable(point)) return {};
   return result;
}
inline std::optional<OrbitSensorMaskGeometry> orbitSensorMaskGeometry(const PlotModel &model,
      const PlotSensorMask &mask,quint64 frame,const QMap<QString,OrbitObjectBounds> *objects=nullptr)
{
   for (const auto &curve:model.curves) if (curve.name==mask.source) {
      const auto *pose=orbitObjectPose(curve,frame);
      return pose ? orbitSensorMaskGeometry(model,mask,*pose,objects) : std::optional<OrbitSensorMaskGeometry>{};
   }
   return {};
}
