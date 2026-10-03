#include <QDialog>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QColorDialog>
#include <functional>
#include "PlotWidget.hpp"
#include <QSignalBlocker>
#include "OrbitRenderer.hpp"
#include "OrbitLabels.hpp"
#include "OrbitCamera.hpp"
#include "OrbitObjectAxes.hpp"
#include "OrbitObjectGuides.hpp"
#include "OrbitVelocity.hpp"
#include "OrbitVectors.hpp"
#include <QGuiApplication>
#include <QResizeEvent>
#include <QAction>
#include <QShortcut>
#include <QFileDialog>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSaveFile>
#include <QFileInfo>
#include <QTextStream>
#include <QLocale>
#include <QMessageBox>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace {
void drawMarker(QPainter &painter,QPointF p,int type,double size,const QColor &color,bool highlight)
{
   painter.save(); painter.setPen(QPen(color,1)); painter.setBrush(Qt::NoBrush);
   const double r=size/2;
   auto cross=[&] { painter.drawLine(p+QPointF(-r,-r),p+QPointF(r,r)); painter.drawLine(p+QPointF(-r,r),p+QPointF(r,-r)); };
   auto plus=[&] { painter.drawLine(p+QPointF(-r,0),p+QPointF(r,0)); painter.drawLine(p+QPointF(0,-r),p+QPointF(0,r)); };
   switch (type) {
   case 0: cross(); break;
   case 1: painter.drawEllipse(p,r,r); break;
   case 2: plus(); break;
   case 3: cross(); plus(); break;
   case 4: painter.drawRect(QRectF(p-QPointF(r,r),QSizeF(size,size))); break;
   case 5: painter.drawPolygon(QPolygonF({p+QPointF(0,-r),p+QPointF(r,0),p+QPointF(0,r),p+QPointF(-r,0)})); break;
   case 6: cross(); painter.drawRect(QRectF(p-QPointF(r,r),QSizeF(size,size))); break;
   case 7: case 8: {
      const double sign=type==7 ? 1 : -1;
      painter.drawPolygon(QPolygonF({p+QPointF(0,-r*sign),p+QPointF(r,r*sign),p+QPointF(-r,r*sign)})); break;
   }
   case 9: painter.drawEllipse(p,r,r); plus(); break;
   }
   if (highlight) painter.drawEllipse(p,r+3,r+3);
   painter.restore();
}
QPen curvePen(const PlotCurve &curve,const QColor &color)
{
   QPen pen(color,curve.width,curve.style);
   switch (curve.lineStyle) {
   case 101: pen.setStyle(Qt::DotLine); break;
   case 102: pen.setDashPattern({8,4}); break;
   case 103: pen.setDashPattern({4,2}); break;
   case 104: pen.setStyle(Qt::DashDotLine); break;
   case 106: pen.setStyle(Qt::NoPen); break;
   default: break;
   }
   return pen;
}
}

PlotCanvas::PlotCanvas(std::shared_ptr<PlotModel> model, QWidget *parent) : QWidget(parent), data(std::move(model))
{
   setMinimumSize(200, 140);
   setMouseTracking(true);
   if (data->kind==PlotModel::Kind::Orbit) setFocusPolicy(Qt::StrongFocus);
   setToolTip("Wheel: zoom · Drag: rotate orbit or pan chart · Shift-drag: pan orbit · Double-click: fit");
   if (data->kind==PlotModel::Kind::Orbit && QGuiApplication::platformName()!="offscreen" && QGuiApplication::platformName()!="minimal")
      renderer=new OrbitRenderer(data,this);
   if (data->userView) restoreView(*data->userView);
   else if (data->scriptedCamera) scriptView();
}
void PlotCanvas::refresh()
{
   if (data->kind==PlotModel::Kind::Orbit) data->userView=PlotViewState{zoom,yaw,pitch,pan,data->fitCamera,data->perspective,data->fieldOfView,
      data->cameraViews.isEmpty() ? QString() : data->cameraViews[data->selectedCamera].name,data->resetCamera};
   QStringList details{data->title};
   if (!data->xLabel.isEmpty()) details << "X: "+data->xLabel;
   if (!data->yLabel.isEmpty()) details << "Y: "+data->yLabel;
   if (data->legend) for (const auto &curve:data->curves) if (curve.visible && (data->kind!=PlotModel::Kind::Orbit || curve.drawsContent())) details << curve.name;
   details << "Wheel: zoom · Drag: rotate orbit or pan chart · Shift-drag: pan orbit · Double-click: fit";
   setToolTip(details.join('\n').toHtmlEscaped().replace("\n","<br>"));
   if (renderer) renderer->setView(zoom,yaw,pitch,pan,visibleFrame); else update();
}
QImage PlotCanvas::captureImage() { return renderer ? renderer->captureImage() : grab().toImage(); }
void PlotCanvas::resizeEvent(QResizeEvent *event) { QWidget::resizeEvent(event); if (renderer) renderer->setGeometry(rect()); }
void PlotCanvas::fit() { zoom = 1; pan = {}; data->fitCamera=true; refresh(); }
void PlotCanvas::scriptView() {
   if (!data->scriptedCamera) return;
   zoom=1; pan={}; yaw=0; pitch=0; data->fitCamera=false; data->resetCamera=false; refresh();
}
void PlotCanvas::resetView()
{
   if (!data->scriptedCamera) { yaw=.55; pitch=.45; fit(); return; }
   const bool named=data->selectedCamera>0 && data->selectedCamera<data->cameraViews.size();
   data->resetCamera=named ? data->cameraViews[data->selectedCamera].reset.has_value() : data->resetView.has_value();
   zoom=1; pan={}; yaw=0; pitch=0; data->fitCamera=false; refresh();
}
void PlotCanvas::restoreView(const PlotViewState &state)
{
   // Copy because refresh replaces data->userView, which can own the argument.
   const auto saved=state;
   zoom=saved.zoom; yaw=saved.yaw; pitch=saved.pitch; pan=saved.pan; data->fitCamera=saved.fit; data->resetCamera=saved.reset;
   for (int i=0;i<data->cameraViews.size();++i) if (data->cameraViews[i].name==saved.camera) { data->selectedCamera=i; break; }
   data->perspective=saved.perspective; data->fieldOfView=saved.fieldOfView;
   if (!data->cameraViews.isEmpty()) { data->cameraViews[data->selectedCamera].perspective=saved.perspective; data->cameraViews[data->selectedCamera].fieldOfView=saved.fieldOfView; }
   refresh();
}
void PlotCanvas::setFrame(quint64 value) { visibleFrame = value; refresh(); }
void PlotCanvas::zoomBy(double steps) { zoom = std::clamp(zoom * std::pow(1.15, steps), 0.02, 200.0); refresh(); }
void PlotCanvas::setViewAngles(double azimuth, double elevation) { yaw = azimuth; pitch = elevation; refresh(); }
void PlotCanvas::mousePressEvent(QMouseEvent *event) { lastMouse = event->position(); if (data->kind==PlotModel::Kind::Orbit) setFocus(Qt::MouseFocusReason); }
void PlotCanvas::mouseMoveEvent(QMouseEvent *event)
{
   const auto delta = event->position() - lastMouse; lastMouse = event->position();
   if (!(event->buttons() & Qt::LeftButton)) return;
   if (data->kind == PlotModel::Kind::Orbit && !(event->modifiers() & Qt::ShiftModifier)) {
      yaw = std::remainder(yaw + delta.x() * 0.008, 6.283185307179586);
      pitch = std::remainder(pitch + delta.y() * 0.008, 6.283185307179586);
   } else pan += delta;
   refresh();
}
void PlotCanvas::mouseDoubleClickEvent(QMouseEvent *) { fit(); }
void PlotCanvas::wheelEvent(QWheelEvent *event) { zoomBy(event->angleDelta().y() / 120.0); event->accept(); }
void PlotCanvas::paintEvent(QPaintEvent *)
{
   if (renderer) return;
   QPainter painter(this);
   painter.setRenderHint(QPainter::Antialiasing);
   const bool orbit = data->kind == PlotModel::Kind::Orbit;
   const bool ground = data->kind == PlotModel::Kind::GroundTrack;
   const QColor background = orbit ? QColor(12, 18, 30) : QColor(Qt::white);
   const QColor foreground = orbit ? QColor(226, 235, 248) : QColor(25, 35, 45);
   painter.fillRect(rect(), background);
   painter.setPen(foreground);
   const auto metrics=painter.fontMetrics();
   painter.drawText(QRectF(10,4,width()-20,22),Qt::AlignCenter,metrics.elidedText(data->title,Qt::ElideRight,width()-20));
   // Wrap legend entries before reserving chart space. Keep enough height for
   // the plot itself; the tooltip retains full labels when space is exhausted.
   QList<const PlotCurve *> legendCurves;
   int preferredWidth=1;
   if (data->legend) for (const auto &curve:data->curves) if (curve.visible && (!orbit || curve.drawsContent())) {
      legendCurves << &curve;
      preferredWidth=std::max(preferredWidth,metrics.horizontalAdvance(curve.name)+34);
   }
   const int legendWidth=width()-24, lineHeight=std::max(20,metrics.height()+4);
   const int columns=std::max(1,legendWidth/preferredWidth);
   const int maxRows=std::max(1,std::min(4,(height()-130)/lineHeight));
   const int rows=std::min(maxRows,(static_cast<int>(legendCurves.size())+columns-1)/columns);
   const int top=32+rows*lineHeight;
   QRectF area(orbit ? 35 : 74,top,width()-(orbit ? 70 : 100),height()-top-56);
   if (ground && area.width() > 2 * area.height()) { const double w = area.height()*2; area.setLeft(area.center().x()-w/2); area.setWidth(w); }
   if (ground && area.height() > area.width()/2) { const double h = area.width()/2; area.setTop(area.center().y()-h/2); area.setHeight(h); }
   if (area.width() <= 0 || area.height() <= 0) return;
   double xmin = ground ? -180 : std::numeric_limits<double>::infinity();
   double xmax = ground ? 180 : -std::numeric_limits<double>::infinity();
   double ymin = ground ? -90 : std::numeric_limits<double>::infinity();
   double ymax = ground ? 90 : -std::numeric_limits<double>::infinity();
   double extent = 1;
   OrbitSceneBounds bounds;
   for (const auto &curve:data->curves) if (curve.drawsContent())
      for (const auto &point:curve.points) {
         const double guideRadius=orbitObjectGuideRadius(curve)*(curve.objectXYPlane ? 15 : curve.objectGrid ? 1 : 0);
         const double radius=std::max(curve.showObject ? curve.radius : 0,guideRadius);
         extent=std::max(extent,std::hypot(point.x,point.y,point.z)+radius);
         bounds.include(point.x,point.y,point.z,radius);
         if (orbit && curve.objectVelocity) if (const auto velocity=orbitVelocitySegment(point)) {
            extent=std::max(extent,velocity->end.length());
            bounds.include(velocity->end.x(),velocity->end.y(),velocity->end.z(),0);
         }
      }
   if (orbit) for (const auto &vector:data->vectors) for (const auto &sample:vector.samples)
      if (const auto arrow=orbitVectorArrow(*data,vector,sample)) for (const auto &point:{arrow->start,arrow->end}) {
         extent=std::max(extent,point.length()); bounds.include(point.x(),point.y(),point.z(),0);
      }
   const auto camera=orbitCamera(*data,visibleFrame,yaw,pitch,extent,area.width()/area.height(),&bounds);
   auto perspectiveScale=[&](const PlotPoint &point) {
      const auto relative=osg::Vec3d(point.x,point.y,point.z)-camera.target;
      const double depth=camera.distance-relative*camera.outward;
      return data->perspective ? (depth>1e-6 ? camera.distance/depth : 0.0) : 1.0;
   };
   auto project = [&](const PlotPoint &point) {
      if (!orbit) return QPointF(point.x, point.y);
      osg::Vec3d relative=osg::Vec3d(point.x,point.y,point.z)-camera.target;
      if (data->perspective) {
         const double hh=camera.distance*std::tan(std::clamp(data->fieldOfView,1.0,150.0)*3.14159265358979323846/360)/zoom;
         relative+=camera.right*(pan.x()*2*hh/area.height())-camera.up*(pan.y()*2*hh/area.height());
      }
      return QPointF(relative*camera.right,relative*camera.up)*perspectiveScale(point);
   };
   if (!ground) for (const auto &curve : data->curves) {
      if (!curve.visible) continue;
      for (const auto &point : curve.points) {
         // Stable fit across replay: frame selection does not change bounds.
         const auto p = project(point);
         xmin = std::min(xmin, p.x()-curve.radius); xmax = std::max(xmax, p.x()+curve.radius);
         ymin = std::min(ymin, p.y()-curve.radius-point.low); ymax = std::max(ymax, p.y()+curve.radius+point.high);
         extent = std::max(extent, std::hypot(std::hypot(point.x, point.y), point.z)+curve.radius);
      }
   }
   if (orbit) {
      const auto extent=data->perspective ? camera.distance*std::tan(std::clamp(data->fieldOfView,1.0,150.0)*3.14159265358979323846/360) : camera.extent;
      xmin = -extent; xmax = extent; ymin = xmin; ymax = xmax;
      const double aspect = area.width()/area.height();
      if (data->perspective || aspect > 1) { xmin *= aspect; xmax *= aspect; } else { ymin /= aspect; ymax /= aspect; }
   } else if (!ground) {
      if (!std::isfinite(xmin)) { xmin=0; xmax=1; ymin=0; ymax=1; }
      const double dx = xmax == xmin ? std::max(1.0, std::abs(xmin)*0.05) : (xmax-xmin)*0.05;
      const double dy = ymax == ymin ? std::max(1.0, std::abs(ymin)*0.05) : (ymax-ymin)*0.08;
      xmin -= dx; xmax += dx; ymin -= dy; ymax += dy;
      auto limits=[](const PlotAxisOptions &axis,double &minimum,double &maximum) {
         if (axis.minimum) minimum=*axis.minimum;
         if (axis.maximum) maximum=*axis.maximum;
         // A fixed edge may lie beyond all current data. Keep the other,
         // automatic edge valid as new points arrive, without moving the fix.
         if (minimum>=maximum) {
            if (axis.minimum && !axis.maximum) maximum=minimum+std::max(1.,std::abs(minimum)*0.05);
            else if (axis.maximum && !axis.minimum) minimum=maximum-std::max(1.,std::abs(maximum)*0.05);
         }
      };
      limits(data->xAxis,xmin,xmax); limits(data->yAxis,ymin,ymax);
      int yWidth=0,xWidth=0;
      for (int i=0;i<=data->yAxis.ticks;++i) yWidth=std::max(yWidth,metrics.horizontalAdvance(QString::number(ymin+(ymax-ymin)*i/data->yAxis.ticks,'g',data->yAxis.precision)));
      for (int i=0;i<=data->xAxis.ticks;++i) xWidth=std::max(xWidth,metrics.horizontalAdvance(QString::number(xmin+(xmax-xmin)*i/data->xAxis.ticks,'g',data->xAxis.precision)));
      // Reserve space for the requested precision instead of clipping long
      // numeric labels into the old fixed-width axis gutters.
      area.setLeft(std::max(area.left(),std::min(width()/2.,double(yWidth+33))));
      area.setRight(width()-std::max(26.,std::min(width()/4.,xWidth/2.+6)));
   }
   const double cx = (xmin+xmax)/2, cy = (ymin+ymax)/2;
   const double dx = (xmax-xmin)/zoom, dy = (ymax-ymin)/zoom;
   const QPointF offset=orbit && data->perspective ? QPointF() : pan;
   xmin = cx-dx/2 - offset.x()*dx/area.width(); xmax = xmin+dx;
   ymin = cy-dy/2 + offset.y()*dy/area.height(); ymax = ymin+dy;
   auto screen = [&](const QPointF &p) { return QPointF(area.left()+(p.x()-xmin)/(xmax-xmin)*area.width(), area.bottom()-(p.y()-ymin)/(ymax-ymin)*area.height()); };
   painter.save(); painter.setClipRect(area);
   if (ground && !data->map.isNull()) {
      const QPointF a = screen({-180,90}), b = screen({180,-90});
      painter.drawImage(QRectF(a,b), data->map);
   }
   if (data->grid) {
      painter.setPen(QPen(ground ? QColor(255,255,255,100) : orbit ? QColor(90,110,140,75) : QColor(215,222,231), 1));
      const int nx = ground ? data->longitudeLines : orbit ? 5 : data->xAxis.ticks, ny = ground ? data->latitudeLines : orbit ? 5 : data->yAxis.ticks;
      for (int i=0;i<=nx;++i) {
         const double x = ground ? -180.0+360.0*i/nx : xmin+(xmax-xmin)*i/nx;
         painter.drawLine(screen({x,ymin}), screen({x,ymax}));
      }
      for (int i=0;i<=ny;++i) {
         const double y = ground ? -90.0+180.0*i/ny : ymin+(ymax-ymin)*i/ny;
         painter.drawLine(screen({xmin,y}), screen({xmax,y}));
      }
   }
   if (orbit && data->axes) {
      const QColor colors[] = {QColor(235,90,80),QColor(100,215,135),QColor(105,150,255)};
      const char *names[] = {"X","Y","Z"};
      for (int i=0;i<3;++i) {
         PlotPoint p; if (i==0) p.x=extent; if (i==1) p.y=extent; if (i==2) p.z=extent;
         const auto end=screen(project(p)); painter.setPen(colors[i]); painter.drawLine(screen(project(PlotPoint{})),end); painter.drawText(end+QPointF(4,-4),names[i]);
      }
   }
   if (orbit) {
      // Paint from far to near so foreground trajectory segments remain visible
      // over the central body, while the far side is occluded by its disk.
      struct Primitive { double depth; const PlotCurve *curve; const PlotPoint *a; const PlotPoint *b; int marker=0; bool guide=false; const PlotPoint *c=nullptr; double guideWidth=1; };
      QVector<Primitive> objects;
      std::deque<PlotPoint> clippedPoints;
      std::deque<PlotCurve> vectorCurves;
      auto depth=[&](const PlotPoint &p) { return osg::Vec3d(p.x,p.y,p.z)*camera.outward; };
      for (const auto &curve:data->curves) {
         if (!curve.visible) continue;
         const PlotPoint *previous=nullptr,*last=nullptr;
         const auto first=data->firstVisibleFrame(curve,visibleFrame);
         for (const auto &point:curve.points) {
            if (point.frame>visibleFrame || point.frame<first) continue;
            if (curve.lines && previous && point.connect) {
               const PlotPoint *a=previous,*b=&point;
               if (data->perspective) {
                  const double near=std::max(1e-6,std::min(extent*.01,camera.distance*.001));
                  const double da=camera.distance-(osg::Vec3d(a->x,a->y,a->z)-camera.target)*camera.outward-near;
                  const double db=camera.distance-(osg::Vec3d(b->x,b->y,b->z)-camera.target)*camera.outward-near;
                  if (da<0 && db<0) a=nullptr;
                  else if (da<0 || db<0) {
                     const double t=da/(da-db); PlotPoint clipped=*a;
                     clipped.x=a->x+(b->x-a->x)*t; clipped.y=a->y+(b->y-a->y)*t; clipped.z=a->z+(b->z-a->z)*t;
                     auto blend=[t](double x,double y) { return x+(y-x)*t; };
                     clipped.color=QColor::fromRgbF(blend(a->color.redF(),b->color.redF()),blend(a->color.greenF(),b->color.greenF()),blend(a->color.blueF(),b->color.blueF()),blend(a->color.alphaF(),b->color.alphaF()));
                     clippedPoints.push_back(clipped); if (da<0) a=&clippedPoints.back(); else b=&clippedPoints.back();
                  }
               }
               if (a) objects.append({(depth(*a)+depth(*b))/2,&curve,a,b});
            }
            if (curve.objectVelocity) if (const auto velocity=orbitVelocitySegment(point)) {
               auto from=velocity->start,to=velocity->end; bool visible=true;
               if (data->perspective) {
                  const double near=1e-6;
                  const double da=camera.distance-(from-camera.target)*camera.outward-near;
                  const double db=camera.distance-(to-camera.target)*camera.outward-near;
                  if (da<0 && db<0) visible=false;
                  else if (da<0 || db<0) { const auto clipped=from+(to-from)*(da/(da-db)); if (da<0) from=clipped; else to=clipped; }
               }
               if (visible) {
                  PlotPoint a=point,b=point; a.x=from.x(); a.y=from.y(); a.z=from.z(); b.x=to.x(); b.y=to.y(); b.z=to.z();
                  clippedPoints.push_back(a); const auto *start=&clippedPoints.back(); clippedPoints.push_back(b); const auto *end=&clippedPoints.back();
                  objects.append({(depth(a)+depth(b))/2,&curve,start,end,0,true});
               }
            }
            previous=&point; last=&point;
         }
         if (last && curve.objectAxes) {
            const auto axes=orbitObjectAxes(*last,orbitObjectAxisRadius(curve));
            auto line=[&](const osg::Vec3d &from,const osg::Vec3d &to) {
               PlotPoint a=*last,b=*last; a.x=from.x(); a.y=from.y(); a.z=from.z(); b.x=to.x(); b.y=to.y(); b.z=to.z();
               if (data->perspective) {
                  const double near=1e-6;
                  const double da=camera.distance-(from-camera.target)*camera.outward-near;
                  const double db=camera.distance-(to-camera.target)*camera.outward-near;
                  if (da<0 && db<0) return;
                  if (da<0 || db<0) { const auto clipped=from+(to-from)*(da/(da-db)); auto &point=da<0 ? a : b; point.x=clipped.x(); point.y=clipped.y(); point.z=clipped.z(); }
               }
               clippedPoints.push_back(a); const auto *start=&clippedPoints.back(); clippedPoints.push_back(b); const auto *end=&clippedPoints.back();
               objects.append({(depth(a)+depth(b))/2,&curve,start,end,0,true});
            };
            for (const auto &axis:axes) { line(axis.start,axis.end); for (const auto &wing:axis.arrow) line(axis.end,wing); }
         }
         if (last && (curve.objectGrid || curve.objectXYPlane)) {
            const auto geometry=orbitObjectGuides(curve,*last,orbitObjectGuideRadius(curve));
            auto point=[&](const osg::Vec3d &position,QColor color) -> const PlotPoint * {
               PlotPoint p; p.x=position.x(); p.y=position.y(); p.z=position.z(); p.color=color; clippedPoints.push_back(p); return &clippedPoints.back();
            };
            auto eyeDepth=[&](const osg::Vec3d &p) { return camera.distance-(p-camera.target)*camera.outward-1e-6; };
            for (const auto &edge:geometry.lines) {
               auto a=edge.start,b=edge.end;
               if (data->perspective) {
                  const double da=eyeDepth(a),db=eyeDepth(b); if (da<0 && db<0) continue;
                  if (da<0 || db<0) { const auto clip=a+(b-a)*(da/(da-db)); if (da<0) a=clip; else b=clip; }
               }
               const auto *pa=point(a,edge.color),*pb=point(b,edge.color);
               objects.append({(depth(*pa)+depth(*pb))/2,&curve,pa,pb,0,true,nullptr,edge.width});
            }
            for (const auto &triangle:geometry.triangles) {
               std::vector<osg::Vec3d> polygon(triangle.points.begin(),triangle.points.end());
               if (data->perspective) {
                  std::vector<osg::Vec3d> clipped; auto previous=polygon.back(); auto da=eyeDepth(previous);
                  for (const auto &current:polygon) {
                     const auto db=eyeDepth(current);
                     if ((da>=0)!=(db>=0)) clipped.push_back(previous+(current-previous)*(da/(da-db)));
                     if (db>=0) clipped.push_back(current); previous=current; da=db;
                  }
                  polygon=std::move(clipped);
               }
               for (size_t i=1;i+1<polygon.size();++i) {
                  const auto *a=point(polygon[0],triangle.color),*b=point(polygon[i],triangle.color),*c=point(polygon[i+1],triangle.color);
                  objects.append({(depth(*a)+depth(*b)+depth(*c))/3,&curve,a,b,0,true,c});
               }
            }
         }
         if (last && curve.showObject && perspectiveScale(*last)>0) objects.append({depth(*last),&curve,last,nullptr});
         for (const auto &marker:data->orbitMarkers(curve,visibleFrame)) {
            const auto &point=*marker.point;
            if (perspectiveScale(point)<=0) continue;
            // The fallback's bodies are spheres. Hide glyphs behind their near
            // surface, including an object's center inside its own shown body.
            const osg::Vec3d position(point.x,point.y,point.z);
            const auto eye=camera.target+camera.outward*camera.distance;
            const auto direction=data->perspective ? position-eye : -camera.outward;
            const auto origin=data->perspective ? eye : position+camera.outward*camera.distance;
            const double reach=data->perspective ? 1.0 : camera.distance;
            bool occluded=false;
            for (const auto &body:data->curves) if (body.visible && body.showObject && body.radius>0) {
               const PlotPoint *center=nullptr; for (const auto &p:body.points) if (p.frame<=visibleFrame) center=&p;
               if (!center) continue;
               const auto delta=origin-osg::Vec3d(center->x,center->y,center->z);
               const double a=direction*direction,b=delta*direction,c=delta*delta-body.radius*body.radius;
               const double discriminant=b*b-a*c;
               if (a>0 && discriminant>=0) { const double near=(-b-std::sqrt(discriminant))/a; if (near>=0 && near<reach-1e-9) { occluded=true; break; } }
            }
            if (!occluded) objects.append({depth(point),&curve,&point,nullptr,marker.endpoint ? 2 : 1});
         }
      }
      for (const auto &vector:data->vectors) if (const auto arrow=orbitVectorArrow(*data,vector,visibleFrame)) {
         vectorCurves.emplace_back(); auto &curve=vectorCurves.back(); curve.color=vector.color;
         auto line=[&](const osg::Vec3d &from,const osg::Vec3d &to) {
            PlotPoint a,b; a.color=b.color=vector.color;
            a.x=from.x(); a.y=from.y(); a.z=from.z(); b.x=to.x(); b.y=to.y(); b.z=to.z();
            if (data->perspective) {
               const double near=1e-6;
               const double da=camera.distance-(from-camera.target)*camera.outward-near;
               const double db=camera.distance-(to-camera.target)*camera.outward-near;
               if (da<0 && db<0) return;
               if (da<0 || db<0) { const auto clipped=from+(to-from)*(da/(da-db)); auto &point=da<0 ? a : b; point.x=clipped.x(); point.y=clipped.y(); point.z=clipped.z(); }
            }
            clippedPoints.push_back(a); const auto *start=&clippedPoints.back(); clippedPoints.push_back(b); const auto *end=&clippedPoints.back();
            objects.append({(depth(a)+depth(b))/2,&curve,start,end,0,true});
         };
         line(arrow->start,arrow->end); for (const auto &wing:arrow->arrow) line(arrow->end,wing);
      }
      std::stable_sort(objects.begin(),objects.end(),[](const Primitive &a,const Primitive &b) { return a.depth<b.depth; });
      for (const auto &object:objects) {
         const auto &curve=*object.curve;
         painter.setPen(QPen(object.a->color,object.guide ? object.guideWidth : curve.orbitLineWidth(),object.guide ? Qt::SolidLine : curve.style));
         const auto pixel=screen(project(*object.a));
         if (object.c) {
            painter.setPen(Qt::NoPen); painter.setBrush(object.a->color);
            painter.drawPolygon(QPolygonF{pixel,screen(project(*object.b)),screen(project(*object.c))});
         } else if (object.marker) {
            const double size=curve.orbitMarkerSize;
            if (object.marker==1) {
               painter.setPen(QPen(object.a->color,size*.1)); painter.setBrush(Qt::NoBrush); painter.drawEllipse(pixel,size*.45,size*.45);
            } else {
               QPainterPath rose;
               for (int i=0;i<=180;++i) { const double angle=i*3.14159265358979323846/90,r=size*.5*std::abs(std::sin(2*angle)); const auto edge=pixel+QPointF(r*std::cos(angle),r*std::sin(angle)); if (i==0) rose.moveTo(edge); else rose.lineTo(edge); }
               rose.closeSubpath(); painter.setPen(Qt::NoPen); painter.setBrush(object.a->color); painter.drawPath(rose);
            }
         } else if (object.b) painter.drawLine(pixel,screen(project(*object.b)));
         else if (curve.radius>0) {
            const double radius=curve.radius/(xmax-xmin)*area.width()*perspectiveScale(*object.a);
            QRadialGradient gradient(pixel-QPointF(radius*.3,radius*.3),radius*1.4);
            gradient.setColorAt(0,curve.color.lighter(145)); gradient.setColorAt(1,curve.color.darker(240));
            painter.setPen(curve.color); painter.setBrush(data->wireframe || curve.wireframeObject ? QBrush(Qt::NoBrush) : QBrush(gradient)); painter.drawEllipse(pixel,radius,radius);
         } else { painter.setBrush(object.a->color); painter.drawEllipse(pixel,3.5,3.5); }
      }
      if (data->labels) for (const auto &vector:data->vectors) if (!vector.label.isEmpty())
         if (const auto arrow=orbitVectorArrow(*data,vector,visibleFrame)) {
            PlotPoint point; point.x=arrow->end.x(); point.y=arrow->end.y(); point.z=arrow->end.z();
            if (perspectiveScale(point)>0) { painter.setPen(vector.color); painter.drawText(screen(project(point))+QPointF(4,-4),vector.label); }
         }
      if (data->labels) for (const auto &curve:data->curves) if (curve.visible && curve.objectAxes) {
         const auto *pose=orbitObjectPose(curve,visibleFrame); if (!pose) continue;
         const auto axes=orbitObjectAxes(*pose,orbitObjectAxisRadius(curve)); painter.setPen(pose->color);
         for (int axis=0;axis<3;++axis) { PlotPoint p=*pose; p.x=axes[axis].end.x(); p.y=axes[axis].end.y(); p.z=axes[axis].end.z(); if (perspectiveScale(p)>0) painter.drawText(screen(project(p))+QPointF(4,-4),QString(QChar('X'+axis))); }
      }
      if (data->labels) for (const auto &curve:data->curves) {
         if (!curve.drawsLabel()) continue;
         const PlotPoint *last=nullptr;
         for (const auto &point:curve.points) if (point.frame<=visibleFrame) last=&point;
         if (last && perspectiveScale(*last)>0) { painter.setPen(foreground); drawOrbitLabel(painter,screen(project(*last)),curve); }
      }
   }
   if (!orbit) for (const auto &curve : data->curves) {
      if (!curve.visible) continue;
      const PlotPoint *previous=nullptr, *last=nullptr;
      const auto first=data->firstVisibleFrame(curve,visibleFrame);
      for (const auto &point : curve.points) {
         if (point.frame > visibleFrame || point.frame<first) continue;
         const QPointF position=project(point), pixel=screen(position);
         painter.setPen(curvePen(curve,point.color));
         if (ground && data->footprints) {
            // Match wx's legacy five-degree reference circles; these are not
            // computed sensor coverage or horizon visibility footprints.
            const auto footprint=PlotModel::groundFootprint(position);
            for (int i=1;i<footprint.size();++i)
               for (const auto &segment:PlotModel::groundSegments(footprint[i-1],footprint[i]))
                  painter.drawLine(screen(segment.first),screen(segment.second));
         }
         if (curve.lines && previous && point.connect) {
            if (ground) for (const auto &segment : PlotModel::groundSegments(project(*previous),position)) painter.drawLine(screen(segment.first),screen(segment.second));
            else painter.drawLine(screen(project(*previous)),pixel);
         }
         if (!orbit && !ground && curve.errorBars && (point.high != 0 || point.low != 0)) {
            const auto high=screen({point.x, point.y+point.high}), low=screen({point.x,point.y-point.low});
            painter.drawLine(high,low); painter.drawLine(high-QPointF(3,0),high+QPointF(3,0)); painter.drawLine(low-QPointF(3,0),low+QPointF(3,0));
         }
         if (curve.markers || point.highlighted) drawMarker(painter,pixel,point.marker<0 ? curve.markerType : point.marker,
            curve.markerSize,point.color,point.highlighted);
         previous=&point; last=&point;
      }
      if (last && curve.showObject && (orbit || ground)) {
         const QPointF pixel=screen(project(*last));
         painter.setPen(last->color); painter.setBrush(last->color); painter.drawEllipse(pixel,3.5,3.5);
         if (data->labels) { painter.setPen(orbit ? foreground : last->color); painter.drawText(pixel+QPointF(7,-7),curve.name); }
      }
   }
   for (const auto &station : data->stations) {
      const auto p=screen({station.longitude,station.latitude}); painter.setPen(station.color); painter.setBrush(station.color);
      painter.drawRect(QRectF(p-QPointF(3,3),QSizeF(6,6))); if (data->labels) painter.drawText(p+QPointF(6,-6),station.name);
   }
   painter.restore(); painter.setPen(foreground);
   if (!orbit) {
      painter.drawRect(area);
      const int nx=ground ? 5 : data->xAxis.ticks,ny=ground ? 5 : data->yAxis.ticks;
      for (int i=0;i<=nx;++i) {
         const double x=xmin+(xmax-xmin)*i/nx,xp=area.left()+area.width()*i/nx;
         const auto label=QString::number(x,'g',ground ? 5 : data->xAxis.precision); const int labelWidth=std::max(76,metrics.horizontalAdvance(label)+8);
         painter.drawText(QRectF(xp-labelWidth/2.,area.bottom()+4,labelWidth,20),Qt::AlignCenter,label);
      }
      for (int i=0;i<=ny;++i) {
         const double y=ymin+(ymax-ymin)*i/ny,yp=area.bottom()-area.height()*i/ny;
         painter.drawText(QRectF(24,yp-10,area.left()-31,20),Qt::AlignRight|Qt::AlignVCenter,QString::number(y,'g',ground ? 5 : data->yAxis.precision));
      }
      painter.drawText(QRectF(area.left(),height()-28,area.width(),22),Qt::AlignCenter,metrics.elidedText(data->xLabel,Qt::ElideMiddle,static_cast<int>(area.width())));
      painter.save(); painter.translate(1,area.center().y()); painter.rotate(-90); painter.drawText(QRectF(-area.height()/2,0,area.height(),20),Qt::AlignCenter,painter.fontMetrics().elidedText(data->yLabel,Qt::ElideRight,static_cast<int>(area.height()))); painter.restore();
   } else painter.drawText(QRectF(10,height()-28,width()-20,22),Qt::AlignCenter,data->coordinates + " · km");
   const int cells=rows*columns, cellWidth=legendWidth/columns;
   for (int i=0;i<std::min(cells,static_cast<int>(legendCurves.size()));++i) {
      const int x=12+(i%columns)*cellWidth, y=28+(i/columns)*lineHeight;
      const bool overflow=i==cells-1 && legendCurves.size()>cells;
      if (overflow) {
         painter.setPen(foreground);
         painter.drawText(QRect(x,y,cellWidth,lineHeight),Qt::AlignVCenter,
            metrics.elidedText(QString("+ %1 more (hover for names)").arg(legendCurves.size()-i),Qt::ElideRight,cellWidth));
      } else {
         const auto &curve=*legendCurves[i];
         painter.setPen(QPen(curve.color,2)); painter.drawLine(x,y+lineHeight/2,x+15,y+lineHeight/2);
         painter.setPen(foreground);
         painter.drawText(QRect(x+20,y,cellWidth-28,lineHeight),Qt::AlignVCenter,
            metrics.elidedText(curve.name,Qt::ElideMiddle,cellWidth-28));
      }
   }
}

PlotWidget::PlotWidget(std::shared_ptr<PlotModel> model, QWidget *parent) : QWidget(parent), data(std::move(model))
{
   historyGeneration=data->historyGeneration;
   auto *layout = new QVBoxLayout(this); layout->setContentsMargins(0,0,0,0);
   auto *bar = new QToolBar(this); layout->addWidget(bar);
   drawing = new PlotCanvas(data,this);
   auto *fit = bar->addAction("Fit"); connect(fit,&QAction::triggered,drawing,&PlotCanvas::fit);
   if (data->kind==PlotModel::Kind::Orbit) {
      auto *cameraDialog=new QDialog(this); cameraDialog->setObjectName("orbitCameraDialog");
      cameraDialog->setWindowTitle(data->title+" — Camera");
      auto *cameraLayout=new QVBoxLayout(cameraDialog);
      auto *help=new QLabel("Changes preview immediately in this viewer. Keep projection writes the selected camera's projection and field of view to the script; save the script to retain them on disk.",cameraDialog);
      help->setWordWrap(true); cameraLayout->addWidget(help);
      auto *cameraForm=new QFormLayout; cameraLayout->addLayout(cameraForm);
      auto *cameraAction=bar->addAction("Camera…"); cameraAction->setObjectName("orbitCameraAction");
      connect(cameraAction,&QAction::triggered,this,[cameraDialog] { cameraDialog->show(); cameraDialog->raise(); cameraDialog->activateWindow(); });
      auto *projection=new QComboBox(cameraDialog); projection->setObjectName("orbitProjection");
      projection->addItems({"Orthographic","Perspective"}); projection->setCurrentIndex(data->perspective ? 1 : 0);
      projection->setToolTip("Camera projection"); cameraForm->addRow("Projection",projection);
      auto *fov=new QDoubleSpinBox(cameraDialog); fov->setObjectName("orbitFieldOfView"); fov->setRange(1,150);
      fov->setSuffix("°"); fov->setValue(data->fieldOfView); fov->setEnabled(data->perspective);
      fov->setToolTip("Vertical field of view before wheel zoom"); cameraForm->addRow("Field of view",fov);
      connect(projection,&QComboBox::currentIndexChanged,this,[this,fov](int index) {
         data->perspective=index==1; fov->setEnabled(data->perspective);
         if (!data->cameraViews.isEmpty()) data->cameraViews[data->selectedCamera].perspective=data->perspective;
         drawing->refresh();
      });
      connect(fov,&QDoubleSpinBox::valueChanged,this,[this](double value) {
         data->fieldOfView=value;
         if (!data->cameraViews.isEmpty()) data->cameraViews[data->selectedCamera].fieldOfView=value;
         drawing->refresh();
      });
      if (!data->cameraViews.isEmpty()) {
         auto *views=new QComboBox(cameraDialog); views->setObjectName("orbitCameraView");
         views->setToolTip("Switch imported cameras without rerunning the mission; replay uses the selected camera's history");
         for (const auto &view:data->cameraViews) views->addItem(view.name);
         views->setCurrentIndex(data->selectedCamera); cameraForm->insertRow(0,"Camera",views);
         connect(views,&QComboBox::currentIndexChanged,this,[this,projection,fov](int index) {
            if (index<0 || index>=data->cameraViews.size()) return;
            data->selectedCamera=index;
            const auto &view=data->cameraViews[index]; data->perspective=view.perspective; data->fieldOfView=view.fieldOfView;
            const QSignalBlocker blockProjection(projection),blockFov(fov);
            projection->setCurrentIndex(data->perspective ? 1 : 0); fov->setValue(data->fieldOfView); fov->setEnabled(data->perspective);
            drawing->scriptView();
         });
      }
      saveProjection=new QAction("Keep projection",this); saveProjection->setObjectName("saveOrbitProjection");
      saveProjection->setEnabled(false);
      saveProjection->setToolTip("Write projection and field of view to the script as an undoable edit; save the script to keep them on disk");
      connect(saveProjection,&QAction::triggered,this,[this] {
         if (!projectionSaver) return;
         const auto error=projectionSaver(data->perspective,data->fieldOfView);
         if (!error.isEmpty()) QMessageBox::warning(this,"Could not keep projection",error);
      });
      auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close,cameraDialog); cameraLayout->addWidget(buttons);
      auto *keep=buttons->addButton("Keep projection",QDialogButtonBox::ActionRole); keep->setObjectName("keepOrbitProjection");
      keep->setEnabled(saveProjection->isEnabled()); keep->setToolTip(saveProjection->toolTip());
      connect(saveProjection,&QAction::changed,keep,[this,keep] { keep->setEnabled(saveProjection->isEnabled()); });
      connect(keep,&QPushButton::clicked,saveProjection,&QAction::trigger);
      connect(buttons,&QDialogButtonBox::rejected,cameraDialog,&QDialog::reject);
      cameraDialog->resize(430,240);
      auto *displayDialog=new QDialog(this); displayDialog->setObjectName("orbitDisplayDialog");
      displayDialog->setWindowTitle(data->title+" — Display");
      auto *displayLayout=new QVBoxLayout(displayDialog);
      auto *displayHelp=new QLabel("Changes affect this viewer immediately. Use the plot resource editor to save display settings in the mission script.",displayDialog);
      displayHelp->setWordWrap(true); displayLayout->addWidget(displayHelp);
      auto *options=new QGridLayout; displayLayout->addLayout(options);
      struct DisplayOption { const char *label; const char *name; bool PlotModel::*value; };
      const DisplayOption displayOptions[]={
         {"Axes","axes",&PlotModel::axes},{"Grid","grid",&PlotModel::grid},
         {"Object labels","labels",&PlotModel::labels},{"Legend","legend",&PlotModel::legend},
         {"XY plane","xyPlane",&PlotModel::xyPlane},{"Ecliptic plane","eclipticPlane",&PlotModel::eclipticPlane},
         {"Wireframe bodies","wireframe",&PlotModel::wireframe},{"Origin–Sun line","sunLine",&PlotModel::sunLine},
         {"Stars","stars",&PlotModel::starsEnabled},{"Constellations","constellations",&PlotModel::constellationsEnabled}};
      QList<QPair<QCheckBox *,bool PlotModel::*>> displayControls;
      int optionIndex=0;
      for (const auto &option:displayOptions) {
         auto *control=new QCheckBox(option.label,displayDialog); control->setObjectName(QString("orbitDisplay_")+option.name);
         control->setChecked(data.get()->*option.value); options->addWidget(control,optionIndex/2,optionIndex%2); ++optionIndex;
         displayControls.append({control,option.value});
         connect(control,&QCheckBox::toggled,this,[this,member=option.value](bool checked) {
            data.get()->*member=checked;
            if (checked && (member==&PlotModel::starsEnabled || member==&PlotModel::constellationsEnabled) && skyRequested) skyRequested();
            drawing->refresh();
         });
      }
      auto *displayClose=new QDialogButtonBox(QDialogButtonBox::Close,displayDialog); displayLayout->addWidget(displayClose);
      connect(displayClose,&QDialogButtonBox::rejected,displayDialog,&QDialog::reject);
      auto *displayAction=bar->addAction("Display…"); displayAction->setObjectName("orbitDisplayAction");
      connect(displayAction,&QAction::triggered,this,[this,displayDialog,displayControls] {
         for (const auto &control:displayControls) { const QSignalBlocker block(control.first); control.first->setChecked(data.get()->*control.second); }
         displayDialog->show(); displayDialog->raise(); displayDialog->activateWindow();
      });
      displayDialog->resize(410,240);
      auto *scriptView=bar->addAction("Script view");
      scriptView->setToolTip("Restore the scripted camera, tracking and scale");
      connect(scriptView,&QAction::triggered,drawing,&PlotCanvas::scriptView);
      auto *resetView=bar->addAction("Reset view"); resetView->setObjectName("orbitResetView");
      resetView->setToolTip("Reset the selected camera to its default pose (Space in the plot)");
      auto *resetShortcut=new QShortcut(QKeySequence(Qt::Key_Space),drawing); resetShortcut->setObjectName("orbitResetShortcut");
      resetShortcut->setContext(Qt::WidgetWithChildrenShortcut);
      connect(resetShortcut,&QShortcut::activated,resetView,&QAction::trigger);
      connect(resetView,&QAction::triggered,drawing,&PlotCanvas::resetView);
      auto *sunlight=bar->addAction("Sunlight"); sunlight->setCheckable(true); sunlight->setChecked(data->sunlight);
      sunlight->setToolTip("Use the mission's Sun position; turn off for lighting from the camera");
      connect(sunlight,&QAction::toggled,this,[this](bool checked) { data->sunlight=checked; drawing->refresh(); });
   }
   if (data->kind==PlotModel::Kind::XY) {
      auto *style=bar->addAction("Style…"); style->setObjectName("plotStyleAction");
      connect(style,&QAction::triggered,this,&PlotWidget::editPlotStyle);
      auto *exportAction=bar->addAction("Export data…"); exportAction->setObjectName("plotExportData");
      exportAction->setToolTip("Save all retained curves, including points outside the visible range");
      connect(exportAction,&QAction::triggered,this,[this] {
         const auto path=QFileDialog::getSaveFileName(this,"Export plot data",QFileInfo(data->title).fileName()+".txt","Text files (*.txt);;All files (*)");
         if (path.isEmpty()) return;
         const auto error=exportData(path); if (!error.isEmpty()) QMessageBox::warning(this,"Could not export plot data",error);
      });
   }
   auto *save = bar->addAction("Save image…"); save->setObjectName("plotSaveImage");
   connect(save,&QAction::triggered,this,[this] {
      const auto path=QFileDialog::getSaveFileName(this,"Save plot image",data->title+".png","PNG image (*.png)");
      if (path.isEmpty()) return;
      const auto error=exportImage(path);
      if (!error.isEmpty()) QMessageBox::warning(this,"Could not save plot",error);
   });
   timeline = new QSlider(Qt::Horizontal,this); timeline->setObjectName("plotTimeline"); timeline->setRange(0,1000); timeline->setValue(1000);
   timeline->hide();
   timer = new QTimer(this); timer->setObjectName("plotReplayTimer"); timer->setInterval(30);
   if (data->kind != PlotModel::Kind::XY) {
      auto *transport=new QWidget(this); transport->setObjectName("plotPlaybackControls");
      auto *transportLayout=new QVBoxLayout(transport); transportLayout->setContentsMargins(4,0,4,2); transportLayout->setSpacing(2);
      auto *buttons=new QHBoxLayout; buttons->setSpacing(2); transportLayout->addLayout(buttons);
      auto *position=new QHBoxLayout; position->setSpacing(6); transportLayout->addLayout(position);
      layout->addWidget(transport);
      auto addPlaybackAction=[&](QStyle::StandardPixmap icon,const QString &text) {
         auto *action=new QAction(style()->standardIcon(icon),text,transport);
         auto *button=new QToolButton(transport); button->setDefaultAction(action);
         button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); buttons->addWidget(button);
         return action;
      };
      auto *start=addPlaybackAction(QStyle::SP_MediaSkipBackward,"Start"); start->setObjectName("plotReplayStart");
      start->setToolTip("Pause and return to the start of retained history");
      replay=addPlaybackAction(QStyle::SP_MediaPlay,"Play"); replay->setObjectName("plotReplayPlay"); replay->setCheckable(true);
      replay->setToolTip("Play or pause recorded history; resumes from the current position");
      auto *latest=addPlaybackAction(QStyle::SP_MediaSkipForward,"Latest"); latest->setObjectName("plotReplayLatest");
      latest->setToolTip("Stop playback and follow the latest available data");
      connect(start,&QAction::triggered,this,[this] { if (replayRequested) replayRequested(); replay->setChecked(false); timeline->setValue(0); });
      connect(latest,&QAction::triggered,this,[this] { if (replayRequested) replayRequested(); replay->setChecked(false); timeline->setValue(1000); });
      connect(replay,&QAction::triggered,this,[this] { if (replayRequested) replayRequested(); });
      connect(replay,&QAction::toggled,this,[this](bool checked) {
         replay->setText(checked ? "Pause" : "Play");
         replay->setIcon(style()->standardIcon(checked ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
         if (checked) { if (timeline->value()==1000) timeline->setValue(0); timer->start(); } else timer->stop();
      });
      timeline->setToolTip("Position within retained mission history"); timeline->setAccessibleName("Playback position");
      timeline->setMinimumWidth(100); timeline->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
      position->addWidget(timeline,1); timeline->show();
      replayPosition=new QLabel("Latest",transport); replayPosition->setObjectName("plotReplayPosition");
      replayPosition->setMinimumWidth(replayPosition->fontMetrics().horizontalAdvance("Latest")+8); position->addWidget(replayPosition);
      auto *speed=new QComboBox(transport); speed->setObjectName("plotReplaySpeed"); speed->setAccessibleName("Playback speed");
      speed->setToolTip("Playback speed relative to a three-second sweep of recorded history");
      for (double rate:{0.25,0.5,1.,2.,4.}) speed->addItem(QString::number(rate)+QString::fromUtf8("×"),rate);
      speed->setCurrentIndex(2); buttons->addStretch(); buttons->addWidget(speed);
      connect(timer,&QTimer::timeout,this,[this,speed,remainder=0.]() mutable {
         const double advance=10*speed->currentData().toDouble()+remainder;
         const int steps=static_cast<int>(advance); remainder=advance-steps;
         timeline->setValue(std::min(1000,timeline->value()+steps));
         if (timeline->value()==1000) replay->setChecked(false);
      });
      connect(timeline,&QSlider::sliderPressed,this,[this] { if (replayRequested) replayRequested(); replay->setChecked(false); });

   }
   connect(timeline,&QSlider::valueChanged,this,[this] { if (replayRequested) replayRequested(); updateReplayFrame(); });
   layout->addWidget(drawing,1);
}
void PlotWidget::setSharedReplayPosition(int value)
{
   timer->stop();
   if (replay) replay->setChecked(false);
   const QSignalBlocker block(timeline);
   timeline->setValue(std::clamp(value,0,1000));
   updateReplayFrame();
}
void PlotWidget::updateReplayFrame()
{
   const int value=timeline->value();
   if (replayPosition) replayPosition->setText(value==1000 ? "Latest" : QString::number(value/10.)+"%");
   if (value==1000) { drawing->setFrame(std::numeric_limits<quint64>::max()); return; }
   size_t pointCount=0;
   for (const auto &curve:data->curves) pointCount+=curve.points.size();
   if (replaySourceFrame!=data->frame || replaySourceGeneration!=data->historyGeneration || replayPointCount!=pointCount) {
      // Current solver mode discards trial samples but retains their callback
      // serials. Sweep actual retained frames, preserving their IDs so cameras
      // and vector histories still select the same publication as the curves.
      replayFrames.clear();
      for (const auto &curve:data->curves) for (const auto &point:curve.points) replayFrames.append(point.frame);
      std::sort(replayFrames.begin(),replayFrames.end());
      replayFrames.erase(std::unique(replayFrames.begin(),replayFrames.end()),replayFrames.end());
      replaySourceFrame=data->frame; replaySourceGeneration=data->historyGeneration; replayPointCount=pointCount;
   }
   drawing->setFrame(replayFrames.isEmpty() ? data->frame : replayFrames[static_cast<qsizetype>((replayFrames.size()-1)*(value/1000.0))]);
}
void PlotWidget::restoreView(const PlotViewState &state)
{
   drawing->restoreView(state);
   if (auto *views=findChild<QComboBox *>("orbitCameraView")) { const QSignalBlocker block(views); views->setCurrentIndex(data->selectedCamera); }
   if (auto *projection=findChild<QComboBox *>("orbitProjection")) { const QSignalBlocker block(projection); projection->setCurrentIndex(data->perspective ? 1 : 0); }
   if (auto *fov=findChild<QDoubleSpinBox *>("orbitFieldOfView")) { const QSignalBlocker block(fov); fov->setValue(data->fieldOfView); fov->setEnabled(data->perspective); }
}

void PlotWidget::refresh()
{
   if (historyGeneration!=data->historyGeneration) {
      historyGeneration=data->historyGeneration;
      timer->stop();
      if (replay) replay->setChecked(false);
      const QSignalBlocker block(timeline);
      timeline->setValue(1000);
   }
   // The retained interval moves as MaxPlotPoints evicts old samples. Resolve
   // the slider against that interval on every update, not only mouse movement.
   updateReplayFrame();
}

void PlotWidget::editPlotStyle()
{
   QDialog dialog(this); dialog.setObjectName("plotStyleDialog"); dialog.setWindowTitle("XY plot style");
   auto *layout=new QVBoxLayout(&dialog);
   auto *tabs=new QTabWidget(&dialog); tabs->setObjectName("plotStyleTabs"); layout->addWidget(tabs);
   auto *general=new QWidget(tabs); auto *generalLayout=new QVBoxLayout(general); tabs->addTab(general,"Plot");
   auto *labels=new QFormLayout; generalLayout->addLayout(labels);
   auto label=[&](const QString &name,const QString &id,const QString &value) { auto *field=new QLineEdit(value,general); field->setObjectName(id); labels->addRow(name,field); return field; };
   auto *title=label("Title","plotTitle",data->title),*xLabel=label("X axis label","plotXLabel",data->xLabel),*yLabel=label("Y axis label","plotYLabel",data->yLabel);
   auto *grid=new QCheckBox("Show grid",&dialog),*legend=new QCheckBox("Show legend",&dialog);
   grid->setObjectName("plotGrid"); legend->setObjectName("plotLegend");
   grid->setChecked(data->grid); legend->setChecked(data->legend);
   generalLayout->addWidget(grid); generalLayout->addWidget(legend); generalLayout->addStretch();
   layout->addWidget(new QLabel("Changes apply to this plot until the mission is rebuilt.",&dialog));
   struct AxisControls { QCheckBox *minimumEnabled,*maximumEnabled; QLineEdit *minimum,*maximum; QSpinBox *ticks,*precision; };
   QVector<AxisControls> axes;
   for (int index=0;index<2;++index) {
      const auto &axis=index==0 ? data->xAxis : data->yAxis; const QString id=index==0 ? "plotXAxis" : "plotYAxis";
      auto *page=new QWidget(tabs); auto *form=new QFormLayout(page); tabs->addTab(page,index==0 ? "X axis" : "Y axis");
      auto limit=[&](const QString &name,const std::optional<double> &value,double fallback) {
         auto *enabled=new QCheckBox(name,page); enabled->setObjectName(id+name+"Enabled"); enabled->setChecked(value.has_value());
         auto *field=new QLineEdit(QString::number(value.value_or(fallback),'g',17),page); field->setObjectName(id+name); field->setEnabled(enabled->isChecked()); form->addRow(enabled,field);
         connect(enabled,&QCheckBox::toggled,field,&QLineEdit::setEnabled); return qMakePair(enabled,field);
      };
      const auto minimum=limit("Minimum",axis.minimum,0),maximum=limit("Maximum",axis.maximum,10);
      auto spin=[&](const QString &name,int value,int low,int high) { auto *field=new QSpinBox(page); field->setObjectName(id+name); field->setRange(low,high); field->setValue(value); form->addRow(name,field); return field; };
      axes.append({minimum.first,maximum.first,minimum.second,maximum.second,spin("Ticks",axis.ticks,1,index==0 ? 20 : 25),spin("Precision",axis.precision,2,16)});
   }
   auto *curves=new QWidget(tabs); auto *curveLayout=new QVBoxLayout(curves); tabs->addTab(curves,"Curves");
   auto *selector=new QComboBox(curves); selector->setObjectName("styleCurve"); curveLayout->addWidget(selector);
   auto *pages=new QStackedWidget(curves); curveLayout->addWidget(pages);
   QVector<std::function<void()>> apply;
   for (auto it=data->curves.cbegin();it!=data->curves.cend();++it) {
      const auto key=it.key(); const auto &curve=it.value();
      selector->addItem(curve.name);
      auto *page=new QWidget(pages); page->setObjectName("curveStyle_"+QString::number(key));
      auto *form=new QFormLayout(page); pages->addWidget(page);
      auto check=[&](const QString &name,bool value) {
         auto *box=new QCheckBox(page); box->setObjectName(name); box->setChecked(value); form->addRow(name,box); return box;
      };
      auto *visible=check("Visible",curve.visible),*lines=check("Lines",curve.lines),*markers=check("Markers",curve.markers),*errors=check("Error bars",curve.errorBars);
      auto spin=[&](const QString &name,int value,int maximum) {
         auto *box=new QSpinBox(page); box->setObjectName(name); box->setRange(1,maximum); box->setValue(value); form->addRow(name,box); return box;
      };
      auto *width=spin("Line width",curve.width,20),*size=spin("Marker size",curve.markerSize,30);
      auto *style=new QComboBox(page); style->setObjectName("Line style");
      const QStringList styles={"Solid","Dotted","Long dash","Short dash","Dash-dot","None"};
      const int codes[]={100,101,102,103,104,106};
      for (int i=0;i<styles.size();++i) style->addItem(styles[i],codes[i]);
      if (style->findData(curve.lineStyle)<0) style->addItem("Current style",curve.lineStyle);
      style->setCurrentIndex(style->findData(curve.lineStyle)); form->addRow("Line style",style);
      auto *marker=new QComboBox(page); marker->setObjectName("Marker shape");
      marker->addItems({"Cross","Circle","Plus","Star","Square","Diamond","Crossed square","Triangle up","Triangle down","Circled plus"});
      marker->setCurrentIndex(curve.markerType); form->addRow("Marker shape",marker);
      auto *color=new QPushButton(curve.color.name(),page); color->setObjectName("Curve color"); color->setProperty("selectedColor",curve.color); form->addRow("Color",color);
      connect(color,&QPushButton::clicked,&dialog,[&dialog,color] {
         const auto chosen=QColorDialog::getColor(color->property("selectedColor").value<QColor>(),&dialog,"Curve color");
         if (chosen.isValid()) { color->setProperty("selectedColor",chosen); color->setText(chosen.name()); }
      });
      apply.append([=,this] {
         auto found=data->curves.find(key); if (found==data->curves.end()) return;
         auto &curve=found.value(); const auto chosen=color->property("selectedColor").value<QColor>();
         for (auto &point:curve.points) {
            if (chosen!=curve.color) point.color=chosen;
            if (marker->currentIndex()!=curve.markerType) point.marker=marker->currentIndex();
         }
         curve.color=chosen; curve.markerType=marker->currentIndex(); curve.visible=visible->isChecked();
         curve.lines=lines->isChecked(); curve.markers=markers->isChecked(); curve.errorBars=errors->isChecked();
         curve.width=width->value(); curve.markerSize=size->value(); curve.lineStyle=style->currentData().toInt();
      });
   }
   connect(selector,&QComboBox::currentIndexChanged,pages,&QStackedWidget::setCurrentIndex);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
   auto *status=new QLabel(&dialog); status->setObjectName("plotAxisStatus"); status->setWordWrap(true); layout->insertWidget(layout->count()-1,status);
   auto validate=[axes,status,buttons] {
      QString error;
      for (const auto &axis:axes) {
         bool minOk=true,maxOk=true; const double minimum=axis.minimum->text().toDouble(&minOk),maximum=axis.maximum->text().toDouble(&maxOk);
         auto valid=[](double value,bool ok) { return ok && std::isfinite(value) && std::abs(value)<=1e100; };
         if ((axis.minimumEnabled->isChecked() && !valid(minimum,minOk)) || (axis.maximumEnabled->isChecked() && !valid(maximum,maxOk))) error="Enter a finite axis limit between -1e100 and 1e100.";
         else if (axis.minimumEnabled->isChecked() && axis.maximumEnabled->isChecked() && minimum>=maximum) error="Each axis minimum must be less than its maximum.";
      }
      status->setText(error); buttons->button(QDialogButtonBox::Ok)->setEnabled(error.isEmpty());
   };
   for (const auto &axis:axes) {
      connect(axis.minimum,&QLineEdit::textChanged,&dialog,[validate] { validate(); }); connect(axis.maximum,&QLineEdit::textChanged,&dialog,[validate] { validate(); });
      connect(axis.minimumEnabled,&QCheckBox::toggled,&dialog,[validate] { validate(); }); connect(axis.maximumEnabled,&QCheckBox::toggled,&dialog,[validate] { validate(); });
   }
   validate();
   connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
   connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
   if (dialog.exec()!=QDialog::Accepted) return;
   if (!buttons->button(QDialogButtonBox::Ok)->isEnabled()) return;
   bool changedLimits=false;
   for (int index=0;index<axes.size();++index) {
      const auto &controls=axes[index]; auto &axis=index==0 ? data->xAxis : data->yAxis;
      const auto minimum=controls.minimumEnabled->isChecked() ? std::optional<double>(controls.minimum->text().toDouble()) : std::nullopt;
      const auto maximum=controls.maximumEnabled->isChecked() ? std::optional<double>(controls.maximum->text().toDouble()) : std::nullopt;
      changedLimits|=minimum!=axis.minimum || maximum!=axis.maximum; axis.minimum=minimum; axis.maximum=maximum; axis.ticks=controls.ticks->value(); axis.precision=controls.precision->value();
   }
   data->title=title->text(); data->xLabel=xLabel->text(); data->yLabel=yLabel->text();
   data->grid=grid->isChecked(); data->legend=legend->isChecked();
   for (const auto &change:apply) change();
   if (changedLimits) drawing->fit(); else drawing->refresh();
}

void PlotWidget::setProjectionSaver(std::function<QString(bool,double)> callback)
{
   projectionSaver=std::move(callback);
   if (saveProjection) saveProjection->setEnabled(bool(projectionSaver));
}

void PlotWidget::setProtectedPaths(std::function<QStringList()> callback) { protectedPaths=std::move(callback); }
QString PlotWidget::exportData(const QString &path) const
{
   if (data->kind!=PlotModel::Kind::XY) return "Only XY plots have this data export format.";
   const auto error=exportPathError(path); if (!error.isEmpty()) return error;
   QSaveFile file(path);
   if (!file.open(QIODevice::WriteOnly)) return "Cannot export plot data: "+file.errorString();
   QTextStream output(&file); output.setLocale(QLocale::c()); output.setRealNumberPrecision(17);
   // Retain wx's title/axis headers and per-curve X,Y sections. Full stored
   // double precision is independent of the viewer's tick-label precision.
   output<<data->title<<'\n'<<data->xLabel<<"   "<<data->yLabel<<'\n';
   for (const auto &curve:data->curves) {
      output<<curve.name<<'\n';
      for (const auto &point:curve.points) output<<point.x<<", "<<point.y<<'\n';
      output<<'\n';
   }
   output.flush();
   if (output.status()!=QTextStream::Ok) { file.cancelWriting(); return "Cannot export plot data: "+file.errorString(); }
   if (!file.commit()) return "Cannot export plot data: "+file.errorString();
   return {};
}
QString PlotWidget::exportPathError(const QString &path) const
{
   if (path.isEmpty()) return "Choose a file for the plot export.";
   const QFileInfo destination(path);
   if (protectedPaths) for (const auto &name:protectedPaths()) {
      if (name.isEmpty()) continue;
      const QFileInfo input(name);
      if (destination.absoluteFilePath()==input.absoluteFilePath() || (!destination.canonicalFilePath().isEmpty() && destination.canonicalFilePath()==input.canonicalFilePath()))
         return "Choose a different file; plot export cannot overwrite an open mission, startup file or mission output.";
   }
   return {};
}
QString PlotWidget::exportImage(const QString &path)
{
   const auto error=exportPathError(path); if (!error.isEmpty()) return error;
   QSaveFile file(path);
   if (!file.open(QIODevice::WriteOnly)) return "Cannot save plot image: "+file.errorString();
   if (!drawing->captureImage().save(&file,"PNG")) { file.cancelWriting(); return "Cannot save plot image: "+file.errorString(); }
   if (!file.commit()) return "Cannot save plot image: "+file.errorString();
   return {};
}
