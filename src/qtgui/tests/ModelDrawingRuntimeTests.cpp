#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "SpacePoint.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Cannot write model drawing fixture/evidence"); }
static void preserve(const QString &name,const QByteArray &bytes)
{
   const auto directory=qEnvironmentVariable("GMAT_QT_MODEL_DRAWING_EVIDENCE"); if (directory.isEmpty()) return;
   require(QDir().mkpath(directory),"Cannot create model drawing runtime evidence directory"); write(directory+"/runtime-"+name,bytes);
}
static QString quoted(QString value) { return "'"+value.replace("'","''")+"'"; }
static const PlotCurve &curve(const PlotModel &model,const QString &name)
{
   for (const auto &candidate:model.curves) if (candidate.name==name) return candidate;
   throw std::runtime_error(("Missing retained model curve: "+name).toStdString());
}
static QJsonArray vector(const std::array<double,3> &values) { return {values[0],values[1],values[2]}; }
static QByteArray snapshot(const PlotModel &model)
{
   QJsonArray curves;
   for (const auto &item:model.curves) {
      QJsonArray points; for (const auto &point:item.points) points.append(QJsonObject{{"frame",double(point.frame)},{"epoch",point.epoch},{"xyz",QJsonArray{point.x,point.y,point.z}},{"connect",point.connect},{"solver",point.solver}});
      curves.append(QJsonObject{{"name",item.name},{"path",item.modelPath},{"override",item.modelOverride},{"scale",item.modelScale},{"offset",vector(item.modelOffset)},{"rotation",vector(item.modelRotation)},{"radius",item.radius},{"texture",item.texturePath},{"showObject",item.showObject},{"points",points}});
   }
   return QJsonDocument(QJsonObject{{"title",model.title},{"frame",double(model.frame)},{"curves",curves}}).toJson(QJsonDocument::Indented);
}
static void overridden(const PlotCurve &item,const QString &path)
{
   require(item.modelOverride && item.modelPath==QFileInfo(path).absoluteFilePath(),"Model override did not freeze its resolved absolute path");
   require(item.modelScale==1 && item.modelOffset==std::array<double,3>{0,0,0} && item.modelRotation==std::array<double,3>{0,0,0},"Model override inherited resource scale/translation/rotation");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("ModelDrawingRuntime");
   if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   try {
      TestSettings isolated; QTemporaryDir files; require(files.isValid(),"Model drawing runtime fixture directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window; require(window.initialize(startup),"Model drawing runtime initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Runtime source editor unavailable");
      const auto scriptFolder=files.filePath("script assets α"),fallbackFolder=files.filePath("startup models α"); require(QDir().mkpath(scriptFolder) && QDir().mkpath(fallbackFolder),"Model asset folders unavailable");
      const QString relative="plot model, α.obj",bodyRelative="body override.obj",stationRelative="station model.obj",missingRelative="missing, override.obj";
      const auto resourceFile=scriptFolder+"/resource model.obj",overrideFile=scriptFolder+'/'+relative,bodyFile=scriptFolder+'/'+bodyRelative,stationFile=scriptFolder+'/'+stationRelative,startupFile=fallbackFolder+"/startup model.obj";
      const QByteArray mesh="v 0 0 0\nv 100 0 0\nv 0 100 0\nf 1 2 3\n"; for (const auto &path:QStringList{resourceFile,overrideFile,bodyFile,stationFile,startupFile}) write(path,mesh);
      const QString resources="% preserve model resources α\nCreate Spacecraft Sat;\nSat.ModelFile = "+quoted(resourceFile)+";\nSat.ModelScale = 2;\nSat.ModelOffsetX = .1;\nSat.ModelOffsetY = .2;\nSat.ModelOffsetZ = .3;\nSat.ModelRotationX = 10;\nSat.ModelRotationY = 20;\nSat.ModelRotationZ = 30;\nCreate GroundStation Station;\nEarth.3DModelFile = "+quoted(resourceFile)+";\nEarth.3DModelScale = 3;\nEarth.3DModelOffsetX = .4;\nEarth.3DModelOffsetY = .5;\nEarth.3DModelOffsetZ = .6;\nEarth.3DModelRotationX = 40;\nEarth.3DModelRotationY = 50;\nEarth.3DModelRotationZ = 60;\n";
      const QString plots="Create OrbitView Inherited Reset Startup;\nInherited.Add = {Sat, Earth, Station};\nReset.Add = {Sat, Earth, Station};\nStartup.Add = {Sat, Earth, Station};\nCreate OpenFramesInterface Overrides Missing;\nOverrides.Add = {Sat, Earth, Station};\nOverrides.DrawModelFile = {"+quoted(relative)+", "+quoted(bodyRelative)+", "+quoted(stationRelative)+"};\nMissing.Add = {Sat, Earth, Station};\nMissing.DrawModelFile = "+quoted(missingRelative)+";\n";
      const auto converted=convertOpenFramesViews(resources+plots+"BeginMissionSequence; % retained, no numerical propagation\n"); require(converted.error.isEmpty(),qPrintable(converted.error));
      auto metadata=qtCameraSettings(converted.script); require(metadata.value("Overrides").objectModelFiles.value("Sat")==relative,"Quoted comma/space/Unicode filename did not survive conversion");
      QtCameraSetting reset; reset.objectModelFiles={{"Sat",relative},{"Earth",bodyRelative},{"Station",stationRelative}}; metadata["Reset"]=reset;
      QtCameraSetting startupSetting; startupSetting.objectModelFiles={{"Sat","startup model.obj"},{"Earth","startup model.obj"}}; metadata["Startup"]=startupSetting;
      auto source=setQtCameraSetting(setQtCameraSetting(converted.script,"Reset",reset),"Startup",startupSetting); const auto saved=scriptFolder+"/model overrides α.script"; write(saved,source.toUtf8());
      require(window.loadScript(saved) && window.buildScript(),qPrintable("Model snapshot fixture failed Build: "+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText())); require(editor->toPlainText()==source,"Build normalized literal model metadata/source"); preserve("source.script",source.toUtf8());
      auto *moderator=Moderator::Instance(); auto *sat=dynamic_cast<Spacecraft *>(moderator->GetConfiguredObject("Sat")); auto *earth=moderator->GetSolarSystemInUse()->GetBody("Earth"); auto *station=dynamic_cast<SpacePoint *>(moderator->GetConfiguredObject("Station")); require(sat && earth && station && station->IsOfType("GroundStation"),"Actual spacecraft/body/station fixtures unavailable"); require(sat->Initialize(),"Runtime spacecraft attitude could not initialize");
      const auto satSource=sat->GetGeneratingString(Gmat::SHOW_SCRIPT),earthSource=earth->GetGeneratingString(Gmat::SHOW_SCRIPT),stationSource=station->GetGeneratingString(Gmat::SHOW_SCRIPT);
      auto *manager=FileManager::Instance(); const auto managerState=manager->CaptureState(); struct Restore { std::shared_ptr<const FileManager::State> state; ~Restore() { FileManager::Instance()->RestoreState(*state); } } restore{managerState}; manager->AddFileType("VEHICLE_MODEL_PATH",(fallbackFolder+'/').toStdString()); manager->AddFileType("BODY_3D_MODEL_PATH",(fallbackFolder+'/').toStdString());
      require(QDir::cleanPath(QString::fromStdString(manager->GetGmatWorkingDirectory()))==scriptFolder,"Relative model fixture is not anchored to the script directory"); require(QDir::currentPath()!=scriptFolder,"Relative model check accidentally used process cwd as script directory");
      QMdiArea area; area.resize(1000,750); QtPlotReceiver receiver(&area); receiver.cameraSettings=metadata; QtPlotReceiver::validateCameraReferences(metadata);
      const StringArray names={"Sat","Earth","Station"}; const std::vector<SpacePoint *> points={sat,earth,station};
      for (const auto &name:QStringList{"Inherited","Overrides","Missing","Reset","Startup"}) { require(receiver.CreateGlPlotWindow(name.toStdString(),"",0,0,.6,.6,false,0),"Runtime plot creation failed"); receiver.SetGlObject(name.toStdString(),names,points); }
      const auto inherited=receiver.model("Inherited"),overrides=receiver.model("Overrides"),missing=receiver.model("Missing"),startupModel=receiver.model("Startup"); require(inherited && overrides && missing && startupModel,"Retained model snapshots unavailable");
      const auto &satInherited=curve(*inherited,"Sat"),&earthInherited=curve(*inherited,"Earth"),&stationInherited=curve(*inherited,"Station");
      require(!satInherited.modelOverride && satInherited.modelPath==QFileInfo(resourceFile).absoluteFilePath() && satInherited.modelScale==2 && satInherited.modelOffset==std::array<double,3>{.1,.2,.3} && satInherited.modelRotation==std::array<double,3>{10,20,30},"Inherited spacecraft model/transforms differ from resource");
      require(!earthInherited.modelOverride && earthInherited.modelPath==QFileInfo(resourceFile).absoluteFilePath() && earthInherited.modelScale==3 && earthInherited.modelOffset==std::array<double,3>{.4,.5,.6} && earthInherited.modelRotation==std::array<double,3>{40,50,60},"Inherited body model/transforms differ from resource"); require(!stationInherited.modelOverride && stationInherited.modelPath.isEmpty(),"Ground station inherited stale spacecraft/body model");
      overridden(curve(*overrides,"Sat"),overrideFile); overridden(curve(*overrides,"Earth"),bodyFile); overridden(curve(*overrides,"Station"),stationFile); overridden(curve(*startupModel,"Sat"),startupFile); overridden(curve(*startupModel,"Earth"),startupFile);
      const auto &unavailable=curve(*missing,"Sat"); overridden(unavailable,scriptFolder+'/'+missingRelative); require(!QFileInfo::exists(unavailable.modelPath),"Missing override fixture unexpectedly exists"); require(curve(*missing,"Earth").modelPath==earthInherited.modelPath && !curve(*missing,"Earth").modelOverride,"One object's missing override changed another object's asset policy");
      receiver.SetGlShowObjectFlag("Overrides",{false,true,false}); require(!curve(*overrides,"Sat").showObject && curve(*overrides,"Earth").showObject && !curve(*overrides,"Station").showObject,"DrawObject visibility was replaced by model file selection"); overridden(curve(*overrides,"Sat"),overrideFile); receiver.SetGlShowObjectFlag("Overrides",{true,true,true});
      require(sat->GetGeneratingString(Gmat::SHOW_SCRIPT)==satSource && earth->GetGeneratingString(Gmat::SHOW_SCRIPT)==earthSource && station->GetGeneratingString(Gmat::SHOW_SCRIPT)==stationSource && editor->toPlainText()==source,"Per-plot model overrides changed resources or source");
      auto invalid=metadata; invalid["Overrides"].objectModelFiles["Sun"]="other.obj"; bool rejected=false; try { QtPlotReceiver::validateCameraReferences(invalid); } catch (const std::exception &) { rejected=true; } require(rejected,"Model override for a space point absent from Add was accepted");
      invalid=metadata; invalid["Overrides"].objectModelFiles["MissingObject"]="other.obj"; rejected=false; try { QtPlotReceiver::validateCameraReferences(invalid); } catch (const std::exception &) { rejected=true; } require(rejected,"Unknown model override object was accepted"); QtPlotReceiver::validateCameraReferences(metadata);
      receiver.cameraSettings["Reset"].objectModelFiles.clear(); receiver.SetGlObject("Reset",names,points); const auto resetModel=receiver.model("Reset"); require(!curve(*resetModel,"Sat").modelOverride && curve(*resetModel,"Sat").modelScale==satInherited.modelScale && curve(*resetModel,"Sat").modelPath==satInherited.modelPath && curve(*resetModel,"Earth").modelRotation==earthInherited.modelRotation && curve(*resetModel,"Station").modelPath.isEmpty(),"Clearing override failed to restore resource model/transforms");
      receiver.SetGlObject("Reset",{"Station"},{station}); require(curve(*receiver.model("Reset"),"Station").modelPath.isEmpty() && !curve(*receiver.model("Reset"),"Station").modelOverride && curve(*receiver.model("Reset"),"Station").radius==0 && curve(*receiver.model("Reset"),"Station").texturePath.isEmpty(),"Repeated object snapshot retained prior spacecraft/body assets");
      // Synthetic publications exercise recorded presentation, without propagation.
      for (const double epoch:{21545.0,21545.001}) { const double shift=epoch==21545 ? 0 : 4000; require(receiver.UpdateGlPlot("Overrides","",names,epoch,{7000+shift,0,-7000},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{},{},false,0,false,true,false),"Model snapshot publication failed"); }
      auto retained=std::make_shared<PlotModel>(*overrides); const auto retainedBytes=snapshot(*retained),receiverBytes=snapshot(*overrides); require(curve(*retained,"Sat").points.size()==2 && curve(*retained,"Station").points.size()==2,"Model snapshot replay samples missing"); preserve("overrides.json",retainedBytes); preserve("inherited.json",snapshot(*inherited)); preserve("missing.json",snapshot(*missing)); preserve("startup.json",snapshot(*startupModel));
      PlotCanvas before(retained); before.resize(640,480); before.setFrame(1); const auto first=before.captureImage(); before.setFrame(2); const auto latest=before.captureImage(); require(!first.isNull() && !latest.isNull() && first!=latest,"Recorded model replay fixture did not distinguish frames");
      editor->setPlainText("Create Spacecraft Replacement;\nBeginMissionSequence;\n"); require(window.buildScript(),"Replacement engine fixture failed Build"); require(!moderator->GetConfiguredObject("Sat") && !moderator->GetConfiguredObject("Station"),"Engine replacement retained old spacecraft/station objects");
      require(snapshot(*retained)==retainedBytes && snapshot(*overrides)==receiverBytes,"Retained model snapshots changed after engine replacement"); PlotCanvas after(retained); after.resize(640,480); after.setFrame(1); require(after.captureImage()==first,"First retained model replay depended on discarded engine objects"); after.setFrame(2); require(after.captureImage()==latest,"Latest retained model replay depended on discarded engine objects");
      require(receiver.show("Overrides"),"Retained model output could not reopen after engine replacement"); auto *output=area.activeSubWindow(); require(output && dynamic_cast<PlotWidget *>(output->widget()),"Actual retained model output missing"); output->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(receiver.show("Overrides") && receiver.model("Overrides")==overrides && snapshot(*overrides)==receiverBytes && snapshot(*retained)==retainedBytes,"Output close/reopen lost pointer-free model snapshot");
      const auto evidence=qEnvironmentVariable("GMAT_QT_MODEL_DRAWING_EVIDENCE"); if (!evidence.isEmpty()) require(first.save(evidence+"/runtime-first.png") && latest.save(evidence+"/runtime-latest.png"),"Cannot preserve pointer-free replay images");
      std::cout<<"PASS real Spacecraft/CelestialBody/GroundStation per-plot model snapshots; literal comma/space/Unicode and script-relative/startup fallback paths; inherited resource transforms versus identity/native-unit override flag; missing-file retention; independent DrawObject visibility; resource/source invariance; membership validation; reset/repeated snapshot; retained replay/close-reopen after engine replacement. Synthetic publications and offscreen only; no host/GPU qualification.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
