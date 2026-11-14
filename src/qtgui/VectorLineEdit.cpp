#include "VectorLineEdit.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QPointer>
#include <QRegularExpression>
#include <QValidator>
#include <QVector>
#include <cmath>

struct VectorPasteGroup
{
   struct Change {
      std::array<QString,3> before,after;
      std::array<bool,3> changed;
   };
   std::array<QPointer<VectorLineEdit>,3> fields;
   std::function<bool()> enabled;
   QVector<Change> history;
   qsizetype position=0;

   bool editable() const
   {
      for (const auto &field:fields)
         if (!field || !field->isEnabled() || field->isReadOnly()) return false;
      return true;
   }

   bool paste()
   {
      if (!editable() || (enabled && !enabled())) return false;
      auto text=QApplication::clipboard()->text().trimmed();
      if (text.size()>=2 && ((text.front()=='[' && text.back()==']') ||
          (text.front()=='(' && text.back()==')') || (text.front()=='{' && text.back()=='}')))
         text=text.mid(1,text.size()-2);
      const auto values=text.split(QRegularExpression("[\\s,;]+"),Qt::SkipEmptyParts);
      if (values.size()!=3) return false;
      Change change;
      bool changed=false;
      for (int index=0;index<3;++index) {
         bool valid=false;
         const double number=values[index].toDouble(&valid);
         auto value=values[index];
         int cursor=value.size();
         const auto *validator=fields[index]->validator();
         if (!valid || !std::isfinite(number) || value.size()>fields[index]->maxLength() ||
             !fields[index]->inputMask().isEmpty() ||
             (validator && validator->validate(value,cursor)!=QValidator::Acceptable)) return false;
         change.before[index]=fields[index]->text();
         change.after[index]=value;
         change.changed[index]=change.before[index]!=value;
         changed=changed || change.changed[index];
      }
      if (!changed) return true;
      for (int index=0;index<3;++index) if (change.changed[index]) {
         fields[index]->selectAll();
         fields[index]->insert(change.after[index]);
         // Start a new native undo group before any subsequent typing.
         fields[index]->setCursorPosition(0);
         fields[index]->setCursorPosition(fields[index]->text().size());
      }
      history.resize(position);
      history.append(change);
      ++position;
      return true;
   }

   bool canStep(bool forward) const
   {
      if (!editable() || (forward ? position>=history.size() : position==0)) return false;
      const auto &change=history[forward ? position : position-1];
      for (int index=0;index<3;++index) {
         if (fields[index]->text()!=(forward ? change.before[index] : change.after[index])) return false;
         if (change.changed[index] && !(forward ? fields[index]->isRedoAvailable() : fields[index]->isUndoAvailable())) return false;
      }
      return true;
   }

   bool step(bool forward)
   {
      if (!canStep(forward)) return false;
      const auto &change=history[forward ? position : position-1];
      for (int index=0;index<3;++index) if (change.changed[index]) {
         if (forward) fields[index]->QLineEdit::redo();
         else fields[index]->QLineEdit::undo();
      }
      position+=forward ? 1 : -1;
      return true;
   }
};

void VectorLineEdit::link(const std::array<QLineEdit *,3> &fields,std::function<bool()> enabled)
{
   std::array<VectorLineEdit *,3> components;
   for (int index=0;index<3;++index) {
      components[index]=dynamic_cast<VectorLineEdit *>(fields[index]);
      if (!components[index]) return;
   }
   auto group=std::make_shared<VectorPasteGroup>();
   group->enabled=std::move(enabled);
   for (int index=0;index<3;++index) {
      group->fields[index]=components[index];
      components[index]->vectorGroup=group;
   }
   fields[0]->setToolTip((fields[0]->toolTip()+'\n'+
      tr("Paste three numbers to fill all components. Undo restores the whole vector.")).trimmed());
}

void VectorLineEdit::paste()
{
   if (vectorGroup && vectorGroup->fields[0]==this && vectorGroup->paste()) return;
   QLineEdit::paste();
}

void VectorLineEdit::undo()
{
   if (vectorGroup && vectorGroup->step(false)) return;
   QLineEdit::undo();
}

void VectorLineEdit::redo()
{
   if (vectorGroup && vectorGroup->step(true)) return;
   QLineEdit::redo();
}

void VectorLineEdit::keyPressEvent(QKeyEvent *event)
{
   if (event->matches(QKeySequence::Paste)) paste();
   else if (event->matches(QKeySequence::Undo)) undo();
   else if (event->matches(QKeySequence::Redo) ||
            (event->key()==Qt::Key_Y && event->modifiers()==Qt::ControlModifier)) redo();
   else { QLineEdit::keyPressEvent(event); return; }
   event->accept();
}

void VectorLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
   auto *menu=createStandardContextMenu();
   for (auto *action:menu->actions()) {
      if (action->objectName()=="edit-paste") {
         disconnect(action,&QAction::triggered,this,nullptr);
         connect(action,&QAction::triggered,this,&VectorLineEdit::paste);
      } else if (action->objectName()=="edit-undo") {
         disconnect(action,&QAction::triggered,this,nullptr);
         action->setEnabled(action->isEnabled() || (vectorGroup && vectorGroup->canStep(false)));
         connect(action,&QAction::triggered,this,&VectorLineEdit::undo);
      } else if (action->objectName()=="edit-redo") {
         disconnect(action,&QAction::triggered,this,nullptr);
         action->setEnabled(action->isEnabled() || (vectorGroup && vectorGroup->canStep(true)));
         connect(action,&QAction::triggered,this,&VectorLineEdit::redo);
      }
   }
   menu->setAttribute(Qt::WA_DeleteOnClose);
   menu->popup(event->globalPos());
}
