#pragma once
#include "PlotInterpolation.hpp"
#include "CameraAlignment.hpp"
#include <limits>
#include <tuple>

// Absolute simulation-time playback selects each object's trajectory separately.
// OpenFrames keeps a containing trajectory sticky, otherwise chooses the first
// inserted containing trajectory, or the closest trajectory with LIMIT clamping.
// These presentation helpers never alter the retained samples or call the engine.
namespace PlotEpochPlayback {
struct Arc {
   quint64 id=0;
   bool regular=false;
   QVector<const PlotPoint *> points;
   double minimum=0,maximum=0;
   double distance(double epoch) const { return std::max({minimum-epoch,0.0,epoch-maximum}); }
};
inline void finish(Arc &arc)
{
   arc.minimum=std::numeric_limits<double>::max(); arc.maximum=-arc.minimum;
   for (const auto *point:arc.points) { arc.minimum=std::min(arc.minimum,point->epoch); arc.maximum=std::max(arc.maximum,point->epoch); }
}
inline QVector<Arc> arcs(const PlotModel &model,const PlotCurve &curve)
{
   QVector<Arc> result;
   const auto regular=model.regularArcs.constFind(curve.name);
   QSet<quint64> covered;
   if (regular!=model.regularArcs.cend()) for (const auto &source:*regular) {
      Arc arc; arc.id=source.id; arc.regular=true;
      auto belongs=[&](const PlotPoint &point) { return !point.solver && (point.regularArc==source.id || source.members.contains(point.regularArc)); };
      for (const auto &point:curve.points) if (belongs(point)) { arc.points.append(&point); covered.insert(point.frame); }
      // Frozen raw endpoints survive collection and bounded point history. Add
      // only endpoints not already represented by a retained publication.
      for (const auto *endpoint:{&source.first,&source.last}) {
         if (!PlotModel::usableValue(endpoint->epoch) || !PlotModel::usableOrbitPosition(endpoint->x,endpoint->y,endpoint->z)) continue;
         bool present=false; for (const auto *point:arc.points) if (point->epoch==endpoint->epoch) { present=true; break; }
         if (!present) arc.points.append(endpoint);
      }
      if (arc.points.isEmpty()) continue;
      bool reverse=source.last.epoch<source.first.epoch;
      for (size_t i=1;i<curve.points.size();++i) if (belongs(curve.points[i-1]) && belongs(curve.points[i]) && curve.points[i].epoch!=curve.points[i-1].epoch) {
         reverse=curve.points[i].epoch<curve.points[i-1].epoch; break;
      }
      if (arc.points.size()==2 && arc.points[0]->frame!=arc.points[1]->frame)
         reverse=(arc.points[0]->epoch>arc.points[1]->epoch)==(arc.points[0]->frame<arc.points[1]->frame);
      std::stable_sort(arc.points.begin(),arc.points.end(),[reverse](const PlotPoint *a,const PlotPoint *b) { return reverse ? a->epoch>b->epoch : a->epoch<b->epoch; });
      finish(arc); result.append(arc);
   }
   if (!result.isEmpty()) return result;
   // Ordinary Orbit/Ground Track data has no OF regular-arc metadata. Respect
   // its actual breaks and solver transitions, including decreasing-time arcs.
   const bool hasAccepted=std::any_of(curve.points.begin(),curve.points.end(),[](const PlotPoint &point) { return !point.solver; });
   Arc pending; const PlotPoint *previous=nullptr; int direction=0;
   auto flush=[&] { if (!pending.points.isEmpty()) { finish(pending); result.append(pending); pending={}; } direction=0; previous=nullptr; };
   for (const auto &point:curve.points) {
      if (covered.contains(point.frame) || (hasAccepted && point.solver)) { flush(); continue; }
      const int next=previous ? ((point.epoch>previous->epoch)-(point.epoch<previous->epoch)) : 0;
      if (previous && (!point.connect || point.solver!=previous->solver || (direction && next && next!=direction))) flush();
      if (pending.points.isEmpty()) pending.id=point.frame;
      pending.points.append(&point); if (previous && next) direction=next; previous=&point;
   }
   flush();
   return result;
}
inline const Arc *choose(const QVector<Arc> &arcs,double epoch,quint64 id,bool regular)
{
   for (const auto &arc:arcs) if (arc.id==id && arc.regular==regular && arc.distance(epoch)==0) return &arc;
   const Arc *closest=nullptr; double distance=std::numeric_limits<double>::max();
   for (const auto &arc:arcs) {
      const auto candidate=arc.distance(epoch);
      if (candidate==0) return &arc;
      if (candidate<distance) { distance=candidate; closest=&arc; }
   }
   return closest;
}
struct Pose { PlotPoint point; PlotReplayCursor cursor; };
inline Pose resolve(const Arc &arc,double epoch,PlotReplayMode mode,PlotModel::Kind kind)
{
   epoch=std::clamp(epoch,arc.minimum,arc.maximum);
   const PlotPoint *before=arc.points.front(),*after=before;
   for (qsizetype i=1;i<arc.points.size();++i) {
      const auto *a=arc.points[i-1],*b=arc.points[i];
      if (epoch>=std::min(a->epoch,b->epoch) && epoch<=std::max(a->epoch,b->epoch)) { before=a; after=b; break; }
   }
   if (epoch==arc.points.back()->epoch) before=after=arc.points.back();
   else if (epoch==before->epoch) after=before;
   else if (epoch==after->epoch) before=after;
   double fraction=before->epoch==after->epoch ? 0 : (epoch-before->epoch)/(after->epoch-before->epoch);
   if (mode==PlotReplayMode::PropagationSteps) { after=before; fraction=0; }
   PlotPoint point=plotInterpolatedPoint(*before,*after,fraction,kind);
   // Metadata endpoints can retain their original callback index; the path
   // cutoff remains the bracket's publication index, separately from its pose.
   PlotReplayCursor cursor{before->frame,after->frame,fraction,point.epoch};
   point.frame=before->frame;
   return {point,cursor};
}
inline const PlotCurve *curve(const PlotModel &model,const QString &name)
{
   for (const auto &item:model.curves) if (item.name==name) return &item;
   return nullptr;
}
inline std::optional<PlotCamera> cameraAtCursor(const std::deque<PlotCamera> &history,const PlotReplayCursor &cursor)
{
   const PlotCamera *before=nullptr,*after=nullptr;
   for (const auto &camera:history) { if (camera.frame==cursor.before) before=&camera; if (camera.frame==cursor.after) after=&camera; }
   if (!before || (cursor.fraction>0 && !after)) return {};
   if (!after || cursor.fraction<=0) return *before;
   PlotCamera value=*before;
   for (int i=0;i<3;++i) {
      value.eye[i]+=(after->eye[i]-value.eye[i])*cursor.fraction;
      value.target[i]+=(after->target[i]-value.target[i])*cursor.fraction;
      value.up[i]+=(after->up[i]-value.up[i])*cursor.fraction;
   }
   if (before->geometry && after->geometry) {
      auto &geometry=*value.geometry; const auto &other=*after->geometry;
      geometry.referencePose=plotInterpolatedPoint(geometry.referencePose,other.referencePose,cursor.fraction,PlotModel::Kind::Orbit);
      geometry.positionPose=plotInterpolatedPoint(geometry.positionPose,other.positionPose,cursor.fraction,PlotModel::Kind::Orbit);
      geometry.targetPose=plotInterpolatedPoint(geometry.targetPose,other.targetPose,cursor.fraction,PlotModel::Kind::Orbit);
      for (int i=0;i<3;++i) {
         geometry.position[i]+=(other.position[i]-geometry.position[i])*cursor.fraction;
         geometry.center[i]+=(other.center[i]-geometry.center[i])*cursor.fraction;
         geometry.up[i]+=(other.up[i]-geometry.up[i])*cursor.fraction;
      }
   }
   value.epoch=cursor.epoch; return value;
}
inline std::optional<PlotPoint> cameraReference(PlotModel &model,const std::deque<PlotCamera> &history,
      int key,const QString &field,const QString &name,PlotPoint PlotCameraGeometry::*member,double epoch,PlotReplayMode mode)
{
   if (const auto *drawn=curve(model,name)) return drawn->simulationPose;
   PlotCurve samples; samples.name=name; const PlotPoint *previous=nullptr; int direction=0;
   for (const auto &camera:history) {
      if (!camera.geometry || !camera.epoch) continue;
      auto point=(*camera.geometry).*member; point.epoch=*camera.epoch; point.frame=camera.frame;
      point.provider=camera.provider; point.solver=camera.solver;
      const int step=previous ? ((point.epoch>previous->epoch)-(point.epoch<previous->epoch)) : 0;
      point.connect=previous && point.solver==previous->solver && point.provider==previous->provider && !(direction && step && direction!=step);
      if (!point.connect) direction=0; else if (step) direction=step;
      samples.points.push_back(point); previous=&samples.points.back();
   }
   const QString state=QString::number(key)+":"+field+":"+name;
   const auto prior=model.simulationCameraReferenceArcs.value(state);
   const auto candidates=arcs(model,samples);
   if (const auto *selected=choose(candidates,epoch,prior.first,prior.second)) {
      model.simulationCameraReferenceArcs[state]={selected->id,selected->regular};
      return resolve(*selected,epoch,mode,model.kind).point;
   }
   return {};
}
inline bool reconstructCamera(PlotModel &model,const std::deque<PlotCamera> &history,int key,
      PlotCamera &camera,double epoch,PlotReplayMode mode)
{
   if (!camera.geometry) return true;
   auto geometry=*camera.geometry;
   for (const auto &input:{std::make_tuple(QString("reference"),geometry.reference,&PlotCameraGeometry::referencePose),
         std::make_tuple(QString("position"),geometry.positionReference,&PlotCameraGeometry::positionPose),
         std::make_tuple(QString("target"),geometry.target,&PlotCameraGeometry::targetPose)}) {
      const auto &name=std::get<1>(input); if (name.isEmpty() || name=="CoordinateSystem") continue;
      const auto value=cameraReference(model,history,key,std::get<0>(input),name,std::get<2>(input),epoch,mode);
      if (!value) return false;
      geometry.*std::get<2>(input)=*value;
   }
   const osg::Vec3d origin(geometry.referencePose.x,geometry.referencePose.y,geometry.referencePose.z);
   const osg::Vec3d target(geometry.targetPose.x,geometry.targetPose.y,geometry.targetPose.z);
   const std::array<double,9> identity={1,0,0,0,1,0,0,0,1};
   const auto &rotation=geometry.bodyRelative ? geometry.referencePose.bodyToView : identity;
   auto body=[&](const osg::Vec3d &v) {
      return osg::Vec3d(rotation[0]*v.x()+rotation[1]*v.y()+rotation[2]*v.z(),
         rotation[3]*v.x()+rotation[4]*v.y()+rotation[5]*v.z(),rotation[6]*v.x()+rotation[7]*v.y()+rotation[8]*v.z());
   };
   const auto direction=target-origin;
   const osg::Vec3d localDirection(rotation[0]*direction.x()+rotation[3]*direction.y()+rotation[6]*direction.z(),
      rotation[1]*direction.x()+rotation[4]*direction.y()+rotation[7]*direction.z(),rotation[2]*direction.x()+rotation[5]*direction.y()+rotation[8]*direction.z());
   const auto alignment=geometry.lookAtRotation ? cameraAlignment(localDirection,geometry.shortestAngle) : osg::Quat();
   auto orient=[&](const std::array<double,3> &v) { return body(alignment*osg::Vec3d(v[0],v[1],v[2])); };
   if (!geometry.positionReference.isEmpty()) geometry.position={geometry.positionPose.x,geometry.positionPose.y,geometry.positionPose.z};
   const auto eye=origin+orient(geometry.position)*geometry.positionScale;
   const auto center=(geometry.centerOnReference ? origin : target)+orient(geometry.center);
   const auto up=orient(geometry.up);
   if (!PlotModel::usableOrbitPosition(eye.x(),eye.y(),eye.z()) || !PlotModel::usableOrbitPosition(center.x(),center.y(),center.z()) ||
       !std::isfinite(up.length()) || up.length()<1e-12 || (eye-center).length()<1e-9) return false;
   for (int i=0;i<3;++i) { camera.eye[i]=eye[i]; camera.target[i]=center[i]; camera.up[i]=up[i]; }
   camera.geometry=geometry; camera.epoch=epoch; return true;
}
inline void camera(PlotModel &model,const std::deque<PlotCamera> &history,int key,double epoch,PlotReplayMode mode)
{
   if (history.empty()) return;
   // Cameras with a drawn reference use that object's selected arc. Independent
   // cameras use their own frozen epoch history, split at time resets.
   const auto &last=history.back();
   auto anchored=[&](const QString &name) -> std::optional<PlotCamera> {
      const auto *source=curve(model,name);
      return source && source->simulationCursor ? cameraAtCursor(history,*source->simulationCursor) : std::optional<PlotCamera>{};
   };
   auto eye=anchored(last.reference),target=anchored(last.targetReference);
   if (!eye || !target) {
      PlotCurve samples; int direction=0; const PlotCamera *previous=nullptr;
      for (const auto &value:history) {
         if (!value.epoch) continue;
         PlotPoint point; point.frame=value.frame; point.epoch=*value.epoch; point.solver=value.solver;
         const int step=previous && previous->epoch ? ((*value.epoch>*previous->epoch)-(*value.epoch<*previous->epoch)) : 0;
         point.connect=previous && value.solver==previous->solver && value.provider==previous->provider && !(direction && step && direction!=step);
         if (!point.connect) direction=0; else if (step) direction=step;
         samples.points.push_back(point); previous=&value;
      }
      PlotModel independent(model.kind);
      const auto candidates=arcs(independent,samples);
      if (const auto *selected=choose(candidates,epoch,model.simulationCameraArcs.value(key),false)) {
         model.simulationCameraArcs[key]=selected->id;
         const auto pose=resolve(*selected,epoch,mode,model.kind);
         const auto value=cameraAtCursor(history,pose.cursor);
         if (!eye) eye=value; if (!target) target=value;
      }
   }
   if (!eye) return;
   if (target) eye->target=target->target;
   if (reconstructCamera(model,history,key,*eye,epoch,mode)) model.simulationCameras[key]=*eye;
}
}

inline void plotSetSimulationEpoch(PlotModel &model,std::optional<double> epoch,PlotReplayMode mode=PlotReplayMode::InterpolatedSteps)
{
   if (epoch && !PlotModel::usableValue(*epoch)) epoch.reset();
   model.simulationEpoch=epoch; model.simulationMode=mode; model.simulationCameras.clear();
   for (auto &curve:model.curves) {
      curve.simulationActive=epoch.has_value(); curve.simulationPose.reset(); curve.simulationCursor.reset();
      if (!epoch) { curve.simulationArc=0; curve.simulationRegularArc=false; continue; }
      const auto arcs=PlotEpochPlayback::arcs(model,curve);
      if (const auto *arc=PlotEpochPlayback::choose(arcs,*epoch,curve.simulationArc,curve.simulationRegularArc)) {
         curve.simulationArc=arc->id; curve.simulationRegularArc=arc->regular;
         const auto pose=PlotEpochPlayback::resolve(*arc,*epoch,mode,model.kind);
         curve.simulationPose=pose.point; curve.simulationCursor=pose.cursor;
      }
   }
   for (auto &vector:model.vectors) {
      vector.simulationActive=epoch.has_value(); vector.simulationSample.reset();
      if (!epoch) { vector.simulationArc=0; vector.simulationRegularArc=false; continue; }
      const auto *source=PlotEpochPlayback::curve(model,vector.source);
      std::optional<PlotEpochPlayback::Pose> independent;
      if (!source) {
         PlotCurve history; history.name=vector.source; const PlotVectorSample *previous=nullptr; int direction=0;
         for (const auto &sample:vector.samples) {
            auto point=sample.source;
            const int step=previous ? ((point.epoch>previous->source.epoch)-(point.epoch<previous->source.epoch)) : 0;
            point.connect=previous && sample.valid && previous->valid && point.provider==previous->source.provider &&
               point.solver==previous->source.solver && !(direction && step && direction!=step);
            if (!point.connect) direction=0; else if (step) direction=step;
            history.points.push_back(point); previous=&sample;
         }
         const auto arcs=PlotEpochPlayback::arcs(model,history);
         if (const auto *selected=PlotEpochPlayback::choose(arcs,*epoch,vector.simulationArc,vector.simulationRegularArc)) {
            vector.simulationArc=selected->id; vector.simulationRegularArc=selected->regular;
            independent=PlotEpochPlayback::resolve(*selected,*epoch,mode,model.kind);
         }
      }
      if (source && (!source->simulationPose || !source->simulationCursor)) continue;
      if (!source && !independent) continue;
      const auto &cursor=source ? *source->simulationCursor : independent->cursor;
      const PlotVectorSample *before=nullptr,*after=nullptr;
      for (const auto &sample:vector.samples) {
         if (sample.source.frame==cursor.before) before=&sample;
         if (sample.source.frame==cursor.after) after=&sample;
      }
      if (!before || !before->valid || (cursor.fraction>0 && (!after || !after->valid))) continue;
      auto sample=after && cursor.fraction>0 ? plotInterpolatedVector(*before,*after,cursor.fraction) : *before;
      sample.source=source ? *source->simulationPose : independent->point;
      if (!vector.bodyFixed) if (const auto *destination=PlotEpochPlayback::curve(model,vector.destination)) {
         if (!destination->simulationPose) continue;
         const auto &point=*destination->simulationPose; sample.destination={point.x,point.y,point.z};
      }
      vector.simulationSample=sample;
   }
   if (!epoch) { model.simulationCameraArcs.clear(); model.simulationCameraReferenceArcs.clear(); return; }
   PlotEpochPlayback::camera(model,model.cameras,0,*epoch,mode);
   if (model.resetView) PlotEpochPlayback::camera(model,model.resetView->cameras,-1,*epoch,mode);
   for (int index=1;index<model.cameraViews.size();++index) {
      const auto &view=model.cameraViews[index]; PlotEpochPlayback::camera(model,view.cameras,index,*epoch,mode);
      if (view.reset) PlotEpochPlayback::camera(model,view.reset->cameras,-index-1,*epoch,mode);
   }
}
