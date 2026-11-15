#include "FormationAcceptance.hpp"
#include "ReportParameterDialog.hpp"
#include "CommandEditor.hpp"
#include "ParameterInfo.hpp"
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTest>
#include <QTimer>
using namespace formaccept;
static void reveal(QWidget *w) {
   require(w,"Picker unit control missing");
   for(auto *p=w;p;p=p->parentWidget()) {
      for(auto *tabs:w->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(w);
   }
   events();require(w->isVisible(),"Picker unit control hidden");acceptanceView(*w);
}
static QJsonObject graph() {
   QJsonObject result;auto *m=Moderator::Instance();for(const auto &n:m->GetListOfObjects(Gmat::UNKNOWN_OBJECT))if(auto *o=m->GetConfiguredObject(n))result[QString::fromStdString(n)]=QString::fromStdString(o->GetGeneratingString(Gmat::SHOW_SCRIPT));return result;
}
int main(int argc,char **argv) {
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("PickerUnitDisplay");
   try {
      require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString source="% Picker units Δ\nCreate Spacecraft Sat;\nSat.NAIFId = -101292;\nSat.NAIFIdReferenceFrame = -911292;\nCreate Variable user;\nuser = 7;\nCreate Array A[2,2];\nCreate Propagator Prop;\nCreate DifferentialCorrector DC;\nBeginMissionSequence;\n";
      auto saved=rig.files.filePath("Picker units Ω.script");write(saved,source.toUtf8());require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Picker unit fixture failed");const auto native=graph();const int undo=rig.source->document()->availableUndoSteps();auto *info=ParameterInfo::Instance();const auto types=info->GetTypesOfParameters(),names=info->GetNamesOfParameters();QJsonArray observations;int failures=0;
      auto stable=[&] {require(graph()==native&&rig.source->toPlainText()==source&&read(saved)==source.toUtf8()&&rig.source->document()->availableUndoSteps()==undo&&info->GetTypesOfParameters()==types&&info->GetNamesOfParameters()==names,"Picker units changed native/source/history/inventory");};
      auto observe=[&](const QString &key,QWidget &owner,const QString &expected) {
         auto *label=owner.findChild<QLabel *>("reportParameterUnits");QString actual;if(label){reveal(label);actual=label->text();}
         const bool passed=actual==expected;failures+=!passed;observations.append(QJsonObject{{"key",key},{"expected",expected},{"actual",actual},{"passed",passed}});rig.save("unit-observations.json",QJsonDocument(observations).toJson());stable();
      };
      using Mode=ReportParameterDialog::Mode;
      struct Case {QString key,reference,context,expected;Mode mode;};
      const QList<Case> cases={
         {"reference","Sat.X",{},"Units: km",Mode::NumericReference},
         {"literal","2","km","Units: km",Mode::NumericSingle},
         {"variable","user","km","Units: user-defined units; field units: km",Mode::NumericSingle},
         {"array","A(1,1)","kg","Units: user-defined units; field units: kg",Mode::NumericSingle},
         {"local-control","2",{},"Units: user-defined units",Mode::VaryControl},
         {"stop","Sat.ElapsedDays",{},"Units: days",Mode::StopParameter},
         {"single-literal","3","days","Units: days",Mode::Single},
         {"ratio","Sat.ECC",{},"Units: dimensionless",Mode::NumericReference}};
      for(const auto &c:cases) {
         ReportParameterDialog dialog({c.reference},&rig.window,c.mode);if(!c.context.isEmpty())dialog.setProperty("unitContext",c.context);dialog.show();events();observe(c.key,dialog,c.expected);
         if(c.key=="literal") {
            dialog.setProperty("unitContext",QString("s"));events();observe("changed-context",dialog,"Units: s");
            auto *entry=dialog.findChild<QComboBox *>("reportParameterEntry");require(entry&&entry->lineEdit(),"Picker entry missing");reveal(entry);QTest::mouseClick(entry->lineEdit(),Qt::LeftButton);QTest::keyClick(entry->lineEdit(),Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(entry->lineEdit(),"Sat.X");events();observe("different-reference",dialog,"Units: km; field units: s");
         }
         dialog.reject();stable();
      }
      for(const auto &key:QStringList{"achieve-caller","propagation-caller"}) {
         const bool achieve=key=="achieve-caller";const QString command=achieve?"Achieve DC(Sat.X = 7000, {Tolerance=1});":"Propagate Prop(Sat) {Sat.ElapsedSecs = 60};";int applied=0;
         auto *editor=new CommandEditor(command,false,{},[&](const QString &){++applied;return QString();},{"Prop"},{"Sat"});auto *window=rig.area->addSubWindow(editor);window->show();window->showNormal();window->resize(rig.area->viewport()->size());events();
         auto *button=editor->findChild<QPushButton *>(achieve?"commandChoose_Value":"propagationChooseGoal");reveal(button);std::exception_ptr failure;bool visited=false;
         QTimer::singleShot(0,editor,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
            try{require(dialog,"Caller picker absent");observe(key,*dialog,achieve?"Units: km":"Units: s");dialog->reject();visited=true;}catch(...){failure=std::current_exception();if(dialog)dialog->reject();}
         });
         QTest::mouseClick(button,Qt::LeftButton);events();if(failure)std::rethrow_exception(failure);require(visited&&applied==0,"Caller units changed command");editor->discardChanges();delete window;events();stable();
      }
      require(observations.size()==12,"Picker unit observation count differs");QJsonObject proof{{"status",failures?"failed":"passed"},{"observations",observations},{"mismatches",failures},{"source_native_history_file_and_parameter_inventory_unchanged",true},{"mission_executions",0},{"new_requirement_credit",0}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return failures?1:0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
