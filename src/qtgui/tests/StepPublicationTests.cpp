// Accepted endpoints must reach only their committed publication, without extra
// legacy packets or mutation of live spacecraft during snapshot conversion.
#include "FormationAcceptance.hpp"
#include "Debugger.hpp"
#include "AcceptedStepHistory.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include "ODEModel.hpp"
#include "RungeKutta89.hpp"
#include "AdamsBashforthMoulton.hpp"
#include <QDialog>
#include <QTimer>
#include <functional>
#include <memory>
using namespace formaccept;
namespace {
struct Packet { RealArray raw; std::vector<Publisher::IntegrationStep> steps; bool metadata=false; };
class Observer final : public Subscriber {
public:
 std::vector<Packet> packets;
 std::function<void(const Real*, Integer)> observe;
 Observer() : Subscriber("StepPublicationObserver", "StepPublicationObserver") {}
 ~Observer() override { Publisher::Instance()->Unsubscribe(this); }
 GmatBase *Clone() const override { return new Observer(*this); }
 bool ReceiveData(const Real *data, Integer count=0) override {
  if (!data || count<=0) return true;
  Packet packet; packet.raw.assign(data,data+count);
  if (const auto *steps=Publisher::GetPublishedIntegrationSteps(data,count)) {
   packet.metadata=true; packet.steps=*steps;
  }
  packets.push_back(std::move(packet));
  if (observe) observe(data,count);
  return true;
 }
protected:
 bool Distribute(Integer) override { return true; }
 bool Distribute(const Real*,Integer) override { return true; }
};
struct TrialTrace { QJsonArray events; };
class TrialABM final : public AdamsBashforthMoulton {
public:
 std::shared_ptr<TrialTrace> trace;
 explicit TrialABM(std::shared_ptr<TrialTrace> t) : AdamsBashforthMoulton("TrialABM"),trace(std::move(t)) {}
 GmatBase *Clone() const override { return new TrialABM(*this); }
 using AdamsBashforthMoulton::Step;
 bool Step() override {
  trace->events.append(QJsonObject{{"event","before-step"},{"time",GetTime()},{"startup",startupCount},{"complete",startupComplete}});
  const bool result=AdamsBashforthMoulton::Step();
  trace->events.append(QJsonObject{{"event",result?"accepted":"failed"},{"time",GetTime()},{"startup",startupCount},{"complete",startupComplete}});
  return result;
 }
};
class ForcedErrorRK final : public RungeKutta89 {
protected: Real EstimateError() override { return 1.; }
};
void fixedIntervalPolicy(ODEModel *model) {
 // Inject an excessive estimate to exercise the unchanged strict/warning
 // policy at a short fixed-interval floor, rather than depending on roundoff.
 ForcedErrorRK stepper;stepper.SetPhysicalModel(model);
 stepper.SetRealParameter("InitialStepSize",3.);stepper.SetRealParameter("MinStep",3.);
 stepper.SetRealParameter("MaxStep",3.);stepper.SetRealParameter("Accuracy",1e-10);
 require(stepper.Initialize(),"Strict-error policy fixture did not initialize");stepper.SetAsFinalStep(false);
 for(double direction:{1.,-1.}) {
  stepper.SetBooleanParameter(stepper.GetParameterID("StopIfAccuracyIsViolated"),true);
  const double before=model->GetTime();bool threw=false;
  try {stepper.Step(direction*.5);}catch(BaseException&) {threw=true;}
  require(threw&&stepper.GetRealParameter("MinStep")==3.&&model->GetTime()==before,"Strict error was bypassed, MinStep leaked, or rejected trial advanced time");
  stepper.SetBooleanParameter(stepper.GetParameterID("StopIfAccuracyIsViolated"),false);
  require(stepper.Step(direction*.5)&&stepper.GetRealParameter("MinStep")==3.&&std::abs(model->GetTime()-before-direction*.5)<1e-10,"Warning policy did not finish the exact short interval and restore MinStep");
 }
}

QString config(const QString &report, const QString &method, double step, bool formation, bool multiple) {
 QString text="Create Spacecraft One Two;\n";
 const QStringList fields{"X","Y","Z","VX","VY","VZ"};
 for (int n=0;n<2;++n) {
  const auto &o=orbits[n]; const auto state=circular(o,0);
  text+=o.name+".DateFormat = A1ModJulian;\n"+o.name+".Epoch = '21545';\n"+o.name+".CoordinateSystem = EarthMJ2000Eq;\n"+o.name+".DisplayStateType = Cartesian;\n";
  for (int c=0;c<6;++c) text+=o.name+"."+fields[c]+" = "+number(state[c])+";\n";
 }
 if (formation) text+="Create Formation Fleet;\nFleet.Add = {Two, One};\n";
 text+="Create Variable Keep;\nKeep = 41;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.SRP = Off;\nFM.Drag = None;\nCreate Propagator P;\nP.FM = FM;\nP.Type = "+method+";\nP.InitialStepSize = "+number(step)+";\nP.MinStep = "+number(std::abs(step))+";\nP.MaxStep = "+number(std::abs(step))+";\nP.Accuracy = 1e-10;\n";
 if (multiple) text+="Create Propagator Q;\nQ.FM = FM;\nQ.Type = RungeKutta89;\nQ.InitialStepSize = 3;\nQ.MinStep = 3;\nQ.MaxStep = 3;\nQ.Accuracy = 1e-10;\n";
 text+="Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nBeginMissionSequence;\nReport 'Arm endpoint observers' Values Keep;\n";
 return text;
}
QJsonArray encode(const std::vector<Packet> &packets) {
 QJsonArray result;
 for (const auto &packet:packets) {
  QJsonArray raw,steps;
  for (double x:packet.raw) raw.append(std::isfinite(x)?QJsonValue(x):QJsonValue("nan"));
  for (const auto &step:packet.steps) {
   QJsonArray states;
   for (const auto &sample:step.states) {
    QJsonArray state; for(int c=0;c<6;++c) state.append(sample.state[c]);
    states.append(QJsonObject{{"object",QString::fromStdString(sample.object->GetName())},{"epoch",sample.epoch.GetMjd()},{"state",state}});
   }
   steps.append(QJsonObject{{"initial",step.initial},{"start",step.startSeconds},{"end",step.endSeconds},{"epoch",step.epoch.GetMjd()},{"method",QString::fromStdString(step.method)},{"states",states}});
  }
  result.append(QJsonObject{{"raw",raw},{"metadata",packet.metadata},{"steps",steps}});
 }
 return result;
}
class LocalPublisher final : public Publisher {
public: LocalPublisher() : Publisher() {}
};
void publicationContract() {
 LocalPublisher publisher;Spacecraft spacecraft("PacketSat");Observer observer;
 require(publisher.Subscribe(&observer),"Local subscriber rejected");
 Publisher::SetIntegrationStepConsumer(&observer,true);
 require(publisher.HasIntegrationStepConsumers(),"Opt-in subscriber not recognized");
 StringArray owners(7,"PacketSat"),elements{"Epoch","PacketSat.X","PacketSat.Y","PacketSat.Z","PacketSat.Vx","PacketSat.Vy","PacketSat.Vz"};
 const Integer id=publisher.RegisterPublishedData(&spacecraft,-1,owners,elements);
 Real data[]={21545,1,2,3,4,5,6};GmatTime epoch(21545);epoch.AddSeconds(1);
 Publisher::IntegrationStep step;step.startSeconds=0;step.endSeconds=1;step.epoch=epoch;step.method="SyntheticContract";
 step.states.push_back({&spacecraft,epoch,Rvector6(1,2,3,4,5,6)});
 std::vector<Publisher::IntegrationStep> steps{step};bool nested=false,observed=false;
 observer.observe=[&](const Real *packet,Integer count) {
  const auto *payload=Publisher::GetPublishedIntegrationSteps(packet,count);
  if(nested) {require(!payload,"Nested legacy publication inherited outer metadata");return;}
  require(payload&&payload->size()==1&&(*payload)[0].states[0].state[0]==1,"Metadata binding failed");
  steps[0].states[0].state[0]=999;
  require((*payload)[0].states[0].state[0]==1,"Caller mutation changed frozen payload");
  nested=true;require(publisher.Publish(&spacecraft,id,data,7),"Nested legacy packet failed");nested=false;
  require(Publisher::GetPublishedIntegrationSteps(packet,count)==payload,"Nested dispatch did not restore outer metadata");observed=true;
 };
 require(publisher.PublishWithIntegrationSteps(&spacecraft,id,data,7,steps)&&observed,"Optional payload did not use existing virtual publication");
 require(!Publisher::GetPublishedIntegrationSteps(data,7),"Completed publication left metadata active");
 observer.observe=[](const Real*,Integer){throw std::runtime_error("intentional callback failure");};
 bool caught=false;try {publisher.PublishWithIntegrationSteps(&spacecraft,id,data,7,steps);}catch(const std::runtime_error&) {caught=true;}
 require(caught&&!Publisher::GetPublishedIntegrationSteps(data,7),"Exception leaked synchronous payload context");observer.observe={};
 publisher.ClearPublishedData();require(publisher.Subscribe(&observer)&&!publisher.HasIntegrationStepConsumers(),"Bulk clear silently re-enabled an old registration");
 Publisher::SetIntegrationStepConsumer(&observer,true);publisher.UnsubscribeAll();require(publisher.Subscribe(&observer)&&!publisher.HasIntegrationStepConsumers(),"UnsubscribeAll retained old registration");
 publisher.Unsubscribe(&observer);
}

class LinearBody final : public CelestialBody {
 Rvector6 answer;
public:
 double bias;
 explicit LinearBody(const std::string &name,double offset):CelestialBody("CelestialBody",name),bias(offset) {}
 GmatBase *Clone() const override { return new LinearBody(*this); }
 const Rvector6 &GetState(GmatTime atTime) override {
  const double seconds=double((atTime-GmatTime(21545)).GetTimeInSec());
  for(int c=0;c<6;++c) answer[c]=bias*(c+1)+seconds*(c+2)*bias/10;
  return answer;
 }
};
class SnapshotModel final : public ODEModel {
public:
 void configure(CelestialBody *origin,CelestialBody *j2000) {
  dimension=15; cartesianStart=2; cartesianCount=2;
  forceOrigin=origin; j2kBody=j2000; centralBodyName="Origin"; j2kBodyName="J2000";
 }
};
void originContract() {
 LinearBody origin("Origin",100),j2000("J2000",10);SnapshotModel model;model.configure(&origin,&j2000);
 RealArray input(15);for(size_t i=0;i<input.size();++i)input[i]=i+.25;
 const auto copy=input;GmatTime time(21545);time.AddSeconds(123.125);
 const auto result=model.GetJ2KStateSnapshot(input,time);
 require(input==copy&&result.size()==input.size(),"Origin conversion changed its input snapshot");
 for(int i=0;i<15;++i) {
  const double expected=(i>=2&&i<14)?input[i]+90*((i-2)%6+1)+9*123.125*((i-2)%6+2):input[i];
  require(std::abs(result[i]-expected)<1e-12,"Snapshot origin translation/sign/block range differs from literal oracle");
 }
 bool rejected=false;try { model.GetJ2KStateSnapshot(RealArray(14),time); } catch(BaseException &) {rejected=true;}
 require(rejected,"Snapshot converter accepted wrong dimension");
}
}
int main(int argc,char **argv) {
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("StepPublication");
 try {
  require(argc==2,"Supply native startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());originContract();publicationContract();
  struct Case { QString name,method,mission;double step,end;bool formation,multiple,single; };
  const double nonlinear=37.123;
  const auto nonlinearX=circular(orbits[0],nonlinear)[0];
  const std::vector<Case> cases{
   {"ForwardSplit","RungeKutta89","Propagate P(One) {One.ElapsedSecs = 10};\nPropagate P(One) {One.ElapsedSecs = 27};\n",10,37,false,false,false},
   {"BackwardSplit","RungeKutta89","Propagate BackProp P(One) {One.ElapsedSecs = -10};\nPropagate BackProp P(One) {One.ElapsedSecs = -27};\n",-10,-37,false,false,false},
   {"SingleSteps","RungeKutta89","Propagate P(One);\nPropagate P(One);\n",10,20,false,false,true},
   {"Synchronized","RungeKutta89","Propagate Synchronized P(One) Q(Two) {One.ElapsedSecs = 23};\n",10,23,false,true,false},
   {"SynchronizedBackward","RungeKutta89","Propagate BackProp Synchronized P(One) Q(Two) {One.ElapsedSecs = -23};\n",-10,-23,false,true,false},
   {"Formation","RungeKutta89","Propagate P(Fleet) {One.ElapsedSecs = 25};\n",10,25,true,false,false},
   {"NonlinearABM","AdamsBashforthMoulton","Propagate P(One) {One.EarthMJ2000Eq.X = "+number(nonlinearX)+", StopTolerance = 1e-10};\n",1,nonlinear,false,false,false},
   {"NonlinearABMQuarter","AdamsBashforthMoulton","Propagate P(One) {One.EarthMJ2000Eq.X = "+number(circular(orbits[0],37.25)[0])+", StopTolerance = 1e-10};\n",1,37.25,false,false,false}
  };
  const auto filter=qEnvironmentVariable("GMAT_STEP_PUBLICATION_CASE");QJsonArray completed;
  for (const auto &test:cases) {
   if(!filter.isEmpty()&&!filter.split(',').contains(test.name))continue;
   std::vector<Packet> baseline;
   for (bool enabled:{false,true}) {
    const auto prefix=test.name+(enabled?"-endpoints":"-legacy");
    const auto script=config(rig.report,test.method,test.step,test.formation,test.multiple)+test.mission+"Report Values One.ElapsedSecs Keep;\n";
    rig.source->setPlainText(script);require(rig.window.buildScript(),"Step publication fixture build failed");rig.save(prefix+".script",script.toUtf8());
    std::shared_ptr<TrialTrace> trial;
    if(test.method=="AdamsBashforthMoulton") {
     trial=std::make_shared<TrialTrace>();TrialABM replacement(trial);
     auto *setup=dynamic_cast<PropSetup*>(Moderator::Instance()->GetConfiguredObject("P"));require(setup,"ABM setup absent");
     for(const auto &field:{"InitialStepSize","MinStep","MaxStep","Accuracy"})replacement.SetRealParameter(field,setup->GetPropagator()->GetRealParameter(field));
     setup->SetPropagator(&replacement);
    }
    Observer normal,consumer;bool armed=false;std::exception_ptr failure;
    auto inspect=[&](const Real *data,Integer count) {
     if(failure)return;
     try {
      const auto *metadata=Publisher::GetPublishedIntegrationSteps(data,count);
      if(!enabled)require(!metadata,"Dormant capture attached endpoint metadata");
      if(metadata)for(const auto &step:*metadata)for(const auto &sample:step.states) {
       auto *sat=dynamic_cast<const Spacecraft*>(sample.object);require(sat,"Endpoint identity is not a runtime spacecraft");
       require(sample.object!=Moderator::Instance()->GetConfiguredObject(sample.object->GetName()),"Endpoint bound configured object instead of runtime clone");
       const auto &orbit=sample.object->GetName()=="One"?orbits[0]:orbits[1];
       const double time=double((sample.epoch-GmatTime(21545)).GetTimeInSec());const auto truth=circular(orbit,time);
       for(int c=0;c<6;++c)require(std::isfinite(sample.state[c])&&std::abs(sample.state[c]-truth[c])<(c<3?3e-5:4e-8),"Published accepted endpoint differs from independent circle");
       require(step.initial ? step.startSeconds==step.endSeconds : step.startSeconds!=step.endSeconds,"Seed/accepted model-relative duration is invalid");
      }
     } catch(...) {failure=std::current_exception();}
    };
    normal.observe=inspect;consumer.observe=inspect;
    Debugger *debug=nullptr;for(auto *object:rig.window.children())if(auto *d=dynamic_cast<Debugger*>(object))debug=d;
    const auto nodes=rig.window.missionSnapshot();int arm=-1;for(int i=0;i<nodes.nodes.size();++i)if(nodes.nodes[i].statement.contains("Arm endpoint observers"))arm=i;
    require(debug&&arm>=0&&rig.window.setBreakpoint(arm,true),"Endpoint observer breakpoint absent");
    const auto previous=debug->paused;
    debug->paused=[&](const QString &name) {
     if(previous)previous(name);
     try {require(!armed,"Endpoint arm unexpectedly repeated");require(Publisher::Instance()->Subscribe(&normal),"Legacy observer subscription failed");
      if(enabled) {require(Publisher::Instance()->Subscribe(&consumer),"Endpoint observer subscription failed");Publisher::SetIntegrationStepConsumer(&consumer,true);}
      armed=true;if(auto *dialog=rig.window.findChild<QDialog*>("debuggerDialog"))acceptanceView(*dialog);
     }catch(...) {failure=std::current_exception();}
     QTimer::singleShot(0,&rig.window,[&]{debug->resume();});
    };
    std::vector<AcceptedStepHistory::Step> rawTrials;
    std::unique_ptr<AcceptedStepHistory::Transaction> ambient;
    if(enabled&&trial)ambient.reset(new AcceptedStepHistory::Transaction(rawTrials));
    rig.messages->clear();const auto result=rig.window.runMission();
    if(ambient){ambient->Commit();ambient.reset();}
    debug->paused=previous;rig.window.setBreakpoint(arm,false);events();
    rig.save(prefix+"-messages.txt",rig.messages->toPlainText().toUtf8());
    rig.save(prefix+"-packets.json",QJsonDocument(encode(normal.packets)).toJson());
    rig.save(prefix+"-consumer.json",QJsonDocument(encode(consumer.packets)).toJson());
    if(trial)rig.save(prefix+"-trials.json",QJsonDocument(trial->events).toJson());
    if(!rawTrials.empty()) {QJsonArray raw;for(const auto &entry:rawTrials)raw.append(QJsonObject{{"start",entry.startSeconds},{"end",entry.endSeconds},{"method",QString::fromStdString(entry.method)}});rig.save(prefix+"-ambient-native-trials.json",QJsonDocument(raw).toJson());}
    if(failure)std::rethrow_exception(failure);
    require(armed&&result==MainWindow::RunResult::Completed,"Step publication native mission failed");
    Publisher::Instance()->Unsubscribe(&normal);Publisher::Instance()->Unsubscribe(&consumer);
    require(!Publisher::Instance()->HasIntegrationStepConsumers(),"Unsubscribe retained endpoint consumer registration");
    require(!normal.packets.empty(),"No ordinary propagation publications observed");
    require(!Publisher::GetPublishedIntegrationSteps(normal.packets.back().raw.data(),normal.packets.back().raw.size()),"Endpoint payload escaped synchronous publication lifetime");
    auto *sat=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("One"));require(sat,"Runtime final spacecraft absent");
    require(std::abs((sat->GetEpoch()-21545)*86400-test.end)<2e-5,"Final mission time differs from independent stop oracle");
    if(!enabled) {baseline=normal.packets;continue;}
    require(normal.packets.size()==baseline.size(),"Endpoint capture changed legacy publication count");
    for(size_t i=0;i<baseline.size();++i) {
     require(normal.packets[i].raw.size()==baseline[i].raw.size(),"Endpoint capture changed legacy publication width");
     for(size_t c=0;c<baseline[i].raw.size();++c)require(normal.packets[i].raw[c]==baseline[i].raw[c]||(std::isnan(normal.packets[i].raw[c])&&std::isnan(baseline[i].raw[c])),"Endpoint capture changed a legacy packet value");
    }
    // Ordinary double-MJD command boundaries quantize time by up to one ULP;
    // two adjacent conversions at this epoch remain within the explicit 1 us bound.
    std::map<std::string,double> last;std::map<std::string,int> advances,seeds;bool partial=false,multipleInPacket=false;
    for(const auto &packet:consumer.packets) {
     require(packet.metadata,"Opt-in propagation packet has no endpoint payload");multipleInPacket|=packet.steps.size()>1;
     for(const auto &step:packet.steps) {
      require(!step.states.empty(),"Endpoint has no Cartesian bindings");
      if(test.formation)require(step.states.size()==2&&step.states[0].object->GetName()=="Two"&&step.states[1].object->GetName()=="One","Formation snapshots lost native member order");
      for(const auto &sample:step.states) {
       const auto name=sample.object->GetName();
       const double absoluteEnd=double((step.epoch-GmatTime(21545)).GetTimeInSec());
       const double absoluteStart=absoluteEnd-(step.endSeconds-step.startSeconds);
       if(step.initial) {++seeds[name];if(last.count(name))require(std::abs(absoluteEnd-last[name])<1e-6,"Repeated command seed advanced trajectory");else last[name]=absoluteEnd;continue;}
       require(last.count(name)&&std::abs(absoluteStart-last[name])<1e-6,"Probe, rollback or duplicate leaked into committed trajectory");
       require((step.endSeconds-step.startSeconds)*(test.end>0?1:-1)>0,"Endpoint advanced in wrong direction");
       require(std::abs(absoluteEnd)<=std::abs(test.end)+2e-5,"Crossed-stop overshoot leaked into committed endpoints");
       partial|=std::abs(step.endSeconds-step.startSeconds)<std::abs(test.step)-1e-7;
       ++advances[name];last[name]=absoluteEnd;
      }
     }
    }
    require(std::abs(last["One"]-test.end)<2e-5,"Final accepted endpoint omitted");
    if(test.multiple) {
     require(multipleInPacket&&std::abs(last["Two"]-test.end)<2e-5&&advances["Two"]>advances["One"],"Synchronized fixed interval lost primitive endpoints");
     auto *command=Moderator::Instance()->GetFirstCommand();while(command&&!command->IsOfType("Propagate"))command=command->GetNext();
     auto *setup=command?dynamic_cast<PropSetup*>(command->GetClone(1)):nullptr;
     require(setup&&setup->GetPropagator()->GetRealParameter("MinStep")==3.,"Synchronized fixed interval did not restore configured MinStep");
     fixedIntervalPolicy(setup->GetODEModel());
    }
    else if(test.single)require(advances["One"]==2&&normal.packets.size()==2&&consumer.packets.size()==4,"Single-step metadata/legacy ephemeris filtering differs");
    else require(partial,"Final partial accepted advance was not exercised");
    if(trial) {
     require(rawTrials.size()>size_t(advances["One"]),"Ambient diagnostic did not retain the rolled-back native trials");
     for(const auto &packet:consumer.packets)for(const auto &step:packet.steps)if(!step.initial) {
      bool found=false;
      for(const auto &raw:rawTrials)if(raw.startSeconds==step.startSeconds&&raw.endSeconds==step.endSeconds&&raw.method==step.method&&raw.state.size()==6) {
       bool equal=true;for(int c=0;c<6;++c)equal&=raw.state[c]==step.states[0].state[c];found|=equal;
      }
      require(found,"Published endpoint is absent from the ambient native journal");
     }
    }
    QJsonObject counts;for(const auto &item:advances)counts[QString::fromStdString(item.first)]=item.second;
    completed.append(QJsonObject{{"case",test.name},{"legacy_packets",int(normal.packets.size())},{"consumer_packets",int(consumer.packets.size())},{"committed_advances",counts},{"final_seconds",last["One"]},{"unchanged_legacy_bytes",true}});
    rig.save("case-results.json",QJsonDocument(completed).toJson());
   }
  }
  require(!completed.isEmpty(),"Unknown requested publication case");
  const QJsonObject proof{{"checkpoint","committed-step-publication"},{"outcome","passed"},{"cases",completed},{"origin_translation_contract",true},{"scope","Native committed-step publication; both plot samplers and original NASA rows remain unqualified."}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<std::endl;}
 return 1;
}
