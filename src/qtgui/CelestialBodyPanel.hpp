#pragma once
#include "EditablePanel.hpp"
#include <QMap>
#include <QVector>
#include <QStringList>
#include <functional>
class CelestialBody;
class QLabel;
struct BodyField {
   QString name,label,section,value;
   QStringList choices;
   int type=0;
   bool readOnly=false,files=false,filename=false;
};
QVector<BodyField> celestialBodyFields(CelestialBody &body);
QString celestialBodyScript(CelestialBody &body,const QString &source,const QMap<QString,QString> &changes,const QString &firstMissionStatement);
QString celestialBodySettingsError(CelestialBody &body,const QMap<QString,QString> &changes);
class CelestialBodyPanel final : public EditablePanel {
public:
   using Apply=std::function<QString(const QMap<QString,QString> &)>;
   CelestialBodyPanel(CelestialBody &body,Apply apply,QWidget *parent=nullptr);
   ~CelestialBodyPanel() override;
   bool hasChanges() const override;
   void discardChanges() override { applied=true; }
   QMap<QString,QString> settings() const;
private:
   QMap<QString,QString> original;
   QMap<QString,std::function<QString()>> values;
   QLabel *status;
   bool applied=false;
};
