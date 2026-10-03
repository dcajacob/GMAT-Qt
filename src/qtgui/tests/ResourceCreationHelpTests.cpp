#include "MainWindow.hpp"
#include "HelpDialog.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTimer>
#include <QTreeWidget>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void key(QWidget *target,Qt::Key value)
{
   QKeyEvent down(QEvent::KeyPress,value,Qt::NoModifier),up(QEvent::KeyRelease,value,Qt::NoModifier);
   QApplication::sendEvent(target,&down); QApplication::sendEvent(target,&up); QApplication::processEvents();
}
static GmatBase *object(const QString &name) { return Moderator::Instance()->GetConfiguredObject(name.toStdString()); }
static ResourceEditor *visiblePanel(QDialog *dialog)
{
   for (auto *widget:dialog->findChildren<QWidget *>())
      if (auto *panel=dynamic_cast<ResourceEditor *>(widget);panel && panel->isVisible()) return panel;
   throw std::runtime_error("Visible resource draft missing");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtResourceCreationHelp");
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","Creation Help requires offscreen startup");
      TestSettings settings; const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Creation Help runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources"); require(editor && tree,"Creation Help workspace missing");
      const QString baseline="% retain creation Help source α\nCreate Spacecraft Vehicle;\nCreate Variable Keep;\nKeep = 7; % preserve value\nBeginMissionSequence;\n";
      editor->setPlainText(baseline); require(window.buildScript(),"Creation Help fixture failed to build");
      const auto missionBefore=window.missionSnapshot(); const bool modifiedBefore=editor->document()->isModified(); auto *keepBefore=object("Keep"); require(keepBefore,"Clone source variable missing");
      const auto helpRoot=QString::fromStdString(FileManager::Instance()->GetFullPathname("HELP_PATH"));
      const auto checkHelp=[&](QWidget *owner,const QString &page,const QString &title) {
         HelpDialog *help=nullptr;
         for (auto *candidate:window.findChildren<QDialog *>("helpDialog"))
            if (auto *found=dynamic_cast<HelpDialog *>(candidate);found && found->isVisible()) { require(!help,"Several Help dialogs opened"); help=found; }
         require(help && help->parentWidget()==owner,"Help did not open under the active creation dialog");
         auto *browser=help->findChild<QTextBrowser *>("helpBrowser"); auto *status=help->findChild<QLabel *>("helpStatus");
         const auto expected=QDir(helpRoot).filePath("html/"+page+".html");
         require(QFileInfo(expected).isReadable(),"Selected offline resource Help is not installed/readable");
         if (!browser || !status || browser->source().toLocalFile()!=expected || !browser->toPlainText().contains(title) || !status->text().isEmpty())
            std::cerr<<"Expected Help "<<expected.toStdString()<<" / "<<title.toStdString()<<", got "<<(browser ? browser->source().toString().toStdString() : "missing browser")<<" status="<<(status ? status->text().toStdString() : "missing status")<<'\n';
         require(browser && status && browser->source().toLocalFile()==expected && browser->toPlainText().contains(title) && status->text().isEmpty() && browser->isReadOnly(),"Wrong or unreadable selected-resource Help");
         key(help,Qt::Key_Escape); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); QApplication::processEvents();
      };
      const auto topic=[&](QDialog *dialog,const char *buttonName,const QString &page,const QString &title) {
         auto *buttons=dialog->findChild<QDialogButtonBox *>(buttonName); auto *name=dialog->findChild<QLineEdit *>("resourceName");
         require(buttons && name,"Creation Help controls missing"); auto *help=buttons->button(QDialogButtonBox::Help);
         require(help && help->isVisible() && help->isEnabled(),"Visible outer creation Help button missing");
         const auto pendingName=name->text(); help->click(); checkHelp(dialog,page,title);
         require(dialog->isVisible() && name->text()==pendingName,"Help button closed creator or changed pending name");
         dialog->activateWindow(); name->setFocus(); QApplication::processEvents(); key(name,Qt::Key_F1); checkHelp(dialog,page,title);
         require(dialog->isVisible() && name->text()==pendingName,"F1 closed creator or changed pending name");
         require(editor->toPlainText()==baseline && !object(pendingName),"Help created a draft or changed mission source");
      };
      const auto creator=[&](const std::function<void(QDialog *)> &inspect) {
         std::exception_ptr failure;
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try { require(dialog,"Unified creation dialog missing"); inspect(dialog); } catch (...) { failure=std::current_exception(); }
            if (dialog) dialog->reject();
         });
         auto *action=window.findChild<QAction *>("createResource"); require(action && action->isEnabled(),"General resource creator unavailable"); action->trigger();
         if (failure) std::rethrow_exception(failure);
      };
      creator([&](QDialog *dialog) {
         auto *types=dialog->findChild<QComboBox *>("resourceType"); auto *name=dialog->findChild<QLineEdit *>("resourceName"); require(types && name,"Creator type/name missing"); name->setText("PendingCreationHelp");
         const QList<QStringList> cases={{"Spacecraft","Spacecraft","Spacecraft"},{"ChemicalTank","FuelTank","ChemicalTank"},{"PropSetup","Propagator","Propagator"},
            {"GmatFunction","GmatFunction","GMATFunction"},{"GroundStation","GroundStation","GroundStation"},{"EclipseLocator","EclipseLocator","EclipseLocator"},
            {"Variable","Variable","Variable"},{"Array","Array","Array"},{"String","String","String"},{"Planet","CelestialBody","CelestialBody"},
            {"Asteroid","CelestialBody","CelestialBody"},{"Moon","CelestialBody","CelestialBody"},{"Comet","CelestialBody","CelestialBody"},
            {"ConicalFOV","FieldOfView","FieldOfView"},{"RectangularFOV","FieldOfView","FieldOfView"},{"CustomFOV","FieldOfView","FieldOfView"}};
         for (const auto &entry:cases) {
            require(types->findText(entry[0])>=0,qPrintable("Expected supported creator type missing: "+entry[0])); types->setCurrentText(entry[0]);
            topic(dialog,"resourceCreationButtons",entry[1],entry[2]);
            require(name->text()=="PendingCreationHelp","Type-switch Help lost entered name");
            if (entry[0]!="GmatFunction") require(!visiblePanel(dialog)->findChild<QDialogButtonBox *>("resourceButtons")->isVisible(),"Creator exposed duplicate editor Apply/Close controls");
         }
         types->setCurrentText("Variable"); auto *value=visiblePanel(dialog)->findChild<QLineEdit *>("parameterValue"); require(value,"Variable draft input missing"); value->setText("-17.25");
         topic(dialog,"resourceCreationButtons","Variable","Variable"); require(value->text()=="-17.25","Variable Help lost pending value");
         types->setCurrentText("String"); auto *text=visiblePanel(dialog)->findChild<QLineEdit *>("parameterValue"); require(text,"String draft input missing"); text->setText("pending help α");
         topic(dialog,"resourceCreationButtons","String","String"); require(text->text()=="pending help α","String Help lost pending text");
         types->setCurrentText("Array"); auto *rows=dialog->findChild<QSpinBox *>("arrayRows"),*columns=dialog->findChild<QSpinBox *>("arrayColumns"); require(rows && columns,"Pending array dimensions missing"); rows->setValue(2); columns->setValue(3);
         auto *arrayPanel=visiblePanel(dialog); auto *matrix=arrayPanel->findChild<QLineEdit *>("resource_RmatValue"); require(matrix,"Pending array value input missing"); matrix->setText("1 2 3; 4 5 6");
         topic(dialog,"resourceCreationButtons","Array","Array"); require(rows->value()==2 && columns->value()==3 && matrix->text()=="1 2 3; 4 5 6","Array Help lost pending dimensions/cells");
         // Use the actual nested Array cells dialog, whose inherited topic
         // must follow the currently selected creator type.
         std::exception_ptr nestedFailure;
         QTimer::singleShot(0,dialog,[&] {
            auto *gridDialog=dialog->findChild<QDialog *>("numericGridDialog");
            try {
               require(gridDialog,"Nested draft Array cells dialog missing"); auto *grid=gridDialog->findChild<QTableWidget *>("numericGrid"); require(grid && grid->rowCount()==2 && grid->columnCount()==3,"Nested pending Array dimensions changed");
               grid->item(0,0)->setText("99.5"); auto *gridButtons=gridDialog->findChild<QDialogButtonBox *>(); require(gridButtons && gridButtons->button(QDialogButtonBox::Help) && gridButtons->button(QDialogButtonBox::Help)->isVisible(),"Nested creation cells lacks visible Help");
               gridButtons->button(QDialogButtonBox::Help)->click(); checkHelp(gridDialog,"Array","Array");
               key(grid,Qt::Key_F1); checkHelp(gridDialog,"Array","Array"); require(grid->item(0,0)->text()=="99.5","Nested Help discarded pending Array cell");
            } catch (...) { nestedFailure=std::current_exception(); }
            if (gridDialog) gridDialog->reject();
         });
         auto *editCells=arrayPanel->findChild<QPushButton *>("editCells_RmatValue"); require(editCells,"Nested creation Array control missing"); editCells->click(); if (nestedFailure) std::rethrow_exception(nestedFailure);
         require(matrix->text()=="1 2 3; 4 5 6" && rows->value()==2 && columns->value()==3,"Nested cells Cancel or Help changed parent pending array");
         types->setCurrentText("Variable"); require(visiblePanel(dialog)->findChild<QLineEdit *>("parameterValue")->text()=="-17.25","Type-switch Help lost Variable pending value");
         types->setCurrentText("String"); require(visiblePanel(dialog)->findChild<QLineEdit *>("parameterValue")->text()=="pending help α","Type-switch Help lost String pending text");
         types->setCurrentText("GmatFunction"); auto *functionPath=dialog->findChild<QLineEdit *>("resourceFunctionPath"); require(functionPath,"Function creator input missing"); functionPath->setText("pending-function.gmf");
         topic(dialog,"resourceCreationButtons","GmatFunction","GMATFunction"); require(functionPath->text()=="pending-function.gmf","Function Help lost pending path");
      });
      require(editor->toPlainText()==baseline && !object("PendingCreationHelp"),"General creator Help/Cancel changed mission");
      // Exercise an actual category-menu Add route and its fixed type.
      QTreeWidgetItem *category=nullptr; for (auto *item:tree->findItems("Ground Stations",Qt::MatchExactly|Qt::MatchRecursive)) if (item->data(0,Qt::UserRole+1).isValid()) category=item;
      require(category,"Ground Stations category missing"); tree->scrollToItem(category); QApplication::processEvents(); std::exception_ptr categoryFailure; bool categoryOpened=false;
      QTimer::singleShot(0,&window,[&] {
         auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
         try {
            require(menu,"Category menu missing"); QAction *add=nullptr; for (auto *candidate:menu->actions()) if (candidate->objectName()=="addResource_GroundStation") add=candidate; require(add && add->isEnabled(),"Category GroundStation Add missing");
            QTimer::singleShot(0,&window,[&] {
               auto *dialog=window.findChild<QDialog *>("newResourceDialog");
               try { require(dialog,"Category creator missing"); categoryOpened=true; auto *type=dialog->findChild<QComboBox *>("resourceType"); require(type && type->count()==1 && type->currentText()=="GroundStation" && !type->isEnabled(),"Category creator type changed"); dialog->findChild<QLineEdit *>("resourceName")->setText("PendingCategoryHelp"); topic(dialog,"resourceCreationButtons","GroundStation","GroundStation"); }
               catch (...) { categoryFailure=std::current_exception(); } if (dialog) dialog->reject();
            });
            // Let this popup callback return before the queued creator check;
            // the creation dialog is dispatched only after QMenu::exec exits.
            menu->setActiveAction(add); QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter);
         } catch (...) { categoryFailure=std::current_exception(); if (menu) menu->close(); }
      });
      tree->customContextMenuRequested(tree->visualItemRect(category).center()); if (categoryFailure) std::rethrow_exception(categoryFailure); require(categoryOpened && !object("PendingCategoryHelp") && editor->toPlainText()==baseline,"Category Help/Cancel created a resource or changed source");
      // Clone has its own outer controls; pending copy edits are never committed.
      const auto keeps=tree->findItems("Keep",Qt::MatchExactly|Qt::MatchRecursive); require(keeps.size()==1,"Clone source tree item missing"); tree->setCurrentItem(keeps.first()); std::exception_ptr cloneFailure;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=window.findChild<QDialog *>("cloneResourceDialog");
         try { require(dialog,"Clone dialog missing"); dialog->findChild<QLineEdit *>("resourceName")->setText("PendingCloneHelp"); auto *value=visiblePanel(dialog)->findChild<QLineEdit *>("parameterValue"); require(value,"Clone Variable input missing"); value->setText("99.5"); topic(dialog,"resourceCloneButtons","Variable","Variable"); require(value->text()=="99.5","Clone Help lost pending copy value"); }
         catch (...) { cloneFailure=std::current_exception(); } if (dialog) dialog->reject();
      });
      auto *clone=window.findChild<QAction *>("cloneResource"); require(clone && clone->isEnabled(),"Clone action missing"); clone->trigger(); if (cloneFailure) std::rethrow_exception(cloneFailure);
      require(!object("PendingCloneHelp") && object("Keep")==keepBefore && keepBefore->GetRealParameter("Value")==7,"Clone Help/Cancel changed resources");
      require(editor->toPlainText()==baseline && editor->document()->isModified()==modifiedBefore && window.missionSnapshot().sourceScript==missionBefore.sourceScript && window.missionSnapshot().nodes.size()==missionBefore.nodes.size(),"Creation Help changed source/dirty state or loaded mission");
      std::cout<<"PASS visible outer Help and name-field F1 for 16 general creation types with installed readable resource topics/aliases; retained pending names/scalars/array cells/function path and actual nested Array Help; category GroundStation and Variable Clone Help/Cancel without resource creation, source/dirty-state or model change. No numerical mission execution or old-suite repeat.\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<'\n'; return 1; }
}
