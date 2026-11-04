#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotPlaybackControls.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QSlider>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QElapsedTimer>
#include <QThread>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <limits>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
static PlotWidget *plot(QtPlotReceiver &receiver,const QString &name)
{
   for (auto *window:receiver.workspaceArea()->subWindowList())
      if (window->property("plotName")==name) return dynamic_cast<PlotWidget *>(window->widget());
   throw std::runtime_error("Missing plot widget");
}
static void sharedPlaybackButtonChecks(MainWindow &window)
{
   auto &receiver=*window.plotReceiver(); receiver.clear();
   auto *controls=dynamic_cast<PlotPlaybackControls *>(window.findChild<QWidget *>("allPlotPlaybackControls")); require(controls,"Shared playback controls missing");
   auto button=[&](const char *name) { auto *value=controls->findChild<QToolButton *>(name); require(value && value->isVisible(),"Shared playback button missing or hidden"); return value; };
   auto *slider=controls->findChild<QSlider *>("allPlotTimeline"); auto *speed=controls->findChild<QComboBox *>("allPlotReplaySpeed");
   auto *mode=controls->findChild<QComboBox *>("allPlotReplayMode"); auto *timer=controls->findChild<QTimer *>("allPlotReplayTimer");
   require(slider && speed && mode && timer,"Shared playback state controls missing");
   receiver.CreateGlPlotWindow("ButtonOrbit","",0,0,0,0,false,0); receiver.SetGlObject("ButtonOrbit",{"Vehicle"},{});
   receiver.CreateGroundTrackWindow("ButtonGround","","Ground",0,0,0,0,false);
   receiver.CreateXyPlotWindow("ButtonXY","",0,0,0,0,false,"XY","x","y"); receiver.AddXyPlotCurve("ButtonXY",0,"Curve",0xff0000);
   for (int i=0;i<20;++i) receiver.UpdateGlPlot("ButtonOrbit","",{"Vehicle"},21545.+i,{7000.},{double(i)*100},{0.},{},{},{},{},{},false,0,true,true,false);
   for (int i=0;i<30;++i) { double point[]={double(i)*4-60,double(i)-15}; receiver.UpdateGroundTrackData("ButtonGround",21545.+i,point,1); }
   for (int i=0;i<40;++i) receiver.UpdateXyPlotCurve("ButtonXY",0,i,i*i);
   receiver.missionFinished(); QApplication::processEvents(); acceptanceView(*controls);
   require(controls->isEnabled(),"Retained data did not enable shared playback");
   mode->setCurrentIndex(0); speed->setCurrentIndex(2);
   auto position=[&](int value) {
      require(receiver.replayPosition()==value && slider->value()==value,"Shared button did not set expected percentage");
      for (const auto &name:{"ButtonOrbit","ButtonGround","ButtonXY"}) {
         require(plot(receiver,name)->findChild<QSlider *>("plotTimeline")->value()==value,"Child timeline did not follow the shared button");
         require(!plot(receiver,name)->findChild<QTimer *>("plotReplayTimer")->isActive(),"Shared button left a competing local timer");
      }
   };
   button("allPlotReplayFaster")->click(); require(speed->currentIndex()==3,"Faster widget did not increase speed");
   button("allPlotReplaySlower")->click(); require(speed->currentIndex()==2,"Slower widget did not decrease speed");
   button("allPlotReplayLatest")->click(); position(1000);
   const auto latest=plot(receiver,"ButtonXY")->canvas()->captureImage(); require(!latest.isNull(),"Latest XY image unavailable");
   button("allPlotReplayStart")->click(); position(0);
   const auto first=plot(receiver,"ButtonXY")->canvas()->captureImage(); require(!first.isNull() && first!=latest,"Start widget did not change the displayed XY history");
   slider->setValue(500); position(500);
   require(plot(receiver,"ButtonOrbit")->canvas()->frame()==10 && plot(receiver,"ButtonGround")->canvas()->frame()==15 && plot(receiver,"ButtonXY")->canvas()->frame()==20,"Shared scrub did not map unequal history lengths");
   speed->setCurrentIndex(0); require(!button("allPlotReplaySlower")->isEnabled(),"Minimum speed still permits Slower");
   button("allPlotReplayStart")->click(); button("allPlotReplayPlay")->click(); require(timer->isActive(),"Play widget did not start the real timer");
   QElapsedTimer deadline; deadline.start();
   while (slider->value()<20 && deadline.elapsed()<2000) { QApplication::processEvents(); QThread::msleep(5); }
   require(slider->value()>=20 && slider->value()<1000,"Real playback timer did not advance retained data");
   button("allPlotReplayPlay")->click(); require(!timer->isActive(),"Pause widget did not stop playback");
   const auto paused=slider->value(); position(paused); const auto pausedImage=plot(receiver,"ButtonXY")->canvas()->captureImage();
   deadline.restart(); while (deadline.elapsed()<100) { QApplication::processEvents(); QThread::msleep(5); }
   require(slider->value()==paused && plot(receiver,"ButtonXY")->canvas()->captureImage()==pausedImage,"Paused display advanced without a timer");
   button("allPlotReplayLatest")->click(); position(1000);
   require(!timer->isActive() && !button("allPlotReplayPlay")->isChecked(),"Latest widget left shared playback running");
   // Latest follows all retained data, represented by an unbounded frame sentinel.
   require(plot(receiver,"ButtonXY")->canvas()->frame()==std::numeric_limits<quint64>::max() && plot(receiver,"ButtonXY")->canvas()->captureImage()==latest,"Latest widget did not restore the full retained XY image");
   mode->setCurrentIndex(1); require(receiver.replayMode()==PlotReplayMode::InterpolatedSteps,"Interpolated mode did not reach receiver");
   mode->setCurrentIndex(0); require(receiver.replayMode()==PlotReplayMode::PropagationSteps,"Retained-step mode did not return");
   require(receiver.model("ButtonOrbit")->curves[0].points.size()==20 && receiver.model("ButtonGround")->curves[0].points.size()==30 && receiver.model("ButtonXY")->curves[0].points.size()==40,"Playback widgets mutated retained history lengths");
   const auto &last=receiver.model("ButtonXY")->curves[0].points.back(); require(last.x==39 && last.y==1521,"Playback widgets mutated the final XY sample");
   receiver.clear(); require(!controls->isEnabled() && !timer->isActive(),"Clear did not disable and stop shared transport");
   const QJsonObject proof{{"kind","shared-playback-buttons"},{"outcome","passed"},{"controls",QJsonArray{"allPlotReplayStart","allPlotReplayPlay","allPlotReplayLatest","allPlotReplaySlower","allPlotReplayFaster"}},
      {"checks",QJsonArray{"widget-clicks","known-frames","unequal-histories","real-timer","paused-image","speed","mode","preserved-samples","clear"}}};
   std::cout<<"GUI_ROUTE_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   // The existing regression below remains offscreen, including its original
   // guard and every assertion. Only this bounded companion uses a visible
   // software-rendered acceptance display; it does not qualify host hardware.
   if (argc==3 && QString::fromLocal8Bit(argv[2])=="--buttons") {
      try {
         require(acceptanceWidgetPlatform(),"Playback companion requires offscreen or supported acceptance widgets");
         if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") require(QGuiApplication::platformName()=="xcb" && qEnvironmentVariable("LIBGL_ALWAYS_SOFTWARE")=="1","Visible playback companion requires software-rendered X11 acceptance");
         require(argc>=2 && argc<=3,"Playback startup argument missing");
         QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("SharedPlaybackVisible"); TestSettings settings;
         const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
         MainWindow window; require(window.initialize(startup),"Visible playback initialization failed"); window.show();
         sharedPlaybackButtonChecks(window); return 0;
      } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   }
   // Desktop qualification is suspended after the recorded compositor crash.
   if (QGuiApplication::platformName()!="offscreen") return 2;
   if (argc<2 || argc>3) return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("SharedPlayback");
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings;
      MainWindow window; require(window.initialize(startup),"Engine initialization failed"); window.show(); app.processEvents();
      auto *controls=dynamic_cast<PlotPlaybackControls *>(window.findChild<QWidget *>("allPlotPlaybackControls"));
      auto *toolbar=window.findChild<QToolBar *>("standardToolbar");
      require(controls && toolbar && controls->parentWidget()==toolbar,"Animation missing from standard toolbar");
      auto *debug=window.findChild<QAction *>("debugMission"),*run=window.findChild<QAction *>("runMission");
      require(debug && run && !debug->icon().isNull(),"Missing Debug icon");
      require(debug->icon().pixmap(32,32).toImage()!=run->icon().pixmap(32,32).toImage(),"Debug still uses Run icon");
      require(debug->shortcut()==QKeySequence("Ctrl+F5"),"Debug shortcut changed");
      auto &receiver=*window.plotReceiver();
      auto *start=controls->findChild<QAction *>("allPlotReplayStart"),*play=controls->findChild<QAction *>("allPlotReplayPlay");
      auto *latest=controls->findChild<QAction *>("allPlotReplayLatest");
      auto *slider=controls->findChild<QSlider *>("allPlotTimeline");
      auto *timer=controls->findChild<QTimer *>("allPlotReplayTimer");
      auto *speed=controls->findChild<QComboBox *>("allPlotReplaySpeed");
      auto *position=controls->findChild<QLabel *>("allPlotReplayPosition");
      require(start && play && latest && slider && timer && speed && position,"Incomplete global transport");
      auto *mode=controls->findChild<QComboBox *>("allPlotReplayMode");
      auto *slower=controls->findChild<QAction *>("allPlotReplaySlower"),*faster=controls->findChild<QAction *>("allPlotReplayFaster");
      require(mode && slower && faster && mode->currentIndex()==1,"Missing playback modes/default or speed buttons");
      mode->setCurrentIndex(0); // Retained-step coverage remains distinct from epoch interpolation.
      require(receiver.replayMode()==PlotReplayMode::PropagationSteps,"Global mode did not reach receiver");
      require(!controls->isEnabled(),"Empty transport enabled");
      receiver.CreateDynamicDataDisplay("Table","","Live values",0,0,0,0);
      require(!controls->isEnabled(),"Table alone enabled retained animation");
      receiver.CreateGlPlotWindow("Orbit","",0,0,0,0,false,0);
      receiver.SetGlObject("Orbit",{"Vehicle"},{});
      receiver.CreateGroundTrackWindow("Ground","","Ground",0,0,0,0,false);
      receiver.CreateXyPlotWindow("XY","",0,0,0,0,false,"XY","x","y");
      receiver.AddXyPlotCurve("XY",0,"Curve",0xff0000);
      for (int i=0;i<20;++i) {
         receiver.UpdateGlPlot("Orbit","",{"Vehicle"},21545.+i,{7000.},{double(i)*100},{0.},{},{},{},{},{},false,0,true,true,false);
      }
      for (int i=0;i<40;++i) { double point[]={double(i)*4-80,double(i)-20}; receiver.UpdateGroundTrackData("Ground",21545.+i,point,1); }
      for (int i=0;i<10;++i) receiver.UpdateXyPlotCurve("XY",0,i,i*i);
      receiver.missionFinished(); app.processEvents();
      require(controls->isEnabled(),"Plot transport disabled");
      faster->trigger(); require(speed->currentIndex()==3,"Faster button did not increase playback speed");
      slower->trigger(); require(speed->currentIndex()==2,"Slower button did not decrease playback speed");
      speed->setCurrentIndex(0); require(!slower->isEnabled() && faster->isEnabled(),"Minimum-speed button state incorrect");
      speed->setCurrentIndex(4); require(!faster->isEnabled() && slower->isEnabled(),"Maximum-speed button state incorrect");
      speed->setCurrentIndex(2);
      for (const auto &name:{"Orbit","Ground","XY"}) require(!receiver.model(name)->curves[0].points.empty(),"Synthetic plot has no history");
      auto checkPosition=[&](int value) {
         require(receiver.replayPosition()==value,"Receiver lost shared position");
         for (const auto &name:{"Orbit","Ground","XY"}) {
            auto *widget=plot(receiver,name);
            require(widget->findChild<QSlider *>("plotTimeline")->value()==value,"Plot position diverged from master");
            require(!widget->findChild<QTimer *>("plotReplayTimer")->isActive(),"Competing local animation timer");
         }
      };
      latest->trigger(); checkPosition(1000);
      const auto xyLatest=plot(receiver,"XY")->canvas()->captureImage();
      start->trigger(); checkPosition(0);
      require(plot(receiver,"XY")->canvas()->captureImage()!=xyLatest,"XY render ignored shared history");
      slider->setValue(500); checkPosition(500);
      require(plot(receiver,"Orbit")->canvas()->frame()==10 && plot(receiver,"Ground")->canvas()->frame()==20 && plot(receiver,"XY")->canvas()->frame()==5,"Unequal histories mapped incorrectly");
      play->trigger(); require(timer->isActive() && play->isChecked(),"Master playback did not start");
      QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection); checkPosition(510);
      speed->setCurrentIndex(4); QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection); checkPosition(550);
      play->trigger(); require(!timer->isActive(),"Master pause failed");
      play->trigger(); require(slider->value()==550,"Resume reset position"); play->trigger();
      slider->setValue(990); play->trigger(); QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection);
      checkPosition(1000); require(!timer->isActive() && !play->isChecked(),"Playback did not finish at Latest");
      play->trigger(); checkPosition(0); play->trigger();
      speed->setCurrentIndex(0); play->trigger();
      QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection); checkPosition(2);
      QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection); checkPosition(5); play->trigger();
      slider->setValue(750);
      auto *closed=dynamic_cast<QMdiSubWindow *>(plot(receiver,"Orbit")->parentWidget());
      require(closed && closed->close(),"Plot close failed"); app.processEvents();
      slider->setValue(250); require(receiver.show("Orbit"),"Reopen failed"); checkPosition(250);
      require(plot(receiver,"Orbit")->canvas()->frame()==5,"Reopened plot used stale frame");
      play->trigger();
      plot(receiver,"Ground")->findChild<QAction *>("plotReplayPlay")->trigger();
      require(!receiver.replayPosition() && !timer->isActive() && !play->isChecked() && position->text()=="Local","Local playback did not release master");
      require(plot(receiver,"Ground")->findChild<QTimer *>("plotReplayTimer")->isActive(),"Local playback stopped unexpectedly");
      latest->trigger(); checkPosition(1000);
      // Latest must take ownership again even when the master slider is already 1000.
      plot(receiver,"Ground")->findChild<QAction *>("plotReplayStart")->trigger();
      require(!receiver.replayPosition(),"Local Start retained global ownership");
      latest->trigger(); checkPosition(1000);
      slider->setValue(500); receiver.UpdateXyPlotCurve("XY",0,10,100); checkPosition(500);
      require(plot(receiver,"XY")->canvas()->frame()==6,"Incoming data lost shared frame position");
      require(receiver.model("Orbit")->curves[0].points.size()==20 && receiver.model("Ground")->curves[0].points.size()==40 && receiver.model("XY")->curves[0].points.size()==11,"Playback mutated retained history");
      receiver.ClearXyPlotData("XY"); receiver.RefreshXyPlot("XY"); checkPosition(500);
      require(plot(receiver,"XY")->canvas()->frame()==0,"Cleared history retained old frame");
      require(window.findChild<QTableWidget *>(),"Global playback removed live table");
      latest->trigger(); app.processEvents();
      require(controls->isVisible() && toolbar->rect().contains(controls->geometry()),"Transport does not fit default toolbar");
      if (argc==3) require(toolbar->grab().save(QFileInfo(argv[2]).absoluteFilePath()),"Toolbar capture failed");
      play->trigger(); receiver.clear();
      require(!controls->isEnabled() && !timer->isActive() && !receiver.replayPosition() && slider->value()==1000,"Clear left stale master state");
      receiver.CreateXyPlotWindow("Rerun","",0,0,0,0,false,"XY","x","y");
      require(controls->isEnabled() && !play->isChecked(),"New run inherited playback");
      receiver.clear();
      // Current solver mode removes trial publications without renumbering the
      // retained samples. Replay must skip those holes, including a removed tail.
      receiver.CreateGlPlotWindow("SparseOrbit","",0,0,0,0,false,0);
      receiver.SetGlObject("SparseOrbit",{"Vehicle"},{});
      receiver.CreateGroundTrackWindow("SparseGround","","Ground",0,0,0,0,false);
      receiver.CreateXyPlotWindow("SparseXY","",0,0,0,0,false,"XY","x","y");
      receiver.AddXyPlotCurve("SparseXY",0,"Curve",0xff0000);
      auto publish=[&](int index,bool solver) {
         receiver.UpdateGlPlot("SparseOrbit","",{"Vehicle"},21545.+index,{7000.},{double(index)},{0.},{},{},{},{},{},solver,0,false,true,false);
         receiver.TakeGroundTrackAction("SparseGround",solver ? "SolverData=On" : "SolverData=Off");
         double point[]={double(index),0};
         receiver.UpdateGroundTrackData("SparseGround",21545.+index,point,1);
         receiver.UpdateXyPlotCurve("SparseXY",0,index,index);
      };
      auto markTrials=[&] { receiver.XyPlotMarkBreak("SparseXY"); };
      auto discardTrials=[&] {
         require(receiver.TakeGlAction("SparseOrbit","ClearSolverData"),"Orbit trial cleanup failed");
         require(receiver.TakeGroundTrackAction("SparseGround","ClearSolverData"),"Ground trial cleanup failed");
         receiver.XyPlotClearFromBreak("SparseXY",-1,-1,-1);
      };
      publish(1,false); publish(2,false); markTrials();
      for (int i=3;i<=202;++i) publish(i,true);
      discardTrials();
      publish(203,false); publish(204,false); publish(205,false);
      receiver.missionFinished();
      slider->setValue(500);
      for (const auto &name:{"SparseOrbit","SparseGround","SparseXY"})
         require(plot(receiver,name)->canvas()->frame()==203,"Replay paused in discarded solver frames between segments");
      markTrials(); for (int i=206;i<=405;++i) publish(i,true);
      receiver.setReplayPosition(500);
      require(plot(receiver,"SparseOrbit")->canvas()->frame()==303,"Incoming trial frames did not invalidate replay index");
      discardTrials(); receiver.setReplayPosition(500);
      for (const auto &name:{"SparseOrbit","SparseGround","SparseXY"}) {
         const auto model=receiver.model(name);
         require(model->frame==405 && model->curves[0].points.size()==5,"Replay changed source history or trial cleanup");
         require(plot(receiver,name)->canvas()->frame()==203,"Removed trailing solver frames still occupied replay time");
      }
      // Local controls use the same retained index; reopened views rebuild it.
      auto *local=plot(receiver,"SparseOrbit")->findChild<QSlider *>("plotTimeline");
      local->setValue(750);
      require(!receiver.replayPosition() && plot(receiver,"SparseOrbit")->canvas()->frame()==204,"Local replay still used cumulative publication count");
      receiver.setReplayPosition(500);
      require(dynamic_cast<QMdiSubWindow *>(plot(receiver,"SparseOrbit")->parentWidget())->close(),"Sparse viewer close failed");
      app.processEvents(); require(receiver.show("SparseOrbit"),"Sparse viewer reopen failed");
      require(plot(receiver,"SparseOrbit")->canvas()->frame()==203,"Reopened replay lost retained frame selection");
      receiver.SetMaxGlDataPoints("SparseOrbit",3); receiver.setReplayPosition(0);
      require(plot(receiver,"SparseOrbit")->canvas()->frame()==203,"Replay index did not follow retained-point trimming");
      receiver.setReplayPosition(500);
      require(plot(receiver,"SparseOrbit")->canvas()->frame()==204,"Trimmed retained replay midpoint incorrect");
      receiver.clear();
      std::cout << "PASS: discarded solver gaps/tails, Orbit/Ground/XY retained replay, local/shared controls, cache updates, trimming and reopen.\n";
      std::cout << "PASS: distinct Debug bug icon; shared Orbit/Ground/XY Start, Play/Pause, Latest, speed, scrubbing, unequal histories, incoming/reset data, local override, close/reopen, clear/rerun; table excluded; source histories retained. Offscreen only.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
