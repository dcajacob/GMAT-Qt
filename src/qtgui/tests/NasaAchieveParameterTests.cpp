#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
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
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); QApplication::processEvents(); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Achieve parameter artifact missing"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Achieve parameter artifact write failed"); }

int main(int argc,char **argv)
{
 QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("NasaAchieveParameter");
 try {
  require(argc==2&&acceptanceWidgetPlatform(),"Supply startup for Achieve parameter case");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Achieve parameter fixture unavailable");
  MainWindow window; require(window.initialize(startup),"Achieve parameter runtime unavailable"); window.show(); acceptanceView(window); events();
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *area=window.findChild<QMdiArea *>("workspace");
  auto *tree=window.findChild<QTreeWidget *>("Mission"); auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");
  require(source&&area&&tree&&messages,"Achieve parameter workspace missing");
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
  auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
  const auto report=files.filePath("parameter results Δ.csv");
  const QString goal="Achieve 'Goal' DC(Quantity = 5, {Tolerance = 0.000001});";
  const QString baseline="% Preserve Achieve scalar goals Ω\nCreate Variable Control Quantity Keep;\nControl = 1;\nQuantity = 0;\nKeep = 41;\nCreate Array Grid[1,2];\nCreate String Note;\nNote = 'retained';\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7;\nSat.VZ = 0;\nCreate DifferentialCorrector DC;\nDC.ShowProgress = false;\nDC.MaximumIterations = 20;\nDC.ReportFile = '"+files.filePath("corrector.txt")+"';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nKeep = Keep + 1;\nTarget 'Branch' DC {ShowProgressWindow = false};\nVary 'Control' DC(Control = 1, {Perturbation = 0.0001, Lower = -10, Upper = 10, MaxStep = 10});\nQuantity = 2 * Control + 1;\nSat.X = 1000 + 3 * Control;\n"+goal+"\nEndTarget;\nReport 'Witness' Values Control Quantity Sat.EarthMJ2000Eq.X Keep;\n% preserve tail Ω\n";
  source->setPlainText(baseline); require(window.buildScript(),"Achieve parameter baseline failed to build"); save("baseline.script",baseline.toUtf8());
  QJsonArray reports;
  auto run=[&](const QString &phase,double optimum,double expectedQuantity,double expectedProperty) {
   const auto accepted=source->toPlainText(); const int undo=source->document()->availableUndoSteps(); acceptanceView(window);
   const auto result=window.runMission(); save(phase+".messages.txt",messages->toPlainText().toUtf8());
   if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();
   require(result==MainWindow::RunResult::Completed,"Achieve parameter mission failed");
   const auto data=read(report); save(phase+".csv",data); save(phase+".script",accepted.toUtf8());
   if (QFileInfo::exists(files.filePath("corrector.txt"))) save(phase+".corrector.txt",read(files.filePath("corrector.txt")));
   std::cerr<<"ACHIEVE_PARAMETER_RESULT "<<phase.toStdString()<<' '<<data.constData()<<std::endl;
   const auto lines=QString::fromUtf8(data).trimmed().split('\n'); require(lines.size()==1,"Achieve parameter report needs one accepted row");
   const auto columns=lines.front().split(','); require(columns.size()==4,"Achieve parameter report lost columns");
   double value[4]; QJsonArray numbers;
   for (int i=0;i<4;++i) { bool ok=false; value[i]=columns[i].trimmed().toDouble(&ok); require(ok&&std::isfinite(value[i]),"Achieve parameter report contains nonfinite value"); numbers.append(value[i]); }
   require(std::abs(value[0]-optimum)<2e-8,"Achieve parameter differs from independent optimum");
   require(std::abs(value[1]-expectedQuantity)<5e-8&&std::abs(value[2]-expectedProperty)<5e-8&&value[3]==42,"Achieve parameter quantity/property/sentinel differs from analytic result");
   require(std::abs(value[1]-(2*value[0]+1))<5e-10&&std::abs(value[2]-(1000+3*value[0]))<5e-10,"Achieve parameter quantities differ from independent formulas");
   require(source->toPlainText()==accepted&&source->document()->availableUndoSteps()==undo,"Achieve parameter Run changed source/history");
   const double desired=phase=="variable" ? 9:1006,achieved=value[phase=="variable" ? 1:2];
   require(std::abs(achieved-desired)<=1e-9,"Achieve goal residual exceeds requested tolerance");
   reports.append(QJsonObject{{"phase",phase},{"values",numbers},{"expected_control",optimum},{"expected_quantity",expectedQuantity},{"expected_property",expectedProperty},{"desired",desired},{"achieved",achieved},{"residual",achieved-desired},{"requested_tolerance",1e-9}});
  };
  QString accepted=baseline;
  for (const auto &phase:QStringList{"variable","property"}) {
  const bool propertyGoal=phase=="property"; const QString selected=propertyGoal ? "Sat.EarthMJ2000Eq.X":"Quantity";
  const QString previous=accepted;
  save(phase+".before.script",previous.toUtf8());
  auto *tabs=window.findChild<QTabWidget *>(); for (int i=0;tabs&&i<tabs->count();++i) if (tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
  QTreeWidgetItem *item=nullptr; for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole+1)=="Achieve") { require(!item,"Achieve parameter tree selection ambiguous"); item=*it; }
  require(item,"Achieve parameter tree item missing"); tree->expandAll(); tree->setCurrentItem(item); tree->scrollToItem(item); events(); acceptanceView(window); tree->itemDoubleClicked(item,0); events();
  auto *child=area->activeSubWindow(); require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Achieve parameter command editor missing");
  child->showNormal(); child->move(0,0); child->resize(area->viewport()->size()); area->setActiveSubWindow(child); child->raise(); events(); acceptanceView(*child);
  auto panel=[&] { return dynamic_cast<CommandEditor *>(child->widget()); };
  auto field=[&] { return child->findChild<QLineEdit *>("commandField_Goal"); };
  auto buttons=[&] { return child->findChild<QDialogButtonBox *>(); };
  auto *choose=child->findChild<QPushButton *>("commandChoose_Goal");
  auto *valueField=child->findChild<QLineEdit *>("commandField_Value");
  auto *toleranceField=child->findChild<QLineEdit *>("commandField_Tolerance");
  require(valueField&&valueField->isVisible()&&toleranceField&&toleranceField->isVisible(),"Achieve value/tolerance controls not visible");
  require(field()&&field()->isVisible()&&field()->text()=="Quantity"&&choose&&choose->isVisible()&&buttons(),"Achieve parameter typed controls not visible");
  auto pick=[&](bool accept) {
   std::exception_ptr failure; bool visited=false;
   QTimer::singleShot(0,child,[&] { auto *dialog=dynamic_cast<ReportParameterDialog *>(QApplication::activeModalWidget());
    try {
     require(dialog,"Achieve parameter picker missing"); visited=true; acceptanceView(*dialog);
     auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry"); auto *buttons=dialog->findChild<QDialogButtonBox *>();
     require(entry&&buttons&&entry->findText("Note")<0,"Achieve picker offers String");
     for (const auto &invalid:QStringList{"17","Note","Grid","Grid(2,1)","nan"}) { entry->setEditText(invalid); events(); require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Achieve picker accepts invalid numeric object"); }
     auto *object=dialog->findChild<QComboBox *>("reportPropertyObject"); auto *property=dialog->findChild<QComboBox *>("reportPropertyType");
     auto *dependency=dialog->findChild<QComboBox *>("reportPropertyDependency"); auto *use=dialog->findChild<QPushButton *>("reportUseReference");
     require(object&&property&&dependency&&use,"Achieve parameter controls missing"); object->setCurrentText("Sat"); events();
     require(property->findText("UTCGregorian")<0&&property->findText("X")>=0,"Achieve parameter list contains nonnumeric fields");
     if (propertyGoal) { property->setCurrentText("X"); dependency->setCurrentText("EarthMJ2000Eq"); events(); use->click(); }
     else entry->setCurrentText("Quantity");
     events(); acceptanceView(*dialog);
     require(dialog->selection()==QStringList{selected}&&buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Achieve scalar-parameter selection wrong");
     buttons->button(accept ? QDialogButtonBox::Ok:QDialogButtonBox::Cancel)->click();
    } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   }); choose->click(); if (failure) std::rethrow_exception(failure); require(visited,"Achieve parameter picker unvisited"); events(); acceptanceView(*child);
  };
  const int steps=source->document()->availableUndoSteps(); pick(false);
  require(field()->text()=="Quantity"&&!panel()->hasChanges()&&source->toPlainText()==previous&&source->document()->availableUndoSteps()==steps,"Achieve parameter Cancel changed source/draft/history");
  pick(true);
  // Selecting a goal rebuilds the typed form; reacquire its sibling fields.
  valueField=child->findChild<QLineEdit *>("commandField_Value"); toleranceField=child->findChild<QLineEdit *>("commandField_Tolerance");
  valueField->setText(propertyGoal ? "1006":"9"); toleranceField->setText("0.000000001"); events(); acceptanceView(*child);
  require(field()->text()==selected&&panel()->hasChanges()&&source->toPlainText()==previous&&source->document()->availableUndoSteps()==steps,"Achieve parameter selection escaped pending state");
  const QString statement="Achieve 'Goal' DC("+selected+" = "+(propertyGoal ? "1006":"9")+", {Tolerance = 0.000000001});";
  const auto expected=propertyGoal ? QString(previous).replace("Achieve 'Goal' DC(Quantity = 9, {Tolerance = 0.000000001});",statement):QString(previous).replace(goal,statement);
  acceptanceView(*child); buttons()->button(QDialogButtonBox::Apply)->click(); events();
  require(source->toPlainText()==expected&&!panel()->hasChanges(),"Achieve parameter Apply changed unrelated source or lost typed goal");
  const int appliedSteps=source->document()->availableUndoSteps(); buttons()->button(QDialogButtonBox::Apply)->click(); events();
  require(source->toPlainText()==expected&&source->document()->availableUndoSteps()==appliedSteps,"Achieve parameter repeated Apply changed source/history");
  require(child->close(),"Achieve parameter clean panel did not close"); events();
  source->undo(); require(source->toPlainText()==previous&&window.buildScript(),"Achieve parameter Undo did not restore exact source/model");
  source->redo(); require(source->toPlainText()==expected&&window.buildScript(),"Achieve parameter Redo did not restore exact source/model");
  const auto saved=files.filePath(phase+" Goal Δ.script"),saveAs=files.filePath(phase+" Goal copy ü.script");
  require(window.saveScriptTo(saved)&&read(saved)==expected.toUtf8()&&window.saveScriptTo(saveAs)&&read(saveAs)==expected.toUtf8()&&window.loadScript(saveAs)&&window.buildScript()&&source->toPlainText()==expected,"Achieve parameter Unicode Save/Save As/reopen changed exact source");
  run(phase,propertyGoal ? 2:4,propertyGoal ? 5:9,propertyGoal ? 1006:1012); accepted=expected;
  }
  const QJsonObject proof{{"checkpoint","achieve-scalar-parameter"},{"outcome","passed"},{"reports",reports},{"requested_tolerance",1e-9},{"numeric_sentinel",42},
   {"facets",QJsonArray{"actual-mission-tree-achieve-editor","numeric-goal-picker-cancel-filtering","pending-apply-idempotence","exact-source-undo-redo-unicode-persistence","independent-linear-goals-and-requested-tolerance"}},
   {"scope","Selected user variable and same-frame spacecraft X Achieve goals, analytic Control4/2 and linear quantity/property checks with actual goal residual within1e-9. No frame transformation, propagation science, arbitrary goal/tolerance/solver domains, native OS input, hardware/platform/stress/current private NASA corpus or full current-build qualification."}};
  save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
