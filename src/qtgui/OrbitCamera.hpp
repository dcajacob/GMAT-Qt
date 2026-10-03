#pragma once
#include "PlotModel.hpp"
#include <osg/Vec3d>
#include <osg/Quat>
#include <cmath>
#include <algorithm>

struct OrbitCameraBasis
{
   osg::Vec3d target,right,up,outward;
   double distance,extent;
};
struct OrbitSceneBounds
{
   osg::Vec3d minimum,maximum;
   bool empty=true;
   void include(double x,double y,double z,double radius) {
      if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(radius)) return;
      const osg::Vec3d point(x,y,z),padding(std::max(0.0,radius),std::max(0.0,radius),std::max(0.0,radius));
      if (empty) { minimum=point-padding; maximum=point+padding; empty=false; return; }
      for (int i=0;i<3;++i) { minimum[i]=std::min(minimum[i],point[i]-padding[i]); maximum[i]=std::max(maximum[i],point[i]+padding[i]); }
   }
   osg::Vec3d center() const { return (minimum+maximum)*.5; }
   double radius() const { return std::max(1e-6,(maximum-minimum).length()*.5); }
};
struct OrbitObjectBounds { osg::Vec3d center; double radius=1; };
inline OrbitCameraBasis orbitCamera(const PlotModel &model,quint64 frame,double yaw,double pitch,double extent,double aspect=1.0,const OrbitSceneBounds *bounds=nullptr,const QMap<QString,OrbitObjectBounds> *objects=nullptr)
{
   double viewExtent=extent*1.1,distance=extent*4;
   osg::Vec3d target;
   osg::Vec3d right(std::cos(yaw),-std::sin(yaw),0);
   osg::Vec3d up(std::cos(pitch)*std::sin(yaw),std::cos(pitch)*std::cos(yaw),-std::sin(pitch));
   osg::Vec3d outward=right^up;
   osg::Vec3d automaticOrigin,automaticRight(1,0,0),automaticUp(0,0,1),automaticOutward(0,-1,0);
   const PlotCameraReset *reset=nullptr;
   if (model.resetCamera) {
      if (model.selectedCamera>0 && model.selectedCamera<model.cameraViews.size()) {
         const auto &view=model.cameraViews[model.selectedCamera]; if (view.reset) reset=&*view.reset;
      } else if (model.resetView) reset=&*model.resetView;
   }
   const PlotCamera *camera=nullptr;
   const auto &history=reset ? reset->cameras : model.selectedCamera>0 && model.selectedCamera<model.cameraViews.size()
      ? model.cameraViews[model.selectedCamera].cameras : model.cameras;
   if (model.scriptedCamera) for (const auto &sample:history) if (sample.frame<=frame) camera=&sample;
   if (camera) {
      target.set(camera->target[0],camera->target[1],camera->target[2]);
      outward.set(camera->eye[0]-target.x(),camera->eye[1]-target.y(),camera->eye[2]-target.z());
      distance=outward.normalize();
      up.set(camera->up[0],camera->up[1],camera->up[2]); up.normalize();
      right=up^outward;
      if (right.normalize()<1e-9) {
         // A parallel up vector has no unique roll. Choose a stable axis.
         up=std::abs(outward.z())<.9 ? osg::Vec3d(0,0,1) : osg::Vec3d(0,1,0);
         right=up^outward; right.normalize();
      }
      up=outward^right;
      automaticOrigin=target; automaticRight=right; automaticUp=up; automaticOutward=outward;
      const osg::Quat azimuth(-yaw,up);
      outward=azimuth*outward; right=azimuth*right;
      const osg::Quat elevation(-pitch,right);
      outward=elevation*outward; up=elevation*up;
      viewExtent=model.fitCamera ? (extent+target.length())*1.1 : distance*.4663076581549986;
      // Orthographic controls retain their established feel. Scripted
      // distance sets framing; pan/rotate are offsets from the tracked view.
      if (model.fitCamera) distance=4*(extent+target.length());
   }
   if (model.fitCamera && bounds && !bounds->empty) {
      target=bounds->center(); extent=bounds->radius();
      viewExtent=extent*1.1; distance=extent*4;
   }
   if (model.perspective && model.fitCamera) {
      const double halfVertical=std::clamp(model.fieldOfView,1.0,150.0)*3.14159265358979323846/360;
      const double halfHorizontal=std::atan(std::max(aspect,1e-6)*std::tan(halfVertical));
      const double radius=bounds && !bounds->empty ? extent : extent+target.length();
      distance=radius*1.1/std::sin(std::min(halfVertical,halfHorizontal));
   }
   const auto trajectory=reset ? reset->automaticTrajectory : model.selectedCamera>0 && model.selectedCamera<model.cameraViews.size()
      ? model.cameraViews[model.selectedCamera].automaticTrajectory : model.automaticTrajectory;
   const auto automaticRadius=reset ? reset->automaticRadius : model.selectedCamera>0 && model.selectedCamera<model.cameraViews.size()
      ? model.cameraViews[model.selectedCamera].automaticRadius : model.automaticRadius;
   const auto body=reset ? reset->automaticBody : model.selectedCamera>0 && model.selectedCamera<model.cameraViews.size()
      ? model.cameraViews[model.selectedCamera].automaticBody : model.automaticBody;
   const auto segment=reset ? reset->segmentFrame : model.selectedCamera>0 && model.selectedCamera<model.cameraViews.size()
      ? model.cameraViews[model.selectedCamera].segmentFrame : model.segmentFrame;
   if (!model.fitCamera && (segment.isEmpty() || camera) && (!trajectory.isEmpty() || !body.isEmpty())) {
      OrbitSceneBounds path;
      OrbitObjectBounds objectBound;
      if (!body.isEmpty()) {
         objectBound.radius=automaticRadius>0 ? automaticRadius : 1;
         if (!segment.isEmpty()) objectBound={osg::Vec3d(),1};
         else if (objects && objects->contains(body)) objectBound=objects->value(body);
         else for (const auto &curve:model.curves) if (curve.name==body) objectBound.radius=curve.radius>0 ? curve.radius : 1;
         path.include(objectBound.center.x(),objectBound.center.y(),objectBound.center.z(),0);
      }
      if (trajectory=="CoordinateSystem") path.include(0,0,0,0);
      for (const auto &curve:model.curves) if (curve.name==trajectory)
         for (const auto &point:curve.points) path.include(point.x,point.y,point.z,0);
      if (!path.empty) {
         const auto center=path.center();
         target=automaticOrigin+automaticRight*center.x()-automaticOutward*center.y()+automaticUp*center.z();
         const double span=(path.maximum-path.minimum).length();
         const double radius=!body.isEmpty() ? std::max(objectBound.radius,1e-6) : automaticRadius>0 ? automaticRadius : span>0 ? span*.5 : 1.0;
         distance=2*radius; viewExtent=radius;
         if (model.perspective) {
            const double halfVertical=std::clamp(model.fieldOfView,1.0,150.0)*3.14159265358979323846/360;
            const double halfHorizontal=std::atan(std::max(aspect,1e-6)*std::tan(halfVertical));
            distance=radius/std::sin(std::min(halfVertical,halfHorizontal));
         }
         right=automaticRight; up=automaticUp; outward=automaticOutward;
         const osg::Quat azimuth(-yaw,up); outward=azimuth*outward; right=azimuth*right;
         const osg::Quat elevation(-pitch,right); outward=elevation*outward; up=elevation*up;
      }
   }
   return {target,right,up,outward,distance,viewExtent};
}
