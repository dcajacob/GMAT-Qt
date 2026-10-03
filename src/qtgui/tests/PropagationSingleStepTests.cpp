#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "PropagationForm.hpp"
#include "PropagationStopsDialog.hpp"
#include "PropagationColorDialog.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFile>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTabWidget>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QTextDocument>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   try {
      require(argc==2,"Expected a startup path"); TestSettings isolated;
      const QString multiple="Propagate 'Groups' BackProp Synchronized A(Sat) B(Other) {Sat.ElapsedSecs = -60, Other.Earth.Periapsis, StopTolerance = 1e-8, OrbitColor = Blue}; % preserve groups";
      PropagationStopsDialog groups(multiple); auto *table=groups.findChild<QTableWidget *>("propagationStopsTable");
      require(groups.statement()==multiple,"Untouched stopping conditions changed source");
      while (table->rowCount()) { table->selectRow(0); groups.findChild<QPushButton *>("propagationStopRemove")->click(); }
      require(groups.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Last stopping condition removal is still blocked");
      const auto stepGroups=groups.statement(); require(stepGroups=="Propagate 'Groups' BackProp Synchronized A(Sat) B(Other) {StopTolerance = 1e-8, OrbitColor = Blue}; % preserve groups","Single-step removal lost header/options/comment");
      PropagationStopsDialog restored(stepGroups); require(restored.statement()==stepGroups,"Untouched no-stop options changed source");
      restored.findChild<QPushButton *>("propagationStopAdd")->click(); auto *newRows=restored.findChild<QTableWidget *>("propagationStopsTable");
      require(!restored.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Blank new stopping condition accepted");
      newRows->item(0,0)->setText("Sat.ElapsedSecs"); newRows->item(0,1)->setText("1200");
      require(restored.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled() && restored.statement().contains("Sat.ElapsedSecs = 1200, StopTolerance = 1e-8, OrbitColor = Blue"),"Single-step command cannot acquire a stopping condition");
      const QString bare="Propagate 'Step' A(Sat); % keep bare";
      require(PropagationStopsDialog::supports(bare),"No-stop command lacks stopping-condition editor");
      PropagationStopsDialog bareStops(bare); require(bareStops.statement()==bare,"No-stop unchanged source lost bytes");
      bareStops.findChild<QPushButton *>("propagationStopAdd")->click(); auto *bareRows=bareStops.findChild<QTableWidget *>("propagationStopsTable");
      bareRows->item(0,0)->setText("Sat.Earth.Apoapsis");
      require(bareStops.statement()=="Propagate 'Step' A(Sat) {Sat.Earth.Apoapsis}; % keep bare","Adding event stop lost bare command header/comment");
      PropagationStopsDialog removed("Propagate 'Step' A(Sat) {Sat.ElapsedSecs = 60}; % keep bare");
      removed.findChild<QPushButton *>("propagationStopRemove")->click(); require(removed.statement()==bare,"Last stop left empty braces instead of valid single-step syntax");
      require(!PropagationStopsDialog::supports("Propagate A(Sat) {};"),"Malformed empty braces accepted");

      QString pending; PropagationForm form({"A","B"},{"Sat","Other"},[&](const QString &value) { pending=value; }); form.setStatement(bare);
      require(!form.isHidden() && form.findChild<QCheckBox *>("propagationSingleStep")->isChecked(),"No-stop command hides Single step form");
      require(!form.findChild<QLineEdit *>("propagationDuration")->isEnabled() && !form.findChild<QLineEdit *>("propagationStopParameter")->isEnabled() && !form.findChild<QLineEdit *>("propagationTolerance")->isEnabled(),"Single step leaves inactive stopping controls enabled");
      form.findChild<QComboBox *>("propagationPropagator")->setCurrentText("B"); form.findChild<QComboBox *>("propagationSpacecraft")->setCurrentText("Other");
      require(pending=="Propagate 'Step' B(Other); % keep bare","Single-step group edits added a stop or lost source");
      form.findChild<QCheckBox *>("propagationSingleStep")->setChecked(false); require(pending=="Propagate 'Step' B(Other) {Other.ElapsedSecs = 60}; % keep bare","Turning Single step off cannot add a default stop");
      form.setStatement("Propagate 'Apsis' BackProp A(Sat) {Sat.ElapsedSecs = 120, StopTolerance = 1e-8, OrbitColor = Blue}; % retain apsis");
      form.findChild<QLineEdit *>("propagationStopParameter")->setText("Sat.Earth.Periapsis");
      require(pending=="Propagate 'Apsis' BackProp A(Sat) {Sat.Earth.Periapsis, StopTolerance = 1e-8, OrbitColor = Blue}; % retain apsis" && !form.findChild<QLineEdit *>("propagationDuration")->isEnabled(),"Event stop conversion lost options/header or left its goal active");
      form.findChild<QCheckBox *>("propagationSingleStep")->setChecked(true);
      require(pending=="Propagate 'Apsis' BackProp A(Sat) {StopTolerance = 1e-8, OrbitColor = Blue}; % retain apsis","Event single-step toggle lost options/header");
      form.findChild<QCheckBox *>("propagationSingleStep")->setChecked(false); form.findChild<QLineEdit *>("propagationStopParameter")->setText("Sat.ElapsedDays"); form.findChild<QLineEdit *>("propagationDuration")->setText("0.1");
      require(pending=="Propagate 'Apsis' BackProp A(Sat) {Sat.ElapsedDays = 0.1, StopTolerance = 1e-8, OrbitColor = Blue}; % retain apsis","Ordinary time stop editing regressed after event/single-step toggle");
      const QString safeColor="OrbitColor = Blue";
      const QString generatedColor="Propagate 'Generated' A(Sat){, OrbitColor = Blue}; % keep generated";
      PropagationStopsDialog generatedStops(generatedColor); require(PropagationStopsDialog::supports(generatedColor) && generatedStops.statement()==generatedColor,"Engine-generated single-step color lost support/exact source");
      form.setStatement(generatedColor); require(!form.isHidden() && form.findChild<QCheckBox *>("propagationSingleStep")->isChecked(),"Engine-generated single-step color hides mode controls");
      require(!PropagationStopsDialog::supports("Propagate A(Sat) {, Sat.ElapsedSecs = 60};"),"Unclassified leading-comma stop unexpectedly accepted");
      form.setStatement(multiple); require(form.isHidden(),"Simple form incorrectly simplified multiple propagation groups");

      QTemporaryDir files; require(files.isValid(),"Temporary files unavailable"); const auto output=files.filePath("step results.txt");
      const QString source="% source retained Δ\nCreate Spacecraft Sat;\nCreate ForceModel FM;\nFM.CentralBody = Earth;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.ErrorControl = None;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.Type = RungeKutta89;\nProp.InitialStepSize = 600;\nProp.MaxStep = 600;\nCreate ReportFile Values;\nValues.Filename = '"+output+"';\nValues.WriteHeaders = false;\nValues.WriteReport = false;\nBeginMissionSequence;\nPropagate 'Stage' Prop(Sat) {Sat.ElapsedSecs = 1200, StopTolerance = 1e-9, OrbitColor = Blue}; % retain stage\nReport Values Sat.ElapsedSecs;\n";
      MainWindow window; window.show(); require(window.initialize(QString::fromLocal8Bit(argv[1])),"Initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); editor->setPlainText(source); require(window.buildScript(),"Fixed-step fixture build failed");
      int opens=0;
      const auto open=[&] {
         ++opens; auto *tabs=window.findChild<QTabWidget *>(); for (int i=0;tabs && i<tabs->count();++i) if (tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
         const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("'Stage'")) index=i;
         require(index>=0,"Stage command absent"); auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator it(tree);
         while (*it && (!(*it)->data(0,Qt::UserRole).isValid() || (*it)->data(0,Qt::UserRole).toInt()!=index)) ++it;
         require(*it,"Stage tree row absent"); tree->itemDoubleClicked(*it,0);
         auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); auto *panel=child ? dynamic_cast<CommandEditor *>(child->widget()) : nullptr;
         if (!panel) std::cerr<<"Open attempt "<<opens<<", node editable="<<snapshot.nodes[index].editable<<", mapped start="<<snapshot.nodes[index].start<<", command="<<snapshot.nodes[index].statement.toStdString()<<"\nCanonical mission:\n"<<snapshot.canonicalScript.mid(snapshot.canonicalScript.indexOf("BeginMissionSequence")).toStdString()<<"\nAuthored mission:\n"<<editor->toPlainText().mid(editor->toPlainText().indexOf("BeginMissionSequence")).toStdString()<<'\n';
         require(panel,"Actual command editor absent"); return panel;
      };
      const auto stops=[&](CommandEditor &panel,bool accept) {
         std::exception_ptr failure;
         QTimer::singleShot(0,&panel,[&] {
            auto *dialog=dynamic_cast<PropagationStopsDialog *>(panel.findChild<QDialog *>("propagationStopsDialog"));
            try {
               require(dialog,"Stopping dialog absent"); auto *rows=dialog->findChild<QTableWidget *>("propagationStopsTable");
               while (rows->rowCount()) { rows->selectRow(0); dialog->findChild<QPushButton *>("propagationStopRemove")->click(); }
               require(dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Actual no-stop dialog blocks OK");
               if (accept) dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); else dialog->reject();
            } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
         });
         panel.findChild<QPushButton *>("editPropagationStops")->click(); if (failure) std::rethrow_exception(failure);
      };
      auto *panel=open(); QApplication::processEvents();
      auto *typedScroll=panel->findChild<QScrollArea *>("commandTypedControls");
      auto *stopField=panel->findChild<QLineEdit *>("propagationStopParameter");
      auto *toleranceField=panel->findChild<QLineEdit *>("propagationTolerance");
      require(typedScroll && typedScroll->verticalScrollBar()->maximum()>0,"Default-size propagation controls do not scroll");
      for (auto *field:{stopField,toleranceField}) {
         typedScroll->ensureWidgetVisible(field); QApplication::processEvents();
         require(field->height()>=field->minimumSizeHint().height() && typedScroll->viewport()->rect().intersects(QRect(field->mapTo(typedScroll->viewport(),QPoint()),field->size())),"Default-size propagation field is compressed or unreachable");
      }
      require(panel->findChild<QPlainTextEdit *>("commandSource")->height()>=90 && panel->findChild<QDialogButtonBox *>()->isVisible(),"Typed control scrolling hides source or Apply");
      auto *command=panel->findChild<QPlainTextEdit *>("commandSource"); const auto originalCommand=command->toPlainText();
      stops(*panel,false); require(command->toPlainText()==originalCommand && editor->toPlainText()==source && !panel->hasChanges(),"Stopping-condition Cancel changed pending/mission source");
      stops(*panel,true); const auto single=command->toPlainText(); require(single.contains("{StopTolerance = 1e-9, OrbitColor = Blue}") && panel->hasChanges() && editor->toPlainText()==source,"No-stop OK lost options or prematurely committed");
      require(panel->findChild<QCheckBox *>("propagationSingleStep")->isChecked() && !panel->findChild<QWidget *>("propagationForm")->isHidden(),"No-stop pending form did not show its mode");
      command->undo(); require(command->toPlainText()==originalCommand,"No-stop pending change was not one Undo step");
      panel->findChild<QCheckBox *>("propagationSingleStep")->setChecked(true); require(command->toPlainText()==single,"Single step checkbox disagrees with removing the last stop");
      panel->findChild<QCheckBox *>("propagationSingleStep")->setChecked(false); require(command->toPlainText()==originalCommand,"Single step toggle lost pending stop values/tolerance");
      panel->findChild<QCheckBox *>("propagationSingleStep")->setChecked(true);
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      const auto savedSource=editor->toPlainText(); require(!panel->hasChanges() && savedSource==QString(source).replace(originalCommand,single),"Single-step Apply changed unrelated source or failed");
      for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("configurationPanel").toBool()) child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      editor->undo(); require(editor->toPlainText()==source && window.buildScript(),"Single-step Apply Undo lost exact source/model");
      editor->redo(); require(editor->toPlainText()==savedSource && window.buildScript(),"Single-step Apply Redo lost exact source/model");
      const auto saved=files.filePath("single step Ω.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && editor->toPlainText()==savedSource && window.buildScript(),"Single-step Save/reopen changed exact source");
      const auto run=[&](double expected) {
         const auto result=window.runMission(); QApplication::processEvents(); require(result==MainWindow::RunResult::Completed,qPrintable("Single-step run failed:\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()));
         QFile report(output); require(report.open(QIODevice::ReadOnly),"Single-step report absent"); const auto values=QString::fromUtf8(report.readAll()).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
         require(values.size()==1 && std::abs(values[0].toDouble()-expected)<1e-6,"Single-step elapsed time differs from independently known fixed step");
      };
      run(600); panel=open(); require(panel->findChild<QCheckBox *>("propagationSingleStep")->isChecked(),"Reopened command lost Single step mode");
      panel->findChild<QCheckBox *>("propagationBackwards")->setChecked(true); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      require(!panel->hasChanges() && editor->toPlainText().contains("Propagate 'Stage' BackProp Prop(Sat) {StopTolerance = 1e-9, OrbitColor = Blue}; % retain stage"),"Backward single-step Apply lost header/options/comment"); run(-600);
      const auto closePanels=[&] {
         for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("configurationPanel").toBool()) child->close();
         QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      };
      const QString noToleranceStop="Propagate 'Stage' Prop(Sat) {Sat.ElapsedSecs = 1200, OrbitColor = Blue}; % retain stage";
      const QString safeStep="Propagate 'Stage' Prop(Sat) {"+safeColor+"}; % retain stage";
      for (const bool checkbox:{false,true}) {
         closePanels(); const auto stopped=QString(savedSource).replace(single,noToleranceStop); editor->setPlainText(stopped); require(window.buildScript(),"Color/no-explicit-tolerance stopped fixture failed to build"); panel=open();
         if (checkbox) panel->findChild<QCheckBox *>("propagationSingleStep")->setChecked(true); else stops(*panel,true);
         require(panel->findChild<QPlainTextEdit *>("commandSource")->toPlainText()==safeStep && editor->toPlainText()==stopped,"Single-step color safeguard lost pending or exact unrelated source");
         panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         const auto exact=QString(stopped).replace(noToleranceStop,safeStep); require(!panel->hasChanges() && editor->toPlainText()==exact,"Colored SingleStep without explicit tolerance failed Apply");
         closePanels(); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==exact,"Colored SingleStep exact Save/reopen failed"); panel=open(); require(panel->findChild<QCheckBox *>("propagationSingleStep")->isChecked(),"Safeguarded color SingleStep lost actual reopened mode"); run(600);
      }
      closePanels(); const QString bareStage="Propagate 'Stage' Prop(Sat); % retain stage";
      const auto bareSource=QString(savedSource).replace(single,bareStage); editor->setPlainText(bareSource); require(window.buildScript(),"Bare SingleStep color fixture failed to build"); panel=open();
      std::exception_ptr colorFailure; QTimer::singleShot(0,panel,[&] {
         auto *dialog=dynamic_cast<PropagationColorDialog *>(panel->findChild<QDialog *>("propagationColorDialog"));
         try {
            require(dialog,"Actual bare SingleStep segment-color dialog absent"); dialog->findChild<QCheckBox *>("propagationColorOverride")->setChecked(true); dialog->findChild<QLineEdit *>("propagationColorValue")->setText("Blue"); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
         } catch (...) { colorFailure=std::current_exception(); if (dialog) dialog->reject(); }
      }); panel->findChild<QPushButton *>("editPropagationColor")->click(); if (colorFailure) std::rethrow_exception(colorFailure);
      require(panel->findChild<QPlainTextEdit *>("commandSource")->toPlainText()==safeStep && editor->toPlainText()==bareSource,"Bare SingleStep segment color did not produce valid pending syntax"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      const auto colorSource=QString(bareSource).replace(bareStage,safeStep); require(!panel->hasChanges() && editor->toPlainText()==colorSource,"Bare SingleStep segment color failed Apply"); closePanels(); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==colorSource,"Bare colored SingleStep exact source round trip failed"); run(600);
      closePanels(); const QString toleranceStop="Propagate 'Stage' Prop(Sat) {Sat.ElapsedSecs = 1200, StopTolerance = 1e-9}; % retain stage";
      const auto stoppedTolerance=QString(savedSource).replace(single,toleranceStop); editor->setPlainText(stoppedTolerance); require(window.buildScript(),"Stopped tolerance-only fixture failed to build"); panel=open(); panel->findChild<QCheckBox *>("propagationSingleStep")->setChecked(true);
      require(panel->findChild<QPlainTextEdit *>("commandSource")->toPlainText()==bareStage,"SingleStep retained invalid inactive-tolerance-only braces"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); const auto bareExact=QString(stoppedTolerance).replace(toleranceStop,bareStage); require(!panel->hasChanges() && editor->toPlainText()==bareExact,"Tolerance-only stop removal failed exact Apply"); closePanels(); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==bareExact,"Tolerance-only stop removal failed exact Save/reopen"); panel=open(); require(panel->findChild<QCheckBox *>("propagationSingleStep")->isChecked(),"Bare SingleStep lost reopened mode"); run(600);
      closePanels(); editor->setPlainText(savedSource); require(window.buildScript(),"Colored inactive-tolerance fixture failed to build"); panel=open(); colorFailure={}; QTimer::singleShot(0,panel,[&] {
         auto *dialog=dynamic_cast<PropagationColorDialog *>(panel->findChild<QDialog *>("propagationColorDialog"));
         try { require(dialog,"Actual colored SingleStep dialog absent"); dialog->findChild<QCheckBox *>("propagationColorOverride")->setChecked(false); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); }
         catch (...) { colorFailure=std::current_exception(); if (dialog) dialog->reject(); }
      }); panel->findChild<QPushButton *>("editPropagationColor")->click(); if (colorFailure) std::rethrow_exception(colorFailure);
      const QString clearedStep="Propagate 'Stage' Prop(Sat) ; % retain stage"; require(panel->findChild<QPlainTextEdit *>("commandSource")->toPlainText()==clearedStep,"Clearing SingleStep color left invalid tolerance-only braces"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); const auto clearedSource=QString(savedSource).replace(single,clearedStep); require(!panel->hasChanges() && editor->toPlainText()==clearedSource,"Clearing SingleStep color failed exact Apply"); closePanels(); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==clearedSource,"Cleared SingleStep color failed exact Save/reopen"); run(600);
      std::cout<<"PASS actual Single step and last-stop removal, multiple-group/options/comment retention, no-stop add/event controls, Cancel/pending one-Undo/Apply, exact mission Undo/Redo and Unicode Save/reopen, independently known fixed 600 s forward and -600 s backward results, valid GUI-authored color-only SingleStep through checkbox/last-stop/color controls, removal of inactive tolerance-only braces and clearing color, exact source round trips and authored colored inactive tolerance mapping/reopening, default-size scrollable controls with source/Apply reachable. No native host acceptance.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
