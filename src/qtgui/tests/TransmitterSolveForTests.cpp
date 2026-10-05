#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static GmatBase *object(const QString &name) { return Moderator::Instance()->GetConfiguredObject(name.toStdString()); }
static QStringList solveFors()
{
   auto *radio=object("Radio"); require(radio,"Configured transmitter missing"); QStringList values;
   for (const auto &value:radio->GetStringArrayParameter("SolveFors")) values.append(QString::fromStdString(value));
   return values;
}
static ResourceEditor *draft(QDialog *dialog)
{
   for (auto *widget:dialog->findChildren<QWidget *>())
      if (auto *panel=dynamic_cast<ResourceEditor *>(widget);panel && panel->isVisible()) return panel;
   throw std::runtime_error("Visible transmitter creation draft missing");
}
static ResourceEditor *panel(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (child->property("resourceName")=="Radio") {
         auto *found=dynamic_cast<ResourceEditor *>(child->widget()); require(found,"Transmitter editor missing"); return found;
      }
   throw std::runtime_error("Transmitter workspace panel missing");
}
static QLineEdit *input(ResourceEditor *panel)
{
   auto *value=panel->findChild<QLineEdit *>("resource_SolveFors");
   require(value && value->isVisible() && !value->isReadOnly(),"Writable transmitter SolveFors GUI control is omitted"); return value;
}
static void choose(ResourceEditor *panel,bool selected,bool accept,bool expectedSelected)
{
   auto *value=input(panel); QPushButton *picker=nullptr;
   for (auto *button:value->parentWidget()->findChildren<QPushButton *>(QString(),Qt::FindDirectChildrenOnly))
      if (button->isVisible() && button->isEnabled()) { require(!picker,"Several SolveFors pickers found"); picker=button; }
   require(picker,"Visible transmitter SolveFors list picker missing"); std::exception_ptr failure;
   QTimer::singleShot(0,panel,[&] {
      auto *dialog=panel->findChild<QDialog *>("resourceSelectionDialog");
      try {
         require(dialog && dialog->isVisible(),"Actual SolveFors selection dialog missing");
         auto *list=dialog->findChild<QListWidget *>("resourceSelectionList");
         require(list && list->count()==1 && list->item(0)->text()=="FrequencyBias","Picker must offer only the engine-supported transmitter solve-for");
         require((list->item(0)->checkState()==Qt::Checked)==expectedSelected,"Reopened picker lost pending/applied solve-for selection");
         list->item(0)->setCheckState(selected ? Qt::Checked : Qt::Unchecked);
         auto *buttons=dialog->findChild<QDialogButtonBox *>(); require(buttons,"Selection actions missing");
         buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
      } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   });
   picker->click(); if (failure) std::rethrow_exception(failure);
}
static void closePanels(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (!child->property("resourceName").toString().isEmpty()) require(child->close(),"Clean resource panel could not close");
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static void discardPanel(MainWindow &window,ResourceEditor *resource)
{
   QMdiSubWindow *child=nullptr;
   for (auto *candidate:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (candidate->widget()==resource) child=candidate;
   require(child,"Pending transmitter panel missing");
   QTimer::singleShot(0,&window,[] {
      if (auto *question=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) question->button(QMessageBox::Discard)->click();
   });
   require(child->close(),"Discard did not close pending transmitter editor"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static ResourceEditor *open(MainWindow &window,QTreeWidget *tree)
{
   QTreeWidgetItem *selected=nullptr;
   for (auto *candidate:tree->findItems("Radio",Qt::MatchExactly|Qt::MatchRecursive))
      if (!candidate->data(0,Qt::UserRole).toString().isEmpty()) selected=candidate;
   require(selected,"Transmitter resource tree entry missing"); tree->setCurrentItem(selected); tree->itemDoubleClicked(selected,0);
   QApplication::processEvents(); return panel(window);
}
static QString withoutRadio(QString source)
{
   source.remove(QRegularExpression("^[ \\t]*Create[ \\t]+Transmitter[ \\t]+Radio;[^\\S\\n]*\\n",QRegularExpression::MultilineOption));
   source.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?Radio\\.[^;\\n]+;[^\\S\\n]*\\n",QRegularExpression::MultilineOption));
   return source;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtTransmitterSolveFors");
   try {
      require(argc==2 && QGuiApplication::platformName()=="offscreen","Transmitter SolveFors test requires offscreen and one startup argument");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Temporary script directory unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Selected-plugin runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources"); require(editor && tree,"Resource workspace missing");
      const QString baseline="% retain exact unrelated source α\nCreate Antenna Dish;\nCreate Variable Keep;\nKeep = 7; % retained value\nBeginMissionSequence;\nKeep = Keep + 2; % retained mission expression\n";
      editor->setPlainText(baseline); const bool built=window.buildScript();
      if (!built) std::cerr<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString();
      require(built,"Antenna/variable fixture did not build");
      const bool undoBefore=editor->document()->isUndoAvailable(),modifiedBefore=editor->document()->isModified();
      const auto creator=[&](const std::function<void(QDialog *)> &inspect) {
         std::exception_ptr failure;
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try {
               require(dialog,"Actual resource creation dialog missing"); auto *types=dialog->findChild<QComboBox *>("resourceType"); auto *name=dialog->findChild<QLineEdit *>("resourceName"); require(types && name,"Creator type/name controls missing");
               require(types->findText("Transmitter")>=0,"Selected runtime does not expose Transmitter creation"); types->setCurrentText("Transmitter"); name->setText("Radio");
               auto *antenna=draft(dialog)->findChild<QLineEdit *>("resource_PrimaryAntenna"); require(antenna,"Primary antenna draft control missing"); antenna->setText("Dish"); inspect(dialog);
            } catch (...) { failure=std::current_exception(); }
            if (dialog && dialog->result()!=QDialog::Accepted) dialog->reject();
         });
         auto *action=window.findChild<QAction *>("createResource"); require(action && action->isEnabled(),"Resource Create action unavailable"); action->trigger(); if (failure) std::rethrow_exception(failure);
      };
      // This first actual GUI assertion also fails meaningfully on the old
      // frontend: its STRINGARRAY whitelist omits Transmitter.SolveFors.
      creator([&](QDialog *dialog) {
         auto *resource=draft(dialog); require(input(resource)->text().isEmpty(),"Transmitter should start with no solve-fors");
         choose(resource,true,false,false); require(input(resource)->text().isEmpty(),"Picker Cancel changed parent draft");
         choose(resource,true,true,false); require(input(resource)->text()=="FrequencyBias","Picker OK did not retain pending solve-for");
         require(editor->toPlainText()==baseline && !object("Radio"),"Pending solve-for picker mutated configured mission");
      });
      require(editor->toPlainText()==baseline && !object("Radio") && editor->document()->isUndoAvailable()==undoBefore && editor->document()->isModified()==modifiedBefore,"Creator Cancel changed source, object, Undo or dirty state");
      creator([&](QDialog *dialog) {
         auto *resource=draft(dialog); auto *create=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok);
         input(resource)->setText("CartesianState"); create->click();
         auto *status=dialog->findChild<QLabel *>("resourceCreationStatus");
         require(dialog->result()!=QDialog::Accepted && status && status->text().contains("FrequencyBias") && editor->toPlainText()==baseline && !object("Radio") && editor->document()->isUndoAvailable()==undoBefore,"Invalid solve-for creation was not an atomic rejection");
         input(resource)->clear(); choose(resource,true,true,false); create->click();
         if (dialog->result()!=QDialog::Accepted && status) std::cerr<<"Create status: "<<status->text().toStdString()<<'\n';
         require(dialog->result()==QDialog::Accepted,"Configured Transmitter Create failed");
      });
      const auto created=editor->toPlainText();
      require(solveFors()==QStringList{"FrequencyBias"} && object("Radio")->GetStringParameter("PrimaryAntenna")=="Dish","Create lost transmitter settings");
      require(created.contains("Radio.SolveFors = {'FrequencyBias'};") && withoutRadio(created)==baseline,"Create changed unrelated exact source or omitted canonical solve-for assignment");
      closePanels(window); editor->undo(); require(editor->toPlainText()==baseline && window.buildScript() && !object("Radio"),"Creation must be one source Undo transaction");
      editor->redo(); require(editor->toPlainText()==created && window.buildScript() && solveFors()==QStringList{"FrequencyBias"},"Creation Redo did not reconstruct solve-fors");
      auto *resource=open(window,tree); require(input(resource)->text()=="FrequencyBias" && !resource->hasChanges(),"Reopened transmitter lost solve-for setting");
      choose(resource,false,false,true); require(input(resource)->text()=="FrequencyBias" && !resource->hasChanges() && editor->toPlainText()==created,"Selection Cancel changed configured or pending solve-fors");
      choose(resource,false,true,true); require(input(resource)->text().isEmpty() && resource->hasChanges() && solveFors()==QStringList{"FrequencyBias"} && editor->toPlainText()==created,"Selection OK committed before parent Apply");
      discardPanel(window,resource); require(editor->toPlainText()==created && solveFors()==QStringList{"FrequencyBias"},"Parent Close/discard changed applied solve-fors");
      resource=open(window,tree); choose(resource,false,true,true); resource->requestApply(); const auto cleared=editor->toPlainText();
      require(solveFors().isEmpty() && cleared.contains("Radio.SolveFors = {};") && withoutRadio(cleared)==baseline && !panel(window)->hasChanges(),"Apply did not clear list atomically while retaining source");
      closePanels(window); editor->undo(); require(editor->toPlainText()==created && window.buildScript() && solveFors()==QStringList{"FrequencyBias"},"Clearing list was not one Undo transaction");
      editor->redo(); require(editor->toPlainText()==cleared && window.buildScript() && solveFors().isEmpty(),"Clear Redo did not reconstruct empty list");
      resource=open(window,tree); require(input(resource)->text().isEmpty(),"Empty SolveFors did not reopen empty"); choose(resource,true,true,false); resource->requestApply();
      const auto restored=editor->toPlainText(); require(restored==created && solveFors()==QStringList{"FrequencyBias"},"Restoring list did not retain exact source/settings");
      resource=panel(window); input(resource)->setText("CartesianState"); resource->requestApply();
      require(editor->toPlainText()==restored && solveFors()==QStringList{"FrequencyBias"} && resource->hasChanges() && resource->findChild<QLabel *>("resourceStatus")->text().contains("FrequencyBias"),"Invalid Apply changed source/model or discarded pending correction");
      input(resource)->setText("FrequencyBias, FrequencyBias"); resource->requestApply();
      require(editor->toPlainText()==restored && solveFors()==QStringList{"FrequencyBias"} && resource->hasChanges() && resource->findChild<QLabel *>("resourceStatus")->text().contains("unique"),"Duplicate solve-for Apply was not an atomic rejection");
      discardPanel(window,resource); editor->undo();
      require(editor->toPlainText()==cleared && window.buildScript() && solveFors().isEmpty(),"Invalid Apply added an Undo entry or damaged prior clear transaction");
      editor->redo(); require(editor->toPlainText()==restored && window.buildScript() && solveFors()==QStringList{"FrequencyBias"},"Restore Redo failed after invalid Apply");
      const auto saved=files.filePath("transmitter α.script"); require(window.saveScriptTo(saved),"Unicode Save failed"); QFile file(saved); require(file.open(QIODevice::ReadOnly) && file.readAll()==restored.toUtf8(),"Save bytes differ from exact source"); file.close();
      require(window.loadScript(saved) && window.buildScript() && editor->toPlainText()==restored && solveFors()==QStringList{"FrequencyBias"},"Fresh saved script reopen changed source or solve-fors");
      resource=open(window,tree); choose(resource,false,false,true); require(input(resource)->text()=="FrequencyBias" && !resource->hasChanges(),"Saved resource reopen/selection Cancel lost solve-fors"); closePanels(window);
      std::cout<<"PASS actual Transmitter creator and visible SolveFors picker; pending/parent Cancel; FrequencyBias and empty-list Apply; invalid/duplicate rollback; exact unrelated UTF-8 source; one-transaction Undo/Redo; Save/reopen. Offscreen only, no numerical/estimation mission.\n"; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<'\n'; return 1; }
}
