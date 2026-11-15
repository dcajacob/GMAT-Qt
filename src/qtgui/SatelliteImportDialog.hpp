#pragma once
#include "SatelliteImportData.hpp"
#include <QDialog>
#include <QPointer>
#include <QUrl>
#include <QMap>
#include <functional>
#include <optional>

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;
class QTabWidget;
class QStackedWidget;

class SatelliteImportDialog final : public QDialog
{
public:
   using Apply=std::function<QString(const SatelliteImportData &,const QString &)>;
   SatelliteImportDialog(const QStringList &frames,const QStringList &bodies,Apply apply,QWidget *parent=nullptr);
   SatelliteImportData selectedData() const;
   QString spacecraftName() const;
   void configurePropagation(const QMap<int,QString> &commands,const QStringList &numericalPropagators,int selectedCommand=-1);
   // No value imports resources only. -1 appends a one-step Propagate command.
   std::optional<int> propagationCommand() const;
   QString statePropagator() const;
protected:
   void changeEvent(QEvent *event) override;
private:
   void updatePalette();
   void updateReadiness();
   void searchCatalog();
   bool fillAccountCredentials();
   void fetch(const QUrl &url,bool historical);
   void loadFile();
   void suggestName();
   void showRecords(const QByteArray &bytes,bool historical);
   QTabWidget *tabs;
   QLineEdit *name,*query,*identity,*password,*file,*fileEpoch,*naif,*stateEpoch;
   QComboBox *source,*fileFormat,*fileBody,*fileFrame,*stateFrame;
   QWidget *propagationOptions;
   QComboBox *propagationTarget,*numericalPropagator;
   QDateEdit *date;
   QListWidget *catalogResults,*fileResults;
   QStackedWidget *catalogView,*fileView;
   QVector<QLineEdit *> stateFields;
   QVector<SatelliteImportData> catalogRecords,fileRecords;
   QLabel *status,*guidance;
   QPushButton *search,*importButton;
   QNetworkAccessManager *network;
   QPointer<QNetworkReply> activeRequest;
   QString previousSuggestion;
   quint64 requestGeneration=0;
   bool spaceTrackAuthenticated=false;
   bool accountCredentialsUsed=false;
   Apply apply;
};
