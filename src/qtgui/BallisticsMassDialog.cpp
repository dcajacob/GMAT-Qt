#include "BallisticsMassDialog.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <cmath>

namespace {
struct Field { const char *name; const char *label; const char *unit; bool nonnegative; };
const Field spherical[] = {
   {"DryMass","Dry mass","kg",true}, {"Cd","Coefficient of drag","dimensionless",false},
   {"Cr","Coefficient of reflectivity","dimensionless",false}, {"DragArea","Drag area","m²",true},
   {"SRPArea","SRP area","m²",true}
};
const Field spad[] = {
   {"SPADSRPFile","SRP file","file",false}, {"SPADSRPScaleFactor","SRP scale factor","dimensionless",true},
   {"SPADSRPInterpolationMethod","SRP interpolation method","choice",false},
   {"SPADDragFile","Drag file","file",false}, {"SPADDragScaleFactor","Drag scale factor","dimensionless",true},
   {"SPADDragInterpolationMethod","Drag interpolation method","choice",false}
};
}

BallisticsMassDialog::BallisticsMassDialog(const QMap<QString,QString> &initial,
   const QMap<QString,QStringList> &choices,QWidget *parent) : QDialog(parent)
{
   setObjectName("ballisticsMassDialog"); setWindowTitle("Ballistics and mass");
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("OK keeps these changes pending. Apply the spacecraft properties to update the mission.",this);
   help->setWordWrap(true); layout->addWidget(help);
   auto addGroup=[&](const QString &title,const auto &fields) {
      auto *group=new QGroupBox(title,this); auto *form=new QFormLayout(group);
      form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
      for (const auto &field:fields) {
         const QString name=field.name;
         if (!initial.contains(name)) continue;
         if (QString(field.unit)=="choice") {
            auto *combo=new QComboBox(group); combo->setObjectName("ballistics_"+name);
            combo->setProperty("ballisticsProperty",name); combo->addItems(choices.value(name));
            if (combo->findText(initial.value(name))<0) combo->addItem(initial.value(name));
            combo->setCurrentText(initial.value(name)); form->addRow(field.label,combo);
         } else {
            auto *row=new QWidget(group); auto *line=new QHBoxLayout(row); line->setContentsMargins(0,0,0,0);
            auto *edit=new QLineEdit(initial.value(name),row); edit->setObjectName("ballistics_"+name);
            edit->setProperty("ballisticsProperty",name); line->addWidget(edit);
            if (QString(field.unit)=="file") {
               auto *browse=new QPushButton("Browse…",row); browse->setObjectName("ballisticsBrowse_"+name); line->addWidget(browse);
               connect(browse,&QPushButton::clicked,this,[this,edit,name] {
                  QFileDialog picker(this,"Choose "+name,edit->text()); picker.setObjectName("ballisticsFileDialog");
                  picker.setAcceptMode(QFileDialog::AcceptOpen); picker.setFileMode(QFileDialog::ExistingFile);
                  if (picker.exec()==QDialog::Accepted && !picker.selectedFiles().isEmpty()) edit->setText(picker.selectedFiles().first());
               });
            } else if (QString(field.unit).size()) {
               auto *unit=new QLabel(field.unit,row);unit->setObjectName("ballisticsUnit_"+name);line->addWidget(unit);
            }
            form->addRow(field.label,row);
         }
      }
      layout->addWidget(group);
   };
   addGroup("Spherical",spherical); addGroup("SPAD files",spad);
   auto *error=new QLabel(this); error->setObjectName("ballisticsError"); error->setWordWrap(true); layout->addWidget(error);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,error] {
      auto validate=[&](const auto &fields) {
         for (const auto &field:fields) {
            if (QString(field.unit)=="choice" || QString(field.unit)=="file") continue;
            auto *edit=findChild<QLineEdit *>("ballistics_"+QString(field.name)); if (!edit) continue;
            bool valid=false; const double value=edit->text().trimmed().toDouble(&valid);
            if (!valid || !std::isfinite(value) || (field.nonnegative && value<0)) {
               error->setText(QString(field.label)+(field.nonnegative ? " must be a finite number greater than or equal to zero." : " must be a finite number."));
               edit->setFocus(); edit->selectAll(); return false;
            }
         }
         return true;
      };
      if (validate(spherical) && validate(spad)) accept();
   });
   resize(620,sizeHint().height());
}

QMap<QString,QString> BallisticsMassDialog::values() const
{
   QMap<QString,QString> result;
   for (auto *edit:findChildren<QLineEdit *>()) {
      const auto name=edit->property("ballisticsProperty").toString();
      if (!name.isEmpty()) result.insert(name,edit->text());
   }
   for (auto *combo:findChildren<QComboBox *>()) {
      const auto name=combo->property("ballisticsProperty").toString();
      if (!name.isEmpty()) result.insert(name,combo->currentText());
   }
   return result;
}
