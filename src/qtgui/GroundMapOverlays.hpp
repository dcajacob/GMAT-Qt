#pragma once
#include <QRectF>
#include <QString>
#include <QTransform>
#include <QVector>
class QPainter;

struct GroundMapOverlayOptions
{
   bool countries=false,counties=false,cities=false;
};

struct GroundMapOverlayLabel
{
   QString text;
   QRectF bounds;
   QPointF position;
   bool city=false;
};

struct GroundMapOverlayResult
{
   QVector<GroundMapOverlayLabel> labels;
   int countryBoundaries=0,countyBoundaries=0,cityPoints=0;
};

GroundMapOverlayResult drawGroundMapOverlays(QPainter &painter,const GroundMapOverlayOptions &options,
                                           const QTransform &toCanvas,const QRectF &viewport);
