#pragma once
#include "FormationAcceptance.hpp"
#include "Function.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"

namespace plotfunction {
inline QString declaration(formaccept::Rig &rig,const QString &key,const QString &name,const QString &body) {
 const auto file=rig.files.filePath(name+".gmf");
 formaccept::write(file,body.toUtf8());
 rig.save(key+"-"+name+".gmf",body.toUtf8());
 return "Create GmatFunction "+name+";\n"+name+".FunctionPath = '"+file+"';\n";
}

inline QString configure(QString setup,formaccept::Rig &rig,const QString &key) {
 const bool shadow=(key=="FunctionShadow"||key=="FunctionLocalPlot");
 QString outputs="Create Variable ep y vz;\n";
 if(key=="FunctionLocalPlot")outputs+=
   "Create OrbitView InnerOrb;\nInnerOrb.Add = {One};\nInnerOrb.CoordinateSystem = EarthMJ2000Eq;\nInnerOrb.IntegrationStepsToSkip = 1;\n"
   "Create GroundTrackPlot InnerMap;\nInnerMap.Add = {One};\nInnerMap.IntegrationStepsToSkip = 1;\n";
 outputs+="BeginMissionSequence;\n";
 const QString globals=shadow?"Global Orb Map;\n":"Global One Two Orb Map;\n";
 const QString result="ep = One.A1ModJulian;\ny = One.EarthMJ2000Eq.Y;\nvz = One.EarthMJ2000Eq.VZ;\n";
 const QString arg=shadow?"One":"request";
 QString declarations=declaration(rig,key,"StepArc","function [ep,y,vz] = StepArc("+arg+")\n"+outputs+globals+
   "Propagate P(One) {One.ElapsedSecs = 27};\n"+result);
 if(key=="FunctionNested") {
  const auto inner=rig.files.filePath("StepArc.gmf");
  declarations+=declaration(rig,key,"OuterArc","function [ep,y,vz] = OuterArc(request)\nCreate Variable ep y vz;\nCreate GmatFunction StepArc;\nStepArc.FunctionPath = '"+inner+"';\nBeginMissionSequence;\n"+globals+
    "Propagate P(One) {One.ElapsedSecs = 10};\n[ep,y,vz] = StepArc(request);\n");
 }
 setup.replace("BeginMissionSequence;",declarations+"Create Variable Request EpochResult YResult VZResult;\nRequest = 1;\nBeginMissionSequence;\n"+globals);
 return setup;
}

inline QString mission(const QString &key) {
 const bool shadow=(key=="FunctionShadow"||key=="FunctionLocalPlot");
 const QString call="[EpochResult,YResult,VZResult] = "+QString(key=="FunctionNested"?"OuterArc":"StepArc")+"("+(shadow?"Two":"Request")+");\n";
 const QString report="Report Values EpochResult YResult VZResult One.A1ModJulian One.EarthMJ2000Eq.Y Two.A1ModJulian Two.EarthMJ2000Eq.Y;\n";
 QString script="Report 'Arm function observer' Values Request;\nPropagate P(One) {One.ElapsedSecs = 10};\n"+call+report;
 if(key=="FunctionGlobalTwice")script+=call+report;
 return script+"Propagate P(One) {One.ElapsedSecs = "+(shadow?"27":"13")+"};\n"+report;
}

class Observer final : public Subscriber {
public:
 Observer(formaccept::Rig &r,bool local) : Subscriber("FunctionStepObserver","FunctionStepObserver"),rig(&r),localPlots(local) {}
 ~Observer() override { Publisher::Instance()->Unsubscribe(this); }
 GmatBase *Clone() const override {return new Observer(*this);}
 bool ReceiveData(const Real *data,Integer count=0) override {
  bool initial=false;
  const auto *roots=Publisher::GetPublishedPropagationObjects(data,count,initial);
  if(!roots)return true;
  auto *command=dynamic_cast<GmatCommand *>(currentProvider);
  auto *function=command?command->GetCurrentFunction():nullptr;
  QJsonArray targets,steps;
  for(const auto *object:*roots)targets.append(objectId(object));
  if(const auto *samples=Publisher::GetPublishedIntegrationSteps(data,count))for(const auto &sample:*samples) {
   for(const auto &value:sample.states)steps.append(QJsonObject{{"object_id",objectId(value.object)},
      {"object",QString::fromStdString(value.object->GetName())},{"initial",sample.initial},
      {"elapsed",(value.epoch-GmatTime(21545)).GetTimeInSec()},{"y",value.state[1]},{"vz",value.state[5]}});
  }
  QJsonArray subscribers,localViews;
  if(localPlots) {
   for(auto *sub:Publisher::Instance()->GetSubscriberList())if(sub->GetName()=="InnerOrb"||sub->GetName()=="InnerMap")subscribers.append(QString::fromStdString(sub->GetName()));
   if(function&&std::abs((data[0]-21545)*86400-27)<1e-4)for(const auto &plot:QStringList{"InnerOrb","InnerMap"}) {
    QJsonArray points;const auto model=rig->window.plotReceiver()->model(plot);
    if(model)for(const auto &curve:model->curves)if(curve.name=="One")for(const auto &pt:curve.points)
     points.append(QJsonObject{{"elapsed",(pt.epoch-21545)*86400},{"x",pt.x},{"y",pt.y},{"z",pt.z},{"connect",pt.connect}});
    localViews.append(QJsonObject{{"plot",plot},{"present",bool(model)},{"points",points}});
   }
  }
  records.append(QJsonObject{{"local_subscribers",subscribers},{"local_views",localViews},{"function",function?QString::fromStdString(function->GetName()):QString()},
    {"initial",initial},{"raw_elapsed",(data[0]-21545)*86400},{"roots",targets},{"steps",steps}});
  return true;
 }
 QJsonArray records;
protected:
 bool Distribute(Integer) override {return true;}
 bool Distribute(const Real *,Integer) override {return true;}
private:
 formaccept::Rig *rig;
 bool localPlots;
 int objectId(const GmatBase *object) {
  const auto address=reinterpret_cast<std::uintptr_t>(object);
  if(!ids.contains(address))ids.insert(address,ids.size()+1);
  return ids.value(address);
 }
 QMap<std::uintptr_t,int> ids;
};

inline void checkPackets(const QString &name,const QJsonArray &packets) {
 struct Arc {QString owner;QVector<int> times;};
 std::vector<Arc> arcs{{QString(),{0,10}}};
 const bool shadow=(name=="FunctionShadow"||name=="FunctionLocalPlot");
 if(shadow)arcs.insert(arcs.end(),{{"StepArc",{0,10,20,27}},{QString(),{10,20,30,37}}});
 else if(name=="FunctionNested")arcs.insert(arcs.end(),{{"OuterArc",{10,20}},{"StepArc",{20,30,40,47}},{QString(),{47,57,60}}});
 else arcs.insert(arcs.end(),{{"StepArc",{10,20,30,37}},{"StepArc",{37,47,57,64}},{QString(),{64,74,77}}});
 int packetIndex=0;
 for(const auto &arc:arcs)for(int i=0;i<arc.times.size();++i) {
  formaccept::require(packetIndex<packets.size(),"Function propagation packet missing");
  const auto packet=packets[packetIndex++].toObject();
  const bool local=shadow&&!arc.owner.isEmpty();const int id=local?2:1,t=arc.times[i];
  formaccept::require(packet["function"].toString()==arc.owner&&packet["roots"].toArray()==QJsonArray{id}&&packet["initial"].toBool()==(i==0),"Function packet owner, runtime identity or phase differs");
  formaccept::require(std::abs(packet["raw_elapsed"].toDouble()-t)<1e-4&&packet["steps"].toArray().size()==1,"Function packet clock or endpoint count differs");
  if(name=="FunctionLocalPlot") {
   QStringList subscribers;for(const auto &v:packet["local_subscribers"].toArray())subscribers.append(v.toString());subscribers.sort();
   formaccept::require(subscribers==(local?QStringList{"InnerMap","InnerOrb"}:QStringList{}),"Local plot registration escaped function lifetime or was missing inside");
   if(local&&t==27) {
    const auto views=packet["local_views"].toArray();formaccept::require(views.size()==2,"Local plot observations missing");
    for(const auto &v:views) {
     const auto view=v.toObject();const auto points=view["points"].toArray();
     formaccept::require(view["present"].toBool()&&points.size()==2,"Local function plot did not collect its formal spacecraft");
     for(int j=0;j<2;++j) {
      const auto pt=points[j].toObject();const auto state=formaccept::circular(formaccept::orbits[1],j*20);
      formaccept::require(std::abs(pt["elapsed"].toDouble()-j*20)<1e-4&&pt["connect"].toBool()==(j>0),"Local curve clock/connection differs");
      int c=0;for(const auto &field:QStringList{"x","y","z"}) {
       const double x=pt[field].toDouble();formaccept::require(std::isfinite(x),"Nonfinite local coordinate");
       if(view["plot"]=="InnerOrb")formaccept::require(std::abs(x-state[c])<4e-5,"Local plotted state differs from formal spacecraft orbit");++c;
      }
     }
    }
   }
  }
  const auto step=packet["steps"].toArray()[0].toObject();const auto truth=formaccept::circular(formaccept::orbits[local?1:0],t);
  formaccept::require(step["object"]=="One"&&step["object_id"].toInt()==id&&step["initial"].toBool()==(i==0)&&std::abs(step["elapsed"].toDouble()-t)<1e-4,"Function epoch or endpoint identity differs");
  formaccept::require(std::abs(step["y"].toDouble()-truth[1])<4e-5&&std::abs(step["vz"].toDouble()-truth[5])<5e-8,"Function endpoint state differs from independent orbit");
 }
 formaccept::require(packetIndex==packets.size(),"Unexpected function publication");
}

inline QJsonArray checkReport(formaccept::Rig &rig,const QString &key) {
 const bool shadow=(key=="FunctionShadow"||key=="FunctionLocalPlot");
 const QVector<double> returned=shadow?QVector<double>{27,27}:key=="FunctionNested"?QVector<double>{47,47}:QVector<double>{37,64,64};
 const QVector<double> caller=shadow?QVector<double>{10,37}:key=="FunctionNested"?QVector<double>{47,60}:QVector<double>{37,64,77};
 const auto raw=formaccept::read(rig.report);
 rig.save(key+"-report.csv",raw);
 const auto lines=QString::fromUtf8(raw).trimmed().split('\n');
 formaccept::require(lines.size()==returned.size()+1&&lines.front().trimmed()=="1",
   "Function arm sentinel or call/report count differs");
 QJsonArray result;
 for(int i=0;i<returned.size();++i) {
  const auto local=formaccept::circular(formaccept::orbits[shadow?1:0],returned[i]);
  const auto propagated=formaccept::circular(formaccept::orbits[0],caller[i]);
  const auto untouched=formaccept::circular(formaccept::orbits[1],0);
  const double truth[]={returned[i],local[1],local[5],caller[i],propagated[1],0,untouched[1]};
  const auto fields=lines[i+1].trimmed().split(',');
  formaccept::require(fields.size()==7,"Function report schema differs");
  QJsonArray row;
  for(int j=0;j<7;++j) {
   bool ok=false;const double rawValue=fields[j].toDouble(&ok);
   const bool epoch=j==0||j==3||j==5;
   const double value=epoch?(rawValue-21545)*86400:rawValue;
   formaccept::require(ok&&std::abs(value-truth[j])<(epoch?1e-4:j==2?5e-8:4e-5),"Function-local result or caller state differs from independent orbit");
   row.append(value);
  }
  result.append(row);
 }
 return result;
}
}
