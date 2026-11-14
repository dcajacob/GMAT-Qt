#include "SatelliteImportDialog.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDateEdit>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("SatelliteCatalog");
   TestSettings settings; QTemporaryDir cache; qputenv("XDG_CACHE_HOME",cache.path().toUtf8());
   try {
      QTcpServer server; require(server.listen(QHostAddress::LocalHost),"Cannot start catalog fixture");
      const QString base=QString("http://127.0.0.1:%1").arg(server.serverPort());
      QSettings().setValue("satellites/celestrakUrl",base+"/gp.php"); QSettings().setValue("satellites/spaceTrackUrl",base);
      const QJsonObject record{{"OBJECT_NAME","Catalog Fixture"},{"NORAD_CAT_ID",100123},{"EPOCH","2000-06-27T18:50:19.733568"},
         {"MEAN_MOTION",10.82419157},{"ECCENTRICITY",.1859667},{"INCLINATION",34.2682},
         {"RA_OF_ASC_NODE",348.7242},{"ARG_OF_PERICENTER",331.7664},{"MEAN_ANOMALY",19.3264},
         {"BSTAR",.000028098},{"MEAN_MOTION_DOT",.00000023},{"MEAN_MOTION_DDOT",0}};
      const auto records=QJsonDocument(QJsonArray{record}).toJson(QJsonDocument::Compact);
      int requests=0,logins=0,historyRequests=0,statusCode=200,delay=0;
      bool credentialEncoding=false,cookieReceived=false;
      QByteArray loginResponse="{\"Login\":\"Success\"}";
      QObject::connect(&server,&QTcpServer::newConnection,&server,[&] {
         auto *socket=server.nextPendingConnection();
         QObject::connect(socket,&QTcpSocket::readyRead,socket,[&,socket] {
            QByteArray bytes=socket->property("requestBytes").toByteArray()+socket->readAll(); socket->setProperty("requestBytes",bytes);
            const int headerEnd=bytes.indexOf("\r\n\r\n"); if (headerEnd<0 || socket->property("served").toBool()) return;
            int length=0;
            for (const auto &line:bytes.left(headerEnd).split('\n')) if (line.toLower().startsWith("content-length:")) length=line.mid(15).trimmed().toInt();
            if (bytes.size()<headerEnd+4+length) return;
            socket->setProperty("served",true); ++requests;
            const bool login=bytes.startsWith("POST /ajaxauth/login");
            if (login) { ++logins; credentialEncoding=bytes.contains("password=x%2B%26%3D%20y"); }
            if (bytes.startsWith("GET /basicspacedata/")) { ++historyRequests; cookieReceived=bytes.contains("fixture=session"); }
            const auto body=login ? loginResponse : records;
            const QByteArray response="HTTP/1.1 "+QByteArray::number(statusCode)+" Result\r\nContent-Type: application/json\r\n"+
               (login ? QByteArray("Set-Cookie: fixture=session; Path=/\r\n") : QByteArray())+
               "Content-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body;
            QTimer::singleShot(delay,socket,[guard=QPointer<QTcpSocket>(socket),response] {
               if (guard) { guard->write(response); guard->disconnectFromHost(); }
            });
         });
         QObject::connect(socket,&QTcpSocket::disconnected,socket,&QTcpSocket::deleteLater);
      });
      bool imported=false;
      SatelliteImportDialog dialog({"EarthMJ2000Eq"},{"Earth"},[&](const SatelliteImportData &data,const QString &) {
         imported=true; require(data.catalogId=="100123","Catalog import truncated the NORAD identifier"); return QString();
      });
      dialog.show(); require(QTest::qWaitForWindowExposed(&dialog),"Catalog dialog did not open");
      auto *query=dialog.findChild<QLineEdit *>("satelliteCatalogQuery");
      auto *search=dialog.findChild<QPushButton *>("searchSatelliteCatalog");
      auto *results=dialog.findChild<QListWidget *>("satelliteCatalogResults");
      auto *source=dialog.findChild<QComboBox *>("satelliteCatalogSource");
      query->setText("100123"); app.processEvents(); require(requests==0,"Typing downloaded catalog data");
      search->click(); require(QTest::qWaitFor([&] { return results->count()==1; },3000),"Latest catalog result was not displayed");
      require(dialog.selectedData().catalogId=="100123","Selected latest record differs");
      search->click(); app.processEvents(); require(requests==1,"Latest catalog query was not cached");
      query->setText("100124"); statusCode=503; search->click();
      require(QTest::qWaitFor([&] { return search->isEnabled() && requests==2; },3000),"Catalog error request did not finish");
      require(results->count()==0 && dialog.findChild<QLabel *>("satelliteImportStatus")->text().contains("503"),"Catalog HTTP error was hidden");
      QTest::qWait(100); require(requests==2,"Catalog HTTP error triggered an automatic retry");
      statusCode=200; source->setCurrentIndex(1); query->setText("100123");
      dialog.findChild<QDateEdit *>("satelliteHistoryDate")->setDate(QDate(2000,6,27));
      auto *identity=dialog.findChild<QLineEdit *>("spaceTrackIdentity"); auto *password=dialog.findChild<QLineEdit *>("spaceTrackPassword");
      identity->setText("test@example.invalid"); password->setText("x+&= y"); search->click();
      require(QTest::qWaitFor([&] { return results->count()==1; },3000),"Historical catalog result was not displayed");
      require(logins==1 && historyRequests==1 && credentialEncoding && cookieReceived,"Historical sign-in, encoding or session handling failed");
      require(password->text().isEmpty(),"Historical search retained the password field");
      identity->clear(); search->click(); app.processEvents();
      require(logins==1 && historyRequests==1,"Historical data was fetched again instead of using its archive cache");
      dialog.findChild<QPushButton *>("importSatellite")->click(); require(imported,"Catalog selection could not be imported");
      loginResponse.clear();
      SatelliteImportDialog emptyLogin({"EarthMJ2000Eq"},{"Earth"},[](const auto &,const auto &) { return QString(); });
      emptyLogin.findChild<QComboBox *>("satelliteCatalogSource")->setCurrentIndex(1);
      emptyLogin.findChild<QLineEdit *>("satelliteCatalogQuery")->setText("100125");
      emptyLogin.findChild<QLineEdit *>("spaceTrackIdentity")->setText("test@example.invalid");
      emptyLogin.findChild<QLineEdit *>("spaceTrackPassword")->setText("x+&= y");
      emptyLogin.findChild<QPushButton *>("searchSatelliteCatalog")->click();
      require(QTest::qWaitFor([&] { return emptyLogin.findChild<QListWidget *>("satelliteCatalogResults")->count()==1; },3000),"Empty successful login reply prevented history lookup");
      require(logins==2 && historyRequests==2,"Empty login did not use the authenticated history route");
      loginResponse="{\"Login\":\"Failed\"}";
      SatelliteImportDialog rejectedLogin({"EarthMJ2000Eq"},{"Earth"},[](const auto &,const auto &) { return QString(); });
      rejectedLogin.findChild<QComboBox *>("satelliteCatalogSource")->setCurrentIndex(1);
      rejectedLogin.findChild<QLineEdit *>("satelliteCatalogQuery")->setText("100126");
      rejectedLogin.findChild<QLineEdit *>("spaceTrackIdentity")->setText("test@example.invalid");
      rejectedLogin.findChild<QLineEdit *>("spaceTrackPassword")->setText("x+&= y");
      auto *rejectedSearch=rejectedLogin.findChild<QPushButton *>("searchSatelliteCatalog");
      rejectedSearch->click();
      require(QTest::qWaitFor([&] { return rejectedSearch->isEnabled(); },3000),"Rejected login did not finish");
      require(logins==3 && historyRequests==2 && rejectedLogin.findChild<QLabel *>("satelliteImportStatus")->text().contains("sign-in failed"),"Rejected credentials were accepted");
      std::cout<<"PASS latest/historical catalog UI, six-digit ID, explicit network requests, HTTP errors, archive cache and credential/session handling\n";
      std::cout<<"PASS empty successful login and explicit credential rejection\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
