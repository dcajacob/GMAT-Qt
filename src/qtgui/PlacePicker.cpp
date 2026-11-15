#include "PlacePicker.hpp"
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <cmath>

PlacePicker::PlacePicker(QWidget *parent,const QUrl &service) : QDialog(parent)
{
   setObjectName("placePicker"); setWindowTitle("Find a place"); resize(680,510);
   endpoint=service.isEmpty() ? QUrl(QSettings().value("geocoding/photonEndpoint","https://photon.komoot.io/api/").toString()) : service;
   network=new QNetworkAccessManager(this);
   auto *layout=new QVBoxLayout(this);
   tabs=new QTabWidget(this); tabs->setObjectName("placeSource"); layout->addWidget(tabs);
   auto *offline=new QWidget(tabs); auto *offlineLayout=new QVBoxLayout(offline);
   offlineQuery=new QLineEdit(offline); offlineQuery->setObjectName("offlinePlaceQuery");
   offlineQuery->setPlaceholderText("Filter by city, region or country"); offlineLayout->addWidget(offlineQuery);
   offlineResults=new QListWidget(offline); offlineResults->setObjectName("offlinePlaceResults"); offlineLayout->addWidget(offlineResults);
   auto *offlineCredit=new QLabel("7,342 cities and towns · Natural Earth (public domain)",offline);
   offlineLayout->addWidget(offlineCredit); tabs->addTab(offline,"Offline places");
   auto *online=new QWidget(tabs); auto *onlineLayout=new QVBoxLayout(online);
   auto *queryRow=new QHBoxLayout;
   onlineQuery=new QLineEdit(online); onlineQuery->setObjectName("onlinePlaceQuery");
   onlineQuery->setPlaceholderText("Place name or address"); queryRow->addWidget(onlineQuery);
   search=new QPushButton("Search",online); search->setObjectName("onlinePlaceSearch"); queryRow->addWidget(search);
   onlineLayout->addLayout(queryRow);
   auto *serviceNotice=new QLabel("Search sends the entered place name or address to "+endpoint.host()+".",online);
   serviceNotice->setWordWrap(true); onlineLayout->addWidget(serviceNotice);
   onlineResults=new QListWidget(online); onlineResults->setObjectName("onlinePlaceResults"); onlineLayout->addWidget(onlineResults);
   status=new QLabel(online); status->setObjectName("onlinePlaceStatus"); status->setWordWrap(true); onlineLayout->addWidget(status);
   auto *credit=new QLabel("Search by <a href='https://github.com/komoot/photon'>Photon</a> · "
                          "<a href='https://www.openstreetmap.org/copyright'>© OpenStreetMap contributors</a>",online);
   credit->setOpenExternalLinks(true); onlineLayout->addWidget(credit); tabs->addTab(online,"Online search");
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Cancel,this);
   choose=buttons->addButton("Use location",QDialogButtonBox::AcceptRole); choose->setObjectName("usePlace"); choose->setEnabled(false);
   layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(offlineQuery,&QLineEdit::textChanged,this,[this] { filterPlaces(); });
   connect(search,&QPushButton::clicked,this,[this] { searchOnline(); });
   connect(onlineQuery,&QLineEdit::returnPressed,search,&QPushButton::click);
   connect(onlineQuery,&QLineEdit::textChanged,this,[this] {
      onlineResults->clear(); onlinePlaces.clear(); status->clear(); updateSelection();
   });
   for (auto *list:{offlineResults,onlineResults}) {
      connect(list,&QListWidget::currentRowChanged,this,[this] { updateSelection(); });
      connect(list,&QListWidget::itemDoubleClicked,this,[this] { if (selectedPlace()) accept(); });
   }
   connect(tabs,&QTabWidget::currentChanged,this,[this] { updateSelection(); });
   connect(this,&QDialog::finished,this,[this] { if (request) request->abort(); });
   filterPlaces();
}

void PlacePicker::filterPlaces()
{
   offlineResults->clear();
   const auto terms=offlineQuery->text().simplified().split(' ',Qt::SkipEmptyParts);
   const auto &places=groundMapPlaces();
   for (int index=0;index<places.size();++index) {
      bool matches=true;
      for (const auto &term:terms) if (!places[index].searchName.contains(term,Qt::CaseInsensitive)) { matches=false; break; }
      if (!matches) continue;
      auto *item=new QListWidgetItem(places[index].displayName,offlineResults);
      item->setData(Qt::UserRole,index);
      item->setToolTip(QString("Latitude %1° · Longitude %2°").arg(places[index].coordinates.y(),0,'f',5).arg(places[index].coordinates.x(),0,'f',5));
   }
   updateSelection();
}

std::optional<GroundMapPlace> PlacePicker::selectedPlace() const
{
   if (tabs->currentIndex()==0) {
      const auto *item=offlineResults->currentItem();
      if (item) return groundMapPlaces().at(item->data(Qt::UserRole).toInt());
   } else {
      const int index=onlineResults->currentRow();
      if (index>=0 && index<onlinePlaces.size()) return onlinePlaces[index];
   }
   return {};
}

void PlacePicker::updateSelection() { choose->setEnabled(selectedPlace().has_value()); }

void PlacePicker::showResults(const QVector<GroundMapPlace> &places)
{
   onlineResults->clear(); onlinePlaces=places;
   for (const auto &place:places) {
      auto *item=new QListWidgetItem(place.displayName,onlineResults);
      item->setToolTip(QString("Latitude %1° · Longitude %2°").arg(place.coordinates.y(),0,'f',6).arg(place.coordinates.x(),0,'f',6));
   }
   status->setText(places.isEmpty() ? "No matching places found." : "Select the matching location.");
   updateSelection();
}

void PlacePicker::searchOnline()
{
   const auto text=onlineQuery->text().simplified();
   if (text.size()<2) { status->setText("Enter at least two characters."); return; }
   if (request) return;
   if (cache.contains(text)) { showResults(cache.value(text)); return; }
   if (lastRequest.isValid() && lastRequest.elapsed()<1000) { status->setText("Wait a moment before searching again."); return; }
   if (!endpoint.isValid() || (endpoint.scheme()!="https" && endpoint.scheme()!="http")) {
      status->setText("The configured Photon search address is invalid."); return;
   }
   QUrl url=endpoint; QUrlQuery query(url);
   query.addQueryItem("q",text); query.addQueryItem("limit","10"); query.addQueryItem("lang","en"); url.setQuery(query);
   QNetworkRequest message(url); message.setTransferTimeout(15000);
   message.setRawHeader("User-Agent","GMAT-Qt/2026 (desktop place picker)");
   message.setRawHeader("Accept","application/geo+json, application/json");
   lastRequest.start(); search->setEnabled(false); status->setText("Searching…");
   request=network->get(message); auto *reply=request.data();
   connect(reply,&QNetworkReply::finished,this,[this,reply,text] {
      reply->deleteLater(); request=nullptr; search->setEnabled(true);
      if (onlineQuery->text().simplified()!=text) return;
      if (reply->error()!=QNetworkReply::NoError) { status->setText("Search failed. "+reply->errorString()); return; }
      QJsonParseError error;
      const auto document=QJsonDocument::fromJson(reply->readAll(),&error);
      if (error.error!=QJsonParseError::NoError || !document.object().value("features").isArray()) {
         status->setText("The search service returned an invalid response."); return;
      }
      QVector<GroundMapPlace> places;
      for (const auto &value:document.object()["features"].toArray()) {
         const auto feature=value.toObject(),geometry=feature["geometry"].toObject(),properties=feature["properties"].toObject();
         const auto coordinates=geometry["coordinates"].toArray();
         if (geometry["type"]!="Point" || coordinates.size()!=2 || !coordinates[0].isDouble() || !coordinates[1].isDouble()) continue;
         GroundMapPlace place; place.coordinates={coordinates[0].toDouble(),coordinates[1].toDouble()};
         if (!std::isfinite(place.coordinates.x()) || !std::isfinite(place.coordinates.y()) ||
             std::abs(place.coordinates.x())>180 || std::abs(place.coordinates.y())>90) continue;
         place.name=properties["name"].toString(); place.region=properties["state"].toString(); place.country=properties["country"].toString();
         QStringList parts{place.name};
         for (const auto &key:{"housenumber","street","city","state","country"}) {
            const auto part=properties[key].toString(); if (!part.isEmpty() && !parts.contains(part)) parts.append(part);
         }
         parts.removeAll(QString()); place.displayName=parts.join(", ");
         if (!place.displayName.isEmpty()) places.append(std::move(place));
      }
      cache.insert(text,places); showResults(places);
   });
}
