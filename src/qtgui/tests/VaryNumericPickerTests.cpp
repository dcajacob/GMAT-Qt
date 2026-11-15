#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include "Vary.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
void events() { QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents(); }
QByteArray read(const QString &path) { QFile f(path);require(f.open(QIODevice::ReadOnly),"Numeric Vary saved file missing");return f.readAll(); }
void write(const QString &path,const QByteArray &bytes) { QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Numeric Vary evidence write failed"); }
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("VaryNumericPicker");
   try {
      require(argc==2&&acceptanceWidgetPlatform(),"Supply numeric Vary startup");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      TestSettings settings;QTemporaryDir files;require(files.isValid(),"Numeric Vary temporary files unavailable");
      MainWindow window;require(window.initialize(startup),"Numeric Vary runtime unavailable");window.show();events();acceptanceView(window);
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *area=window.findChild<QMdiArea *>("workspace");auto *tree=window.findChild<QTreeWidget *>("Mission");
      require(source&&area&&tree,"Numeric Vary workspace missing");
      auto statement=[](const QString &value) { return "Vary 'Control' DC("+value+" = 2, {Perturbation = 1, Lower = -20, Upper = 20, MaxStep = 1, AdditiveScaleFactor = 1, MultiplicativeScaleFactor = 1});"; };
      QString accepted="% Numeric Vary Ω\nCreate Variable Control Quantity Keep;\nControl = 7;\nQuantity = 3;\nKeep = 41;\nCreate Array Grid[1,2];\nCreate String Note;\nNote = 'preserved';\nCreate DifferentialCorrector DC;\nDC.ShowProgress = false;\nDC.ReportFile = '"+files.filePath("solver Δ.txt")+"';\nBeginMissionSequence;\nTarget 'Branch' DC {SolveMode = RunInitialGuess, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\n"+statement("Control")+"\nAchieve DC(Quantity = 3, {Tolerance = 0.000001});\nEndTarget;\nKeep = Keep + 1;\n% preserve tail Ω\n";
      source->setPlainText(accepted);require(window.buildScript(),"Numeric Vary baseline did not build");
      auto open=[&] {
         auto *tabs=window.findChild<QTabWidget *>();for(int i=0;tabs&&i<tabs->count();++i)if(tabs->tabText(i)=="Mission")tabs->setCurrentIndex(i);
         QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->text(0).endsWith("Control")){require(!item,"Numeric Vary tree label ambiguous");item=*it;}
         require(item,"Numeric Vary tree item missing");tree->expandAll();tree->setCurrentItem(item);tree->scrollToItem(item);events();acceptanceView(window);tree->itemDoubleClicked(item,0);events();
         auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Numeric Vary actual editor missing");child->showNormal();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);child->raise();events();acceptanceView(*child);return child;
      };
      QJsonArray observations;QString previous="Control";int phase=0;
      for(const auto &value:QStringList{"1","-1.25e+1","Control"}) {
         auto *child=open();auto editor=[&] {auto *current=dynamic_cast<CommandEditor *>(child->widget());require(current,"Numeric Vary current editor missing");return current;};auto *preview=child->findChild<QPlainTextEdit *>("commandSource");auto *field=child->findChild<QLineEdit *>("commandField_Variable");auto *choose=child->findChild<QPushButton *>("commandChoose_Variable");
         require(preview&&field&&choose&&field->text()==previous,"Numeric Vary typed field missing");const auto oldDraft=preview->toPlainText();
         for(bool accept:{false,true}) {
            std::exception_ptr failure;bool visited=false;
            QTimer::singleShot(0,child,[&] {
               auto *dialog=dynamic_cast<ReportParameterDialog *>(QApplication::activeModalWidget());
               try {
                  require(dialog,"Numeric Vary actual picker missing");visited=true;auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry");auto *allow=dialog->findChild<QCheckBox *>("varyAllowNumericControl");auto *box=dialog->findChild<QDialogButtonBox *>();
                  require(entry&&allow&&box&&allow->isChecked()==(previous!="Control"),"Numeric Vary picker did not preserve numeric/resource mode");
                  allow->setChecked(false);entry->setEditText("17");require(!box->button(QDialogButtonBox::Ok)->isEnabled(),"Default Vary resource mode accepted a number");
                  allow->setChecked(true);
                  for(const auto &invalid:QStringList{"nan","inf","1e309","Grid","Grid(2,1)","Note"}) {entry->setEditText(invalid);require(!box->button(QDialogButtonBox::Ok)->isEnabled(),"Numeric Vary picker accepted a nonfinite or invalid resource control");}
                  for(const auto &valid:QStringList{"+17",".25","-1.25e+1","Grid(1,2)","Control"}) {entry->setEditText(valid);require(box->button(QDialogButtonBox::Ok)->isEnabled(),"Numeric Vary picker rejected a supported control");}
                  entry->setEditText(value);require(box->button(QDialogButtonBox::Ok)->isEnabled(),"Numeric Vary chosen control unavailable");acceptanceView(*dialog);if(accept)box->button(QDialogButtonBox::Ok)->click();else dialog->reject();
               } catch(...) {failure=std::current_exception();if(dialog)dialog->reject();}
            });
            choose->click();if(failure)std::rethrow_exception(failure);require(visited,"Numeric Vary picker unvisited");events();
            if(!accept)require(preview->toPlainText()==oldDraft&&field->text()==previous&&source->toPlainText()==accepted&&!editor()->hasChanges(),"Numeric Vary Cancel changed pending/source state");
         }
         require(field->text()==value&&source->toPlainText()==accepted&&editor()->hasChanges(),"Numeric Vary selection escaped pending state");
         const auto before=accepted;accepted.replace(statement(previous),statement(value));
         // A successful Apply replaces the editor widget inside this MDI child.
         // Reacquire its buttons instead of clicking a retired editor's button.
         auto apply=[&] {auto *buttons=child->findChild<QDialogButtonBox *>();auto *button=buttons?buttons->button(QDialogButtonBox::Apply):nullptr;require(button,"Numeric Vary Apply missing");button->click();events();};
         apply();require(source->toPlainText()==accepted&&!editor()->hasChanges(),"Numeric Vary Apply changed unrelated source or failed");
         const int undo=source->document()->availableUndoSteps();apply();require(source->toPlainText()==accepted&&source->document()->availableUndoSteps()==undo,"Numeric Vary repeated Apply changed history");require(child->close(),"Numeric Vary clean editor would not close");events();
         source->undo();require(source->toPlainText()==before&&window.buildScript(),"Numeric Vary Undo lost exact source/model");source->redo();require(source->toPlainText()==accepted&&window.buildScript(),"Numeric Vary Redo lost exact source/model");
         const auto path=files.filePath(QString::number(phase)+" Vary Δ.script"),copy=files.filePath(QString::number(phase)+" copy ü.script");
         require(window.saveScriptTo(path)&&read(path)==accepted.toUtf8()&&window.saveScriptTo(copy)&&read(copy)==accepted.toUtf8()&&window.loadScript(copy)&&window.buildScript()&&source->toPlainText()==accepted,"Numeric Vary Unicode save/reopen lost exact source");
         const int runUndo=source->document()->availableUndoSteps();require(window.runMission()==MainWindow::RunResult::Completed,"Numeric Vary GUI mission did not complete");
         SolverBranchCommand *branch=nullptr;for(auto *c=Moderator::Instance()->GetFirstCommand();c;c=c->GetNext())if(c->IsOfType("Target")){branch=dynamic_cast<SolverBranchCommand *>(c);break;}
         auto *solver=branch?dynamic_cast<Solver *>(branch->GetClone(0)):nullptr;Vary *vary=nullptr;if(branch)for(auto *c=branch->GetChildCommand(0);c&&c!=branch;c=c->GetNext())if((vary=dynamic_cast<Vary *>(c)))break;
         require(solver&&vary&&vary->GetRealParameter(vary->GetParameterID("CurrentValue"))==2&&solver->GetSolverVariable(0)==3,"Numeric Vary native physical/scaled arithmetic differs");
         for(const auto &option:QStringList{"Perturbation","MaxStep","AdditiveScaleFactor","MultiplicativeScaleFactor"})require(vary->GetRealParameter(vary->GetParameterID(option.toStdString()))==1,"Numeric Vary execution changed a same-text setting");
         require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42&&source->toPlainText()==accepted&&source->document()->availableUndoSteps()==runUndo,"Numeric Vary Run changed sentinel/source/history");
         auto *reopened=open();auto *reopenedField=reopened->findChild<QLineEdit *>("commandField_Variable");require(reopenedField&&reopenedField->text()==value,"Reopened numeric Vary field lost its selected control");require(reopened->close(),"Reopened numeric Vary clean editor would not close");events();
         observations.append(QJsonObject{{"control",value},{"physical",2},{"scaled",3},{"source_history_and_unicode",true}});
         const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!evidence.isEmpty())write(QDir(evidence).filePath(QString::number(phase)+"-accepted.script"),accepted.toUtf8());previous=value;++phase;
      }
      QJsonObject proof{{"checkpoint","vary-numeric-picker"},{"outcome","passed"},{"observations",observations},{"scope","Explicit finite numeric control selection and return to named control through actual GUI Apply/history/save/reopen/native Target RunInitialGuess. Full scalar/solver/domain and original parent qualification remain open."}};
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!evidence.isEmpty())write(QDir(evidence).filePath("proof.json"),QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   } catch(BaseException &e) {std::cerr<<e.GetFullMessage()<<std::endl;}catch(std::exception &e) {std::cerr<<e.what()<<std::endl;}return 1;
}
