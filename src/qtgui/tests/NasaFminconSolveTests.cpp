#include "FormationAcceptance.hpp"
#include "MatlabAcceptanceStartup.hpp"
#include "MatlabInterface.hpp"
#include "GmatInterface.hpp"
#include "ReportViewer.hpp"
#include <QRegularExpression>
#include <QSet>
#include <QTimer>
#include <QAction>
using namespace formaccept;
static QString quote(QString value) {return "'"+value.replace("'","''")+"'";}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("NasaFminconSolve");
   try {
      require(argc==4,"Supply startup, MATLAB plugin and fmincon profile");
      const QString requested=argv[3];const bool stopRecovery=requested=="StopRecovery";
      const QString mode=stopRecovery?QString("Constrained"):requested;const bool scalar=mode=="BoundedScalar";
      require(scalar || mode=="Constrained","Unknown real fmincon solve profile");
      QTemporaryDir runtime;require(runtime.isValid(),"Fmincon solve startup isolation failed");
      const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),runtime.path(),QFileInfo(argv[2]).absoluteFilePath());
      const auto plugin=qEnvironmentVariable("GMAT_QT_FMINCON_PLUGIN");
      require(QFileInfo(plugin).isFile(),"Actual fmincon plugin file unavailable");
      write(startup,read(startup)+"\nPLUGIN = "+plugin.toUtf8()+"\n");
      Rig rig(startup);rig.save("startup.txt",read(startup));
      require(rig.window.availableEngineTypes().contains("FminconOptimizer"),"Actual fmincon factory unavailable");
      const auto trials=rig.files.filePath("Fmincon evaluations Ω.csv");
      const QString script="% Actual MATLAB fmincon callback witness Ω\nCreate Variable Alpha Beta Cost Constraint Calls Keep;\n"
         "Alpha = 0;\nBeta = 0;\nCost = -99;\nConstraint = -99;\nCalls = 0;\nKeep = 41;\n"
         "Create FminconOptimizer Opt;\nOpt.MaximumIterations = 60;\nOpt.MaxFunEvals = '500';\n"
         "Opt.TolX = '1e-9';\nOpt.TolFun = '1e-10';\nOpt.TolCon = '1e-10';\nOpt.ShowProgress = false;\n"
         "Opt.ReportFile = "+quote(rig.files.filePath("Fmincon solver Ω.txt"))+";\n"
         "Create ReportFile Trials Values;\nTrials.Filename = "+quote(trials)+";\nTrials.SolverIterations = All;\n"
         "Trials.WriteHeaders = false;\nTrials.FixedWidth = false;\nTrials.Delimiter = ',';\nTrials.Precision = 25;\n"
         "Values.Filename = "+quote(rig.report)+";\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\n"
         "BeginMissionSequence;\nOptimize 'Real fmincon' Opt {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\n"
         "Vary Opt(Alpha = 0, {Lower = -5, Upper = "+QString(scalar?"3":"5")+", AdditiveScaleFactor = 0, MultiplicativeScaleFactor = 1});\n"+
         (scalar?QString():QString("Vary Opt(Beta = 0, {Lower = -5, Upper = 5, AdditiveScaleFactor = 0, MultiplicativeScaleFactor = 1});\n"))+
         (scalar?QString("Cost = (Alpha - 4)^2;\nConstraint = Alpha - 1;\n"):QString("Cost = (Alpha - 2)^2 + (Beta + 1)^2;\nConstraint = Alpha + Beta;\n"))+
         "Calls = Calls + 1;\nReport Trials Alpha Beta Cost Constraint Calls;\nMinimize Opt(Cost);\n"+
         (scalar?QString("NonlinearConstraint Opt(Constraint >= 0);\n"):QString("NonlinearConstraint Opt(Constraint = 0);\n"))+
         "EndOptimize;\nReport Values Alpha Beta Cost Constraint Calls Keep;\n% Preserve optimizer caller tail Ω\n";
      const auto saved=rig.files.filePath("Fmincon caller Ω.script");rig.source->setPlainText(script);
      const bool built=rig.window.buildScript();events();rig.save("build-messages.txt",rig.messages->toPlainText().toUtf8());
      if(!built)std::cerr<<rig.messages->toPlainText().toStdString();
      require(built && rig.window.saveScriptTo(saved),"Actual fmincon caller Build/Save failed");
      rig.save("caller.script",script.toUtf8());
      QJsonObject stopped;
      if(stopRecovery) {
         int requests=0;QString reentryError;std::exception_ptr failure;QTimer stopTimer;stopTimer.setInterval(5);
         QObject::connect(&stopTimer,&QTimer::timeout,[&] {
            try {
               auto *calls=Moderator::Instance()->GetInternalObject("Calls");
               if(requests || Moderator::Instance()->GetRunState()!=Gmat::RUNNING || !calls || calls->GetRealParameter("Value")<1)return;
               try {MatlabInterface::Instance()->EvalString("gmat_unexpected_reentry = 1;");}
               catch(BaseException &error){reentryError=QString::fromStdString(error.GetFullMessage());}
               require(reentryError.contains("already active"),"Actual callback allowed a reentrant MATLAB evaluation");
               auto *action=rig.window.findChild<QAction*>("stopMission");require(action&&action->isEnabled(),"Actual fmincon Stop action unavailable");
               ++requests;stopTimer.stop();action->trigger();
            }catch(...){failure=std::current_exception();stopTimer.stop();if(auto *action=rig.window.findChild<QAction*>("stopMission"))action->trigger();}
         });
         const int history=rig.source->document()->availableUndoSteps();stopTimer.start();const auto status=rig.window.runMission();stopTimer.stop();events();
         rig.save("stopped-messages.txt",rig.messages->toPlainText().toUtf8());
         if(QFileInfo::exists(trials))rig.save("stopped-evaluations.csv",read(trials));
         if(failure)std::rethrow_exception(failure);
         require(requests==1&&status==MainWindow::RunResult::Stopped,"Real fmincon callback Stop was not reported as stopped");
         require(!GmatInterface::Instance()->ExecuteCallback(),"Stopped fmincon mission retained a callback pointer");
         require(rig.source->toPlainText()==script&&rig.source->document()->availableUndoSteps()==history&&read(saved)==script.toUtf8(),"Fmincon Stop changed caller/history/disk");
         auto *matlab=MatlabInterface::Instance();matlab->RunMatlabString("gmat_after_stop = 13; gmat_reentry_present = exist('gmat_unexpected_reentry','var');");
         double value=0;Integer rows=0,cols=0;
         require(matlab->GetRealArray("gmat_after_stop",1,&value,rows,cols)==1&&value==13,"Engine did not recover after fmincon Stop");
         require(matlab->GetRealArray("gmat_reentry_present",1,&value,rows,cols)==1&&value==0,"Rejected reentrant MATLAB statement actually executed");
         stopped=QJsonObject{{"actual_Stop_action_requests",requests},{"reported_stopped",true},{"reentrant_error",reentryError},{"Engine_recovered",true},{"rejected_statement_absent",true}};
         rig.save("stop-recovery.json",QJsonDocument(stopped).toJson());
      }
      QJsonArray phases;
      for(const auto &phase:QStringList{"initial","no-explicit-build-repeat","reopened"}) {
         if(phase=="reopened")require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Fmincon Unicode reopen failed");
         const int undo=rig.source->document()->availableUndoSteps();int ticks=0;
         QTimer heartbeat;heartbeat.setInterval(1);QObject::connect(&heartbeat,&QTimer::timeout,[&]{++ticks;});heartbeat.start();
         acceptanceView(rig.window);const auto status=rig.window.runMission();heartbeat.stop();events();
         rig.save(phase+"-messages.txt",rig.messages->toPlainText().toUtf8());
         if(QFileInfo::exists(rig.report))rig.save(phase+"-report.csv",read(rig.report));
         if(QFileInfo::exists(trials))rig.save(phase+"-evaluations.csv",read(trials));
         if(status!=MainWindow::RunResult::Completed)std::cerr<<rig.messages->toPlainText().toStdString();
         require(status==MainWindow::RunResult::Completed,"Actual MATLAB fmincon mission failed");
         const double expectedAlpha=scalar?3:1.5,expectedBeta=scalar?0:-1.5,expectedCost=scalar?1:.5,expectedConstraint=scalar?2:0;
         QJsonObject native;
         for(const auto &name:QStringList{"Alpha","Beta","Cost","Constraint","Calls","Keep"}) {
            auto *object=Moderator::Instance()->GetInternalObject(name.toStdString());
            require(object&&object->GetTypeName()=="Variable","Actual fmincon native result absent");
            const auto value=object->GetRealParameter("Value");require(std::isfinite(value),"Nonfinite actual fmincon result");native[name]=value;
         }
         rig.save(phase+"-native.json",QJsonDocument(native).toJson());
         require(std::abs(native["Alpha"].toDouble()-expectedAlpha)<1e-6 && std::abs(native["Beta"].toDouble()-expectedBeta)<1e-6 &&
            std::abs(native["Cost"].toDouble()-expectedCost)<1e-8 && std::abs(native["Constraint"].toDouble()-expectedConstraint)<1e-6 && native["Keep"].toDouble()==41,
            "Real fmincon native controls/cost/constraint differ from the independent analytic optimum");
         double x[2]={0,0},cost=0,flag=0;Integer rows=0,cols=0;auto *matlab=MatlabInterface::Instance();
         require(matlab->GetRealArray("X",2,x,rows,cols)==(scalar?1:2)&&rows==(scalar?1:2)&&cols==1,"Actual MATLAB converged control vector shape differs");
         require(std::abs(x[0]-expectedAlpha)<1e-6 && (scalar || std::abs(x[1]-expectedBeta)<1e-6),"MATLAB final X differs from native analytic controls");
         require(matlab->GetRealArray("fVal",1,&cost,rows,cols)==1&&std::abs(cost-expectedCost)<1e-8,"Actual fmincon fVal differs from the analytic optimum");
         require(matlab->GetRealArray("exitFlag",1,&flag,rows,cols)==1&&flag>0,"Actual fmincon did not converge");
         const auto lines=QString::fromUtf8(read(trials)).trimmed().split('\n');require(lines.size()>=3,"Real callback evaluation trace absent");
         double previous=0;
         for(const auto &line:lines) {
            const auto fields=line.split(',');require(fields.size()==5,"Fmincon evaluation report shape differs");
            std::array<double,5> values;for(int i=0;i<5;++i){bool ok=false;values[i]=fields[i].toDouble(&ok);require(ok&&std::isfinite(values[i]),"Nonfinite fmincon evaluation report");}
            const auto formula=scalar?std::pow(values[0]-4,2):std::pow(values[0]-2,2)+std::pow(values[1]+1,2);
            const auto constraint=scalar?values[0]-1:values[0]+values[1];
            require(std::abs(values[2]-formula)<1e-10 && std::abs(values[3]-constraint)<1e-12 && values[4]==previous+1,
               "Actual GMAT callback trace differs from independent objective/constraint or evaluation sequence");previous=values[4];
         }
         require(previous==native["Calls"].toDouble()&&ticks>=3,"Fmincon callback census or Qt event delivery differs");
         const auto report=QString::fromUtf8(read(rig.report)).trimmed().split(',');require(report.size()==6,"Fmincon final report shape differs");
         const QStringList names{"Alpha","Beta","Cost","Constraint","Calls","Keep"};
         for(int i=0;i<6;++i){bool ok=false;const auto value=report[i].toDouble(&ok);require(ok&&std::abs(value-native[names[i]].toDouble())<1e-12,"Fmincon final report differs from native state");}
         auto *output=rig.window.findChild<QTreeWidget*>("Output");const auto items=output?output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive):QList<QTreeWidgetItem*>();
         require(items.size()==1,"Actual fmincon report output absent");output->itemDoubleClicked(items.first(),0);events();QMdiSubWindow *viewer=nullptr;
         for(auto *child:rig.area->subWindowList())if(dynamic_cast<ReportViewer*>(child->widget())){require(!viewer,"Ambiguous actual fmincon report viewer");viewer=child;}
         require(viewer,"Actual fmincon ReportViewer absent");auto *text=viewer->findChild<QPlainTextEdit*>();require(text&&text->isReadOnly()&&text->toPlainText().trimmed()==QString::fromUtf8(read(rig.report)).trimmed(),"Actual fmincon ReportViewer differs from raw report");
         acceptanceView(*viewer);require(viewer->close(),"Fmincon report viewer did not close");events();
         require(!GmatInterface::Instance()->ExecuteCallback(),"Completed fmincon mission retained an executable callback pointer");
         require(rig.source->toPlainText()==script&&rig.source->document()->availableUndoSteps()==undo&&read(saved)==script.toUtf8(),"Fmincon Run changed caller/history/saved source");
         phases.append(QJsonObject{{"phase",phase},{"native",native},{"actual_MATLAB_X",scalar?QJsonArray{x[0]}:QJsonArray{x[0],x[1]}},{"actual_MATLAB_fVal",cost},{"actual_MATLAB_exitFlag",flag},{"evaluations",lines.size()},{"Qt_heartbeat_ticks",ticks},{"actual_ReportViewer",true}});
         rig.save("phase-results.json",QJsonDocument(phases).toJson());
      }
      const QJsonObject proof{{"checkpoint","real-Qt-fmincon-solve"},{"profile",requested},{"phases",phases},{"stop_recovery",stopped},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return 0;
   }catch(BaseException &error){std::cerr<<error.GetFullMessage()<<'\n';return 1;}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
