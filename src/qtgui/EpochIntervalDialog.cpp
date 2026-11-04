#include "EpochIntervalDialog.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "TimeSystemConverter.hpp"
#include "GmatTime.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QSignalBlocker>
#include <cmath>
#include <algorithm>
#include <memory>
#include <stdexcept>

namespace {
GmatTime epoch(const QString &format,const QString &value)
{
   GmatTime mjd; std::string result;
   TimeSystemConverter::Instance()->Convert(format.toStdString(),GmatTime(-999.999),value.trimmed().toStdString(),"TAIModJulian",mjd,result);
   if (!std::isfinite(mjd.GetMjd())) throw std::runtime_error("Enter a finite epoch.");
   return mjd;
}
}
bool hasEpochInterval(GmatBase &object) { return object.IsOfType("Simulator") || object.IsOfType(Gmat::DATA_FILTER); }
QMap<QString,QString> epochIntervalSettings(GmatBase &object)
{
   QMap<QString,QString> values;
   for (const auto &name:QStringList{"EpochFormat","InitialEpoch","FinalEpoch"}) values.insert(name,QString::fromStdString(object.GetStringParameter(name.toStdString())));
   return values;
}
QMap<QString,QString> convertEpochInterval(const QMap<QString,QString> &values,const QString &format)
{
   auto converted=values;
   if (values.value("EpochFormat")==format) return converted;
   for (const auto &name:QStringList{"InitialEpoch","FinalEpoch"}) {
      GmatTime mjd; std::string result; bool leapSecond=false;
      TimeSystemConverter::Instance()->Convert(values.value("EpochFormat").toStdString(),GmatTime(-999.999),values.value(name).trimmed().toStdString(),format.toStdString(),mjd,result,1,&leapSecond);
      if (!std::isfinite(mjd.GetMjd())) throw std::runtime_error("Enter a finite epoch.");
      // The converter's numeric display string passes through Real, which can
      // move a boundary by more than the simulator's nanosecond tolerance.
      // Keep the GmatTime representation when serializing numeric epochs.
      // GregorianDate's parser accepts milliseconds. Use its supported
      // calendar representation; numeric epochs retain full time precision.
      converted[name]=QString::fromStdString(format.endsWith("ModJulian") ? mjd.ToString() : result);
   }
   converted["EpochFormat"]=format;
   return converted;
}
QSet<QString> applyEpochIntervalProperties(GmatBase &object,const QMap<QString,QString> &changes)
{
   QSet<QString> consumed;
   if (!hasEpochInterval(object)) return consumed;
   for (const auto &name:QStringList{"EpochFormat","InitialEpoch","FinalEpoch"}) if (changes.contains(name)) consumed.insert(name);
   if (consumed.isEmpty()) return consumed;
   auto values=epochIntervalSettings(object);
   // A format-only edit must preserve both physical endpoints. Explicit
   // pending endpoints are already expressed in the selected new format.
   values=convertEpochInterval(values,changes.value("EpochFormat",values.value("EpochFormat")));
   for (const auto &name:consumed) values[name]=changes.value(name).trimmed();
   if (epoch(values["EpochFormat"],values["InitialEpoch"])>epoch(values["EpochFormat"],values["FinalEpoch"])) throw std::runtime_error("Initial epoch must not follow final epoch.");
   for (const auto &name:QStringList{"EpochFormat","InitialEpoch","FinalEpoch"}) object.SetStringParameter(name.toStdString(),values.value(name).toStdString());
   return consumed;
}
QString convertWarmStartEpoch(const QString &value,const QString &from,const QString &to)
{
   if (value=="FirstMeasurement" || value=="LastWarmStartRecord") return value;
   return convertEpochInterval({{"EpochFormat",from},{"InitialEpoch",value},{"FinalEpoch",value}},to).value("InitialEpoch");
}
QSet<QString> applyWarmStartProperties(GmatBase &object,const QMap<QString,QString> &changes)
{
   QSet<QString> consumed;
   if (!object.IsOfType("SeqEstimator")) return consumed;
   for (const auto &name:QStringList{"WarmStartEpochFormat","WarmStartEpoch"}) if (changes.contains(name)) consumed.insert(name);
   if (consumed.isEmpty()) return consumed;
   const auto from=QString::fromStdString(object.GetStringParameter("WarmStartEpochFormat")),to=changes.value("WarmStartEpochFormat",from);
   const auto value=changes.contains("WarmStartEpoch") ? changes.value("WarmStartEpoch").trimmed() : convertWarmStartEpoch(QString::fromStdString(object.GetStringParameter("WarmStartEpoch")),from,to);
   // Validate the epoch in its pending format rather than QMap's alphabetical
   // order, which would assign the new epoch in the old format.
   object.SetStringParameter("WarmStartEpochFormat",to.toStdString());
   object.SetStringParameter("WarmStartEpoch",value.toStdString());
   return consumed;
}
QString convertEphemerisEpoch(const QString &value,const QString &from,const QString &to)
{
   const auto &formats=TimeSystemConverter::Instance()->GetValidTimeRepresentations();
   const auto valid=[&](const QString &format) { return std::find(formats.begin(),formats.end(),format.toStdString())!=formats.end(); };
   if (!valid(from) || !valid(to)) throw std::runtime_error("Select a supported ephemeris epoch format.");
   const auto input=value.trimmed();
   if (input=="FromSpacecraft" || input=="EphemStart") return input;
   epoch(from,input); // Validate even a same-format or paired transaction now.
   return convertEpochInterval({{"EpochFormat",from},{"InitialEpoch",input},{"FinalEpoch",input}},to).value("InitialEpoch");
}
EpochIntervalDialog::EpochIntervalDialog(GmatBase &object,const QMap<QString,QString> &pending,QWidget *parent) : QDialog(parent)
{
   setObjectName("epochIntervalDialog"); setWindowTitle("Time interval — "+QString::fromStdString(object.GetName()));
   auto values=epochIntervalSettings(object); for (auto it=pending.cbegin();it!=pending.cend();++it) if (values.contains(it.key())) values[it.key()]=it.value();
   auto *layout=new QVBoxLayout(this); auto *help=new QLabel("Changing the epoch format converts both endpoints. Gregorian dates use milliseconds; numeric formats retain full precision. OK keeps changes pending until resource Apply.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *form=new QFormLayout; layout->addLayout(form); format=new QComboBox(this); format->setObjectName("intervalEpochFormat");
   for (const auto &name:TimeSystemConverter::Instance()->GetValidTimeRepresentations()) format->addItem(QString::fromStdString(name));
   format->setCurrentText(values["EpochFormat"]); previousFormat=values["EpochFormat"]; form->addRow("Epoch format",format);
   start=new QLineEdit(values["InitialEpoch"],this); start->setObjectName("intervalInitialEpoch"); form->addRow("Initial epoch",start);
   finish=new QLineEdit(values["FinalEpoch"],this); finish->setObjectName("intervalFinalEpoch"); form->addRow("Final epoch",finish);
   error=new QLabel(this); error->setObjectName("epochIntervalError"); error->setWordWrap(true); layout->addWidget(error);
   connect(format,&QComboBox::currentTextChanged,this,[this](const QString &next) {
      try {
         auto values=settings(); values["EpochFormat"]=previousFormat; const auto converted=convertEpochInterval(values,next);
         start->setText(converted["InitialEpoch"]); finish->setText(converted["FinalEpoch"]); previousFormat=next; error->clear();
      } catch (BaseException &failure) { const QSignalBlocker blocker(format); format->setCurrentText(previousFormat); error->setText(QString::fromStdString(failure.GetFullMessage())); }
      catch (const std::exception &failure) { const QSignalBlocker blocker(format); format->setCurrentText(previousFormat); error->setText(QString::fromUtf8(failure.what())); }
   });
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,&object] {
      try { std::unique_ptr<GmatBase> copy(object.Clone()); applyEpochIntervalProperties(*copy,settings()); accept(); }
      catch (BaseException &failure) { error->setText(QString::fromStdString(failure.GetFullMessage())); }
      catch (const std::exception &failure) { error->setText(QString::fromUtf8(failure.what())); }
   });
   resize(570,240);
}
QMap<QString,QString> EpochIntervalDialog::settings() const { return {{"EpochFormat",format->currentText()},{"InitialEpoch",start->text()},{"FinalEpoch",finish->text()}}; }
