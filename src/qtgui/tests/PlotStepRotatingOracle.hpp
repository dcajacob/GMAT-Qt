#pragma once
#include "FormationAcceptance.hpp"
#include "ODEModel.hpp"
#include "PlotOriginScope.hpp"

namespace plotrotating {
// Independently prescribed radial/tangential/normal basis of Two's circular
// yz orbit. Differentiate the transformed position analytically for velocity.
inline std::array<double,6> state(int object,double t,bool translated,bool velocity=false) {
 const auto input=formaccept::circular(formaccept::orbits[object],t);
 const auto origin=formaccept::circular(formaccept::orbits[1],t);
 const double rate=std::sqrt(formaccept::mu/std::pow(9000.,3));
 const double angle=.4+rate*t,c=std::cos(angle),s=std::sin(angle);
 std::array<double,6> relative=input,result{};
 if(translated)for(int i=0;i<6;++i)relative[i]-=origin[i];
 result[0]=c*relative[1]+s*relative[2];
 result[1]=-s*relative[1]+c*relative[2];
 result[2]=relative[0];
 result[3]=c*relative[4]+s*relative[5]+rate*result[1];
 result[4]=-s*relative[4]+c*relative[5]-rate*result[0];
 result[5]=relative[3];
 if(velocity)result={result[1],-result[0],result[2],result[4],-result[3],result[5]};
 return result;
}
using Vec=std::array<double,3>;
inline Vec cross(const Vec &a,const Vec &b) {return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
inline Vec unit(Vec a) {double norm=0;for(double v:a)norm+=v*v;norm=std::sqrt(norm);for(auto &v:a)v/=norm;return a;}
inline Vec pairPosition(int object,double t) {
 const auto one=formaccept::circular(formaccept::orbits[0],t),two=formaccept::circular(formaccept::orbits[1],t);
 const auto satellite=formaccept::circular(formaccept::orbits[object],t);
 Vec r{},v{},position{};for(int i=0;i<3;++i){r[i]=two[i]-one[i];v[i]=two[i+3]-one[i+3];position[i]=satellite[i]-two[i];}
 const auto x=unit(r),z=unit(cross(r,v)),y=cross(z,x);Vec result{};
 for(int i=0;i<3;++i){result[0]+=x[i]*position[i];result[1]+=y[i]*position[i];result[2]+=z[i]*position[i];}
 return result;
}
inline std::array<double,6> pairState(int object,double t) {
 const double h=.1;const auto p=pairPosition(object,t),a=pairPosition(object,t-2*h),b=pairPosition(object,t-h),c=pairPosition(object,t+h),d=pairPosition(object,t+2*h);
 std::array<double,6> result{};for(int i=0;i<3;++i){result[i]=p[i];result[i+3]=(a[i]-8*b[i]+8*c[i]-d[i])/(12*h);}return result;
}
inline QJsonArray contextChecks() {
 class Force final : public PhysicalModel {
 public:
  int calls=0;
  Force():PhysicalModel(Gmat::PHYSICAL_MODEL,"RotatingAcceptanceForce","RotatingAcceptanceForce"){}
  GmatBase *Clone()const override{return new Force(*this);}
  bool HasLocalClones()override{return false;}
  bool RenameRefObject(const UnsignedInt,const std::string&,const std::string&)override{return false;}
  bool Initialize()override{isInitialized=true;return true;}
  Rvector6 GetDerivativesForSpacecraft(Spacecraft*)override{++calls;return Rvector6(0,0,0,.001,.002,.003);}
 };
 auto *moderator=Moderator::Instance();auto *sc=dynamic_cast<Spacecraft*>(moderator->GetInternalObject("Two"));
 auto *command=moderator->GetFirstCommand();while(command&&!command->IsOfType("Propagate"))command=command->GetNext();
 auto *setup=command?dynamic_cast<PropSetup*>(command->GetClone(1)):nullptr;
 auto *model=setup?setup->GetODEModel():nullptr;
 formaccept::require(sc&&model,"Rotating acceleration context fixture missing");
 auto *earth=model->GetForceOrigin();auto *force=new Force;model->AddForce(force);
 formaccept::require(model->Initialize(),"Rotating generic force setup failed");model->ActivateAccelerationContext();
 Rvector3 expected;formaccept::require(ODEModel::GetBoundRelativeAcceleration(sc,earth,A1Mjd(sc->GetEpoch()),expected),"Generic live context unavailable");
 const GmatTime epoch=sc->GetEpochGT();Rvector6 native;for(int i=0;i<6;++i)native[i]=sc->GetState()[i];
 class LocalPublisher final : public Publisher {public:LocalPublisher():Publisher(){}} publisher;
 PlotStepStream observer;formaccept::require(publisher.Subscribe(&observer),"Context observer failed");
 const StringArray owners(7,"Two"),labels{"Epoch","Two.X","Two.Y","Two.Z","Two.Vx","Two.Vy","Two.Vz"};
 const auto id=publisher.RegisterPublishedData(sc,-1,owners,labels);formaccept::require(id>=0,"Context registration failed");
 Real data[]={epoch.GetMjd(),native[0],native[1],native[2],native[3],native[4],native[5]};
 QJsonArray checks;int mode=0;
 observer.sample=[&](const Real *raw,Integer count){
  const auto *steps=Publisher::GetPublishedIntegrationSteps(raw,count);formaccept::require(steps&&steps->size()==1,"Context sample missing");
  PlotOriginScope::Packet packet(raw,count,sc);PlotOriginScope scope(packet,steps->front().epoch);
  formaccept::require(scope.Available(),"Context state unavailable");
  const int before=force->calls;Rvector3 observed(11,12,13);bool rejected=false;
  try {formaccept::require(ODEModel::GetBoundRelativeAcceleration(sc,earth,A1Mjd(steps->front().epoch.GetMjd()),observed),"Sampled context not selected");}
  catch(ODEModelException &e){rejected=e.GetFullMessage().find("Sampled object-frame acceleration is unavailable")!=std::string::npos;}
  if(mode==0){formaccept::require(!rejected&&observed==expected&&force->calls==before+1,"Coeval generic force behavior changed");checks.append("coeval-generic-force-preserved");}
  else {formaccept::require(rejected&&observed==Rvector3(11,12,13)&&force->calls==before,"Unavailable sampled force used live inputs or changed output");checks.append(mode==1?"stale-epoch-force-rejected":"same-epoch-changed-state-force-rejected");}
 };
 for(mode=0;mode<3;++mode){
  Publisher::IntegrationStep step;step.epoch=epoch;step.initial=true;
  if(mode==1)step.epoch.AddSeconds(-3);Rvector6 sample=native;if(mode==2)sample[0]+=1;
  step.states.push_back({sc,step.epoch,sample});
  formaccept::require(publisher.PublishWithIntegrationSteps(sc,id,data,7,{step}),"Context publication failed");
 }
 publisher.Unsubscribe(&observer);
 for(int i=0;i<6;++i)formaccept::require(sc->GetState()[i]==native[i],"Context probe changed live state");
 formaccept::require(sc->GetEpochGT()==epoch,"Context probe changed live epoch");
 return checks;
}
}
