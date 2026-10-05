#include "PlotModel.hpp"
#include <algorithm>
#include <cmath>

void PlotModel::append(int index, double x, double y, double z, double epoch,
                      bool drawing, bool solver, double high, double low)
{
   auto &curve = curves[index];
   if (!active) { curve.breakNext = true; return; }
   if (!usableValue(x) || !usableValue(y) || !usableValue(z) || !usableValue(epoch) ||
       (kind==Kind::Orbit && !usableOrbitPosition(x,y,z))) {
      curve.breakNext = true; return;
   }
   bool connect = penDown && drawing && !curve.breakNext && !curve.points.empty();
   // Solver iterations and backward propagation must not join unrelated arcs.
   if (!curve.points.empty() && curve.points.back().solver != solver) connect = false;
   high = usableValue(high) && high >= 0 ? high : 0;
   low = usableValue(low) && low >= 0 ? low : 0;
   curve.points.push_back({x, y, z, epoch, frame, curve.color, connect, solver, high, low});
   curve.points.back().marker=curve.markerType;
   curve.breakNext = !penDown || !drawing;
   while (curve.points.size() > static_cast<size_t>(maxPoints)) curve.points.pop_front();
   if (!curve.points.empty()) curve.points.front().connect = false;
   // A solver's break anchor must survive trimming: ClearFromBreak can be
   // called repeatedly, including after an entire iteration was trimmed.
   if (!curve.points.empty()) {
      const auto first=curve.points.front().frame;
      int lastOlder=-1;
      for (int i=0;i<curve.breaks.size();++i) if (curve.breaks[i]<first) lastOlder=i;
      for (int i=lastOlder-1;i>=0;--i) if (curve.breaks[i]<first) curve.breaks.remove(i);
   }
   lastEpoch = epoch;
}
void PlotModel::clear()
{
   for (auto &curve : curves) { curve.points.clear(); curve.replayPose.reset(); curve.simulationActive=false; curve.simulationPose.reset(); curve.simulationCursor.reset(); curve.simulationArc=0; curve.simulationRegularArc=false; curve.breaks.clear(); curve.breakNext = true; }
   frame = 0; pendingUpdates = 0; endOfRun=false; ++historyGeneration;
   regularArcs.clear(); nextRegularArc=0;
   cameras.clear(); replayCursor.reset(); simulationEpoch.reset(); simulationCameras.clear(); simulationCameraArcs.clear(); simulationCameraReferenceArcs.clear();
   for (auto &vector:vectors) { vector.samples.clear(); vector.replaySample.reset(); vector.simulationActive=false; vector.simulationSample.reset(); vector.simulationArc=0; vector.simulationRegularArc=false; }
   if (resetView) resetView->cameras.clear();
   for (auto &view:cameraViews) { view.cameras.clear(); if (view.reset) view.reset->cameras.clear(); }
}
void PlotModel::breakLines()
{
   for (auto &curve : curves) curve.breakNext = true;
}
void PlotModel::trim()
{
   maxPoints = std::max(1, maxPoints);
   for (auto &vector:vectors) while (vector.samples.size()>static_cast<size_t>(maxPoints)) vector.samples.pop_front();
   while (cameras.size()>static_cast<size_t>(maxPoints)) cameras.pop_front();
   auto trimCamera=[&](std::deque<PlotCamera> &history) { while (history.size()>static_cast<size_t>(maxPoints)) history.pop_front(); };
   if (resetView) trimCamera(resetView->cameras);
   for (auto &view:cameraViews) { trimCamera(view.cameras); if (view.reset) trimCamera(view.reset->cameras); }
   for (auto &curve : curves) {
      while (curve.points.size() > static_cast<size_t>(maxPoints)) curve.points.pop_front();
      if (!curve.points.empty()) curve.points.front().connect = false;
   }
}
QVector<PlotOrbitMarker> PlotModel::orbitMarkers(const PlotCurve &curve,quint64 through) const
{
   QVector<PlotOrbitMarker> result;
   if (!curve.visible || curve.orbitMarkerSize==0 || (!curve.centerMarker && !curve.endpointMarkers)) return result;
   const bool simulation=curve.simulationActive && through==std::numeric_limits<quint64>::max();
   if (simulation) through=curve.simulationCursor ? curve.simulationCursor->before : 0;
   const auto first=firstVisibleFrame(curve,through);
   const PlotPoint *start=nullptr,*last=nullptr;
   auto ends=[&] { if (curve.endpointMarkers && start) { result.append({start,true}); if (last!=start) result.append({last,true}); } };
   for (const auto &point:curve.points) {
      if (point.frame>through || point.frame<first) continue;
      if (!last || !point.connect || point.provider!=last->provider) { ends(); start=&point; }
      last=&point;
   }
   const auto &tip=simulation ? curve.simulationPose : curve.replayPose;
   if (tip && (simulation || tip->frame==through)) {
      last=&*tip; if (!start) start=last;
   }
   ends(); if (curve.centerMarker && last) result.append({last,false});
   return result;
}
quint64 PlotModel::firstVisibleFrame(const PlotCurve &curve,quint64 through) const
{
   if (curve.simulationActive && through==std::numeric_limits<quint64>::max()) through=curve.simulationCursor ? curve.simulationCursor->before : 0;
   if (kind==Kind::XY || redrawPoints<=0 || (endOfRun && through>=frame)) return 0;
   // wx uses endIndex - NumPointsToRedraw: retain the preceding endpoint
   // so the requested number of recent segments can still be connected.
   int count=0;
   quint64 first=std::numeric_limits<quint64>::max();
   for (auto point=curve.points.rbegin();point!=curve.points.rend();++point) {
      if (point->frame>through) continue;
      first=point->frame;
      if (count++==redrawPoints) break;
   }
   return first;
}
QVector<QPair<QPointF, QPointF>> PlotModel::groundSegments(const QPointF &a, const QPointF &b)
{
   if (std::abs(b.x() - a.x()) <= 180.0) return {{a, b}};
   const double adjusted = b.x() + (b.x() > a.x() ? -360.0 : 360.0);
   if (std::abs(adjusted-a.x())<1e-12) return {};
   const double edge = adjusted > a.x() ? 180.0 : -180.0;
   const double fraction = (edge - a.x()) / (adjusted - a.x());
   const double latitude = a.y() + fraction * (b.y() - a.y());
   return {{a, {edge, latitude}}, {{-edge, latitude}, b}};
}

QVector<QPointF> PlotModel::groundFootprint(QPointF center,double radiusDegrees)
{
   QVector<QPointF> result;
   if (!std::isfinite(center.x()) || !std::isfinite(center.y()) || std::abs(center.y())>90 ||
       !std::isfinite(radiusDegrees) || radiusDegrees<=0 || radiusDegrees>=180) return result;
   constexpr double radians=3.14159265358979323846/180;
   const double latitude=center.y()*radians,longitude=center.x()*radians,radius=radiusDegrees*radians;
   for (int i=0;i<=72;++i) {
      const double bearing=i*5*radians;
      const double lat=std::asin(std::clamp(std::sin(latitude)*std::cos(radius)+std::cos(latitude)*std::sin(radius)*std::cos(bearing),-1.0,1.0));
      const double lon=longitude+std::atan2(std::sin(bearing)*std::sin(radius)*std::cos(latitude),std::cos(radius)-std::sin(latitude)*std::sin(lat));
      result.append({std::remainder(lon/radians,360.0),lat/radians});
   }
   return result;
}
