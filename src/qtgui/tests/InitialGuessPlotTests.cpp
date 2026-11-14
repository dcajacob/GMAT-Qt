// Supplement the affected RunInitialGuess branch without replaying the original
// plot integration suite. Its helpers and independent bounds remain unchanged.
#define main retainedPlotIntegrationStepMain
#include "PlotIntegrationStepTests.cpp"
#undef main

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("InitialGuessPlots");
   try {
      require(argc==3,"Supply startup and Target/Optimize");
      const QString context=argv[2];require(context=="Target"||context=="Optimize","Unknown plot solver context");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto report=rig.files.filePath("Initial guess Ω.csv");
      QString setup=config(report,0,false,false);
      QString definitions="Create Variable Alpha;\nAlpha = 0;\n";
      definitions+=context=="Target"?"Create DifferentialCorrector CheckSolver;\n":"Create Yukon CheckSolver;\n";
      definitions+="CheckSolver.ReportFile = '"+rig.files.filePath("solver.txt")+"';\nOrb.SolverIterations = Current;\nMap.SolverIterations = Current;\n";
      setup.replace("BeginMissionSequence;",definitions+"BeginMissionSequence;");
      QString mission="Propagate P(One) {One.ElapsedSecs = 10};\n"+context+
         " CheckSolver {SolveMode = RunInitialGuess, ExitMode = SaveAndContinue, ShowProgressWindow = false};\n"
         "Vary CheckSolver(Alpha = 0, {Perturbation = 0.01, Lower = -5, Upper = 5, MaxStep = 1});\n"
         "Report Values Alpha;\nPropagate P(One) {One.ElapsedSecs = 27};\n";
      mission+=context=="Target"?"Achieve CheckSolver(Alpha = 0, {Tolerance = 1e-8});\nEndTarget;\n":
         "Minimize CheckSolver(Alpha);\nEndOptimize;\n";
      mission+="Propagate P(One) {One.ElapsedSecs = 13};\n";
      rig.source->setPlainText(setup+mission);require(rig.window.buildScript(),"Initial-guess plot Build failed");
      editInteger(rig,"Orb","IntegrationStepsToSkip",1);
      editInteger(rig,"Map","IntegrationStepsToSkip",1);
      const auto script=rig.source->toPlainText(),saved=rig.files.filePath("Initial guess Ω.script");
      require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==script,"Initial-guess plot save/reopen failed");
      rig.save("mission.script",script.toUtf8());
      const auto result=rig.window.runMission();events();
      rig.save("messages.txt",rig.messages->toPlainText().toUtf8());
      require(result==MainWindow::RunResult::Completed,"Initial-guess plot mission failed");
      const auto lines=QString::fromUtf8(read(report)).trimmed().split('\n');
      require(lines.size()==2,"Expected nominal and final initial-guess passes");
      for(const auto &line:lines){bool ok=false;require(line.trimmed().toDouble(&ok)==0&&ok,"Initial guess changed its literal control");}
      const QVector<double> times{0,20,37,50};QJsonArray observations;
      for(const auto &name:QStringList{"Orb","Map"}) {
         const auto model=rig.window.plotReceiver()->model(name);require(model&&model->endOfRun,"Initial-guess plot incomplete");
         for(int object=0;object<2;++object) {
            const PlotCurve *curve=nullptr;
            for(const auto &entry:model->curves)if(entry.name==orbits[object].name)curve=&entry;
            require((curve?curve->points.size():0)==size_t(object==0?times.size():0),"Current initial-guess plot retained trial samples or lost accepted cadence");
            if(object!=0)continue;
            QJsonArray points;
            for(int i=0;i<times.size();++i) {
               const auto &point=curve->points[i];const double elapsed=(point.epoch-21545)*86400;
               require(std::abs(elapsed-times[i])<1e-4,"Initial-guess plot epoch differs from retained cadence oracle");
               require(std::isfinite(point.x)&&std::isfinite(point.y)&&std::isfinite(point.z),"Initial-guess plot has nonfinite coordinates");
               if(name=="Orb") {
                  const auto truth=circular(orbits[0],times[i]);
                  require(std::abs(point.x-truth[0])<4e-5&&std::abs(point.y-truth[1])<4e-5&&std::abs(point.z-truth[2])<4e-5,"Initial-guess orbit differs from independent circle");
                  require(point.hasVelocity,"Initial-guess sampled velocity missing");
                  for(int j=0;j<3;++j)require(std::abs(point.viewVelocity[j]-truth[j+3])<5e-8,"Initial-guess velocity differs from independent circle");
               }
               points.append(QJsonObject{{"elapsed",elapsed},{"x",point.x},{"y",point.y},{"z",point.z}});
            }
            observations.append(QJsonObject{{"plot",name},{"object",orbits[object].name},{"points",points}});
         }
      }
      const QJsonObject proof{{"outcome","passed"},{"context",context},{"mode","RunInitialGuess"},{"passes",2},{"expected_times",QJsonArray{0,20,37,50}},{"plots",observations},
         {"scope","Two affected initial-guess checkpoints with actual skip controls/history/persistence; Current OrbitView and GroundTrack cadence, independent circular positions/velocities for OrbitView, finite-only ground coordinates. Original suite and bounds unchanged; no broad frame or mission qualification."}};
      rig.save("proof.json",QJsonDocument(proof).toJson());
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return 0;
   }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}
   catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<std::endl;}
   return 1;
}
