#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "OrbitRenderer.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QDataStream>
#include <QCryptographicHash>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTest>
#include <QToolTip>
#include <QRegularExpression>
#include <QWheelEvent>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Plot presentation artifact missing"); return f.readAll(); }
static void write(const QString &path,const QByteArray &value) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(value)==value.size(),"Plot presentation artifact write failed"); }
static QByteArray raw(const PlotModel &model) { QByteArray value; QDataStream stream(&value,QIODevice::WriteOnly); for (const auto &curve:model.curves) for (const auto &p:curve.points) stream<<p.frame<<p.epoch<<p.x<<p.y<<p.z<<p.connect<<p.solver; return value; }
static QString hash(const QByteArray &value) { return QString::fromLatin1(QCryptographicHash::hash(value,QCryptographicHash::Sha256).toHex()); }
static void wheel(QWidget *target,int delta) { const QPointF pos=target->rect().center(); QWheelEvent event(pos,target->mapToGlobal(pos.toPoint()),{},QPoint(0,delta),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false); QApplication::sendEvent(target,&event); events(); }
// The QWidget QtTest overload warps QCursor through the platform plugin; on a
// scaled private desktop that can deliver different logical coordinates. Route
// through QWindow so Qt dispatches the requested logical input to the canvas.
// This is widget/window input coverage, not native OS cursor qualification.
static void move(QWidget *target,QPoint pos) { QTest::mouseMove(target->window()->windowHandle(),target->mapTo(target->window(),pos)); events(); }
static void drag(QWidget *target,QPoint start,QPoint offset,Qt::KeyboardModifiers modifiers=Qt::NoModifier) { QTest::mousePress(target,Qt::LeftButton,modifiers,start); QMouseEvent move(QEvent::MouseMove,start+offset,target->mapToGlobal(start+offset),Qt::NoButton,Qt::LeftButton,modifiers); QApplication::sendEvent(target,&move); QTest::mouseRelease(target,Qt::LeftButton,modifiers,start+offset); events(); }

int main(int argc,char **argv)
{
 QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("NasaPlotPresentation");
 try {
  require(argc==3&&acceptanceWidgetPlatform(),"Expected startup and plot presentation case");
  const QString mode=argv[2],startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Plot presentation temporary directory unavailable");
  MainWindow window; require(window.initialize(startup),"Plot presentation runtime initialization failed"); window.show(); acceptanceView(window); events();
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *area=window.findChild<QMdiArea *>("workspace"); require(source&&area,"Plot presentation workspace missing");
  const auto report=files.filePath("Plot witness Δ.csv"); const QString literal="Create Variable Keep;\nKeep = 9;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nBeginMissionSequence;\nReport Values Keep;\n";
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"); auto save=[&](const QString &name,const QByteArray &value) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),value); };
  auto image=[&](const QString &name,const QImage &value) { require(!value.isNull(),"Plot image unavailable"); if (!evidence.isEmpty()) require(value.save(QDir(evidence).filePath(name+".png")),"Plot image save failed"); };
  QJsonArray observations; bool missionWitness=false; const bool epoch=mode.startsWith("Epoch"),orbit=mode=="EpochOrbit"||mode=="OrbitPointer";
  if (epoch) {
   const QString mission="Create Variable Keep;\nKeep = 9;\nCreate Spacecraft Sat;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2000 12:00:00.000';\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 5;\nProp.MinStep = 5;\nProp.MaxStep = 5;\nCreate OrbitView Orb;\nOrb.Add = {Sat, Earth};\nCreate GroundTrackPlot Map;\nMap.Add = {Sat};\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 300};\nReport Values Sat.ElapsedSecs Keep;\n";
   source->setPlainText(mission); require(window.buildScript()&&window.runMission()==MainWindow::RunResult::Completed,"Real plot epoch mission failed"); save("mission.script",mission.toUtf8()); save("mission.csv",read(report)); const auto result=QString::fromUtf8(read(report)).trimmed().split(','); require(result.size()==2&&std::abs(result[0].toDouble()-300)<1e-5&&result[1].trimmed()=="9","Real plot epoch mission witness wrong"); missionWitness=true;
   const auto name=orbit ? QString("Orb") : QString("Map"); require(window.plotReceiver()->show(name),"Real plot output did not open"); events(); auto *real=dynamic_cast<PlotWidget *>(area->currentSubWindow()->widget()); require(real,"Real plot viewer missing"); real->refresh(); events(); acceptanceView(*real); auto *canvas=real->canvas(); require(canvas->accessibleDescription()=="UTC: 01 Jan 2000 12:05:00.000","Real mission plot did not display expected UTC Gregorian epoch"); image("real-mission",canvas->captureImage()); observations.append(QJsonObject{{"kind","real-mission"},{"caption",canvas->accessibleDescription()},{"elapsed",300}});
  } else { source->setPlainText(literal); require(window.buildScript(),"Plot literal fixture failed"); }
  const auto stableSource=source->toPlainText(); const int stableUndo=source->document()->availableUndoSteps();
  auto model=std::make_shared<PlotModel>(orbit ? PlotModel::Kind::Orbit : PlotModel::Kind::GroundTrack);
  model->title="Plot presentation Δ"; model->legend=false; model->axes=false; model->grid=false; model->labels=false; model->sunlight=false; model->endOfRun=true;
  model->curves[0].name="Probe"; model->curves[0].color=Qt::red; model->curves[0].radius=orbit ? .1 : 0; model->curves[0].lines=mode=="OrbitPointer";
  // Independent literal UTC reference: TAI-UTC32s and A1-TAI.0343817s at J2000.
  const double base=21545+32.0343817/86400.; const double times[]={base,base+300/86400.,base-1};
  for (int i=0;i<3;++i) { model->frame=i+1; model->append(0,mode=="OrbitPointer" ? i*2-2 : 0,mode=="OrbitPointer" ? i-1 : 0,mode=="OrbitPointer" ? i%2 : 0,times[i]); if (orbit) model->cameras.push_back({quint64(i+1),{0,0,10},{0,0,0},{0,1,0},false}); }
  if (orbit) model->scriptedCamera=true; else { model->map=QImage(400,200,QImage::Format_RGB32); model->map.fill(QColor(20,100,160)); }
  const auto original=raw(*model); auto *viewer=new PlotWidget(model); auto *child=area->addSubWindow(viewer); child->showNormal(); child->setGeometry(area->viewport()->rect()); area->setActiveSubWindow(child); child->raise(); viewer->setFocus(); events(); acceptanceView(*child); auto *canvas=viewer->canvas(); canvas->refresh(); events();
  auto visibleCanvas=[&](PlotCanvas *target) { const QRect box(target->mapTo(area->viewport(),QPoint()),target->size()); require(area->activeSubWindow()==child&&target->isVisible()&&area->viewport()->rect().contains(box),"Plot canvas and footer are not fully inside the visible workspace"); }; visibleCanvas(canvas);
  const bool settled=QTest::qWaitFor([&] { return area->activeSubWindow()==child && canvas->width()>400 && canvas->height()>200; },3000); std::cout<<"LAYOUT canvas="<<canvas->width()<<","<<canvas->height()<<" viewer="<<viewer->width()<<","<<viewer->height()<<" child="<<child->width()<<","<<child->height()<<" viewport="<<area->viewport()->width()<<","<<area->viewport()->height()<<" active="<<(area->activeSubWindow()==child)<<std::endl; require(settled,"Plot layout did not reach a readable settled size"); events(); visibleCanvas(canvas);
  if (orbit&&qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") { auto *native=dynamic_cast<OrbitRenderer *>(canvas->findChild<QOpenGLWidget *>()); require(native&&native->isValid()&&native->testAttribute(Qt::WA_TransparentForMouseEvents),"Visible orbit renderer not valid or blocks canvas input"); }
  auto frame=[&](quint64 value,const QString &expected,const QString &label) { canvas->setFrame(value); events(); acceptanceView(*child); visibleCanvas(canvas); const auto captured=canvas->captureImage(); require(canvas->accessibleDescription()==expected,"Plot UTC caption differs from independent literal epoch"); if (auto *native=canvas->findChild<QOpenGLWidget *>()) require(native->accessibleDescription()==expected,"Native rendered UTC caption differs from independent literal epoch"); image(label,captured); observations.append(QJsonObject{{"kind",label},{"caption",canvas->accessibleDescription()}}); return captured.copy(0,captured.height()-qRound(28*captured.devicePixelRatio()),captured.width(),qRound(22*captured.devicePixelRatio())); };
  if (epoch) {
   const auto first=frame(1,"UTC: 01 Jan 2000 12:00:00.000","first"),next=frame(2,"UTC: 01 Jan 2000 12:05:00.000","next"); require(first!=next,"UTC caption text did not change rendered footer pixels");
   frame(3,"UTC: 31 Dec 1999 12:00:00.000","backward"); frame(std::numeric_limits<quint64>::max(),"UTC: 31 Dec 1999 12:00:00.000","latest-backward");
   canvas->setReplayCursor({1,2,.5,base+150/86400.}); events(); acceptanceView(*child); require(canvas->accessibleDescription()=="UTC: 01 Jan 2000 12:02:30.000","Interpolated cursor UTC caption wrong"); observations.append(QJsonObject{{"kind","cursor"},{"caption",canvas->accessibleDescription()}}); image("cursor",canvas->captureImage());
   canvas->setSimulationEpoch(base+60/86400.,PlotReplayMode::InterpolatedSteps); events(); acceptanceView(*child); require(canvas->accessibleDescription()=="UTC: 01 Jan 2000 12:01:00.000","Absolute clock UTC caption wrong"); observations.append(QJsonObject{{"kind","clock"},{"caption",canvas->accessibleDescription()}}); image("clock",canvas->captureImage());
   canvas->setFrame(std::numeric_limits<quint64>::max()); auto bad=model->curves[0].points.back(); bad.frame=4; bad.epoch=std::numeric_limits<double>::quiet_NaN(); model->curves[0].points.push_back(bad); canvas->refresh(); events(); require(canvas->accessibleDescription()=="UTC epoch unavailable","Invalid epoch retained a stale successful caption"); model->curves[0].points.pop_back(); const auto saved=model->curves; model->curves.clear(); canvas->refresh(); events(); acceptanceView(*child); require(canvas->accessibleDescription()=="UTC epoch unavailable","Empty plot invented an epoch"); image("empty",canvas->captureImage()); model->curves=saved; canvas->refresh(); events(); observations.append(QJsonObject{{"kind","empty-invalid"},{"caption","UTC epoch unavailable"}});
  } else if (mode=="OrbitPointer") {
   canvas->scriptView(); events(); const auto before=canvas->captureImage(); image("pointer-before",before); wheel(canvas,120); require(std::abs(canvas->zoomFactor()-1.15)<1e-12,"Actual wheel input did not zoom orbit"); acceptanceView(*child); visibleCanvas(canvas); const auto zoomed=canvas->captureImage(); require(zoomed!=before,"Actual wheel input left orbit pixels unchanged"); image("pointer-zoom",zoomed); wheel(canvas,-120); require(std::abs(canvas->zoomFactor()-1)<1e-12,"Reverse wheel input did not restore orbit scale");
   drag(canvas,canvas->rect().center(),QPoint(30,-20)); require(model->userView&&std::abs(model->userView->yaw-.24)<1e-12&&std::abs(model->userView->pitch+.16)<1e-12,"Actual drag rotation differs from independent angular increment"); const auto rotated=canvas->captureImage(); require(rotated!=before,"Actual drag left orbit pixels unchanged"); image("pointer-rotated",rotated); drag(canvas,canvas->rect().center(),QPoint(17,11),Qt::ShiftModifier); require(model->userView->pan==QPointF(17,11),"Shift-drag did not pan independently of rotation"); acceptanceView(*child); observations.append(QJsonObject{{"kind","pointer"},{"wheel_factor",1.15},{"yaw",model->userView->yaw},{"pitch",model->userView->pitch},{"pan_x",model->userView->pan.x()},{"pan_y",model->userView->pan.y()}});
  } else {
   require(mode=="HoverBase"||mode=="HoverResize"||mode=="HoverPanZoom","Unknown plot presentation case");
   model->curves.clear(); canvas->refresh(); events();
   auto bounds=[&](const QString &label) { visibleCanvas(canvas); const auto captured=canvas->captureImage(); image(label,captured); int left=captured.width(),right=-1,top=captured.height(),bottom=-1; for (int y=0;y<captured.height();++y) for (int x=0;x<captured.width();++x) if (captured.pixelColor(x,y)==QColor(20,100,160)) { left=std::min(left,x); right=std::max(right,x); top=std::min(top,y); bottom=std::max(bottom,y); } require(right-left>100&&bottom-top>50,"Independent map-pixel calibration missing"); const double dpr=captured.devicePixelRatio(); std::cout<<"MAP_CALIBRATION "<<label.toStdString()<<" canvas="<<canvas->width()<<","<<canvas->height()<<" pixels="<<captured.width()<<","<<captured.height()<<" dpr="<<dpr<<" bounds="<<left<<","<<top<<","<<right<<","<<bottom<<std::endl; return QRectF(left/dpr,top/dpr,(right-left+1)/dpr,(bottom-top+1)/dpr); };
   const auto calibration=bounds("map-before"); double zoom=1; QPoint offset;
   if (mode=="HoverResize") { child->showNormal(); const auto size=area->viewport()->size(); child->setGeometry(0,0,size.width()-120,size.height()-60); events(); acceptanceView(*child); visibleCanvas(canvas); }
   if (mode=="HoverPanZoom") { wheel(canvas,120); zoom=1.15; require(std::abs(canvas->zoomFactor()-zoom)<1e-12,"Ground wheel input did not zoom"); offset=QPoint(25,15); drag(canvas,canvas->rect().center(),offset); events(); }
   const auto map=mode=="HoverResize" ? bounds("map-resized") : calibration;
   auto hover=[&](QPoint pos,double longitude,double latitude) { move(canvas,pos); acceptanceView(*child); visibleCanvas(canvas); std::cout<<"HOVER_REQUEST "<<pos.x()<<","<<pos.y()<<" canvas="<<canvas->width()<<","<<canvas->height()<<" tooltip="<<canvas->toolTip().toStdString()<<" visible="<<QToolTip::isVisible()<<std::endl; const auto match=QRegularExpression("Longitude: ([+-]?[0-9.]+) deg · Latitude: ([+-]?[0-9.]+) deg").match(canvas->toolTip()); require(match.hasMatch()&&QToolTip::isVisible()&&QToolTip::text()==canvas->toolTip(),"Ground hover did not show visible geographic feedback"); const double error=(canvas->devicePixelRatioF()+2)*360/(map.width()*canvas->devicePixelRatioF()); require(std::abs(match.captured(1).toDouble()-longitude)<error&&std::abs(match.captured(2).toDouble()-latitude)<error,"Ground hover differs from independent map-pixel/input calibration"); observations.append(QJsonObject{{"kind","hover"},{"pixel_x",pos.x()},{"pixel_y",pos.y()},{"expected_longitude",longitude},{"expected_latitude",latitude},{"actual_longitude",match.captured(1).toDouble()},{"actual_latitude",match.captured(2).toDouble()},{"tolerance_degrees",error},{"map_left",map.left()},{"map_top",map.top()},{"map_width",map.width()},{"map_height",map.height()},{"zoom",zoom},{"pan_x",offset.x()},{"pan_y",offset.y()}}); };
   for (const auto &fraction:QList<QPointF>{{.5,.5},{.25,.25},{.75,.75}}) { const QPoint pos(qRound(map.left()+map.width()*fraction.x()),qRound(map.top()+map.height()*fraction.y())); const double lon=((pos.x()-map.center().x()-offset.x())/map.width())*360/zoom,lat=-((pos.y()-map.center().y()-offset.y())/map.height())*180/zoom; hover(pos,lon,lat); }
   move(canvas,QPoint(4,4)); require(!canvas->toolTip().startsWith("Longitude:"),"Map margin retained stale geographic feedback"); QEvent leave(QEvent::Leave); QApplication::sendEvent(canvas,&leave); require(!canvas->toolTip().startsWith("Longitude:"),"Map leave retained stale geographic feedback"); model->curves[0].name="Probe"; for (int i=0;i<3;++i) { model->frame=i+1; model->append(0,0,0,0,times[i]); } canvas->refresh(); events();
  }
  require(raw(*model)==original,"Plot presentation mutated retained samples"); require(source->toPlainText()==stableSource&&source->document()->availableUndoSteps()==stableUndo,"Plot presentation changed mission source/history");
  if (!missionWitness) { require(window.runMission()==MainWindow::RunResult::Completed&&QString::fromUtf8(read(report)).trimmed()=="9","Plot presentation damaged independent literal mission"); save("mission.csv",read(report)); }
  save("observations.json",QJsonDocument(observations).toJson()); save("samples.sha256",hash(original).toUtf8());
  const QJsonArray facets{"visible-plot-presentation-and-actual-input","independent-utc-or-map-pixel-gesture-oracle","rendered-pixels-or-visible-tooltip","unchanged-retained-samples-and-source-history","literal-mission-witness"};
  const QJsonObject proof{{"checkpoint","plot-presentation-"+mode},{"outcome","passed"},{"mode",mode},{"numeric_sentinel",9},{"sample_sha256",hash(original)},{"observations",observations},{"facets",facets},{"scope","Selected UTC Gregorian real-mission/frame/backward/cursor/clock/empty cases or literal native pointer and ground-map pixel calibration after resize/pan/zoom. No all epochs/leap transitions, arbitrary camera/map/domain, native OS input, host GPU/compositor/platform/stress or full current-build qualification."}}; save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
