#pragma once
#include "PlotModel.hpp"
#include <QQuaternion>
#include <algorithm>
#include <cmath>

// These helpers create presentation-only samples; retained engine output is
// never edited or resampled. Callers establish arc/availability continuity.
inline std::array<double,9> plotInterpolatedRotation(
      const std::array<double,9> &from,const std::array<double,9> &to,double fraction)
{
   QMatrix3x3 a,b;
   for (int row=0;row<3;++row) for (int col=0;col<3;++col) {
      a(row,col)=static_cast<float>(from[row*3+col]);
      b(row,col)=static_cast<float>(to[row*3+col]);
   }
   const auto matrix=QQuaternion::slerp(QQuaternion::fromRotationMatrix(a),
      QQuaternion::fromRotationMatrix(b),static_cast<float>(fraction)).normalized().toRotationMatrix();
   std::array<double,9> result;
   for (int row=0;row<3;++row) for (int col=0;col<3;++col) result[row*3+col]=matrix(row,col);
   return result;
}

inline PlotPoint plotInterpolatedPoint(const PlotPoint &from,const PlotPoint &to,
      double fraction,PlotModel::Kind kind)
{
   const double t=std::clamp(fraction,0.0,1.0);
   if (t==0) return from;
   if (t==1) return to;
   auto blend=[t](double a,double b) { return a+(b-a)*t; };
   PlotPoint result=from;
   result.x=kind==PlotModel::Kind::GroundTrack
      ? std::remainder(from.x+std::remainder(to.x-from.x,360.0)*t,360.0)
      : blend(from.x,to.x);
   result.y=blend(from.y,to.y); result.z=blend(from.z,to.z);
   result.epoch=blend(from.epoch,to.epoch);
   result.high=blend(from.high,to.high); result.low=blend(from.low,to.low);
   result.bodyToView=plotInterpolatedRotation(from.bodyToView,to.bodyToView,t);
   result.inertialToView=plotInterpolatedRotation(from.inertialToView,to.inertialToView,t);
   result.hasSun=from.hasSun && to.hasSun;
   result.hasVelocity=from.hasVelocity && to.hasVelocity;
   for (int axis=0;axis<3;++axis) {
      result.sunPosition[axis]=blend(from.sunPosition[axis],to.sunPosition[axis]);
      result.viewVelocity[axis]=blend(from.viewVelocity[axis],to.viewVelocity[axis]);
   }
   result.connect=to.connect;
   result.highlighted=false;
   return result;
}

inline PlotVectorSample plotInterpolatedVector(const PlotVectorSample &from,
      const PlotVectorSample &to,double fraction)
{
   const double t=std::clamp(fraction,0.0,1.0);
   if (t==0) return from;
   if (t==1) return to;
   PlotVectorSample result=from;
   result.source=plotInterpolatedPoint(from.source,to.source,t,PlotModel::Kind::Orbit);
   result.valid=from.valid && to.valid;
   for (int axis=0;axis<3;++axis)
      result.destination[axis]=from.destination[axis]+(to.destination[axis]-from.destination[axis])*t;
   result.sourceRadius=from.sourceRadius+(to.sourceRadius-from.sourceRadius)*t;
   return result;
}

// View the retained path through the selected frame plus its interpolated tip.
// This range allocates no history copy and leaves exports/data admission intact.
class PlotDisplayedPoints
{
   const PlotCurve &curve;
   quint64 through;
public:
   PlotDisplayedPoints(const PlotCurve &value,quint64 frame) : curve(value),through(frame) {}
   class Iterator {
      const PlotCurve *curve;
      quint64 through;
      std::deque<PlotPoint>::const_iterator point;
      bool tip=false,done=false;
      void finishRaw() {
         if (point==curve->points.end() || point->frame>through) {
            tip=curve->replayPose && curve->replayPose->frame==through;
            done=!tip;
         }
      }
   public:
      Iterator(const PlotCurve &value,quint64 frame,bool end=false)
         : curve(&value),through(frame),point(value.points.begin()),done(end) { if (!done) finishRaw(); }
      const PlotPoint &operator*() const { return tip ? *curve->replayPose : *point; }
      Iterator &operator++() { if (tip) done=true; else { ++point; finishRaw(); } return *this; }
      bool operator!=(const Iterator &other) const { return done!=other.done; }
   };
   Iterator begin() const { return {curve,through}; }
   Iterator end() const { return {curve,through,true}; }
};
inline PlotDisplayedPoints plotDisplayedPoints(const PlotCurve &curve,quint64 through) { return {curve,through}; }

inline void plotSetReplayCursor(PlotModel &model,std::optional<PlotReplayCursor> cursor)
{
   model.replayCursor=cursor;
   for (auto &curve:model.curves) {
      curve.replayPose.reset();
      if (!cursor || cursor->fraction<=0) continue;
      const PlotPoint *before=nullptr,*after=nullptr;
      for (const auto &point:curve.points) {
         if (point.frame==cursor->before) before=&point;
         if (point.frame==cursor->after) { after=&point; break; }
      }
      if (before && after && after->connect && before->solver==after->solver && after->epoch>before->epoch)
         curve.replayPose=plotInterpolatedPoint(*before,*after,cursor->fraction,model.kind);
   }
   for (auto &vector:model.vectors) {
      vector.replaySample.reset();
      if (!cursor || cursor->fraction<=0) continue;
      const PlotVectorSample *before=nullptr,*after=nullptr;
      for (const auto &sample:vector.samples) {
         if (sample.source.frame==cursor->before) before=&sample;
         if (sample.source.frame==cursor->after) { after=&sample; break; }
      }
      bool continuous=false;
      for (const auto &curve:model.curves) if (curve.name==vector.source && curve.replayPose) continuous=true;
      if (continuous && before && after && before->valid && after->valid && before->source.solver==after->source.solver)
         vector.replaySample=plotInterpolatedVector(*before,*after,cursor->fraction);
   }
}

inline std::optional<PlotCamera> plotInterpolatedCamera(const std::deque<PlotCamera> &history,const PlotReplayCursor &cursor)
{
   const PlotCamera *before=nullptr,*after=nullptr;
   for (const auto &camera:history) {
      if (camera.frame==cursor.before) before=&camera;
      if (camera.frame==cursor.after) after=&camera;
   }
   if (!before || !after || before->solver!=after->solver || cursor.fraction<=0) return {};
   PlotCamera result=*before;
   for (int axis=0;axis<3;++axis) {
      result.eye[axis]+=(after->eye[axis]-result.eye[axis])*cursor.fraction;
      result.target[axis]+=(after->target[axis]-result.target[axis])*cursor.fraction;
      result.up[axis]+=(after->up[axis]-result.up[axis])*cursor.fraction;
   }
   return result;
}
