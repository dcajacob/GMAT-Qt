#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "FileManager.hpp"
#include "Moderator.hpp"
#include "ResourceProperties.hpp"
#include "ResourceEditor.hpp"
#include "CoordinateConverter.hpp"
#include "CoordinateSystem.hpp"
#include "Spacecraft.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "AcceptanceView.hpp"
#include <QDialogButtonBox>
#include <QDialog>
#include <QColorDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableWidget>
#include <QHeaderView>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <QSlider>
#include <QTabWidget>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTabWidget>
#include <QMouseEvent>
#include <fstream>
#include <iostream>
#include <cmath>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static const PlotCurve &curve(const PlotModel &model,const QString &name)
{
   for (const auto &c:model.curves) if (c.name==name) return c;
   throw std::runtime_error("Expected curve missing");
}
static void xyStyleWidgetChecks()
{
   auto model=std::make_shared<PlotModel>(PlotModel::Kind::XY);
   model->title="Style widget fixture"; model->grid=false; model->legend=false;
   model->xAxis.minimum=0; model->xAxis.maximum=4; model->yAxis.minimum=0; model->yAxis.maximum=4;
   auto &first=model->curves[0]; first.name="Edited curve"; first.color=Qt::red; first.width=3; first.lines=true; first.markers=false; first.errorBars=false;
   auto &second=model->curves[1]; second.name="Unchanged curve"; second.color=QColor(20,180,30); second.width=3; second.lines=true; second.markers=false; second.errorBars=false;
   // Wide fixture strokes contain fully colored interior pixels at both DPRs;
   // the color oracle must not depend on subpixel antialiasing of a one-pixel line.
   for (int i=0;i<3;++i) { model->frame=i+1; model->append(0,i+1,i==1 ? 2 : 1,0,0,true,false,.5,.25); model->append(1,i+1,3.5); }
   PlotWidget view(model); view.resize(800,600); view.show(); acceptanceView(view); QApplication::processEvents();
   const auto original=view.canvas()->captureImage(); require(!original.isNull(),"XY style fixture did not render");
   auto samples=[&] {
      require(model->curves[0].points.size()==3 && model->curves[1].points.size()==3,"Style controls changed sample counts");
      for (int i=0;i<3;++i) {
         const auto &a=model->curves[0].points[i],&b=model->curves[1].points[i];
         require(a.x==i+1 && a.y==(i==1 ? 2 : 1) && a.high==.5 && a.low==.25 && b.x==i+1 && b.y==3.5,"Style controls changed numerical samples or error values");
      }
      require(model->curves[1].color==QColor(20,180,30) && model->curves[1].lines && !model->curves[1].markers && !model->curves[1].errorBars,"Style controls changed an unselected curve");
   };
   auto pixels=[](const QImage &image,bool green=false) {
      int count=0;
      // Inspect only the plot interior. Subpixel text antialiasing can leave
      // red fringes in black title/tick glyphs outside the curve area.
      for (int y=image.height()/10;y<9*image.height()/10;++y) for (int x=image.width()/8;x<7*image.width()/8;++x) {
         const auto c=image.pixelColor(x,y);
         if (green ? c.green()>140 && c.red()<100 && c.blue()<100 : c.red()>160 && c.green()<110 && c.blue()<110) ++count;
      }
      return count;
   };
   auto edit=[&](const std::function<void(QDialog &,QWidget &)> &change,bool accept) {
      std::exception_ptr failure; bool visited=false; QTimer timer; timer.setSingleShot(true);
      QObject::connect(&timer,&QTimer::timeout,&view,[&] {
         auto *dialog=view.findChild<QDialog *>("plotStyleDialog");
         try {
            require(dialog,"XY style dialog did not open"); dialog->findChild<QTabWidget *>("plotStyleTabs")->setCurrentIndex(3);
            auto *page=dialog->findChild<QWidget *>("curveStyle_0"); require(page && page->isVisible(),"Selected curve style page is not visible"); visited=true; change(*dialog,*page);
            dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
         } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
      });
      timer.start(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible" ? 100 : 0);
      view.findChild<QAction *>("plotStyleAction")->trigger(); timer.stop();
      if (failure) std::rethrow_exception(failure); require(visited,"Curve style controls were not visited"); samples();
   };
   auto check=[](QWidget &page,const char *name,bool value) {
      auto *box=page.findChild<QCheckBox *>(name); require(box && box->isVisible() && box->isEnabled(),"Curve style checkbox is not usable");
      if (box->isChecked()!=value) box->click(); require(box->isChecked()==value,"Curve style checkbox did not change");
   };
   auto color=[&](QDialog &owner,QWidget &page,const QColor &value,bool accept) {
      auto *button=page.findChild<QPushButton *>("Curve color"); require(button && button->isVisible(),"Curve color button is not visible");
      const auto previous=button->property("selectedColor").value<QColor>(); bool visited=false; std::exception_ptr failure;
      QTimer timer; timer.setSingleShot(true); QObject::connect(&timer,&QTimer::timeout,&owner,[&] {
         auto *picker=qobject_cast<QColorDialog *>(QApplication::activeModalWidget());
         try { require(picker,"Actual color picker did not open"); visited=true; picker->setCurrentColor(value); if (accept) picker->accept(); else picker->reject(); }
         catch (...) { failure=std::current_exception(); if (picker) picker->reject(); }
      });
      timer.start(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible" ? 100 : 0); button->click(); timer.stop();
      if (failure) std::rethrow_exception(failure); require(visited,"Color picker was not exercised");
      require(button->property("selectedColor").value<QColor>()==(accept ? value : previous),"Color picker returned the wrong accepted/cancelled color");
   };
   const QColor blue(20,80,220);
   edit([&](QDialog &d,QWidget &p) { check(p,"Visible",false); color(d,p,blue,false); color(d,p,blue,true); require(model->curves[0].color==Qt::red,"Pending color changed the rendered model"); },false);
   require(view.canvas()->captureImage()==original && model->curves[0].visible && model->curves[0].color==Qt::red,"Style Cancel changed existing appearance");
   edit([&](QDialog &,QWidget &p) { check(p,"Visible",false); },true);
   auto hidden=view.canvas()->captureImage(); require(!model->curves[0].visible && pixels(hidden)==0 && pixels(hidden,true)>0,qPrintable(QString("Visible checkbox failed: visible=%1, red=%2, green=%3").arg(model->curves[0].visible).arg(pixels(hidden)).arg(pixels(hidden,true))));
   edit([&](QDialog &,QWidget &p) { check(p,"Visible",true); check(p,"Lines",false); check(p,"Markers",false); check(p,"Error bars",false); },true);
   const auto blank=view.canvas()->captureImage(); require(pixels(blank)==0,"Disabled curve features left a red trace");
   edit([&](QDialog &,QWidget &p) { check(p,"Markers",true); },true);
   const auto markers=view.canvas()->captureImage(); require(model->curves[0].markers && pixels(markers)>0 && markers!=blank,"Markers checkbox did not render markers");
   edit([&](QDialog &,QWidget &p) { check(p,"Markers",false); check(p,"Error bars",true); },true);
   const auto errors=view.canvas()->captureImage(); require(model->curves[0].errorBars && pixels(errors)>0 && errors!=markers,"Error-bars checkbox did not render the stored error values");
   edit([&](QDialog &d,QWidget &p) { check(p,"Lines",true); check(p,"Markers",true); color(d,p,blue,true); },true);
   require(model->curves[0].lines && model->curves[0].markers && model->curves[0].color==blue,"Accepted style widgets did not update the selected curve");
   for (const auto &point:model->curves[0].points) require(point.color==blue,"Accepted color did not update retained point colors");
   const auto changed=view.canvas()->captureImage(); require(changed!=original && pixels(changed)==0 && pixels(changed,true)>0,"Accepted color failed to replace red while preserving the other curve");
   samples(); view.close();
   const QJsonObject proof{{"kind","xy-style-widgets"},{"outcome","passed"},{"controls",QJsonArray{"Visible","Lines","Markers","Error bars","Curve color"}},
      {"checks",QJsonArray{"widget-clicks","color-dialog-accept-cancel","parent-cancel","selected-curve-only","rendered-features","retained-sample-values"}}};
   std::cout<<"GUI_ROUTE_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   if (argc==2 && QString::fromLocal8Bit(argv[1])=="--style-controls") { try { xyStyleWidgetChecks(); return 0; } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; } }
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtPlots");
   if (argc<3 || argc>4) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),script=QFileInfo(argv[2]).absoluteFilePath();
   const auto image=argc==4 ? QFileInfo(argv[3]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings isolatedSettings;
      for (bool perspective:{false,true}) {
         auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
         model->perspective=perspective; model->fitCamera=true; model->labels=false; model->legend=false; model->axes=false; model->grid=false;
         model->curves[0].radius=1; model->curves[0].color=Qt::green; model->append(0,0,0,0);
         PlotCanvas canvas(model); canvas.resize(600,400); canvas.show(); canvas.setViewAngles(0,0); app.processEvents();
         const auto visible=canvas.captureImage();
         model->curves[1].lines=false; model->curves[1].showObject=false; model->append(1,1e9,0,0);
         canvas.refresh(); app.processEvents(); require(canvas.captureImage()==visible,"Undrawn camera/Sun object changed Fit bounds");
      }
      for (const auto kind:{PlotModel::Kind::Orbit,PlotModel::Kind::GroundTrack}) {
         auto model=std::make_shared<PlotModel>(kind); model->axes=false; model->grid=false; model->labels=false; model->legend=false;
         model->redrawPoints=1; model->curves[0].name="Track"; model->curves[0].color=Qt::red;
         for (const auto &point:QVector<QPointF>{{-100,-50},{100,-50},{100,50},{-100,50},{-100,0}}) { ++model->frame; model->append(0,point.x(),point.y()); }
         PlotCanvas canvas(model); canvas.resize(600,400); canvas.show(); app.processEvents();
         const auto recent=canvas.captureImage(); require(!recent.isNull(),"Recent trajectory capture failed");
         model->endOfRun=true; canvas.refresh(); app.processEvents(); const auto complete=canvas.captureImage();
         require(recent!=complete,"Run completion did not restore complete retained trajectory");
         model->endOfRun=false; model->redrawPoints=0; canvas.refresh(); app.processEvents();
         require(canvas.captureImage()==complete,"Redraw zero did not show all retained trajectory points");
         model->endOfRun=true; model->redrawPoints=1; canvas.setFrame(3); app.processEvents(); const auto replay=canvas.captureImage();
         model->redrawPoints=0; canvas.refresh(); app.processEvents(); require(canvas.captureImage()!=replay,"Replay ignored recent trajectory setting");
         require(model->curves[0].points.size()==5,"Redraw setting discarded retained history");
         model->redrawPoints=1; model->clear(); require(!model->endOfRun,"Rerun retained completion state");
      }
      {
         auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
         model->scriptedCamera=true; model->axes=false; model->grid=false; model->labels=false; model->legend=false;
         model->curves[0].radius=1; model->curves[0].color=Qt::green;
         model->append(0,0,0,0); model->cameras.push_back({0,{0,0,6},{0,0,0},{0,1,0}});
         PlotWidget view(model); view.resize(640,480); view.show(); QApplication::processEvents();
         auto *projection=view.findChild<QComboBox *>("orbitProjection");
         auto *fov=view.findChild<QDoubleSpinBox *>("orbitFieldOfView");
         require(projection && fov && !fov->isEnabled(),"Camera projection controls missing");
         view.resize(330,480); QApplication::processEvents();
         auto *cameraAction=view.findChild<QAction *>("orbitCameraAction"); require(cameraAction,"Camera panel action missing");
         cameraAction->trigger(); QApplication::processEvents();
         auto *cameraDialog=view.findChild<QDialog *>("orbitCameraDialog");
         require(cameraDialog && cameraDialog->isVisible() && projection->isVisible() && fov->isVisible(),"Tiled camera controls inaccessible");
         require(cameraDialog->rect().contains(projection->mapTo(cameraDialog,QPoint(0,0))) &&
            cameraDialog->rect().contains(fov->mapTo(cameraDialog,fov->rect().bottomRight())),"Camera controls clipped");
         require(!cameraDialog->findChild<QPushButton *>("keepOrbitProjection")->isEnabled(),"Standalone camera offered unavailable script save");
         if (!image.isEmpty()) require(cameraDialog->grab().save(image+".camera.png"),"Camera panel screenshot failed");
         cameraDialog->close(); view.resize(640,480); QApplication::processEvents();
         const auto orthographic=view.canvas()->captureImage();
         projection->setCurrentIndex(1);
         require(model->perspective && fov->isEnabled(),"Perspective selector did not update model");
         const auto perspective=view.canvas()->captureImage();
         fov->setValue(90);
         require(view.canvas()->captureImage()!=perspective,"FOV control did not affect rendered view");
         fov->setValue(50); model->curves[0].points.front().z=2;
         require(view.canvas()->captureImage()!=perspective,"Perspective depth did not affect rendered view");
         model->curves[0].points.front().z=0; projection->setCurrentIndex(0);
         require(view.canvas()->captureImage()==orthographic,"Orthographic view was not restored after perspective");
         cameraAction->trigger(); QApplication::processEvents();
         require(cameraDialog->isVisible() && projection->currentIndex()==0 && fov->value()==50,"Camera panel reopen lost live settings");
         cameraDialog->close();
         auto *display=view.findChild<QAction *>("orbitDisplayAction"); require(display,"Display action missing");
         display->trigger(); QApplication::processEvents();
         auto *displayDialog=view.findChild<QDialog *>("orbitDisplayDialog"); require(displayDialog && displayDialog->isVisible(),"Display panel did not open");
         const auto beforeDisplay=view.canvas()->captureImage();
         const auto frame=model->frame; const auto history=model->cameras.size();
         const QList<QPair<QString,bool PlotModel::*>> controls={
            {"axes",&PlotModel::axes},{"grid",&PlotModel::grid},{"labels",&PlotModel::labels},{"legend",&PlotModel::legend},
            {"xyPlane",&PlotModel::xyPlane},{"eclipticPlane",&PlotModel::eclipticPlane},{"wireframe",&PlotModel::wireframe},{"sunLine",&PlotModel::sunLine}};
         for (const auto &entry:controls) {
            auto *control=displayDialog->findChild<QCheckBox *>("orbitDisplay_"+entry.first);
            require(control && control->isVisible() && !control->isChecked(),"Display setting missing or initial value incorrect");
            control->setChecked(true); require(model.get()->*entry.second,"Display toggle did not change model");
         }
         require(view.canvas()->captureImage()!=beforeDisplay,"Display controls did not affect rendering");
         require(model->frame==frame && model->cameras.size()==history,"Display toggles changed mission history");
         if (!image.isEmpty()) require(displayDialog->grab().save(image+".display.png"),"Display screenshot failed");
         displayDialog->close(); model->axes=false; display->trigger();
         require(!displayDialog->findChild<QCheckBox *>("orbitDisplay_axes")->isChecked(),"Display panel reopen did not synchronize model changes");
         for (const auto &entry:controls) displayDialog->findChild<QCheckBox *>("orbitDisplay_"+entry.first)->setChecked(false);
         require(view.canvas()->captureImage()==beforeDisplay,"Display settings did not restore original rendering");
         displayDialog->close();
      }
      {
         auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
         model->scriptedCamera=true; model->axes=false; model->grid=false; model->labels=false; model->legend=false;
         model->curves[0].radius=1; model->curves[0].color=Qt::green;
         model->append(0,0,0,0); model->cameras.push_back({0,{0,0,6},{0,0,0},{0,1,0}});
         model->cameraViews={{"Wide",false,50,{}},{"Close",true,35,{{0,{0,0,4},{0,0,0},{0,1,0}}}}};
         PlotWidget view(model); view.resize(640,480); view.show(); QApplication::processEvents();
         auto *selector=view.findChild<QComboBox *>("orbitCameraView");
         require(selector && selector->count()==2,"Camera selector absent");
         const auto primary=view.canvas()->captureImage();
         selector->setCurrentIndex(1); const auto close=view.canvas()->captureImage();
         require(primary!=close && model->selectedCamera==1 && model->perspective,"Named camera selection did not affect rendering");
         selector->setCurrentIndex(0); require(view.canvas()->captureImage()==primary,"Camera switching failed to restore initial render");
         model->clear(); require(model->cameras.empty() && model->cameraViews[1].cameras.empty(),"Clear left secondary camera history behind");
      }
      PlotModel sample(PlotModel::Kind::GroundTrack);
      sample.maxPoints=3; sample.append(0,170,2); sample.append(0,175,4);
      sample.append(0,std::numeric_limits<double>::quiet_NaN(),8); sample.append(0,-175,8);
      require(!sample.curves[0].points.back().connect,"Missing data bridged a track");
      sample.append(0,-170,9); require(sample.curves[0].points.size()==3,"Point bound not enforced");
      const auto split=PlotModel::groundSegments({170,0},{-170,20});
      require(split.size()==2 && split[0].second==QPointF(180,10) && split[1].first==QPointF(-180,10),"Map-edge interpolation wrong");
      MainWindow window; window.show();
      require(window.initialize(startup),"Initialization failed");
      QTemporaryDir output;
      require(output.isValid(),"Temporary output directory unavailable");
      FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",(output.path()+"/").toStdString());
      require(window.runMission()==MainWindow::RunResult::Completed,"Default mission failed");
      QApplication::processEvents();
      const auto *activePlot=window.findChild<QMdiArea *>()->activeSubWindow();
      require(activePlot && !activePlot->property("plotName").toString().isEmpty(),
              "Default mission left plots behind the script window");
      for (const auto &name : {"DefaultOrbitView","DefaultGroundTrackPlot"}) {
         const auto model=window.plotReceiver()->model(name);
         require(model && !curve(*model,"DefaultSC").points.empty(),"Default mission plot history missing");
         bool visible=false;
         for (auto *child:window.findChildren<QMdiSubWindow *>())
            if (child->property("plotName").toString()==name && child->isVisible()) visible=true;
         require(visible,"Default mission plot window missing");
      }
      auto *defaultArea=window.findChild<QMdiArea *>("workspace");
      auto *defaultOutput=window.findChild<QTreeWidget *>("Output");
      for (auto *tabs:window.findChildren<QTabWidget *>())
         if (tabs->indexOf(defaultOutput)>=0) tabs->setCurrentWidget(defaultOutput);
      auto openDefaultOrbit=[&] {
         const auto items=defaultOutput->findItems("DefaultOrbitView",Qt::MatchExactly|Qt::MatchRecursive);
         require(items.size()==1,"Default orbit absent from Output");
         defaultOutput->scrollToItem(items.first()); QApplication::processEvents();
         const QPointF position=defaultOutput->visualItemRect(items.first()).center();
         for (auto type:{QEvent::MouseButtonPress,QEvent::MouseButtonRelease,QEvent::MouseButtonDblClick,QEvent::MouseButtonRelease}) {
            QMouseEvent event(type,position,defaultOutput->viewport()->mapToGlobal(position.toPoint()),Qt::LeftButton,
               type==QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(defaultOutput->viewport(),&event);
         }
         QApplication::processEvents();
         auto *opened=defaultArea->activeSubWindow();
         require(opened && opened->property("plotName")=="DefaultOrbitView","Output double-click did not activate default OrbitView");
         require(!opened->isMinimized() && opened->widget()->isVisible(),"Output activation left OrbitView minimized or hidden");
         require(defaultArea->subWindowList(QMdiArea::StackingOrder).last()==opened,"Output activation left OrbitView behind another window");
         return opened;
      };
      auto *defaultOrbit=openDefaultOrbit();
      for (auto *child:defaultArea->subWindowList()) if (child->property("plotName")=="DefaultGroundTrackPlot") {
         child->showMaximized(); defaultArea->setActiveSubWindow(child); child->raise();
      }
      QApplication::processEvents(); openDefaultOrbit();
      defaultOrbit->showMinimized(); QApplication::processEvents();
      openDefaultOrbit();
      defaultOrbit->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      openDefaultOrbit();
      require(window.loadScript(script),"Plot mission could not open");
      require(window.runMission()==MainWindow::RunResult::Completed,"Plot mission failed");
      auto *receiver=window.plotReceiver();
      auto orbit=receiver->model("QtOrbit"),ground=receiver->model("QtGround"),xy=receiver->model("QtXY");
      require(orbit && ground && xy,"Mixed plots not created");
      require(receiver->model("QtData") && receiver->model("QtData")->kind==PlotModel::Kind::Table,"Dynamic data display missing");
      require(orbit->kind==PlotModel::Kind::Orbit && ground->kind==PlotModel::Kind::GroundTrack && xy->kind==PlotModel::Kind::XY,"Plot types mixed up");
      const auto &o=curve(*orbit,"QtSat"),&g=curve(*ground,"QtSat");
      require(o.points.size()>100 && g.points.size()>100 && xy->curves[0].points.size()>100,"No real plot histories recorded");
      require(!ground->map.isNull(),"Resolved central body map did not load");
      const auto fixedStars=receiver->model("QtFixedStars");
      require(fixedStars && fixedStars->starsEnabled && fixedStars->starCount==1234 &&
              fixedStars->starCatalog.stars.size()>40000,"Script star settings or catalog resolution failed");
      require(fixedStars->constellationsEnabled && fixedStars->constellationCatalog.segments.size()>500 &&
         fixedStars->xyPlane && fixedStars->eclipticPlane && fixedStars->wireframe && fixedStars->sunLine,
         "Scripted drawing settings or constellation catalog missing");
      require(receiver->model("QtLegacyGround")->footprints,"Ground-track footprint setting ignored");
      require(!orbit->starsEnabled && !orbit->starCatalogLoaded,"Disabled stars unnecessarily loaded a catalog");
      require(orbit->cameras.size()==o.points.size(),"Object camera history length differs from orbit");
      for (const auto *camera:{&orbit->cameras.front(),&orbit->cameras.back()}) {
         const PlotPoint *position=nullptr;
         for (const auto &point:o.points) if (point.frame==camera->frame) position=&point;
         require(position,"Object camera frame missing");
         const double coordinates[]={position->x,position->y,position->z};
         for (int axis=0;axis<3;++axis) require(std::abs(camera->eye[axis]-(axis+1)*100-3*coordinates[axis])<1e-9 && camera->target[axis]==0,
            "Object viewpoint/vector reference or direction vector ignored");
      }
      auto *inertial=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("EarthMJ2000Eq"));
      auto *earthFixed=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("EarthFixed"));
      CoordinateConverter skyReference;
      const auto &fixedEarth=curve(*fixedStars,"Earth");
      require(fixedStars->scriptedCamera && fixedStars->cameras.size()>100,"Scripted camera history missing");
      const auto &fixedSat=curve(*fixedStars,"QtSat");
      for (const auto *camera:{&fixedStars->cameras.front(),&fixedStars->cameras.back()}) {
         const PlotPoint *position=nullptr;
         for (const auto &point:fixedSat.points) if (point.frame==camera->frame) position=&point;
         require(position,"Camera frame has no matching spacecraft state");
         const double coordinates[]={position->x,position->y,position->z},offset[]={20000,4000,6000};
         Rvector6 up(1,0,0,0,0,0),converted;
         skyReference.Convert(position->epoch,up,inertial,converted,earthFixed);
         for (int axis=0;axis<3;++axis) {
            require(std::abs(camera->target[axis]-coordinates[axis])<1e-9,"Camera target did not track spacecraft");
            require(std::abs(camera->eye[axis]-coordinates[axis]-offset[axis])<1e-9,"Camera reference/scale incorrect");
            require(std::abs(camera->up[axis]-converted[axis])<1e-10,"Camera up frame differs from independent conversion");
         }
      }
      for (const auto *point:{&fixedEarth.points.front(),&fixedEarth.points.back()}) {
         Rvector6 direction(1,0,0,0,0,0),converted;
         skyReference.Convert(point->epoch,direction,inertial,converted,earthFixed);
         for (int axis=0;axis<3;++axis)
            require(std::abs(converted[axis]-point->inertialToView[axis*3])<1e-10,"Star frame differs from independent coordinate conversion");
      }
      const auto &earth=curve(*orbit,"Earth");
      require(!QImage(earth.texturePath).isNull(),"Resolved orbit texture did not load");
      auto *fixed=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("EarthFixed"));
      auto *view=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject(orbit->coordinates.toStdString()));
      require(fixed && view && !earth.points.empty(),"Body orientation reference missing");
      CoordinateConverter orientationReference;
      for (const auto *point:{&earth.points.front(),&earth.points.back()}) {
         Rvector6 surface(1,0,0,0,0,0),converted;
         orientationReference.Convert(point->epoch,surface,fixed,converted,view);
         for (int row=0;row<3;++row)
            require(std::abs(converted[row]-point->bodyToView[row*3])<1e-10,
                    "Texture orientation differs from body-fixed coordinate conversion");
         require(point->hasSun,"Sun position was not recorded");
         auto sun=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Sun")->GetMJ2000State(point->epoch);
         auto *internal=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("EarthMJ2000Eq"));
         orientationReference.Convert(point->epoch,sun,internal,converted,view);
         for (int row=0;row<3;++row)
            require(std::abs(converted[row]-point->sunPosition[row])<1e-7,"Recorded Sun position differs from ephemeris");
      }
      auto *spacecraft=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"));
      require(spacecraft,"Spacecraft attitude reference missing");
      for (const auto *point:{&o.points.front(),&o.points.back()}) {
         view->ToBaseSystem(A1Mjd(point->epoch),Rvector6(),true);
         const auto viewToBody=spacecraft->GetAttitude(point->epoch)*view->GetLastRotationMatrix();
         for (int row=0;row<3;++row) for (int col=0;col<3;++col) {
            double product=0;
            for (int k=0;k<3;++k) product+=viewToBody(row,k)*point->bodyToView[k*3+col];
            require(std::abs(product-(row==col ? 1.0 : 0.0))<1e-10,"Recorded spacecraft attitude has wrong orientation");
         }
      }
      const auto sampled=receiver->model("QtSampledGround");
      require(sampled && sampled->frame==(ground->frame+6)/7,"Configured collection frequency was not honored");
      require(curve(*sampled,"QtSat").points.size()==50 && curve(*sampled,"QtSat").width==3,
         "Configured retention limit or line width was not honored");
      std::ifstream report((output.path()+"/QtPlotReference.txt").toStdString());
      double secs,x,y,z,lon,lat;
      require(static_cast<bool>(report>>secs>>x>>y>>z>>lon>>lat),"Reference report missing or malformed");
      require(std::abs(secs-12000)<1e-6,"Mission did not finish at requested epoch");
      const auto &last=o.points.back();
      require(std::abs(last.x-x)<1e-7 && std::abs(last.y-y)<1e-7 && std::abs(last.z-z)<1e-7,"Orbit coordinates differ from engine report");
      const auto &lastxy=xy->curves.constFind(0).value().points.back();
      require(std::abs(lastxy.x-secs)<1e-5 && std::abs(lastxy.y-x)<1e-7,"XY samples differ from engine report");
      const auto &lastg=g.points.back();
      require(std::abs(std::remainder(lastg.x-lon,360.0))<1e-6 && std::abs(lastg.y-lat)<1e-5,"Ground track differs from geodetic engine report");
      const auto legacy=receiver->model("QtLegacyGround");
      require(legacy && legacy->kind==PlotModel::Kind::GroundTrack && !legacy->map.isNull(),"Legacy ground track type or map incorrect");
      require(Moderator::Instance()->GetConfiguredObject("QtLegacyGround")->IsOfType("GroundTrack"),"GroundTrackPlot script alias no longer maps to GroundTrack");
      const auto &legacyCurve=curve(*legacy,"QtSat");
      require(legacyCurve.points.size()==80,"Legacy ground track retention not honored");
      const auto &legacyLast=legacyCurve.points.back();
      require(std::abs(std::remainder(legacyLast.x-lon,360.0))<1e-6 && std::abs(legacyLast.y-lat)<1e-5,
         "GroundTrackPlot script alias differs from geodetic engine report");
      const auto count=o.points.size();
      auto *area=window.findChild<QMdiArea *>("workspace"); require(area!=nullptr,"Workspace missing");
      require(receiver->show("QtData"),"Dynamic display could not open");
      auto *table=qobject_cast<QTableWidget *>(area->activeSubWindow()->widget());
      require(table && table->rowCount()>0 && table->columnCount()>0,"Dynamic table is empty");
      QString tableText; QColor tableColor; bool elapsedFound=false;
      for (int r=0;r<table->rowCount();++r) for (int c=0;c<table->columnCount();++c) {
         if (auto *item=table->item(r,c)) if (item->text().contains("12000")) {
            tableText=item->text(); tableColor=item->foreground().color(); elapsedFound=true;
         }
      }
      require(elapsedFound && tableColor==QColor(20,80,150),"Dynamic table values or colors differ from mission");
      area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(receiver->show("QtData"),"Closed dynamic table could not reopen");
      table=qobject_cast<QTableWidget *>(area->activeSubWindow()->widget());
      const auto matches=table->findItems(tableText,Qt::MatchExactly);
      require(matches.size()==1 && matches.first()->foreground().color()==tableColor,"Reopened dynamic table lost values or colors");
      auto *outputTree=window.findChild<QTreeWidget *>("Output");
      const auto reports=outputTree->findItems("QtReport",Qt::MatchExactly|Qt::MatchRecursive);
      require(reports.size()==1,"Report not listed in Output tree");
      outputTree->itemDoubleClicked(reports.first(),0);
      auto *viewer=window.findChild<QPlainTextEdit *>("report:QtReport");
      require(viewer && viewer->toPlainText().contains("12000"),"Output report did not open generated file");
      viewer->parentWidget()->parentWidget()->close();
      auto closePlots=[&] { for (auto *child:area->subWindowList()) if (!child->property("plotName").toString().isEmpty()) child->close(); };
      closePlots(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(!receiver->IsThere("QtOrbit") && receiver->show("QtOrbit"),"Closed plot could not reopen");
      require(receiver->model("QtOrbit")->curves[0].points.size()==count,"Reopening lost recorded samples");
      auto *plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()); require(plot!=nullptr,"Reopened plot wrong widget type");
      const double zoom=plot->canvas()->zoomFactor(); plot->canvas()->zoomBy(1); plot->canvas()->zoomBy(-1);
      require(std::abs(plot->canvas()->zoomFactor()-zoom)<1e-12,"Wheel zoom not reversible");
      plot->canvas()->setFrame(2); plot->canvas()->grab(); plot->canvas()->setFrame(std::numeric_limits<quint64>::max());
      require(receiver->model("QtOrbit")->curves[0].points.size()==count,"Replay discarded recorded data");
      bool closedDuringRun=false;
      QTimer::singleShot(1,&window,[&] { closePlots(); closedDuringRun=window.isRunning(); });
      require(window.runMission()==MainWindow::RunResult::Completed,"Plot rerun failed");
      require(closedDuringRun,"Close-during-run was not exercised");
      require(receiver->model("QtOrbit")->curves[0].points.size()==count,"Rerun accumulated old data");
      receiver->CreateGroundTrackWindow("Sparse","","Sparse",0,0,.5,.5,false);
      receiver->TakeGroundTrackAction("Sparse","Satellites=QtSat|MissingSat");
      const double valid[]={10,20,30,40},missing[]={11,21,NAN,NAN},again[]={12,22,32,42};
      receiver->UpdateGroundTrackData("Sparse",1,valid,2); receiver->UpdateGroundTrackData("Sparse",2,missing,2); receiver->UpdateGroundTrackData("Sparse",3,again,2);
      auto sparse=receiver->model("Sparse");
      require(sparse->curves[0].points.size()==3 && sparse->curves[1].points.size()==2 && !sparse->curves[1].points.back().connect,"Absent satellite corrupted track slots");
      require(!receiver->TakeGroundTrackAction("Sparse","AddStation=QtSat"),"Non-station object accepted as station");
      receiver->DeleteGlPlot("Sparse");
      const auto footprint=PlotModel::groundFootprint({179,87});
      require(footprint.size()==73,"Ground footprint missing");
      constexpr double radians=3.14159265358979323846/180;
      for (const auto &point:footprint) {
         const double cosine=std::sin(87*radians)*std::sin(point.y()*radians)+std::cos(87*radians)*std::cos(point.y()*radians)*std::cos((point.x()-179)*radians);
         require(std::abs(cosine-std::cos(5*radians))<1e-12,"Footprint does not maintain angular radius near a pole");
      }
      require(PlotModel::groundSegments({180,0},{-180,0}).isEmpty(),"Equivalent dateline endpoints produce an invalid segment");
      require(receiver->CreateXyPlotWindow("IterationCheck","",0,0,.5,.5,false,"Iterations","x","y"),"XY callback fixture failed");
      receiver->AddXyPlotCurve("IterationCheck",0,"value",0xff0000);
      receiver->UpdateXyPlotCurve("IterationCheck",0,0,0);
      receiver->XyPlotMarkBreak("IterationCheck"); receiver->XyPlotMarkBreak("IterationCheck");
      for (int iteration=0;iteration<3;++iteration) {
         receiver->UpdateXyPlotCurve("IterationCheck",0,1,iteration+1);
         receiver->UpdateXyPlotCurve("IterationCheck",0,2,iteration+2);
         receiver->XyPlotClearFromBreak("IterationCheck",-1);
         const auto &c=receiver->model("IterationCheck")->curves[0];
         require(c.points.size()==1 && c.breaks.size()==1,"Repeated solver clear lost its anchor or retained old iterations");
      }
      receiver->UpdateXyPlotCurve("IterationCheck",0,1,1);
      receiver->XyPlotChangeMarker("IterationCheck",1,5,0);
      receiver->XyPlotMarkPoint("IterationCheck",1,0);
      const auto &styled=receiver->model("IterationCheck")->curves[0];
      require(styled.points[0].marker==0 && styled.points[1].marker==5 && !styled.points[0].highlighted && styled.points[1].highlighted,
         "Indexed marker/highlight changed unrelated points");
      receiver->UpdateXyPlotCurve("IterationCheck",0,2,1.5);
      receiver->show("IterationCheck");
      auto *stylePlot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget());
      QVector<QImage> markerImages;
      for (int marker=0;marker<10;++marker) {
         receiver->XyPlotCurveSettings("IterationCheck",false,1,100,true,14,marker,false,0);
         markerImages.append(stylePlot->canvas()->captureImage());
         for (int previous=0;previous<marker;++previous) require(markerImages[previous]!=markerImages.back(),"Distinct XY marker shapes rendered identically");
      }
      QVector<QImage> lineImages;
      for (int style:{100,101,102,103,104,106}) {
         receiver->XyPlotCurveSettings("IterationCheck",true,1,style,false,14,0,false,0);
         lineImages.append(stylePlot->canvas()->captureImage());
         for (int previous=0;previous<lineImages.size()-1;++previous) require(lineImages[previous]!=lineImages.back(),"Distinct XY line styles rendered identically");
      }
      receiver->AddXyPlotCurve("IterationCheck",1,"unchanged",0x00ff00);
      receiver->UpdateXyPlotCurve("IterationCheck",1,0,2);
      receiver->UpdateXyPlotCurve("IterationCheck",1,2,3);
      auto *styleAction=stylePlot->findChild<QAction *>("plotStyleAction");
      require(styleAction,"XY style controls missing");
      const auto beforeStyle=stylePlot->canvas()->captureImage();
      auto editStyle=[&](bool accept) {
         QTimer::singleShot(0,stylePlot,[&,accept] {
            auto *dialog=stylePlot->findChild<QDialog *>("plotStyleDialog");
            if (!dialog) return;
            auto *page=dialog->findChild<QWidget *>("curveStyle_0");
            page->findChild<QCheckBox *>("Lines")->setChecked(true);
            page->findChild<QCheckBox *>("Markers")->setChecked(true);
            page->findChild<QSpinBox *>("Line width")->setValue(4);
            page->findChild<QComboBox *>("Line style")->setCurrentIndex(1);
            page->findChild<QComboBox *>("Marker shape")->setCurrentIndex(5);
            page->findChild<QPushButton *>("Curve color")->setProperty("selectedColor",QColor(Qt::magenta));
            dialog->findChild<QCheckBox *>("plotLegend")->setChecked(false);
            if (accept) dialog->accept(); else dialog->reject();
         });
         styleAction->trigger();
      };
      editStyle(false);
      require(stylePlot->canvas()->captureImage()==beforeStyle,"Cancel altered XY appearance");
      editStyle(true);
      const auto modified=receiver->model("IterationCheck");
      require(modified->curves[0].width==4 && modified->curves[0].markerType==5 && modified->curves[0].lineStyle==101 &&
         modified->curves[0].points.front().color==QColor(Qt::magenta) && !modified->legend,
         "XY style dialog did not apply selected settings to existing points");
      require(modified->curves[1].width==1 && modified->curves[1].color!=QColor(Qt::magenta),"Style edit affected another curve");
      require(stylePlot->canvas()->captureImage()!=beforeStyle,"XY style controls did not change rendered appearance");
      receiver->DeleteXyPlot("IterationCheck");
      // Direct callback compatibility: the public script factory routes
      // GroundTrackPlot to GroundTrack, so it cannot exercise this older path.
      const auto previousView=receiver->GetViewType();
      receiver->SetViewType(GmatPlot::GROUND_TRACK_PLOT);
      require(receiver->CreateGlPlotWindow("CartesianTrack","",0,0,.5,.5,false,0),"Cartesian track callback failed to create");
      receiver->SetGlObject("CartesianTrack",{"SyntheticSat"},{});
      require(receiver->UpdateGlPlot("CartesianTrack","",{"SyntheticSat"},1,
         {1000},{1000},{1000},{0},{0},{0},{},{},false,0,true,true,false),"Cartesian track callback failed");
      receiver->CreateDynamicDataDisplay("WidthCheck","","Widths",0,0,.5,.5);
      std::vector<std::vector<DDD>> widthCells(1,std::vector<DDD>(2));
      widthCells[0][0].paramName="Short"; widthCells[0][0].paramValue="1";
      widthCells[0][1].paramName=std::string(300,'W');
      receiver->UpdateDynamicDataDisplay("WidthCheck",widthCells); receiver->show("WidthCheck");
      auto *widthTable=qobject_cast<QTableWidget *>(area->activeSubWindow()->widget());
      require(widthTable && widthTable->columnWidth(1)<1000,"Long dynamic label produced an unbounded column");
      require(widthTable->horizontalHeader()->sectionResizeMode(0)==QHeaderView::Interactive,"Dynamic column cannot be dragged");
      widthTable->setColumnWidth(0,233);
      widthCells[0][0].paramValue="123456789.123456789";
      receiver->UpdateDynamicDataDisplay("WidthCheck",widthCells);
      require(widthTable->columnWidth(0)==233,"Dynamic update overwrote adjusted column width");
      const auto &cartesian=curve(*receiver->model("CartesianTrack"),"SyntheticSat").points.back();
      require(std::abs(cartesian.x-45)<1e-10 && std::abs(cartesian.y-35.264389682754654)<1e-10,
         "Cartesian compatibility callback has incorrect spherical projection");
      receiver->DeleteGlPlot("CartesianTrack");
      receiver->SetViewType(previousView);
      receiver->DeleteDynamicData("QtData","");
      receiver->DeleteGlPlot("QtSampledGround");
      receiver->DeleteGlPlot("QtLegacyGround");
      for (const auto &name:receiver->names()) receiver->show(name);
      auto *windowMenu=window.findChild<QMenu *>("windowMenu");
      require(windowMenu!=nullptr,"Window menu missing");
      windowMenu->aboutToShow();
      QAction *orbitEntry=nullptr;
      for (auto *action:windowMenu->actions())
         if (action->objectName()=="windowEntry" && action->data().toString()=="QtOrbit") orbitEntry=action;
      require(orbitEntry!=nullptr,"Orbit window not listed");
      orbitEntry->trigger();
      require(area->activeSubWindow()->property("plotName").toString()=="QtOrbit","Window menu did not activate selected plot");
      area->activeSubWindow()->showMinimized();
      orbitEntry->trigger();
      require(!area->activeSubWindow()->isMinimized(),"Window menu did not restore minimized plot");
      area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      orbitEntry->trigger(); // Menu entries may outlive a window closed by another event.
      windowMenu->aboutToShow();
      for (auto *action:windowMenu->actions()) require(action->data().toString()!="QtOrbit","Closed plot remained listed");
      require(receiver->show("QtOrbit"),"Plot could not reopen after menu selection");
      area->tileSubWindows(); QApplication::processEvents();
      const auto rendered=window.grab();
      require(std::abs(rendered.width()-window.width()*window.devicePixelRatioF())<=1,"Plot capture ignored physical display scale");
      if (!image.isEmpty()) require(rendered.save(image),"Plot screenshot failed");
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto hiddenEarth=source->toPlainText();
      hiddenEarth.replace("QtOrbit.Add = {QtSat, Earth};","QtOrbit.Add = {QtSat, Earth};\nQtOrbit.DrawObject = [true false];");
      source->setPlainText(hiddenEarth); require(window.buildScript(),"Visibility setup failed");
      auto change=[&](const QString &resource,const QString &property,const QString &value) {
         const auto error=window.applyResourceChanges(resource,{{property,value}},source->toPlainText());
         if (!error.isEmpty()) std::cerr<<resource.toStdString()<<": "<<error.toStdString()<<'\n';
         require(error.isEmpty(),"Subscriber list edit failed");
      };
      change("QtOrbit","Add","Earth, QtSat, Luna");
      require(Moderator::Instance()->GetConfiguredObject("QtOrbit")->GetStringArrayParameter("Add").size()==3,"Orbit list was not updated");
      const auto visibility=Moderator::Instance()->GetConfiguredObject("QtOrbit")->GetBooleanArrayParameter("DrawObject");
      require(visibility.size()==3 && !visibility[0] && visibility[1] && visibility[2],"List edit lost visibility by name");
      change("QtReport","Add","QtSat.ElapsedSecs, QtSat.EarthMJ2000Eq.X");
      const auto beforeInvalid=source->toPlainText();
      for (const auto &bad : {"NoSuchSpacecraft", "Moon", "QtSat; Stop;", "QtSat, QtSat", "QtSat,"}) {
         require(!window.applyResourceChanges("QtOrbit",{{"Add",bad}},beforeInvalid).isEmpty(),"Invalid list accepted");
         require(source->toPlainText()==beforeInvalid,"Invalid list changed the script");
      }
      require(!window.applyResourceChanges("QtXY",{{"YVariables","QtSat.NoSuchParameter"}},beforeInvalid).isEmpty(),"Unknown plot parameter accepted");
      require(source->toPlainText()==beforeInvalid,"Failed parameter validation lost prior script");
      auto *resources=window.findChild<QTreeWidget *>("Resources");
      const auto xyItems=resources->findItems("QtXY",Qt::MatchExactly|Qt::MatchRecursive);
      require(xyItems.size()==1,"XY resource missing");
      resources->itemDoubleClicked(xyItems.first(),0);
      ResourceEditor *panel=nullptr;
      for (auto *widget:window.findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<ResourceEditor *>(widget)) panel=candidate;
      require(panel!=nullptr,"XY resource panel missing");
      auto *properties=panel->findChild<QTableWidget *>(); bool foundList=false;
      for (int row=0;row<properties->rowCount();++row) if (properties->item(row,0)->text()=="YVariables") {
         require(properties->item(row,1)->text().contains(".Y"),"List did not show current entries");
         properties->item(row,1)->setText("QtSat.EarthMJ2000Eq.Z"); foundList=true;
      }
      require(foundList,"YVariables list unavailable in panel");
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      require(window.runMission()==MainWindow::RunResult::Completed,"Edited subscriber lists failed to run");
      require(receiver->model("QtXY")->curves.size()==1,"List replacement appended to old XY entries");
      const auto &edited=receiver->model("QtXY")->curves.constBegin().value().points.back();
      require(std::abs(edited.y-z)<1e-7,"Edited XY list did not plot selected Z parameter");
      require(curve(*receiver->model("QtOrbit"),"Luna").points.size()>0,"Added orbit body not published");
      const auto reportNames=Moderator::Instance()->GetConfiguredObject("QtReport")->GetStringArrayParameter("Add");
      require(reportNames.size()==2 && reportNames[0]=="QtSat.ElapsedSecs","Report parameter list not replaced");
      {
         const auto before=source->toPlainText(); QWidget owner; QString error="No Apply";
         ResourceEditor colors(*Moderator::Instance()->GetConfiguredObject("QtSat"),
            [&](const QMap<QString,QString> &changes) { error=window.applyResourceChanges("QtSat",changes,before); return error; },&owner);
         auto pickColor=[&](const QString &field,const QColor &color,bool accept) {
            auto *button=colors.findChild<QPushButton *>("chooseProperty_"+field); require(button,"Spacecraft color picker missing");
            QTimer::singleShot(0,&colors,[&colors,color,accept] {
               auto *dialog=colors.findChild<QColorDialog *>("resourceColorDialog"); if (!dialog) return;
               dialog->setCurrentColor(color); if (accept) dialog->accept(); else dialog->reject();
            }); button->click();
         };
         const QColor orbitColor(23,145,210),targetColor(180,30,90);
         pickColor("OrbitColor",orbitColor,false); require(!colors.hasChanges(),"Color Cancel changed pending resource");
         pickColor("OrbitColor",orbitColor,true); pickColor("TargetColor",targetColor,true);
         require(colors.hasChanges() && source->toPlainText()==before,"Color picker applied prematurely");
         colors.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty(),qPrintable(error));
         const auto selected=source->toPlainText();
         source->undo(); require(source->toPlainText()==before && window.buildScript(),"Color Undo failed");
         source->redo(); require(window.buildScript(),"Color Redo failed");
         require(!window.applyResourceChanges("QtSat",{{"OrbitColor","[256 0 0]"}},selected).isEmpty() && source->toPlainText()==selected,"Invalid RGB color did not roll back");
         require(window.saveScriptTo(output.filePath("colors.script")) && window.loadScript(output.filePath("colors.script")) &&
            window.runMission()==MainWindow::RunResult::Completed,"Color save/reopen or run failed");
         require(curve(*receiver->model("QtOrbit"),"QtSat").color==orbitColor &&
            curve(*receiver->model("QtOrbit"),"QtSat").points.back().color==orbitColor,"Selected orbit color did not reach the viewer");
         auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("QtSat"));
         require(sat && QColor::fromRgb(sat->GetCurrentTargetColor() & 0xffffff)==targetColor,"Target color changed during round trip");
      }
      change("QtReport","Add","");
      require(Moderator::Instance()->GetConfiguredObject("QtReport")->GetStringArrayParameter("Add").empty(),"Clearing report list retained entries");
      {
         auto history=std::make_shared<PlotModel>(PlotModel::Kind::GroundTrack);
         history->maxPoints=3; history->legend=false; history->labels=false;
         auto append=[&](int frame) { history->frame=frame; history->append(0,frame,0); };
         for (int frame=1;frame<=3;++frame) append(frame);
         PlotWidget replayPlot(history); replayPlot.resize(600,400); replayPlot.show(); app.processEvents();
         auto *slider=replayPlot.findChild<QSlider *>();
         auto *play=replayPlot.findChild<QAction *>("plotReplayPlay"); auto *timer=replayPlot.findChild<QTimer *>("plotReplayTimer");
         auto *speed=replayPlot.findChild<QComboBox *>("plotReplaySpeed");
         require(play && timer && speed && replayPlot.findChild<QWidget *>("plotPlaybackControls"),"Separate playback controls missing");
         replayPlot.resize(330,400); app.processEvents();
         require(replayPlot.width()==330,"Playback controls forced a tiled plot wider than requested");
         require(slider->isVisible() && slider->width()>=100 && speed->isVisible(),"Narrow plot hid playback timeline or speed");
         require(replayPlot.rect().contains(slider->mapTo(&replayPlot,slider->rect().bottomRight())) &&
                 replayPlot.rect().contains(speed->mapTo(&replayPlot,speed->rect().bottomRight())),"Playback controls extend outside narrow plot");
         slider->setValue(400); play->trigger();
         require(slider->value()==400 && timer->isActive() && play->text()=="Pause","Playback resumed from start instead of selected position");
         play->trigger(); require(slider->value()==400 && !timer->isActive(),"Pause lost playback position");
         speed->setCurrentIndex(4); play->trigger(); QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection);
         require(slider->value()==440,"Playback speed control did not change replay advance");
         slider->sliderPressed(); require(!timer->isActive() && !play->isChecked(),"Scrubbing did not pause replay");
         replayPlot.findChild<QAction *>("plotReplayStart")->trigger(); require(slider->value()==0,"Start did not rewind replay");
         replayPlot.findChild<QAction *>("plotReplayLatest")->trigger(); require(slider->value()==1000 && !timer->isActive(),"Latest did not restore live end of history");
         play->trigger(); require(slider->value()==0,"Play at end did not restart history");
         slider->setValue(990); QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection);
         require(slider->value()==1000 && !play->isChecked() && !timer->isActive(),"Replay failed to stop at history end");
         speed->setCurrentIndex(0); slider->setValue(400); play->trigger();
         QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection);
         QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection);
         require(slider->value()==405,"Quarter-speed replay lost fractional advances");
         play->trigger();
         slider->setValue(0);
         for (int frame=4;frame<=6;++frame) append(frame);
         replayPlot.refresh();
         auto redPixels=[](const QImage &image) {
            int count=0;
            for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) {
               const auto color=image.pixelColor(x,y);
               if (color.red()>180 && color.green()<80 && color.blue()<80) ++count;
            }
            return count;
         };
         require(redPixels(replayPlot.canvas()->captureImage())>10,"Trimmed replay selection left the viewer empty");
         history->clear(); append(1); replayPlot.refresh();
         require(slider->value()==1000,"Cleared plot retained a stale replay position");
      }
      std::cout<<"PASS: subscriber lists, GUI Apply, actual XY Z output, added orbit body, visibility by name, empty lists and invalid inputs\n";
      std::cout<<"PASS: real orbit/XY/geodetic samples, map, dateline, sparse data, close-during-run/reopen/rerun, bounded history, replay retention, reversible zoom, dynamic values/colors, Output report viewer\n";
      xyStyleWidgetChecks();
   } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
