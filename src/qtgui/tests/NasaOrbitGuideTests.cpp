#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "PlotWidget.hpp"
#include "OrbitRenderer.hpp"
#include <QApplication>
#include <QMainWindow>
#include <QScreen>
#include <QToolButton>
#include <QAction>
#include <QDialog>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTest>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool v,const char *s) {if(!v)throw std::runtime_error(s);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
int main(int argc,char **argv) {
 QApplication app(argc,argv);
 try {
  require(argc==2,"Expected Axes or Grid and a widget platform");const QString mode=argv[1];require(mode=="Axes"||mode=="Grid"||mode=="GridRaster","Unknown orbit guide mode");const bool raster=mode=="GridRaster",grid=mode!="Axes";require(raster ? QGuiApplication::platformName()=="offscreen" : (QGuiApplication::platformName()=="xcb"||QGuiApplication::platformName()=="windows"),"Orbit guide platform does not match renderer route");
  const auto dir=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes){if(!dir.isEmpty()){QFile f(QDir(dir).filePath(name));require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Orbit guide artifact write failed");}};
  auto image=[&](const QString &name,const QImage &pixels){require(!pixels.isNull(),"Orbit guide image missing");if(!dir.isEmpty())require(pixels.save(QDir(dir).filePath(name+".png")),"Orbit guide oracle image save failed");};
  auto data=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);data->title="NASA "+mode+" display";data->grid=false;data->axes=false;data->axesLabels=false;data->frameLabel=false;data->labels=false;data->legend=false;data->sunlight=false;data->endOfRun=true;
  auto &body=data->curves[0];body.name="Sphere";body.color=QColor(80,160,220);body.radius=2;body.lines=false;body.showObject=grid;data->append(0,0,0,0);body.points.back().bodyToView={1,0,0,0,1,0,0,0,1};
  auto *viewer=new PlotWidget(data);QMainWindow window;window.setWindowTitle("NASA orbit guide acceptance — "+mode);window.setCentralWidget(viewer);const auto available=app.primaryScreen()->availableGeometry();window.resize(std::min(1100,available.width()-60),std::min(760,available.height()-80));window.show();acceptanceView(window);events();
  auto *canvas=viewer->canvas();auto *native=dynamic_cast<OrbitRenderer *>(canvas->findChild<QOpenGLWidget *>());if(raster)require(!native&&canvas->width()>400,"Raster route unexpectedly created a native renderer");else require(native&&QTest::qWaitFor([&]{return native->isValid()&&canvas->width()>400;},3000),"Native orbit renderer unavailable");canvas->setViewAngles(.55,.45);events();
  auto raw=[&] {QJsonArray rows;for(const auto &p:body.points){QJsonArray rotation;for(double v:p.bodyToView)rotation.append(v);rows.append(QJsonObject{{"x",p.x},{"y",p.y},{"z",p.z},{"epoch",p.epoch},{"frame",double(p.frame)},{"rotation",rotation}});}return QJsonDocument(rows).toJson(QJsonDocument::Compact);};
  auto capture=[&] {canvas->refresh();events();acceptanceView(window);auto pixels=canvas->captureImage();require(!pixels.isNull(),"Orbit framebuffer absent");return pixels;};
  auto toggle=[&](bool checked){auto *action=viewer->findChild<QAction *>("orbitDisplayAction");require(action,"Orbit display action absent");QToolButton *button=nullptr;for(auto *b:viewer->findChildren<QToolButton *>())if(b->defaultAction()==action)button=b;require(button&&button->isVisible()&&button->isEnabled(),"Orbit display toolbar action inaccessible");QTest::mouseClick(button,Qt::LeftButton);events();auto *dialog=viewer->findChild<QDialog *>("orbitDisplayDialog");require(dialog&&dialog->isVisible(),"Orbit display dialog did not open");acceptanceView(*dialog);auto *check=dialog->findChild<QCheckBox *>(grid?"orbitDisplay_grid":"orbitDisplay_axes");require(check&&check->isVisible()&&check->isEnabled()&&check->isChecked()!=checked,"Orbit guide checkbox state wrong");QTest::mouseClick(check,Qt::LeftButton,Qt::NoModifier,QPoint(8,check->height()/2));events();require(check->isChecked()==checked&&(grid?data->grid:data->axes)==checked,"Orbit guide checkbox did not update display model");acceptanceView(*dialog);auto *close=dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Close);require(close,"Orbit display Close absent");QTest::mouseClick(close,Qt::LeftButton);events();require(!dialog->isVisible(),"Orbit display close failed");};
  QColor backdrop;auto blue=[&](QColor c){return c!=backdrop&&c.blue()>20&&c.blue()>c.red()*1.5&&c.blue()>c.green()*1.15;};QJsonArray observations;QImage previousGrid;
  const int phases=grid?3:1;
  for(int phase=0;phase<phases;++phase){
   if(phase==1){const double angle=27*std::acos(-1)/180,c=std::cos(angle),s=std::sin(angle);body.points.back().bodyToView={1,0,0,0,c,-s,0,s,c};}
   if(phase==2){body.points.back().x=3;body.points.back().y=-2;body.points.back().z=1;}
   const auto stable=raw();const auto before=capture();backdrop=before.pixelColor(0,0);toggle(true);const auto on=capture();image(QString::number(phase)+"-off",before);image(QString::number(phase)+"-on",on);require(before.size()==on.size()&&on!=before,"Optional guide did not change rendered pixels");int changed=0,outside=0,darkened=0,red=0,green=0,blueAxis=0;int left=before.width(),right=-1,top=before.height(),bottom=-1;
   // Include the shaded body silhouette and the three-logical-pixel line edge.
   // A screen-aligned background grid remains outside this localized region.
   const int edgeAllowance=std::max(2,qRound(3*before.devicePixelRatio()));QVector<int> rowLeft(before.height(),before.width()),rowRight(before.height(),-1);
   for(int y=0;y<before.height();++y)for(int x=0;x<before.width();++x)if(blue(before.pixelColor(x,y))){rowLeft[y]=std::min(rowLeft[y],x);rowRight[y]=std::max(rowRight[y],x);}
   for(int y=0;y<before.height();++y)for(int x=0;x<before.width();++x){const auto a=before.pixelColor(x,y),b=on.pixelColor(x,y);if(blue(a)){left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);}if(a==b)continue;++changed;if(grid){bool inside=false;for(int row=std::max(0,y-edgeAllowance);row<=std::min(before.height()-1,y+edgeAllowance);++row)if(rowRight[row]>=rowLeft[row]&&x>=rowLeft[row]-edgeAllowance&&x<=rowRight[row]+edgeAllowance){inside=true;break;}if(!inside)++outside;if(qGray(a.rgb())>qGray(b.rgb())+35)++darkened;}else{if(b.red()>150&&b.red()>b.green()*1.5&&b.red()>b.blue()*1.5)++red;if(b.green()>150&&b.green()>b.red()*1.5&&b.green()>b.blue()*1.25)++green;if(b.blue()>200&&b.blue()>b.red()*1.5&&b.blue()>b.green()*1.3)++blueAxis;}}
   save("latest-pixel-observation.json",QJsonDocument(QJsonObject{{"phase",phase},{"edge_allowance",edgeAllowance},{"device_pixel_ratio",before.devicePixelRatio()},{"changed",changed},{"outside_body",outside},{"darkened",darkened},{"red_axis",red},{"green_axis",green},{"blue_axis",blueAxis},{"body_left",left},{"body_right",right},{"body_top",top},{"body_bottom",bottom}}).toJson());
   if(grid){require(right>left&&bottom>top&&darkened>100,"Latitude/longitude grid not rendered over celestial body");require(outside<changed/20+8,"Orbit Grid drew a background instead of body latitude/longitude lines");if(phase==1)require(on!=previousGrid,"Body-fixed latitude/longitude grid ignored body rotation");previousGrid=on;}else require(red>15&&green>15&&blueAxis>15,"Coordinate axes did not render three distinct colored directions");
   require(raw()==stable,"Display edit changed raw position/epoch/rotation samples");toggle(false);const auto restored=capture();require(restored==before,"Disabling guide did not restore exact framebuffer");require(raw()==stable,"Guide restoration changed raw samples");observations.append(QJsonObject{{"phase",phase},{"edge_allowance",edgeAllowance},{"device_pixel_ratio",before.devicePixelRatio()},{"changed",changed},{"outside_body",outside},{"darkened",darkened},{"red_axis",red},{"green_axis",green},{"blue_axis",blueAxis},{"raw_before_after_unchanged",true},{"off_restored_exactly",true}});save("raw-samples-"+QString::number(phase)+".json",stable);
  }
  QJsonObject proof{{"checkpoint","orbit-guides-"+mode},{"family",mode},{"renderer",raster?"raster":"native"},{"outcome","passed"},{"phases",phases},{"observations",observations},{"facets",QJsonArray{"actual-viewer-display-dialog",raster?"raster-three-dimensional-framebuffer":"native-three-dimensional-framebuffer","on-off-pixel-result","raw-sample-isolation","exact-off-pixel-restoration"}},{"scope","Selected synthetic orbital display poses and in-process Qt button events. Grid body localization/rotation/translation or three coordinate axes; no engine propagation, resource-tree path, OS input, arbitrary frames, general physical truth or host-GPU/platform qualification."}};save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}
}
