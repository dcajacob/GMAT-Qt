#ifndef GMATQT_ORBITCOORDINATELABELS_HPP
#define GMATQT_ORBITCOORDINATELABELS_HPP

#include "PlotModel.hpp"
#include <cmath>

struct OrbitCoordinateLabel {
   QString text;
   PlotPoint pose;
};

// Coordinate labels belong to the reference frame, independently of object
// labels. The supplied extent is the renderer's existing global-axis length.
inline QVector<OrbitCoordinateLabel> orbitCoordinateLabels(const PlotModel &model,double axisExtent)
{
   QVector<OrbitCoordinateLabel> result;
   if (model.kind!=PlotModel::Kind::Orbit) return result;
   if (model.axes && model.axesLabels && std::isfinite(axisExtent) && axisExtent>0) {
      for (int axis=0;axis<3;++axis) {
         PlotPoint pose;
         if (axis==0) pose.x=axisExtent;
         if (axis==1) pose.y=axisExtent;
         if (axis==2) pose.z=axisExtent;
         result.append({QString(QChar('X'+axis)),pose});
      }
   }
   if (model.frameLabel && !model.coordinates.isEmpty()) result.append({model.coordinates,PlotPoint{}});
   return result;
}

#endif
