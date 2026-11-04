#include "PlotWidget.hpp"
#include "OrbitRenderer.hpp"
#include <QApplication>
#include <QDataStream>
#include <QDir>
#include <QEventLoop>
#include <QIODevice>
#include <QTimer>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *why) { if (!value) throw std::runtime_error(why); }
struct Pixels { int count=0,left=0,right=-1,top=0,bottom=-1; double x=0,y=0; };
static Pixels pixels(const QImage &image,int channel)
{
   Pixels result; result.left=image.width(); result.top=image.height();
   for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) {
      const auto c=image.pixelColor(x,y);
      const bool selected=channel==0 ? c.green()>50 && c.green()>2*c.red() && c.green()>2*c.blue() :
         channel==1 ? c.red()>50 && c.red()>2*c.green() && c.red()>2*c.blue() :
         c.blue()>70 && c.blue()>2*c.red() && c.blue()>2*c.green();
      if (!selected) continue;
      ++result.count; result.x+=x; result.y+=y;
      result.left=std::min(result.left,x); result.right=std::max(result.right,x);
      result.top=std::min(result.top,y); result.bottom=std::max(result.bottom,y);
   }
   if (result.count) { result.x/=result.count; result.y/=result.count; }
   return result;
}
static void near(double value,double expected,double tolerance,const char *why)
{
   if (std::abs(value-expected)>tolerance) { std::cerr<<why<<": "<<value<<" expected "<<expected<<" tolerance "<<tolerance<<'\n'; throw std::runtime_error(why); }
}
static QByteArray raw(const PlotModel &model)
{
   QByteArray bytes; QDataStream stream(&bytes,QIODevice::WriteOnly);
   for (const auto &curve:model.curves) for (const auto &point:curve.points)
      stream<<point.frame<<point.epoch<<point.x<<point.y<<point.z<<point.connect<<point.solver<<point.regularArc;
   return bytes;
}
static void addArc(PlotModel &model,int index,quint64 id,const QString &name,
      quint64 frame,double begin,double end,double x0,double x1,double z)
{
   PlotRegularArc arc; arc.id=id; arc.name=name; arc.members.insert(id); arc.finalized=true;
   model.curves[index].breakNext=true;
   model.frame=frame; model.append(index,x0,0,z,begin); auto &a=model.curves[index].points.back(); a.regularArc=id; a.provider=name; arc.first=a;
   model.frame=frame+1; model.append(index,x1,0,z,end); auto &b=model.curves[index].points.back(); b.regularArc=id; b.provider=name; arc.last=b;
   model.regularArcs[model.curves[index].name].append(arc);
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   const bool native=argc==2 && QString::fromLocal8Bit(argv[1])=="--native";
   if (QGuiApplication::platformName()!=(native ? "xcb" : "offscreen")) return 2;
   if (native && qEnvironmentVariable("LIBGL_ALWAYS_SOFTWARE")!="1") return 2;
   try {
      auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
      model->grid=false; model->axes=false; model->labels=false; model->legend=false; model->sunlight=false;
      model->scriptedCamera=true; model->perspective=false;
      auto &probe=model->curves[0]; probe.name="Probe"; probe.color=Qt::green; probe.radius=3; probe.lines=false;
      auto &other=model->curves[1]; other.name="Other"; other.color=Qt::red; other.radius=3; other.lines=false;
      addArc(*model,0,1,"First",1,100,120,-40,40,-20);
      addArc(*model,0,2,"Reset",3,105,130,-70,-20,-20);
      addArc(*model,1,3,"Unequal",5,95,135,-30,50,20);
      for (const auto &curve:model->curves) for (const auto &point:curve.points) {
         PlotCamera camera; camera.frame=point.frame; camera.epoch=point.epoch;
         camera.eye={0,-300,0}; camera.target={0,0,0}; camera.up={0,0,1}; model->cameras.push_back(camera);
      }
      const auto original=raw(*model);
      PlotCanvas canvas(model);
      // These masks measure scene geometry. LCD text antialiasing can put
      // strongly red fringe pixels in the neutral footer, moving the red
      // object's measured centroid by several pixels on offscreen platforms.
      // Use grayscale text antialiasing without changing geometry or bounds.
      auto font=canvas.font(); font.setStyleStrategy(QFont::NoSubpixelAntialias); canvas.setFont(font);
      canvas.resize(720,520); canvas.show(); canvas.scriptView(); app.processEvents();
      if (native) {
         QEventLoop exposed; QTimer::singleShot(150,&exposed,&QEventLoop::quit); exposed.exec();
         auto *renderer=dynamic_cast<OrbitRenderer *>(canvas.findChild<QOpenGLWidget *>());
         require(renderer && renderer->isValid(),"Private software native renderer unavailable");
      }
      const auto evidence=qEnvironmentVariable("GMAT_QT_PLAYBACK_OPTIONS_EVIDENCE");
      const QString prefix=native ? "native-epoch-" : "fallback-epoch-";
      if (!evidence.isEmpty()) require(QDir().mkpath(evidence),"Cannot create playback evidence directory");
      auto capture=[&](double epoch,const QString &name) {
         canvas.setSimulationEpoch(epoch,PlotReplayMode::InterpolatedSteps); app.processEvents();
         auto image=canvas.captureImage(); require(!image.isNull(),"Absolute epoch framebuffer unavailable");
         if (!evidence.isEmpty()) require(image.save(evidence+"/"+prefix+name+".png"),"Epoch screenshot save failed");
         return image;
      };
      const auto first=capture(100,"100"),mid=capture(110,"110"),last=capture(120,"120");
      const auto a0=pixels(first,0),a1=pixels(mid,0),a2=pixels(last,0);
      const auto b0=pixels(first,1),b1=pixels(mid,1),b2=pixels(last,1);
      for (const auto &image:{first,mid,last}) {
         const auto footer=image.copy(0,image.height()-60,image.width(),60);
         require(pixels(footer,0).count==0 && pixels(footer,1).count==0 && pixels(footer,2).count==0,
            "Neutral footer contaminated scene color masks");
      }
      for (const auto &group:{a0,a1,a2,b0,b1,b2}) require(group.count>25,"Absolute-epoch object missing from rendered pixels");
      const double scale=(a2.x-a0.x)/80.; require(scale>.5,"Recorded object movement too small to measure");
      // These coordinates follow independently specified linear fixtures; no
      // pose/camera helper is used to derive pixel expectations.
      near(a1.x,(a0.x+a2.x)/2,1.5,"First object did not advance uniformly in absolute time");
      near(b1.x,(b0.x+b2.x)/2,1.5,"Unequal object interval did not advance uniformly");
      near(a1.x,b1.x,1.5,"Objects at the same physical X used independent percentages instead of one epoch");
      near(a1.x-a0.x,2*(b1.x-b0.x),2.5,"Unequal interval movement used a shared frame fraction");
      const auto outside=capture(125,"125-next-arc"),overlap=capture(110,"110-sticky-arc");
      const auto next=pixels(outside,0),sticky=pixels(overlap,0);
      require(next.count>25 && sticky.count>25,"Reset trajectory object disappeared");
      near(next.x,a1.x-30*scale,2,"Next containing trajectory used the wrong pose");
      near(sticky.x,a1.x-60*scale,2,"Overlapping trajectory selection was not sticky in pixels");
      // Return to an unambiguous first arc, then expose its progressive path
      // and a body-fixed vector at the independently resolved moving tip.
      canvas.setSimulationEpoch(std::nullopt,PlotReplayMode::InterpolatedSteps);
      probe.lines=true; probe.showObject=false;
      PlotVector arrow; arrow.name="Tip axis"; arrow.source="Probe"; arrow.bodyFixed=true;
      arrow.automaticLength=false; arrow.length=12; arrow.direction={0,0,1}; arrow.color=Qt::blue;
      for (const auto &point:probe.points) { PlotVectorSample sample; sample.source=point; arrow.samples.push_back(sample); }
      model->vectors.append(arrow);
      const auto path=capture(110,"path-and-vector"); const auto pathPixels=pixels(path,0),vectorPixels=pixels(path,2);
      require(pathPixels.count>30 && vectorPixels.count>10,"Epoch path or vector missing from pixels");
      near(pathPixels.left,a0.x,2.5,"Epoch path lost its retained start");
      near(pathPixels.right,a1.x,2.5,"Epoch path endpoint differs from resolved object position");
      near(vectorPixels.x,a1.x,3,"Epoch vector did not follow the visible path tip");
      require(vectorPixels.top<a1.y-5,"Epoch body-fixed vector had the wrong direction");
      // Frozen camera geometry follows the same selected reference. A camera
      // attached to Probe must keep it stationary on screen while Other moves.
      probe.lines=false; probe.showObject=true; model->vectors.clear();
      for (auto &camera:model->cameras) {
         camera.reference="Probe"; camera.targetReference="Probe";
         PlotCameraGeometry geometry; geometry.reference="Probe"; geometry.target="Probe";
         geometry.position={0,-300,0}; geometry.up={0,0,1}; geometry.centerOnReference=true; camera.geometry=geometry;
      }
      canvas.setSimulationEpoch(std::nullopt,PlotReplayMode::InterpolatedSteps);
      const auto tracking1=capture(110,"tracking-110"),tracking2=capture(120,"tracking-120");
      const auto trackA=pixels(tracking1,0),trackB=pixels(tracking2,0);
      require(trackA.count>25 && trackB.count>25,"Tracked epoch object missing");
      near(trackA.x,trackB.x,1.5,"Frozen camera did not follow the selected epoch reference");
      require(std::abs(pixels(tracking2,1).x-pixels(tracking1,1).x)>5,"Tracked camera did not move relative to independently propagated object");
      require(raw(*model)==original,"Absolute-epoch painting changed retained numerical samples");
      std::cout<<"PASS: "<<(native ? "private software X11 native" : "offscreen fallback")
         <<" absolute-epoch pixels: unequal intervals, uniform motion, sticky reset arcs, per-object path tip/vector and tracked camera; raw samples unchanged.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
