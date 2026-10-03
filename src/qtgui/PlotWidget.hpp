#pragma once
#include <functional>
#include "PlotModel.hpp"
#include <QWidget>
#include <QStringList>
#include <memory>
class QAction;
class QSlider;
class QTimer;
class QLabel;
class OrbitRenderer;
class PlotCanvas final : public QWidget
{
public:
   explicit PlotCanvas(std::shared_ptr<PlotModel> data, QWidget *parent = nullptr);
   void fit();
   void scriptView();
   void resetView();
   void restoreView(const PlotViewState &state);
   void setFrame(quint64 value);
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
   void mouseDoubleClickEvent(QMouseEvent *) override;
   void wheelEvent(QWheelEvent *) override;
private:
   std::shared_ptr<PlotModel> data;
   QPointF lastMouse, pan;
   double zoom = 1, yaw = 0.55, pitch = 0.45;
   quint64 visibleFrame = std::numeric_limits<quint64>::max();
   OrbitRenderer *renderer=nullptr;
};
class PlotWidget final : public QWidget
{
public:
   explicit PlotWidget(std::shared_ptr<PlotModel> data, QWidget *parent = nullptr);
   void refresh();
   void setSharedReplayPosition(int value);
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
   QAction *replay=nullptr;
   QLabel *replayPosition=nullptr;
   QAction *saveProjection=nullptr;
   std::function<QString(bool,double)> projectionSaver;
   std::function<QStringList()> protectedPaths;
   quint64 historyGeneration=0;
   QVector<quint64> replayFrames;
   quint64 replaySourceFrame=std::numeric_limits<quint64>::max();
   quint64 replaySourceGeneration=std::numeric_limits<quint64>::max();
   size_t replayPointCount=0;
   void updateReplayFrame();
   void editPlotStyle();
   QString exportPathError(const QString &path) const;
};
