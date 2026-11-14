#pragma once
#include <QFontMetrics>
#include <QRectF>
#include <QString>
#include <QVector>
#include <algorithm>
#include <cmath>

struct GroundTrackAxisLabel
{
   QString text;
   QRectF bounds;
};

struct GroundTrackAxisTick
{
   double value;
   QString text;
};

inline QVector<GroundTrackAxisTick> groundTrackAxisTicks(double minimum,double maximum,int intervals)
{
   if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum>=maximum || intervals<1)
      return {};
   const double raw=(maximum-minimum)/intervals;
   const double magnitude=std::pow(10.,std::floor(std::log10(raw)));
   if (!std::isfinite(magnitude) || magnitude<=0) return {};
   double step=10*magnitude;
   for (double multiple:{1.,2.,3.,5.,6.,10.}) if (raw<=multiple*magnitude) {
      step=multiple*magnitude; break;
   }
   const int precision=std::max(0,-static_cast<int>(std::floor(std::log10(step))));
   QVector<GroundTrackAxisTick> ticks;
   const double first=std::ceil(minimum/step)*step;
   for (int index=0;index<=intervals+1;++index) {
      double value=first+index*step;
      if (value>maximum) break;
      if (value<minimum) continue;
      if (std::abs(value)<step*1e-10) value=0;
      ticks.append({value,precision<=12 ? QString::number(value,'f',precision) : QString::number(value,'g',17)});
   }
   if (ticks.isEmpty()) {
      const double value=minimum+(maximum-minimum)/2;
      ticks.append({value,QString::number(value,'g',17)});
   }
   return ticks;
}

inline QVector<GroundTrackAxisLabel> groundTrackAxisLabels(
      const QRectF &area, double minimum, double maximum,
      const QFontMetrics &metrics, Qt::Orientation orientation, double gutterLeft)
{
   if (area.isEmpty() || !std::isfinite(minimum) || !std::isfinite(maximum) || minimum >= maximum)
      return {};
   const bool horizontal = orientation == Qt::Horizontal;
   const double height = metrics.height();
   if (!horizontal && area.height() < height) return {};
   const double availableWidth = horizontal ? area.width() : area.left() - gutterLeft - 8;
   if (availableWidth <= 0) return {};
   auto labelAt = [&](const GroundTrackAxisTick &tick) {
      const double fraction=(tick.value-minimum)/(maximum-minimum);
      const double width=metrics.horizontalAdvance(tick.text);
      QRectF bounds;
      if (horizontal) {
         const double center = area.left() + fraction * area.width();
         bounds = QRectF(std::clamp(center - width / 2, area.left(), std::max(area.left(),area.right()-width)),
                         area.bottom() + 4, width, height);
      } else {
         const double center = area.bottom() - fraction * area.height();
         const double top = area.height() >= height
            ? std::clamp(center - height / 2, area.top(), area.bottom() - height)
            : area.center().y() - height / 2;
         bounds = QRectF(area.left() - 8 - width, top, width, height);
      }
      return GroundTrackAxisLabel{tick.text, bounds};
   };
   for (int intervals : {6, 4, 2, 1}) {
      QVector<GroundTrackAxisLabel> labels;
      bool fits = true;
      for (const auto &tick:groundTrackAxisTicks(minimum,maximum,intervals)) {
         auto label=labelAt(tick);
         if (label.bounds.width()>availableWidth ||
             (!labels.isEmpty() && labels.back().bounds.adjusted(-3,-3,3,3).intersects(label.bounds))) {
            fits = false;
            break;
         }
         labels.append(label);
      }
      if (fits && !labels.isEmpty()) return labels;
   }
   return {};
}
