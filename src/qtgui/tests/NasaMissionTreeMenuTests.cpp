#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); QApplication::processEvents(); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Tree menu artifact missing"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Tree menu artifact write failed"); }

int main(int argc,char **argv)
{
 QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("NasaMissionTreeMenu");
 try {
  require(argc==3&&acceptanceWidgetPlatform(),"Supply startup and Mission Tree menu case");
  const QString mode=argv[2],startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Tree menu fixture directory unavailable");
  MainWindow window; require(window.initialize(startup),"Tree menu runtime unavailable"); window.show(); acceptanceView(window); events();
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Mission"); auto *area=window.findChild<QMdiArea *>("workspace");
  require(source&&tree&&area,"Tree menu workspace missing");
  auto *tabs=window.findChild<QTabWidget *>(); for (int i=0;tabs&&i<tabs->count();++i) if (tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
  const auto report=files.filePath("Tree witness Δ.csv");
  const QString base="% applied source sentinel Ω\nCreate Variable V i A;\nV = 0;\ni = 0;\nA = 1;\nCreate DifferentialCorrector DC;\nCreate Yukon Opt;\nCreate Spacecraft Sat;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 0;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nBeginMissionSequence;\nFor 'Outer α' i = 1:1:3;\n   If 'Inner β' i > 0;\n      'Increment Δ' V = V + 2;\n   EndIf;\nEndFor;\n'Final Δ' V = V + 3;\nReport 'Witness' Values V i;\n% final source sentinel Ω\n";
  source->setPlainText(base); require(window.buildScript(),"Tree menu baseline failed to build"); const int undo=source->document()->availableUndoSteps();
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"); auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
  save("mission.script",base.toUtf8()); QJsonArray observations,actions; bool witness=false;
  auto item=[&](const QString &name) { const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (missionCommandLabel(snapshot.nodes[i].statement).name==name) index=i; require(index>=0,"Named tree command missing"); for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index) return *it; throw std::runtime_error("Named command tree item missing"); };
  auto root=[&] { require(tree->topLevelItemCount()==1,"Mission tree root missing"); return tree->topLevelItem(0); };
  auto invoke=[&](QTreeWidgetItem *target,const QString &name,std::function<void()> after={}) {
   for (auto *parent=target->parent();parent;parent=parent->parent()) parent->setExpanded(true);
   tree->setCurrentItem(target); tree->scrollToItem(target); events(); require(tree->isVisible(),"Tree popup owner not visible"); acceptanceView(window);
   std::exception_ptr failure; bool visited=false;
   QTimer::singleShot(0,&window,[&] {
    try {
     auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); require(menu,"Actual tree popup missing"); QAction *choice=nullptr;
     for (auto *action:menu->actions()) if (action->objectName()==name||action->text()==name) choice=action;
     require(choice&&choice->isEnabled(),"Required tree popup action missing or disabled"); menu->setActiveAction(choice); acceptanceView(*menu); visited=true;
     if (after) QTimer::singleShot(0,&window,[&] { try { after(); } catch (...) { failure=std::current_exception(); if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) dialog->reject(); } });
     QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter);
    } catch (...) { failure=std::current_exception(); if (auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget())) menu->close(); }
   });
   tree->customContextMenuRequested(tree->visualItemRect(target).center()); events(); if (failure) std::rethrow_exception(failure); require(visited,"Tree popup action was not exercised"); actions.append(name);
  };
  auto panel=[&] { auto *child=area->activeSubWindow(); require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Actual command editor missing"); child->showNormal(); area->setActiveSubWindow(child); child->raise(); events(); acceptanceView(*child); require(area->viewport()->rect().contains(child->geometry()),"Command editor extends outside visible workspace"); return child; };
  auto preview=[&](QMdiSubWindow *child) { auto *text=child->findChild<QPlainTextEdit *>("commandSource"); require(text,"Command source missing"); return text; };
  auto inspect=[&](const QString &file,const QString &expected) { auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); require(dialog,"Tree inspection dialog missing"); auto *text=dialog->findChild<QPlainTextEdit *>("inspectionText"); require(text&&text->isReadOnly(),"Tree inspection is not read-only"); acceptanceView(*dialog); save(file,text->toPlainText().toUtf8()); require(text->toPlainText()==expected,"Tree inspection differs from independent literal source/hierarchy"); dialog->reject(); };
  if (mode=="OpenReuse") {
   invoke(item("Final Δ"),"Edit command…"); QPointer<QMdiSubWindow> owner=panel(); preview(owner)->setPlainText("'Final Δ' V = V + 30;"); const int count=area->subWindowList().size();
   invoke(item("Increment Δ"),"Edit command…"); QPointer<QMdiSubWindow> other=panel(); require(other!=owner,"Different command reused the wrong owner");
   invoke(item("Final Δ"),"Edit command…"); require(panel()==owner&&area->subWindowList().size()==count+1&&preview(owner)->toPlainText()=="'Final Δ' V = V + 30;","Repeated context Open duplicated or discarded the pending command owner");
   tree->itemDoubleClicked(item("Final Δ"),0); events(); require(panel()==owner&&area->subWindowList().size()==count+1,"Repeated double-click Open duplicated the command owner");
   observations.append(QJsonObject{{"same_owner",true},{"pending","'Final Δ' V = V + 30;"},{"command_owners",2}}); dynamic_cast<CommandEditor *>(owner->widget())->discardChanges(); owner->close(); other->close(); events();
  } else if (mode=="ClosePending") {
   const int count=area->subWindowList().size(); invoke(item("Final Δ"),"missionCloseCommand"); require(area->subWindowList().size()==count,"Close without an owner changed the workspace");
   invoke(item("Final Δ"),"Edit command…"); QPointer<QMdiSubWindow> owner=panel(); preview(owner)->setPlainText("'Final Δ' V = V + 30;");
   invoke(item("Increment Δ"),"Edit command…"); QPointer<QMdiSubWindow> other=panel();
   auto answer=[&](QMessageBox::StandardButton button) { auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); require(box&&box->button(button),"Tree Close pending-choice dialog missing"); acceptanceView(*box); box->button(button)->click(); };
   invoke(item("Final Δ"),"missionCloseCommand",[&] { answer(QMessageBox::Cancel); }); require(owner&&other&&preview(owner)->toPlainText()=="'Final Δ' V = V + 30;","Tree Close Cancel lost pending owner or closed another command");
   invoke(item("Final Δ"),"missionCloseCommand",[&] { answer(QMessageBox::Discard); }); require(!owner&&other,"Tree Close Discard retained target or removed another owner");
   invoke(item("Increment Δ"),"missionCloseCommand"); require(!other&&area->subWindowList().size()==count,"Clean tree Close left its command owner"); observations.append(QJsonObject{{"no_owner_noop",true},{"cancel_retained_pending",true},{"discard_closed_only_target",true},{"clean_closed",true}});
  } else if (mode=="Run") {
   require(source->find("'Final Δ' V = V + 3;"),"Pending run edit anchor absent"); auto cursor=source->textCursor(); cursor.insertText("'Final Δ' V = V + 4;"); const auto pending=source->toPlainText();
   invoke(root(),"missionRun"); require(source->toPlainText()==pending&&QString::fromUtf8(read(report)).trimmed()=="10,3","Tree Run did not build/execute the pending script to independent literal10,3"); save("run.csv",read(report)); save("run.script",pending.toUtf8()); observations.append(QJsonObject{{"run_result",QJsonArray{10,3}},{"built_pending_source",true}});
   source->undo(); require(source->toPlainText()==base,"Tree Run damaged exact pending-source Undo");
  } else if (mode=="ShowScript") {
   source->appendPlainText("% pending editor text ONLY"); const auto pending=source->toPlainText(); const int steps=source->document()->availableUndoSteps();
   invoke(root(),"missionShowScript",[&] { inspect("script-preview.txt",base); }); require(source->toPlainText()==pending&&source->document()->availableUndoSteps()==steps,"Applied-script preview changed pending editor/history"); observations.append(QJsonObject{{"applied_exact",true},{"pending_excluded",true}}); source->undo();
  } else if (mode=="Detail") {
   require(item("Final Δ")->text(0)=="GMAT — Final Δ","Initial short command label differs from literal name"); const int index=item("Final Δ")->data(0,Qt::UserRole).toInt(); require(window.setBreakpoint(index,true),"Detail breakpoint fixture unavailable");
   invoke(root(),"missionShowDetail"); require(item("Final Δ")->text(0)=="● 'Final Δ' V = V + 3;","Detail did not show full literal command with breakpoint marker"); observations.append(item("Final Δ")->text(0));
   invoke(root(),"missionShowDetail"); require(item("Final Δ")->text(0)=="● GMAT — Final Δ","Detail toggle did not restore short name/breakpoint"); require(window.setBreakpoint(index,false),"Detail breakpoint cleanup failed");
   invoke(root(),"missionShowDetail"); require(window.buildScript()&&item("Final Δ")->text(0)=="'Final Δ' V = V + 3;","Detail setting did not survive tree rebuild"); observations.append(item("Final Δ")->text(0)); invoke(root(),"missionShowDetail");
  } else if (mode=="DetailText") {
   auto quoted=base; quoted.replace("Create Variable V i A;","Create String Note;\nCreate Variable V i A;"); quoted.replace("'Final Δ' V = V + 3;","'Text spacing' Note = 'two  spaces';\n'Final Δ' V = V + 3;");
   auto cursor=source->textCursor(); cursor.beginEditBlock(); cursor.select(QTextCursor::Document); cursor.insertText(quoted); cursor.endEditBlock(); require(window.buildScript(),"Quoted detail fixture failed to build");
   invoke(root(),"missionShowDetail"); require(item("Text spacing")->text(0)=="'Text spacing' Note = 'two  spaces';","Detail changed significant whitespace inside a quoted string"); acceptanceView(window); observations.append(item("Text spacing")->text(0));
   invoke(root(),"missionShowDetail"); require(item("Text spacing")->text(0)=="GMAT — Text spacing","Quoted detail did not return to its short label"); source->undo(); require(source->toPlainText()==base,"Quoted detail fixture changed unrelated source");
  } else if (mode=="Expansion") {
   const QString extra="While 'Never' V < 0;\n V = 0;\nEndWhile;\nTarget 'Target branch' DC {SolveMode = RunInitialGuess, ShowProgressWindow = false};\n Vary DC(A = 1);\n Achieve DC(A = 2);\nEndTarget;\nOptimize 'Optimize branch' Opt {SolveMode = RunInitialGuess, ShowProgressWindow = false};\n Vary Opt(A = 1);\n Minimize Opt(A);\nEndOptimize;\n";
   require(source->find("Report 'Witness'"),"Expansion insertion anchor missing"); auto cursor=source->textCursor(); cursor.setPosition(cursor.selectionStart()); cursor.insertText(extra); require(window.buildScript(),"Expansion branch matrix failed to build"); save("expansion.script",source->toPlainText().toUtf8());
   for (int pass=0;pass<2;++pass) {
    if (pass) { window.resize(window.width()-80,window.height()-50); events(); }
    invoke(root(),"missionCollapseAll"); for (QTreeWidgetItemIterator it(tree);*it;++it) require(!(*it)->isExpanded(),"Popup Collapse All left an expanded tree item");
    invoke(root(),"missionExpandAll"); int branches=0; for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->childCount()) { ++branches; require((*it)->isExpanded(),"Popup Expand All left a collapsed branch"); }
    require(branches>=6,"Expansion fixture did not contain all five branch types"); observations.append(QJsonObject{{"expanded_branches",branches},{"pass",pass}}); acceptanceView(window);
   }
   source->undo(); require(source->toPlainText()==base,"Expansion changed source beyond its explicit fixture edit");
  } else if (mode=="Sequence") {
   const QString expected="Mission Sequence\n  BeginMissionSequence\n  For — Outer α\n    If — Inner β\n      GMAT — Increment Δ\n      EndIf\n    EndFor\n  GMAT — Final Δ\n  Report — Witness\n";
   invoke(root(),"missionShowSequence",[&] { inspect("sequence-preview.txt",expected); }); observations.append(QJsonObject{{"hierarchy_exact",true},{"source_expressions_excluded",true}});
  } else if (mode=="Summary") {
   require(window.runMission()==MainWindow::RunResult::Completed&&QString::fromUtf8(read(report)).trimmed()=="9,3","Summary baseline witness differs from literal9,3"); witness=true;
   invoke(item("Final Δ"),"missionCommandSummary",[&] {
    auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); require(dialog,"Context command summary absent"); auto *text=dialog->findChild<QPlainTextEdit *>("inspectionText"); require(text&&text->isReadOnly(),"Summary text is editable"); acceptanceView(*dialog); save("summary.txt",text->toPlainText().toUtf8()); QJsonArray states;
    const QStringList keys{"X","Y","Z","VX","VY","VZ"}; const double expected[]={7000,0,0,0,7.5,0};
    for (int i=0;i<6;++i) { const auto match=QRegularExpression("\\b"+keys[i]+"\\s*=\\s*([-+0-9.eE]+)\\s+km").match(text->toPlainText()); bool ok=false; const double value=match.captured(1).toDouble(&ok); require(match.hasMatch()&&ok&&std::isfinite(value)&&std::abs(value-expected[i])<1e-9,"Context summary differs from independent static Cartesian state"); states.append(value); }
    observations.append(QJsonObject{{"summary_state",states}}); dialog->reject();
   });
  } else require(false,"Unknown Mission Tree menu case");
  require(source->toPlainText()==base&&source->document()->availableUndoSteps()==undo,"Tree menu mechanism altered source or undo history");
  if (!witness) require(window.runMission()==MainWindow::RunResult::Completed,"Tree menu literal witness failed to execute");
  require(QString::fromUtf8(read(report)).trimmed()=="9,3","Tree menu mechanism changed independent literal9,3 mission result"); save("mission.csv",read(report)); save("observations.json",QJsonDocument(observations).toJson());
  const QJsonObject proof{{"checkpoint","mission-tree-menu-"+mode},{"mode",mode},{"outcome","passed"},{"numeric_sentinel",9},{"loop_index",3},{"observations",observations},{"actions",actions},{"facets",QJsonArray{"actual-tree-popup-route","independent-source-label-state-or-literal-oracle","pending-owner-and-history-protection","visible-exposure-boundaries","literal-mission-witness"}},{"scope","Selected actual Mission Tree menu routes and exact independent literals. No every command/domain, native OS pointer, scientific propagation/frame algorithms, host GPU/compositor/platform/stress, current private NASA corpus or full current-build qualification."}};
  save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
