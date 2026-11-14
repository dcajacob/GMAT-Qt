#include "FunctionValidation.hpp"
#include "Moderator.hpp"
#include "MessageInterface.hpp"
#include "QtMessageReceiver.hpp"
#include "FileManager.hpp"
#include "Function.hpp"
#include "UserDefinedFunction.hpp"
#include "GmatCommand.hpp"
#include "Assignment.hpp"
#include "BaseException.hpp"
#include "SandboxException.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QSet>
#include <QTemporaryDir>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>
namespace {
QByteArray bytes(const QString &path){QFile file(path);if(!file.open(QIODevice::ReadOnly))throw std::runtime_error(("Cannot read function validation input: "+path).toStdString());return file.readAll();}
void put(const QString &path,const QByteArray &data){QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit())throw std::runtime_error("Cannot write private function validation inputs.");}
QString reason(const BaseException &e){return QString::fromStdString(e.GetFullMessage());}
}
void qtValidateFunctionBodies(const std::string &mission)
{
 auto *engine=Moderator::Instance();bool any=false;
 for(const auto &name:engine->GetListOfObjects(Gmat::FUNCTION)){auto *object=engine->GetConfiguredObject(name);if(object&&object->IsOfType("GmatFunction")){any=true;break;}}
 if(!any)return;
 auto *files=FileManager::Instance();QTemporaryDir temp;if(!temp.isValid())throw std::runtime_error("Cannot create private function validation directory.");
 const auto startup=temp.filePath("startup.txt"),source=temp.filePath("mission.script"),request=temp.filePath("request.json"),response=temp.filePath("response.json");
 const auto saved=files->CaptureState();try{files->WriteStartupFile(startup.toStdString());files->RestoreState(*saved);}catch(...){files->RestoreState(*saved);throw;}
 // Keep the session's aliases/search paths and isolate only the child's log.
 put(startup,bytes(startup)+"\nLOG_FILE = "+temp.filePath("validation.log").toUtf8()+"\n");put(source,QByteArray::fromStdString(mission));
 QString executable="GmatQt";
#ifdef Q_OS_WIN
 executable+=".exe";
#endif
 executable=QDir(QString::fromStdString(files->GetBinDirectory(executable.toStdString()))).filePath(executable);
 if(!QFileInfo(executable).isExecutable())throw std::runtime_error("The GMAT function validator executable is unavailable.");
 put(request,QJsonDocument(QJsonObject{{"startup",startup},{"mission",source},{"response",response},{"cwd",QDir::currentPath()}}).toJson());
 QProcess process;process.setProgram(executable);process.setArguments({"--validate-functions",request});process.setWorkingDirectory(QDir::currentPath());process.start();
 if(!process.waitForStarted(5000))throw std::runtime_error("Cannot start GMAT function validation.");
 // Build is synchronous. Do not dispatch queued Run/Pause/Stop timers while
 // a different process is validating the model.
 if(!process.waitForFinished(60000)){process.kill();process.waitForFinished(5000);throw std::runtime_error("GMAT function validation did not finish within 60 seconds.");}
 if(!QFileInfo::exists(response))throw std::runtime_error(("GMAT function validation stopped unexpectedly.\n"+QString::fromUtf8(process.readAllStandardError())).toStdString());
 const auto result=QJsonDocument::fromJson(bytes(response)).object();if(process.exitStatus()!=QProcess::NormalExit||process.exitCode()!=0||!result.value("valid").toBool())throw std::runtime_error(result.value("error").toString("GMAT function validation failed.").toStdString());
}
int qtFunctionValidationWorker(int argc,char **argv)
{
 QCoreApplication app(argc,argv);QJsonObject result{{"valid",false}};QString response;QJsonArray checked;QString diagnostics;QtMessageReceiver receiver;receiver.SetMessageCallback(&app,[&](const QString &text){diagnostics+=text;});struct ResetReceiver{~ResetReceiver(){MessageInterface::SetMessageReceiver(nullptr);}}resetReceiver;
 try{
  if(argc!=3)throw std::runtime_error("Invalid function validation request.");const auto request=QJsonDocument::fromJson(bytes(QString::fromLocal8Bit(argv[2]))).object();response=request.value("response").toString();if(response.isEmpty())throw std::runtime_error("Missing validation response destination.");
  if(!QDir::setCurrent(request.value("cwd").toString()))throw std::runtime_error("Cannot retain function validation working directory.");auto *files=FileManager::Instance();files->SetBinDirectory(QFileInfo(app.applicationFilePath()).fileName().toStdString(),app.applicationFilePath().toStdString());auto *engine=Moderator::Instance();receiver.SetLogFile(QFileInfo(response).dir().filePath("validation.log").toStdString());MessageInterface::SetMessageReceiver(&receiver);
  if(!engine->Initialize(request.value("startup").toString().toStdString(),false))throw std::runtime_error("Cannot initialize function validation engine.");
  std::istringstream script(bytes(request.value("mission").toString()).toStdString());if(!engine->InterpretScript(&script,true))throw std::runtime_error("Cannot parse function validation mission.");
  QCoreApplication::sendPostedEvents(&app,QEvent::MetaCall);diagnostics.clear();
  QSet<QString> visited;std::vector<std::unique_ptr<Function>> discovered;
  std::function<void(UserDefinedFunction *,ObjectMap)> validate;
  validate=[&](UserDefinedFunction *function,ObjectMap scope){
   const auto path=QString::fromStdString(function->GetStringParameter("FunctionPath"));const auto identity=QFileInfo(path).canonicalFilePath();if(!identity.isEmpty()&&visited.contains(identity))return;if(!identity.isEmpty())visited.insert(identity);
   try{
    const auto content=bytes(path);checked.append(QJsonObject{{"name",QString::fromStdString(function->GetName())},{"path",path},{"sha256",QString::fromLatin1(QCryptographicHash::hash(content,QCryptographicHash::Sha256).toHex())}});
    function->SetSolarSystem(engine->GetSolarSystemInUse());auto *sequence=engine->InterpretGmatFunction(function,&scope,engine->GetSolarSystemInUse());if(!sequence)throw std::runtime_error("Native parser rejected function body.");function->SetFunctionControlSequence(sequence);
    for(const auto &entry:*function->GetFunctionObjectMap())scope[entry.first]=entry.second;
    auto resolve=[&](const std::string &name){Function *callee=nullptr;const auto at=scope.find(name);if(at!=scope.end())callee=dynamic_cast<Function *>(at->second);if(!callee){
     // Native parsing puts declared functions in the sandbox global map, not
     // the local data map. Preserve their actual kind and explicit path.
     try{callee=dynamic_cast<Function *>(engine->GetInternalObject(name));}
     catch(const SandboxException &){} // An unresolved name retains the .gmf fallback.
    }if(!callee){discovered.emplace_back(engine->CreateFunction("GmatFunction",name,0));callee=discovered.back().get();if(!callee)throw std::runtime_error("Cannot resolve nested GMAT function: "+name);scope[name]=callee;}if(callee->IsOfType("GmatFunction"))validate(dynamic_cast<UserDefinedFunction *>(callee),scope);};
    QSet<GmatCommand *> seen;std::function<void(GmatCommand *)> walk=[&](GmatCommand *command){for(;command&&!seen.contains(command);command=command->GetNext()){seen.insert(command);if(command->IsOfType("CallGmatFunction")&&!command->IsOfType("CallBuiltinGmatFunction"))resolve(command->GetStringParameter("FunctionName"));if(auto *assignment=dynamic_cast<Assignment *>(command))for(const auto &name:assignment->GetGmatFunctionNames())resolve(name);for(int i=0;auto *child=command->GetChildCommand(i);++i)walk(child);}};walk(sequence);
    // Local declarations are subject to the same rule even when not called.
    std::vector<UserDefinedFunction *> locals;for(const auto &entry:*function->GetFunctionObjectMap())if(entry.second->IsOfType("GmatFunction"))locals.push_back(dynamic_cast<UserDefinedFunction *>(entry.second));for(auto *local:locals)validate(local,scope);
   }catch(const BaseException &e){throw std::runtime_error(("Function validation failed in "+path+":\n"+reason(e)).toStdString());}catch(const std::exception &e){throw std::runtime_error(("Function validation failed in "+path+":\n"+QString::fromUtf8(e.what())).toStdString());}
  };
  const auto names=engine->GetListOfObjects(Gmat::FUNCTION);const auto scope=*engine->GetConfiguredObjectMap();for(const auto &name:names){auto *object=engine->GetConfiguredObject(name);if(object&&object->IsOfType("GmatFunction"))validate(dynamic_cast<UserDefinedFunction *>(object),scope);}
  result["valid"]=true;
 }catch(const BaseException &e){result["error"]=reason(e);}catch(const std::exception &e){result["error"]=QString::fromUtf8(e.what());}catch(...){result["error"]="Unexpected error in native function validation.";}
 QCoreApplication::sendPostedEvents(&app,QEvent::MetaCall);
 if(!result.value("valid").toBool()&&!diagnostics.isEmpty())result["error"]=result.value("error").toString()+"\n"+diagnostics;
 result["functions"]=checked;if(!response.isEmpty()){try{put(response,QJsonDocument(result).toJson());}catch(...){return 2;}}return result.value("valid").toBool()?0:1;
}
