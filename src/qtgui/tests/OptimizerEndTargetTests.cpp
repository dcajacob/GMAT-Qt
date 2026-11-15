#include "Moderator.hpp"
#include "ScriptInterpreter.hpp"
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
#include <stdexcept>

namespace {
void require(bool ok,const char *error){if(!ok)throw std::runtime_error(error);}
void write(const QString &path,const QByteArray &bytes){QFile file(path);require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Terminator evidence write failed");}
QString target(const QString &inside={}){
   return "Target DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary DC(V = 1, {Perturbation = 0.01, Lower = -10, Upper = 10, MaxStep = 10});\n"+inside+"Quantity = V + 1;\nAchieve DC(Quantity = 5, {Tolerance = 0.0000000001});\nEndTarget;\n";
}
QString optimize(const QString &ending,const QString &inside={}){
   return "Optimize Opt {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary Opt(U = 1, {Perturbation = 0.000001, Lower = 0, Upper = 5, MaxStep = 5});\n"+inside+"Cost = (U - 2)^2;\nMinimize Opt(Cost);\n"+ending+";\n";
}
}
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");QJsonObject proof{{"passed",false}};
   try {
      require(argc==3&&!directory.isEmpty(),"Supply startup, terminator case and evidence directory");
      const QString name=argv[2];proof["case"]=name;const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      auto *moderator=Moderator::Instance();require(moderator->Initialize(startup.toStdString(),false),"Terminator runtime initialization failed");
      write(qEnvironmentVariable("GMAT_SOLVER_MAP_READY"),QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer timer;timer.start();
      while(!QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK"))&&timer.elapsed()<10000)QThread::msleep(5);
      require(QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")),"Terminator native maps not acknowledged");
      const bool rejected=name.startsWith("Wrong")||name=="OrphanEndTarget",worksheet=name.startsWith("Worksheet");
      const auto ending=worksheet?QString("EndTarget"):QString("EndOptimize");QString body;bool nested=false;
      if(name.endsWith("Direct"))body=optimize(ending);
      else if(name.endsWith("If"))body="If Keep == 41;\n"+optimize(ending)+"EndIf;\n";
      else if(name.endsWith("TargetInside")){body=optimize(ending,target());nested=true;}
      else if(name.endsWith("OptimizeInside")){body=target(optimize(ending));nested=true;}
      else if(name=="WrongTargetEndOptimize"){body=target();body.replace("EndTarget;","EndOptimize;");}
      else if(name=="WrongIfEndTarget")body="If Keep == 41;\nKeep = Keep + 0;\nEndTarget;\n";
      else if(name=="OrphanEndTarget")body="EndTarget;\n";
      else throw std::runtime_error("Unknown terminator case");
      QString source="Create Variable U V Cost Quantity Keep After;\nU = 1;\nV = 1;\nCost = 0;\nQuantity = 0;\nKeep = 41;\nAfter = 0;\nCreate Yukon Opt;\nOpt.ShowProgress = false;\nOpt.FeasibilityTolerance = 0.00000001;\nOpt.OptimalityTolerance = 0.00000001;\nOpt.FunctionTolerance = 0.00000001;\nOpt.ReportFile = '"+directory+"/Opt.txt';\nCreate DifferentialCorrector DC;\nDC.ShowProgress = false;\nDC.ReportFile = '"+directory+"/DC.txt';\nBeginMissionSequence;\n"+body+"After = 7;\n";
      const auto script=directory+"/mission.script";write(script,source.toUtf8());const bool built=moderator->InterpretScript(script.toStdString());QJsonArray errors;
      for(const auto &error:moderator->GetScriptInterpreter()->GetErrorList())errors.append(QString::fromStdString(error));
      bool passed=built!=rejected;int status=-99;QJsonObject values,expected{{"U",2.},{"V",nested?4.:1.},{"Cost",0.},{"Quantity",nested?5.:0.},{"Keep",41.},{"After",7.}};
      if(rejected)passed=passed&&!errors.isEmpty();
      else if(built){
         status=moderator->RunMission();passed=passed&&status==1;
         for(auto it=expected.begin();it!=expected.end();++it){auto *object=moderator->GetInternalObject(it.key().toStdString());require(object,"Terminator scalar witness missing");const auto value=object->GetRealParameter("Value");values[it.key()]=value;passed=passed&&std::isfinite(value)&&std::abs(value-it.value().toDouble())<=(it.key()=="Keep"||it.key()=="After"?0:2e-5);}
      }
      proof=QJsonObject{{"case",name},{"passed",passed},{"built",built},{"expected_build",!rejected},{"worksheet_ending",worksheet},{"nested_solver",nested},{"run_status",status},{"values",values},{"expected",expected},{"errors",errors}};
   }catch(BaseException &e){proof["error"]=QString::fromStdString(e.GetFullMessage());}catch(std::exception &e){proof["error"]=QString::fromUtf8(e.what());}
   write(QDir(directory).filePath("proof.json"),QJsonDocument(proof).toJson());std::cout<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
   return proof["passed"].toBool()?0:1;
}
