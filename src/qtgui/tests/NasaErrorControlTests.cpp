#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}

static GmatBase *forceModel()
{
   auto *model=Moderator::Instance()->GetConfiguredObject("Gravity");
   require(model,"Force model missing"); return model;
}

static QMdiSubWindow *forceWindow(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (child->property("resourceName").toString()=="Gravity") return child;
   throw std::runtime_error("Force model window missing");
}

static ResourceEditor *openForceModel(MainWindow &window)
{
   const auto items=window.findChild<QTreeWidget *>("Resources")->findItems("Gravity",Qt::MatchExactly|Qt::MatchRecursive);
   require(items.size()==1,"Force model tree item missing");
   window.findChild<QTreeWidget *>("Resources")->itemDoubleClicked(items.front(),0);
   acceptanceView(window);
   auto *panel=dynamic_cast<ResourceEditor *>(forceWindow(window)->widget());
   require(panel,"Force model editor missing"); return panel;
}

static void closeForceModel(MainWindow &window)
{
   require(forceWindow(window)->close(),"Clean force model window did not close");
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}

static void accuracyStopChecks(MainWindow &window,QPlainTextEdit &editor,const QString &directory,bool terminalOnly=false)
{
   const QStringList methods{"RungeKutta89","RungeKutta68","RungeKutta56","PrinceDormand45","PrinceDormand78","AdamsBashforthMoulton","BulirschStoer"};
   const QStringList policies=terminalOnly ? QStringList{"short-final-warn","short-final-fixed","short-final-loose","backward-short-final-warn","backward-short-final-fixed","backward-short-final-loose","first-final-warn","first-final-fixed","first-final-loose","backward-first-final-warn","backward-first-final-fixed","backward-first-final-loose"} : QStringList{"stop","warn","fixed","loose","reduce-stop","reduce-warn","backward-stop","backward-warn","backward-reduce-stop","backward-reduce-warn","backward-loose"};
   QStringList failures;
   for (const auto &method:methods) for (const auto &policy:policies) {
      std::cout<<"Accuracy policy START "<<method.toStdString()<<" "<<policy.toStdString()<<std::endl;
      const bool stop=!policy.endsWith("warn"),mustStop=policy.endsWith("stop");
      const double direction=policy.startsWith("backward-") ? -1 : 1;
      const double target=terminalOnly ? (policy.contains("first-final") ? 50 : 1850) : 1800;
      const auto output=directory+"/accuracy-"+method+"-"+policy+".txt";
      QString source=QString(R"GMAT(Earth.Mu = 398600.4418;
Create Spacecraft Vehicle;
Vehicle.CoordinateSystem = EarthMJ2000Eq;
Vehicle.DisplayStateType = Cartesian;
Vehicle.X = 7000;
Vehicle.Y = 0;
Vehicle.Z = 0;
Vehicle.VX = 0;
Vehicle.VY = 7.546053290107542;
Vehicle.VZ = 0;
Create ForceModel Gravity;
Gravity.CentralBody = Earth;
Gravity.PrimaryBodies = {};
Gravity.PointMasses = {Earth};
Gravity.Drag = None;
Gravity.SRP = Off;
Gravity.ErrorControl = RSSStep;
Create Propagator Integrator;
Integrator.Type = %1;
Integrator.FM = Gravity;
Integrator.InitialStepSize = 300;
Integrator.MinStep = 300;
Integrator.MaxStep = 300;
Integrator.Accuracy = 1e-30;
Integrator.MaxStepAttempts = 100;
%3Create Variable Reached;
Reached = 0;
Create ReportFile Values;
Values.Filename = '%4';
Values.WriteHeaders = false;
Values.WriteReport = false;
Values.Precision = 17;
Integrator.StopIfAccuracyIsViolated = %2;
BeginMissionSequence;
Propagate Integrator(Vehicle) {Vehicle.ElapsedSecs = 1800};
Reached = 1;
Report Values Vehicle.ElapsedSecs Vehicle.X Vehicle.Y Vehicle.Z Vehicle.VX Vehicle.VY Vehicle.VZ Reached;
)GMAT").arg(method,stop ? "false" : "true",method=="AdamsBashforthMoulton" ? "Integrator.LowerError = 1e-34;\nIntegrator.TargetError = 1e-32;\n" : "",output);
      if (policy.contains("reduce-")) { source.replace("InitialStepSize = 300","InitialStepSize = 600"); source.replace("MaxStep = 300","MaxStep = 600"); }
      source.replace("ElapsedSecs = 1800",QString("ElapsedSecs = %1").arg(direction*target));
      if (direction<0) { if (!policy.contains("first-final")) source.replace("InitialStepSize = ","InitialStepSize = -"); source.replace("Propagate Integrator(","Propagate BackProp Integrator("); }
      if (policy.endsWith("loose")) { source.replace("Accuracy = 1e-30","Accuracy = 1e-2"); source.replace("LowerError = 1e-34","LowerError = 8e-3"); source.replace("TargetError = 1e-32","TargetError = 9e-3"); }
      const auto stepsFile=output+".steps";
      source.replace("BeginMissionSequence;\n","Create ReportFile Steps;\nSteps.Filename = '"+stepsFile+"';\nSteps.WriteHeaders = false;\nSteps.Precision = 17;\nSteps.Add = {Vehicle.ElapsedSecs};\nBeginMissionSequence;\n");
      editor.setPlainText(source); require(window.buildScript(),"Accuracy-policy fixture failed to build");
      if (policy.endsWith("fixed")) {
         auto *panel=openForceModel(window); auto *mode=panel->findChild<QComboBox *>("resource_ErrorControl"); require(mode,"Fixed-step selector missing");
         mode->setCurrentText("None"); require(panel->hasChanges(),"Fixed-step mode did not remain pending"); panel->requestApply();
         QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(forceModel()->GetStringParameter("ErrorControl")=="None","Fixed-step selector did not apply"); closeForceModel(window);
      }
      const auto before=editor.toPlainText();
      auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems("Integrator",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Integrator tree item missing"); tree->itemDoubleClicked(items.front(),0); acceptanceView(window);
      QMdiSubWindow *child=nullptr;
      for (auto *candidate:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (candidate->property("resourceName").toString()=="Integrator") child=candidate;
      auto *panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr; require(panel,"Accuracy-policy integrator editor missing");
      auto *flag=panel->findChild<QComboBox *>("resource_StopIfAccuracyIsViolated"); require(flag,"Accuracy-policy selector missing"); flag->setCurrentText(stop ? "true" : "false");
      require(panel->hasChanges() && editor.toPlainText()==before,"Accuracy policy escaped pending isolation");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      auto *configured=Moderator::Instance()->GetConfiguredObject("Integrator"); require(configured && configured->GetBooleanParameter("StopIfAccuracyIsViolated")==stop,"Accuracy policy did not apply");
      const auto applied=editor.toPlainText(); QString expected=before,unrelated=applied;
      // The edited assignment may move within the configuration section. All
      // other bytes must remain exact, and the assignment must remain unique.
      const QRegularExpression policyLine("^(?:GMAT[ \\t]+)?Integrator\\.StopIfAccuracyIsViolated[ \\t]*=[^;\\n]*;[ \\t]*\\n",QRegularExpression::MultilineOption);
      expected.remove(policyLine); unrelated.remove(policyLine);
      require(unrelated==expected && applied.count("Integrator.StopIfAccuracyIsViolated") == 1,"Accuracy policy changed unrelated source");
      require(child->close(),"Clean accuracy-policy editor did not close"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      editor.undo(); require(editor.toPlainText()==before && window.buildScript(),"Accuracy policy Undo was not exact");
      editor.redo(); require(editor.toPlainText()==applied && window.buildScript(),"Accuracy policy Redo was not exact");
      auto *messages=window.findChild<QPlainTextEdit *>("messageWindow"); require(messages,"Message window missing"); messages->clear();
      QFile::remove(output); QFile::remove(stepsFile); const auto outcome=window.runMission(); QApplication::processEvents();
      const auto diagnostics=messages->toPlainText(); const auto phrase=QString("Accuracy settings will be violated");
      QFile report(output); QString contents; if (report.open(QIODevice::ReadOnly)) contents=QString::fromUtf8(report.readAll()).trimmed();
      const auto values=contents.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
      auto *runtime=dynamic_cast<PropSetup *>(Moderator::Instance()->GetInternalObject("Integrator"));
      const bool runtimePolicy=runtime && runtime->GetPropagator()->GetTypeName()==method.toStdString() && runtime->GetBooleanParameter("StopIfAccuracyIsViolated")==stop && runtime->GetRealParameter("MinStep")==300 && runtime->GetRealParameter("MaxStep")== (policy.contains("reduce-") ? 600 : 300);
      const bool terminated=outcome==MainWindow::RunResult::Failed && diagnostics.contains(phrase,Qt::CaseInsensitive) && contents.isEmpty();
      bool completed=outcome==MainWindow::RunResult::Completed && values.size()==8 && std::abs(values.value(0).toDouble()-direction*target)<1e-6 && values.value(7).toDouble()==1;
      for (const auto &value:values) { bool valid=false; const auto number=value.toDouble(&valid); completed=completed && valid && std::isfinite(number); }
      const bool warning=diagnostics.contains(phrase,Qt::CaseInsensitive);
      bool grid=true; QFile steps(stepsFile);
      if (completed) {
         grid=steps.open(QIODevice::ReadOnly); const auto epochs=grid ? QString::fromUtf8(steps.readAll()).trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts) : QStringList{};
         grid=grid && epochs.size()>1; double previous=0; bool firstEpoch=true;
         const double maximum=policy.contains("reduce-") ? 600 : 300;
         for (const auto &epoch:epochs) { bool ok=false; const double time=epoch.toDouble(&ok),step=direction*(time-previous); const bool finalException=terminalOnly && std::abs(time-direction*target)<1e-6 && step>0; grid=grid && ok && std::isfinite(time) && (firstEpoch ? std::abs(time)<1e-6 : (step>=300-1e-6 || finalException) && step<=maximum+1e-6); previous=time; firstEpoch=false; }
         grid=grid && std::abs(previous-direction*target)<1e-6;
      }
      const bool passed=runtimePolicy && (mustStop ? terminated : completed && grid && (policy.endsWith("warn") ? warning : !warning));
      std::cout<<"Accuracy policy "<<method.toStdString()<<" "<<policy.toStdString()<<": "<<(passed ? "PASS" : "FAIL")<<std::endl;
      if (!passed) { failures.append(method+"."+policy); std::cerr<<"Outcome="<<static_cast<int>(outcome)<<" runtime="<<runtimePolicy<<" completed="<<completed<<" grid="<<grid<<"\n"<<diagnostics.toStdString()<<"\nReport: "<<contents.toStdString()<<'\n'; }
   }
   require(failures.isEmpty(),qPrintable("Accuracy-policy behavior mismatch: "+failures.join(", ")));
   for (const auto &method:methods) for (const auto &policy:policies) {
      const QJsonObject proof{{"checkpoint","accuracy-policy."+method+"."+policy},{"method",method},{"policy",policy},{"outcome","passed"},{"facets",QJsonArray{"gui-policy","pending-isolation","undo-redo","runtime-policy","mission-outcome","diagnostic","post-propagation-reachability","step-bounds"}}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
   }
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaErrorControl");
   if (argc!=2 || !acceptanceWidgetPlatform()) return 2;
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Script editor missing");
      if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_TERMINAL_ONLY")=="1") { accuracyStopChecks(window,*editor,files.path(),true); return 0; }
      const QStringList modes{"RSSStep","RSSState","LargestStep","LargestState","None"};
      const double mu=398600.4418,radius=7000,speed=std::sqrt(mu/radius),duration=600;
      for (const auto &mode:modes) {
         const auto report=files.filePath("circle-"+mode+".txt");
         const QString mission="BeginMissionSequence;\nPropagate Integrator(Vehicle) {Vehicle.ElapsedSecs = 600};\n";
         const QString baseline="% Independent circular orbit and error-control GUI selection\nEarth.Mu = 398600.4418;\nCreate Spacecraft Vehicle;\nVehicle.CoordinateSystem = EarthMJ2000Eq;\nVehicle.DisplayStateType = Cartesian;\nVehicle.X = 7000;\nVehicle.Y = 0;\nVehicle.Z = 0;\nVehicle.VX = 0;\nVehicle.VY = "+QString::number(speed,'g',17)+";\nVehicle.VZ = 0;\nCreate ForceModel Gravity;\nGravity.CentralBody = Earth;\nGravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\nGravity.Drag = None;\nGravity.SRP = Off;\nGravity.ErrorControl = "+(mode=="RSSStep" ? QString("RSSState") : QString("RSSStep"))+";\nCreate Propagator Integrator;\nIntegrator.Type = RungeKutta89;\nIntegrator.FM = Gravity;\nIntegrator.InitialStepSize = 10;\nIntegrator.Accuracy = 1e-12;\nIntegrator.MinStep = 0;\nIntegrator.MaxStep = 10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 17;\nValues.Add = {Vehicle.ElapsedSecs, Vehicle.X, Vehicle.Y, Vehicle.Z, Vehicle.VX, Vehicle.VY, Vehicle.VZ};\n"+mission;
         editor->setPlainText(baseline); require(window.buildScript(),"Circular-orbit fixture did not build");
         const auto oldMode=forceModel()->GetStringParameter("ErrorControl");
         auto *panel=openForceModel(window);
         auto *choice=panel->findChild<QComboBox *>("resource_ErrorControl"); require(choice,"Error-control selector missing");
         for (const auto &expected:modes) require(choice->findText(expected)>=0,"Documented error-control mode missing");
         choice->setCurrentText(mode);
         require(panel->hasChanges() && editor->toPlainText()==baseline && forceModel()->GetStringParameter("ErrorControl")==oldMode,"Pending error mode changed committed source/model");
         panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
         panel=dynamic_cast<ResourceEditor *>(forceWindow(window)->widget());
         require(panel && !panel->hasChanges() && forceModel()->GetStringParameter("ErrorControl")==mode.toStdString(),"Selected error mode did not apply");
         const auto applied=editor->toPlainText();
         require(applied.endsWith(mission),"Error mode changed the mission sequence"); closeForceModel(window);
         editor->undo(); require(editor->toPlainText()==baseline && window.buildScript() && forceModel()->GetStringParameter("ErrorControl")==oldMode,"Error mode Undo did not restore exact source/model");
         editor->redo(); require(editor->toPlainText()==applied && window.buildScript() && forceModel()->GetStringParameter("ErrorControl")==mode.toStdString(),"Error mode Redo did not restore source/model");
         const auto saved=files.filePath("mode-"+mode+".script");
         require(window.saveScriptTo(saved) && window.loadScript(saved) && editor->toPlainText()==applied && forceModel()->GetStringParameter("ErrorControl")==mode.toStdString(),"Error mode save/reopen lost settings");
         QFile::remove(report); require(!QFileInfo::exists(report),"Stale circular-orbit report remains");
         require(window.runMission()==MainWindow::RunResult::Completed,"Circular-orbit mission failed");
         QFile output(report); require(output.open(QIODevice::ReadOnly),"Circular-orbit report missing");
         const auto rows=QString::fromUtf8(output.readAll()).trimmed().split('\n',Qt::SkipEmptyParts);
         require(rows.size()>2,"Circular-orbit report has no trajectory samples");
         double previous=-1,maxPositionError=0,maxVelocityError=0;
         for (const auto &row:rows) {
            const auto columns=row.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
            require(columns.size()==7,"Trajectory report row has the wrong shape");
            bool valid=false; const double time=columns[0].toDouble(&valid);
            require(valid && std::isfinite(time) && time>=0 && time>previous,"Trajectory epochs are invalid or unordered");
            if (mode=="None") require(std::abs(time-(previous<0 ? 0 : previous+10))<1e-6,"Fixed-step mode did not retain the requested 10-second grid");
            const double angle=std::sqrt(mu/(radius*radius*radius))*time;
            const double truth[]={radius*std::cos(angle),radius*std::sin(angle),0,-speed*std::sin(angle),speed*std::cos(angle),0};
            for (int i=0;i<6;++i) {
               const double value=columns[i+1].toDouble(&valid),error=std::abs(value-truth[i]);
               require(valid && std::isfinite(value) && error<(i<3 ? 2e-5 : 2e-8),"Trajectory differs from the independent circular-orbit solution");
               if (i<3) maxPositionError=std::max(maxPositionError,error); else maxVelocityError=std::max(maxVelocityError,error);
            }
            previous=time;
         }
         require(std::abs(previous-duration)<1e-6,"Trajectory did not reach the requested final epoch");
         auto *runtime=Moderator::Instance()->GetInternalObject("Gravity");
         require(runtime && runtime->GetStringParameter("ErrorControl")==mode.toStdString(),"Runtime force model used a different error mode");
         const QJsonObject proof{{"checkpoint","error-control."+mode},{"mode",mode},{"outcome","passed"},{"samples",rows.size()},{"max_position_error_km",maxPositionError},{"max_velocity_error_km_s",maxVelocityError},{"facets",QJsonArray{"gui-selection","pending-isolation","undo-redo","save-reopen","runtime-mode","analytic-trajectory","step-grid"}}};
         std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      }
      accuracyStopChecks(window,*editor,files.path());
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
