#pragma once
#include "PlotModel.hpp"
#include "TimeSystemConverter.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QString>
#include <optional>

// Presentation follows the selected clock/cursor, or the last publication
// frame. Epochs need not increase: backward propagation and reset arcs are valid.
inline std::optional<double> plotDisplayedEpoch(const PlotModel &model,quint64 frame)
{
   if (model.simulationEpoch) return model.simulationEpoch;
   if (model.replayCursor) return model.replayCursor->epoch;
   std::optional<double> epoch;
   quint64 selected=0;
   for (const auto &curve:model.curves) for (const auto &point:curve.points)
      if (point.frame<=frame && (!epoch || point.frame>=selected)) {
         epoch=point.epoch; selected=point.frame;
      }
   return epoch;
}

inline QString plotUtcCaption(const PlotModel &model,quint64 frame)
{
   if (model.kind!=PlotModel::Kind::Orbit && model.kind!=PlotModel::Kind::GroundTrack) return {};
   const auto epoch=plotDisplayedEpoch(model,frame);
   if (epoch && PlotModel::usableValue(*epoch) && Moderator::Instance()->IsInitialized()) {
      try {
         Real convertedEpoch;
         std::string converted;
         TimeSystemConverter::Instance()->Convert("A1ModJulian",*epoch,"","UTCGregorian",convertedEpoch,converted);
         if (!converted.empty()) return "UTC: "+QString::fromStdString(converted);
      } catch (BaseException &) {
         // Painting remains usable before initialization or with unavailable
         // time data; never leave a stale successful caption on screen.
      }
   }
   return "UTC epoch unavailable";
}
