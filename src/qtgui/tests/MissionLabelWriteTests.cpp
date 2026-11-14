#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "CommandForm.hpp"
#include "MissionModel.hpp"
#include "ReportParameterDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Assignment.hpp"
#include "GmatGlobal.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
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

static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static int findLabel(const MissionSnapshot &snapshot,const QString &name)
{
   for (int i=0;i<snapshot.nodes.size();++i)
      if (missionCommandLabel(snapshot.nodes[i].statement).name==name) return i;
   QString diagnostic="Missing command label "+name+"\nCanonical mission:\n"+snapshot.canonicalScript+"\nMapped nodes:\n";
   for (int i=0;i<snapshot.nodes.size();++i) {
      const auto &node=snapshot.nodes[i]; const auto label=missionCommandLabel(node.statement);
      diagnostic+=QString("%1 type=%2 label=%3 editable=%4 source=%5..%6 parsedName=%7\n%8\n").arg(i).arg(node.type,node.label).arg(node.editable).arg(node.start).arg(node.end).arg(label.name,node.statement);
   }
   throw std::runtime_error(diagnostic.toStdString());
}
static QTreeWidgetItem *treeItem(QTreeWidget *tree,int index)
{
   for (QTreeWidgetItemIterator it(tree);*it;++it)
      if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==index) return *it;
   throw std::runtime_error("Mission tree anchor missing");
}
static CommandEditor *panel(QMdiSubWindow *child)
{
   auto *result=child ? dynamic_cast<CommandEditor *>(child->widget()) : nullptr;
   require(result,"Command panel unavailable"); return result;
}
static void apply(QMdiSubWindow *child)
{
   auto *buttons=panel(child)->findChild<QDialogButtonBox *>();
   require(buttons && buttons->button(QDialogButtonBox::Apply),"Command Apply missing");
   buttons->button(QDialogButtonBox::Apply)->click();
}
static void serializationChecks()
{
   auto *global=GmatGlobal::Instance();
   struct RestoreKeyword {
      GmatGlobal *global; bool original;
      ~RestoreKeyword() { global->SetWriteGmatKeyword(original); }
   } restore{global,global->IsWritingGmatKeyword()};
   Assignment assignment; assignment.SetLHS("V"); assignment.SetRHS("V + 1"); assignment.SetName("Compute fixture");
   for (bool keyword:{false,true,false}) {
      global->SetWriteGmatKeyword(keyword);
      const auto expected=keyword ? "   GMAT 'Compute fixture' V = V + 1;" : "   'Compute fixture' V = V + 1;";
      require(assignment.GetGeneratingString(Gmat::SCRIPTING,"   ")==expected,"Assignment serialization lost/misplaced command name or changed keyword mode");
      require(assignment.GetGeneratingString(Gmat::SCRIPTING,"   ")==expected,"Repeated assignment serialization duplicated name");
      require(assignment.GetLHS()=="V" && assignment.GetRHS()=="V + 1" && assignment.GetName()=="Compute fixture","Assignment serialization changed operands/name");
   }
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtMissionLabelsWrite");
   try {
      require(argc==2 && acceptanceWidgetPlatform(),"Mission Label/Write check requires offscreen and one startup argument");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Fixture folder unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Mission Label/Write runtime initialization failed"); window.show();
      serializationChecks();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Mission");
      auto *workspace=window.findChild<QMdiArea *>("workspace"); require(editor && tree && workspace,"Mission workspace missing");
      const auto report=files.filePath("label write values.txt");
      const QString baseline="% preserve configuration spelling α\nCreate Spacecraft Craft;\nCreate Propagator Integrator;\n"
         "Create Variable V i;\nV = 7.0000;\ni = 0;\nCreate Array Cells[1,1];\nCells(1,1) = 2.5000;\n"
         "Create String Caption;\nCaption = 'V stays literal';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\n"
         "BeginMissionSequence;\n% branch comment α\nIf 'Outer old' V > 0; % preserve condition\n"
         "   For 'Loop old' i = 1:1:2; % preserve implicit options\n"
         "      GMAT 'Compute old' V = V + 1; % nested calculation\n"
         "   EndFor; % matching loop end\nElse; % alternate comments\n   V = V - 1;\nEndIf; % branch end\n"
         "Report 'Read old' Values V Cells(1,1); % no final newline";
      editor->setPlainText(baseline);
      if (!window.buildScript()) throw std::runtime_error(("Label fixture build failed:\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()).toStdString());
      // Prefix patches never rename RHS references/literals or regenerate a body.
      const QString assignment="  GMAT V = 'unchanged text'; % retain literal\n";
      require(setMissionCommandLabel(assignment,"Caption step")=="  GMAT 'Caption step' V = 'unchanged text'; % retain literal\n","Assignment label changed its RHS/comment");
      require(setMissionCommandLabel("Stop;","Stop here")=="Stop 'Stop here';","No-operand command label placement incorrect");
      require(!missionCommandLabel("SaveMission 'path.script';").editable(),"SaveMission filename was exposed as a command Name");
      require(!missionCommandLabel("BeginMissionSequence;").editable() && !missionCommandLabel("EndIf;").editable(),"Structural command became labelable");
      for (const auto &name:QStringList{"Someone's command",QString("Two\nlines"),QString("Tab\tname")}) {
         bool rejected=false; try { setMissionCommandLabel("Report Values V;",name); } catch (const std::exception &) { rejected=true; }
         require(rejected,"Unsafe command label accepted");
      }
      {
         CommandEditor boundary("BeginMissionSequence;",false,{},[](const QString &) { return QString(); });
         require(!boundary.findChild<QLineEdit *>("commandName")->isEnabled(),"BeginMissionSequence Name field enabled");
      }
      auto open=[&](const QString &name) {
         auto *item=treeItem(tree,findLabel(window.missionSnapshot(),name)); tree->itemDoubleClicked(item,0);
         auto *child=workspace->currentSubWindow(); panel(child); return child;
      };
      auto close=[&](QMdiSubWindow *child) { child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); };
      auto *child=open("Compute old"); auto *name=panel(child)->findChild<QLineEdit *>("commandName");
      auto *source=panel(child)->findChild<QPlainTextEdit *>("commandSource");
      require(name && name->text()=="Compute old" && name->isEnabled(),"Ordinary command Name missing");
      const auto oldPending=source->toPlainText(); name->setText("Broken's label"); apply(child);
      require(editor->toPlainText()==baseline && source->toPlainText()==oldPending && name->text()=="Broken's label" && panel(child)->hasChanges() && panel(child)->findChild<QLabel *>("commandStatus")->text().contains("Single quotes"),"Invalid Name changed source or lost correctable input");
      name->setText("Compute pending"); require(editor->toPlainText()==baseline && source->toPlainText().contains("'Compute pending'"),"Name did not remain pending");
      std::exception_ptr failure; bool visited=false;
      QTimer::singleShot(0,&window,[&] {
         auto *prompt=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
         try { require(prompt && prompt->button(QMessageBox::Cancel),"Pending Name close guard missing"); visited=true; prompt->button(QMessageBox::Cancel)->click(); }
         catch (...) { failure=std::current_exception(); if (prompt) prompt->reject(); }
      });
      panel(child)->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Close)->click();
      if (failure) std::rethrow_exception(failure); require(visited && child->isVisible() && panel(child)->hasChanges() && editor->toPlainText()==baseline,"Cancel lost pending Name");
      visited=false;
      QTimer::singleShot(0,&window,[&] {
         auto *prompt=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
         try { require(prompt && prompt->button(QMessageBox::Discard),"Name Discard missing"); visited=true; prompt->button(QMessageBox::Discard)->click(); }
         catch (...) { failure=std::current_exception(); if (prompt) prompt->reject(); }
      });
      panel(child)->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Close)->click();
      if (failure) std::rethrow_exception(failure); require(visited && editor->toPlainText()==baseline,"Discard applied Name"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      QString expected=baseline;
      for (const auto &pair:QList<QPair<QString,QString>>{{"Compute old","Compute α"},{"Loop old","Loop label"},{"Outer old","Outer label"},{"Read old","Read label"}}) {
         child=open(pair.first); name=panel(child)->findChild<QLineEdit *>("commandName"); name->setText(pair.second);
         auto renamed=expected; renamed.replace("'"+pair.first+"'","'"+pair.second+"'"); apply(child);
         require(editor->toPlainText()==renamed && !panel(child)->hasChanges() && panel(child)->findChild<QLineEdit *>("commandName")->text()==pair.second,"Label Apply lost branch body/comment/indentation/final newline or retained stale Name"); close(child);
         editor->undo(); require(editor->toPlainText()==expected && window.buildScript(),"Label Undo was not whole-source atomic");
         editor->redo(); require(editor->toPlainText()==renamed && window.buildScript(),"Label Redo changed source"); expected=renamed;
      }
      child=open("Read label"); source=panel(child)->findChild<QPlainTextEdit *>("commandSource");
      auto raw=source->toPlainText(); raw.replace("'Read label'","'Raw source label'"); source->setPlainText(raw);
      require(panel(child)->findChild<QLineEdit *>("commandName")->text()=="Raw source label","Raw source edits did not synchronize Name");
      panel(child)->findChild<QLineEdit *>("commandName")->clear();
      require(source->toPlainText()==raw.replace("'Raw source label'","") && panel(child)->hasChanges(),"Blank Name did not remove only its quoted label");
      panel(child)->discardChanges(); close(child); require(editor->toPlainText()==expected,"Discard after raw label edit mutated source");
      // Solver names are header labels; Vary operands and nested footer stay raw.
      const QString solver="% solver comment\nTarget 'Old solver label' DC {SolveMode = RunInitialGuess, ShowProgressWindow = false}; % header\n   Vary DC(V = +1.000e0, {Perturbation = 0.001});\n   Achieve DC(V = 2, {Tolerance = 1e-4});\nEndTarget; % footer\n";
      const auto solverRenamed=setMissionCommandLabel(solver,"Named target");
      auto solverExpected=solver; solverExpected.replace("'Old solver label'","'Named target'"); require(solverRenamed==solverExpected,"Solver label regenerated nested body/options");
      require(missionCommandLabel("Write V;").editable(),"Selected runtime Write cannot be named");
      // Exercise actual insertion template, ordered picker Cancel/Accept and
      // interpreter validation instead of inserting raw text directly.
      auto *anchor=treeItem(tree,findLabel(window.missionSnapshot(),"Read label")); tree->expandAll(); tree->scrollToItem(anchor); QApplication::processEvents();
      visited=false; failure=nullptr;
      QTimer::singleShot(0,&window,[&] {
         auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
         try {
            require(menu,"Mission append menu missing"); QAction *action=nullptr;
            for (auto *candidate:menu->actions()) if (candidate->text()=="Append command…") action=candidate;
            require(action && action->isEnabled(),"Mission append action missing"); visited=true; menu->setActiveAction(action);
            QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter);
         } catch (...) { failure=std::current_exception(); if (menu) menu->close(); }
      });
      tree->customContextMenuRequested(tree->visualItemRect(anchor).center()); if (failure) std::rethrow_exception(failure); require(visited,"Append action not exercised");
      child=workspace->currentSubWindow(); auto *choices=panel(child)->findChild<QComboBox *>("commandTemplate");
      require(choices && choices->findText("Write")>=0,"Selected runtime Write insertion template missing"); choices->setCurrentText("Write"); choices->textActivated("Write");
      name=panel(child)->findChild<QLineEdit *>("commandName"); name->setText("Write values"); source=panel(child)->findChild<QPlainTextEdit *>("commandSource");
      auto *choose=panel(child)->findChild<QPushButton *>("commandChoose_Parameters"); require(choose,"Simple Write parameter picker missing");
      const auto pendingWrite=source->toPlainText();
      auto pick=[&](bool accept) {
         failure=nullptr; visited=false;
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=panel(child)->findChild<QDialog *>("reportParameterDialog");
            try {
               require(dialog,"Write ordered parameter selector missing"); visited=true;
               dialog->findChild<QPushButton *>("reportParameterRemoveAll")->click();
               auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry");
               for (const auto &value:QStringList{"V","Cells(1,1)"}) { entry->setEditText(value); dialog->findChild<QPushButton *>("reportAddParameter")->click(); }
               auto *list=dialog->findChild<QListWidget *>("reportSelectedParameters"); require(list && list->count()==2 && list->item(1)->text()=="Cells(1,1)","Write picker lost parameter order/array reference");
               dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
            } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
         });
         choose->click(); if (failure) std::rethrow_exception(failure); require(visited,"Write picker not exercised");
      };
      pick(false); require(source->toPlainText()==pendingWrite && editor->toPlainText()==expected,"Write selector Cancel committed input");
      pick(true); require(source->toPlainText()=="Write 'Write values' V Cells(1,1);" && editor->toPlainText()==expected,"Write selector did not retain ordered pending values/name");
      source->setPlainText("Write 'Write values' MissingParameter;"); apply(child);
      require(editor->toPlainText()==expected && panel(child)->hasChanges() && source->toPlainText().contains("MissingParameter"),"Invalid Write lost pending input or mutated mission");
      source->setPlainText("Write 'Write values' V Cells(1,1); % Write comment α"); apply(child);
      const auto written=expected+"\nWrite 'Write values' V Cells(1,1); % Write comment α\n";
      require(editor->toPlainText()==written && !panel(child)->hasChanges(),"Write insertion did not commit one preserved command"); close(child);
      editor->undo(); require(editor->toPlainText()==expected && window.buildScript(),"Write Undo not exact/atomic");
      editor->redo(); require(editor->toPlainText()==written && window.buildScript(),"Write Redo not exact");
      // Option dictionaries use the complete source editor unchanged.
      QString changed; CommandForm full([&](const QString &value) { changed=value; });
      const QString options="Write 'Detailed values' V Cells(1,1) {Style = Verbose, MessageWindow = true, LogFile = false}; % full options";
      full.setStatement(options); require(full.isHidden() && changed.isEmpty(),"Write full options were captured by a lossy simple form");
      const auto saved=files.filePath("named mission α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && editor->toPlainText()==written && window.buildScript(),"Mission Name/Write Unicode save/reopen lost exact source");
      require(window.runMission()==MainWindow::RunResult::Completed,"Named commands/Write short mission did not complete");
      QFile data(report); require(data.open(QIODevice::ReadOnly),"Named-command report missing");
      const auto values=QString::fromUtf8(data.readAll()).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
      require(values.size()==2 && values[0].toDouble()==9 && values[1].toDouble()==2.5,"Label/Write editing changed independent mission calculations");
      std::cout<<"PASS pending Name/invalid correction/Cancel/Discard, ordinary+nested branch source labels, guarded structural command, exact comments/indentation/no-final-newline and atomic Undo/Redo, raw synchronization, runtime Write insertion/ordered picker/Cancel/reference validation/full-options fallback, Unicode Save/reopen and independent short mission report 9,2.5.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
