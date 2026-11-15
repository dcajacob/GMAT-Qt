#include "NativeMessageCapture.hpp"
#include "Moderator.hpp"
#include "GmatCommand.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include "Optimizer.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
void need(bool ok,const char *message) {if(!ok)throw std::runtime_error(message);}
QByteArray read(const QString &name) {QFile file(name);need(file.open(QIODevice::ReadOnly),"Missing convergence input");return file.readAll();}
void save(const QString &name,const QByteArray &data) {QFile file(name);need(file.open(QIODevice::WriteOnly)&&file.write(data)==data.size(),"Cannot save convergence proof");}
void solvers(GmatCommand *command,QJsonArray &out,std::set<GmatCommand*> &seen) {
   for(;command && seen.insert(command).second;command=command->GetNext()) {
      if(auto *branch=dynamic_cast<SolverBranchCommand*>(command)) {
         if(auto *solver=dynamic_cast<Solver*>(branch->GetClone(0))) {
            QJsonObject record{{"name",QString::fromStdString(solver->GetName())},
               {"status",solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"))},
               {"converged",solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"))==Gmat::CONVERGED ||
                  solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"))==Gmat::IN_TOLERANCE}};
            if(auto *optimizer=dynamic_cast<Optimizer*>(solver))
               record["physical_tolerances_enabled"]=optimizer->GetBooleanParameter(optimizer->GetParameterID("CheckPhysicalTolerances"));
            out.append(record);
         }
      }
      if(auto *child=command->GetChildCommand())solvers(child,out,seen);
   }
}
}
int main(int argc,char **argv) {
   QCoreApplication app(argc,argv);
   try {
      need(argc==3,"Supply startup and convergence plan");const auto plan=QJsonDocument::fromJson(read(argv[2])).object();
      const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");need(!output.isEmpty(),"Supply output directory");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      NativeMessageCapture messages;auto *engine=Moderator::Instance();need(engine->Initialize(startup.toStdString(),false),"Convergence engine init failed");
      const auto ready=qEnvironmentVariable("GMAT_STM_MAP_READY"),ack=qEnvironmentVariable("GMAT_STM_MAP_ACK");
      if(!ready.isEmpty()){save(ready,QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer t;t.start();while(!QFile::exists(ack)&&t.elapsed()<10000)QThread::msleep(5);need(QFile::exists(ack),"Convergence image receipt missing");}
      QJsonArray observations;
      for(const auto &item:plan["cases"].toArray()) {
         const auto spec=item.toObject();const auto key=spec["key"].toString();
         for(const auto &phaseItem:spec["phases"].toArray()) {
            const auto phase=phaseItem.toObject();const auto prefix=key+"-"+phase["key"].toString();
            const auto directory=QDir(output).filePath(prefix);need(QDir().mkpath(directory),"Convergence directory failed");
            auto script=phase["script"].toString();script.replace("@WORK@",directory);const auto path=directory+"/mission.script";save(path,script.toUtf8());
            messages.text.clear();const bool built=engine->InterpretScript(path.toStdString(),false,"",true);save(directory+"/build-messages.txt",QByteArray::fromStdString(messages.text));need(built,"Convergence mission did not build");
            messages.text.clear();const int status=engine->RunMission();save(directory+"/run-messages.txt",QByteArray::fromStdString(messages.text));
            auto *u=engine->GetInternalObject("U"),*v=engine->GetInternalObject("V");need(u&&v,"Convergence control variables absent");
            QJsonArray solverStates;std::set<GmatCommand*> seen;solvers(engine->GetFirstCommand(),solverStates,seen);
            const auto ephemeris=directory+"/Final only Δ.oem";
            observations.append(QJsonObject{{"case",key},{"phase",phase["key"]},{"run_status",status},
               {"U",u->GetRealParameter("Value")},{"V",v->GetRealParameter("Value")},{"solvers",solverStates},
               {"ephemeris_exists",QFileInfo::exists(ephemeris)},{"ephemeris",ephemeris},
               {"trials",directory+"/Trials.csv"}});
            save(output+"/partial-proof.json",QJsonDocument(QJsonObject{{"observations",observations}}).toJson());
         }
      }
      save(output+"/proof.json",QJsonDocument(QJsonObject{{"status","procedure-completed"},{"observations",observations},{"native_missions",observations.size()}}).toJson());
      std::cout<<"Convergence output missions "<<observations.size()<<'\n';return 0;
   }catch(BaseException &error){std::cerr<<error.GetFullMessage();}catch(std::exception &error){std::cerr<<error.what();}return 1;
}
