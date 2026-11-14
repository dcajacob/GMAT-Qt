#include "Vf13adAcceptance.hpp"
#include "CommandEditor.hpp"
#include "ReportParameterDialog.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidgetItemIterator>
using namespace vfaccept;
static void reveal(QWidget *w)
{
 require(w,"Optimize Vary control absent");
 for(QWidget *p=w;p;p=p->parentWidget())if(auto *s=qobject_cast<QScrollArea *>(p))s->ensureWidgetVisible(w);
 events();require(w->isVisible()&&w->isEnabled(),"Optimize Vary control unreachable");
}
int main(int argc,char **argv)
{
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
 app.setOrganizationName("GMATTests");app.setApplicationName("NasaVaryOptimizeProperty");
 try {
  require(argc==2,"Supply Optimize Vary property startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());Options options;options.initial=1;
  const QString property="Sat.EarthMJ2000Eq.X";
  auto baseline=rig.script(options);
  baseline.replace("Create ReportFile Trials Final;","Create Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7;\nSat.VZ = 0;\nCreate String Note;\nNote = 'retained';\nCreate Array Grid[1,2];\nCreate ReportFile Trials Final;");
  for(const auto &pair:QList<QPair<QString,QString>>{{"Cost = (X - 3)^2;","Cost = ("+property+" - 3)^2;"},{"Report Trials X Cost Keep;","Report Trials "+property+" Cost Keep;"},{"NonlinearConstraint Opt(X <= 2);","NonlinearConstraint Opt("+property+" <= 2);"},{"Report Final X Cost Keep;","Report Final "+property+" Cost Keep;"}}){require(baseline.count(pair.first)==1,"Optimize Vary reused fixture anchor differs");baseline.replace(pair.first,pair.second);}
  const auto original=vary(options),changed=QString(original).replace("(X =","("+property+" =");
  require(original!=changed&&baseline.count(original)==1,"Optimize Vary literal statement ambiguous");
  rig.source->setPlainText(baseline);rig.save("initial.script",baseline.toUtf8());require(rig.window.buildScript(),"Optimize Vary property baseline failed");
  auto configured=[&]{auto *sat=Moderator::Instance()->GetConfiguredObject("Sat");auto *x=Moderator::Instance()->GetConfiguredObject("X");require(sat&&x,"Optimize Vary configured objects absent");return QJsonArray{sat->GetRealParameter("X"),x->GetRealParameter("Value")};};
  require(configured()==QJsonArray{7000,-4},"Optimize Vary baseline model differs");
  QTreeWidgetItem *entry=nullptr;
  for(QTreeWidgetItemIterator it(rig.mission);*it;++it)if((*it)->text(0).contains("VF control")){require(!entry,"Optimize Vary label ambiguous");entry=*it;}
  require(entry,"Optimize Vary actual mission node absent");rig.mission->expandAll();rig.mission->scrollToItem(entry);rig.mission->setCurrentItem(entry);events();rig.mission->itemDoubleClicked(entry,0);events();
  QPointer<QMdiSubWindow> child=rig.area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Optimize Vary actual MDI absent");
  child->showNormal();child->move(0,0);child->resize(rig.area->viewport()->size());events();acceptanceView(*child);
  auto panel=[&]{auto *p=child?dynamic_cast<CommandEditor *>(child->widget()):nullptr;require(p,"Optimize Vary owner absent");return p;};
  auto *field=child->findChild<QLineEdit *>("commandField_Variable");auto *choose=child->findChild<QPushButton *>("commandChoose_Variable");reveal(field);reveal(choose);
  require(field->text()=="X","Optimize Vary original control differs");
  for(const auto &name:QStringList{"Lower","Upper"}){auto *w=child->findChild<QLineEdit *>("commandField_"+name);require(w&&!w->isEnabled()&&w->toolTip().contains("does not use"),"Optimize Vary backend capability bound policy differs");}
  const int undo=rig.source->document()->availableUndoSteps();QJsonArray visits;
  for(bool accept:{false,true}) {
   std::exception_ptr failure;bool visited=false;
   QTimer::singleShot(0,child,[&]{auto *dialog=dynamic_cast<ReportParameterDialog *>(QApplication::activeModalWidget());
    try {
     require(dialog,"Optimize Vary actual property picker absent");visited=true;acceptanceView(*dialog);
     auto *combo=dialog->findChild<QComboBox *>("reportParameterEntry");auto *buttons=dialog->findChild<QDialogButtonBox *>();require(combo&&buttons&&combo->findText("Note")<0,"Optimize Vary picker type filter differs");
     for(const auto &bad:QStringList{"17","Note","Grid","Grid(2,1)","Sat.ElapsedSecs"}){combo->setEditText(bad);events();require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Optimize Vary picker accepted invalid/readonly control");}
     auto *object=dialog->findChild<QComboBox *>("reportPropertyObject"),*type=dialog->findChild<QComboBox *>("reportPropertyType"),*dependency=dialog->findChild<QComboBox *>("reportPropertyDependency");
     require(object&&type&&dependency,"Optimize Vary picker property controls absent");object->setCurrentText("Sat");events();require(type->findText("X")>=0&&type->findText("ElapsedSecs")<0,"Optimize Vary picker writable-property census differs");type->setCurrentText("X");dependency->setCurrentText("EarthMJ2000Eq");dialog->findChild<QPushButton *>("reportUseReference")->click();events();
     require(dialog->selection()==QStringList{property}&&buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Optimize Vary chosen property differs");acceptanceView(*dialog);
     visits.append(QJsonObject{{"accept",accept},{"selection",property},{"invalid_rejections",5},{"readonly_filtered",true}});QTest::mouseClick(buttons->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel),Qt::LeftButton);
    }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}
   });QTest::mouseClick(choose,Qt::LeftButton);events();if(failure)std::rethrow_exception(failure);require(visited,"Optimize Vary picker unvisited");
   require(rig.source->toPlainText()==baseline&&rig.source->document()->availableUndoSteps()==undo&&configured()==QJsonArray{7000,-4},"Optimize Vary picker escaped source/model/history isolation");
   require(field->text()==(accept?property:"X")&&panel()->hasChanges()==accept,"Optimize Vary Cancel/OK draft state differs");acceptanceView(*child);
  }
  auto *preview=child->findChild<QPlainTextEdit *>("commandSource");require(preview&&preview->toPlainText()==changed,"Optimize Vary picker changed other statement fields");
  const auto expected=QString(baseline).replace(original,changed);auto click=[&]{auto *box=panel()->findChild<QDialogButtonBox *>();auto *apply=box?box->button(QDialogButtonBox::Apply):nullptr;reveal(apply);QTest::mouseClick(apply,Qt::LeftButton);events();};
  click();require(rig.source->toPlainText()==expected&&!panel()->hasChanges(),"Optimize Vary Apply differs from exact property-only source");const int applied=rig.source->document()->availableUndoSteps();click();require(rig.source->toPlainText()==expected&&rig.source->document()->availableUndoSteps()==applied,"Optimize Vary repeated Apply changed source/history");rig.closePanels();
  rig.source->undo();require(rig.source->toPlainText()==baseline&&rig.window.buildScript()&&configured()==QJsonArray{7000,-4},"Optimize Vary Undo failed exact source/model");rig.source->redo();require(rig.source->toPlainText()==expected&&rig.window.buildScript()&&configured()==QJsonArray{7000,-4},"Optimize Vary Redo failed exact source/model");
  rig.save("accepted.script",expected.toUtf8());rig.save("picker-visits.json",QJsonDocument(visits).toJson());QJsonArray native;
  for(const auto &phase:QStringList{"gui","reopened"}) {
   if(phase=="reopened"){const auto file=rig.files.filePath("Optimize Vary property Ω.script"),copy=rig.files.filePath("Optimize Vary property copy Δ.script");require(rig.window.saveScriptTo(file)&&read(file)==expected.toUtf8()&&rig.window.saveScriptTo(copy)&&read(copy)==expected.toUtf8()&&rig.window.loadScript(copy)&&rig.window.buildScript()&&rig.source->toPlainText()==expected,"Optimize Vary Unicode Save/SaveAs/reopen differs");}
   rig.run(phase,options);
   auto *command=Moderator::Instance()->GetFirstCommand();while(command&&!command->IsOfType("Optimize"))command=command->GetNext();auto *branch=dynamic_cast<SolverBranchCommand *>(command);auto *solver=branch?dynamic_cast<Solver *>(branch->GetClone(0)):nullptr;require(solver,"Optimize Vary native clone absent");
   const auto variables=solver->GetStringArrayParameter(solver->GetParameterID("Variables"));auto *sat=Moderator::Instance()->GetInternalObject("Sat"),*unused=Moderator::Instance()->GetInternalObject("X");require(sat&&unused&&variables==StringArray{property.toStdString()}&&solver->GetIntegerParameter(solver->GetParameterID("NumberOfVariables"))==1,"Optimize Vary native control identity/count differs");
   const double physical=sat->GetRealParameter("X"),sentinel=unused->GetRealParameter("Value");require(std::abs(physical-2)<1e-6&&sentinel==-4&&configured()==QJsonArray{7000,-4},"Optimize Vary selected property/unused variable/configured state differs");native.append(QJsonObject{{"phase",phase},{"variable",property},{"type",QString::fromStdString(solver->GetTypeName())},{"count",1},{"status",solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"))},{"physical",physical},{"scaled",solver->GetSolverVariable(0)},{"unused_variable",sentinel},{"configured",configured()}});
  }
  rig.save("native-controls.json",QJsonDocument(native).toJson());
  const QJsonObject proof{{"checkpoint","vary-optimize-property-subdialog"},{"outcome","passed"},{"context","Optimize"},{"backend","VF13ad"},{"property",property},{"picker_visits",visits.size()},{"invalid_rejections",10},{"phases",2},{"facets",QJsonArray{"actual-optimize-vary-writable-property-picker","picker-cancel-ok-invalid-readonly-filter-and-pending-isolation","property-only-apply-idempotence-history-unicode-save-reopen","independent-physical-probes-constrained-quadratic-and-native-control-identity","unused-variable-and-configured-state-preserved"}},{"scope","One VF13ad Optimize Vary spacecraft Cartesian X control in its own EarthMJ2000Eq frame. Fixed initial1,perturbation.01,analytic active-bound optimum2/cost1 and actual native control identity. Other subdialogs/parameters/backends/problems,frame/propagation science,solver text-report truth,native OS input/GPU/hardware/platform/stress/current NASA/full newest-build remain open."}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
