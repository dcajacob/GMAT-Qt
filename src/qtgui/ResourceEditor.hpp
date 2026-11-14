#pragma once
#include "EditablePanel.hpp"
#include <QMap>
#include <QString>
#include <QSet>
#include <functional>
#include <QMdiSubWindow>
class GmatBase;
class QTableWidget;
class QLabel;
class ResourceEditor final : public EditablePanel
{
public:
   using Apply = std::function<QString(const QMap<QString, QString> &)>;
   ResourceEditor(GmatBase &object, Apply apply, QWidget *parent = nullptr, const QString &script = {}, bool applyUnchanged = false, bool creation = false);
   void requestApply();
   void resizeArray(int rows,int columns);
   std::function<void(int,int)> onArrayDimensions;
   std::function<void(const QString &)> onStatus;
   bool hasChanges() const override;
   bool hasUnappliedInput() const override { return !applied && (applyDirty || hasChanges()); }
   void discardChanges() override { applied = true; }
private:
   void reportStatus(const QString &message);
   void refreshCoordinateAxisFields(const QString &type);
   void trackApplyState(bool allowUnchanged);
   void updateApplyState();
   std::function<QString()> scalarValue;
   QString originalScalarValue;
   QTableWidget *table=nullptr;
   std::function<QMap<QString,QString>()> formValues;
   std::function<QMap<QString,QString>()> formEdits;
   QMap<QString,QString> formOriginal;
   QSet<QString> formFields;
   class QPushButton *applyButton=nullptr;
   QLabel *status=nullptr;
   QMap<QString, QString> original;
   QMap<QString, QString> coordinateAxisEdits;
   QMap<QString, QString> coordinateAxisInitial;
   QMap<QString, QString> attitudeEdits;
   QMap<QString, QString> atmosphereEdits;
   QMap<QString, QString> gravityEdits;
   QMap<QString, QString> externalEdits,originalExternal;
   QMap<QString, QString> stationEdits;
   QMap<QString, QString> eventEdits;
   QMap<QString, QString> objectDrawingEdits;
   QString originalPolyhedron,pendingPolyhedron;
   QString pendingDynamicData;
   QString pendingTrackingConfigs;
   QSet<QString> attitudeNames;
   std::function<QMap<QString,QString>()> pendingOrbit;
   std::function<QMap<QString,QString>()> pendingPropagation,propagationChanges;
   QMap<QString,QString> originalPropagation;
   QSet<QString> propagationFields;
   QString originalExpressions="[]",expressions="[]";
   bool applied = false;
   bool applyDirty = false;
   bool allowUnchangedApply = false;
   bool pairedTankEdits = false;
};
