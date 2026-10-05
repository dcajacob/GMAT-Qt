#pragma once
#include "PlotModel.hpp"
#include "OrbitObjectAxes.hpp"
#include "PlotInterpolation.hpp"
#include <algorithm>

struct OrbitSegmentLabel
{
   QString name;
   PlotPoint pose;
};

// Each regular OF segment follows its own trajectory with LIMIT clamping.
// Compact raw endpoints complete brackets lost through collection or trimming;
// interpolation uses the available recorded geometry only, without changing it.
inline QVector<OrbitSegmentLabel> orbitSegmentLabels(const PlotModel &model,const PlotCurve &curve,quint64 frame)
{
   if (model.kind!=PlotModel::Kind::Orbit || !model.labels || !curve.visible || !curve.segmentLabels) return {};
   const auto found=model.regularArcs.constFind(curve.name);
   if (found==model.regularArcs.cend()) return {};
   const auto *recorded=orbitObjectPose(curve,frame);
   if (!model.replayCursor && !recorded) return {};
   const double epoch=model.replayCursor ? model.replayCursor->epoch : recorded->epoch;
   if (!PlotModel::usableValue(epoch)) return {};
   auto usable=[](const PlotPoint &point) {
      return !point.solver && PlotModel::usableValue(point.epoch) && PlotModel::usableOrbitPosition(point.x,point.y,point.z);
   };
   struct Selection {
      const PlotRegularArc *arc;
      const PlotPoint *before=nullptr,*after=nullptr;
      double epoch;
   };
   QVector<Selection> selections;
   QMap<quint64,int> members;
   auto offer=[](Selection &selection,const PlotPoint &point) {
      if (point.epoch<=selection.epoch && (!selection.before || point.epoch>selection.before->epoch)) selection.before=&point;
      if (point.epoch>=selection.epoch && (!selection.after || point.epoch<selection.after->epoch)) selection.after=&point;
   };
   for (const auto &arc:*found) {
      if (arc.name.isEmpty() || !usable(arc.first) || !usable(arc.last)) continue;
      const int index=selections.size();
      selections.append({&arc,nullptr,nullptr,std::clamp(epoch,std::min(arc.first.epoch,arc.last.epoch),std::max(arc.first.epoch,arc.last.epoch))});
      auto &selection=selections.back(); offer(selection,arc.first); offer(selection,arc.last);
      if (arc.id) members.insert(arc.id,index);
      for (const auto member:arc.members) if (member) members.insert(member,index);
   }
   for (const auto &point:curve.points) {
      if (!point.regularArc || !usable(point)) continue;
      const auto selected=members.constFind(point.regularArc); if (selected==members.cend()) continue;
      auto &selection=selections[*selected];
      const auto &arc=*selection.arc;
      if (point.epoch<std::min(arc.first.epoch,arc.last.epoch) || point.epoch>std::max(arc.first.epoch,arc.last.epoch)) continue;
      offer(selection,point);
   }
   QVector<OrbitSegmentLabel> result; result.reserve(selections.size());
   for (const auto &selection:selections) {
      if (!selection.before || !selection.after) continue;
      PlotPoint pose=*selection.before;
      if (selection.after->epoch>selection.before->epoch) {
         const double fraction=(selection.epoch-selection.before->epoch)/(selection.after->epoch-selection.before->epoch);
         pose=plotInterpolatedPoint(*selection.before,*selection.after,fraction,PlotModel::Kind::Orbit);
      }
      pose.epoch=selection.epoch;
      result.append({selection.arc->name,pose});
   }
   return result;
}
