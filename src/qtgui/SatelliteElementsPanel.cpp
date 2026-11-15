#include "SatelliteElementsPanel.hpp"
#include "SatelliteImportData.hpp"
#include "FileUtil.hpp"
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <algorithm>

SatelliteElementsPanel::SatelliteElementsPanel(QWidget *parent) : QWidget(parent)
{
   setObjectName("satelliteElementsPanel");
   auto *layout=new QVBoxLayout(this);
   fileLabel=new QLabel(this);fileLabel->setObjectName("satelliteElementsFile");
   fileLabel->setTextFormat(Qt::PlainText);fileLabel->setWordWrap(true);
   fileLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
   layout->addWidget(fileLabel);
   auto *form=new QFormLayout;
   const QList<QPair<QString,QString>> labels={{"NORAD_CAT_ID","NORAD ID"},{"EPOCH","Epoch (UTC)"},
      {"INCLINATION","Inclination (deg)"},{"RA_OF_ASC_NODE","RAAN (deg)"},
      {"ECCENTRICITY","Eccentricity"},{"ARG_OF_PERICENTER","Argument of perigee (deg)"},
      {"MEAN_ANOMALY","Mean anomaly (deg)"},{"MEAN_MOTION","Mean motion (rev/day)"},
      {"BSTAR","BSTAR (1/Earth radii)"}};
   for (const auto &entry:labels) {
      auto *field=new QLineEdit(this);field->setReadOnly(true);
      field->setObjectName("satelliteMean_"+entry.first);fields.insert(entry.first,field);
      form->addRow(entry.second,field);
   }
   layout->addWidget(new QLabel("SGP4 mean elements at the orbital record's epoch.",this));
   layout->addLayout(form);
   status=new QLabel(this);status->setObjectName("satelliteElementsStatus");status->setWordWrap(true);
   layout->addWidget(status);
   source=new QPlainTextEdit(this);source->setObjectName("satelliteElementsSource");
   source->setReadOnly(true);source->setLineWrapMode(QPlainTextEdit::NoWrap);
   source->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
   source->setAccessibleName("TLE or OMM source record");
   layout->addWidget(source,1);
}

bool SatelliteElementsPanel::setSource(const QString &filename,const QString &catalogId)
{
   for (auto *field:fields) field->clear();
   source->clear();status->clear();fileLabel->setText(filename);
   if (filename.trimmed().isEmpty()) return false;
   const auto suffix=QFileInfo(filename).suffix().toLower();
   const bool known=suffix=="tle" || suffix=="omm";
   QString path=filename;
   if (!QFileInfo(path).isAbsolute()) path=QString::fromStdString(GmatFileUtil::FindFile(path.toStdString(),true,true));
   QFile input(path);
   if (!input.open(QIODevice::ReadOnly)) {
      status->setText("The orbital-elements file could not be opened.");return known;
   }
   fileLabel->setText(QFileInfo(input).absoluteFilePath());
   const auto prefix=input.peek(512).trimmed();
   const bool candidate=known || prefix.startsWith("CCSDS_OMM_VERS") || prefix.startsWith('{') || prefix.startsWith('[') ||
      QRegularExpression("(?:^|\\n)1 [A-Z0-9]{5}").match(QString::fromUtf8(prefix)).hasMatch();
   if (!candidate) return false;
   const auto bytes=input.readAll();source->setPlainText(QString::fromUtf8(bytes));
   try {
      const auto records=parseSatelliteElements(bytes);
      const auto matches=[&](const SatelliteImportData &record) {
         if (catalogId.isEmpty() || catalogId=="SatId" || record.catalogId.compare(catalogId,Qt::CaseInsensitive)==0) return true;
         bool recordNumeric=false,requestedNumeric=false;
         const auto recordNumber=record.catalogId.toULongLong(&recordNumeric);
         const auto requestedNumber=catalogId.toULongLong(&requestedNumeric);
         return recordNumeric && requestedNumeric && recordNumber==requestedNumber;
      };
      const auto selected=std::find_if(records.cbegin(),records.cend(),matches);
      if (selected==records.cend()) {
         status->setText("NORAD ID "+catalogId+" was not found in this orbital-elements file.");return true;
      }
      for (auto it=selected->meanElements.cbegin();it!=selected->meanElements.cend();++it)
         if (fields.contains(it.key())) fields.value(it.key())->setText(it.value());
      fields.value("NORAD_CAT_ID")->setText(selected->catalogId);
      fields.value("EPOCH")->setText(selected->epoch);
      if (records.size()>1) source->setPlainText(QString::fromUtf8(selected->elements));
   } catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); }
   return true;
}
