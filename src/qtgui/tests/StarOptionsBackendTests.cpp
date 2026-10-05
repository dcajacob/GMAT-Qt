#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "ScriptCompatibility.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QString converted(const QString &source) {
   const auto result=convertOpenFramesViews(source); require(result.error.isEmpty(),qPrintable(result.error)); return result.script;
}
static QString source(const QString &settings={}) { return "Create OpenFramesInterface Orb; % preserve α\nOrb.Add = {Sat, Earth};\n"+settings; }
static QtStarSetting options(const QString &settings={}) {
   const auto camera=qtCameraSettings(converted(source(settings))).value("Orb"); require(camera.stars.has_value(),"OF default Monitor star options omitted"); return *camera.stars;
}
static void conversionChecks() {
   const auto initial=options(); require(initial.preset=="Monitor" && initial.customCount==40000 && initial.catalog=="inp_StarsHYGv3.txt" && initial.minMagnitude==-2 && initial.maxMagnitude==6 && initial.minPixels==1 && initial.maxPixels==10 && initial.minDimRatio==.5,"OF Monitor/custom constructor defaults changed");
   const QString fields="Orb.StarCount = 3;\nOrb.MinStarMag = 0;\nOrb.MaxStarMag = 4;\nOrb.MinStarPixels = 2;\nOrb.MaxStarPixels = 9;\nOrb.MinStarDimRatio = .2;\nOrb.StarCatalog = 'custom α''s.txt';\n";
   const auto stored=options(fields+"Orb.StarSettings = 'Virtual Reality';\nOrb.StarSettings = Monitor;\n");
   require(stored.preset=="Monitor" && stored.customCount==3 && stored.catalog=="custom α's.txt" && stored.minMagnitude==0 && stored.maxMagnitude==4 && stored.minPixels==2 && stored.maxPixels==9 && stored.minDimRatio==.2,"Preset changes discarded custom settings/source order");
   const auto script=converted(source(fields+"Orb.StarSettings = Custom;\n"));
   require(script.contains("% preserve α") && script.contains("% Qt conversion: Orb.StarCatalog = 'custom α''s.txt';") && script.contains("Orb.StarCount = 3;"),"Original star inputs/comment/backing StarCount were lost");
   require(qtCameraSettings(script).value("Orb").stars->preset=="Custom","Custom preset conversion failed");
   require(!qtCameraSettings("Create OrbitView Native;\n").contains("Native"),"Ordinary Qt plot gained OF style");
   require(!qtStars(" null ") && qtStarsJson({})=="null" && qtStarsJson(qtStars(qtStarsJson(stored)))==qtStarsJson(stored),"Optional stars JSON round trip failed");
   for (const auto &bad:QStringList{"Orb.StarCount = 0;","Orb.StarCount = 1.5;","Orb.MinStarMag = 21;","Orb.MaxStarMag = -3;","Orb.MinStarPixels = 0;","Orb.MaxStarPixels = .5;","Orb.MinStarDimRatio = 1.1;","Orb.StarCatalog = '';","Orb.StarSettings = Other;","Orb.MinStarMag = 7;\nOrb.MaxStarMag = 6;","Orb.MinStarPixels = 11;\nOrb.MaxStarPixels = 10;"}) {
      const auto original=source(bad+'\n'); const auto result=convertOpenFramesViews(original);
      require(!result.error.isEmpty() && result.script==original,"Invalid source-ordered star setting did not retain source");
   }
   // Max setters compare with the then-current minima, even if later minima
   // would make the final values valid; a final-state-only parser is incorrect.
   const auto badOrder=source("Orb.MaxStarMag = -3;\nOrb.MinStarMag = -4;\n");
   require(!convertOpenFramesViews(badOrder).error.isEmpty() && options("Orb.MinStarMag = -4;\nOrb.MaxStarMag = -3;\n").maxMagnitude==-3,"Magnitude setter source order lost");
   const auto valid=QJsonDocument::fromJson(qtStarsJson(stored).toUtf8()).object();
   for (int test=0;test<8;++test) {
      auto object=valid;
      if (test==0) object["customCount"]=0;
      if (test==1) object["customCount"]=2.5;
      if (test==2) object["minPixels"]=0;
      if (test==3) object["maxMagnitude"]=-1;
      if (test==4) object["minDimRatio"]=".2";
      if (test==5) object["preset"]="Other";
      if (test==6) object["catalog"]="";
      if (test==7) object["unknown"]=true;
      bool rejected=false; try { (void)qtStars(QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact))); } catch (const std::exception &) { rejected=true; }
      require(rejected,"Invalid star JSON accepted");
   }
}
static std::shared_ptr<const PlotModel> display(QtPlotReceiver &receiver,const QString &name,const std::optional<QtStarSetting> &stars) {
   QtCameraSetting camera; camera.stars=stars; receiver.cameraSettings.insert(name,camera);
   receiver.CreateGlPlotWindow(name.toStdString(),"",0,0,.6,.6,false,0);
   receiver.SetGl3dDrawingOption(name.toStdString(),false,false,false,false,false,false,false,false,true,true,false,123);
   return receiver.model(name);
}
int main(int argc,char **argv) {
   QApplication app(argc,argv); if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("StarOptionsBackend");
   try {
      TestSettings settings; conversionChecks(); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Startup initialization failed"); window.show(); auto &receiver=*window.plotReceiver();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Missing source editor"); const auto original=editor->toPlainText(); const bool modified=editor->document()->isModified();
      const auto ordinary=display(receiver,"Ordinary",{}); require(ordinary && !ordinary->starOptions && !ordinary->printBackground && ordinary->starCount==123,"Ordinary Qt style/count changed");
      const auto monitor=display(receiver,"Monitor",QtStarSetting{}); require(monitor && monitor->starOptions && monitor->starCount==40000 && monitor->starsEnabled && !monitor->printBackground && monitor->starCatalog.error.isEmpty(),"Bundled OF Monitor catalog/preset not loaded");
      require(monitor->starCatalog.stars.size()>40000 && monitor->starOptions->minMagnitude==-2 && monitor->starOptions->maxMagnitude==6 && monitor->starOptions->maxPixels==10 && monitor->starOptions->minDimRatio==.5,"OF Monitor preset differs");
      // Bundled HYG's first Sirius row is RA6.752481 HOURS, DEC-16.716116.
      // This independently catches a degrees-only parser/default STAR_FILE use.
      constexpr double radians=0.017453292519943295769;
      const auto &sirius=monitor->starCatalog.stars.front(); const double ra=6.752481*15*radians,dec=-16.716116*radians;
      require(std::abs(sirius.direction[0]-std::cos(ra)*std::cos(dec))<1e-12 && std::abs(sirius.direction[1]-std::sin(ra)*std::cos(dec))<1e-12 && std::abs(sirius.direction[2]-std::sin(dec))<1e-12 && sirius.magnitude==-1.44,"Bundled Sirius hours header/source row changed");
      auto preset=QtStarSetting{}; preset.preset="Virtual Reality"; preset.customCount=3; preset.maxPixels=17;
      const auto vr=display(receiver,"VR",preset); require(vr && vr->starCount==40000 && vr->starOptions->maxPixels==5 && vr->starOptions->minDimRatio==.1,"VR failed to select its effective preset independently of custom values");
      preset.preset="Printer"; const auto printer=display(receiver,"Printer",preset); require(printer && printer->printBackground && !printer->starsEnabled && !printer->starCatalogLoaded,"Printer did not suppress stars/use white background flag");
      QTemporaryDir files; require(files.isValid(),"No custom star directory"); const auto path=files.filePath("custom α's.txt"); QFile custom(path); require(custom.open(QIODevice::WriteOnly|QIODevice::Text),"Cannot write custom catalog");
      custom.write("ra_360 dec mag colorindex\n90 0 4 0.7\n0 0 0 0\n180 0 2 0.2\n"); custom.close();
      preset.preset="Custom"; preset.catalog=path; preset.customCount=2; preset.minMagnitude=0; preset.maxMagnitude=4; preset.minPixels=2; preset.maxPixels=9; preset.minDimRatio=.2;
      const auto own=display(receiver,"Custom",preset); require(own && own->starCount==2 && own->starCatalog.error.isEmpty() && own->starOptions->maxPixels==9 && own->starOptions->minDimRatio==.2,"Custom stars ignored explicit options/catalog");
      require(own->starCatalog.stars.size()==3 && own->starCatalog.stars[0].magnitude==4 && own->starCatalog.stars[1].magnitude==0 && own->starCatalog.stars[2].magnitude==2 && std::abs(own->starCatalog.stars[0].direction[1]-1)<1e-12,"Styled source catalog order/header changed");
      // Validate the raw fallback alias in a real configured OrbitView without
      // a numerical mission or an unrelated startup rewrite.
      const auto saved=FileManager::Instance()->CaptureState();
      struct Restore { std::shared_ptr<const FileManager::State> state; ~Restore() { FileManager::Instance()->RestoreState(*state); } } restore{saved};
      FileManager::Instance()->AddFileType("OUTPUT_PATH",files.path().toStdString()+"/");
      FileManager::Instance()->AddFileType("STAR_PATH","OUTPUT_PATH/");
      preset.catalog="custom α's.txt"; QtCameraSetting camera; camera.stars=preset;
      const auto configured="Create Spacecraft Sat;\nCreate OrbitView Configured;\nConfigured.Add = {Sat, Earth};\n"+qtCameraDirective("Configured",camera)+"BeginMissionSequence;\n";
      editor->setPlainText(configured); require(window.buildScript(),"Valid raw fallback catalog failed Build");
      require(qtCameraSettings(editor->toPlainText()).value("Configured").stars->catalog==preset.catalog && FileManager::Instance()->GetPathDefinition("STAR_PATH")=="OUTPUT_PATH/","Build normalized raw custom catalog/fallback alias source");
      auto invalid=qtCameraSettings(configured); invalid["Configured"].stars->catalog="missing-owned-catalog.txt"; bool rejected=false;
      try { QtPlotReceiver::validateCameraReferences(invalid); } catch (BaseException &) { rejected=true; } catch (const std::exception &) { rejected=true; }
      require(rejected && qtStarsJson(qtCameraSettings(configured).value("Configured").stars)==qtStarsJson(camera.stars),"Missing catalog validation mutated prior source/settings");
      require(own->starOptions && own->starCatalog.stars[0].magnitude==4,"Build discarded retained pointer-free custom sky data");
      editor->setPlainText(original); editor->document()->setModified(modified);
      std::cout<<"PASS source-ordered presets/custom settings/catalog/strict JSON recovery; ordinary style unchanged; bundled HYG Sirius hours; Monitor/VR/Printer/Custom effective fields; source order/raw fallback and replaced-engine retained data. No numerical mission or host rendering.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
