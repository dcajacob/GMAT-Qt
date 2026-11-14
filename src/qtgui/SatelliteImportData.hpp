#pragma once
#include <QByteArray>
#include <QString>
#include <QVector>
#include <QMap>
#include <array>
#include <optional>

struct SatelliteImportData
{
   QString name,catalogId,epoch,propagatorType="SPICESGP4",coordinateSystem="EarthMJ2000Eq",centralBody="Earth";
   QByteArray elements;
   QString extension,sourceFile,source;
   QMap<QString,QString> meanElements;
   std::optional<std::array<double,6>> state;
   int naifId=0;
};

QVector<SatelliteImportData> parseSatelliteElements(const QByteArray &data);
QString satelliteEpochGregorian(const QString &epoch);
QString satelliteImportScript(const SatelliteImportData &data,const QString &name,const QString &path);
