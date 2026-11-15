#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "CoordinateSystem.hpp"
#include "AxisSystem.hpp"
#include "Parameter.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include "BaseException.hpp"
#include "ParameterException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
constexpr double omega = .0001;
constexpr double degrees = 180.0 / 3.14159265358979323846;
void require(bool condition, const char *message) {
   if (!condition) throw std::runtime_error(message);
}
void save(const QString &path, const QByteArray &bytes) {
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(), "Cannot save parameter sample evidence");
}
class SampleAxes : public AxisSystem {
public:
   int realCalls = 0, preciseCalls = 0;
   bool fail = false;
   GmatTime last;
   SampleAxes() : AxisSystem("PrescribedSampleAxes") { baseSystem = "FK5"; }
   GmatBase *Clone() const override { return new SampleAxes(*this); }
   void rotate(const GmatTime &epoch) {
      if (fail) throw ParameterException("intentional parameter frame failure");
      last = epoch;
      const double angle = omega * (epoch - GmatTime(21545.125)).GetTimeInSec();
      const double c = std::cos(angle), s = std::sin(angle);
      rotMatrix.Set(1,0,0, 0,c,-s, 0,s,c);
      rotDotMatrix.Set(0,0,0, 0,-omega*s,-omega*c, 0,omega*c,-omega*s);
      rotData = rotMatrix.GetDataVector(); rotDotData = rotDotMatrix.GetDataVector();
   }
   void CalculateRotationMatrix(const A1Mjd &epoch, bool) override { ++realCalls; rotate(GmatTime(epoch.Get())); }
   void CalculateRotationMatrix(const GmatTime &epoch, bool) override { ++preciseCalls; rotate(epoch); }
};
Rvector6 independentFrame(const Rvector6 &state, const GmatTime &epoch) {
   const double a = omega * (epoch - GmatTime(21545.125)).GetTimeInSec();
   const double c = std::cos(a), s = std::sin(a);
   const double y = c*state[1]+s*state[2], z = -s*state[1]+c*state[2];
   return Rvector6(state[0],y,z,state[3],c*state[4]+s*state[5]+omega*z,-s*state[4]+c*state[5]-omega*y);
}
using V = std::array<double,3>;
V cross(V a, V b) { return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}; }
double dot(V a,V b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
V scaled(V a,double s) { return {a[0]*s,a[1]*s,a[2]*s}; }
V unit(V a) { return scaled(a,1/std::sqrt(dot(a,a))); }
double bplane(const std::string &name,const Rvector6 &state,double mu) {
   const V r{state[0],state[1],state[2]}, v{state[3],state[4],state[5]};
   const V h=cross(r,v), vxh=scaled(cross(v,h),1/mu), rhat=unit(r);
   const V e{vxh[0]-rhat[0],vxh[1]-rhat[1],vxh[2]-rhat[2]};
   const double ecc=std::sqrt(dot(e,e)), vinf=std::sqrt(dot(v,v)-2*mu/std::sqrt(dot(r,r)));
   require(ecc>1 && std::isfinite(vinf),"Independent B-plane fixture must be hyperbolic");
   const V eh=unit(e), q=cross(unit(h),eh);
   const double f=std::sqrt(1-1/(ecc*ecc));
   const V incoming{eh[0]/ecc+f*q[0],eh[1]/ecc+f*q[1],eh[2]/ecc+f*q[2]};
   // Angular momentum and incoming velocity determine the asymptotic impact vector.
   const V impact=scaled(cross(incoming,h),1/vinf), t=unit(cross(incoming,{0,0,1})), rr=cross(incoming,t);
   const double bt=dot(impact,t), br=dot(impact,rr);
   if (name=="BdotT") return bt;
   if (name=="BdotR") return br;
   if (name=="BVectorMag") return std::hypot(bt,br);
   return degrees*std::atan2(br,bt);
}
class SamplePublisher : public Publisher {};
struct Evaluation {
   Spacecraft *craft;
   CelestialBody *body;
   SampleAxes *axes;
   std::vector<Parameter*> parameters;
   QJsonArray rows;
   bool passed = true;
   void check(const std::string &phase,const Rvector6 &input,const GmatTime &epoch,bool precise) {
      const auto state=independentFrame(input,epoch);
      const double longitude=degrees*std::atan2(state[1],state[0]);
      for (auto *parameter:parameters) {
         const auto type=parameter->GetTypeName();
         double expected;
         if (type=="Latitude") expected=degrees*std::atan2(state[2],std::hypot(state[0],state[1]));
         else if (type=="Longitude") expected=longitude;
         else if (type=="Altitude") expected=std::sqrt(state[0]*state[0]+state[1]*state[1]+state[2]*state[2])-6378.0;
         else if (type=="MHA" || type=="LST") {
            // Hour-angle model is a declared native input, not an independent astronomy oracle.
            expected=body->GetHourAngle(A1Mjd(epoch.GetMjd()))+(type=="LST"?longitude:0);
         } else expected=bplane(type,state,body->GetGravitationalConstant());
         axes->realCalls=axes->preciseCalls=0;
         const double actual=parameter->EvaluateReal();
         const bool angular=type=="Longitude"||type=="MHA"||type=="LST"||type=="BVectorAngle";
         const double error=std::abs(angular?std::remainder(actual-expected,360.):actual-expected);
         const double bound=(type=="Altitude"||type=="BdotT"||type=="BdotR"||type=="BVectorMag")?1e-8:1e-9;
         const bool dispatch=precise?axes->realCalls==0 && axes->preciseCalls>0 && axes->last==epoch:axes->realCalls>0 && axes->preciseCalls==0;
         const bool ok=std::isfinite(actual)&&error<=bound&&dispatch;
         passed=passed&&ok;
         rows.append(QJsonObject{{"phase",QString::fromStdString(phase)},{"type",QString::fromStdString(type)},{"actual",actual},{"expected",expected},{"error",error},{"bound",bound},{"real_calls",axes->realCalls},{"precise_calls",axes->preciseCalls},{"epoch_dispatch",dispatch},{"passed",ok}});
      }
   }
};
class ParameterSink : public Subscriber {
public:
   SamplePublisher &publisher;
   Evaluation &evaluation;
   Rvector6 live, sample;
   GmatTime liveEpoch, sampleEpoch;
   bool nested, livePrecise;
   int depth=0, deliveries=0, stream=-1;
   ParameterSink(SamplePublisher &p,Evaluation &e,Rvector6 l,Rvector6 s,GmatTime le,GmatTime se,bool nest,bool lp)
      : Subscriber("ParameterSampleSink","ParameterSampleSink"),publisher(p),evaluation(e),live(l),sample(s),liveEpoch(le),sampleEpoch(se),nested(nest),livePrecise(lp) {}
   GmatBase *Clone() const override { return new ParameterSink(*this); }
   bool ReceiveData(const char*) override { return true; }
   bool ReceiveData(const Real *data,const Integer count=0) override {
      evaluation.check(depth?"nested-unscoped":"unscoped",live,liveEpoch,livePrecise);
      {
         Publisher::ParameterStateScope view(data,count);
         evaluation.check(depth?"nested-legacy":"published",depth?live:sample,depth?liveEpoch:sampleEpoch,depth?livePrecise:true);
         if (nested && !depth) {
            ++depth;require(publisher.Publish(evaluation.craft,stream,const_cast<Real*>(data),count),"Nested legacy publication failed");--depth;
            evaluation.check("restored-outer",sample,sampleEpoch,true);
         }
      }
      evaluation.check("after-scope",live,liveEpoch,livePrecise);
      ++deliveries;
      return true;
   }
};
}

int main(int argc,char **argv) {
   QCoreApplication application(argc,argv);
   const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   QJsonObject proof{{"passed",false}};
   try {
      require(argc==3&&!directory.isEmpty(),"Supply parameter sample startup, case and evidence directory");
      const QString name=argv[2];proof["case"]=name;
      const bool planet=name.startsWith("Planet"), mixed=name=="NestedPublished", published=name.endsWith("Published"), failure=name=="BplaneFrameFailure";
      const bool local=name.contains("Local");
      const bool precise=name.endsWith("Precise")||mixed;
      require(planet||name.startsWith("Bplane")||mixed,"Unknown parameter sample case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      auto *moderator=Moderator::Instance();require(moderator->Initialize(startup.toStdString(),false),"Native parameter runtime failed");
      save(qEnvironmentVariable("GMAT_SOLVER_MAP_READY"),QByteArray::number(QCoreApplication::applicationPid()));
      QElapsedTimer timer;timer.start();while(!QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK"))&&timer.elapsed()<10000)QThread::msleep(5);
      require(QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")),"Native images not acknowledged");
      const auto script=directory+"/mission.script";
      save(script,(QString("Create Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545.125';\nCreate CoordinateSystem Tracked;\nTracked.Origin = ")+(local?"Pluto":"Earth")+";\nTracked.Axes = MJ2000Eq;\nBeginMissionSequence;\n").toUtf8());
      require(moderator->InterpretScript(script.toStdString()),"Parameter fixture did not build");
      auto *craft=dynamic_cast<Spacecraft*>(moderator->GetConfiguredObject("Sat"));
      auto *frame=dynamic_cast<CoordinateSystem*>(moderator->GetConfiguredObject("Tracked"));
      auto *solar=moderator->GetSolarSystemInUse();auto *earth=solar->GetBody(local?"Pluto":"Earth");
      require(craft&&frame&&earth,"Native parameter fixture absent");
      earth->SetRealParameter(earth->GetParameterID("Flattening"),0.0);
      earth->SetRealParameter(earth->GetParameterID("EquatorialRadius"),6378.0);
      SampleAxes prescribed;require(frame->SetRefObject(&prescribed,Gmat::AXIS_SYSTEM,prescribed.GetName())&&frame->Initialize(),"Prescribed frame failed");
      auto *axes=dynamic_cast<SampleAxes*>(frame->GetAxisSystem());require(axes,"Prescribed axes lost native identity");
      GmatTime liveEpoch(21545.125);if(precise)liveEpoch.AddSeconds(1e-7);
      craft->SetPrecisionTimeFlag(precise);craft->SetEpochGT(liveEpoch);
      Rvector6 live(12000,400,-800,1.3,14,-2);
      if(local) { live[0]+=.123456789; live[1]+=1.23456789e-7; live[2]+=3.14159265359; }
      const Rvector6 sample(10000,1000,2000,0,12,3);
      const Rvector6 backing=local?live+earth->GetMJ2000State(liveEpoch):live;
      craft->SetState(backing);
      if(local)require(craft->SetOriginRelativeState(earth,liveEpoch,live),"Native local-state cache setup failed");
      Evaluation evaluation{craft,earth,axes};
      QStringList types;if(planet||mixed)types={"Latitude","Longitude","Altitude","MHA","LST"};
      if(!planet)types+=QStringList{"BdotT","BdotR","BVectorMag","BVectorAngle"};
      for(const auto &type:types) {
         const bool p=QStringList{"Latitude","Longitude","Altitude","MHA","LST"}.contains(type);
         const std::string dependency=p?earth->GetName():"Tracked",full="Sat."+dependency+"."+type.toStdString();
         auto *parameter=moderator->CreateParameter(type.toStdString(),full,"Sat",dependency,1);
         require(parameter,"Actual native Parameter factory failed");
         parameter->SetRefObjectName(Gmat::COORDINATE_SYSTEM,"Tracked");
         require(parameter->AddRefObject(frame,true)&&parameter->Initialize(),"Actual native Parameter binding failed");
         evaluation.parameters.push_back(parameter);
      }
      if(published) {
         GmatTime sampleEpoch(21545.125);sampleEpoch.AddSeconds(2.0000001);
         const Rvector6 originAtSample=local?earth->GetMJ2000State(sampleEpoch):Rvector6(0,0,0,0,0,0);
         const Rvector6 sampleWorld=sample+originAtSample;
         const Rvector6 sampleOracle=sampleWorld-originAtSample;
         SamplePublisher publisher;ParameterSink sink(publisher,evaluation,live,sampleOracle,liveEpoch,sampleEpoch,mixed,precise);
         require(publisher.Subscribe(&sink),"Parameter sink registration failed");
         sink.stream=publisher.RegisterPublishedData(craft,-1,StringArray{"Sat","Sat"},StringArray{"Epoch","Sat.X"});
         Real packet[2]={liveEpoch.GetMjd(),backing[0]};
         require(publisher.PublishWithSamples(craft,sink.stream,packet,2,liveEpoch,{{craft,sampleEpoch,sampleWorld}})&&sink.deliveries==(mixed?2:1),"Canonical publication failed");
         publisher.Unsubscribe(&sink);
         GmatTime absent;Rvector6 state;require(!Publisher::GetParameterCartesianState(craft,absent,state),"Report sample escaped its scope");
      } else if(failure) {
         axes->fail=true;bool rejected=false;
         try { evaluation.parameters.front()->EvaluateReal(); }
         catch(BaseException &error) { rejected=error.GetFullMessage().find("intentional parameter frame failure")!=std::string::npos; }
         evaluation.passed=rejected;evaluation.rows.append(QJsonObject{{"phase","frame-error"},{"rejected",rejected}});
         axes->fail=false;evaluation.check("recovered",live,liveEpoch,precise);
      } else evaluation.check("live",live,liveEpoch,precise);
      require(craft->GetEpochGT()==liveEpoch&&Rvector6(craft->GetState().GetState())==backing,"Parameter evaluation changed the live spacecraft");
      if(local) { Rvector6 cached;require(craft->GetOriginRelativeState(earth,liveEpoch,cached)&&cached==live,"Parameter evaluation changed its native local input"); }
      proof["passed"]=evaluation.passed;proof["observations"]=evaluation.rows;
      proof["live_state_and_epoch_unchanged"]=true;
      proof["scope"]="Actual factory Parameters, native Publisher sample scopes, prescribed Rx/Rdot and independent spherical/vector geometry. Native hour angle is an explicit input. No body orientation/provider,all-frame STM or whole NASA qualification.";
   } catch(BaseException &error) { proof["error"]=QString::fromStdString(error.GetFullMessage()); }
     catch(std::exception &error) { proof["error"]=QString::fromUtf8(error.what()); }
   save(QDir(directory).filePath("proof.json"),QJsonDocument(proof).toJson());
   std::cout<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
   return proof["passed"].toBool()?0:1;
}
