#pragma once
#include <QMap>
#include <QString>
#include "ResourcePreview.hpp"
class GmatBase;
// An unregistered template: opening/canceling the creator cannot mutate a mission.
QtResourcePreview resourceDraft(const QString &type,const QString &name);
QMap<QString,QString> defaultOrbitViewCameraSettings();
// New numerical GUI propagators own a local, editable force model.
QString defaultPropagatorForceModelName(const QString &name);
bool propagatorUsesForceModel(const QString &type);
