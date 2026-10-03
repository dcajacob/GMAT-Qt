#include "CustomFovDialog.hpp"
#include "CustomFovProperties.hpp"
#include "ResourcePreview.hpp"
#include "BaseException.hpp"
#include "Rvector.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <stdexcept>

CustomFovDialog::CustomFovDialog(GmatBase &object,const QString &script,QWidget *parent) : QDialog(parent)
{
   setObjectName("customFovDialog"); setWindowTitle("Custom field of view");
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Edit settings here. Apply commits them; Close lets you discard pending changes.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *form=new QFormLayout; layout->addLayout(form);
   source=new QComboBox(this); source->setObjectName("customFovSourceMode"); source->addItem("Paired vertices","Vertices"); source->addItem("FOV file","File"); form->addRow("Define FOV with",source);
   coordinates=new QComboBox(this); coordinates->setObjectName("customFovCoordinates"); coordinates->addItem("Cone / clock","ConeClock"); coordinates->addItem("Azimuth / elevation","AzimuthElevation");
   coordinates->setCurrentIndex(customFovCoordinates(object,script)=="AzimuthElevation" ? 1 : 0); form->addRow("Angle coordinates",coordinates);
   auto *fileRow=new QWidget(this); auto *fileLayout=new QHBoxLayout(fileRow); fileLayout->setContentsMargins(0,0,0,0);
   const auto currentFile=QString::fromStdString(object.GetStringParameter("FOVFileName"));
   file=new QLineEdit(currentFile,fileRow); file->setObjectName("customFov_FOVFileName"); fileLayout->addWidget(file);
   auto *browse=new QPushButton("Browse…",fileRow); browse->setObjectName("customFovBrowse"); fileLayout->addWidget(browse); form->addRow("FOV input file",fileRow);
   connect(browse,&QPushButton::clicked,this,[this] { const auto path=QFileDialog::getOpenFileName(this,"Choose FOV input file",file->text(),"FOV text files (*.txt);;All files (*)"); if (!path.isEmpty()) file->setText(path); });
   interpolation=new QLineEdit(QString::number(object.GetRealParameter("InterpolationStepSize"),'g',17),this); interpolation->setObjectName("customFov_InterpolationStepSize"); form->addRow("Interpolation step (deg)",interpolation);
   auto *note=new QLabel("Enter at least three paired vertices in degrees. The last vertex connects to the first. Cone/clock defines polygon edges. Azimuth/elevation interpolates when its step is nonzero; use 0 or 0.2–180 degrees. FOV files contain a supported angle-format header and paired rows.",this); note->setWordWrap(true); layout->addWidget(note);
   vertices=new QTableWidget(this); vertices->setObjectName("customFovVertices"); vertices->setColumnCount(2);
   vertices->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive); vertices->horizontalHeader()->setStretchLastSection(true); vertices->setColumnWidth(0,210); vertices->setColumnWidth(1,210);
   vertices->setSelectionBehavior(QAbstractItemView::SelectRows); vertices->setSelectionMode(QAbstractItemView::SingleSelection); vertices->setMinimumHeight(180); layout->addWidget(vertices,1);
   const bool cone=coordinates->currentData().toString()=="ConeClock";
   const Rvector a=object.GetRvectorParameter(cone ? "ConeAngles" : "Azimuth"),b=object.GetRvectorParameter(cone ? "ClockAngles" : "Elevation");
   vertices->setRowCount(std::max(3,std::max(a.GetSize(),b.GetSize())));
   for (int i=0;i<vertices->rowCount();++i) {
      vertices->setItem(i,0,new QTableWidgetItem(i<a.GetSize() ? QString::number(a[i],'g',17) : QString()));
      vertices->setItem(i,1,new QTableWidgetItem(i<b.GetSize() ? QString::number(b[i],'g',17) : QString()));
   }
   const auto headers=[this] { vertices->setHorizontalHeaderLabels(coordinates->currentData().toString()=="ConeClock" ? QStringList{"Cone angle (deg)","Clock angle (deg)"} : QStringList{"Azimuth (deg)","Elevation (deg)"}); }; headers();
   connect(coordinates,&QComboBox::currentIndexChanged,this,[this,headers] {
      const bool toCone=coordinates->currentData().toString()=="ConeClock";
      for (int i=0;i<vertices->rowCount();++i) {
         const auto first=vertices->item(i,0)->text(),second=vertices->item(i,1)->text();
         bool valid=false; const auto value=(toCone ? second : first).toDouble(&valid);
         const auto complement=valid && std::isfinite(value) ? QString::number(90-value,'g',17) : (toCone ? second : first);
         vertices->item(i,0)->setText(toCone ? complement : second); vertices->item(i,1)->setText(toCone ? first : complement);
      }
      headers();
   });
   auto *actions=new QHBoxLayout; layout->addLayout(actions);
   auto *add=new QPushButton("Add vertex",this); add->setObjectName("customFovAddVertex"); actions->addWidget(add);
   auto *remove=new QPushButton("Remove vertex",this); remove->setObjectName("customFovRemoveVertex"); actions->addWidget(remove); actions->addStretch();
   connect(add,&QPushButton::clicked,this,[this] { if (vertices->rowCount()>=100000) return; const int row=vertices->rowCount(); vertices->insertRow(row); vertices->setItem(row,0,new QTableWidgetItem); vertices->setItem(row,1,new QTableWidgetItem); vertices->setCurrentCell(row,0); vertices->editItem(vertices->item(row,0)); });
   connect(remove,&QPushButton::clicked,this,[this] { if (vertices->currentRow()>=0) vertices->removeRow(vertices->currentRow()); });
   const auto dependencies=[this,fileRow,add,remove] { const bool fromFile=source->currentData().toString()=="File"; fileRow->setEnabled(fromFile); coordinates->setEnabled(!fromFile); vertices->setEnabled(!fromFile); add->setEnabled(!fromFile); remove->setEnabled(!fromFile); };
   connect(source,&QComboBox::currentIndexChanged,this,dependencies); source->setCurrentIndex(currentFile.isEmpty() ? 0 : 1); dependencies();
   auto *error=new QLabel(this); error->setObjectName("customFovError"); error->setWordWrap(true); layout->addWidget(error);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   auto original=qtResourcePreviewShared(object.Clone());
   connect(buttons,&QDialogButtonBox::accepted,this,[this,original,error] { try { QtResourcePreview preview(original->Clone()); applyCustomFovProperties(*preview,settings()); accept(); } catch (BaseException &failure) { error->setText(QString::fromStdString(failure.GetFullMessage())); } catch (const std::exception &failure) { error->setText(QString::fromUtf8(failure.what())); } });
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
}
QMap<QString,QString> CustomFovDialog::settings() const
{
   const bool fromFile=source->currentData().toString()=="File",cone=coordinates->currentData().toString()=="ConeClock";
   QMap<QString,QString> result{{"@CustomFovSource",fromFile ? "File" : "Vertices"},{"@CustomFovCoordinates",coordinates->currentData().toString()},
      {"FOVFileName",fromFile ? file->text().trimmed() : QString()},{"InterpolationStepSize",interpolation->text()},
      {"ConeAngles",{}},{"ClockAngles",{}},{"Azimuth",{}},{"Elevation",{}}};
   if (!fromFile) for (int column=0;column<2;++column) {
      QStringList values; for (int row=0;row<vertices->rowCount();++row) { const auto *item=vertices->item(row,column); const auto text=item ? item->text().trimmed() : QString(); values.append(text.isEmpty() ? "?" : text); }
      result[cone ? (column ? "ClockAngles" : "ConeAngles") : (column ? "Elevation" : "Azimuth")]=values.join(' ');
   }
   return result;
}
