#include "MainWindow.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QMenu>
#include <QSlider>
#include <QTest>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
void events() { QApplication::processEvents(); QTest::qWait(20); }
QAction *action(PlotWidget &plot,const char *name)
{
   auto *value=plot.findChild<QAction *>(name);
   require(value,"Plot action is missing"); return value;
}
void contextAction(PlotWidget &plot,const char *name)
{
   QContextMenuEvent event(QContextMenuEvent::Mouse,QPoint(20,20),plot.canvas()->mapToGlobal(QPoint(20,20)));
   QApplication::sendEvent(plot.canvas(),&event); events();
   auto *menu=plot.findChild<QMenu *>("plotContextMenu");
   require(menu && menu->isVisible(),"Right-click did not open the plot menu");
   auto *selected=action(plot,name);
   require(menu->actions().contains(selected),"Plot context menu lost a view action");
   QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(selected).center()); events();
}
void verifyMonochromeIcons(PlotWidget &plot,QToolBar *bar)
{
   QList<QIcon> icons;
   for (auto *item:bar->actions()) icons.append(item->icon());
   for (const char *name:{"plotActionsButton","plotRestoreToolbar","plotReplayModeButton"})
      if (auto *button=plot.findChild<QToolButton *>(name)) icons.append(button->icon());
   const QPalette original=plot.palette();
   // Change the palette after the icons exist to exercise live theme changes.
   for (bool dark:{false,true}) {
      auto palette=original;
      const QColor foreground=dark ? QColor(223,192,128) : QColor(40,110,70);
      const QColor disabled=dark ? QColor(151,151,151) : QColor(108,108,108);
      const QColor selected(250,250,250);
      palette.setColor(QPalette::Window,dark ? QColor(40,40,40) : QColor(245,245,245));
      palette.setColor(QPalette::ButtonText,foreground);
      palette.setColor(QPalette::Disabled,QPalette::ButtonText,disabled);
      palette.setColor(QPalette::Highlight,QColor(35,65,110));
      palette.setColor(QPalette::HighlightedText,selected);
      plot.setPalette(palette);
      for (const auto &icon:icons) for (auto mode:{QIcon::Normal,QIcon::Active,QIcon::Selected,QIcon::Disabled})
         for (int size:{16,20,40}) {
            const QColor expected=mode==QIcon::Disabled ? disabled : mode==QIcon::Selected ? selected : foreground;
            const auto image=icon.pixmap(QSize(size,size),mode).toImage();
            int opaquePixels=0;
            for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) {
               const QColor pixel=image.pixelColor(x,y);
               if (pixel.alpha()<200) continue;
               ++opaquePixels;
               require(std::abs(pixel.red()-expected.red())<=2 && std::abs(pixel.green()-expected.green())<=2
                       && std::abs(pixel.blue()-expected.blue())<=2,"A plot icon contains an outline or ignores its palette");
            }
            require(opaquePixels>=8,"A plot icon has no readable silhouette");
         }
   }
   plot.setPalette(original);
}
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication application(argc,argv);
   application.setOrganizationName("GMATTests"); application.setApplicationName("PlotToolbar");
   if (qEnvironmentVariableIsSet("GMAT_QT_TOOLBAR_DARK")) {
      auto palette=application.palette();
      palette.setColor(QPalette::Window,QColor(40,40,40)); palette.setColor(QPalette::Button,QColor(48,48,48));
      palette.setColor(QPalette::Base,QColor(30,30,30)); palette.setColor(QPalette::WindowText,QColor(235,235,235));
      palette.setColor(QPalette::ButtonText,QColor(235,235,235)); palette.setColor(QPalette::Text,QColor(235,235,235));
      application.setPalette(palette);
   }
   TestSettings settings;
   try {
      require(argc==2,"Supply a startup file");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow runtime; require(runtime.initialize(startup),"Toolbar runtime initialization failed");
      const QString directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (!directory.isEmpty()) QDir().mkpath(directory);
      for (auto kind:{PlotModel::Kind::Orbit,PlotModel::Kind::GroundTrack,PlotModel::Kind::XY}) {
         auto model=std::make_shared<PlotModel>(kind);
         model->title="Toolbar check"; model->curves[0].name="Sat";
         for (int index=0;index<3;++index) { ++model->frame; model->append(0,7000,index*10,0); }
         PlotWidget plot(model); plot.resize(800,480); plot.show();
         require(QTest::qWaitForWindowExposed(&plot),"Plot window was not exposed"); events();
         auto *bar=plot.findChild<QToolBar *>("plotViewToolbar");
         auto *controls=plot.findChild<QWidget *>(kind==PlotModel::Kind::XY ? "plotControlBar" : "plotPlaybackControls");
         auto *menu=plot.findChild<QMenu *>("plotContextMenu");
         require(bar && controls && menu,"Plot controls are incomplete");
         require(controls->property("controlRows").toInt()==1,"Wide plot has more than one control row");
         for (auto *item:bar->actions()) {
            require(!item->icon().isNull() && !item->toolTip().isEmpty(),"A view action lacks an icon or tooltip");
            require(menu->actions().contains(item),"A view action is missing from the context menu");
         }
         verifyMonochromeIcons(plot,bar);
         for (auto *button:plot.findChildren<QToolButton *>()) if (button->isVisible() && button->defaultAction())
            require(button->toolButtonStyle()==Qt::ToolButtonIconOnly,"A toolbar action still displays its text label");
         if (kind!=PlotModel::Kind::XY) {
            for (const char *name:{"plotReplayStart","plotReplayPlay","plotReplayLatest"}) {
               auto *item=action(plot,name);
               QToolButton *button=nullptr;
               for (auto *candidate:plot.findChildren<QToolButton *>()) if (candidate->defaultAction()==item) button=candidate;
               require(button && button->isVisible() && !button->toolTip().isEmpty() && !button->accessibleName().isEmpty(),
                       "A playback icon is inaccessible");
            }
            auto *slider=plot.findChild<QSlider *>("plotTimeline");
            require(std::abs(slider->mapToGlobal(slider->rect().center()).y()-bar->mapToGlobal(bar->rect().center()).y())<=5,
                    "Timeline is outside the single-row toolbar");
            auto *mode=plot.findChild<QToolButton *>("plotReplayModeButton");
            plot.setSharedReplayMode(PlotReplayMode::PropagationSteps);
            require(mode->toolTip().contains("Propagation steps") && mode->menu()->actions().first()->isChecked(),
                    "Playback mode icon/menu did not follow the selected mode");
            action(plot,"plotReplayStart")->trigger(); require(slider->value()==0,"Start no longer rewinds playback");
            action(plot,"plotReplayPlay")->trigger(); require(plot.findChild<QTimer *>("plotReplayTimer")->isActive(),"Play did not start playback");
            action(plot,"plotReplayLatest")->trigger(); require(slider->value()==1000 && !plot.findChild<QTimer *>("plotReplayTimer")->isActive(),"Latest did not stop playback at the end");
            PlotSimulationClocks::State clock;
            clock.playing=true; clock.hasRange=true; clock.minimum=1; clock.maximum=2; clock.epoch=1.5;
            clock.rate=10; clock.multiplier=1; clock.mode=int(PlotReplayMode::PropagationSteps);
            plot.setSimulationClock(clock,"Root"); events();
            QToolButton *play=nullptr;
            for (auto *button:plot.findChildren<QToolButton *>()) if (button->defaultAction()==action(plot,"plotReplayPlay")) play=button;
            require(play && play->isChecked() && play->accessibleName()=="Pause","Shared clock did not update the playback icon state");
            plot.setSharedReplayPosition(1000); events();
            require(!play->isChecked() && play->accessibleName()=="Play","Shared playback reset left stale icon state");
            plot.setSimulationClock(std::nullopt); events();
         }
         plot.resize(480,480); events();
         if (controls->property("controlRows").toInt()!=1) {
            std::cerr<<"Toolbar rows="<<controls->property("controlRows").toInt()<<" width="<<controls->width()<<std::endl;
            for (auto *child:controls->findChildren<QWidget *>(QString(),Qt::FindDirectChildrenOnly))
               std::cerr<<child->metaObject()->className()<<' '<<child->objectName().toStdString()<<" width="<<child->width()
                        <<" hint="<<child->sizeHint().width()<<" minimum="<<child->minimumSizeHint().width()
                        <<" hidden="<<child->isHidden()<<std::endl;
         }
         require(controls->property("controlRows").toInt()==1,"A normal tiled pane unnecessarily wraps its controls");
         if (!directory.isEmpty()) plot.grab().save(directory+QString("/kind-%1-480.png").arg(int(kind)));
         plot.resize(320,480); events();
         if (kind!=PlotModel::Kind::XY)
            require(controls->property("controlRows").toInt()==2,"Narrow plot did not use its compact two-row fallback");
         require(controls->rect().contains(controls->mapFromGlobal(bar->mapToGlobal(bar->rect().center()))),"View toolbar escaped its control strip");
         for (auto *button:controls->findChildren<QToolButton *>()) if (button->isVisible()) {
            const QRect bounds(controls->mapFromGlobal(button->mapToGlobal(QPoint())),button->size());
            require(controls->rect().adjusted(-1,-1,1,1).contains(bounds),"A narrow-toolbar button was clipped");
         }
         if (!directory.isEmpty()) plot.grab().save(directory+QString("/kind-%1-320.png").arg(int(kind)));
         plot.resize(800,480); events();
         plot.canvas()->zoomBy(3);
         contextAction(plot,"plotFitAction"); require(plot.canvas()->zoomFactor()==1,"Context-menu Fit did not use the plot action");
         if (kind==PlotModel::Kind::Orbit) {
            const bool oldSunlight=model->sunlight;
            contextAction(plot,"orbitSunlight"); require(model->sunlight!=oldSunlight,"Context-menu Sunlight did not change the plot");
            contextAction(plot,"orbitCameraAction");
            auto *camera=plot.findChild<QDialog *>("orbitCameraDialog"); require(camera && camera->isVisible(),"Camera menu action did not open its dialog"); camera->hide();
            contextAction(plot,"orbitDisplayAction");
            auto *display=plot.findChild<QDialog *>("orbitDisplayDialog"); require(display && display->isVisible(),"Display menu action did not open its dialog"); display->hide();
         }
         action(plot,"plotToggleToolbar")->trigger(); events();
         require(controls->isHidden() && plot.findChild<QToolButton *>("plotRestoreToolbar")->isVisible(),"Hidden toolbar cannot be restored");
         plot.findChild<QToolButton *>("plotRestoreToolbar")->click(); events();
         require(controls->isVisible() && action(plot,"plotToggleToolbar")->isChecked(),"Toolbar restore did not restore its state");
         require(model->curves[0].points.size()==3,"Toolbar actions changed retained plot history");
         if (!directory.isEmpty()) plot.grab().save(directory+QString("/kind-%1-wide.png").arg(int(kind)));
      }
      std::cout<<"PASS icon-only view/playback actions, tooltips, single-row and narrow layouts, context menus, playback mode, transport, hidden-toolbar recovery and retained history\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
