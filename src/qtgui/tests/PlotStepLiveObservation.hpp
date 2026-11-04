#ifndef GMAT_PLOT_STEP_LIVE_OBSERVATION_HPP
#define GMAT_PLOT_STEP_LIVE_OBSERVATION_HPP
#include "FormationAcceptance.hpp"
#include "QtPlotReceiver.hpp"
#include "Subscriber.hpp"
#include <QTimer>

// This observer deliberately uses literal fixture step grids, not the native
// integration journal or the consumer's cadence-selection implementation.
class PlotStepStream final : public Subscriber {
public:
   std::function<void(const Real *,Integer)> sample;
   std::function<void()> flush;
   PlotStepStream():Subscriber("PlotStepStream","PlotStepStream") {}
   ~PlotStepStream() override { Publisher::Instance()->Unsubscribe(this); }
   GmatBase *Clone() const override { return new PlotStepStream(*this); }
   bool ReceiveData(const Real *v,Integer n=0) override {
      if(v && n>=7 && sample)sample(v,n); return true;
   }
   bool FlushData(bool end=true) override {
      if(flush)flush(); return Subscriber::FlushData(end);
   }
protected:
   bool Distribute(Integer) override { return true; }
   bool Distribute(const Real *,Integer) override { return true; }
};

class PlotStepMembership {
   struct Point { double seconds; int pass; bool accepted; };
   formaccept::Rig &rig;
   QString name,mode,phase="before";
   QVector<Point> selected;
   QVector<double> rawPhase;
   QJsonArray observations,phases;
   int pass=0,flushes=0,publications=0,retained=1000;
   bool accepted=true,armed=false;
   std::exception_ptr failure;
   PlotStepStream stream;

   void guard(const std::function<void()> &f) {
      if(failure)return;
      try { f(); } catch(...) {
         failure=std::current_exception();
         QTimer::singleShot(0,&rig.window,[this]{rig.window.stopMission();});
      }
   }
   void finishPhase() {
      const QVector<double> expected=phase=="before"?QVector<double>{0,10}:
         phase=="solver"?QVector<double>{10,20,30,37}:QVector<double>{37,47,50};
      QJsonArray raw;for(double t:rawPhase)raw.append(t);
      phases.append(QJsonObject{{"phase",phase},{"pass",pass},{"accepted",accepted},{"raw_seconds",raw}});
      rig.save(name+"-live-phases.json",QJsonDocument(phases).toJson());
      formaccept::require(rawPhase==expected,"Live solver raw publication grid differs from literal fixture grid");
      rawPhase.clear();
   }
   void check(const QString &boundary,bool flushed) {
      QJsonArray plots;
      for(const auto &plot:QStringList{"Orb","Map"}) {
         // The native OrbitView Current backend emits its buffered trial only
         // at FlushData. GroundTrack and the other modes update at publication.
         if(plot=="Orb"&&mode=="Current"&&!flushed)continue;
         auto model=rig.window.plotReceiver()->model(plot);
         formaccept::require(bool(model),"Live step viewer absent");
         const PlotCurve *curve=nullptr;
         for(const auto &c:model->curves)if(c.name=="One")curve=&c;
         QVector<Point> expected;
         // MaxPlotPoints is a streaming storage bound. Once a point has been
         // evicted, later trial cleanup cannot resurrect it. Apply the bound
         // after each literal selected event, not to an unlimited filtered log.
         for(const auto &p:selected) {
            if(mode=="None"&&!p.accepted)continue;
            if(mode=="Current")expected.erase(std::remove_if(expected.begin(),expected.end(),[&](const Point &old){return !old.accepted&&old.pass!=p.pass;}),expected.end());
            expected<<p;
            while(expected.size()>retained)expected.removeFirst();
         }
         if(mode=="Current")expected.erase(std::remove_if(expected.begin(),expected.end(),[&](const Point &old){return !old.accepted&&old.pass!=pass;}),expected.end());
         QJsonArray actual,wanted;
         if(curve)for(const auto &p:curve->points)actual.append(QJsonObject{
            {"seconds",(p.epoch-21545)*86400},{"solver",p.solver},{"x",p.x},{"y",p.y},{"z",p.z}});
         for(const auto &p:expected)wanted.append(QJsonObject{{"seconds",p.seconds},{"solver",!p.accepted}});
         plots.append(QJsonObject{{"plot",plot},{"actual",actual},{"expected",wanted}});
         observations.append(QJsonObject{{"boundary",boundary},{"pass",pass},{"phase",phase},
            {"accepted",accepted},{"publication",publications},{"raw_seconds",rawPhase.isEmpty()?-1:rawPhase.last()},{"retention_limit",retained},{"plots",plots}});
         rig.save(name+"-live-membership.json",QJsonDocument(observations).toJson());
         if(actual.size()!=expected.size())throw std::runtime_error((plot+" live step membership differs at "+boundary+": "+QString::number(actual.size())+" vs "+QString::number(expected.size())).toStdString());
         for(int i=0;i<expected.size();++i) {
            const auto &p=curve->points[i];const auto &truth=expected[i];
            formaccept::require(std::abs((p.epoch-21545)*86400-truth.seconds)<.0001&&p.solver==!truth.accepted,
               "Live step epoch/trial membership differs from independent solver-state oracle");
            formaccept::require(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z),"Live step coordinate nonfinite");
            if(plot=="Orb") {
               const auto state=formaccept::circular(formaccept::orbits[0],truth.seconds);
               formaccept::require(std::abs(p.x-state[0])<.00004&&std::abs(p.y-state[1])<.00004&&std::abs(p.z-state[2])<.00004,
                  "Live step position differs from independent circular solution");
            }
         }
      }
      formaccept::events();
   }
public:
   PlotStepMembership(formaccept::Rig &r,const QString &n,const QString &m,int limit=1000):rig(r),name(n),mode(m),retained(limit) {
      stream.sample=[this](const Real *data,Integer count){guard([&]{
         formaccept::require(armed&&count>=7,"Unarmed live step observation");
         const double t=(data[0]-21545)*86400;
         const auto state=formaccept::circular(formaccept::orbits[0],t);
         for(int i=0;i<6;++i)formaccept::require(std::isfinite(data[i+1])&&std::abs(data[i+1]-state[i])<(i<3?.00004:.00000005),"Live raw publication differs from circular oracle");
         const auto grid=phase=="before"?QVector<double>{0,10}:phase=="solver"?QVector<double>{10,20,30,37}:QVector<double>{37,47,50};
         int at=-1;for(int i=0;i<grid.size();++i)if(std::abs(grid[i]-t)<.0001)at=i;
         formaccept::require(at>=0,"Live raw epoch outside literal grid");
         // Repeated adjacent boundary publications do not represent an advance.
         if(rawPhase.isEmpty()||rawPhase.last()!=grid[at]) {
            rawPhase<<grid[at];
            if((phase=="before"&&at==0)||(phase=="solver"&&(at==1||at==3))||(phase=="after"&&at==2))
               selected.append({grid[at],pass,accepted});
         }
         ++publications;check("publication",false);
      });};
      stream.flush=[this]{if(armed)guard([&]{++flushes;check("native-flush",true);});};
   }
   void arm() { guard([&]{formaccept::require(!armed&&Publisher::Instance()->Subscribe(&stream),"Live step attach failed");armed=true;}); }
   void enterPass(bool isAccepted) { guard([&]{finishPhase();phase="solver";++pass;accepted=isAccepted;}); }
   void regularAfter() { guard([&]{finishPhase();phase="after";pass=0;accepted=true;}); }
   QJsonObject finish() {
      Publisher::Instance()->Unsubscribe(&stream);armed=false;
      if(failure)std::rethrow_exception(failure);
      finishPhase();check("completed",true);
      formaccept::require(flushes>=3&&publications>10,"Live membership observation matrix incomplete");
      return {{"publications",publications},{"flushes",flushes},{"phase_count",phases.size()},
         {"observation_count",observations.size()},{"orbit_current_boundary","native flush"}};
   }
};
#endif
