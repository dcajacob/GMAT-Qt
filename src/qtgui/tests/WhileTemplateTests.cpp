#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFile>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   try {
      require(argc==2,"Expected a startup path"); TestSettings isolated;
      QTemporaryDir files; require(files.isValid(),"Temporary folder unavailable");
      const auto output=files.filePath("while result.txt");
      const QString prefix="% Keep unrelated source Δ\nCreate Spacecraft Sat;\nCreate ForceModel FM;\nFM.CentralBody = Earth;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.ErrorControl = None;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.Type = RungeKutta89;\nProp.InitialStepSize = 600;\nProp.MaxStep = 600;\nCreate ReportFile Values;\nValues.Filename = '"+output+"';\nValues.WriteHeaders = false;\nValues.WriteReport = false;\nBeginMissionSequence;\nPropagate 'First' Prop(Sat); % keep first step\n";
      const QString suffix="Report Values Sat.ElapsedSecs; % keep report\n",baseline=prefix+suffix;
      MainWindow window; window.show(); require(window.initialize(QString::fromLocal8Bit(argv[1])),"Initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); editor->setPlainText(baseline); require(window.buildScript(),"Known-step fixture failed to build");
      auto *tree=window.findChild<QTreeWidget *>("Mission"); auto *workspace=window.findChild<QMdiArea *>("workspace"); require(tree && workspace,"Mission UI unavailable");
      auto *tabs=window.findChild<QTabWidget *>(); for (int i=0;tabs && i<tabs->count();++i) if (tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
      const auto node=[&](const QString &type) { const auto &snapshot=window.missionSnapshot(); for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type==type) return i; throw std::runtime_error("Expected mission node absent"); };
      const auto open=[&](int index,bool inside) {
         QTreeWidgetItem *item=nullptr;
         for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==index) item=*it;
         require(item,"Insertion tree anchor absent"); tree->expandAll(); tree->scrollToItem(item); QApplication::processEvents();
         std::exception_ptr failure; bool visited=false;
         QTimer::singleShot(0,&window,[&] {
            auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
            try {
               require(menu,"Actual insertion menu absent"); QAction *choice=nullptr;
               for (auto *action:menu->actions()) if (inside ? action->objectName()=="appendInsideMissionBranch" : action->text()=="Insert after…") choice=action;
               require(choice && choice->isEnabled(),"Insertion action unavailable"); visited=true; menu->setActiveAction(choice);
               QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter);
            } catch (...) { failure=std::current_exception(); if (menu) menu->close(); }
         });
         tree->customContextMenuRequested(tree->visualItemRect(item).center()); if (failure) std::rethrow_exception(failure);
         require(visited,"Actual insertion action was not exercised"); auto *child=workspace->activeSubWindow();
         require(child && dynamic_cast<CommandEditor *>(child->widget()),"Actual insertion editor absent"); return child;
      };
      const auto select=[](QMdiSubWindow *child,const QString &type) {
         auto *choices=child->widget()->findChild<QComboBox *>("commandTemplate"); require(choices && choices->findText(type)>=0,"Command template absent");
         choices->setCurrentText(type); choices->textActivated(type);
      };
      const auto apply=[](QMdiSubWindow *child) { auto *buttons=child->widget()->findChild<QDialogButtonBox *>(); require(buttons,"Command buttons absent"); buttons->button(QDialogButtonBox::Apply)->click(); };
      // While::Execute resets a local clone of elapsed time on branch entry.
      // After the outside 600 s step, local time starts at zero. A 300 s
      // boundary requires one 600 s body step, hence 1200 s mission elapsed;
      // it avoids comparing either local state with an exact step boundary.
      auto *child=open(node("Propagate"),false); select(child,"While");
      auto *condition=child->widget()->findChild<QLineEdit *>("commandField_Condition"); require(condition,"Typed While condition absent"); condition->setText("Sat.ElapsedSecs < 300");
      child->widget()->findChild<QLineEdit *>("commandName")->setText("Coast");
      const auto pending=child->widget()->findChild<QPlainTextEdit *>("commandSource")->toPlainText();
      require(!pending.contains("VariableName") && editor->toPlainText()==baseline,"Typed While template retains an undeclared body assignment or prematurely changes source");
      apply(child); require(!dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Typed While insertion rejected a valid spacecraft condition");
      const auto loopOnly=editor->toPlainText(); require(loopOnly.startsWith(prefix) && loopOnly.endsWith(suffix) && loopOnly.contains("While 'Coast' Sat.ElapsedSecs < 300;"),"While insertion changed unrelated source or lost typed condition/name");
      child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      child=open(node("While"),true); select(child,"Propagate");
      auto *single=child->widget()->findChild<QCheckBox *>("propagationSingleStep"); require(single,"Inserted Propagate lacks Single step control"); single->setChecked(true);
      child->widget()->findChild<QLineEdit *>("commandName")->setText("Loop step"); apply(child);
      require(!dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Single-step loop body insertion failed");
      const auto savedSource=editor->toPlainText(); const auto &snapshot=window.missionSnapshot(); bool body=false;
      for (const auto &entry:snapshot.nodes) if (entry.type=="Propagate" && entry.parent>=0 && snapshot.nodes[entry.parent].type=="While") body=entry.statement.contains("'Loop step'") && !entry.statement.contains('{');
      require(body && savedSource.startsWith(prefix) && savedSource.endsWith(suffix),"Single-step command was not inserted inside the While branch or changed unrelated source");
      child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto path=files.filePath("while coast ü.script"); require(window.saveScriptTo(path) && window.loadScript(path) && editor->toPlainText()==savedSource && window.buildScript(),"While typed construction Save/reopen lost exact source/model");
      require(window.runMission()==MainWindow::RunResult::Completed,qPrintable("Constructed While run failed:\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()));
      QFile report(output); require(report.open(QIODevice::ReadOnly),"While result report absent"); const auto result=QString::fromUtf8(report.readAll()).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
      if (!(result.size()==1 && std::abs(result[0].toDouble()-1200)<1e-6)) std::cerr<<"Actual While report: "<<result.join(" | ").toStdString()<<"\n";
      require(result.size()==1 && std::abs(result[0].toDouble()-1200)<1e-6,"Independently known While coast did not stop at two 600-second steps");
      std::cout<<"PASS actual While template insertion with a spacecraft condition and no undeclared body seed, typed Single-step interior append, unrelated source preservation, Save/reopen, known two-step 1200-second run. No native host acceptance.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
