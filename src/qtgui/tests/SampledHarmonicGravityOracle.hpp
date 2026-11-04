#pragma once
#include "GravityField.hpp"
#include "Planet.hpp"
#include "CoordinateConverter.hpp"
#include "CoordinateSystem.hpp"
#include "HarmonicGravityOracle.hpp"
#include <tuple>
#include <functional>

namespace sampledgravity {
// Inspect a typed copy of the actual initialized field, keeping protected clock
// checks in test code. The production API adds no clock getters or data members.
class Probe : public GravityField {
public:
 explicit Probe(const GravityField &field):GravityField(field){}
 auto clock()const{return std::make_tuple(epoch,epochGT,elapsedTime,hasPrecisionTime);}
 auto tideClock()const{return std::make_tuple(now.Get(),nowGT);}
 void setTideClock(const GmatTime &value){now=value.GetMjd();nowGT=value;}
};
inline QJsonArray check(GravityField &native,Spacecraft &owner,CoordinateSystem *input,
      CoordinateSystem *fixed,const HarmonicOracle::Field &field,int degree,int order,
      double radius) {
 using namespace HarmonicOracle;
 Probe probe(native);
 // Native copy construction intentionally clears frame pointers and field
 // initialization. Bind borrowed frames and initialize the independent probe.
 require(probe.SetRefObject(input,Gmat::COORDINATE_SYSTEM,probe.GetStringParameter("InputCoordinateSystem"))&&probe.SetRefObject(fixed,Gmat::COORDINATE_SYSTEM,probe.GetStringParameter("FixedCoordinateSystem"))&&probe.Initialize(),"Sampled harmonic typed probe initialization failed");
 probe.SetTime(1234.5);
 const auto saved=probe.clock();const auto ownerBefore=owner.GetState();
 QJsonArray records;CoordinateConverter converter;Rvector6 zero(0,0,0,0,0,0),unused;
 for(double offset:{-10000.25,-3.125,17.5,10000.75}) {
  GmatTime epoch(21545);epoch.AddSeconds(offset);
  const auto origin=probe.GetForceOrigin()->GetMJ2000State(epoch);
  fixed->SetForGravityUpdate(true);converter.Convert(epoch,zero,input,unused,fixed,false,true);fixed->SetForGravityUpdate(false);
  const auto matrix=converter.GetLastRotationMatrix();M rotation{};
  for(int i=0;i<3;++i)for(int j=0;j<3;++j)rotation[i][j]=matrix(i,j);
  for(V position:std::vector<V>{{radius,0,0},{-radius,0,0},{0,radius,0},{0,-radius,0},{0,0,radius},{0,0,-radius},{.7*radius,-.4*radius,.8*radius}}) {
   const auto local=rotate(rotation,position,true);Rvector6 cartesian;
   for(int i=0;i<3;++i){cartesian[i]=origin[i]+local[i];cartesian[i+3]=origin[i+3]+.1*(i+1);}
   const auto before=cartesian;const auto derivative=probe.GetDerivativesAtState(cartesian,epoch);
   require(probe.clock()==saved&&cartesian==before,"Sampled harmonic query changed its force clock or input value");
   V actualLocal{},observed{},errors{};for(int i=0;i<3;++i)actualLocal[i]=cartesian[i]-origin[i];
   const auto expected=force(field,degree,order,actualLocal,rotation);
   for(int i=0;i<3;++i){observed[i]=derivative[i+3];errors[i]=std::abs(observed[i]-expected[i]);require(errors[i]<1e-11,"Sampled harmonic acceleration differs from independent potential gradient");require(std::abs(derivative[i]-(cartesian[i+3]-origin[i+3]))<1e-15,"Sampled harmonic relative velocity differs");}
   auto values=[](const V &v){QJsonArray a;for(double x:v)a.append(x);return a;};
   QJsonArray rotationJson;for(const auto &r:rotation)rotationJson.append(values(r));
   records.append(QJsonObject{{"epoch_offset",offset},{"absolute_mjd",epoch.GetMjd()},{"force_clock_seconds",1234.5},{"local_position",values(actualLocal)},{"rotation",rotationJson},{"native_acceleration",values(observed)},{"independent_acceleration",values(expected)},{"errors",values(errors)},{"clock_restored",true},{"input_unchanged",true}});
  }
 }
 class FailingOrigin : public Planet {
 public:
  FailingOrigin():Planet("SampledGravityFailure"){}
  const Rvector6 GetMJ2000State(const GmatTime&)override{throw std::runtime_error("intentional sampled gravity origin failure");}
 } failing;
 Rvector6 recoveryState;for(int i=0;i<6;++i)recoveryState[i]=ownerBefore[i];
 const auto recoveryExpected=probe.GetDerivativesAtState(recoveryState,owner.GetEpochGT());
 auto *origin=probe.GetForceOrigin();probe.SetForceOrigin(&failing);bool rejected=false;
 try{probe.GetDerivativesAtState(zero,GmatTime(21545));}catch(const std::runtime_error &error){rejected=std::string(error.what())=="intentional sampled gravity origin failure";}
 probe.SetForceOrigin(origin);
 require(rejected&&probe.clock()==saved,"Sampled harmonic provider exception changed the force clock");
 require(probe.GetDerivativesAtState(recoveryState,owner.GetEpochGT())==recoveryExpected&&probe.clock()==saved,"Sampled harmonic query did not recover after provider failure");
 require(owner.GetEpochGT()==ownerBefore.GetEpochGT(),"Sampled harmonic query changed the live owner epoch");
 for(int i=0;i<ownerBefore.GetSize();++i)require(owner.GetState()[i]==ownerBefore[i],"Sampled harmonic query changed the live owner state");
 require(records.size()==28,"Sampled harmonic epoch/grid matrix incomplete");
 return records;
}
inline QJsonArray checkTides(GravityField &native,CoordinateSystem *input,
      CoordinateSystem *fixed,const std::function<void(const QJsonArray&)> &retain) {
 Probe sampled(native),reference(native),untided(native);
 for(auto *field:{&sampled,&reference,&untided}) {
  require(field->SetRefObject(input,Gmat::COORDINATE_SYSTEM,field->GetStringParameter("InputCoordinateSystem"))&&field->SetRefObject(fixed,Gmat::COORDINATE_SYSTEM,field->GetStringParameter("FixedCoordinateSystem"))&&field->Initialize(),"Sampled tidal probe initialization failed");
  field->SetIntegerParameter("Degree",4);field->SetIntegerParameter("Order",4);
  field->SetTime(1234.5);field->setTideClock(GmatTime(21546));
 }
 QJsonArray rows;
 for(const auto &tide:std::vector<std::string>{"Solid","SolidAndPole"}) {
  require(sampled.SetStringParameter("TideModel",tide)&&reference.SetStringParameter("TideModel",tide)&&untided.SetStringParameter("TideModel","None"),"Actual tidal provider selection failed");
  for(double offset:{-172800.25,.125,345600.75}) {
   GmatTime epoch(21545);epoch.AddSeconds(offset);reference.setTideClock(epoch);
   const auto origin=sampled.GetForceOrigin()->GetMJ2000State(epoch);
   Rvector6 state(origin[0]+7000,origin[1]-1200,origin[2]+3400,origin[3]+.2,origin[4]+7,origin[5]-.3);
   const auto saved=sampled.clock();const auto savedTides=sampled.tideClock();
   const auto expected=reference.GetDerivativesAtState(state,epoch),actual=sampled.GetDerivativesAtState(state,epoch),none=untided.GetDerivativesAtState(state,epoch);
   QJsonArray observed,wanted,noTides,errors;double maximum=0,effect=0;
   for(int i=3;i<6;++i){observed.append(actual[i]);wanted.append(expected[i]);noTides.append(none[i]);errors.append(std::abs(actual[i]-expected[i]));maximum=std::max(maximum,std::abs(actual[i]-expected[i]));effect=std::max(effect,std::abs(expected[i]-none[i]));}
   rows.append(QJsonObject{{"tide_model",QString::fromStdString(tide)},{"epoch_offset",offset},{"stale_cached_mjd",21546},{"sample_epoch_mjd",epoch.GetMjd()},{"sampled_acceleration",observed},{"coeval_reference_acceleration",wanted},{"untided_acceleration",noTides},{"errors",errors},{"maximum_error",maximum},{"tidal_effect",effect},{"force_clock_restored",sampled.clock()==saved},{"tide_clock_restored",sampled.tideClock()==savedTides},{"scope","Clock coherence against an explicitly coeval instance of the same native tide algorithm; not independent tidal physics truth."}});retain(rows);
   require(maximum<5e-15&&effect>1e-12,"Sampled tidal force used a stale Sun/Moon epoch");
   require(sampled.clock()==saved&&sampled.tideClock()==savedTides,"Sampled tide query changed cached clocks");
  }
 }
 require(rows.size()==6,"Sampled tidal model/epoch matrix incomplete");return rows;
}
}
