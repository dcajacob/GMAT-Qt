#include "OrbitSensorMaskDialog.hpp"
#include "TableColumns.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QScreen>
#include <stdexcept>
#include <QSet>
#include <algorithm>

OrbitSensorMaskDialog::OrbitSensorMaskDialog(const QStringList &objects,const QVector<QtSensorMaskSetting> &masks,QWidget *parent)
   : QDialog(parent),sources(objects)
{
   setObjectName("orbitSensorMaskDialog"); setWindowTitle("Sensor masks");
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Display the field of view configured on a hardware resource. The mask follows its source and uses the hardware's location and pointing. Color and opacity come from the field of view. Auto length follows the source size; Manual length is in kilometres. OK keeps changes pending until Apply.",this);
   help->setWordWrap(true); layout->addWidget(help);
   auto *moderator=Moderator::Instance();
   if (moderator->IsInitialized()) for (const auto &name:moderator->GetListOfObjects(Gmat::HARDWARE)) {
      auto *object=moderator->GetConfiguredObject(name);
      try { if (object && !object->GetRefObjectName(Gmat::FIELD_OF_VIEW).empty()) hardware.append(QString::fromStdString(name)); }
      catch (BaseException &) {} // Hardware without a field of view is not a mask source.
   }
   table=new QTableWidget(0,6,this); table->setObjectName("orbitSensorMasks"); table->verticalHeader()->hide();
   table->setHorizontalHeaderLabels({"Mask","Source","Hardware","Label","Length type","Length (km)"});
   configureTableColumns(table,{12,12,12,14,10,12}); table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection); layout->addWidget(table,1);
   for (const auto &mask:masks) append(mask);
   auto *actions=new QHBoxLayout; layout->addLayout(actions);
   for (const auto &name:QStringList{"Add","Remove","Up","Down"}) {
      auto *button=new QPushButton(name,this); button->setObjectName("sensorMask"+name); actions->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,name] {
         const int row=table->currentRow();
         if (name=="Add") {
            QSet<QString> used; for (int r=0;r<table->rowCount();++r) used.insert(table->item(r,0)->text());
            QtSensorMaskSetting mask; int i=1; do { mask.name="Mask"+QString::number(i++); } while (used.contains(mask.name));
            mask.source=sources.value(0); mask.hardware=hardware.value(0); mask.label=mask.name; append(mask); table->setCurrentCell(table->rowCount()-1,0);
         } else if (row>=0 && name=="Remove") table->removeRow(row);
         else if (row>=0) {
            const int next=row+(name=="Up" ? -1 : 1); if (next<0 || next>=table->rowCount()) return;
            // Move the complete editable row without parsing other pending input.
            for (int column=0;column<table->columnCount();++column) {
               if (column==0 || column==3) { auto *a=table->takeItem(row,column),*b=table->takeItem(next,column); table->setItem(row,column,b); table->setItem(next,column,a); }
               else if (column==5) { auto *a=qobject_cast<QLineEdit *>(table->cellWidget(row,column)),*b=qobject_cast<QLineEdit *>(table->cellWidget(next,column)); const auto value=a->text(); a->setText(b->text()); b->setText(value); }
               else { auto *a=qobject_cast<QComboBox *>(table->cellWidget(row,column)),*b=qobject_cast<QComboBox *>(table->cellWidget(next,column)); const auto value=a->currentText(); a->setCurrentText(b->currentText()); b->setCurrentText(value); }
            }
            table->setCurrentCell(next,0);
         }
      });
   }
   actions->addStretch(); auto *status=new QLabel(this); status->setObjectName("sensorMaskStatus"); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,status] { try { settings(); accept(); } catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); } });
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   const auto screen=QGuiApplication::primaryScreen()->availableGeometry(); resize(std::min(950,screen.width()-80),std::min(540,screen.height()-80));
}
void OrbitSensorMaskDialog::append(const QtSensorMaskSetting &mask)
{
   const int row=table->rowCount(); table->insertRow(row); table->setItem(row,0,new QTableWidgetItem(mask.name)); table->setItem(row,3,new QTableWidgetItem(mask.label));
   for (int column:{1,2,4}) {
      auto *choice=new QComboBox(table); choice->setEditable(column!=4); choice->setInsertPolicy(QComboBox::NoInsert);
      choice->addItems(column==1 ? sources : column==2 ? hardware : QStringList{"Auto","Manual"});
      const auto value=column==1 ? mask.source : column==2 ? mask.hardware : mask.automaticLength ? "Auto" : "Manual";
      if (choice->findText(value)<0) choice->addItem(value); choice->setCurrentText(value); table->setCellWidget(row,column,choice);
   }
   auto *length=new QLineEdit(QString::number(mask.length,'g',17),table); length->setEnabled(!mask.automaticLength); table->setCellWidget(row,5,length);
   auto *mode=qobject_cast<QComboBox *>(table->cellWidget(row,4)); connect(mode,&QComboBox::currentTextChanged,length,[length](const QString &value) { length->setEnabled(value=="Manual"); });
}
QVector<QtSensorMaskSetting> OrbitSensorMaskDialog::settings() const
{
   QVector<QtSensorMaskSetting> result;
   for (int row=0;row<table->rowCount();++row) {
      QtSensorMaskSetting mask; mask.name=table->item(row,0)->text().trimmed(); mask.label=table->item(row,3)->text();
      mask.source=qobject_cast<QComboBox *>(table->cellWidget(row,1))->currentText().trimmed(); mask.hardware=qobject_cast<QComboBox *>(table->cellWidget(row,2))->currentText().trimmed();
      mask.automaticLength=qobject_cast<QComboBox *>(table->cellWidget(row,4))->currentText()=="Auto";
      bool ok=false; mask.length=qobject_cast<QLineEdit *>(table->cellWidget(row,5))->text().toDouble(&ok); if (!ok) throw std::runtime_error("Enter a positive finite mask length."); result.append(mask);
   }
   return qtSensorMasks(qtSensorMasksJson(result));
}
