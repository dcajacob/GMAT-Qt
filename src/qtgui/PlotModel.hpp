#pragma once
#include <QColor>
#include <QImage>
#include <QMap>
#include <QSet>
#include <QPointF>
#include <QVector>
#include <QString>
#include <deque>
#include <limits>
#include <array>
#include <optional>
#include <cmath>
#include "StarCatalog.hpp"

struct PlotViewState
{
   double zoom=1,yaw=0,pitch=0;
   QPointF pan;
   bool fit=false,perspective=false;
   double fieldOfView=50;
   QString camera;
   bool reset=false;
};
struct PlotPoint
{
   double x = 0, y = 0, z = 0, epoch = 0;
   quint64 frame = 0;
   QColor color;
   bool connect = false;
   bool solver = false;
   double high = 0, low = 0;
   // Row-major body-fixed to plot-frame rotation, captured while engine objects live.
   std::array<double,9> bodyToView = {1,0,0,0,1,0,0,0,1};
   std::array<double,3> sunPosition = {0,0,0};
   bool hasSun = false;
   std::array<double,9> inertialToView = {1,0,0,0,1,0,0,0,1};
   int marker=-1;
   bool highlighted=false;
   QString provider;
   // Actual publisher velocity (km/s) in the same view frame as x/y/z.
   std::array<double,3> viewVelocity{};
   bool hasVelocity=false;
};
// Regular trajectory identity is independent of collected/trimmed points.
struct PlotRegularArc
{
   quint64 id=0;
   QString name;
   QSet<QString> providers;
   PlotPoint first,last;
   bool finalized=false;
};
struct PlotCurve
{
   QString name;
   QColor color = Qt::red;
   std::deque<PlotPoint> points;
   QVector<quint64> breaks;
   bool breakNext = true, visible = true, lines = true, markers = false;
   bool showObject = true;
   std::optional<bool> importedLabel;
   bool centerMarker=false,endpointMarkers=false,objectAxes=false,objectGrid=false,objectXYPlane=false,objectVelocity=false;
   quint32 orbitMarkerSize=10;
   std::optional<double> importedLineWidth;
   std::optional<quint32> importedFontSize;
   QString importedFontPosition;
   double orbitLineWidth() const { return importedLineWidth.value_or(width); }
   bool drawsLabel() const { return visible && importedLabel.value_or(showObject); }
   bool drawsContent() const { return visible && (lines || markers || showObject || centerMarker || endpointMarkers || objectAxes || objectGrid || objectXYPlane || objectVelocity || importedLabel.value_or(false)); }
   bool wireframeObject = false;
   double radius = 0;
   QString texturePath;
   QString modelPath;
   std::array<double,3> modelOffset = {0,0,0}, modelRotation = {0,0,0};
   double modelScale = 1;
   int width = 1, markerSize = 4;
   int markerType=0,lineStyle=100;
   bool errorBars=true;
   Qt::PenStyle style = Qt::SolidLine;
};
struct PlotOrbitMarker { const PlotPoint *point; bool endpoint; };

struct PlotStation
{
   QString name;
   double longitude = 0, latitude = 0;
   QColor color;
};
struct PlotVectorSample
{
   PlotPoint source;
   std::array<double,3> destination{};
   double sourceRadius=1;
   bool valid=true;
};
struct PlotVector
{
   QString name,source,destination,label;
   bool bodyFixed=false,automaticLength=true;
   double length=1;
   QColor color=Qt::red;
   std::array<double,3> start{},direction{1,0,0};
   std::deque<PlotVectorSample> samples;
};
struct PlotCamera
{
   quint64 frame=0;
   std::array<double,3> eye{},target{},up{0,0,1};
   bool solver=false;
};
struct PlotCameraReset
{
   std::deque<PlotCamera> cameras;
   QString automaticTrajectory;
   double automaticRadius=0;
   QString automaticBody,segmentFrame;
};
struct PlotCameraView
{
   QString name;
   bool perspective=true;
   double fieldOfView=45;
   std::deque<PlotCamera> cameras;
   QString automaticTrajectory;
   double automaticRadius=0;
   QString automaticBody;
   QString segmentFrame;
   std::optional<PlotCameraReset> reset;
};
struct PlotAxisOptions
{
   std::optional<double> minimum,maximum;
   int ticks=5,precision=5;
};
struct PlotModel
{
   enum class Kind { Orbit, GroundTrack, XY, Table };
   // SPICE can publish +/-REAL_MAX as a finite unavailable-state marker.
   static bool usableValue(double value) {
      return std::isfinite(value) && std::abs(value)<std::numeric_limits<double>::max();
   }
   static bool usableOrbitPosition(double x,double y,double z) {
      // OSG trajectory vertices are floats even though GMAT states are doubles.
      const auto limit=static_cast<double>(std::numeric_limits<float>::max());
      return usableValue(x) && usableValue(y) && usableValue(z) &&
            std::abs(x)<=limit && std::abs(y)<=limit && std::abs(z)<=limit;
   }
   explicit PlotModel(Kind value) : kind(value) {}
   Kind kind;
   QString title, xLabel, yLabel, coordinates;
   PlotAxisOptions xAxis,yAxis;
   QMap<int, PlotCurve> curves;
   QVector<PlotStation> stations;
   QImage map;
   bool grid = true, legend = true, labels = true, penDown = true, active = true;
   bool axes = true;
   bool xyPlane=false,eclipticPlane=false,wireframe=false,sunLine=false;
   bool footprints=false;
   bool sunlight = true;
   bool starsEnabled = false, starCatalogLoaded = false;
   int starCount = 7000;
   StarCatalog starCatalog;
   bool constellationsEnabled=false,constellationCatalogLoaded=false;
   ConstellationCatalog constellationCatalog;
   bool scriptedCamera=false, fitCamera=false, resetCamera=false;
   std::optional<PlotCameraReset> resetView;
   std::optional<PlotViewState> userView;
   bool perspective=false;
   double fieldOfView=50;
   std::deque<PlotCamera> cameras;
   QVector<PlotVector> vectors;
   QVector<PlotCameraView> cameraViews; // Index zero uses the standard scripted history above.
   int selectedCamera=0;
   QString automaticTrajectory;
   double automaticRadius=0;
   QString automaticBody;
   QString segmentFrame;
   int maxPoints = 20000, updateFrequency = 1, pendingUpdates = 0;
   int redrawPoints = 0;
   bool endOfRun = false;
   int defaultLineWidth = 1;
   int longitudeLines = 12, latitudeLines = 6;
   quint64 frame = 0, historyGeneration = 0;
   quint64 nextRegularArc=0;
   // Compact endpoints survive point trimming; retained until model/data reset.
   QMap<QString,QVector<PlotRegularArc>> regularArcs;
   double lastEpoch = 0;
   void append(int curve, double x, double y, double z = 0, double epoch = 0,
               bool drawing = true, bool solver = false, double high = 0, double low = 0);
   void clear();
   void breakLines();
   void trim();
   QVector<PlotOrbitMarker> orbitMarkers(const PlotCurve &curve,quint64 through) const;
   quint64 firstVisibleFrame(const PlotCurve &curve,quint64 through) const;
   static QVector<QPair<QPointF, QPointF>> groundSegments(const QPointF &a, const QPointF &b);
   static QVector<QPointF> groundFootprint(QPointF center,double radiusDegrees=5);
};
