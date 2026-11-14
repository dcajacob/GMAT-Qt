#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QSet>
class GmatBase;
struct ResourceProperty
{
   QString name;
   QString value;
   QString unit;
   QString help;
   QStringList choices;
   QStringList references;
   QStringList hiddenReferences;
   bool color = false;
   bool filename = false;
   bool fileList = false;
   bool fileInput = false;
   bool fileOutput = false;
   bool list = false;
   bool resizableVector = false;
   int rows = 0, columns = 0;
};
QVector<ResourceProperty> resourceProperties(GmatBase &object);
// Display fallback for quantities whose definitions have no physical unit.
// An empty result means the quantity has not been classified here.
QString dimensionlessResourceUnit(GmatBase &object,const QString &name);
// Display units for the native five-term electric-thruster polynomials.
QString electricThrusterCoefficientUnit(const QString &name);
// Read-only display metadata for numerical dates stored as strings.
QString epochInputUnit(const QString &format);
QString resourceEpochFormatField(GmatBase &object,const QString &name);
QStringList pythonModuleNames();
QStringList dataInterfaceFields(GmatBase &object);
void validateResourceProperties(GmatBase &object,bool replacingPolyhedron=false);
void setResourceProperty(GmatBase &object, const QString &name, const QString &value);
QString orbitCovarianceError(const QString &value);
QSet<QString> applyAttitudeProperties(GmatBase &spacecraft, const QMap<QString,QString> &values);
QSet<QString> applyGravityBodyProperties(GmatBase &object,const QMap<QString,QString> &values);
// Applies coupled configuration fields on a detached pending clone only.
QSet<QString> applyChemicalTankProperties(GmatBase &object,const QMap<QString,QString> &values);
QSet<QString> applyPropSetupProperties(GmatBase &object,const QMap<QString,QString> &values);
QSet<QString> applyRegionProperties(GmatBase &object,const QMap<QString,QString> &values);
QString regionResourceScript(GmatBase &object);
QStringList splitResourceReferences(const QString &value);
bool isResourceFileList(GmatBase &object, const QString &name);
bool isResourceList(GmatBase &object, const QString &name);
bool isResizableResourceVector(GmatBase &object,const QString &name);
QString replaceResourceList(GmatBase &object, const QString &block, const QString &name, const QString &value, const QString *mixture = nullptr);
