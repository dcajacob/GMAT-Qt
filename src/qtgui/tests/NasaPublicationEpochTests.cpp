#include "Publisher.hpp"
#include "PublisherException.hpp"
#include "Subscriber.hpp"
#include "Spacecraft.hpp"
#include "EphemWriterSPK.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <thread>
static void require(bool value,const char *text){if(!value)throw std::runtime_error(text);}
class TestPublisher : public Publisher {
public:
 int dispatches=0;
 bool Publish(GmatBase*p,Integer id,Real*d,Integer n,Real direction=1)override{++dispatches;return Publisher::Publish(p,id,d,n,direction);}
};
class Sink : public Subscriber {
public:
 enum Mode{Precise,Legacy,NestedLegacy,NestedPrecise,Throw,Reject};
 TestPublisher *publisher;GmatBase *provider;Integer stream=-1;Mode mode=Precise;int depth=0,checks=0;GmatTime expected,inner;
 Sink(TestPublisher*p,GmatBase*g):Subscriber("EpochSink","EpochSink"),publisher(p),provider(g){}
 GmatBase *Clone() const override { return new Sink(*this); }
 bool ReceiveData(const char*)override{return true;}
 bool ReceiveData(const Real *data,const Integer len=0)override{
  GmatTime epoch;const bool found=Publisher::GetPublishedEpoch(data,len,epoch);
  const bool wanted=depth?mode==NestedPrecise:mode!=Legacy;
  require(found==wanted,"Precision metadata availability leaked or disappeared");
  if(found)require(epoch==(depth?inner:expected),"Sub-MJD epoch was rounded or stale");
  Real copy[2]={data[0],data[1]};require(!Publisher::GetPublishedEpoch(copy,len,epoch),"Precision metadata matched a different packet pointer");require(!Publisher::GetPublishedEpoch(data,len-1,epoch),"Precision metadata matched a different packet length");++checks;
  if(!depth){
   bool otherThread=true;std::thread reader([&]{GmatTime local;otherThread=Publisher::GetPublishedEpoch(data,len,local);});reader.join();require(!otherThread,"Publication epoch escaped its thread");
   if(mode==NestedLegacy||mode==NestedPrecise){++depth;if(mode==NestedLegacy)require(publisher->Publish(provider,stream,const_cast<Real*>(data),len),"Nested legacy dispatch failed");else require(publisher->PublishWithEpoch(provider,stream,const_cast<Real*>(data),len,inner),"Nested precise dispatch failed");--depth;require(Publisher::GetPublishedEpoch(data,len,epoch)&&epoch==expected,"Outer precise epoch not restored after nested dispatch");}
   if(mode==Throw)throw std::runtime_error("intentional receiver exception");
   if(mode==Reject)return false;
  }
  return true;
 }
};
class BufferProbe : public EphemWriterSPK {
public:
 BufferProbe():EphemWriterSPK("EpochBuffer"){}
 BufferProbe(const BufferProbe&o):EphemWriterSPK(o){}
 void add(const GmatTime&epoch,bool precise){SetPublishedEpoch(precise?&epoch:nullptr);Real state[6]={7000,0,0,0,7,0},cov[21]={},quat[4]={},accel[3]={};BufferOrbitData(epoch.GetMjd(),state,cov,quat,accel);}
 void reset(){ClearOrbitData();}
 size_t size()const{return preciseEpochArray.size();}
 GmatTime epoch(size_t i)const{return preciseEpochArray.at(i);}
 bool precise(size_t i)const{return preciseEpochFlags.at(i);}
};
int main(int argc,char **argv){try{
 require(argc==2,"Expected native epoch contract startup file");FileManager::Instance()->ReadStartupFile(argv[1]);
 TestPublisher publisher;Spacecraft provider("EpochSource");Sink sink(&publisher,&provider);require(publisher.Subscribe(&sink),"Subscribe failed");sink.stream=publisher.RegisterPublishedData(&provider,-1,StringArray{"EpochSource","EpochSource"},StringArray{"Epoch","EpochSource.X"});GmatTime base(26088.0),epoch=base;epoch.AddSeconds(20e-9);sink.expected=epoch;sink.inner=epoch;sink.inner.AddSeconds(40e-9);require(epoch!=base&&epoch.GetMjd()==base.GetMjd()&&sink.inner.GetMjd()==epoch.GetMjd(),"Sub-MJD collision fixture invalid");Real data[2]={epoch.GetMjd(),7000};const std::array<Real,2> original{data[0],data[1]};GmatTime absent;int contracts=0;
 require(!Publisher::GetPublishedEpoch(data,2,absent),"Precision metadata present before dispatch");++contracts;
 require(publisher.PublishWithEpoch(&provider,sink.stream,data,2,epoch),"Precise dispatch failed");require(publisher.dispatches==1,"Precise dispatch bypassed existing Publisher override");++contracts;
 require(!Publisher::GetPublishedEpoch(data,2,absent),"Precision metadata persisted after dispatch");++contracts;
 sink.mode=Sink::Legacy;require(publisher.Publish(&provider,sink.stream,data,2),"Legacy dispatch failed");++contracts;
 sink.mode=Sink::NestedLegacy;require(publisher.PublishWithEpoch(&provider,sink.stream,data,2,epoch),"Nested legacy test failed");++contracts;
 sink.mode=Sink::NestedPrecise;require(publisher.PublishWithEpoch(&provider,sink.stream,data,2,epoch),"Nested precise test failed");++contracts;
 sink.mode=Sink::Throw;bool threw=false;try{publisher.PublishWithEpoch(&provider,sink.stream,data,2,epoch);}catch(const std::runtime_error&e){threw=std::string(e.what())=="intentional receiver exception";}require(threw&&!Publisher::GetPublishedEpoch(data,2,absent),"Exception did not restore publication scope");++contracts;
 sink.mode=Sink::Reject;require(!publisher.PublishWithEpoch(&provider,sink.stream,data,2,epoch)&&!Publisher::GetPublishedEpoch(data,2,absent),"Rejected dispatch did not restore scope");++contracts;
 sink.mode=Sink::Legacy;require(publisher.Publish(&provider,sink.stream,data,2),"Legacy dispatch after failure inherited stale precision");++contracts;
 for(int variant=0;variant<3;++variant){bool rejected=false;try{GmatTime wrong=epoch;wrong.AddSeconds(1);publisher.PublishWithEpoch(&provider,sink.stream,variant==0?nullptr:data,variant==1?0:2,variant==2?wrong:epoch);}catch(const BaseException&){rejected=true;}require(rejected&&!Publisher::GetPublishedEpoch(data,2,absent),"Invalid precise request accepted or contaminated scope");++contracts;}
 require(data[0]==original[0]&&data[1]==original[1],"Epoch channel changed state or ordinary epoch data");++contracts;
 BufferProbe buffer;buffer.add(epoch,true);buffer.add(sink.inner,false);require(buffer.size()==2&&buffer.precise(0)&&!buffer.precise(1)&&buffer.epoch(0)==epoch,"SPK buffer lost precise epoch or legacy flag");++contracts;
 buffer.reset();buffer.add(sink.inner,true);require(buffer.size()==1&&buffer.precise(0)&&buffer.epoch(0)==sink.inner,"SPK reset retained stale precise epochs");++contracts;
 BufferProbe copy(buffer);require(copy.size()==0,"SPK clone copied transient epoch metadata without a state buffer");++contracts;
 publisher.Unsubscribe(&sink);std::cout<<"NASA_CHECK {\"checkpoint\":\"publication-epoch-contract\",\"outcome\":\"passed\",\"contracts\":"<<contracts<<",\"receiver_checks\":"<<sink.checks<<",\"scope\":\"Synchronous precise packet metadata,nesting,legacy isolation,exception/rejection/thread restoration andSPK buffer reset;no state resampling or physical SPK acceptance\"}"<<std::endl;return 0;
 }catch(BaseException&e){std::cerr<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<e.what()<<std::endl;}return 1;}
