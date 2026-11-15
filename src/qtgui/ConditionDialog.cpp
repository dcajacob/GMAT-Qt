#include "ConditionDialog.hpp"
#include "ReportParameterDialog.hpp"
#include "ParameterReferences.hpp"
#include "TableColumns.hpp"
#include <QRegularExpression>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <cmath>
namespace {
const QString operand=R"((?:[A-Za-z_][A-Za-z0-9_.]*(?:\(\s*[1-9][0-9]*\s*,\s*[1-9][0-9]*\s*\))?|[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?))";
const QRegularExpression comparison("^\\s*("+operand+")\\s*(==|~=|<=|>=|<|>)\\s*("+operand+")\\s*$");
const QRegularExpression operandPattern("^"+operand+"$");
}
bool ConditionDialog::supports(const QString &value)
{
   const auto parts=value.split(QRegularExpression("[&|]"));
   if (parts.isEmpty()) return false;
   for (const auto &part:parts) if (!comparison.match(part).hasMatch()) return false;
   return true;
}
ConditionDialog::ConditionDialog(const QString &value,QWidget *parent) : QDialog(parent)
{
   setObjectName("conditionDialog"); setWindowTitle("Edit conditions"); resize(760,360);
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Compare parameters, array elements or numbers. Join rows with AND (&) or OR (|). Select an operand cell to browse parameters.",this);
   help->setWordWrap(true); layout->addWidget(help);
   table=new QTableWidget(0,4,this); table->setObjectName("conditionTable"); table->setHorizontalHeaderLabels({"Join","Left side","Relation","Right side"});
   configureTableColumns(table,{8,22,8,22}); layout->addWidget(table);
   auto *unitNotice=new QLabel(this);unitNotice->setObjectName("conditionUnits");unitNotice->setWordWrap(true);layout->addWidget(unitNotice);
   status=new QLabel(this); status->setObjectName("conditionStatus"); layout->addWidget(status);
   auto *actions=new QHBoxLayout; layout->addLayout(actions);
   auto addButton=[&](const QString &text,const QString &name,auto callback) {
      auto *button=new QPushButton(text,this); button->setObjectName(name); actions->addWidget(button); connect(button,&QPushButton::clicked,this,callback);
   };
   addButton("Add comparison","conditionAdd",[this] { addRow("&","","==",""); validate(); });
   addButton("Remove comparison","conditionRemove",[this] {
      if (table->currentRow()>=0) table->removeRow(table->currentRow()); validate();
   });
   addButton("Choose operand…","conditionChoose",[this] {
      const int row=table->currentRow(),column=table->currentColumn();
      if (row<0 || (column!=1 && column!=3)) { status->setText("Select a left or right operand cell first."); return; }
      ReportParameterDialog dialog({table->item(row,column)->text()},this,ReportParameterDialog::Mode::Single);
      dialog.setProperty("unitContext",parameterUnitDescription(table->item(row,column==1?3:1)->text()));
      if (dialog.exec()!=QDialog::Accepted) return;
      table->item(row,column)->setText(dialog.selection().first());
   });
   actions->addStretch();
   buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   auto joins=QRegularExpression("[&|]").globalMatch(value); QString join;
   for (const auto &part:value.split(QRegularExpression("[&|]"))) {
      const auto match=comparison.match(part); if (!match.hasMatch()) continue;
      addRow(join,match.captured(1),match.captured(2),match.captured(3));
      if (joins.hasNext()) join=joins.next().captured();
   }
   connect(table,&QTableWidget::itemChanged,this,[this] { validate(); });
   connect(table,&QTableWidget::itemSelectionChanged,this,[this] { validate(); });
   validate(); fitTableColumns(table,3);
}
void ConditionDialog::addRow(const QString &join,const QString &left,const QString &relation,const QString &right)
{
   const int row=table->rowCount(); table->insertRow(row);
   auto *logic=new QComboBox(table); logic->addItem("AND","&"); logic->addItem("OR","|"); logic->setCurrentIndex(join=="|" ? 1 : 0); table->setCellWidget(row,0,logic);
   table->setItem(row,1,new QTableWidgetItem(left));
   auto *op=new QComboBox(table); op->addItems({"==","~=","<","<=",">",">="}); op->setCurrentText(relation); table->setCellWidget(row,2,op);
   table->setItem(row,3,new QTableWidgetItem(right)); table->setCurrentCell(row,1);
}
void ConditionDialog::validate()
{
   if(auto *notice=findChild<QLabel *>("conditionUnits")) {
      const int row=table->currentRow();
      if(row>=0 && table->item(row,1) && table->item(row,3)) {
         const auto left=table->item(row,1)->text().trimmed(),right=table->item(row,3)->text().trimmed();
         const auto number=[](const QString &s){bool ok=false;const auto n=s.toDouble(&ok);return ok&&std::isfinite(n);};
         notice->setText("Left: "+parameterUnitDescription(number(left)&&!number(right)?right:left)+"; Right: "+parameterUnitDescription(number(right)&&!number(left)?left:right));
      } else notice->clear();
   }
   bool valid=table->rowCount()>0;
   for (int row=0;row<table->rowCount();++row) {
      table->cellWidget(row,0)->setEnabled(row!=0);
      for (int column:{1,3}) valid=valid && table->item(row,column) && operandPattern.match(table->item(row,column)->text().trimmed()).hasMatch();
   }
   buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
   status->setText(valid ? "Apply in the command editor validates the complete mission." : "Each comparison needs a parameter, array element or number on both sides.");
}
QString ConditionDialog::condition() const
{
   QString result;
   for (int row=0;row<table->rowCount();++row) {
      if (row) result+=" "+static_cast<QComboBox *>(table->cellWidget(row,0))->currentData().toString()+" ";
      result+=table->item(row,1)->text().trimmed()+" "+static_cast<QComboBox *>(table->cellWidget(row,2))->currentText()+" "+table->item(row,3)->text().trimmed();
   }
   return result;
}
