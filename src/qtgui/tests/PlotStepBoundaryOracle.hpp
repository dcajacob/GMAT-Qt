#pragma once
#include "PlotStepLiveObservation.hpp"
#include "GroundTrack.hpp"

namespace plotboundary {
inline QJsonArray checkPacketScope() {
 class LocalPublisher final : public Publisher { public: LocalPublisher() : Publisher() {} } publisher;
 class Probe final : public GroundTrack {
 public:
  Probe() : GroundTrack("BoundaryProbe") {}
  using GroundTrack::RepeatsPropagationStart;
 } probe;
 Spacecraft first("SameName"),second("SameName");
 PlotStepStream stream;
 const StringArray names(7,"SameName"),labels{"Epoch","SameName.X","SameName.Y","SameName.Z","SameName.Vx","SameName.Vy","SameName.Vz"};
 formaccept::require(publisher.Subscribe(&stream),"Boundary protocol observer unavailable");
 const auto id=publisher.RegisterPublishedData(&first,-1,names,labels);
 formaccept::require(id>=0,"Boundary protocol stream registration failed");
 probe.SetDataLabels(labels);probe.SetRunState(Gmat::RUNNING);probe.SetPropagationDirection(1);
 Real data[]={21545,1,2,3,4,5,6};
 QJsonArray outcomes;
 auto publish=[&](const QString &label,Spacecraft &owner,bool initial,bool withSteps,bool repeated) {
  ObjectArray objects{&owner};
  bool nested=false,observed=false;
  stream.sample=[&](const Real *packet,Integer size) {
   bool flagged=true;
   const auto *binding=Publisher::GetPublishedPropagationObjects(packet,size,flagged);
   if(nested) {
    formaccept::require(!binding&&!flagged,"Nested plain publication inherited propagation identity");
    return;
   }
   formaccept::require(binding&&*binding==objects&&flagged==initial,"Boundary provider identity or initial flag differs");
   formaccept::require(bool(Publisher::GetPublishedIntegrationSteps(packet,size))==withSteps,"Boundary provenance changed step subscription");
   formaccept::require(!Publisher::GetPublishedPropagationObjects(packet,size-1,flagged)&&!Publisher::GetPublishedPropagationObjects(packet+1,size,flagged),"Boundary metadata escaped pointer/count binding");
   const bool actual=probe.RepeatsPropagationStart(packet,size);
   formaccept::require(actual==repeated,"Legacy boundary predicate merged distinct inputs or missed duplicate");
   nested=true;formaccept::require(publisher.Publish(&first,id,data,7),"Nested raw publication failed");nested=false;
   formaccept::require(Publisher::GetPublishedPropagationObjects(packet,size,flagged)==binding&&flagged==initial,"Nested publication lost outer identity");
   outcomes.append(QJsonObject{{"case",label},{"initial",initial},{"step_metadata",withSteps},{"repeated",actual}});observed=true;
  };
  const std::vector<Publisher::IntegrationStep> steps;
  formaccept::require(publisher.PublishPropagatePacket(&first,id,data,7,1,objects,initial,withSteps?&steps:nullptr)&&observed,"Boundary packet not observed");
  bool flagged=true;
  formaccept::require(!Publisher::GetPublishedPropagationObjects(data,7,flagged)&&!flagged,"Completed packet leaked metadata");
 };
 publish("ordinary",first,false,false,false);
 publish("matching-start",first,true,false,true);
 publish("ordinary-repeat",first,false,true,false);
 publish("same-name-distinct-object",second,true,true,false);
 data[6]+=1;publish("changed-state",second,true,false,false);
 probe.SetPropagationDirection(-1);publish("reversal",second,true,true,false);
 probe.SetRunState(Gmat::SOLVING);publish("trial",second,true,false,false);
 probe.SetRunState(Gmat::RUNNING);publish("resume-after-trial",second,true,true,false);
 publisher.Unsubscribe(&stream);
 return outcomes;
}
}
