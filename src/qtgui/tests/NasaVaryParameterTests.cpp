#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
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
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); QApplication::processEvents(); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Vary parameter artifact missing"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Vary parameter artifact write failed"); }
using Row=std::array<double,3>;
static std::vector<Row> rows(const QByteArray &bytes) {
 std::vector<Row> result;
 for (const auto &line:QString::fromUtf8(bytes).trimmed().split('\n')) {
  const auto columns=line.split(','); require(columns.size()==3,"Vary report lost columns"); Row row;
  for (int i=0;i<3;++i) { bool ok=false; row[i]=columns[i].trimmed().toDouble(&ok); require(ok&&std::isfinite(row[i]),"Vary report contains nonfinite value"); }
  require(std::abs(row[1]-(2*row[0]+1))<5e-10&&row[2]==42,"Vary report differs from independent linear quantity/sentinel"); result.push_back(row);
 }
 return result;
}
static QJsonArray numbers(const std::vector<Row> &rows) { QJsonArray result; for (const auto &r:rows) result.append(QJsonArray{r[0],r[1],r[2]}); return result; }

int main(int argc,char **argv)
{
 QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("NasaVaryParameter");
 try {
  require(argc==2&&acceptanceWidgetPlatform(),"Supply startup for Vary parameter case");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Vary parameter fixture unavailable");
  MainWindow window; require(window.initialize(startup),"Vary parameter runtime unavailable"); window.show(); events(); acceptanceView(window);
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *area=window.findChild<QMdiArea *>("workspace");
  auto *tree=window.findChild<QTreeWidget *>("Mission"); auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");
  require(source&&area&&tree&&messages,"Vary parameter workspace missing");
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"); auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
  const auto trials=files.filePath("trials Δ.csv"),final=files.filePath("final Δ.csv");
  const QString oldVary="Vary 'Control' First(Control = 1, {Perturbation = 0.1, Lower = -10, Upper = 10, MaxStep = 10, AdditiveScaleFactor = 0, MultiplicativeScaleFactor = 1});";
  const QString newVary="Vary 'Control' Second(Sat.EarthMJ2000Eq.X = 2, {Perturbation = 0.02, Lower = 1, Upper = 5, MaxStep = 0.5, AdditiveScaleFactor = 3, MultiplicativeScaleFactor = 2});";
  QString baseline="% Vary physical/scaled witness Ω\nCreate Variable Control Quantity Keep;\nKeep = 41;\nCreate Array Grid[1,2];\nCreate String Note;\nNote = 'retained';\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7;\nSat.VZ = 0;\nCreate DifferentialCorrector First Second;\nCreate Yukon Other;\n";
  for (const auto &name:QStringList{"First","Second"}) baseline+=name+".ShowProgress = false;\n"+name+".MaximumIterations = 10;\n"+name+".DerivativeMethod = ForwardDifference;\n"+name+".ReportFile = '"+files.filePath(name+".txt")+"';\n";
  baseline+="Create ReportFile Trials Final;\nTrials.Filename = '"+trials+"';\nFinal.Filename = '"+final+"';\n";
  for (const auto &name:QStringList{"Trials","Final"}) baseline+=name+".WriteHeaders = false;\n"+name+".FixedWidth = false;\n"+name+".Delimiter = ',';\n"+name+".Precision = 17;\n"+name+".SolverIterations = All;\n";
  baseline+="BeginMissionSequence;\nKeep = Keep + 1;\nTarget 'Branch' First {SolveMode = Solve, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\n"+oldVary+"\nQuantity = 2 * Sat.EarthMJ2000Eq.X + 1;\nReport Trials Sat.EarthMJ2000Eq.X Quantity Keep;\nAchieve 'Goal' First(Quantity = 9, {Tolerance = 0.000000001});\nEndTarget;\nReport Final Sat.EarthMJ2000Eq.X Quantity Keep;\n% preserve tail Ω\n";
  source->setPlainText(baseline); require(window.buildScript(),"Vary parameter baseline failed to build"); save("baseline.script",baseline.toUtf8());
  auto open=[&](const QString &label) {
   auto *tabs=window.findChild<QTabWidget *>(); for (int i=0;tabs&&i<tabs->count();++i) if (tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
   QTreeWidgetItem *item=nullptr; for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->text(0).endsWith(label)) { require(!item,"Vary tree label ambiguous"); item=*it; }
   require(item,"Vary tree item missing"); tree->expandAll(); tree->setCurrentItem(item); tree->scrollToItem(item); events(); acceptanceView(window); tree->itemDoubleClicked(item,0); events();
   auto *child=area->activeSubWindow(); require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Vary actual command editor missing"); child->showNormal(); child->move(0,0); child->resize(area->viewport()->size()); area->setActiveSubWindow(child); child->raise(); events(); acceptanceView(*child); return child;
  };
  auto apply=[&](QMdiSubWindow *child,const QString &before,const QString &after,const QString &phase) {
   require(source->toPlainText()==before&&dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Vary draft escaped pending state"); acceptanceView(*child);
   child->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); events();
   require(source->toPlainText()==after&&!dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Vary Apply changed unrelated source or lost accepted draft");
   const int steps=source->document()->availableUndoSteps(); child->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); events();
   require(source->toPlainText()==after&&source->document()->availableUndoSteps()==steps,"Vary repeated Apply changed source/history"); require(child->close(),"Vary clean panel did not close"); events();
   source->undo(); require(source->toPlainText()==before&&window.buildScript(),"Vary Undo did not restore exact source/model"); source->redo(); require(source->toPlainText()==after&&window.buildScript(),"Vary Redo did not restore exact source/model");
   const auto path=files.filePath(phase+" Vary Δ.script"),copy=files.filePath(phase+" copy ü.script");
   require(window.saveScriptTo(path)&&read(path)==after.toUtf8()&&window.saveScriptTo(copy)&&read(copy)==after.toUtf8()&&window.loadScript(copy)&&window.buildScript()&&source->toPlainText()==after,"Vary Unicode Save/Save As/reopen changed exact source"); save(phase+".script",after.toUtf8());
  };
  auto *child=open("Branch"); auto *preview=child->findChild<QPlainTextEdit *>("commandSource"); require(preview,"Target solver source preview missing");
  const auto branchDraft=preview->toPlainText();
  for (bool accept:{false,true}) {
   std::exception_ptr failure; bool visited=false; QTimer::singleShot(0,child,[&] { auto *dialog=qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
    try { require(dialog,"Target solver picker missing"); visited=true; auto *combo=dialog->findChild<QComboBox *>(); require(combo&&combo->findText("First")>=0&&combo->findText("Second")>=0&&combo->findText("Other")<0,"Target solver picker type filtering failed"); combo->setCurrentText("Second"); acceptanceView(*dialog); if (accept) dialog->accept(); else dialog->reject(); }
    catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   }); child->findChild<QPushButton *>("commandChoose_Solver")->click(); if (failure) std::rethrow_exception(failure); require(visited,"Target solver picker unvisited"); events(); acceptanceView(*child);
   if (!accept) require(preview->toPlainText()==branchDraft&&source->toPlainText()==baseline&&!dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Target solver Cancel changed pending/source state");
  }
  require(child->findChild<QLineEdit *>("commandField_Solver")->text()=="Second"&&source->toPlainText()==baseline,"Target solver selection escaped pending state");
  // The branch preview also exposes its owned commands. Rebind those references
  // explicitly in this one atomic edit; the picker changes the header only.
  preview->setPlainText(QString(preview->toPlainText()).replace("First(","Second(")); events(); acceptanceView(*child);
  const auto selected=QString(baseline).replace("Target 'Branch' First","Target 'Branch' Second").replace("First(","Second("); apply(child,baseline,selected,"solver-selection");
  child=open("Control"); auto field=[&](const QString &name) { auto *p=child->findChild<QLineEdit *>("commandField_"+name); require(p&&p->isVisible()&&p->isEnabled(),"Vary typed control missing/disabled"); return p; };
  require(field("Variable")->text()=="Control","Vary original variable lost");
  for (bool accept:{false,true}) {
   std::exception_ptr failure; bool visited=false; QTimer::singleShot(0,child,[&] { auto *dialog=dynamic_cast<ReportParameterDialog *>(QApplication::activeModalWidget());
    try { require(dialog,"Vary writable picker missing"); visited=true; auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry"); auto *box=dialog->findChild<QDialogButtonBox *>(); require(entry&&box&&entry->findText("Note")<0,"Vary writable picker offers String");
     for (const auto &invalid:QStringList{"17","Grid","Grid(2,1)","Note"}) { entry->setEditText(invalid); require(!box->button(QDialogButtonBox::Ok)->isEnabled(),qPrintable("Vary writable picker accepts invalid control: "+invalid)); }
     dialog->findChild<QComboBox *>("reportPropertyObject")->setCurrentText("Sat"); auto *property=dialog->findChild<QComboBox *>("reportPropertyType"); require(property->findText("X")>=0&&property->findText("ElapsedSecs")<0,"Vary picker writable-property filtering failed");
     property->setCurrentText("X"); dialog->findChild<QComboBox *>("reportPropertyDependency")->setCurrentText("EarthMJ2000Eq"); dialog->findChild<QPushButton *>("reportUseReference")->click(); acceptanceView(*dialog);
     require(dialog->selection()==QStringList{"Sat.EarthMJ2000Eq.X"}&&box->button(QDialogButtonBox::Ok)->isEnabled(),"Vary scalar-property selection wrong"); box->button(accept ? QDialogButtonBox::Ok:QDialogButtonBox::Cancel)->click();
    } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   }); child->findChild<QPushButton *>("commandChoose_Variable")->click(); if (failure) std::rethrow_exception(failure); require(visited,"Vary writable picker unvisited"); events(); acceptanceView(*child);
   if (!accept) require(field("Variable")->text()=="Control"&&source->toPlainText()==selected&&!dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Vary property Cancel changed pending/source state");
  }
  for (const auto &pair:QList<QPair<QString,QString>>{{"Initial value","2"},{"Perturbation","0.02"},{"Lower","1"},{"Upper","5"},{"MaxStep","0.5"},{"AdditiveScaleFactor","3"},{"MultiplicativeScaleFactor","2"}}) field(pair.first)->setText(pair.second);
  events(); acceptanceView(*child); QString accepted=QString(selected).replace(QString(oldVary).replace("First(","Second("),newVary); apply(child,selected,accepted,"vary-settings");
  QJsonArray reports;
  for (const auto &phase:QStringList{"feasible","upper","lower"}) {
   const double desired=phase=="feasible" ? 9:phase=="upper" ? 13:-1,endpoint=phase=="feasible" ? 4:phase=="upper" ? 5:1;
   if (phase!="feasible") {
    child=open("Goal"); auto *value=child->findChild<QLineEdit *>("commandField_Value"); require(value&&value->isVisible(),"Vary bound goal editor missing"); value->setText(QString::number(desired)); events(); acceptanceView(*child);
    const auto previous=accepted; accepted.replace(phase=="upper" ? "Quantity = 9,":"Quantity = 13,","Quantity = "+QString::number(desired)+","); apply(child,previous,accepted,phase+"-goal");
   }
   const int undo=source->document()->availableUndoSteps(); acceptanceView(window); const auto status=window.runMission(); save(phase+".messages.txt",messages->toPlainText().toUtf8());
   if (status!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString(); require(status==MainWindow::RunResult::Completed,"Vary boundary mission failed to complete");
   const auto trialBytes=read(trials),finalBytes=read(final); save(phase+".trials.csv",trialBytes); save(phase+".final.csv",finalBytes); save(phase+".script",accepted.toUtf8());
   std::cerr<<"VARY_PARAMETER_RESULT "<<phase.toStdString()<<' '<<finalBytes.constData()<<std::endl;
   const auto trialRows=rows(trialBytes),finalRows=rows(finalBytes); require(finalRows.size()==1,"Vary final report needs one accepted row"); const int nominal=phase=="feasible" ? 5:10;
   require(trialRows.size()==std::size_t(2*nominal),"Vary nominal/perturbation/final publish row count differs");
   for (int i=0;i<nominal;++i) {
    const double x=phase=="lower" ? std::max(1.,2.-0.5*i):std::min(endpoint,2.+0.5*i);
    require(std::abs(trialRows[2*i][0]-x)<5e-9,"Vary nominal value violates independent initial/max-step/bound sequence");
    if (i+1<nominal) { const double pert=x+0.02>5 ? x-0.02:x+0.02; require(std::abs(trialRows[2*i+1][0]-pert)<5e-9,"Vary perturbation magnitude/direction violates independent physical-space oracle"); }
   }
   require(std::abs(finalRows.front()[0]-endpoint)<5e-9&&std::abs(trialRows.back()[0]-endpoint)<5e-9,"Vary final accepted state differs from independent bounded result");
   auto *command=Moderator::Instance()->GetFirstCommand(); while (command&&!command->IsOfType("Target")) command=command->GetNext(); auto *branch=dynamic_cast<SolverBranchCommand *>(command); auto *solver=branch ? dynamic_cast<Solver *>(branch->GetClone(0)):nullptr;
   require(solver&&solver->GetName()=="Second","Target selected solver identity not executed"); const int solverStatus=solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"));
   require(solverStatus==(phase=="feasible" ? Gmat::CONVERGED:Gmat::EXCEEDED_ITERATIONS),"Vary unreachable goal incorrectly converged or feasible goal failed");
   const double scaled=solver->GetSolverVariable(0); require(std::abs(scaled-2*(endpoint+3))<1e-8,"Vary scaled native variable violates documented m*(x+a) equation");
   require(source->toPlainText()==accepted&&source->document()->availableUndoSteps()==undo,"Vary Run changed exact source/history");
   reports.append(QJsonObject{{"phase",phase},{"desired",desired},{"endpoint",endpoint},{"converged",phase=="feasible"},{"nominal_passes",nominal},{"scaled_variable",scaled},{"trials",numbers(trialRows)},{"final",numbers(finalRows)}});
  }
  const QJsonObject proof{{"checkpoint","vary-scalar-parameter"},{"outcome","passed"},{"reports",reports},{"selected_solver","Second"},{"numeric_sentinel",42},
   {"facets",QJsonArray{"actual-target-solver-picker-and-owned-reference-edit","actual-vary-writable-property-picker","all-seven-dc-option-controls","pending-apply-idempotence-history-unicode","independent-initial-perturbation-step-bounds-and-scaled-values","explicit-nonconvergence-for-unreachable-goals"}},
   {"scope","Selected DifferentialCorrector forward-difference linear scalar property, explicit physical/scaled initial/perturbation/max-step sequences and upper/lower clipping with nonconvergence. No propagation/frame science, arbitrary scalar/solver/conditioning/boundary domains, fmincon/VF13ad qualification, native OS input, hardware/platform/stress/current private NASA corpus or full current-build qualification."}};
  save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
