#include "PropagatorSettings.hpp"
#include "ResourceProperties.hpp"
#include "ResourcePreview.hpp"
#include "EpochIntervalDialog.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "FactoryManager.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <stdexcept>

PropagatorSettings::PropagatorSettings(PropSetup &setup,QWidget *parent) : QWidget(parent)
{
   setObjectName("propagatorSettings");form=new QFormLayout(this);form->setContentsMargins(0,0,0,0);
   type=new QComboBox(this);type->setObjectName("resource_Type");
   for (const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::PROPAGATOR)) type->addItem(QString::fromStdString(name));
   form->addRow("Type",type);forceRow=new QWidget(this);auto *forceLayout=new QHBoxLayout(forceRow);forceLayout->setContentsMargins(0,0,0,0);
   forceModel=new QLineEdit(QString::fromStdString(setup.GetStringParameter("FM")),forceRow);forceModel->setObjectName("resource_FM");forceLayout->addWidget(forceModel);
   auto *choose=new QPushButton("Select…",forceRow);choose->setObjectName("propagatorForceModelChoose");forceLayout->addWidget(choose);form->addRow("FM",forceRow);
   connect(choose,&QPushButton::clicked,this,[this] {
      QDialog dialog(this);dialog.setObjectName("propagatorForceModelDialog");dialog.setWindowTitle("Select force model");auto *layout=new QVBoxLayout(&dialog);auto *list=new QListWidget(&dialog);list->setObjectName("propagatorForceModelList");
      for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::ODE_MODEL)) list->addItem(QString::fromStdString(name));
      const auto matches=list->findItems(forceModel->text(),Qt::MatchExactly);if (!matches.isEmpty()) list->setCurrentItem(matches.front());layout->addWidget(list);auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);
      buttons->button(QDialogButtonBox::Ok)->setEnabled(list->currentItem());connect(list,&QListWidget::currentItemChanged,&dialog,[buttons](QListWidgetItem *item) {buttons->button(QDialogButtonBox::Ok)->setEnabled(item);});connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
      if (dialog.exec()==QDialog::Accepted && list->currentItem()) forceModel->setText(list->currentItem()->text());
   });
   auto *propagator=setup.GetPropagator();if (!propagator) throw std::runtime_error("A propagator draft needs an owned propagator.");
   activeType=QString::fromStdString(propagator->GetTypeName());type->setCurrentText(activeType);populate(*propagator,{});original=values();originalFields={"Type","FM"};for (const auto &field:resourceProperties(*propagator)) originalFields.insert(field.name);
   connect(type,&QComboBox::currentTextChanged,this,[this](const QString &value) {selectType(value);});
   connect(forceModel,&QLineEdit::textChanged,this,[this](const QString &value) {changed("FM",value);});
}
QMap<QString,QString> PropagatorSettings::values() const
{
   QMap<QString,QString> result{{"Type",activeType}};if (numeric) result["FM"]=forceModel->text();
   for (auto it=editors.cbegin();it!=editors.cend();++it) {
      if (activeType=="SPK" && it.key()=="CentralBody") continue;
      if (auto *choice=qobject_cast<QComboBox *>(it.value())) result[it.key()]=choice->currentText();
      else if (auto *line=qobject_cast<QLineEdit *>(it.value())) result[it.key()]=line->text();
   }return result;
}
QMap<QString,QString> PropagatorSettings::changes() const
{
   auto result=values();const bool switched=activeType!=original.value("Type");
   for (auto it=result.begin();it!=result.end();) {
      // The backend preserves compatible numeric/boolean fields across Type.
      // String settings belong to the selected prototype and must accompany a
      // switch even if their spelling happens to match the previous type.
      const auto kind=fieldTypes.value(it.key(),-1);const bool stringOnSwitch=switched && (kind==Gmat::STRING_TYPE || kind==Gmat::ENUMERATION_TYPE);
      if (original.contains(it.key()) && it.value()==original.value(it.key()) && !stringOnSwitch) it=result.erase(it);else ++it;
   }return result;
}
void PropagatorSettings::changed(const QString &name,const QString &value)
{
   if (!updating && onValueChanged) onValueChanged(name,value);
}
void PropagatorSettings::setValue(const QString &name,const QString &value)
{
   if (updating) return;
   if (name=="Type") {type->setCurrentText(value);return;}
   if (name=="FM") {forceModel->setText(value);return;}
   if (auto *choice=qobject_cast<QComboBox *>(editors.value(name))) choice->setCurrentText(value);
   else if (auto *line=qobject_cast<QLineEdit *>(editors.value(name))) line->setText(value);
}
void PropagatorSettings::populate(Propagator &propagator,const QMap<QString,QString> &pending)
{
   if (owned) {form->removeRow(owned);owned=nullptr;}
   editors.clear();fieldTypes.clear();numeric=propagator.UsesODEModel();forceRow->setEnabled(numeric);forceRow->setToolTip(numeric ? QString() : "SPK/ephemeris propagation does not use a force model.");
   owned=new QWidget(this);auto *fields=new QFormLayout(owned);fields->setContentsMargins(0,0,0,0);form->addRow(owned);
   for (const auto &field:resourceProperties(propagator)) {
      const auto value=pending.value(field.name,field.value);QWidget *input=nullptr;
      fieldTypes[field.name]=propagator.GetParameterType(propagator.GetParameterID(field.name.toStdString()));
      if (!field.choices.isEmpty()) {
         auto *choice=new QComboBox(owned);choice->addItems(field.choices);if (field.name=="StartEpoch") {choice->setEditable(true);choice->setInsertPolicy(QComboBox::NoInsert);}
         if (choice->findText(value)<0) choice->addItem(value);choice->setCurrentText(value);input=choice;
         connect(choice,&QComboBox::currentTextChanged,this,[this,name=field.name](const QString &text) {changed(name,text);});
      }else {
         auto *line=new QLineEdit(value,owned);input=line;
         if (activeType=="SPK" && field.name=="CentralBody") {line->setReadOnly(true);line->setToolTip("The current SPK engine ignores CentralBody. Configurable Origin remains an unresolved historical requirement.");}
         connect(line,&QLineEdit::textChanged,this,[this,name=field.name](const QString &text) {changed(name,text);});
      }
      input->setObjectName("resource_"+field.name);if (input->toolTip().isEmpty()) input->setToolTip(field.help);editors[field.name]=input;fields->addRow(field.name+(field.unit.isEmpty() ? QString() : " ("+field.unit+")"),input);
   }
   auto *format=qobject_cast<QComboBox *>(editors.value("EpochFormat"));auto *epoch=qobject_cast<QComboBox *>(editors.value("StartEpoch"));
   if (format && epoch) {
      auto previous=std::make_shared<QString>(format->currentText());auto converting=std::make_shared<bool>(false);
      connect(format,&QComboBox::currentTextChanged,this,[this,format,epoch,previous,converting](const QString &next) {
         if (*converting) return;*converting=true;
         try {epoch->setCurrentText(convertEphemerisEpoch(epoch->currentText(),*previous,next));*previous=next;if (onError) onError({});}
         catch (BaseException &error) {format->setCurrentText(*previous);if (onError) onError(QString::fromStdString(error.GetFullMessage()));}
         catch (const std::exception &error) {format->setCurrentText(*previous);if (onError) onError(QString::fromUtf8(error.what()));}
         *converting=false;
      });
   }
}
void PropagatorSettings::selectType(const QString &selected)
{
   if (selected==activeType) return;
   try {
      QtResourcePreview prototype(FactoryManager::Instance()->CreatePropagator(selected.toStdString(),""));auto *propagator=dynamic_cast<Propagator *>(prototype.get());if (!propagator) throw std::runtime_error("The selected propagator could not be prepared.");
      const auto previous=values();cached[activeType]=previous;QMap<QString,QString> pending=cached.value(selected);
      if (pending.isEmpty()) for (const auto &field:resourceProperties(*propagator)) {
         const auto kind=propagator->GetParameterType(propagator->GetParameterID(field.name.toStdString()));
         if (previous.contains(field.name) && fieldTypes.value(field.name,-1)==kind && (kind==Gmat::REAL_TYPE || kind==Gmat::INTEGER_TYPE || kind==Gmat::UNSIGNED_INT_TYPE || kind==Gmat::BOOLEAN_TYPE)) pending[field.name]=previous.value(field.name);
      }
      activeType=selected;populate(*propagator,pending);changed("Type",selected);if (onError) onError({});
   }catch (BaseException &error) {const QSignalBlocker block(type);type->setCurrentText(activeType);if (onError) onError(QString::fromStdString(error.GetFullMessage()));}
   catch (const std::exception &error) {const QSignalBlocker block(type);type->setCurrentText(activeType);if (onError) onError(QString::fromUtf8(error.what()));}
}
