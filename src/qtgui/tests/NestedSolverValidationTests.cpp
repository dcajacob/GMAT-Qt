#include "Moderator.hpp"
#include "ScriptInterpreter.hpp"
#include "Parameter.hpp"
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
#include <vector>

namespace {
void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void write(const QString &path,const QByteArray &data){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"Solver validation evidence write failed");}
struct Case{QString name,body;bool build=true,run=true,nested=false;};
QString vary(const QString &solver="DC",const QString &variable="U"){
 return "Vary "+solver+"("+variable+" = 1, {Perturbation = 0.01, Lower = -10, Upper = 10, MaxStep = 10, AdditiveScaleFactor = 0, MultiplicativeScaleFactor = 1});\n";
}
QString controls(const QString &solver="DC",const QString &variable="U",const QString &quantity="Quantity",int goal=9){
 return vary(solver,variable)+quantity+" = "+(variable=="U"?"2 * U + 1":"V + 1")+";\nAchieve "+solver+"("+quantity+" = "+QString::number(goal)+", {Tolerance = 0.0000000001});\n";
}
QString target(const QString &body,const QString &solver="DC"){
 return "Target "+solver+" {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\n"+body+"EndTarget;\n";
}
QString branch(const QString &body,const QString &kind){
 if(kind=="If")return "If Keep == 41;\n"+body+"EndIf;\n";
 if(kind=="Else")return "If Keep == 0;\nKeep = Keep + 0;\nElse;\n"+body+"EndIf;\n";
 if(kind=="For")return "For I = 1:1:1;\n"+body+"EndFor;\n";
 if(kind=="While")return "I = 0; % initialize the loop on every solver evaluation\nWhile I < 1;\n"+body+"I = I + 1;\nEndWhile;\n";
 throw std::runtime_error("Unknown branch kind");
}
std::vector<Case> cases(){
 std::vector<Case> result{{"TargetDirect",target(controls())}};
 for(const auto &kind:QStringList{"If","Else","For","While"})result.push_back({"Target"+kind,target(branch(controls(),kind))});
 result.push_back({"TargetDoubleNesting",target(branch(branch(controls(),"For"),"If"))});
 result.push_back({"TargetInsideIf",branch(target(controls()),"If")});
 for(const auto &solver:QStringList{"Inner","DC"}){
  const auto inner=target(controls(solver,"V","OtherQuantity",3),solver);
  result.push_back({solver=="Inner"?"NestedIndependentSolver":"NestedSameSolver",target(vary()+"Quantity = 2 * U + 1;\n"+branch(inner,"If")+"Achieve DC(Quantity = 9, {Tolerance = 0.0000000001});\n"),true,true,true});
 }
 for(const auto &kind:QStringList{"If","Else","For","While"}){
  const auto body=vary("Opt")+"Cost = (U - 2)^2;\nMinimize Opt(Cost);\nNonlinearConstraint Opt(U >= 0);\n";
  result.push_back({"Optimize"+kind,"Optimize Opt {ExitMode = SaveAndContinue, ShowProgressWindow = false};\n"+branch(body,kind)+"EndOptimize;\n",true,false});
 }
 result.push_back({"OutsideIfVary",branch(vary(),"If"),false,false});
 result.push_back({"OutsideElseAchieve",branch("Achieve DC(Quantity = 9);\n","Else"),false,false});
 result.push_back({"TargetWrongNameIf",target(branch(vary("Other"),"If")+"Quantity = 2 * U + 1;\nAchieve DC(Quantity = 9);\n"),false,false});
 result.push_back({"TargetWrongNameElse",target(vary()+"Quantity = 2 * U + 1;\n"+branch("Achieve Other(Quantity = 9);\n","Else")),false,false});
 result.push_back({"NestedWrongOwner",target(controls()+target(controls("DC","V","OtherQuantity",3),"Inner")),false,false});
 result.push_back({"TargetWrongSolverType",target(controls("Opt"),"Opt"),false,false});
 result.push_back({"OptimizeWrongControlType","Optimize Opt {ShowProgressWindow = false};\n"+vary("Opt")+"Achieve DC(Quantity = 9);\nEndOptimize;\n",false,false});
 return result;
}
}
int main(int argc,char **argv){
 QCoreApplication app(argc,argv);
 try{
  require(argc==3,"Supply startup and solver-validation case");const QString name=argv[2],directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");require(!directory.isEmpty(),"Supply solver evidence directory");
  const auto all=cases();auto found=all.end();for(auto it=all.begin();it!=all.end();++it)if(it->name==name)found=it;require(found!=all.end(),"Unknown solver-validation case");const auto &test=*found;
  const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());auto *moderator=Moderator::Instance();require(moderator->Initialize(startup.toStdString(),false),"Native solver runtime failed to initialize");
  const auto ready=qEnvironmentVariable("GMAT_SOLVER_MAP_READY");
  if(!ready.isEmpty()){
   write(ready,QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer timer;timer.start();while(!QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK"))&&timer.elapsed()<10000)QThread::msleep(5);
   require(QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")),"Native solver maps not acknowledged");
  }
  QString source="Create Variable U V Quantity OtherQuantity Cost Keep I;\nU = 1;\nV = 1;\nQuantity = 0;\nOtherQuantity = 0;\nCost = 0;\nKeep = 41;\nI = 0;\nCreate DifferentialCorrector DC Other Inner;\n";
  for(const auto &solver:QStringList{"DC","Other","Inner"})source+=solver+".ShowProgress = false;\n"+solver+".MaximumIterations = 20;\n"+solver+".ReportFile = '"+directory+'/'+solver+".txt';\n";
  source+="Create Yukon Opt;\nOpt.ShowProgress = false;\nOpt.ReportFile = '"+directory+"/Opt.txt';\nBeginMissionSequence;\n"+test.body;
  const auto script=directory+"/mission.script";write(script,source.toUtf8());
  bool built=moderator->InterpretScript(script.toStdString());QJsonArray errors;
  for(const auto &error:moderator->GetScriptInterpreter()->GetErrorList())errors.append(QString::fromStdString(error));
  bool passed=built==test.build;int status=-99;QJsonObject values;
  if(!test.build)passed=passed&&!errors.isEmpty();
  if(built&&test.build&&test.run){
   status=moderator->RunMission();passed=passed&&status==1;
   for(const auto &field:QStringList{"U","V","Quantity","OtherQuantity","Keep"}){
    auto *value=moderator->GetInternalObject(field.toStdString());require(value,"Native solver result object missing");const auto number=value->GetRealParameter("Value");values[field]=number;passed=passed&&std::isfinite(number);
   }
   passed=passed&&std::abs(values["U"].toDouble()-4)<1e-8&&std::abs(values["Quantity"].toDouble()-9)<1e-8&&values["Keep"].toDouble()==41;
   if(test.nested)passed=passed&&std::abs(values["V"].toDouble()-2)<1e-8&&std::abs(values["OtherQuantity"].toDouble()-3)<1e-8;
  }
  const QJsonObject proof{{"case",name},{"expected_build",test.build},{"built",built},{"expected_run",test.run},{"nested_owner_check",test.nested},{"run_status",status},{"values",values},{"errors",errors},{"passed",passed},{"new_requirement_credit",0}};
  write(directory+"/proof.json",QJsonDocument(proof).toJson());std::cout<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';return passed?0:1;
 }catch(BaseException &e){std::cerr<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<e.what()<<'\n';}return 1;
}
