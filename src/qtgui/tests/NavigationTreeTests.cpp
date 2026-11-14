#include "NavigationTree.hpp"
#include <QAccessible>
#include <QApplication>
#include <QLineEdit>
#include <QTest>
#include <QVBoxLayout>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool result,const char *message)
{
   if (!result) throw std::runtime_error(message);
}
void events() { QApplication::processEvents();QTest::qWait(20); }
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   try {
      QWidget window;
      auto *layout=new QVBoxLayout(&window);
      auto *tree=new NavigationTree(&window);
      tree->setHeaderHidden(true);
      layout->addWidget(tree);
      auto *edit=new QLineEdit(&window);layout->addWidget(edit);
      auto *root=new QTreeWidgetItem(tree,{"Resources"});
      auto *group=new QTreeWidgetItem(root,{"Spacecraft"});
      auto *alpha=new QTreeWidgetItem(group,{"Alpha"});
      auto *beta=new QTreeWidgetItem(group,{"Beta"});
      auto *propagators=new QTreeWidgetItem(root,{"Propagators"});
      auto *prop=new QTreeWidgetItem(propagators,{"P"});
      tree->expandAll();window.resize(500,500);window.show();events();
      auto *accessible=QAccessible::queryAccessibleInterface(tree);
      auto *table=accessible->tableInterface();
      require(table,"Navigation tree table interface is missing");
      const auto check=[&](QTreeWidgetItem *item) {
         const auto selected=table->selectedCells();
         require(selected.size()==1 && selected[0]->text(QAccessible::Name)==item->text(0),"Accessibility reports a different selected item");
         const auto cells=table->selectedCells();
         require(cells==selected,"Table and selection interfaces disagree");
         require(accessible->focusChild()==selected[0],"Accessibility reports a different focused item");
         const int row=accessible->indexOfChild(selected[0]);
         require(row>=0 && table->selectedRows()==QList<int>{row} && table->isRowSelected(row),"Flattened row mapping differs");
         require(table->cellAt(row,0)==selected[0],"Selected row does not round-trip through cellAt");
#if QT_VERSION >= QT_VERSION_CHECK(6,5,0)
         auto *selection=accessible->selectionInterface();
         require(selection && selection->selectedItems()==selected,"Selection interface reports a different item");
         require(selection->select(selected[0]),"Restoring the accessible selection failed");
#else
         require(table->selectRow(row),"Restoring the accessible selection failed");
#endif
         events();
         require(tree->currentItem()==item && item->isSelected(),"Reading and restoring selection moved it to a parent");
         edit->setFocus();events();tree->setFocus();events();
         require(tree->currentItem()==item && item->isSelected(),"Focus return lost the selection");
      };
      for (auto *item:{beta,alpha,prop,root}) {
         tree->scrollToItem(item);
         QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(item).center());events();
         check(item);
      }
      require(!table->selectRow(-1) && !table->selectRow(1000),"Invalid row was accepted");
      tree->setCurrentItem(beta);events();
      auto *betaAccessible=table->selectedCells().first();
      const auto betaId=QAccessible::uniqueId(betaAccessible);
      int opened=0;
      QObject::connect(tree,&QTreeWidget::itemDoubleClicked,&window,[&](QTreeWidgetItem *item,int) { if (item==beta) ++opened; });
      betaAccessible->actionInterface()->doAction(QAccessibleActionInterface::pressAction());events();
      require(opened==1,"Accessible activation did not open the requested resource");
      group->setExpanded(false);events();
      require(betaAccessible->state().invisible,"Collapsed descendants remained visible to accessibility");
      group->setExpanded(true);events();
      check(beta);
      tree->clear();events();
      const auto *removed=QAccessible::accessibleInterface(betaId);
      require(!removed || !removed->isValid(),"Removed item retained a valid accessibility index");
      auto *replacement=new QTreeWidgetItem(tree,{"Replacement"});tree->setCurrentItem(replacement);events();
      check(replacement);
      std::cout<<"PASS nested selection enumeration, read/restore, focus return, activation, collapse/expand and tree rebuild\n";
      return 0;
   } catch (const std::exception &error) {
      std::cerr<<error.what()<<'\n';return 1;
   }
}
