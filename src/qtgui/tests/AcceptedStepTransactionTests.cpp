// Immutable accepted primitive steps and transactional admission; no plot credit.
#include "FormationAcceptance.hpp"
#include "AcceptedStepHistory.hpp"
#include "AdamsBashforthMoulton.hpp"
#include <memory>
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include <functional>
#include <limits>
#include <thread>
using namespace formaccept;
namespace {
class StartupJournalABM final:public AdamsBashforthMoulton {
public:
 std::shared_ptr<QJsonArray> journal;
 explicit StartupJournalABM(std::shared_ptr<QJsonArray> data):AdamsBashforthMoulton("StartupJournalABM"),journal(std::move(data)){}
 GmatBase* Clone()const override{return new StartupJournalABM(*this);}
 using AdamsBashforthMoulton::Step;
 void note(const QString &event){QJsonObject entry{{"event",event},{"time",GetTime()},{"startup_count",startupCount},{"startup_complete",startupComplete},{"final_step",finalStep}};if(event=="step-exception"&&startupComplete){entry["estimated_error"]=maxError;QJsonArray delta;for(unsigned i=0;i<dimension;++i)delta.append(std::abs(correctorState[i]-predictorState[i]));entry["predictor_corrector_deltas"]=delta;}journal->append(entry);}
 bool Step()override{note("before-step");try{const bool result=AdamsBashforthMoulton::Step();note(result?"accepted-step":"failed-step");return result;}catch(...){note("step-exception");throw;}}
 void RevertSpaceObject()override{note("before-revert");Propagator::RevertSpaceObject();note("after-revert");}
};
class PacketCounter final:public Subscriber {
public:
 int packets=0;
 PacketCounter():Subscriber("AcceptedStepPacketCounter","AcceptedStepPacketCounter"){}
 ~PacketCounter()override{Publisher::Instance()->Unsubscribe(this);}
 GmatBase* Clone()const override{return new PacketCounter(*this);}
 bool ReceiveData(const Real *data,Integer count=0)override{if(data&&count>0)++packets;return true;}
protected:bool Distribute(Integer)override{return true;}bool Distribute(const Real*,Integer)override{return true;}
};
}
int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("AcceptedStepTransactions");try{
 require(argc==2,"Supply native startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
 const QStringList methods{"RungeKutta89","RungeKutta56","RungeKutta68","PrinceDormand45","PrinceDormand78","PrinceDormand853","BulirschStoer","AdamsBashforthMoulton"};
 const auto warmupMode=qEnvironmentVariable("GMAT_ACCEPTED_STEP_WARMUP","stop");require(warmupMode=="stop"||warmupMode=="single","Unknown warmup mode");
 const auto selectedMethod=qEnvironmentVariable("GMAT_ACCEPTED_STEP_METHOD");require(selectedMethod.isEmpty()||methods.contains(selectedMethod),"Unknown requested matrix integrator");
 QJsonArray observations;Propagator *last=nullptr;const PhysicalModel *lastModel=nullptr;
 for(const auto &method:methods) for(int direction:{1,-1}){
  if(!selectedMethod.isEmpty()&&selectedMethod!=method)continue;
  const double primitive=method=="AdamsBashforthMoulton"?1.:5.;
  const QString key=method+(direction==1?"Forward":"Backward");
  QString script="Create Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+number(std::sqrt(mu/7000.))+";\nSat.VZ = 0;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.SRP = Off;\nFM.Drag = None;\nCreate Propagator P;\nP.FM = FM;\nP.Type = "+method+";\nP.InitialStepSize = "+number(direction*primitive)+";\nP.MinStep = "+number(primitive)+";\nP.MaxStep = "+number(primitive)+";\nP.Accuracy = 1e-10;\nBeginMissionSequence;\nPropagate "+QString(direction<0?"BackProp ":"")+"P(Sat) {Sat.ElapsedSecs = "+number(direction*primitive)+"};\n";
  if(warmupMode=="single")script=script.left(script.indexOf("BeginMissionSequence;"))+"BeginMissionSequence;\nPropagate "+QString(direction<0?"BackProp ":"")+"P(Sat);\n";
  rig.source->setPlainText(script);require(rig.window.buildScript(),"Accepted-step matrix initial build failed");rig.save(key+".script",script.toUtf8());
  std::shared_ptr<QJsonArray> startupJournal;
  if(method=="AdamsBashforthMoulton"){
   startupJournal=std::make_shared<QJsonArray>();StartupJournalABM replacement(startupJournal);auto *setup=dynamic_cast<PropSetup*>(Moderator::Instance()->GetConfiguredObject("P"));require(setup,"ABM configured propagator missing");
   for(const auto &name:{"InitialStepSize","MinStep","MaxStep","Accuracy"})replacement.SetRealParameter(name,setup->GetPropagator()->GetRealParameter(name));
   setup->SetPropagator(&replacement);
  }
  require(Moderator::Instance()->RunMission()==1,"Accepted-step native initialization failed");events();acceptanceView(rig.window);
  auto *command=Moderator::Instance()->GetFirstCommand();while(command&&!command->IsOfType("Propagate"))command=command->GetNext();
  auto *runtime=command?dynamic_cast<PropSetup*>(command->GetClone(0)):nullptr;auto *p=runtime?runtime->GetPropagator():nullptr;
  require(p&&p->GetTypeName()==method.toStdString()&&std::abs(p->GetTime()-direction*primitive)<1e-8,"Actual native propagator clone/time differs");
  std::vector<AcceptedStepHistory::Step> committed;PacketCounter observer;require(Publisher::Instance()->Subscribe(&observer),"Native packet observer unavailable");
  {
   AcceptedStepHistory::Transaction trial(committed);require(AcceptedStepHistory::IsCapturing(),"Explicit transaction did not capture");
   try { require(p->Step(direction*primitive*10),"Fixed-interval native step failed"); }
   catch(BaseException &error){QJsonArray captured;for(const auto &step:trial.Steps()){QJsonArray state;for(auto value:step.state)state.append(value);captured.append(QJsonObject{{"start",step.startSeconds},{"end",step.endSeconds},{"method",QString::fromStdString(step.method)},{"state",state}});}rig.save(key+"-exception-steps.json",QJsonDocument(captured).toJson());if(startupJournal)rig.save(key+"-startup-history.json",QJsonDocument(*startupJournal).toJson());rig.save(key+"-native-error.txt",QString::fromStdString(error.GetFullMessage()).toUtf8());throw;}

   QJsonArray rawSteps;for(const auto &step:trial.Steps()){QJsonArray state;for(auto value:step.state)state.append(value);rawSteps.append(QJsonObject{{"start",step.startSeconds},{"end",step.endSeconds},{"method",QString::fromStdString(step.method)},{"state",state}});}
   rig.save(key+"-candidate-steps.json",QJsonDocument(rawSteps).toJson());
   require(committed.empty()&&trial.Steps().size()==10,"Primitive endpoints leaked or were counted per enclosing call");
   trial.Commit();require(!AcceptedStepHistory::IsCapturing(),"Committed transaction still records");
  }
  if(startupJournal){rig.save(key+"-startup-history.json",QJsonDocument(*startupJournal).toJson());for(auto value:*startupJournal){const auto event=value.toObject();if(event["event"]=="after-revert")require(event["startup_count"].toInt()==0&&!event["startup_complete"].toBool(),"Physical rollback retained invalid ABM startup history");}}
  Publisher::Instance()->Unsubscribe(&observer);require(!AcceptedStepHistory::IsCapturing()&&observer.packets==0&&committed.size()==10,"Capture published packets or lost primitive steps");
  QJsonArray steps;
  for(size_t i=0;i<committed.size();++i){const auto &step=committed[i];const double time=direction*primitive+direction*primitive*(int(i)+1),begin=direction*primitive+direction*primitive*int(i);
   require(step.model&&step.integrator&&step.state.size()==6&&std::abs(step.startSeconds-begin)<1e-8&&std::abs(step.endSeconds-time)<1e-8,"Actual accepted primitive time/identity/width differs");
   require(i==0||step.model==committed.front().model,"One propagator changed its native force-model identity");
   const auto expected=circular(orbits[0],time);QJsonArray actual;
   for(int j=0;j<6;++j){require(std::isfinite(step.state[j])&&std::abs(step.state[j]-expected[j])<(j<3?3e-5:4e-8),"Captured native endpoint differs from independent circle");actual.append(step.state[j]);}
   steps.append(QJsonObject{{"start",step.startSeconds},{"end",step.endSeconds},{"method",QString::fromStdString(step.method)},{"state",actual}});
  }
  if(method=="AdamsBashforthMoulton"){bool startup=false,corrector=false;for(const auto &step:committed){startup|=step.method=="RungeKutta89";corrector|=step.method=="AdamsBashforthMoulton";}require(startup&&corrector,"ABM capture did not exercise both startup and genuine corrector steps");}
  auto *sat=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("Sat"));require(sat&&std::abs((sat->GetEpoch()-21545)*86400-direction*primitive)<1e-5,"Capture changed the live spacecraft publication epoch");
  const auto unchanged=circular(orbits[0],direction*primitive);for(int i=0;i<6;++i)require(std::isfinite(sat->GetState()[i])&&std::abs(sat->GetState()[i]-unchanged[i])<(i<3?3e-5:4e-8),"Capture changed the live spacecraft Cartesian state");
  observations.append(QJsonObject{{"case",key},{"primitive_step_seconds",primitive},{"primitive_steps",steps},{"raw_publications",observer.packets}});rig.save("accepted-matrix.json",QJsonDocument(observations).toJson());last=p;lastModel=committed.front().model;
 }
 require(last&&lastModel,"No native matrix context");
 int rejected=0;auto reject=[&](const std::function<void()> &action){bool caught=false;try{action();}catch(BaseException&){caught=true;}require(caught,"Invalid transaction operation accepted");++rejected;};
 reject([&]{AcceptedStepHistory::Transaction orphan;});
 std::vector<AcceptedStepHistory::Step> output;
 auto record=[&](double a,double b){AcceptedStepHistory::Record(last,lastModel,a,b,last->GetState(),6);};
 { AcceptedStepHistory::Transaction outer(output);record(0,1);
  { AcceptedStepHistory::Transaction probe;record(1,2); }require(outer.Steps().size()==1&&output.empty(),"Discarded probe leaked endpoints");
  try { AcceptedStepHistory::Transaction probe;record(1,3);throw std::runtime_error("intentional transaction unwind"); }catch(const std::runtime_error&){}
  require(outer.Steps().size()==1&&AcceptedStepHistory::IsCapturing(),"Exception lost outer transaction");
  { AcceptedStepHistory::Transaction child;record(1,4);reject([&]{outer.Commit();});child.Commit();reject([&]{child.Commit();}); }
  require(outer.Steps().size()==2&&output.empty(),"Nested committed records bypassed outer staging");
  reject([&]{AcceptedStepHistory::Transaction secondRoot(output);});
  reject([&]{AcceptedStepHistory::Record(nullptr,lastModel,0,1,last->GetState(),6);});
  reject([&]{AcceptedStepHistory::Record(last,lastModel,0,1,nullptr,6);});
  reject([&]{AcceptedStepHistory::Record(last,lastModel,0,1,last->GetState(),0);});
  reject([&]{record(0,std::numeric_limits<double>::quiet_NaN());});
  record(4,4);require(outer.Steps().size()==2,"Zero advance counted as a step");
  std::exception_ptr failure;std::vector<AcceptedStepHistory::Step> separate;
  std::thread worker([&]{try{require(!AcceptedStepHistory::IsCapturing(),"Capture scope leaked across threads");AcceptedStepHistory::Transaction own(separate);record(10,11);own.Commit();}catch(...){failure=std::current_exception();}});worker.join();if(failure)std::rethrow_exception(failure);
  require(separate.size()==1&&outer.Steps().size()==2&&output.empty(),"Thread-local transactions contaminated parent");
  outer.Commit();reject([&]{outer.Commit();});
 }
 require(output.size()==2&&!AcceptedStepHistory::IsCapturing()&&rejected==9,"Committed output or transaction restoration differs");
 { AcceptedStepHistory::Transaction discard(output);record(5,6); }require(output.size()==2&&!AcceptedStepHistory::IsCapturing(),"Uncommitted root appended records");
 AcceptedStepHistory::Record(nullptr,nullptr,0,1,nullptr,0);require(output.size()==2,"Dormant recorder touched output");
 const QJsonObject proof{{"checkpoint","accepted-step-transaction-matrix"},{"outcome","passed"},{"native_cases",observations},{"independent_state_values",observations.size()*10*6},{"all_eight_methods_exercised",observations.size()==16},{"method_filter",selectedMethod},{"warmup_mode",warmupMode},{"rejected_operations",rejected},{"thread_isolated",true},{"nested_discard_and_commit",true},{"no_publication_or_live_spacecraft_mutation",true},{"facets",QJsonArray{"actual-primitive-steps-in-fixed-intervals","independent-forward-backward-circle-endpoints","transactional-trial-discard-and-commit","nested-exception-and-thread-isolation"}},{"scope","Dormant accepted-endpoint capture for all eight registered numerical integrators, both directions. This records force-origin integration states only; Propagate commit/rollback, J2000 conversion, plot sampling and the four original NASA skip rows remain unqualified until integrated and independently checked."}};
 rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException&e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;}
