#pragma once
#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTest>
#include <QTimer>

namespace function_scope_ui {
using namespace formaccept;
inline void reveal(QWidget *widget)
{
   require(widget,"Function scope control absent");
   for(auto *page=widget;page;page=page->parentWidget())
      for(auto *tabs:widget->window()->findChildren<QTabWidget*>())if(tabs->indexOf(page)>=0)tabs->setCurrentWidget(page);
   events();require(widget->isVisible()&&widget->isEnabled(),"Function scope control unreachable");acceptanceView(*widget);
   auto *top=widget->window();top->raise();top->activateWindow();QApplication::setActiveWindow(top);widget->setFocus(Qt::OtherFocusReason);events();
}
template<class Prototypes>
QJsonArray editFunction(Rig &rig,const QString &path,const QString &original,const QString &edited,const QString &caller,Prototypes prototypes)
{
   const auto before=prototypes();const int history=rig.source->document()->availableUndoSteps();
   for(auto *tabs:rig.window.template findChildren<QTabWidget*>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==rig.resources||tabs->widget(i)->isAncestorOf(rig.resources))tabs->setCurrentIndex(i);
   const auto entries=rig.resources->findItems("ReadShared",Qt::MatchExactly|Qt::MatchRecursive);require(entries.size()==1,"Function scope resource absent");rig.resources->scrollToItem(entries[0]);rig.resources->itemDoubleClicked(entries[0],0);events();
   auto *owner=rig.area->activeSubWindow();auto *panel=owner?dynamic_cast<ResourceEditor*>(owner->widget()):nullptr;require(panel,"Function scope editor absent");owner->showNormal();owner->move(0,0);owner->resize(rig.area->viewport()->size());events();auto *edit=panel->findChild<QPushButton*>("editFunctionFile");QJsonArray decisions;
   for(bool commit:{false,true})
   {
      bool visited=false;std::exception_ptr failure;QTimer timer;timer.setInterval(20);
      QObject::connect(&timer,&QTimer::timeout,panel,[&]{auto *dialog=panel->findChild<QDialog*>("functionFileDialog");if(!dialog)return;timer.stop();try{auto *text=dialog->findChild<QPlainTextEdit*>("functionFileText");require(text&&!text->isReadOnly()&&text->toPlainText()==original,"Function scope initial file differs");reveal(text);QTest::mouseClick(text->viewport(),Qt::LeftButton);QTest::keyClick(text,Qt::Key_A,Qt::ControlModifier);QApplication::clipboard()->setText(edited);QTest::keyClick(text,Qt::Key_V,Qt::ControlModifier);events();require(text->toPlainText()==edited&&read(path)==original.toUtf8()&&rig.source->toPlainText()==caller&&rig.source->document()->availableUndoSteps()==history&&prototypes()==before,"Function draft changed caller/history/file/prototypes");auto *button=dialog->findChild<QDialogButtonBox*>()->button(commit?QDialogButtonBox::Save:QDialogButtonBox::Cancel);reveal(button);QTest::mouseClick(button,Qt::LeftButton);visited=true;}catch(...){failure=std::current_exception();dialog->reject();}});
      reveal(edit);timer.start();QTest::mouseClick(edit,Qt::LeftButton);timer.stop();if(failure)std::rethrow_exception(failure);require(visited&&read(path)==(commit?edited:original).toUtf8()&&rig.source->toPlainText()==caller,"Function scope Save/Cancel differs");decisions.append(QJsonObject{{"saved",commit}});
   }
   require(owner->close(),"Function scope editor failed to close");events();return decisions;
}
}
