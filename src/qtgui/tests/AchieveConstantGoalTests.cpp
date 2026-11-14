#include "Moderator.hpp"
#include "ScriptInterpreter.hpp"
#include "Achieve.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
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
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool ok,const char *error){if(!ok)throw std::runtime_error(error);}
void write(const QString &path,const QByteArray &bytes){QFile file(path);require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Achieve evidence write failed");}
SolverBranchCommand *target(Moderator *moderator){
   for(auto *command=moderator->GetFirstCommand();command;command=command->GetNextWhileExecuting())if(command->GetTypeName()=="Target")return dynamic_cast<SolverBranchCommand *>(command);
   return nullptr;
}
Achieve *achieve(SolverBranchCommand *branch){
   if(branch)for(auto *command=branch->GetChildCommand(0);command&&command!=branch;command=command->GetNextWhileExecuting())if(auto *goal=dynamic_cast<Achieve *>(command))return goal;
   return nullptr;
}
}
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");QJsonObject proof{{"passed",false}};
   try{
      require(argc==3&&!directory.isEmpty(),"Supply startup, constant-goal case and evidence directory");const QString name=argv[2];proof["case"]=name;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());auto *moderator=Moderator::Instance();require(moderator->Initialize(startup.toStdString(),false),"Achieve native runtime failed");
      write(qEnvironmentVariable("GMAT_SOLVER_MAP_READY"),QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer timer;timer.start();
      while(!QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK"))&&timer.elapsed()<10000)QThread::msleep(5);require(QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")),"Achieve native maps not acknowledged");
      const bool variable=name=="VariableDefault"||name=="VariableFalse",allow=name.startsWith("True"),explicitFalse=name=="VariableFalse"||name=="LiteralFalse",reject=name=="LiteralDefault"||name=="LiteralFalse"||name=="TrueOverflow",inconsistent=name=="TrueInconsistent";
      require(variable||reject||name=="TrueFraction"||name=="TrueScientific"||inconsistent,"Unknown constant-goal case");
      const QString goal=variable?"Quantity":name=="TrueScientific"?"-2.5e-1":name=="TrueOverflow"?"1e309":"1.25";
      const QString desired=variable?"9":name=="TrueScientific"?"-0.25":inconsistent?"2.25":"1.25";
      const QString option=allow?", AllowConstantGoal = true":explicitFalse?", AllowConstantGoal = false":"";
      const QString statement="Achieve DC("+goal+" = "+desired+", {Tolerance = 0.0000000001"+option+"});";
      const QString source="Create Variable U Quantity Keep After;\nU = 1;\nQuantity = 0;\nKeep = 41;\nAfter = 0;\nCreate DifferentialCorrector DC;\nDC.ShowProgress = false;\nDC.MaximumIterations = 10;\nDC.ReportFile = '"+directory+"/DC.txt';\nBeginMissionSequence;\nTarget DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary DC(U = 1, {Perturbation = 0.01, Lower = -10, Upper = 10, MaxStep = 10});\nQuantity = 2 * U + 1;\n"+statement+"\nEndTarget;\nAfter = 7;\n";
      const auto script=directory+"/mission.script";write(script,source.toUtf8());const bool built=moderator->InterpretScript(script.toStdString());QJsonArray errors;
      for(const auto &error:moderator->GetScriptInterpreter()->GetErrorList())errors.append(QString::fromStdString(error));
      bool passed=built!=reject;int runStatus=-99,solverStatus=-99;bool converged=false;QJsonObject values,policy;double measured=0;
      if(reject)passed=passed&&!errors.isEmpty();
      if(built&&!reject){
         auto *command=achieve(target(moderator));require(command,"Actual Achieve command absent");const auto generated=QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS));
         require(command->GetBooleanParameter("AllowConstantGoal")==allow&&QString::fromStdString(command->GetStringParameter(command->GetParameterID("Goal")))==goal,"Native Goal or opt-in metadata differs");
         require(generated.contains("AllowConstantGoal")==(allow||explicitFalse),"Omitted/explicit goal policy serialization differs");
         if(allow||explicitFalse)require(generated.contains(allow?"AllowConstantGoal = true":"AllowConstantGoal = false"),"Serialized goal policy value differs");
         std::unique_ptr<GmatBase> clone(command->Clone());Achieve assigned;assigned=*command;
         require(clone->GetBooleanParameter("AllowConstantGoal")==allow&&assigned.GetBooleanParameter("AllowConstantGoal")==allow&&QString::fromStdString(clone->GetGeneratingString(Gmat::NO_COMMENTS))==generated&&QString::fromStdString(assigned.GetGeneratingString(Gmat::NO_COMMENTS))==generated,"Achieve clone/assignment lost explicit goal policy");
         policy=QJsonObject{{"allow",allow},{"explicit_option",allow||explicitFalse},{"measured_goal",goal},{"desired",desired},{"generated",generated},{"clone_and_assignment",true}};write(directory+"/generated-command.txt",generated.toUtf8());
         runStatus=moderator->RunMission();auto *branch=target(moderator);auto *solver=branch?dynamic_cast<Solver *>(branch->GetClone(0)):nullptr;require(solver,"Actual target solver clone absent");solverStatus=solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"));converged=solverStatus==Gmat::CONVERGED;
         command=achieve(branch);require(command,"Executed Achieve command absent");
         const auto achievedId=command->GetParameterID("AchievedValue");const double numberMeasured=command->GetNumber(achievedId);QString accessorError;
         policy["achieved_type_real"]=command->GetParameterType(achievedId)==Gmat::REAL_TYPE;policy["achieved_read_only"]=command->IsParameterReadOnly(achievedId);policy["GetNumber"]=numberMeasured;
         try{measured=command->GetRealParameter(achievedId);}catch(BaseException &e){accessorError=QString::fromStdString(e.GetFullMessage());passed=false;}
         policy["GetRealParameter_error"]=accessorError;passed=passed&&policy["achieved_type_real"].toBool()&&policy["achieved_read_only"].toBool()&&std::isfinite(numberMeasured)&&measured==numberMeasured;
         for(const auto &field:QStringList{"U","Quantity","Keep","After"}){auto *object=moderator->GetInternalObject(field.toStdString());require(object,"Actual constant-goal result absent");values[field]=object->GetRealParameter("Value");require(std::isfinite(values[field].toDouble()),"Constant-goal result is nonfinite");}
         passed=passed&&std::isfinite(measured)&&values["Keep"].toDouble()==41;
         if(inconsistent)passed=passed&&!converged&&measured==1.25&&std::abs(measured-2.25)>1e-10;
         else{passed=passed&&runStatus==1&&converged&&values["After"].toDouble()==7;const double wantU=variable?4.:1.,wantQuantity=variable?9.:3.,wantMeasured=variable?9.:name=="TrueScientific"?-.25:1.25;passed=passed&&std::abs(values["U"].toDouble()-wantU)<1e-8&&std::abs(values["Quantity"].toDouble()-wantQuantity)<1e-8&&std::abs(measured-wantMeasured)<1e-8;}
      }
      proof=QJsonObject{{"case",name},{"passed",passed},{"expected_build",!reject},{"built",built},{"inconsistent",inconsistent},{"policy",policy},{"run_status",runStatus},{"solver_status",solverStatus},{"converged",converged},{"measured",measured},{"values",values},{"errors",errors}};
   }catch(BaseException &e){proof["error"]=QString::fromStdString(e.GetFullMessage());}catch(std::exception &e){proof["error"]=QString::fromUtf8(e.what());}
   write(QDir(directory).filePath("proof.json"),QJsonDocument(proof).toJson());std::cout<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return proof["passed"].toBool()?0:1;
}
