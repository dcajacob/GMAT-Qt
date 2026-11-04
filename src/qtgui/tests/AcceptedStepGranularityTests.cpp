// Diagnose real accepted integration steps without changing production code.
#include "FormationAcceptance.hpp"
#include "RungeKutta89.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include <memory>
using namespace formaccept;

struct Trace { bool enabled=false; QJsonArray steps; int publications=0; };
class RecordingRK89 final:public RungeKutta89 {
public:
 std::shared_ptr<Trace> trace;
 explicit RecordingRK89(std::shared_ptr<Trace> t):RungeKutta89("RecordedRK89"),trace(std::move(t)){}
 GmatBase *Clone()const override{return new RecordingRK89(*this);}
 using RungeKutta89::Step;
 bool Step()override{
  const bool ok=RungeKutta89::Step();
  if(ok&&trace->enabled){QJsonArray state;for(int i=0;i<6;++i)state.append(GetState()[i]);trace->steps.append(QJsonObject{{"seconds",GetTime()},{"state",state}});}
  return ok;
 }
};
class PublicationCounter final:public Subscriber {
public:
 std::shared_ptr<Trace> trace;
 explicit PublicationCounter(std::shared_ptr<Trace> t):Subscriber("StepGranularityObserver","StepGranularityObserver"),trace(std::move(t)){}
 ~PublicationCounter()override{Publisher::Instance()->Unsubscribe(this);}
 GmatBase *Clone()const override{return new PublicationCounter(*this);}
 bool ReceiveData(const Real *data,Integer count=0)override{if(data&&count>0)++trace->publications;return true;}
protected:
 bool Distribute(Integer)override{return true;}
 bool Distribute(const Real *,Integer)override{return true;}
};

int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("AcceptedStepGranularity");try{
 require(argc==2,"Supply native startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
 const QString script="Create Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+number(std::sqrt(mu/7000.))+";\nSat.VZ = 0;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.SRP = Off;\nFM.Drag = None;\nCreate Propagator P;\nP.FM = FM;\nP.Type = RungeKutta89;\nP.InitialStepSize = 10;\nP.MinStep = 0;\nP.MaxStep = 10;\nP.Accuracy = 1e-12;\nBeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 10};\n";
 rig.source->setPlainText(script);require(rig.window.buildScript(),"Granularity baseline build failed");rig.save("source.script",script.toUtf8());
 auto *setup=dynamic_cast<PropSetup *>(Moderator::Instance()->GetConfiguredObject("P"));require(setup&&setup->GetPropagator(),"Configured native propagator absent");
 auto trace=std::make_shared<Trace>();RecordingRK89 recording(trace);
 for(const auto &name:{"InitialStepSize","MinStep","MaxStep","Accuracy"})recording.SetRealParameter(name,setup->GetPropagator()->GetRealParameter(name));
 setup->SetPropagator(&recording);require(Moderator::Instance()->RunMission()==1,"Native preparation mission failed");events();acceptanceView(rig.window);
 auto *command=Moderator::Instance()->GetFirstCommand();while(command&&!command->IsOfType("Propagate"))command=command->GetNext();
 auto *runtime=command?dynamic_cast<PropSetup *>(command->GetClone(0)):nullptr;
 auto *integrator=runtime?dynamic_cast<RecordingRK89 *>(runtime->GetPropagator()):nullptr;
 require(integrator&&std::abs(integrator->GetTime()-10)<1e-8,"Actual cloned native recorder or initial time differs");
 PublicationCounter observer(trace);require(Publisher::Instance()->Subscribe(&observer),"Native publication observer unavailable");
 trace->enabled=true;const bool stepped=integrator->Step(100.0);trace->enabled=false;
 Publisher::Instance()->Unsubscribe(&observer);
 rig.save("accepted-steps.json",QJsonDocument(trace->steps).toJson());
 require(stepped&&trace->steps.size()==10&&trace->publications==0,"One fixed-interval call did not contain ten unpublished accepted native steps");
 for(int n=0;n<10;++n){const auto row=trace->steps[n].toObject();const double time=20+10*n;require(std::abs(row["seconds"].toDouble()-time)<1e-8,"Accepted native time differs from independent grid");
  const auto expected=circular(orbits[0],time);const auto state=row["state"].toArray();require(state.size()==6,"Native step width differs");
  for(int i=0;i<6;++i)require(std::isfinite(state[i].toDouble())&&std::abs(state[i].toDouble()-expected[i])<(i<3?3e-5:4e-8),"Accepted native state differs from independent circle");
 }
 const QJsonObject proof{{"checkpoint","accepted-step-granularity"},{"outcome","passed"},{"fixed_interval_seconds",100},{"maximum_step_seconds",10},{"accepted_steps",trace->steps},{"raw_publications_during_interval",trace->publications},
  {"scope","One real native fixed-interval RK89 call contains ten accepted numerical steps and no publisher callbacks. Test-only subclass calls the unchanged native Step exactly once; this diagnoses a producer identity gap and earns zero NASA/GUI-mechanism credit."}};
 rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
