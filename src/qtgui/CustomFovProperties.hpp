#pragma once
#include <QString>
#include <QMap>
#include <QSet>
class GmatBase;
// Coordinate representation is source information: the engine exposes both
// aliases for the same vectors, but their interpolation semantics differ.
QString customFovCoordinates(const GmatBase &object,const QString &script);
QString customFovEditedCoordinates(const QMap<QString,QString> &values,const QString &fallback="ConeClock");
QSet<QString> applyCustomFovProperties(GmatBase &object,const QMap<QString,QString> &values,const QString &fallback="ConeClock");
QString customFovResourceScript(GmatBase &object,const QString &coordinates);
