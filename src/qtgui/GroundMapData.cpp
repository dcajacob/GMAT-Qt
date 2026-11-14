#include "GroundMapData.hpp"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <cmath>

static void initializeGroundMapResources()
{
   Q_INIT_RESOURCE(GroundMapData);
}

namespace {
QJsonArray read(const QString &name)
{
   static const bool initialized=[] { initializeGroundMapResources(); return true; }();
   Q_UNUSED(initialized);
   QFile file(":/gmat/ground-map/"+name+".json");
   if (!file.open(QIODevice::ReadOnly)) return {};
   return QJsonDocument::fromJson(file.readAll()).array();
}
QPointF point(const QJsonValue &value)
{
   const auto pair=value.toArray();
   return {pair.at(0).toDouble(),pair.at(1).toDouble()};
}
QPainterPath path(const QJsonValue &value)
{
   QPainterPath result;
   for (const auto &contour:value.toArray()) {
      const auto points=contour.toArray();
      if (points.size()<2) continue;
      result.moveTo(point(points.first()));
      for (int i=1;i<points.size();++i) result.lineTo(point(points[i]));
   }
   return result;
}
QVector<GroundMapBoundary> boundaries(const QString &name)
{
   QVector<GroundMapBoundary> result;
   for (const auto &value:read(name)) {
      const auto item=value.toObject();
      GroundMapBoundary boundary{item["name"].toString(),point(item["point"]),path(item["coarse"]),path(item["fine"]),{}};
      boundary.bounds=boundary.fine.boundingRect();
      result.append(std::move(boundary));
   }
   return result;
}
}

const QVector<GroundMapPlace> &groundMapPlaces()
{
   static const auto places=[] {
      QVector<GroundMapPlace> result;
      for (const auto &value:read("places")) {
         const auto item=value.toObject();
         GroundMapPlace place;
         place.name=item["name"].toString(); place.region=item["region"].toString(); place.country=item["country"].toString();
         place.coordinates=point(item["point"]); place.rank=item["rank"].toInt(); place.population=item["population"].toDouble();
         QStringList parts{place.name};
         if (!place.region.isEmpty() && place.region!=place.name) parts.append(place.region);
         if (!place.country.isEmpty()) parts.append(place.country);
         place.displayName=parts.join(", ");
         place.searchName=place.displayName+' '+item["ascii"].toString();
         if (std::isfinite(place.coordinates.x()) && std::isfinite(place.coordinates.y()) &&
             std::abs(place.coordinates.x())<=180 && std::abs(place.coordinates.y())<=90)
            result.append(std::move(place));
      }
      return result;
   }();
   return places;
}

const QVector<GroundMapBoundary> &groundMapCountries()
{
   static const auto result=boundaries("countries");
   return result;
}

const QVector<GroundMapBoundary> &groundMapCounties()
{
   static const auto result=boundaries("counties");
   return result;
}
