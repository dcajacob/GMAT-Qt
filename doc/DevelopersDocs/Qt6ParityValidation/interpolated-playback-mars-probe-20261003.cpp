#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotPlaybackControls.hpp"
#include "PlotWidget.hpp"
#include "ScriptCompatibility.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDataStream>
#include "OrbitCamera.hpp"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSlider>
#include <QTimer>
#include <algorithm>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

static void require(bool ok,const char *why) { if (!ok) throw std::runtime_error(why); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"File read failed"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly),"File write failed"); require(f.write(bytes)==bytes.size(),"Short write"); }
static QString hash(const QByteArray &bytes) { return QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()); }
static QString num(quint64 n) { return QString::number(n); }
static QByteArray rawSignature(const PlotModel &model) {
   QByteArray bytes; QDataStream stream(&bytes,QIODevice::WriteOnly);
   auto point=[&](const PlotPoint &p) {
      stream<<p.frame<<p.x<<p.y<<p.z<<p.epoch<<p.color<<p.connect<<p.solver<<p.high<<p.low<<p.marker<<p.highlighted<<p.provider<<p.hasSun<<p.hasVelocity;
      for (const auto x:p.bodyToView) stream<<x; for (const auto x:p.inertialToView) stream<<x;
      for (const auto x:p.sunPosition) stream<<x; for (const auto x:p.viewVelocity) stream<<x;
   };
   auto cameras=[&](const std::deque<PlotCamera> &h) { stream<<quint64(h.size()); for (const auto &c:h) { stream<<c.frame<<c.solver; for (const auto x:c.eye) stream<<x; for (const auto x:c.target) stream<<x; for (const auto x:c.up) stream<<x; } };
   stream<<model.frame<<model.historyGeneration;
   for (const auto &curve:model.curves) { stream<<curve.name<<quint64(curve.points.size()); for (const auto &p:curve.points) point(p); }
   cameras(model.cameras); if (model.resetView) cameras(model.resetView->cameras);
   for (const auto &view:model.cameraViews) { cameras(view.cameras); if (view.reset) cameras(view.reset->cameras); }
   for (const auto &vector:model.vectors) { stream<<vector.name<<quint64(vector.samples.size()); for (const auto &sample:vector.samples) { point(sample.source); stream<<sample.sourceRadius<<sample.valid; for (const auto x:sample.destination) stream<<x; } }
   return bytes;
}
static double epochAt(const PlotModel &model,quint64 frame) {
   if (model.replayCursor) return model.replayCursor->epoch;
   const PlotCurve *curve=nullptr; for (const auto &c:model.curves) if (c.name=="MAVEN") curve=&c;
   require(curve && !curve->points.empty(),"Missing MAVEN history");
   const PlotPoint *point=&curve->points.front(); for (const auto &p:curve->points) if (p.frame<=frame) point=&p;
   return point->epoch;
}
static PlotWidget *widget(QtPlotReceiver &receiver,const QString &name) {
   for (auto *child:receiver.workspaceArea()->subWindowList()) if (child->property("plotName")==name) return dynamic_cast<PlotWidget *>(child->widget());
   throw std::runtime_error("Missing retained plot widget");
}
int main(int argc,char **argv) {
   QApplication app(argc,argv);
   if (argc!=4 || QGuiApplication::platformName()!="offscreen") return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),source=QFileInfo(argv[2]).absoluteFilePath(),artifacts=QFileInfo(argv[3]).absoluteFilePath();
   QSettings::setDefaultFormat(QSettings::IniFormat);
   QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,artifacts+"/settings");
   QSettings::setPath(QSettings::IniFormat,QSettings::SystemScope,artifacts+"/settings/system");
   app.setOrganizationName("GMATPrivateProbe"); app.setApplicationName("MarsSegmentReplay");
   QDir::setCurrent(artifacts);
   try {
      const auto original=read(source); const auto conversion=convertOpenFramesViews(QString::fromUtf8(original));
      require(conversion.error.isEmpty(),"OF conversion failed");
      write(artifacts+"/converted.script",conversion.script.toUtf8());
      MainWindow window; require(window.initialize(startup),"Initialization failed");
      require(window.loadScript(artifacts+"/converted.script"),"Converted script load failed");
      require(window.buildScript(),"Converted script build failed");
      require(window.runMission()==MainWindow::RunResult::Completed,"Affected Mars mission did not complete");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor && editor->toPlainText()==conversion.script,"Mission source changed");
      require(read(source)==original,"Original sample changed");
      QJsonObject receipt{{"source",source},{"source_sha256",hash(original)},{"converted_sha256",hash(conversion.script.toUtf8())},{"source_unchanged",true},{"numerical_run_count",1},{"platform","offscreen"},{"limits","Retained-frame replay inspection only; no native desktop, host/GPU, new tutorial, or numerical scientific qualification"}};
      QJsonArray plots;
      auto &receiver=*window.plotReceiver();
      require(receiver.replayMode()==PlotReplayMode::InterpolatedSteps,"Interpolated mode is not the default");
      receiver.setReplayMode(PlotReplayMode::PropagationSteps);
      auto *controls=dynamic_cast<PlotPlaybackControls *>(window.findChild<QWidget *>("allPlotPlaybackControls")); require(controls,"No shared playback controls");
      auto *play=controls->findChild<QAction *>("allPlotReplayPlay"),*stop=controls->findChild<QAction *>("allPlotReplayLatest");
      auto *timer=controls->findChild<QTimer *>("allPlotReplayTimer"); require(play && timer && stop,"No shared transport");
      auto *globalMode=controls->findChild<QComboBox *>("allPlotReplayMode");
      auto *globalSpeed=controls->findChild<QComboBox *>("allPlotReplaySpeed");
      auto *globalSlower=controls->findChild<QAction *>("allPlotReplaySlower"),*globalFaster=controls->findChild<QAction *>("allPlotReplayFaster");
      require(globalMode && globalSpeed && globalSlower && globalFaster,"Missing shared mode or speed buttons");
      globalFaster->trigger(); require(globalSpeed->currentIndex()==3,"Shared Faster did not increase speed");
      globalSlower->trigger(); require(globalSpeed->currentIndex()==2,"Shared Slower did not decrease speed");
      for (const auto &name:receiver.names()) {
         const auto model=receiver.model(name); if (!model || model->kind==PlotModel::Kind::Table) continue;
         const auto originalRaw=rawSignature(*model);
         receiver.setReplayMode(PlotReplayMode::PropagationSteps);
         std::set<quint64> unique; QJsonArray curves;
         for (const auto &curve:model->curves) {
            QJsonObject c{{"name",curve.name},{"point_count",static_cast<int>(curve.points.size())}};
            quint64 gap=0,from=0,to=0; QString fromProvider,toProvider; const PlotPoint *previous=nullptr;
            for (const auto &point:curve.points) { unique.insert(point.frame); if (previous && point.frame>previous->frame+1 && point.frame-previous->frame-1>gap) { gap=point.frame-previous->frame-1; from=previous->frame; to=point.frame; fromProvider=previous->provider; toProvider=point.provider; } previous=&point; }
            c.insert("largest_missing_interval",QJsonObject{{"missing_frames",num(gap)},{"from",num(from)},{"to",num(to)},{"from_provider",fromProvider},{"to_provider",toProvider}}); curves.append(c);
         }
         require(!unique.empty(),"Affected plot has no retained samples"); std::vector<quint64> frames(unique.begin(),unique.end());
         quint64 maxGap=0,gapFrom=0,gapTo=0; for (size_t i=1;i<frames.size();++i) if (frames[i]-frames[i-1]-1>maxGap) { maxGap=frames[i]-frames[i-1]-1; gapFrom=frames[i-1]; gapTo=frames[i]; }
         const double oldSpan=static_cast<double>(model->frame-frames.front());
         QJsonObject result{{"name",name},{"callback_frame",num(model->frame)},{"retained_unique_frame_count",static_cast<int>(frames.size())},{"first_retained_frame",num(frames.front())},{"last_retained_frame",num(frames.back())},{"curves",curves},{"largest_missing_interval",QJsonObject{{"missing_frames",num(maxGap)},{"from",num(gapFrom)},{"to",num(gapTo)}}},{"old_largest_gap_fraction",oldSpan ? maxGap/oldSpan : 0},{"old_largest_gap_seconds_at_1x",oldSpan ? 3*maxGap/oldSpan : 0}};
         QJsonArray selections; auto *plot=widget(receiver,name); auto *local=plot->findChild<QSlider *>("plotTimeline"); require(local,"No local timeline");
         for (int position:{0,100,250,500,750,900,990,1000}) {
            const auto expected=position==1000 ? std::numeric_limits<quint64>::max() : frames[static_cast<size_t>((frames.size()-1)*(position/1000.0))];
            receiver.setReplayPosition(position); const auto shared=plot->canvas()->frame(); require(shared==expected,"Shared replay selected an empty frame gap");
            local->setValue(position==0 ? 1 : 0); local->setValue(position); const auto own=plot->canvas()->frame(); require(own==expected,"Local replay selected an empty frame gap");
            selections.append(QJsonObject{{"position",position},{"shared_frame",num(shared)},{"local_frame",num(own)},{"old_raw_frame",num(position==1000 ? model->frame : frames.front()+static_cast<quint64>(oldSpan*(position/1000.0)))}});
         }
         result.insert("propagation_selection_checks",selections);
         auto *ownMode=plot->findChild<QComboBox *>("plotReplayMode"),*ownSpeed=plot->findChild<QComboBox *>("plotReplaySpeed");
         auto *ownSlower=plot->findChild<QAction *>("plotReplaySlower"),*ownFaster=plot->findChild<QAction *>("plotReplayFaster");
         require(ownMode && ownSpeed && ownSlower && ownFaster,"Missing local mode/speed buttons");
         ownFaster->trigger(); require(ownSpeed->currentIndex()==3,"Local Faster did not increase speed");
         ownSlower->trigger(); require(ownSpeed->currentIndex()==2,"Local Slower did not decrease speed");
         const PlotCurve *maven=nullptr; for (const auto &c:model->curves) if (c.name=="MAVEN") maven=&c;
         require(maven && !maven->points.empty(),"Missing MAVEN retained curve");
         double connectedDays=0,disconnectedDays=0; int disconnectedEdges=0;
         for (size_t i=1;i<maven->points.size();++i) { const auto &a=maven->points[i-1],&b=maven->points[i]; if (b.connect && b.epoch>a.epoch) connectedDays+=b.epoch-a.epoch; else { disconnectedDays+=b.epoch-a.epoch; ++disconnectedEdges; } }
         result.insert("connected_time_seconds",connectedDays*86400); result.insert("disconnected_time_seconds",disconnectedDays*86400); result.insert("disconnected_edges",disconnectedEdges);
         QJsonArray cadence; double previousEpoch=0,minPropDelta=std::numeric_limits<double>::max(),maxPropDelta=-std::numeric_limits<double>::max();
         for (int position=0;position<=1000;position+=10) { receiver.setReplayPosition(position); const auto epoch=epochAt(*model,plot->canvas()->frame()); if (position) { minPropDelta=std::min(minPropDelta,(epoch-previousEpoch)*86400); maxPropDelta=std::max(maxPropDelta,(epoch-previousEpoch)*86400); } previousEpoch=epoch; }
         receiver.setReplayMode(PlotReplayMode::InterpolatedSteps);
         double minInterpDelta=std::numeric_limits<double>::max(),maxInterpDelta=-std::numeric_limits<double>::max(),maxLinearError=0,maxCameraTargetError=0; int fractionalPoses=0,checkedCameraTargets=0;
         previousEpoch=0;
         for (int position=0;position<=1000;position+=10) {
            receiver.setReplayPosition(position); const auto epoch=epochAt(*model,plot->canvas()->frame());
            const double linear=maven->points.front().epoch+(maven->points.back().epoch-maven->points.front().epoch)*(position/1000.0);
            maxLinearError=std::max(maxLinearError,std::abs(epoch-linear)*86400);
            // For a continuous mission chronology, equal slider advances must
            // equal independently known first-to-last simulation-time advances.
            if (std::abs(disconnectedDays)<1e-10) require(std::abs(epoch-linear)*86400<1e-4,"Interpolated epoch did not advance uniformly");
            if (position) { minInterpDelta=std::min(minInterpDelta,(epoch-previousEpoch)*86400); maxInterpDelta=std::max(maxInterpDelta,(epoch-previousEpoch)*86400); }
            previousEpoch=epoch;
            if (model->replayCursor && model->replayCursor->fraction>0 && model->replayCursor->fraction<1 && maven->replayPose) {
               ++fractionalPoses; require(std::abs(maven->replayPose->epoch-epoch)<1e-10,"MAVEN pose epoch lagged display epoch");
               const auto &cursor=*model->replayCursor; const PlotCamera *a=nullptr,*b=nullptr;
               for (const auto &c:model->cameras) { if (c.frame==cursor.before) a=&c; if (c.frame==cursor.after) b=&c; }
               if (a && b && a->solver==b->solver && model->scriptedCamera && !model->fitCamera && model->automaticBody.isEmpty() && model->automaticTrajectory.isEmpty()) {
                  const auto camera=orbitCamera(*model,plot->canvas()->frame(),0,0,1);
                  for (int axis=0;axis<3;++axis) maxCameraTargetError=std::max(maxCameraTargetError,std::abs(camera.target[axis]-(a->target[axis]+(b->target[axis]-a->target[axis])*cursor.fraction)));
                  ++checkedCameraTargets;
               }
            }
            local->setValue(position==0 ? 1 : 0); local->setValue(position);
            require(std::abs(epochAt(*model,plot->canvas()->frame())-epoch)<1e-10,"Local and shared interpolation epochs differ");
            cadence.append(QJsonObject{{"position",position},{"epoch",epoch},{"before",model->replayCursor ? num(model->replayCursor->before) : num(plot->canvas()->frame())},{"fraction",model->replayCursor ? model->replayCursor->fraction : 0}});
         }
         require(fractionalPoses>0,"Adaptive intervals received no interpolated poses");
         require(maxCameraTargetError<1e-5,"Camera target did not follow interpolated camera history");
         require(rawSignature(*model)==originalRaw,"Playback mutated retained numerical/camera/vector history");
         result.insert("cadence_checks",cadence); result.insert("propagation_epoch_delta_min_seconds",minPropDelta); result.insert("propagation_epoch_delta_max_seconds",maxPropDelta);
         result.insert("interpolated_epoch_delta_min_seconds",minInterpDelta); result.insert("interpolated_epoch_delta_max_seconds",maxInterpDelta); result.insert("max_global_linear_epoch_error_seconds",maxLinearError);
         result.insert("fractional_presentation_poses",fractionalPoses); result.insert("checked_interpolated_camera_targets",checkedCameraTargets); result.insert("max_camera_target_error_km",maxCameraTargetError); result.insert("retained_history_byte_identical",true);
         result.insert("retained_history_sha256",hash(originalRaw)); QJsonArray ids; for (const auto frame:frames) ids.append(num(frame)); result.insert("retained_frame_ids",ids); plots.append(result);
      }
      // Drive all 100 master timer advances without a real-time wait; every selected
      // frame must belong to that plot's retained history and all plots finish Latest.
      receiver.setReplayMode(PlotReplayMode::PropagationSteps); stop->trigger(); play->trigger(); int ticks=0;
      while (timer->isActive() && ticks<101) {
         for (const auto &name:receiver.names()) { const auto model=receiver.model(name); if (!model || model->kind==PlotModel::Kind::Table) continue; const auto selected=widget(receiver,name)->canvas()->frame(); if (selected==std::numeric_limits<quint64>::max()) continue; bool present=false; for (const auto &curve:model->curves) for (const auto &point:curve.points) present=present || point.frame==selected; require(present,"Animation tick selected empty callback gap"); }
         QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection); ++ticks;
      }
      require(ticks==100 && !timer->isActive() && receiver.replayPosition()==1000,"Shared sweep failed to finish Latest");
      receipt.insert("master_animation_ticks_checked",ticks); receipt.insert("plots",plots);
      write(artifacts+"/receipt.json",QJsonDocument(receipt).toJson()); std::cout<<"PASS one affected Mars mission; both replay modes, uniform interpolation cadence, presentation poses/cameras, raw history, speed buttons and 100 master advances checked.\n"; return 0;
   } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
