#include "ResourcePreview.hpp"
#include "ResourceEditor.hpp"
#include "ResourceForm.hpp"
#include <QScrollArea>
#include <QHBoxLayout>
#include "InspectionDialog.hpp"
#include "TrackingConfigDialog.hpp"
#include "EpochIntervalDialog.hpp"
#include "FunctionFileDialog.hpp"
#include "BallisticsMassDialog.hpp"
#include "VisualModelDialog.hpp"
#include "AttitudeDialog.hpp"
#include "Moderator.hpp"
#include "AxisSystem.hpp"
#include "TimeSystemConverter.hpp"
#include "BaseException.hpp"
#include <memory>
#include <stdexcept>
#include "ResourceProperties.hpp"
#include "SpacecraftOrbit.hpp"
#include "AtmosphereDialog.hpp"
#include "ExternalForceDialog.hpp"
#include "PolyhedronDialog.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include "GroundStationDialog.hpp"
#include "EventLocatorDialog.hpp"
#include "XYPlotDialog.hpp"
#include "GroundTrackDialog.hpp"
#include "OrbitViewDialog.hpp"
#include "OrbitObjectDrawingDialog.hpp"
#include "ThrusterDialog.hpp"
#include "BurnDialog.hpp"
#include "EphemerisDialog.hpp"
#include "DynamicDataDialog.hpp"
#include "EphemerisFile.hpp"
#include "Spacecraft.hpp"
#include "CalculatedPoint.hpp"
#include "ReportParameterDialog.hpp"
#include "RgbColor.hpp"
#include <QColorDialog>
#include <QPixmap>
#include "ScriptCompatibility.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "TableColumns.hpp"
#include <QTimer>
#include <QSignalBlocker>
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QLocale>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QMessageBox>
#include <QTabBar>
#include <QTabWidget>
#include <QSet>
#include <QDialog>
#include <QSpinBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QListWidget>
#include <QFormLayout>
#include <QRegularExpression>
#include <cmath>
#include <algorithm>

namespace {
class CovarianceCellDelegate final : public QStyledItemDelegate
{
public:
   using QStyledItemDelegate::QStyledItemDelegate;
   QString displayText(const QVariant &value,const QLocale &locale) const override
   {
      bool valid=false; const auto number=value.toString().toDouble(&valid);
      return valid ? locale.toString(number,'g',12) : QStyledItemDelegate::displayText(value,locale);
   }
};
QString comboValue(const QComboBox *combo)
{
   if (!combo->property("resourceValueData").toBool()) return combo->currentText();
   const int index=combo->findText(combo->currentText());
   return index>=0 ? combo->itemData(index).toString() : combo->currentText();
}
}

ResourceEditor::ResourceEditor(GmatBase &object, Apply apply, QWidget *parent,const QString &script,bool applyUnchanged,bool creation) : EditablePanel(parent)
{
   auto *layout = new QVBoxLayout(this);
   if (!creation) layout->addWidget(new QLabel(QString::fromStdString(object.GetName() + " — " + object.GetTypeName()), this));
   if (object.GetTypeName()=="Variable" || object.GetTypeName()=="String") {
      const bool numeric=object.GetTypeName()=="Variable";
      originalScalarValue=numeric ? QString::number(object.GetRealParameter("Value"),'g',17) : QString::fromStdString(object.GetStringParameter("Expression"));
      auto *form=new QFormLayout; auto *value=new QLineEdit(originalScalarValue,this); value->setObjectName("parameterValue");
      value->setPlaceholderText(numeric ? "Finite number" : "Literal text (no enclosing quotes)");
      form->addRow("Initial value",value); layout->addLayout(form);
      scalarValue=[value] { return value->text(); };
      status=new QLabel(numeric ? "Mission assignments can change this value during execution. Edit expressions in the mission sequence." : "Enter literal text without enclosing quotes. Mission assignments stay unchanged.",this);
      status->setWordWrap(true); layout->addWidget(status); layout->addStretch();
      auto *buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Close,this); layout->addWidget(buttons);
      buttons->setObjectName("resourceButtons"); applyButton=buttons->button(QDialogButtonBox::Apply);
      const auto scriptPreview=QString::fromStdString(object.GetGeneratingString(Gmat::SHOW_SCRIPT));
      auto *preview=new QPushButton("Show script…",this); preview->setObjectName("showScript"); buttons->addButton(preview,QDialogButtonBox::ActionRole);
      connect(preview,&QPushButton::clicked,this,[this,scriptPreview] { InspectionDialog dialog("Parameter script",scriptPreview,"Applied initializer. Pending edits are not included.",this); dialog.exec(); });
      connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,this,[this,apply] {
         if (!hasChanges() && !applyButton->property("createResource").toBool()) { reportStatus("No changes to apply."); return; }
         const auto error=apply({{"Value",scalarValue()}});
         if (error.isEmpty()) { applied=true; appliedSuccessfully(); } else reportStatus(error);
      });
      connect(buttons,&QDialogButtonBox::rejected,this,[this] { parentWidget()->close(); });
      applyButton->setProperty("createResource",creation);
      return;
   }
   if (object.IsOfType("CoordinateSystem") && object.GetOwnedObject(0)) {
      auto initial=qtResourcePreviewShared(object.GetOwnedObject(0)->Clone());
      QMap<QString,QString> initialValues;
      for (const auto &field:resourceProperties(*initial)) initialValues[field.name]=field.value;
      initialValues["Axes"]=QString::fromStdString(initial->GetTypeName());
      auto *axesButton=new QPushButton("Axes…",this); axesButton->setObjectName("editCoordinateAxes");
      layout->addWidget(axesButton);
      connect(axesButton,&QPushButton::clicked,this,[this,initial,initialValues] {
         QMap<QString,QString> pending;
         for (int row=0;row<table->rowCount();++row) {
            const auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,1));
            pending[table->item(row,0)->text()]=choice ? comboValue(choice) : table->item(row,1)->text();
         }
         for (auto it=coordinateAxisEdits.cbegin();it!=coordinateAxisEdits.cend();++it)
            if (!pending.contains(it.key())) pending[it.key()]=it.value();
         QDialog dialog(this); dialog.setObjectName("coordinateAxesDialog"); dialog.setWindowTitle("Coordinate system axes");
         auto *layout=new QVBoxLayout(&dialog);
         auto *type=new QComboBox(&dialog); type->setObjectName("coordinateAxisType");
         for (const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::AXIS_SYSTEM)) type->addItem(QString::fromStdString(name));
         type->setCurrentText(pending.value("Axes",initialValues.value("Axes"))); layout->addWidget(type);
         auto *help=new QLabel("Choose an axis type and set its dependent properties. Changing type resets pending axis edits. OK keeps these settings pending until the parent Create or Apply.",&dialog);
         help->setWordWrap(true); layout->addWidget(help);
         ResourceEditor *panel=nullptr;
         auto rebuild=[&] {
            if (panel) { delete panel; panel=nullptr; }
            try {
               const auto selected=type->currentText();
               QtResourcePreview axes(selected==initialValues.value("Axes") ? initial->Clone() :
                  Moderator::Instance()->CreateAxisSystem(selected.toStdString(),"",0));
               if (!axes) { help->setText("The engine could not create these axes."); return; }
               if (selected==pending.value("Axes")) for (const auto &field:resourceProperties(*axes))
                  if (pending.contains(field.name)) setResourceProperty(*axes,field.name,pending.value(field.name));
               QMap<QString,QString> values;
               for (const auto &field:resourceProperties(*axes)) values[field.name]=field.value;
               panel=new ResourceEditor(*axes,[&,selected,values](const QMap<QString,QString> &changes) {
                  auto candidate=values;
                  for (auto it=changes.cbegin();it!=changes.cend();++it) candidate[it.key()]=it.value();
                  candidate["Axes"]=selected;
                  // Update fields already present in the parent and retain new
                  // dependent fields locally. No parent callback runs here.
                  for (int row=0;row<table->rowCount();++row) {
                     const auto name=table->item(row,0)->text(); if (!candidate.contains(name)) continue;
                     if (auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,1))) choice->setCurrentText(candidate.value(name));
                     else table->item(row,1)->setText(candidate.value(name));
                  }
                  coordinateAxisEdits.clear();
                  for (auto it=candidate.cbegin();it!=candidate.cend();++it)
                     if (selected!=initialValues.value("Axes") || !initialValues.contains(it.key()) || it.value()!=initialValues.value(it.key()))
                        coordinateAxisEdits[it.key()]=it.value();
                  return QString();
               },&dialog,{},true);
               auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");
               buttons->button(QDialogButtonBox::Apply)->setText("OK"); buttons->button(QDialogButtonBox::Close)->setText("Cancel");
               panel->onApplied=[&dialog] { dialog.accept(); };
               layout->addWidget(panel);
            } catch (BaseException &error) { help->setText(QString::fromStdString(error.GetFullMessage())); }
            catch (const std::exception &error) { help->setText(QString::fromUtf8(error.what())); }
         };
         connect(type,&QComboBox::currentTextChanged,&dialog,[&] { rebuild(); }); rebuild();
         dialog.resize(700,600); dialog.exec();
      });
   }
   auto *search = new QLineEdit(this);
   search->setObjectName("propertyFilter");
   search->setPlaceholderText("Filter properties…");
   QTabBar *sections=nullptr;
   const bool spacecraft=object.IsOfType("Spacecraft");
   const bool thruster=object.IsOfType("Thruster");
   const bool impulsive=object.IsOfType("ImpulsiveBurn");
   const bool tank=object.IsOfType("FuelTank");
   const bool solver=object.IsOfType("Solver");
   const bool forces=object.IsOfType("ODEModel");
   const bool power=object.IsOfType("PowerSystem");
   if (spacecraft || thruster || tank || solver || forces || power) {
      sections=new QTabBar(this);
      sections->setObjectName("propertySections");
      sections->setExpanding(false);
      layout->addWidget(sections);
   }
   if (thruster || impulsive || object.IsOfType("EphemerisFile")) {
      auto initial=qtResourcePreviewShared(object.Clone());
      const bool ephemeris=object.IsOfType("EphemerisFile");
      auto *setup=new QPushButton(ephemeris ? "Ephemeris output…" : impulsive ? "Impulsive burn setup…" : "Thruster setup…",this); setup->setObjectName(ephemeris ? "editEphemeris" : impulsive ? "editBurn" : "editThruster"); layout->addWidget(setup);
      connect(setup,&QPushButton::clicked,this,[this,initial,impulsive,ephemeris] {
         QMap<QString,QString> pending;
         for (int row=0;row<table->rowCount();++row) {
            const auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1));
            pending.insert(table->item(row,0)->text(),combo ? comboValue(combo) : table->item(row,1)->text());
         }
         QMap<QString,QString> values;
         if (ephemeris) { EphemerisDialog dialog(*initial,pending,this); if (dialog.exec()!=QDialog::Accepted) return; values=dialog.settings(); }
         else if (impulsive) { BurnDialog dialog(*initial,pending,this); if (dialog.exec()!=QDialog::Accepted) return; values=dialog.settings(); }
         else { ThrusterDialog dialog(*initial,pending,this); if (dialog.exec()!=QDialog::Accepted) return; values=dialog.settings(); }
         table->setProperty("ephemerisGroupedUpdate",ephemeris);
         for (int row=0;row<table->rowCount();++row) {
            const auto name=table->item(row,0)->text(); if (!values.contains(name)) continue;
            if (auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1))) combo->setCurrentText(values.value(name)); else table->item(row,1)->setText(values.value(name));
         }
         table->setProperty("ephemerisGroupedUpdate",false);
      });
   }
   if (auto *point=dynamic_cast<CalculatedPoint *>(&object); point && point->IsBuiltIn()) {
      auto *note=new QLabel("This is a built-in calculated point. Its body membership is fixed; its orbit and target colors can be changed.",this);
      note->setObjectName("builtInPointInfo"); note->setWordWrap(true); layout->addWidget(note);
   }
   if (object.IsOfType("DynamicDataDisplay")) {
      auto initial=qtResourcePreviewShared(object.Clone()); auto *setup=new QPushButton("Dynamic data setup…",this); setup->setObjectName("editDynamicData"); layout->addWidget(setup);
      connect(setup,&QPushButton::clicked,this,[this,initial] {
         QMap<QString,QString> pending;
         if (!pendingDynamicData.isEmpty()) pending.insert("@DynamicData",pendingDynamicData);
         for (int row=0;row<table->rowCount();++row) { const auto name=table->item(row,0)->text(); if (name=="WarnColor" || name=="CritColor") pending.insert(name,table->item(row,1)->text()); }
         DynamicDataDialog dialog(*initial,pending,this); if (dialog.exec()!=QDialog::Accepted) return; const auto values=dialog.settings(); pendingDynamicData=values.value("@DynamicData");
         for (int row=0;row<table->rowCount();++row) { const auto name=table->item(row,0)->text(); if (values.contains(name)) table->item(row,1)->setText(values.value(name)); }
      });
   }
   if (object.IsOfType("TrackingFileSet")) {
      auto initial=qtResourcePreviewShared(object.Clone());
      auto *setup=new QPushButton("Tracking configurations…",this); setup->setObjectName("editTrackingConfigs"); layout->addWidget(setup);
      connect(setup,&QPushButton::clicked,this,[this,initial] {
         try { TrackingConfigDialog dialog(*initial,pendingTrackingConfigs,this); if (dialog.exec()==QDialog::Accepted) pendingTrackingConfigs=dialog.settings(); }
         catch (BaseException &failure) { reportStatus(QString::fromStdString(failure.GetFullMessage())); }
         catch (const std::exception &failure) { reportStatus(QString::fromUtf8(failure.what())); }
      });
   }
   if (object.IsOfType("ChemicalThruster") || object.IsOfType("ElectricThruster")) {
      const bool electric=object.IsOfType("ElectricThruster");
      QStringList names[2],units[2];
      for (int family=0;family<2;++family) {
         const auto values=object.GetStringArrayParameter(electric ? (family ? "MF_UNITS" : "T_UNITS") : (family ? "K_UNITS" : "C_UNITS"));
         const QString prefix=electric ? (family ? "MassFlowCoeff" : "ThrustCoeff") : (family ? "K" : "C");
         for (size_t i=0;i<values.size();++i) {
            names[family].append(prefix+QString::number(i+1)); units[family].append(QString::fromStdString(values[i]));
         }
      }
      auto *button=new QPushButton("Coefficients…",this); button->setObjectName("thrusterCoefficients"); layout->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,electric,names,units] {
         QDialog dialog(this); dialog.setObjectName("thrusterCoefficientDialog"); dialog.setWindowTitle("Thruster coefficients");
         auto *layout=new QVBoxLayout(&dialog);
         auto *help=new QLabel("Edit the coefficient sets below. OK keeps changes pending until you Apply the thruster properties.",&dialog);
         help->setWordWrap(true); layout->addWidget(help);
         auto *tabs=new QTabWidget(&dialog); layout->addWidget(tabs);
         QTableWidget *grids[2];
         QList<QTableWidgetItem *> targets[2];
         for (int family=0;family<2;++family) {
            auto *grid=new QTableWidget(names[family].size(),3,tabs); grids[family]=grid;
            grid->setObjectName(family ? "thrusterSecondaryCoefficients" : "thrusterThrustCoefficients");
            grid->setHorizontalHeaderLabels({"Coefficient","Value","Unit"}); grid->verticalHeader()->hide();
            for (int row=0;row<names[family].size();++row) {
               QTableWidgetItem *target=nullptr;
               for (int r=0;r<table->rowCount();++r) if (table->item(r,0)->text()==names[family][row]) { target=table->item(r,1); break; }
               targets[family].append(target);
               grid->setItem(row,0,new QTableWidgetItem(names[family][row]));
               grid->setItem(row,1,new QTableWidgetItem(target ? target->text() : QString()));
               grid->setItem(row,2,new QTableWidgetItem(units[family][row]));
               for (int column:{0,2}) grid->item(row,column)->setFlags(grid->item(row,column)->flags() & ~Qt::ItemIsEditable);
            }
            configureTableColumns(grid,{18,24,16}); fitTableColumns(grid);
            tabs->addTab(grid,family ? (electric ? "Mass Flow Coefficients" : "Impulse Coefficients") : "Thrust Coefficients");
         }
         auto *error=new QLabel(&dialog); error->setObjectName("thrusterCoefficientError"); error->setWordWrap(true); layout->addWidget(error);
         auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
         connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
         connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
            for (int family=0;family<2;++family) for (int row=0;row<grids[family]->rowCount();++row) {
               bool valid=false; const double value=grids[family]->item(row,1)->text().trimmed().toDouble(&valid);
               if (!targets[family][row] || !valid || !std::isfinite(value)) {
                  error->setText(names[family][row]+" must be a finite number."); tabs->setCurrentIndex(family);
                  grids[family]->setCurrentCell(row,1); return;
               }
            }
            dialog.accept();
         });
         dialog.resize(680,530);
         if (dialog.exec()!=QDialog::Accepted) return;
         for (int family=0;family<2;++family) for (int row=0;row<grids[family]->rowCount();++row)
            if (targets[family][row]) targets[family][row]->setText(grids[family]->item(row,1)->text().trimmed());
      });
   }
   if (thruster) {
      QStringList available;
      for (const auto &field:resourceProperties(object)) if (field.name=="Tank") available=field.references;
      auto *button=new QPushButton("Tanks and mixtures…",this); button->setObjectName("thrusterTankMixtures"); layout->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,available] {
         QTableWidgetItem *tanks=nullptr,*ratios=nullptr;
         for (int row=0;row<table->rowCount();++row) {
            if (table->item(row,0)->text()=="Tank") tanks=table->item(row,1);
            if (table->item(row,0)->text()=="MixRatio") ratios=table->item(row,1);
         }
         if (!tanks || !ratios) return;
         QDialog dialog(this); dialog.setObjectName("tankMixtureDialog"); dialog.setWindowTitle("Tanks and mixture ratios");
         auto *layout=new QVBoxLayout(&dialog);
         auto *help=new QLabel("Each ratio belongs to the tank on the same row. Ratios must be positive; they need not sum to one. OK keeps changes pending until Apply.",&dialog);
         help->setWordWrap(true); layout->addWidget(help);
         auto *grid=new QTableWidget(0,2,&dialog); grid->setObjectName("tankMixtureTable"); grid->setHorizontalHeaderLabels({"Tank","Mixture ratio"});
         grid->setSelectionBehavior(QAbstractItemView::SelectRows); grid->setSelectionMode(QAbstractItemView::SingleSelection);
         grid->verticalHeader()->hide(); configureTableColumns(grid,{24,20}); layout->addWidget(grid);
         auto append=[&](const QString &name,const QString &ratio) {
            const int row=grid->rowCount(); grid->insertRow(row); auto *item=new QTableWidgetItem(name);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable); grid->setItem(row,0,item); grid->setItem(row,1,new QTableWidgetItem(ratio)); grid->selectRow(row);
         };
         const auto selected=splitResourceReferences(tanks->text());
         const auto values=ratios->text().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
         for (int i=0;i<selected.size();++i) append(selected[i],i<values.size() ? values[i] : "1");
         fitTableColumns(grid);
         auto *controls=new QHBoxLayout; layout->addLayout(controls);
         auto *choices=new QComboBox(&dialog); choices->setObjectName("tankMixtureAvailable"); choices->addItems(available); controls->addWidget(choices,1);
         auto addButton=[&](const QString &text,const QString &name) { auto *control=new QPushButton(text,&dialog); control->setObjectName(name); controls->addWidget(control); return control; };
         auto *add=addButton("Add","tankMixtureAdd"),*remove=addButton("Remove","tankMixtureRemove");
         auto *up=addButton("Up","tankMixtureUp"),*down=addButton("Down","tankMixtureDown");
         auto refresh=[&] {
            bool exists=false;
            for (int row=0;row<grid->rowCount();++row) exists=exists || grid->item(row,0)->text()==choices->currentText();
            add->setEnabled(choices->count()>0 && !exists); remove->setEnabled(grid->currentRow()>=0);
            up->setEnabled(grid->currentRow()>0); down->setEnabled(grid->currentRow()>=0 && grid->currentRow()+1<grid->rowCount());
         };
         connect(add,&QPushButton::clicked,&dialog,[&] { append(choices->currentText(),"1"); refresh(); });
         connect(remove,&QPushButton::clicked,&dialog,[&] { grid->removeRow(grid->currentRow()); refresh(); });
         auto move=[&](int delta) {
            const int row=grid->currentRow(),target=row+delta;
            if (row<0 || target<0 || target>=grid->rowCount()) return;
            const QSignalBlocker block(grid);
            for (int col=0;col<2;++col) { auto *first=grid->takeItem(row,col),*second=grid->takeItem(target,col); grid->setItem(row,col,second); grid->setItem(target,col,first); }
            grid->selectRow(target); refresh();
         };
         connect(up,&QPushButton::clicked,&dialog,[&] { move(-1); }); connect(down,&QPushButton::clicked,&dialog,[&] { move(1); });
         connect(choices,&QComboBox::currentTextChanged,&dialog,[&] { refresh(); });
         connect(grid,&QTableWidget::itemSelectionChanged,&dialog,[&] { refresh(); }); refresh();
         auto *error=new QLabel(&dialog); error->setObjectName("tankMixtureError"); error->setWordWrap(true); layout->addWidget(error);
         auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
         connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
         connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
            for (int row=0;row<grid->rowCount();++row) {
               bool valid=false; const double value=grid->item(row,1)->text().toDouble(&valid);
               if (!valid || !std::isfinite(value) || value<=0) { error->setText("Enter a finite ratio greater than zero for "+grid->item(row,0)->text()+"."); grid->setCurrentCell(row,1); return; }
            }
            dialog.accept();
         });
         dialog.resize(620,420); if (dialog.exec()!=QDialog::Accepted) return;
         QStringList names,mixes;
         for (int row=0;row<grid->rowCount();++row) { names.append(grid->item(row,0)->text()); mixes.append(grid->item(row,1)->text().trimmed()); }
         tanks->setText(names.join(", ")); ratios->setText(mixes.join(" ")); ratios->setData(Qt::UserRole+1,mixes.size()); pairedTankEdits=true;
      });
   }
   if (object.IsOfType("GmatFunction")) {
      auto *edit=new QPushButton("Edit function file…",this); edit->setObjectName("editFunctionFile"); layout->addWidget(edit);
      auto *create=new QPushButton("New function file…",this); create->setObjectName("newFunctionFile"); layout->addWidget(create);
      const auto functionName=QString::fromStdString(object.GetName());
      connect(create,&QPushButton::clicked,this,[this,functionName] {
         QFileDialog chooser(this,"New GMAT function file",functionName+".gmf","GMAT functions (*.gmf);;All files (*)");
         chooser.setObjectName("newFunctionFileDialog"); chooser.setAcceptMode(QFileDialog::AcceptSave); chooser.setDefaultSuffix("gmf");
         if (chooser.exec()!=QDialog::Accepted || chooser.selectedFiles().isEmpty()) return;
         const auto path=chooser.selectedFiles().first();
         if (QFileInfo::exists(path)) { reportStatus("That file already exists. Use Edit function file or choose a new name."); return; }
         QString source;
         try { source=functionFileTemplate(path); } catch (const std::exception &error) { reportStatus(QString::fromUtf8(error.what())); return; }
         FunctionFileDialog dialog(path,this,source);
         if (dialog.exec()!=QDialog::Accepted) return;
         for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="FunctionPath") table->item(row,1)->setText(dialog.savedPath());
         reportStatus("Function file created. Apply to use its path in the mission.");
      });
      connect(edit,&QPushButton::clicked,this,[this] {
         QString path;
         for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="FunctionPath") path=table->item(row,1)->text();
         if (path.trimmed().isEmpty()) { reportStatus("Choose a function file first."); return; }
         FunctionFileDialog dialog(path,this);
         if (dialog.exec()==QDialog::Accepted && dialog.savedPath()!=QFileInfo(path).absoluteFilePath()) {
            for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="FunctionPath") table->item(row,1)->setText(dialog.savedPath());
            reportStatus("Function saved to a new file. Apply to use its new path in the mission.");
         }
      });
   }
   if (object.IsOfType("Array")) {
      auto *button=new QPushButton("Expressions…",this); button->setObjectName("arrayExpressions"); layout->addWidget(button);
      const auto name=QString::fromStdString(object.GetName());
      const int rows=object.GetIntegerParameter("NumRows"),columns=object.GetIntegerParameter("NumCols");
      try { originalExpressions=expressions=arrayExpressions(script,name); setArrayExpressions(script,name,expressions,rows,columns); }
      catch (const std::exception &error) { button->setEnabled(false); button->setToolTip(QString::fromUtf8(error.what())); }
      connect(button,&QPushButton::clicked,this,[this,rows,columns] {
         int pendingRows=rows,pendingColumns=columns;
         for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="RmatValue") {
            const auto lines=table->item(row,1)->text().split(';');
            pendingRows=lines.size(); pendingColumns=0;
            for (const auto &line:lines) pendingColumns=std::max(pendingColumns,static_cast<int>(line.trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts).size()));
         }
         int gridRows=pendingRows,gridColumns=pendingColumns;
         for (const auto &value:QJsonDocument::fromJson(expressions.toUtf8()).array()) {
            const auto cell=value.toObject();
            gridRows=std::max(gridRows,cell["row"].toInt()); gridColumns=std::max(gridColumns,cell["column"].toInt());
         }
         QDialog dialog(this); dialog.setObjectName("arrayExpressionDialog"); dialog.setWindowTitle("Array expressions");
         auto *layout=new QVBoxLayout(&dialog);
         auto *help=new QLabel("Nonempty cells run at mission start, in row order, before existing commands. Blank cells keep their numeric initial values. Apply validates formulas without running them. Numeric values and formulas are applied together.",&dialog);
         help->setWordWrap(true); layout->addWidget(help);
         auto *grid=new QTableWidget(gridRows,gridColumns,&dialog); grid->setObjectName("arrayExpressionGrid");
         for (int r=0;r<gridRows;++r) for (int c=0;c<gridColumns;++c) {
            auto *item=new QTableWidgetItem; grid->setItem(r,c,item);
            if (r>=pendingRows || c>=pendingColumns) { item->setBackground(QColor(255,235,190)); item->setToolTip("Outside the pending array dimensions. Clear this formula or cancel and enlarge the numeric grid."); }
         }
         for (const auto &value:QJsonDocument::fromJson(expressions.toUtf8()).array()) {
            const auto cell=value.toObject(); const int r=cell["row"].toInt()-1,c=cell["column"].toInt()-1;
            if (r>=0 && r<gridRows && c>=0 && c<gridColumns) grid->item(r,c)->setText(cell["expression"].toString());
         }
         configureTableColumns(grid); fitTableColumns(grid); layout->addWidget(grid);
         auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
         auto *error=new QLabel(&dialog); error->setWordWrap(true); layout->addWidget(error);
         const auto validate=[=] {
            bool valid=pendingRows>0 && pendingColumns>0;
            for (int r=0;r<gridRows;++r) for (int c=0;c<gridColumns;++c)
               if ((r>=pendingRows || c>=pendingColumns) && !grid->item(r,c)->text().trimmed().isEmpty()) valid=false;
            buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
            error->setText(valid ? "" : "Clear formulas in highlighted cells, or cancel and enlarge the numeric grid.");
         };
         connect(grid,&QTableWidget::itemChanged,&dialog,[=] { validate(); }); validate();
         connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
         connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
         dialog.resize(650,350);
         if (dialog.exec()!=QDialog::Accepted) return;
         QJsonArray cells;
         for (int r=0;r<gridRows;++r) for (int c=0;c<gridColumns;++c) {
            const auto text=grid->item(r,c)->text().trimmed();
            if (!text.isEmpty()) cells.append(QJsonObject{{"row",r+1},{"column",c+1},{"expression",text}});
         }
         expressions=QString::fromUtf8(QJsonDocument(cells).toJson(QJsonDocument::Compact));
      });
   }
   layout->addWidget(search);
   if (object.GetTypeName()=="OrbitView" || object.GetTypeName()=="XYPlot" || object.GetTypeName()=="GroundTrackPlot" || object.GetTypeName()=="GroundTrack") {
      const bool orbit=object.GetTypeName()=="OrbitView";
      const bool ground=!orbit && object.GetTypeName()!="XYPlot";
      auto initial=qtResourcePreviewShared(object.Clone());
      if (orbit) {
         const auto setting=qtCameraSettings(script).value(QString::fromStdString(object.GetName()));
         const QMap<QString,QString> originalDrawing={{"@QtObjectLabels",qtObjectFlagsJson(setting.objectLabels)},{"@QtObjectTrajectories",qtObjectFlagsJson(setting.objectTrajectories)},{"@QtObjectCenters",qtObjectFlagsJson(setting.objectCenters)},{"@QtObjectEndpoints",qtObjectFlagsJson(setting.objectEndpoints)},{"@QtObjectMarkerSizes",qtObjectSizesJson(setting.objectMarkerSizes)},{"@QtObjectLineWidths",qtObjectWidthsJson(setting.objectLineWidths)},{"@QtObjectFontSizes",qtObjectSizesJson(setting.objectFontSizes)},{"@QtObjectFontPositions",qtObjectFontPositionsJson(setting.objectFontPositions)},{"@QtObjectAxes",qtObjectFlagsJson(setting.objectAxes)},{"@QtObjectGrids",qtObjectFlagsJson(setting.objectGrids)},{"@QtObjectXYPlanes",qtObjectFlagsJson(setting.objectXYPlanes)},{"@QtObjectVelocities",qtObjectFlagsJson(setting.objectVelocities)}};
         auto *drawing=new QPushButton("Object drawing…",this); drawing->setObjectName("editOrbitDrawing"); layout->addWidget(drawing);
         connect(drawing,&QPushButton::clicked,this,[this,originalDrawing,drawing] {
            try {
               QStringList names;
               for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="Add") {
                  const auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,1)); names=splitResourceReferences(choice ? comboValue(choice) : table->item(row,1)->text());
               }
               if (formValues) names=splitResourceReferences(formValues().value("Add"));
               QSet<QString> unique;
               for (const auto &name:names) {
                  auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());
                  if (!object || !object->IsOfType(Gmat::SPACE_POINT) || unique.contains(name)) throw std::runtime_error("Select each available space point at most once in Orbit-view setup.");
                  unique.insert(name);
               }
               const auto labels=qtObjectFlags(objectDrawingEdits.value("@QtObjectLabels",originalDrawing.value("@QtObjectLabels"))),trajectories=qtObjectFlags(objectDrawingEdits.value("@QtObjectTrajectories",originalDrawing.value("@QtObjectTrajectories")));
               QtCameraSetting markers;
               markers.objectCenters=qtObjectFlags(objectDrawingEdits.value("@QtObjectCenters",originalDrawing.value("@QtObjectCenters")));
               markers.objectEndpoints=qtObjectFlags(objectDrawingEdits.value("@QtObjectEndpoints",originalDrawing.value("@QtObjectEndpoints")));
               markers.objectAxes=qtObjectFlags(objectDrawingEdits.value("@QtObjectAxes",originalDrawing.value("@QtObjectAxes")));
               markers.objectGrids=qtObjectFlags(objectDrawingEdits.value("@QtObjectGrids",originalDrawing.value("@QtObjectGrids")));
               markers.objectXYPlanes=qtObjectFlags(objectDrawingEdits.value("@QtObjectXYPlanes",originalDrawing.value("@QtObjectXYPlanes")));
               markers.objectVelocities=qtObjectFlags(objectDrawingEdits.value("@QtObjectVelocities",originalDrawing.value("@QtObjectVelocities")));
               markers.objectMarkerSizes=qtObjectSizes(objectDrawingEdits.value("@QtObjectMarkerSizes",originalDrawing.value("@QtObjectMarkerSizes")));
               markers.objectLineWidths=qtObjectWidths(objectDrawingEdits.value("@QtObjectLineWidths",originalDrawing.value("@QtObjectLineWidths")));
               markers.objectFontSizes=qtObjectFontSizes(objectDrawingEdits.value("@QtObjectFontSizes",originalDrawing.value("@QtObjectFontSizes")));
               markers.objectFontPositions=qtObjectFontPositions(objectDrawingEdits.value("@QtObjectFontPositions",originalDrawing.value("@QtObjectFontPositions")));
               OrbitObjectDrawingDialog dialog(names,labels,trajectories,this,&markers); if (dialog.exec()!=QDialog::Accepted) return;
               const auto values=dialog.settings();
               for (auto it=values.cbegin();it!=values.cend();++it) { if (it.value()==originalDrawing.value(it.key())) objectDrawingEdits.remove(it.key()); else objectDrawingEdits.insert(it.key(),it.value()); }
               drawing->setText(objectDrawingEdits.isEmpty() ? "Object drawing…" : "Object drawing… (pending)");
               reportStatus(objectDrawingEdits.isEmpty() ? "Object drawing is unchanged." : "Object drawing is pending. Apply keeps the drawing choices.");
            } catch (BaseException &error) { reportStatus(QString::fromStdString(error.GetFullMessage())); }
            catch (const std::exception &error) { reportStatus(QString::fromUtf8(error.what())); }
         });
      }
      auto *button=new QPushButton(orbit ? "Orbit-view setup…" : ground ? "Ground-track setup…" : "XY plot setup…",this); button->setObjectName(orbit ? "editOrbitView" : ground ? "editGroundTrack" : "editXYPlot"); layout->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,initial,ground,orbit] {
         QMap<QString,QString> pending;
         for (int row=0;row<table->rowCount();++row) {
            const auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1));
            pending.insert(table->item(row,0)->text(),combo ? comboValue(combo) : table->item(row,1)->text());
         }
         QMap<QString,QString> values;
         if (orbit) { OrbitViewDialog dialog(*initial,pending,this); if (dialog.exec()!=QDialog::Accepted) return; values=dialog.settings(); }
         else if (ground) { GroundTrackDialog dialog(*initial,pending,this); if (dialog.exec()!=QDialog::Accepted) return; values=dialog.settings(); }
         else { XYPlotDialog dialog(*initial,pending,this); if (dialog.exec()!=QDialog::Accepted) return; values=dialog.settings(); }
         for (int row=0;row<table->rowCount();++row) {
            const auto name=table->item(row,0)->text(); if (!values.contains(name)) continue;
            if (auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1))) combo->setCurrentText(values.value(name));
            else table->item(row,1)->setText(values.value(name));
         }
      });
   }
   table = new QTableWidget(this); table->setObjectName("resourceProperties");
   table->setColumnCount(4);
   table->setHorizontalHeaderLabels({"Property", "Value", "Unit", ""});
   configureTableColumns(table,{20,28,5,10});
   table->verticalHeader()->hide();
   for (const auto &field : resourceProperties(object)) {
      int row = table->rowCount(); table->insertRow(row);
      original.insert(field.name, field.value);
      auto *name = new QTableWidgetItem(field.name);
      name->setFlags(name->flags() & ~Qt::ItemIsEditable);
      table->setItem(row, 0, name);
      if (spacecraft && field.name=="PowerSystem") {
         auto *choices=new QComboBox(table); choices->setObjectName("spacecraftPowerSystem");
         choices->setProperty("resourceValueData",true);
         choices->addItem("No power system",QString());
         for (const auto &name:field.references) choices->addItem(name,name);
         if (choices->findData(field.value)<0) choices->addItem(field.value,field.value);
         choices->setCurrentIndex(choices->findData(field.value)); table->setCellWidget(row,1,choices);
      } else if (object.GetTypeName()=="ReportFile" && field.name=="Delimiter") {
         auto *choices=new QComboBox(table);
         choices->setObjectName("reportDelimiter");
         choices->setEditable(true);
         choices->setInsertPolicy(QComboBox::NoInsert);
         choices->setProperty("resourceValueData",true);
         choices->addItem("Space"," "); choices->addItem("Tab","\t");
         choices->addItem("Comma",","); choices->addItem("Semicolon",";");
         choices->addItem("Pipe","|");
         if (choices->findData(field.value)<0) choices->addItem(field.value,field.value);
         choices->setCurrentIndex(choices->findData(field.value));
         table->setCellWidget(row,1,choices);
      } else if (field.choices.isEmpty()) {
         auto *value=new QTableWidgetItem(field.value);
         if (!field.help.isEmpty()) value->setToolTip(field.help);
         if (field.fileList) {
            value->setFlags(value->flags() & ~Qt::ItemIsEditable);
            value->setToolTip("Ordered file paths. Browse to add, remove or reorder files; Apply replaces the complete list.");
         } else if (field.list) value->setToolTip("Comma-separated resource or parameter names. Apply replaces the complete list.");
         if (field.rows>0 && field.columns>0) {
            value->setFlags(value->flags() & ~Qt::ItemIsEditable);
            value->setData(Qt::UserRole,field.rows);
            value->setData(Qt::UserRole+1,field.columns);
            value->setData(Qt::UserRole+2,object.IsOfType("Array") && field.name=="RmatValue");
            value->setData(Qt::UserRole+3,field.resizableVector);
            value->setData(Qt::UserRole+4,spacecraft && field.name=="OrbitErrorCovariance");
            value->setToolTip("Double-click to edit the numeric cells.");
            auto *edit=new QPushButton("Edit cells…",table);
            edit->setObjectName("editCells_"+field.name);
            table->setCellWidget(row,3,edit);
            connect(edit,&QPushButton::clicked,this,[this,value] { table->itemDoubleClicked(value); });
         }
         table->setItem(row, 1, value);
         if (field.color) {
            value->setToolTip("Enter a GMAT color name or [red green blue], or choose a color. Components range from 0 to 255.");
            auto *choose=new QPushButton("Choose color…",table); choose->setObjectName("chooseProperty_"+field.name); table->setCellWidget(row,3,choose);
            auto currentColor=[value] {
               try { return QColor::fromRgb(RgbColor::ToIntColor(value->text().trimmed().toStdString()) & 0xffffff); }
               catch (BaseException &) { return QColor(); }
            };
            auto preview=[choose,currentColor] {
               const auto color=currentColor(); QPixmap swatch(24,16); swatch.fill(color.isValid() ? color : Qt::transparent);
               choose->setIcon(QIcon(swatch)); choose->setToolTip(color.isValid() ? color.name() : "Invalid color; choose a replacement");
            };
            connect(table,&QTableWidget::itemChanged,choose,[value,preview](QTableWidgetItem *changed) { if (changed==value) preview(); }); preview();
            connect(choose,&QPushButton::clicked,this,[this,value,currentColor] {
               QColorDialog dialog(currentColor(),this); dialog.setObjectName("resourceColorDialog"); dialog.setWindowTitle("Choose color");
               if (dialog.exec()==QDialog::Accepted) {
                  const auto color=dialog.selectedColor();
                  value->setText(QString("[%1 %2 %3]").arg(color.red()).arg(color.green()).arg(color.blue()));
               }
            });
         }
         const bool reportParameters=object.GetTypeName()=="ReportFile" && field.name=="Add";
         const bool plotParameters=object.GetTypeName()=="XYPlot" && (field.name=="XVariable" || field.name=="YVariables");
         const bool filterFiles=object.IsOfType(Gmat::DATA_FILTER) && field.name=="FileNames",acceptFilter=object.IsOfType("AcceptFilter");
         const bool forceBodies=object.IsOfType("ODEModel") && (field.name=="PrimaryBodies" || field.name=="PointMasses");
         if (field.filename || !field.references.isEmpty() || reportParameters || plotParameters) {
            auto *choose=new QPushButton(field.filename ? "Browse…" : "Select…",table);
            choose->setObjectName("chooseProperty_"+field.name);
            if (!field.help.isEmpty()) choose->setToolTip(field.help);
            table->setCellWidget(row,3,choose);
            connect(choose,&QPushButton::clicked,this,[this,value,field,reportParameters,plotParameters,filterFiles,acceptFilter,forceBodies] {
               if (reportParameters || plotParameters) {
                  const auto mode=plotParameters ? (field.name=="XVariable" ? ReportParameterDialog::Mode::PlottableSingle : ReportParameterDialog::Mode::PlottableMultiple) : ReportParameterDialog::Mode::Multiple;
                  ReportParameterDialog dialog(splitResourceReferences(value->text()),this,mode);
                  if (dialog.exec()==QDialog::Accepted) value->setText(dialog.selection().join(", "));
               } else if (field.fileList) {
                  QDialog dialog(this); dialog.setObjectName("kernelFileDialog"); dialog.setWindowTitle(field.name); dialog.resize(650,360);
                  auto *layout=new QVBoxLayout(&dialog);
                  layout->addWidget(new QLabel("Add files. Drag rows to change their order.",&dialog));
                  auto *list=new QListWidget(&dialog); list->setObjectName("kernelFileList");
                  list->addItems(value->text().split('\n',Qt::SkipEmptyParts)); list->setSelectionMode(QAbstractItemView::ExtendedSelection);
                  list->setDragDropMode(QAbstractItemView::InternalMove); layout->addWidget(list);
                  auto *actions=new QHBoxLayout; layout->addLayout(actions);
                  auto *add=new QPushButton("Add files…",&dialog); add->setObjectName("kernelFileAdd"); actions->addWidget(add);
                  auto *remove=new QPushButton("Remove selected",&dialog); remove->setObjectName("kernelFileRemove"); actions->addWidget(remove); actions->addStretch();
                  connect(add,&QPushButton::clicked,&dialog,[&] {
                     QFileDialog picker(&dialog,"Select files"); picker.setObjectName("kernelFilePicker"); picker.setFileMode(field.fileOutput ? QFileDialog::AnyFile : QFileDialog::ExistingFiles);
                     if (field.fileOutput) { picker.setAcceptMode(QFileDialog::AcceptSave); picker.setOption(QFileDialog::DontConfirmOverwrite,true); }
                     if (picker.exec()!=QDialog::Accepted) return;
                     for (const auto &path:picker.selectedFiles()) if (list->findItems(path,Qt::MatchExactly).isEmpty()) list->addItem(path);
                  });
                  connect(remove,&QPushButton::clicked,&dialog,[list] { qDeleteAll(list->selectedItems()); });
                  if (filterFiles) {
                     auto sentinel=[&](const QString &label,const QString &name,const QString &value) { auto *button=new QPushButton(label,&dialog); button->setObjectName(name); actions->addWidget(button); connect(button,&QPushButton::clicked,&dialog,[list,value] { list->clear(); list->addItem(value); }); };
                     sentinel("All files","filterAllFiles","All");
                     if (acceptFilter) sentinel("From tracking","filterTrackingFiles","From_AddTrackingConfig");
                  }
                  auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons);
                  connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
                  if (dialog.exec()==QDialog::Accepted) {
                     QStringList paths; for (int i=0;i<list->count();++i) paths.append(list->item(i)->text()); value->setText(paths.join('\n'));
                  }
               } else if (field.filename) {
                  QFileDialog dialog(this,"Choose "+field.name,value->text());
                  dialog.setObjectName("resourceFileDialog");
                  dialog.setAcceptMode(field.fileOutput ? QFileDialog::AcceptSave : QFileDialog::AcceptOpen);
                  dialog.setFileMode(field.fileInput ? QFileDialog::ExistingFile : QFileDialog::AnyFile);
                  // Selecting an output path does not write it. Apply/run owns
                  // file creation, so do not imply that browsing overwrites it.
                  dialog.setOption(QFileDialog::DontConfirmOverwrite,true);
                  if (dialog.exec()==QDialog::Accepted && !dialog.selectedFiles().isEmpty())
                     value->setText(dialog.selectedFiles().first());
               } else if (!field.list) {
                  auto names=field.references;
                  if (!names.contains(value->text())) names.prepend(value->text());
                  bool accepted=false;
                  const auto selected=QInputDialog::getItem(this,"Select "+field.name,field.name,names,
                     names.indexOf(value->text()),true,&accepted);
                  if (accepted) value->setText(selected);
               } else {
                  QDialog dialog(this); dialog.setObjectName("resourceSelectionDialog");
                  dialog.setWindowTitle("Select "+field.name);
                  auto *layout=new QVBoxLayout(&dialog);
                  layout->addWidget(new QLabel("Select resources. Drag rows to change their order.",&dialog));
                  auto *list=new QListWidget(&dialog); list->setObjectName("resourceSelectionList");
                  list->setDragDropMode(QAbstractItemView::InternalMove);
                  QStringList selected=splitResourceReferences(value->text());
                  for (auto &name:selected) name=name.trimmed();
                  auto names=selected;
                  for (const auto &name:field.references) if (!names.contains(name)) names.append(name);
                  auto hidden=field.hiddenReferences;
                  // Match wx's primary/point-mass exclusion using the pending
                  // table values, rather than the model at panel creation.
                  if (forceBodies) {
                     const auto other=field.name=="PointMasses" ? "PrimaryBodies" : "PointMasses";
                     for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==other)
                        hidden+=splitResourceReferences(table->item(row,1)->text());
                  }
                  for (const auto &name:hidden) names.removeAll(name);
                  for (const auto &name:names) {
                     auto *item=new QListWidgetItem(name,list);
                     item->setFlags(item->flags()|Qt::ItemIsUserCheckable);
                     item->setCheckState(selected.contains(name) ? Qt::Checked : Qt::Unchecked);
                  }
                  layout->addWidget(list);
                  auto *selectionActions=new QHBoxLayout;
                  auto *selectAll=new QPushButton("Select all",&dialog); selectAll->setObjectName("resourceSelectAll");
                  auto *clearAll=new QPushButton("Clear selection",&dialog); clearAll->setObjectName("resourceClearSelection");
                  selectionActions->addWidget(selectAll); selectionActions->addWidget(clearAll); selectionActions->addStretch();
                  layout->addLayout(selectionActions);
                  connect(selectAll,&QPushButton::clicked,&dialog,[list] {
                     for (int i=0;i<list->count();++i) list->item(i)->setCheckState(Qt::Checked);
                  });
                  connect(clearAll,&QPushButton::clicked,&dialog,[list] {
                     for (int i=0;i<list->count();++i) list->item(i)->setCheckState(Qt::Unchecked);
                  });
                  auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
                  layout->addWidget(buttons);
                  connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
                  connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
                  if (dialog.exec()==QDialog::Accepted) {
                     QStringList names;
                     for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) names.append(list->item(i)->text());
                     value->setText(names.join(", "));
                  }
               }
            });
         }
      }
      else {
         auto *choices = new QComboBox(table);
         choices->addItems(field.choices);
         if (object.IsOfType("EphemerisFile") && QStringList{"StepSize","InitialEpoch","FinalEpoch"}.contains(field.name)) { choices->setEditable(true); choices->setInsertPolicy(QComboBox::NoInsert); }
         if (choices->findText(field.value) < 0) choices->addItem(field.value);
         choices->setCurrentText(field.value); table->setCellWidget(row, 1, choices);
      }
      auto *unit = new QTableWidgetItem(field.unit); unit->setFlags(unit->flags() & ~Qt::ItemIsEditable);
      table->setItem(row, 2, unit);
   }
   if (object.IsOfType("ErrorModel")) {
      auto initial=qtResourcePreviewShared(object.Clone()); QComboBox *type=nullptr;
      for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="Type") type=qobject_cast<QComboBox *>(table->cellWidget(row,1));
      if (type) connect(type,&QComboBox::currentTextChanged,this,[this,initial](const QString &selected) {
         try {
            QtResourcePreview copy(initial->Clone()); copy->SetStringParameter("Type",selected.toStdString()); const auto unit=QString::fromStdString(copy->GetParameterUnit(copy->GetParameterID("Bias")));
            for (int row=0;row<table->rowCount();++row) if (QStringList{"NoiseSigma","Bias","BiasSigma","PassBiases"}.contains(table->item(row,0)->text())) table->item(row,2)->setText(unit);
         } catch (BaseException &failure) { reportStatus(QString::fromStdString(failure.GetFullMessage())); }
      });
   }
   if (object.IsOfType("EventLocator")) {
      auto *button=new QPushButton("Event locator…",this); button->setObjectName("editEventLocator"); layout->addWidget(button);
      const auto name=object.GetName();
      connect(button,&QPushButton::clicked,this,[this,name,button] {
         try {
            auto *configured=Moderator::Instance()->GetConfiguredObject(name); if (!configured) throw std::runtime_error("This locator is no longer available.");
            auto pending=eventEdits;
            if (pending.isEmpty()) for (int row=0;row<table->rowCount();++row) { const auto field=table->item(row,0)->text(); const auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1)); const auto value=combo ? comboValue(combo) : table->item(row,1)->text(); if (value!=original.value(field)) pending.insert(field,value); }
            EventLocatorDialog dialog(*configured,pending,this); if (dialog.exec()!=QDialog::Accepted) return;
            eventEdits=dialog.values();
            for (int row=0;row<table->rowCount();++row) {
               const auto field=table->item(row,0)->text(); if (!eventEdits.contains(field)) continue;
               if (auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1))) combo->setCurrentText(eventEdits.value(field)); else table->item(row,1)->setText(eventEdits.value(field));
               for (int column=1;column<table->columnCount();++column) { if (auto *widget=table->cellWidget(row,column)) widget->setEnabled(false); if (auto *item=table->item(row,column)) item->setFlags(item->flags() & ~Qt::ItemIsEditable); }
            }
            button->setText("Event locator… (pending)"); reportStatus("Locator settings are pending. Review them with Event locator…, then Apply.");
         } catch (BaseException &failure) { reportStatus(QString::fromStdString(failure.GetFullMessage())); }
         catch (const std::exception &failure) { reportStatus(QString::fromUtf8(failure.what())); }
      });
   }
   if (object.IsOfType("GroundStation")) {
      auto *button=new QPushButton("Ground station…",this); button->setObjectName("editGroundStation"); layout->addWidget(button);
      const auto name=object.GetName();
      const QSet<QString> names={"Id","CentralBody","StateType","HorizonReference","Location1","Location2","Location3","MinimumElevationAngle","OrbitColor","TargetColor","HorizonMaskFileName"};
      connect(button,&QPushButton::clicked,this,[this,name,button,names] {
         try {
            auto *configured=Moderator::Instance()->GetConfiguredObject(name);
            if (!configured) throw std::runtime_error("This station is no longer available.");
            auto pending=stationEdits;
            if (pending.isEmpty()) for (int row=0;row<table->rowCount();++row) {
               const auto field=table->item(row,0)->text(); if (!names.contains(field)) continue;
               const auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1));
               const auto value=combo ? comboValue(combo) : table->item(row,1)->text();
               if (value!=original.value(field)) pending.insert(field,value);
            }
            GroundStationDialog dialog(*configured,pending,this); if (dialog.exec()!=QDialog::Accepted) return;
            stationEdits=dialog.values();
            for (int row=0;row<table->rowCount();++row) {
               const auto field=table->item(row,0)->text(); if (!names.contains(field)) continue;
               const auto value=stationEdits.value(field);
               if (auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1))) { if (combo->findText(value)<0) combo->addItem(value); combo->setCurrentText(value); }
               else if (auto *item=table->item(row,1)) item->setText(value);
               for (int column=1;column<table->columnCount();++column) {
                  if (auto *widget=table->cellWidget(row,column)) widget->setEnabled(false);
                  if (auto *item=table->item(row,column)) item->setFlags(item->flags() & ~Qt::ItemIsEditable);
               }
            }
            button->setText("Ground station… (pending)"); reportStatus("Station settings are pending. Use Ground station… to review them, then Apply.");
         } catch (BaseException &failure) { reportStatus(QString::fromStdString(failure.GetFullMessage())); }
         catch (const std::exception &failure) { reportStatus(QString::fromUtf8(failure.what())); }
      });
   }
   if (forces) {
      originalExternal=externalForceSettings(object);
      auto *external=new QPushButton("Python external force…",this); external->setObjectName("forceExternal"); layout->addWidget(external);
      const auto types=Moderator::Instance()->GetListOfAllFactoryItems();
      external->setEnabled(std::find(types.begin(),types.end(),"ExternalModel")!=types.end());
      const auto externalName=object.GetName();
      connect(external,&QPushButton::clicked,this,[this,externalName,external] {
         try {
            auto *model=Moderator::Instance()->GetConfiguredObject(externalName); if (!model) throw std::runtime_error("This force model is no longer available.");
            auto pending=externalEdits;
            if (pending.isEmpty()) {
               pending=externalForceSettings(*model);
               const QMap<QString,QString> names={{"External.ScriptFileName","@ExternalForce.Module"},{"External.DerivativesFunction","@ExternalForce.Function"},{"External.ExcludeOtherForces","@ExternalForce.ExcludeOtherForces"}};
               for (int row=0;row<table->rowCount();++row) if (names.contains(table->item(row,0)->text())) {
                  const auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1)); pending[names.value(table->item(row,0)->text())]=combo ? comboValue(combo) : table->item(row,1)->text();
               }
            }
            ExternalForceDialog dialog(*model,pending,this); if (dialog.exec()!=QDialog::Accepted) return;
            externalEdits=dialog.values();
            const bool changed=externalEdits!=originalExternal;
            const QMap<QString,QString> names={{"External.ScriptFileName","@ExternalForce.Module"},{"External.DerivativesFunction","@ExternalForce.Function"},{"External.ExcludeOtherForces","@ExternalForce.ExcludeOtherForces"}};
            for (int row=0;row<table->rowCount();++row) if (names.contains(table->item(row,0)->text())) {
               const auto value=externalEdits.value(names.value(table->item(row,0)->text()));
               if (auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1))) combo->setCurrentText(value); else table->item(row,1)->setText(value);
               for (int column=1;column<table->columnCount();++column) {
                  if (auto *widget=table->cellWidget(row,column)) widget->setEnabled(!changed);
                  if (auto *item=table->item(row,column);item && column==1 && !table->cellWidget(row,column)) item->setFlags(changed ? item->flags() & ~Qt::ItemIsEditable : item->flags() | Qt::ItemIsEditable);
               }
            }
            if (!changed) externalEdits.clear();
            external->setText(changed ? "Python external force… (pending)" : "Python external force…"); reportStatus(changed ? "External force settings are pending. Review them with Python external force…, then Apply." : "External force settings are unchanged.");
         } catch (BaseException &error) { reportStatus(QString::fromStdString(error.GetFullMessage())); }
         catch (const std::exception &error) { reportStatus(QString::fromUtf8(error.what())); }
      });
      originalPolyhedron=QString::fromUtf8(QJsonDocument(polyhedronSettings(object)).toJson(QJsonDocument::Compact));
      auto *polyhedron=new QPushButton("Polyhedron gravity…",this); polyhedron->setObjectName("forcePolyhedron"); layout->addWidget(polyhedron);
      polyhedron->setEnabled(std::find(types.begin(),types.end(),"PolyhedronGravityModel")!=types.end());
      connect(polyhedron,&QPushButton::clicked,this,[this,externalName,polyhedron] {
         try {
            auto *model=Moderator::Instance()->GetConfiguredObject(externalName); if (!model) throw std::runtime_error("This force model is no longer available.");
            auto settings=QJsonDocument::fromJson((pendingPolyhedron.isEmpty() ? originalPolyhedron : pendingPolyhedron).toUtf8()).array();
            if (pendingPolyhedron.isEmpty()) for (int index=0;index<settings.size();++index) {
               auto force=settings[index].toObject(); const auto prefix="PolyhedronGravityModel."+force.value("body").toString()+".";
               const QMap<QString,QString> names={{"CreateForceBody","body"},{"ShapeFileName","shape"},{"BodyDensity","density"}};
               for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text().startsWith(prefix)) {
                  const auto leaf=table->item(row,0)->text().mid(prefix.size()); if (names.contains(leaf)) force[names.value(leaf)]=table->item(row,1)->text();
               }
               const auto originalBody=settings[index].toObject().value("body").toString();
               if (originalBody!=force.value("body").toString()) force["previousBody"]=originalBody;
               settings[index]=force;
            }
            PolyhedronDialog dialog(settings,this); if (dialog.exec()!=QDialog::Accepted) return;
            pendingPolyhedron=QString::fromUtf8(QJsonDocument(dialog.values()).toJson(QJsonDocument::Compact)); const bool changed=pendingPolyhedron!=originalPolyhedron;
            const auto originals=QJsonDocument::fromJson(originalPolyhedron.toUtf8()).array();
            for (const auto &entry:originals) {
               const auto force=entry.toObject(); const auto prefix="PolyhedronGravityModel."+force.value("body").toString()+".";
               const QMap<QString,QString> names={{"CreateForceBody","body"},{"ShapeFileName","shape"},{"BodyDensity","density"}};
               for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text().startsWith(prefix)) {
                  const auto leaf=table->item(row,0)->text().mid(prefix.size());
                  if (!changed && names.contains(leaf)) table->item(row,1)->setText(force.value(names.value(leaf)).toString());
                  for (int column=1;column<table->columnCount();++column) {
                     if (auto *widget=table->cellWidget(row,column)) widget->setEnabled(!changed);
                     if (auto *item=table->item(row,column);item && column==1 && !table->cellWidget(row,column)) item->setFlags(changed ? item->flags() & ~Qt::ItemIsEditable : item->flags() | Qt::ItemIsEditable);
                  }
               }
            }
            if (!changed) pendingPolyhedron.clear();
            polyhedron->setText(changed ? "Polyhedron gravity… (pending)" : "Polyhedron gravity…"); reportStatus(changed ? "Polyhedron contributors are pending. Review them with Polyhedron gravity…, then Apply." : "Polyhedron settings are unchanged.");
         } catch (BaseException &error) { reportStatus(QString::fromStdString(error.GetFullMessage())); }
         catch (const std::exception &error) { reportStatus(QString::fromUtf8(error.what())); }
      });
      auto *button=new QPushButton("Atmosphere and drag…",this); button->setObjectName("forceAtmosphere"); layout->addWidget(button);
      const auto name=object.GetName();
      connect(button,&QPushButton::clicked,this,[this,name,button] {
         try {
            auto *configured=Moderator::Instance()->GetConfiguredObject(name);
            if (!configured) throw std::runtime_error("This force model is no longer available.");
            auto pending=atmosphereEdits;
            if (pending.isEmpty()) for (int row=0;row<table->rowCount();++row) {
               const auto field=table->item(row,0)->text();
               if (field!="Drag" && !field.startsWith("Drag.")) continue;
               const auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1));
               const auto value=combo ? comboValue(combo) : table->item(row,1)->text();
               if (value!=original.value(field)) pending.insert(field,value);
            }
            QMap<QString,QString> bodyEdits;
            for (int row=0;row<table->rowCount();++row) {
               const auto field=table->item(row,0)->text(); if (field!="PrimaryBodies" && field!="PointMasses") continue;
               const auto value=table->item(row,1)->text(); if (value!=original.value(field)) bodyEdits.insert(field,value);
            }
            // Preview pending gravity selections before validating drag, so
            // a newly selected primary can acquire drag in the same Apply.
            QtResourcePreview preview(configured->Clone()); applyGravityBodyProperties(*preview,bodyEdits);
            AtmosphereDialog dialog(*preview,pending,this);
            if (dialog.exec()!=QDialog::Accepted) return;
            atmosphereEdits=dialog.values();
            for (int row=0;row<table->rowCount();++row) {
               const auto field=table->item(row,0)->text();
               if (field!="Drag" && !field.startsWith("Drag.")) continue;
               const auto value=field=="Drag" ? atmosphereEdits.value("Drag.AtmosphereModel") :
                  atmosphereEdits.value(field,"Not used by "+atmosphereEdits.value("Drag.AtmosphereModel"));
               if (auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1))) {
                  if (combo->findText(value)<0) combo->addItem(value);
                  combo->setCurrentText(value);
               } else if (auto *item=table->item(row,1)) item->setText(value);
               for (int column=1;column<table->columnCount();++column) {
                  if (auto *widget=table->cellWidget(row,column)) widget->setEnabled(false);
                  if (auto *item=table->item(row,column)) item->setFlags(item->flags() & ~Qt::ItemIsEditable);
               }
            }
            button->setText("Atmosphere and drag… (pending)");
            reportStatus("Atmosphere settings are pending. Use Atmosphere and drag… to review them, then Apply the force model.");
         } catch (BaseException &error) { reportStatus(QString::fromStdString(error.GetFullMessage())); }
         catch (const std::exception &error) { reportStatus(QString::fromUtf8(error.what())); }
      });
   }
   if (spacecraft) {
      attitudeNames.insert("Attitude");
      if (auto *attitude=object.GetOwnedObject(0)) for (int id=0;id<attitude->GetParameterCount();++id) {
         const auto name=QString::fromStdString(attitude->GetParameterText(id));
         if (name!="Epoch") attitudeNames.insert(name);
      }
      auto *attitudeButton=new QPushButton("Attitude…",this); attitudeButton->setObjectName("spacecraftAttitude"); layout->addWidget(attitudeButton);
      const auto attitudeSpacecraftName=object.GetName();
      connect(attitudeButton,&QPushButton::clicked,this,[this,attitudeSpacecraftName,attitudeButton] {
         try {
            auto *configured=Moderator::Instance()->GetConfiguredObject(attitudeSpacecraftName);
            if (!configured) throw std::runtime_error("This spacecraft is no longer available.");
            auto pending=attitudeEdits;
            if (pending.isEmpty()) for (int row=0;row<table->rowCount();++row) {
               const auto name=table->item(row,0)->text();
               if (!attitudeNames.contains(name)) continue;
               const auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1));
               const auto value=combo ? comboValue(combo) : table->item(row,1)->text();
               if (value!=original.value(name)) pending.insert(name,value);
            }
            AttitudeDialog dialog(*configured,pending,this);
            if (dialog.exec()!=QDialog::Accepted) return;
            attitudeEdits=dialog.values();
            // The generic table is a snapshot of the original model. Keep it
            // visible for context, but use the dialog for subsequent edits.
            for (int row=0;row<table->rowCount();++row) if (attitudeNames.contains(table->item(row,0)->text())) {
               const auto name=table->item(row,0)->text();
               const auto value=attitudeEdits.contains(name) ? attitudeEdits.value(name) : "Not used by "+attitudeEdits.value("Attitude");
               if (auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1))) {
                  if (combo->findText(value)<0) combo->addItem(value);
                  combo->setCurrentText(value);
               } else if (auto *item=table->item(row,1)) item->setText(value);
               for (int column=1;column<table->columnCount();++column) {
                  if (auto *widget=table->cellWidget(row,column)) widget->setEnabled(false);
                  if (auto *item=table->item(row,column)) item->setFlags(item->flags() & ~Qt::ItemIsEditable);
               }
            }
            attitudeButton->setText("Attitude… (pending)");
            reportStatus("Attitude settings are pending. Use Attitude… to review them, then Apply the spacecraft.");
         } catch (BaseException &error) { reportStatus(QString::fromStdString(error.GetFullMessage())); }
         catch (const std::exception &error) { reportStatus(QString::fromUtf8(error.what())); }
      });
      auto *visual=new QPushButton("Visual model…",this); visual->setObjectName("spacecraftVisualModel"); layout->addWidget(visual);
      const auto spacecraftName=object.GetName();
      connect(visual,&QPushButton::clicked,this,[this,spacecraftName] {
         QMap<QString,QString> values;
         for (int row=0;row<table->rowCount();++row) {
            const auto name=table->item(row,0)->text();
            if (name=="ModelFile" || name.startsWith("ModelRotation") || name.startsWith("ModelOffset") || name=="ModelScale" || name=="OrbitColor" || name=="TargetColor")
               values.insert(name,table->item(row,1)->text());
         }
         VisualModelDialog dialog(values,[spacecraftName](const QMap<QString,QString> &pending) {
            auto *configured=Moderator::Instance()->GetConfiguredObject(spacecraftName);
            if (!configured) throw std::runtime_error("This spacecraft is no longer available.");
            QtResourcePreview preview(configured->Clone());
            if (!preview) throw std::runtime_error("This spacecraft cannot be previewed.");
            for (auto it=pending.cbegin();it!=pending.cend();++it) setResourceProperty(*preview,it.key(),it.value());
            QMap<QString,QString> normalized=pending;
            for (auto it=pending.cbegin();it!=pending.cend();++it)
               if (it.key().startsWith("ModelRotation") || it.key().startsWith("ModelOffset") || it.key()=="ModelScale")
                  normalized[it.key()]=QString::number(preview->GetRealParameter(it.key().toStdString()),'g',17);
            normalized["@ResolvedModelFile"]=pending.value("ModelFile").isEmpty() ? QString() : QString::fromStdString(preview->GetStringParameter("ModelFileFullPath"));
            return normalized;
         },this);
         if (dialog.exec()!=QDialog::Accepted) return;
         const auto edited=dialog.values();
         for (int row=0;row<table->rowCount();++row) {
            const auto name=table->item(row,0)->text(); if (edited.contains(name)) table->item(row,1)->setText(edited.value(name));
         }
      });
      auto *button=new QPushButton("Ballistics and mass…",this); button->setObjectName("spacecraftBallisticsMass");
      layout->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this] {
         QMap<QString,QString> values; QMap<QString,QStringList> options;
         for (int row=0;row<table->rowCount();++row) {
            const auto name=table->item(row,0)->text();
            auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1));
            values.insert(name,combo ? comboValue(combo) : table->item(row,1)->text());
            if (combo) for (int i=0;i<combo->count();++i) options[name].append(combo->itemText(i));
         }
         BallisticsMassDialog dialog(values,options,this);
         if (dialog.exec()!=QDialog::Accepted) return;
         const auto edited=dialog.values();
         for (int row=0;row<table->rowCount();++row) {
            const auto name=table->item(row,0)->text(); if (!edited.contains(name)) continue;
            if (auto *combo=qobject_cast<QComboBox *>(table->cellWidget(row,1))) combo->setCurrentText(edited.value(name));
            else table->item(row,1)->setText(edited.value(name));
         }
      });
   }
   if (power || spacecraft) {
      const QString formatProperty=power ? "EpochFormat" : "DateFormat";
      const QString epochProperty=power ? "InitialEpoch" : "Epoch";
      int formatRow=-1; QTableWidgetItem *epoch=nullptr;
      for (int row=0;row<table->rowCount();++row) {
         if (table->item(row,0)->text()==formatProperty) formatRow=row;
         if (table->item(row,0)->text()==epochProperty) epoch=table->item(row,1);
      }
      if (formatRow>=0 && epoch) {
         const auto initialFormat=original.value(formatProperty);
         auto *format=new QComboBox(table); format->setObjectName(power ? "powerEpochFormat" : "spacecraftEpochFormat");
         for (const auto &value:TimeSystemConverter::Instance()->GetValidTimeRepresentations()) format->addItem(QString::fromStdString(value));
         if (format->findText(initialFormat)<0) format->addItem(initialFormat);
         format->setCurrentText(initialFormat); table->setCellWidget(formatRow,1,format);
         connect(format,&QComboBox::currentTextChanged,this,[this,format,epoch,previous=initialFormat](const QString &next) mutable {
            auto text=epoch->text().trimmed();
            if (text.startsWith("'") && text.endsWith("'")) text=text.mid(1,text.size()-2);
            try {
               Real mjd; std::string converted;
               TimeSystemConverter::Instance()->Convert(previous.toStdString(),-999.999,text.toStdString(),next.toStdString(),mjd,converted);
               epoch->setText(QString::fromStdString(converted)); previous=next;
               reportStatus("Epoch converted. Apply keeps the date and format together.");
            } catch (BaseException &error) {
               const QSignalBlocker blocker(format); format->setCurrentText(previous);
               reportStatus("Epoch conversion failed: "+QString::fromStdString(error.GetFullMessage()));
            }
         });
      }
   }
   if (spacecraft) {
      QComboBox *representation=nullptr,*frame=nullptr,*anomaly=nullptr; QVector<int> elementRows;
      for (int row=0;row<table->rowCount();++row) {
         const auto name=table->item(row,0)->text();
         if (name=="DisplayStateType") representation=qobject_cast<QComboBox *>(table->cellWidget(row,1));
         else if (name=="CoordinateSystem" || name=="AnomalyType") {
            auto *choice=new QComboBox(table);
            if (name=="CoordinateSystem") {
               for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::COORDINATE_SYSTEM)) choice->addItem(QString::fromStdString(name));
               frame=choice; frame->setObjectName("spacecraftOrbitFrame");
            } else { choice->addItems(spacecraftOrbitAnomalies(object)); anomaly=choice; anomaly->setObjectName("spacecraftAnomalyType"); }
            if (choice->findText(original.value(name))<0) choice->addItem(original.value(name));
            choice->setCurrentText(original.value(name)); table->setCellWidget(row,1,choice);
            table->removeCellWidget(row,3);
         }
      }
      for (int element=1;element<=6;++element) {
         const auto label=spacecraftOrbitElementNames(object)[element-1];
         for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==label) { elementRows.append(row); break; }
      }
      if (representation && frame && anomaly && elementRows.size()==6) {
         representation->setObjectName("spacecraftStateRepresentation");
         struct Selection { QString frame,representation,anomaly; QStringList displayed; Rvector6 internal; };
         QStringList displayed;
         for (const auto row:elementRows) displayed.append(table->item(row,1)->text());
         auto previous=std::make_shared<Selection>(Selection{frame->currentText(),representation->currentText(),anomaly->currentText(),displayed,
            static_cast<Spacecraft &>(object).GetState().GetState()});
         anomaly->setEnabled(previous->representation=="Keplerian" || previous->representation=="ModifiedKeplerian");
         const auto resourceName=object.GetName();
         auto orbitDraft=qtResourcePreviewShared(object.Clone());
         pendingOrbit=[this,representation,frame,anomaly,elementRows,resourceName,previous,creation,orbitDraft] {
            auto *current=creation ? orbitDraft.get() : Moderator::Instance()->GetConfiguredObject(resourceName);
            if (!current) throw std::runtime_error("The spacecraft no longer exists. Reopen this panel.");
            QtResourcePreview preview(current->Clone());
            auto *spacecraft=static_cast<Spacecraft *>(preview.get());
            spacecraft->SetState(previous->internal);
            QMap<QString,QString> source={{"CoordinateSystem",frame->currentText()},{"DisplayStateType",representation->currentText()},{"AnomalyType",anomaly->currentText()}};
            for (int row=0;row<table->rowCount();++row) {
               const auto name=table->item(row,0)->text();
               if (name!="DateFormat" && name!="Epoch") continue;
               const auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,1));
               source.insert(name,choice ? comboValue(choice) : table->item(row,1)->text());
            }
            bool edited=false;
            for (int i=0;i<elementRows.size();++i) edited=edited || table->item(elementRows[i],1)->text()!=previous->displayed[i];
            if (edited) for (const auto row:elementRows) source.insert(table->item(row,0)->text(),table->item(row,1)->text());
            applySpacecraftOrbitProperties(*preview,source);
            if (!edited) spacecraft->SetState(previous->internal);
            QStringList state;
            const auto *internal=spacecraft->GetState().GetState();
            for (int i=0;i<6;++i) state.append(QString::number(internal[i],'g',17));
            source.insert("@OrbitCartesianState",state.join(','));
            return source;
         };
         auto convert=[this,representation,frame,anomaly,elementRows,resourceName,previous,creation,orbitDraft] {
            try {
               auto nextRepresentation=representation->currentText();
               const auto nextFrame=frame->currentText(); auto nextAnomaly=anomaly->currentText();
               const auto available=spacecraftOrbitRepresentations(*spacecraftOrbitFrame(nextFrame));
               // Like wx, changing to a frame that cannot support the current
               // representation falls back to Cartesian while retaining the orbit.
               if (nextFrame!=previous->frame && !available.contains(nextRepresentation)) nextRepresentation="Cartesian";
               auto *current=creation ? orbitDraft.get() : Moderator::Instance()->GetConfiguredObject(resourceName);
               if (!current) throw std::runtime_error("The spacecraft no longer exists. Reopen this panel.");
               QtResourcePreview preview(current->Clone());
               QMap<QString,QString> source={{"CoordinateSystem",previous->frame},{"DisplayStateType",previous->representation},{"AnomalyType",previous->anomaly}};
               for (int row=0;row<table->rowCount();++row) {
                  const auto name=table->item(row,0)->text();
                  if (name!="DateFormat" && name!="Epoch") continue;
                  const auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,1));
                  source.insert(name,choice ? comboValue(choice) : table->item(row,1)->text());
               }
               bool edited=false;
               for (int i=0;i<elementRows.size();++i) edited=edited || table->item(elementRows[i],1)->text()!=previous->displayed[i];
               // Keep the physical Cartesian state between display changes,
               // as wx does. Some engine representation conversions are not
               // exact inverses; repeatedly parsing untouched display values
               // otherwise changes the orbit merely by browsing the choices.
               static_cast<Spacecraft *>(preview.get())->SetState(previous->internal);
               if (edited) for (const auto row:elementRows) source.insert(table->item(row,0)->text(),table->item(row,1)->text());
               applySpacecraftOrbitProperties(*preview,source);
               if (!edited) static_cast<Spacecraft *>(preview.get())->SetState(previous->internal);
               applySpacecraftOrbitProperties(*preview,{{"CoordinateSystem",nextFrame},{"DisplayStateType",nextRepresentation}});
               const auto availableAnomalies=spacecraftOrbitAnomalies(*preview);
               if (!availableAnomalies.contains(nextAnomaly) && (nextFrame!=previous->frame || nextRepresentation!=previous->representation)) nextAnomaly="TA";
               applySpacecraftOrbitProperties(*preview,{{"AnomalyType",nextAnomaly}});
               const auto converted=static_cast<Spacecraft *>(preview.get())->GetState(nextRepresentation.toStdString());
               QStringList labels,values,units;
               for (int element=1;element<=6;++element) {
                  labels.append(spacecraftOrbitElementNames(*preview)[element-1]);
                  values.append(QString::number(converted[element-1],'g',17));
                  units.append(QString::fromStdString(preview->GetStringParameter("Element"+std::to_string(element)+"Units")));
               }
               for (int i=0;i<6;++i) {
                  table->item(elementRows[i],0)->setText(labels[i]); table->item(elementRows[i],1)->setText(values[i]); table->item(elementRows[i],2)->setText(units[i]);
               }
               { const QSignalBlocker block(representation); representation->clear(); representation->addItems(available); representation->setCurrentText(nextRepresentation); }
               { const QSignalBlocker block(anomaly); anomaly->clear(); anomaly->addItems(availableAnomalies); anomaly->setCurrentText(nextAnomaly); }
               anomaly->setEnabled(nextRepresentation=="Keplerian" || nextRepresentation=="ModifiedKeplerian");
               *previous={nextFrame,nextRepresentation,nextAnomaly,values,static_cast<Spacecraft *>(preview.get())->GetState().GetState()};
               reportStatus("Orbit values converted. Apply keeps the frame, representation and state together.");
            } catch (BaseException &error) {
               const QSignalBlocker blockRepresentation(representation),blockFrame(frame),blockAnomaly(anomaly);
               representation->setCurrentText(previous->representation); frame->setCurrentText(previous->frame); anomaly->setCurrentText(previous->anomaly);
               reportStatus(QString::fromStdString(error.GetFullMessage()));
            } catch (const std::exception &error) {
               const QSignalBlocker blockRepresentation(representation),blockFrame(frame),blockAnomaly(anomaly);
               representation->setCurrentText(previous->representation); frame->setCurrentText(previous->frame); anomaly->setCurrentText(previous->anomaly);
               reportStatus(QString::fromUtf8(error.what()));
            }
         };
         connect(representation,&QComboBox::currentTextChanged,this,[convert] { convert(); });
         connect(frame,&QComboBox::currentTextChanged,this,[convert] { convert(); });
         connect(anomaly,&QComboBox::currentTextChanged,this,[convert] { convert(); });
      }
   }
   bool hasCellEditor=false;
   for (int row=0;row<table->rowCount();++row) hasCellEditor=hasCellEditor || table->cellWidget(row,3);
   table->setColumnHidden(3,!hasCellEditor);
   fitTableColumns(table);
   QTimer::singleShot(0,table,[this] { fitTableColumns(table,1); });
   connect(table,&QTableWidget::itemDoubleClicked,this,[this](QTableWidgetItem *item) {
      const int rows=item->data(Qt::UserRole).toInt(), columns=item->data(Qt::UserRole+1).toInt();
      if (item->column()!=1 || rows<=0 || columns<=0) return;
      QDialog dialog(this);
      dialog.setObjectName("numericGridDialog");
      dialog.setWindowTitle(table->item(item->row(),0)->text());
      auto *layout=new QVBoxLayout(&dialog);
      auto *grid=new QTableWidget(rows,columns,&dialog);
      grid->setObjectName("numericGrid");
      const bool covariance=item->data(Qt::UserRole+4).toBool();
      if (covariance) {
         // Shorten presentation only. EditRole retains all original digits;
         // opening/accepting the grid must not round the stored covariance.
         grid->setItemDelegate(new CovarianceCellDelegate(grid));
         connect(grid,&QTableWidget::itemChanged,&dialog,[](QTableWidgetItem *cell) { if (cell->toolTip()!=cell->text()) cell->setToolTip(cell->text()); });
         bool keplerian=false;
         for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="SolveFors") keplerian=splitResourceReferences(table->item(row,1)->text()).contains("KeplerianState");
         const QStringList labels=keplerian ? QStringList{"SMA (km)","ECC","INC (deg)","RAAN (deg)","AOP (deg)","MA (deg)"} : QStringList{"X (km)","Y (km)","Z (km)","VX (km/s)","VY (km/s)","VZ (km/s)"};
         grid->setHorizontalHeaderLabels(labels); grid->setVerticalHeaderLabels(labels);
         dialog.setProperty("helpTopic","SpacecraftNavigation");
         auto *help=new QLabel("Enter a symmetric, positive-definite initial state covariance. Each cell's units are the product of its row and column units. Estimation uses EarthMJ2000Eq regardless of the displayed orbit frame; Keplerian solve-fors use mean anomaly (MA). Covariance propagation requires MJ2000Eq axes. EKF warm starts use the input file's covariance; batch estimation uses this matrix only with UseInitialCovariance enabled.",&dialog);
         help->setText(help->text()+" Labels follow pending solve-fors; changing solve-fors does not convert these values.");
         help->setText(help->text()+" Cells show 12 significant digits; editing or hovering shows the full value.");
         help->setWordWrap(true); help->setObjectName("covarianceHelp"); layout->addWidget(help);
         auto *mirror=new QPushButton("Copy upper triangle to lower triangle",&dialog); mirror->setObjectName("covarianceMirrorUpper"); layout->addWidget(mirror);
         connect(mirror,&QPushButton::clicked,&dialog,[grid] { for (int r=0;r<grid->rowCount();++r) for (int c=r+1;c<grid->columnCount();++c) grid->item(c,r)->setText(grid->item(r,c)->text()); });
      }
      const auto values=item->text().split(';');
      for (int r=0;r<rows;++r) {
         const auto cells=values.value(r).trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
         for (int c=0;c<columns;++c) grid->setItem(r,c,new QTableWidgetItem(cells.value(c,"0")));
      }
      const bool vector=item->data(Qt::UserRole+3).toBool();
      if (item->data(Qt::UserRole+2).toBool() || vector) {
         auto *dimensions=new QFormLayout;
         auto *rowCount=new QSpinBox(&dialog),*columnCount=new QSpinBox(&dialog);
         rowCount->setObjectName("arrayRows"); columnCount->setObjectName("arrayColumns");
         rowCount->setRange(1,1000); columnCount->setRange(1,1000);
         rowCount->setMaximum(std::max(1000,rows)); columnCount->setMaximum(std::max(1000,columns));
         rowCount->setValue(rows); columnCount->setValue(columns);
         if (vector) { rowCount->setRange(1,1); rowCount->hide(); }
         else dimensions->addRow("Rows",rowCount);
         dimensions->addRow(vector ? "Coefficients" : "Columns",columnCount);
         layout->addLayout(dimensions);
         layout->addWidget(new QLabel("New cells start at zero. Shrinking removes cells outside the new dimensions.",&dialog));
         auto resize=[=] {
            grid->setRowCount(rowCount->value()); grid->setColumnCount(columnCount->value());
            for (int r=0;r<grid->rowCount();++r) for (int c=0;c<grid->columnCount();++c)
               if (!grid->item(r,c)) grid->setItem(r,c,new QTableWidgetItem("0"));
         };
         connect(rowCount,&QSpinBox::valueChanged,&dialog,resize);
         connect(columnCount,&QSpinBox::valueChanged,&dialog,resize);
      }
      configureTableColumns(grid);
      fitTableColumns(grid);
      layout->addWidget(grid);
      auto *error=new QLabel(&dialog); error->setObjectName("numericGridError"); error->setWordWrap(true); layout->addWidget(error);
      if (item->data(Qt::UserRole+2).toBool()) {
         auto *cellControls=new QWidget(&dialog); auto *row=new QHBoxLayout(cellControls); row->setContentsMargins(0,0,0,0);
         auto *cellRow=new QSpinBox(cellControls),*cellColumn=new QSpinBox(cellControls);
         cellRow->setObjectName("arrayCellRow"); cellColumn->setObjectName("arrayCellColumn");
         cellRow->setRange(1,grid->rowCount()); cellColumn->setRange(1,grid->columnCount());
         auto *cellValue=new QLineEdit(cellControls); cellValue->setObjectName("arrayCellValue");
         auto *set=new QPushButton("Set cell",cellControls); set->setObjectName("arraySetCell");
         row->addWidget(new QLabel("Row",cellControls)); row->addWidget(cellRow);
         row->addWidget(new QLabel("Column",cellControls)); row->addWidget(cellColumn);
         row->addWidget(new QLabel("Value",cellControls)); row->addWidget(cellValue,1); row->addWidget(set);
         layout->insertWidget(layout->indexOf(grid),cellControls);
         auto select=[=] {
            const int r=cellRow->value()-1,c=cellColumn->value()-1;
            auto *cell=grid->item(r,c); if (!cell) return;
            grid->setCurrentCell(r,c); grid->scrollToItem(cell); cellValue->setText(cell->text());
         };
         connect(cellRow,&QSpinBox::valueChanged,&dialog,select); connect(cellColumn,&QSpinBox::valueChanged,&dialog,select);
         connect(grid,&QTableWidget::currentCellChanged,&dialog,[=](int r,int c,int,int) {
            if (r<0 || c<0) return;
            const QSignalBlocker rowBlock(cellRow),columnBlock(cellColumn);
            cellRow->setValue(r+1); cellColumn->setValue(c+1);
            if (auto *cell=grid->item(r,c)) cellValue->setText(cell->text());
         });
         connect(grid,&QTableWidget::itemChanged,&dialog,[=](QTableWidgetItem *cell) {
            if (cell->row()==cellRow->value()-1 && cell->column()==cellColumn->value()-1) cellValue->setText(cell->text());
         });
         const auto dimensionsChanged=[=] {
            cellRow->setMaximum(std::max(1,grid->rowCount())); cellColumn->setMaximum(std::max(1,grid->columnCount()));
         };
         connect(grid->model(),&QAbstractItemModel::rowsInserted,&dialog,dimensionsChanged);
         connect(grid->model(),&QAbstractItemModel::rowsRemoved,&dialog,dimensionsChanged);
         connect(grid->model(),&QAbstractItemModel::columnsInserted,&dialog,dimensionsChanged);
         connect(grid->model(),&QAbstractItemModel::columnsRemoved,&dialog,dimensionsChanged);
         connect(set,&QPushButton::clicked,&dialog,[=] {
            bool valid=false; const double number=cellValue->text().toDouble(&valid);
            if (!valid || !std::isfinite(number)) { error->setText("Enter a finite numeric cell value."); return; }
            auto *cell=grid->item(cellRow->value()-1,cellColumn->value()-1); if (!cell) return;
            cell->setText(QString::number(number,'g',17)); error->clear();
         });
         connect(cellValue,&QLineEdit::returnPressed,set,&QPushButton::click); select();
      }
      auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
      layout->addWidget(buttons);
      connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
      connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
         QStringList output;
         for (int r=0;r<grid->rowCount();++r) {
            QStringList cells;
            for (int c=0;c<grid->columnCount();++c) {
               bool valid=false;
               const double value=grid->item(r,c)->text().toDouble(&valid);
               if (!valid || !std::isfinite(value)) {
                  error->setText(QString("Cell (%1, %2) needs a finite number.").arg(r+1).arg(c+1));
                  grid->setCurrentCell(r,c); return;
               }
               cells.append(QString::number(value,'g',17));
            }
            output.append(cells.join(" "));
         }
         if (covariance) { const auto invalid=orbitCovarianceError(output.join("; ")); if (!invalid.isEmpty()) { error->setText(invalid); return; } }
         item->setData(Qt::UserRole,grid->rowCount());
         item->setData(Qt::UserRole+1,grid->columnCount());
         item->setText(output.join("; ")); dialog.accept();
      });
      dialog.resize(covariance ? QSize(920,520) : QSize(560,320));
      if (covariance) {
         layout->activate(); grid->setProperty("sizingColumns",true);
         for (int c=0;c<columns;++c) grid->setColumnWidth(c,std::max(80,grid->viewport()->width()/columns));
         grid->setProperty("sizingColumns",false);
      }
      dialog.exec();
   });
   if (sections) {
      // Use the engine's current element labels (Cartesian, Keplerian, etc.).
      QSet<QString> orbit={"Epoch","DateFormat","StateType","DisplayStateType","AnomalyType","CoordinateSystem"};
      if (spacecraft) for (const auto &name:spacecraftOrbitElementNames(object)) orbit.insert(name);
      QSet<QString> attitudeFields;
      if (spacecraft) if (auto *attitude=object.GetOwnedObject(0))
         for (const auto &field:resourceProperties(*attitude)) attitudeFields.insert(field.name);
      QSet<QString> present;
      for (int row=0;row<table->rowCount();++row) {
         const auto name=table->item(row,0)->text();
         QString section;
         if (!spacecraft) {
            section="General";
            if (thruster) {
               if (name=="Tank" || name=="MixRatio" || name=="DecrementMass") section="Fuel";
               else if (name.startsWith("ThrustDirection") || name=="CoordinateSystem" || name=="Origin" || name=="Axes") section="Direction";
               else if (name.startsWith("C") || name.startsWith("K") || name.contains("Thrust") || name.contains("Isp") || name=="DutyCycle") section="Performance";
            } else if (power) {
               if (name.startsWith("BusCoeff")) section="Bus coefficients";
               else if (name.startsWith("SolarCoeff")) section="Solar coefficients";
               else if (name.startsWith("Shadow")) section="Shadow";
            } else if (tank) {
               if (name.contains("Mass") || name.contains("Density") || name=="Volume" || name.contains("Pressure") || name.contains("Temperature")) section="Fuel";
            } else if (solver) {
               if (name.contains("Report") || name.contains("Progress") || name.contains("Show")) section="Output";
               else if (name.contains("Tolerance") || name.contains("Iterations") || name.contains("Algorithm") || name.contains("Derivative")) section="Convergence";
            } else if (forces) {
               if (name.startsWith("GravityField.")) section="Gravity field";
               else if (name.startsWith("Drag.")) section="Atmosphere";
               else if (name.startsWith("SRP.")) section="Radiation pressure";
               else if (name.contains("Bodies") || name=="PointMasses" || name=="CentralBody") section="Bodies";
               else if (name.contains("Drag") || name.contains("SRP") || name.contains("Relativistic")) section="Perturbations";
            }
         }
         else if (orbit.contains(name)) section="Orbit";
         else if (name.startsWith("NAIF") || name.contains("Spice")) section="SPICE";
         else if (name=="Attitude" || name.startsWith("Attitude") || attitudeFields.contains(name)) section="Attitude";
         else if (name.startsWith("Model") || name=="OrbitColor" || name=="TargetColor") section="Visualization";
         else if (name.startsWith("Dry") || name.startsWith("System") || name.startsWith("SPAD") ||
                  name.startsWith("AtmosDensity") || name.contains("Mass") || name.contains("Inertia") ||
                  QSet<QString>{"Cd","Cr","CdSigma","CrSigma","DragArea","SRPArea"}.contains(name)) section="Ballistic/Mass";
         else if (name=="PowerSystem") section="Power System";
         else if (name=="Tanks" || name=="Thrusters" || name=="AddHardware" || name=="AddPlates") section="Hardware";
         table->item(row,0)->setData(Qt::UserRole,section);
         if (!section.isEmpty()) present.insert(section);
      }
      for (const auto &section : {"Orbit","Attitude","Ballistic/Mass","Hardware","Power System","SPICE","Visualization",
                                 "General","Bus coefficients","Solar coefficients","Shadow","Bodies","Gravity field","Atmosphere","Radiation pressure","Perturbations","Direction","Fuel","Performance","Convergence","Output"})
         if (present.contains(section)) sections->addTab(section);
      sections->addTab("All Properties");
   }
   if (thruster || impulsive) {
      int frameRow=-1,axesRow=-1,originRow=-1;
      for (int row=0;row<table->rowCount();++row) { const auto name=table->item(row,0)->text(); if (name=="CoordinateSystem") frameRow=row; if (name=="Axes") axesRow=row; if (name=="Origin") originRow=row; }
      if (frameRow>=0 && axesRow>=0 && originRow>=0) {
         auto *axes=qobject_cast<QComboBox *>(table->cellWidget(axesRow,1));
         auto update=[this,frameRow,originRow,axes] {
            if (!axes) return;
            const QSignalBlocker block(table);
            const bool local=table->item(frameRow,1)->text()=="Local"; axes->setEnabled(local);
            const bool origin=local && axes->currentText()!="MJ2000Eq" && axes->currentText()!="SpacecraftBody";
            if (auto *item=table->item(originRow,1)) item->setFlags(origin ? item->flags()|Qt::ItemIsEditable : item->flags() & ~Qt::ItemIsEditable);
            if (auto *picker=table->cellWidget(originRow,3)) picker->setEnabled(origin);
         };
         connect(table,&QTableWidget::itemChanged,this,[update](QTableWidgetItem *) { update(); });
         if (axes) connect(axes,&QComboBox::currentTextChanged,this,update); update();
      }
   }
   if (object.IsOfType("ElectricThruster")) {
      auto *coefficients=findChild<QPushButton *>("thrusterCoefficients");
      for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="ThrustModel") {
         if (auto *model=qobject_cast<QComboBox *>(table->cellWidget(row,1))) {
            auto update=[model,coefficients] { coefficients->setEnabled(model->currentText()=="ThrustMassPolynomial"); };
            connect(model,&QComboBox::currentTextChanged,this,update); update();
         }
      }
   }
   if (object.IsOfType("EphemerisFile")) {
      QMap<QString,int> rows; for (int row=0;row<table->rowCount();++row) rows.insert(table->item(row,0)->text(),row);
      auto combo=[this,rows](const QString &name) { return qobject_cast<QComboBox *>(table->cellWidget(rows.value(name),1)); };
      auto *format=combo("FileFormat"),*epochFormat=combo("EpochFormat"),*start=combo("InitialEpoch"),*end=combo("FinalEpoch");
      auto dependencies=[combo,format] {
         const auto value=format->currentText(); combo("DistanceUnit")->setEnabled(value=="STK-TimePosVel"); combo("IncludeEventBoundaries")->setEnabled(value=="STK-TimePosVel"); combo("OutputFormat")->setEnabled(value=="Code-500"); combo("StepSize")->setEnabled(value!="SPK");
      };
      connect(format,&QComboBox::currentTextChanged,this,[this,rows,combo,dependencies](const QString &next) {
         dependencies();
         if (table->property("ephemerisGroupedUpdate").toBool()) return;
         const auto interpolator=next=="SPK" ? "Hermite" : "Lagrange";
         if (auto *field=combo("Interpolator")) field->setCurrentText(interpolator); else if (rows.contains("Interpolator")) table->item(rows.value("Interpolator"),1)->setText(interpolator);
         auto *step=combo("StepSize"); if (next=="SPK") step->setCurrentText("IntegratorSteps"); else if (next=="Code-500" && step->currentText()=="IntegratorSteps") step->setCurrentText("60");
         const int filename=rows.value("Filename"); EphemerisFile naming("QtEphemerisNaming");
         table->item(filename,1)->setText(QString::fromStdString(naming.GetProperFileName(table->item(filename,1)->text().toStdString(),next.toStdString(),false)));
      }); dependencies();
      connect(epochFormat,&QComboBox::currentTextChanged,this,[this,epochFormat,start,end,previous=epochFormat->currentText()](const QString &next) mutable {
         if (table->property("ephemerisGroupedUpdate").toBool()) { previous=next; return; }
         try {
            QStringList converted;
            for (auto *endpoint:{start,end}) {
               const auto value=endpoint->currentText().trimmed();
               if (value==(endpoint==start ? "InitialSpacecraftEpoch" : "FinalSpacecraftEpoch")) { converted.append(value); continue; }
               Real mjd; std::string result; TimeSystemConverter::Instance()->Convert(previous.toStdString(),-999.999,value.toStdString(),next.toStdString(),mjd,result); converted.append(QString::fromStdString(result));
            }
            start->setCurrentText(converted[0]); end->setCurrentText(converted[1]); previous=next; reportStatus("Both epochs converted. Apply retains the format and dates together.");
         } catch (BaseException &failure) { const QSignalBlocker blocker(epochFormat); epochFormat->setCurrentText(previous); reportStatus("Epoch conversion failed: "+QString::fromStdString(failure.GetFullMessage())); }
      });
   }
   if (object.IsOfType("Array")) connect(table,&QTableWidget::itemChanged,this,[this](QTableWidgetItem *item) {
      if (item->column()!=1 || table->item(item->row(),0)->text()!="RmatValue" || !onArrayDimensions) return;
      const auto rows=item->text().split(';');
      const int columns=rows.first().trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts).size();
      if (columns>0 && columns<=1000 && rows.size()<=1000) onArrayDimensions(rows.size(),columns);
   });
   if (object.IsOfType("CoordinateSystem")) for (int row=0;row<table->rowCount();++row)
      if (table->item(row,0)->text()=="Axes") if (auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,1)))
         connect(choice,&QComboBox::currentTextChanged,this,[this] { coordinateAxisEdits.clear(); });
   // Primary forms and Advanced share one pending snapshot and one Apply.
   // The table remains the backing store for generic controls and converters.
   auto *pages=new QTabWidget(this); pages->setObjectName("resourceEditorTabs");
   auto *primary=new ResourceForm(object,pages,script);
   if (creation) for (auto *label:primary->findChildren<QLabel *>())
      if (label->text()=="Edit settings here. Apply commits them; Close lets you discard pending changes.")
         label->setText("Edit settings here. Create adds the resource; Cancel discards pending settings.");
   if (!primary->isEmpty()) {
      formValues=[primary] { return primary->values(); };
      formEdits=[primary] { return primary->changes(); };
      formOriginal=formValues(); formFields=primary->fields();
      formFields.unite(attitudeNames);
   }
   auto *generic=new QScrollArea(pages); generic->setWidgetResizable(true); generic->setFrameShape(QFrame::NoFrame);
   auto *content=new QWidget(generic); auto *fields=new QFormLayout(content); generic->setWidget(content);
   int common=0;
   for (int row=0;row<table->rowCount();++row) {
      auto *key=table->item(row,0); const auto name=key->text(),section=key->data(Qt::UserRole).toString();
      const bool advanced=name.contains('.') || name.contains("Covariance") || section=="SPICE" || name.startsWith("NAIF");
      if (formFields.contains(name) || advanced || (!primary->isEmpty() && !spacecraft)) { if (formFields.contains(name)) key->setData(Qt::UserRole+1,true); continue; }
      auto *line=new QWidget(content); auto *lineLayout=new QHBoxLayout(line); lineLayout->setContentsMargins(0,0,0,0);
      if (auto *source=qobject_cast<QComboBox *>(table->cellWidget(row,1))) {
         auto *choice=new QComboBox(line); choice->setObjectName("resource_"+name); choice->setEditable(source->isEditable());
         for (int i=0;i<source->count();++i) choice->addItem(source->itemText(i),source->itemData(i)); choice->setCurrentText(source->currentText());
         connect(choice,&QComboBox::currentTextChanged,this,[source](const QString &value) { source->setCurrentText(value); });
         connect(source,&QComboBox::currentTextChanged,choice,[choice](const QString &value) { const QSignalBlocker block(choice); if (choice->findText(value)<0) choice->addItem(value); choice->setCurrentText(value); });
         lineLayout->addWidget(choice);
      } else {
         auto *value=new QLineEdit(table->item(row,1)->text(),line); value->setObjectName("resource_"+name);
         value->setReadOnly(!(table->item(row,1)->flags() & Qt::ItemIsEditable));
         connect(value,&QLineEdit::textChanged,this,[this,row](const QString &text) { if (table->item(row,1)->text()!=text) table->item(row,1)->setText(text); });
         connect(table,&QTableWidget::itemChanged,value,[this,row,value](QTableWidgetItem *item) { if (item==table->item(row,1)) { const QSignalBlocker block(value); value->setText(item->text()); } });
         lineLayout->addWidget(value);
      }
      if (auto *source=qobject_cast<QPushButton *>(table->cellWidget(row,3))) {
         auto *choose=new QPushButton(source->text(),line); connect(choose,&QPushButton::clicked,source,&QPushButton::click); lineLayout->addWidget(choose);
      }
      if (!table->item(row,2)->text().isEmpty()) lineLayout->addWidget(new QLabel(table->item(row,2)->text(),line));
      auto *label=new QLabel(name,content); fields->addRow(label,line);
      connect(table,&QTableWidget::itemChanged,label,[key,label](QTableWidgetItem *item) { if (item==key) label->setText(key->text()); });
      key->setData(Qt::UserRole+1,true); ++common;
   }
   if (common) pages->addTab(generic,spacecraft ? "Orbit and hardware" : "Properties"); else generic->hide();
   if (!primary->isEmpty()) pages->insertTab(0,primary,"Setup"); else primary->hide();
   if (spacecraft && common) pages->setCurrentWidget(generic);
   auto *advanced=new QWidget(pages); auto *advancedLayout=new QVBoxLayout(advanced);
   bool advancedProperties=false; int advancedActions=0;
   for (int row=0;row<table->rowCount();++row) advancedProperties=advancedProperties || !table->item(row,0)->data(Qt::UserRole+1).toBool();
   const QSet<QString> replaced={"editEphemeris","editBurn","editThruster","editDynamicData","editTrackingConfigs","editEventLocator","editGroundStation","editOrbitView","editXYPlot","editGroundTrack","spacecraftAttitude","spacecraftBallisticsMass","spacecraftVisualModel"};
   for (auto *button:findChildren<QPushButton *>(QString(),Qt::FindDirectChildrenOnly)) {
      layout->removeWidget(button);
      if (replaced.contains(button->objectName())) button->hide(); else { advancedLayout->addWidget(button); ++advancedActions; }
   }
   layout->removeWidget(search); advancedLayout->addWidget(search);
   if (sections) { layout->removeWidget(sections); advancedLayout->addWidget(sections); }
   advancedLayout->addWidget(table,1);
   if (!advancedProperties) { search->hide(); table->hide(); if (sections) sections->hide(); advancedLayout->addStretch(); }
   if (advancedProperties || advancedActions) pages->addTab(advanced,"Advanced"); else advanced->hide();
   layout->addWidget(pages,1);
   if (!primary->isEmpty() && !spacecraft) pages->setCurrentWidget(primary);
   status = new QLabel(creation ? "Create validates settings and adds the resource to the mission." : "Apply validates changes and updates the mission script.", this); status->setObjectName("resourceStatus");
   if (object.IsOfType("SeqEstimator")) {
      int formatRow=-1,epochRow=-1;
      for (int row=0;row<table->rowCount();++row) {
         if (table->item(row,0)->text()=="WarmStartEpochFormat") formatRow=row;
         if (table->item(row,0)->text()=="WarmStartEpoch") epochRow=row;
      }
      auto *format=formatRow>=0 ? qobject_cast<QComboBox *>(table->cellWidget(formatRow,1)) : nullptr;
      if (format && epochRow>=0) {
         format->setProperty("previousEpochFormat",format->currentText());
         connect(format,&QComboBox::currentTextChanged,this,[this,format,epochRow](const QString &next) {
            try {
               const auto converted=convertWarmStartEpoch(table->item(epochRow,1)->text(),format->property("previousEpochFormat").toString(),next);
               table->item(epochRow,1)->setText(converted); format->setProperty("previousEpochFormat",next); status->clear();
            } catch (BaseException &failure) { const QSignalBlocker block(format); format->setCurrentText(format->property("previousEpochFormat").toString()); reportStatus(QString::fromStdString(failure.GetFullMessage())); }
            catch (const std::exception &failure) { const QSignalBlocker block(format); format->setCurrentText(format->property("previousEpochFormat").toString()); reportStatus(QString::fromUtf8(failure.what())); }
         });
      }
   }
   if (hasEpochInterval(object)) {
      auto initial=qtResourcePreviewShared(object.Clone()); QMap<QString,int> rows;
      for (int row=0;row<table->rowCount();++row) if (QStringList{"EpochFormat","InitialEpoch","FinalEpoch"}.contains(table->item(row,0)->text())) rows.insert(table->item(row,0)->text(),row);
      auto *format=qobject_cast<QComboBox *>(table->cellWidget(rows.value("EpochFormat"),1));
      if (format && rows.size()==3) {
         format->setToolTip("Changing format converts both endpoints. Gregorian dates use milliseconds; numeric formats retain full precision.");
         auto read=[this,rows,format] { return QMap<QString,QString>{{"EpochFormat",format->currentText()},{"InitialEpoch",table->item(rows.value("InitialEpoch"),1)->text()},{"FinalEpoch",table->item(rows.value("FinalEpoch"),1)->text()}}; };
         auto write=[this,rows,format](const QMap<QString,QString> &values) { const QSignalBlocker block(format); format->setCurrentText(values.value("EpochFormat")); format->setProperty("previousEpochFormat",values.value("EpochFormat")); for (const auto &name:QStringList{"InitialEpoch","FinalEpoch"}) table->item(rows.value(name),1)->setText(values.value(name)); };
         format->setProperty("previousEpochFormat",format->currentText());
         connect(format,&QComboBox::currentTextChanged,this,[this,format,read,write](const QString &next) {
            try { auto values=read(); values["EpochFormat"]=format->property("previousEpochFormat").toString(); write(convertEpochInterval(values,next)); status->clear(); }
            catch (BaseException &failure) { const QSignalBlocker block(format); format->setCurrentText(format->property("previousEpochFormat").toString()); reportStatus(QString::fromStdString(failure.GetFullMessage())); }
            catch (const std::exception &failure) { const QSignalBlocker block(format); format->setCurrentText(format->property("previousEpochFormat").toString()); reportStatus(QString::fromUtf8(failure.what())); }
         });
         auto *button=new QPushButton("Time interval…",this); button->setObjectName("editEpochInterval"); layout->addWidget(button);
         connect(button,&QPushButton::clicked,this,[this,initial,read,write] { EpochIntervalDialog dialog(*initial,read(),this); if (dialog.exec()==QDialog::Accepted) write(dialog.settings()); });
      }
   }
   status->setWordWrap(true); layout->addWidget(status);
   auto *buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close, this);
   buttons->setObjectName("resourceButtons"); applyButton=buttons->button(QDialogButtonBox::Apply);
   layout->addWidget(buttons);
   QString previewText;
   try { previewText=QString::fromStdString(object.GetGeneratingString(Gmat::SHOW_SCRIPT)); }
   catch (BaseException &error) { previewText=QString::fromStdString(error.GetFullMessage()); }
   auto *preview=new QPushButton("Show script…",this); preview->setObjectName("showScript"); buttons->addButton(preview,QDialogButtonBox::ActionRole);
   connect(preview,&QPushButton::clicked,this,[this,previewText] {
      InspectionDialog dialog("Resource script",previewText,"Applied resource. Pending edits are not included.",this); dialog.exec();
   });
   const auto filter=[this,search,sections] {
      const auto section=sections ? sections->tabText(sections->currentIndex()) : QString();
      bool actions=false;
      for (int row=0;row<table->rowCount();++row) {
         const auto *name=table->item(row,0);
         const bool inSection=!sections || section=="All Properties" || name->data(Qt::UserRole).toString()==section;
         table->setRowHidden(row,name->data(Qt::UserRole+1).toBool() || !inSection || !name->text().contains(search->text(),Qt::CaseInsensitive));
         actions=actions || (!table->isRowHidden(row) && table->cellWidget(row,3));
      }
      table->setColumnHidden(3,!actions);
   };
   connect(search, &QLineEdit::textChanged, this, filter);
   if (sections) connect(sections,&QTabBar::currentChanged,this,filter);
   filter();
   connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, [this, apply, applyUnchanged,creation] {
      QMap<QString, QString> changes=coordinateAxisEdits;
      for (auto it=attitudeEdits.cbegin();it!=attitudeEdits.cend();++it) changes[it.key()]=it.value();
      if (formValues) try {
         const auto values=formEdits();
         for (auto it=values.cbegin();it!=values.cend();++it)
            if (!creation || it.value()!=formOriginal.value(it.key()) || !formOriginal.contains(it.key())) changes.insert(it.key(),it.value());
      } catch (BaseException &error) { reportStatus(QString::fromStdString(error.GetFullMessage())); return; }
      catch (const std::exception &error) { reportStatus(QString::fromUtf8(error.what())); return; }
      if (!objectDrawingEdits.isEmpty()) {
         QStringList selected;
         for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="Add") {
            const auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,1)); selected=splitResourceReferences(choice ? comboValue(choice) : table->item(row,1)->text());
         }
         if (formValues) selected=splitResourceReferences(formValues().value("Add"));
         // Drawing edits may predate a pending object removal in Orbit setup.
         for (auto it=objectDrawingEdits.cbegin();it!=objectDrawingEdits.cend();++it) {
            auto prune=[&](auto &values) { for (auto item=values.begin();item!=values.end();) { if (!selected.contains(item.key())) item=values.erase(item); else ++item; } };
            if (it.key()=="@QtObjectMarkerSizes") { auto values=qtObjectSizes(it.value()); prune(values); changes.insert(it.key(),qtObjectSizesJson(values)); }
            else if (it.key()=="@QtObjectLineWidths") { auto values=qtObjectWidths(it.value()); prune(values); changes.insert(it.key(),qtObjectWidthsJson(values)); }
            else if (it.key()=="@QtObjectFontSizes") { auto values=qtObjectFontSizes(it.value()); prune(values); changes.insert(it.key(),qtObjectSizesJson(values)); }
            else if (it.key()=="@QtObjectFontPositions") { auto values=qtObjectFontPositions(it.value()); prune(values); changes.insert(it.key(),qtObjectFontPositionsJson(values)); }
            else { auto values=qtObjectFlags(it.value()); prune(values); changes.insert(it.key(),qtObjectFlagsJson(values)); }
         }
      }
      if (!pendingPolyhedron.isEmpty()) changes.insert("@PolyhedronForces",pendingPolyhedron);
      if (!pendingDynamicData.isEmpty()) changes.insert("@DynamicData",pendingDynamicData);
      if (!pendingTrackingConfigs.isEmpty()) changes.insert("@TrackingConfigs",pendingTrackingConfigs);
      for (auto it=atmosphereEdits.cbegin();it!=atmosphereEdits.cend();++it) changes.insert(it.key(),it.value());
      if (externalEdits!=originalExternal) for (auto it=externalEdits.cbegin();it!=externalEdits.cend();++it) changes.insert(it.key(),it.value());
      for (auto it=stationEdits.cbegin();it!=stationEdits.cend();++it) changes.insert(it.key(),it.value());
      for (auto it=eventEdits.cbegin();it!=eventEdits.cend();++it) changes.insert(it.key(),it.value());
      if (expressions!=originalExpressions) changes.insert("@ArrayExpressions",expressions);
      for (int row = 0; row < table->rowCount(); ++row) {
         const QString name = table->item(row, 0)->text();
         if (changes.contains(name) && formFields.contains(name)) continue;
         if (!attitudeEdits.isEmpty() && attitudeNames.contains(name)) continue;
         if (!atmosphereEdits.isEmpty() && (name=="Drag" || name.startsWith("Drag."))) continue;
         if (!externalEdits.isEmpty() && name.startsWith("External.")) continue;
         if (!pendingPolyhedron.isEmpty() && name.startsWith("PolyhedronGravityModel.")) continue;
         if (stationEdits.contains(name)) continue;
         if (eventEdits.contains(name)) continue;
         const auto *combo = qobject_cast<QComboBox *>(table->cellWidget(row, 1));
         const QString value = combo ? comboValue(combo) : table->item(row, 1)->text();
         if (value != original.value(name)) changes.insert(name, value);
      }
      if (pairedTankEdits && (changes.contains("Tank") || changes.contains("MixRatio"))) {
         for (int row=0;row<table->rowCount();++row) {
            const auto name=table->item(row,0)->text();
            if (name=="Tank" || name=="MixRatio") changes.insert(name,table->item(row,1)->text());
         }
      }
      if (changes.isEmpty() && !applyUnchanged && !creation) { reportStatus("No changes to apply."); return; }
      if (pendingOrbit) {
         bool orbitChanged=false;
         for (int row=0;row<table->rowCount();++row)
            orbitChanged=orbitChanged || (table->item(row,0)->data(Qt::UserRole).toString()=="Orbit" && changes.contains(table->item(row,0)->text()));
         if (orbitChanged) try {
            const auto orbit=pendingOrbit();
            // The cached state supersedes the table snapshot. Include all
            // dependent selectors so paired edits are interpreted together.
            for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->data(Qt::UserRole).toString()=="Orbit") changes.remove(table->item(row,0)->text());
            for (auto it=orbit.cbegin();it!=orbit.cend();++it) changes.insert(it.key(),it.value());
         } catch (BaseException &error) { reportStatus(QString::fromStdString(error.GetFullMessage())); return; }
         catch (const std::exception &error) { reportStatus(QString::fromUtf8(error.what())); return; }
      }
      const QString error = apply(changes);
      if (error.isEmpty()) {
         // The workspace replaces this snapshot with the reconstructed model.
         applied = true;
         appliedSuccessfully();
      } else reportStatus(error);
   });
   connect(buttons, &QDialogButtonBox::rejected, this, [this] { parentWidget()->close(); });
}

bool ResourceEditor::hasChanges() const
{
   if (applied) return false;
   if (scalarValue) return scalarValue()!=originalScalarValue;
   if (formValues) try { if (formValues()!=formOriginal) return true; }
   catch (...) { return true; }
   if (!coordinateAxisEdits.isEmpty() || !attitudeEdits.isEmpty() || !atmosphereEdits.isEmpty() || !stationEdits.isEmpty() || !eventEdits.isEmpty() || !pendingDynamicData.isEmpty() || !pendingTrackingConfigs.isEmpty()) return true;
   if (!externalEdits.isEmpty() && externalEdits!=originalExternal) return true;
   if (!pendingPolyhedron.isEmpty()) return true;
   if (expressions!=originalExpressions || !objectDrawingEdits.isEmpty()) return true;
   for (int row = 0; row < table->rowCount(); ++row) {
      const auto *combo = qobject_cast<QComboBox *>(table->cellWidget(row, 1));
      if (!externalEdits.isEmpty() && table->item(row,0)->text().startsWith("External.")) continue;
      if (!pendingPolyhedron.isEmpty() && table->item(row,0)->text().startsWith("PolyhedronGravityModel.")) continue;
      const QString value = combo ? comboValue(combo) : table->item(row, 1)->text();
      if (value != original.value(table->item(row, 0)->text())) return true;
   }
   return false;
}
void ResourceEditor::reportStatus(const QString &message)
{
   status->setText(message); status->setToolTip(message);
   if (onStatus) onStatus(message);
}
void ResourceEditor::requestApply() { if (applyButton) applyButton->click(); }
void ResourceEditor::resizeArray(int rows,int columns)
{
   if (!table) return;
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="RmatValue") {
      auto *item=table->item(row,1); const auto oldRows=item->text().split(';'); QStringList values;
      for (int r=0;r<rows;++r) {
         const auto oldCells=oldRows.value(r).trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts); QStringList cells;
         for (int c=0;c<columns;++c) cells.append(oldCells.value(c,"0"));
         values.append(cells.join(" "));
      }
      item->setData(Qt::UserRole,rows); item->setData(Qt::UserRole+1,columns); item->setText(values.join("; "));
      return;
   }
}
