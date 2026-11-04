#include "TutorialQtTestDriver.hpp"
#include <QtTest/QTest>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QStyleOptionButton>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QScrollArea>
#include <QTabBar>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QRegularExpression>
#include <QKeySequence>
#include <algorithm>
#include <stdexcept>

namespace {
QString normalized(QString text)
{
   return text.toLower().remove(QRegularExpression("[^a-z0-9]"));
}
QMdiSubWindow *mdi(QWidget *widget)
{
   for (auto *parent=widget;parent;parent=parent->parentWidget())
      if (auto *panel=qobject_cast<QMdiSubWindow *>(parent)) return panel;
   return nullptr;
}
bool editable(QWidget *widget)
{
   if (auto *line=qobject_cast<QLineEdit *>(widget)) return !line->isReadOnly();
   if (auto *text=qobject_cast<QPlainTextEdit *>(widget)) return !text->isReadOnly();
   return false;
}
QPoint buttonPoint(QAbstractButton *button)
{
   QStyleOptionButton option; option.initFrom(button); option.text=button->text();
   if (qobject_cast<QCheckBox *>(button)) return button->style()->subElementRect(QStyle::SE_CheckBoxIndicator,&option,button).center();
   if (qobject_cast<QRadioButton *>(button)) return button->style()->subElementRect(QStyle::SE_RadioButtonIndicator,&option,button).center();
   return button->rect().center();
}
QWidget *focus(QWidget &root)
{
   if (auto *popup=QApplication::activePopupWidget()) return popup->focusWidget() ? popup->focusWidget() : popup;
   if (auto *modal=QApplication::activeModalWidget()) return modal->focusWidget() ? modal->focusWidget() : modal;
   return QApplication::focusWidget() ? QApplication::focusWidget() : &root;
}
}

TutorialQtTestDriver::TutorialQtTestDriver(QWidget &root,const QString &directory)
   :root(root),directory(directory)
{
   QDir().mkpath(directory+"/input-accepted");
   QDir().mkpath(directory+"/input-finished");
   QDir().mkpath(directory+"/input-suspended");
   QObject::connect(&timer,&QTimer::timeout,&root,[this] { poll(); });
   timer.start(20);
}
void TutorialQtTestDriver::record(const QString &path,const QJsonObject &object)
{
   QSaveFile file(directory+"/"+path);
   const auto data=QJsonDocument(object).toJson();
   if (!file.open(QIODevice::WriteOnly) || file.write(data)!=data.size() || !file.commit())
      throw std::runtime_error("Cannot record QtTest input receipt");
}

TutorialQtTestDriver::Target TutorialQtTestDriver::locate(const QJsonObject &action)
{
   const auto kind=action["action"].toString();
   if (kind=="window") return {&root,{},"window",-1,{{"class",root.metaObject()->className()}}};
   const auto name=action["target_name"].toString(action["field_button"].toString());
   const auto text=normalized(action["target_text"].toString());
   const auto role=action["target_role"].toString();
   const auto label=normalized(action["field_hint"].toString().section('|',0,0));
   if (name.isEmpty() && text.isEmpty() && label.isEmpty()) {
      if (kind!="key" && kind!="text") throw std::runtime_error("QtTest requires a semantic selector; coordinate fallback is disabled");
      auto *widget=focus(root);
      if (!widget || !widget->isVisible() || !widget->isEnabled()) throw std::runtime_error("No enabled focused GUI control");
      return {widget,widget->rect().center(),"focus",-1,{{"name",widget->objectName()},{"class",widget->metaObject()->className()}}};
   }
   struct Choice { Target target; int score; };
   std::vector<Choice> choices;
   auto *modal=QApplication::activeModalWidget();
   auto *area=root.findChild<QMdiArea *>();
   auto eligible=[&](QWidget *widget) {
      if (!widget || !widget->isVisible() || !widget->isEnabled()) return false;
      if (modal && widget!=modal && !modal->isAncestorOf(widget)) return false;
      auto *panel=mdi(widget);
      if (!modal && panel && area && panel!=area->activeSubWindow()) return false;
      const auto scope=action["scope_name"].toString();
      if (!scope.isEmpty() && (!panel || panel->property("resourceName").toString()!=scope)) return false;
      return true;
   };
   auto add=[&](QWidget *widget,const QPoint &point,const QString &type,const QString &caption,int index=-1,int score=1) {
      if (!eligible(widget)) return;
      choices.push_back({{widget,point,type,index,{{"name",widget->objectName()},{"class",widget->metaObject()->className()},{"text",caption},{"role",type}}},score});
   };
   auto matches=[&](const QString &caption) {
      const auto value=normalized(caption);
      return value==text ? 2 : (!text.isEmpty() && value.contains(text) ? 1 : 0);
   };
   for (auto *widget:QApplication::allWidgets()) {
      if (!eligible(widget)) continue;
      if (!name.isEmpty() && (widget->objectName()==name ||
          (name=="documentEditor" && (widget->objectName()=="scriptEditor" || widget->objectName()=="inactiveScriptEditor")))) {
         auto *target=widget;
         if (action.contains("field_button")) {
            target=nullptr;
            for (auto *button:widget->parentWidget()->findChildren<QAbstractButton *>())
               if (button->isVisible() && button->isEnabled()) {
                  if (target) throw std::runtime_error("Field has multiple selector buttons");
                  target=button;
               }
            if (!target) throw std::runtime_error("Field selector button is absent");
         }
         add(target,target->rect().center(),"widget",{},-1,3);
      }
      if (!text.isEmpty()) {
         if ((role.isEmpty() || role=="button")) if (auto *button=qobject_cast<QAbstractButton *>(widget))
            if (const auto score=matches(button->text())) add(button,button->rect().center(),"button",button->text(),-1,score);
         if (role=="tab") if (auto *tabs=qobject_cast<QTabBar *>(widget))
            for (int i=0;i<tabs->count();++i) if (const auto score=matches(tabs->tabText(i))) add(tabs,{},"tab",tabs->tabText(i),i,score);
         if (role=="menu") if (auto *menu=qobject_cast<QMenu *>(widget))
            for (int i=0;i<menu->actions().size();++i) {
               auto *item=menu->actions()[i];
               if (item->isEnabled() && !item->isSeparator()) if (const auto score=matches(item->text()))
                  add(menu,menu->actionGeometry(item).center(),"menu",item->text(),i,score);
            }
         if (role=="tree") if (auto *tree=qobject_cast<QTreeWidget *>(widget)) {
            int index=0;
            for (QTreeWidgetItemIterator i(tree);*i;++i,++index) if (const auto score=matches((*i)->text(0)))
               add(tree,{},"tree",(*i)->text(0),index,score);
         }
         if (role=="list") if (auto *list=qobject_cast<QListWidget *>(widget))
            for (int i=0;i<list->count();++i) if (const auto score=matches(list->item(i)->text())) add(list,{},"list",list->item(i)->text(),i,score);
      }
      if (!label.isEmpty()) {
         if (auto *caption=qobject_cast<QLabel *>(widget);caption && normalized(caption->text())==label) {
            QWidget *buddy=caption->buddy();
            if (!buddy && caption->parentWidget()) if (auto *form=qobject_cast<QFormLayout *>(caption->parentWidget()->layout())) {
               int row=-1; QFormLayout::ItemRole role; form->getWidgetPosition(caption,&row,&role);
               if (row>=0) if (auto *item=form->itemAt(row,QFormLayout::FieldRole)) buddy=item->widget();
            }
            if (buddy) {
               if (!qobject_cast<QLineEdit *>(buddy) && !qobject_cast<QComboBox *>(buddy))
                  for (auto *child:buddy->findChildren<QWidget *>(QString(),Qt::FindDirectChildrenOnly))
                     if (qobject_cast<QLineEdit *>(child) || qobject_cast<QComboBox *>(child)) { buddy=child; break; }
               add(buddy,buddy->rect().center(),"field",caption->text(),-1,3);
            }
         }
         if (auto *table=qobject_cast<QTableWidget *>(widget))
            for (int row=0;row<table->rowCount();++row) if (!table->isRowHidden(row) && table->item(row,0) && normalized(table->item(row,0)->text())==label)
               add(table,{},"table",table->item(row,0)->text(),row,3);
      }
   }
   if (choices.empty()) throw std::runtime_error("Semantic GUI target is not visible/enabled: "+QJsonDocument(action).toJson(QJsonDocument::Compact).toStdString());
   const auto score=std::max_element(choices.begin(),choices.end(),[](const Choice &a,const Choice &b) { return a.score<b.score; })->score;
   std::vector<Target> best;
   for (const auto &choice:choices) if (choice.score==score) best.push_back(choice.target);
   if (best.size()!=1) throw std::runtime_error("Semantic GUI selector is ambiguous; add a container or an exact object name");
   return best.front();
}

void TutorialQtTestDriver::execute(const QJsonObject &action,Target target,const std::shared_ptr<Frame> &frame)
{
   auto *widget=target.widget.data();
   if (!widget) throw std::runtime_error("GUI control was destroyed before input");
   for (auto *parent=widget->parentWidget();parent;parent=parent->parentWidget())
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(widget);
   if (target.role=="tree") {
      auto *tree=qobject_cast<QTreeWidget *>(widget); QTreeWidgetItemIterator i(tree);
      for (int n=0;n<target.index;++n) ++i;
      tree->scrollToItem(*i); target.point=tree->visualItemRect(*i).center(); widget=tree->viewport();
   } else if (target.role=="list") {
      auto *list=qobject_cast<QListWidget *>(widget); auto *item=list->item(target.index);
      list->scrollToItem(item); target.point=list->visualItemRect(item).center(); widget=list->viewport();
   } else if (target.role=="table") {
      auto *table=qobject_cast<QTableWidget *>(widget); const auto index=table->model()->index(target.index,1);
      table->scrollTo(index); target.point=table->visualRect(index).center();
      if (auto *cell=table->cellWidget(target.index,1)) { widget=cell; target.point=cell->rect().center(); }
      else widget=table->viewport();
   }
   const auto kind=action["action"].toString();
   if (kind=="key" || kind=="text")
      if (auto *combo=qobject_cast<QComboBox *>(widget);combo && combo->isEditable()) {
         widget=combo->lineEdit(); target.point=widget->rect().center();
      }
   if (kind=="window") { root.showMaximized(); return; }
   if (target.role=="tab") {
      auto *tabs=qobject_cast<QTabBar *>(widget);
      // Keyboard navigation also handles tabs clipped by scroll arrows.
      const auto current=tabs->tabRect(tabs->currentIndex()).intersected(tabs->rect());
      QTest::mouseClick(tabs,Qt::LeftButton,Qt::NoModifier,current.center());
      for (int n=0;n<tabs->count()+1 && tabs->currentIndex()!=target.index;++n)
         QTest::keyClick(tabs,tabs->currentIndex()<target.index ? Qt::Key_Right : Qt::Key_Left);
      if (tabs->currentIndex()!=target.index) throw std::runtime_error("Tab did not become selected");
      return;
   }
   if (kind=="select") {
      auto *combo=qobject_cast<QComboBox *>(widget);
      if (!combo) throw std::runtime_error("Select target is not a combo box");
      const auto expected=action["value"].toString(); const int index=combo->findText(expected);
      if (index<0) throw std::runtime_error("Combo choice is unavailable: "+expected.toStdString());
      if (combo->currentIndex()==index) return;
      QTest::mouseClick(combo,Qt::LeftButton);
      auto *receiver=focus(root); QTest::keyClick(receiver,Qt::Key_Home);
      for (int i=0;i<index;++i) QTest::keyClick(focus(root),Qt::Key_Down);
      QTest::keyClick(focus(root),Qt::Key_Return);
      if (combo->currentText()!=expected) throw std::runtime_error("Combo selection did not commit");
      return;
   }
   if (kind=="check") {
      const bool expected=action["checked"].toBool();
      if (target.role=="list") {
         auto *list=qobject_cast<QListWidget *>(target.widget.data()); auto *item=list->item(target.index);
         if (!item->flags().testFlag(Qt::ItemIsUserCheckable)) throw std::runtime_error("List item is not checkable");
         if ((item->checkState()==Qt::Checked)!=expected) { QTest::mouseClick(widget,Qt::LeftButton,Qt::NoModifier,target.point); QTest::keyClick(list,Qt::Key_Space); }
         if ((item->checkState()==Qt::Checked)!=expected) throw std::runtime_error("List check state did not commit");
      } else {
         auto *button=qobject_cast<QAbstractButton *>(widget);
         if (!button || !button->isCheckable()) throw std::runtime_error("Control is not checkable");
         if (button->isChecked()!=expected) QTest::mouseClick(button,Qt::LeftButton,Qt::NoModifier,buttonPoint(button));
         if (button->isChecked()!=expected) throw std::runtime_error("Check state did not commit");
      }
      return;
   }
   if (kind=="click") {
      const auto button=action["button"].toInt(1)==3 ? Qt::RightButton : Qt::LeftButton;
      if (auto *control=qobject_cast<QAbstractButton *>(widget)) target.point=buttonPoint(control);
      QTest::mouseClick(widget,button,Qt::NoModifier,target.point);
      if (action["count"].toInt(1)==2) QTest::mouseDClick(widget,button,Qt::NoModifier,target.point);
      if (button==Qt::RightButton && !frame->popupSeen && !QApplication::activePopupWidget()) {
         QContextMenuEvent event(QContextMenuEvent::Mouse,target.point,widget->mapToGlobal(target.point));
         QCoreApplication::sendEvent(widget,&event);
      }
      return;
   }
   if (kind=="key" || kind=="text") {
      if (action.contains("target_name") || action.contains("field_hint"))
         if (QApplication::focusWidget()!=widget && QApplication::focusWidget()!=target.widget.data())
            QTest::mouseClick(widget,Qt::LeftButton,Qt::NoModifier,target.point);
      if (kind=="text") {
         if (!editable(widget)) throw std::runtime_error("Text input requires an editable, identified GUI field");
         const auto text=action["text"].toString();
         for (const auto character:text) if (character.unicode()<32 || character.unicode()>126) throw std::runtime_error("Tutorial text must be printable ASCII");
         QString expected;
         if (auto *line=qobject_cast<QLineEdit *>(widget)) {
            const int start=line->hasSelectedText() ? line->selectionStart() : line->cursorPosition();
            expected=line->text().left(start)+text+line->text().mid(start+line->selectedText().size());
         } else if (auto *editor=qobject_cast<QPlainTextEdit *>(widget)) {
            const auto cursor=editor->textCursor();
            expected=editor->toPlainText().left(cursor.selectionStart())+text+editor->toPlainText().mid(cursor.selectionEnd());
         }
         QTest::keyClicks(widget,text,Qt::NoModifier,0);
         if (auto *line=qobject_cast<QLineEdit *>(widget);line && line->text()!=expected) throw std::runtime_error("GUI field did not retain the typed value");
         if (auto *editor=qobject_cast<QPlainTextEdit *>(widget);editor && editor->toPlainText()!=expected) throw std::runtime_error("GUI editor did not retain the typed text");
      } else {
         auto keys=action["keys"].toString();
         keys.replace(QRegularExpression("\\bCTRL\\b",QRegularExpression::CaseInsensitiveOption),"Ctrl");
         keys.replace(QRegularExpression("\\bESC\\b",QRegularExpression::CaseInsensitiveOption),"Esc");
         keys.replace(QRegularExpression("\\bENTER\\b",QRegularExpression::CaseInsensitiveOption),"Return");
         const auto sequence=QKeySequence::fromString(keys,QKeySequence::PortableText);
         if (sequence.count()!=1 || sequence[0].key()==Qt::Key_unknown || sequence[0].key()==0) throw std::runtime_error("Unsupported GUI key sequence");
         QTest::keyClick(widget,sequence[0].key(),sequence[0].keyboardModifiers(),0);
      }
      return;
   }
   throw std::runtime_error("Unsupported QtTest GUI input action");
}

void TutorialQtTestDriver::poll()
{
   if (!frames.empty()) {
      const auto &frame=frames.back();
      const bool modal=QApplication::activeModalWidget() && QApplication::activeModalWidget()!=frame->modal;
      const bool popup=frame->allowPopup && QApplication::activePopupWidget() && QApplication::activePopupWidget()!=frame->popup;
      if (!modal && !popup) return;
      frame->popupSeen|=popup;
      record("input-suspended/"+frame->id+".json",{{"id",frame->id},{"reason",modal ? "modal dialog" : "context menu"}});
   }
   QFile file(directory+"/input-request.json"); if (!file.open(QIODevice::ReadOnly)) return;
   QJsonParseError error; const auto request=QJsonDocument::fromJson(file.readAll(),&error).object();
   if (error.error!=QJsonParseError::NoError) return;
   const auto id=request["id"].toString(); if (id.isEmpty() || id==last || !QRegularExpression("^[0-9]+$").match(id).hasMatch()) return;
   const auto action=request["action"].toObject();
   Target target;
   try { target=locate(action); }
   catch (const std::exception &problem) { record("input-pending.json",{{"id",id},{"error",problem.what()}}); return; }
   last=id;
   record("input-accepted/"+id+".json",{{"id",id},{"framework","QtTest"},{"target",target.identity},{"action",action}});
   QTimer::singleShot(0,&root,[this,id,action,target] {
      auto frame=std::make_shared<Frame>(); frame->id=id; frame->modal=QApplication::activeModalWidget(); frame->popup=QApplication::activePopupWidget(); frame->allowPopup=action["action"]=="click" && action["button"].toInt(1)==3;
      frames.push_back(frame);
      QJsonObject result{{"id",id},{"framework","QtTest"},{"ok",true},{"target",target.identity}};
      try { execute(action,target,frame); }
      catch (const std::exception &problem) { result["ok"]=false; result["error"]=problem.what(); }
      record("input-finished/"+id+".json",result);
      frames.erase(std::find(frames.begin(),frames.end(),frame));
   });
}
