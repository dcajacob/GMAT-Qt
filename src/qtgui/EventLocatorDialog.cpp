#include "EventLocatorDialog.hpp"
#include "ResourceProperties.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "TimeSystemConverter.hpp"
#include "Moderator.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QScreen>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
double epochMjd(const QString &format,const QString &value)
{
   Real mjd; std::string converted;
   TimeSystemConverter::Instance()->Convert(format.toStdString(),-999.999,value.trimmed().toStdString(),"TAIModJulian",mjd,converted);
   if (!std::isfinite(mjd)) throw std::runtime_error("Enter a valid finite epoch.");
   return mjd;
}
}

QSet<QString> applyEventLocatorProperties(GmatBase &object,const QMap<QString,QString> &values,bool creating)
{
   QSet<QString> consumed;
   if (!object.IsOfType("EventLocator")) return consumed;
   const auto initialProperties=resourceProperties(object);
   for (const auto &name:QStringList{"Spacecraft","Target"}) if (values.contains(name)) {
      auto property=std::find_if(initialProperties.cbegin(),initialProperties.cend(),[&](const auto &property) { return property.name==name; });
      if (property==initialProperties.cend() || !property->references.contains(values.value(name))) throw std::runtime_error(("Select an available "+name+".").toStdString());
      setResourceProperty(object,name,values.value(name)); consumed.insert(name);
   }
   // Epoch setters interpret dates in the current input format. Set the
   // format before either endpoint, regardless of map/property ordering.
   for (const auto &name:QStringList{"InputEpochFormat","InitialEpoch","FinalEpoch"}) if (values.contains(name)) {
      setResourceProperty(object,name,values.value(name)); consumed.insert(name);
   }
   const auto properties=resourceProperties(object);
   for (const auto &property:properties) {
      const auto name=property.name; if (!values.contains(name) || consumed.contains(name)) continue;
      const auto value=values.value(name);
      if (property.list) {
         const auto selected=splitResourceReferences(value); QSet<QString> unique;
         if (name=="EventTypes" && selected.isEmpty()) throw std::runtime_error("Select at least one event type.");
         for (const auto &entry:selected) {
            if (!property.references.contains(entry)) throw std::runtime_error(("Select an available "+name+" entry: "+entry).toStdString());
            if (unique.contains(entry)) throw std::runtime_error("Select each entry only once.");
            unique.insert(entry);
         }
         if (!object.TakeAction("Clear",name.toStdString())) throw std::runtime_error("This locator cannot replace "+name.toStdString()+".");
         for (const auto &entry:selected) if (!object.SetStringParameter(name.toStdString(),entry.toStdString()))
            throw std::runtime_error(("The locator rejected "+name+" entry: "+entry).toStdString());
      } else {
         const auto id=object.GetParameterID(name.toStdString());
         if (object.GetParameterType(id)==Gmat::OBJECT_TYPE && !property.references.contains(value))
            throw std::runtime_error(("Select an available "+name+".").toStdString());
         setResourceProperty(object,name,value);
      }
      consumed.insert(name);
   }
   if (object.IsOfType("ContactLocator") && !values.isEmpty()) {
      auto *target=Moderator::Instance()->GetConfiguredObject(object.GetStringParameter("Target"));
      const bool regionTarget=target && target->IsOfType(Gmat::REGION);
      const auto observers=object.GetStringArrayParameter("Observers");
      const auto types=object.GetStringArrayParameter("EventTypes");
      const auto additional=object.GetStringArrayParameter("AdditionalTargets");
      if (!additional.empty() && (!target || !target->IsOfType("Spacecraft")))
         throw std::runtime_error("Additional spacecraft require a spacecraft primary Target.");
      for (const auto &name:additional) {
         auto *selected=Moderator::Instance()->GetConfiguredObject(name);
         if (!selected || !selected->IsOfType("Spacecraft")) throw std::runtime_error("Select an available additional spacecraft.");
         if (selected==target) throw std::runtime_error("The primary Target cannot also be an additional spacecraft.");
         if (std::find(observers.begin(),observers.end(),name)!=observers.end())
            throw std::runtime_error("A selected target cannot also be an observer.");
      }
      if (std::find(observers.begin(),observers.end(),object.GetStringParameter("Target"))!=observers.end())
         throw std::runtime_error("A selected target cannot also be an observer.");
      const bool stationSelection=std::find(types.begin(),types.end(),"StationRise")!=types.end() || std::find(types.begin(),types.end(),"StationSet")!=types.end();
      if (stationSelection && (!target || !target->IsOfType("Spacecraft"))) throw std::runtime_error("Station rise/set events require a spacecraft target.");
      // Creation may precede the spacecraft/observer configuration. Keep
      // explicit observer edits and edits of existing locators strict;
      // FindEvents initialization remains the engine's responsibility.
      const bool pendingObservers=creating && observers.empty() && !values.contains("Observers");
      if (regionTarget && observers.size()!=1 && !pendingObservers) throw std::runtime_error("Select exactly one spacecraft observer for a planetographic region.");
      for (const auto &name:observers) {
         auto *observer=Moderator::Instance()->GetConfiguredObject(name);
         if (stationSelection && (!observer || !observer->IsOfType("GroundStation"))) throw std::runtime_error("Station rise/set events require ground-station observers.");
         if (!observer || (regionTarget ? !observer->IsOfType("Spacecraft") : (!observer->IsOfType("GroundStation") && !observer->IsOfType("Spacecraft"))))
            throw std::runtime_error(regionTarget ? "A planetographic region requires one spacecraft observer." : "Select an available ground station or spacecraft observer.");
      }
   }
   if (!object.GetBooleanParameter("UseEntireInterval")) {
      const auto format=QString::fromStdString(object.GetStringParameter("InputEpochFormat"));
      const auto start=epochMjd(format,QString::fromStdString(object.GetStringParameter("InitialEpoch")));
      const auto end=epochMjd(format,QString::fromStdString(object.GetStringParameter("FinalEpoch")));
      if (start>end) throw std::runtime_error("Initial epoch must not follow final epoch.");
   }
   if (!object.GetBooleanParameter("UseLightTimeDelay") && object.GetBooleanParameter("UseStellarAberration"))
      throw std::runtime_error("Stellar aberration requires light-time delay.");
   if (object.IsOfType("ContactLocator") && QString::fromStdString(object.GetStringParameter("ReportFormat")).startsWith("Azimuth") && object.GetRealParameter("IntervalStepSize")<=0)
      throw std::runtime_error("Choose a positive report interval step for an azimuth/elevation report.");
   if (object.IsOfType("IntrusionLocator") && object.GetStringParameter("ReportCoordinates")=="FixedGrid") {
      const QFileInfo file(QString::fromStdString(object.GetStringParameter("SpiceGridFrameFile")));
      if (!file.isFile() || !file.isReadable()) throw std::runtime_error("Choose a readable SPICE grid-frame input file.");
   }
   return consumed;
}

EventLocatorDialog::EventLocatorDialog(GmatBase &object,const QMap<QString,QString> &pending,QWidget *parent)
   : QDialog(parent),original(object.Clone())
{
   setObjectName("eventLocatorDialog"); setWindowTitle(QString::fromStdString(object.GetTypeName())+" settings");
   auto *layout=new QVBoxLayout(this); auto *help=new QLabel("OK keeps changes pending. Apply the locator to update the mission.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *scroll=new QScrollArea(this); scroll->setObjectName("eventLocatorScroll"); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
   auto *content=new QWidget(scroll); auto *groups=new QVBoxLayout(content); groups->setContentsMargins(0,0,0,0); scroll->setWidget(content); layout->addWidget(scroll,1);
   std::unique_ptr<GmatBase> preview(original->Clone()); QString initialError;
   try { applyEventLocatorProperties(*preview,pending); }
   catch (BaseException &failure) { initialError=QString::fromStdString(failure.GetFullMessage()); }
   catch (const std::exception &failure) { initialError=QString::fromUtf8(failure.what()); }
   QMap<QString,ResourceProperty> properties; for (const auto &property:resourceProperties(*preview)) properties.insert(property.name,property);
   auto group=[&](const QString &title) { auto *box=new QGroupBox(title,content); groups->addWidget(box); return new QFormLayout(box); };
   auto field=[&](QFormLayout *form,const QString &name,const QString &label) {
      if (!properties.contains(name)) return;
      const auto property=properties.value(name); const auto value=pending.value(name,property.value);
      const QMap<QString,QString> labels={{"OccultingBodies","Occulting bodies"},{"IntrudingBodies","Intruding bodies"},{"CentralBody","Central body"},{"EclipseTypes","Eclipse types"},{"RunMode","Run mode"},{"WriteReport","Write report"},{"Filename","Report file"},{"ReportFormat","Report format"},{"ReportTimeFormat","Time format"},{"ReportPrecision","Precision"},{"LeftJustified","Left justify"},{"IntervalStepSize","Report interval step"},{"MinimumPhase","Minimum phase"},{"ReportCoordinates","Report coordinates"},{"SpiceGridFrameFile","SPICE grid-frame file"}};
      const auto title=labels.value(label,label);
      if (property.list) {
         auto *list=new QListWidget(content); list->setObjectName("event_"+name); list->setMaximumHeight(130); list->setMinimumHeight(90); lists.insert(name,list);
         const auto selected=splitResourceReferences(value); auto candidates=property.references;
         for (const auto &entry:selected) if (!candidates.contains(entry)) candidates.append(entry);
         for (const auto &entry:candidates) { auto *item=new QListWidgetItem(entry,list); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(selected.contains(entry) ? Qt::Checked : Qt::Unchecked); }
         form->addRow(title,list);
      } else if (property.choices==QStringList{"true","false"}) {
         auto *check=new QCheckBox(title,content); check->setObjectName("event_"+name); check->setChecked(value=="true"); checks.insert(name,check); form->addRow(check);
      } else if (!property.choices.isEmpty() || !property.references.isEmpty()) {
         auto *combo=new QComboBox(content); combo->setObjectName("event_"+name); combo->addItems(property.choices.isEmpty() ? property.references : property.choices);
         if (combo->findText(value)<0) combo->addItem(value); combo->setCurrentText(value); choices.insert(name,combo); form->addRow(title,combo);
      } else {
         auto *row=new QWidget(content); auto *line=new QHBoxLayout(row); line->setContentsMargins(0,0,0,0);
         auto *edit=new QLineEdit(value,row); edit->setObjectName("event_"+name); text.insert(name,edit); line->addWidget(edit);
         const auto unit=property.unit.isEmpty() && (name=="StepSize" || name=="IntervalStepSize") ? QString("s") : property.unit;
         if (!unit.isEmpty()) line->addWidget(new QLabel(unit,row));
         if (property.filename) {
            auto *browse=new QPushButton("Browse…",row); browse->setObjectName("eventBrowse_"+name); line->addWidget(browse);
            connect(browse,&QPushButton::clicked,this,[this,edit,name,property] {
               QFileDialog picker(this,property.fileInput ? "Choose locator input file" : "Choose event report",edit->text()); picker.setObjectName("eventFileDialog");
               picker.setFileMode(property.fileInput ? QFileDialog::ExistingFile : QFileDialog::AnyFile); picker.setAcceptMode(property.fileInput ? QFileDialog::AcceptOpen : QFileDialog::AcceptSave);
               if (picker.exec()==QDialog::Accepted && !picker.selectedFiles().isEmpty()) edit->setText(picker.selectedFiles().first());
            });
         }
         form->addRow(title,row);
      }
   };
   auto *selection=group("Objects");
   for (const auto &name:QStringList{"Spacecraft","Target","AdditionalTargets","CentralBody","OccultingBodies","IntrudingBodies","Observers","EclipseTypes","EventTypes","Sensors"}) field(selection,name,name=="AdditionalTargets" ? "Additional spacecraft" : name);
   auto *interval=group("Search interval");
   field(interval,"UseEntireInterval","Use entire propagated interval"); field(interval,"InputEpochFormat","Epoch format"); field(interval,"InitialEpoch","Initial epoch"); field(interval,"FinalEpoch","Final epoch"); field(interval,"StepSize","Search step");
   auto *corrections=group("Light-time corrections");
   field(corrections,"UseLightTimeDelay","Use light-time delay"); field(corrections,"UseStellarAberration","Use stellar aberration"); field(corrections,"LightTimeDirection","Light-time direction");
   auto *report=group("Report and run mode");
   for (const auto &name:QStringList{"RunMode","WriteReport","Filename","ReportFormat","ReportTimeFormat","ReportPrecision","LeftJustified","IntervalStepSize","MinimumPhase","ReportCoordinates","SpiceGridFrameFile"}) field(report,name,name);
   groups->addStretch();
   error=new QLabel(this); error->setObjectName("eventLocatorError"); error->setWordWrap(true); layout->addWidget(error);
   error->setText(initialError);
   for (auto *edit:text) connect(edit,&QLineEdit::textChanged,error,&QLabel::clear);
   previousFormat=choices.value("InputEpochFormat")->currentText();
   connect(choices.value("InputEpochFormat"),&QComboBox::currentTextChanged,this,[this] { convertEpochs(); });
   for (const auto &name:QStringList{"UseEntireInterval","UseLightTimeDelay"}) if (auto *check=checks.value(name)) connect(check,&QCheckBox::toggled,this,[this,name](bool enabled) {
      if (name=="UseLightTimeDelay" && !enabled) checks.value("UseStellarAberration")->setChecked(false);
      updateDependencies();
   });
   if (auto *combo=choices.value("ReportCoordinates")) connect(combo,&QComboBox::currentTextChanged,this,[this] { updateDependencies(); });
   if (auto *combo=choices.value("ReportFormat")) connect(combo,&QComboBox::currentTextChanged,this,[this] { updateDependencies(); });
   if (object.IsOfType("IntrusionLocator")) connect(choices.value("Spacecraft"),&QComboBox::currentTextChanged,this,[this](const QString &target) {
      auto *list=lists.value("IntrudingBodies"); QStringList selected;
      for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) selected.append(list->item(i)->text());
      list->clear();
      for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SPACE_POINT)) {
         const auto entry=QString::fromStdString(name); if (entry==target) continue;
         auto *item=new QListWidgetItem(entry,list); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(selected.contains(entry) ? Qt::Checked : Qt::Unchecked);
      }
   });
   if (object.IsOfType("ContactLocator")) {
      auto updateObservers=[this] {
         const auto target=choices.value("Target")->currentText();
         auto *list=lists.value("Observers"); QStringList selected,targets{target};
         for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) selected.append(list->item(i)->text());
         auto *additional=lists.value("AdditionalTargets");
         for (int i=0;i<additional->count();++i) if (additional->item(i)->checkState()==Qt::Checked) targets.append(additional->item(i)->text());
         list->clear();
         auto *configuredTarget=Moderator::Instance()->GetConfiguredObject(target.toStdString());
         const bool regionTarget=configuredTarget && configuredTarget->IsOfType(Gmat::REGION);
         for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SPACE_POINT)) {
            const auto entry=QString::fromStdString(name); auto *observer=Moderator::Instance()->GetConfiguredObject(name);
            if (targets.contains(entry) || !observer || (regionTarget ? !observer->IsOfType("Spacecraft") : (!observer->IsOfType("GroundStation") && !observer->IsOfType("Spacecraft")))) continue;
            auto *item=new QListWidgetItem(entry,list); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(selected.contains(entry) ? Qt::Checked : Qt::Unchecked);
         }
      };
      connect(lists.value("AdditionalTargets"),&QListWidget::itemChanged,this,[updateObservers](QListWidgetItem*) { updateObservers(); });
      connect(choices.value("Target"),&QComboBox::currentTextChanged,this,[this,updateObservers](const QString &target) {
         auto *list=lists.value("AdditionalTargets"); const QSignalBlocker block(list); QStringList selected;
         for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) selected.append(list->item(i)->text());
         list->clear();
         for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SPACECRAFT)) {
            const auto entry=QString::fromStdString(name); if (entry==target) continue;
            auto *item=new QListWidgetItem(entry,list); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(selected.contains(entry) ? Qt::Checked : Qt::Unchecked);
         }
         updateObservers();
      });
   }
   updateDependencies();
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this] {
      try { auto proposed=settings(); std::unique_ptr<GmatBase> preview(original->Clone()); applyEventLocatorProperties(*preview,proposed); acceptedValues=proposed; accept(); }
      catch (BaseException &failure) { error->setText(QString::fromStdString(failure.GetFullMessage())); }
      catch (const std::exception &failure) { error->setText(QString::fromUtf8(failure.what())); }
   });
   const auto available=screen()->availableGeometry(); resize(std::min(800,available.width()-40),std::min(820,available.height()-80));
}
EventLocatorDialog::~EventLocatorDialog()=default;
QMap<QString,QString> EventLocatorDialog::settings() const
{
   QMap<QString,QString> result;
   for (auto it=text.cbegin();it!=text.cend();++it) result.insert(it.key(),it.value()->text());
   for (auto it=choices.cbegin();it!=choices.cend();++it) result.insert(it.key(),it.value()->currentText());
   for (auto it=checks.cbegin();it!=checks.cend();++it) result.insert(it.key(),it.value()->isChecked() ? "true" : "false");
   for (auto it=lists.cbegin();it!=lists.cend();++it) { QStringList selected; for (int row=0;row<it.value()->count();++row) if (it.value()->item(row)->checkState()==Qt::Checked) selected.append(it.value()->item(row)->text()); result.insert(it.key(),selected.join(", ")); }
   return result;
}
void EventLocatorDialog::updateDependencies()
{
   const bool entire=checks.value("UseEntireInterval")->isChecked(),lightTime=checks.value("UseLightTimeDelay")->isChecked();
   choices.value("InputEpochFormat")->setEnabled(!entire); text.value("InitialEpoch")->setEnabled(!entire); text.value("FinalEpoch")->setEnabled(!entire);
   checks.value("UseStellarAberration")->setEnabled(lightTime); if (auto *direction=choices.value("LightTimeDirection")) direction->setEnabled(lightTime);
   if (auto *report=choices.value("ReportFormat")) text.value("IntervalStepSize")->setEnabled(report->currentText().startsWith("Azimuth"));
   if (auto *coordinates=choices.value("ReportCoordinates")) {
      const bool fixed=coordinates->currentText()=="FixedGrid"; text.value("SpiceGridFrameFile")->setEnabled(fixed); findChild<QPushButton *>("eventBrowse_SpiceGridFrameFile")->setEnabled(fixed);
   }
}
void EventLocatorDialog::convertEpochs()
{
   auto *format=choices.value("InputEpochFormat"); const auto next=format->currentText();
   try {
      QStringList converted;
      for (const auto &name:QStringList{"InitialEpoch","FinalEpoch"}) {
         Real mjd; std::string value; TimeSystemConverter::Instance()->Convert(previousFormat.toStdString(),-999.999,text.value(name)->text().trimmed().toStdString(),next.toStdString(),mjd,value); converted.append(QString::fromStdString(value));
      }
      text.value("InitialEpoch")->setText(converted[0]); text.value("FinalEpoch")->setText(converted[1]); previousFormat=next; error->clear();
   } catch (BaseException &failure) { const QSignalBlocker blocker(format); format->setCurrentText(previousFormat); error->setText(QString::fromStdString(failure.GetFullMessage())); }
}
