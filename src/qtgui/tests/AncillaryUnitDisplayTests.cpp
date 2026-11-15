#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include "PropagationStopsDialog.hpp"
#include "ConditionDialog.hpp"
#include "ComparisonPanel.hpp"
#include "FolderRunDialog.hpp"
#include "ParameterInfo.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTableWidget>
#include <QTest>
using namespace formaccept;
static void reveal(QWidget *w) {
   require(w,"Ancillary unit control missing");
   for(auto *p=w;p;p=p->parentWidget()) {
      for(auto *tabs:w->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *s=qobject_cast<QScrollArea *>(p))s->ensureWidgetVisible(w);
   }
   events();require(w->isVisible(),"Ancillary unit control hidden");acceptanceView(*w);
}
static QJsonObject graph() {
   QJsonObject g;auto *m=Moderator::Instance();
   for(const auto &name:m->GetListOfObjects(Gmat::UNKNOWN_OBJECT))if(auto *o=m->GetConfiguredObject(name))g[QString::fromStdString(name)]=QString::fromStdString(o->GetGeneratingString(Gmat::SHOW_SCRIPT));return g;
}
int main(int argc,char **argv) {
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("AncillaryUnitDisplay");
   try {
      require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString source="% Ancillary unit display Δ\nCreate Spacecraft Sat;\nSat.NAIFId = -101291;\nSat.NAIFIdReferenceFrame = -911291;\nCreate Variable user;\nuser = 7;\nCreate Array A[2,2];\nCreate Propagator Prop;\nBeginMissionSequence;\n";
      const auto saved=rig.files.filePath("Units Ω.script");write(saved,source.toUtf8());require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Ancillary fixture failed");
      const auto native=graph();const int undo=rig.source->document()->availableUndoSteps();auto *info=ParameterInfo::Instance();const auto names=info->GetNamesOfParameters(),types=info->GetTypesOfParameters();QJsonArray observations;int failures=0;
      auto stable=[&] {require(graph()==native&&rig.source->toPlainText()==source&&read(saved)==source.toUtf8()&&rig.source->document()->availableUndoSteps()==undo&&info->GetNamesOfParameters()==names&&info->GetTypesOfParameters()==types,"Ancillary display changed native/source/history/inventory");};
      auto observe=[&](const QString &key,QWidget &owner,const QString &name,const QString &expected) {
         auto *label=owner.findChild<QLabel *>(name);QString actual;if(label){reveal(label);actual=label->text();}
         const bool passed=actual.contains(expected);failures+=!passed;observations.append(QJsonObject{{"key",key},{"control",name},{"expected",expected},{"actual",actual},{"passed",passed}});rig.save("unit-observations.json",QJsonDocument(observations).toJson());stable();
      };
      for(const auto &kind:QStringList{"time","distance","user","apsis"}) {
         const auto stop=kind=="time"?"Sat.ElapsedSecs = 60":kind=="distance"?"Sat.X = 7000":kind=="user"?"user = 9":"Sat.Periapsis";
         const auto statement="Propagate Prop(Sat) {"+QString(stop)+", StopTolerance = 0.001};";
         auto *editor=new CommandEditor(statement,false,{},[](const QString &){return QString();},{"Prop"},{"Sat"});auto *window=rig.area->addSubWindow(editor);window->show();window->showNormal();window->resize(rig.area->viewport()->size());events();
         const QString unit=kind=="time"?"s":kind=="distance"?"km":kind=="user"?"user-defined units":"dimensionless";
         observe(kind,*editor,"propagationParameterUnit","("+unit+")");observe(kind,*editor,"propagationValueUnit","("+(kind=="apsis"?QString("no goal"):unit)+")");observe(kind,*editor,"propagationToleranceUnit","("+unit+")");
         if(kind=="time") {
            auto *choice=editor->findChild<QComboBox *>("propagationUnits");reveal(choice);QTest::mouseClick(choice,Qt::LeftButton);QTest::keyClick(choice,Qt::Key_Home);QTest::keyClick(choice,Qt::Key_Down);QTest::keyClick(choice,Qt::Key_Return);events();require(choice->currentData().toString()=="ElapsedDays","Time unit selection failed");
            for(const auto &name:QStringList{"propagationParameterUnit","propagationValueUnit","propagationToleranceUnit"})observe("days-transition",*editor,name,"(days)");
         }
         editor->discardChanges();delete window;events();stable();
      }
      {
         const QString original="Propagate Prop(Sat) {Sat.ElapsedSecs = 60, Sat.X = 7000, Sat.Periapsis};";
         PropagationStopsDialog dialog(original,&rig.window);dialog.show();events();auto *table=dialog.findChild<QTableWidget *>("propagationStopsTable");require(table&&table->rowCount()==3,"Stopping rows missing");
         const QStringList expected={"Goal and StopTolerance units: s","Goal and StopTolerance units: km","Event residual units: dimensionless; no goal"};
         for(int i=0;i<3;++i){reveal(table);table->scrollToItem(table->item(i,0));QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,table->visualItemRect(table->item(i,0)).center());events();observe("stop-row-"+QString::number(i),dialog,"propagationStopUnits",expected[i]);require(dialog.statement()==original,"Unit selection rewrote stopping source");}dialog.reject();stable();
      }
      {
         const QString original="Sat.X > 1 & 2 < Sat.ElapsedSecs | user == A(1,1)";
         ConditionDialog dialog(original,&rig.window);dialog.show();events();auto *table=dialog.findChild<QTableWidget *>("conditionTable");require(table&&table->rowCount()==3,"Condition rows missing");
         const QStringList expected={"Left: km; Right: km","Left: s; Right: s","Left: user-defined units; Right: user-defined units"};
         for(int i=0;i<3;++i){reveal(table);table->scrollToItem(table->item(i,1));QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,table->visualItemRect(table->item(i,1)).center());events();observe("condition-row-"+QString::number(i),dialog,"conditionUnits",expected[i]);require(dialog.condition()==original,"Unit selection rewrote condition source");}
         reveal(table);table->scrollToItem(table->item(0,1));const auto cellPoint=table->visualItemRect(table->item(0,1)).center();QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,cellPoint);events();QTest::mouseDClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,cellPoint);events();QLineEdit *input=nullptr;for(auto *candidate:table->findChildren<QLineEdit *>())if(candidate->isVisible())input=candidate;require(input,"Condition cell editor missing");QTest::keyClick(input,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(input,"Sat.ECC");QTest::keyClick(input,Qt::Key_Return);events();observe("condition-changed",dialog,"conditionUnits","Left: dimensionless; Right: dimensionless");dialog.reject();stable();
      }
      {
         ComparisonPanel panel;panel.resize(1000,650);panel.show();events();auto *mode=panel.findChild<QComboBox *>("comparisonMode");reveal(mode);mode->setCurrentIndex(2);events();observe("comparison",panel,"comparisonToleranceUnit","(column units)");panel.close();stable();
      }
      {
         FolderRunDialog dialog(rig.files.path(),[](const FolderRunOptions &,QtPlotReceiver &,const std::atomic_bool &,const std::function<void(int,int,const FolderRunItem &)> &){throw std::runtime_error("Unit inspection must not run folder missions");return FolderRunResult{};},[] {},&rig.window);
         dialog.show();events();auto *check=dialog.findChild<QCheckBox *>("folderCompare");reveal(check);QTest::mouseClick(check,Qt::LeftButton);events();observe("folder-comparison",dialog,"folderToleranceUnit","(column units)");dialog.reject();stable();
      }
      require(observations.size()==24,"Ancillary observation count differs");QJsonObject proof{{"status",failures?"failed":"passed"},{"observations",observations},{"mismatches",failures},{"source_native_history_file_and_parameter_inventory_unchanged",true},{"mission_executions",0},{"new_requirement_credit",0}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return failures?1:0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
