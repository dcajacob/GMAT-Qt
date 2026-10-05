#include "OrbitStars.hpp"
#include "OrbitRenderer.hpp"
#include "PlotWidget.hpp"
#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QTemporaryDir>
#include <QTimer>
#include <QtMath>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
void settle(QApplication &app,int ms=20) { app.processEvents(); QEventLoop loop; QTimer::singleShot(ms,&loop,&QEventLoop::quit); loop.exec(); }
void preserve(const QString &name,const QImage &image,bool native) {
   const auto path=qEnvironmentVariable("GMAT_QT_STAR_STYLE_EVIDENCE"); if (path.isEmpty()) return;
   require(QDir().mkpath(path) && image.save(path+'/'+(native ? "native-" : "fallback-")+name+".png"),"Cannot preserve star capture");
}
QPointF pixel(bool native,double x,double y) {
   // Catalog directions (x,y,-1), 60-degree perspective and a fixed +Z eye.
   const double height=native ? 480 : 392,centerY=native ? 240 : 228;
   return {320+x*height*std::sqrt(3.)/2,centerY-y*height*std::sqrt(3.)/2};
}
QRect around(QPointF center,int radius=12) { return {qRound(center.x())-radius,qRound(center.y())-radius,2*radius+1,2*radius+1}; }
int illuminated(const QImage &image,QRect area) {
   area=area.intersected(image.rect()); int n=0;
   for (int y=area.top();y<=area.bottom();++y) for (int x=area.left();x<=area.right();++x) n+=qGray(image.pixel(x,y))>15; return n;
}
int brightness(const QImage &image,QRect area) {
   area=area.intersected(image.rect()); int result=0;
   for (int y=area.top();y<=area.bottom();++y) for (int x=area.left();x<=area.right();++x) result=std::max(result,qGray(image.pixel(x,y))); return result;
}
QByteArray frozen(const PlotModel &model) {
   std::ostringstream out; out<<std::hexfloat;
   for (const auto &star:model.starCatalog.stars) { for (const auto v:star.direction) out<<v<<' '; out<<star.magnitude<<' '<<star.colorIndex<<'\n'; }
   for (const auto &curve:model.curves) for (const auto &p:curve.points) {
      out<<p.frame<<' '<<p.epoch<<' '<<p.x<<' '<<p.y<<' '<<p.z<<' ';
      for (const auto r:p.inertialToView) out<<r<<' '; out<<'\n';
   } return QByteArray::fromStdString(out.str());
}
void catalogChecks() {
   QTemporaryDir files; require(files.isValid(),"Catalog directory unavailable"); QFile file(files.filePath("hours.txt")); require(file.open(QIODevice::WriteOnly),"Cannot create catalog");
   file.write("ra_24\tdec mag bv\n12 0 4 1.5\n0 0 -1 0\n"); file.close();
   const auto styled=StarCatalog::read(file.fileName(),true),ordinary=StarCatalog::read(file.fileName());
   require(styled.error.isEmpty() && styled.stars.size()==2 && styled.rejectedLines==0 && styled.stars[0].magnitude==4 && std::abs(styled.stars[0].direction[0]+1)<1e-12 && styled.stars[0].colorIndex==1.5,"Styled catalog source order, hour header or B-V lost");
   require(ordinary.rejectedLines==1 && ordinary.stars[0].magnitude==-1 && ordinary.stars[1].direction[0]>.97,"Ordinary catalog order/degrees/header behavior changed");
   const auto red=orbitStarColor(1.5,1),blue=orbitStarColor(0,1); require(red.red()>red.blue()+70 && blue.blue()>blue.red()+30,"B-V colors do not distinguish red/blue stars");
}
std::shared_ptr<PlotModel> fixture() {
   auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit); model->axes=false; model->legend=false; model->grid=false; model->labels=false;
   model->scriptedCamera=true; model->perspective=true; model->fieldOfView=60; model->coordinates="Star style frame"; model->cameras.push_back({0,{0,0,20},{0,0,0},{0,1,0},false});
   auto &source=model->curves[0]; source.name="Body"; source.lines=false; source.showObject=false; source.radius=2; source.color=Qt::green;
   PlotPoint first; first.frame=1; first.epoch=21545; PlotPoint second=first; second.frame=2; second.epoch+=10./86400; second.connect=true; second.inertialToView={0,-1,0,1,0,0,0,0,1}; source.points={first,second}; model->frame=2;
   model->starCatalog.stars={{{0,.3,-1},-3,0},{{-.3,0,-1},6,0},{{.3,0,-1},-2,0},{{0,-.3,-1},2,1.5},{{0,0,1},0,0}};
   model->starOptions=PlotStarOptions{-2,6,6,14,.2}; model->starsEnabled=true; model->starCount=2; return model;
}
void appearanceChecks(PlotModel &model) {
   auto stars=orbitSkyStars(model); require(stars.size()==2 && stars[0].direction[0]==-.3 && stars[1].direction[0]==.3,"Magnitude filter must precede source-order star count");
   require(std::abs(stars[0].pixels-6)<1e-12 && std::abs(stars[1].pixels-14)<1e-12,"Styled size extrema incorrect");
   require(stars[0].color.blue()<60 && stars[1].color.blue()==255,"Brightness ratio floor incorrect");
   model.starOptions=PlotStarOptions{-2,6,1,10,.5}; stars=orbitSkyStars(model); require(stars[0].pixels==1 && stars[1].pixels==10,"Monitor sizes incorrect");
   model.starOptions=PlotStarOptions{-2,6,1,5,.1}; stars=orbitSkyStars(model); require(stars[0].pixels==1 && stars[1].pixels==5 && stars[0].color.blue()<30,"VR display preset sizes/dimming incorrect");
   model.starOptions=PlotStarOptions{6,6,4,8,.3}; stars=orbitSkyStars(model); require(stars.size()==1 && stars[0].pixels==4 && stars[0].color.blue()<80,"Equal magnitude interval handling incorrect");
   model.starOptions=PlotStarOptions{-2,6,6,14,.2};
}
}
int main(int argc,char **argv) {
   QApplication app(argc,argv); const bool native=argc==2 && QString::fromLocal8Bit(argv[1])=="--native";
   if (argc>2 || QGuiApplication::platformName()!=(native ? "xcb" : "offscreen") || (native && qEnvironmentVariable("LIBGL_ALWAYS_SOFTWARE")!="1")) return 2;
   try {
      catalogChecks(); auto model=fixture(); appearanceChecks(*model); const auto raw=frozen(*model);
      PlotCanvas canvas(model); canvas.resize(640,480); canvas.show(); canvas.setFrame(1); settle(app,150);
      if (native) {
         auto *renderer=dynamic_cast<OrbitRenderer *>(canvas.findChild<QOpenGLWidget *>()); require(renderer && renderer->isValid(),"Native star renderer unavailable");
         renderer->makeCurrent(); const auto gl=QString::fromLatin1(reinterpret_cast<const char *>(renderer->context()->functions()->glGetString(GL_RENDERER))); renderer->doneCurrent(); std::cout<<"Private renderer: "<<gl.toStdString()<<'\n';
      }
      auto capture=[&](quint64 frame=1) { canvas.setFrame(frame); settle(app); return canvas.captureImage(); };
      const auto styled=capture(); preserve("custom",styled,native);
      const auto dim=around(pixel(native,-.3,0)),bright=around(pixel(native,.3,0));
      require(illuminated(styled,dim)>4 && illuminated(styled,bright)>50 && brightness(styled,bright)>brightness(styled,dim)+100,"Analytic star locations, sizes or dimming absent");
      require(illuminated(styled,around(pixel(native,0,.3)))==0 && illuminated(styled,around(pixel(native,0,-.3)))==0,"Magnitude rejection or admitted-count limit ignored");
      model->starCount=1; const auto limited=capture(); require(illuminated(limited,dim)>4 && illuminated(limited,bright)==0,"Star count did not retain filtered source order"); model->starCount=2;
      model->starOptions->minDimRatio=.8; const auto dimmed=capture(); preserve("dimming-floor",dimmed,native); require(brightness(dimmed,dim)>brightness(styled,dim)+100,"Dimming floor did not affect rendered dim star"); model->starOptions->minDimRatio=.2;
      model->starOptions->maxPixels=8; const auto smaller=capture(); preserve("smaller",smaller,native); require(illuminated(smaller,bright)<illuminated(styled,bright)*.7,"Maximum pixel diameter did not affect rendered size"); model->starOptions->maxPixels=14;
      require(capture()==styled,"Restored star options did not restore pixels"); const auto rotated=capture(2); preserve("rotated-frame",rotated,native); require(rotated!=styled && capture()==styled,"Sky did not follow retained inertial frame/replay");
      canvas.restoreView({1,0,0,{100,-30},false,true,60,{}}); require(capture()==styled,"Stars moved with camera pan"); canvas.restoreView({1,0,0,{},false,true,60,{}});
      model->cameras[0].eye={100,-70,60}; model->cameras[0].target={100,-70,40}; require(capture()==styled,"Stars moved with camera translation"); model->cameras[0].eye={0,0,20}; model->cameras[0].target={0,0,0};
      canvas.zoomBy(3); require(capture()!=styled,"Perspective zoom did not change celestial framing"); canvas.restoreView({1,0,0,{},false,false,60,{}}); const auto orthographic=capture(); canvas.zoomBy(3); require(capture()==orthographic,"Orthographic zoom moved infinite-distance stars"); canvas.restoreView({1,0,0,{},false,true,60,{}});
      const auto catalog=model->starCatalog.stars; model->starCatalog.stars={{{0,0,-1},-2,0}}; model->curves[0].showObject=true; model->starsEnabled=false; const auto body=capture(); model->starsEnabled=true; require(capture()==body,"Star behind body leaked through disk"); model->curves[0].showObject=false; require(capture()!=body,"Unoccluded central star absent"); model->starCatalog.stars=catalog;
      model->printBackground=true; const auto printer=capture(); preserve("printer",printer,native); require(printer.pixelColor(5,80)==QColor(Qt::white),"Printer background is not white"); model->starsEnabled=false; require(capture()==printer,"Printer mode displayed stars");
      int dark=0; for (int y=445;y<475;++y) for (int x=50;x<590;++x) dark+=qGray(printer.pixel(x,y))<100; require(dark>20,"Printer HUD is unreadable"); model->printBackground=false;
      model->starOptions.reset(); const auto ordinaryBlank=capture(); model->starsEnabled=true; const auto ordinary=capture(); preserve("ordinary",ordinary,native); require(ordinary!=ordinaryBlank,"Ordinary Qt star display absent"); model->starOptions=PlotStarOptions{-2,6,6,14,.2}; require(capture()==styled,"Returning from ordinary stars lost style");
      model->starCount=0; const auto zero=capture(); model->starsEnabled=false; require(capture()==zero,"Zero count still displayed stars"); require(frozen(*model)==raw,"Sky rendering mutated catalog or numerical pose history");
      std::cout<<"PASS styled catalog/source order/B-V/RA header, magnitude filter before count, nonlinear pixel/dimming bounds, native/fallback analytic pixels, source-frame replay, translation/zoom behavior, body occlusion, Printer/HUD and frozen source. Private rendering only.\n";
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; } return 0;
}
