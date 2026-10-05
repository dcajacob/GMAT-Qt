#include "OrbitRenderer.hpp"
#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
void settle(QApplication &app,int milliseconds=25)
{
   app.processEvents(); QEventLoop loop; QTimer::singleShot(milliseconds,&loop,&QEventLoop::quit); loop.exec();
}
void preserve(const QString &name,const QImage &image)
{
   const auto path=qEnvironmentVariable("GMAT_QT_NATIVE_VECTOR_EVIDENCE"); if (path.isEmpty()) return;
   require(QDir().mkpath(path) && image.save(path+'/'+name+".png"),"Cannot preserve native vector capture");
}
QByteArray rawHistory(const PlotModel &model)
{
   std::ostringstream result; result<<std::hexfloat;
   auto point=[&](const PlotPoint &p) {
      result<<p.frame<<' '<<p.epoch<<' '<<p.x<<' '<<p.y<<' '<<p.z<<' '<<p.connect<<' '<<p.solver<<' '<<p.color.rgba()<<' ';
      for (const auto x:p.bodyToView) result<<x<<' '; for (const auto x:p.inertialToView) result<<x<<' ';
      for (const auto x:p.viewVelocity) result<<x<<' '; result<<p.hasVelocity<<' '<<p.regularArc<<' '<<p.provider.toStdString()<<'\n';
   };
   for (const auto &curve:model.curves) { result<<curve.name.toStdString()<<'\n'; for (const auto &p:curve.points) point(p); }
   for (const auto &vector:model.vectors) {
      result<<vector.name.toStdString()<<'\n';
      for (const auto &sample:vector.samples) { point(sample.source); for (const auto x:sample.destination) result<<x<<' '; result<<sample.sourceRadius<<' '<<sample.valid<<'\n'; }
   }
   for (const auto &camera:model.cameras) { result<<camera.frame<<' '; for (const auto x:camera.eye) result<<x<<' '; for (const auto x:camera.target) result<<x<<' '; for (const auto x:camera.up) result<<x<<' '; result<<camera.solver<<'\n'; }
   return QByteArray::fromStdString(result.str());
}
// Independent pinhole projection: eye=(0,0,20), target=(0,0,0), up=+Y,
// vertical field of view=60 degrees. All shaft and in-plane head probes have z=0.
QPointF pixel(const QImage &image,double x,double y)
{
   const double scale=image.height()*std::sqrt(3.0)/40.0;
   return {image.width()/2.0+x*scale,image.height()/2.0-y*scale};
}
bool tint(QColor actual,QColor expected)
{
   return std::abs(actual.red()-expected.red())<32 && std::abs(actual.green()-expected.green())<32 && std::abs(actual.blue()-expected.blue())<32;
}
int colored(const QImage &image,QColor color,QRect area={})
{
   if (area.isNull()) area=image.rect(); area=area.intersected(image.rect()); int count=0;
   for (int y=area.top();y<=area.bottom();++y) for (int x=area.left();x<=area.right();++x) count+=tint(image.pixelColor(x,y),color);
   return count;
}
int around(const QImage &image,QColor color,QPointF center,int radius=2)
{
   return colored(image,color,QRect(qRound(center.x())-radius,qRound(center.y())-radius,2*radius+1,2*radius+1));
}
int different(const QImage &a,const QImage &b,QRect area={})
{
   require(!a.isNull() && a.size()==b.size(),"Incompatible native captures"); if (area.isNull()) area=a.rect(); area=area.intersected(a.rect()); int count=0;
   for (int y=area.top();y<=area.bottom();++y) for (int x=area.left();x<=area.right();++x) count+=a.pixel(x,y)!=b.pixel(x,y);
   return count;
}
struct Expected { double x,y,dx,dy,length; };
void arrowPixels(const QImage &image,const QImage &blank,QColor color,const Expected &arrow,const QString &name)
{
   const double ex=arrow.x+arrow.dx*arrow.length,ey=arrow.y+arrow.dy*arrow.length;
   const int total=colored(image,color);
   require(total>20 && different(image,blank)>20,"Native arrow has no independently colored pixels");
   for (const double fraction:{.2,.5,.7}) require(around(image,color,pixel(image,arrow.x+arrow.dx*arrow.length*fraction,arrow.y+arrow.dy*arrow.length*fraction))>0,"Native shaft missing at analytic interior probe");
   require(around(image,color,pixel(image,arrow.x,arrow.y))>0 && around(image,color,pixel(image,ex,ey))>0,"Native shaft endpoint misplaced");
   // The two in-plane head tips lie behind the endpoint and off the shaft.
   // Probe their pixels independently of the shaft and of OrbitVectors.hpp.
   const double transverseX=-arrow.dy,transverseY=arrow.dx;
   for (const int sign:{-1,1}) {
      const auto tip=pixel(image,ex-arrow.dx*arrow.length*.2+sign*transverseX*arrow.length*.07,ey-arrow.dy*arrow.length*.2+sign*transverseY*arrow.length*.07);
      require(around(image,color,tip,1)>0,"Native arrowhead wing missing at analytic tip");
   }
   const auto before=pixel(image,arrow.x-arrow.dx*.6,arrow.y-arrow.dy*.6),beyond=pixel(image,ex+arrow.dx*.6,ey+arrow.dy*.6);
   require(around(image,color,before,2)==0 && around(image,color,beyond,2)==0,"Native arrow exceeds analytic Auto/Manual endpoints");
   std::cout<<name.toStdString()<<": color pixels="<<total<<", blank differences="<<different(image,blank)<<'\n';
}
PlotPoint pose(quint64 frame,double x,double y,bool rotated=false)
{
   PlotPoint p; p.frame=frame; p.epoch=21545+double(frame)/86400; p.x=x; p.y=y; p.color=Qt::green;
   if (rotated) p.bodyToView={0,-1,0,1,0,0,0,0,1}; return p;
}
std::shared_ptr<PlotModel> fixture()
{
   auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
   model->axes=false; model->grid=false; model->legend=false; model->labels=false;
   model->coordinates="Analytic vector frame"; model->perspective=true; model->fieldOfView=60; model->scriptedCamera=true;
   model->cameras.push_back({0,{0,0,20},{0,0,0},{0,1,0},false}); model->frame=3;
   auto &relative=model->curves[0]; relative.name="RelativeSource"; relative.radius=2; relative.lines=false; relative.showObject=false;
   relative.points={pose(1,-6,-4),pose(2,-6,-4,true),pose(3,-6,-4,true)};
   auto &body=model->curves[1]; body.name="BodySource"; body.radius=2; body.lines=false; body.showObject=false;
   body.points={pose(1,-6,4),pose(2,4,-5,true),pose(3,4,-5,true)};
   PlotVector r; r.name="Relative"; r.source=relative.name; r.destination="Destination"; r.color=QColor(240,80,80); r.automaticLength=false; r.length=5;
   r.samples.push_back({relative.points[0],{6,-4,0},2,true}); r.samples.push_back({relative.points[1],{-6,8,0},2,true}); r.samples.push_back({relative.points[2],{-6,8,0},2,false});
   PlotVector b; b.name="Body"; b.source=body.name; b.bodyFixed=true; b.start={.5,0,0}; b.direction={1,0,0}; b.color=QColor(70,190,250); b.automaticLength=false; b.length=5;
   b.samples.push_back({body.points[0],{},2,true}); b.samples.push_back({body.points[1],{},2,true}); b.samples.push_back({body.points[2],{},2,false});
   model->vectors={r,b}; return model;
}
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   if (QGuiApplication::platformName()!="xcb" || qEnvironmentVariable("LIBGL_ALWAYS_SOFTWARE")!="1") return 2;
   try {
      auto model=fixture(); const auto definitions=model->vectors; model->vectors.clear();
      OrbitRenderer viewer(model); viewer.resize(640,480); viewer.show(); viewer.setView(1,0,0,{},1); settle(app,150);
      require(viewer.isValid() && viewer.context() && viewer.context()->isValid(),"Actual OrbitRenderer OpenGL context unavailable");
      viewer.makeCurrent(); const auto renderer=QString::fromLatin1(reinterpret_cast<const char *>(viewer.context()->functions()->glGetString(GL_RENDERER))); viewer.doneCurrent();
      require(!renderer.isEmpty(),"Cannot identify actual GL renderer"); std::cout<<"Private xcb OpenGL renderer: "<<renderer.toStdString()<<'\n';
      auto capture=[&](quint64 frame) { viewer.setView(1,0,0,{},frame); settle(app); const auto image=viewer.captureImage(); require(!image.isNull(),"Native framebuffer unavailable"); return image; };
      const auto blank=capture(1); preserve("blank",blank);
      auto install=[&](int index,bool automatic) { model->vectors={definitions[index]}; model->vectors[0].automaticLength=automatic; model->labels=false; };
      for (int type=0;type<2;++type) for (const bool automatic:{false,true}) {
         install(type,automatic); const auto raw=rawHistory(*model); const double length=automatic ? 2 : 5;
         const auto first=capture(1); const auto name=QString(type==0 ? "relative" : "body")+(automatic ? "-auto" : "-manual"); preserve(name+"-frame1",first);
         arrowPixels(first,blank,definitions[type].color,type==0 ? Expected{-4,-4,1,0,length} : Expected{-5.5,4,1,0,length},name);
         const auto second=capture(2); preserve(name+"-frame2",second);
         arrowPixels(second,blank,definitions[type].color,type==0 ? Expected{-6,-2,0,1,length} : Expected{4,-4.5,0,1,length},name+" replay");
         require(different(first,second)>40,"Recorded vector position/orientation did not change native pixels");
         require(capture(1)==first,"Returning to recorded frame did not restore vector pixels");
         const auto invalid=capture(3); preserve(name+"-invalid",invalid);
         require(invalid==blank && rawHistory(*model)==raw,"Invalid latest vector was shown stale, or rendering mutated retained history");
      }
      // Independent length/color comparison on one fixed pose.
      install(0,true); const auto autoImage=capture(1); model->vectors[0].automaticLength=false; const auto manual=capture(1);
      require(different(autoImage,manual)>30,"Manual length did not change native shaft/head pixels");
      model->vectors[0].color=QColor(240,215,65); const auto yellow=capture(1); preserve("relative-recolored",yellow);
      arrowPixels(yellow,blank,model->vectors[0].color,{-4,-4,1,0,5},"relative recolored"); require(colored(yellow,definitions[0].color)==0,"Native vector retained the previous color");
      for (int type=0;type<2;++type) {
         install(type,false); const auto raw=rawHistory(*model); const auto without=capture(1); model->vectors[0].label=type==0 ? "Relative position" : "Body direction"; model->labels=true;
         const auto text=capture(1); preserve(type==0 ? "relative-label" : "body-label",text);
         const auto end=pixel(text,type==0 ? 1 : -.5,type==0 ? -4 : 4); const double scale=text.devicePixelRatio();
         const QRect region(qFloor(end.x()+3*scale),qFloor(end.y()-26*scale),qCeil(140*scale),qCeil(26*scale));
         require(different(text,without,region)>30 && colored(text,definitions[type].color,region)>10,"Native vector color/label text absent at analytic endpoint");
         model->labels=false; require(capture(1)==without && rawHistory(*model)==raw,"Label master did not hide native vector text or changed retained samples");
         const auto stored=model->vectors; model->vectors.clear(); require(capture(1)==blank,"Removed vector left native arrow pixels"); model->vectors=stored;
         require(rawHistory(*model)==raw,"Vector removal/reinstallation changed retained data");
      }
      // Both independent arrows are visible together, then each removes cleanly.
      model->vectors=definitions; const auto raw=rawHistory(*model); const auto both=capture(2); preserve("both-frame2",both);
      arrowPixels(both,blank,definitions[0].color,{-6,-2,0,1,5},"combined relative"); arrowPixels(both,blank,definitions[1].color,{4,-4.5,0,1,5},"combined body");
      require(rawHistory(*model)==raw,"Combined rendering mutated retained numerical/vector/camera samples");
      viewer.close(); std::cout<<"PASS native Relative Position/Body-Fixed shafts and heads, Auto/Manual analytic lengths, color/labels, recorded replay, invalid hiding/removal and unchanged retained data. Private Xvfb/software GL only; no host/GPU qualification.\n";
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
