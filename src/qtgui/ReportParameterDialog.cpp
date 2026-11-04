#include "ReportParameterDialog.hpp"
#include "ParameterReferences.hpp"
#include "BulkPropertyDialog.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "Parameter.hpp"
#include "ParameterInfo.hpp"
#include "BaseException.hpp"
#include <QFormLayout>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <QGroupBox>
#include <QMap>
#include "Array.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QListWidget>
#include <QDialogButtonBox>

ReportParameterDialog::ReportParameterDialog(const QStringList &selected,QWidget *parent,Mode mode) : QDialog(parent)
{
   const bool arguments=mode==Mode::FunctionInputs || mode==Mode::FunctionOutputs;
   const bool numeric=mode==Mode::NumericSingle || mode==Mode::NumericReference;
   const bool literal=mode==Mode::NumericSingle;
   const bool plottable=mode==Mode::PlottableSingle || mode==Mode::PlottableMultiple || numeric;
   const bool single=mode!=Mode::Multiple && mode!=Mode::PlottableMultiple && !arguments;
   const bool writable=mode==Mode::Writable || mode==Mode::WritableReal || mode==Mode::FunctionOutputs;
   const bool realOnly=mode==Mode::WritableReal;
   const bool stop=mode==Mode::StopParameter;
   auto writableParameter=[realOnly](const Parameter *parameter) {
      const bool user=parameter->IsOfType("Variable") || parameter->IsOfType("Array") || parameter->IsOfType("String");
      return (user || parameter->IsSettable()) && (!realOnly || parameter->GetReturnType()==Gmat::REAL_TYPE || parameter->IsOfType("Array"));
   };
   setObjectName("reportParameterDialog"); setWindowTitle(numeric ? "Select numeric value" : plottable ? "Plot parameters" : arguments ? (writable ? "Function outputs" : "Function inputs") : single ? "Select parameter" : "Report parameters"); resize(600,single ? 350 : 450);
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Choose a configured parameter, or enter a reference such as Sat.EarthMJ2000Eq.X. Apply validates references.",this);
   help->setWordWrap(true); layout->addWidget(help);
   if (plottable) help->setText("Choose a numeric plot parameter or an array element. Browse object properties and their reference frames below. Apply validates the complete plot.");
   if (numeric) help->setText(literal ? "Choose a Variable, array element or numeric object property, or enter a finite real number. Apply validates the complete command." : "Choose a Variable, array element or numeric object property. Apply validates the complete command.");
   auto *entry=new QComboBox(this); entry->setObjectName("reportParameterEntry");
   if (single) singleEntry=entry;
   entry->setEditable(true); entry->setInsertPolicy(QComboBox::NoInsert);
   auto *moderator=Moderator::Instance();
   QStringList names;
   for (const auto &name:moderator->GetListOfObjects(Gmat::PARAMETER)) {
      auto *parameter=moderator->GetParameter(name);
      if (parameter && (plottable ? (parameter->IsPlottable() || parameter->IsOfType("Array")) : writable ? writableParameter(parameter) : (parameter->IsReportable() || (stop && (parameter->GetTypeName()=="Periapsis" || parameter->GetTypeName()=="Apoapsis"))))) names.append(QString::fromStdString(name));
   }
   if (arguments) {
      help->setText("Arguments are positional. Add, remove or reorder entries below; Apply validates the function signature and references.");
      for (const auto &name:moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT)) {
         auto *object=moderator->GetConfiguredObject(name);
         if (object && (object->IsOfType(Gmat::SPACE_POINT) || object->IsOfType("ImpulsiveBurn") || object->IsOfType("String"))) names.append(QString::fromStdString(name));
      }
   }
   names.sort(); names.removeDuplicates(); entry->addItems(names); layout->addWidget(entry);
   auto *browser=new QGroupBox("Browse object properties",this);
   auto *form=new QFormLayout(browser);
   auto *owner=new QComboBox(browser); owner->setObjectName("reportPropertyObject");
   auto *property=new QComboBox(browser); property->setObjectName("reportPropertyType");
   auto *dependency=new QComboBox(browser); dependency->setObjectName("reportPropertyDependency");
   auto *dependencyLabel=new QLabel("Reference",browser);
   auto *use=new QPushButton("Use reference",browser); use->setObjectName("reportUseReference");
   form->addRow("Object",owner); form->addRow("Property",property); form->addRow(dependencyLabel,dependency); form->addRow(use);
   layout->addWidget(browser);
   auto *info=ParameterInfo::Instance();
   QMap<QString,QStringList> properties;
   for (const auto &type:info->GetTypesOfParameters()) {
      if (plottable ? !info->IsPlottable(type) : writable ? (!info->IsSettable(type) || (realOnly && !info->IsPlottable(type))) : (!info->IsReportable(type) && !(stop && (type=="Periapsis" || type=="Apoapsis")))) continue;
      const auto ownerType=info->GetObjectType(type);
      if (ownerType==Gmat::UNKNOWN_OBJECT || ownerType==Gmat::PARAMETER) continue;
      for (const auto &name:moderator->GetListOfObjects(ownerType)) properties[QString::fromStdString(name)].append(QString::fromStdString(type));
   }
   owner->addItems(properties.keys());
   auto updateDependency=[property,dependency,dependencyLabel,use,owner,writable,moderator] {
      auto dep=parameterDependency(owner->currentText(),property->currentText());
      // Burn components can be read in other frames, but native writes use
      // the burn's own configured axes. An external frame would reject a
      // Local VNB/LVLH write at execution rather than set its components.
      auto *object=moderator->GetConfiguredObject(owner->currentText().toStdString());
      if (writable && object && object->IsOfType("ImpulsiveBurn") &&
          QStringList{"Element1","Element2","Element3","V","N","B"}.contains(property->currentText()))
         dep.needed=false;
      dependency->clear(); dependency->addItems(dep.choices);
      dependencyLabel->setText(dep.label);
      dependency->setVisible(dep.needed); dependencyLabel->setVisible(dep.needed);
      if (dependency->findText(dep.preferred)>=0) dependency->setCurrentText(dep.preferred);
      use->setEnabled(!owner->currentText().isEmpty() && !property->currentText().isEmpty() && (!dep.needed || dependency->count()>0));
   };
   auto updateProperties=[owner,property,properties,updateDependency] {
      property->clear(); auto choices=properties.value(owner->currentText()); choices.removeDuplicates(); choices.sort();
      property->addItems(choices); updateDependency();
   };
   connect(owner,&QComboBox::currentTextChanged,this,[updateProperties] { updateProperties(); });
   connect(property,&QComboBox::currentTextChanged,this,[updateDependency] { updateDependency(); });
   connect(use,&QPushButton::clicked,this,[owner,property,dependency,entry] {
      QStringList parts{owner->currentText()};
      if (!dependency->isHidden()) parts.append(dependency->currentText());
      parts.append(property->currentText()); entry->setEditText(parts.join('.'));
   });
   updateProperties();
   auto *indices=new QHBoxLayout;
   auto *row=new QSpinBox(this); row->setObjectName("reportArrayRow"); row->setMinimum(1);
   auto *column=new QSpinBox(this); column->setObjectName("reportArrayColumn"); column->setMinimum(1);
   indices->addWidget(new QLabel("Array row",this)); indices->addWidget(row);
   indices->addWidget(new QLabel("Column",this)); indices->addWidget(column);
   auto *element=new QPushButton("Add element",this); element->setObjectName("reportAddElement"); indices->addWidget(element);
   auto *add=new QPushButton("Add parameter",this); add->setObjectName("reportAddParameter"); indices->addWidget(add);
   layout->addLayout(indices);
   list=new QListWidget(this); list->setObjectName("reportSelectedParameters"); list->addItems(selected);
   list->setDragDropMode(QAbstractItemView::InternalMove); layout->addWidget(list);
   auto addName=[this,plottable,arguments](const QString &name) {
      if (name.isEmpty()) return;
      if (singleEntry) { singleEntry->setEditText(name); return; }
      if (plottable && !isPlottableReference(name)) return;
      if (!arguments && !list->findItems(name,Qt::MatchExactly).isEmpty()) return;
      list->addItem(name); list->setCurrentRow(list->count()-1);
   };
   connect(add,&QPushButton::clicked,this,[entry,addName] { addName(entry->currentText().trimmed()); });
   connect(element,&QPushButton::clicked,this,[entry,row,column,addName] {
      addName(entry->currentText()+QString("(%1,%2)").arg(row->value()).arg(column->value()));
   });
   auto updateArray=[entry,row,column,element,moderator] {
      auto *array=dynamic_cast<Array *>(moderator->GetConfiguredObject(entry->currentText().toStdString()));
      if (array && QString::fromStdString(array->GetName())!=entry->currentText().trimmed()) array=nullptr;
      row->setEnabled(array); column->setEnabled(array); element->setEnabled(array);
      if (array) {
         row->setMaximum(array->GetIntegerParameter("NumRows"));
         column->setMaximum(array->GetIntegerParameter("NumCols"));
      }
   };
   connect(entry,&QComboBox::currentTextChanged,this,[updateArray] { updateArray(); }); updateArray();
   auto *actions=new QHBoxLayout;
   for (const auto &label:{QString("Remove"),QString("Up"),QString("Down")}) {
      auto *button=new QPushButton(label,this); button->setObjectName("reportParameter"+label); actions->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,label] {
         const int index=list->currentRow(); if (index<0) return;
         if (label=="Remove") { delete list->takeItem(index); return; }
         const int destination=index+(label=="Up" ? -1 : 1);
         if (destination<0 || destination>=list->count()) return;
         auto *item=list->takeItem(index); list->insertItem(destination,item); list->setCurrentRow(destination);
      });
   }
   auto *removeAll=new QPushButton("Remove all",this); removeAll->setObjectName("reportParameterRemoveAll"); actions->addWidget(removeAll);
   connect(removeAll,&QPushButton::clicked,list,&QListWidget::clear);
   auto *addAll=new QPushButton("Add all configured",this); addAll->setObjectName("reportParameterAddAll"); actions->addWidget(addAll);
   connect(addAll,&QPushButton::clicked,this,[this,names,addName] {
      for (const auto &name:names) if (list->findItems(name,Qt::MatchExactly).isEmpty()) addName(name);
   });
   auto *bulk=new QPushButton("Select multiple object properties…",browser); bulk->setObjectName("reportBulkProperties"); form->addRow(bulk);
   bulk->setVisible(!single); bulk->setEnabled(!properties.isEmpty());
   connect(bulk,&QPushButton::clicked,this,[this,properties,owner,property,addName] {
      BulkPropertyDialog dialog(properties,owner->currentText(),property->currentText(),this);
      if (dialog.exec()!=QDialog::Accepted) return;
      for (const auto &name:dialog.selection()) if (list->findItems(name,Qt::MatchExactly).isEmpty()) addName(name);
   });
   layout->addLayout(actions);
   if (single) {
      list->hide(); add->hide(); element->setText("Use element");
      for (int i=0;i<actions->count();++i) if (auto *widget=actions->itemAt(i)->widget()) widget->hide();
      entry->setEditText(selected.value(0));
   }
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   if (single) {
      auto validate=[this,entry,buttons,moderator,info,writable,realOnly,mode,writableParameter] {
         const auto value=entry->currentText().trimmed();
         const auto *object=moderator->GetConfiguredObject(value.toStdString());
         const bool bareArray=object && object->IsOfType("Array") && QString::fromStdString(object->GetName())==value;
         bool valid=!value.isEmpty() && (!bareArray || mode==Mode::Writable);
         if (writable) {
            static const QRegularExpression reference(R"(^[A-Za-z_][A-Za-z0-9_.]*(?:\(\s*[1-9][0-9]*\s*,\s*[1-9][0-9]*\s*\))?$)");
            valid=valid && reference.match(value).hasMatch();
            if (const auto *parameter=dynamic_cast<const Parameter *>(object)) valid=valid && writableParameter(parameter);
            else if (object) valid=false;
            else {
               const auto type=value.section('.',-1).toStdString();
               const auto &types=info->GetTypesOfParameters();
               if (std::find(types.begin(),types.end(),type)!=types.end())
                  valid=valid && info->IsSettable(type) && (!realOnly || info->IsPlottable(type));
            }
         }
         // Vary requires an existing numeric destination. The syntactic
         // writable check alone also admits unknown/out-of-range array cells.
         if (realOnly) valid=valid && isPlottableReference(value);
         buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
      };
      connect(entry,&QComboBox::currentTextChanged,this,[validate] { validate(); }); validate();
   }
   if (mode==Mode::FunctionOutputs) {
      auto validate=[this,buttons,moderator,info,writableParameter] {
         static const QRegularExpression reference(R"(^[A-Za-z_][A-Za-z0-9_.]*(?:\(\s*[1-9][0-9]*\s*,\s*[1-9][0-9]*\s*\))?$)");
         bool valid=true;
         for (int i=0;i<list->count();++i) {
            const auto value=list->item(i)->text().trimmed(); valid=valid && reference.match(value).hasMatch();
            if (const auto *object=moderator->GetConfiguredObject(value.toStdString())) {
               if (const auto *parameter=dynamic_cast<const Parameter *>(object)) valid=valid && writableParameter(parameter);
            } else {
               // A reference typed here need not have been instantiated by a
               // report or expression yet. Apply the same parameter metadata
               // check as the Assignment destination picker in that case.
               const auto type=value.section('.',-1).toStdString();
               const auto &types=info->GetTypesOfParameters();
               if (std::find(types.begin(),types.end(),type)!=types.end())
                  valid=valid && info->IsSettable(type);
            }
         }
         buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
      };
      connect(list->model(),&QAbstractItemModel::rowsInserted,this,[=] { validate(); });
      connect(list->model(),&QAbstractItemModel::rowsRemoved,this,[=] { validate(); }); validate();
   }
   if (plottable) {
      auto validate=[this,buttons,entry,add,literal] {
         auto allowed=[literal](const QString &value) {
            bool number=false; const auto real=value.toDouble(&number);
            return (literal && number && std::isfinite(real)) || isPlottableReference(value);
         };
         const auto values=selection(); bool valid=singleEntry ? !values.isEmpty() : true;
         for (const auto &value:values) valid=valid && allowed(value);
         buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
         add->setEnabled(allowed(entry->currentText()));
      };
      connect(entry,&QComboBox::currentTextChanged,this,[validate] { validate(); });
      connect(list->model(),&QAbstractItemModel::rowsInserted,this,[validate] { validate(); });
      connect(list->model(),&QAbstractItemModel::rowsRemoved,this,[validate] { validate(); });
      validate();
   }
   connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
}
bool ReportParameterDialog::isPlottableReference(const QString &reference)
{
   const auto value=reference.trimmed(); auto *moderator=Moderator::Instance();
   static const QRegularExpression element(R"(^([A-Za-z_][A-Za-z0-9_]*)\(\s*([1-9][0-9]*)\s*,\s*([1-9][0-9]*)\s*\)$)");
   const auto match=element.match(value);
   if (match.hasMatch()) {
      auto *array=dynamic_cast<Array *>(moderator->GetConfiguredObject(match.captured(1).toStdString()));
      bool rowValid=false,columnValid=false;
      const auto row=match.captured(2).toLongLong(&rowValid),column=match.captured(3).toLongLong(&columnValid);
      return array && rowValid && columnValid && row<=array->GetIntegerParameter("NumRows") && column<=array->GetIntegerParameter("NumCols");
   }
   if (auto *object=moderator->GetConfiguredObject(value.toStdString())) {
      auto *parameter=dynamic_cast<Parameter *>(object);
      return parameter && parameter->IsPlottable();
   }
   static const QRegularExpression property(R"(^[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)+$)");
   if (!property.match(value).hasMatch()) return false;
   auto *info=ParameterInfo::Instance(); const auto type=value.section('.',-1).toStdString();
   const auto &types=info->GetTypesOfParameters();
   if (std::find(types.begin(),types.end(),type)==types.end() || !info->IsPlottable(type)) return false;
   auto *owner=moderator->GetConfiguredObject(value.section('.',0,0).toStdString());
   return owner && owner->IsOfType(info->GetObjectType(type));
}
QStringList ReportParameterDialog::selection() const
{
   if (singleEntry) return singleEntry->currentText().trimmed().isEmpty() ? QStringList() : QStringList{singleEntry->currentText().trimmed()};
   QStringList names;
   for (int i=0;i<list->count();++i) names.append(list->item(i)->text());
   return names;
}
