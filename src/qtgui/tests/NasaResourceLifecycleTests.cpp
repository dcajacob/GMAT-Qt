#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QTabBar>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const QString &message) { if (!ok) throw std::runtime_error(message.toStdString()); }
static GmatBase *object(const QString &name) { return Moderator::Instance()->GetConfiguredObject(name.toStdString()); }

class Lifecycle {
public:
   MainWindow window;
   QJsonObject data;
   QPlainTextEdit *editor;
   QTreeWidget *tree;
   QString field,kind,type,key;
   int delay=0;
   Lifecycle(const QString &startup,const QJsonObject &data) : data(data) {
      key=data["key"].toString(); type=data["type"].toString(); field=data["field"].toString(); kind=data["kind"].toString();
      delay=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible" ? 100 : 0;
      window.show(); require(window.initialize(startup),"Engine initialization failed");
      editor=window.findChild<QPlainTextEdit *>("scriptEditor"); tree=window.findChild<QTreeWidget *>("Resources"); require(editor && tree,"Workspace missing");
   }
   void later(const std::function<void()> &action,std::exception_ptr &failure) {
      QTimer::singleShot(delay,&window,[&,action] { try { action(); } catch (...) { failure=std::current_exception(); if (auto *modal=qobject_cast<QDialog *>(QApplication::activeModalWidget())) modal->reject(); if (auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget())) menu->close(); } });
   }
   QTreeWidgetItem *item(const QString &name,bool category=false) {
      const auto found=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);
      for (auto *entry:found) if (category ? entry->data(0,Qt::UserRole+1).isValid() : !entry->data(0,Qt::UserRole).toString().isEmpty()) return entry;
      throw std::runtime_error(("Missing tree entry: "+name).toStdString());
   }
   QMdiSubWindow *child(const QString &name) {
      for (auto *entry:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (entry->property("resourceName").toString()==name) return entry;
      return nullptr;
   }
   ResourceEditor *panel(const QString &name) { auto *entry=child(name); auto *result=entry ? dynamic_cast<ResourceEditor *>(entry->widget()) : nullptr; require(result,"Resource panel missing: "+name); return result; }
   void collect() { QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
   void unrelated(const QString &baseline) {
      QString rest=editor->toPlainText();
      for (const auto &owner:QStringList{"NasaItem","NasaCopy","RenamedItem","RenamedCopy","NasaItem_ForceModel","RenamedItem_ForceModel"}) {
         const auto escaped=QRegularExpression::escape(owner);
         rest.remove(QRegularExpression("^[ \\t]*Create[ \\t]+[A-Za-z][A-Za-z0-9_]*[ \\t]+"+escaped+"[ \\t]*;[ \\t]*\\n",QRegularExpression::MultilineOption));
         rest.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?"+escaped+"(?:\\.[A-Za-z][A-Za-z0-9_.]*)?[ \\t]*=[^\\n]*\\n",QRegularExpression::MultilineOption));
      }
      require(rest==baseline,"Unrelated source bytes changed during the resource lifecycle");
   }
   void context(const QString &name,const QString &action,const std::function<void()> &after={},bool category=false) {
      auto *entry=item(name,category); tree->expandAll(); tree->scrollToItem(entry); tree->setCurrentItem(entry); QApplication::processEvents();
      std::exception_ptr failure;
      later([&] {
         auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); require(menu,"Context menu missing");
         QAction *chosen=nullptr; for (auto *candidate:menu->actions()) if (candidate->objectName()==action || candidate->text()==action) chosen=candidate;
         require(chosen && chosen->isEnabled(),"Context action unavailable: "+action);
         if (after) later(after,failure);
         menu->setActiveAction(chosen); QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter);
      },failure);
      tree->customContextMenuRequested(tree->visualItemRect(entry).center()); if (failure) std::rethrow_exception(failure); collect();
   }
   void show(QWidget *control) {
      for (auto *page=control;page && page!=&window;page=page->parentWidget()) {
         if (auto *tabs=qobject_cast<QTabWidget *>(page)) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==control || tabs->widget(i)->isAncestorOf(control)) tabs->setCurrentIndex(i);
         if (auto *scroll=qobject_cast<QScrollArea *>(page)) scroll->ensureWidgetVisible(control);
      }
   }
   void set(ResourceEditor *owner,const QString &name,const QString &value,const QString &valueKind={}) {
      if (name=="Value") if (auto *input=owner->findChild<QLineEdit *>("parameterValue")) { input->setText(value); return; }
      const auto explicitName=data["controls"].toObject()[name].toString();
      for (auto *control:owner->findChildren<QWidget *>()) if (control->objectName().endsWith("_"+name) || (!explicitName.isEmpty() && control->objectName()==explicitName)) {
         if (auto *input=qobject_cast<QLineEdit *>(control)) { show(input); input->setText(value); return; }
         if (auto *choice=qobject_cast<QComboBox *>(control)) { show(choice); require(choice->isEditable() || choice->findText(value)>=0,"Choice absent: "+value); choice->setCurrentText(value); return; }
         if (auto *check=qobject_cast<QCheckBox *>(control)) { show(check); check->setChecked(value=="true"); return; }
         if (auto *spin=qobject_cast<QSpinBox *>(control)) { show(spin); spin->setValue(value.toInt()); return; }
      }
      auto *table=owner->findChild<QTableWidget *>("resourceProperties"); require(table,"Advanced property table missing");
      for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) {
         if (valueKind=="list") {
            auto *choose=owner->findChild<QPushButton *>("chooseProperty_"+name); require(choose,"Resource list chooser missing"); std::exception_ptr failure;
            later([&] { auto *dialog=owner->findChild<QDialog *>("resourceSelectionDialog"); require(dialog,"Resource list dialog missing"); auto *list=dialog->findChild<QListWidget *>("resourceSelectionList"); require(list,"Resource list missing"); const auto wanted=splitResourceReferences(value); QSet<QString> selected;
               for (int i=0;i<list->count();++i) { auto *item=list->item(i); item->setCheckState(wanted.contains(item->text()) ? Qt::Checked : Qt::Unchecked); if (wanted.contains(item->text())) selected.insert(item->text()); }
               require(selected.size()==wanted.size(),"List did not offer every requested resource"); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            },failure); show(choose); choose->click(); if (failure) std::rethrow_exception(failure);
         } else if (auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,1))) { show(table); require(!table->isRowHidden(row),"Property has a primary control; do not edit its hidden backing row: "+name); require(choice->isEditable() || choice->findText(value)>=0,"Advanced choice absent"); choice->setCurrentText(value); }
         else { show(table); require(!table->isRowHidden(row) && table->item(row,1)->flags().testFlag(Qt::ItemIsEditable),"Property is hidden or not editable through this control: "+name); table->scrollToItem(table->item(row,1)); table->item(row,1)->setText(value); }
         return;
      }
      throw std::runtime_error(("Editable field missing: "+name).toStdString());
   }
   QString value(const QString &name) {
      auto *model=object(name); require(model,"Configured object missing: "+name);
      if (kind=="real") return QString::number(model->GetRealParameter(field.toStdString()),'g',17);
      if (kind=="integer") return QString::number(model->GetIntegerParameter(field.toStdString()));
      if (kind=="list") { QStringList entries; for (const auto &entry:model->GetStringArrayParameter(field.toStdString())) entries.append(QString::fromStdString(entry)); return entries.join(','); }
      return QString::fromStdString(model->GetStringParameter((type=="String" && field=="Value" ? QString("Expression") : field).toStdString()));
   }
   void check(const QString &name,const QString &expected) {
      const auto actual=value(name);
      if (kind=="real") { bool ok=false; const auto number=actual.toDouble(&ok); require(ok && std::isfinite(number) && number==expected.toDouble(),"Wrong numeric property: "+name+"."+field+" = "+actual+", expected "+expected); }
      else if (kind=="list") { auto a=splitResourceReferences(actual),b=splitResourceReferences(expected); a.sort(); b.sort(); require(a==b,"Wrong resource list"); }
      else require(actual==expected,"Wrong property: "+name+"."+field+" = "+actual+", expected "+expected);
   }
   void proof(const QString &operation,const QStringList &facets={"source-model-tree","pending-protection"}) {
      QJsonArray checks; for (const auto &facet:facets) checks.append(facet);
      QJsonObject record={{"checkpoint","resource."+key+"."+operation},{"family",key},{"operation",operation},{"field",field},{"facets",checks},{"outcome","passed"}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(record).toJson(QJsonDocument::Compact).constData()<<std::endl;
   }
   void open(const QString &name,bool doubleClick=false) {
      if (doubleClick) { tree->itemDoubleClicked(item(name),0); collect(); } else context(name,"openResource");
      auto *view=child(name); require(view && view->isVisible() && view->windowTitle()==name,"Open did not expose/name the correct panel"); require(view->property("sourceScript").toString()==editor->toPlainText(),"Panel belongs to a stale source snapshot"); panel(name);
   }
   void close(const QString &name,bool button=false) {
      QPointer<QMdiSubWindow> previous=child(name); require(previous,"Close target missing");
      if (button) panel(name)->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Close)->click(); else context(name,"closeResource");
      collect(); require(previous.isNull() && !child(name),"Resource Close did not close the panel");
   }
   void file(const QString &path,bool save) {
      QAction *action=save ? window.findChild<QAction *>("saveScriptAs") : nullptr;
      if (!save) for (auto *candidate:window.findChildren<QAction *>()) if (candidate->shortcut()==QKeySequence::Open) action=candidate;
      require(action && action->isEnabled(),"File menu action missing"); std::exception_ptr failure;
      QTimer ready; QElapsedTimer elapsed; elapsed.start(); bool initialized=false; int settled=0; QPointer<QFileDialog> activePicker;
      const qint64 timeout=10000LL+static_cast<qint64>(std::clamp(qEnvironmentVariableIntValue("GMAT_QT_ACCEPTANCE_PAUSE_MS"),0,5000))*std::clamp(qEnvironmentVariableIntValue("GMAT_QT_ACCEPTANCE_MAX_CHECKPOINTS"),0,200);
      QObject::connect(&ready,&QTimer::timeout,&ready,[&] {
         try {
            auto *picker=qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
            require(elapsed.elapsed()<timeout,"File chooser did not become ready within its execution and viewing budget");
            if (!picker) return;
            activePicker=picker;
            require(picker->acceptMode()==(save ? QFileDialog::AcceptSave : QFileDialog::AcceptOpen),"Wrong file chooser mode");
            const auto parent=QFileInfo(path).absolutePath(),filename=QFileInfo(path).fileName();
            if (!initialized) { initialized=true; if (picker->directory().absolutePath()!=parent) picker->setDirectory(parent); }
            auto *entry=picker->findChild<QLineEdit *>("fileNameEdit"); require(entry,"File chooser filename input missing");
            if (entry->text()!=filename) { entry->setText(filename); settled=0; return; }
            auto *buttons=picker->findChild<QDialogButtonBox *>(); require(buttons,"File chooser buttons missing");
            auto *submit=buttons->button(save ? QDialogButtonBox::Save : QDialogButtonBox::Open); require(submit,"File chooser submission button missing");
            if (picker->directory().absolutePath()==parent && submit->isEnabled() && ++settled>=3) { submit->click(); if (!picker->isVisible()) ready.stop(); else settled=0; }
         } catch (...) { failure=std::current_exception(); ready.stop(); if (auto *modal=qobject_cast<QDialog *>(QApplication::activeModalWidget())) modal->reject(); if (activePicker) activePicker->reject(); }
      });
      ready.start(delay ? 50 : 10);
      action->trigger(); if (failure) std::rethrow_exception(failure); collect();
   }
   void change(const QString &name,const QString &target) {
      open(name); const auto before=editor->toPlainText(); const auto old=value(name); auto *view=panel(name); set(view,field,target,kind);
      require(view->hasChanges() && editor->toPlainText()==before,"Property edit did not remain pending"); check(name,old); view->requestApply(); collect();
      const auto *status=panel(name)->findChild<QLabel *>("resourceStatus");
      require(editor->toPlainText()!=before && !panel(name)->hasChanges(),"Apply did not commit and refresh clean panel: "+(status ? status->text() : QString("no status label"))); check(name,target);
   }
   void panelButton(const QString &name,QDialogButtonBox::StandardButton which) {
      auto *buttons=panel(name)->findChild<QDialogButtonBox *>("resourceButtons");
      auto *button=buttons ? buttons->button(which) : nullptr;
      require(button && button->isEnabled() && button->isVisible(),"Resource OK/Cancel button absent or disabled");
      button->click(); collect();
   }
   void panelButtons(const QString &name,const QString &first,const QString &second,
                     const QString &applied,const QString &baseline) {
      open(name); QPointer<QMdiSubWindow> previous=child(name);
      const auto steps=editor->document()->availableUndoSteps();
      panelButton(name,QDialogButtonBox::Ok);
      require(!previous && !child(name) && editor->toPlainText()==applied && editor->document()->availableUndoSteps()==steps,
         "Clean resource OK changed source/history or did not close"); check(name,first);

      open(name); previous=child(name); set(panel(name),field,second,kind);
      require(panel(name)->hasChanges() && editor->toPlainText()==applied,"Resource Cancel fixture did not remain pending"); check(name,first);
      panelButton(name,QDialogButtonBox::Cancel);
      require(!previous && !child(name) && editor->toPlainText()==applied && editor->document()->availableUndoSteps()==steps,
         "Resource Cancel saved pending data, lost prior Apply or changed history"); check(name,first);
      proof("cancel",{"source-model-tree","known-property","discard-pending","retain-prior-apply","no-extra-undo","closed-owner"});

      open(name); previous=child(name); set(panel(name),field,second,kind);
      require(panel(name)->hasChanges() && editor->toPlainText()==applied,"Resource OK fixture committed prematurely"); check(name,first);
      panelButton(name,QDialogButtonBox::Ok);
      require(!previous && !child(name) && editor->toPlainText()!=applied,"Resource OK failed to commit then close"); check(name,second); unrelated(baseline);
      const auto changed=editor->toPlainText();
      editor->undo(); require(editor->toPlainText()==applied && window.buildScript(),"Resource OK Undo did not restore exact accepted source"); check(name,first);
      editor->redo(); require(editor->toPlainText()==changed && window.buildScript(),"Resource OK Redo did not restore the committed source"); check(name,second); unrelated(baseline);
      editor->undo(); require(editor->toPlainText()==applied && window.buildScript(),"Resource OK test did not restore the applied fixture"); check(name,first);
      proof("ok",{"source-model-tree","known-property","valid-commit-close","clean-noop","unchanged-unrelated-source","undo-redo","closed-owner"});
   }
};

int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("NasaResourceLifecycle");
   try {
      require(argc==4,"Supply startup, family data and family key"); TestSettings settings; QTemporaryDir files; require(files.isValid(),"Cannot create temporary fixtures");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QFile input(QString::fromLocal8Bit(argv[2])); require(input.open(QIODevice::ReadOnly),"Cannot read resource-family data");
      QJsonObject data; for (const auto &candidate:QJsonDocument::fromJson(input.readAll()).object()["cases"].toArray()) if (candidate.toObject()["key"].toString()==QString::fromLocal8Bit(argv[3])) data=candidate.toObject(); require(!data.isEmpty(),"Unknown resource family");
      QDir::setCurrent(QFileInfo(startup).absolutePath()); Lifecycle test(startup,data); auto &window=test.window; auto *editor=test.editor;
      const QString baseline="% NASA retained independent configuration α\nCreate Spacecraft Anchor Other;\nCreate Variable Keep;\nKeep = 7; % retained value\n"+data["fixture"].toString()+"BeginMissionSequence;\nKeep = Keep + 2; % retained mission\n";
      editor->setPlainText(baseline); require(window.buildScript(),"Lifecycle fixture failed to build"); const QString name="NasaItem",copy="NasaCopy"; const auto first=data["first"].toString(),second=data["second"].toString();
      auto create=[&](bool accept) { test.context(data["category"].toString(),"addResource_"+test.type,[&] {
         auto *dialog=window.findChild<QDialog *>("newResourceDialog"); require(dialog,"Category creator missing"); auto *types=dialog->findChild<QComboBox *>("resourceType"); require(types && types->count()==1 && types->currentText()==test.type,"Category creator has the wrong type"); auto *entry=dialog->findChild<QLineEdit *>("resourceName"); require(entry,"Resource name input missing"); entry->setText(name);
         auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons"); require(buttons,"Create buttons missing");
         if (!accept) { buttons->button(QDialogButtonBox::Cancel)->click(); return; }
         for (const auto &invalid:QStringList{"not valid","Anchor","Propagate"}) { entry->setText(invalid); buttons->button(QDialogButtonBox::Ok)->click(); require(dialog->isVisible() && editor->toPlainText()==baseline && !object(name),"Invalid or duplicate creation changed source/model"); }
         entry->setText(name); ResourceEditor *panel=nullptr;
         for (auto *candidate:dialog->findChildren<QWidget *>()) if (auto *form=dynamic_cast<ResourceEditor *>(candidate);form && form->isVisible()) panel=form;
         require(panel,"Creation form missing"); const auto defaults=data["create_fields"].toObject(); for (auto it=defaults.begin();it!=defaults.end();++it) test.set(panel,it.key(),it.value().toString());
         buttons->button(QDialogButtonBox::Ok)->click(); require(dialog->result()==QDialog::Accepted,"Resource creation rejected: "+dialog->findChild<QLabel *>("resourceCreationStatus")->text());
      },true); };
      create(false); require(editor->toPlainText()==baseline && !object(name),"Create Cancel changed source/model"); create(true);
      require(object(name) && object(name)->IsOfType(data["object_type"].toString(test.type).toStdString()),"Add failed typed model registration");
      test.unrelated(baseline); test.item(name); require(test.child(name),"New resource editor did not open"); test.proof("add");
      const auto added=editor->toPlainText(),initial=test.value(name);
      if (data["clone_allowed"].toBool()) {
         test.context(name,"cloneResource",[&] { auto *dialog=window.findChild<QDialog *>("cloneResourceDialog"); require(dialog,"Default clone dialog missing"); dialog->findChild<QLineEdit *>("resourceName")->setText(copy); dialog->findChild<QDialogButtonBox *>("resourceCloneButtons")->button(QDialogButtonBox::Ok)->click(); require(dialog->result()==QDialog::Accepted,"Unmodified default clone rejected"); });
         test.check(copy,initial); test.check(name,initial); test.proof("clone-default");
         auto plainRename=[&](const QString &old,const QString &next) { test.context(old,"renameResource",[&] { auto *dialog=window.findChild<QDialog *>("renameResourceDialog"); require(dialog,"Default clone rename dialog missing"); dialog->findChild<QLineEdit *>("resourceName")->setText(next); dialog->findChild<QDialogButtonBox *>("resourceRenameButtons")->button(QDialogButtonBox::Ok)->click(); require(dialog->result()==QDialog::Accepted,"Default resource rename rejected"); }); require(!object(old) && object(next),"Default rename did not synchronize model"); test.item(next); test.check(next,initial); };
         plainRename(copy,"DefaultCopy"); test.check(name,initial); test.proof("clone-default-rename-clone");
         plainRename(name,"DefaultOriginal"); test.check("DefaultCopy",initial); plainRename("DefaultOriginal",name); test.proof("clone-default-rename-original");
         test.context("DefaultCopy","Delete resource…",[&] { auto *prompt=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); require(prompt,"Default clone deletion prompt missing"); prompt->button(QMessageBox::Yes)->click(); });
         require(!object("DefaultCopy") && editor->toPlainText()==added,"Default clone/rename/delete did not restore exact original source"); test.check(name,initial);
      }
      test.close(name); require(editor->toPlainText()==added,"Close changed source"); test.proof("close-context");
      test.open(name,true); test.check(name,initial); test.proof("open-double-click"); test.close(name,true); test.proof("close-button"); test.open(name); test.check(name,initial); test.proof("open-context");
      auto *panel=test.panel(name); test.set(panel,test.field,first,test.kind); require(panel->hasChanges() && editor->toPlainText()==added,"Pending edit changed source"); test.check(name,initial);
      for (const auto answer:{QMessageBox::Cancel,QMessageBox::Discard}) {
         QPointer<QMdiSubWindow> previous=test.child(name); std::exception_ptr failure;
         test.context(name,"closeResource",[&] { auto *prompt=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); require(prompt && prompt->button(answer),"Unapplied-changes prompt missing"); prompt->button(answer)->click(); });
         require(editor->toPlainText()==added,"Unapplied Close changed source"); test.check(name,initial);
         if (answer==QMessageBox::Cancel) { require(previous && test.panel(name)->hasChanges(),"Cancel discarded pending changes"); test.proof("exit-no"); }
         else { test.collect(); require(previous.isNull(),"Discard did not close pending panel"); test.open(name); require(!test.panel(name)->hasChanges(),"Discard reopened dirty"); test.proof("exit-yes"); }
      }
      test.change(name,first); const auto applied=editor->toPlainText(); test.unrelated(baseline); test.proof("apply");
      const auto undoSteps=editor->document()->availableUndoSteps(); test.panel(name)->requestApply(); require(editor->toPlainText()==applied && editor->document()->availableUndoSteps()==undoSteps,"Repeated Apply changed source/Undo"); test.proof("apply-twice"); test.close(name);
      editor->undo(); require(editor->toPlainText()==added && window.buildScript(),"Apply Undo not exact"); test.check(name,initial); editor->redo(); require(editor->toPlainText()==applied && window.buildScript(),"Apply Redo not exact"); test.check(name,first);
      const auto saved=files.filePath("resource α.script"); test.file(saved,true); QFile savedFile(saved); require(savedFile.open(QIODevice::ReadOnly) && savedFile.readAll()==applied.toUtf8(),"GUI Save As changed exact source bytes");
      test.open(name); test.file(saved,false); require(editor->toPlainText()==applied,"GUI Load with panel open changed source"); test.open(name); test.check(name,first); test.close(name); test.proof("load-panel-open",{"source-model-tree","pending-protection","qt-file-dialog","unicode-save-reopen"});
      test.file(saved,false); require(editor->toPlainText()==applied,"Clean GUI Load changed exact source"); test.check(name,first); test.proof("load",{"source-model-tree","pending-protection","qt-file-dialog","unicode-save-reopen"});
      if (test.key=="XYPlot") {
         require(!window.applyResourceChanges(name,{{"ShowPlot","true"}},applied).isEmpty() && editor->toPlainText()==applied,"Enabling an incomplete XY plot was accepted or changed source");
         require(window.applyResourceChanges(name,{{"XVariable","Anchor.A1ModJulian"},{"YVariables","Anchor.X"}},applied).isEmpty(),"Cannot configure valid XY references");
         const auto referenced=editor->toPlainText();
         require(!window.applyResourceChanges(name,{{"XVariable",""}},referenced).isEmpty() && editor->toPlainText()==referenced && object(name)->GetStringParameter("XVariable")=="Anchor.A1ModJulian","Clearing an existing XY reference bypassed validation");
         editor->undo(); require(editor->toPlainText()==applied && window.buildScript(),"XY validation fixture Undo changed the accepted display configuration"); test.check(name,first);
         test.proof("disabled-xy-reference-validation");
      }
      test.proof("create-configure",{"category-menu","typed-create","configured-property","source-model-tree","pending-apply","undo-redo","unicode-save-reopen"});
      test.panelButtons(name,first,second,applied,baseline);
      const auto clonedValue=data["clone_fields"].toObject().value(test.field).toString(first);
      const auto editedClone=data["clone_edit"].toString(second);
      if (data["clone_allowed"].toBool()) {
         auto clone=[&](bool accept) { test.context(name,"cloneResource",[&] { auto *dialog=window.findChild<QDialog *>("cloneResourceDialog"); require(dialog,"Clone dialog missing"); dialog->findChild<QLineEdit *>("resourceName")->setText(copy); auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceCloneButtons"); if (!accept) buttons->button(QDialogButtonBox::Cancel)->click(); else {
            if (data["clone_default_rejected"].toBool()) {
               buttons->button(QDialogButtonBox::Ok)->click(); QApplication::processEvents();
               require(dialog->isVisible() && !object(copy) && editor->toPlainText()==applied,"Invalid shared-members Formation clone was not rejected atomically"); test.check(name,first);
               require(dialog->findChild<QLabel *>("resourceCloneStatus")->text().contains("disjoint members"),"Formation clone did not explain the correctable membership constraint");
               std::cout<<"NASA_LIMIT Formation exact populated clone conflicts with the interpreter's disjoint-members rule; the tested copy must choose different spacecraft.\n";
            }
            const auto edits=data["clone_fields"].toObject();
            if (!edits.isEmpty()) { ResourceEditor *form=nullptr; for (auto *candidate:dialog->findChildren<QWidget *>()) if (auto *view=dynamic_cast<ResourceEditor *>(candidate);view && view->isVisible()) form=view; require(form,"Clone configuration form missing"); for (auto it=edits.begin();it!=edits.end();++it) test.set(form,it.key(),it.value().toString(),test.kind); }
            buttons->button(QDialogButtonBox::Ok)->click(); QApplication::processEvents(); require(dialog->result()==QDialog::Accepted,"Clone did not commit: "+dialog->findChild<QLabel *>("resourceCloneStatus")->text()+"\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText());
         } }); };
         clone(false); require(editor->toPlainText()==applied && !object(copy),"Clone Cancel changed model/source"); clone(true); require(object(copy),"Clone object missing"); test.check(copy,clonedValue); test.check(name,first); test.item(copy); test.proof(data["clone_default_rejected"].toBool() ? "clone-reconfigured" : "clone-modified");
         test.change(copy,editedClone); test.check(name,first); test.proof("change-clone-independent"); test.change(name,second); test.change(name,first); test.check(copy,editedClone); test.proof("change-original-independent");
      } else {
         auto *entry=test.item(name); test.tree->scrollToItem(entry); std::exception_ptr failure;
         test.later([&] { auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); require(menu,"Protected clone menu missing"); QAction *action=nullptr; for (auto *candidate:menu->actions()) if (candidate->objectName()=="cloneResource") action=candidate; require(action && !action->isEnabled(),"Propagator clone was enabled"); menu->close(); },failure);
         test.tree->customContextMenuRequested(test.tree->visualItemRect(entry).center()); if (failure) std::rethrow_exception(failure); test.proof("clone-disabled");
      }
      auto rename=[&](const QString &old,const QString &next,bool accept) { const auto before=editor->toPlainText(); test.context(old,"renameResource",[&] { auto *dialog=window.findChild<QDialog *>("renameResourceDialog"); require(dialog,"Rename dialog missing"); auto *input=dialog->findChild<QLineEdit *>("resourceName"); auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceRenameButtons");
         if (!accept) { input->setText(next); buttons->button(QDialogButtonBox::Cancel)->click(); return; }
         for (const auto &invalid:QStringList{"bad name","Anchor","Propagate"}) { input->setText(invalid); buttons->button(QDialogButtonBox::Ok)->click(); require(dialog->isVisible() && editor->toPlainText()==before && object(old),"Invalid/duplicate rename changed source/model"); }
         input->setText(next); buttons->button(QDialogButtonBox::Ok)->click(); require(dialog->result()==QDialog::Accepted,"Rename did not commit");
      }); if (!accept) require(editor->toPlainText()==before && object(old) && !object(next),"Rename Cancel changed source/model"); else { require(!object(old) && object(next),"Rename left stale or absent object"); test.item(next); } };
      rename(name,"Cancelled",false); test.proof("rename-cancel"); rename(name,"RenamedItem",true); test.check("RenamedItem",first); test.proof("rename"); test.proof("rename-error"); test.proof("rename-used-name");
      if (data["clone_allowed"].toBool()) { rename(copy,"RenamedCopy",true); test.check("RenamedCopy",editedClone); test.check("RenamedItem",first); test.proof("rename-clone"); }
      auto remove=[&](const QString &name) { const auto before=editor->toPlainText(); test.context(name,"Delete resource…",[&] { auto *prompt=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); require(prompt,"Delete prompt missing"); prompt->button(QMessageBox::Cancel)->click(); }); require(editor->toPlainText()==before && object(name),"Delete Cancel changed source/model");
         test.context(name,"Delete resource…",[&] { auto *prompt=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); require(prompt,"Delete confirmation missing"); prompt->button(QMessageBox::Yes)->click(); }); require(!object(name) && test.tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive).isEmpty() && !test.child(name),"Delete left source/model/tree/panel state");
      };
      if (data["clone_allowed"].toBool()) remove("RenamedCopy"); remove("RenamedItem"); test.unrelated(baseline); test.proof("delete");
      std::cout<<"PASS: "<<test.key.toStdString()<<" resource tree category creation/configuration, both open paths, both close paths, pending Cancel/Discard, Apply/Undo/reload, typed clone independence, rename validation and confirmed unused deletion\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
