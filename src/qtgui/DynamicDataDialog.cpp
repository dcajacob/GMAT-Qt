#include "DynamicDataDialog.hpp"
#include "DynamicDataDisplay.hpp"
#include "ReportParameterDialog.hpp"
#include "ParameterReferences.hpp"
#include "TableColumns.hpp"
#include "Moderator.hpp"
#include "Parameter.hpp"
#include "RgbColor.hpp"
#include "BaseException.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QColorDialog>
#include <QShortcut>
#include <QHeaderView>
#include <QRegularExpression>
#include <QSet>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace {
DDD blank()
{
   DDD cell{}; cell.paramBackgroundColor=0xffffff;
   cell.warnLowerBound=cell.critLowerBound=-9.999e300; cell.warnUpperBound=cell.critUpperBound=9.999e300; return cell;
}
QString encode(const std::vector<std::vector<DDD>> &cells)
{
   QJsonArray rows;
   for (const auto &row:cells) {
      QJsonArray columns;
      for (const auto &cell:row) columns.append(QJsonObject{{"name",QString::fromStdString(cell.paramName)},{"text",static_cast<int>(cell.paramTextColor)},{"background",static_cast<int>(cell.paramBackgroundColor)},{"warnLower",cell.warnLowerBound},{"warnUpper",cell.warnUpperBound},{"critLower",cell.critLowerBound},{"critUpper",cell.critUpperBound}});
      rows.append(columns);
   }
   return QString::fromUtf8(QJsonDocument(rows).toJson(QJsonDocument::Compact));
}
std::vector<std::vector<DDD>> decode(const QString &settings)
{
   QJsonParseError parse; const auto document=QJsonDocument::fromJson(settings.toUtf8(),&parse);
   if (parse.error!=QJsonParseError::NoError || !document.isArray()) throw std::runtime_error("Invalid dynamic data grid.");
   std::vector<std::vector<DDD>> cells; QSet<QString> names; int count=0;
   const QRegularExpression reference("^[A-Za-z][A-Za-z0-9_]*(?:\\.[A-Za-z][A-Za-z0-9_]*)*(?:\\(\\s*[1-9][0-9]*\\s*,\\s*[1-9][0-9]*\\s*\\))?$");
   for (const auto &row:document.array()) {
      if (!row.isArray()) throw std::runtime_error("Invalid dynamic data row.");
      std::vector<DDD> columns;
      for (const auto &entry:row.toArray()) {
         if (++count>100000 || !entry.isObject()) throw std::runtime_error("The dynamic data grid must contain at most 100000 cells.");
         auto cell=blank(); const auto value=entry.toObject(); cell.paramName=value.value("name").toString().trimmed().toStdString(); const auto name=QString::fromStdString(cell.paramName);
         if (!name.isEmpty()) {
            if (!reference.match(name).hasMatch()) throw std::runtime_error("Choose a parameter reference or a numeric array element.");
            if (names.contains(name)) throw std::runtime_error("Each parameter can appear only once in a dynamic data display."); names.insert(name);
            if (auto *parameter=dynamic_cast<Parameter *>(Moderator::Instance()->GetConfiguredObject(cell.paramName))) {
               const bool arrayElement=parameter->IsOfType("Array") && name.contains('(');
               if (!arrayElement && parameter->GetReturnType()!=Gmat::REAL_TYPE && parameter->GetReturnType()!=Gmat::STRING_TYPE) throw std::runtime_error("Dynamic data cells require real or string parameters; select an array element instead of the whole array.");
            }
         }
         auto color=[&](const char *key) {
            const auto number=value.value(key); const double code=number.toDouble(-1);
            if (!number.isDouble() || code<0 || code>0xffffff || std::floor(code)!=code) throw std::runtime_error("Choose a valid RGB color."); return static_cast<UnsignedInt>(code);
         };
         cell.paramTextColor=color("text"); cell.paramBackgroundColor=color("background"); cell.isTextColorUserSet=cell.paramTextColor!=0;
         auto bound=[&](const char *key) { const auto number=value.value(key); const double result=number.toDouble(); if (!number.isDouble() || !std::isfinite(result)) throw std::runtime_error("Enter finite warning and critical bounds."); return result; };
         cell.warnLowerBound=bound("warnLower"); cell.warnUpperBound=bound("warnUpper"); cell.critLowerBound=bound("critLower"); cell.critUpperBound=bound("critUpper");
         if (cell.warnLowerBound>cell.warnUpperBound || cell.critLowerBound>cell.critUpperBound) throw std::runtime_error("Each lower bound must be no greater than its upper bound.");
         if (name.isEmpty()) cell=blank();
         columns.push_back(cell);
      }
      cells.push_back(columns);
   }
   return cells;
}
QColor color(UnsignedInt value) { RgbColor rgb(value); return QColor(rgb.Red(),rgb.Green(),rgb.Blue()); }
QPushButton *colorButton(QWidget *parent,const QString &name,UnsignedInt initial)
{
   auto *button=new QPushButton(parent); button->setObjectName(name);
   auto update=[button](const QColor &value) { button->setProperty("rgb",value); button->setText(value.name()); button->setStyleSheet("QPushButton { border-left: 14px solid "+value.name()+"; }"); };
   update(color(initial)); QObject::connect(button,&QPushButton::clicked,parent,[parent,button,update] { QColorDialog picker(button->property("rgb").value<QColor>(),parent); picker.setObjectName("dynamicColorPicker"); if (picker.exec()==QDialog::Accepted) update(picker.selectedColor()); }); return button;
}
UnsignedInt rgb(QPushButton *button) { const auto value=button->property("rgb").value<QColor>(); return RgbColor(value.red(),value.green(),value.blue()).GetIntColor(); }
}
QString dynamicDataSettings(GmatBase &object)
{
   auto *display=dynamic_cast<DynamicDataDisplay *>(&object); if (!display) throw std::runtime_error("Select a dynamic data display."); return encode(display->GetDynamicDataStruct());
}
void applyDynamicDataSettings(GmatBase &object,const QString &settings)
{
   auto *display=dynamic_cast<DynamicDataDisplay *>(&object); if (!display) throw std::runtime_error("Grid settings require a dynamic data display."); display->SetParamSettings(decode(settings));
}
DynamicDataDialog::DynamicDataDialog(GmatBase &object,const QMap<QString,QString> &pending,QWidget *parent) : QDialog(parent)
{
   setObjectName("dynamicDataDialog"); setWindowTitle("Dynamic data display — "+QString::fromStdString(object.GetName())); resize(760,540);
   cells=decode(pending.value("@DynamicData",dynamicDataSettings(object)));
   auto *layout=new QVBoxLayout(this); auto *help=new QLabel("Resize the grid, then double-click a cell to choose its parameter, colors and bounds. OK keeps changes pending until resource Apply.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *sizing=new QHBoxLayout; layout->addLayout(sizing); rows=new QSpinBox(this); rows->setObjectName("dynamicRows"); columns=new QSpinBox(this); columns->setObjectName("dynamicColumns");
   int width=0; for (const auto &row:cells) width=std::max(width,static_cast<int>(row.size())); rows->setRange(0,std::max(1000,static_cast<int>(cells.size()))); columns->setRange(0,std::max(1000,width)); rows->setValue(cells.size()); columns->setValue(width);
   sizing->addWidget(new QLabel("Rows",this)); sizing->addWidget(rows); sizing->addWidget(new QLabel("Columns",this)); sizing->addWidget(columns); auto *resize=new QPushButton("Resize grid",this); resize->setObjectName("dynamicResize"); sizing->addWidget(resize); sizing->addStretch();
   grid=new QTableWidget(this); grid->setObjectName("dynamicGrid"); configureTableColumns(grid); grid->setEditTriggers(QAbstractItemView::NoEditTriggers); layout->addWidget(grid,1);
   auto *actions=new QHBoxLayout; layout->addLayout(actions); auto *edit=new QPushButton("Edit cell…",this); edit->setObjectName("dynamicEditCell"); actions->addWidget(edit); auto *clear=new QPushButton("Clear selected",this); clear->setObjectName("dynamicClear"); actions->addWidget(clear); actions->addStretch();
   auto *colors=new QFormLayout; layout->addLayout(colors);
   for (const auto &name:QStringList{"WarnColor","CritColor"}) { auto *button=colorButton(this,"dynamic_"+name,RgbColor::ToIntColor(pending.value(name,QString::fromStdString(object.GetStringParameter(name.toStdString()))).toStdString())); colors->addRow(name=="WarnColor" ? "Warning color" : "Critical color",button); }
   error=new QLabel(this); error->setObjectName("dynamicError"); error->setWordWrap(true); layout->addWidget(error);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(resize,&QPushButton::clicked,this,[this] { resizeGrid(); }); connect(grid,&QTableWidget::cellDoubleClicked,this,[this](int r,int c) { editCell(r,c); }); connect(edit,&QPushButton::clicked,this,[this] { if (grid->currentRow()>=0) editCell(grid->currentRow(),grid->currentColumn()); });
   auto clearCells=[this] { for (auto *item:grid->selectedItems()) cells[item->row()][item->column()]=blank(); refresh(); }; connect(clear,&QPushButton::clicked,this,clearCells); auto *remove=new QShortcut(QKeySequence::Delete,grid); connect(remove,&QShortcut::activated,this,clearCells);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject); connect(buttons,&QDialogButtonBox::accepted,this,[this] { try { decode(settings().value("@DynamicData")); accept(); } catch (const std::exception &failure) { error->setText(QString::fromUtf8(failure.what())); } }); refresh();
}
void DynamicDataDialog::resizeGrid()
{
   if (static_cast<qint64>(rows->value())*columns->value()>100000) { error->setText("Choose dimensions containing at most 100000 cells."); return; }
   cells.resize(rows->value()); for (auto &row:cells) row.resize(columns->value(),blank()); refresh(); error->clear();
}
void DynamicDataDialog::refresh()
{
   const int r=grid->currentRow(),c=grid->currentColumn(); int width=0; for (const auto &row:cells) width=std::max(width,static_cast<int>(row.size()));
   for (auto &row:cells) row.resize(width,blank()); grid->setRowCount(cells.size()); grid->setColumnCount(width);
   for (int i=0;i<static_cast<int>(cells.size());++i) for (int j=0;j<width;++j) { auto *item=new QTableWidgetItem(QString::fromStdString(cells[i][j].paramName)); item->setForeground(color(cells[i][j].paramTextColor)); item->setBackground(color(cells[i][j].paramBackgroundColor)); grid->setItem(i,j,item); }
   fitTableColumns(grid); if (!cells.empty() && width) grid->setCurrentCell(std::clamp(r,0,static_cast<int>(cells.size())-1),std::clamp(c,0,width-1));
}
void DynamicDataDialog::editCell(int row,int column)
{
   auto proposed=cells[row][column]; QDialog dialog(this); dialog.setObjectName("dynamicCellDialog"); dialog.setWindowTitle(QString("Cell %1, %2").arg(row+1).arg(column+1)); auto *layout=new QVBoxLayout(&dialog); auto *form=new QFormLayout; layout->addLayout(form);
   auto *parameter=new QLineEdit(QString::fromStdString(proposed.paramName),&dialog); parameter->setObjectName("dynamicParameter"); auto *selection=new QWidget(&dialog); auto *selectLayout=new QHBoxLayout(selection); selectLayout->setContentsMargins(0,0,0,0); selectLayout->addWidget(parameter); auto *select=new QPushButton("Select…",selection); select->setObjectName("dynamicSelectParameter"); selectLayout->addWidget(select); form->addRow("Parameter",selection);
   connect(select,&QPushButton::clicked,&dialog,[&] { ReportParameterDialog picker({parameter->text()},&dialog,ReportParameterDialog::Mode::Single); if (picker.exec()==QDialog::Accepted) parameter->setText(picker.selection().value(0)); });
   auto *text=colorButton(&dialog,"dynamicTextColor",proposed.paramTextColor),*background=colorButton(&dialog,"dynamicBackgroundColor",proposed.paramBackgroundColor); form->addRow("Text color",text); form->addRow("Background color",background); auto *hint=new QLabel("Black text uses the warning and critical colors for numeric values. A custom text color overrides the bounds colors.",&dialog); hint->setWordWrap(true); layout->addWidget(hint);
   QMap<QString,QLineEdit *> bounds;
   QMap<QString,QLabel *> boundLabels;
   const QMap<QString,QString> boundNames={{"warnLower","Warning lower"},{"warnUpper","Warning upper"},
      {"critLower","Critical lower"},{"critUpper","Critical upper"}};
   for (const auto &entry:QList<QPair<QString,double>>{{"warnLower",proposed.warnLowerBound},{"warnUpper",proposed.warnUpperBound},{"critLower",proposed.critLowerBound},{"critUpper",proposed.critUpperBound}}) {
      auto *field=new QLineEdit(QString::number(entry.second,'g',17),&dialog); field->setObjectName("dynamic_"+entry.first); bounds.insert(entry.first,field);
      auto *label=new QLabel(&dialog); label->setObjectName("dynamic_"+entry.first+"Unit"); label->setBuddy(field);
      boundLabels.insert(entry.first,label); form->addRow(label,field);
   }
   auto updateBoundUnits=[parameter,boundLabels,boundNames] {
      const auto unit=parameterUnitDescription(parameter->text());
      for (auto it=boundLabels.cbegin();it!=boundLabels.cend();++it)
         it.value()->setText(boundNames.value(it.key())+" ("+unit+")");
   };
   connect(parameter,&QLineEdit::textChanged,&dialog,[updateBoundUnits] { updateBoundUnits(); }); updateBoundUnits();
   auto *message=new QLabel(&dialog); message->setObjectName("dynamicCellError"); message->setWordWrap(true); layout->addWidget(message); auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
      try {
         proposed.paramName=parameter->text().trimmed().toStdString(); proposed.paramTextColor=rgb(text); proposed.isTextColorUserSet=proposed.paramTextColor!=0; proposed.paramBackgroundColor=rgb(background);
         auto number=[&](const QString &name) { bool ok; const double value=bounds.value(name)->text().toDouble(&ok); if (!ok || !std::isfinite(value)) throw std::runtime_error("Enter finite numeric bounds."); return value; };
         proposed.warnLowerBound=number("warnLower"); proposed.warnUpperBound=number("warnUpper"); proposed.critLowerBound=number("critLower"); proposed.critUpperBound=number("critUpper"); auto trial=cells; trial[row][column]=proposed; decode(encode(trial)); if (proposed.paramName.empty()) proposed=blank(); dialog.accept();
      } catch (const std::exception &failure) { message->setText(QString::fromUtf8(failure.what())); }
   }); dialog.resize(560,430); if (dialog.exec()==QDialog::Accepted) { cells[row][column]=proposed; refresh(); error->clear(); }
}
QMap<QString,QString> DynamicDataDialog::settings() const
{
   QMap<QString,QString> result{{"@DynamicData",encode(cells)}}; for (const auto &name:QStringList{"WarnColor","CritColor"}) result.insert(name,QString::fromStdString(RgbColor::ToRgbString(rgb(findChild<QPushButton *>("dynamic_"+name))))); return result;
}
