#include "FormationAcceptance.hpp"
#include "MatlabAcceptanceStartup.hpp"
#include "MatlabInterface.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include <QDateTime>
#include <QRegularExpression>
#include <set>
using namespace formaccept;

int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FminconEphemeris");
   try {
      require(argc==4,"Supply startup, MATLAB plugin and frozen plan");
      const auto plan=QJsonDocument::fromJson(read(QFileInfo(argv[3]).absoluteFilePath())).object();
      QTemporaryDir runtime;require(runtime.isValid(),"Fmincon runtime directory unavailable");
      const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),runtime.path(),QFileInfo(argv[2]).absoluteFilePath());
      Rig rig(startup);require(rig.window.availableEngineTypes().contains("FminconOptimizer"),"Actual Fmincon factory absent");
      const auto input=plan["script"].toString();QJsonArray results,capabilities;
      for(const auto &name:QStringList{"FminconOptimizer","Yukon","VF13ad"}) {
         auto *object=Moderator::Instance()->CreateSolver(name.toStdString(),"Capability"+name.toStdString());
         if(object) capabilities.append(QJsonObject{{"type",name},{"physical_tolerance_readonly",object->IsParameterReadOnly(object->GetParameterID("CheckPhysicalTolerances"))}});
      }
      for(const auto &entry:plan["phases"].toArray()) {
         const auto phase=entry.toObject();const auto key=phase["key"].toString();
         const auto directory=rig.files.filePath(key);require(QDir().mkpath(directory),"Fmincon phase directory unavailable");
         auto script=input;script.replace("@WORK@",directory).replace("@ITERATIONS@",QString::number(phase["iterations"].toInt())).replace("@EVALUATIONS@",phase["evaluations"].toString()).replace("@MODE@",phase["mode"].toString());
         rig.source->setPlainText(script);rig.save(key+"-source.script",script.toUtf8());
         const bool built=rig.window.buildScript();events();rig.save(key+"-build.txt",rig.messages->toPlainText().toUtf8());require(built,"Fmincon ephemeris Build failed");
         const auto outcome=rig.window.runMission();events();rig.save(key+"-run.txt",rig.messages->toPlainText().toUtf8());
         auto *engine=Moderator::Instance();auto *alpha=engine->GetInternalObject("Alpha"),*cost=engine->GetInternalObject("Cost");require(alpha&&cost,"Fmincon result variables absent");
         auto *command=engine->GetFirstCommand();std::set<GmatCommand*> seen;
         while(command&&!command->IsOfType("Optimize")&&seen.insert(command).second)command=command->GetNext();
         auto *branch=dynamic_cast<SolverBranchCommand*>(command);auto *solver=branch?dynamic_cast<Solver*>(branch->GetClone(0)):nullptr;
         require(solver&&solver->GetTypeName()=="FminconOptimizer"&&!solver->IsSolverInternal(),"Actual external Fmincon optimizer did not execute");
         const int status=solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"));
         QJsonArray rows;const auto ephemeris=directory+"/Final.oem";
         if(QFileInfo::exists(ephemeris)) {
            const auto raw=read(ephemeris);rig.save(key+"-mission.oem",raw);
            for(auto line:QString::fromUtf8(raw).split('\n')) {
               if(!QRegularExpression("^\\d{4}-\\d\\d-\\d\\dT").match(line).hasMatch())continue;
               const auto fields=line.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(fields.size()==7,"Fmincon OEM row width differs");
               const auto stamp=QDateTime::fromString(fields[0]+"Z",Qt::ISODateWithMs);require(stamp.isValid(),"Fmincon OEM epoch invalid");
               const auto initial=QDateTime::fromString("2000-01-01T12:00:00.000Z",Qt::ISODateWithMs);
               QJsonArray row{initial.msecsTo(stamp)/1000.};for(int i=1;i<7;++i){bool ok=false;const double value=fields[i].toDouble(&ok);require(ok&&std::isfinite(value),"Nonfinite Fmincon ephemeris state");row.append(value);}rows.append(row);
            }
         }
         const auto trials=read(directory+"/Trials.csv");rig.save(key+"-trials.csv",trials);
         double exitFlag=0;Integer nr=0,nc=0;bool actualMatlab=false;
         if(phase["mode"].toString()=="Solve")actualMatlab=MatlabInterface::Instance()->GetRealArray("exitFlag",1,&exitFlag,nr,nc)==1;
         results.append(QJsonObject{{"key",key},{"completed",outcome==MainWindow::RunResult::Completed},{"alpha",alpha->GetRealParameter("Value")},{"cost",cost->GetRealParameter("Value")},{"native_status",status},{"converged",status==Gmat::CONVERGED||status==Gmat::IN_TOLERANCE},{"rows",rows},{"actual_matlab_exit_flag",actualMatlab},{"exit_flag",exitFlag},{"trials_present",!trials.trimmed().isEmpty()}});
         rig.save("partial-proof.json",QJsonDocument(QJsonObject{{"phases",results},{"capabilities",capabilities}}).toJson());
      }
      rig.save("proof.json",QJsonDocument(QJsonObject{{"outcome","procedure-completed"},{"phases",results},{"capabilities",capabilities},{"native_missions",results.size()}}).toJson());
      std::cout<<"Fmincon ephemeris mission phases "<<results.size()<<'\n';return 0;
   }catch(BaseException &error){std::cerr<<error.GetFullMessage();}catch(std::exception &error){std::cerr<<error.what();}return 1;
}
