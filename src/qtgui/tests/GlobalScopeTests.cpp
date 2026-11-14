#include "MainWindow.hpp"
#include "AcceptanceView.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWindow>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Global scope output missing"); return file.readAll(); }
static void write(const QString &path,const QByteArray &data) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"Global function fixture write failed"); }
static QTreeWidgetItem *globalItem(MainWindow &window) { auto *tree=window.findChild<QTreeWidget *>("Mission"); QTreeWidgetItemIterator it(tree); while (*it && !(*it)->text(0).endsWith(" — share resources")) ++it; require(*it,"Labeled Global tree entry missing"); return *it; }
static QMdiSubWindow *active(MainWindow &window) { auto *child=window.findChild<QMdiArea *>("workspace")->activeSubWindow(); require(child && dynamic_cast<CommandEditor *>(child->widget()),"Global command panel missing"); return child; }
static void closePanels(MainWindow &window) { for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (auto *panel=dynamic_cast<CommandEditor *>(child->widget())) { panel->discardChanges(); child->close(); } QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static void apply(QMdiSubWindow *child) { acceptanceView(*child); child->widget()->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); }
static void choose(QMdiSubWindow *child,bool accept,const QString &capture={}) {
   bool visited=false; std::exception_ptr failure;
   QTimer::singleShot(0,child,[&] {
      auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); visited=true;
      try {
         require(dialog && dialog->objectName()=="commandObjectDialog","Actual Global checklist missing"); auto *list=dialog->findChild<QListWidget *>("commandObjectList");
         for (int i=0;i<list->count();++i) list->item(i)->setCheckState(Qt::Unchecked);
         const QStringList names{"Grid","Counter","Text","Sat"};
         for (int i=names.size()-1;i>=0;--i) { auto items=list->findItems(names[i],Qt::MatchExactly); require(items.size()==1,"Global shared resource omitted"); auto *item=list->takeItem(list->row(items.first())); list->insertItem(0,item); item->setCheckState(Qt::Checked); }
         if (!capture.isEmpty()) { QEventLoop loop; QTimer::singleShot(200,&loop,&QEventLoop::quit); loop.exec(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(capture),"Native Global checklist not exposed"); }
         dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
      } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   });
   auto *button=child->widget()->findChild<QPushButton *>("commandChoose_Objects"); require(button,"Global checklist action unavailable"); button->click(); require(visited,"Global checklist not visited"); if (failure) std::rethrow_exception(failure);
}
static QMdiSubWindow *insertBefore(MainWindow &window) {
   auto *tree=window.findChild<QTreeWidget *>("Mission"); auto *item=globalItem(window); tree->expandAll(); tree->scrollToItem(item);
   std::exception_ptr failure; bool visited=false;
   QTimer::singleShot(0,&window,[&] { auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); try { require(menu,"Global insertion context menu missing"); for (auto *action:menu->actions()) if (action->text()=="Insert before…") { visited=true; require(action->isEnabled(),"Global insertion disabled"); menu->setActiveAction(action); QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter); return; } require(false,"Global insertion action missing"); } catch (...) { failure=std::current_exception(); if (menu) menu->close(); } });
   tree->customContextMenuRequested(tree->visualItemRect(item).center()); require(visited,"Global insertion menu not visited"); if (failure) std::rethrow_exception(failure); return active(window);
}
int main(int argc,char **argv) {
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtGlobalScopes");
   try {
      TestSettings settings; QTemporaryDir files; require((argc==2 || argc==3) && files.isValid(),"Global scope setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window; window.show(); require(window.initialize(startup),"Global scope runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto commands=Moderator::Instance()->GetListOfFactoryItems(Gmat::COMMAND); require(std::find(commands.begin(),commands.end(),"Global")!=commands.end() && std::find(commands.begin(),commands.end(),"Clear")==commands.end() && std::find(commands.begin(),commands.end(),"ClearPlot")!=commands.end(),"Selected runtime Global/Clear disposition changed");
      const auto function=files.filePath("ScopeTouch.gmf"),report=files.filePath("scope.txt"),saved=files.filePath("global scope Δ.script"),saveAs=files.filePath("global scope copy ü.script");
      const QByteArray body="function [output] = ScopeTouch(input)\nCreate Variable output Keep;\nKeep = 123;\nBeginMissionSequence;\nGlobal Counter Grid Text Sat;\nCounter = Counter + input;\nGrid(1,1) = Grid(1,1) + 2 * input;\nText = 'shared';\nSat.X = Sat.X + input;\noutput = Counter + Keep;\n"; write(function,body);
      const QString prefix="% retain Global source α and implicit settings\nCreate Variable Counter Input Keep FirstResult SecondResult Decoy;\nCounter = 10;\nInput = 2;\nKeep = 99;\nCreate Array Grid[1,2];\nGrid(1,1) = 1;\nGrid(1,2) = 7;\nCreate String Text;\nText = 'seed';\nCreate Spacecraft Sat;\nSat.X = 7000;\nCreate GmatFunction ScopeTouch;\nScopeTouch.FunctionPath = '"+function+"';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 17;\nBeginMissionSequence;\n";
      const QString original="Global 'share resources' Decoy; % retain Global comment\n",selected="Global 'share resources' Grid Counter Text Sat; % retain Global comment\n";
      const QString tail="FirstResult = ScopeTouch(Input); % first call\nReport Values Counter Grid(1,1) Grid(1,2) Text Sat.EarthMJ2000Eq.X Keep FirstResult SecondResult;\nInput = 3;\nSecondResult = ScopeTouch(Input); % second call\nReport Values Counter Grid(1,1) Grid(1,2) Text Sat.EarthMJ2000Eq.X Keep FirstResult SecondResult;\n";
      const auto source=prefix+original+tail,expected=prefix+selected+tail;
      auto run=[&] {
         const auto result=window.runMission(); QApplication::processEvents(); require(result==MainWindow::RunResult::Completed,qPrintable(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText())); const auto data=read(report); const auto rows=QString::fromUtf8(data).trimmed().split('\n'); require(rows.size()==2,"Global scope report does not contain both calls");
         const QList<QList<double>> numbers={{12,5,7,7002,99,135,0},{15,11,7,7005,99,135,138}};
         for (int r=0;r<rows.size();++r) { const auto values=rows[r].split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(values.size()==8 && values[3]=="shared",qPrintable("Unexpected Global report row: "+rows[r])); int column=0; for (int i=0;i<values.size();++i) if (i!=3) require(std::abs(values[i].toDouble()-numbers[r][column++])<1e-10,"Global/local scope result differs from independent values"); }
         for (const auto &name:QStringList{"Grid","Counter","Text","Sat"}) { auto *object=Moderator::Instance()->GetInternalObject(name.toStdString()); require(object && object->IsGlobal(),"Selected run object did not remain global"); }
         require(read(function)==body && !window.isRunning(),"Global run changed function file or kept runtime locked"); return data;
      };
      editor->setPlainText(expected); const auto reference=run(); closePanels(window);
      editor->setPlainText(source); require(window.buildScript(),"Global source build failed"); auto *tree=window.findChild<QTreeWidget *>("Mission"); tree->itemDoubleClicked(globalItem(window),0); auto *child=active(window);
      choose(child,false); require(editor->toPlainText()==source && !dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Global Cancel changed pending/source"); choose(child,true,!capture.isEmpty() ? capture+".picker.png" : QString()); require(editor->toPlainText()==source && dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Global checklist applied prematurely"); apply(child); require(editor->toPlainText()==expected && !dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Global Apply changed labels/comments/unrelated source or did not retain panel"); editor->undo(); require(editor->toPlainText()==source,"Global Undo not exact"); editor->redo(); require(editor->toPlainText()==expected,"Global Redo not exact"); closePanels(window);
      require(window.saveScriptTo(saved) && window.saveScriptTo(saveAs) && read(saved)==expected.toUtf8() && read(saveAs)==expected.toUtf8() && window.loadScript(saveAs),"Global Unicode Save/Save As/reopen not exact"); require(run()==reference,"GUI Global complete report differs from independent script");
      child=insertBefore(window); QComboBox *templates=nullptr; for (auto *choice:child->widget()->findChildren<QComboBox *>()) if (choice->itemText(0)=="Choose a command template…") templates=choice; require(templates && templates->findText("Global")>=0 && templates->findText("Clear")<0,"Active Global missing from insertion templates or unsupported Clear offered"); templates->setCurrentText("Global"); templates->textActivated("Global"); require(child->widget()->findChild<QPlainTextEdit *>("commandSource")->toPlainText()=="Global Sat;","Global template activation did not populate command text"); choose(child,true);
      auto *pending=child->widget()->findChild<QPlainTextEdit *>("commandSource"); const QString repeated="Global 'repeat globals' Grid Counter Text Sat; % retain repeated scope"; pending->setPlainText(repeated); require(editor->toPlainText()==expected && dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Global insertion applied prematurely"); apply(child);
      const auto inserted=QString(expected).replace(selected,repeated+"\n"+selected); require(editor->toPlainText()==inserted && !dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Global insertion changed unrelated source or panel state"); apply(child); require(editor->toPlainText()==inserted && editor->toPlainText().count("repeat globals")==1,"Repeated insertion Apply duplicated Global"); editor->undo(); require(editor->toPlainText()==expected,"Inserted Global Undo not exact"); editor->redo(); require(editor->toPlainText()==inserted,"Inserted Global Redo not exact"); closePanels(window);
      require(window.saveScriptTo(saveAs) && read(saveAs)==inserted.toUtf8() && window.loadScript(saveAs) && run()==reference,"Repeated Global scope/reopen altered complete reference report");
      tree->itemDoubleClicked(globalItem(window),0); child=active(window); if (!capture.isEmpty()) { QEventLoop loop; QTimer::singleShot(200,&loop,&QEventLoop::quit); loop.exec(); require(window.grab().save(capture+".png"),"Native Global scope capture failed"); write(capture+".state.txt",reference); }
      // Global intentionally resolves objects during execution; names can
      // become available in dynamic scopes. A text fallback must preserve that
      // engine contract rather than rejecting every unconfigured name on Apply.
      child->widget()->findChild<QLineEdit *>("commandField_Objects")->setText("MissingResource"); apply(child);
      require(editor->toPlainText()==inserted && dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Typed Global accepted an unknown resource");
      auto *label=child->widget()->findChild<QLineEdit *>("commandName"); label->setText("renamed pending scope"); apply(child);
      require(editor->toPlainText()==inserted && dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Renaming bypassed typed Global reference validation");
      auto *raw=child->widget()->findChild<QPlainTextEdit *>("commandSource");
      require(raw,"Global raw script fallback missing");
      raw->setPlainText(QString(selected).replace("Grid Counter Text Sat","Missing.Resource").trimmed()); apply(child);
      require(editor->toPlainText()==inserted && dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Raw Global accepted a malformed object identifier");
      raw->setPlainText(QString(selected).replace("Grid Counter Text Sat","MissingResource").trimmed());
      label->setText("temporary scope"); label->setText("share resources"); apply(child);
      const auto invalid=QString(inserted).replace(selected,"Global 'share resources' MissingResource; % retain Global comment\n");
      require(editor->toPlainText()==invalid && !dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Global deferred reference Apply lost source/panel"); closePanels(window);
      const auto failed=window.runMission(); QApplication::processEvents();
      require(failed==MainWindow::RunResult::Failed && !window.isRunning() && editor->toPlainText()==invalid && window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().contains("MissingResource"),"Unknown Global runtime failure hid its error or kept controls locked");
      tree->itemDoubleClicked(globalItem(window),0); child=active(window); choose(child,true); apply(child);
      require(editor->toPlainText()==inserted && !dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Global reference correction failed"); closePanels(window); require(run()==reference,"Global runtime-failure correction changed complete scope/report");
      std::cout<<"PASS: actual Global MDI checklist/Cancel/pending/retained Apply and insertion template; Variable/Array/String/Spacecraft sharing across two function calls, function-local shadow retained (main Keep=99), known Counter/Grid/X and 135/138 outputs, repeated Global idempotence, exact labels/comments/source/Undo/Redo/Unicode Save/Save As/reopen and full independent reports; deferred-reference runtime failure with released controls and correction/rerun. Selected runtime has Global/ClearPlot but no Clear; no numerical-engine change.\n";
   } catch (BaseException &e) { std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
