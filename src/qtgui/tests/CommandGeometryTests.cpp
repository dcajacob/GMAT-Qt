#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   try {
      require(argc==2,"Expected startup path"); TestSettings settings;
      MainWindow window; window.resize(1280,1000); window.show(); require(window.initialize(QString::fromLocal8Bit(argv[1])),"Initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      // Achieve and Vary are offered only inside a Target branch. Geometry
      // checks must use that legal insertion context, even without execution.
      const QString source="% Preserve pending command source Δ\nCreate Spacecraft Sat;\nCreate ImpulsiveBurn Burn;\nCreate DifferentialCorrector DC;\nBeginMissionSequence;\nTarget DC;\nStop;\nEndTarget;\n";
      editor->setPlainText(source); require(window.buildScript(),"Geometry fixture failed to build");
      auto *tree=window.findChild<QTreeWidget *>("Mission"); auto *area=window.findChild<QMdiArea *>("workspace"); require(tree && area,"Mission UI absent");
      struct Cleanup { QMdiArea *area; ~Cleanup() { for(auto *child:area->subWindowList()) if(dynamic_cast<CommandEditor *>(child->widget())) dynamic_cast<CommandEditor *>(child->widget())->discardChanges(); } } cleanup{area};
      auto *tabs=window.findChild<QTabWidget *>(); for(int i=0;tabs && i<tabs->count();++i) if(tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
      QMdiSubWindow *script=nullptr; for(auto *child:area->subWindowList()) if(child->widget()==editor || child->widget()->isAncestorOf(editor)) script=child;
      require(script,"Script subwindow absent");
      const auto open=[&] {
         QTreeWidgetItem *item=nullptr; for(QTreeWidgetItemIterator it(tree);*it;++it) if((*it)->data(0,Qt::UserRole).isValid() && window.missionSnapshot().nodes[(*it)->data(0,Qt::UserRole).toInt()].type=="Stop") item=*it;
         require(item,"Insertion anchor absent"); tree->expandAll(); tree->scrollToItem(item); app.processEvents();
         std::exception_ptr failure; bool visited=false;
         QTimer::singleShot(0,&window,[&] {
            auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
            try {
               require(menu,"Insertion menu absent"); QAction *choice=nullptr; for(auto *a:menu->actions()) if(a->text()=="Insert before…") choice=a;
               require(choice && choice->isEnabled(),"Insertion choice absent"); visited=true; menu->setActiveAction(choice);
               QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter);
            } catch(...) { failure=std::current_exception(); if(menu) menu->close(); }
         });
         tree->customContextMenuRequested(tree->visualItemRect(item).center()); if(failure) std::rethrow_exception(failure);
         require(visited,"Insertion action not visited"); app.processEvents(); app.processEvents();
         auto *child=area->activeSubWindow(); require(child && dynamic_cast<CommandEditor *>(child->widget()),"Actual insertion panel absent"); return child;
      };
      const auto check=[&](QMdiSubWindow *child,const QString &type) {
         auto *choice=child->widget()->findChild<QComboBox *>("commandTemplate"); require(choice && choice->findText(type)>=0,"Template absent"); choice->setCurrentText(type); choice->textActivated(type); app.processEvents(); app.processEvents();
         std::cout<<type.toStdString()<<" geometry "<<child->width()<<"x"<<child->height()<<'\n';
         require(!child->isMaximized() && area->viewport()->rect().contains(child->geometry()),"New normal command panel escaped viewport");
         require(child->width()>=qMin(700,area->viewport()->width()) && child->height()>=qMin(500,area->viewport()->height()),"New command panel lost its requested usable size");
         auto *preview=child->widget()->findChild<QPlainTextEdit *>("commandSource"); auto *status=child->widget()->findChild<QLabel *>("commandStatus");
         auto *scroll=child->widget()->findChild<QScrollArea *>("commandTypedControls"); auto *buttons=child->widget()->findChild<QDialogButtonBox *>(); auto *apply=buttons ? buttons->button(QDialogButtonBox::Apply) : nullptr;
         require(preview && status && scroll && apply && preview->isVisible() && apply->isVisible(),"Required command controls absent");
         const auto rect=[&](QWidget *widget) { return QRect(widget->mapTo(child,QPoint()),widget->size()); };
         require(child->rect().contains(rect(preview)) && child->rect().contains(rect(apply)) && !rect(preview).intersects(rect(status)) && !rect(scroll).intersects(rect(status)),"Preview, typed controls, status or Apply overlap or clip");
         require(editor->toPlainText()==source,"Pending command geometry changed source");
      };
      script->showMaximized(); app.processEvents(); auto *first=open(); check(first,"Achieve");
      first->showMaximized(); app.processEvents(); auto *second=open(); check(second,"Vary");
      dynamic_cast<CommandEditor *>(second->widget())->discardChanges(); dynamic_cast<CommandEditor *>(first->widget())->discardChanges();
      second->close(); first->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); app.processEvents();
      script->showNormal(); app.processEvents(); auto *third=open(); check(third,"Propagate");
      third->resize(780,580); app.processEvents(); const auto manual=third->geometry(); area->setActiveSubWindow(script); area->setActiveSubWindow(third); app.processEvents();
      require(third->geometry()==manual,"Reactivation overwrote a user-sized command panel");
      std::cout<<"PASS actual command insertion after maximized script/panel, normal restoration, preview/status/Apply reachability, pending source and manual-size preservation. No mission rerun or host acceptance.\n";
   } catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
