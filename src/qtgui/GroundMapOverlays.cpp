#include "GroundMapOverlays.hpp"
#include "GroundMapData.hpp"
#include <QFontMetrics>
#include <QPainter>
#include <algorithm>
#include <cmath>

GroundMapOverlayResult drawGroundMapOverlays(QPainter &painter,const GroundMapOverlayOptions &options,
                                           const QTransform &toCanvas,const QRectF &viewport)
{
   GroundMapOverlayResult result;
   if (!options.countries && !options.counties && !options.cities) return result;
   bool invertible=false;
   const auto inverse=toCanvas.inverted(&invertible);
   if (!invertible || viewport.isEmpty()) return result;
   const auto visible=inverse.mapRect(viewport).normalized();
   const double pixelsPerDegree=viewport.width()/visible.width();
   painter.save(); painter.setClipRect(viewport,Qt::IntersectClip);
   painter.setBrush(Qt::NoBrush);
   auto outlines=[&](const QVector<GroundMapBoundary> &boundaries,QColor color,bool fine) {
      int count=0;
      painter.save(); painter.setWorldTransform(toCanvas,true);
      QPen pen(color,1); pen.setCosmetic(true); painter.setPen(pen);
      for (const auto &boundary:boundaries) if (visible.intersects(boundary.bounds)) {
         painter.drawPath(fine ? boundary.fine : boundary.coarse); ++count;
      }
      painter.restore(); return count;
   };
   if (options.countries) result.countryBoundaries=outlines(groundMapCountries(),QColor(250,242,205,210),pixelsPerDegree>=10);
   if (options.counties && pixelsPerDegree>=24)
      result.countyBoundaries=outlines(groundMapCounties(),QColor(240,245,250,155),pixelsPerDegree>=40);
   QFont font=painter.font(); font.setPointSizeF(std::max(8.,font.pointSizeF()-1)); painter.setFont(font);
   const QFontMetrics metrics(font);
   QVector<QRectF> occupied;
   auto label=[&](const QString &text,QPointF position,bool city) {
      const QSizeF size(metrics.horizontalAdvance(text)+2,metrics.height());
      QVector<QPointF> offsets{{8,-size.height()/2},{-8-size.width(),-size.height()/2},
                              {-size.width()/2,-8-size.height()},{-size.width()/2,8}};
      if (!city) offsets.prepend({-size.width()/2,-size.height()/2});
      for (const auto &offset:offsets) {
         const QRectF bounds(position+offset,size);
         if (!viewport.adjusted(3,3,-3,-3).contains(bounds)) continue;
         const auto padded=bounds.adjusted(-4,-3,4,3);
         if (std::any_of(occupied.cbegin(),occupied.cend(),[&](const QRectF &area) { return area.intersects(padded); })) continue;
         occupied.append(padded); result.labels.append({text,bounds,position,city}); return;
      }
   };
   if (options.cities) {
      const int rank=std::clamp(static_cast<int>(2+2*std::log2(std::max(.01,pixelsPerDegree/2))),1,10);
      for (const auto &place:groundMapPlaces()) if (place.rank<=rank && visible.contains(place.coordinates)) {
         const auto position=toCanvas.map(place.coordinates);
         painter.setPen(QPen(QColor(20,28,36),1)); painter.setBrush(QColor(255,229,150));
         painter.drawEllipse(position,2.5,2.5); ++result.cityPoints;
         occupied.append(QRectF(position-QPointF(3,3),QSizeF(6,6)));
         label(place.name,position,true);
      }
   }
   if (options.counties && pixelsPerDegree>=45)
      for (const auto &county:groundMapCounties()) if (visible.contains(county.center))
         label(county.name,toCanvas.map(county.center),false);
   for (const auto &item:result.labels) {
      QPainterPath text;
      text.addText(item.bounds.topLeft()+QPointF(0,metrics.ascent()),font,item.text);
      painter.strokePath(text,QPen(QColor(15,23,30,220),2.5,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
      painter.fillPath(text,item.city ? QColor(255,244,211) : QColor(231,237,245));
   }
   painter.restore();
   return result;
}
