#pragma once
#include <QWidget>
#include <QSet>
#include <QStringList>
#include <functional>
class QTreeWidget;
class QTabWidget;
class QMainWindow;
class QDockWidget;
class QComboBox;
class QAction;
class MissionNavigation final : public QWidget
{
public:
   MissionNavigation(QTreeWidget *tree,QTabWidget *tabs,QMainWindow *window);
   void refresh();
   void refreshLabels();
   void setDetailed(bool detailed);
   bool isDetailed() const { return detailed; }
   void setExpansion(int level);
   void showFilter();
   void setDetached(bool detached);
   void restorePlacement();
   QAction *detachAction() const { return detach; }
   std::function<QStringList()> commandTypes;
private:
   QTreeWidget *tree;
   QTabWidget *tabs;
   QDockWidget *dock;
   QComboBox *depth;
   QAction *detach;
   QSet<QString> selectedTypes;
   bool filtering=false,exclude=false,detached=false,moving=false,detailed=false;
};
