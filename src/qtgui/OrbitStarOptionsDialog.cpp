#include "OrbitStarOptionsDialog.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>
#include <algorithm>
#include <stdexcept>

OrbitStarOptionsDialog::OrbitStarOptionsDialog(const std::optional<QtStarSetting> &initial,QWidget *parent,std::optional<int> pendingCount)
   : QDialog(parent)
{
   setObjectName("orbitStarOptionsDialog"); setWindowTitle("Star options");
   const auto setting=initial.value_or(QtStarSetting());
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Choose a star display style. Virtual Reality is a display preset and does not connect to a device. Printer suppresses star drawing. Monitor and Virtual Reality use their preset star count; custom values remain stored for later use. The existing Stars control turns star drawing on or off. OK keeps changes pending until Apply.",this);
   help->setWordWrap(true); layout->addWidget(help);
   auto *style=new QFormLayout; layout->addLayout(style);
   preset=new QComboBox(this); preset->setObjectName("starPreset"); preset->addItems({"Qt default","Monitor","Virtual Reality","Printer","Custom"}); preset->setCurrentText(initial ? setting.preset : "Qt default"); style->addRow("Display preset",preset);
   auto *description=new QLabel(this); description->setObjectName("starPresetDescription"); description->setWordWrap(true); layout->addWidget(description);
   catalogFields=new QWidget(this); auto *catalogLayout=new QFormLayout(catalogFields); catalogLayout->setContentsMargins(0,0,0,0);
   auto *path=new QWidget(catalogFields); auto *pathLayout=new QHBoxLayout(path); pathLayout->setContentsMargins(0,0,0,0);
   catalog=new QLineEdit(setting.catalog,path); catalog->setObjectName("starCatalog"); pathLayout->addWidget(catalog,1);
   auto *browse=new QPushButton("Browse…",path); browse->setObjectName("starCatalogBrowse"); pathLayout->addWidget(browse);
   catalogLayout->addRow("Star catalog",path); layout->addWidget(catalogFields);
   connect(browse,&QPushButton::clicked,this,[this] {
      const auto selected=QFileDialog::getOpenFileName(this,"Choose star catalog",catalog->text(),"Star catalogs (*.txt *.dat);;All files (*)");
      if (!selected.isEmpty()) catalog->setText(selected);
   });
   auto *catalogHelp=new QLabel("Custom resolves a bare or relative catalog name through the configured star path. Monitor and Virtual Reality use the bundled catalog. Qt default removes the override and uses the built-in Qt star settings.",this); catalogHelp->setWordWrap(true); layout->addWidget(catalogHelp);
   customFields=new QWidget(this); auto *fields=new QFormLayout(customFields); fields->setContentsMargins(0,0,0,0);
   auto number=[&](const char *name,const QString &label,double value) { auto *edit=new QLineEdit(QString::number(value,'g',17),customFields); edit->setObjectName(name); fields->addRow(label,edit); return edit; };
   count=number("starCustomCount","Custom star count",pendingCount.value_or(setting.customCount));
   minMagnitude=number("starMinMagnitude","Minimum magnitude",setting.minMagnitude); maxMagnitude=number("starMaxMagnitude","Maximum magnitude",setting.maxMagnitude);
   minPixels=number("starMinPixels","Minimum size (pixels)",setting.minPixels); maxPixels=number("starMaxPixels","Maximum size (pixels)",setting.maxPixels);
   minDimRatio=number("starMinDimRatio","Minimum dimming ratio (0–1)",setting.minDimRatio); layout->addWidget(customFields);
   auto *status=new QLabel(this); status->setObjectName("starOptionsStatus"); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); buttons->setObjectName("starOptionsButtons"); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,status] { try { settings(); accept(); } catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); } });
   auto dependencies=[this,description] {
      const auto selected=preset->currentText(); customFields->setEnabled(selected=="Custom"); catalogFields->setEnabled(selected=="Custom");
      description->setText(selected=="Qt default" ? "Use the existing Qt star display and the ordinary OrbitView Star count." : selected=="Custom" ? "Use the retained magnitude, pixel-size, dimming and count values below." : selected=="Printer" ? "Suppress star drawing while retaining the catalog and custom values." : "Use the selected display style and its 40,000-star limit. Custom values below are retained but do not determine this preset's appearance.");
   };
   connect(preset,&QComboBox::currentTextChanged,this,[dependencies](const QString &) { dependencies(); }); dependencies();
   const auto screen=QGuiApplication::primaryScreen()->availableGeometry(); resize(std::min(680,screen.width()-80),std::min(600,screen.height()-80));
}
std::optional<QtStarSetting> OrbitStarOptionsDialog::settings() const
{
   if (preset->currentText()=="Qt default") return std::nullopt;
   QtStarSetting setting; setting.preset=preset->currentText(); setting.catalog=catalog->text();
   bool ok=false; setting.customCount=count->text().toInt(&ok); if (!ok) throw std::runtime_error("Enter a positive integer custom star count.");
   auto value=[](QLineEdit *field) { bool valid=false; const auto number=field->text().toDouble(&valid); if (!valid) throw std::runtime_error("Enter finite numeric star values."); return number; };
   setting.minMagnitude=value(minMagnitude); setting.maxMagnitude=value(maxMagnitude); setting.minPixels=value(minPixels); setting.maxPixels=value(maxPixels); setting.minDimRatio=value(minDimRatio);
   return qtStars(qtStarsJson(setting));
}
