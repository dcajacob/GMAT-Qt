#include "FunctionFileDialog.hpp"
#include "ScriptEditor.hpp"
#include "Moderator.hpp"
#include "FindReplaceDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QAction>
#include <QInputDialog>
#include <QTextBlock>
#include <QTextCursor>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QSaveFile>
#include <QStringConverter>
#include <QRegularExpression>
#include <stdexcept>

QString functionFileTemplate(const QString &path)
{
   const QFileInfo file(path); const auto name=file.completeBaseName();
   if (file.suffix()!="gmf" || !QRegularExpression("^[A-Za-z][A-Za-z0-9_]*$").match(name).hasMatch())
      throw std::runtime_error("Choose a .gmf filename starting with a letter and containing only letters, digits or underscores.");
   return "function [output] = "+name+"(input)\nCreate Variable output;\nBeginMissionSequence;\noutput = input;\n";
}

FunctionFileDialog::FunctionFileDialog(const QString &filename,QWidget *parent,const QString &initialText)
   : QDialog(parent),path(QFileInfo(filename).absoluteFilePath())
{
   setObjectName("functionFileDialog"); setWindowTitle("Edit function — "+path); resize(850,600);
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Save updates the function file. Run rebuilds the mission before using it. Cancel discards edits in this window.",this);
   help->setWordWrap(true); layout->addWidget(help);
   editor=new ScriptEditor(this); editor->setObjectName("functionFileText"); layout->addWidget(editor);
   QStringList keywords={"function"};
   auto *moderator=Moderator::Instance();
   if (moderator->IsInitialized())
      for (const auto &word:moderator->GetListOfAllFactoryItems()) keywords.append(QString::fromStdString(word));
   editor->setKeywords(keywords);

   auto *tools=new QHBoxLayout; layout->insertLayout(1,tools);
   auto *numbers=new QCheckBox("Line numbers",this); numbers->setObjectName("functionLineNumbers");
   numbers->setChecked(editor->lineNumbersVisible()); tools->addWidget(numbers);
   connect(numbers,&QCheckBox::toggled,editor,&ScriptEditor::setLineNumbersVisible);
   auto *go=new QPushButton("Go to line…",this); go->setObjectName("functionGoToLine");
   go->setAutoDefault(false); tools->addWidget(go); tools->addStretch();

   status=new QLabel(this); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   auto *saveAsButton=buttons->addButton("Save As…",QDialogButtonBox::ActionRole); saveAsButton->setObjectName("functionSaveAs");
   connect(saveAsButton,&QPushButton::clicked,this,[this] {
      QFileDialog chooser(this,"Save function as",path,"GMAT functions (*.gmf);;All files (*)");
      chooser.setObjectName("functionSaveAsDialog"); chooser.setAcceptMode(QFileDialog::AcceptSave); chooser.setDefaultSuffix("gmf");
      if (chooser.exec()!=QDialog::Accepted || chooser.selectedFiles().isEmpty()) return;
      const auto error=saveAs(chooser.selectedFiles().first(),true);
      if (error.isEmpty()) accept(); else status->setText(error);
   });
   auto *find=buttons->addButton("Find / replace…",QDialogButtonBox::ActionRole);
   find->setObjectName("functionFind"); find->setAutoDefault(false);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons->button(QDialogButtonBox::Save),&QPushButton::clicked,this,[this] {
      const auto error=save(); if (error.isEmpty()) accept(); else status->setText(error);
   });
   if (!initialText.isNull()) {
      newFile=true; loaded=true; editor->setPlainText(initialText); editor->document()->setModified(true);
   } else {
   QFile file(path);
   if (!file.open(QIODevice::ReadOnly)) status->setText("Cannot open function file: "+file.errorString());
   else {
      original=file.readAll();
      QStringDecoder decoder(QStringDecoder::Utf8); const QString text=decoder(original);
      if (file.error()!=QFileDevice::NoError) status->setText("Cannot read function file: "+file.errorString());
      else if (decoder.hasError()) status->setText("The function file is not valid UTF-8. It has not been opened for editing.");
      else { loaded=true; editor->setPlainText(text); editor->document()->setModified(false); }
   }
   }
   editor->setReadOnly(!loaded); buttons->button(QDialogButtonBox::Save)->setEnabled(loaded); find->setEnabled(loaded); saveAsButton->setEnabled(loaded);

   numbers->setEnabled(loaded); go->setEnabled(loaded);
   auto *search=new FindReplaceDialog(editor,this);
   auto localAction=[this](const char *name,const QString &text,const QKeySequence &shortcut,auto callback) {
      auto *action=new QAction(text,this); action->setObjectName(name); action->setShortcut(shortcut);
      action->setShortcutContext(Qt::WidgetWithChildrenShortcut); action->setEnabled(loaded); addAction(action);
      connect(action,&QAction::triggered,this,callback); return action;
   };
   auto *findAction=localAction("functionFindAction","Find…",QKeySequence::Find,[search] { search->openSearch(); });
   localAction("functionReplaceAction","Replace…",QKeySequence::Replace,[search] { search->openSearch(); });
   localAction("functionFindNextAction","Find next",QKeySequence::FindNext,[search] { search->findNext(); });
   localAction("functionFindPreviousAction","Find previous",QKeySequence::FindPrevious,[search] { search->findNext(true); });
   auto *goAction=localAction("functionGoToLineAction","Go to line…",QKeySequence("Ctrl+L"),[this] {
      bool accepted=false;
      const int line=QInputDialog::getInt(this,"Go to line","Line number",editor->textCursor().blockNumber()+1,
         1,editor->document()->blockCount(),1,&accepted);
      if (accepted) {
         editor->setTextCursor(QTextCursor(editor->document()->findBlockByNumber(line-1)));
         editor->centerCursor(); editor->setFocus();
      }
   });
   connect(find,&QPushButton::clicked,findAction,&QAction::trigger);
   connect(go,&QPushButton::clicked,goAction,&QAction::trigger);
   connect(this,&QDialog::finished,search,&QWidget::hide);

}
QString FunctionFileDialog::save()
{
   if (!loaded) return "Open a readable UTF-8 function file before saving.";
   if (newFile) {
      if (QFileInfo::exists(path)) return "A file now exists at this destination. Choose another path with Save As.";
      return writeTo(path);
   }
   QFile current(path);
   if (!current.open(QIODevice::ReadOnly)) return "Cannot check the current function file: "+current.errorString();
   const auto bytes=current.readAll();
   if (current.error()!=QFileDevice::NoError) return "Cannot read the current function file: "+current.errorString();
   if (bytes!=original) return "The function file changed outside this editor. Cancel and reopen it before saving.";
   current.close();
   if (!editor->document()->isModified()) return {};
   return writeTo(path);
}
QString FunctionFileDialog::saveAs(const QString &destination,bool replaceExisting)
{
   if (!loaded) return "Open a readable UTF-8 function file before saving.";
   if (destination.trimmed().isEmpty()) return "Choose a destination file.";
   const auto target=QFileInfo(destination).absoluteFilePath();
   if (target==path || (!QFileInfo(target).canonicalFilePath().isEmpty() && QFileInfo(target).canonicalFilePath()==QFileInfo(path).canonicalFilePath())) return save();
   if (QFileInfo::exists(target) && !replaceExisting) return "The destination already exists. Choose another file or confirm replacement.";
   return writeTo(target);
}
QString FunctionFileDialog::writeTo(const QString &destination)
{
   auto result=editor->toPlainText().toUtf8();
   if (original.contains("\r\n") && !QByteArray(original).replace("\r\n","").contains('\n')) result.replace("\n","\r\n");
   if (original.startsWith("\xef\xbb\xbf") && !result.startsWith("\xef\xbb\xbf")) result.prepend("\xef\xbb\xbf");
   QSaveFile file(destination);
   if (!file.open(QIODevice::WriteOnly)) return "Cannot save function file: "+file.errorString();
   if (file.write(result)!=result.size() || !file.commit()) return "Cannot save function file: "+file.errorString();
   newFile=false; original=result; path=destination; setWindowTitle("Edit function — "+path); editor->document()->setModified(false); return {};
}
