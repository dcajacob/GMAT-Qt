#include "NonlinearConstraint.hpp"
#include "NumberWrapper.hpp"
#include "VariableWrapper.hpp"
#include "Variable.hpp"
#include "Moderator.hpp"
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
void require(bool ok,const char *message) {if(!ok)throw std::runtime_error(message);}
void write(const QString &path,const QByteArray &bytes) {QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Constraint operand evidence write failed");}
const std::array<std::string,3> fields{{"ConstraintArg1","ConstraintArg2","Tolerance"}};
class ProbeConstraint : public NonlinearConstraint {
public:
   std::array<ElementWrapper*,3> bindings() const {return {{arg1,arg2,tolerance}};}
};
struct CountedNumber : NumberWrapper {
   static int alive,clones,failAt;
   CountedNumber() {++alive;SetDescription("1");}
   CountedNumber(const CountedNumber &other):NumberWrapper(other) {++alive;}
   ~CountedNumber() override {--alive;}
   ElementWrapper *Clone() const override {if(++clones==failAt)throw std::runtime_error("intentional constraint clone failure");return new CountedNumber(*this);}
};
int CountedNumber::alive=0;int CountedNumber::clones=0;int CountedNumber::failAt=-1;
void names(ProbeConstraint &command,const std::string &value) {for(const auto &field:fields)command.SetStringParameter(command.GetParameterID(field),value);}
void numeric(ProbeConstraint &command) {names(command,"1");auto *number=new NumberWrapper;number->SetDescription("1");require(command.SetElementWrapper(number,"1"),"Constraint numeric binding failed");}
QJsonArray values(const ProbeConstraint &command) {QJsonArray result;for(auto *w:command.bindings())result.append(w?w->EvaluateReal():-999);return result;}
void unit(const QString &name,QJsonObject &proof) {
   if(name=="RhsNumber"||name=="ToleranceNumber"||name=="RhsField"||name=="ToleranceField") {
      ProbeConstraint command;numeric(command);const bool tolerance=name.startsWith("Tolerance");const auto field=tolerance?"Tolerance":"RHSValue";const double value=tolerance?.25:2;
      try {if(name.endsWith("Field"))command.SetField(command.GetParameterID(field),value);else command.SetNumber(field,value);}
      catch(BaseException &){proof["values_after_rejected_valid_update"]=values(command);throw;}
      proof["values"]=values(command);const auto b=command.bindings();require(b[0]->EvaluateReal()==1&&b[1]->EvaluateReal()==(tolerance?1:2)&&b[2]->EvaluateReal()==(tolerance?.25:1),"Updating one constraint numeric operand changed another role");
   } else if(name=="SharedVariable") {
      Variable shared("Shared");shared.SetReal(1);ProbeConstraint command;names(command,"Shared");auto *wrapper=new VariableWrapper;wrapper->SetDescription("Shared");require(wrapper->SetRefObject(&shared)&&command.SetElementWrapper(wrapper,"Shared"),"Constraint shared native binding failed");command.SetNumber("RHSValue",2);for(auto *w:command.bindings())require(w->EvaluateReal()==2,"Constraint replaced explicit shared Variable identity with value copies");require(shared.EvaluateReal()==2,"Constraint shared update missed its native owner");proof["values"]=values(command);
   } else if(name=="RebindLifetime") {
      {ProbeConstraint command;names(command,"1");for(int i=0;i<5;++i)require(command.SetElementWrapper(new CountedNumber,"1"),"Constraint repeated binding failed");command.ClearWrappers();proof["remaining_owned_wrappers"]=CountedNumber::alive;require(CountedNumber::alive==0,"Constraint rebinding leaked owned wrappers");}require(CountedNumber::alive==0,"Constraint destruction leaked owned wrappers");
   } else if(name=="CloneFailure") {
      ProbeConstraint command;numeric(command);const auto before=command.bindings();auto incoming=std::make_unique<CountedNumber>();CountedNumber::clones=0;CountedNumber::failAt=2;bool rejected=false;
      try{if(command.SetElementWrapper(incoming.get(),"1"))incoming.release();}catch(const std::runtime_error &e){rejected=std::string(e.what())=="intentional constraint clone failure";}CountedNumber::failAt=-1;
      require(rejected&&command.bindings()==before,"Failed constraint clone staging partially replaced bindings");require(CountedNumber::alive==1,"Failed constraint staging leaked a clone or consumed incoming ownership");proof["old_bindings_preserved"]=true;
   } else if(name=="ColdCopies") {
      ProbeConstraint source;numeric(source);ProbeConstraint copied(source),assigned;numeric(assigned);assigned=source;for(const auto *command:{&copied,&assigned})for(auto *w:command->bindings())require(!w,"Cold constraint copy inherited a live binding");require(source.bindings()[0]->EvaluateReal()==1,"Constraint copy changed its source");proof["cold_copies"]=true;
   } else throw std::runtime_error("Unknown constraint operand case");
}
}
int main(int argc,char **argv) {
   QCoreApplication app(argc,argv);const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");QJsonObject proof{{"passed",false}};
   try {
      require(argc==3&&!directory.isEmpty(),"Supply constraint operand startup, case and output");const QString name=argv[2];proof["case"]=name;const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());require(Moderator::Instance()->Initialize(startup.toStdString(),false),"Constraint operand runtime unavailable");write(qEnvironmentVariable("GMAT_SOLVER_MAP_READY"),QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer timer;timer.start();while(!QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK"))&&timer.elapsed()<10000)QThread::msleep(5);require(QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")),"Constraint native images not acknowledged");unit(name,proof);proof["passed"]=true;proof["scope"]="Native comparison operand numeric role independence, public setters and binding lifetime/rollback with explicit named-resource sharing. Full solver/domain/science and original parent conjunctions remain open.";
   }catch(BaseException &e){proof["error"]=QString::fromStdString(e.GetFullMessage());}catch(std::exception &e){proof["error"]=QString::fromUtf8(e.what());}
   write(QDir(directory).filePath("proof.json"),QJsonDocument(proof).toJson());std::cout<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return proof["passed"].toBool()?0:1;
}
