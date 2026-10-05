#include "PlotEpochPlayback.hpp"
#include "OrbitCamera.hpp"
#include "OrbitObjectAxes.hpp"
#include "OrbitVectors.hpp"
#include "OrbitSensorMasks.hpp"
#include "OrbitSegmentLabels.hpp"
#include <QCoreApplication>
#include <QDataStream>
#include <QIODevice>
#include <iostream>
#include <iomanip>
#include <stdexcept>

static void require(bool value,const char *why) { if (!value) throw std::runtime_error(why); }
static void near(double value,double expected,const char *why,double tolerance=1e-8) {
   if (std::abs(value-expected)>=tolerance) { std::cerr<<std::setprecision(17)<<why<<": "<<value<<" expected "<<expected<<" tolerance "<<tolerance<<'\n'; throw std::runtime_error(why); }
}
static QByteArray raw(const PlotModel &model)
{
   QByteArray bytes; QDataStream out(&bytes,QIODevice::WriteOnly);
   for (const auto &curve:model.curves) for (const auto &p:curve.points) {
      out<<curve.name<<p.x<<p.y<<p.z<<p.epoch<<p.frame<<p.color<<p.connect<<p.solver<<p.high<<p.low<<p.provider<<p.regularArc;
      for (double value:p.bodyToView) out<<value;
      for (double value:p.viewVelocity) out<<value;
   }
   for (const auto &camera:model.cameras) { out<<camera.frame<<camera.solver; for (double x:camera.eye) out<<x; for (double x:camera.target) out<<x; }
   for (const auto &vector:model.vectors) for (const auto &sample:vector.samples) { out<<sample.source.frame<<sample.source.x; for (double value:sample.destination) out<<value; }
   return bytes;
}
static void arc(PlotModel &model,int curve,quint64 id,const QString &name,std::initializer_list<std::array<double,3>> values)
{
   PlotRegularArc summary; summary.id=id; summary.name=name; summary.members.insert(id); summary.finalized=true;
   bool first=true;
   for (const auto &value:values) {
      model.frame=quint64(value[0]); if (first) model.curves[curve].breakNext=true;
      model.append(curve,value[2],0,0,value[1]); auto &point=model.curves[curve].points.back(); point.regularArc=id; point.provider=name;
      if (first) summary.first=summary.last=point;
      if (point.epoch<summary.first.epoch) summary.first=point;
      if (point.epoch>summary.last.epoch) summary.last=point;
      first=false;
   }
   model.regularArcs[model.curves[curve].name].append(summary);
}
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);
   try {
      PlotModel model(PlotModel::Kind::Orbit); model.curves[0].name="Probe"; model.curves[1].name="Other";
      auto &probe=model.curves[0]; probe.centerMarker=true; probe.segmentLabels=true;
      arc(model,0,1,"First",{{1,100,0},{2,110,10}});
      arc(model,0,2,"Reset",{{3,105,100},{4,120,115}});
      arc(model,1,3,"OtherArc",{{5,95,200},{6,115,220}});
      model.scriptedCamera=true;
      for (const auto &curve:model.curves) for (const auto &point:curve.points) {
         PlotCamera camera; camera.frame=point.frame; camera.epoch=point.epoch; camera.reference="Probe"; camera.targetReference="Other";
         // Each source's capture contains the independent target's state too.
         camera.eye={point.frame<=4 ? point.x : point.epoch-100,-100,0};
         camera.target={point.frame>=5 ? point.x : point.epoch+105,0,0};
         model.cameras.push_back(camera);
      }
      PlotVector vector; vector.source="Probe"; vector.destination="Other"; vector.length=5;
      for (const auto &point:probe.points) { PlotVectorSample sample; sample.source=point; sample.destination={point.epoch+105,0,0}; vector.samples.push_back(sample); }
      model.vectors.append(vector);
      PlotSensorMask mask; mask.source="Probe"; mask.length=5; mask.automaticLength=false;
      mask.boundary={{-.1,-.1,-1},{.1,-.1,-1},{.1,.1,-1},{-.1,.1,-1}}; model.sensorMasks.append(mask);
      const auto original=raw(model);
      constexpr auto frame=std::numeric_limits<quint64>::max();
      plotSetSimulationEpoch(model,107.);
      near(orbitObjectPose(probe,frame)->x,7,"First containing arc was not selected");
      near(orbitObjectPose(model.curves[1],frame)->x,212,"Second object normalized its independent interval");
      require(probe.simulationArc==1 && model.curves[1].simulationArc==3,"Per-object arc identity was lost");
      auto camera=orbitCamera(model,frame,0,0,300);
      near(camera.target.x(),212,"Camera target did not follow the independent object's cursor");
      near(camera.target.x()+camera.outward.x()*camera.distance,7,"Camera eye did not follow its source cursor");
      const auto *v=orbitVectorSample(model.vectors[0],frame); require(v,"Epoch vector missing");
      near(v->source.x,7,"Vector source used a global frame"); near(v->destination[0],212,"Vector destination used source history fraction");
      const auto maskGeometry=orbitSensorMaskGeometry(model,mask,frame); require(maskGeometry.has_value(),"Epoch mask missing");
      near(maskGeometry->origin.x(),7,"Sensor mask did not follow epoch pose");
      const auto markers=model.orbitMarkers(probe,frame); require(!markers.empty(),"Epoch marker missing"); near(markers.back().point->x,7,"Center marker lagged behind epoch pose");
      const auto labels=orbitSegmentLabels(model,probe,frame); require(labels.size()==2,"Per-segment labels missing");
      near(labels[0].pose.x,7,"First segment label used wrong epoch"); near(labels[1].pose.x,102,"Reset segment label used wrong epoch");
      int displayed=0; const PlotPoint *last=nullptr; for (const auto &point:plotDisplayedPoints(probe,frame)) { ++displayed; last=&point; }
      require(displayed==2 && last && last->x==7,"Per-object path tail exposed later publications");
      // A finite publication frame remains an explicit raw-history query even
      // while the canvas is displaying its separate absolute-clock sentinel.
      near(orbitObjectPose(probe,1)->x,0,"Absolute clock overrode explicit finite object frame");
      near(orbitVectorSample(model.vectors[0],1)->source.x,0,"Absolute clock overrode explicit finite vector frame");
      near(orbitCamera(model,1,0,0,300).target.x(),205,"Absolute clock overrode explicit finite camera frame");
      int rawCount=0; for (const auto &point:plotDisplayedPoints(probe,1)) { ++rawCount; near(point.x,0,"Absolute clock replaced finite-frame path sample"); }
      require(rawCount==1,"Absolute clock changed finite-frame path cutoff");
      near(model.orbitMarkers(probe,1).back().point->x,0,"Absolute clock replaced finite-frame marker");
      near(orbitSegmentLabels(model,probe,1).front().pose.x,0,"Absolute clock replaced finite-frame segment label epoch");
      plotSetSimulationEpoch(model,117.); require(probe.simulationArc==2,"Clock did not move to the next containing arc"); near(probe.simulationPose->x,112,"Next arc interpolation wrong");
      plotSetSimulationEpoch(model,107.); require(probe.simulationArc==2,"Overlapping current arc was not sticky"); near(probe.simulationPose->x,102,"Sticky arc used first arc pose");
      plotSetSimulationEpoch(model,90.); require(probe.simulationArc==1,"Closest arc before all data was not selected"); near(probe.simulationPose->x,0,"Before-history LIMIT clamp failed");
      plotSetSimulationEpoch(model,125.); require(probe.simulationArc==2,"Closest arc after all data was not selected"); near(probe.simulationPose->x,115,"After-history LIMIT clamp failed");
      plotSetSimulationEpoch(model,107.,PlotReplayMode::PropagationSteps); near(probe.simulationPose->x,100,"Propagation mode synthesized a point");
      plotSetSimulationEpoch(model,{},PlotReplayMode::PropagationSteps);
      require(!model.simulationEpoch && !probe.simulationActive && !probe.simulationPose && model.simulationCameras.empty(),"Clock release retained synthetic state");
      near(orbitObjectPose(probe,frame)->x,115,"Ordinary latest pose changed after releasing clock");
      require(raw(model)==original,"Clock playback mutated raw numerical/camera/vector histories");
      // Coupled LookAt cameras must recompute their orientation after independently
      // selecting both references, not blend a stale precombined camera pose.
      auto coupled=model;
      for (auto &record:coupled.cameras) {
         PlotCameraGeometry geometry; geometry.reference="Probe"; geometry.target="Other";
         geometry.referencePose.x=-800; geometry.targetPose.y=500; // Deliberately stale captured combination.
         geometry.position={0,-100,0}; geometry.up={0,0,1}; geometry.centerOnReference=true;
         geometry.lookAtRotation=true; geometry.shortestAngle=true; record.geometry=geometry;
      }
      plotSetSimulationEpoch(coupled,107.);
      require(coupled.simulationCameras.contains(0),"Coupled clock camera unavailable");
      near(coupled.simulationCameras[0].target[0],7,"LookAt center did not follow selected source");
      near(coupled.simulationCameras[0].eye[0],-93,"LookAt orientation retained stale other-reference branch");
      near(coupled.simulationCameras[0].eye[1],0,"LookAt orientation did not point local +Y at target");
      for (auto &point:coupled.curves[0].points) point.bodyToView={0,-1,0,1,0,0,0,0,1};
      for (auto &record:coupled.cameras) { record.geometry->lookAtRotation=false; record.geometry->bodyRelative=true; }
      plotSetSimulationEpoch(coupled,107.);
      std::cout<<std::setprecision(17)<<"Body-relative interpolated camera eye="<<coupled.simulationCameras[0].eye[0]<<","<<coupled.simulationCameras[0].eye[1]<<" (expected 107,0; Qt float rotation)\n";
      // The established rotation interpolator uses Qt float quaternions/DCMs.
      // A 100-unit camera offset amplifies float matrix rounding to about 1e-5.
      near(coupled.simulationCameras[0].eye[0],107,"Body-relative camera did not use selected source attitude",2e-5);
      near(coupled.simulationCameras[0].eye[1],0,"Body-relative camera local vector was not transformed",2e-5);
      // A named segment's frozen reference must not turn into a moving body.
      for (auto &record:coupled.cameras) {
         auto &g=*record.geometry; g.reference.clear(); g.referencePose.x=500; g.referencePose.y=600; g.referencePose.z=700;
         g.referencePose.bodyToView={1,0,0,0,1,0,0,0,1};
      }
      plotSetSimulationEpoch(coupled,108.);
      near(coupled.simulationCameras[0].eye[0],500,"Fixed segment camera origin moved with its body");
      near(coupled.simulationCameras[0].eye[1],500,"Fixed segment camera offset changed");
      // A decreasing-epoch regular arc must interpolate in recorded order and
      // retain the preceding recorded publication in propagation mode.
      PlotModel reverse(PlotModel::Kind::Orbit); reverse.curves[0].name="Reverse";
      arc(reverse,0,9,"Backward",{{10,200,0},{11,190,10},{12,180,20}});
      plotSetSimulationEpoch(reverse,195.); near(reverse.curves[0].simulationPose->x,5,"Backward arc interpolation failed");
      require(reverse.curves[0].simulationCursor->before==10 && reverse.curves[0].simulationCursor->after==11,"Backward bracket lost publication order");
      plotSetSimulationEpoch(reverse,185.,PlotReplayMode::PropagationSteps); near(reverse.curves[0].simulationPose->x,10,"Backward sampled mode chose later publication");
      // Non-OF histories split at real disconnections and choose nearest arc,
      // rather than drawing an invented bridge across an unrecorded gap.
      PlotModel ordinary(PlotModel::Kind::GroundTrack); ordinary.curves[0].name="Ground";
      for (int i=0;i<4;++i) { ordinary.frame=i+1; if (i==2) ordinary.breakLines(); ordinary.append(0,i<2 ? 179+i*2 : 20+i,0,0,i<2 ? i : 8+i); }
      plotSetSimulationEpoch(ordinary,.5); near(std::abs(ordinary.curves[0].simulationPose->x),180,"Clock ground interpolation crossed long dateline path");
      plotSetSimulationEpoch(ordinary,5.); near(ordinary.curves[0].simulationPose->x,181,"Gap did not clamp nearest arc");
      plotSetSimulationEpoch(ordinary,10.5); near(ordinary.curves[0].simulationPose->x,22.5,"Second disconnected interval missing");
      // Compact accepted endpoints remain usable after bounded history trimming.
      reverse.curves[0].points.pop_front(); reverse.curves[0].points.pop_front();
      plotSetSimulationEpoch(reverse,195.); near(reverse.curves[0].simulationPose->x,5,"Frozen regular endpoints were lost after trimming");
      PlotModel hidden(PlotModel::Kind::Orbit); hidden.scriptedCamera=true;
      PlotVector hiddenVector; hiddenVector.source="HiddenSource"; hiddenVector.bodyFixed=true;
      for (int i=0;i<2;++i) {
         PlotVectorSample v; v.source.frame=20+i; v.source.epoch=200+i*10; v.source.x=100+i*10; hiddenVector.samples.push_back(v);
         PlotCamera camera; camera.frame=20+i; camera.epoch=v.source.epoch; camera.reference="HiddenSource";
         camera.eye={v.source.x,-100,0}; camera.target={v.source.x,0,0}; hidden.cameras.push_back(camera);
      }
      hidden.vectors.append(hiddenVector); plotSetSimulationEpoch(hidden,205.);
      const auto *hiddenSample=orbitVectorSample(hidden.vectors.front(),frame); require(hiddenSample,"Undrawn vector source disappeared during clock playback");
      near(hiddenSample->source.x,105,"Undrawn vector source did not use frozen epoch history");
      near(orbitCamera(hidden,frame,0,0,200).target.x(),105,"Undrawn camera reference did not use frozen epoch history");
      model.clear(); require(!model.simulationEpoch && !probe.simulationActive && !probe.simulationPose,"Mission reset retained clock presentation state");
      std::cout<<"PASS: absolute per-object epochs; sticky/first/nearest regular arc selection; reverse and sampled playback; camera eye/target, vector, mask, markers, labels and path tips; discontinuity/trim/reset boundaries; byte-identical raw histories.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
