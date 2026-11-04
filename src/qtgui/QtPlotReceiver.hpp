#pragma once
#include "PlotReceiver.hpp"
#include "PlotModel.hpp"
#include "PlotSimulationClock.hpp"
#include <QElapsedTimer>
#include "OrbitTrialHistory.hpp"
#include "ScriptCompatibility.hpp"
#include <QMap>
#include <QPointer>
#include <QStringList>
#include <QSet>
#include <QRect>
#include <functional>
#include <memory>
#include <optional>
class QMdiArea;
class QMdiSubWindow;
class QTableWidget;
class PlotWidget;
class QTimer;
class QtPlotReceiver final : public PlotReceiver
{
public:
   explicit QtPlotReceiver(QMdiArea *workspace);
   ~QtPlotReceiver() override;
   void clear(bool resetViews=false);
   void missionFinished();
   bool hasReplayDisplays() const;
   void setReplayPosition(int value);
   void setReplayMode(PlotReplayMode mode);
   PlotReplayMode replayMode() const { return sharedReplayMode; }
   void releaseSharedReplay();
   std::optional<PlotSimulationClocks::State> simulationClock(const QString &name) const;
   void advanceSimulationClocks(double seconds);
   std::optional<int> replayPosition() const { return sharedReplayPosition; }
   static void validateCameraReferences(const QMap<QString,QtCameraSetting> &settings);
   QStringList names() const;
   bool show(const QString &name);
   QMdiArea *workspaceArea() const { return workspace; }
   std::shared_ptr<const PlotModel> model(const QString &name) const;
   std::function<void()> changed;
   std::function<void()> replayChanged;
   std::function<QString(const QString &,bool,double)> saveProjection;
   std::function<QStringList()> protectedPaths;
   QMap<QString,QtCameraSetting> cameraSettings;
virtual bool CreateGlPlotWindow(const std::string &plotName,
                        const std::string &oldName,
                        Real positionX, Real positionY,
                        Real width, Real height, bool isMaximized,
                        Integer numPtsToRedraw) override;
virtual void SetGlSolarSystem(const std::string &plotName,
                        SolarSystem *ss) override;
virtual void SetGlObject(const std::string &plotName,
                        const StringArray &objNames,
                        const std::vector<SpacePoint*> &objArray) override;
virtual void SetGlCoordSystem(const std::string &plotName,
                        CoordinateSystem *internalCs,
                        CoordinateSystem *viewCs,
                        CoordinateSystem *viewUpCs) override;
virtual void SetGl2dDrawingOption(const std::string &plotName,
                        const std::string &centralBodyName,
                        const std::string &textureMap,
                        Integer footPrintOption) override;
virtual void SetGl3dDrawingOption(const std::string &plotName,
                        bool showLabels, bool drawEcPlane,
                        bool drawXyPlane, bool drawWireFrame,
                        bool drawAxes, bool drawGrid, bool drawSunLine,
                        bool overlapPlot, bool usevpInfo,
                        bool drawStars, bool drawConstellations,
                        Integer starCount) override;
virtual void SetGl3dViewOption(const std::string &plotName,
                        SpacePoint *vpRefObj, SpacePoint *vpVecObj,
                        SpacePoint *vdObj, Real vsFactor,
                        const Rvector3 &vpRefVec, const Rvector3 &vpVec,
                        const Rvector3 &vdVec, const std::string &upAxis,
                        bool usevpRefVec, bool usevpVec, bool usevdVec) override;
virtual void SetGlDrawOrbitFlag(const std::string &plotName,
                        const std::vector<bool> &drawArray) override;
virtual void SetGlShowObjectFlag(const std::string &plotName,
                        const std::vector<bool> &showArray) override;
virtual void SetGlUpdateFrequency(const std::string &plotName, 
                        Integer updFreq) override;
virtual bool IsThere(const std::string &plotName) override;
virtual bool InitializeGlPlot(const std::string &plotName) override;
virtual bool RefreshGlPlot(const std::string &plotName) override;
virtual bool DeleteGlPlot(const std::string &plotName) override;
virtual bool SetGlEndOfRun(const std::string &plotName) override;
virtual bool UpdateGlPlot(const std::string &plotName,
                        const std::string &oldName,
                        const StringArray &scNames, const Real &time,
                        const RealArray &posX, const RealArray &posY,
                        const RealArray &posZ, const RealArray &velX,
                        const RealArray &velY, const RealArray &velZ,
                        const ColorMap &orbitColorMap, const ColorMap &targetColorMap, 
                        bool solving, Integer solverOption, bool updateCanvas,
                        bool drawing, bool inFunction) override;
virtual bool TakeGlAction(const std::string &plotName,
                        const std::string &action) override;
virtual void SetMaxGlDataPoints(const std::string &plotName, Integer maxDataPoints) override;
virtual bool CreateXyPlotWindow(const std::string &plotName,
                        const std::string &oldName,
                        Real positionX, Real positionY,
                        Real width, Real height, bool isMaximized,
                        const std::string &plotTitle,
                        const std::string &xAxisTitle,
                        const std::string &yAxisTitle,
                        bool drawGrid = false,
                        bool canSaveLocation = true) override;
virtual bool DeleteXyPlot(const std::string &plotName) override;
virtual bool AddXyPlotCurve(const std::string &plotName, int curveIndex,
                        const std::string &curveTitle,
                        UnsignedInt penColor) override;
virtual bool DeleteAllXyPlotCurves(const std::string &plotName,
                        const std::string &oldName) override;
virtual bool DeleteXyPlotCurve(const std::string &plotName,
                        int curveIndex) override;
virtual void ClearXyPlotData(const std::string &plotName) override;
virtual void XyPlotPenUp(const std::string &plotName) override;
virtual void XyPlotPenDown(const std::string &plotName) override;
virtual void XyPlotDarken(const std::string &plotName, Integer factor,
                        Integer index = -1, Integer forCurve = -1) override;
virtual void XyPlotLighten(const std::string &plotName, Integer factor,
                        Integer index = -1, Integer forCurve = -1) override;
virtual void XyPlotMarkPoint(const std::string &plotName, Integer index = -1,
                        Integer forCurve = -1) override;
virtual void XyPlotMarkBreak(const std::string &plotName, Integer index = -1,
                        Integer curveNumber = -1) override;
virtual void XyPlotClearFromBreak(const std::string &plotName,
                        Integer breakNumber, Integer index = -1, Integer curveNumber = -1) override;
virtual void XyPlotChangeColor(const std::string &plotName,
                        Integer index = -1, UnsignedInt newColor = 0xffffff,
                        Integer forCurve = -1) override;
virtual void XyPlotChangeMarker(const std::string &plotName,
                        Integer index = -1, Integer newMarker = -1, int forCurve = -1) override;
virtual void XyPlotChangeWidth(const std::string &plotName,
                        Integer index = -1, Integer newWidth = 1, int forCurve = -1) override;
virtual void XyPlotChangeStyle(const std::string &plotName,
                        Integer index = -1, Integer newStyle = 100, int forCurve = -1) override;
virtual void XyPlotRescale(const std::string &plotName) override;
virtual void XyPlotCurveSettings(const std::string &plotName,
                        bool useLines = true,
                        Integer lineWidth = 1,
                        Integer lineStyle = 100,
                        bool useMarkers = false,
                        Integer markerSize = 3,
                        Integer marker = 1,
                        bool useHiLow = false,
                        Integer forCurve = -1) override;
virtual void SetXyPlotTitle(const std::string &plotName,
                        const std::string &plotTitle) override;
virtual void ShowXyPlotLegend(const std::string &plotName) override;
virtual bool RefreshXyPlot(const std::string &plotName) override;
virtual bool UpdateXyPlot(const std::string &plotName,
                        const std::string &oldName,
                        const Real &xval, const Rvector &yvals,
                        const std::string &plotTitle,
                        const std::string &xAxisTitle,
                        const std::string &yAxisTitle,
                        bool updateCanvas, bool drawGrid) override;
virtual bool UpdateXyPlotData(const std::string &plotName, const Real &xval,
                        const Rvector &yvals, const Rvector *yhis = NULL,
                        const Rvector *ylows = NULL) override;
virtual bool UpdateXyPlotCurve(const std::string &plotName,
                        const Integer whichCurve, const Real xval,
                        const Real yval, const Real yhi = 0.0,
                        const Real ylow = 0.0) override;
virtual bool DeactivateXyPlot(const std::string &plotName) override;
virtual bool ActivateXyPlot(const std::string &plotName) override;
virtual bool TakeXYAction(const std::string &plotName,
                        const std::string &action) override;
virtual bool CreateGroundTrackWindow(const std::string &plotName,
                        const std::string &oldName,
                        const std::string &plotTitle, Real positionX,
                        Real positionY, Real width, Real height,
                        bool isMaximized) override;
virtual void SetGroundTrackOption(const std::string &plotName,
                        const std::string &optionSetting,
                        const std::string &optionValue) override;
virtual bool UpdateGroundTrackData(const std::string &plotName,
                        const double epoch, const double *longlat,
                        const int satcount) override;
virtual bool TakeGroundTrackAction(const std::string &plotName,
                        const std::string &action) override;
virtual bool CreateDynamicDataDisplay(const std::string &plotName,
                        const std::string &oldName,
                        const std::string &plotTitle, Real positionX,
                        Real positionY, Real width, Real height) override;
virtual bool SetDynamicDataTableSize(const std::string &plotName,
                        Integer maxRowCount, Integer maxColCount) override;
virtual bool UpdateDynamicDataDisplay(const std::string &plotName,
                        std::vector<std::vector<DDD>> newData) override;
virtual bool DeleteDynamicData(const std::string &plotName,
                        const std::string &oldName) override;
virtual bool SetDynamicDataTextColor(const std::string &plotName,
                        std::vector<std::vector<DDD>>) override;
private:
   struct Cell { QString text; QColor foreground = Qt::black, background = Qt::white; };
   struct OrbitDataAvailability { double epoch=0; bool committedSteps=false; QMap<QString,bool> objects; QSet<QString> arcStarts, skipped; };
   struct Entry {
      std::shared_ptr<PlotModel> data;
      QPointer<QMdiSubWindow> window;
      QPointer<PlotWidget> widget;
      QPointer<QTableWidget> table;
      StringArray objects;
      std::vector<SpacePoint *> points;
      CoordinateSystem *internal = nullptr, *view = nullptr, *viewUp = nullptr;
      SpacePoint *cameraReference=nullptr,*cameraPosition=nullptr,*cameraDirection=nullptr;
      std::array<double,3> referenceVector{},positionVector{},directionVector{},upVector{0,0,1};
      bool referenceIsVector=true,positionIsVector=true,directionIsVector=true;
      double cameraScale=1;
      SolarSystem *solarSystem = nullptr;
      QVector<QVector<Cell>> cells;
      // OwnedPlot Deactivate batches data while postponing display refresh.
      bool xyRedrawSuspended = false;
      bool ignoreTimeSequence = false;
      bool solverData = false;
      bool clearSolverOnNextOrbitUpdate = false;
      bool bufferedOrbitReplay = false;
      QString solverScope;
      QMap<QString,QMap<QString,quint64>> solverBreaks;
      QMap<QString,QColor> segmentColors;
      QMap<QString,QColor> orbitColors,targetColors;
      bool automaticGeometry = false;
      QRect automaticRect;
      bool useInitialView = true;
      bool toolbarVisible = true;
      bool rawClockPublication = false;
      QString provider;
      // Consumed by one real UpdateGlPlot. Arc-camera preparation can read
      // this publication's matching mask without consuming the real sample.
      std::optional<OrbitDataAvailability> dataAvailability;
      QString arcProvider;
      bool arcTrial=false, arcMetadataOnly=false, replayArcCamera=false;
      QMap<QString,PlotPoint> arcPublishedPoses;
      QMap<int,PlotCamera> preparedArcCameras, replayArcCameras;
      QMap<int,QMap<int,PlotCamera>> bufferedArcCameras;
      OrbitTrialHistory trialHistory;
   };
   Entry *find(const std::string &name);
   Entry &create(const std::string &name, PlotModel::Kind kind, Real x, Real y, Real w, Real h, bool maximized);
   void refresh(Entry &entry, bool force = false);
   void clearOrbitSolverSamples(Entry &entry);
   void loadSkyCatalogs(Entry &entry,const std::string &name);
   bool remove(const std::string &name);
   void warn(const std::string &name, const std::string &option);
   template<class Fn> void curves(const std::string &name, int index, Fn fn) {
      if (auto *entry = find(name)) for (auto it=entry->data->curves.begin(); it!=entry->data->curves.end(); ++it)
         if (index < 0 || it.key()==index) fn(it.value());
   }
   void configureSimulationClocks();
   void syncSimulationClock(const QString &name);
   void simulationCommand(const QString &name,int command,double value);
   void updateSimulationTimer();
   PlotSimulationClocks simulationClocks;
   QTimer *simulationTimer=nullptr;
   QElapsedTimer simulationElapsed;
   QMdiArea *workspace;
   QMap<QString, Entry> entries;
   std::optional<int> sharedReplayPosition;
   PlotReplayMode sharedReplayMode=PlotReplayMode::InterpolatedSteps;
   QSet<QString> warnings;
   QMap<QString,PlotViewState> savedViews;
};
