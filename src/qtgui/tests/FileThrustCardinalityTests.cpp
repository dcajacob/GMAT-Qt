#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "CommandForm.hpp"
#include "TestSettings.hpp"
#include "FactoryManager.hpp"
#include "GmatCommand.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

static void require(bool value,const char *message)
{
   if (!value) throw std::runtime_error(message);
}
static void write(const QString &path,const QString &body)
{
   QFile file(path); const auto bytes=body.toUtf8();
   require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Cannot write owned thrust input");
}
static void picker(CommandForm &form,const std::function<void(QDialog &,QListWidget &)> &action)
{
   std::exception_ptr failure;
   QTimer::singleShot(0,&form,[&] {
      auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
      try {
         require(dialog,"Spacecraft picker is absent");
         auto *list=dialog->findChild<QListWidget *>("fileThrustSpacecraftList");
         require(list && list->count()==2,"Picker does not contain the two spacecraft");
         require(list->selectionMode()==QAbstractItemView::SingleSelection,"Picker allows multiple row selection");
         action(*dialog,*list);
      } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   });
   auto *choose=form.findChild<QPushButton *>("commandChoose_Spacecraft");
   require(choose,"Spacecraft selector is absent"); choose->click();
   if (failure) std::rethrow_exception(failure);
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("FileThrustCardinality");
   if (argc!=2 || !acceptanceWidgetPlatform()) return 2;
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"No private fixture directory");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime initialization failed"); window.show();
      // Assert the real plugin boundary independently of Qt's validator.
      for (const auto &type:{"BeginFileThrust","EndFileThrust"}) {
         std::unique_ptr<GmatCommand> command(FactoryManager::Instance()->CreateCommand(type,""));
         require(command && command->SetRefObjectName(Gmat::SPACECRAFT,"A"),"Plugin command rejects its first spacecraft");
         require(command->SetRefObjectName(Gmat::SPACECRAFT,"A"),"Plugin no longer ignores duplicate identical spacecraft");
         bool rejected=false;
         try { command->SetRefObjectName(Gmat::SPACECRAFT,"B"); }
         catch (BaseException &error) { rejected=error.GetFullMessage().find("multiple Spacecraft")!=std::string::npos; }
         require(rejected,"Plugin did not reject a second distinct spacecraft");
      }
      const auto history=files.filePath("owned two segments.thf");
      QString input;
      for (const auto &segment:{"One","Two"}) input+=QString("BeginThrust {%1}\nStart_Epoch = 01 Jan 2000 12:00:%2.000\nThrust_Vector_Coordinate_System = EarthMJ2000Eq\nThrust_Vector_Interpolation_Method = None\nMass_Flow_Rate_Interpolation_Method = None\nModelAccel\n0 0.00001 0 0\n10 0.00001 0 0\nEndThrust {%1}\n").arg(segment).arg(QString::fromLatin1(segment)=="One" ? "00" : "10");
      write(history,input);
      QString source="% Keep ordinary mission source and comment α\nCreate Spacecraft A B;\nCreate ThrustSegment One Two;\nCreate ThrustHistoryFile HistA HistB;\n";
      source+="HistA.FileName = '"+history+"';\nHistA.AddThrustSegment = {One};\nHistB.FileName = '"+history+"';\nHistB.AddThrustSegment = {Two};\nBeginMissionSequence;\n";
      source+="BeginFileThrust 'First begin' HistA(A); % retain first begin\nBeginFileThrust 'Second begin' HistB(A); % retain second begin\nEndFileThrust 'First end' HistA(A); % retain first end\nEndFileThrust 'Second end' HistB(A); % retain second end\n";
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Source editor absent");
      editor->setPlainText(source); require(window.buildScript(),"Two independent history commands did not build");
      for (const auto &type:{QString("BeginFileThrust"),QString("EndFileThrust")}) {
         const auto snapshot=window.missionSnapshot(); int index=-1;
         for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type==type && snapshot.nodes[i].statement.contains("HistB(")) index=i;
         require(index>=0,"Second history command is absent");
         auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator item(tree);
         while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item;
         require(*item,"Second command tree item is absent"); tree->itemDoubleClicked(*item,0);
         auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow();
         auto *panel=child ? dynamic_cast<CommandEditor *>(child->widget()) : nullptr;
         require(panel,"Retained file-thrust command editor is absent");
         auto *form=dynamic_cast<CommandForm *>(panel->findChild<QGroupBox *>("commandForm"));
         auto *spacecraft=panel->findChild<QLineEdit *>("commandField_Spacecraft");
         auto *apply=panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);
         require(form && spacecraft && apply,"File-thrust controls are absent");
         spacecraft->setText("A, B");
         require(panel->hasChanges() && editor->toPlainText()==source,"Invalid multiple choice mutated the mission while pending");
         apply->click();
         require(editor->toPlainText()==source && panel->hasChanges(),"Invalid multiple choice was committed or discarded");
         require(panel->findChild<QLabel *>("commandStatus")->text().contains("Use separate BeginFileThrust"),"Pre-Apply cardinality guidance is absent");
         const auto pending=panel->findChild<QPlainTextEdit *>("commandSource")->toPlainText();
         picker(*form,[](QDialog &dialog,QListWidget &list) {
            require(!dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Unsupported imported list is silently preselected");
            for (int i=0;i<list.count();++i) require(list.item(i)->checkState()==Qt::Unchecked,"Unsupported list was silently normalized");
            dialog.reject();
         });
         require(panel->findChild<QPlainTextEdit *>("commandSource")->toPlainText()==pending && editor->toPlainText()==source,"Picker Cancel changed unsupported source");
         picker(*form,[](QDialog &dialog,QListWidget &list) {
            auto a=list.findItems("A",Qt::MatchExactly),b=list.findItems("B",Qt::MatchExactly);
            require(a.size()==1 && b.size()==1,"Typed spacecraft choices are absent");
            a.front()->setCheckState(Qt::Checked); b.front()->setCheckState(Qt::Checked);
            require(a.front()->checkState()==Qt::Unchecked && b.front()->checkState()==Qt::Checked,"Checkboxes permit two simultaneous choices");
            require(dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Valid single choice cannot be accepted");
            dialog.accept();
         });
         require(editor->toPlainText()==source && form->validationError().isEmpty(),"Single choice did not remain valid and pending");
         const auto expected=QString(source).replace(type+" 'Second "+(type=="BeginFileThrust" ? "begin" : "end")+"' HistB(A)",type+" 'Second "+(type=="BeginFileThrust" ? "begin" : "end")+"' HistB(B)");
         apply->click();
         auto *accepted=dynamic_cast<CommandEditor *>(child->widget());
         if (editor->toPlainText()!=expected || !child->isVisible() || !accepted || accepted->hasChanges()) {
            std::cerr<<"Cardinality Apply type="<<type.toStdString()<<" visible="<<child->isVisible()
               <<" clean="<<(accepted && !accepted->hasChanges())<<"\nEXPECTED\n"<<expected.toStdString()
               <<"\nACTUAL\n"<<editor->toPlainText().toStdString()<<"\nSTATUS\n"
               <<panel->findChild<QLabel *>("commandStatus")->text().toStdString()<<'\n';
            throw std::runtime_error("Single-choice Apply changed labels/comments or failed retention");
         }
         editor->undo(); require(editor->toPlainText()==source,"Cardinality correction Undo is not byte exact");
         editor->redo(); require(editor->toPlainText()==expected,"Cardinality correction Redo is not byte exact"); source=expected;
      }
      const auto saved=files.filePath("two thrust histories ü.script");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==source,"Two separate spacecraft commands did not survive Save/reopen");
      // Raw unsupported statements can still be inspected; opening/Cancel
      // never rewrites imported text. Validation only blocks a new Apply.
      QString changed;
      CommandForm imported([&](const QString &text) { changed=text; });
      const QString unsupported="  EndFileThrust 'Keep imported' HistA(A, B); % keep comment\n";
      imported.setStatement(unsupported);
      require(changed.isEmpty() && !imported.validationError().isEmpty(),"Opening unsupported source rewrote or accepted its cardinality");
      picker(imported,[](QDialog &dialog,QListWidget &) { dialog.reject(); });
      require(changed.isEmpty(),"Unsupported-source Cancel emitted a source change");
      imported.findChild<QLineEdit *>("commandField_Spacecraft")->setText("A, A");
      require(imported.validationError().isEmpty(),"Qt rejects backend-compatible duplicate identical references");
      std::cout<<"PASS: actual plugin one-spacecraft boundary; exclusive picker, unsupported-source Cancel, typed pre-Apply rejection/correction in retained Begin/End editors, exact source/Undo/Redo/Unicode reopen for two distinct history commands. No numerical mission run.\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
