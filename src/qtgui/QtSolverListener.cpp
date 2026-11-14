#include "QtSolverListener.hpp"
#include "TableColumns.hpp"
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPointer>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>
#include <QMap>
#include <cmath>
#include <QJsonObject>

class QtSolverListener final : public ISolverListener
{
public:
   QPointer<QMdiSubWindow> window;
   QPointer<QTableWidget> table;
   QPointer<QLabel> status;
   QMap<QString,int> rows;
   bool active=false;
   void value(const QString &kind,const std::string &name,const QString &current,
              const QString &desired={},const QString &residual={}) {
      if (!table) return;
      if (status) { status->setText("Iterating…"); status->setProperty("converged",QVariant()); }
      const auto key=kind+"\n"+QString::fromStdString(name);
      if (!rows.contains(key)) { rows[key]=table->rowCount(); table->insertRow(table->rowCount()); }
      const QStringList cells={kind,QString::fromStdString(name),current,desired,residual};
      for (int col=0;col<cells.size();++col) {
         auto *item=new QTableWidgetItem(cells[col]); item->setFlags(item->flags() & ~Qt::ItemIsEditable);
         table->setItem(rows[key],col,item);
      }
      fitTableColumns(table);
   }
   void VariabledChanged(std::string name,Real number) override { value("Variable",name,QString::number(number,'g',16)); }
   void VariabledChanged(std::string name,std::string &text) override { value("Variable",name,QString::fromStdString(text)); }
   void ObjectiveChanged(std::string name,Real number) override { value("Objective",name,QString::number(number,'g',16)); }
   void ConstraintChanged(std::string name,Real desired,Real current,Integer condition) override {
      value(condition==0 ? "Goal =" : condition<0 ? "Goal ≤" : "Goal ≥",name,QString::number(current,'g',16),
         QString::number(desired,'g',16),QString::number(condition==1 ? desired-current : current-desired,'g',16));
   }
   void Convergence(bool converged,std::string info) override {
      if (status) {
         status->setText((converged ? "Converged" : "Did not converge")+
            (info.empty() ? QString() : " — "+QString::fromStdString(info)));
         status->setProperty("converged",converged);
      }
   }
};

QtSolverListenerManager::QtSolverListenerManager(QMdiArea *area) : workspace(area) {}
QtSolverListenerManager::~QtSolverListenerManager()=default;
void QtSolverListenerManager::missionStarted()
{
   for (auto &entry:listeners) entry.second->active=false;
}
void QtSolverListenerManager::missionFinished(bool stopped,bool failed)
{
   for (auto &entry:listeners) {
      auto &listener=*entry.second;
      if (!listener.active) {
         // Commands identify listeners by generated source. Editing a mode can
         // produce a new key; retire its old window rather than leave stale
         // results beside the new run. Keep the listener itself alive because
         // engine commands hold non-owning pointers to it.
         delete listener.window.data();
         continue;
      }
      if (listener.status && !listener.status->property("converged").isValid())
         listener.status->setText(stopped ? "Mission stopped" : failed ? "Mission failed" : "Finished without a convergence report");
   }
}
ISolverListener *QtSolverListenerManager::CreateSolverListener(const std::string &name,const std::string &,
      Real x,Real y,Real width,Real height,bool maximized)
{
   auto &listener=listeners[name];
   if (!listener) listener=std::make_unique<QtSolverListener>();
   listener->active=true;
   if (!listener->window) {
      auto *panel=new QWidget;
      auto *layout=new QVBoxLayout(panel);
      listener->status=new QLabel("Iterating…",panel); listener->status->setObjectName("solverStatus");
      listener->status->setWordWrap(true); layout->addWidget(listener->status);
      listener->table=new QTableWidget(panel); listener->table->setObjectName("solverProgress");
      listener->table->setColumnCount(5);
      listener->table->setHorizontalHeaderLabels({"Type","Name","Current","Desired","Residual"});
      configureTableColumns(listener->table,{9,22,18,18,18});
      fitTableColumns(listener->table);
      listener->table->verticalHeader()->hide();
      layout->addWidget(listener->table);
      listener->window=workspace->addSubWindow(panel);
      listener->window->setAttribute(Qt::WA_DeleteOnClose);
      listener->window->setWindowTitle("Solver — "+QString::fromStdString(name).section('\n',0,0));
      listener->window->resize(860,320);
      if (std::isfinite(width) && std::isfinite(height) && width>0 && height>0 && width<=1 && height<=1)
         listener->window->resize(std::max(300,int(width*workspace->width())),std::max(180,int(height*workspace->height())));
      if (std::isfinite(x) && std::isfinite(y) && x>=0 && x<=1 && y>=0 && y<=1)
         listener->window->move(int(x*workspace->width()),int(y*workspace->height()));
   }
   listener->rows.clear(); listener->table->setRowCount(0);
   listener->status->setText("Iterating…"); listener->status->setProperty("converged",QVariant());
   if (maximized) listener->window->showMaximized(); else listener->window->show();
   return listener.get();
}

QJsonArray QtSolverListenerManager::snapshot() const
{
   QJsonArray result;
   for (const auto &entry:listeners) {
      const auto &listener=*entry.second;
      if (!listener.active) continue;
      QJsonArray goals,variables;
      if (listener.table) for(int row=0;row<listener.table->rowCount();++row) {
         const auto text=[&](int col) { const auto *cell=listener.table->item(row,col);return cell?cell->text():QString(); };
         bool numeric=false;const auto current=text(2).toDouble(&numeric);
         QJsonObject value{{"name",text(1)},{"value",numeric?QJsonValue(current):QJsonValue(text(2))}};
         if(text(0)=="Variable") variables.append(value);
         else { value["desired"]=text(3).isEmpty()?QJsonValue():QJsonValue(text(3).toDouble());value["residual"]=text(4).isEmpty()?QJsonValue():QJsonValue(text(4).toDouble());goals.append(value); }
      }
      const auto known=listener.status && listener.status->property("converged").isValid();
      const auto convergence=known?(listener.status->property("converged").toBool()?"converged":"not_converged"):"unavailable";
      result.append(QJsonObject{{"name",QString::fromStdString(entry.first).section('\n',0,0)},{"convergence",convergence},{"native_status",QJsonValue()},{"variables",variables},{"goals",goals},{"iterations",QJsonValue()},{"source","Qt native solver listener"}});
   }
   return result;
}
