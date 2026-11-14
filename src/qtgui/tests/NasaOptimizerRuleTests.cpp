#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); QApplication::processEvents(); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Optimizer rule artifact missing"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Optimizer rule artifact write failed"); }

int main(int argc,char **argv)
{
 QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("NasaOptimizerRules");
 try {
  require(argc==3&&acceptanceWidgetPlatform(),"Supply startup and optimizer rule case");
  const QString mode=argv[2],startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
  require(QStringList{"Unique","Multiple","Feasible","Nonlinear"}.contains(mode),"Unknown optimizer rule case");
  const bool nonlinear=mode=="Nonlinear";
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Optimizer rule fixture unavailable");
  MainWindow window; require(window.initialize(startup),"Optimizer rule runtime unavailable"); window.show(); acceptanceView(window); events();
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *area=window.findChild<QMdiArea *>("workspace"); auto *tree=window.findChild<QTreeWidget *>("Mission"); auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");
  require(source&&area&&tree&&messages,"Optimizer rule workspace missing");
  const auto report=files.filePath("rule results Δ.csv");
  QString setup="% Optimizer cardinality source sentinel Ω\nCreate Variable X Y CostA CostB Keep;\nX = 0;\nY = 0;\nCostA = 0;\nCostB = 0;\nKeep = 41;\nCreate Yukon First Second;\n";
  if (nonlinear) setup.replace("Create Variable X Y CostA CostB Keep;","Create Variable X Y CostA CostB Keep LowerCurve UpperCurve;");
  for (const auto &solver:QStringList{"First","Second"}) setup+=solver+".ShowProgress = false;\n"+solver+".ReportFile = '"+files.filePath(solver+".solver.txt")+"';\n"+solver+".FeasibilityTolerance = 0.00000001;\n"+solver+".OptimalityTolerance = 0.00000001;\n"+solver+".FunctionTolerance = 0.00000001;\n";
  setup+="Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nKeep = Keep + 1;\n";
  const QString objective="Minimize 'First objective' First(CostA);";
  QString first="Optimize 'First branch' First {ShowProgressWindow = false, ExitMode = SaveAndContinue}; % retain first header\nVary First(X = 0, {Perturbation = 0.000001, Lower = -10, Upper = 10, MaxStep = 10});\n'First cost' CostA = (X - 2)^2;\n"+objective+"\nEndOptimize; % retain first footer\n";
  QString second="Optimize 'Second branch' Second {ShowProgressWindow = false, ExitMode = SaveAndContinue};\nVary Second(Y = 0, {Perturbation = 0.000001, Lower = -10, Upper = 10, MaxStep = 10});\nCostB = (Y + 1)^2;\nMinimize 'Second objective' Second(CostB);\nEndOptimize;\nReport 'Witness' Values X Y CostA CostB Keep;\n% retain tail Ω\n";
  if (nonlinear) {
   first.replace("X = 0","X = 2"); first.replace(objective,"LowerCurve = X^2;\nUpperCurve = (X + 1)^2;\n"+objective);
   second.replace("Values X Y CostA CostB Keep;","Values X Y CostA CostB Keep LowerCurve UpperCurve;");
  }
  const QString baseline=setup+first+second; source->setPlainText(baseline); require(window.buildScript(),"Optimizer rule baseline failed to build");
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"); auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
  save("baseline.script",baseline.toUtf8()); QJsonArray observations,reports;
  auto run=[&](const QString &phase,double expected,bool feasible) {
   const auto accepted=source->toPlainText(); const int undo=source->document()->availableUndoSteps(); acceptanceView(window);
   const auto status=window.runMission(); if (status!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString(); require(status==MainWindow::RunResult::Completed,"Optimizer rule execution did not complete");
   const auto data=read(report);
   // Keep failed numerical outputs too: QTemporaryDir is removed on failure.
   save(phase+".csv",data); save(phase+".script",accepted.toUtf8()); save(phase+".messages.txt",messages->toPlainText().toUtf8());
   for (const auto &solver:QStringList{"First","Second"}) { const auto file=files.filePath(solver+".solver.txt"); if (QFileInfo::exists(file)) save(phase+"."+solver+".solver.txt",read(file)); }
   std::cerr<<"OPTIMIZER_RULE_RESULT "<<phase.toStdString()<<' '<<data.constData()<<std::endl;
   const auto lines=QString::fromUtf8(data).trimmed().split('\n'); require(lines.size()==1,"Optimizer rule report did not contain one accepted result");
   const auto columns=lines.front().split(','); const int count=nonlinear ? 7:5; require(columns.size()==count,"Optimizer rule result lost columns");
   double values[7]; QJsonArray observed; for (int i=0;i<count;++i) { bool ok=false; values[i]=columns[i].trimmed().toDouble(&ok); require(ok&&std::isfinite(values[i]),"Optimizer rule result is not finite"); observed.append(values[i]); }
   require(feasible ? (values[0]>=3-2e-5&&values[0]<=4+2e-5) : std::abs(values[0]-expected)<2e-5,"Optimizer variable violates independent optimum or feasibility bounds");
   require(std::abs(values[1]+1)<2e-5&&std::abs(values[3])<2e-5&&values[4]==42,"Second independent optimizer or literal sentinel is wrong");
   require(std::abs(values[2]-std::pow(values[0]-2,2))<2e-5,"Optimizer cost differs from independently evaluated quadratic");
   if (!feasible) require(std::abs(values[2]-std::pow(expected-2,2))<2e-5,"Optimizer objective differs from independent constrained minimum");
   if (nonlinear) {
    require(std::abs(values[5]-values[0]*values[0])<2e-5&&std::abs(values[6]-std::pow(values[0]+1,2))<2e-5,"Reported nonlinear constraint functions differ from independent curves");
    const bool upper=phase=="nonlinear-upper"; const double lower=upper ? 0:9,limit=upper ? 4:25;
    require(values[5]>=lower-2e-5&&values[6]<=limit+2e-5,"Nonlinear constraint functions violate their simultaneous bounds");
    require(upper ? std::abs(values[6]-4)<2e-5 : std::abs(values[5]-9)<2e-5,"Expected nonlinear constraint was not active");
   }
   require(source->toPlainText()==accepted&&source->document()->availableUndoSteps()==undo,"Optimizer execution changed exact source/history");
   reports.append(QJsonObject{{"phase",phase},{"values",observed},{"feasibility_only",feasible},{"expected_x",expected},{"absolute_tolerance",2e-5}});
  };
  auto context=[&](const QString &label,const QString &name) {
   auto *tabs=window.findChild<QTabWidget *>(); for (int i=0;tabs&&i<tabs->count();++i) if (tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
   QTreeWidgetItem *target=nullptr; for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->text(0).endsWith(label)) { require(!target,"Optimizer tree label is ambiguous"); target=*it; }
   require(target,"Optimizer rule tree item missing"); tree->expandAll(); tree->setCurrentItem(target); tree->scrollToItem(target); events(); acceptanceView(window);
   std::exception_ptr failure; bool visited=false;
   QTimer::singleShot(0,&window,[&] { try {
    auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); require(menu,"Actual optimizer command popup missing"); QAction *choice=nullptr;
    for (auto *a:menu->actions()) if (a->text()==name) choice=a;
    require(choice&&choice->isEnabled(),"Optimizer popup action unavailable"); menu->setActiveAction(choice); acceptanceView(*menu); visited=true;
    QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter);
   } catch (...) { failure=std::current_exception(); if (auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget())) menu->close(); } });
   tree->customContextMenuRequested(tree->visualItemRect(target).center()); events(); if (failure) std::rethrow_exception(failure); require(visited,"Optimizer popup not exercised");
   auto *child=area->activeSubWindow(); require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Actual optimizer command editor missing"); child->showNormal(); child->move(0,0); child->resize(area->viewport()->size()); area->setActiveSubWindow(child); child->raise(); events(); acceptanceView(*child); return child;
  };
  auto apply=[&](QMdiSubWindow *child) { acceptanceView(*child); auto *box=child->findChild<QDialogButtonBox *>(); require(box&&box->button(QDialogButtonBox::Apply)->isEnabled(),"Optimizer Apply unavailable"); box->button(QDialogButtonBox::Apply)->click(); events(); };
  QMdiSubWindow *child=nullptr; QString draft,expected;
  if (mode=="Unique") {
   run("separate-sequences",2,false);
   // Check the chooser where another objective is legal, then independently
   // exercise native duplicate validation through the raw script fallback.
   source->setPlainText(QString(baseline).replace(objective+"\n",QString()));
   require(window.buildScript(),"Objective-free optimizer context did not build");
   child=context("First cost","Insert after…");
   auto *templates=child->findChild<QComboBox *>("commandTemplate"); require(templates&&templates->findText("Minimize")>=0,"Minimize insertion template missing"); templates->showPopup(); auto *choices=templates->view(); choices->setCurrentIndex(templates->model()->index(templates->findText("Minimize"),0)); acceptanceView(*choices); QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(choices,&enter); events();
   require(child->findChild<QPlainTextEdit *>("commandSource")->toPlainText().startsWith("Minimize "),"Legal Minimize template did not activate");
   dynamic_cast<CommandEditor *>(child->widget())->discardChanges(); require(child->close(),"Unused legal objective draft did not close"); events();
   source->setPlainText(baseline); require(window.buildScript(),"Accepted optimizer source did not rebuild");
   const int undo=source->document()->availableUndoSteps(); child=context("First objective","Insert before…");
   templates=child->findChild<QComboBox *>("commandTemplate");
   require(templates&&templates->findText("Minimize")<0,"Duplicate Minimize was offered by the chooser");
   child->findChild<QPlainTextEdit *>("commandSource")->setPlainText("Minimize First(CostA);"); events();
   auto *optimizer=child->findChild<QLineEdit *>("commandField_Solver"),*objectiveField=child->findChild<QLineEdit *>("commandField_Objective"); require(optimizer&&objectiveField&&optimizer->isVisible()&&objectiveField->isVisible(),"Minimize typed fields not visible"); optimizer->setText("First"); objectiveField->setText("CostA"); events(); acceptanceView(*child);
   auto *preview=child->findChild<QPlainTextEdit *>("commandSource"); require(preview&&preview->toPlainText()=="Minimize First(CostA);","Minimize template or fields changed the insertion draft"); draft=preview->toPlainText(); apply(child);
   auto *panel=dynamic_cast<CommandEditor *>(child->widget()); auto *status=child->findChild<QLabel *>("commandStatus");
   require(panel&&panel->hasChanges()&&status&&status->text().contains("Message Window")&&messages->toPlainText().contains("only allowed one Minimize command"),"Second Minimize was accepted or lacked the specific uniqueness diagnostic");
   require(preview->toPlainText()==draft&&source->toPlainText()==baseline&&source->document()->availableUndoSteps()==undo&&window.missionSnapshot().sourceScript==baseline,"Rejected duplicate Minimize lost draft or changed accepted source/history/model");
   save("rejected-insertion.txt",draft.toUtf8()); save("rejection.txt",status->text().toUtf8()); save("rejection-messages.txt",messages->toPlainText().toUtf8()); observations.append(QJsonObject{{"second_minimize_rejected",true},{"draft_retained",true},{"source_history_model_retained",true},{"diagnostic",status->text()}});
   preview->setPlainText("CostA = CostA;"); draft=preview->toPlainText(); expected=QString(baseline).replace(objective,draft+"\n"+objective);
  } else {
   child=context("First branch","Edit command…"); auto *preview=child->findChild<QPlainTextEdit *>("commandSource"); require(preview&&preview->toPlainText().contains(objective),"First optimizer branch preview lost its objective");
   const QString constraints=nonlinear ? "NonlinearConstraint 'Lower bound' First(LowerCurve >= 9);\nNonlinearConstraint 'Upper bound' First(UpperCurve <= 25);\n" : "NonlinearConstraint 'Lower bound' First(X >= 3);\nNonlinearConstraint 'Upper bound' First(X <= 4);\n";
   draft=preview->toPlainText(); draft.replace(objective,constraints+(mode=="Feasible" ? QString():objective)); preview->setPlainText(draft); expected=QString(baseline).replace(objective,constraints+(mode=="Feasible" ? QString():objective));
   observations.append(QJsonObject{{"two_constraints",true},{"lower_bound",nonlinear ? 9:3},{"upper_bound",nonlinear ? 25:4},{"first_objectives",mode=="Feasible" ? 0:1}});
  }
  require(source->toPlainText()==baseline&&dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Optimizer branch draft escaped Apply"); apply(child);
  require(source->toPlainText()==expected&&!dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Optimizer Apply differs from exact independent source oracle");
  const int steps=source->document()->availableUndoSteps(); apply(child); require(source->toPlainText()==expected&&source->document()->availableUndoSteps()==steps,"Repeated optimizer Apply changed source/history"); require(child->close(),"Clean optimizer editor did not close"); events();
  source->undo(); require(source->toPlainText()==baseline&&window.buildScript(),"Optimizer Undo did not restore exact source/model"); source->redo(); require(source->toPlainText()==expected&&window.buildScript(),"Optimizer Redo did not restore exact source/model");
  const auto saved=files.filePath("Optimizer rules ü.script"); require(window.saveScriptTo(saved)&&read(saved)==expected.toUtf8()&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==expected,"Optimizer Unicode persistence lost exact source");
  run(mode=="Unique" ? "recovered" : mode.toLower(),mode=="Unique" ? 2:3,mode=="Feasible");
  if (mode=="Multiple"||nonlinear) {
   // Reverse which inequality binds: ignoring either one must fail a result
   // assertion, rather than merely proving that two statements were accepted.
   for (const auto &edit:QList<QPair<QString,QString>>{{"Lower bound","0"},{"Upper bound",nonlinear ? "4":"1"}}) {
    child=context(edit.first,"Edit command…"); auto *right=child->findChild<QLineEdit *>("commandField_Right side");
    require(right&&right->isVisible(),"Constraint bound control not visible"); right->setText(edit.second); events(); acceptanceView(*child); apply(child);
    require(!dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Constraint active-bound reversal did not apply"); require(child->close(),"Constraint editor did not close"); events();
   }
   const auto upper=nonlinear ? QString(expected).replace("First(LowerCurve >= 9)","First(LowerCurve >= 0)").replace("First(UpperCurve <= 25)","First(UpperCurve <= 4)") : QString(expected).replace("First(X >= 3)","First(X >= 0)").replace("First(X <= 4)","First(X <= 1)");
   require(source->toPlainText()==upper,"Two-constraint reversal changed unrelated source"); source->undo(); source->undo(); require(source->toPlainText()==expected&&window.buildScript(),"Two constraint Undo did not restore exact source/model"); source->redo(); source->redo(); require(source->toPlainText()==upper&&window.buildScript(),"Two constraint Redo did not restore exact source/model");
   require(window.saveScriptTo(saved)&&read(saved)==upper.toUtf8()&&window.loadScript(saved)&&window.buildScript(),"Reversed constraint persistence changed source"); run(nonlinear ? "nonlinear-upper":"multiple-upper",1,false);
   observations.append(QJsonObject{{"active_bound_reversed",true},{"lower_bound",0},{"upper_bound",nonlinear ? 4:1},{"expected_x",1}});
  }
  const QJsonObject proof{{"checkpoint","optimizer-rule-"+mode},{"mode",mode},{"outcome","passed"},{"numeric_sentinel",42},{"observations",observations},{"reports",reports},{"facets",QJsonArray{"actual-mission-tree-popup-editor","pending-apply-and-rollback","exact-source-history-and-unicode","independent-quadratic-and-bound-oracles","separate-native-optimizer-sequences"}},{"scope","Selected Yukon one/two objective, multiple simultaneous scalar inequalities or zero-objective feasibility through actual command editors. No arbitrary branch/optimizer/input/scientific domains, native OS input, hardware/platform/stress/current private NASA corpus or full current-build qualification."}};
  auto record=proof; if (nonlinear) { auto facets=record["facets"].toArray(); facets.append("nonlinear-curve-results-and-active-bound-reversal"); record["facets"]=facets; record["scope"]="Two selected nonlinear constraint functions X^2 and (X+1)^2 with independent global optima3/1 and reversed active bounds, through actual command editors. No arbitrary nonlinear functions/regimes/solvers, native OS input, hardware/platform/stress/current NASA corpus or full current-build qualification."; }
  save("proof.json",QJsonDocument(record).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(record).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
