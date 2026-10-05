#include "ScriptCompatibility.hpp"
#include "ResourceRename.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QString converted(const QString &source) {
   const auto result=convertOpenFramesViews(source); require(result.error.isEmpty(),qPrintable(result.error)); return result.script;
}
static QString source(const QString &settings={}) {
   return "Create Spacecraft Sat;\nCreate OpenFramesInterface Parent Child; % Unicode α stays\nParent.Add = {Sat};\nChild.Add = {Sat};\n"+settings;
}
static QtPlaybackSetting setting(const QString &settings={}) {
   const auto playback=qtCameraSettings(converted(source(settings))).value("Child").playback;
   require(playback.has_value(),"Converted OF omitted its default playback options"); return *playback;
}
static QString withoutMetadata(QString value) {
   value.remove(QRegularExpression("^[ \\t]*% GMAT-Qt-Camera [^\\n]*(?:\\n|$)",QRegularExpression::MultilineOption)); return value;
}
static void conversionChecks() {
   const auto defaults=setting(); require(defaults.timeScale==3600 && defaults.syncParent.isEmpty() && defaults.showToolbar==true,"Legacy clock/toolbar defaults changed");
   const auto signedScale=setting("Child.PlaybackTimeScale = 1.25;\nChild.PlaybackTimeScale = -12.5;\nChild.TimeSyncParent = Parent;\nChild.ShowToolbar = true;\nChild.ShowToolbar = false;\n");
   require(signedScale.timeScale==-12.5 && signedScale.syncParent=="Parent" && signedScale.showToolbar==false,"Source-ordered scale/parent/toolbar values lost");
   const auto zero=setting("Child.PlaybackTimeScale = -1;\nChild.PlaybackTimeScale = 0;\nChild.TimeSyncParent = Parent;\nChild.TimeSyncParent = '';\n");
   require(zero.timeScale==0 && zero.syncParent.isEmpty(),"Zero clock/cleared parent lost");
   const auto prior=source("Child.PlaybackTimeScale = -12.5; % preserve rate comment\nChild.TimeSyncParent = 'Parent';\nChild.ShowToolbar = false;\n");
   const auto script=converted(prior);
   require(script.contains("% Unicode α stays") && script.contains("% Qt conversion: Child.PlaybackTimeScale = -12.5; % preserve rate comment") && script.contains("% Qt conversion: Child.TimeSyncParent = 'Parent';"),"Original playback settings/comments not retained");
   const auto first=qtCameraSettings(script); const auto edited=setQtCameraSetting(script,"Child",first.value("Child"));
   require(edited==script && qtPlaybackJson(first.value("Child").playback)==qtPlaybackJson(qtCameraSettings(edited).value("Child").playback),"No-op metadata edit changed authored source");
   const auto retained=retainQtCameraSettings(script,withoutMetadata(script));
   require(qtPlaybackJson(qtCameraSettings(retained).value("Child").playback)==qtPlaybackJson(first.value("Child").playback),"Canonical source retention discarded playback");
   const auto removed=removeQtCameraSetting(script,"Parent");
   require(!qtCameraSettings(removed).contains("Parent") && qtCameraSettings(removed).value("Child").playback->syncParent=="Parent","Metadata owner removal silently cleared dependent parent");
   for (const auto &bad:QStringList{"Child.PlaybackTimeScale = NaN;","Child.PlaybackTimeScale = Infinity;","Child.PlaybackTimeScale = '2';","Child.TimeSyncParent = Sat.NamedPropagate;","Child.TimeSyncParent = Child;","Child.ShowToolbar = On;","Child.ShowToolbar = 1;"}) {
      const auto original=source(bad+'\n'); const auto result=convertOpenFramesViews(original);
      require(!result.error.isEmpty() && result.script==original,"Invalid playback conversion changed source");
   }
   require(!qtPlayback("null") && !qtPlayback("{\"timeScale\":null,\"syncParent\":\"\",\"showToolbar\":null}") && qtPlaybackJson(QtPlaybackSetting{})=="null","Empty playback/default normalization failed");
   QtPlaybackSetting toolbar; toolbar.showToolbar=false;
   const auto toolbarOnly=qtPlayback(qtPlaybackJson(toolbar)); require(toolbarOnly && !toolbarOnly->timeScale && toolbarOnly->syncParent.isEmpty() && toolbarOnly->showToolbar==false,"Toolbar-only metadata imposed a legacy clock");
   QtCameraSetting ordinary; ordinary.playback=toolbarOnly;
   const auto ordinaryDirective=qtCameraDirective("Native",ordinary); require(qtCameraSettings(ordinaryDirective).value("Native").playback->showToolbar==false,"Independent toolbar preference lost");
   ordinary.playback.reset(); require(!qtCameraSettings(qtCameraDirective("Native",ordinary)).value("Native").playback,"Bare Qt gained a playback override");
   const auto valid=QJsonDocument::fromJson(qtPlaybackJson(signedScale).toUtf8()).object();
   for (int test=0;test<8;++test) {
      auto object=valid;
      if (test==0) object["timeScale"]="3600";
      if (test==1) object["syncParent"]=true;
      if (test==2) object["showToolbar"]="false";
      if (test==3) object["timeScale"]=QJsonValue(QJsonValue::Null);
      if (test==4) object["syncParent"]="Parent.Segment";
      if (test==5) object["unknown"]=0;
      if (test==6) object.remove("showToolbar");
      if (test==7) object.remove("syncParent");
      bool rejected=false; try { (void)qtPlayback(QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact))); } catch (const std::exception &) { rejected=true; }
      require(rejected,"Invalid playback JSON accepted");
   }
   for (const auto invalid:{std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
      auto value=signedScale; value.timeScale=invalid; bool rejected=false;
      try { (void)qtPlaybackJson(value); } catch (const std::exception &) { rejected=true; }
      require(rejected,"Nonfinite scale serialized as a silent default/null");
   }
   auto self=signedScale; self.syncParent="Child"; QtCameraSetting camera; camera.playback=self; bool rejected=false;
   try { (void)qtCameraSettings(qtCameraDirective("Child",camera)); } catch (const std::exception &) { rejected=true; }
   require(rejected,"Self synchronization metadata accepted");
}
int main(int argc,char **argv) {
   QApplication app(argc,argv); if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("PlaybackOptionsBackend");
   try {
      TestSettings settings; conversionChecks(); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Source editor unavailable");
      QtCameraSetting parent,child; parent.playback=QtPlaybackSetting{-7200,{},true}; child.playback=QtPlaybackSetting{3600,"Parent",false};
      const QString prefix="% Parent Child α comment\nCreate Spacecraft Sat;\nCreate String Text;\nText = 'Parent Child';\nCreate OrbitView Parent Child;\nParent.Add = {Sat};\nChild.Add = {Sat};\n";
      const auto original=prefix+qtCameraDirective("Parent",parent)+qtCameraDirective("Child",child)+"BeginMissionSequence;\n";
      editor->setPlainText(original); require(window.buildScript(),"New parent-link configuration failed Build");
      auto *object=Moderator::Instance()->GetConfiguredObject("Parent"); require(object,"Configured parent resource missing");
      const auto renamed=renameResourceSource(original,*object,"Controller");
      require(renamed.contains("% Parent Child α comment") && renamed.contains("Text = 'Parent Child';") && renamed.contains("Create OrbitView Controller Child;"),"Rename changed comment/literal or lost resource owner");
      const auto metadata=qtCameraSettings(renamed); require(metadata.contains("Controller") && !metadata.contains("Parent") && metadata.value("Child").playback->syncParent=="Controller" && metadata.value("Controller").playback->timeScale==-7200 && metadata.value("Child").playback->showToolbar==false,"Parent Rename lost nested playback refs/settings");
      editor->setPlainText(renamed); require(window.buildScript(),"Renamed parent-link configuration failed Build");
      // Explicit null is valid and must remain null when another owner changes.
      const auto nullSource=prefix+"% GMAT-Qt-Camera {\"plot\":\"Parent\",\"perspective\":false,\"fieldOfView\":50,\"playback\":null}\nBeginMissionSequence;\n";
      editor->setPlainText(nullSource); require(window.buildScript(),"Explicit null default fixture failed Build");
      object=Moderator::Instance()->GetConfiguredObject("Parent");
      const auto nullRename=renameResourceSource(nullSource,*object,"Controller");
      require(nullRename.contains("\"playback\":null") && !qtCameraSettings(nullRename).value("Controller").playback,"Rename changed explicit null to invalid object or imposed a clock");
      std::cout<<"PASS legacy source order/default3600/signed/zero/parent/toolbar conversion; strict nullable metadata; independent Qt defaults; exact no-op/retention/removal semantics; configured parent Rename and preserved explicit null/comment/literal. Configuration only, no numerical mission or host work.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
