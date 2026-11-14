#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "CommandEditor.hpp"
#include "ReportViewer.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QTreeWidgetItem *item(QTreeWidget *tree,const QString &name) {
   const auto matches=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);
   require(matches.size()==1,"Navigation item missing/ambiguous"); return matches.first();
}
static void key(QWidget *target,int code,bool repeat=false,Qt::KeyboardModifiers modifiers=Qt::NoModifier) {
   QKeyEvent press(QEvent::KeyPress,code,modifiers,QString(),repeat),release(QEvent::KeyRelease,code,modifiers);
   QApplication::sendEvent(target,&press); QApplication::sendEvent(target,&release);
}
static void mouseDoubleClick(QTreeWidget *tree,QTreeWidgetItem *selected) {
   const QPointF point=tree->visualItemRect(selected).center(),global=tree->viewport()->mapToGlobal(point.toPoint());
   QMouseEvent press(QEvent::MouseButtonPress,point,global,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
   QMouseEvent release(QEvent::MouseButtonRelease,point,global,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
   QMouseEvent twice(QEvent::MouseButtonDblClick,point,global,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
   QApplication::sendEvent(tree->viewport(),&press); QApplication::sendEvent(tree->viewport(),&release);
   QApplication::sendEvent(tree->viewport(),&twice); QApplication::sendEvent(tree->viewport(),&release);
}
static QByteArray bytes(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Navigation report unavailable"); return file.readAll(); }
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtTreeActivation");
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","Tree activation check requires offscreen startup");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Navigation fixture directory unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Navigation runtime unavailable"); window.resize(1280,900); window.show(); window.activateWindow(); app.processEvents();
      auto *area=window.findChild<QMdiArea *>("workspace"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto *resources=window.findChild<QTreeWidget *>("Resources"),*mission=window.findChild<QTreeWidget *>("Mission"),*output=window.findChild<QTreeWidget *>("Output");
      auto *tabs=window.findChild<QDockWidget *>("navigation")->findChild<QTabWidget *>(); require(area && editor && resources && mission && output && tabs,"Navigation controls missing");
      const auto focus=[&](QTreeWidget *tree,QTreeWidgetItem *selected,int tab) {
         tabs->setCurrentIndex(tab); tree->expandAll(); tree->setCurrentItem(selected); tree->scrollToItem(selected);
         window.activateWindow(); tree->setFocus(Qt::OtherFocusReason); app.processEvents();
         require(tree->isVisible() && QApplication::focusWidget()==tree,"Navigation tree did not receive actual focus");
      };
      const auto report=files.filePath("keyboard report α.txt");
      const QString source="% preserve navigation source α\nCreate Spacecraft Sat;\nCreate GroundStation Site;\nSite.MinimumElevationAngle = 12;\n"
         "Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\n"
         "Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.WriteReport = false;\n"
         "BeginMissionSequence;\nPropagate 'KeyboardArc' P(Sat) {Sat.ElapsedSecs = 60};\nReport Values Sat.EarthMJ2000Eq.X;\n";
      editor->setPlainText(source); require(window.buildScript(),"Navigation fixture did not build");
      bool runGuard=false; std::exception_ptr callbackFailure;
      auto *receiver=window.plotReceiver();
      const auto originalNotify=receiver->changed;
      receiver->changed=[&,originalNotify] {
         if (originalNotify) originalNotify();
         // The 60-second mission can complete before the event-pump timer.
         // Observe a real synchronous publication instead of lengthening it.
         if (!window.isRunning() || runGuard || callbackFailure) return;
         try {
            const auto count=area->subWindowList().size();
            resources->setCurrentItem(item(resources,"Site")); key(resources,Qt::Key_Return);
            mission->setCurrentItem(item(mission,"Propagate — KeyboardArc")); key(mission,Qt::Key_Enter);
            output->setCurrentItem(item(output,"Values")); key(output,Qt::Key_Return);
            require(area->subWindowList().size()==count && editor->toPlainText()==source,"Tree keyboard activation bypassed an existing run guard"); runGuard=true;
         } catch (...) { callbackFailure=std::current_exception(); window.stopMission(); }
      };
      const auto result=window.runMission(); receiver->changed=originalNotify; if (callbackFailure) std::rethrow_exception(callbackFailure);
      if (!runGuard || result!=MainWindow::RunResult::Completed) std::cerr<<"Navigation run guard="<<runGuard<<" result="<<int(result)<<" messages="<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()<<'\n';
      require(runGuard && result==MainWindow::RunResult::Completed,"One short navigation mission/run guard failed"); const auto reportBefore=bytes(report);
      int resourcesActivated=0,missionActivated=0,outputActivated=0;
      QObject::connect(resources,&QTreeWidget::itemDoubleClicked,&window,[&] { ++resourcesActivated; });
      QObject::connect(mission,&QTreeWidget::itemDoubleClicked,&window,[&] { ++missionActivated; });
      QObject::connect(output,&QTreeWidget::itemDoubleClicked,&window,[&] { ++outputActivated; });
      auto before=area->subWindowList().size(); focus(resources,item(resources,"Site"),0); key(resources,Qt::Key_Return); app.processEvents();
      require(resourcesActivated==1 && area->subWindowList().size()==before+1,"Return did not open exactly one resource editor");
      auto *resourceWindow=area->activeSubWindow(); auto *panel=dynamic_cast<ResourceEditor *>(resourceWindow->widget());
      auto *elevation=panel ? panel->findChild<QLineEdit *>("station_MinimumElevationAngle") : nullptr; require(elevation,"Actual GroundStation settings missing");
      const auto initialElevation=elevation->text(); elevation->setText("21.5"); require(panel->hasChanges(),"Pending navigation fixture did not change settings");
      focus(resources,item(resources,"Site"),0); key(resources,Qt::Key_Enter,false,Qt::KeypadModifier); app.processEvents();
      require(resourcesActivated==2 && area->subWindowList().size()==before+1 && area->activeSubWindow()==resourceWindow && panel->hasChanges() && elevation->text()=="21.5" && editor->toPlainText()==source,"Enter duplicated or discarded the pending resource panel");
      key(resources,Qt::Key_Return,true); require(resourcesActivated==2,"Held Enter repeatedly opened navigation panels");
      before=area->subWindowList().size(); focus(mission,item(mission,"Propagate — KeyboardArc"),1); key(mission,Qt::Key_Enter); app.processEvents();
      require(missionActivated==1 && area->subWindowList().size()==before+1 && dynamic_cast<CommandEditor *>(area->activeSubWindow()->widget()),"Enter did not open exactly one real mission editor");
      area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); app.processEvents();
      before=area->subWindowList().size(); focus(mission,item(mission,"Propagate — KeyboardArc"),1); mouseDoubleClick(mission,mission->currentItem()); app.processEvents();
      require(missionActivated==2 && area->subWindowList().size()==before+1,"Mouse double-click activated a command more than once");
      area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); app.processEvents();
      before=area->subWindowList().size(); focus(output,item(output,"Values"),2); key(output,Qt::Key_Return); app.processEvents();
      require(outputActivated==1 && area->subWindowList().size()==before+1 && dynamic_cast<ReportViewer *>(area->activeSubWindow()->widget()),"Return did not open exactly one actual report viewer");
      area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); app.processEvents();
      before=area->subWindowList().size(); output->itemDoubleClicked(item(output,"Values"),0);
      require(outputActivated==2 && area->subWindowList().size()==before+1,"Existing direct double-click callers no longer activate output");
      auto model=window.plotReceiver()->model("Orb"); require(bool(model),"Short run did not create retained plot history");
      for (auto *child:area->subWindowList()) if (child->property("plotName")=="Orb") child->close();
      QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); app.processEvents();
      before=area->subWindowList().size(); focus(output,item(output,"Orb"),2); key(output,Qt::Key_Enter); app.processEvents();
      require(area->subWindowList().size()==before+1 && dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget()) && window.plotReceiver()->model("Orb")==model,"Enter did not reopen exactly one retained Output plot");
      before=area->subWindowList().size();
      focus(resources,item(resources,"Ground Stations"),0); key(resources,Qt::Key_Return);
      focus(mission,mission->topLevelItem(0),1); key(mission,Qt::Key_Enter);
      focus(output,item(output,"Reports"),2); key(output,Qt::Key_Return);
      require(area->subWindowList().size()==before,"Category activation opened an unintended panel");
      editor->appendPlainText("% edited without Build"); const auto pendingSource=editor->toPlainText();
      focus(resources,item(resources,"Site"),0); key(resources,Qt::Key_Enter);
      focus(mission,item(mission,"Propagate — KeyboardArc"),1); key(mission,Qt::Key_Return);
      require(area->subWindowList().size()==before && editor->toPlainText()==pendingSource && panel->hasChanges() && elevation->text()=="21.5","Unbuilt source guard was bypassed or pending data changed");
      require(bytes(report)==reportBefore,"Navigation activation changed independent report bytes");
      elevation->setText(initialElevation); require(!panel->hasChanges(),"Navigation cleanup did not restore the pending field");
      // Reverting a value retains NASA's dirty-input flag until Apply. Use
      // the normal no-op Apply route before asking Build to replace the model.
      panel->requestApply();
      require(!panel->hasUnappliedInput() && editor->toPlainText()==pendingSource,
         "Navigation cleanup did not accept the reverted draft without changing source");
      editor->setPlainText(source); require(window.buildScript(),"Unicode report overwrite fixture failed to rebuild");
      { QFile stale(report); require(stale.open(QIODevice::WriteOnly|QIODevice::Truncate),"Unicode report overwrite fixture unavailable"); stale.write("stale report bytes"); }
      require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==reportBefore,"Unicode report rerun did not truncate the intended file");
      auto appendSource=source; appendSource.replace("Values.WriteHeaders = false;","Values.WriteHeaders = false;\nValues.AppendToExistingFile = true;");
      editor->setPlainText(appendSource);
      require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==reportBefore+reportBefore,"Unicode report append did not use the same intended file");


      std::cout<<"PASS focused Return/Enter opens real Resource/Mission/Report/retained Plot once; pending settings/source/report preserved; mouse/direct callers, category no-op, key repeat and actual run/unbuilt guards retained. Offscreen only.\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
