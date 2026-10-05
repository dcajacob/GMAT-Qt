#include "OrbitSensorMasks.hpp"
#include "OrbitRenderer.hpp"
#include "PlotWidget.hpp"
#include "PlotInterpolation.hpp"
#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QtMath>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
void settle(QApplication &app,int ms=25) { app.processEvents(); QEventLoop loop; QTimer::singleShot(ms,&loop,&QEventLoop::quit); loop.exec(); }
int different(const QImage &a,const QImage &b,QRect area={}) {
   require(!a.isNull() && a.size()==b.size(),"Incompatible sensor-mask captures"); if (area.isNull()) area=a.rect(); area=area.intersected(a.rect()); int n=0;
   for (int y=area.top();y<=area.bottom();++y) for (int x=area.left();x<=area.right();++x) n+=a.pixel(x,y)!=b.pixel(x,y); return n;
}
void preserve(const QString &name,const QImage &image,bool native) {
   const auto path=qEnvironmentVariable("GMAT_QT_SENSOR_MASK_EVIDENCE"); if (path.isEmpty()) return;
   require(QDir().mkpath(path) && image.save(path+'/'+(native ? "native-" : "fallback-")+name+".png"),"Cannot preserve sensor-mask capture");
}
QByteArray frozen(const PlotModel &model) {
   std::ostringstream out; out<<std::hexfloat;
   for (const auto &curve:model.curves) for (const auto &p:curve.points) {
      out<<curve.name.toStdString()<<' '<<p.frame<<' '<<p.epoch<<' '<<p.x<<' '<<p.y<<' '<<p.z<<' '<<p.connect<<' ';
      for (const auto r:p.bodyToView) out<<r<<' '; out<<'\n';
   }
   for (const auto &mask:model.sensorMasks) {
      out<<mask.name.toStdString()<<' '<<mask.source.toStdString()<<' '<<mask.hardware.toStdString()<<' '<<mask.label.toStdString()<<' '<<mask.automaticLength<<' '<<mask.length<<' '<<mask.sourceRadius<<' '<<mask.color.rgba()<<' '<<mask.drawEdges<<' ';
      for (const auto x:mask.origin) out<<x<<' '; for (const auto x:mask.axis) out<<x<<' '; for (const auto &p:mask.boundary) for (const auto x:p) out<<x<<' '; out<<'\n';
   }
   return QByteArray::fromStdString(out.str());
}
// Independent pinhole projection for the fixed analytic camera. The fallback's
// documented canvas margins leave a 570 by392 plot at (35,32) in640 by480.
QPointF pixel(bool native,double x,double y,double z=0) {
   const double height=native ? 480 : 392,centerY=native ? 240 : 228;
   const double scale=height*std::sqrt(3.)/(2*(20-z)); return {320+x*scale,centerY-y*scale};
}
int white(const QImage &image,QPointF center,bool native,int radius=2) {
   int count=0; for (int y=qRound(center.y())-radius;y<=qRound(center.y())+radius;++y) for (int x=qRound(center.x())-radius;x<=qRound(center.x())+radius;++x)
      if (image.rect().contains(x,y)) { const auto c=image.pixelColor(x,y); // The fallback's antialiased one-pixel white line can cover two
         // neighboring pixels. Native aliased lines retain a white core.
         count+=native ? c.red()>220 && c.green()>220 && c.blue()>220 :
            std::min({c.red(),c.green(),c.blue()})>140 && std::max({c.red(),c.green(),c.blue()})-std::min({c.red(),c.green(),c.blue()})<20; } return count;
}
QColor at(const QImage &image,QPointF center) { return image.pixelColor(qRound(center.x()),qRound(center.y())); }
void near(const osg::Vec3d &actual,const osg::Vec3d &expected,const char *message) { require((actual-expected).length()<2e-6,message); }
std::shared_ptr<PlotModel> fixture() {
   auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit); model->axes=false; model->grid=false; model->legend=false; model->labels=false;
   model->scriptedCamera=true; model->perspective=true; model->fieldOfView=60; model->coordinates="Frozen sensor frame";
   model->cameras.push_back({0,{0,0,20},{0,0,0},{0,1,0},false}); model->frame=2;
   auto &source=model->curves[0]; source.name="Sat"; source.radius=2; source.lines=false; source.showObject=false;
   PlotPoint first; first.frame=1; first.epoch=21545; first.x=-4; first.y=-1;
   PlotPoint second=first; second.frame=2; second.epoch+=10./86400; second.x=2; second.y=-4; second.connect=true; second.bodyToView={0,-1,0,1,0,0,0,0,1}; source.points={first,second};
   PlotSensorMask mask; mask.name="Sensor"; mask.source="Sat"; mask.hardware="Camera"; mask.automaticLength=false; mask.length=5; mask.sourceRadius=2;
   mask.origin={.5,0,0}; mask.axis={1,0,0}; mask.color=QColor(240,70,55,80); mask.drawEdges=true;
   mask.boundary={{1,-.5,-.5},{1,.5,-.5},{1,.5,.5},{1,-.5,.5}}; model->sensorMasks={mask}; return model;
}
void geometryChecks(PlotModel &model) {
   auto &mask=model.sensorMasks[0]; auto &source=model.curves[0];
   const auto initial=orbitSensorMaskGeometry(model,mask,quint64(1)); require(initial && initial->boundary.size()==4,"Frozen rectangular geometry absent");
   near(initial->origin,{-3.5,-1,0},"Mount location lost"); near(initial->end,{1.5,-1,0},"Manual axial length wrong"); near(initial->boundary[0],{1.5,-3.5,-2.5},"Axial boundary was incorrectly normalized");
   const auto rotated=orbitSensorMaskGeometry(model,mask,quint64(2)); require(bool(rotated),"Recorded geometry missing");
   near(rotated->origin,{2,-3.5,0},"Hardware location did not rotate with source"); near(rotated->end,{2,1.5,0},"Mask boresight did not follow source attitude"); near(rotated->boundary[0],{4.5,1.5,-2.5},"Mask boundary did not follow source attitude");
   require(!orbitSensorMaskGeometry(model,mask,quint64(0)),"Mask displayed before first source pose");
   mask.automaticLength=true; near(orbitSensorMaskGeometry(model,mask,quint64(1))->end,{-1.5,-1,0},"Auto fallback radius wrong");
   QMap<QString,OrbitObjectBounds> loaded; loaded.insert("Sat",{{9,8,7},4});
   near(orbitSensorMaskGeometry(model,mask,quint64(1),&loaded)->end,{.5,-1,0},"Native loaded bound radius not used or center moved mount"); mask.automaticLength=false;
   source.visible=false; require(!orbitSensorMaskGeometry(model,mask,quint64(1)),"Hidden source retained mask"); source.visible=true;
   mask.source="Missing"; require(!orbitSensorMaskGeometry(model,mask,quint64(1)),"Absent source retained mask"); mask.source="Sat";
   plotSetReplayCursor(model,PlotReplayCursor{1,2,.5,21545+5./86400}); const auto midpoint=orbitSensorMaskGeometry(model,mask,quint64(1)); require(bool(midpoint),"Interpolated geometry missing");
   const double root=std::sqrt(.5); near(midpoint->origin,{-1+.5*root,-2.5+.5*root,0},"Interpolated mount origin wrong"); near(midpoint->end,{-1+5.5*root,-2.5+5.5*root,0},"Interpolated mask position or attitude wrong");
   plotSetReplayCursor(model,{});
}
}
int main(int argc,char **argv) {
   QApplication app(argc,argv); const bool native=argc==2 && QString::fromLocal8Bit(argv[1])=="--native";
   if (argc>2 || QGuiApplication::platformName()!=(native ? "xcb" : "offscreen") || (native && qEnvironmentVariable("LIBGL_ALWAYS_SOFTWARE")!="1")) return 2;
   try {
      auto model=fixture(); geometryChecks(*model); const auto definitions=model->sensorMasks; model->sensorMasks.clear();
      PlotCanvas canvas(model); canvas.resize(640,480); canvas.show(); canvas.setFrame(1); settle(app,150);
      if (native) {
         auto *renderer=dynamic_cast<OrbitRenderer *>(canvas.findChild<QOpenGLWidget *>()); require(renderer && renderer->isValid(),"Actual native mask renderer unavailable");
         renderer->makeCurrent(); const auto gl=QString::fromLatin1(reinterpret_cast<const char *>(renderer->context()->functions()->glGetString(GL_RENDERER))); renderer->doneCurrent();
         require(!gl.isEmpty(),"GL renderer identity unavailable"); std::cout<<"Private OpenGL renderer: "<<gl.toStdString()<<'\n';
      }
      auto capture=[&](quint64 frame) { canvas.setFrame(frame); settle(app); return canvas.captureImage(); };
      const auto blank=capture(1); preserve("blank",blank,native); model->sensorMasks=definitions;
      const auto raw=frozen(*model); const auto first=capture(1); preserve("rectangle-manual",first,native);
      require(different(first,blank)>200,"Translucent sensor sides absent");
      require(white(first,pixel(native,1.5,-1,2.5),native)>0,"Rectangular base outline absent at analytic near-edge midpoint");
      const auto interior=at(first,pixel(native,0,-1)); require(interior.red()>interior.green()+35 && interior.red()>interior.blue()+35,"Sensor fill color absent at analytic interior");
      const auto second=capture(2); preserve("rectangle-replay",second,native); require(different(first,second)>200 && capture(1)==first,"Recorded sensor replay does not follow position and attitude or restore exactly");
      canvas.setReplayCursor({1,2,.5,21545+5./86400}); settle(app); const auto midpoint=canvas.captureImage(); preserve("rectangle-interpolated",midpoint,native);
      require(different(midpoint,first)>200 && different(midpoint,second)>200 && capture(1)==first && frozen(*model)==raw,"Interpolation did not move sensor geometry or mutated frozen data");
      model->sensorMasks[0].drawEdges=false; const auto noEdges=capture(1); preserve("rectangle-no-edges",noEdges,native); require(different(first,noEdges)>20,"Optional radial edges had no rendered effect");
      model->sensorMasks[0].color.setAlpha(15); const auto faint=capture(1); preserve("rectangle-faint",faint,native); const auto faintInterior=at(faint,pixel(native,0,-1));
      require(interior.red()>faintInterior.red()+25,"FieldOfView alpha did not change transparent side pixels"); model->sensorMasks=definitions;
      model->sensorMasks[0].automaticLength=true; const auto automatic=capture(1); preserve("rectangle-auto",automatic,native);
      require(different(first,automatic)>100 && white(automatic,pixel(native,-1.5,-1,1),native)>0,"Auto source radius did not set analytic base length"); model->sensorMasks=definitions;
      model->sensorMasks[0].label="Sensor field"; const auto text=capture(1); preserve("label",text,native);
      const auto end=pixel(native,1.5,-1); const QRect region(qFloor(end.x()+3),qFloor(end.y()-26),140,26); require(different(text,first,region)>30,"Sensor label absent at axial endpoint with object labels Off");
      model->labels=true; require(capture(1)==text,"Object-label master unexpectedly controls mask label"); model->labels=false; model->sensorMasks=definitions;
      require(capture(0)==blank,"Sensor appeared before first valid source pose"); model->curves[0].visible=false; require(capture(1)==blank,"Hidden source retained mask pixels"); model->curves[0].visible=true;
      model->sensorMasks[0].source="Missing"; require(capture(1)==blank,"Absent source retained mask pixels"); model->sensorMasks=definitions;
      for (int shape=0;shape<2;++shape) {
         auto &mask=model->sensorMasks[0]; mask.boundary.clear(); mask.drawEdges=shape==1;
         const int count=shape==0 ? 120 : 3; for (int i=0;i<count;++i) { const double angle=2*3.14159265358979323846*i/count; mask.boundary.append({1,.5*std::cos(angle),.5*std::sin(angle)}); }
         const auto image=capture(1); preserve(shape==0 ? "conical" : "custom",image,native); require(different(image,blank)>150,"Conical/custom frozen geometry not rendered");
      }
      model->sensorMasks=definitions; require(frozen(*model)==raw && capture(1)==first,"Shape display changed frozen mask/source data"); model->sensorMasks.clear(); require(capture(1)==blank,"Removed mask retained pixels");
      std::cout<<"PASS sensor-mask axial geometry, mount/body rotation, Auto/Manual bounds, rectangular/conical/custom sides, alpha, white outline/radial edges, independent label, recorded/interpolated replay, absent/hidden/removal and frozen data. Private "<<(native ? "Xvfb/software GL" : "offscreen fallback")<<" only.\n";
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; } return 0;
}
