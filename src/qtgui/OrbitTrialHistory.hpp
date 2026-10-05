#pragma once
#include "PlotModel.hpp"
#include <algorithm>

// Trial identity is assigned only from explicit end-of-receive lifecycle,
// never from provider identity, epoch resets or XY solver-break heuristics.
// The receiver calls observe() after one real UpdateGlPlot publication;
// metadata-only camera preparation must not call it.
class OrbitTrialHistory
{
public:
   struct Group { quint64 id=0; QSet<quint64> frames; bool finalized=false; };
   enum class Stage { Solving, SolvedPass, Regular };
   QMap<QString,QVector<Group>> groups;
   quint64 nextGroup=0;
   unsigned solvedRemovals=0;

   void clear() { groups.clear(); nextGroup=0; solvedRemovals=0; }
   void observe(PlotModel &model)
   {
      bool synchronizeNeeded=false;
      const auto activeLimit=quint64(2)*static_cast<quint64>(std::max(1,model.maxPoints));
      for (auto &curve:model.curves) {
         if (curve.points.empty()) continue;
         auto &point=curve.points.back(); if (!point.solver || point.frame!=model.frame) continue;
         auto &object=groups[curve.name];
         if (object.isEmpty() || object.back().finalized) {
            object.append({++nextGroup,{},false}); point.connect=false;
            // Legacy dealer resets the frame cleanup guard on a new solution.
            solvedRemovals=0;
         }
         object.back().frames.insert(point.frame);
         synchronizeNeeded=synchronizeNeeded || static_cast<quint64>(object.back().frames.size())>activeLimit;
      }
      // Long single propagations trim their collected points continuously.
      // Intersect their frame sets only once per O(maxPoints) publications,
      // rather than scanning all retained history at every real sample.
      if (synchronizeNeeded) synchronize(model);
   }
   void synchronize(const PlotModel &model)
   {
      // MaxPlotPoints trimming may expire a complete group. Empty groups never
      // occupy a visible finalized slot; preserve the object's dealer-map key.
      for (auto it=groups.begin();it!=groups.end();++it) {
         QSet<quint64> retained;
         for (const auto &curve:model.curves) if (curve.name==it.key())
            for (const auto &point:curve.points) if (point.solver) retained.insert(point.frame);
         for (auto &group:it.value()) group.frames.intersect(retained);
         auto &object=it.value(); object.erase(std::remove_if(object.begin(),object.end(),[](const Group &group) { return group.frames.isEmpty(); }),object.end());
      }
   }
   int finalizedCount(const QString &object) const
   {
      int count=0; for (const auto &group:groups.value(object)) if (group.finalized && !group.frames.isEmpty()) ++count; return count;
   }
   bool finalize(PlotModel &model,Stage stage,quint64 maximum,const QSet<QString> &spacecraft)
   {
      synchronize(model); QMap<QString,QSet<quint64>> removed;
      if (stage==Stage::Solving) {
         // TrajectoryDealer finalizes all currently open solution groups in
         // this view frame, including other spacecraft and celestial bodies.
         for (auto &object:groups) if (!object.isEmpty()) object.back().finalized=true;
         if (maximum>0) for (auto it=groups.begin();it!=groups.end();++it) {
            auto &object=it.value();
            while (static_cast<quint64>(finalizedCount(it.key()))>maximum) {
               auto found=std::find_if(object.begin(),object.end(),[](const Group &group) { return group.finalized && !group.frames.isEmpty(); });
               if (found==object.end()) break; removed[it.key()].unite(found->frames); object.erase(found);
            }
         }
      } else if (stage==Stage::SolvedPass && solvedRemovals<2) {
         // Preserve the literal dealer behavior: each frame-map entry invokes
         // all listeners until its frame-wide guard reaches two. Thus two
         // populated objects can consume both removals on one callback.
         const auto invocations=std::min<unsigned>(2-solvedRemovals,static_cast<unsigned>(groups.size()));
         for (unsigned invocation=0;invocation<invocations;++invocation) {
            ++solvedRemovals;
            for (auto it=groups.begin();it!=groups.end();++it) {
               auto &object=it.value();
               if (spacecraft.contains(it.key())) {
                  if (!object.isEmpty()) { removed[it.key()].unite(object.back().frames); object.pop_back(); }
               } else {
                  for (const auto &group:object) removed[it.key()].unite(group.frames); object.clear();
               }
            }
         }
      }
      return erase(model,removed);
   }
   bool clearTrials(PlotModel &model)
   {
      QMap<QString,QSet<quint64>> removed;
      for (const auto &curve:model.curves) for (const auto &point:curve.points) if (point.solver) removed[curve.name].insert(point.frame);
      bool changed=erase(model,removed);
      auto cameras=[&](std::deque<PlotCamera> &history) {
         const auto size=history.size();
         history.erase(std::remove_if(history.begin(),history.end(),[](const PlotCamera &sample) { return sample.solver; }),history.end());
         changed=changed || size!=history.size();
      };
      cameras(model.cameras); if (model.resetView) cameras(model.resetView->cameras);
      for (auto &view:model.cameraViews) { cameras(view.cameras); if (view.reset) cameras(view.reset->cameras); }
      for (auto &vector:model.vectors) {
         const auto size=vector.samples.size();
         vector.samples.erase(std::remove_if(vector.samples.begin(),vector.samples.end(),[](const PlotVectorSample &sample) { return sample.source.solver; }),vector.samples.end());
         changed=changed || size!=vector.samples.size();
         if (vector.replaySample && vector.replaySample->source.solver) vector.replaySample.reset();
      }
      for (auto &curve:model.curves) if (curve.replayPose && curve.replayPose->solver) curve.replayPose.reset();
      if (changed) ++model.historyGeneration;
      clear(); return changed;
   }
private:
   static bool erase(PlotModel &model,const QMap<QString,QSet<quint64>> &removed)
   {
      if (removed.isEmpty()) return false; bool changed=false;
      for (auto &curve:model.curves) {
         const auto frames=removed.value(curve.name); if (frames.isEmpty()) continue;
         const bool tail=!curve.points.empty() && curve.points.back().solver && frames.contains(curve.points.back().frame);
         bool gap=false;
         std::deque<PlotPoint> retained;
         for (auto point:curve.points) {
            if (point.solver && frames.contains(point.frame)) { changed=true; gap=true; continue; }
            if (gap) { point.connect=false; gap=false; }
            retained.push_back(std::move(point));
         }
         curve.points.swap(retained);
         if (!curve.points.empty()) curve.points.front().connect=false;
         if (tail) curve.breakNext=true;
         if (curve.replayPose && curve.replayPose->solver && frames.contains(curve.replayPose->frame)) curve.replayPose.reset();
      }
      if (!changed) return false;
      QSet<quint64> retainedTrials,retainedFrames;
      for (const auto &curve:model.curves) for (const auto &point:curve.points) {
         retainedFrames.insert(point.frame); if (point.solver) retainedTrials.insert(point.frame);
      }
      auto camera=[&](std::deque<PlotCamera> &history) {
         history.erase(std::remove_if(history.begin(),history.end(),[&](const PlotCamera &sample) { return sample.solver && !retainedTrials.contains(sample.frame); }),history.end());
      };
      camera(model.cameras); if (model.resetView) camera(model.resetView->cameras);
      for (auto &view:model.cameraViews) { camera(view.cameras); if (view.reset) camera(view.reset->cameras); }
      for (auto &vector:model.vectors) {
         const auto sourceFrames=removed.value(vector.source);
         const auto discard=[&](const PlotVectorSample &sample) { return sample.source.solver && (sourceFrames.contains(sample.source.frame) || !retainedTrials.contains(sample.source.frame)); };
         vector.samples.erase(std::remove_if(vector.samples.begin(),vector.samples.end(),discard),vector.samples.end());
         if (vector.replaySample && discard(*vector.replaySample)) vector.replaySample.reset();
      }
      if (model.replayCursor && (!retainedFrames.contains(model.replayCursor->before) || !retainedFrames.contains(model.replayCursor->after))) model.replayCursor.reset();
      ++model.historyGeneration; // PlotWidget rebuilds its retained-frame timeline.
      return true;
   }
};
