#include "SolarSystemPanel.hpp"
#include "SolarSystem.hpp"
#include "DeFile.hpp"
#include "UserParameter.hpp"
#include "InspectionDialog.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <stdexcept>
#include <cmath>

SolarSystemSettings solarSystemSettings(SolarSystem &system)
{
   SolarSystemSettings result;
   for (const auto &type:system.GetPlanetarySourceTypes()) {
      const auto name=QString::fromStdString(type); result.sources.append(name);
      if (name.startsWith("DE")) result.deFiles.insert(name,QString::fromStdString(system.GetPlanetarySourceName(type)));
   }
   for (const auto *field:{"EphemerisSource","SPKFilename","PCKFilename"}) result.values.insert(field,QString::fromStdString(system.GetStringParameter(field)));
   const auto current=result.values.value("EphemerisSource");
   if (result.deFiles.contains(current)) result.deSource=current;
   else for (const auto &type:system.GetPlanetarySourceTypesInUse())
      if (result.deFiles.contains(QString::fromStdString(type))) { result.deSource=QString::fromStdString(type); break; }
   if (result.deSource.isEmpty() && !result.deFiles.isEmpty()) result.deSource=result.deFiles.firstKey();
   result.values.insert("DEFilename",result.deFiles.value(result.deSource));
   result.values.insert("@DEFileSource",result.deSource);
   result.values.insert("UseTTForEphemeris",system.GetBooleanParameter("UseTTForEphemeris") ? "true" : "false");
   result.values.insert("EphemerisUpdateInterval",QString::number(system.GetEphemUpdateInterval(),'g',17));
   return result;
}

QString solarSystemScript(const QString &script,const SolarSystemSettings &initial,const QMap<QString,QString> &values,const QString &firstMissionStatement)
{
   const auto selected=values.value("EphemerisSource");
   if (!initial.sources.contains(selected)) throw std::runtime_error("Choose an ephemeris source available in this runtime.");
   bool ok=false; const auto interval=values.value("EphemerisUpdateInterval").toDouble(&ok);
   if (!ok || !std::isfinite(interval) || interval<0) throw std::runtime_error("Enter a finite ephemeris update interval of zero or greater (seconds).");
   if (values.value("UseTTForEphemeris")!="true" && values.value("UseTTForEphemeris")!="false") throw std::runtime_error("Choose whether to use TT for ephemeris.");
   QString block;
   auto assignment=[&](const QString &field,const QString &value) { block+="GMAT SolarSystem."+field+" = "+value+";\n"; };
   auto literal=[](const QString &value) {
      // File names with quotes or percent signs cannot safely round-trip in
      // GMAT's filename parser. Do not silently alter such a path.
      for (const auto ch:value) if (ch.unicode()<32 || ch==QChar(0x2028) || ch==QChar(0x2029) || ch=='\'' || ch=='%') throw std::runtime_error("Choose a filename without apostrophes, percent signs or control characters.");
      return "'"+value+"'";
   };
   const auto deSource=initial.deFiles.contains(selected) ? selected : values.value("@DEFileSource",initial.deSource);
   if (!initial.deFiles.contains(deSource)) throw std::runtime_error("The DE fallback source is unavailable. Reopen the solar-system panel.");
   const QFileInfo deInfo(values.value("DEFilename"));
   const qint64 recordBytes=sizeof(double)*(deSource=="DE405" ? DeFile::ARRAY_SIZE_405 : deSource=="DE421" ? DeFile::ARRAY_SIZE_421 : DeFile::ARRAY_SIZE_424);
   // The binary reader consumes two header records and a coefficient record
   // without checking short header reads. Reject incomplete files at the GUI
   // boundary; the engine still validates the DE version and ephemeris data.
   if (!deInfo.isFile() || !deInfo.isReadable() || deInfo.size()<3*recordBytes || deInfo.size()%recordBytes!=0)
      throw std::runtime_error("Choose a readable, complete binary DE ephemeris file for the selected source.");
   // DEFilename applies to the selected DE source. Preserve that pairing when
   // switching to SPICE, whose DEFilename setter cannot configure a DE file.
   assignment("EphemerisSource",deSource);
   assignment("DEFilename",literal(values.value("DEFilename")));
   if (selected!=deSource) assignment("EphemerisSource",selected);
   if (initial.sources.contains("SPICE")) {
      assignment("SPKFilename",literal(values.value("SPKFilename")));
      assignment("PCKFilename",literal(values.value("PCKFilename")));
   }
   assignment("UseTTForEphemeris",values.value("UseTTForEphemeris"));
   assignment("EphemerisUpdateInterval",QString::number(interval,'g',17));
   return setConfigurationBlock(script,"SolarSystem",{"Ephemeris","EphemerisSource","DEFilename","SPKFilename","PCKFilename","UseTTForEphemeris","EphemerisUpdateInterval"},block,firstMissionStatement);
}

QString solarSystemSettingsError(SolarSystem &system,const QMap<QString,QString> &values)
{
   const auto actual=solarSystemSettings(system);
   for (const auto *field:{"EphemerisSource","UseTTForEphemeris"})
      if (actual.values.value(field)!=values.value(field)) return QString("GMAT did not retain %1. The previous configuration was restored.").arg(field);
   if (system.GetEphemUpdateInterval()!=values.value("EphemerisUpdateInterval").toDouble()) return "GMAT did not retain the update interval. The previous configuration was restored.";
   for (const auto *field:{"DEFilename","SPKFilename","PCKFilename"}) {
      if (QString(field)!="DEFilename" && !actual.sources.contains("SPICE")) continue;
      const QFileInfo expected(values.value(field)),applied(actual.values.value(field));
      if (!expected.exists() || expected.canonicalFilePath()!=applied.canonicalFilePath())
         return QString("GMAT did not retain %1. The previous configuration was restored.").arg(field);
   }
   return {};
}

SolarSystemPanel::SolarSystemPanel(SolarSystem &system,Apply apply,QWidget *parent)
   : EditablePanel(parent),initial(solarSystemSettings(system)),deFiles(initial.deFiles),deSource(initial.deSource)
{
   setObjectName("solarSystemPanel"); auto *layout=new QVBoxLayout(this);
   layout->addWidget(new QLabel("Solar System — planetary ephemeris",this));
   auto *form=new QFormLayout; layout->addLayout(form);
   source=new QComboBox(this); source->setObjectName("ephemerisSource"); source->addItems(initial.sources); source->setCurrentText(initial.values.value("EphemerisSource")); form->addRow("Ephemeris source",source);
   auto fileRow=[&](const QString &title,const QString &name,const QString &value) {
      auto *row=new QWidget(this); auto *box=new QHBoxLayout(row); box->setContentsMargins(0,0,0,0);
      auto *edit=new QLineEdit(value,row); edit->setObjectName(name); box->addWidget(edit,1);
      auto *browse=new QPushButton("Browse…",row); browse->setObjectName(name+"Browse"); box->addWidget(browse);
      connect(browse,&QPushButton::clicked,this,[this,edit,title] { const auto path=QFileDialog::getOpenFileName(this,"Select "+title,edit->text()); if (!path.isEmpty()) edit->setText(path); });
      form->addRow(title,row); return edit;
   };
   deFile=fileRow("DE ephemeris file","deFilename",initial.values.value("DEFilename"));
   spkFile=fileRow("Planetary SPK file","spkFilename",initial.values.value("SPKFilename"));
   pckFile=fileRow("Planetary PCK file","pckFilename",initial.values.value("PCKFilename"));
   interval=new QLineEdit(initial.values.value("EphemerisUpdateInterval"),this); interval->setObjectName("ephemerisUpdateInterval"); form->addRow("Update interval (s)",interval);
   useTT=new QCheckBox("Use TT for ephemeris",this); useTT->setObjectName("useTTForEphemeris"); useTT->setChecked(initial.values.value("UseTTForEphemeris")=="true"); form->addRow(useTT);
   auto *note=new QLabel("Zero updates the ephemeris at every request. Changes stay pending until Apply. Switching sources retains pending DE filenames for this panel; Apply saves the selected DE file (or the DE fallback for SPICE).",this); note->setWordWrap(true); layout->addWidget(note);
   status=new QLabel(this); status->setObjectName("solarSystemStatus"); status->setWordWrap(true); layout->addWidget(status); layout->addStretch();
   const auto update=[this] {
      if (deFiles.contains(source->currentText())) deSource=source->currentText();
      deFile->setText(deFiles.value(deSource));
      deFile->parentWidget()->setEnabled(deFiles.contains(source->currentText()));
      const bool kernels=initial.sources.contains("SPICE") && source->currentText()!="TwoBodyPropagation";
      spkFile->parentWidget()->setEnabled(kernels); pckFile->parentWidget()->setEnabled(kernels);
   };
   connect(deFile,&QLineEdit::textChanged,this,[this](const QString &path) { deFiles[deSource]=path; });
   connect(source,&QComboBox::currentTextChanged,this,[update] { update(); }); update();
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Close,this); layout->addWidget(buttons);
   auto *preview=new QPushButton("Show script…",this); preview->setObjectName("showScript"); buttons->addButton(preview,QDialogButtonBox::ActionRole);
   const auto previewText=QString::fromStdString(system.GetGeneratingString(Gmat::SHOW_SCRIPT));
   connect(preview,&QPushButton::clicked,this,[this,previewText] { InspectionDialog dialog("Solar-system script",previewText,"Applied settings. Pending edits are not included.",this); dialog.exec(); });
   connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,this,[this,apply] {
      if (!hasChanges()) { status->setText("No changes to apply."); return; }
      try { const auto error=apply(settings()); if (error.isEmpty()) { applied=true; appliedSuccessfully(); } else status->setText(error); }
      catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); }
   });
   connect(buttons->button(QDialogButtonBox::Close),&QPushButton::clicked,this,[this] { parentWidget()->close(); });
}
QMap<QString,QString> SolarSystemPanel::settings() const
{
   return {{"EphemerisSource",source->currentText()},{"DEFilename",deFile->text()},{"@DEFileSource",deSource},{"SPKFilename",spkFile->text()},{"PCKFilename",pckFile->text()},{"UseTTForEphemeris",useTT->isChecked() ? "true" : "false"},{"EphemerisUpdateInterval",interval->text()}};
}
bool SolarSystemPanel::hasChanges() const { return !applied && settings()!=initial.values; }
