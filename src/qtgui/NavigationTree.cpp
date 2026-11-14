#include "NavigationTree.hpp"
#include <QAccessibleWidget>
#include <QKeyEvent>
#include <QPersistentModelIndex>
#include <QPointer>

namespace {
// Qt's default tree adapter uses a nested model row as a flattened row when
// enumerating selected cells. Use persistent indexes for every navigation and
// selection route, including clients that read and restore the selection.
class NavigationItemAccessible;
class NavigationAccessible final : public QAccessibleWidget,
                                   public QAccessibleTableInterface
#if QT_VERSION >= QT_VERSION_CHECK(6,5,0)
                                 , public QAccessibleSelectionInterface
#endif
{
public:
   explicit NavigationAccessible(NavigationTree *tree) : QAccessibleWidget(tree,QAccessible::Tree) {}
   ~NavigationAccessible() override
   {
      for (const auto id:items) QAccessible::deleteAccessibleInterface(id);
   }
   NavigationTree *tree() const { return static_cast<NavigationTree *>(widget()); }
   QList<QModelIndex> visibleIndexes() const
   {
      QList<QModelIndex> result;
      const auto visit=[&](auto &&self,QTreeWidgetItem *item)->void {
         if (item->isHidden()) return;
         result.append(tree()->indexFromItem(item));
         if (item->isExpanded())
            for (int i=0;i<item->childCount();++i) self(self,item->child(i));
      };
      for (int i=0;i<tree()->topLevelItemCount();++i) visit(visit,tree()->topLevelItem(i));
      return result;
   }
   QAccessibleInterface *forIndex(const QModelIndex &index) const;
   int childCount() const override { return visibleIndexes().size(); }
   QAccessibleInterface *child(int row) const override
   {
      const auto indexes=visibleIndexes();
      return row>=0 && row<indexes.size() ? forIndex(indexes[row]) : nullptr;
   }
   int indexOfChild(const QAccessibleInterface *item) const override;
   QAccessibleInterface *childAt(int x,int y) const override
   {
      return forIndex(tree()->indexAt(tree()->viewport()->mapFromGlobal(QPoint(x,y))));
   }
   QAccessibleInterface *focusChild() const override { return forIndex(tree()->currentIndex()); }
   void *interface_cast(QAccessible::InterfaceType type) override
   {
      if (type==QAccessible::TableInterface) return static_cast<QAccessibleTableInterface *>(this);
#if QT_VERSION >= QT_VERSION_CHECK(6,5,0)
      if (type==QAccessible::SelectionInterface) return static_cast<QAccessibleSelectionInterface *>(this);
#endif
      return QAccessibleWidget::interface_cast(type);
   }
   QAccessibleInterface *caption() const override { return nullptr; }
   QAccessibleInterface *summary() const override { return nullptr; }
   QAccessibleInterface *cellAt(int row,int column) const override { return column==0 ? child(row) : nullptr; }
   QString columnDescription(int) const override { return {}; }
   QString rowDescription(int row) const override
   {
      const auto *item=child(row);return item ? item->text(QAccessible::Name) : QString();
   }
   int columnCount() const override { return 1; }
   int rowCount() const override { return childCount(); }
   QList<QAccessibleInterface *> selectedItems() const
   {
      QList<QAccessibleInterface *> selected;
      for (const auto &index:visibleIndexes())
         if (tree()->selectionModel()->isSelected(index)) selected.append(forIndex(index));
      return selected;
   }
   QList<QAccessibleInterface *> selectedCells() const override { return selectedItems(); }
   int selectedItemCount() const { return selectedItems().size(); }
   int selectedCellCount() const override { return selectedItemCount(); }
   int selectedRowCount() const override { return selectedItemCount(); }
   int selectedColumnCount() const override { return isColumnSelected(0) ? 1 : 0; }
   QList<int> selectedRows() const override
   {
      QList<int> selected;
      const auto indexes=visibleIndexes();
      for (int i=0;i<indexes.size();++i)
         if (tree()->selectionModel()->isSelected(indexes[i])) selected.append(i);
      return selected;
   }
   QList<int> selectedColumns() const override { return isColumnSelected(0) ? QList<int>{0} : QList<int>{}; }
   bool isRowSelected(int row) const override
   {
      const auto indexes=visibleIndexes();
      return row>=0 && row<indexes.size() && tree()->selectionModel()->isSelected(indexes[row]);
   }
   bool isColumnSelected(int column) const override
   {
      return column==0 && rowCount()>0 && selectedItemCount()==rowCount();
   }
   bool select(QAccessibleInterface *item);
   bool unselect(QAccessibleInterface *item);
   bool selectRow(int row) override { return select(child(row)); }
   bool unselectRow(int row) override { return unselect(child(row)); }
   bool selectColumn(int column) override { return column==0 && rowCount()==1 && selectRow(0); }
   bool unselectColumn(int) override { return false; }
   bool selectAll() { return rowCount()==1 && selectRow(0); }
   bool clear() { tree()->clearSelection();return true; }
   void modelChange(QAccessibleTableModelChangeEvent *) override;
private:
   mutable QList<QAccessible::Id> items;
};

class NavigationItemAccessible final : public QAccessibleInterface,
                                       public QAccessibleTableCellInterface,
                                       public QAccessibleActionInterface
{
public:
   NavigationItemAccessible(NavigationTree *tree,const QModelIndex &index) : owner(tree),index(index) {}
   QPointer<NavigationTree> owner;
   QPersistentModelIndex index;
   bool isValid() const override { return owner && index.isValid(); }
   QObject *object() const override { return nullptr; }
   QWindow *window() const override { return owner ? owner->window()->windowHandle() : nullptr; }
   QAccessibleInterface *parent() const override { return owner ? QAccessible::queryAccessibleInterface(owner) : nullptr; }
   QAccessibleInterface *child(int) const override { return nullptr; }
   QAccessibleInterface *childAt(int,int) const override { return nullptr; }
   int childCount() const override { return 0; }
   int indexOfChild(const QAccessibleInterface *) const override { return -1; }
   QString text(QAccessible::Text type) const override
   {
      if (!isValid()) return {};
      if (type==QAccessible::Name) {
         const auto accessible=index.data(Qt::AccessibleTextRole).toString();
         return accessible.isEmpty() ? index.data(Qt::DisplayRole).toString() : accessible;
      }
      if (type==QAccessible::Description) return index.data(Qt::ToolTipRole).toString();
      return {};
   }
   void setText(QAccessible::Text,const QString &) override {}
   QRect rect() const override
   {
      if (!isValid()) return {};
      auto result=owner->visualRect(index);
      result.translate(owner->viewport()->mapToGlobal(QPoint()));return result;
   }
   QAccessible::Role role() const override { return QAccessible::TreeItem; }
   QAccessible::State state() const override
   {
      QAccessible::State result;
      if (!isValid()) { result.invalid=true;return result; }
      auto *item=owner->itemFromIndex(index);
      result.disabled=!owner->isEnabled() || !(index.flags() & Qt::ItemIsEnabled);
      result.selectable=index.flags() & Qt::ItemIsSelectable;
      result.focusable=result.selectable;
      result.selected=isSelected();result.focused=owner->currentIndex()==index;
      result.expandable=item->childCount()>0;result.expanded=item->isExpanded();
      result.collapsed=result.expandable && !result.expanded;
      result.invisible=!owner->isVisible() || rowIndex()<0;
      result.offscreen=!rect().intersects(QRect(owner->viewport()->mapToGlobal(QPoint()),owner->viewport()->size()));
      return result;
   }
   void *interface_cast(QAccessible::InterfaceType type) override
   {
      if (type==QAccessible::TableCellInterface) return static_cast<QAccessibleTableCellInterface *>(this);
      if (type==QAccessible::ActionInterface) return static_cast<QAccessibleActionInterface *>(this);
      return nullptr;
   }
   int columnIndex() const override { return 0; }
   int rowIndex() const override
   {
      const auto *tree=dynamic_cast<NavigationAccessible *>(parent());
      return tree ? tree->visibleIndexes().indexOf(index) : -1;
   }
   int columnExtent() const override { return 1; }
   int rowExtent() const override { return 1; }
   QList<QAccessibleInterface *> columnHeaderCells() const override { return {}; }
   QList<QAccessibleInterface *> rowHeaderCells() const override { return {}; }
   bool isSelected() const override { return isValid() && owner->selectionModel()->isSelected(index); }
   QAccessibleInterface *table() const override { return parent(); }
   QStringList actionNames() const override
   {
      return {pressAction(),setFocusAction(),toggleAction()};
   }
   void doAction(const QString &action) override
   {
      if (!isValid() || state().disabled) return;
      auto *tree=dynamic_cast<NavigationAccessible *>(parent());
      if (!tree) return;
      auto *item=owner->itemFromIndex(index);
      if (action!=pressAction() && action!=setFocusAction() && action!=toggleAction()) return;
      tree->select(this);
      if (action==setFocusAction()) { owner->setFocus();return; }
      if (action==pressAction()) {
         if (item->childCount()) item->setExpanded(!item->isExpanded());
         owner->itemDoubleClicked(item,0);
      }
   }
   QStringList keyBindingsForAction(const QString &) const override { return {}; }
};

QAccessibleInterface *NavigationAccessible::forIndex(const QModelIndex &index) const
{
   if (!index.isValid()) return nullptr;
   for (const auto id:items) {
      auto *item=dynamic_cast<NavigationItemAccessible *>(QAccessible::accessibleInterface(id));
      if (item && item->index==index) return item;
   }
   auto *item=new NavigationItemAccessible(tree(),index);
   items.append(QAccessible::registerAccessibleInterface(item));return item;
}
int NavigationAccessible::indexOfChild(const QAccessibleInterface *child) const
{
   const auto *item=dynamic_cast<const NavigationItemAccessible *>(child);
   return item && item->owner==tree() ? visibleIndexes().indexOf(item->index) : -1;
}
bool NavigationAccessible::select(QAccessibleInterface *child)
{
   auto *item=dynamic_cast<NavigationItemAccessible *>(child);
   if (!item || item->owner!=tree() || !item->isValid() || item->state().disabled || !item->state().selectable) return false;
   tree()->selectionModel()->setCurrentIndex(item->index,QItemSelectionModel::ClearAndSelect);
   tree()->scrollTo(item->index);return true;
}
bool NavigationAccessible::unselect(QAccessibleInterface *child)
{
   auto *item=dynamic_cast<NavigationItemAccessible *>(child);
   if (!item || item->owner!=tree() || !item->isValid()) return false;
   tree()->selectionModel()->select(item->index,QItemSelectionModel::Deselect);return true;
}
void NavigationAccessible::modelChange(QAccessibleTableModelChangeEvent *)
{
   for (int i=items.size()-1;i>=0;--i) {
      auto *item=QAccessible::accessibleInterface(items[i]);
      if (!item || !item->isValid()) QAccessible::deleteAccessibleInterface(items.takeAt(i));
   }
}
}

NavigationTree::NavigationTree(QWidget *parent) : QTreeWidget(parent)
{
   static const bool installed=[] {
      QAccessible::installFactory([](const QString &,QObject *object)->QAccessibleInterface * {
         auto *tree=dynamic_cast<NavigationTree *>(object);
         return tree ? new NavigationAccessible(tree) : nullptr;
      });
      return true;
   }();
   Q_UNUSED(installed);
   setSelectionBehavior(QAbstractItemView::SelectItems);
   connect(this,&QTreeWidget::itemSelectionChanged,this,[this] {
      const auto selected=selectedItems();
      if (selected.size()==1 && currentItem()!=selected.first())
         setCurrentItem(selected.first(),0,QItemSelectionModel::NoUpdate);
   });
}
void NavigationTree::keyPressEvent(QKeyEvent *event)
{
   if ((event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter) &&
       (event->modifiers()==Qt::NoModifier || event->modifiers()==Qt::KeypadModifier)) {
      if (!event->isAutoRepeat() && currentItem()) emit itemDoubleClicked(currentItem(),currentColumn());
      event->accept();return;
   }
   QTreeWidget::keyPressEvent(event);
}
