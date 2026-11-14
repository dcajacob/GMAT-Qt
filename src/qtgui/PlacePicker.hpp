#pragma once
#include "GroundMapData.hpp"
#include <QDialog>
#include <QElapsedTimer>
#include <QMap>
#include <QPointer>
#include <QUrl>
#include <optional>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTabWidget;
class QNetworkAccessManager;
class QNetworkReply;

class PlacePicker final : public QDialog
{
public:
   explicit PlacePicker(QWidget *parent=nullptr,const QUrl &endpoint={});
   std::optional<GroundMapPlace> selectedPlace() const;
private:
   void filterPlaces();
   void searchOnline();
   void showResults(const QVector<GroundMapPlace> &places);
   void updateSelection();
   QTabWidget *tabs;
   QLineEdit *offlineQuery,*onlineQuery;
   QListWidget *offlineResults,*onlineResults;
   QLabel *status;
   QPushButton *search,*choose;
   QNetworkAccessManager *network;
   QPointer<QNetworkReply> request;
   QElapsedTimer lastRequest;
   QUrl endpoint;
   QVector<GroundMapPlace> onlinePlaces;
   QMap<QString,QVector<GroundMapPlace>> cache;
};
