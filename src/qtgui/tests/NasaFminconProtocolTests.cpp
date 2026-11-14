#include "FormationAcceptance.hpp"
#include "MatlabAcceptanceStartup.hpp"
#include <QEventLoop>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSet>
#include <QTcpSocket>
#include <QTimer>
using namespace formaccept;

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("NasaFminconProtocol");
   try {
      require(argc==3,"Supply startup and actual MATLAB plugin");QTemporaryDir runtime;
      require(runtime.isValid(),"Callback protocol private startup failed");
      const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),runtime.path(),QFileInfo(argv[2]).absoluteFilePath());
      Rig rig(startup);rig.save("startup.txt",read(startup));
      const QString source="% Callback protocol caller/control witness\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\n";
      rig.source->setPlainText(source);require(rig.window.buildScript(),"Callback protocol control build failed");
      const int history=rig.source->document()->availableUndoSteps();require(Moderator::Instance()->StartMatlabServer(),"Actual Qt callback service startup failed");
      const auto endpoint=QJsonDocument::fromJson(qgetenv("GMAT_QT_MATLAB_CALLBACK")).object();
      require(endpoint["port"].toInt()>0&&endpoint["token"].isString(),"Actual callback endpoint absent");
      QTcpSocket client;client.connectToHost("127.0.0.1",endpoint["port"].toInt());require(client.waitForConnected(3000),"Actual loopback callback connection failed");
      auto request=[&](int id){return QJsonObject{{"id",id},{"token",endpoint["token"]},{"op","evaluate"},{"x",QJsonArray{0,0}}};};
      auto encode=[](QJsonObject object){return QJsonDocument(object).toJson(QJsonDocument::Compact)+'\n';};
      struct Packet {QString name;QByteArray bytes;QString error;};QList<Packet> packets;
      packets.append({"malformed-json","{bad\n","Invalid callback JSON"});
      packets.append({"wrong-root","[]\n","Invalid callback JSON"});
      auto value=request(1);value.remove("id");packets.append({"missing-identity",encode(value),"Missing callback request identity"});
      packets.append({"invalid-identity",encode(request(0)),"Invalid or replayed callback identity"});
      value=request(1);value["token"]="incorrect-session";packets.append({"wrong-session",encode(value),"Invalid callback session"});
      value=request(2);value["op"]="RunScript";packets.append({"script-operation",encode(value),"Unsupported callback operation"});
      value=request(3);value["script"]="Keep = 0;";packets.append({"unexpected-fields",encode(value),"Invalid callback session"});
      packets.append({"replayed-identity",encode(request(3)),"Invalid or replayed callback identity"});
      packets.append({"idle-evaluation",encode(request(4)),"No active uninterrupted GMAT optimizer mission"});
      packets.append({"oversized-frame",QByteArray(4*1024*1024+1,'x'),"Callback request exceeds frame capacity"});
      QJsonArray results;
      for(const auto &packet:packets) {
         QEventLoop loop;QTimer deadline;deadline.setSingleShot(true);deadline.setInterval(5000);
         auto read=QObject::connect(&client,&QTcpSocket::readyRead,&loop,[&]{if(client.canReadLine())loop.quit();});
         QObject::connect(&deadline,&QTimer::timeout,&loop,&QEventLoop::quit);deadline.start();
         require(client.write(packet.bytes)==packet.bytes.size(),"Callback negative packet write failed");
         if(!client.canReadLine())loop.exec();deadline.stop();QObject::disconnect(read);
         require(client.canReadLine(),"Actual callback rejection response timed out");const auto raw=client.readLine();
         QJsonParseError error;const auto document=QJsonDocument::fromJson(raw,&error);const auto response=document.object();
         require(error.error==QJsonParseError::NoError&&document.isObject()&&response["ok"].isBool()&&!response["ok"].toBool()&&response["error"].toString().contains(packet.error),"Actual callback rejection differs from its exact protocol rule");
         results.append(QJsonObject{{"case",packet.name},{"request_bytes",packet.bytes.size()},{"response",response}});
         rig.save("rejections.json",QJsonDocument(results).toJson());
      }
      require(results.size()==10,"Callback protocol rejection census differs");
      require(rig.source->toPlainText()==source&&rig.source->document()->availableUndoSteps()==history&&Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value")==41,"Rejected callback packet changed caller/history/native control");
      acceptanceView(rig.window);const QJsonObject proof{{"checkpoint","actual-Qt-fmincon-protocol"},{"rejections",results},{"caller_unchanged",true},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &error){std::cerr<<error.GetFullMessage()<<'\n';return 1;}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
