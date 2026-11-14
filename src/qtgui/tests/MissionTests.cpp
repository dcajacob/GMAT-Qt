#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "CommandEditor.hpp"
#include "ScriptEventDialog.hpp"
#include "CommandForm.hpp"
#include "ConditionDialog.hpp"
#include "ReportParameterDialog.hpp"
#include "ResourceEditor.hpp"
#include <QFileDialog>
#include <QFile>
#include <QSpinBox>
#include <QComboBox>
#include "Moderator.hpp"
#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <QMenu>
#include <QKeyEvent>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QMdiSubWindow>
#include <QMdiArea>
#include <QMessageBox>
#include <QTimer>
#include <QInputDialog>
#include <QTemporaryDir>
#include <QCheckBox>
#include <QDockWidget>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>
#include <stdexcept>
static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static int find(const MissionSnapshot &snapshot,const QString &type,const QString &parent={})
{
   for (int i=0;i<snapshot.nodes.size();++i) {
      const auto &node=snapshot.nodes[i];
      if (node.type==type && (parent.isEmpty() || (node.parent>=0 && snapshot.nodes[node.parent].type==parent))) return i;
   }
   throw std::runtime_error("Expected mission node missing");
}
static int forAssignment(const MissionSnapshot &snapshot)
{
   for (int i=0;i<snapshot.nodes.size();++i) {
      const auto &node=snapshot.nodes[i];
      if (node.parent>=0 && snapshot.nodes[node.parent].type=="For" && node.statement.contains("total = total + 1")) return i;
   }
   throw std::runtime_error("For-body assignment missing");
}
static void insertionTests(MainWindow &window,QPlainTextEdit *editor)
{
   auto *tree=window.findChild<QTreeWidget *>("Mission");
   auto *workspace=window.findChild<QMdiArea *>();
   require(tree && workspace,"Insertion workspace unavailable");
   const QString baseline="% preserve insertion source α\nCreate Variable V i;\nV = 1;\n"
      "Create DifferentialCorrector DC;\nCreate Yukon Opt;\nCreate ReportFile Idle;\n"
      "BeginMissionSequence;\nIf 'Outer' V > 0; % keep condition\n"
      "   For 'Inner' i = 1:1:2;\n      V = V + 1; % keep inner body\n"
      "   EndFor; % inner for end\nElse; % alternate branch\n"
      "   While V < 0;\n      V = V + 2;\n   EndWhile; % while end\n"
      "EndIf; % outer if end\n"
      "Target 'Keep target' DC {SolveMode = RunInitialGuess, ShowProgressWindow = false};\n"
      "   Vary DC(V = 1, {Perturbation = 0.001});\n   Achieve DC(V = 2);\n"
      "EndTarget; % target end\n"
      "Optimize 'Keep optimizer' Opt {SolveMode = RunInitialGuess, ShowProgressWindow = false};\n"
      "   Vary Opt(V = 1, {Perturbation = 0.001});\n   Minimize Opt(V);\n"
      "EndOptimize; % optimizer end\n";
   auto build=[&] { editor->setPlainText(baseline); require(window.buildScript(),"Branch insertion fixture failed to build"); };
   auto open=[&](int index) {
      QTreeWidgetItem *item=nullptr;
      for (QTreeWidgetItemIterator it(tree);*it;++it)
         if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==index) item=*it;
      require(item,"Branch insertion tree anchor missing"); tree->expandAll(); tree->scrollToItem(item); QApplication::processEvents();
      std::exception_ptr failure; bool visited=false;
      QTimer::singleShot(0,&window,[&] {
         auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
         try {
            require(menu,"Branch insertion menu missing"); QAction *choice=nullptr;
            for (auto *action:menu->actions()) if (action->objectName()=="appendInsideMissionBranch") choice=action;
            require(choice && choice->isEnabled(),"Append inside branch action missing or disabled"); visited=true;
            menu->setActiveAction(choice); QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter);
         } catch (...) { failure=std::current_exception(); if (menu) menu->close(); }
      });
      tree->customContextMenuRequested(tree->visualItemRect(item).center());
      if (failure) std::rethrow_exception(failure); require(visited,"Branch insertion menu was not exercised");
      auto *child=workspace->currentSubWindow();
      require(child && dynamic_cast<CommandEditor *>(child->widget()),"Branch insertion editor missing"); return child;
   };
   auto apply=[&](QMdiSubWindow *child) {
      auto *buttons=child->widget()->findChild<QDialogButtonBox *>();
      require(buttons && buttons->button(QDialogButtonBox::Apply),"Insertion Apply unavailable"); buttons->button(QDialogButtonBox::Apply)->click();
   };
   // The mapped closing terminator belongs to the selected branch. NASA's
   // MissionTree::Append inserts at its own Else when present, otherwise at
   // that closing terminator (src/gui/mission/MissionTree.cpp).
   const QMap<QString,QString> boundaries={{"For","   EndFor; % inner for end\n"},
      {"While","   EndWhile; % while end\n"},{"If","Else; % alternate branch\n"},
      {"Target","EndTarget; % target end\n"},{"Optimize","EndOptimize; % optimizer end\n"}};
   for (auto it=boundaries.cbegin();it!=boundaries.cend();++it) {
      build(); const auto snapshot=window.missionSnapshot(); const int index=find(snapshot,it.key());
      const int end=missionBranchEnd(snapshot,index);
      require(end>=0 && snapshot.nodes[end].parent==index && snapshot.nodes[end].type=="End"+it.key(),"Branch append selected a nested or unrelated terminator");
      require(missionBranchEnd(snapshot,end)==-1 && missionBranchEnd(snapshot,find(snapshot,"GMAT"))==-1 && missionBranchEnd(snapshot,-1)==-1,"Non-branch has an interior append target");
      auto *child=open(index); auto *source=child->widget()->findChild<QPlainTextEdit *>("commandSource");
      require(source,"Insertion source missing"); source->setPlainText("V = V + 5;");
      require(editor->toPlainText()==baseline,"Pending insertion changed mission source"); apply(child);
      auto expected=baseline; expected.replace(it.value(),"V = V + 5;\n"+it.value());
      if (editor->toPlainText()!=expected || !child->isVisible() || dynamic_cast<CommandEditor *>(child->widget())->hasChanges()) {
         const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
         if (!evidence.isEmpty()) {
            const QMap<QString,QString> values{{"actual",editor->toPlainText()},{"expected",expected}};
            for (auto entry=values.cbegin();entry!=values.cend();++entry) {
               QFile file(QDir(evidence).filePath("insertion-"+it.key()+"-"+entry.key()+".script"));
               if (file.open(QIODevice::WriteOnly)) file.write(entry.value().toUtf8());
            }
         }
         const auto *status=child->findChild<QLabel *>("commandStatus");
         std::cerr<<"Branch insertion "<<it.key().toStdString()<<" source_match="<<(editor->toPlainText()==expected)
            <<" child_visible="<<child->isVisible()<<" panel_changed="<<dynamic_cast<CommandEditor *>(child->widget())->hasChanges()
            <<" status="<<(status ? status->text().toStdString() : "missing")<<'\n';
      }
      require(editor->toPlainText()==expected && child->isVisible() && !dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Branch append changed outside source or did not retain the accepted editor");
      source=child->widget()->findChild<QPlainTextEdit *>("commandSource");
      require(source && source->toPlainText().trimmed()=="V = V + 5;","Accepted branch insertion points to its terminator or enclosing branch");
      source->setPlainText("V = V + 6;"); apply(child); expected.replace("V = V + 5;","V = V + 6;");
      require(editor->toPlainText()==expected && editor->toPlainText().count("V = V + 6;")==1,"Retained insertion Apply duplicated the command or changed its anchor"); child->close();
      editor->undo(); expected.replace("V = V + 6;","V = V + 5;"); require(editor->toPlainText()==expected,"Retained insertion Undo lost exact source");
      editor->undo(); require(editor->toPlainText()==baseline && window.buildScript(),"Branch insertion Undo lost exact source or model");
      editor->redo(); editor->redo(); expected.replace("V = V + 5;","V = V + 6;");
      require(editor->toPlainText()==expected && window.buildScript(),"Branch insertion Redo changed exact source or model");
   }
   // Toggle creation must expose the same typed controls as existing Toggles.
   build(); auto *child=open(find(window.missionSnapshot(),"For"));
   auto selectToggle=[&](QMdiSubWindow *window) {
      auto *templates=window->widget()->findChild<QComboBox *>("commandTemplate");
      require(templates && templates->findText("Toggle")>=0,"Toggle creation template unavailable");
      templates->setCurrentText("Toggle"); templates->textActivated("Toggle");
      require(window->widget()->findChild<QComboBox *>("commandToggleState") && window->widget()->findChild<QPushButton *>("commandChoose_Subscribers"),"Toggle template did not expose typed output/state controls");
   };
   selectToggle(child); bool discarded=false; std::exception_ptr cancelFailure;
   QTimer::singleShot(0,&window,[&] {
      auto *prompt=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
      try { require(prompt && prompt->button(QMessageBox::Discard),"Pending Toggle close guard unavailable"); discarded=true; prompt->button(QMessageBox::Discard)->click(); }
      catch (...) { cancelFailure=std::current_exception(); if (prompt) prompt->reject(); }
   });
   child->widget()->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Close)->click();
   if (cancelFailure) std::rethrow_exception(cancelFailure);
   require(discarded && editor->toPlainText()==baseline,"Discarding a pending Toggle changed source");
   child=open(find(window.missionSnapshot(),"For")); selectToggle(child);
   auto *source=child->widget()->findChild<QPlainTextEdit *>("commandSource");
   source->setPlainText("Toggle MissingOutput Off;"); apply(child);
   require(editor->toPlainText()==baseline && dynamic_cast<CommandEditor *>(child->widget())->hasChanges() && source->toPlainText()=="Toggle MissingOutput Off;","Invalid inserted Toggle lost pending input or mutated mission");
   source->setPlainText("Toggle 'Pause outputs' Idle On; % retain toggle comment");
   auto *state=child->widget()->findChild<QComboBox *>("commandToggleState"); require(state,"Corrected Toggle typed state unavailable"); state->setCurrentText("Off");
   require(source->toPlainText()=="Toggle 'Pause outputs' Idle Off; % retain toggle comment","Typed Toggle state lost label/comment"); apply(child);
   auto expected=baseline; expected.replace(boundaries.value("For"),"Toggle 'Pause outputs' Idle Off; % retain toggle comment\n"+boundaries.value("For"));
   require(editor->toPlainText()==expected && !dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Configured Toggle was not inserted inside the selected loop"); child->close();
   editor->undo(); require(editor->toPlainText()==baseline && window.buildScript(),"Toggle insertion Undo changed exact source");
   editor->redo(); require(editor->toPlainText()==expected && window.buildScript(),"Toggle insertion Redo changed exact source");
   QTemporaryDir saved; require(saved.isValid(),"Insertion round-trip folder unavailable"); const auto path=saved.filePath("branch insertion ü.script");
   require(window.saveScriptTo(path) && window.loadScript(path) && editor->toPlainText()==expected && window.buildScript(),"Branch/Toggle Unicode Save and reopen changed exact source or model");
   std::cout<<"PASS: actual If/For/While/Target/Optimize interior append, own closing boundaries, retained Apply, exact unrelated source and Undo/Redo; Toggle template/typed state, pending Discard, invalid-reference correction and Unicode Save/reopen without mission execution\n";
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtMission");
   if (argc<3 || argc>4) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(), script=QFileInfo(argv[2]).absoluteFilePath();
   const bool insertionOnly=argc==4 && QString::fromLocal8Bit(argv[3])=="--insertion";
   if (insertionOnly && !acceptanceWidgetPlatform()) return 2;
   const auto screenshot=argc==4 && !insertionOnly ? QFileInfo(argv[3]).absoluteFilePath() : QString();
   QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings isolatedSettings;
      // Saved scripts and referenced assets must outlive the open window.
      std::vector<std::unique_ptr<QTemporaryDir>> fixtureDirectories;
      auto fixtureDirectory=[&]() -> QTemporaryDir & {
         fixtureDirectories.emplace_back(new QTemporaryDir);
         return *fixtureDirectories.back();
      };
      MainWindow window; window.show(); require(window.initialize(startup),"Runtime failed");
      if (!insertionOnly) {
         // Function-call forms follow the initialized native registry and
         // declared function resources, just like forms opened by the GUI.
         require(Moderator::Instance()->CreateFunction("GmatFunction","myFunction"),"Standalone function-form fixture unavailable");
         // The shipped Ex_GMATFunction_Math.script likewise declares cross as
         // a GmatFunction; it is not registered as a Function factory type.
         require(Moderator::Instance()->CreateFunction("GmatFunction","cross"),"Cross-product function-form fixture unavailable");
         QString result;
         CommandForm form([&](const QString &text) { result=text; });
         const QString vary="  Vary 'Change X' DC(x = 1, {Upper = 10, Perturbation = 0.001, Lower = -10}); % keep this\n";
         form.setStatement(vary);
         auto *upper=form.findChild<QLineEdit *>("commandField_Upper");
         require(upper && !form.isHidden(),"Vary form missing");
         upper->setText("25");
         QString expected=vary; expected.replace("Upper = 10","Upper = 25");
         require(result==expected,"Vary form changed unrelated source");
         form.findChild<QLineEdit *>("commandField_Initial value")->setText("3.5");
         expected.replace("x = 1","x = 3.5"); require(result==expected,"Multiple field edits lost a value");
         form.findChild<QPushButton *>("commandAddOptions")->click();
         require(result.contains("Upper = 25") && result.contains("x = 3.5") && result.contains("% keep this") &&
            result.count("Perturbation =")==1 && result.contains("MultiplicativeScaleFactor = 1.0"),"Adding Vary defaults lost pending values or duplicated options");
         require(!form.findChild<QPushButton *>("commandAddOptions"),"Vary defaults can be added twice");
         for (const auto &command:{QString("Vary"),QString("Achieve")}) {
            form.setStatement(command+" 'Keep name' DC(x = 2); % keep comment");
            form.findChild<QLineEdit *>("commandField_Solver")->setText("OtherDC");
            form.findChild<QPushButton *>("commandAddOptions")->click();
            require(result.startsWith(command+" 'Keep name' OtherDC(x = 2, {") && result.endsWith("}); % keep comment"),
               "Omitted solver options inserted outside argument list or lost source");
            auto *setting=form.findChild<QLineEdit *>(command=="Vary" ? "commandField_MaxStep" : "commandField_Tolerance");
            require(setting,"Added solver options did not expose editable controls"); setting->setText("0.02");
            require(result.contains(command=="Vary" ? "MaxStep = 0.02" : "Tolerance = 0.02"),"Added solver option cannot be edited");
         }
         const QString loop="For 'Keep loop' i = 1:2:7;\n   total = total + i; % body stays exact\nEndFor;";
         form.setStatement(loop);
         auto *step=form.findChild<QLineEdit *>("commandField_Step");
         require(step && !form.isHidden(),"For controls missing");
         step->setText("3"); expected=loop; expected.replace("1:2:7","1:3:7");
         require(result==expected,"For control changed branch contents or label");
         const QString condition="If 'Choose' total > 2 & i < 9;\n total = 8;\nElse;\n total = 4;\nEndIf;";
         form.setStatement(condition);
         auto *predicate=form.findChild<QLineEdit *>("commandField_Condition");
         require(predicate,"Conditional controls missing");
         auto *builder=form.findChild<QPushButton *>("commandChoose_Condition");
         require(builder && builder->isEnabled(),"Compound condition builder missing");
         QTimer::singleShot(0,&form,[&] {
            if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
               dialog->findChild<QTableWidget *>("conditionTable")->item(0,1)->setText("discarded"); dialog->reject();
            }
         }); builder->click(); require(predicate->text()=="total > 2 & i < 9","Cancel changed the condition");
         bool emptyRejected=false;
         QTimer::singleShot(0,&form,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
            auto *table=dialog->findChild<QTableWidget *>("conditionTable");
            table->setCurrentCell(1,1); dialog->findChild<QPushButton *>("conditionRemove")->click();
            dialog->findChild<QPushButton *>("conditionAdd")->click();
            emptyRejected=!dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled();
            dialog->findChild<QPushButton *>("conditionRemove")->click();
            static_cast<QComboBox *>(table->cellWidget(0,2))->setCurrentText(">="); table->item(0,3)->setText("3"); dialog->accept();
         }); builder->click(); require(emptyRejected,"Incomplete condition row was accepted");
         expected=condition; expected.replace("total > 2 & i < 9","total >= 3");
         require(result==expected,"Condition editor changed branch bodies");
         predicate->setText("(total > 2 | i < 9)");
         require(!builder->isEnabled(),"Unsupported grouped syntax was offered a lossy conversion");
         require(ConditionDialog::supports("A(1, 2) >= -1.2e-3 | total ~= 0"),"Array/numeric compound condition was rejected");

         const QString bareBranch="Target 'Keep label' DC; % keep header\n   total = total + 1; % keep body\nEndTarget;";
         form.setStatement(bareBranch);
         auto *defaults=form.findChild<QPushButton *>("commandAddOptions"); require(defaults,"Omitted solver options cannot be exposed"); defaults->click();
         require(result.contains("SolveMode = Solve") && result.contains("ExitMode = DiscardAndContinue") && result.contains("ShowProgressWindow = true") &&
            result.endsWith("; % keep header\n   total = total + 1; % keep body\nEndTarget;"),"Default options damaged branch source");
         auto *solveMode=form.findChild<QComboBox *>("commandChoice_SolveMode"),*exitChoice=form.findChild<QComboBox *>("commandChoice_ExitMode");
         require(solveMode && solveMode->findText("RunInitialGuess")>=0 && exitChoice && exitChoice->findText("Stop")>=0,"Solver option choices missing");
         solveMode->setCurrentText("RunInitialGuess"); exitChoice->setCurrentText("SaveAndContinue");
         form.findChild<QCheckBox *>("commandCheck_ShowProgressWindow")->setChecked(false);
         require(result.contains("SolveMode = RunInitialGuess") && result.contains("ExitMode = SaveAndContinue") && result.contains("ShowProgressWindow = false"),"Solver controls did not update source");
         form.setStatement("Optimize Opt {ExitMode = Stop};\nEndOptimize;");
         form.findChild<QLineEdit *>("commandField_Solver")->setText("OtherOpt");
         form.findChild<QPushButton *>("commandAddOptions")->click();
         require(result.contains("Optimize OtherOpt") && result.contains("ExitMode = Stop") && result.count("ExitMode")==1,"Adding missing options lost a pending field or replaced an existing option");
         form.setStatement("FindEvents 'Find' Locator; % keep");
         form.findChild<QLineEdit *>("commandField_Locator")->setText("OtherLocator");
         form.findChild<QPushButton *>("commandAddOptions")->click();
         require(result=="FindEvents 'Find' OtherLocator {Append = false}; % keep","Omitted Append lost pending locator or source");
         require(!form.findChild<QPushButton *>("commandAddOptions"),"Existing Append offered duplicate insertion");
         form.setStatement("FindEvents 'Find' Locator {Append = false}; % keep");
         form.findChild<QCheckBox *>("commandCheck_Append")->setChecked(true);
         require(result=="FindEvents 'Find' Locator {Append = true}; % keep","Append checkbox changed unrelated command text");
         const QString assignment="GMAT total = sqrt(4) + 3; % preserve comment\n";
         form.setStatement(assignment);
         auto *expression=form.findChild<QLineEdit *>("commandField_Expression");
         require(expression,"Assignment controls missing"); expression->setText("sqrt(9) + 4");
         expected=assignment; expected.replace("sqrt(4) + 3","sqrt(9) + 4");
         require(result==expected,"Assignment control changed other source");
         const QString branch="Target 'Keep label' DC {ExitMode = SaveAndContinue, SolveMode = Solve};\n   Vary DC(x = 1);\nEndTarget;";
         form.setStatement(branch);
         auto *exitMode=form.findChild<QLineEdit *>("commandField_ExitMode");
         require(exitMode,"Target branch form missing"); exitMode->setText("DiscardAndContinue");
         expected=branch; expected.replace("SaveAndContinue","DiscardAndContinue");
         require(result==expected,"Solver form changed nested command source");
         for (const auto &statement:{"Maneuver 'Keep label' Burn(Sat);","BeginFiniteBurn Burn(Sat);",
              "Achieve DC(x = 7, {Tolerance = 0.001});","Minimize Opt(cost);",
              "NonlinearConstraint Opt(x <= 4);","Report R x y;","FindEvents Locator {Append = true};",
              "[out] = cross(vec1, vec2);","myFunction(input);",
              "Toggle PlotA PlotB Off;","Global Sat total;","Clear Sat total;",
              "For i = 1:3;\nEndFor;","While total < 3;\nEndWhile;"}) {
            form.setStatement(statement); require(!form.isHidden(),"Command-specific form missing");
         }
         form.setStatement("% Preserve advanced script\nUnknown foo;"); require(form.isHidden(),"Unknown command was simplified");
      }
      require(window.loadScript(script) && window.buildScript(),"Mission failed to build");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString original=editor->toPlainText();
      insertionTests(window,editor);
      if (insertionOnly) return 0;
      require(window.loadScript(script) && window.buildScript(),"Insertion test restoration failed");
      {
         editor->setPlainText("Create Spacecraft FirstSat SecondSat;\nCreate ImpulsiveBurn FirstBurn SecondBurn;\n"
            "SecondBurn.CoordinateSystem = EarthMJ2000Eq;\nSecondBurn.Element1 = 0.01;\n"
            "Create FiniteBurn Continuous;\nCreate DifferentialCorrector DC;\nCreate Yukon Opt;\n"
            "Create EclipseLocator Eclipse;\nEclipse.Spacecraft = FirstSat;\nEclipse.RunMode = Manual;\n"
            "BeginMissionSequence;\nManeuver 'Named burn' FirstBurn(FirstSat); % keep\n");
         require(window.buildScript(),"Resource picker fixture failed");
         auto pick=[&](CommandForm &form,const QString &field,const QString &chosen,const QString &excluded,bool accept=true) {
            bool visited=false;
            QTimer::singleShot(0,[&] {
               auto *dialog=form.findChild<QInputDialog *>();
               require(dialog && dialog->comboBoxItems().contains(chosen),"Resource picker omitted compatible resource");
               require(excluded.isEmpty() || !dialog->comboBoxItems().contains(excluded),"Resource picker included incompatible resource");
               dialog->setTextValue(chosen); visited=true;
               if (accept) dialog->accept(); else dialog->reject();
            });
            auto *button=form.findChild<QPushButton *>("commandChoose_"+field); require(button,"Command reference selector missing"); button->click();
            require(visited,"Command resource dialog did not open");
         };
         QString replacement; CommandForm form([&](const auto &value) { replacement=value; });
         form.setStatement("Maneuver 'Named burn' FirstBurn(FirstSat); % keep");
         pick(form,"Burn","SecondBurn","Continuous"); pick(form,"Spacecraft","SecondSat","SecondBurn");
         const auto accepted=replacement;
         pick(form,"Burn","FirstBurn","Continuous",false);
         require(replacement==accepted && replacement=="Maneuver 'Named burn' SecondBurn(SecondSat); % keep", "Resource pickers changed labels, comments or cancelled values");
         const auto snapshot=window.missionSnapshot();
         require(window.applyMissionChange(snapshot,find(snapshot,"Maneuver"),MissionEdit::Replace,replacement).isEmpty(),"Picked maneuver references failed to apply");
         auto &selectedMission=fixtureDirectory();
         require(selectedMission.isValid() && window.saveScriptTo(selectedMission.filePath("picked.script")) &&
            window.loadScript(selectedMission.filePath("picked.script")) && window.buildScript(),"Picked command save/reopen failed");
         auto *before=Moderator::Instance()->GetConfiguredObject("SecondSat"); const double vx=before->GetRealParameter("VX");
         require(window.runMission()==MainWindow::RunResult::Completed,"Picked maneuver failed to execute");
         auto *after=Moderator::Instance()->GetInternalObject("SecondSat");
         require(std::abs(after->GetRealParameter("VX")-vx-.01)<1e-10,"Picked inertial burn did not change selected spacecraft velocity");
         const auto forward=window.missionSnapshot(); const int maneuver=find(forward,"Maneuver");
         form.setStatement(forward.nodes[maneuver].statement);
         auto *backprop=form.findChild<QCheckBox *>("commandBackProp"); require(backprop && !backprop->isChecked(),"Backprop control missing or initially enabled");
         backprop->setChecked(true);
         require(replacement.contains("BackProp SecondBurn(SecondSat)") && replacement.contains("'Named burn'"),
            "Backprop toggle displaced burn reference or command label");
         require(window.applyMissionChange(forward,maneuver,MissionEdit::Replace,replacement).isEmpty(),"Backprop command edit failed");
         require(window.saveScriptTo(selectedMission.filePath("backprop.script")) && window.loadScript(selectedMission.filePath("backprop.script")) &&
            window.runMission()==MainWindow::RunResult::Completed,"Backprop save/reopen or execution failed");
         require(std::abs(Moderator::Instance()->GetInternalObject("SecondSat")->GetRealParameter("VX")-vx+.01)<1e-10,
            "Backprop did not reverse inertial delta-V");
         form.setStatement(replacement); backprop=form.findChild<QCheckBox *>("commandBackProp");
         require(backprop->isChecked(),"Existing BackProp keyword not reflected in control"); backprop->setChecked(false);
         require(!replacement.contains("BackProp") && replacement.contains("SecondBurn(SecondSat)"),"Clearing Backprop damaged command");
         form.setStatement("BeginFiniteBurn Continuous(FirstSat);"); pick(form,"Burn","Continuous","SecondBurn");
         form.setStatement("Target DC;\nEndTarget;"); pick(form,"Solver","DC","Opt");
         form.setStatement("Optimize Opt;\nEndOptimize;"); pick(form,"Solver","Opt","DC");
         form.setStatement("Achieve DC(FirstSat.X = 1);"); pick(form,"Solver","DC","Opt");
         form.setStatement("Minimize Opt(FirstSat.X);"); pick(form,"Solver","Opt","DC");
         form.setStatement("NonlinearConstraint Opt(FirstSat.X >= 1);"); pick(form,"Solver","Opt","DC");
         form.setStatement("Vary 'Keep options' DC(FirstSat.X = 1, {Lower = -20, Upper = 25}); % preserved");
         form.findChild<QPushButton *>("commandAddOptions")->click();
         auto *lower=form.findChild<QLineEdit *>("commandField_Lower");
         auto *upper=form.findChild<QLineEdit *>("commandField_Upper");
         require(lower->isEnabled() && upper->isEnabled(),"DifferentialCorrector bounds are disabled");
         lower->setText("-30"); const auto dcSource=replacement;
         pick(form,"Solver","Opt",{});
         require(!lower->isEnabled() && !upper->isEnabled() && lower->text()=="-30" && upper->text()=="25" && !lower->toolTip().isEmpty(),
            "Yukon bounds were editable or their pending values were lost");
         for (const auto &key:{"Perturbation","MaxStep","AdditiveScaleFactor","MultiplicativeScaleFactor"})
            require(form.findChild<QLineEdit *>(QString("commandField_")+key)->isEnabled(),"Yukon supported setting disabled");
         pick(form,"Solver","DC",{});
         require(lower->isEnabled() && upper->isEnabled() && replacement==dcSource,"Switching solvers lost pending settings or source");
         form.findChild<QLineEdit *>("commandField_Solver")->setText("MissingSolver");
         require(!lower->isEnabled(),"Unknown solver exposes capability-specific controls");
         form.findChild<QLineEdit *>("commandField_Solver")->setText("DC");
         require(lower->isEnabled() && replacement==dcSource,"Invalid solver recovery lost settings");
         pick(form,"Solver","Opt",{},false);
         require(lower->isEnabled() && replacement==dcSource,"Cancelled solver selection changed controls");
         form.setStatement("FindEvents Eclipse;"); pick(form,"Locator","Eclipse","DC");
         editor->setPlainText(original); require(window.buildScript(),"Picker fixture restoration failed");
      }

      auto snapshot=window.missionSnapshot();
      for (const auto &node:snapshot.nodes) {
         if (node.start<0) std::cerr<<"Unmapped "<<node.type.toStdString()<<": "<<node.statement.toStdString()<<'\n';
         require(node.start>=0 && node.end>node.start,"Command source range not mapped");
      }
      require(snapshot.nodes[find(snapshot,"If","If")].parent>=0,"Nested branch not represented");
      require(find(snapshot,"Else","If")>=0 && find(snapshot,"EndFor","For")>=0,"Branch structure missing");
      auto total=[&] {
         const auto result=window.runMission();
         if (result!=MainWindow::RunResult::Completed) {
            std::cerr<<"Mission execution failure source:\n"<<editor->toPlainText().toStdString()<<'\n';
            if (auto *messages=window.findChild<QPlainTextEdit *>("messageWindow")) std::cerr<<messages->toPlainText().toStdString()<<'\n';
         }
         require(result==MainWindow::RunResult::Completed,"Mission did not run");
         auto *value=Moderator::Instance()->GetInternalObject("total"); require(value!=nullptr,"Result variable missing");
         return value->GetRealParameter("Value");
      };
      require(total()==6,"Initial mission result incorrect");
      {
         QString changed;
         CommandForm form([&](const QString &value) { changed=value; });
         snapshot=window.missionSnapshot();
         const auto index=find(snapshot,"For");
         form.setStatement(snapshot.nodes[index].statement);
         auto *step=form.findChild<QLineEdit *>("commandField_Step");
         require(step,"Real mission For header has no step control");
         QTimer::singleShot(0,[&] {
            auto *dialog=form.findChild<QInputDialog *>();
            require(dialog && dialog->comboBoxItems().contains("count"),"For index picker omitted Variable");
            for (const auto &name:dialog->comboBoxItems())
               require(Moderator::Instance()->GetConfiguredObject(name.toStdString())->IsOfType("Variable"),"For index picker included a non-variable");
            dialog->setTextValue("total"); dialog->reject();
         }); form.findChild<QPushButton *>("commandChoose_Index")->click();
         require(form.findChild<QLineEdit *>("commandField_Index")->text()=="count","Cancelled For index changed source");
         for (const auto &field:{QString("Start"),QString("Step"),QString("End")}) {
            QTimer::singleShot(0,[&] {
               auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
               auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry");
               require(entry && entry->findText("total")>=0,"For bound picker omitted configured parameter");
               entry->setEditText(field=="Step" ? "2" : field=="Start" ? "1" : "3");
               dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
            }); form.findChild<QPushButton *>("commandChoose_"+field)->click();
         }
         require(window.applyMissionChange(snapshot,index,MissionEdit::Replace,changed).isEmpty() && total()==5,
            "For form edit did not execute with the new step");
         editor->undo(); require(window.buildScript() && total()==6,"For form undo changed mission results");
         snapshot=window.missionSnapshot();
         const auto condition=find(snapshot,"If");
         form.setStatement(snapshot.nodes[condition].statement);
         auto *predicate=form.findChild<QLineEdit *>("commandField_Condition");
         if (!predicate) std::cerr<<"Condition source: "<<snapshot.nodes[condition].statement.toStdString()<<std::endl;
         require(predicate,"Real mission If header has no condition control");
         QTimer::singleShot(0,&form,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
            auto *table=dialog->findChild<QTableWidget *>("conditionTable");
            table->item(0,1)->setText("count"); static_cast<QComboBox *>(table->cellWidget(0,2))->setCurrentText("~=");
            table->item(0,3)->setText("0"); dialog->accept();
         }); form.findChild<QPushButton *>("commandChoose_Condition")->click();
         require(window.applyMissionChange(snapshot,condition,MissionEdit::Replace,changed).isEmpty() && total()==-196,
            "Condition form edit did not execute the correct branch");
         editor->undo(); require(window.buildScript() && total()==6,"Condition form undo changed mission results");
      }
      snapshot=window.missionSnapshot();
      const int assignment=forAssignment(snapshot);
      const auto error=window.applyMissionChange(snapshot,assignment,MissionEdit::Replace,"total = total + 2;");
      if (!error.isEmpty()) std::cerr<<error.toStdString()<<'\n';
      require(error.isEmpty() && total()==9,"Repeated-command edit changed wrong branch");
      require(!window.applyMissionChange(snapshot,assignment,MissionEdit::Remove,{}).isEmpty(),"Stale mission editor was accepted");
      editor->undo(); require(editor->toPlainText()==original && window.buildScript() && total()==6,"Mission edit lost undo history");
      snapshot=window.missionSnapshot();
      require(!window.applyMissionChange(snapshot,forAssignment(snapshot),MissionEdit::Replace,"Propagate Missing(Spacecraft);").isEmpty(),"Invalid mission command accepted");
      require(editor->toPlainText()==snapshot.sourceScript && total()==6,"Invalid edit did not restore previous mission");
      require(window.applyMissionChange(snapshot,forAssignment(snapshot),MissionEdit::InsertBefore,"total = total + 10;").isEmpty(),"Corrected insertion after failure rejected");
      require(total()==36,"Inserted command did not execute inside selected loop");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(window.applyMissionChange(snapshot,find(snapshot,"If"),MissionEdit::Remove,{}).isEmpty() && total()==4,"Deleting a branch did not remove its whole block");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(window.applyMissionChange(snapshot,find(snapshot,"For"),MissionEdit::InsertAfter,"total = total + 10;").isEmpty() && total()==16,"Insert-after remained inside branch");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(window.applyMissionChange(snapshot,-1,MissionEdit::Append,"total = total + 10;").isEmpty() && total()==16,"Append mission command failed");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(window.applyMissionChange(snapshot,find(snapshot,"BeginScript"),MissionEdit::Replace,
         "BeginScript;\n   total = total + 5;\nEndScript;").isEmpty() && total()==10,
         "Script event replacement did not replace the complete block");
      require(window.loadScript(script) && window.buildScript(),"Could not restore test mission");
      snapshot=window.missionSnapshot();
      require(!window.applyMissionChange(snapshot,find(snapshot,"EndIf"),MissionEdit::Remove,{}).isEmpty(),"Structural end command could be deleted alone");
      auto *tree=window.findChild<QTreeWidget *>("Mission"); require(tree!=nullptr,"Mission tree missing");
      QTreeWidgetItem *target=nullptr;
      const int selected=forAssignment(snapshot);
      for (QTreeWidgetItemIterator it(tree);*it;++it)
         if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==selected) target=*it;
      require(target && target->parent()->data(0,Qt::UserRole+1).toString()=="For","Tree hierarchy wrong");
      window.findChild<QDockWidget *>("navigation")->findChild<QTabWidget *>()->setCurrentIndex(1);
      tree->expandAll(); tree->setCurrentItem(target); tree->itemDoubleClicked(target,0);
      CommandEditor *panel=nullptr;
      for (auto *widget:window.findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<CommandEditor *>(widget)) panel=candidate;
      require(panel!=nullptr,"Mission double click did not open editor");
      auto *commandSource=panel->findChild<QPlainTextEdit *>("commandSource");
      auto *commandWindow=qobject_cast<QMdiSubWindow *>(panel->parentWidget());
      require(commandWindow,"Command editor has no workspace window");
      window.findChild<QMdiArea *>("workspace")->setActiveSubWindow(commandWindow);
      window.activateWindow(); commandSource->setFocus(); QApplication::processEvents();
      require(QApplication::focusWidget()==commandSource,"Command source did not receive focus before clipboard routing");
      const auto scriptBeforeTextEdit=editor->toPlainText();
      const auto commandBeforeTextEdit=commandSource->toPlainText();
      commandSource->selectAll(); QApplication::clipboard()->setText("total = total + 4;");
      window.findChild<QAction *>("edit_paste")->trigger();
      require(commandSource->toPlainText()=="total = total + 4;" && editor->toPlainText()==scriptBeforeTextEdit,"Paste targeted the wrong document");
      QMenu *editMenu=nullptr;
      for (auto *menu:window.findChildren<QMenu *>()) if (menu->title()=="&Edit") editMenu=menu;
      require(editMenu!=nullptr,"Edit menu missing");
      editMenu->popup(window.mapToGlobal(QPoint(100,30))); QApplication::processEvents();
      editMenu->setActiveAction(window.findChild<QAction *>("edit_undo"));
      QKeyEvent activate(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
      QApplication::sendEvent(editMenu,&activate); QApplication::processEvents();
      require(commandSource->toPlainText()==commandBeforeTextEdit && editor->toPlainText()==scriptBeforeTextEdit,"Undo targeted the script instead of the command panel");
      window.findChild<QAction *>("edit_redo")->trigger();
      require(commandSource->toPlainText()=="total = total + 4;","Redo did not target command editor");
      tree->setFocus(); QApplication::processEvents();
      window.findChild<QAction *>("edit_undo")->trigger();
      require(editor->toPlainText()==scriptBeforeTextEdit && commandSource->toPlainText()=="total = total + 4;","Undo with tree focus changed an editor");
      require(panel->hasChanges() && !window.buildScript(),"Unapplied mission changes did not protect Build");
      if (!screenshot.isEmpty()) { QApplication::processEvents(); require(window.grab().save(screenshot),"Mission screenshot failed"); }
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      require(total()==15,"Mission panel Apply did not update the loop");
      {
         auto eventScript=original; eventScript.prepend("Create String Note;\n");
         eventScript.replace("BeginScript;\n   total = total + 1;\nEndScript;", "% original event comment\nBeginScript 'Keep event'; % begin inline\n   Note = 'EndScript; % literal';\n   % BeginScript; inside a comment\n   If total > 0;\n      total = total + 2;\n   Else;\n      total = -99;\n   EndIf;\n   BeginScript 'Nested event';\n      total = total + 3;\n   EndScript; % nested end\nEndScript; % end inline");
         editor->setPlainText(eventScript); require(window.buildScript() && total()==10,"Nested named script-event fixture rejected");
         const auto snapshot=window.missionSnapshot(); const int index=find(snapshot,"BeginScript"); const auto statement=snapshot.nodes[index].statement;
         if (!ScriptEventDialog::supports(statement)) throw std::runtime_error(("Actual named script event not recognized:\n"+statement).toStdString()); ScriptEventDialog unchanged(statement); require(unchanged.statement()==statement,"Opening script-event controls changed exact source");
         require(!ScriptEventDialog::supports("total = 1;\n") && !ScriptEventDialog::supports("BeginScript;\ntotal = 1;\n"),"Incomplete/non-event source accepted by structured controls");
         QWidget owner; QString error; CommandEditor event(statement,false,{},[&](const QString &text) { return error=window.applyMissionChange(snapshot,index,MissionEdit::Replace,text); },{},{},&owner);
         auto *button=event.findChild<QPushButton *>("editScriptEvent"); require(button && !button->isHidden(),"Script-event controls missing from command editor"); auto *source=event.findChild<QPlainTextEdit *>("commandSource");
         std::exception_ptr failure;
         auto edit=[&](bool accept,const QString &body,const QString &comments) {
            QTimer::singleShot(0,&event,[&] { try {
               auto *dialog=event.findChild<QDialog *>("scriptEventDialog"); require(dialog,"Script-event dialog missing");
               require(dialog->findChild<QLabel *>("scriptEventBegin")->text().contains("Keep event") && dialog->findChild<QLabel *>("scriptEventEnd")->text().contains("end inline"),"Script-event boundary labels/comments lost");
               dialog->findChild<QPlainTextEdit *>("scriptEventBody")->setPlainText(body); dialog->findChild<QPlainTextEdit *>("scriptEventComments")->setPlainText(comments);
               if (!screenshot.isEmpty() && comments.startsWith("updated")) require(dialog->grab().save(screenshot+".script-event.png"),"Script-event capture failed");
               dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
            } catch (...) { failure=std::current_exception(); if (auto *dialog=event.findChild<QDialog *>("scriptEventDialog")) dialog->reject(); } }); button->click(); if (failure) std::rethrow_exception(failure);
         };
         const auto oldBody=unchanged.findChild<QPlainTextEdit *>("scriptEventBody")->toPlainText();
         ScriptEventDialog commentsOnly(statement); commentsOnly.findChild<QPlainTextEdit *>("scriptEventComments")->setPlainText("comment-only update"); require(commentsOnly.statement().endsWith(statement.mid(statement.indexOf("BeginScript 'Keep event'"))),"Comment-only edit changed event commands or boundary text");
         edit(false,"total = -99;","cancelled"); require(source->toPlainText()==statement && !event.hasChanges(),"Script-event Cancel changed pending source");
         edit(true,"Propagate Missing(Spacecraft);","invalid command"); const auto before=editor->toPlainText(); event.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(!error.isEmpty() && editor->toPlainText()==before && total()==10 && event.hasChanges(),"Invalid event edit failed to preserve source/execution");
         edit(true,QString(oldBody).replace("total + 2","total + 4"),"updated event comment\nsecond comment");
         const auto pending=source->toPlainText(); require(pending.contains("% updated event comment\n% second comment") && pending.contains("% begin inline") && pending.contains("% nested end") && pending.contains("% end inline") && pending.contains("EndScript; % literal") && total()==10,"Pending event edit lost boundaries/comments or changed mission");
         source->undo(); require(source->toPlainText().contains("Propagate Missing"),"Structured event changes are not one undo step"); source->redo(); require(source->toPlainText()==pending,"Event redo changed pending source");
         event.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(error.isEmpty() && total()==12,"Corrected script event did not execute through GUI Apply"); const auto applied=editor->toPlainText();
         editor->undo(); require(editor->toPlainText()==before && window.buildScript() && total()==10,"Script-event Undo did not restore exact source/results"); editor->redo(); require(editor->toPlainText()==applied && window.buildScript() && total()==12,"Script-event Redo changed source/results");
         auto &events=fixtureDirectory(); require(events.isValid(),"Event round-trip directory failed"); const auto path=events.filePath("script event ü.script"); require(window.saveScriptTo(path) && window.loadScript(path) && total()==12,"Script-event Unicode save/reopen changed execution");
         const auto emptySnapshot=window.missionSnapshot(); const int emptyIndex=find(emptySnapshot,"BeginScript"); ScriptEventDialog empty(emptySnapshot.nodes[emptyIndex].statement); empty.findChild<QPlainTextEdit *>("scriptEventBody")->clear();
         require(window.applyMissionChange(emptySnapshot,emptyIndex,MissionEdit::Replace,empty.statement()).isEmpty() && total()==5,"Empty script-event body failed or retained old commands");
         require(window.loadScript(script) && window.buildScript(),"Script-event test restoration failed");
      }
      editor->setPlainText("Create DifferentialCorrector DC;\nGMAT DC.MaximumIterations = 20;\n"
         "Create Variable x goalValue;\nCreate String Note;\nCreate Spacecraft FilterSat;\nCreate Array Choice[2,3];\nGMAT x = 1;\nGMAT goalValue = 8;\nBeginMissionSequence;\n"
         "Target DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = true};\n"
         "Vary DC(goalValue = 1, {Perturbation = 0.001, Lower = -20, Upper = 20, MaxStep = 10});\n"
         "Achieve DC(x = 7, {Tolerance = 0.000001});\nEndTarget;\n");
      require(window.buildScript(),"Targeting fixture did not build");
      auto &solverOutput=fixtureDirectory(); require(solverOutput.isValid(),"Solver report directory failed");
      const auto solverReport=solverOutput.filePath("target report.txt");
      {
         const auto before=editor->toPlainText(); QString error; QWidget owner;
         ResourceEditor settings(*Moderator::Instance()->GetConfiguredObject("DC"),[&](const auto &changes) {
            error=window.applyResourceChanges("DC",changes,before); return error;
         },&owner);
         auto *grid=settings.findChild<QTableWidget *>();
         auto row=[&](const QString &name) { for (int i=0;i<grid->rowCount();++i) if (grid->item(i,0)->text()==name) return i; throw std::runtime_error("Missing DC setting"); };
         auto combo=[&](const QString &name) { auto *value=qobject_cast<QComboBox *>(grid->cellWidget(row(name),1)); require(value,"DC dropdown missing"); return value; };
         require(combo("Algorithm")->count()==3 && combo("DerivativeMethod")->count()==3 && combo("ReportStyle")->count()==4,"DC choices do not match wx");
         combo("Algorithm")->setCurrentText("Broyden"); combo("DerivativeMethod")->setCurrentText("CentralDifference");
         combo("ReportStyle")->setCurrentText("Verbose"); combo("ShowProgress")->setCurrentText("true");
         grid->item(row("MaximumIterations"),1)->setText("30");
         auto *reportItem=grid->item(row("ReportFile"),1); const auto originalReport=reportItem->text();
         QTimer::singleShot(0,&settings,[&] {
            auto *dialog=settings.findChild<QFileDialog *>("resourceFileDialog"); require(dialog && dialog->acceptMode()==QFileDialog::AcceptSave,"Solver report chooser is not an output picker");
            dialog->selectFile(solverReport); dialog->reject();
         }); settings.findChild<QPushButton *>("chooseProperty_ReportFile")->click();
         require(reportItem->text()==originalReport,"Cancelled solver report changed path");
         reportItem->setText(solverReport);
         require(Moderator::Instance()->GetConfiguredObject("DC")->GetStringParameter("Algorithm")=="NewtonRaphson","Pending solver settings changed engine");
         settings.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         require(error.isEmpty() && Moderator::Instance()->GetConfiguredObject("DC")->GetStringParameter("Algorithm")=="Broyden","DC GUI settings failed to apply");
      }
      {
         ReportParameterDialog numeric({"x"},&window,ReportParameterDialog::Mode::WritableReal);
         auto *entry=numeric.findChild<QComboBox *>("reportParameterEntry");
         auto *ok=numeric.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
         require(entry->findText("x")>=0 && entry->findText("Choice")>=0 && entry->findText("Note")<0,"Vary picker offers nonnumeric user values");
         auto *owner=numeric.findChild<QComboBox *>("reportPropertyObject"); owner->setCurrentText("FilterSat");
         auto *properties=numeric.findChild<QComboBox *>("reportPropertyType");
         require(properties->findText("X")>=0 && properties->findText("ElapsedSecs")<0,"Writable property browser includes read-only elapsed time");
         entry->setEditText("FilterSat.ElapsedSecs"); require(!ok->isEnabled(),"Read-only typed parameter was accepted");
         entry->setEditText("42"); require(!ok->isEnabled(),"Numeric literal accepted as writable destination");
         ReportParameterDialog destination({"Choice"},&window,ReportParameterDialog::Mode::Writable);
         require(destination.findChild<QComboBox *>("reportParameterEntry")->findText("Note")>=0 &&
            destination.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Assignment picker excluded string or whole array");
         const auto current=window.missionSnapshot(); QString varied;
         CommandForm varyForm([&](const QString &value) { varied=value; }); varyForm.setStatement(current.nodes[find(current,"Vary")].statement);
         bool selected=false;
         QTimer::singleShot(0,&varyForm,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
            auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry"); selected=entry->findText("x")>=0;
            entry->setCurrentText("x"); dialog->accept();
         }); varyForm.findChild<QPushButton *>("commandChoose_Variable")->click();
         require(selected && window.applyMissionChange(current,find(current,"Vary"),MissionEdit::Replace,varied).isEmpty(),"Selected Vary variable did not apply");
      }

      {
         const auto current=window.missionSnapshot(); QString changed;
         CommandForm optionsForm([&](const QString &value) { changed=value; }); optionsForm.setStatement("Vary DC(x = 1);");
         optionsForm.findChild<QPushButton *>("commandAddOptions")->click();
         optionsForm.findChild<QLineEdit *>("commandField_Lower")->setText("-20");
         optionsForm.findChild<QLineEdit *>("commandField_Upper")->setText("20");
         optionsForm.findChild<QLineEdit *>("commandField_MaxStep")->setText("2");
         require(window.applyMissionChange(current,find(current,"Vary"),MissionEdit::Replace,changed).isEmpty(),"Added Vary options did not apply");
      }
      snapshot=window.missionSnapshot();
      const int achieve=find(snapshot,"Achieve");
      QString replacement;
      CommandForm goalForm([&](const QString &text) { replacement=text; });
      goalForm.setStatement("Achieve DC(x = 7);");
      goalForm.findChild<QPushButton *>("commandAddOptions")->click();
      goalForm.findChild<QLineEdit *>("commandField_Tolerance")->setText("0.000001");
      auto *goal=goalForm.findChild<QLineEdit *>("commandField_Value");
      require(goal,"Engine-generated Achieve command has no form");
      {
         ReportParameterDialog selector({"x"},&window,ReportParameterDialog::Mode::Single);
         auto *entry=selector.findChild<QComboBox *>("reportParameterEntry");
         auto *ok=selector.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
         require(selector.selection()==QStringList{"x"} && selector.findChild<QWidget *>("reportSelectedParameters")->isHidden(),"Single chooser displayed a report list or lost initial value");
         entry->setEditText(""); require(!ok->isEnabled(),"Empty single selection enabled OK");
         entry->setEditText("Choice"); require(!ok->isEnabled(),"Bare array accepted as a scalar operand");
         selector.findChild<QSpinBox *>("reportArrayRow")->setValue(2);
         selector.findChild<QSpinBox *>("reportArrayColumn")->setValue(3);
         selector.findChild<QPushButton *>("reportAddElement")->click();
         require(ok->isEnabled() && selector.selection()==QStringList{"Choice(2,3)"},"Single array-element selection failed");
      }
      bool picked=false;
      QTimer::singleShot(0,&goalForm,[&] {
         auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
         auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry");
         picked=entry && entry->findText("goalValue")>=0; if (entry) entry->setCurrentText("goalValue"); dialog->accept();
      }); goalForm.findChild<QPushButton *>("commandChoose_Value")->click();
      require(picked && goal->text()=="goalValue","Achieve target parameter was not selected");
      QTimer::singleShot(0,&goalForm,[&] {
         auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
         dialog->findChild<QComboBox *>("reportParameterEntry")->setCurrentText("x"); dialog->reject();
      }); goalForm.findChild<QPushButton *>("commandChoose_Value")->click();
      require(goal->text()=="goalValue","Cancel changed the selected target");
      require(window.applyMissionChange(snapshot,achieve,MissionEdit::Replace,replacement).isEmpty(),"Achieve form edit rejected");
      {
         const auto before=editor->toPlainText(); const auto current=window.missionSnapshot(); QString changed;
         CommandForm modeForm([&](const QString &value) { changed=value; }); modeForm.setStatement(current.nodes[find(current,"Target")].statement);
         modeForm.findChild<QComboBox *>("commandChoice_SolveMode")->setCurrentText("RunInitialGuess");
         require(window.applyMissionChange(current,find(current,"Target"),MissionEdit::Replace,changed).isEmpty() &&
            window.runMission()==MainWindow::RunResult::Completed,"Selected initial-guess mode failed");
         require(std::abs(Moderator::Instance()->GetInternalObject("x")->GetRealParameter("Value")-1)<1e-12,"Initial-guess mode unexpectedly solved the target");
         editor->undo(); require(editor->toPlainText()==before && window.buildScript(),"Solver mode Undo failed");
      }
      auto &selectedGoal=fixtureDirectory();
      require(selectedGoal.isValid() && window.saveScriptTo(selectedGoal.filePath("target.script")) &&
         window.loadScript(selectedGoal.filePath("target.script")) && window.buildScript(),"Selected target save/reopen failed");
      require(window.runMission()==MainWindow::RunResult::Completed,"Targeting did not execute");
      auto solved=[&] { return Moderator::Instance()->GetInternalObject("x")->GetRealParameter("Value"); };
      require(std::abs(solved()-8)<1e-6,"Targeting did not achieve edited goal");
      QFile solverReportFile(solverReport);
      require(solverReportFile.open(QIODevice::ReadOnly) && !solverReportFile.readAll().trimmed().isEmpty(),"GUI-configured solver report was not written after reopen");
      require(window.findChildren<QTableWidget *>("solverProgress").size()==1,"Edited solver mode left a stale progress window");
      auto *progress=window.findChild<QTableWidget *>("solverProgress");
      auto *solverStatus=window.findChild<QLabel *>("solverStatus");
      require(progress && progress->rowCount()>=2 && solverStatus && solverStatus->property("converged").toBool(),
         "Solver progress or convergence feedback missing");
      bool goalSeen=false;
      for (int row=0;row<progress->rowCount();++row) if (progress->item(row,0)->text()=="Goal =")
         goalSeen=progress->item(row,3)->text().toDouble()==8 && std::abs(progress->item(row,4)->text().toDouble())<1e-6;
      require(goalSeen,"Solver table did not show final goal/residual");
      for (int column=0;column<progress->columnCount();++column)
         require(progress->horizontalHeader()->sectionResizeMode(column)==QHeaderView::Interactive,"Solver column cannot be dragged");
      progress->setColumnWidth(1,301); progress->setColumnWidth(4,177);
      require(window.runMission()==MainWindow::RunResult::Completed,"Solver resize repeat mission failed");
      require(progress->columnWidth(1)==301 && progress->columnWidth(4)==177,"Live solver updates overwrote adjusted widths");
      auto *progressWindow=qobject_cast<QMdiSubWindow *>(progress->parentWidget()->parentWidget());
      require(progressWindow,"Solver progress is not an MDI window"); progressWindow->close();
      QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(window.runMission()==MainWindow::RunResult::Completed && std::abs(solved()-8)<1e-6,
         "Closing solver progress broke repeated execution");
      require(window.findChild<QTableWidget *>("solverProgress"),"Rerun did not recreate closed solver progress");
      {
         const QString original="% original compact source α\nCreate Variable i total; % grouped declarations\ntotal = 0;\nBeginMissionSequence\n% keep loop comment\nFor 'Implicit step' i = 1:2 % keep header\n   total = total + i % keep body\nEndFor\n";
         editor->setPlainText(original); require(window.runMission()==MainWindow::RunResult::Completed && Moderator::Instance()->GetInternalObject("total")->GetRealParameter("Value")==3,"Implicit unit-step loop reference failed");
         const auto snapshot=window.missionSnapshot(); const int index=find(snapshot,"For"); QString changed; CommandForm form([&](const QString &value) { changed=value; }); form.setStatement(snapshot.nodes[index].statement);
         auto *end=form.findChild<QLineEdit *>("commandField_End"); require(end && !form.findChild<QLineEdit *>("commandField_Step"),"Original two-bound semicolon-free loop was not retained in controls"); end->setText("3");
         auto expected=original; expected.replace("i = 1:2","i = 1:3"); require(window.applyMissionChange(snapshot,index,MissionEdit::Replace,changed).isEmpty() && editor->toPlainText()==expected && window.runMission()==MainWindow::RunResult::Completed && Moderator::Instance()->GetInternalObject("total")->GetRealParameter("Value")==6,"Implicit-step source edit changed comments/configuration or execution");
         editor->undo(); require(editor->toPlainText()==original,"Implicit-step Undo not exact"); editor->redo(); require(editor->toPlainText()==expected,"Implicit-step Redo not exact"); auto &files=fixtureDirectory(); require(window.saveScriptTo(files.filePath("implicit loop ü.script")) && window.loadScript(files.filePath("implicit loop ü.script")) && window.runMission()==MainWindow::RunResult::Completed && Moderator::Instance()->GetInternalObject("total")->GetRealParameter("Value")==6,"Implicit-step loop save/reopen failed");
         editor->setPlainText(expected); require(window.runMission()==MainWindow::RunResult::Completed && Moderator::Instance()->GetInternalObject("total")->GetRealParameter("Value")==6,"Independent implicit-step edited source differs");
      }
      std::cout<<"PASS: nested branches, duplicate-command identity, replace/insert/delete/append, script-event comments/body/nested labels and quoted literals, pending/Cancel/empty body/Unicode round trips, invalid-edit rollback, stale panel, undo, Mission tree and Apply\n";
   } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
