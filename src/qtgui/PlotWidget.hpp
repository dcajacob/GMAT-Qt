#pragma once
#include <functional>
#include "PlotModel.hpp"
#include "PlotSimulationClock.hpp"
#include <QWidget>
#include <QStringList>
#include <QTransform>
#include <memory>
class QAction;
class QSlider;
class QComboBox;
class QTimer;
class QLabel;
class QLineEdit;
class QToolBar;
class QToolButton;
class OrbitRenderer;
class QRubberBand;
class PlotCanvas final : public QWidget
{
public:
   explicit PlotCanvas(std::shared_ptr<PlotModel> data, QWidget *parent = nullptr);
   void fit();
   void scriptView();
   void resetView();
   void restoreView(const PlotViewState &state);
   void setFrame(quint64 value);
   void setReplayCursor(const PlotReplayCursor &cursor);
   void setSimulationEpoch(std::optional<double> epoch,PlotReplayMode mode);
   quint64 frame() const { return visibleFrame; }
   double zoomFactor() const { return zoom; }
   void zoomBy(double wheelSteps);
   void setViewAngles(double azimuth, double elevation);
   void refresh();
   QImage captureImage();
protected:
   void resizeEvent(QResizeEvent *) override;
   void paintEvent(QPaintEvent *) override;
   void mousePressEvent(QMouseEvent *) override;
   void mouseMoveEvent(QMouseEvent *) override;
   void mouseReleaseEvent(QMouseEvent *) override;
   void leaveEvent(QEvent *) override;
   void mouseDoubleClickEvent(QMouseEvent *) override;
   void wheelEvent(QWheelEvent *) override;
   void keyPressEvent(QKeyEvent *) override;
   bool event(QEvent *) override;
private:
   std::shared_ptr<PlotModel> data;
   QPointF lastMouse, pan;
   double zoom = 1, yaw = 0.55, pitch = 0.45;
   quint64 visibleFrame = std::numeric_limits<quint64>::max();
   OrbitRenderer *renderer=nullptr;
   QRectF groundViewport;
   QTransform groundToCanvas;
   bool groundTransformValid=false;
   QRectF xyViewport,selectionViewport;
   QTransform xyToCanvas,selectionToData;
   bool xyTransformValid=false;
   std::optional<QRectF> xySelectionView;
   std::optional<QPointF> selectionStart;
   QRubberBand *selectionBand=nullptr;
   QString standardTooltip,groundCoordinates;
   void clearGroundReadout();
   void groundReadout(const QPointF &position,const QPoint &globalPosition);
   void cancelSelection();
};
class PlotWidget final : public QWidget
{
public:
   explicit PlotWidget(std::shared_ptr<PlotModel> data, QWidget *parent = nullptr);
   void refresh();
   void setSharedReplayPosition(int value);
   void setSharedReplayMode(PlotReplayMode mode);
   enum class SimulationCommand { Seek, Play, Start, Latest, Rate, Multiplier, Mode };
   std::function<void(SimulationCommand,double)> simulationRequested;
   std::function<void(bool)> toolbarVisibilityChanged;
   void setSimulationClock(const std::optional<PlotSimulationClocks::State> &state,const QString &root={});
   void setPlotToolbarVisible(bool visible);

   std::function<void()> replayRequested;
   std::function<void()> skyRequested;
   void restoreView(const PlotViewState &state);
   void setProjectionSaver(std::function<QString(bool,double)> callback);
   void setProtectedPaths(std::function<QStringList()> callback);
   QString exportData(const QString &path) const;
   QString exportImage(const QString &path);
   PlotCanvas *canvas() const { return drawing; }
private:
   std::shared_ptr<PlotModel> data;
   PlotCanvas *drawing;
   QSlider *timeline;
   QTimer *timer;
   QComboBox *replayMode;
   QComboBox *playbackSpeed=nullptr;
   QLineEdit *simulationRate=nullptr;
   QWidget *simulationRateRow=nullptr,*transportControls=nullptr;
   QToolBar *plotToolbar=nullptr;
   QToolButton *restoreToolbar=nullptr;
   std::optional<PlotSimulationClocks::State> simulationClock;


   QAction *replay=nullptr;
   QLabel *replayPosition=nullptr;
   QAction *saveProjection=nullptr;
   std::function<QString(bool,double)> projectionSaver;
   std::function<QStringList()> protectedPaths;
   quint64 historyGeneration=0;
   QVector<quint64> replayFrames;
   struct ReplaySample { double epoch=0,time=0; };
   QVector<ReplaySample> replayTimes;
   quint64 replaySourceFrame=std::numeric_limits<quint64>::max();
   quint64 replaySourceGeneration=std::numeric_limits<quint64>::max();
   size_t replayPointCount=0;
   void updateReplayFrame();
   void editPlotStyle();
   QString exportPathError(const QString &path) const;
};
