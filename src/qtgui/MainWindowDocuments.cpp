#include "MainWindow.hpp"
#include "ScriptDocument.hpp"
#include "ScriptEditor.hpp"
#include "EditablePanel.hpp"
#include "QtPlotReceiver.hpp"
#include "WelcomeDialog.hpp"
#include <QDir>
#include <QApplication>
#include <QMdiArea>
#include <QTextDocument>
#include <QFontDatabase>
#include <QFileInfo>
#include <QFileDialog>
#include <QSaveFile>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTreeWidget>

std::shared_ptr<ScriptDocument> MainWindow::createScriptDocument(const QString &path,const QString &text)
{
   auto document=std::make_shared<ScriptDocument>();
   auto *source=new ScriptEditor; source->setObjectName("inactiveScriptEditor");
   source->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)); source->setLineWrapMode(QPlainTextEdit::NoWrap);
   if (ready) source->setKeywords(availableEngineTypes());
   source->setPlainText(text); source->document()->setModified(false);
   auto *window=new ScriptSubWindow; window->setWidget(source); workspace->addSubWindow(window);
   window->setProperty("scriptDocument",true); window->setProperty("helpTopic","UsingGmat"); window->resize(850,540);
   document->editor=source; document->window=window; document->path=path; document->savedText=text;
   document->search=new FindReplaceDialog(source,source);
   scriptDocuments.append(document);
   window->mayClose=[this,document] { return closeScriptDocument(document); };
   connect(source->document(),&QTextDocument::modificationChanged,this,[this] { if (activeDocument) updateTitle(); });
   return document;
}
std::shared_ptr<ScriptDocument> MainWindow::selectedScriptDocument() const
{
   auto *focused=QApplication::focusWidget();
   for (const auto &document:scriptDocuments) if (document->editor && focused &&
         (focused==document->editor || document->editor->isAncestorOf(focused))) return document;
   for (const auto &document:scriptDocuments) if (document->window==workspace->activeSubWindow()) return document;
   return activeDocument;
}
void MainWindow::updateScriptDocuments()
{
   for (const auto &document:scriptDocuments) {
      if (!document->editor || !document->window) continue;
      QString label=document->path.isEmpty() ? QString("Untitled %1").arg(scriptDocuments.indexOf(document)+1) : QFileInfo(document->path).fileName();
      if (!document->path.isEmpty()) for (const auto &other:scriptDocuments)
         if (other!=document && QFileInfo(other->path).fileName()==label) { label=document->path; break; }
      const bool active=document==activeDocument;
      document->editor->setObjectName(active ? "scriptEditor" : "inactiveScriptEditor");
      document->window->setProperty("activeScript",active); document->window->setProperty("scriptPath",document->path);
      document->window->setWindowTitle(label+(document->editor->document()->isModified() ? " *" : "")+(active ? " — Active mission" : " — Inactive script"));
      document->editor->setToolTip(document->path+"\n"+(active ? "Resources, Mission and Output belong to this script." : "Editing and saving this script does not change the active mission. Use Make selected script active or Save and build script."));
   }
}
bool MainWindow::openScriptDocument(const QString &path)
{
   if (running || path.isEmpty()) return false;
   const auto identity=scriptDocumentIdentity(path);
   for (const auto &document:scriptDocuments) if (!document->path.isEmpty() && scriptDocumentIdentity(document->path)==identity) {
      document->window->showNormal(); workspace->setActiveSubWindow(document->window); document->editor->setFocus(); return true;
   }
   QString text; if (!readScriptDocumentFile(this,path,text)) return false;
   auto document=createScriptDocument(QFileInfo(path).absoluteFilePath(),text); document->window->show();
   workspace->setActiveSubWindow(document->window); document->editor->setFocus();
   updateScriptDocuments(); rememberMissionFile(document->path); refreshRecentMenu();
   statusBar()->showMessage("Script opened for editing; the active mission is unchanged"); return true;
}
bool MainWindow::saveScriptDocument(const std::shared_ptr<ScriptDocument> &document,bool saveAs)
{
   if (running || !document || !document->editor) return false;
   QString path=document->path;
   if (saveAs || path.isEmpty()) path=QFileDialog::getSaveFileName(this,"Save GMAT script",path,"GMAT scripts (*.script)");
   return saveScriptDocumentTo(document,path);
}
bool MainWindow::saveScriptDocumentTo(const std::shared_ptr<ScriptDocument> &document,const QString &path)
{
   if (running || !document || !document->editor || path.isEmpty()) return false;
   const auto identity=scriptDocumentIdentity(path);
   for (const auto &other:scriptDocuments) if (other!=document && !other->path.isEmpty() && scriptDocumentIdentity(other->path)==identity) {
      QMessageBox::warning(this,"Save failed","This file is open in another script window. Choose a different filename."); return false;
   }
   QSaveFile file(path); const auto bytes=document->editor->toPlainText().toUtf8();
   if (!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()) { QMessageBox::warning(this,"Save failed",file.errorString()); return false; }
   document->path=QFileInfo(path).absoluteFilePath(); document->savedText=document->editor->toPlainText();
   if (document==activeDocument) scriptPath=document->path;
   document->editor->document()->setModified(false); updateTitle(); rememberMissionFile(document->path); refreshRecentMenu(); return true;
}
bool MainWindow::resolveDocumentPanels()
{
   for (auto *child:workspace->subWindowList()) {
      auto *panel=dynamic_cast<EditablePanel *>(child->widget()); if (!panel || !panel->hasUnappliedInput()) continue;
      const auto choice=QMessageBox::question(this,"Pending active-mission changes","Apply or discard this panel's changes before changing the active mission?\n\n"+child->windowTitle(),QMessageBox::Apply|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Cancel);
      if (choice==QMessageBox::Cancel) return false;
      if (choice==QMessageBox::Discard) { panel->discardChanges(); child->close(); continue; }
      auto *buttons=panel->findChild<QDialogButtonBox *>(); auto *apply=buttons ? buttons->button(QDialogButtonBox::Apply) : nullptr;
      if (!apply) { statusBar()->showMessage("Apply this panel's changes before changing the active mission"); return false; }
      apply->click();
      auto *current=dynamic_cast<EditablePanel *>(child->widget());
      if (current && current->hasUnappliedInput()) { statusBar()->showMessage("Panel changes were rejected; correct them before changing the active mission"); return false; }
   }
   return true;
}
bool MainWindow::activateScriptDocument(const std::shared_ptr<ScriptDocument> &document)
{
   if (!ready || running || !document || !document->editor) return false;
   if (document==activeDocument) return buildScript();
   if (!resolveDocumentPanels()) return false;
   const auto previous=activeDocument; const auto previousPath=scriptPath,previousBuilt=builtScript,previousBreakpointSource=breakpointSource;
   const auto previousBreakpoints=breakpoints; const bool wasValid=modelValid;
   editor=document->editor; scriptPath=document->path;
   breakpoints.clear(); breakpointSource.clear(); summaryAvailable=false;
   if (!buildScript()) {
      editor=previous->editor; scriptPath=previousPath; builtScript=previousBuilt;
      breakpoints=previousBreakpoints; breakpointSource=previousBreakpointSource;
      if (wasValid) restoreBuiltModel(); else { modelValid=false; refreshTrees(); }
      updateTitle(); statusBar()->showMessage("Selected script could not be built; the previous active mission was retained"); return false;
   }
   activeDocument=document;
   for (auto *child:workspace->subWindowList()) if (auto *panel=dynamic_cast<EditablePanel *>(child->widget())) { panel->discardChanges(); child->close(); }
   for (const auto &other:scriptDocuments) if (other!=document && other->search) other->search->hide();
   plots->clear(true); updateTitle(); document->window->showNormal(); workspace->setActiveSubWindow(document->window); document->editor->setFocus();
   statusBar()->showMessage("Selected script is now the active mission"); return true;
}
bool MainWindow::confirmScriptClose(const std::shared_ptr<ScriptDocument> &document,bool discard)
{
   if (!document || !document->editor || !document->editor->document()->isModified()) return true;
   const auto choice=QMessageBox::question(this,"Unsaved script",QString("Save changes to %1 before closing?").arg(document->path.isEmpty() ? document->window->windowTitle() : document->path),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Cancel);
   if (choice==QMessageBox::Save) return saveScriptDocument(document);
   if (choice!=QMessageBox::Discard) return false;
   if (discard) { document->editor->setPlainText(document->savedText); document->editor->document()->setModified(false); }
   return true;
}
bool MainWindow::closeScriptDocument(const std::shared_ptr<ScriptDocument> &document)
{
   if (running || !confirmScriptClose(document,true)) return false;
   if (document->search) document->search->hide();
   if (document==activeDocument) {
      // Closing the active editor leaves its mission loaded, as in wx. The
      // hidden editor keeps the source needed for Build, GUI edits and Run.
      statusBar()->showMessage("Active script window closed; the mission remains loaded"); return true;
   }
   scriptDocuments.removeAll(document); document->window->setAttribute(Qt::WA_DeleteOnClose); updateScriptDocuments(); return true;
}
void MainWindow::reloadScriptDocument()
{
   if (running) return;
   const auto document=selectedScriptDocument(); if (!document || document->path.isEmpty()) { statusBar()->showMessage("Save this script before reloading it"); return; }
   QString text; if (!readScriptDocumentFile(this,document->path,text)) return;
   const auto answer=QMessageBox::question(this,"Reload script","Replace this editor's contents with the saved file?\n\n"+document->path,QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel);
   if (answer!=QMessageBox::Yes) return;
   if (document==activeDocument && !resolveDocumentPanels()) return;
   document->editor->setPlainText(text); document->savedText=text; document->editor->document()->setModified(false);
   if (document==activeDocument) { ++modelGeneration; summaryAvailable=false; modelValid=false; plots->clear(true); refreshTrees(); }
   updateTitle(); statusBar()->showMessage(document==activeDocument ? "Active script reloaded; Build to refresh Resources and Mission" : "Inactive script reloaded; the active mission is unchanged");
}
