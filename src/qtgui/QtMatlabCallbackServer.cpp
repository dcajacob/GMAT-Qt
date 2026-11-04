#include "QtMatlabCallbackServer.hpp"
#include "GmatInterface.hpp"
#include "GmatGlobal.hpp"
#include "Moderator.hpp"
#include "InterpreterException.hpp"
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

QtMatlabCallbackServer::QtMatlabCallbackServer()
{
   server.setMaxPendingConnections(4);
   connect(&server,&QTcpServer::newConnection,this,[this] {
      while (auto *socket=server.nextPendingConnection())
      {
         socket->setReadBufferSize(maximumFrame+1);
         auto lastId=std::make_shared<double>(0);
         connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
         const QPointer<QTcpSocket> connection(socket);
         connect(socket,&QTcpSocket::readyRead,this,[this,connection,lastId] {
            if (!connection) return;
            auto *socket=connection.data();
            auto send=[connection](QJsonObject response) {
               if (!connection) return;
               auto *socket=connection.data();
               const auto bytes=QJsonDocument(response).toJson(QJsonDocument::Compact)+'\n';
               if (bytes.size()>maximumFrame) {
                  socket->write("{\"ok\":false,\"error\":\"Callback response exceeds frame capacity\"}\n");
                  socket->disconnectFromHost();
               } else socket->write(bytes);
            };
            if (socket->bytesAvailable()>maximumFrame && !socket->canReadLine()) {
               send({{"ok",false},{"error","Callback request exceeds frame capacity"}});
               socket->disconnectFromHost();
               return;
            }
            while (connection && connection->canReadLine())
            {
               const auto bytes=connection->readLine(maximumFrame+2);
               double id=0;
               try {
                  require(bytes.size()<=maximumFrame && bytes.endsWith('\n'),"Callback request exceeds frame capacity");
                  QJsonParseError error;
                  const auto document=QJsonDocument::fromJson(bytes,&error);
                  require(error.error==QJsonParseError::NoError && document.isObject(),"Invalid callback JSON object");
                  auto request=document.object();
                  require(request.value("id").isDouble(),"Missing callback request identity");
                  id=request.value("id").toDouble();
                  require(std::isfinite(id) && id>0 && id<=9007199254740991.0 && id==std::floor(id) && id>*lastId,"Invalid or replayed callback identity");
                  *lastId=id;
                  auto response=evaluate(request);
                  response["id"]=id;response["ok"]=true;
                  send(response);
               } catch (BaseException &error) {
                  send({{"ok",false},{"id",id},{"error",QString::fromStdString(error.GetFullMessage())}});
               } catch (const std::exception &error) {
                  send({{"ok",false},{"id",id},{"error",QString::fromUtf8(error.what())}});
               }
            }
         });
      }
   });
}

QtMatlabCallbackServer::~QtMatlabCallbackServer()
{
   server.close();
   if (qgetenv("GMAT_QT_MATLAB_CALLBACK")==published) qunsetenv("GMAT_QT_MATLAB_CALLBACK");
}

void QtMatlabCallbackServer::start()
{
   if (!server.isListening() && !server.listen(QHostAddress::LocalHost,0))
      throw InterpreterException("Cannot start the MATLAB optimizer callback service: "+server.errorString().toStdString());
   for (auto *socket:server.findChildren<QTcpSocket*>()) socket->abort();
   token=QUuid::createUuid().toString(QUuid::WithoutBraces);
   published=QJsonDocument(QJsonObject{{"port",server.serverPort()},{"token",token}}).toJson(QJsonDocument::Compact);
   if (!qputenv("GMAT_QT_MATLAB_CALLBACK",published))
      throw InterpreterException("Cannot publish the MATLAB optimizer callback endpoint");
}

QJsonObject QtMatlabCallbackServer::evaluate(const QJsonObject &request)
{
   const bool iteration=request.value("op").toString()=="iteration";
   require(request.size()==(iteration?8:4) && request.value("token").isString() && request.value("token").toString()==token,
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
   require(!processing,"A GMAT callback is already executing");
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
   struct Guard {bool &value; explicit Guard(bool &v):value(v){value=true;} ~Guard(){value=false;}} guard(processing);
   auto input=data.str();
   auto *interface=GmatInterface::Instance();
   if (iteration) require(interface->PublishCallbackIteration(notification.str(),true),"No registered optimizer iteration callback");
   interface->PutCallbackData(input);
   require(interface->ExecuteCallback(),"No registered optimizer callback");
   auto result=numericResults(interface->GetCallbackResultString(),values.size());
   if (iteration) require(interface->PublishCallbackIteration(notification.str()),"No registered optimizer iteration callback");
   return result;
}
