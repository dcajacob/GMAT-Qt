#include "FormationAcceptance.hpp"
#include "OrbitViewDialog.hpp"
#include "ParameterInfo.hpp"
#include <QAbstractItemView>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTest>
using namespace formaccept;
static void reveal(QWidget *field)
{
   require(field,"Camera unit input missing");
   for(auto *parent=field;parent;parent=parent->parentWidget())if(auto *scroll=qobject_cast<QScrollArea *>(parent))scroll->ensureWidgetVisible(field);
   events();require(field->isVisible(),"Camera unit input hidden");acceptanceView(*field);
}
static QJsonObject graph()
{
   QJsonObject result;auto *moderator=Moderator::Instance();
   for(const auto &name:moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT))if(auto *object=moderator->GetConfiguredObject(name))
      result[QString::fromStdString(name)]=QString::fromStdString(object->GetGeneratingString(Gmat::SHOW_SCRIPT));
   return result;
}
static void select(QComboBox *choice,const QString &value)
{
   reveal(choice);const int index=choice->findText(value);require(index>=0,"Camera selection missing");
   QTest::mouseClick(choice,Qt::LeftButton,Qt::NoModifier,QPoint(choice->width()-10,choice->height()/2));events();
   QTest::keyClick(choice->view(),Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(choice->view(),Qt::Key_Down);
   QTest::keyClick(choice->view(),Qt::Key_Return);events();require(choice->currentText()==value,"Camera selection failed");
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("OrbitCameraUnits");
   try {
      require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString source="Create Spacecraft Sat;\nSat.NAIFId = -101296;\nSat.NAIFIdReferenceFrame = -911296;\nCreate OrbitView View;\nView.Add = {Sat};\nView.ViewPointReference = [1 2 3];\nView.ViewPointVector = [4 5 30000];\nView.ViewDirection = [6 7 -1];\nBeginMissionSequence;\n";
      const auto saved=rig.files.filePath("Camera units Ω.script");write(saved,source.toUtf8());require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Camera unit fixture failed");
      const auto native=graph();const int undo=rig.source->document()->availableUndoSteps();auto *info=ParameterInfo::Instance();const auto names=info->GetNamesOfParameters(),types=info->GetTypesOfParameters();
      auto stable=[&] {require(graph()==native&&rig.source->toPlainText()==source&&read(saved)==source.toUtf8()
         &&rig.source->document()->availableUndoSteps()==undo&&info->GetNamesOfParameters()==names&&info->GetTypesOfParameters()==types,"Camera units changed native/source/history/inventory");};
      auto *view=Moderator::Instance()->GetConfiguredObject("View");require(view,"Camera view missing");OrbitViewDialog dialog(*view,{},&rig.window);const auto before=dialog.settings();dialog.show();events();
      QJsonArray observations;int failures=0;
      const QStringList fields={"ViewPointReference","ViewPointVector","ViewDirection"};
      const QStringList captions={"Viewpoint reference","Viewpoint offset","View direction / target"};
      for(int i=0;i<fields.size();++i) {
         const auto name=fields[i];auto *choice=dialog.findChild<QComboBox *>("orbit_"+name);require(choice&&choice->currentText()=="Vector","Camera did not start in vector mode");
         QStringList values;for(int j=1;j<=3;++j){auto *field=dialog.findChild<QLineEdit *>("orbit_"+name+QString::number(j));require(field,"Camera component missing");values.append(field->text());}
         for(const auto &phase:QStringList{"initial-vector","object","return-vector"}) {
            if(phase=="object")select(choice,"Earth");else if(phase=="return-vector")select(choice,"Vector");
            const bool vector=phase!="object";
            for(int j=1;j<=3;++j) {
               auto *field=dialog.findChild<QLineEdit *>("orbit_"+name+QString::number(j));reveal(field);QString caption;
               for(QWidget *parent=field;parent&&parent!=&dialog;parent=parent->parentWidget())for(auto *form:dialog.findChildren<QFormLayout *>())
                  if(auto *label=qobject_cast<QLabel *>(form->labelForField(parent)))if(label->isVisible()){reveal(label);caption=label->text();}
               const auto expected=captions[i]+(vector?" (km)":"");const bool passed=caption==expected&&field->isEnabled()==vector;failures+=!passed;
               require(field->text()==values[j-1],"Camera mode switch converted vector values");
               observations.append(QJsonObject{{"field",name},{"component",j},{"phase",phase},{"expected_caption",expected},{"actual_caption",caption},{"enabled",field->isEnabled()},{"passed",passed}});
               rig.save("unit-observations.json",QJsonDocument(observations).toJson());stable();
            }
         }
      }
      require(dialog.settings()==before,"Camera vector round trip changed pending settings");dialog.reject();stable();
      require(observations.size()==27,"Camera unit observation inventory differs");
      const QJsonObject proof{{"status",failures?"failed":"passed"},{"observations",observations},{"mismatches",failures},
         {"source_native_history_file_and_parameter_inventory_unchanged",true},{"vector_values_and_pending_settings_unchanged",true},{"mission_executions",0},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return failures?1:0;
   }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';}return 1;
}
