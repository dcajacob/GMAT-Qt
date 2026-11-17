#include "SatelliteImportDialog.hpp"
#include "SpaceTrackAccount.hpp"
#include "SpaceTrackAccountDialog.hpp"
#include "VectorLineEdit.hpp"
#include "StandardIcons.hpp"
#include <QEvent>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDateEdit>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <cmath>
#include <algorithm>
#include <deque>
#include <stdexcept>

namespace {
bool officialSpaceTrackOrigin()
{
   const QUrl origin(QSettings().value("satellites/spaceTrackUrl","https://www.space-track.org").toString());
   return origin.scheme()=="https" && origin.userInfo().isEmpty() && origin.port(443)==443 &&
      (origin.host()=="www.space-track.org" || origin.host()=="space-track.org");
}
QString cacheFile(const QUrl &url)
{
   const auto directory=QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/satellite-catalog";
   QDir().mkpath(directory);
   return directory+'/'+QString::fromLatin1(QCryptographicHash::hash(url.toEncoded(),QCryptographicHash::Sha256).toHex())+".json";
}
QString responseError(QNetworkReply *reply)
{
   const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
   if (status!=200) return QString("Catalog service returned HTTP %1. %2").arg(status).arg(reply->errorString());
   if (reply->error()!=QNetworkReply::NoError) return reply->errorString();
   return {};
}
QNetworkRequest request(const QUrl &url)
{
   QNetworkRequest result(url); result.setTransferTimeout(20000);
   result.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
   result.setRawHeader("User-Agent","GMAT-Qt/2026 (interactive satellite import)");
   return result;
}
bool reserveSpaceTrackRequest()
{
   static std::deque<qint64> requests;
   const auto now=QDateTime::currentMSecsSinceEpoch();
   while (!requests.empty() && requests.front()<=now-3600000) requests.pop_front();
   if (requests.size()>=299 || std::count_if(requests.begin(),requests.end(),[&](qint64 time) { return time>now-60000; })>=29)
      return false;
   requests.push_back(now); return true;
}
}

SatelliteImportDialog::SatelliteImportDialog(const QStringList &frames,const QStringList &bodies,Apply callback,QWidget *parent)
   : QDialog(parent),apply(std::move(callback))
{
   setObjectName("satelliteImportDialog"); setWindowTitle("Import satellite"); resize(760,650);
   network=new QNetworkAccessManager(this);
   auto *layout=new QVBoxLayout(this);
   auto *naming=new QFormLayout;
   name=new QLineEdit(this); name->setObjectName("importSatelliteName"); naming->addRow("Spacecraft name",name); layout->addLayout(naming);
   tabs=new QTabWidget(this); tabs->setObjectName("satelliteImportSource"); layout->addWidget(tabs,1);
   auto *catalog=new QWidget(tabs); auto *catalogLayout=new QVBoxLayout(catalog); auto *catalogForm=new QFormLayout;
   source=new QComboBox(catalog); source->setObjectName("satelliteCatalogSource"); source->addItems({"Latest — CelesTrak","Historical — Space-Track"});
   catalogForm->addRow("Catalog source",source);
   query=new QLineEdit(catalog); query->setObjectName("satelliteCatalogQuery"); query->setPlaceholderText("Satellite name, NORAD number or international designator");
   catalogForm->addRow("Satellite",query);
   date=new QDateEdit(QDate::currentDate(),catalog); date->setObjectName("satelliteHistoryDate"); date->setCalendarPopup(true); date->setDisplayFormat("yyyy-MM-dd");
   catalogForm->addRow("Historical date",date);
   identity=new QLineEdit(catalog); identity->setObjectName("spaceTrackIdentity"); catalogForm->addRow("Space-Track username",identity);
   password=new QLineEdit(catalog); password->setObjectName("spaceTrackPassword"); password->setEchoMode(QLineEdit::Password); catalogForm->addRow("Space-Track password",password);
   catalogLayout->addLayout(catalogForm);
   auto *catalogActions=new QHBoxLayout;
   search=new QPushButton("Search catalog",catalog); search->setObjectName("searchSatelliteCatalog"); catalogActions->addWidget(search,1);
   auto *accountButton=new QPushButton("Space-Track account…",catalog);accountButton->setObjectName("configureSpaceTrackAccount");catalogActions->addWidget(accountButton);
   catalogLayout->addLayout(catalogActions);
   catalogResults=new QListWidget(catalog); catalogResults->setObjectName("satelliteCatalogResults");
   const auto resultsView=[](QListWidget *results,const QString &instruction,QWidget *parent) {
      auto *view=new QStackedWidget(parent);
      auto *empty=new QLabel(instruction,view); empty->setWordWrap(true); empty->setAlignment(Qt::AlignCenter);
      empty->setMargin(16);
      view->addWidget(results); view->addWidget(empty);
      return view;
   };
   catalogView=resultsView(catalogResults,"Enter a satellite name or NORAD number, then search the catalog. Select a record to fill the spacecraft name and orbital state.",catalog);
   catalogLayout->addWidget(catalogView,1);
   auto *credit=new QLabel(catalog);
   credit->setObjectName("satelliteCatalogCredits");
   credit->setWordWrap(true); credit->setOpenExternalLinks(true); catalogLayout->addWidget(credit);
   tabs->addTab(catalog,"Satellite catalog");
   auto *local=new QWidget(tabs); auto *localLayout=new QVBoxLayout(local); auto *localForm=new QFormLayout;
   auto *fileRow=new QWidget(local); auto *fileLayout=new QHBoxLayout(fileRow); fileLayout->setContentsMargins(0,0,0,0);
   file=new QLineEdit(fileRow); file->setObjectName("satelliteImportFile"); fileLayout->addWidget(file);
   auto *browse=new QPushButton("Browse…",fileRow); fileLayout->addWidget(browse); localForm->addRow("Orbital file",fileRow);
   fileFormat=new QComboBox(local); fileFormat->setObjectName("satelliteFileFormat");
   fileFormat->addItems({"TLE / OMM elements","CCSDS-OEM","STK","SPK","Code500"}); localForm->addRow("Format",fileFormat);
   fileEpoch=new QLineEdit(local); fileEpoch->setObjectName("satelliteFileEpoch"); fileEpoch->setPlaceholderText("2026-11-12T12:00:00.000Z"); localForm->addRow("Start epoch (UTC)",fileEpoch);
   fileBody=new QComboBox(local); fileBody->setObjectName("satelliteFileBody"); fileBody->addItems(bodies); fileBody->setCurrentText("Earth"); localForm->addRow("Central body",fileBody);
   fileFrame=new QComboBox(local); fileFrame->setObjectName("satelliteFileFrame"); fileFrame->addItems(frames); fileFrame->setCurrentText("EarthMJ2000Eq"); localForm->addRow("Coordinate system",fileFrame);
   connect(fileBody,&QComboBox::currentTextChanged,fileFrame,[this](const QString &body) {
      const int index=fileFrame->findText(body+"MJ2000Eq"); if (index>=0) fileFrame->setCurrentIndex(index);
   });
   naif=new QLineEdit(local); naif->setObjectName("satelliteNaifId"); localForm->addRow("SPK spacecraft NAIF ID",naif);
   localLayout->addLayout(localForm);
   auto *load=new QPushButton("Read orbital file",local); load->setObjectName("readSatelliteFile"); localLayout->addWidget(load);
   fileResults=new QListWidget(local); fileResults->setObjectName("satelliteFileResults");
   fileView=resultsView(fileResults,"Choose an orbital-elements file, then select the satellite and dated record to import.",local);
   localLayout->addWidget(fileView,1);
   auto *fileHelp=new QLabel("Select a dated record from TLE/OMM files. For sampled ephemerides, enter a start epoch inside the file's coverage.",local);
   fileHelp->setWordWrap(true); localLayout->addWidget(fileHelp); tabs->addTab(local,"Orbital file");
   auto *state=new QWidget(tabs); auto *stateLayout=new QFormLayout(state);
   stateEpoch=new QLineEdit(state); stateEpoch->setObjectName("satelliteStateEpoch"); stateEpoch->setPlaceholderText("2026-11-12T12:00:00.000Z"); stateLayout->addRow("Epoch (UTC)",stateEpoch);
   stateFrame=new QComboBox(state); stateFrame->setObjectName("satelliteStateFrame"); stateFrame->addItems(frames); stateFrame->setCurrentText("EarthMJ2000Eq"); stateLayout->addRow("Coordinate system",stateFrame);
   for (const auto &component:QStringList{"X","Y","Z","VX","VY","VZ"}) {
      auto *field=new VectorLineEdit(state); field->setObjectName("satelliteState"+component); stateFields.append(field);
      stateLayout->addRow(component+(component.startsWith('V') ? " (km/s)" : " (km)"),field);
   }
   VectorLineEdit::link({stateFields[0],stateFields[1],stateFields[2]});
   VectorLineEdit::link({stateFields[3],stateFields[4],stateFields[5]});
   tabs->addTab(state,"State vector");
   propagationOptions=new QWidget(this);
   auto *propagationForm=new QFormLayout(propagationOptions); propagationForm->setContentsMargins(0,0,0,0);
   propagationTarget=new QComboBox(propagationOptions); propagationTarget->setObjectName("satellitePropagationTarget");
   propagationTarget->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
   propagationTarget->setMinimumContentsLength(35);
   propagationForm->addRow("Mission propagation",propagationTarget);
   propagationTarget->setToolTip("Add the spacecraft to an existing Propagate command, append a one-step command, or import its resources only. Existing stopping conditions are retained.");
   numericalPropagator=new QComboBox(propagationOptions); numericalPropagator->setObjectName("satelliteStatePropagator");
   propagationForm->addRow("State-vector propagator",numericalPropagator);
   layout->addWidget(propagationOptions); propagationOptions->hide();
   guidance=new QLabel(this); guidance->setObjectName("satelliteImportGuidance"); guidance->setWordWrap(true); layout->addWidget(guidance);
   status=new QLabel(this); status->setObjectName("satelliteImportStatus"); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Cancel,this); importButton=buttons->addButton("Import satellite",QDialogButtonBox::AcceptRole);
   importButton->setObjectName("importSatellite"); layout->addWidget(buttons);
   auto sourceChanged=[this,catalogForm] {
      ++requestGeneration;
      const bool history=source->currentIndex()==1;
      for (auto *field:{static_cast<QWidget *>(date),static_cast<QWidget *>(identity),static_cast<QWidget *>(password)}) {
         field->setVisible(history); if (auto *label=catalogForm->labelForField(field)) label->setVisible(history);
      }
      catalogRecords.clear(); catalogResults->clear(); status->clear();
      query->setPlaceholderText(history ? "NORAD catalog number" : "Satellite name, NORAD number or international designator");
      if (history && officialSpaceTrackOrigin()) {
         const auto generation=requestGeneration;
         SpaceTrackAccount::instance().load(this,[this,generation](const QString &error) {
            if (generation!=requestGeneration) return;
            fillAccountCredentials();
            if (!error.isEmpty()) status->setText(error);
         });
      }
   };
   connect(source,&QComboBox::currentIndexChanged,this,[sourceChanged] { sourceChanged(); }); sourceChanged();
   auto invalidate=[this] { ++requestGeneration; catalogRecords.clear(); catalogResults->clear(); status->clear(); };
   connect(query,&QLineEdit::textChanged,this,invalidate);
   connect(date,&QDateEdit::dateChanged,this,invalidate);
   connect(identity,&QLineEdit::textChanged,this,[this] { spaceTrackAuthenticated=false; });
   for (auto *field:{identity,password}) connect(field,&QLineEdit::textEdited,this,[this] { accountCredentialsUsed=false; });
   connect(accountButton,&QPushButton::clicked,this,[this] {
      SpaceTrackAccountDialog dialog(SpaceTrackAccount::instance(),this);
      if (dialog.exec()==QDialog::Accepted) {
         spaceTrackAuthenticated=false;
         if (officialSpaceTrackOrigin()) {
            identity->clear();password->clear();fillAccountCredentials();
         }
      }
   });
   auto invalidateFile=[this] { fileRecords.clear(); fileResults->clear(); };
   connect(file,&QLineEdit::textChanged,this,[this,invalidateFile] {
      invalidateFile();
      fileFormat->setCurrentIndex(0);
      fileEpoch->clear();
      naif->clear();
      status->clear();
   });
   connect(fileFormat,&QComboBox::currentIndexChanged,this,invalidateFile);
   auto fileFormatChanged=[this,localForm] {
      const bool elements=fileFormat->currentIndex()==0;
      fileView->setVisible(elements);
      for (auto *field:{static_cast<QWidget *>(fileEpoch),static_cast<QWidget *>(fileBody),static_cast<QWidget *>(fileFrame)}) {
         field->setVisible(!elements); if (auto *label=localForm->labelForField(field)) label->setVisible(!elements);
      }
      naif->setVisible(fileFormat->currentText()=="SPK");
      localForm->labelForField(naif)->setVisible(fileFormat->currentText()=="SPK");
   };
   connect(fileFormat,&QComboBox::currentIndexChanged,this,[fileFormatChanged] { fileFormatChanged(); }); fileFormatChanged();
   connect(search,&QPushButton::clicked,this,[this] { searchCatalog(); }); connect(query,&QLineEdit::returnPressed,search,&QPushButton::click);
   connect(browse,&QPushButton::clicked,this,[this] {
      const auto path=QFileDialog::getOpenFileName(this,"Select orbital elements or ephemeris",file->text(),"Orbital data (*.tle *.txt *.omm *.json *.oem *.e *.bsp *.spk *.ephem);;All files (*)");
      if (!path.isEmpty()) { file->setText(path); loadFile(); }
   });
   connect(load,&QPushButton::clicked,this,[this] { loadFile(); });
   for (auto *list:{catalogResults,fileResults}) connect(list,&QListWidget::currentRowChanged,this,[this] { suggestName(); });
   connect(tabs,&QTabWidget::currentChanged,this,[this] { status->clear(); suggestName(); });
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this] {
      try {
         const auto data=selectedData(); const auto error=apply(data,spacecraftName());
         if (!error.isEmpty()) { status->setText(error); return; }
         accept();
      } catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); }
   });
   connect(this,&QDialog::finished,this,[this] { password->clear(); if (activeRequest) activeRequest->abort(); });
   for (auto *input:findChildren<QLineEdit *>()) connect(input,&QLineEdit::textChanged,this,[this] { updateReadiness(); });
   for (auto *input:findChildren<QComboBox *>()) connect(input,&QComboBox::currentIndexChanged,this,[this] { updateReadiness(); });
   for (auto *results:{catalogResults,fileResults}) connect(results,&QListWidget::currentRowChanged,this,[this] { updateReadiness(); });
   connect(tabs,&QTabWidget::currentChanged,this,[this] { updateReadiness(); });
   name->setToolTip("Suggested from the selected record. You can enter a different unused spacecraft name.");
   updateReadiness();
   updatePalette();
   query->setFocus();
}

void SatelliteImportDialog::updateReadiness()
{
   const bool propagating=propagationCommand().has_value();
   const bool stateVector=propagating && tabs->currentIndex()==2;
   numericalPropagator->setVisible(stateVector);
   auto *form=qobject_cast<QFormLayout *>(propagationOptions->layout());
   form->labelForField(numericalPropagator)->setVisible(stateVector);
   catalogView->setCurrentIndex(catalogResults->count() ? 0 : 1);
   fileView->setCurrentIndex(fileResults->count() ? 0 : 1);
   name->setPlaceholderText(tabs->currentIndex()==2 ? "New spacecraft name" : "Suggested from the selected orbital record");
   QString reason;
   try {
      selectedData();
      if (!QRegularExpression("^[A-Za-z][A-Za-z0-9_]*$").match(spacecraftName()).hasMatch())
         reason="Enter a spacecraft name beginning with a letter, using letters, numbers and underscores.";
      else if (stateVector && numericalPropagator->currentText().isEmpty())
         reason="Create a numerical propagator before assigning this state vector to the mission, or choose Import resources only.";
   } catch (const std::exception &error) {
      const bool selecting=tabs->currentIndex()==0 || (tabs->currentIndex()==1 && fileFormat->currentIndex()==0);
      if (!selecting) reason=QString::fromUtf8(error.what());
      importButton->setEnabled(false);
      guidance->setText(reason);
      return;
   }
   guidance->setText(reason);
   importButton->setEnabled(reason.isEmpty());
}

void SatelliteImportDialog::changeEvent(QEvent *event)
{
   QDialog::changeEvent(event);
   if (event->type()==QEvent::PaletteChange)
      QTimer::singleShot(0,this,[this] { updatePalette(); });
}

void SatelliteImportDialog::updatePalette()
{
   const auto sourcePalette=palette();
   if (auto *credit=findChild<QLabel *>("satelliteCatalogCredits")) {
      auto colors=credit->palette();
      for (auto group:{QPalette::Active,QPalette::Inactive,QPalette::Disabled})
         for (auto role:{QPalette::Link,QPalette::LinkVisited})
            colors.setColor(group,role,GmatQtIcons::readableColor(sourcePalette.color(group,role),sourcePalette.color(group,QPalette::Window),7.0));
      credit->setPalette(colors);
      credit->setText(QString("Latest elements from <a href='https://celestrak.org/' style='color:%1'>CelesTrak</a>. "
         "Historical searches require your <a href='https://www.space-track.org/' style='color:%1'>Space-Track</a> "
         "account. Catalog responses are cached.").arg(colors.color(QPalette::Link).name()));
   }
   for (auto *input:findChildren<QLineEdit *>()) if (!input->placeholderText().isEmpty()) {
      auto colors=input->palette();
      for (auto group:{QPalette::Active,QPalette::Inactive,QPalette::Disabled})
         colors.setColor(group,QPalette::PlaceholderText,GmatQtIcons::readableColor(sourcePalette.color(group,QPalette::PlaceholderText),colors.color(group,QPalette::Base),4.5));
      input->setPalette(colors);
   }
}

QString SatelliteImportDialog::spacecraftName() const { return name->text().trimmed(); }

void SatelliteImportDialog::configurePropagation(const QMap<int,QString> &commands,const QStringList &numericalPropagators,int selectedCommand)
{
   propagationTarget->clear();
   for (auto it=commands.cbegin();it!=commands.cend();++it) propagationTarget->addItem(it.value(),it.key());
   propagationTarget->addItem("New Propagate command (one step)",-1);
   propagationTarget->addItem("Import resources only",-2);
   const int selected=propagationTarget->findData(selectedCommand);
   propagationTarget->setCurrentIndex(selected>=0 && commands.contains(selectedCommand) ? selected : 0);
   numericalPropagator->clear(); numericalPropagator->addItems(numericalPropagators);
   propagationOptions->show(); updateReadiness();
}

std::optional<int> SatelliteImportDialog::propagationCommand() const
{
   if (propagationTarget->count()==0 || propagationTarget->currentData().toInt()==-2) return std::nullopt;
   return propagationTarget->currentData().toInt();
}

QString SatelliteImportDialog::statePropagator() const { return numericalPropagator->currentText(); }

void SatelliteImportDialog::suggestName()
{
   QString next;
   const auto &records=tabs->currentIndex()==0 ? catalogRecords : fileRecords;
   const int index=tabs->currentIndex()==0 ? catalogResults->currentRow() : fileResults->currentRow();
   if (index>=0 && index<records.size()) next=records[index].name;
   else if (tabs->currentIndex()==1) next=QFileInfo(file->text()).completeBaseName();
   if (next.isEmpty()) return;
   next.replace(QRegularExpression("[^A-Za-z0-9_]+"),"_");
   if (!next.front().isLetter()) next.prepend("Sat_");
   if (name->text().isEmpty() || name->text()==previousSuggestion) name->setText(next);
   previousSuggestion=next;
}

SatelliteImportData SatelliteImportDialog::selectedData() const
{
   if (tabs->currentIndex()==0 || (tabs->currentIndex()==1 && fileFormat->currentIndex()==0)) {
      const bool catalog=tabs->currentIndex()==0;
      const auto &records=catalog ? catalogRecords : fileRecords;
      const int row=catalog ? catalogResults->currentRow() : fileResults->currentRow();
      if (row<0 || row>=records.size()) throw std::runtime_error("Select an orbital record to import.");
      return records[row];
   }
   SatelliteImportData result;
   if (tabs->currentIndex()==1) {
      const QFileInfo input(file->text());
      if (!input.isFile() || !input.isReadable()) throw std::runtime_error("Select a readable ephemeris file.");
      result.sourceFile=input.absoluteFilePath(); result.name=input.completeBaseName(); result.propagatorType=fileFormat->currentText();
      result.epoch=fileEpoch->text().trimmed(); result.centralBody=fileBody->currentText(); result.coordinateSystem=fileFrame->currentText();
      if (result.propagatorType=="SPK") {
         bool valid=false; result.naifId=naif->text().toInt(&valid);
         if (!valid || !result.naifId) throw std::runtime_error("Enter the SPK spacecraft's NAIF ID.");
      }
   } else {
      result.propagatorType.clear(); result.epoch=stateEpoch->text().trimmed(); result.coordinateSystem=stateFrame->currentText();
      std::array<double,6> values{};
      for (int i=0;i<6;++i) {
         bool valid=false; values[i]=stateFields[i]->text().toDouble(&valid);
         if (!valid || !std::isfinite(values[i])) throw std::runtime_error("Enter six finite Cartesian state components.");
      }
      result.state=values;
   }
   satelliteEpochGregorian(result.epoch);
   return result;
}

void SatelliteImportDialog::loadFile()
{
   fileRecords.clear(); fileResults->clear();
   try {
      QFile input(file->text()); if (!input.open(QIODevice::ReadOnly)) throw std::runtime_error("The orbital file could not be opened.");
      const auto bytes=input.peek(65536);
      const auto header=QString::fromUtf8(bytes).trimmed();
      if (header.startsWith("CCSDS_OEM_VERS")) fileFormat->setCurrentText("CCSDS-OEM");
      else if (header.startsWith("stk.v")) fileFormat->setCurrentText("STK");
      else if (header.startsWith("DAF/SPK")) fileFormat->setCurrentText("SPK");
      else if (bytes.startsWith("EPHEM   ")) fileFormat->setCurrentText("Code500");
      else if (!bytes.contains('\0')) fileFormat->setCurrentIndex(0);
      if (fileFormat->currentIndex()==0) {
         if (input.size()>25*1024*1024) throw std::runtime_error("Select an orbital-elements file smaller than 25 MB.");
         fileRecords=parseSatelliteElements(input.readAll());
         for (auto &record:fileRecords) { record.source=QFileInfo(input).absoluteFilePath(); fileResults->addItem(record.name+" · "+record.catalogId+" · "+record.epoch); }
         if (!fileRecords.isEmpty()) fileResults->setCurrentRow(0);
      } else {
         const QString metadata=header.section("META_STOP",0,0);
         const auto timeField=[&metadata](const QString &key) {
            return QRegularExpression("(?m)^\\s*"+key+"\\s*=\\s*(\\S+)").match(metadata).captured(1);
         };
         auto epoch=timeField("USEABLE_START_TIME");
         if (epoch.isEmpty()) epoch=timeField("START_TIME");
         if (!epoch.isEmpty()) fileEpoch->setText(epoch);
         const auto scenario=QRegularExpression("ScenarioEpoch\\s+([^\\r\\n]+)").match(header);
         if (scenario.hasMatch()) {
            const auto epoch=QLocale(QLocale::English).toDateTime(scenario.captured(1).simplified(),"d MMM yyyy HH:mm:ss.zzz");
            if (epoch.isValid()) fileEpoch->setText(epoch.toString("yyyy-MM-ddTHH:mm:ss.zzz")+'Z');
         }
         status->setText("The selected native ephemeris propagator will read this file at the specified start epoch.");
      }
      suggestName();
   } catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); }
}

void SatelliteImportDialog::showRecords(const QByteArray &bytes,bool historical)
{
   try {
      catalogRecords=parseSatelliteElements(bytes); catalogResults->clear();
      for (auto &record:catalogRecords) {
         record.source=historical ? "Space-Track" : "CelesTrak";
         catalogResults->addItem(record.name+" · NORAD "+record.catalogId+" · "+record.epoch);
      }
      if (!catalogRecords.isEmpty()) catalogResults->setCurrentRow(0);
      status->setText(QString("%1 orbital records. Select the satellite and epoch to import.").arg(catalogRecords.size()));
   } catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); }
}

void SatelliteImportDialog::fetch(const QUrl &url,bool historical)
{
   const auto path=cacheFile(url); QFile cached(path);
   if (QFileInfo(path).isFile() && (historical || QFileInfo(path).lastModified().secsTo(QDateTime::currentDateTime())<7200) && cached.open(QIODevice::ReadOnly)) {
      search->setEnabled(true); showRecords(cached.readAll(),historical); return;
   }
   if (historical && !reserveSpaceTrackRequest()) {
      search->setEnabled(true); status->setText("Space-Track's request limit has been reached. Try again later."); return;
   }
   search->setEnabled(false); status->setText("Retrieving orbital records…");
   activeRequest=network->get(request(url)); auto *reply=activeRequest.data();
   const auto generation=requestGeneration;
   connect(reply,&QNetworkReply::finished,this,[this,reply,path,historical,generation] {
      reply->deleteLater(); activeRequest=nullptr; search->setEnabled(true);
      if (generation!=requestGeneration) return;
      const auto error=responseError(reply); if (!error.isEmpty()) {
         if (historical && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==401) spaceTrackAuthenticated=false;
         status->setText(error); return;
      }
      const auto bytes=reply->readAll();
      try { parseSatelliteElements(bytes); }
      catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); return; }
      QSaveFile output(path); if (output.open(QIODevice::WriteOnly)) { output.write(bytes); output.commit(); }
      showRecords(bytes,historical);
   });
}

bool SatelliteImportDialog::fillAccountCredentials()
{
   if (!officialSpaceTrackOrigin()) return false;
   const auto account=SpaceTrackAccount::instance().credentials();
   if (account.identity.isEmpty() || account.password.isEmpty() || !password->text().isEmpty() ||
       (!identity->text().isEmpty() && identity->text()!=account.identity)) return false;
   identity->setText(account.identity);
   password->setText(account.password);
   accountCredentialsUsed=true;
   return true;
}

void SatelliteImportDialog::searchCatalog()
{
   if (activeRequest) return;
   catalogRecords.clear(); catalogResults->clear();
   const auto text=query->text().trimmed(); bool numeric=false; const auto id=text.toLongLong(&numeric);
   if (text.size()<2 || (numeric && (id<=0 || id>999999999))) { status->setText("Enter a satellite name or valid NORAD number."); return; }
   QSettings settings;
   if (source->currentIndex()==0) {
      QUrl url(settings.value("satellites/celestrakUrl","https://celestrak.org/NORAD/elements/gp.php").toString());
      QUrlQuery query;
      const bool designator=QRegularExpression("^\\d{4}-\\d{3}[A-Za-z]*$").match(text).hasMatch();
      query.addQueryItem(numeric ? "CATNR" : designator ? "INTDES" : "NAME",text); query.addQueryItem("FORMAT","JSON");
      url.setQuery(query); fetch(url,false); return;
   }
   if (!numeric) { status->setText("Historical searches use a NORAD catalog number."); return; }
   const QUrl origin(settings.value("satellites/spaceTrackUrl","https://www.space-track.org").toString());
   const auto interval=date->date().addDays(-3).toString(Qt::ISODate)+"--"+date->date().addDays(1).toString(Qt::ISODate);
   const auto history=origin.resolved(QUrl("/basicspacedata/query/class/gp_history/NORAD_CAT_ID/"+text+"/EPOCH/"+interval+"/orderby/EPOCH%20desc/limit/100/format/json"));
   if (QFileInfo::exists(cacheFile(history))) { fetch(history,true); return; }
   if (spaceTrackAuthenticated) { fetch(history,true); return; }
   if (accountCredentialsUsed && !officialSpaceTrackOrigin()) {
      password->clear();accountCredentialsUsed=false;
      status->setText("The catalog endpoint changed. Enter credentials for this endpoint to continue.");return;
   }
   if ((identity->text().isEmpty() || password->text().isEmpty()) && officialSpaceTrackOrigin()) {
      const auto generation=requestGeneration;
      search->setEnabled(false);
      SpaceTrackAccount::instance().load(this,[this,generation](const QString &error) {
         search->setEnabled(true);
         if (generation!=requestGeneration) return;
         if (fillAccountCredentials()) searchCatalog();
         else status->setText(error.isEmpty() ? "Set up your Space-Track account or enter a sign-in for historical data." : error);
      });
      return;
   }
   if (identity->text().isEmpty() || password->text().isEmpty()) { status->setText("Enter your Space-Track sign-in for historical data, or load a historical file."); return; }
   if (!reserveSpaceTrackRequest()) { status->setText("Space-Track's request limit has been reached. Try again later."); return; }
   const QByteArray credentials="identity="+QUrl::toPercentEncoding(identity->text())+"&password="+QUrl::toPercentEncoding(password->text());
   auto message=request(origin.resolved(QUrl("/ajaxauth/login"))); message.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
   search->setEnabled(false); status->setText("Signing in to Space-Track…");
   activeRequest=network->post(message,credentials); auto *reply=activeRequest.data();
   const auto generation=requestGeneration;
   connect(reply,&QNetworkReply::finished,this,[this,reply,history,generation] {
      reply->deleteLater(); activeRequest=nullptr; password->clear();
      if (generation!=requestGeneration) { search->setEnabled(true); return; }
      const auto error=responseError(reply);
      const auto response=QJsonDocument::fromJson(reply->readAll()).object();
      const auto loginStatus=response.value("Login").toString();
      // Space-Track may return an empty successful login body. The protected
      // history request verifies the resulting cookie session.
      if (!error.isEmpty() || (!loginStatus.isEmpty() && loginStatus.compare("Success",Qt::CaseInsensitive)!=0)) {
         search->setEnabled(true); status->setText(error.isEmpty() ? "Space-Track sign-in failed." : error); return;
      }
      spaceTrackAuthenticated=true;
      fetch(history,true);
   });
}
