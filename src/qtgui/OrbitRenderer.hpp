#pragma once
#include "PlotModel.hpp"
#include <QOpenGLWidget>
#include <memory>

bool orbitModelFileReadable(const QString &path);
bool orbitModelFileReadable(const QString &path,QString *error);

// A rendering surface only; PlotCanvas continues to own the existing controls.
class OrbitRenderer final : public QOpenGLWidget
{
public:
   explicit OrbitRenderer(std::shared_ptr<PlotModel> model,QWidget *parent=nullptr);
   ~OrbitRenderer() override;
   QImage captureImage();
   void setView(double zoom,double yaw,double pitch,QPointF pan,quint64 frame,
      const std::optional<std::array<double,9>> &rotation={});
protected:
   void initializeGL() override;
   void resizeGL(int width,int height) override;
   void paintGL() override;
private:
   void releaseGraphics();
   void drawOverlay(QPainter &painter);
   struct Scene;
   std::unique_ptr<Scene> scene;
};
