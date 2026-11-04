#include "EphemerisDialog.hpp"
#include "ResourceProperties.hpp"
#include "EphemerisFile.hpp"
#include "CoordinateSystem.hpp"
#include "GmatBase.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TimeSystemConverter.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QScrollArea>
#include <QGuiApplication>
#include <QScreen>
#include <QSignalBlocker>
#include <QFileInfo>
#include <memory>
#include <stdexcept>

namespace {
QStringList frames(const QString &format)
{
   QStringList result;
   for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::COORDINATE_SYSTEM)) {
      auto *frame=Moderator::Instance()->GetConfiguredObject(name); auto *axes=frame ? frame->GetOwnedObject(0) : nullptr;
      if (!axes) continue;
      if ((format=="SPK" || format=="CK") && !axes->IsOfType("MJ2000EqAxes")) continue;
      if (format=="Code-500") {
         if (!axes->IsOfType("MJ2000EqAxes") && !axes->IsOfType("BodyFixedAxes") && !axes->IsOfType("TrueOfDateAxes")) continue;
         if (!QStringList{"Earth","Luna","Sun","Mars","Jupiter","Saturn","Uranus","Neptune","Pluto","Mercury","Venus"}.contains(QString::fromStdString(frame->GetStringParameter("Origin")))) continue;
      }
      result.append(QString::fromStdString(name));
   } result.sort(); return result;
}
QStringList variantFields() { return {"CoordinateSystem","StepSize","OutputFormat","DistanceUnit","IncludeEventBoundaries","Interpolator","OutputTimeSystem"}; }
}
QSet<QString> applyEphemerisProperties(GmatBase &object,const QMap<QString,QString> &values)
{
   QSet<QString> applied;
   if (!object.IsOfType("EphemerisFile")) return applied;
   // Format changes select an interpolator and can reset the sampling step.
   // Dates are parsed in the current epoch format. Apply these before dependents.
   for (const auto &name:QStringList{"FileFormat","EpochFormat"}) if (values.contains(name)) { setResourceProperty(object,name,values.value(name)); applied.insert(name); }
   for (auto it=values.cbegin();it!=values.cend();++it) if (!applied.contains(it.key())) {
      const auto format=object.GetStringParameter("FileFormat");
      if (it.key()=="OutputTimeSystem" && format!="CCSDS-OEM") { applied.insert(it.key()); continue; }
      if ((it.key()=="DistanceUnit" || it.key()=="IncludeEventBoundaries") && format!="STK-TimePosVel") {
         if (!values.contains("FileFormat")) throw std::runtime_error("Distance units and event boundaries are STK output settings.");
         applied.insert(it.key()); continue;
      }
      setResourceProperty(object,it.key(),it.value()); applied.insert(it.key());
   }
   // Clones initially retain old reference pointers. Rebind them to the selected
   // names so validation sees the proposed spacecraft and coordinate system.
   const auto spacecraft=object.GetStringParameter("Spacecraft"),coordinate=object.GetStringParameter("CoordinateSystem");
   auto *sat=Moderator::Instance()->GetConfiguredObject(spacecraft),*frame=Moderator::Instance()->GetConfiguredObject(coordinate);
   if (!sat || !sat->IsOfType(Gmat::SPACECRAFT)) throw std::runtime_error("Select an available spacecraft.");
   if (!frame || !frame->IsOfType(Gmat::COORDINATE_SYSTEM)) throw std::runtime_error("Select an available output coordinate system.");
   object.SetRefObject(sat,Gmat::SPACECRAFT,spacecraft); object.SetRefObject(frame,Gmat::COORDINATE_SYSTEM,coordinate);
   const auto format=QString::fromStdString(object.GetStringParameter("FileFormat"));
   if (!frames(format).contains(QString::fromStdString(coordinate))) throw std::runtime_error("Select a coordinate system supported by the output format.");
   if (format=="CK" && object.GetStringParameter("StateType")!="Quaternion") throw std::runtime_error("CK output requires Quaternion states. This Cartesian ephemeris configuration cannot write CK; select an orbit format.");
   if (format=="Code-500" && object.GetStringParameter("StepSize")=="IntegratorSteps") throw std::runtime_error("Code-500 requires a positive numeric step size.");
   if (object.GetStringParameter("FullPathFileName").empty()) throw std::runtime_error("Select an ephemeris filename in an existing output directory.");
   if (QFileInfo(QString::fromStdString(object.GetStringParameter("FullPathFileName"))).isDir()) throw std::runtime_error("Select an output filename, rather than a directory.");
   if (!object.Validate()) throw std::runtime_error("The ephemeris rejected these settings.");
   return applied;
}
EphemerisDialog::EphemerisDialog(GmatBase &object,const QMap<QString,QString> &pending,QWidget *parent) : QDialog(parent)
{
   setObjectName("ephemerisDialog"); setWindowTitle("Ephemeris output — "+QString::fromStdString(object.GetName())); resize(660,std::min(760,QGuiApplication::primaryScreen()->availableGeometry().height()-80));
   QMap<QString,ResourceProperty> fields; QMap<QString,QString> initial;
   for (const auto &field:resourceProperties(object)) { fields.insert(field.name,field); initial.insert(field.name,field.value); }
   for (auto it=pending.cbegin();it!=pending.cend();++it) initial.insert(it.key(),it.value());
   auto snapshot=std::shared_ptr<GmatBase>(object.Clone());
   auto *layout=new QVBoxLayout(this); auto *help=new QLabel("Configure the output file, frame, sampling and time interval. OK keeps changes pending until resource Apply.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *scroll=new QScrollArea(this); scroll->setObjectName("ephemerisScroll"); scroll->setWidgetResizable(true); layout->addWidget(scroll,1); auto *content=new QWidget(scroll); scroll->setWidget(content); auto *groups=new QVBoxLayout(content);
   auto group=[&](const QString &label) { auto *box=new QGroupBox(label,content); groups->addWidget(box); return new QFormLayout(box); };
   auto choice=[&](QFormLayout *form,const QString &name,const QString &label,bool editable=false) {
      auto *combo=new QComboBox(content); combo->setObjectName("ephemeris_"+name); combo->setEditable(editable); combo->setInsertPolicy(QComboBox::NoInsert);
      combo->addItems(fields.value(name).choices.isEmpty() ? fields.value(name).references : fields.value(name).choices);
      if (combo->findText(initial.value(name))<0 && !initial.value(name).isEmpty()) combo->addItem(initial.value(name)); combo->setCurrentText(initial.value(name)); choices.insert(name,combo); form->addRow(label,combo);
   };
   auto line=[&](QFormLayout *form,const QString &name,const QString &label) { auto *edit=new QLineEdit(initial.value(name),content); edit->setObjectName("ephemeris_"+name); text.insert(name,edit); form->addRow(label,edit); };
   auto check=[&](QFormLayout *form,const QString &name,const QString &label) { auto *box=new QCheckBox(label,content); box->setObjectName("ephemeris_"+name); box->setChecked(initial.value(name)=="true"); checks.insert(name,box); form->addRow(box); };
   auto *output=group("Output"); check(output,"WriteEphemeris","Write ephemeris"); choice(output,"Spacecraft","Spacecraft"); choice(output,"FileFormat","File format");
   auto *filename=new QWidget(content); auto *fileRow=new QHBoxLayout(filename); fileRow->setContentsMargins(0,0,0,0); auto *path=new QLineEdit(initial.value("Filename"),filename); path->setObjectName("ephemeris_Filename"); text.insert("Filename",path); fileRow->addWidget(path,1);
   auto *browse=new QPushButton("Browse…",filename); browse->setObjectName("ephemerisBrowse"); fileRow->addWidget(browse); output->addRow("Filename",filename);
   connect(browse,&QPushButton::clicked,this,[this,path] { QFileDialog picker(this,"Choose ephemeris output",path->text()); picker.setObjectName("ephemerisFilePicker"); picker.setAcceptMode(QFileDialog::AcceptSave); picker.setFileMode(QFileDialog::AnyFile); picker.setOption(QFileDialog::DontConfirmOverwrite,true); if (picker.exec()==QDialog::Accepted && !picker.selectedFiles().isEmpty()) path->setText(picker.selectedFiles().first()); });
   choice(output,"CoordinateSystem","Output coordinate system");
   choice(output,"OutputTimeSystem","Output time system (OEM)");
   auto *sampling=group("Sampling and format"); line(sampling,"Interpolator","Interpolator"); text.value("Interpolator")->setReadOnly(true); line(sampling,"InterpolationOrder","Interpolation order (1–10)"); choice(sampling,"StepSize","Step size (s)",true); choice(sampling,"OutputFormat","Byte order"); choice(sampling,"DistanceUnit","Distance unit"); check(sampling,"IncludeEventBoundaries","Include event boundaries");
   auto *interval=group("Time interval"); choice(interval,"EpochFormat","Epoch format"); choice(interval,"InitialEpoch","Initial epoch",true); choice(interval,"FinalEpoch","Final epoch",true);
   groups->addStretch(); error=new QLabel(this); error->setObjectName("ephemerisError"); error->setWordWrap(true); layout->addWidget(error);
   previousFormat=choices.value("FileFormat")->currentText(); previousEpochFormat=choices.value("EpochFormat")->currentText();
   for (auto *edit:text) connect(edit,&QLineEdit::textChanged,error,&QLabel::clear);
   for (auto *combo:choices) connect(combo,&QComboBox::currentTextChanged,error,&QLabel::clear);
   for (auto *box:checks) connect(box,&QCheckBox::toggled,error,&QLabel::clear);
   connect(choices.value("FileFormat"),&QComboBox::currentTextChanged,this,[this] { changeFileFormat(); });
   connect(choices.value("EpochFormat"),&QComboBox::currentTextChanged,this,[this] { convertEpochs(); });
   updateFrames(initial.value("CoordinateSystem")); changeFileFormat();
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,snapshot] {
      try { std::unique_ptr<GmatBase> candidate(snapshot->Clone()); applyEphemerisProperties(*candidate,settings()); accept(); }
      catch (BaseException &exception) { error->setText(QString::fromStdString(exception.GetFullMessage())); }
      catch (const std::exception &exception) { error->setText(QString::fromUtf8(exception.what())); }
   });
}
void EphemerisDialog::updateFrames(const QString &frame)
{
   auto *combo=choices.value("CoordinateSystem"); const QSignalBlocker blocker(combo); combo->clear(); combo->addItems(frames(choices.value("FileFormat")->currentText()));
   if (combo->findText(frame)<0) combo->addItem(frame); combo->setCurrentText(frame);
}
void EphemerisDialog::changeFileFormat()
{
   const auto next=choices.value("FileFormat")->currentText();
   if (next!=previousFormat) {
      QMap<QString,QString> saved;
      for (const auto &name:variantFields()) saved.insert(name,choices.contains(name) ? choices.value(name)->currentText() : checks.contains(name) ? checks.value(name)->isChecked() ? "true" : "false" : text.value(name)->text());
      variants.insert(previousFormat,saved);
      auto restored=variants.value(next,saved); const auto compatible=frames(next);
      if (!compatible.contains(restored.value("CoordinateSystem"))) restored["CoordinateSystem"]=compatible.contains("EarthMJ2000Eq") ? "EarthMJ2000Eq" : compatible.value(0);
      if (next=="Code-500" && restored.value("StepSize")=="IntegratorSteps") restored["StepSize"]="60";
      if (!variants.contains(next)) { restored["Interpolator"]=next=="SPK" ? "Hermite" : "Lagrange"; if (next=="SPK") restored["StepSize"]="IntegratorSteps"; }
      updateFrames(restored.value("CoordinateSystem"));
      for (const auto &name:variantFields()) if (name!="CoordinateSystem") { if (auto *combo=choices.value(name)) combo->setCurrentText(restored.value(name)); else if (auto *box=checks.value(name)) box->setChecked(restored.value(name)=="true"); else text.value(name)->setText(restored.value(name)); }
      EphemerisFile naming("QtEphemerisNaming"); text.value("Filename")->setText(QString::fromStdString(naming.GetProperFileName(text.value("Filename")->text().toStdString(),next.toStdString(),false)));
      previousFormat=next;
   }
   choices.value("StepSize")->setEnabled(next!="SPK"); choices.value("OutputFormat")->setEnabled(next=="Code-500"); choices.value("DistanceUnit")->setEnabled(next=="STK-TimePosVel"); checks.value("IncludeEventBoundaries")->setEnabled(next=="STK-TimePosVel");
   choices.value("OutputTimeSystem")->setEnabled(next=="CCSDS-OEM");
}
void EphemerisDialog::convertEpochs()
{
   auto *format=choices.value("EpochFormat"); const auto next=format->currentText();
   try {
      QStringList converted;
      for (const auto &name:QStringList{"InitialEpoch","FinalEpoch"}) {
         const auto value=choices.value(name)->currentText().trimmed();
         if (value==(name=="InitialEpoch" ? "InitialSpacecraftEpoch" : "FinalSpacecraftEpoch")) { converted.append(value); continue; }
         Real mjd; std::string result; TimeSystemConverter::Instance()->Convert(previousEpochFormat.toStdString(),-999.999,value.toStdString(),next.toStdString(),mjd,result); converted.append(QString::fromStdString(result));
      }
      choices.value("InitialEpoch")->setCurrentText(converted[0]); choices.value("FinalEpoch")->setCurrentText(converted[1]); previousEpochFormat=next; error->clear();
   } catch (BaseException &failure) { const QSignalBlocker blocker(format); format->setCurrentText(previousEpochFormat); error->setText(QString::fromStdString(failure.GetFullMessage())); }
}
QMap<QString,QString> EphemerisDialog::settings() const
{
   QMap<QString,QString> result;
   for (auto it=choices.cbegin();it!=choices.cend();++it) if (((it.key()!="DistanceUnit") || choices.value("FileFormat")->currentText()=="STK-TimePosVel") && (it.key()!="OutputTimeSystem" || choices.value("FileFormat")->currentText()=="CCSDS-OEM")) result.insert(it.key(),it.value()->currentText().trimmed());
   for (auto it=text.cbegin();it!=text.cend();++it) result.insert(it.key(),it.value()->text().trimmed());
   for (auto it=checks.cbegin();it!=checks.cend();++it) if (it.key()!="IncludeEventBoundaries" || choices.value("FileFormat")->currentText()=="STK-TimePosVel") result.insert(it.key(),it.value()->isChecked() ? "true" : "false");
   return result;
}
