#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static PlotWidget *plot(QtPlotReceiver &receiver,const QString &name)
{
   for (auto *window:receiver.workspaceArea()->subWindowList())
      if (window->property("plotName")==name) return dynamic_cast<PlotWidget *>(window->widget());
   throw std::runtime_error("Missing orbit widget");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   if (QGuiApplication::platformName()!="offscreen" || argc<2 || argc>3) return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("SkyControls");
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings;
      MainWindow window; require(window.initialize(startup),"Initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Missing source editor");
      const auto source=editor->toPlainText(); const bool modified=editor->document()->isModified();
      auto &receiver=*window.plotReceiver();
      receiver.CreateGlPlotWindow("EnabledSky","",0,0,0,0,false,0);
      receiver.SetGl3dDrawingOption("EnabledSky",false,false,false,true,false,false,false,false,true,true,true,1234);
      receiver.CreateGlPlotWindow("LazySky","",0,0,0,0,false,0);
      const auto enabled=receiver.model("EnabledSky"),lazy=receiver.model("LazySky");
      require(enabled->starCatalog.error.isEmpty() && enabled->starCatalog.stars.size()>40000 && enabled->constellationCatalog.segments.size()>500,"Startup sky catalogs missing");
      require(!lazy->starCatalogLoaded && !lazy->constellationCatalogLoaded,"Disabled sky loaded eagerly");
      auto controls=[&](const QString &name) {
         auto *widget=plot(receiver,name); widget->findChild<QAction *>("orbitDisplayAction")->trigger();
         auto *dialog=widget->findChild<QDialog *>("orbitDisplayDialog");
         auto *stars=dialog->findChild<QCheckBox *>("orbitDisplay_stars"),*constellations=dialog->findChild<QCheckBox *>("orbitDisplay_constellations");
         require(dialog->isVisible() && stars && constellations,"Missing visible sky toggles");
         return std::make_pair(stars,constellations);
      };
      auto [stars,constellations]=controls("EnabledSky");
      require(stars->isChecked() && constellations->isChecked(),"Initial script settings not reflected");
      const auto *starCache=enabled->starCatalog.stars.constData(); const auto *lineCache=enabled->constellationCatalog.segments.constData();
      stars->click(); require(!enabled->starsEnabled && enabled->constellationsEnabled,"Stars toggle changed constellations");
      require(!constellations->isEnabled(),"Stars Off did not disable the constellation control");
      stars->click(); require(constellations->isEnabled() && enabled->constellationsEnabled,"Re-enabling Stars lost the retained constellation choice");
      constellations->click(); stars->click(); require(!enabled->starsEnabled && !enabled->constellationsEnabled,"Could not disable sky");
      stars->click(); require(enabled->starsEnabled && !enabled->constellationsEnabled,"Stars failed to re-enable independently");
      constellations->click(); stars->click();
      require(!constellations->isEnabled() && enabled->constellationsEnabled,"Stars Off did not retain the disabled constellation choice");
      stars->click(); constellations->click(); stars->click();
      require(enabled->starCatalog.stars.constData()==starCache && enabled->constellationCatalog.segments.constData()==lineCache && enabled->starCount==1234,"Toggle discarded catalogs or changed StarCount");
      require(!enabled->labels && enabled->wireframe,"Sky toggle changed unrelated display options");
      auto *closed=dynamic_cast<QMdiSubWindow *>(plot(receiver,"EnabledSky")->parentWidget());
      require(closed && closed->close(),"Close failed"); app.processEvents(); require(receiver.show("EnabledSky"),"Reopen failed");
      auto reopened=controls("EnabledSky"); require(!reopened.first->isChecked() && !reopened.second->isChecked(),"Reopen restored hidden sky");
      auto fresh=controls("LazySky"); require(!fresh.first->isChecked() && !fresh.second->isChecked(),"Disabled defaults ignored");
      require(!fresh.second->isEnabled(),"Initially disabled Stars left constellation editing enabled");
      fresh.first->click(); require(lazy->starsEnabled && lazy->starCatalogLoaded && lazy->starCatalog.error.isEmpty() && lazy->starCatalog.stars.size()>40000 && !lazy->constellationCatalogLoaded,"Lazy star enabling failed");
      fresh.second->click(); require(lazy->constellationsEnabled && lazy->constellationCatalogLoaded && lazy->constellationCatalog.error.isEmpty() && lazy->constellationCatalog.segments.size()>500,"Lazy constellation enabling failed");
      require(!enabled->starsEnabled && !enabled->constellationsEnabled,"Changed a different viewer's sky");
      require(editor->toPlainText()==source && editor->document()->isModified()==modified,"Live controls changed source");
      app.processEvents();
      auto *dialog=plot(receiver,"LazySky")->findChild<QDialog *>("orbitDisplayDialog");
      require(dialog->rect().contains(fresh.first->geometry()) && dialog->rect().contains(fresh.second->geometry()),"Sky controls clipped");
      if (argc==3) require(dialog->grab().save(QFileInfo(argv[2]).absoluteFilePath()),"Display panel capture failed");
      receiver.clear();
      std::cout << "PASS: independent Stars/Constellations display toggles, enabled/disabled initial states, cached re-enable, lazy startup catalog loading, per-view isolation, closed/reopened state, StarCount and unrelated options preserved, source unchanged. Offscreen only.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
