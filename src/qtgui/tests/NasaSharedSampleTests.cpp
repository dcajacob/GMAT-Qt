#include "Publisher.hpp"
#include "PublisherException.hpp"
#include "Subscriber.hpp"
#include "Spacecraft.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
static void require(bool b,const char *s){if(!b)throw std::runtime_error(s);}
class TestPublisher:public Publisher{public:int calls=0;bool Publish(GmatBase*p,Integer id,Real*d,Integer n,Real dir=1)override{++calls;return Publisher::Publish(p,id,d,n,dir);}};
class Sink:public Subscriber{
public:
 enum Mode{Ordinary,Legacy,PreciseOnly,NestedLegacy,NestedPrecise,NestedSamples,Throw,Reject,Mutate};
 Mode mode=Ordinary;int depth=0,checks=0;TestPublisher *publisher;Spacecraft *a,*b,*unknown;Integer stream;GmatTime source,epoch,innerEpoch,*callerEpoch=nullptr;std::vector<Publisher::CartesianSample>*callerSamples=nullptr;Rvector6 first,second,innerState;
 Sink(TestPublisher*p,Spacecraft*x,Spacecraft*y,Spacecraft*z):Subscriber("SampleSink","SampleSink"),publisher(p),a(x),b(y),unknown(z){}
 GmatBase*Clone()const override{return new Sink(*this);}bool ReceiveData(const char*)override{return true;}
 bool ReceiveData(const Real*data,const Integer len=0)override{
  const bool available=depth?mode==NestedSamples:mode!=Legacy&&mode!=PreciseOnly;GmatTime time(-7);Rvector6 state(1,2,3,4,5,6);const auto sentinel=state;
  require(Publisher::GetPublishedCartesianState(data,len,a,time,state)==available,"Sample availability leaked or disappeared");
  if(!available){require(time==GmatTime(-7)&&state==sentinel,"Absent sample changed outputs");require(!Publisher::GetParameterCartesianState(a,time,state),"Missing/nested sample leaked into parameter view");++checks;return true;}
  const auto wanted=depth?innerState:first;const auto wantedEpoch=depth?innerEpoch:epoch;require(time==wantedEpoch&&state==wanted,"Sample epoch/state changed");
  require(!Publisher::GetParameterCartesianState(a,time,state),"Parameter view activated outside report scope");
  Real copied[2]={data[0],data[1]};require(!Publisher::GetPublishedCartesianState(copied,len,a,time,state)&&!Publisher::GetPublishedCartesianState(data,len-1,a,time,state),"Sample matched wrong packet pointer/length");require(!Publisher::GetPublishedCartesianState(data,len,unknown,time,state),"Sample matched another object with the same name");
  bool escaped=true;std::thread t([&]{GmatTime e;Rvector6 x;escaped=Publisher::GetPublishedCartesianState(data,len,a,e,x);});t.join();require(!escaped,"Sample escaped its publication thread");
  {
   Publisher::ParameterStateScope view(data,len);require(Publisher::GetParameterCartesianState(a,time,state)&&time==wantedEpoch&&state==wanted,"Report parameter view lost sample");
   if(!depth){require(Publisher::GetParameterCartesianState(b,time,state)&&time==epoch&&state==second,"Object identity or second sample lost");
    if(mode==NestedLegacy||mode==NestedPrecise||mode==NestedSamples){++depth;if(mode==NestedLegacy)require(publisher->Publish(a,stream,const_cast<Real*>(data),len),"Nested legacy publish failed");else if(mode==NestedPrecise)require(publisher->PublishWithEpoch(a,stream,const_cast<Real*>(data),len,source),"Nested precise publish failed");else{std::vector<Publisher::CartesianSample> nested{{a,innerEpoch,innerState}};require(publisher->PublishWithSamples(a,stream,const_cast<Real*>(data),len,source,nested),"Nested sample publish failed");}--depth;require(Publisher::GetParameterCartesianState(a,time,state)&&time==epoch&&state==first,"Outer report sample not restored after nesting");}
    if(mode==Mutate){(*callerSamples)[0].state[0]+=100;callerEpoch->AddSeconds(1);require(Publisher::GetPublishedCartesianState(data,len,a,time,state)&&time==epoch&&state==first,"Callback changed frozen producer samples");GmatTime e;require(Publisher::GetPublishedEpoch(data,len,e)&&e==source,"Callback changed frozen source epoch");}
    if(mode==Throw)throw std::runtime_error("intentional sample receiver exception");
    if(mode==Reject)return false;
   }
  }
  require(!Publisher::GetParameterCartesianState(a,time,state),"Parameter sample scope persisted");++checks;return true;
 }
};
int main(int argc,char**argv){try{
 require(argc==2,"Supply shared sample contract startup");FileManager::Instance()->ReadStartupFile(argv[1]);TestPublisher publisher;Spacecraft a("Same"),b("Same"),unknown("Same");Sink sink(&publisher,&a,&b,&unknown);require(publisher.Subscribe(&sink),"Subscribe failed");sink.stream=publisher.RegisterPublishedData(&a,-1,StringArray{"Same","Same"},StringArray{"Epoch","Same.X"});GmatTime source(26088);source.AddSeconds(.0343817);sink.source=source;sink.epoch=source;sink.epoch.AddSeconds(20e-9);sink.innerEpoch=source;sink.innerEpoch.AddSeconds(-20e-9);sink.first=Rvector6(7000,1,2,3,4,5);sink.second=Rvector6(8000,6,7,8,9,10);sink.innerState=Rvector6(9000,11,12,13,14,15);std::vector<Publisher::CartesianSample> samples{{&a,sink.epoch,sink.first},{&b,sink.epoch,sink.second}};Real data[2]={source.GetMjd(),7000};const std::array<Real,2> before{data[0],data[1]};const auto liveA=a.GetLastState(),liveB=b.GetLastState();const auto epochA=a.GetEpochGT(),epochB=b.GetEpochGT();GmatTime absent;Rvector6 value;int contracts=0;
 require(!Publisher::GetPublishedCartesianState(data,2,&a,absent,value),"Sample present before publication");++contracts;
 for(auto mode:{Sink::Ordinary,Sink::Legacy,Sink::PreciseOnly,Sink::NestedLegacy,Sink::NestedPrecise,Sink::NestedSamples}){sink.mode=mode;if(mode==Sink::Legacy)require(publisher.Publish(&a,sink.stream,data,2),"Legacy publish failed");else if(mode==Sink::PreciseOnly)require(publisher.PublishWithEpoch(&a,sink.stream,data,2,source),"Precise-only publish failed");else require(publisher.PublishWithSamples(&a,sink.stream,data,2,source,samples),"Sample publish failed");require(!Publisher::GetPublishedCartesianState(data,2,&a,absent,value)&&!Publisher::GetParameterCartesianState(&a,absent,value),"Publication metadata persisted");++contracts;}
 sink.mode=Sink::Throw;bool threw=false;try{publisher.PublishWithSamples(&a,sink.stream,data,2,source,samples);}catch(const std::runtime_error&e){threw=std::string(e.what())=="intentional sample receiver exception";}require(threw&&!Publisher::GetParameterCartesianState(&a,absent,value)&&!Publisher::GetPublishedCartesianState(data,2,&a,absent,value),"Exception leaked sample state");++contracts;
 sink.mode=Sink::Reject;require(!publisher.PublishWithSamples(&a,sink.stream,data,2,source,samples)&&!Publisher::GetPublishedCartesianState(data,2,&a,absent,value)&&!Publisher::GetParameterCartesianState(&a,absent,value),"Rejection leaked sample state");++contracts;
 sink.mode=Sink::Mutate;sink.callerSamples=&samples;sink.callerEpoch=&source;require(publisher.PublishWithSamples(&a,sink.stream,data,2,source,samples),"Immutable-copy test failed");source=sink.source;samples[0].state=sink.first;++contracts;
 for(int variant=0;variant<5;++variant){auto invalid=samples;if(variant==0)invalid[0].object=nullptr;if(variant==1)invalid[1].object=&a;if(variant==2)invalid[0].state[4]=std::numeric_limits<double>::quiet_NaN();bool rejected=false;try{GmatTime wrong=source;wrong.AddSeconds(1);publisher.PublishWithSamples(&a,sink.stream,variant==3?nullptr:data,2,variant==4?wrong:source,invalid);}catch(BaseException&){rejected=true;}require(rejected&&!Publisher::GetPublishedCartesianState(data,2,&a,absent,value)&&!Publisher::GetParameterCartesianState(&a,absent,value),"Invalid sample metadata accepted or leaked");++contracts;}
 sink.mode=Sink::Ordinary;require(publisher.PublishWithSamples(&a,sink.stream,data,2,source,samples),"Sample publication did not recover");require(data[0]==before[0]&&data[1]==before[1]&&a.GetLastState()==liveA&&b.GetLastState()==liveB&&a.GetEpochGT()==epochA&&b.GetEpochGT()==epochB,"Sample path changed packet or live spacecraft");++contracts;
 publisher.Unsubscribe(&sink);std::cout<<"NASA_CHECK {\"checkpoint\":\"shared-sample-contract\",\"outcome\":\"passed\",\"contracts\":"<<contracts<<",\"receiver_checks\":"<<sink.checks<<",\"virtual_dispatches\":"<<publisher.calls<<",\"scope\":\"Synchronous immutable Cartesian samples,object and packet identity,explicit parameter scope,nested legacy/precise/sample isolation,thread and exception/rejection cleanup;no general-force or GUI acceptance credit\"}"<<std::endl;return 0;
 }catch(BaseException&e){std::cerr<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<e.what()<<std::endl;}return 1;}
