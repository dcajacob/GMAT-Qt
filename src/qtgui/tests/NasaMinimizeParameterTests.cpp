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
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Minimize parameter artifact missing"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Minimize parameter artifact write failed"); }

int main(int argc,char **argv)
{
 QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("NasaMinimizeParameter");
 try {
  require(argc==2&&acceptanceWidgetPlatform(),"Supply startup for Minimize parameter case");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Minimize parameter fixture unavailable");
  MainWindow window; require(window.initialize(startup),"Minimize parameter runtime unavailable"); window.show(); acceptanceView(window); events();
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *area=window.findChild<QMdiArea *>("workspace");
  auto *tree=window.findChild<QTreeWidget *>("Mission"); auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");
  require(source&&area&&tree&&messages,"Minimize parameter workspace missing");
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
  auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
  const auto report=files.filePath("parameter results Δ.csv");
  const QString objective="Minimize 'Objective' Opt(Cost);";
  const QString baseline="% Preserve scalar-property objective Ω\nCreate Variable Control Cost Keep;\nControl = 1;\nCost = 0;\nKeep = 41;\nCreate Array Grid[1,2];\nCreate String Note;\nNote = 'retained';\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7;\nSat.VZ = 0;\nCreate Yukon Opt;\nOpt.ShowProgress = false;\nOpt.ReportFile = '"+files.filePath("optimizer.txt")+"';\nOpt.OptimalityTolerance = 0.00000001;\nOpt.FunctionTolerance = 0.00000001;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nKeep = Keep + 1;\nOptimize 'Branch' Opt {ShowProgressWindow = false};\nVary 'Control' Opt(Control = 1, {Perturbation = 0.000001, Lower = 0, Upper = 5, MaxStep = 5});\nCost = (Control - 2)^2;\nSat.X = 1000 + (Control - 3)^2;\n"+objective+"\nEndOptimize;\nReport 'Witness' Values Control Cost Sat.EarthMJ2000Eq.X Keep;\n% preserve tail Ω\n";
  source->setPlainText(baseline); require(window.buildScript(),"Minimize parameter baseline failed to build"); save("baseline.script",baseline.toUtf8());
  QJsonArray reports;
  auto run=[&](const QString &phase,double optimum,double expectedCost,double expectedProperty) {
   const auto accepted=source->toPlainText(); const int undo=source->document()->availableUndoSteps(); acceptanceView(window);
   const auto result=window.runMission(); save(phase+".messages.txt",messages->toPlainText().toUtf8());
   if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();
   require(result==MainWindow::RunResult::Completed,"Minimize parameter mission failed");
   const auto data=read(report); save(phase+".csv",data); save(phase+".script",accepted.toUtf8());
   if (QFileInfo::exists(files.filePath("optimizer.txt"))) save(phase+".optimizer.txt",read(files.filePath("optimizer.txt")));
   std::cerr<<"MINIMIZE_PARAMETER_RESULT "<<phase.toStdString()<<' '<<data.constData()<<std::endl;
   const auto lines=QString::fromUtf8(data).trimmed().split('\n'); require(lines.size()==1,"Minimize parameter report needs one accepted row");
   const auto columns=lines.front().split(','); require(columns.size()==4,"Minimize parameter report lost columns");
   double value[4]; QJsonArray numbers;
   for (int i=0;i<4;++i) { bool ok=false; value[i]=columns[i].trimmed().toDouble(&ok); require(ok&&std::isfinite(value[i]),"Minimize parameter report contains nonfinite value"); numbers.append(value[i]); }
   require(std::abs(value[0]-optimum)<2e-5,"Minimize parameter differs from independent optimum");
   require(std::abs(value[1]-expectedCost)<2e-5&&std::abs(value[2]-expectedProperty)<2e-5&&value[3]==42,"Minimize parameter cost/property/sentinel differs from analytic result");
   require(std::abs(value[1]-std::pow(value[0]-2,2))<2e-5&&std::abs(value[2]-(1000+std::pow(value[0]-3,2)))<2e-5,"Minimize parameter quantities differ from independent formulas");
   require(source->toPlainText()==accepted&&source->document()->availableUndoSteps()==undo,"Minimize parameter Run changed source/history");
   reports.append(QJsonObject{{"phase",phase},{"values",numbers},{"expected_control",optimum},{"expected_cost",expectedCost},{"expected_property",expectedProperty},{"absolute_tolerance",2e-5}});
  };
  run("variable",2,0,1001);
  auto *tabs=window.findChild<QTabWidget *>(); for (int i=0;tabs&&i<tabs->count();++i) if (tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
  QTreeWidgetItem *item=nullptr; for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole+1)=="Minimize") { require(!item,"Minimize parameter tree selection ambiguous"); item=*it; }
  require(item,"Minimize parameter tree item missing"); tree->expandAll(); tree->setCurrentItem(item); tree->scrollToItem(item); events(); acceptanceView(window); tree->itemDoubleClicked(item,0); events();
  auto *child=area->activeSubWindow(); require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Minimize parameter command editor missing");
  child->showNormal(); child->move(0,0); child->resize(area->viewport()->size()); area->setActiveSubWindow(child); child->raise(); events(); acceptanceView(*child);
  auto panel=[&] { return dynamic_cast<CommandEditor *>(child->widget()); };
  auto field=[&] { return child->findChild<QLineEdit *>("commandField_Objective"); };
  auto buttons=[&] { return child->findChild<QDialogButtonBox *>(); };
  auto *choose=child->findChild<QPushButton *>("commandChoose_Objective");
  require(field()&&field()->isVisible()&&field()->text()=="Cost"&&choose&&choose->isVisible()&&buttons(),"Minimize parameter typed controls not visible");
  auto pick=[&](bool accept) {
   std::exception_ptr failure; bool visited=false;
   QTimer::singleShot(0,child,[&] { auto *dialog=dynamic_cast<ReportParameterDialog *>(QApplication::activeModalWidget());
    try {
     require(dialog,"Minimize parameter picker missing"); visited=true; acceptanceView(*dialog);
     auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry"); auto *buttons=dialog->findChild<QDialogButtonBox *>();
     require(entry&&buttons&&entry->findText("Note")<0,"Minimize picker offers String");
     for (const auto &invalid:QStringList{"17","Note","Grid","Grid(2,1)","nan"}) { entry->setEditText(invalid); events(); require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Minimize picker accepts invalid numeric object"); }
     auto *object=dialog->findChild<QComboBox *>("reportPropertyObject"); auto *property=dialog->findChild<QComboBox *>("reportPropertyType");
     auto *dependency=dialog->findChild<QComboBox *>("reportPropertyDependency"); auto *use=dialog->findChild<QPushButton *>("reportUseReference");
     require(object&&property&&dependency&&use,"Minimize property controls missing"); object->setCurrentText("Sat"); events();
     require(property->findText("UTCGregorian")<0&&property->findText("X")>=0,"Minimize property list contains nonnumeric fields");
     property->setCurrentText("X"); dependency->setCurrentText("EarthMJ2000Eq"); events(); use->click(); events(); acceptanceView(*dialog);
     require(dialog->selection()==QStringList{"Sat.EarthMJ2000Eq.X"}&&buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Minimize scalar-property selection wrong");
     buttons->button(accept ? QDialogButtonBox::Ok:QDialogButtonBox::Cancel)->click();
    } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   }); choose->click(); if (failure) std::rethrow_exception(failure); require(visited,"Minimize property picker unvisited"); events(); acceptanceView(*child);
  };
  const int steps=source->document()->availableUndoSteps(); pick(false);
  require(field()->text()=="Cost"&&!panel()->hasChanges()&&source->toPlainText()==baseline&&source->document()->availableUndoSteps()==steps,"Minimize property Cancel changed source/draft/history");
  pick(true); require(field()->text()=="Sat.EarthMJ2000Eq.X"&&panel()->hasChanges()&&source->toPlainText()==baseline&&source->document()->availableUndoSteps()==steps,"Minimize property selection escaped pending state");
  const auto expected=QString(baseline).replace(objective,"Minimize 'Objective' Opt(Sat.EarthMJ2000Eq.X);");
  acceptanceView(*child); buttons()->button(QDialogButtonBox::Apply)->click(); events();
  require(source->toPlainText()==expected&&!panel()->hasChanges(),"Minimize property Apply changed unrelated source or lost typed objective");
  const int appliedSteps=source->document()->availableUndoSteps(); buttons()->button(QDialogButtonBox::Apply)->click(); events();
  require(source->toPlainText()==expected&&source->document()->availableUndoSteps()==appliedSteps,"Minimize property repeated Apply changed source/history");
  require(child->close(),"Minimize property clean panel did not close"); events();
  source->undo(); require(source->toPlainText()==baseline&&window.buildScript(),"Minimize property Undo did not restore exact source/model");
  source->redo(); require(source->toPlainText()==expected&&window.buildScript(),"Minimize property Redo did not restore exact source/model");
  const auto saved=files.filePath("Objective Δ.script"),saveAs=files.filePath("Objective copy ü.script");
  require(window.saveScriptTo(saved)&&read(saved)==expected.toUtf8()&&window.saveScriptTo(saveAs)&&read(saveAs)==expected.toUtf8()&&window.loadScript(saveAs)&&window.buildScript()&&source->toPlainText()==expected,"Minimize property Unicode Save/Save As/reopen changed exact source");
  run("property",3,1,1000);
  const QJsonObject proof{{"checkpoint","minimize-scalar-property"},{"outcome","passed"},{"reports",reports},{"objective","Sat.EarthMJ2000Eq.X"},{"numeric_sentinel",42},
   {"facets",QJsonArray{"actual-mission-tree-command-editor","numeric-property-picker-cancel-filtering","pending-apply-idempotence","exact-source-undo-redo-unicode-persistence","independent-objective-changing-optimum"}},
   {"scope","One calculated spacecraft Cartesian component in its own EarthMJ2000Eq frame, with analytic Control optima2/3 and quantity/sentinel checks. No frame transformation, propagation science, arbitrary scalar properties/optimizer domains, native OS input, hardware/platform/stress/current private NASA corpus or full current-build qualification."}};
  save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
