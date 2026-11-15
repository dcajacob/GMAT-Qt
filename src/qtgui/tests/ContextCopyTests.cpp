#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTest>
#include <QTextCursor>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
void events()
{
   QApplication::processEvents();
   QTest::qWait(10);
}
QTreeWidgetItem *resourceItem(QTreeWidget *tree,const QString &name)
{
   for (auto *item:tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive))
      if (!item->data(0,Qt::UserRole).toString().isEmpty()) return item;
   throw std::runtime_error(("Resource is missing: "+name).toStdString());
}
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication application(argc,argv);
   application.setOrganizationName("GMATTests");
   application.setApplicationName("ContextCopy");
   TestSettings settings;
   try {
      require(argc==2,"Supply a startup file");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window;
      window.resize(1400,900);
      window.show();
      require(window.initialize(startup),"Copy test runtime initialization failed");
      require(QTest::qWaitForWindowExposed(&window),"Copy test window was not exposed");
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto *tree=window.findChild<QTreeWidget *>("Resources");
      auto *area=window.findChild<QMdiArea *>("workspace");
      auto *copy=window.findChild<QAction *>("edit_copy");
      require(source && tree && area && copy,"Copy controls are missing");
      const QString baseline="Create Spacecraft Sat;\nCreate Propagator P;\nBeginMissionSequence;\n";
      source->setPlainText(baseline);
      require(window.buildScript(),"Copy fixture did not build");
      QMenu *edit=nullptr;
      for (auto *action:window.menuBar()->actions())
         if (action->text().remove('&')=="Edit") edit=action->menu();
      require(edit,"Edit menu is missing");
      const QStringList removed{"cloneResource","clonePropagator","copyCoordinateSystem","copyCalculatedPoint","copyCelestialBody"};
      for (auto *action:edit->actions())
         require(!removed.contains(action->objectName()),"An object-specific copy remains in the Edit menu");

      const auto selectResource=[&](const QString &name) {
         auto *item=resourceItem(tree,name);
         tree->expandAll(); tree->setCurrentItem(item); tree->scrollToItem(item);
         window.activateWindow(); events();
         QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(item).center());
         tree->setFocus(); events();
         require(tree->hasFocus(),"Resource tree did not receive focus");
      };
      const auto duplicate=[&](const QString &name,bool keyboard,bool commit) {
         selectResource(name);
         const auto before=source->toPlainText();
         const auto undo=source->document()->availableUndoSteps();
         QApplication::clipboard()->setText("resource copy sentinel");
         bool visited=false;
         std::exception_ptr failure;
         const auto inspect=[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
            try {
               require(dialog && dialog->objectName()=="cloneResourceDialog","Context Copy did not open the resource copy dialog");
               bool matches=false;
               for (auto *label:dialog->findChildren<QLabel *>())
                  matches=matches || label->text().startsWith(name+" — ");
               require(matches,"Context Copy opened a different resource");
               auto *field=dialog->findChild<QLineEdit *>("resourceName");
               require(field && !field->placeholderText().isEmpty(),"Copy did not propose an available resource name");
               visited=true;
               if (commit) {
                  field->setText("SatCopy");
                  dialog->findChild<QDialogButtonBox *>("resourceCloneButtons")->button(QDialogButtonBox::Ok)->click();
                  require(dialog->result()==QDialog::Accepted,"Context Copy did not commit the resource");
               } else dialog->reject();
            } catch (...) {
               failure=std::current_exception();
               if (dialog) dialog->reject();
            }
         };
         if (keyboard) {
            QTimer::singleShot(0,&window,inspect);
            QTest::keyClick(tree,Qt::Key_C,Qt::ControlModifier);
         } else {
            QTimer::singleShot(0,&window,[&] {
               QTimer::singleShot(0,&window,inspect);
               QTest::mouseClick(edit,Qt::LeftButton,Qt::NoModifier,edit->actionGeometry(copy).center());
            });
            edit->exec(window.menuBar()->mapToGlobal(QPoint(40,window.menuBar()->height())));
         }
         events();
         if (failure) std::rethrow_exception(failure);
         require(visited,"Context Copy was not dispatched");
         require(QApplication::clipboard()->text()=="resource copy sentinel","Resource copy unexpectedly replaced text on the clipboard");
         if (!commit)
            require(source->toPlainText()==before && source->document()->availableUndoSteps()==undo,"Cancelling resource Copy changed source or history");
      };
      duplicate("Sat",false,false);
      for (const auto &name:QStringList{"P","EarthMJ2000Eq","SolarSystemBarycenter","Earth"}) duplicate(name,true,false);
      duplicate("Sat",true,true);
      auto *original=Moderator::Instance()->GetConfiguredObject("Sat");
      auto *cloned=Moderator::Instance()->GetConfiguredObject("SatCopy");
      require(original && cloned && original!=cloned && original->GetRealParameter("DryMass")==cloned->GetRealParameter("DryMass"),
              "Context Copy did not create an independent configured resource");
      source->undo();
      require(source->toPlainText()==baseline,"Context Copy did not make one undoable source edit");
      require(window.buildScript(),"Undo rebuild failed");

      selectResource("SolarSystem");
      copy->trigger(); events();
      require(!QApplication::activeModalWidget() && source->toPlainText()==baseline,"Protected resource Copy changed the model");

      auto *script=source->parentWidget();
      while (script && !qobject_cast<QMdiSubWindow *>(script)) script=script->parentWidget();
      require(script,"Script window is missing");
      script->showMaximized();
      area->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(script));
      source->setFocus();
      QTextCursor selection(source->document());
      selection.setPosition(0); selection.setPosition(6,QTextCursor::KeepAnchor); source->setTextCursor(selection);
      events(); copy->trigger();
      require(QApplication::clipboard()->text()=="Create" && source->toPlainText()==baseline,"Script Copy lost its text selection");

      auto *field=new QLineEdit("readonly text"); field->setReadOnly(true);
      auto *fieldWindow=area->addSubWindow(field); fieldWindow->show(); area->setActiveSubWindow(fieldWindow);
      field->setFocus(); field->selectAll(); events(); copy->trigger();
      require(QApplication::clipboard()->text()=="readonly text","Read-only field Copy stopped working");
      auto *button=new QPushButton("Unrelated control");
      auto *buttonWindow=area->addSubWindow(button); buttonWindow->show(); area->setActiveSubWindow(buttonWindow);
      button->setFocus(); events(); copy->trigger();
      require(!QApplication::activeModalWidget() && QApplication::clipboard()->text()=="readonly text", "Unrelated focus reused a stale Copy target");

      source->appendPlainText("% unbuilt change");
      const auto dirty=source->toPlainText();
      selectResource("Sat"); copy->trigger(); events();
      require(!QApplication::activeModalWidget() && source->toPlainText()==dirty,"Resource Copy bypassed the build guard");
      const QString directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (!directory.isEmpty()) {
         QDir().mkpath(directory);
         edit->popup(window.menuBar()->mapToGlobal(QPoint(40,window.menuBar()->height()))); events();
         require(edit->grab().save(directory+"/edit-menu.png"),"Could not save the cleaned Edit menu");
         edit->hide();
      }
      std::cout<<"PASS cleaned Edit menu, resource Copy menu and shortcut dispatch for ordinary resources, propagators, built-in frames, calculated points and bodies, cancel/commit/undo, text and read-only fields, focus isolation and stale-model guard\n";
      return 0;
   } catch (const std::exception &error) {
      std::cerr<<error.what()<<'\n'; return 1;
   }
}
