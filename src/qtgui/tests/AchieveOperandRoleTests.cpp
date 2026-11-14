#include "Achieve.hpp"
#include "NumberWrapper.hpp"
#include "VariableWrapper.hpp"
#include "Variable.hpp"
#include "Moderator.hpp"
#include "SolverBranchCommand.hpp"
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
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
void write(const QString &path,const QByteArray &bytes) { QFile file(path);require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Achieve role evidence write failed"); }
const std::array<std::string,3> fields{{"Goal","GoalValue","Tolerance"}};
class ProbeAchieve : public Achieve {
public:
   std::array<ElementWrapper*,3> bindings() const { return {{goal,achieve,tolerance}}; }
};
struct CountedNumber : NumberWrapper {
   static int alive,clones,failAt;
   CountedNumber() {++alive;SetDescription("1");}
   CountedNumber(const CountedNumber &other):NumberWrapper(other) {++alive;}
   ~CountedNumber() override {--alive;}
   ElementWrapper *Clone() const override {if(++clones==failAt)throw std::runtime_error("intentional Achieve clone failure");return new CountedNumber(*this);}
};
int CountedNumber::alive=0;int CountedNumber::clones=0;int CountedNumber::failAt=-1;
void names(ProbeAchieve &command,const std::string &value) {for(const auto &field:fields)command.SetStringParameter(command.GetParameterID(field),value);}
void numeric(ProbeAchieve &command) {command.SetBooleanParameter("AllowConstantGoal",true);names(command,"1");auto *wrapper=new NumberWrapper;wrapper->SetDescription("1");require(command.SetElementWrapper(wrapper,"1"),"Achieve numeric binding failed");}
QJsonArray values(const ProbeAchieve &command) {QJsonArray result;for(auto *wrapper:command.bindings())result.append(wrapper?wrapper->EvaluateReal():-999);return result;}
void unit(const QString &name,QJsonObject &proof) {
   if(name=="DesiredValue"||name=="Tolerance"||name=="ConstantRoles") {
      Variable quantity("Quantity");quantity.SetReal(1);ProbeAchieve command;numeric(command);
      if(name!="ConstantRoles") {command.SetStringParameter(command.GetParameterID("Goal"),"Quantity");auto *wrapper=new VariableWrapper;wrapper->SetDescription("Quantity");require(wrapper->SetRefObject(&quantity)&&command.SetElementWrapper(wrapper,"Quantity"),"Achieve native Variable binding failed");}
      const bool tolerance=name=="Tolerance";command.SetField(command.GetParameterID(tolerance?"Tolerance":"GoalValue"),tolerance?.25:2);
      proof["values"]=values(command);const auto bindings=command.bindings();
      require(bindings[0]->EvaluateReal()==1&&bindings[1]->EvaluateReal()==(tolerance?1:2)&&bindings[2]->EvaluateReal()==(tolerance?.25:1),"Changing an Achieve numeric operand changed another role");
      require(quantity.EvaluateReal()==1,"Achieve literal update changed the caller's Variable");
   } else if(name=="SharedVariable") {
      Variable shared("Shared");shared.SetReal(1);ProbeAchieve command;names(command,"Shared");auto *wrapper=new VariableWrapper;wrapper->SetDescription("Shared");require(wrapper->SetRefObject(&shared)&&command.SetElementWrapper(wrapper,"Shared"),"Shared Achieve Variable binding failed");command.SetField(command.GetParameterID("GoalValue"),2);
      for(auto *w:command.bindings())require(w->EvaluateReal()==2,"Achieve replaced explicit shared Variable identity with a value copy");require(shared.EvaluateReal()==2,"Achieve shared update missed its native owner");proof["values"]=values(command);
   } else if(name=="RebindLifetime") {
      {ProbeAchieve command;command.SetBooleanParameter("AllowConstantGoal",true);names(command,"1");for(int i=0;i<5;++i)require(command.SetElementWrapper(new CountedNumber,"1"),"Achieve repeated numeric binding failed");command.ClearWrappers();require(CountedNumber::alive==0,"Achieve numeric rebinding leaked owned wrappers");}
      require(CountedNumber::alive==0,"Achieve destruction leaked owned wrappers");proof["remaining_owned_wrappers"]=CountedNumber::alive;
   } else if(name=="CloneFailure") {
      ProbeAchieve command;numeric(command);const auto before=command.bindings();auto incoming=std::make_unique<CountedNumber>();CountedNumber::clones=0;CountedNumber::failAt=2;bool rejected=false;
      try{if(command.SetElementWrapper(incoming.get(),"1"))incoming.release();}catch(const std::runtime_error &e){rejected=std::string(e.what())=="intentional Achieve clone failure";}CountedNumber::failAt=-1;
      require(rejected&&command.bindings()==before,"Failed Achieve literal staging partially replaced bindings");require(CountedNumber::alive==1,"Failed Achieve staging leaked a clone or consumed caller ownership");proof["old_bindings_preserved"]=true;
   } else if(name=="ColdCopies") {
      ProbeAchieve source;numeric(source);ProbeAchieve copied(source),assigned;numeric(assigned);assigned=source;
      for(const auto *command:{&copied,&assigned})for(auto *w:command->bindings())require(!w,"Cold Achieve copy inherited a live numeric binding");require(source.bindings()[0]->EvaluateReal()==1,"Cold Achieve copy changed its source");proof["cold_copies"]=true;
   } else throw std::runtime_error("Unknown Achieve operand unit case");
}
void native(const QString &name,const QString &directory,QJsonObject &proof) {
   const bool constant=name=="NativeConstant",tolerance=name=="NativeTolerance";
   const QString source="Create Variable Control Quantity Keep;\nQuantity = 1;\nKeep = 41;\nCreate DifferentialCorrector DC;\nDC.ShowProgress = false;\nDC.ReportFile = '"+directory+"/solver.txt';\nBeginMissionSequence;\nTarget DC {SolveMode = RunInitialGuess, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\nVary DC(Control = 2, {Perturbation = 0.1});\nAchieve DC("+(constant?QString("1"):QString("Quantity"))+" = 1, {Tolerance = 1"+(constant?QString(", AllowConstantGoal = true"):QString())+"});\nEndTarget;\nKeep = Keep + 1;\n";
   const auto path=directory+"/mission.script";write(path,source.toUtf8());auto *moderator=Moderator::Instance();require(moderator->InterpretScript(path.toStdString()),"Achieve role mission did not build");const auto status=moderator->RunMission();proof["mission_status"]=status;require(status==1,"Achieve role mission did not complete");
   SolverBranchCommand *branch=nullptr;for(auto *c=moderator->GetFirstCommand();c;c=c->GetNext())if(c->IsOfType("Target")){branch=dynamic_cast<SolverBranchCommand *>(c);break;}
   Achieve *command=nullptr;if(branch)for(auto *c=branch->GetChildCommand(0);c&&c!=branch;c=c->GetNext())if((command=dynamic_cast<Achieve *>(c)))break;require(command,"Actual native Achieve command absent");
   require(command->GetRealParameter(command->GetParameterID("Goal"))==1&&command->GetRealParameter(command->GetParameterID("GoalValue"))==1&&command->GetRealParameter(command->GetParameterID("Tolerance"))==1,"Native Achieve initial operand values differ");
   command->SetField(command->GetParameterID(tolerance?"Tolerance":"GoalValue"),tolerance?.25:2);QJsonObject values;
   for(const auto &field:fields)values[QString::fromStdString(field)]=command->GetRealParameter(command->GetParameterID(field));proof["values"]=values;
   require(values["Goal"].toDouble()==1&&values["GoalValue"].toDouble()==(tolerance?1:2)&&values["Tolerance"].toDouble()==(tolerance?.25:1),"Native Achieve operand update changed a same-text peer");require(moderator->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Achieve role mission continuation failed");
}
}
int main(int argc,char **argv) {
   QCoreApplication app(argc,argv);const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");QJsonObject proof{{"passed",false}};
   try {
      require(argc==3&&!directory.isEmpty(),"Supply Achieve operand startup, case and output");const QString name=argv[2];proof["case"]=name;const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());require(Moderator::Instance()->Initialize(startup.toStdString(),false),"Achieve operand runtime unavailable");
      write(qEnvironmentVariable("GMAT_SOLVER_MAP_READY"),QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer timer;timer.start();while(!QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK"))&&timer.elapsed()<10000)QThread::msleep(5);require(QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")),"Achieve operand native images not acknowledged");
      if(name.startsWith("Native"))native(name,directory,proof);else unit(name,proof);proof["passed"]=true;proof["scope"]="Numeric operand role independence, deliberate named-resource sharing and binding lifetimes, including actual native Target post-run API. Complete any-scalar and solver/domain/science qualification remain open.";
   }catch(BaseException &e){proof["error"]=QString::fromStdString(e.GetFullMessage());}catch(std::exception &e){proof["error"]=QString::fromUtf8(e.what());}
   write(QDir(directory).filePath("proof.json"),QJsonDocument(proof).toJson());std::cout<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return proof["passed"].toBool()?0:1;
}
