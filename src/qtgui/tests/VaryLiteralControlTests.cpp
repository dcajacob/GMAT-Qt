#include "Vary.hpp"
#include "NumberWrapper.hpp"
#include "VariableWrapper.hpp"
#include "Variable.hpp"
#include "Moderator.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include "Spacecraft.hpp"
#include "Array.hpp"
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
#include <cmath>
#include <iostream>
#include <memory>
#include <limits>
#include <stdexcept>

namespace {
void require(bool value,const char *message) { if(!value)throw std::runtime_error(message); }
void save(const QString &path,const QByteArray &bytes) { QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Cannot save Vary literal evidence"); }
const std::array<std::string,8> fields{{"Variable","InitialValue","Perturbation","Lower","Upper","MaxStep","AdditiveScaleFactor","MultiplicativeScaleFactor"}};
class ProbeVary : public Vary {
public:
   std::array<ElementWrapper*,8> bindings() const { return {{variable,initialValue,perturbation,variableLower,variableUpper,variableMaximumStep,additiveScaleFactor,multiplicativeScaleFactor}}; }
};
struct CountedNumber : NumberWrapper {
   static int alive, clones, failAt;
   CountedNumber() { ++alive;SetDescription("1"); }
   CountedNumber(const CountedNumber &other):NumberWrapper(other) { ++alive; }
   ~CountedNumber() override { --alive; }
   ElementWrapper *Clone() const override {
      if(++clones==failAt)throw std::runtime_error("intentional numeric clone failure");
      return new CountedNumber(*this);
   }
};
int CountedNumber::alive=0;int CountedNumber::clones=0;int CountedNumber::failAt=-1;
void setNames(ProbeVary &command,const std::string &text) { for(const auto &field:fields)command.SetStringParameter(field,text); }
void numeric(ProbeVary &command) { setNames(command,"1");auto *number=new NumberWrapper;number->SetDescription("1");require(command.SetElementWrapper(number,"1"),"Numeric binding failed"); }
QJsonArray values(const ProbeVary &command) { QJsonArray result;for(auto *w:command.bindings())result.append(w?w->EvaluateReal():-999.0);return result; }
void unit(const QString &name,QJsonObject &proof) {
   if(name=="LiteralRoles"||name=="NumericOptions") {
      ProbeVary command;numeric(command);Variable control("Control");control.SetReal(7);
      if(name=="NumericOptions") {
         command.SetStringParameter("Variable","Control");auto *wrapper=new VariableWrapper;wrapper->SetDescription("Control");require(wrapper->SetRefObject(&control)&&command.SetElementWrapper(wrapper,"Control"),"Variable binding failed");
      }
      command.SetField(command.GetParameterID("Perturbation"),2);
      proof["values"]=values(command);
      const auto bindings=command.bindings();
      for(std::size_t i=0;i<bindings.size();++i)require(bindings[i]->EvaluateReal()==(i==0&&name=="NumericOptions"?7:i==2?2:1),"Updating one literal option changed another Vary role");
      require(control.EvaluateReal()==7,"Literal setting changed a caller Variable");
   } else if(name=="SharedVariable") {
      Variable shared("Shared");shared.SetReal(1);ProbeVary command;setNames(command,"Shared");auto *wrapper=new VariableWrapper;wrapper->SetDescription("Shared");
      require(wrapper->SetRefObject(&shared)&&command.SetElementWrapper(wrapper,"Shared"),"Shared native Variable binding failed");
      command.SetField(command.GetParameterID("Perturbation"),2);
      for(auto *w:command.bindings())require(w->EvaluateReal()==2,"Explicit shared Variable identity was replaced by a value copy");
      require(shared.EvaluateReal()==2,"Shared Variable update did not reach its native owner");
      proof["values"]=values(command);
   } else if(name=="RebindLifetime") {
      {
         ProbeVary command;setNames(command,"1");
         for(int i=0;i<5;++i)require(command.SetElementWrapper(new CountedNumber,"1"),"Repeated literal binding failed");
         command.ClearWrappers();require(CountedNumber::alive==0,"Literal rebinding leaked an owned wrapper");
      }
      require(CountedNumber::alive==0,"Literal destruction leaked an owned wrapper");
      proof["remaining_owned_wrappers"]=CountedNumber::alive;
   } else if(name=="CloneFailure") {
      ProbeVary command;numeric(command);const auto before=command.bindings();
      auto incoming=std::make_unique<CountedNumber>();CountedNumber::clones=0;CountedNumber::failAt=3;bool rejected=false;
      try { if(command.SetElementWrapper(incoming.get(),"1"))incoming.release(); }
      catch(const std::runtime_error &error) { rejected=std::string(error.what())=="intentional numeric clone failure"; }
      CountedNumber::failAt=-1;
      require(rejected&&command.bindings()==before,"Failed literal staging partially replaced Vary bindings");
      require(CountedNumber::alive==1,"Failed staging leaked a clone or consumed the caller's incoming wrapper");
      proof["old_bindings_preserved"]=true;
   } else if(name=="ColdCopies") {
      ProbeVary source;numeric(source);ProbeVary copied(source),assigned;numeric(assigned);assigned=source;
      for(const auto *command:{&copied,&assigned})for(auto *w:command->bindings())require(!w,"Cold Vary copy inherited a live literal binding");
      require(source.bindings()[0]->EvaluateReal()==1,"Cold copy changed its source control");
      proof["cold_copies"]=true;
   } else throw std::runtime_error("Unknown native literal unit case");
}
void nativeMission(const QString &name,const QString &directory,QJsonObject &proof) {
   auto *moderator=Moderator::Instance();const bool shared=name=="NativeSharedVariable",duplicate=name=="NativeDuplicates"||shared;
   const QString control=name=="NativeVariable"||shared?"Control":name=="NativeArray"?"Grid(1,2)":name=="NativeParameter"?"Sat.EarthMJ2000Eq.X":name=="NativeSigned"?"-1.25e+1":"1";
   QString script="Create Variable Control Quantity Keep;\nControl = 7;\nQuantity = 3;\nKeep = 41;\nCreate Array Grid[1,2];\nGrid(1,2) = 8;\nCreate Spacecraft Sat;\nCreate DifferentialCorrector DC;\nDC.ShowProgress = false;\nDC.ReportFile = '"+directory+"/solver.txt';\nBeginMissionSequence;\nTarget DC {SolveMode = RunInitialGuess, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\n";
   const QString options="{Perturbation = 1, Lower = -20, Upper = 20, MaxStep = 1, AdditiveScaleFactor = 1, MultiplicativeScaleFactor = 1}";
   script+="Vary 'First' DC("+control+" = 2, "+options+");\n";
   if(duplicate)script+="Vary 'Second' DC("+control+" = 3, "+options+");\n";
   script+="Achieve DC(Quantity = 3, {Tolerance = 0.000001});\nEndTarget;\nKeep = Keep + 1;\n";
   const auto path=directory+"/mission.script";save(path,script.toUtf8());require(moderator->InterpretScript(path.toStdString()),"Native literal-control script did not build");
   const auto status=moderator->RunMission();proof["mission_status"]=status;require(status==1,"Native literal-control mission did not complete");
   SolverBranchCommand *branch=nullptr;
   for(auto *c=moderator->GetFirstCommand();c;c=c->GetNext())if(c->IsOfType("Target")){branch=dynamic_cast<SolverBranchCommand*>(c);break;}
   require(branch,"Actual native Target absent");std::vector<Vary*> commands;
   for(auto *c=branch->GetChildCommand(0);c&&c!=branch;c=c->GetNext())if(auto *vary=dynamic_cast<Vary*>(c))commands.push_back(vary);
   require(commands.size()==std::size_t(duplicate?2:1),"Native Vary command identities differ");
   auto *solver=dynamic_cast<Solver*>(branch->GetClone(0));require(solver,"Actual native solver clone absent");QJsonArray observations;
   for(std::size_t i=0;i<commands.size();++i) {
      auto *command=commands[i];QJsonObject row{{"current",command->GetRealParameter(command->GetParameterID("CurrentValue"))},{"solver",solver->GetSolverVariable(int(i))}};
      for(const auto &field:fields)if(field!="Variable")row[QString::fromStdString(field)]=command->GetRealParameter(command->GetParameterID(field));
      observations.append(row);
   }
   proof["observations"]=observations;
   for(std::size_t i=0;i<commands.size();++i) {
      const auto row=observations[int(i)].toObject();const double initial=2+double(i);
      require(row["current"].toDouble()==initial&&row["solver"].toDouble()==initial+1,"Actual native control/scaled value differs from independent literal arithmetic");
      for(const auto &field:fields)if(field!="Variable")require(row[QString::fromStdString(field)].toDouble()==(field=="InitialValue"?initial:field=="Lower"?-20:field=="Upper"?20:1),"Executing a literal control changed a numeric setting");
   }
   if(duplicate) {
      commands[0]->SetField(commands[0]->GetParameterID("InitialValue"),4);
      require(commands[0]->TakeAction("SolverReset"),"Literal solver refresh failed");
      proof["refreshed_solver"]=QJsonArray{solver->GetSolverVariable(0),solver->GetSolverVariable(1)};
      const double peer=shared?5:4;
      require(solver->GetSolverVariable(0)==5&&solver->GetSolverVariable(1)==peer,"Refresh changed literal independence or named-resource alias behavior");
      std::array<Real,6> data{{6,1,-20,20,1,5}};
      require(!solver->RefreshSolverVariable(data.data(),-1,control.toStdString())&&!solver->RefreshSolverVariable(data.data(),2,control.toStdString())&&!solver->RefreshSolverVariable(data.data(),0,"Missing"),"Indexed refresh accepted an invalid control identity");
      data[5]=std::numeric_limits<double>::quiet_NaN();bool rejected=false;
      try{solver->RefreshSolverVariable(data.data(),0,control.toStdString());}catch(BaseException &){rejected=true;}
      require(rejected&&solver->GetSolverVariable(0)==5&&solver->GetSolverVariable(1)==peer,"Rejected indexed refresh partially changed the decision vector");
      proof["rejected_refresh_atomic"]=true;
   }
   require(moderator->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Literal mission continuation failed");
   require(!moderator->GetConfiguredObject("1")&&!moderator->GetConfiguredObject("-1.25e+1"),"Literal control created a global numeric-name resource");
}
}
int main(int argc,char **argv) {
   QCoreApplication app(argc,argv);const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");QJsonObject proof{{"passed",false}};
   try {
      require(argc==3&&!directory.isEmpty(),"Supply native literal startup, case and output");const QString name=argv[2];proof["case"]=name;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());require(Moderator::Instance()->Initialize(startup.toStdString(),false),"Native literal runtime failed");
      save(qEnvironmentVariable("GMAT_SOLVER_MAP_READY"),QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer timer;timer.start();while(!QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK"))&&timer.elapsed()<10000)QThread::msleep(5);require(QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")),"Native images not acknowledged");
      if(name.startsWith("Native"))nativeMission(name,directory,proof);else unit(name,proof);
      proof["passed"]=true;proof["scope"]="Per-role numeric value ownership, actual resource identity and native Target/Vary execution/refresh; no arbitrary scalar/solver/domain or complete original parent qualification.";
   } catch(BaseException &e) { proof["error"]=QString::fromStdString(e.GetFullMessage()); }
     catch(std::exception &e) { proof["error"]=QString::fromUtf8(e.what()); }
   save(QDir(directory).filePath("proof.json"),QJsonDocument(proof).toJson());std::cout<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return proof["passed"].toBool()?0:1;
}
