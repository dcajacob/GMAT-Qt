#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include "ParameterInfo.hpp"
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTest>
using namespace formaccept;
static void reveal(QWidget *w) {
   require(w,"Command unit widget missing");
   for(auto *p=w;p;p=p->parentWidget())if(auto *s=qobject_cast<QScrollArea *>(p))s->ensureWidgetVisible(w);
   events();require(w->isVisible(),"Command unit widget hidden");acceptanceView(*w);
}
static QJsonObject graph() {
   QJsonObject result;auto *m=Moderator::Instance();
   for(const auto &name:m->GetListOfObjects(Gmat::UNKNOWN_OBJECT))if(auto *o=m->GetConfiguredObject(name))result[QString::fromStdString(name)]=QString::fromStdString(o->GetGeneratingString(Gmat::SHOW_SCRIPT));
   return result;
}
int main(int argc,char **argv) {
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("CommandUnitDisplay");
   try {
      require(argc==3||argc==4,"Supply startup, unit cases and optional case selection");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString source="% Command unit labels Δ\nCreate Spacecraft Sat;\nSat.NAIFId = -101280;\nSat.NAIFIdReferenceFrame = -911280;\nCreate DifferentialCorrector DC;\nCreate Yukon Opt;\nCreate Variable user;\nuser = 7;\nCreate Array A[2,2];\nCreate Propagator Prop;\nBeginMissionSequence;\n";
      auto saved=rig.files.filePath("Command units Ω.script");write(saved,source.toUtf8());require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Command unit fixture failed");
      const auto native=graph();const int undo=rig.source->document()->availableUndoSteps();auto *info=ParameterInfo::Instance();const auto types=info->GetTypesOfParameters(),names=info->GetNamesOfParameters();
      auto stable=[&] {require(graph()==native&&rig.source->toPlainText()==source&&read(saved)==source.toUtf8()&&rig.source->document()->availableUndoSteps()==undo&&info->GetTypesOfParameters()==types&&info->GetNamesOfParameters()==names,"Unit inspection changed native/source/history/parameter inventory");};
      const auto fixture=QJsonDocument::fromJson(read(argv[2])).object();QJsonArray observations,cases;int failures=0,expectedCount=0;
      for(const auto &entry:fixture["cases"].toArray()) {
         auto c=entry.toObject();if(argc==4&&!QString(argv[3]).split(',').contains(c["key"].toString()))continue;int applied=0;
         expectedCount+=c["units"].toObject().size();if(c.contains("transition"))expectedCount+=c["transition"].toObject()["units"].toObject().size();
         auto *editor=new CommandEditor(c["statement"].toString(),false,{},[&](const QString &){++applied;return QString();},{"Prop"},{"Sat"});
         auto *owner=rig.area->addSubWindow(editor);owner->show();owner->showNormal();owner->move(0,0);owner->resize(rig.area->viewport()->size());events();acceptanceView(*editor);
         auto inspect=[&](const QString &phase,const QJsonObject &units) {
            for(auto it=units.begin();it!=units.end();++it) {
               auto *label=editor->findChild<QLabel *>("commandUnit_"+it.key());QString actual;if(label){reveal(label);actual=label->text();}
               const bool passed=actual.contains("("+it.value().toString()+")");failures+=!passed;
               observations.append(QJsonObject{{"case",c["key"]},{"phase",phase},{"field",it.key()},{"expected_unit",it.value()},{"actual_label",actual},{"passed",passed}});stable();
            }
         };
         inspect("initial",c["units"].toObject());
         if(c.contains("transition")) {
            const auto transition=c["transition"].toObject();auto *input=editor->findChild<QLineEdit *>("commandField_"+transition["field"].toString());reveal(input);
            QTest::mouseClick(input,Qt::LeftButton);QTest::keyClick(input,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(input,transition["value"].toString());QTest::keyClick(input,Qt::Key_Tab);events();inspect("changed-control",transition["units"].toObject());
         }
         require(applied==0,"Unit inspection committed a command");editor->discardChanges();delete owner;events();stable();cases.append(c["key"]);
      }
      require(expectedCount>0&&observations.size()==expectedCount,"Command unit observation inventory differs");
      QJsonObject proof{{"status",failures?"failed":"passed"},{"cases",cases},{"observations",observations},{"mismatches",failures},{"native_source_history_file_and_parameter_inventory_unchanged",true},{"mission_executions",0},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return failures?1:0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
