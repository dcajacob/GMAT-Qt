#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "OrbitObjectAxes.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include <QApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMdiSubWindow>
#include <QMdiArea>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *why) { if (!value) throw std::runtime_error(why); }
static void near(double actual,double expected,double tolerance,const char *why) {
   if (std::abs(actual-expected)>tolerance) { std::cerr<<why<<": "<<actual<<" != "<<expected<<'\n'; throw std::runtime_error(why); }
}
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Clock report missing"); return file.readAll(); }
static void preserve(const QString &name,const QByteArray &data) {
   const auto directory=qEnvironmentVariable("GMAT_QT_PLAYBACK_OPTIONS_EVIDENCE"); if (directory.isEmpty()) return;
   require(QDir().mkpath(directory),"Cannot create clock-arc evidence directory"); QFile file(directory+"/clock-arc-"+name);
   require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"Cannot save clock-arc evidence");
}
static QByteArray raw(const PlotModel &model) {
   QByteArray bytes; QDataStream stream(&bytes,QIODevice::WriteOnly);
   for (const auto &curve:model.curves) for (const auto &point:curve.points) {
      stream<<curve.name<<point.frame<<point.epoch<<point.x<<point.y<<point.z<<point.provider<<point.regularArc<<point.connect<<point.solver;
      for (double value:point.bodyToView) stream<<value;
      for (double value:point.inertialToView) stream<<value;
      for (double value:point.viewVelocity) stream<<value;
   }
   return bytes;
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   app.setOrganizationName("GMATTests"); app.setApplicationName("ClockArcCapture");
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Private clock fixture directory missing");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Clock-arc engine initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Clock-arc script editor missing");
      const auto report=files.filePath("clock arc states.txt");
      QtCameraSetting settingsForPlot; QtPlaybackSetting playback; playback.timeScale=-10.; settingsForPlot.playback=playback;
      // No segment labels or named-segment cameras request the raw arc metadata.
      // Its only consumer in this fixture is the newly saved simulation clock.
      const QString state="Report Values Sat.A1ModJulian Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z;\n";
      const QString source="Create Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\n"
         "Sat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.54605329010754;\nSat.VZ = 0;\n"
         "Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\n"
         "Create Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\n"
         "Create Variable StartEpoch StopEpoch;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\n"
         "Values.WriteHeaders = false;\nValues.WriteReport = false;\nValues.Precision = 17;\n"
         "Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nOrb.CoordinateSystem = EarthMJ2000Eq;\n"
         "Orb.DataCollectFrequency = 3;\nOrb.MaxPlotPoints = 2;\n"+qtCameraDirective("Orb",settingsForPlot)+"\n"
         "BeginMissionSequence;\nStartEpoch = Sat.A1ModJulian;\n"+state+
         "StopEpoch = StartEpoch - 37 / 86400;\nPropagate 'Backward' BackProp P(Sat) {Sat.A1ModJulian = StopEpoch};\n"+state+
         "StopEpoch = StartEpoch + 53 / 86400;\nPropagate 'Forward' P(Sat) {Sat.A1ModJulian = StopEpoch};\n"+state;
      editor->setPlainText(source); bool timedOut=false; QTimer watchdog; watchdog.setSingleShot(true);
      QObject::connect(&watchdog,&QTimer::timeout,&window,[&] { timedOut=true; window.stopMission(); }); watchdog.start(10000);
      const auto result=window.runMission(); watchdog.stop();
      if (timedOut || result!=MainWindow::RunResult::Completed)
         throw std::runtime_error(("Clock-arc mission failed\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()).toStdString());
      require(editor->toPlainText()==source,"Clock recording rewrote authored source");
      const auto reportBytes=read(report); preserve("source.script",source.toUtf8()); preserve("before.report",reportBytes);
      QVector<std::array<double,4>> rows;
      for (const auto &line:QString::fromUtf8(reportBytes).trimmed().split('\n')) {
         const auto values=line.simplified().split(' '); require(values.size()==4,"Clock report columns missing");
         std::array<double,4> row; for (int i=0;i<4;++i) { bool ok=false; row[i]=values[i].toDouble(&ok); require(ok,"Clock report value invalid"); } rows.append(row);
      }
      require(rows.size()==3,"Clock report rows missing");
      near((rows[1][0]-rows[0][0])*86400,-37.,2e-5,"Backward propagation duration incorrect");
      near((rows[2][0]-rows[0][0])*86400,53.,2e-5,"Forward propagation duration incorrect");
      auto *receiver=window.plotReceiver(); const auto model=receiver->model("Orb"); require(model!=nullptr,"Clock plot model missing");
      const auto arcs=model->regularArcs.value("Sat");
      require(arcs.size()==2 && arcs[0].name=="Backward" && arcs[1].name=="Forward","Clock-only resource did not request real accepted arc metadata");
      near(arcs[0].last.epoch,rows[1][0],1e-12,"Raw backward minimum omitted skipped stop endpoint");
      near(arcs[0].first.epoch,rows[0][0],1e-12,"Raw backward maximum omitted start endpoint");
      for (int axis=0;axis<3;++axis) {
         const double first=axis==0 ? arcs[0].first.x : axis==1 ? arcs[0].first.y : arcs[0].first.z;
         const double last=axis==0 ? arcs[0].last.x : axis==1 ? arcs[0].last.y : arcs[0].last.z;
         near(last,rows[1][axis+1],1e-7,"Backward raw stop pose differs from independent report"); near(first,rows[0][axis+1],1e-7,"Backward raw start pose differs from independent report");
      }
      const auto clock=receiver->simulationClock("Orb"); require(clock && clock->hasRange,"Recorded clock envelope missing");
      near(clock->minimum,rows[1][0],1e-12,"Display collection truncated raw clock minimum");
      near(clock->maximum,rows[2][0],1e-12,"Display collection truncated raw clock maximum");
      near(clock->epoch,rows[2][0],1e-12,"Buffered replay overwrote final raw publication epoch");
      const PlotCurve *sat=nullptr; for (const auto &curve:model->curves) if (curve.name=="Sat") sat=&curve;
      require(sat && !sat->points.empty() && sat->points.size()<=2,"Collected history point bound failed");
      for (const auto &point:sat->points) require(point.provider!="Backward" && point.regularArc,"Fixture did not trim backward samples or retain accepted arc identities");
      // Body raw endpoints also need their actual rotating frame, not identity
      // placeholders that were formerly sufficient for text-only label poses.
      const auto bodies=model->regularArcs.value("Earth"); require(!bodies.isEmpty(),"Clock did not freeze body endpoint histories");
      auto *earth=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth"); require(earth,"Earth reference unavailable");
      for (const auto *endpoint:{&bodies.front().first,&bodies.front().last}) {
         const auto expected=earth->GetAttitude(endpoint->epoch);
         for (int row=0;row<3;++row) for (int column=0;column<3;++column)
            near(endpoint->bodyToView[3*row+column],expected(row,column),1e-10,"Frozen body endpoint attitude differs from engine reference");
      }
      require(receiver->show("Orb"),"Recorded clock viewer unavailable"); PlotWidget *widget=nullptr;
      for (auto *child:receiver->workspaceArea()->subWindowList()) if (child->property("plotName")=="Orb") widget=dynamic_cast<PlotWidget *>(child->widget());
      require(widget,"Recorded clock widget missing"); const auto original=raw(*model); preserve("raw-before.bin",original);
      widget->canvas()->setSimulationEpoch(std::nullopt,PlotReplayMode::InterpolatedSteps);
      const double midpoint=(rows[0][0]+rows[1][0])/2;
      widget->canvas()->setSimulationEpoch(midpoint,PlotReplayMode::InterpolatedSteps);
      const auto *pose=orbitObjectPose(*sat,widget->canvas()->frame()); require(pose && sat->simulationArc==arcs[0].id,"Trimmed backward arc did not resolve independently at common epoch");
      near(pose->epoch,midpoint,1e-12,"Backward interpolated presentation epoch incorrect");
      near(pose->x,(rows[0][1]+rows[1][1])/2,1e-7,"Backward X did not interpolate frozen endpoint geometry");
      near(pose->y,(rows[0][2]+rows[1][2])/2,1e-7,"Backward Y did not interpolate frozen endpoint geometry");
      require(sat->simulationCursor && sat->simulationCursor->fraction>0 && sat->simulationCursor->fraction<1,"Backward clock fell back to disconnected propagation steps");
      widget->canvas()->setSimulationEpoch(midpoint,PlotReplayMode::PropagationSteps); const auto *sample=orbitObjectPose(*sat,widget->canvas()->frame());
      require(sample,"Backward propagation-mode pose missing"); near(sample->epoch,rows[0][0],1e-12,"Backward propagation mode selected later instead of preceding publication");
      require(raw(*model)==original && read(report)==reportBytes && editor->toPlainText()==source,"Epoch seeking altered raw history, numerical report or source");
      preserve("raw-after.bin",raw(*model)); preserve("after.report",read(report));
      std::cout<<"PASS one new real-engine backward/forward mission: clock-only raw arc capture, collection/trim-independent accepted envelope and body attitude, skipped endpoints matching Reports, interpolated/sampled backward replay after trimming, unchanged raw histories/report/source. Offscreen only.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
