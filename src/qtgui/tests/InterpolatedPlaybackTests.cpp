#include "PlotWidget.hpp"
#include "PlotInterpolation.hpp"
#include "OrbitCamera.hpp"
#include "OrbitObjectAxes.hpp"
#include "OrbitVectors.hpp"
#include "OrbitRenderer.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QTemporaryDir>
#include <QFile>
#include <QSlider>
#include <QTimer>
#include <QEventLoop>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *why) { if (!value) throw std::runtime_error(why); }
static void near(double value,double expected,const char *why) { require(std::abs(value-expected)<1e-6,why); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Export read failed"); return file.readAll(); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   const bool native=argc==2 && QString::fromLocal8Bit(argv[1])=="--native";
   if (QGuiApplication::platformName()!=(native ? "xcb" : "offscreen")) return 2;
   try {
      auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
      model->curves[0].name="Probe"; model->curves[0].centerMarker=true;
      model->curves[0].color=Qt::green; model->curves[0].radius=3;
      model->scriptedCamera=true;
      PlotVector vector; vector.name="Body axis"; vector.source="Probe"; vector.bodyFixed=true;
      for (const auto &sample:QVector<QPair<quint64,double>>{{1,0},{2,1},{500,10},{501,100}}) {
         model->frame=sample.first; model->append(0,sample.second,0,0,21545+sample.second/86400);
         auto &point=model->curves[0].points.back(); point.hasVelocity=true; point.viewVelocity={1,0,0};
         point.bodyToView={0,-1,0,1,0,0,0,0,1};
         model->cameras.push_back({sample.first,{sample.second,-100,0},{sample.second,0,0},{0,0,1},false});
         PlotVectorSample v; v.source=point; vector.samples.push_back(v);
      }
      model->vectors.append(vector); model->endOfRun=true;
      PlotWidget widget(model); widget.resize(700,500); widget.show(); app.processEvents();
      auto *mode=widget.findChild<QComboBox *>("plotReplayMode");
      auto *speed=widget.findChild<QComboBox *>("plotReplaySpeed");
      auto *slower=widget.findChild<QAction *>("plotReplaySlower"),*faster=widget.findChild<QAction *>("plotReplayFaster");
      require(mode && speed && slower && faster && mode->currentIndex()==1,"Local mode/speed controls missing");
      QTemporaryDir files; require(files.isValid(),"Private export folder unavailable");
      const auto rawPoints=model->curves[0].points;
      auto xy=std::make_shared<PlotModel>(*model); xy->kind=PlotModel::Kind::XY;
      PlotWidget exportPlot(xy);
      const auto path=files.filePath("raw.txt"); require(exportPlot.exportData(path).isEmpty(),"Raw export failed"); const auto original=read(path);
      widget.setSharedReplayPosition(500);
      QImage interpolatedImage;
      if (native) {
         QEventLoop exposed; QTimer::singleShot(150,&exposed,&QEventLoop::quit); exposed.exec();
         auto *renderer=dynamic_cast<OrbitRenderer *>(widget.findChild<QOpenGLWidget *>());
         require(renderer && renderer->isValid(),"Private native OpenGL renderer unavailable");
         interpolatedImage=widget.canvas()->captureImage();
         require(!interpolatedImage.isNull(),"Interpolated framebuffer unavailable");
         int green=0;
         for (int y=0;y<interpolatedImage.height();++y) for (int x=0;x<interpolatedImage.width();++x) {
            const auto pixel=interpolatedImage.pixelColor(x,y);
            if (pixel.green()>40 && pixel.green()>pixel.red()*2 && pixel.green()>pixel.blue()*2) ++green;
         }
         require(green>100,"Interpolated object missing from native framebuffer");
      }
      require(model->replayCursor && model->curves[0].replayPose,"Adaptive steps were not interpolated");
      const auto *pose=orbitObjectPose(model->curves[0],widget.canvas()->frame()); require(pose,"Interpolated pose missing");
      near(pose->x,50,"Midpoint did not represent half of simulation time");
      near((pose->epoch-21545)*86400,50,"Presentation epoch did not advance uniformly");
      const auto camera=orbitCamera(*model,widget.canvas()->frame(),0,0,100);
      near(camera.target.x(),50,"Tracked camera did not follow interpolated object");
      const auto *v=orbitVectorSample(model->vectors[0],widget.canvas()->frame()); require(v,"Interpolated vector missing");
      near(v->source.x,50,"Vector did not follow interpolated object");
      const auto markers=model->orbitMarkers(model->curves[0],widget.canvas()->frame()); require(!markers.isEmpty(),"Center marker missing");
      near(markers.back().point->x,50,"Center marker lagged behind interpolated pose");
      int displayed=0; for (const auto &point:plotDisplayedPoints(model->curves[0],widget.canvas()->frame())) { ++displayed; if (displayed==4) near(point.x,50,"Rendered trajectory tip not interpolated"); }
      require(displayed==4,"Virtual path tip missing or future data leaked");
      for (int percent=100;percent<1000;percent+=100) {
         widget.setSharedReplayPosition(percent); const auto *p=orbitObjectPose(model->curves[0],widget.canvas()->frame());
         near(p->x,percent/10.,"Uniform timeline moved at adaptive-step speed");
      }
      widget.setSharedReplayMode(PlotReplayMode::PropagationSteps); widget.setSharedReplayPosition(500);
      require(!model->replayCursor && !model->curves[0].replayPose && widget.canvas()->frame()==2,"Propagation mode resampled retained steps");
      near(orbitObjectPose(model->curves[0],2)->x,1,"Propagation mode changed raw sample");
      if (native) {
         const auto propagationImage=widget.canvas()->captureImage();
         require(!propagationImage.isNull() && propagationImage!=interpolatedImage,"Native renderer ignored interpolated trajectory/camera");
         const auto capture=qEnvironmentVariable("GMAT_QT_PLAYBACK_CAPTURE_DIR");
         if (!capture.isEmpty()) require(interpolatedImage.save(capture+"/interpolated.png") &&
            propagationImage.save(capture+"/propagation.png"),"Native playback captures failed");
      }
      faster->trigger(); require(speed->currentIndex()==3,"Local Faster did not increase speed");
      slower->trigger(); require(speed->currentIndex()==2,"Local Slower did not decrease speed");
      speed->setCurrentIndex(0); require(!slower->isEnabled(),"Local minimum speed remained enabled");
      speed->setCurrentIndex(4); require(!faster->isEnabled(),"Local maximum speed remained enabled");
      widget.setSharedReplayMode(PlotReplayMode::InterpolatedSteps); widget.setSharedReplayPosition(1000);
      require(!model->replayCursor && !model->curves[0].replayPose,"Latest retained interpolated state");
      exportPlot.setSharedReplayPosition(500);
      require(!xy->replayCursor && exportPlot.exportData(path).isEmpty() && read(path)==original,"Presentation interpolation changed XY raw export");
      for (size_t i=0;i<rawPoints.size();++i) {
         const auto &a=rawPoints[i],&b=model->curves[0].points[i];
         require(a.frame==b.frame && a.epoch==b.epoch && a.x==b.x && a.y==b.y && a.z==b.z && a.bodyToView==b.bodyToView && a.viewVelocity==b.viewVelocity,"Interpolation mutated raw engine output");
      }
      require(model->curves[0].points.size()==4 && model->curves[0].points[1].x==1,"Raw numerical samples changed");
      // Rotation SLERP preserves orthonormality; longitude takes shortest dateline path.
      PlotPoint a,b; b.bodyToView={0,-1,0,1,0,0,0,0,1}; a.x=179; b.x=-179;
      const auto midpoint=plotInterpolatedPoint(a,b,.5,PlotModel::Kind::GroundTrack);
      near(std::abs(midpoint.x),180,"Ground interpolation took long path across map");
      for (int row=0;row<3;++row) { double length=0; for (int col=0;col<3;++col) length+=midpoint.bodyToView[3*row+col]*midpoint.bodyToView[3*row+col]; near(length,1,"Interpolated orientation is not orthonormal"); }
      // Disconnected/solver-reset edges jump instead of interpolating an invented arc.
      auto broken=std::make_shared<PlotModel>(PlotModel::Kind::Orbit); broken->curves[0].name="Probe";
      for (int i=0;i<4;++i) { broken->frame=i+1; if (i==2) broken->breakLines(); broken->append(0,i*10,0,0,21545+(i<2 ? i : 100+i)/86400.); }
      PlotWidget gap(broken); gap.setSharedReplayPosition(500);
      require(broken->replayCursor && broken->replayCursor->before==3,"Disconnected time gap consumed animation duration");
      gap.setSharedReplayPosition(250); require(!broken->curves[0].replayPose || broken->replayCursor->after==2,"Interpolated across disconnected arc");
      std::cout<<"PASS: propagation/interpolated modes; uniform simulation-time poses, tracked camera/vector/path/marker, sparse IDs, speed buttons, exact raw export, rotation, dateline and disconnected boundaries. "<<(native ? "Private software X11 native rendering." : "Offscreen rendering.")<<"\n";
      return 0;
   } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
