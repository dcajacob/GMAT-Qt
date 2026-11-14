#pragma once
#include "PlotStepLiveObservation.hpp"
#include "PlotOriginScope.hpp"

namespace plotorigin {
inline QJsonArray protocol() {
 class LocalPublisher final : public Publisher { public: LocalPublisher():Publisher(){} } publisher;
 Spacecraft origin("OriginProbe"),unrelated("OtherProbe");
 PlotStepStream observer;
 formaccept::require(publisher.Subscribe(&observer),"Origin observer missing");
 const StringArray owners(7,"OriginProbe"),labels{"Epoch","OriginProbe.X","OriginProbe.Y","OriginProbe.Z","OriginProbe.Vx","OriginProbe.Vy","OriginProbe.Vz"};
 const int id=publisher.RegisterPublishedData(&origin,-1,owners,labels);
 formaccept::require(id>=0,"Origin stream registration failed");
 Real data[]={21545,1,2,3,4,5,6};
 std::vector<Publisher::IntegrationStep> records;
 for(int i=0;i<3;++i) {
  Publisher::IntegrationStep s;s.epoch=GmatTime(21545);s.endSeconds=i==0?3:6;s.epoch.AddSeconds(s.endSeconds);
  s.states.push_back({&origin,s.epoch,Rvector6(42.+i,2,3,4,5,6)});records.push_back(s);
 }
 const auto epoch=records[0].epoch;
 auto close=epoch;close.AddSeconds(1e-9);
 formaccept::require(close!=epoch&&close.GetMjd()==epoch.GetMjd(),"Origin precision discriminator unavailable");
 struct Unwind {};
 QJsonArray checks;bool nested=false;
 observer.sample=[&](const Real *raw,Integer count) {
  if(nested) {
   formaccept::require(!PlotOriginScope::Find(&origin,epoch),"Nested raw publication inherited outer origin");
   checks.append("nested-raw-isolation");return;
  }
  PlotOriginScope::Packet indexed(raw,count,&origin);
  PlotOriginScope scope(indexed,epoch);
  const auto *value=PlotOriginScope::Find(&origin,epoch);
  formaccept::require(scope.Available()&&value&&(*value)[0]==42&&PlotOriginScope::Find(&origin,epoch.GetMjd())==value,"Origin precision or Real lookup differs");
  formaccept::require(!PlotOriginScope::Find(&origin,close)&&!PlotOriginScope::Find(&unrelated,epoch),"Origin lookup escaped epoch/object binding");
  checks.append("bound-state-and-precision");
  try {
   PlotOriginScope ambiguous(indexed,records[1].epoch);
   formaccept::require(!ambiguous.Available()&&!PlotOriginScope::Find(&origin,records[1].epoch),"Conflicting coeval origin states accepted");
   checks.append("ambiguous-state-rejected");throw Unwind{};
  } catch(const Unwind &) {formaccept::require(PlotOriginScope::Find(&origin,epoch)==value,"Exception lost outer origin scope");}
  checks.append("exception-restores-outer-scope");nested=true;
  formaccept::require(publisher.Publish(&origin,id,data,7),"Nested origin packet failed");nested=false;
  formaccept::require(PlotOriginScope::Find(&origin,epoch)==value,"Nested raw publication lost outer origin");
  checks.append("nested-return-restores-origin");
 };
 formaccept::require(publisher.PublishWithIntegrationSteps(&origin,id,data,7,records),"Origin protocol dispatch failed");
 formaccept::require(!PlotOriginScope::Find(&origin,epoch),"Origin scope escaped dispatch");checks.append("outside-scope-isolation");
 publisher.Unsubscribe(&observer);
 return checks;
}
}
