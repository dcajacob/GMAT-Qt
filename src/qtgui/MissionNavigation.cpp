#include "MissionNavigation.hpp"
#include <QTreeWidget>
#include <QTabWidget>
#include <QMainWindow>
#include <QDockWidget>
#include <QToolBar>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCloseEvent>
#include <QSettings>
#include <algorithm>

namespace {
QString engineType(QString type) {
   if (type=="Equation" || type=="Assignment") return "GMAT";
   if (type=="ScriptEvent") return "BeginScript";
   return type;
}
QString displayType(QString type) {
   type=engineType(type);
   return type=="GMAT" ? "Equation" : type=="BeginScript" ? "ScriptEvent" : type;
}
class MissionDock final : public QDockWidget {
public:
   MissionDock(QMainWindow *parent,std::function<void()> restore) : QDockWidget("Mission",parent),restore(std::move(restore)) {}
   void closeEvent(QCloseEvent *event) override { event->ignore(); restore(); }
private:
   std::function<void()> restore;
};
}
MissionNavigation::MissionNavigation(QTreeWidget *tree,QTabWidget *tabs,QMainWindow *window)
   : QWidget(tabs),tree(tree),tabs(tabs)
{
   setObjectName("missionNavigation"); setProperty("helpTopic","MissionTree");
   const int index=tabs->indexOf(tree); tabs->removeTab(index);
   auto *layout=new QVBoxLayout(this); layout->setContentsMargins(0,0,0,0);
   auto *tools=new QToolBar(this); tools->setObjectName("missionTreeTools"); layout->addWidget(tools);
   auto *all=tools->addAction("All"); all->setObjectName("missionShowAll"); all->setToolTip("Show all mission commands");
   connect(all,&QAction::triggered,this,[this] { filtering=false; refresh(); });
   auto *filter=tools->addAction("Filter…"); filter->setObjectName("missionFilter"); connect(filter,&QAction::triggered,this,&MissionNavigation::showFilter);
   depth=new QComboBox(tools); depth->setObjectName("missionDepth"); depth->setToolTip("Mission tree expansion level"); depth->addItem("Collapsed",0);
   for (int i=1;i<=3;++i) depth->addItem(QString("Level %1").arg(i),i);
   depth->addItem("Expand all",-1); depth->setCurrentIndex(3); tools->addWidget(depth);
   connect(depth,&QComboBox::currentIndexChanged,this,[this] { refresh(); });
   layout->addWidget(tree,1); tree->show(); tabs->insertTab(index,this,"Mission");
   detach=tools->addAction("Undock"); detach->setObjectName("undockMissionTree"); detach->setCheckable(true);
   connect(detach,&QAction::triggered,this,[this](bool checked) { setDetached(checked); });
   dock=new MissionDock(window,[this] { setDetached(false); }); dock->setObjectName("missionDock"); dock->resize(420,650);
   window->addDockWidget(Qt::LeftDockWidgetArea,dock); dock->hide();
   connect(dock,&QDockWidget::topLevelChanged,this,[this](bool floating) { if (!moving && detached && !floating) setDetached(false); });
}
void MissionNavigation::setDetached(bool value)
{
   if (moving || value==detached) return;
   moving=true; detached=value; detach->setChecked(value); detach->setText(value ? "Dock" : "Undock");
   if (value) {
      tabs->removeTab(tabs->indexOf(this)); dock->setWidget(this); dock->setFloating(true); dock->show(); dock->raise(); tree->setFocus();
   } else {
      dock->setWidget(nullptr); setParent(tabs); tabs->insertTab(std::min(1,int(tabs->count())),this,"Mission"); tabs->setCurrentWidget(this); show(); dock->hide(); tree->setFocus();
   }
   moving=false;
   QSettings().setValue("MissionTree/detached",detached);
}
void MissionNavigation::restorePlacement()
{
   if (QSettings().value("MissionTree/detached",false).toBool()) setDetached(true);
   else dock->hide();
}
void MissionNavigation::setExpansion(int level)
{
   const int index=depth->findData(level);
   if (index>=0) { depth->setCurrentIndex(index); refresh(); }
}
void MissionNavigation::setDetailed(bool value)
{
   detailed=value; refreshLabels();
}
void MissionNavigation::refreshLabels()
{
   std::function<void(QTreeWidgetItem *)> label=[&](QTreeWidgetItem *item) {
      if (item->data(0,Qt::UserRole+3).isValid()) {
         const auto text=item->data(0,Qt::UserRole+(detailed ? 4 : 3)).toString();
         item->setText(0,(item->data(0,Qt::UserRole+2).toBool() ? QString("● ") : QString())+text);
      }
      for (int i=0;i<item->childCount();++i) label(item->child(i));
   };
   for (int i=0;i<tree->topLevelItemCount();++i) label(tree->topLevelItem(i));
}
void MissionNavigation::refresh()
{
   refreshLabels();
   std::function<bool(QTreeWidgetItem *)> visible=[&](QTreeWidgetItem *item) {
      bool descendant=false;
      for (int i=0;i<item->childCount();++i) descendant=visible(item->child(i)) || descendant;
      const auto type=engineType(item->data(0,Qt::UserRole+1).toString());
      const bool structural=type.isEmpty() || type=="BeginMissionSequence";
      const bool own=!filtering || selectedTypes.contains(type)!=exclude;
      const bool show=structural || own || descendant; item->setHidden(!show);
      // Keep branch terminators beside a retained branch or matching child.
      if (show) for (int i=0;i<item->childCount();++i) {
         const auto childType=item->child(i)->data(0,Qt::UserRole+1).toString();
         if (childType.startsWith("End") || childType=="Else") item->child(i)->setHidden(false);
      }
      return show;
   };
   for (int i=0;i<tree->topLevelItemCount();++i) visible(tree->topLevelItem(i));
   if (auto *current=tree->currentItem()) {
      while (current && current->isHidden()) current=current->parent();
      if (current) tree->setCurrentItem(current);
   }
   tree->collapseAll(); const int level=depth->currentData().toInt();
   if (level<0) tree->expandAll(); else if (level>0) tree->expandToDepth(level-1);
}
void MissionNavigation::showFilter()
{
   QDialog dialog(this); dialog.setObjectName("missionFilterDialog"); dialog.setWindowTitle("Mission command filter"); dialog.setProperty("helpTopic","MissionTree"); dialog.resize(390,520);
   auto *layout=new QVBoxLayout(&dialog); auto *mode=new QComboBox(&dialog); mode->setObjectName("missionFilterMode"); mode->addItems({"Include checked commands","Exclude checked commands"}); mode->setCurrentIndex(exclude ? 1 : 0); layout->addWidget(mode);
   auto *list=new QListWidget(&dialog); list->setObjectName("missionFilterTypes"); layout->addWidget(list,1);
   auto types=commandTypes ? commandTypes() : QStringList();
   std::function<void(QTreeWidgetItem *)> add=[&](QTreeWidgetItem *item) { const auto type=item->data(0,Qt::UserRole+1).toString(); if (!type.isEmpty()) types.append(type); for (int i=0;i<item->childCount();++i) add(item->child(i)); };
   for (int i=0;i<tree->topLevelItemCount();++i) add(tree->topLevelItem(i));
   for (auto &type:types) type=displayType(type);
   types.removeAll("NoOp"); types.removeAll("BeginMissionSequence"); types.removeDuplicates(); types.sort();
   for (const auto &type:types) { auto *item=new QListWidgetItem(type,list); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(selectedTypes.contains(engineType(type)) ? Qt::Checked : Qt::Unchecked); }
   auto *bulk=new QHBoxLayout; layout->addLayout(bulk);
   for (bool check:{true,false}) { auto *button=new QPushButton(check ? "Check all" : "Uncheck all",&dialog); button->setObjectName(check ? "missionFilterCheckAll" : "missionFilterUncheckAll"); bulk->addWidget(button); connect(button,&QPushButton::clicked,&dialog,[list,check] { for (int i=0;i<list->count();++i) list->item(i)->setCheckState(check ? Qt::Checked : Qt::Unchecked); }); }
   auto *note=new QLabel("Branches containing a matching command remain visible with their boundaries. This changes the tree view; the mission source is retained.",&dialog); note->setWordWrap(true); layout->addWidget(note);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Close,&dialog); layout->addWidget(buttons);
   connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,&dialog,[this,list,mode] { selectedTypes.clear(); for (int i=0;i<list->count();++i) if (list->item(i)->checkState()==Qt::Checked) selectedTypes.insert(engineType(list->item(i)->text())); exclude=mode->currentIndex()==1; filtering=true; refresh(); });
   connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject); dialog.exec();
}
