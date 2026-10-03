#pragma once
#include <QString>
#include <QStringList>
struct QtScriptConversion {
   QString script,error;
   QStringList notes;
   int plots=0;
};
QtScriptConversion convertOpenFramesViews(const QString &source);

#include <QMap>
#include <QVector>
#include <array>
#include <optional>
// The legacy Reset view destination is independent of the saved Current pose.
struct QtCameraReset {
   QString reference,target,automaticTrajectory,automaticBody,segmentFrame;
   std::array<double,3> eye{0,-1,0},center{},up{0,0,1};
   bool bodyRelative=false,lookAtRotation=false,shortestAngle=false;
};
struct QtCameraPreset {
   QString name,reference,target;
   QString automaticTrajectory;
   QString automaticBody;
   QString segmentFrame;
   std::array<double,3> eye{0,-30000,0},center{},up{0,0,1};
   bool perspective=true;
   double fieldOfView=45;
   bool bodyRelative=false;
   bool lookAtRotation=false, shortestAngle=false;
   std::optional<QtCameraReset> reset;
};
struct QtVectorSetting {
   QString name,source,destination,label="DefaultVector";
   QString type="Body-Fixed";
   quint32 color=0xff0000;
   bool automaticLength=true;
   double length=1;
   std::array<double,3> start{},direction{1,0,0};
};
struct QtCameraSetting {
   bool perspective=false; double fieldOfView=50;
   std::optional<std::array<double,3>> up;
   QString primaryName;
   QVector<QtCameraPreset> views;
   QVector<QtVectorSetting> vectors;
   bool bodyRelative=false;
   bool lookAtRotation=false, shortestAngle=false;
   std::optional<std::array<double,3>> centerOffset;
   QString automaticTrajectory;
   QString automaticBody;
   QString segmentFrame;
   std::optional<QtCameraReset> reset;
   QMap<QString,bool> objectLabels,objectTrajectories,objectCenters,objectEndpoints,objectAxes,objectGrids,objectXYPlanes,objectVelocities;
   QMap<QString,quint32> objectMarkerSizes;
   QMap<QString,double> objectLineWidths;
   QMap<QString,quint32> objectFontSizes;
   QMap<QString,QString> objectFontPositions;
};
QMap<QString,quint32> qtObjectFontSizes(const QString &json);
QMap<QString,QString> qtObjectFontPositions(const QString &json);
QString qtObjectFontPositionsJson(const QMap<QString,QString> &positions);
QMap<QString,double> qtObjectWidths(const QString &json);
QString qtObjectWidthsJson(const QMap<QString,double> &widths);
QMap<QString,quint32> qtObjectSizes(const QString &json);
QString qtObjectSizesJson(const QMap<QString,quint32> &sizes);
QMap<QString,bool> qtObjectFlags(const QString &json);
QString qtObjectFlagsJson(const QMap<QString,bool> &flags);
QMap<QString,QtCameraSetting> qtCameraSettings(const QString &source);
QString qtCameraDirective(const QString &plot,const QtCameraSetting &setting);
QString retainQtCameraSettings(const QString &original,const QString &candidate,const QString &removedPlot={});
QString removeQtCameraSetting(const QString &source,const QString &plot);
QString setQtCameraSetting(const QString &source,const QString &plot,const QtCameraSetting &setting);
QString arrayExpressions(const QString &source,const QString &name);
QString setArrayExpressions(const QString &source,const QString &name,const QString &cells,int rows,int columns);
