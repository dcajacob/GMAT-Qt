#include "PropagationStopsDialog.hpp"
#include "PropagationOptions.hpp"
#include "ReportParameterDialog.hpp"
#include "PropagationGroupsDialog.hpp"
#include "TableColumns.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QRegularExpression>
#include <QSignalBlocker>

namespace {
bool isApsisStop(const QString &name) { return name.endsWith(".Periapsis") || name.endsWith(".Apoapsis"); }
bool parameter(const QString &name) { return QRegularExpression("^[A-Za-z][A-Za-z0-9_.]*$").match(name).hasMatch(); }
struct Stops { bool valid=false; int start=0,length=0,braceStart=-1,braceLength=0; QList<QPair<QString,QString>> rows; QStringList options; };
Stops parse(const QString &source)
{
   Stops result;
   const auto match=QRegularExpression("^\\s*Propagate\\s+([^{};\\n]+?)(\\s*\\{([^{};]*)\\})?\\s*;[ \\t]*(?:%[^\\n]*)?\\s*$").match(source);
   if (!match.hasMatch() || !PropagationGroupsDialog::supports(source)) return result;
   result.braceStart=match.capturedStart(2); result.braceLength=match.capturedLength(2);
   result.start=result.braceStart<0 ? match.capturedEnd(1) : match.capturedStart(3); result.length=match.capturedLength(3);
   if (result.braceStart<0) { result.valid=true; return result; }
   const auto body=match.captured(3); QStringList entries; int depth=0,begin=0;
   for (int i=0;i<body.size();++i) {
      if (body[i]=='(' || body[i]=='[') ++depth;
      if (body[i]==')' || body[i]==']') { if (--depth<0) return result; }
      if (body[i]==',' && depth==0) { entries.append(body.mid(begin,i-begin).trimmed()); begin=i+1; }
   }
   if (depth!=0) return result; entries.append(body.mid(begin).trimmed());
   if (entries.size()>1 && entries.first().isEmpty()) {
      for (int i=1;i<entries.size();++i) if (!QRegularExpression("^(?:StopTolerance|OrbitColor)\\s*=").match(entries[i]).hasMatch()) return result;
      entries.removeFirst();
   }
   for (const auto &entry:entries) {
      const int equal=entry.indexOf('='); const auto name=(equal<0 ? entry : entry.left(equal)).trimmed();
      const auto value=equal<0 ? QString() : entry.mid(equal+1).trimmed();
      if (name=="StopTolerance" || name=="OrbitColor") {
         if (value.isEmpty()) return result; result.options.append(entry); continue;
      }
      if (!parameter(name) || (isApsisStop(name) ? equal>=0 : value.isEmpty())) return result;
      result.rows.append({name,value});
   }
   result.valid=true; return result;
}
}
bool PropagationStopsDialog::supports(const QString &source) { return parse(source).valid; }
PropagationStopsDialog::PropagationStopsDialog(const QString &source,QWidget *parent) : QDialog(parent),source(source)
{
   setObjectName("propagationStopsDialog"); setWindowTitle("Propagation stopping conditions"); resize(720,430);
   const auto parsed=parse(source); start=parsed.start; length=parsed.length; braceStart=parsed.braceStart; braceLength=parsed.braceLength; options=parsed.options;
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Propagation stops when any condition is satisfied. Remove all conditions to advance one integration step. Periapsis and apoapsis take no goal. OK updates the command; Apply validates the complete mission.",this);
   help->setWordWrap(true); layout->addWidget(help);
   table=new QTableWidget(0,2,this); table->setObjectName("propagationStopsTable"); table->setHorizontalHeaderLabels({"Parameter","Goal"});
   table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection); table->verticalHeader()->hide();
   configureTableColumns(table,{32,24}); layout->addWidget(table);
   auto addRow=[this](const QString &name,const QString &goal) {
      const int row=table->rowCount(); table->insertRow(row); table->setItem(row,0,new QTableWidgetItem(name)); table->setItem(row,1,new QTableWidgetItem(goal)); table->selectRow(row);
   };
   for (const auto &row:parsed.rows) addRow(row.first,row.second); fitTableColumns(table);
   auto *controls=new QHBoxLayout; layout->addLayout(controls);
   auto button=[&](const QString &label,const QString &name) { auto *b=new QPushButton(label,this); b->setObjectName(name); controls->addWidget(b); return b; };
   auto *add=button("Add","propagationStopAdd"),*remove=button("Remove","propagationStopRemove");
   auto *up=button("Up","propagationStopUp"),*down=button("Down","propagationStopDown");
   auto *choose=button("Choose parameter…","propagationStopChoose"),*goal=button("Choose goal…","propagationGoalChoose");
   auto *error=new QLabel(this); error->setObjectName("propagationStopsError"); error->setWordWrap(true); layout->addWidget(error);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   auto refresh=[=] {
      const QSignalBlocker block(table); const int row=table->currentRow();
      remove->setEnabled(row>=0); up->setEnabled(row>0); down->setEnabled(row>=0 && row+1<table->rowCount()); choose->setEnabled(row>=0);
      goal->setEnabled(row>=0 && table->item(row,0) && !isApsisStop(table->item(row,0)->text().trimmed()));
      bool valid=parsed.valid;
      for (int r=0;r<table->rowCount();++r) {
         if (!table->item(r,0) || !table->item(r,1)) { valid=false; continue; }
         const auto name=table->item(r,0)->text().trimmed(); auto *value=table->item(r,1);
         const bool isEvent=isApsisStop(name); value->setFlags(isEvent ? value->flags() & ~Qt::ItemIsEditable : value->flags() | Qt::ItemIsEditable);
         if (isEvent && !value->text().isEmpty()) { value->setData(Qt::UserRole,value->text()); value->setText({}); }
         else if (!isEvent && value->text().isEmpty() && value->data(Qt::UserRole).isValid()) { value->setText(value->data(Qt::UserRole).toString()); value->setData(Qt::UserRole,{}); }
         value->setToolTip(isEvent ? "This event takes no goal value" : "Numeric value or parameter expression");
         valid=valid && parameter(name) && (isEvent || !value->text().trimmed().isEmpty());
      }
      error->setText(valid && table->rowCount()==0 ? "Single step: advance once using the propagator’s current integration step." : QString());
      buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
   };
   connect(add,&QPushButton::clicked,this,[=] { addRow({},{}); refresh(); });
   connect(remove,&QPushButton::clicked,this,[=] { table->removeRow(table->currentRow()); refresh(); });
   auto move=[=](int delta) {
      const int row=table->currentRow(),target=row+delta; if (row<0 || target<0 || target>=table->rowCount()) return;
      const QSignalBlocker block(table);
      for (int c=0;c<2;++c) { auto *a=table->takeItem(row,c),*b=table->takeItem(target,c); table->setItem(row,c,b); table->setItem(target,c,a); }
      table->selectRow(target); refresh();
   };
   connect(up,&QPushButton::clicked,this,[=] { move(-1); }); connect(down,&QPushButton::clicked,this,[=] { move(1); });
   auto pick=[=](int column) {
      const int row=table->currentRow(); if (row<0) return;
      ReportParameterDialog dialog({table->item(row,column)->text()},this,column==0 ? ReportParameterDialog::Mode::StopParameter : ReportParameterDialog::Mode::Single);
      if (dialog.exec()==QDialog::Accepted && !dialog.selection().isEmpty()) table->item(row,column)->setText(dialog.selection().first());
   };
   connect(choose,&QPushButton::clicked,this,[=] { pick(0); }); connect(goal,&QPushButton::clicked,this,[=] { pick(1); });
   connect(table,&QTableWidget::itemChanged,this,[=] { refresh(); }); connect(table,&QTableWidget::itemSelectionChanged,this,refresh);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[=] {
      if (const auto candidate=parse(statement()); !candidate.valid || candidate.rows.size()!=table->rowCount() || candidate.options!=(table->rowCount()==0 ? singleStepOptions(options) : options)) { error->setText("Check the parameter names and goals. Use the source editor for unsupported expressions."); return; }
      accept();
   }); refresh();
}
QString PropagationStopsDialog::statement() const
{
   QStringList rows;
   for (int row=0;row<table->rowCount();++row) {
      const auto name=table->item(row,0)->text().trimmed();
      rows.append(name+(isApsisStop(name) ? QString() : " = "+table->item(row,1)->text().trimmed()));
   }
   const auto original=parse(source);
   if (original.rows.size()==table->rowCount()) {
      bool same=true;
      for (int row=0;row<table->rowCount();++row) same=same && table->item(row,0)->text().trimmed()==original.rows[row].first && (isApsisStop(original.rows[row].first) || table->item(row,1)->text().trimmed()==original.rows[row].second);
      if (same) return source;
   }
   rows.append(table->rowCount()==0 ? singleStepOptions(options) : options); QString result=source;
   if (braceStart<0) { if (!rows.isEmpty()) result.insert(start," {"+rows.join(", ")+"}"); }
   else if (rows.isEmpty()) result.remove(braceStart,braceLength);
   else result.replace(start,length,rows.join(", "));
   return result;
}
