#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotPlaybackControls.hpp"
#include "PlotWidget.hpp"
#include "ScriptCompatibility.hpp"
#include <QApplication>
#include <QAction>
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
      auto *controls=dynamic_cast<PlotPlaybackControls *>(window.findChild<QWidget *>("allPlotPlaybackControls")); require(controls,"No shared playback controls");
      auto *play=controls->findChild<QAction *>("allPlotReplayPlay"),*stop=controls->findChild<QAction *>("allPlotReplayLatest");
      auto *timer=controls->findChild<QTimer *>("allPlotReplayTimer"); require(play && timer && stop,"No shared transport");
      for (const auto &name:receiver.names()) {
         const auto model=receiver.model(name); if (!model || model->kind==PlotModel::Kind::Table) continue;
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
         result.insert("selection_checks",selections); QJsonArray ids; for (const auto frame:frames) ids.append(num(frame)); result.insert("retained_frame_ids",ids); plots.append(result);
      }
      // Drive all 100 master timer advances without a real-time wait; every selected
      // frame must belong to that plot's retained history and all plots finish Latest.
      stop->trigger(); play->trigger(); int ticks=0;
      while (timer->isActive() && ticks<101) {
         for (const auto &name:receiver.names()) { const auto model=receiver.model(name); if (!model || model->kind==PlotModel::Kind::Table) continue; const auto selected=widget(receiver,name)->canvas()->frame(); if (selected==std::numeric_limits<quint64>::max()) continue; bool present=false; for (const auto &curve:model->curves) for (const auto &point:curve.points) present=present || point.frame==selected; require(present,"Animation tick selected empty callback gap"); }
         QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection); ++ticks;
      }
      require(ticks==100 && !timer->isActive() && receiver.replayPosition()==1000,"Shared sweep failed to finish Latest");
      receipt.insert("master_animation_ticks_checked",ticks); receipt.insert("plots",plots);
      write(artifacts+"/receipt.json",QJsonDocument(receipt).toJson()); std::cout<<"PASS one affected Mars mission; local/shared percentages and 100 master advances select retained frame IDs.\n"; return 0;
   } catch (const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
