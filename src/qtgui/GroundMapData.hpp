#pragma once
#include <QPainterPath>
#include <QPointF>
#include <QString>
#include <QVector>

struct GroundMapPlace
{
   QString name,region,country,displayName,searchName;
   QPointF coordinates;
   int rank=0;
   double population=0;
};

struct GroundMapBoundary
{
   QString name;
   QPointF center;
   QPainterPath coarse,fine;
   QRectF bounds;
};

const QVector<GroundMapPlace> &groundMapPlaces();
const QVector<GroundMapBoundary> &groundMapCountries();
const QVector<GroundMapBoundary> &groundMapCounties();
