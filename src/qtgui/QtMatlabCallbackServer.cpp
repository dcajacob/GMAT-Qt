#include "QtMatlabCallbackServer.hpp"
#include "GmatInterface.hpp"
#include "GmatGlobal.hpp"
#include "Moderator.hpp"
#include "InterpreterException.hpp"
#include "InterfaceException.hpp"
#include <QVariant>
#include "MatlabCallbackValueCodec.hpp"
#include <QEventLoop>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTimer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <QPointer>
#include <cmath>
#include <iomanip>
#include <locale>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace {
constexpr qint64 maximumFrame = 4*1024*1024;
void require(bool condition, const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
QJsonObject numericResults(const std::string &text, int variables)
{
   const auto input = QString::fromStdString(text);
   const QSet<QString> fields{"F","GradF","NonLinearEqCon","NonLinearIneqCon","JacNonLinearEqCon","JacNonLinearIneqCon"};
   const QRegularExpression assignment(R"(([A-Za-z][A-Za-z0-9_]*)\s*=\s*(\[[^\]]*\]|[^;]+)\s*;)");
   auto matches = assignment.globalMatch(input);
   QJsonObject result;
   qsizetype previous = 0;
   while (matches.hasNext())
   {
      const auto match = matches.next();
      require(input.mid(previous,match.capturedStart()-previous).remove(';').trimmed().isEmpty(),"Invalid callback result syntax");
      previous = match.capturedEnd();
      const auto key = match.captured(1), value = match.captured(2).trimmed();
      require(fields.contains(key) && !result.contains(key),"Unknown or repeated callback result");
      if (key == "F")
      {
         bool ok=false;
         const auto number = value.toDouble(&ok);
         require(ok && std::isfinite(number),"Nonfinite callback objective");
         result[key]=number;
      }
      else
      {
         require(value.startsWith('[') && value.endsWith(']'),"Callback result is not a vector");
         QJsonArray numbers;
         for (const auto &part:value.mid(1,value.size()-2).split(QRegularExpression("[;,\\s]+"),Qt::SkipEmptyParts))
         {
            bool ok=false;
            const auto number=part.toDouble(&ok);
            require(ok && std::isfinite(number),"Nonfinite callback vector");
            numbers.append(number);
         }
         if (key == "GradF") require(numbers.isEmpty() || numbers.size()==variables,"Callback gradient shape differs");
         if (key.startsWith("Jac")) require(numbers.isEmpty(),"Unexpected callback Jacobian shape");
         result[key]=numbers;
      }
   }
   require(result.size()==fields.size() && input.mid(previous).remove(';').trimmed().isEmpty(),"Incomplete callback results");
   return result;
}
}

struct QtMatlabCallbackServer::PendingMatlabJob
{
   QPointer<QTcpSocket> socket;
   double requestId = 0;
   bool done = false,ok = false;
   QString error;
   QEventLoop *loop = nullptr;
};

QtMatlabCallbackServer::QtMatlabCallbackServer()
{
   server.setMaxPendingConnections(4);
   connect(&server,&QTcpServer::newConnection,this,[this] {
      while(auto *socket=server.nextPendingConnection()) {
         socket->setReadBufferSize(maximumFrame+1);
         socket->setProperty("gmatCallbackLastId",0.0);
         connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
         const QPointer<QTcpSocket> connection(socket);
         connect(socket,&QTcpSocket::readyRead,this,[this,connection] {
            if(connection) processSocket(connection);
         });
      }
   });
}

void QtMatlabCallbackServer::processSocket(QTcpSocket *socket)
{
   const QPointer<QTcpSocket> connection(socket);
   const auto send=[connection](QJsonObject response) {
      if(!connection) return;
      const auto bytes=QJsonDocument(response).toJson(QJsonDocument::Compact)+'\n';
      if(bytes.size()>maximumFrame) {
         connection->write("{\"ok\":false,\"error\":\"Callback response exceeds frame capacity\"}\n");
         connection->disconnectFromHost();
      } else connection->write(bytes);
   };
   if(socket->bytesAvailable()>maximumFrame && !socket->canReadLine()) {
      send({{"ok",false},{"error","Callback request exceeds frame capacity"}});
      socket->disconnectFromHost();return;
   }
   while(connection && connection->canReadLine()) {
      const auto bytes=connection->readLine(maximumFrame+2);
      double id=0;
      try {
         require(bytes.size()<=maximumFrame && bytes.endsWith('\n'),"Callback request exceeds frame capacity");
         QJsonParseError error;const auto document=QJsonDocument::fromJson(bytes,&error);
         require(error.error==QJsonParseError::NoError && document.isObject(),"Invalid callback JSON object");
         const auto request=document.object();
         require(request.value("id").isDouble(),"Missing callback request identity");
         id=request.value("id").toDouble();
         if(request.value("op").toString()=="matlab_result") {
            require(request.size()==6 && request.value("token").toString()==token &&
               request.value("job").isString() && request.value("ok").isBool() && request.value("error").isString(),
               "Invalid nested MATLAB response");
            auto *job=matlabJobs.value(request.value("job").toString(),nullptr);
            require(job && job->socket==connection && job->requestId==id && !job->done,"Unexpected nested MATLAB response identity");
            job->done=true;job->ok=request.value("ok").toBool();job->error=request.value("error").toString();
            if(job->loop) job->loop->quit();
            continue;
         }
         const double lastId=connection->property("gmatCallbackLastId").toDouble();
         require(std::isfinite(id) && id>0 && id<=9007199254740991.0 && id==std::floor(id) && id>lastId,
            "Invalid or replayed callback identity");
         connection->setProperty("gmatCallbackLastId",id);
         auto response=evaluate(request,connection);response["id"]=id;response["ok"]=true;send(response);
      } catch(BaseException &error) {
         send({{"ok",false},{"id",id},{"error",QString::fromStdString(error.GetFullMessage())}});
      } catch(const std::exception &error) {
         send({{"ok",false},{"id",id},{"error",QString::fromUtf8(error.what())}});
      }
   }
}

QtMatlabCallbackServer::~QtMatlabCallbackServer()
{
   server.close();
   if (qgetenv("GMAT_QT_MATLAB_CALLBACK")==published) qunsetenv("GMAT_QT_MATLAB_CALLBACK");
}

void QtMatlabCallbackServer::start()
{
   // Initializing a nested optimizer reuses the active authenticated session.
   if(processing) { require(server.isListening(),"The active MATLAB callback service has ended");return; }
   if (!server.isListening() && !server.listen(QHostAddress::LocalHost,0))
      throw InterpreterException("Cannot start the MATLAB optimizer callback service: "+server.errorString().toStdString());
   for (auto *socket:server.findChildren<QTcpSocket*>()) socket->abort();
   token=QUuid::createUuid().toString(QUuid::WithoutBraces);
   published=QJsonDocument(QJsonObject{{"port",server.serverPort()},{"token",token}}).toJson(QJsonDocument::Compact);
   if (!qputenv("GMAT_QT_MATLAB_CALLBACK",published))
      throw InterpreterException("Cannot publish the MATLAB optimizer callback endpoint");
}

QJsonObject QtMatlabCallbackServer::evaluate(const QJsonObject &request,QTcpSocket *socket)
{
   const bool iteration=request.value("op").toString()=="iteration";
   const bool nested=request.contains("protocol");
   require(!nested || (request.value("protocol").isDouble() && request.value("protocol").toDouble()==2),"Unsupported callback protocol version");
   require(request.size()==(iteration?8:4)+(nested?1:0) && request.value("token").isString() && request.value("token").toString()==token,
      "Invalid callback session or request fields");
   require((iteration || request.value("op").toString()=="evaluate") && request.value("x").isArray(),"Unsupported callback operation");
   std::ostringstream notification;
   notification.imbue(std::locale::classic());notification<<std::setprecision(17);
   if (iteration) {
      const auto phase=request.value("state").toString();
      require(phase=="init" || phase=="iter" || phase=="done","Invalid optimizer iteration state");
      notification<<phase.toStdString()<<' ';
      for (const auto *key:{"iteration","funccount"}) {
         const auto value=request.value(key);
         require(value.isDouble() && std::isfinite(value.toDouble()) && value.toDouble()>=0 &&
            value.toDouble()<=2147483647 && value.toDouble()==std::floor(value.toDouble()),"Invalid optimizer iteration counter");
         notification<<static_cast<int>(value.toDouble())<<' ';
      }
      require(request.value("fval").isDouble() && std::isfinite(request.value("fval").toDouble()),"Invalid optimizer iteration objective");
      notification<<request.value("fval").toDouble();
   }
   require(!processing || matlabWaits>0,"A GMAT callback is already executing");
   const auto state=Moderator::Instance()->GetRunState();
   require((state==Gmat::RUNNING || state==Gmat::PAUSED) && !GmatGlobal::Instance()->GetRunInterrupted(),"No active uninterrupted GMAT optimizer mission");
   const auto values=request.value("x").toArray();
   require(!values.isEmpty(),"Missing callback variables");
   std::ostringstream data;
   data.imbue(std::locale::classic());data<<std::setprecision(17);
   for (const auto &value:values) {
      require(value.isDouble() && std::isfinite(value.toDouble()),"Callback variables must be finite numbers");
      data<<value.toDouble()<<' ';
   }
   struct Guard {int &value; explicit Guard(int &v):value(v){++value;} ~Guard(){--value;}} guard(processing);
   auto input=data.str();
   auto *interface=GmatInterface::Instance();
   GmatInterface::CallbackScope callbackScope;
   callbackScope.ValidateSelection();
   std::unique_ptr<MatlabCallbackBridge::Scope> bridge;
   if(nested) {
      const QPointer<QTcpSocket> connection(socket);
      const double id=request.value("id").toDouble();
      bridge=std::make_unique<MatlabCallbackBridge::Scope>([this,connection,id,&callbackScope](const MatlabCallbackRequest &job) {
         require(bool(connection),"The waiting MATLAB callback client has disconnected");
         return runMatlabJob(job,connection,id,[&callbackScope]{callbackScope.ValidateRetention();});
      });
   } else bridge=std::make_unique<MatlabCallbackBridge::Scope>();
   if (iteration) require(interface->PublishCallbackIteration(notification.str(),true),"No registered optimizer iteration callback");
   callbackScope.ValidateSelection();
   interface->PutCallbackData(input);
   callbackScope.ValidateSelection();
   require(interface->ExecuteCallback(),"No registered optimizer callback");
   callbackScope.ValidateSelection();
   auto result=numericResults(interface->GetCallbackResultString(),values.size());
   callbackScope.ValidateSelection();
   if (iteration) require(interface->PublishCallbackIteration(notification.str()),"No registered optimizer iteration callback");
   callbackScope.ValidateSelection();
   return result;
}

MatlabCallbackReply QtMatlabCallbackServer::runMatlabJob(const MatlabCallbackRequest &request,QTcpSocket *socket,
      double requestId,const std::function<void()> &validate)
{
   validate();
   QTemporaryDir directory;
   require(directory.isValid(),"Cannot create nested MATLAB exchange directory");
   const auto inputPath=directory.filePath("request.json"),outputPath=directory.filePath("result.json");
   const QJsonObject payload{{"session",token},{"operation",static_cast<int>(request.operation)},
      {"name",QString::fromStdString(request.name)},{"expression",QString::fromStdString(request.expression)},
      {"value",MatlabCallbackValueCodec::encode(request.value)}};
   QSaveFile input(inputPath);require(input.open(QIODevice::WriteOnly),"Cannot write nested MATLAB request");
   const auto bytes=QJsonDocument(payload).toJson(QJsonDocument::Compact);
   require(input.write(bytes)==bytes.size() && input.commit(),"Cannot finish nested MATLAB request");
   const auto identity=QUuid::createUuid().toString(QUuid::WithoutBraces);
   QEventLoop loop;PendingMatlabJob pending;pending.socket=socket;pending.requestId=requestId;pending.loop=&loop;
   matlabJobs.insert(identity,&pending);
   struct Guard {
      QHash<QString,PendingMatlabJob*> &jobs;QString identity;int &waits;
      Guard(QHash<QString,PendingMatlabJob*> &j,QString id,int &w):jobs(j),identity(std::move(id)),waits(w){++waits;}
      ~Guard(){jobs.remove(identity);--waits;}
   } guard(matlabJobs,identity,matlabWaits);
   const auto frame=QJsonDocument(QJsonObject{{"kind","matlab"},{"id",requestId},{"job",identity},
      {"token",token},{"request",inputPath},{"result",outputPath}}).toJson(QJsonDocument::Compact)+'\n';
   require(socket->write(frame)==frame.size(),"Cannot send nested MATLAB request");socket->flush();
   QTimer timer;
   connect(&timer,&QTimer::timeout,&loop,[&] {
      try {
         validate();
         if(!pending.socket || pending.socket->state()!=QAbstractSocket::ConnectedState ||
             GmatGlobal::Instance()->GetRunInterrupted())
            throw std::runtime_error("The nested MATLAB callback was interrupted or disconnected");
         // Qt does not recursively emit readyRead for the parent socket while
         // its original handler is evaluating a callback. Drain replies here.
         if(pending.socket->canReadLine()) processSocket(pending.socket);
      } catch(BaseException &error) {
         pending.done=true;pending.ok=false;pending.error=QString::fromStdString(error.GetFullMessage());
      } catch(const std::exception &error) {
         pending.done=true;pending.ok=false;pending.error=QString::fromUtf8(error.what());
      }
      if(pending.done) loop.quit();
   });
   timer.start(5);if(!pending.done) loop.exec();timer.stop();
   validate();
   if(!pending.ok) throw InterfaceException("Nested MATLAB callback: "+pending.error.toStdString());
   QFile output(outputPath);require(output.open(QIODevice::ReadOnly),"Missing nested MATLAB result file");
   QJsonParseError error;const auto document=QJsonDocument::fromJson(output.readAll(),&error);
   require(error.error==QJsonParseError::NoError && document.isObject(),"Invalid nested MATLAB result file");
   const auto result=document.object();require(result.size()==4 && result.value("found").isBool() &&
      result.value("reserved").isBool() && result.value("value").isObject(),"Incomplete nested MATLAB result");
   MatlabCallbackReply reply;reply.found=result.value("found").toBool();reply.reserved=result.value("reserved").toBool();
   reply.value=MatlabCallbackValueCodec::decode(result.value("value").toObject());
   const auto text=MatlabCallbackValueCodec::bytes(result.value("output"));
   reply.output.assign(text.constData(),static_cast<size_t>(text.size()));return reply;
}
