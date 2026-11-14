#include "MatrixTableWidget.hpp"
#include <QAction>
#include <QApplication>
#include <QChildEvent>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QHeaderView>
#include <QScrollBar>
#include <QMenu>
#include <QRegularExpression>
#include <QToolTip>
#include <cmath>

namespace {
const char *editCommand(const QKeyEvent *event)
{
   if (event->matches(QKeySequence::Paste)) return "paste";
   if (event->matches(QKeySequence::Copy)) return "copy";
   if (event->matches(QKeySequence::Undo)) return "undo";
   if (event->matches(QKeySequence::Redo) ||
       (event->key()==Qt::Key_Y && event->modifiers()==Qt::ControlModifier)) return "redo";
   return nullptr;
}

QVector<QStringList> clipboardMatrix()
{
   auto text=QApplication::clipboard()->text().trimmed();
   if (text.size()>=2 && ((text.front()=='[' && text.back()==']') ||
       (text.front()=='(' && text.back()==')') || (text.front()=='{' && text.back()=='}')))
      text=text.mid(1,text.size()-2);
   QVector<QStringList> result;
   const auto rows=text.split(QRegularExpression("[\\r\\n;]+"),Qt::SkipEmptyParts);
   for (const auto &row:rows) {
      const auto values=row.trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
      if (values.isEmpty() || (!result.isEmpty() && values.size()!=result.first().size())) return {};
      for (const auto &value:values) {
         bool valid=false;
         const double number=value.toDouble(&valid);
         if (!valid || !std::isfinite(number)) return {};
      }
      result.append(values);
   }
   return result;
}
}

MatrixTableWidget::MatrixTableWidget(int rows,int columns,QWidget *parent)
   : QTableWidget(rows,columns,parent)
{
   viewport()->installEventFilter(this);
   const auto clearHistory=[this] { history.clear(); historyPosition=0; updateMinimumHeight(); };
   connect(model(),&QAbstractItemModel::rowsInserted,this,clearHistory);
   connect(model(),&QAbstractItemModel::rowsRemoved,this,clearHistory);
   connect(model(),&QAbstractItemModel::columnsInserted,this,clearHistory);
   connect(model(),&QAbstractItemModel::columnsRemoved,this,clearHistory);
   connect(verticalHeader(),&QHeaderView::sectionResized,this,[this] { updateMinimumHeight(); });
   updateMinimumHeight();
}

void MatrixTableWidget::updateMinimumHeight()
{
   int height=horizontalHeader()->sizeHint().height()+horizontalScrollBar()->sizeHint().height()+2*frameWidth();
   for (int row=0;row<std::min(6,rowCount());++row) height+=rowHeight(row);
   setMinimumHeight(height);
}

void MatrixTableWidget::changeEvent(QEvent *event)
{
   QTableWidget::changeEvent(event);
   if (event->type()==QEvent::FontChange || event->type()==QEvent::StyleChange) updateMinimumHeight();
}

bool MatrixTableWidget::paste()
{
   const auto values=clipboardMatrix();
   if (values.isEmpty()) return false;
   auto *focused=QApplication::focusWidget();
   const bool editing=focused && focused!=this && focused!=viewport() && isAncestorOf(focused);
   if (editing && values.size()==1 && values.first().size()==1) return false;
   const int firstRow=std::max(0,currentRow()),firstColumn=std::max(0,currentColumn());
   if (firstRow+values.size()>rowCount() || firstColumn+values.first().size()>columnCount()) {
      QToolTip::showText(viewport()->mapToGlobal(visualRect(currentIndex()).center()),
                        tr("The clipboard matrix does not fit from the selected cell."),this);
      return true;
   }
   Change change;
   for (int row=0;row<values.size();++row) for (int column=0;column<values[row].size();++column) {
      const auto *cell=item(firstRow+row,firstColumn+column);
      if (cell && !(cell->flags() & Qt::ItemIsEditable)) return true;
      change.append({firstRow+row,firstColumn+column,cell ? cell->text() : QString(),values[row][column]});
   }
   if (editing) {
      auto *editor=focused;
      while (editor->parentWidget() && editor->parentWidget()!=viewport()) editor=editor->parentWidget();
      closeEditor(editor,QAbstractItemDelegate::RevertModelCache);
   }
   for (const auto &cell:change) {
      if (!item(cell.row,cell.column)) setItem(cell.row,cell.column,new QTableWidgetItem);
      item(cell.row,cell.column)->setText(cell.after);
   }
   history.resize(historyPosition);
   history.append(change);
   ++historyPosition;
   clearSelection();
   setRangeSelected(QTableWidgetSelectionRange(firstRow,firstColumn,firstRow+values.size()-1,firstColumn+values.first().size()-1),true);
   setFocus();
   return true;
}

bool MatrixTableWidget::copy()
{
   const auto ranges=selectedRanges();
   if (ranges.size()!=1) return false;
   QStringList rows;
   for (int row=ranges.first().topRow();row<=ranges.first().bottomRow();++row) {
      QStringList values;
      for (int column=ranges.first().leftColumn();column<=ranges.first().rightColumn();++column)
         values.append(item(row,column) ? item(row,column)->text() : QString());
      rows.append(values.join('\t'));
   }
   QApplication::clipboard()->setText(rows.join('\n'));
   return true;
}

bool MatrixTableWidget::canStep(bool forward) const
{
   if (forward ? historyPosition>=history.size() : historyPosition==0) return false;
   const auto &change=history[forward ? historyPosition : historyPosition-1];
   for (const auto &cell:change) {
      const auto *current=item(cell.row,cell.column);
      if (!current || !(current->flags() & Qt::ItemIsEditable) ||
          current->text()!=(forward ? cell.before : cell.after)) return false;
   }
   return true;
}

bool MatrixTableWidget::step(bool forward)
{
   if (!canStep(forward)) return false;
   const auto &change=history[forward ? historyPosition : historyPosition-1];
   for (const auto &cell:change) item(cell.row,cell.column)->setText(forward ? cell.after : cell.before);
   historyPosition+=forward ? 1 : -1;
   return true;
}

bool MatrixTableWidget::editAction(const char *action)
{
   auto *line=qobject_cast<QLineEdit *>(QApplication::focusWidget());
   const bool editing=line && isAncestorOf(line);
   if (!qstrcmp(action,"paste")) return paste();
   if (!qstrcmp(action,"copy")) return !editing && copy();
   if (!qstrcmp(action,"undo")) return !(editing && line->isUndoAvailable()) && step(false);
   if (!qstrcmp(action,"redo")) return !(editing && line->isRedoAvailable()) && step(true);
   return false;
}

bool MatrixTableWidget::editFocusedWidget(QWidget *focused,const char *action)
{
   for (auto *widget=focused;widget;widget=widget->parentWidget())
      if (auto *matrix=dynamic_cast<MatrixTableWidget *>(widget)) return matrix->editAction(action);
   return false;
}

void MatrixTableWidget::keyPressEvent(QKeyEvent *event)
{
   const char *action=editCommand(event);
   if (action && editAction(action)) { event->accept(); return; }
   QTableWidget::keyPressEvent(event);
}

void MatrixTableWidget::contextMenuEvent(QContextMenuEvent *event)
{
   QMenu menu(this);
   menu.addAction(tr("Undo"),[this] { step(false); })->setEnabled(canStep(false));
   menu.addAction(tr("Redo"),[this] { step(true); })->setEnabled(canStep(true));
   menu.addSeparator();
   menu.addAction(tr("Copy"),[this] { copy(); })->setEnabled(selectedRanges().size()==1);
   menu.addAction(tr("Paste"),[this] { paste(); })->setEnabled(!clipboardMatrix().isEmpty());
   menu.exec(event->globalPos());
}

void MatrixTableWidget::showEditorMenu(QWidget *editor,QContextMenuEvent *event)
{
   auto *line=qobject_cast<QLineEdit *>(editor);
   if (!line) return;
   auto *menu=line->createStandardContextMenu();
   const auto replace=[&](const char *name,const char *operation) {
      for (auto *action:menu->actions()) if (action->objectName()==name) {
         disconnect(action,nullptr,line,nullptr);
         if (!qstrcmp(operation,"undo")) action->setEnabled(action->isEnabled() || canStep(false));
         if (!qstrcmp(operation,"redo")) action->setEnabled(action->isEnabled() || canStep(true));
         connect(action,&QAction::triggered,this,[this,line,operation] {
            if (!editAction(operation)) QMetaObject::invokeMethod(line,operation,Qt::DirectConnection);
         });
      }
   };
   replace("edit-paste","paste"); replace("edit-undo","undo"); replace("edit-redo","redo");
   menu->setAttribute(Qt::WA_DeleteOnClose);
   menu->popup(event->globalPos());
}

bool MatrixTableWidget::eventFilter(QObject *object,QEvent *event)
{
   if (object==viewport() && event->type()==QEvent::ChildAdded)
      static_cast<QChildEvent *>(event)->child()->installEventFilter(this);
   if (object!=viewport() && event->type()==QEvent::KeyPress) {
      auto *key=static_cast<QKeyEvent *>(event);
      const char *action=editCommand(key);
      if (action && editAction(action)) return true;
   }
   if (object!=viewport() && event->type()==QEvent::ContextMenu && qobject_cast<QLineEdit *>(object)) {
      showEditorMenu(qobject_cast<QWidget *>(object),static_cast<QContextMenuEvent *>(event));
      return true;
   }
   return QTableWidget::eventFilter(object,event);
}
