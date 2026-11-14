#include "AcceptancePlatform.hpp"
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
#include <QSet>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static GmatBase *object(const QString &name) { return Moderator::Instance()->GetConfiguredObject(name.toStdString()); }
static QStringList models()
{
   auto *receiver=object("Rx"); require(receiver,"Configured receiver missing"); QStringList values;
   for (const auto &value:receiver->GetStringArrayParameter("ErrorModels")) values.append(QString::fromStdString(value));
   return values;
}
static ResourceEditor *draft(QDialog *dialog)
{
   for (auto *widget:dialog->findChildren<QWidget *>())
      if (auto *panel=dynamic_cast<ResourceEditor *>(widget);panel && panel->isVisible()) return panel;
   throw std::runtime_error("Visible receiver creation draft missing");
}
static ResourceEditor *panel(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (child->property("resourceName")=="Rx") {
         auto *found=dynamic_cast<ResourceEditor *>(child->widget()); require(found,"Receiver editor missing"); return found;
      }
   throw std::runtime_error("Receiver workspace panel missing");
}
static QLineEdit *input(ResourceEditor *panel)
{
   auto *value=panel->findChild<QLineEdit *>("resource_ErrorModels");
   require(value && value->isVisible() && !value->isReadOnly(),"Writable receiver ErrorModels GUI control is omitted"); return value;
}
static void choose(ResourceEditor *panel,const QStringList &selected,bool accept,const QStringList &expectedSelected)
{
   auto *value=input(panel); QPushButton *picker=nullptr;
   for (auto *button:value->parentWidget()->findChildren<QPushButton *>(QString(),Qt::FindDirectChildrenOnly))
      if (button->isVisible() && button->isEnabled()) { require(!picker,"Several ErrorModels pickers found"); picker=button; }
   require(picker,"Visible receiver ErrorModels list picker missing"); std::exception_ptr failure;
   QTimer::singleShot(0,panel,[&] {
      auto *dialog=panel->findChild<QDialog *>("resourceSelectionDialog");
      try {
         require(dialog && dialog->isVisible(),"Actual ErrorModels selection dialog missing"); auto *list=dialog->findChild<QListWidget *>("resourceSelectionList"); require(list,"ErrorModels selection list missing");
         QSet<QString> available; QStringList checked;
         for (int i=0;i<list->count();++i) { available.insert(list->item(i)->text()); if (list->item(i)->checkState()==Qt::Checked) checked.append(list->item(i)->text()); }
         require(available==QSet<QString>{"PositionError","RangeError","AlternatePositionError"},"Picker must offer existing ErrorModel resources, excluding antenna/variable resources");
         require(checked==expectedSelected,"Reopened picker lost pending/applied model order");
         auto *clear=dialog->findChild<QPushButton *>("resourceClearSelection"); require(clear,"Clear selection action missing"); clear->click();
         // Reorder the actual pending QListWidget, as its InternalMove view
         // does when the user drags rows; no engine object is changed here.
         for (int destination=0;destination<selected.size();++destination) {
            int source=-1; for (int i=0;i<list->count();++i) if (list->item(i)->text()==selected[destination]) source=i;
            require(source>=0,"Requested model absent from picker"); auto *item=list->takeItem(source); list->insertItem(destination,item); item->setCheckState(Qt::Checked);
         }
         auto *buttons=dialog->findChild<QDialogButtonBox *>(); require(buttons,"Selection actions missing"); buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
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
   QMdiSubWindow *child=nullptr; for (auto *candidate:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (candidate->widget()==resource) child=candidate;
   require(child,"Pending receiver panel missing");
   QTimer::singleShot(0,&window,[] { if (auto *question=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) question->button(QMessageBox::Discard)->click(); });
   require(child->close(),"Discard did not close pending receiver editor"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static ResourceEditor *open(MainWindow &window,QTreeWidget *tree)
{
   QTreeWidgetItem *selected=nullptr;
   for (auto *candidate:tree->findItems("Rx",Qt::MatchExactly|Qt::MatchRecursive)) if (!candidate->data(0,Qt::UserRole).toString().isEmpty()) selected=candidate;
   require(selected,"Receiver resource tree entry missing"); tree->setCurrentItem(selected); tree->itemDoubleClicked(selected,0); QApplication::processEvents(); return panel(window);
}
static QString withoutRx(QString source)
{
   source.remove(QRegularExpression("^[ \\t]*Create[ \\t]+Receiver[ \\t]+Rx;[^\\S\\n]*\\n",QRegularExpression::MultilineOption));
   source.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?Rx\\.[^;\\n]+;[^\\S\\n]*\\n",QRegularExpression::MultilineOption)); return source;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtReceiverErrorModels");
   try {
      require(argc==2 && acceptanceWidgetPlatform(),"Receiver ErrorModels test requires offscreen and one startup argument");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Temporary script directory unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Selected-plugin runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources"); require(editor && tree,"Resource workspace missing");
      // Antenna Delay/PhaseCenterLocation are inactive; use a valid bare
      // antenna, and two distinct supported measurement types for model order.
      const QString baseline="% retain exact unrelated source α\nCreate Antenna Dish;\nCreate ErrorModel PositionError RangeError AlternatePositionError;\nAlternatePositionError.Type = 'GPS_PosVec';\nPositionError.Type = 'GPS_PosVec';\nPositionError.Bias = 2.5000; % retained position bias\nRangeError.Type = 'Range_Skin';\nRangeError.Bias = 1.2500; % retained range bias\nCreate Variable Keep;\nKeep = 7; % retained value\nBeginMissionSequence;\nKeep = Keep + 2; % retained mission expression\n";
      editor->setPlainText(baseline);
      if (!window.buildScript()) { auto *messages=window.findChild<QPlainTextEdit *>("messageWindow"); throw std::runtime_error(("Receiver reference fixture did not build\n"+(messages ? messages->toPlainText() : QString())).toStdString()); }
      const bool undoBefore=editor->document()->isUndoAvailable(),modifiedBefore=editor->document()->isModified();
      const QStringList initial={"RangeError","PositionError"},reordered={"PositionError","RangeError"};
      const auto creator=[&](const std::function<void(QDialog *)> &inspect) {
         std::exception_ptr failure;
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try {
               require(dialog,"Actual resource creation dialog missing"); auto *types=dialog->findChild<QComboBox *>("resourceType"); auto *name=dialog->findChild<QLineEdit *>("resourceName"); require(types && name,"Creator type/name controls missing");
               require(types->findText("Receiver")>=0,"Selected runtime does not expose Receiver creation"); types->setCurrentText("Receiver"); name->setText("Rx");
               auto *antenna=draft(dialog)->findChild<QLineEdit *>("resource_PrimaryAntenna"); require(antenna,"Primary antenna draft control missing"); antenna->setText("Dish"); inspect(dialog);
            } catch (...) { failure=std::current_exception(); }
            if (dialog && dialog->result()!=QDialog::Accepted) dialog->reject();
         });
         auto *action=window.findChild<QAction *>("createResource"); require(action && action->isEnabled(),"Resource Create action unavailable"); action->trigger(); if (failure) std::rethrow_exception(failure);
      };
      // Meaningful unchanged-frontend baseline: the first actual creator
      // assertion fails at the omitted writable OBJECTARRAY field.
      creator([&](QDialog *dialog) {
         auto *resource=draft(dialog); require(input(resource)->text().isEmpty(),"Receiver should start with no error models");
         choose(resource,initial,false,{}); require(input(resource)->text().isEmpty(),"Picker Cancel changed parent draft");
         choose(resource,initial,true,{}); require(input(resource)->text()==initial.join(", "),"Picker OK did not retain pending model order");
         require(editor->toPlainText()==baseline && !object("Rx"),"Pending model selection mutated configured mission");
      });
      require(editor->toPlainText()==baseline && !object("Rx") && editor->document()->isUndoAvailable()==undoBefore && editor->document()->isModified()==modifiedBefore,"Creator Cancel changed source/object/Undo/dirty state");
      creator([&](QDialog *dialog) {
         auto *resource=draft(dialog); auto *create=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok); auto *status=dialog->findChild<QLabel *>("resourceCreationStatus");
         input(resource)->setText("Dish"); create->click();
         require(dialog->result()!=QDialog::Accepted && status && status->text().contains("ErrorModel") && editor->toPlainText()==baseline && !object("Rx") && editor->document()->isUndoAvailable()==undoBefore,"Wrong-type model creation was not an atomic rejection");
         input(resource)->clear(); choose(resource,initial,true,{}); create->click();
         if (dialog->result()!=QDialog::Accepted && status) std::cerr<<"Create status: "<<status->text().toStdString()<<'\n'; require(dialog->result()==QDialog::Accepted,"Configured Receiver Create failed");
      });
      const auto created=editor->toPlainText(); require(models()==initial && object("Rx")->GetStringParameter("PrimaryAntenna")=="Dish","Create lost receiver settings/order");
      require(created.contains("Rx.ErrorModels = {RangeError, PositionError};") && withoutRx(created)==baseline,"Create changed unrelated exact source or omitted canonical model assignment");
      closePanels(window); editor->undo(); require(editor->toPlainText()==baseline && window.buildScript() && !object("Rx"),"Creation must be one source Undo transaction");
      editor->redo(); require(editor->toPlainText()==created && window.buildScript() && models()==initial,"Creation Redo did not reconstruct models");
      auto *resource=open(window,tree); require(input(resource)->text()==initial.join(", ") && !resource->hasChanges(),"Reopened receiver lost model settings");
      choose(resource,{},false,initial); require(input(resource)->text()==initial.join(", ") && !resource->hasChanges() && editor->toPlainText()==created,"Selection Cancel changed configured/pending models");
      choose(resource,reordered,true,initial); require(input(resource)->text()==reordered.join(", ") && resource->hasChanges() && models()==initial && editor->toPlainText()==created,"Ordered selection committed before parent Apply");
      discardPanel(window,resource); require(editor->toPlainText()==created && models()==initial,"Parent Close/discard changed applied models");
      resource=open(window,tree); choose(resource,reordered,true,initial); resource->requestApply(); const auto ordered=editor->toPlainText();
      require(models()==reordered && ordered.contains("Rx.ErrorModels = {PositionError, RangeError};") && withoutRx(ordered)==baseline && !panel(window)->hasChanges(),"Apply did not commit model order while retaining source");
      closePanels(window); editor->undo(); require(editor->toPlainText()==created && window.buildScript() && models()==initial,"Model reorder was not one Undo transaction");
      editor->redo(); require(editor->toPlainText()==ordered && window.buildScript() && models()==reordered,"Reorder Redo did not reconstruct names/order");
      resource=open(window,tree); choose(resource,{},true,reordered); require(models()==reordered && editor->toPlainText()==ordered,"Clear selection committed before parent Apply"); resource->requestApply(); const auto cleared=editor->toPlainText();
      // The existing source patcher leaves an empty insertion row when the
      // last explicit field is removed. Verify that exact scoped edit, while
      // preserving all unrelated bytes, rather than deleting its whitespace.
      auto expectedCleared=ordered; expectedCleared.replace("Rx.ErrorModels = {PositionError, RangeError};","");
      require(models().isEmpty() && cleared==expectedCleared && !panel(window)->hasChanges(),"Empty list did not reconstruct empty with exact source omission");
      closePanels(window); editor->undo(); require(editor->toPlainText()==ordered && window.buildScript() && models()==reordered,"Clearing models was not one Undo transaction");
      editor->redo(); require(editor->toPlainText()==cleared && window.buildScript() && models().isEmpty(),"Clear Redo did not reconstruct empty list");
      resource=open(window,tree); require(input(resource)->text().isEmpty(),"Empty ErrorModels did not reopen empty"); choose(resource,reordered,true,{}); resource->requestApply();
      const auto restored=editor->toPlainText(); auto expectedRestored=cleared;
      expectedRestored.insert(expectedRestored.indexOf("BeginMissionSequence;"),"Rx.ErrorModels = {PositionError, RangeError};\n");
      require(restored==expectedRestored && models()==reordered,"Restore did not retain exact source/settings");
      resource=panel(window);
      for (const auto &bad:QStringList{"Dish","MissingModel","PositionError, PositionError","PositionError, AlternatePositionError"}) {
         input(resource)->setText(bad); resource->requestApply(); const auto status=resource->findChild<QLabel *>("resourceStatus");
         require(editor->toPlainText()==restored && models()==reordered && resource->hasChanges() && status && !status->text().isEmpty(),"Wrong-type/missing/duplicate Apply changed source/model or discarded pending correction");
         if (bad.contains("AlternatePositionError")) require(status->text().contains("one receiver error model per measurement type"),"Same-measurement models were not rejected before silent engine omission");
      }
      discardPanel(window,resource); editor->undo(); require(editor->toPlainText()==cleared && window.buildScript() && models().isEmpty(),"Invalid Apply added an Undo entry or damaged prior transaction");
      editor->redo(); require(editor->toPlainText()==restored && window.buildScript() && models()==reordered,"Restore Redo failed after invalid Apply");
      const auto saved=files.filePath("receiver α.script"); require(window.saveScriptTo(saved),"Unicode Save failed"); QFile file(saved); require(file.open(QIODevice::ReadOnly) && file.readAll()==restored.toUtf8(),"Save bytes differ from exact source"); file.close();
      require(window.loadScript(saved) && window.buildScript() && editor->toPlainText()==restored && models()==reordered,"Fresh saved script reopen changed source or model order");
      require(object("PositionError")->GetStringParameter("Type")=="GPS_PosVec" && object("PositionError")->GetRealParameter("Bias")==2.5 && object("RangeError")->GetStringParameter("Type")=="Range_Skin" && object("RangeError")->GetRealParameter("Bias")==1.25,"Apply/reconstruction changed selected ErrorModel settings");
      resource=open(window,tree); choose(resource,{},false,reordered); require(input(resource)->text()==reordered.join(", ") && !resource->hasChanges(),"Saved resource reopen/selection Cancel lost models"); closePanels(window);
      // Existing source Rename must update the newly editable typed reference
      // list too; Undo restores its exact pre-Rename spelling and order.
      const auto renameError=window.renameResource("PositionError","RenamedPosition",restored); require(renameError.isEmpty(),qPrintable(renameError));
      require(!object("PositionError") && object("RenamedPosition") && models()==QStringList{"RenamedPosition","RangeError"} && editor->toPlainText().contains("Rx.ErrorModels = {RenamedPosition, RangeError};"),"ErrorModel Rename did not update receiver list references");
      resource=open(window,tree); require(input(resource)->text()=="RenamedPosition, RangeError" && !resource->hasChanges(),"Renamed receiver model reference did not reopen"); closePanels(window);
      editor->undo(); require(editor->toPlainText()==restored && window.buildScript() && object("PositionError") && !object("RenamedPosition") && models()==reordered,"ErrorModel Rename Undo did not restore exact source/references/order");
      std::cout<<"PASS actual Receiver creator and visible ErrorModels picker; existing typed references and order; pending/parent Cancel; reorder/empty-by-omission/restore Apply; wrong-type/missing/duplicate rollback; exact unrelated UTF-8 source and model settings; one-transaction Undo/Redo; Save/reopen; ErrorModel Rename/Undo reference readback. Offscreen configuration only; no sandbox/GPS measurement or numerical mission qualification.\n"; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<'\n'; return 1; }
}
