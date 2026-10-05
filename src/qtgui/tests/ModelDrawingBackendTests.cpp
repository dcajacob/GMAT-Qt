#include "ScriptCompatibility.hpp"
#include "ResourceRename.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QString converted(const QString &source) {
   const auto result=convertOpenFramesViews(source); require(result.error.isEmpty(),qPrintable(result.error)); return result.script;
}
static QString source(const QString &settings={}) {
   return "Create OpenFramesInterface Display Other; % retained Unicode α\nDisplay.Add = {Sat, Earth};\nOther.Add = {Sat, Earth};\n"+settings;
}
static QMap<QString,QString> files(const QString &settings={}) {
   return qtCameraSettings(converted(source(settings))).value("Display").objectModelFiles;
}
static QString withoutMetadata(QString value) {
   value.remove(QRegularExpression("^[ \\t]*% GMAT-Qt-Camera [^\\n]*(?:\\n|$)",QRegularExpression::MultilineOption)); return value;
}
static void conversionChecks() {
   require(files().isEmpty(),"Bare OF imposed per-plot model overrides");
   const auto paths=files("Display.DrawModelFile = {'model one, α.obj', '../Earth.obj'};\n");
   require(paths==QMap<QString,QString>{{"Sat","model one, α.obj"},{"Earth","../Earth.obj"}},"Quoted comma/space/Unicode model paths lost");
   const auto literal=files("Display.DrawModelFile = {'C:\\models\\sat.obj', 'O''Brien.obj'};\n");
   require(literal==QMap<QString,QString>{{"Sat","C:\\models\\sat.obj"},{"Earth","O''Brien.obj"}},"Legacy model paths decoded backslash/apostrophe bytes");
   require(files("Display.DrawModelFile = sat.obj;\nDisplay.DrawModelFile = earth.obj;\nDisplay.DrawModelFile = replacement.obj;\n")==QMap<QString,QString>{{"Sat","replacement.obj"},{"Earth","earth.obj"}},"Scalar model assignments did not cycle in Add order");
   require(files("Display.DrawModelFile = first.obj;\nDisplay.DrawModelFile = {'second.obj', 'third.obj', 'fourth.obj'};\n")==QMap<QString,QString>{{"Sat","third.obj"},{"Earth","fourth.obj"}},"Interpreter array elements did not use the scalar counter/wrap");
   require(files("Display.DrawModelFile = first.obj;\nDisplay.DrawModelFile = {};\nDisplay.DrawModelFile = second.obj;\n")==QMap<QString,QString>{{"Sat","first.obj"},{"Earth","second.obj"}},"Empty braced array changed files or counter");
   require(files("Display.DrawModelFile = {'first.obj', 'second.obj'};\nDisplay.DrawModelFile = '';\nDisplay.DrawModelFile = replacement.obj;\n")==QMap<QString,QString>{{"Earth","replacement.obj"}},"Quoted empty did not restore inherited model and advance counter");
   require(files("Display.DrawModelFile = first.obj;\nDisplay.Add = {Earth, Sat};\nDisplay.DrawModelFile = replacement.obj;\n")==QMap<QString,QString>{{"Sat","first.obj"},{"Earth","replacement.obj"}},"Repeated Add changed object order, model override or scalar counter");
   const auto append=files("Display.DrawModelFile = first.obj;\nDisplay.Add = Station;\nDisplay.Add = {Earth, Station};\nDisplay.DrawModelFile = second.obj;\nDisplay.DrawModelFile = third.obj;\n");
   require(append==QMap<QString,QString>{{"Sat","first.obj"},{"Earth","second.obj"},{"Station","third.obj"}},"Scalar/braced Add did not append unique names and preserve old model overrides");
   require(files("Display.DrawModelFile = first.obj;\nDisplay.Add = {};\nDisplay.DrawModelFile = second.obj;\n")==QMap<QString,QString>{{"Sat","first.obj"},{"Earth","second.obj"}},"Empty Add changed existing models/counter");
   const auto defaults=qtCameraSettings(converted(source("Display.DrawLabel = [false];\nDisplay.DrawLineWidth = [3.5];\nDisplay.DrawModelFile = first.obj;\nDisplay.Add = Station;\n"))).value("Display");
   require(!defaults.objectLabels.value("Sat") && defaults.objectLabels.value("Station") && defaults.objectLineWidths.value("Sat")==3.5 && defaults.objectLineWidths.value("Station")==2 && !defaults.objectModelFiles.contains("Station") && defaults.objectModelFiles.value("Sat")=="first.obj","Appended object defaults changed existing named display settings");
   const auto namedInitial=converted("Create OpenFramesInterface Display;\nCreate OpenFramesView Camera;\nDisplay.Add = Sat;\nDisplay.DrawModelFile = first.obj;\nDisplay.Add = Earth;\nCamera.ViewFrame = Sat;\nCamera.ViewTrajectory = On;\nDisplay.View = Camera;\n");
   require(qtCameraSettings(namedInitial).value("Display").automaticTrajectory=="Sat" && qtCameraSettings(namedInitial).value("Display").objectModelFiles==QMap<QString,QString>{{"Sat","first.obj"}},"Scalar Add or accumulated trajectory-camera membership lost");
   const auto selected=converted(source("Display.DrawModelFile = {'one.obj', 'two.obj'};\nOther.DrawModelFile = other.obj;\n"));
   require(qtCameraSettings(selected).value("Other").objectModelFiles==QMap<QString,QString>{{"Sat","other.obj"}},"Model-file counters leaked between plots");
   require(selected.contains("% Qt conversion: Display.DrawModelFile = {'one.obj', 'two.obj'};") && selected.contains("% retained Unicode α") && !selected.contains("DrawModelFile: retained as a comment; no direct Qt equivalent"),"Active model settings/comments were lost or still warned unsupported");
   const auto metadata=qtCameraSettings(selected).value("Display");
   require(setQtCameraSetting(selected,"Display",metadata)==selected,"No-op model edit changed source");
   const auto retained=retainQtCameraSettings(selected,withoutMetadata(selected));
   require(qtCameraSettings(retained).value("Display").objectModelFiles==metadata.objectModelFiles,"Source recovery lost model overrides");
   const auto removed=removeQtCameraSetting(selected,"Display");
   require(!qtCameraSettings(removed).contains("Display") && qtCameraSettings(removed).value("Other").objectModelFiles==QMap<QString,QString>{{"Sat","other.obj"}},"Owner removal changed another plot's model files");
   const auto namedView=converted(source("Create OpenFramesView Camera;\nCamera.ViewFrame = CoordinateSystem;\nDisplay.View = Camera;\nDisplay.DrawModelFile = one.obj;\n"));
   require(qtCameraSettings(namedView).value("Display").objectModelFiles==QMap<QString,QString>{{"Sat","one.obj"}},"Selected primary camera lost model overrides");
   for (const auto &bad:QStringList{
      "Display.DrawModelFile = {'unfinished.obj};\n",
      "Display.DrawModelFile = {unfinished.obj;\n",
      "Display.Add = Bad.Frame;\n"}) {
      const auto original=source(bad); const auto result=convertOpenFramesViews(original);
      require(!result.error.isEmpty() && result.script==original,"Unconvertible model assignment changed original source");
   }
   const QString noObject="Create OpenFramesInterface Display;\nDisplay.Add = {};\nDisplay.DrawModelFile = no-object.obj;\n";
   const auto rejectedEmpty=convertOpenFramesViews(noObject); require(!rejectedEmpty.error.isEmpty() && rejectedEmpty.script==noObject,"Model assignment without selected objects changed original source");
   require(qtObjectModelFiles("{\"Sat\":\"\",\"Earth\":\"earth.obj\"}")==QMap<QString,QString>{{"Earth","earth.obj"}} && qtObjectModelFilesJson({{"Sat",QString()}})=="{}","Blank model defaults were not normalized away");
   for (const auto &invalid:QStringList{"null","[]","{\"Sat\":false}","{\"Sat\":null}","{\"Sat\":1}","{\"Sat.Frame\":\"file.obj\"}","{\"\":\"\"}"}) {
      bool rejected=false; try { (void)qtObjectModelFiles(invalid); } catch (const std::exception &) { rejected=true; }
      require(rejected,"Invalid model-file JSON accepted");
   }
   const QString authored="% metadata formatting and explicit resource default stay\n  % GMAT-Qt-Camera { \"plot\": \"Display\", \"perspective\": false, \"fieldOfView\": 50, \"objectModelFiles\": {\"Sat\":\"\"} }\n";
   require(qtCameraSettings(authored).value("Display").objectModelFiles.isEmpty() && setQtCameraSetting(authored,"Display",qtCameraSettings(authored).value("Display"))==authored,"Canonical no-op discarded authored empty/default metadata bytes");
}
int main(int argc,char **argv) {
   QApplication app(argc,argv); if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("ModelDrawingBackend");
   try {
      TestSettings settings; conversionChecks(); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Source editor unavailable");
      QtCameraSetting first,other; first.objectModelFiles={{"Sat","/tmp/Sat model.obj"},{"Earth","earth.obj"}}; other.objectModelFiles={{"Sat","other.obj"}};
      const QString prefix="% Sat authored α comment\nCreate Spacecraft Sat;\nCreate String Text;\nText = 'Sat';\nCreate OrbitView Display Other;\nDisplay.Add = {Sat, Earth};\nOther.Add = {Sat};\n";
      const auto original=prefix+qtCameraDirective("Display",first)+qtCameraDirective("Other",other)+"BeginMissionSequence;\n";
      editor->setPlainText(original); require(window.buildScript(),"New model-file configuration failed Build");
      auto *object=Moderator::Instance()->GetConfiguredObject("Sat"); require(object,"Configured spacecraft missing");
      const auto renamed=renameResourceSource(original,*object,"RenamedSat");
      const auto metadata=qtCameraSettings(renamed);
      require(metadata.value("Display").objectModelFiles==QMap<QString,QString>{{"RenamedSat","/tmp/Sat model.obj"},{"Earth","earth.obj"}} && metadata.value("Other").objectModelFiles==QMap<QString,QString>{{"RenamedSat","other.obj"}},"Rename lost model references or changed literal model path");
      require(renamed.contains("% Sat authored α comment") && renamed.contains("Text = 'Sat';"),"Rename changed comments or string literals");
      editor->setPlainText(renamed); require(window.buildScript(),"Renamed model configuration failed Build");
      editor->setPlainText(original); require(window.buildScript(),"Original model configuration recovery failed");
      const QMap<QString,QString> changedFiles={{"Sat","changed model.obj"},{"Earth","earth.obj"}};
      require(window.applyResourceChanges("Display",{{"@QtObjectModelFiles",qtObjectModelFilesJson(changedFiles)}},original).isEmpty(),"Model metadata Apply failed");
      const auto changed=editor->toPlainText();
      require(qtCameraSettings(changed).value("Display").objectModelFiles==changedFiles && removeQtCameraSetting(changed,"Display")==removeQtCameraSetting(original,"Display"),"Model Apply changed unrelated source/metadata");
      editor->undo(); require(editor->toPlainText()==original,"Model Undo was not exact"); editor->redo(); require(editor->toPlainText()==changed && window.buildScript(),"Model Redo/build failed");
      const auto rejected=window.applyResourceChanges("Display",{{"@QtObjectModelFiles","{\"Missing\":\"missing.obj\"}"}},changed);
      require(!rejected.isEmpty() && editor->toPlainText()==changed && window.buildScript(),"Invalid model reference did not restore source/configuration atomically");
      require(!window.applyResourceChanges("Display",{{"@QtObjectModelFiles","{\"Sat\":false}"}},changed).isEmpty() && editor->toPlainText()==changed,"Wrong model path type altered source");
      QTemporaryDir saved; require(saved.isValid(),"Private save directory unavailable"); const auto path=saved.filePath("models α.script");
      require(window.saveScriptTo(path) && window.loadScript(path) && window.buildScript() && editor->toPlainText()==changed,"Unicode model Save/reopen lost source");
      require(window.applyResourceChanges("Display",{{"Add","Earth"}},changed).isEmpty() && qtCameraSettings(editor->toPlainText()).value("Display").objectModelFiles==QMap<QString,QString>{{"Earth","earth.obj"}},"Add removal left stale model override");
      editor->undo(); require(editor->toPlainText()==changed && window.buildScript(),"Add pruning Undo failed");
      require(window.applyResourceChanges("Display",{{"@QtObjectModelFiles","{}"}},changed).isEmpty() && qtCameraSettings(editor->toPlainText()).value("Display").objectModelFiles.isEmpty(),"Resource model inheritance could not be restored");
      require(qtCameraSettings(editor->toPlainText()).value("Other").objectModelFiles==other.objectModelFiles,"Clearing one plot changed another plot's model");
      std::cout<<"PASS model-file conversion: actual scalar/array cycling, empty/no-op/default inheritance, Add append/duplicate/empty semantics and counter retention, quoted/raw path bytes, per-plot isolation, strict metadata, exact source/no-op/recovery, Rename/literal preservation, Apply/rollback/Undo/Redo/Unicode reopen/Add pruning. Configuration only; no numerical mission or host work.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
