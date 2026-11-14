#include "PlacePicker.hpp"
#include "GroundStationDialog.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTimer>
#include <QUrlQuery>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
static PlacePicker *placePicker(QWidget &owner)
{
   for (auto *dialog:owner.findChildren<QDialog *>())
      if (auto *picker=dynamic_cast<PlacePicker *>(dialog)) return picker;
   return nullptr;
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("PlacePicker");
   TestSettings settings;
   try {
      require(argc==2,"Supply a startup file");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      QTcpServer server; require(server.listen(QHostAddress::LocalHost),"Cannot start local geocoder fixture");
      int requests=0;
      QByteArray response=R"({"features":[{"properties":{"name":"Fixture City","state":"Fixture State","country":"Fixture Country"},"geometry":{"type":"Point","coordinates":[12.75,-18.5]}}]})";
      const QUrl endpoint(QString("http://127.0.0.1:%1/api/").arg(server.serverPort()));
      QObject::connect(&server,&QTcpServer::newConnection,&server,[&] {
         auto *socket=server.nextPendingConnection();
         QObject::connect(socket,&QTcpSocket::readyRead,socket,[&,socket] {
            if (socket->property("responded").toBool()) return;
            const auto request=socket->readAll(); if (!request.contains("\r\n\r\n")) return;
            socket->setProperty("responded",true); ++requests;
            require(request.contains("q=") && request.contains("GMAT-Qt/2026"),"Search omitted its query or app identity");
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "+QByteArray::number(response.size())+"\r\nConnection: close\r\n\r\n"+response);
            socket->disconnectFromHost();
         });
         QObject::connect(socket,&QTcpSocket::disconnected,socket,&QTcpSocket::deleteLater);
      });
      PlacePicker picker(nullptr,endpoint); picker.show();
      require(QTest::qWaitForWindowExposed(&picker),"Place picker did not open");
      auto *filter=picker.findChild<QLineEdit *>("offlinePlaceQuery");
      auto *offline=picker.findChild<QListWidget *>("offlinePlaceResults");
      auto *choose=picker.findChild<QPushButton *>("usePlace");
      require(offline->count()==7342 && !choose->isEnabled(),"Offline catalog or initial selection is wrong");
      filter->setText("Denver"); require(offline->count()>0,"Offline city search failed");
      offline->setCurrentRow(0); const auto denver=picker.selectedPlace();
      require(denver && std::abs(denver->coordinates.y()-39.74)<.1 && std::abs(denver->coordinates.x()+104.99)<.1,"Offline selection returned incorrect coordinates");
      require(requests==0,"Offline search contacted a network service");
      filter->setText("No matching city 495872"); require(!choose->isEnabled() && !picker.selectedPlace(),"Empty filter retained a stale selection");
      picker.findChild<QTabWidget *>("placeSource")->setCurrentIndex(1);
      auto *query=picker.findChild<QLineEdit *>("onlinePlaceQuery");
      auto *search=picker.findChild<QPushButton *>("onlinePlaceSearch");
      auto *online=picker.findChild<QListWidget *>("onlinePlaceResults");
      query->setText("Fixture City"); app.processEvents(); require(requests==0,"Typing triggered a network request");
      search->click(); require(QTest::qWaitFor([&] { return online->count()==1; },3000),"Online result was not displayed");
      online->setCurrentRow(0); const auto selected=picker.selectedPlace();
      require(selected && selected->coordinates==QPointF(12.75,-18.5),"Online latitude/longitude were reversed or altered");
      search->click(); require(requests==1,"Repeated search bypassed the cache");
      query->setText("Changed query"); require(!picker.selectedPlace() && !choose->isEnabled(),"Edited query retained stale coordinates");
      response=R"({"features":[{"properties":{"name":"Invalid"},"geometry":{"type":"Point","coordinates":[200,100]}}]})";
      QTest::qWait(1050); search->click();
      require(QTest::qWaitFor([&] { return search->isEnabled() && requests==2; },3000),"Invalid-result request did not finish");
      require(online->count()==0 && !choose->isEnabled(),"Out-of-range online coordinates were accepted");
      query->setText("Malformed response"); response="not json"; QTest::qWait(1050); search->click();
      require(QTest::qWaitFor([&] { return search->isEnabled() && requests==3; },3000),"Malformed-result request did not finish");
      require(picker.findChild<QLabel *>("onlinePlaceStatus")->text().contains("invalid"),"Malformed response has no diagnostic");
      picker.close();
      MainWindow window; require(window.initialize(startup),"Station runtime failed to initialize");
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString mission="Create GroundStation Site;\nSite.StateType = Spherical;\nSite.HorizonReference = Ellipsoid;\nSite.Location1 = 0;\nSite.Location2 = 0;\nSite.Location3 = 1.25;\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nKeep = Keep + 1;\n";
      source->setPlainText(mission); require(window.buildScript(),"Station fixture did not build");
      auto *station=Moderator::Instance()->GetConfiguredObject("Site"); require(station,"Station fixture missing");
      GroundStationDialog dialog(*station,{},&window); dialog.show();
      std::exception_ptr selectionError;
      QTimer::singleShot(0,&dialog,[&] {
         try {
            auto *places=placePicker(dialog); require(places,"Station did not open its place picker");
            places->findChild<QLineEdit *>("offlinePlaceQuery")->setText("Denver");
            places->findChild<QListWidget *>("offlinePlaceResults")->setCurrentRow(0);
            places->findChild<QPushButton *>("usePlace")->click();
         } catch (...) { selectionError=std::current_exception(); if (auto *places=placePicker(dialog)) places->reject(); }
      });
      dialog.findChild<QPushButton *>("stationFindPlace")->click(); if (selectionError) std::rethrow_exception(selectionError);
      const auto values=dialog.settings();
      require(std::abs(values["Location1"].toDouble()-denver->coordinates.y())<1e-9,"Station did not fill latitude");
      require(std::abs(values["Location2"].toDouble()-(360+denver->coordinates.x()))<1e-9,"Station longitude normalization is incorrect");
      require(std::abs(values["Location3"].toDouble()-1.25)<1e-9,"Place selection changed station altitude");
      require(source->toPlainText()==mission && station->GetRealParameter("Location1")==0,"Place preview changed the mission before Apply");
      require(window.applyResourceChanges("Site",values,source->toPlainText()).isEmpty(),"Place selection could not be applied");
      require(window.runMission()==MainWindow::RunResult::Completed,"Place-selected station broke the mission");
      dialog.findChild<QComboBox *>("station_CentralBody")->setCurrentText("Mars");
      require(!dialog.findChild<QPushButton *>("stationFindPlace")->isEnabled(),"Earth place search was enabled on Mars");
      std::cout<<"PASS offline catalog/filter, explicit online search/cache/errors, coordinate order, station autofill/altitude, preview isolation and Apply\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
