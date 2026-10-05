#include "OrbitRenderer.hpp"
#include "PlotModel.hpp"
#include <QApplication>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QTemporaryDir>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char *why) { if (!value) throw std::runtime_error(why); }
void near(double value,double expected,double tolerance,const char *why) {
   if (std::abs(value-expected)>tolerance) {
      std::cerr<<why<<": "<<value<<" expected "<<expected<<" tolerance "<<tolerance<<'\n';
      throw std::runtime_error(why);
   }
}
void settle(QApplication &app,int milliseconds=25) {
   app.processEvents(); QEventLoop loop;
   QTimer::singleShot(milliseconds,&loop,&QEventLoop::quit); loop.exec();
}
struct Pixels {
   int count=0,left=0,right=-1,top=0,bottom=-1;
   double x=0,y=0;
   int width() const { return count ? right-left+1 : 0; }
   int height() const { return count ? bottom-top+1 : 0; }
};
Pixels pixels(const QImage &image,int channel) {
   Pixels result; result.left=image.width(); result.top=image.height();
   for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) {
      const auto c=image.pixelColor(x,y);
      const bool selected=channel==0 ? c.green()>50 && c.green()>2*c.red() && c.green()>2*c.blue() :
         channel==1 ? c.red()>50 && c.red()>2*c.green() && c.red()>2*c.blue() :
         c.blue()>50 && c.blue()>2*c.red() && c.blue()>2*c.green();
      if (!selected) continue;
      ++result.count; result.x+=x; result.y+=y;
      result.left=std::min(result.left,x); result.right=std::max(result.right,x);
      result.top=std::min(result.top,y); result.bottom=std::max(result.bottom,y);
   }
   if (result.count) { result.x/=result.count; result.y/=result.count; }
   return result;
}
QJsonObject json(const Pixels &value) {
   return {{"count",value.count},{"left",value.left},{"right",value.right},{"top",value.top},
      {"bottom",value.bottom},{"width",value.width()},{"height",value.height()},{"centerX",value.x},{"centerY",value.y}};
}
QString hash(const QByteArray &bytes) {
   return QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());
}
QByteArray raw(const PlotModel &model) {
   QByteArray bytes; QDataStream stream(&bytes,QIODevice::WriteOnly);
   for (const auto &curve:model.curves) for (const auto &point:curve.points) {
      stream<<curve.name<<point.frame<<point.epoch<<point.x<<point.y<<point.z<<point.connect<<point.solver<<point.color;
      for (const auto value:point.bodyToView) stream<<value;
   }
   return bytes;
}
struct Evidence {
   QString folder=qEnvironmentVariable("GMAT_QT_MODEL_DRAWING_EVIDENCE");
   QJsonObject document={{"scope","private Xvfb software OpenGL only"},{"platform",QGuiApplication::platformName()},
      {"display",qEnvironmentVariable("DISPLAY")},{"softwareRequested",qEnvironmentVariable("LIBGL_ALWAYS_SOFTWARE")}};
   QJsonArray captures,assets;
   void prepare() {
      if (!folder.isEmpty()) require(QDir().mkpath(folder),"Cannot create model drawing evidence directory");
   }
   void write(const QString &status,const QString &error={}) {
      if (folder.isEmpty()) return;
      document.insert("status",status); document.insert("captures",captures); document.insert("assets",assets);
      if (!error.isEmpty()) document.insert("error",error);
      const auto bytes=QJsonDocument(document).toJson(QJsonDocument::Indented);
      QFile file(folder+"/native-model-drawing.json");
      require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Cannot preserve model drawing JSON");
   }
   void asset(const QString &name,const QByteArray &bytes) {
      assets.append(QJsonObject{{"name",name},{"sha256",hash(bytes)},{"bytes",bytes.size()}});
      if (!folder.isEmpty()) {
         QFile file(folder+"/"+name);
         require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Cannot preserve model drawing fixture");
      }
      write("RUNNING");
   }
   void capture(const QString &name,const QImage &image,QJsonObject expected={}) {
      require(!image.isNull(),"Native model framebuffer unavailable");
      captures.append(QJsonObject{{"name",name},{"imageWidth",image.width()},{"imageHeight",image.height()},
         {"green",json(pixels(image,0))},{"red",json(pixels(image,1))},{"blue",json(pixels(image,2))},{"expected",expected}});
      if (!folder.isEmpty()) require(image.save(folder+"/native-model-"+name+".png"),"Cannot preserve native model PNG");
      write("RUNNING");
   }
};
QString mesh(const QTemporaryDir &directory,const QString &name,double width,double height,const QByteArray &color,Evidence &evidence) {
   const auto material=name+".mtl",object=name+".obj";
   const QByteArray mtl="newmtl panel\nKa "+color+"\nKd "+color+"\nKs 0 0 0\nd 1\nillum 1\n";
   const auto number=[](double value) { return QByteArray::number(value,'g',17); };
   const QByteArray obj="mtllib "+material.toUtf8()+"\nv "+number(-width/2)+" "+number(-height/2)+" 0\n"+
      "v "+number(width/2)+" "+number(-height/2)+" 0\n"+
      "v "+number(width/2)+" "+number(height/2)+" 0\n"+
      "v "+number(-width/2)+" "+number(height/2)+" 0\n"+
      "vn 0 0 1\nusemtl panel\nf 1//1 2//1 3//1 4//1\n";
   for (const auto &item:{std::make_pair(material,mtl),std::make_pair(object,obj)}) {
      QFile file(directory.filePath(item.first));
      require(file.open(QIODevice::WriteOnly) && file.write(item.second)==item.second.size(),"Cannot write independent OBJ fixture");
      evidence.asset(item.first,item.second);
   }
   return directory.filePath(object);
}
std::shared_ptr<PlotModel> fixture(const QString &path) {
   auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
   model->grid=false; model->axes=false; model->labels=false; model->legend=false; model->sunlight=false;
   model->scriptedCamera=true; model->perspective=false;
   model->cameras.push_back({0,{0,0,200},{0,0,0},{0,1,0},false});
   auto &curve=model->curves[0]; curve.name="Probe"; curve.color=Qt::blue; curve.lines=false;
   curve.modelPath=path; curve.modelOverride=true;
   model->frame=1; model->append(0,0,0,0,21545);
   model->frame=2; model->append(0,12,-8,0,21545+10./86400);
   curve.points.back().bodyToView={0,-1,0,1,0,0,0,0,1};
   return model;
}
// An independent orthographic projection of the fixed 200-unit camera, whose
// 50-degree framing sets vertical half-span to 200*tan(25 degrees). No model,
// bounds, normalization or camera implementation helper derives expectations.
double pixelScale(const QImage &image) {
   return image.height()/(400*std::tan(25*3.14159265358979323846/180));
}
QJsonObject expected(const QImage &image,double width,double height,double x=0,double y=0) {
   const auto scale=pixelScale(image);
   return {{"worldWidth",width},{"worldHeight",height},{"worldX",x},{"worldY",y},
      {"pixelsPerUnit",scale},{"width",width*scale},{"height",height*scale},
      {"centerX",image.width()/2.+x*scale},{"centerY",image.height()/2.-y*scale}};
}
void dimensions(const QImage &image,int channel,double width,double height,const char *why,double x=0,double y=0) {
   const auto found=pixels(image,channel); const auto scale=pixelScale(image);
   require(found.count>100,why);
   near(found.width(),width*scale,3,why); near(found.height(),height*scale,3,why);
   near(found.x,image.width()/2.+x*scale,1.5,why); near(found.y,image.height()/2.-y*scale,1.5,why);
}
QImage capture(OrbitRenderer &viewer,QApplication &app,quint64 frame=1) {
   viewer.setView(1,0,0,{},frame); settle(app);
   return viewer.captureImage();
}
}
int main(int argc,char **argv) {
   QApplication app(argc,argv);
   if (argc!=2 || QString::fromLocal8Bit(argv[1])!="--native" || QGuiApplication::platformName()!="xcb" || qEnvironmentVariable("LIBGL_ALWAYS_SOFTWARE")!="1") return 2;
   Evidence evidence;
   try {
      evidence.prepare(); QTemporaryDir files; require(files.isValid(),"Model fixture directory unavailable");
      const auto wide=mesh(files,"wide-panel",40,20,"0 1 0",evidence);
      const auto tall=mesh(files,"tall-panel",12,30,"1 0 0",evidence);
      auto model=fixture(wide); const auto original=raw(*model);
      auto &curve=model->curves[0]; curve.radius=17; curve.modelScale=11;
      curve.modelOffset={65,23,-7}; curve.modelRotation={33,12,75};
      OrbitRenderer viewer(model); viewer.resize(720,520); viewer.show(); viewer.setView(1,0,0,{},1); settle(app,150);
      require(viewer.isValid(),"Private software native renderer unavailable");
      viewer.makeCurrent();
      const auto glString=[&](GLenum name) {
         const auto *value=viewer.context()->functions()->glGetString(name);
         return value ? QString::fromLatin1(reinterpret_cast<const char *>(value)) : QString();
      };
      const auto renderer=glString(GL_RENDERER),vendor=glString(GL_VENDOR),version=glString(GL_VERSION);
      viewer.doneCurrent(); require(!renderer.isEmpty(),"Software OpenGL identity unavailable");
      evidence.document.insert("glRenderer",renderer);
      evidence.document.insert("glVendor",vendor); evidence.document.insert("glVersion",version);
      const auto identity=capture(viewer,app); evidence.capture("physical-identity",identity,expected(identity,40,20));
      dimensions(identity,0,40,20,"Override did not retain raw dimensions and identity resource transform");
      const auto pose=capture(viewer,app,2); evidence.capture("recorded-pose",pose,expected(pose,20,40,12,-8));
      dimensions(pose,0,20,40,"Override lost recorded object position or attitude",12,-8);
      const auto restore=capture(viewer,app); evidence.capture("recorded-restore",restore,expected(restore,40,20));
      require(restore==identity,"Recorded model replay did not restore its original pixels");

      // Holding the exact filename constant while changing only the override
      // flag must rebuild normalized versus raw geometry in the asset cache.
      curve.radius=0; curve.modelScale=.1; curve.modelOffset={0,0,0}; curve.modelRotation={0,0,0}; curve.modelOverride=false;
      const double normalizedWidth=40/std::sqrt(20*20+10*10)*100;
      const double normalizedHeight=20/std::sqrt(20*20+10*10)*100;
      const auto inherited=capture(viewer,app);
      evidence.capture("inherited-same-file",inherited,expected(inherited,normalizedWidth,normalizedHeight));
      dimensions(inherited,0,normalizedWidth,normalizedHeight,"Inherited resource model normalization changed");
      curve.modelOverride=true;
      const auto overridden=capture(viewer,app); evidence.capture("override-same-file",overridden,expected(overridden,40,20));
      dimensions(overridden,0,40,20,"Same-file override reused normalized resource geometry");
      curve.modelOverride=false;
      const auto reset=capture(viewer,app); evidence.capture("reset-same-file",reset,expected(reset,normalizedWidth,normalizedHeight));
      require(reset==inherited,"Use-resource reset did not restore same-file inherited model pixels");
      curve.modelOverride=true;

      // Equal object names in different plot models may select separate assets.
      auto other=fixture(tall); const auto otherOriginal=raw(*other);
      OrbitRenderer otherViewer(other); otherViewer.resize(720,520); otherViewer.show(); otherViewer.setView(1,0,0,{},1); settle(app,150);
      require(otherViewer.isValid(),"Second private native renderer unavailable");
      const auto otherBefore=capture(otherViewer,app);
      evidence.capture("second-plot-tall",otherBefore,expected(otherBefore,12,30));
      dimensions(otherBefore,1,12,30,"Second plot did not select its independent model file");
      curve.modelOverride=false;
      const auto independentReset=capture(viewer,app);
      evidence.capture("first-plot-reset",independentReset,expected(independentReset,normalizedWidth,normalizedHeight));
      dimensions(independentReset,0,normalizedWidth,normalizedHeight,"First plot could not reset independently");
      const auto otherAfter=capture(otherViewer,app); evidence.capture("second-plot-unchanged",otherAfter,expected(otherAfter,12,30));
      require(otherAfter==otherBefore,"Changing one plot changed another plot's model");
      curve.modelOverride=true; curve.showObject=false;
      const auto hidden=capture(viewer,app); evidence.capture("draw-object-off",hidden);
      require(pixels(hidden,0).count==0 && pixels(hidden,1).count==0 && pixels(hidden,2).count==0,
         "DrawObject Off retained model or fallback pixels");
      curve.showObject=true;
      const auto shown=capture(viewer,app); evidence.capture("draw-object-on",shown,expected(shown,40,20));
      dimensions(shown,0,40,20,"DrawObject On did not restore its per-plot model");

      curve.modelPath=files.filePath("absent.obj"); curve.radius=4;
      const auto bodyFallback=capture(viewer,app); evidence.capture("missing-body-fallback",bodyFallback,expected(bodyFallback,8,8));
      dimensions(bodyFallback,2,8,8,"Missing body override did not show the physical-radius fallback");
      require(pixels(bodyFallback,0).count==0,"Missing override retained its old mesh");
      curve.radius=0;
      const auto markerFallback=capture(viewer,app); evidence.capture("missing-marker-fallback",markerFallback);
      const auto marker=pixels(markerFallback,2);
      require(marker.count>=25 && marker.width()>=5 && marker.width()<=9 && marker.height()>=5 && marker.height()<=9,
         "Missing spacecraft override did not show its default marker");
      near(marker.x,markerFallback.width()/2.,1.5,"Missing-model marker moved from its recorded position");
      near(marker.y,markerFallback.height()/2.,1.5,"Missing-model marker moved from its recorded position");
      require(pixels(markerFallback,0).count==0,"Missing spacecraft override retained its old mesh");
      require(raw(*model)==original && raw(*other)==otherOriginal,"Model drawing changed retained numerical samples");
      evidence.document.insert("rawBeforeSha256",hash(original)); evidence.document.insert("rawAfterSha256",hash(raw(*model)));
      evidence.document.insert("otherRawBeforeSha256",hash(otherOriginal)); evidence.document.insert("otherRawAfterSha256",hash(raw(*other)));
      evidence.write("PASS");
      std::cout<<"PASS: private software native model pixels: raw dimensions/identity, recorded pose/replay, same-file inheritance/override/reset cache, independent plots, DrawObject and missing body/marker fallback; retained samples unchanged. Renderer "<<renderer.toStdString()<<'\n';
      return 0;
   } catch (const std::exception &error) {
      try { evidence.write("FAIL",QString::fromUtf8(error.what())); } catch (const std::exception &) {}
      std::cerr<<error.what()<<'\n'; return 1;
   }
}
