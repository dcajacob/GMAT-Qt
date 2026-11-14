#include "PlotWidget.hpp"
#include "MarkerRasterOracle.hpp"
#include "AcceptanceView.hpp"
#include "TestSettings.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QWindow>
#include <cstring>
#include <iostream>
using markeroracle::require;
namespace {
QJsonObject checkpoint;
QImage lastImage;
QJsonArray observations;
QString evidence;
void save(const QString &name,const QByteArray &bytes)
{
   if(evidence.isEmpty())return;QFile f(evidence+"/"+name);
   require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Cannot preserve marker evidence");
}
void events() { QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents(); }
std::shared_ptr<PlotModel> model(bool ground)
{
   auto m=std::make_shared<PlotModel>(ground?PlotModel::Kind::GroundTrack:PlotModel::Kind::XY);
   m->title="Marker acceptance Ω";m->grid=m->legend=m->labels=m->axes=m->footprints=false;m->endOfRun=true;
   m->xAxis.minimum=0;m->xAxis.maximum=20;m->yAxis.minimum=0;m->yAxis.maximum=10;
   m->xAxis.ticks=m->yAxis.ticks=2;m->xAxis.precision=m->yAxis.precision=2;
   auto &c=m->curves[0];c.name="Probe";c.color=Qt::red;c.lines=false;c.markers=true;c.showObject=false;c.errorBars=false;
   m->frame=1;
   if(ground) { m->map=QImage(8,4,QImage::Format_RGB32);m->map.fill(Qt::white); }
   return m;
}
// Independent scalar screen/data correspondence, reusing the previously
// qualified 600x400 XY fixture gutters and a 2:1 ground-map letterbox.
QRectF area(QSize size,bool ground)
{
   QRectF box(74,32,size.width()-100,size.height()-88-(ground?22:0));
   if(ground) {
      const double w=std::min(box.width(),2*box.height()),h=w/2;
      box=QRectF(box.center()-QPointF(w/2,h/2),QSizeF(w,h));
   }
   return box;
}
QRectF physical(QRectF box,double ratio)
{ return {box.left()*ratio,box.top()*ratio,box.width()*ratio,box.height()*ratio}; }
QPointF dataAt(QPointF pixel,QRectF box,double ratio,bool ground)
{
   const double x=(pixel.x()/ratio-box.left())/box.width(),y=(pixel.y()/ratio-box.top())/box.height();
   return ground?QPointF(-180+360*x,90-180*y):QPointF(20*x,10-10*y);
}
QByteArray samples(const PlotModel &m)
{
   QJsonArray a;
   for(auto i=m.curves.cbegin();i!=m.curves.cend();++i)for(const auto &p:i->points)
      a.append(QJsonArray{i.key(),p.x,p.y,p.z,p.epoch,qint64(p.frame),p.connect,p.solver,p.high,p.low,p.marker,p.highlighted,p.color.name()});
   return QJsonDocument(a).toJson(QJsonDocument::Compact);
}
QByteArray numbers(const PlotModel &m)
{
   QJsonArray a;
   for(auto i=m.curves.cbegin();i!=m.curves.cend();++i)for(const auto &p:i->points)
      a.append(QJsonArray{i.key(),p.x,p.y,p.z,p.epoch,qint64(p.frame),p.connect,p.solver,p.high,p.low});
   return QJsonDocument(a).toJson(QJsonDocument::Compact);
}
QImage capture(PlotCanvas &canvas,const PlotModel &m,double ratio)
{
   const auto before=samples(m);canvas.refresh();events();auto image=canvas.captureImage();
   require(!image.isNull()&&std::abs(canvas.devicePixelRatioF()-ratio)<1e-6&&
           std::abs(image.devicePixelRatio()-ratio)<1e-6,"Unexpected physical capture/device scale");
   require(image.width()==qRound(canvas.width()*ratio)&&image.height()==qRound(canvas.height()*ratio),
           "Capture changed physical resolution");
   require(samples(m)==before,"Rendering/capture changed retained samples or marker metadata");
   lastImage=image.convertToFormat(QImage::Format_RGB32);return lastImage;
}
void add(PlotModel &m,QPointF center,QRectF box,double ratio,bool ground,int marker,bool highlight)
{
   const auto p=dataAt(center,box,ratio,ground);m.append(0,p.x(),p.y());
   auto &point=m.curves[0].points.back();point.marker=marker;point.highlighted=highlight;
}
void raster(const QJsonObject &spec,bool ground,double ratio)
{
   auto m=model(ground);PlotCanvas canvas(m);canvas.resize(600,400);canvas.show();events();acceptanceView(canvas);
   const auto box=area(canvas.size(),ground),clip=physical(box,ratio);int checked=0;
   for(auto entry:spec["markers"].toArray()) {
      const auto definition=entry.toObject();const int id=definition["id"].toInt();markeroracle::Shape shape(definition);
      for(auto sizeValue:spec["sizes"].toArray())for(int policy=0;policy<2;++policy) {
         const double size=sizeValue.toDouble();QImage normal;
         for(bool highlighted:{false,true}) {
            checkpoint={{"mode",ground?"RasterGround":"RasterXY"},{"marker",id},{"size",size},{"highlighted",highlighted},{"point_override",policy==1},{"scale",ratio}};
            auto &curve=m->curves[0];curve.points.clear();curve.breakNext=true;curve.markerSize=int(size);curve.markerType=policy?(id+1)%10:id;
            std::vector<QPointF> centers;QJsonArray phases=spec["physical_pixel_phases"].toArray();
            for(int y=0;y<4;++y)for(int x=0;x<4;++x) {
               const QPointF base((box.left()+(x+.5)*box.width()/4)*ratio,(box.top()+(y+.5)*box.height()/4)*ratio);
               const QPointF center(std::floor(base.x())+phases[x].toDouble(),std::floor(base.y())+phases[y].toDouble());
               centers.push_back(center);add(*m,center,box,ratio,ground,policy?id:-1,highlighted);
            }
            const auto image=capture(canvas,*m,ratio);QJsonArray metrics;
            for(int k=0;k<int(centers.size());++k) {
               checkpoint["phase_index"]=k;metrics.append(markeroracle::json(markeroracle::check(image,centers[k],shape,size,ratio,highlighted,clip)));++checked;
            }
            if(highlighted)require(image!=normal,"Highlight ring did not change rendered pixels");else normal=image;
            observations.append(QJsonObject{{"marker",id},{"size",size},{"highlighted",highlighted},{"point_override",policy==1},{"phase_metrics",metrics}});
         }
      }
      acceptanceView(canvas);
   }
   require(checked==1280,"Raster phase/shape/size/highlight/policy matrix incomplete");
}
void identicalPixels(QImage a,QImage b)
{
   a=a.convertToFormat(QImage::Format_ARGB32);b=b.convertToFormat(QImage::Format_ARGB32);
   require(!a.isNull()&&a.size()==b.size(),"Export changed physical image dimensions");
   for(int y=0;y<a.height();++y)require(std::memcmp(a.constScanLine(y),b.constScanLine(y),size_t(a.width())*4)==0,
         "PNG decoding changed captured pixels");
}
void captureAndClip(const QJsonObject &spec,double ratio)
{
   int checked=0,exports=0,visibility=0;QTemporaryDir files;require(files.isValid(),"Temporary PNG destination unavailable");
   for(bool ground:{false,true}) {
      auto m=model(ground);PlotCanvas canvas(m);canvas.resize(600,400);canvas.show();events();acceptanceView(canvas);
      const auto box=area(canvas.size(),ground),clip=physical(box,ratio);
      for(auto entry:spec["markers"].toArray()) {
         const auto definition=entry.toObject();const int id=definition["id"].toInt();markeroracle::Shape shape(definition);
         for(auto sizeValue:spec["sizes"].toArray())for(bool highlighted:{false,true}) {
            const double size=sizeValue.toDouble();
            for(auto px:spec["physical_pixel_phases"].toArray())for(auto py:spec["physical_pixel_phases"].toArray()) {
               checkpoint={{"mode","CaptureClip"},{"ground",ground},{"marker",id},{"size",size},{"highlighted",highlighted},{"phase_x",px},{"phase_y",py},{"scale",ratio}};
               const double left=clip.left()+px.toDouble(),right=clip.right()-(1-px.toDouble());
               const double top=clip.top()+py.toDouble(),bottom=clip.bottom()-(1-py.toDouble());
               const double cx=std::floor(clip.center().x())+px.toDouble(),cy=std::floor(clip.center().y())+py.toDouble();
               const double outside=(size/2+3)*ratio+std::max(1.,ratio)+3;
               const std::vector<QPointF> centers={{left,cy},{right,cy},{cx,top},{cx,bottom},
                  {left,top},{right,top},{left,bottom},{right,bottom},
                  {clip.left()-outside,cy},{clip.right()+outside,cy},{cx,clip.top()-outside},{cx,clip.bottom()+outside}};
               auto &curve=m->curves[0];curve.points.clear();curve.breakNext=true;curve.markerType=id;curve.markerSize=int(size);
               for(auto center:centers)add(*m,center,box,ratio,ground,id,highlighted);
               const auto image=capture(canvas,*m,ratio);QJsonArray metrics;
               for(int k=0;k<int(centers.size());++k) {
                  checkpoint["clip_position"]=k;metrics.append(markeroracle::json(markeroracle::check(image,centers[k],shape,size,ratio,highlighted,clip,k<8)));++checked;
               }
               observations.append(QJsonObject{{"ground",ground},{"marker",id},{"size",size},{"highlighted",highlighted},{"phase_x",px},{"phase_y",py},{"clip_metrics",metrics}});
            }
         }
         acceptanceView(canvas);
      }
      // Visibility and replay filters are checked separately from geometric
      // clipping. Highlighted points must respect the curve/frame filters.
      for(auto entry:spec["markers"].toArray())for(auto sizeValue:spec["sizes"].toArray()) {
         const auto definition=entry.toObject();const int id=definition["id"].toInt();const double size=sizeValue.toDouble();markeroracle::Shape shape(definition);
         auto &c=m->curves[0];c.points.clear();c.breakNext=true;c.markerType=id;c.markerSize=int(size);
         const auto center=clip.center();add(*m,center,box,ratio,ground,id,true);
         for(int mode=0;mode<4;++mode) {
            checkpoint={{"mode","Visibility"},{"ground",ground},{"marker",id},{"size",size},{"filter",mode},{"scale",ratio}};
            c.visible=mode!=0;c.markers=mode!=1&&mode!=3;c.points.front().highlighted=mode!=1;canvas.setFrame(mode==2?0:1);
            const auto image=capture(canvas,*m,ratio);markeroracle::check(image,center,shape,size,ratio,c.points.front().highlighted,clip,mode==3);++visibility;
         }
         c.visible=c.markers=true;c.points.front().highlighted=false;canvas.setFrame(1);
      }
      canvas.hide();
      PlotWidget viewer(m);viewer.resize(800,600);viewer.show();events();acceptanceView(viewer);
      for(auto entry:spec["markers"].toArray())for(auto sizeValue:spec["sizes"].toArray())for(bool highlight:{false,true}) {
         const int id=entry.toObject()["id"].toInt();const int size=sizeValue.toInt();
         auto &c=m->curves[0];c.points.clear();c.breakNext=true;c.markerType=id;c.markerSize=size;
         m->append(0,ground?0:10,ground?0:5);c.points.front().marker=id;c.points.front().highlighted=highlight;
         checkpoint={{"mode","Export"},{"ground",ground},{"marker",id},{"size",size},{"highlighted",highlight},{"scale",ratio}};
         const auto before=samples(*m);const auto image=capture(*viewer.canvas(),*m,ratio);const auto path=files.filePath("Marker Δ.png");
         require(viewer.exportImage(path).isEmpty(),"Marker PNG export failed");identicalPixels(image,QImage(path));
         require(samples(*m)==before,"Export changed retained samples/marker metadata");++exports;
      }
   }
   require(checked==15360&&visibility==160&&exports==80,"Capture/clipping/visibility/export domain incomplete");
   observations.append(QJsonObject{{"clip_observations",checked},{"visibility_observations",visibility},{"pixel_exact_exports",exports}});
}
void controls(const QJsonObject &spec,double ratio)
{
   auto m=model(false);auto &c=m->curves[0];c.markers=false;c.markerSize=4;m->append(0,5,5);m->append(0,10,5);
   auto &other=m->curves[1];other.name="Untouched witness";other.color=Qt::blue;other.lines=false;other.markers=true;other.markerSize=12;m->append(1,15,7);
   PlotWidget viewer(m);viewer.resize(800,600);viewer.show();events();acceptanceView(viewer);
   const auto numerical=numbers(*m);const auto witness=m->curves[1];int accepted=0,cancelled=0;
   for(auto entry:spec["markers"].toArray())for(auto sizeValue:spec["sizes"].toArray()) {
      const int id=entry.toObject()["id"].toInt(),size=sizeValue.toInt();
      checkpoint={{"mode","Controls"},{"marker",id},{"size",size},{"scale",ratio}};
      auto edit=[&](bool accept,bool change) {
         viewer.raise();viewer.activateWindow();
         if(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible")require(QTest::qWaitForWindowActive(&viewer,5000),"Marker viewer did not activate");
         std::exception_ptr error;
         QTimer::singleShot(0,&viewer,[&] {
            auto *dialog=viewer.findChild<QDialog *>("plotStyleDialog");
            try {
               require(dialog,"Actual plot-style dialog absent");auto *tabs=dialog->findChild<QTabWidget *>("plotStyleTabs");
               require(tabs&&tabs->count()==4,"Plot style pages changed");tabs->setCurrentIndex(3);events();acceptanceView(*dialog);
               auto *page=dialog->findChild<QWidget *>("curveStyle_0");auto *combo=page?page->findChild<QComboBox *>("Marker shape"):nullptr;
               auto *spin=page?page->findChild<QSpinBox *>("Marker size"):nullptr;auto *enabled=page?page->findChild<QCheckBox *>("Markers"):nullptr;
               require(combo&&spin&&enabled&&combo->count()==10&&spin->minimum()==1&&spin->maximum()==30,"Marker controls or domain changed");
               for(int i=0;i<10;++i)require(combo->itemText(i)==spec["markers"].toArray()[i].toObject()["name"].toString(),"Marker identity/order differs from reviewed enumeration");
               if(change) {
                  QTest::mouseClick(combo,Qt::LeftButton,Qt::NoModifier,QPoint(combo->width()-10,combo->height()/2));events();
                  require(combo->view()->isVisible(),"Actual marker popup did not open");QTest::keyClick(combo->view(),Qt::Key_Home);
                  for(int i=0;i<id;++i)QTest::keyClick(combo->view(),Qt::Key_Down);QTest::keyClick(combo->view(),Qt::Key_Return);events();
                  auto *line=spin->findChild<QLineEdit *>();require(line,"Marker spin edit absent");line->setFocus();line->selectAll();QTest::keyClicks(line,QString::number(size));QTest::keyClick(line,Qt::Key_Tab);
                  if(!enabled->isChecked()) { enabled->setFocus();QTest::keyClick(enabled,Qt::Key_Space); }
               }
               require(combo->currentIndex()==id&&spin->value()==size&&enabled->isChecked(),"Actual marker settings differ from chosen values");
               require(numbers(*m)==numerical,"Pending marker controls changed numeric samples");acceptanceView(*dialog);
               auto *buttons=dialog->findChild<QDialogButtonBox *>();require(buttons,"Marker dialog buttons absent");
               QTest::mouseClick(buttons->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel),Qt::LeftButton);
            }catch(...) { error=std::current_exception();if(dialog)dialog->reject(); }
         });
         auto *action=viewer.findChild<QAction *>("plotStyleAction");require(action,"Actual Style action absent");action->trigger();if(error)std::rethrow_exception(error);events();
      };
      const auto before=capture(*viewer.canvas(),*m,ratio);const auto retained=samples(*m);const int oldType=c.markerType,oldSize=c.markerSize;const bool oldMarkers=c.markers;
      edit(false,true);require(samples(*m)==retained&&c.markerType==oldType&&c.markerSize==oldSize&&c.markers==oldMarkers&&capture(*viewer.canvas(),*m,ratio)==before,"Cancel changed marker presentation or data");++cancelled;
      edit(true,true);require(c.markerType==id&&c.markerSize==size&&c.markers,"Accepted marker settings not applied");
      for(const auto &p:c.points)require(p.marker==id,"Marker choice did not update retained point presentation");
      require(numbers(*m)==numerical&&other.markerType==witness.markerType&&other.markerSize==witness.markerSize&&other.color==witness.color&&other.markers==witness.markers,"Marker edit changed numeric data or another curve");++accepted;
      edit(false,false);++cancelled;capture(*viewer.canvas(),*m,ratio);
   }
   require(accepted==20&&cancelled==40,"Marker control matrix incomplete");
   observations.append(QJsonObject{{"accepted_marker_settings",accepted},{"cancelled_dialogs",cancelled},{"numeric_samples_unchanged",true},{"other_curve_unchanged",true}});
}
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("MarkerAcceptance");
   try {
      require(argc==5,"Supply startup,marker specification,mode and profile");TestSettings settings;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QFile file(QFileInfo(argv[2]).absoluteFilePath());
      require(file.open(QIODevice::ReadOnly),"Marker specification missing");const auto spec=QJsonDocument::fromJson(file.readAll()).object();
      const QString mode=argv[3],profile=argv[4];const auto profiles=spec["profiles"].toObject();require(profiles.contains(profile),"Unknown marker scale profile");const double ratio=profiles[profile].toDouble();
      evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");QDir::setCurrent(QFileInfo(startup).absolutePath());FileManager::Instance()->ReadStartupFile(startup.toStdString());
      require(spec["markers"].toArray().size()==10&&spec["sizes"].toArray().size()==2&&spec["physical_pixel_phases"].toArray().size()==4,"Marker domain incomplete");
      if(mode=="RasterXY"||mode=="RasterGround")raster(spec,mode=="RasterGround",ratio);
      else if(mode=="Capture")captureAndClip(spec,ratio);
      else if(mode=="Controls")controls(spec,ratio);
      else throw std::runtime_error("Unknown marker mode");
      const QJsonObject proof{{"checkpoint","marker-"+mode+"-"+profile},{"outcome","passed"},{"mode",mode},{"profile",profile},{"scale",ratio},
         {"metrics_columns",QJsonArray{"ink_pixels","maximum_centerline_distance_px","maximum_witness_distance_px","peak_white_equivalent_red_contrast","original_full_color_pixels","checked_geometric_witnesses","isolated_clipping_contacts","fractional_clip_excluded_witnesses"}},
         {"observations",observations},{"scope","Bounded current Qt marker semantics and physical raster/input/export checks. Legacy size/highlight/crossed-square differences, other sizes/scales/OS/GPU and full NASA qualification remain explicit gaps."}};
      save("marker-proof.json",QJsonDocument(proof).toJson());
      std::cout<<"PASS marker "<<mode.toStdString()<<" "<<profile.toStdString()<<": "<<observations.size()<<" observation groups; no retained passing images.\n";return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}
   catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}
   std::cerr<<QJsonDocument(checkpoint).toJson(QJsonDocument::Compact).constData()<<'\n';
   if(!evidence.isEmpty()) { save("marker-failure.json",QJsonDocument(checkpoint).toJson());if(!lastImage.isNull())lastImage.save(evidence+"/marker-failure.png"); }
   return 1;
}
