#pragma once
#include "OrbitTrialHistory.hpp"
#include <QStringList>
#include <stdexcept>

namespace OrbitTrialHistoryChecksDetail {
inline void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
struct Fixture {
   PlotModel model{PlotModel::Kind::Orbit};
   OrbitTrialHistory history;
   QStringList names;
   explicit Fixture(QStringList objects={"Sat"}) : names(std::move(objects))
   {
      int index=0;
      for (const auto &name:names) { model.curves[index++].name=name; PlotVector vector; vector.name=name+" velocity"; vector.source=name; model.vectors.append(vector); }
      model.resetView.emplace(); model.cameraViews.resize(2); model.cameraViews[1].reset.emplace();
      publish(false,names,1);
      for (const auto &name:names) {
         PlotRegularArc arc; arc.id=77; arc.name="Accepted propagation"; arc.first=arc.last=model.curves[names.indexOf(name)].points.front(); arc.finalized=true;
         model.regularArcs[name].append(arc);
      }
   }
   void publish(bool trial,const QStringList &objects,double epoch)
   {
      ++model.frame;
      for (const auto &name:objects) {
         const int index=names.indexOf(name); require(index>=0,"Unknown synthetic object");
         model.append(index,double(model.frame),index,0,epoch,true,trial);
         model.curves[index].points.back().provider=model.frame%2 ? "Provider A" : "Provider B";
         for (auto &vector:model.vectors) if (vector.source==name) { PlotVectorSample sample; sample.source=model.curves[index].points.back(); vector.samples.push_back(sample); }
      }
      PlotCamera camera; camera.frame=model.frame; camera.solver=trial; camera.eye={double(model.frame),0,1};
      model.cameras.push_back(camera); model.resetView->cameras.push_back(camera);
      model.cameraViews[1].cameras.push_back(camera); model.cameraViews[1].reset->cameras.push_back(camera);
      history.observe(model);
   }
   void trial(quint64 maximum)
   {
      // Epoch/provider changes are deliberately independent of the boundary.
      publish(true,names,10); publish(true,names,9);
      history.finalize(model,OrbitTrialHistory::Stage::Solving,maximum,QSet<QString>{"Sat","OtherSat"});
   }
   int trials(const QString &name) const { return history.finalizedCount(name); }
   int points(const QString &name,bool trial) const
   {
      int count=0; for (const auto &curve:model.curves) if (curve.name==name) for (const auto &point:curve.points) if (point.solver==trial) ++count; return count;
   }
   bool cameraAt(quint64 frame) const
   {
      for (const auto &camera:model.cameras) if (camera.frame==frame) return true; return false;
   }
   bool vectorAt(const QString &name,quint64 frame) const
   {
      for (const auto &vector:model.vectors) if (vector.source==name) for (const auto &sample:vector.samples) if (sample.source.frame==frame) return true; return false;
   }
};
}

inline void runOrbitTrialHistoryChecks()
{
   using namespace OrbitTrialHistoryChecksDetail;
   for (const auto capacity:{quint64(0),quint64(2),quint64(4)}) {
      Fixture fixture;
      for (int iteration=0;iteration<6;++iteration) fixture.trial(capacity);
      const int expected=capacity==0 ? 6 : static_cast<int>(capacity);
      require(fixture.trials("Sat")==expected,"LastN 0/1/2 finalized trajectory capacity failed");
      require(fixture.points("Sat",true)==2*expected,"LastN discarded a partial finalized trajectory");
      require(fixture.points("Sat",false)==1 && fixture.model.curves[0].points.front().frame==1,"LastN discarded an accepted point");
      require(fixture.model.regularArcs.value("Sat").size()==1 && fixture.model.regularArcs.value("Sat").front().id==77 && fixture.model.regularArcs.value("Sat").front().name=="Accepted propagation","LastN changed regular segment labels");
      const auto pointCount=fixture.model.curves[0].points.size();
      fixture.history.finalize(fixture.model,OrbitTrialHistory::Stage::Solving,capacity,{"Sat"});
      require(fixture.trials("Sat")==expected && fixture.model.curves[0].points.size()==pointCount,"Empty finalization consumed a trajectory slot");
   }
   {
      Fixture fixture;
      fixture.trial(2); fixture.trial(2);
      fixture.publish(true,{"Sat"},10); fixture.publish(true,{"Sat"},10); fixture.publish(true,{"Sat"},9);
      require(fixture.trials("Sat")==2 && fixture.history.groups.value("Sat").size()==3,"In-progress/duplicate epochs consumed a finalized slot");
      fixture.history.finalize(fixture.model,OrbitTrialHistory::Stage::Solving,2,{"Sat"});
      require(fixture.trials("Sat")==2 && fixture.points("Sat",true)==5,"Explicit trial boundary was not applied");
      require(fixture.model.curves[0].points[1].connect==false,"Retained trajectory start joined a discarded group");
   }
   {
      Fixture fixture;
      fixture.trial(2); fixture.trial(2);
      const auto removed=fixture.history.groups.value("Sat").front().frames;
      const auto oldFrame=*removed.cbegin();
      fixture.model.replayCursor=PlotReplayCursor{oldFrame,oldFrame,0,10};
      fixture.model.curves[0].replayPose=fixture.model.curves[0].points[1];
      fixture.model.vectors[0].replaySample=fixture.model.vectors[0].samples[1];
      const auto generation=fixture.model.historyGeneration;
      fixture.trial(2);
      require(!fixture.cameraAt(oldFrame) && !fixture.vectorAt("Sat",oldFrame),"LastN retained discarded trial camera/vector samples");
      require(!fixture.model.replayCursor && !fixture.model.curves[0].replayPose && !fixture.model.vectors[0].replaySample,"LastN replay still points into discarded history");
      for (const auto &camera:fixture.model.resetView->cameras) require(!removed.contains(camera.frame),"Default camera retained discarded trial frames");
      for (const auto &camera:fixture.model.cameraViews[1].cameras) require(!removed.contains(camera.frame),"Named camera retained discarded trial frames");
      for (const auto &camera:fixture.model.cameraViews[1].reset->cameras) require(!removed.contains(camera.frame),"Named default camera retained discarded trial frames");
      require(fixture.cameraAt(1) && fixture.vectorAt("Sat",1) && fixture.model.historyGeneration>generation,"Accepted display state or replay timeline was lost");
   }
   {
      Fixture fixture({"Sat","Moon"});
      fixture.publish(true,{"Sat","Moon"},10); const auto shared=fixture.model.frame;
      fixture.history.finalize(fixture.model,OrbitTrialHistory::Stage::Solving,2,{"Sat"});
      fixture.publish(true,{"Sat"},20); fixture.history.finalize(fixture.model,OrbitTrialHistory::Stage::Solving,2,{"Sat"});
      fixture.publish(true,{"Sat"},30); fixture.history.finalize(fixture.model,OrbitTrialHistory::Stage::Solving,2,{"Sat"});
      require(fixture.trials("Sat")==2 && fixture.trials("Moon")==1,"LastN capacity was applied globally rather than per object");
      require(fixture.cameraAt(shared) && !fixture.vectorAt("Sat",shared) && fixture.vectorAt("Moon",shared),"Shared publication camera/vector ownership was not preserved");
   }
   {
      Fixture fixture;
      for (int iteration=0;iteration<4;++iteration) fixture.trial(0);
      fixture.history.finalize(fixture.model,OrbitTrialHistory::Stage::SolvedPass,0,{"Sat"});
      require(fixture.trials("Sat")==3 && fixture.history.solvedRemovals==1,"First solved-pass cleanup did not remove one latest spacecraft group");
      fixture.history.finalize(fixture.model,OrbitTrialHistory::Stage::SolvedPass,0,{"Sat"});
      require(fixture.trials("Sat")==2 && fixture.history.solvedRemovals==2,"Second solved-pass cleanup did not remove one latest spacecraft group");
      fixture.history.finalize(fixture.model,OrbitTrialHistory::Stage::SolvedPass,0,{"Sat"});
      require(fixture.trials("Sat")==2,"Repeated solved-pass callbacks evicted old history");
      fixture.publish(true,{"Sat"},40); require(fixture.history.solvedRemovals==0,"New trial did not reset the frame cleanup guard");
   }
   {
      Fixture fixture({"Sat","Moon"});
      for (int iteration=0;iteration<4;++iteration) fixture.trial(0);
      fixture.history.finalize(fixture.model,OrbitTrialHistory::Stage::SolvedPass,0,{"Sat"});
      require(fixture.trials("Sat")==2 && fixture.trials("Moon")==0 && fixture.history.solvedRemovals==2,"Frame-wide solved-pass listener/body cleanup disagrees with OF");
      require(fixture.points("Moon",false)==1 && fixture.points("Sat",false)==1 && fixture.model.regularArcs.size()==2,"Solved-pass cleanup removed accepted history");
      fixture.history.finalize(fixture.model,OrbitTrialHistory::Stage::SolvedPass,0,{"Sat"});
      require(fixture.trials("Sat")==2,"Two-object solved-pass cleanup exceeded its frame guard");
      fixture.history.clearTrials(fixture.model);
      require(fixture.history.groups.isEmpty() && fixture.points("Sat",true)==0 && fixture.points("Moon",true)==0 && fixture.points("Sat",false)==1,"ClearSolverData left trial groups or removed accepted data");
   }
   {
      Fixture fixture; fixture.model.maxPoints=3;
      for (int point=0;point<25;++point) fixture.publish(true,{"Sat"},point);
      require(fixture.history.groups.value("Sat").size()==1 && fixture.history.groups.value("Sat").back().frames.size()<=6,"Long active trajectory bookkeeping grew without its amortized trim");
      fixture.history.finalize(fixture.model,OrbitTrialHistory::Stage::Solving,2,{"Sat"});
      require(fixture.history.groups.value("Sat").back().frames.size()==3,"Finalization did not synchronize active retained frames");
   }
   {
      Fixture fixture; fixture.trial(0); fixture.trial(0);
      fixture.model.maxPoints=2; fixture.model.trim(); fixture.history.synchronize(fixture.model);
      require(fixture.trials("Sat")==1,"MaxPlotPoints trimming left expired trajectory slots");
      fixture.model.clear(); fixture.history.clear();
      require(fixture.history.groups.isEmpty() && fixture.model.cameras.empty() && fixture.model.regularArcs.isEmpty() && fixture.model.curves[0].points.empty(),"Data reset left LastN history armed");
   }
}
