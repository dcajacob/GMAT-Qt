#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotPlaybackControls.hpp"
#include "PlotWidget.hpp"
#include "OrbitCamera.hpp"
#include "OrbitObjectAxes.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDataStream>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <cmath>
#include <iostream>
#include <stdexcept>

#undef near // Legacy Windows memory-model macro conflicts with the assertion helper.
static void require(bool ok,const char *why) { if (!ok) throw std::runtime_error(why); }
static void near(double actual,double expected,const char *why) { require(std::abs(actual-expected)<2e-5,why); }
static PlotWidget *plot(QtPlotReceiver &receiver,const QString &name)
{
   for (auto *child:receiver.workspaceArea()->subWindowList()) if (child->property("plotName")==name) return dynamic_cast<PlotWidget *>(child->widget());
   throw std::runtime_error("Plot viewer missing");
}
static QAction *action(QtPlotReceiver &receiver,const QString &name,const char *id)
{
   auto *result=plot(receiver,name)->findChild<QAction *>(id); require(result,"Local playback action missing"); return result;
}
static QComboBox *choice(QtPlotReceiver &receiver,const QString &name,const char *id)
{
   auto *result=plot(receiver,name)->findChild<QComboBox *>(id); require(result,"Local playback choice missing"); return result;
}
static PlotSimulationClocks::State state(QtPlotReceiver &receiver,const QString &name)
{
   const auto result=receiver.simulationClock(name); require(result.has_value(),"Expected simulation clock unavailable"); return *result;
}
static void setRate(QtPlotReceiver &receiver,const QString &name,const QString &value)
{
   auto *field=plot(receiver,name)->findChild<QLineEdit *>("plotSimulationRate"); require(field && field->isVisible(),"Simulation s/s entry not visible");
   field->setText(value); require(QMetaObject::invokeMethod(field,"editingFinished",Qt::DirectConnection),"Cannot finish rate edit");
}
static QByteArray histories(QtPlotReceiver &receiver)
{
   QByteArray result; QDataStream out(&result,QIODevice::WriteOnly);
   for (const auto &name:QStringList{"Parent","Child","Grandchild"}) {
      const auto model=receiver.model(name); require(model!=nullptr,"Raw history model missing"); out<<name<<model->frame;
      for (auto it=model->curves.cbegin();it!=model->curves.cend();++it) { out<<it.key()<<it->name<<quint64(it->points.size());
         for (const auto &p:it->points) {
            out<<p.x<<p.y<<p.z<<p.epoch<<p.frame<<p.color.rgba()<<p.connect<<p.solver<<p.high<<p.low<<p.hasSun<<p.marker<<p.highlighted<<p.provider<<p.hasVelocity<<p.regularArc;
            for (double v:p.bodyToView) out<<v; for (double v:p.sunPosition) out<<v; for (double v:p.inertialToView) out<<v; for (double v:p.viewVelocity) out<<v;
         }
      }
   }
   return result;
}
static void preserve(const QString &name,const QByteArray &bytes)
{
   const auto directory=qEnvironmentVariable("GMAT_QT_PLAYBACK_OPTIONS_EVIDENCE"); if (directory.isEmpty()) return;
   require(QDir().mkpath(directory),"Cannot create runtime evidence directory"); QFile file(directory+"/runtime-"+name); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Cannot save runtime evidence");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("PlaybackOptionsRuntime");
   try {
      TestSettings settings; const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime engine initialization failed"); window.show(); app.processEvents(); auto &receiver=*window.plotReceiver(); receiver.clear();
      const auto configure=[&](const QString &name,std::optional<double> rate,const QString &parent,std::optional<bool> toolbar=std::nullopt) {
         QtCameraSetting camera; QtPlaybackSetting playback; playback.timeScale=rate; playback.syncParent=parent; playback.showToolbar=toolbar; camera.playback=playback; receiver.cameraSettings[name]=camera;
      };
      configure("Child",100.,"Parent",false); configure("Grandchild",5.,"Child"); configure("Parent",10.,{});
      configure("DefaultChild",25.,"DefaultParent");
      const auto create=[&](const std::string &name) { require(receiver.CreateGlPlotWindow(name,"",0,0,0,0,false,0),"Cannot create playback plot"); receiver.SetGlObject(name,{"Vehicle"},{}); };
      // Child-first creation must not make the child's saved rate the root rate.
      create("Child"); create("Grandchild"); create("Parent"); create("DefaultChild"); create("DefaultParent");
      near(state(receiver,"DefaultChild").rate,3600,"Parent without metadata did not get the independent default clock"); near(state(receiver,"DefaultParent").rate,3600,"Clock participation was lost when implicit parent opened later");
      constexpr double epoch=21545.;
      const auto publish=[&](const std::string &name,double seconds,double offset,bool solver=false) {
         require(receiver.UpdateGlPlot(name,"",{"Vehicle"},epoch+seconds/86400.,{offset+seconds},{seconds*2},{0.},{1.},{2.},{0.},{},{},solver,0,true,true,false),"Synthetic clock publication rejected");
      };
      publish("Parent",0,100); publish("Parent",10,100);
      publish("Child",5,1000); publish("Child",15,1000); publish("Child",25,1000);
      publish("Grandchild",-10,2000); publish("Grandchild",0,2000); publish("Grandchild",40,2000);
      publish("Parent",20,100); receiver.missionFinished(); app.processEvents();
      const auto original=histories(receiver); preserve("original-history.bin",original);
      const auto group=[&](double seconds,bool playing,double multiplier=1.,int mode=1) {
         for (const auto &name:QStringList{"Parent","Child","Grandchild"}) {
            const auto clock=state(receiver,name); near((clock.epoch-epoch)*86400,seconds,"Synchronized plot clocks diverged"); require(clock.playing==playing && clock.multiplier==multiplier && clock.mode==mode,"Synchronized pause/speed/mode diverged");
            require(!plot(receiver,name)->findChild<QTimer *>("plotReplayTimer")->isActive(),"Group clock left a competing local timer");
         }
      };
      group(20,false); near((state(receiver,"Child").minimum-epoch)*86400,0,"Most recent regular publisher did not supply its own minimum"); near((state(receiver,"Child").maximum-epoch)*86400,20,"Group range became a union instead of latest publisher envelope"); near(state(receiver,"Child").rate,10,"Child saved rate overrode its parent");
      // Hidden toolbars retain a visible restore action; it restores both strips.
      auto *child=plot(receiver,"Child"); auto *restore=child->findChild<QToolButton *>("plotRestoreToolbar"); auto *transport=child->findChild<QWidget *>("plotPlaybackControls"); auto *toolbar=child->findChild<QToolBar *>();
      require(restore && restore->isVisible() && toolbar && toolbar->isHidden() && transport && transport->isHidden(),"Saved hidden toolbar stranded restore or left controls visible");
      restore->click(); require(toolbar->isVisible() && transport->isVisible() && !restore->isVisible(),"Visible restore did not restore both toolbar strips");
      auto *toggle=child->findChild<QAction *>("plotToggleToolbar"); require(toggle && toggle->shortcut()==QKeySequence("Ctrl+Shift+T") && toggle->shortcutContext()==Qt::WidgetWithChildrenShortcut,"Keyboard toolbar restore binding missing"); toggle->trigger(); require(restore->isVisible(),"Toolbar shortcut action did not hide controls"); restore->click();
      action(receiver,"Child","plotReplayStart")->trigger(); group(0,false); action(receiver,"Child","plotReplayPlay")->trigger(); group(0,true);
      receiver.advanceSimulationClocks(.75); group(7.5,true);
      const QMap<QString,double> expectedPoses={{"Parent",107.5},{"Child",1007.5},{"Grandchild",2007.5}};
      for (const auto &entry:expectedPoses.asKeyValueRange()) {
         const auto model=receiver.model(entry.first); const auto *pose=orbitObjectPose(model->curves.constFind(0).value(),plot(receiver,entry.first)->canvas()->frame()); require(pose,"Interpolated group pose missing"); near(pose->x,entry.second,"Shared epoch did not resolve each plot's independent sample range");
      }
      action(receiver,"Grandchild","plotReplayFaster")->trigger(); group(7.5,true,2.); receiver.advanceSimulationClocks(.25); group(12.5,true,2.);
      action(receiver,"Child","plotReplaySlower")->trigger(); group(12.5,true); action(receiver,"Parent","plotReplayPlay")->trigger(); group(12.5,false);
      choice(receiver,"Child","plotReplayMode")->setCurrentIndex(0); group(12.5,false,1.,0);
      near(orbitObjectPose(receiver.model("Parent")->curves.constFind(0).value(),plot(receiver,"Parent")->canvas()->frame())->x,110,"Propagation mode did not select recorded parent sample");
      near(orbitObjectPose(receiver.model("Child")->curves.constFind(0).value(),plot(receiver,"Child")->canvas()->frame())->x,1005,"Propagation mode reused another plot's sample index");
      choice(receiver,"Grandchild","plotReplayMode")->setCurrentIndex(1); group(12.5,false);
      setRate(receiver,"Child","-10"); near(state(receiver,"Parent").rate,-10,"Child rate edit did not reach ultimate parent");
      action(receiver,"Parent","plotReplayStart")->trigger(); action(receiver,"Child","plotReplayPlay")->trigger(); group(20,true); receiver.advanceSimulationClocks(.25); group(17.5,true);
      receiver.advanceSimulationClocks(10); group(0,true); // Legacy clamps without wrapping or automatically pausing.
      setRate(receiver,"Grandchild","0"); receiver.advanceSimulationClocks(10); group(0,true); near(state(receiver,"Child").rate,0,"Zero clock rate was rejected"); action(receiver,"Parent","plotReplayPlay")->trigger(); group(0,false);
      setRate(receiver,"Child","not a number"); near(state(receiver,"Parent").rate,0,"Invalid rate changed the clock"); require(plot(receiver,"Child")->findChild<QLineEdit *>("plotSimulationRate")->text()=="0","Invalid rate text was not restored");
      setRate(receiver,"Parent","4"); choice(receiver,"Child","plotReplaySpeed")->setCurrentIndex(4); group(0,false,4.); setRate(receiver,"Grandchild","2"); group(0,false); near(state(receiver,"Parent").rate,2,"Rate edit failed to reset multiplier");
      plot(receiver,"Child")->findChild<QSlider *>("plotTimeline")->setValue(500); group(10,false);
      require(histories(receiver)==original,"Playback controls mutated retained sample bytes");
      // Close a parent window while its descendants keep its state alive.
      auto *parentWindow=qobject_cast<QMdiSubWindow *>(plot(receiver,"Parent")->parentWidget()); require(parentWindow && parentWindow->close(),"Parent viewer close failed"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      action(receiver,"Child","plotReplayPlay")->trigger(); receiver.advanceSimulationClocks(.5); action(receiver,"Child","plotReplayPlay")->trigger(); near((state(receiver,"Parent").epoch-epoch)*86400,11,"Closed parent clock stopped responding to children");
      require(receiver.show("Parent"),"Parent reopen failed"); group(11,false);
      auto *childWindow=qobject_cast<QMdiSubWindow *>(plot(receiver,"Child")->parentWidget()); require(childWindow && childWindow->close(),"Child viewer close failed"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(receiver.show("Child"),"Child reopen failed"); group(11,false);
      require(plot(receiver,"Child")->findChild<QWidget *>("plotPlaybackControls")->isVisible() && !plot(receiver,"Child")->findChild<QToolButton *>("plotRestoreToolbar")->isVisible(),"Live toolbar restore did not survive viewer reopen");
      // Shared percentage playback owns presentation until a local clock action.
      auto *shared=dynamic_cast<PlotPlaybackControls *>(window.findChild<QWidget *>("allPlotPlaybackControls")); require(shared,"Shared playback controls missing");
      auto *sharedStart=shared->findChild<QAction *>("allPlotReplayStart"); require(sharedStart,"Shared Start action missing"); sharedStart->trigger(); require(receiver.replayPosition()==0,"Shared Start did not take presentation ownership");
      for (const auto &name:QStringList{"Parent","Child","Grandchild"}) require(!receiver.model(name)->simulationEpoch,"Shared percentage replay retained absolute-epoch presentation");
      action(receiver,"Child","plotReplayLatest")->trigger(); require(!receiver.replayPosition(),"Local clock action did not release percentage playback"); group(20,false);
      // A different regular publisher updates the group's range and current time;
      // its solver trials must not extend that accepted-time envelope.
      publish("Child",30,1000); near((state(receiver,"Parent").minimum-epoch)*86400,5,"Incoming child did not supply its own clock minimum"); near((state(receiver,"Parent").maximum-epoch)*86400,30,"Incoming child did not supply its own clock maximum"); group(30,false);
      const auto beforeTrial=state(receiver,"Parent"); publish("Grandchild",500,2000,true); const auto afterTrial=state(receiver,"Parent");
      require(beforeTrial.minimum==afterTrial.minimum && beforeTrial.maximum==afterTrial.maximum && beforeTrial.epoch==afterTrial.epoch,"Solver trial changed accepted simulation clock envelope");
      publish("Parent",22,100); group(22,false); near((state(receiver,"Child").minimum-epoch)*86400,0,"Regular parent did not regain envelope"); near((state(receiver,"Child").maximum-epoch)*86400,22,"Trial/child endpoints leaked into parent envelope");
      const auto afterPublications=histories(receiver); plot(receiver,"Child")->findChild<QSlider *>("plotTimeline")->setValue(250); group(5.5,false); action(receiver,"Grandchild","plotReplayLatest")->trigger(); group(22,false); require(histories(receiver)==afterPublications,"Clock seek mutated histories after incoming publication"); preserve("final-history.bin",afterPublications);
      const auto directory=qEnvironmentVariable("GMAT_QT_PLAYBACK_OPTIONS_EVIDENCE"); if (!directory.isEmpty()) { app.processEvents(); require(plot(receiver,"Child")->grab().save(directory+"/runtime-clock-controls.png"),"Cannot capture restored group clock controls"); }
      // Deleting a subscriber is distinct from closing its viewer: discard its
      // publication envelope and stop an orphaned running clock.
      const auto preservedGroup=state(receiver,"Parent");
      configure("DeletedClock",12.,{}); create("DeletedClock"); publish("DeletedClock",100,3000); publish("DeletedClock",200,3000);
      setRate(receiver,"DeletedClock","24"); action(receiver,"DeletedClock","plotReplayPlay")->trigger();
      auto *simulationTimer=window.findChild<QTimer *>("plotSimulationTimer"); require(simulationTimer && simulationTimer->isActive() && state(receiver,"DeletedClock").playing,"Independent deletion fixture did not start its clock");
      require(receiver.DeleteGlPlot("DeletedClock"),"Subscriber deletion failed");
      require(!receiver.model("DeletedClock") && !receiver.simulationClock("DeletedClock") && !simulationTimer->isActive(),"Deleted subscriber retained model/clock or orphaned timer");
      const auto afterDelete=state(receiver,"Parent"); require(afterDelete.epoch==preservedGroup.epoch && afterDelete.minimum==preservedGroup.minimum && afterDelete.maximum==preservedGroup.maximum && afterDelete.rate==preservedGroup.rate && !afterDelete.playing,"Independent deletion changed an existing paused group");
      create("DeletedClock"); near(state(receiver,"DeletedClock").rate,12,"Recreated subscriber inherited the deleted clock's live rate"); require(!state(receiver,"DeletedClock").hasRange && !state(receiver,"DeletedClock").playing,"Recreated subscriber retained a deleted clock range/play state");
      publish("DeletedClock",400,3000); publish("DeletedClock",450,3000); const auto recreated=state(receiver,"DeletedClock");
      near((recreated.minimum-epoch)*86400,400,"Recreated subscriber retained the old minimum"); near((recreated.maximum-epoch)*86400,450,"Recreated subscriber retained the old maximum"); near((recreated.epoch-epoch)*86400,450,"Recreated subscriber retained the old epoch");
      require(histories(receiver)==afterPublications,"Subscriber deletion/recreation changed another group's retained samples");
      receiver.clear(); require(!receiver.simulationClock("Parent"),"Receiver clear retained simulation clocks");
      std::cout<<"PASS actual receiver/local controls: child-first inherited clocks, implicit parent default, signed/zero/rate and speed buttons, independently sampled shared epochs and both modes, pause/seek/close/reopen, hidden-toolbar restore and keyboard binding, global-percent ownership, regular-publisher envelopes excluding solver trials, subscriber deletion/recreation without orphaned clocks, unchanged raw histories. Offscreen only; no host/GPU qualification.\n";
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
