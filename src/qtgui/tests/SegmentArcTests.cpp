#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "OrbitCamera.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <memory>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QByteArray bytes(const QString &path) {
   QFile file(path); require(file.open(QIODevice::ReadOnly),"Arc report unavailable"); return file.readAll();
}
static osg::Vec3d xyz(const std::array<double,3> &value) { return {value[0],value[1],value[2]}; }
static osg::Vec3d position(const PlotPoint &point) { return {point.x,point.y,point.z}; }
static osg::Vec3d rotate(const PlotPoint &point,const osg::Vec3d &value) {
   const auto &r=point.bodyToView;
   return {r[0]*value.x()+r[1]*value.y()+r[2]*value.z(),r[3]*value.x()+r[4]*value.y()+r[5]*value.z(),r[6]*value.x()+r[7]*value.y()+r[8]*value.z()};
}
static QVector<std::array<double,4>> rows(const QByteArray &report) {
   QVector<std::array<double,4>> result;
   for (const auto &line:QString::fromUtf8(report).trimmed().split('\n')) {
      const auto fields=line.simplified().split(' '); require(fields.size()==4,"Arc report has unexpected columns");
      std::array<double,4> row{};
      for (int i=0;i<4;++i) { bool valid=false; row[i]=fields[i].toDouble(&valid); require(valid,"Arc report has invalid numeric data"); }
      result.append(row);
   }
   return result;
}
static void endpoint(const PlotPoint &point,const std::array<double,4> &row) {
   require(std::abs(point.epoch-row[0])<1e-12 && (position(point)-osg::Vec3d(row[1],row[2],row[3])).length()<1e-7,"Captured arc endpoint differs from independently reported state");
}
static void camera(const PlotModel &model,const PlotPoint &pose) {
   require(!model.cameras.empty(),"Named regular arc camera never acquired");
   auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));
   require(sat,"Independent spacecraft attitude reference missing");
   // This fixture's plot and internal frames are both EarthMJ2000Eq. Query
   // the engine at the captured endpoint, rather than trusting the copied DCM.
   const auto expected=sat->GetAttitude(pose.epoch).Transpose();
   for (int row=0;row<3;++row) for (int column=0;column<3;++column)
      require(std::abs(pose.bodyToView[3*row+column]-expected(row,column))<1e-12,
              "Copied arc attitude differs from independent endpoint engine attitude");
   const auto &sample=model.cameras.back();
   require(!sample.solver && (xyz(sample.target)-position(pose)).length()<1e-7 &&
      (xyz(sample.eye)-(position(pose)+rotate(pose,{0,-12000,1000}))).length()<1e-7 &&
      (xyz(sample.up)-rotate(pose,{0,0,1})).length()<1e-10,"Latest segment camera lost copied regular endpoint/attitude");
   const auto basis=orbitCamera(model,model.frame,0,0,10000);
   require((basis.target-position(pose)).length()<1e-7,"Rendered camera basis ignored the selected regular arc");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtSegmentArcs");
   try {
      require((argc==2 || argc==3) && QGuiApplication::platformName()=="offscreen","Arc check requires offscreen startup and optional evidence prefix");
      const auto capture=argc==3 ? QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath() : QString();
      const auto preserve=[&](const QString &suffix,const QByteArray &data) {
         if (capture.isEmpty()) return;
         QFile file(capture+suffix);
         require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"Arc evidence cannot be preserved");
      };
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Arc fixture directory unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Arc runtime unavailable"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Arc script editor unavailable");
      const auto report=files.filePath("arc states α.txt");
      const QString resources="% preserve arc source α\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\n"
         "Sat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.54605329010754;\nSat.VZ = 0;\n"
         "Sat.Attitude = Spinner;\nSat.AttitudeDisplayStateType = EulerAngles;\nSat.EulerAngle1 = 30;\nSat.EulerAngle2 = 20;\nSat.EulerAngle3 = 10;\n"
         "Sat.AttitudeRateDisplayStateType = AngularVelocity;\nSat.AngularVelocityX = 0;\nSat.AngularVelocityY = 0;\nSat.AngularVelocityZ = 0.2;\n"
         "Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\n"
         "Create Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\n"
         "Create Variable Counter StopTime Alpha;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.WriteReport = false;\nValues.Precision = 17;\n";
      const QString state="Report Values Sat.A1ModJulian Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z;\n";
      QtCameraSetting setting; setting.segmentFrame="Sat.A"; setting.bodyRelative=true; setting.up=std::array<double,3>{0,0,1};
      const QString display="Create OrbitView Orb;\nOrb.Add = {Sat, Earth};\nOrb.CoordinateSystem = EarthMJ2000Eq;\n"
         "Orb.ViewPointReference = Sat;\nOrb.ViewPointVector = [0 -12000 1000];\nOrb.ViewDirection = [0 0 0];\n"+qtCameraDirective("Orb",setting)+"\n";
      const auto run=[&](const QString &source) {
         editor->setPlainText(source); bool timeout=false; QTimer watchdog; watchdog.setSingleShot(true);
         QObject::connect(&watchdog,&QTimer::timeout,&window,[&] { timeout=true; window.stopMission(); }); watchdog.start(10000);
         const auto result=window.runMission(); watchdog.stop();
         if (timeout || result!=MainWindow::RunResult::Completed)
            throw std::runtime_error(("Arc mission failed\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()).toStdString());
         require(editor->toPlainText()==source,"Arc visualization rewrote source");
         return bytes(report);
      };
      int scenario=0;
      const auto compare=[&](const QString &extra,const QString &mission,const QString &options) {
         const auto prefix=QString(".case%1").arg(++scenario);
         preserve(prefix+".reference.script",(resources+extra+mission).toUtf8());
         preserve(prefix+".authored.script",(resources+extra+display+options+mission).toUtf8());
         const auto expected=run(resources+extra+mission);
         preserve(prefix+".reference.report.txt",expected);
         const auto actual=run(resources+extra+display+options+mission);
         preserve(prefix+".actual.report.txt",actual);
         require(actual==expected,"Arc metadata changed complete independent numerical report");
         auto model=window.plotReceiver()->model("Orb"); require(model && model->regularArcs.contains("Sat"),"Regular arc metadata missing");
         std::cout<<"arcs="<<model->regularArcs.value("Sat").size()<<" frame="<<model->frame<<" cameras="<<model->cameras.size()<<'\n';
         return model;
      };
      // One provider pointer repeats after a real state reset. Same name and
      // monotonic epochs cannot turn the second orbit into the first one.
      const QString reset="BeginMissionSequence;\nFor Counter = 1:2;\n"
         "StopTime = Sat.A1ModJulian + 30 / 86400;\nPropagate 'A' P(Sat) {Sat.A1ModJulian = StopTime};\n"+state+
         "Sat.X = Sat.X + 200; % reset after every invocation, before loop flush\nEndFor;\n";
      auto model=compare({},reset,{}); const auto resetRows=rows(bytes(report));
      const auto &resetArcs=model->regularArcs.value("Sat");
      require(resetRows.size()==2 && resetArcs.size()==2 && resetArcs[0].providers==resetArcs[1].providers && resetArcs[0].id!=resetArcs[1].id,"Same-command reset did not create distinct regular arcs");
      endpoint(resetArcs[0].last,resetRows[0]); endpoint(resetArcs[1].last,resetRows[1]); camera(*model,resetArcs[0].last);
      require((position(resetArcs[1].last)-position(resetArcs[0].last)).length()>100,"Reset fixture did not distinguish endpoints");
      // A and B commands repeat with exact connected endpoints and a common
      // direction: the original A name and provider membership survive merge.
      const QString connected="BeginMissionSequence;\nFor Counter = 1:2;\n"
         "StopTime = Sat.A1ModJulian + 30 / 86400;\nPropagate 'A' P(Sat) {Sat.A1ModJulian = StopTime};\n"+state+
         "StopTime = Sat.A1ModJulian + 30 / 86400;\nPropagate 'B' P(Sat) {Sat.A1ModJulian = StopTime};\n"+state+"EndFor;\n";
      model=compare({},connected,{}); const auto connectedRows=rows(bytes(report)); const auto &joined=model->regularArcs.value("Sat");
      require(connectedRows.size()==4 && joined.size()==1 && joined[0].name=="A" && joined[0].providers.size()==2 && joined[0].members.size()==2 && joined[0].finalized,"Connected A-B-A did not preserve the first regular arc identity");
      endpoint(joined[0].last,connectedRows.back()); camera(*model,joined[0].last);
      // A trial-only first pass must not acquire the named camera. The accepted
      // pass's skipped endpoint and pose remain after the plot deque is trimmed.
      const QString solver="Create DifferentialCorrector DC;\nDC.ShowProgress = false;\nDC.ReportFile = '"+files.filePath("arc dc.txt")+"';\n";
      const QString accepted="BeginMissionSequence;\nTarget DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\n"
         "Vary DC(Alpha = 1, {Perturbation = 0.01, Lower = 0, Upper = 3, MaxStep = 1});\nSat.X = 7000 + 100 * Alpha;\n"
         "Propagate 'A' P(Sat) {Sat.ElapsedSecs = 37};\nAchieve DC(Alpha = 2, {Tolerance = 0.000001});\nEndTarget;\n"+state+
         "Propagate 'Later' P(Sat) {Sat.ElapsedSecs = 93};\n"+state;
      auto duplicateAccepted=std::make_shared<bool>(false);
      auto *receiver=window.plotReceiver();
      const auto originalNotify=receiver->changed;
      receiver->changed=[receiver,duplicateAccepted,originalNotify] {
         const auto observed=receiver->model("Orb");
         if (observed) for (const auto &curve:observed->curves) if (curve.name=="Sat")
            for (size_t i=1;i<curve.points.size();++i) {
               const auto &before=curve.points[i-1],&after=curve.points[i];
               if (!before.solver && !after.solver && before.provider=="A" && after.provider=="A" &&
                   before.epoch==after.epoch && before.x==after.x && before.y==after.y && before.z==after.z)
                  *duplicateAccepted=true;
            }
         if (originalNotify) originalNotify();
      };
      model=compare(solver,accepted,"Orb.SolverIterations = Current;\nOrb.DataCollectFrequency = 7;\nOrb.MaxPlotPoints = 3;\n");
      window.plotReceiver()->changed=originalNotify;
      require(!*duplicateAccepted,"Current replay duplicated an already published accepted arc sample");
      const auto solverRows=rows(bytes(report)); const auto &acceptedArcs=model->regularArcs.value("Sat");
      require(solverRows.size()==2 && acceptedArcs.size()==2 && acceptedArcs[0].name=="A" && std::abs(acceptedArcs[0].first.x-7200)<1e-5,"Trial pass became the first named regular arc");
      endpoint(acceptedArcs[0].last,solverRows[0]); camera(*model,acceptedArcs[0].last);
      bool retainedA=false;
      for (const auto &curve:model->curves) if (curve.name=="Sat") {
         require(curve.points.size()<=3,"Arc metadata bypassed the trajectory point limit");
         for (const auto &point:curve.points) if (point.provider=="A") retainedA=true;
      }
      require(!retainedA && model->cameras.size()<=3,"Trim fixture did not remove the selected arc's trajectory samples or bound camera history");
      for (const auto &sample:model->cameras) require(!sample.solver,"Current trial camera remained after trial cleanup");
      const auto target=model->cameras.back().target,eye=model->cameras.back().eye,up=model->cameras.back().up;
      auto *area=window.findChild<QMdiArea *>("workspace"); require(area && window.plotReceiver()->show("Orb"),"Trimmed arc viewer cannot open");
      area->activeSubWindow()->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(window.plotReceiver()->show("Orb") && window.plotReceiver()->model("Orb")==model && model->cameras.back().target==target && model->cameras.back().eye==eye && model->cameras.back().up==up,"Reopen lost the selected regular arc's copied pose");
      const auto source=editor->toPlainText(); const auto saved=files.filePath("arc authored α.script");
      require(window.saveScriptTo(saved) && bytes(saved)==source.toUtf8(),"Arc save changed exact authored source");
      std::cout<<"PASS same-provider reset identity, connected A-B-A regular merge, Current trial/accepted ownership plus decimation/trim/copied endpoint/reopen and exact independent reports/source. Offscreen only.\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
