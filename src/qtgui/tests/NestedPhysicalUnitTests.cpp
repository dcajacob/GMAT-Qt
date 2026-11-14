#include "FormationAcceptance.hpp"
#include "AtmosphereDialog.hpp"
#include "GravityDialog.hpp"
#include "PolyhedronDialog.hpp"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTableWidget>
using namespace formaccept;
static void reveal(QWidget *field)
{
   require(field,"Nested physical-unit input missing");
   for(auto *parent=field;parent;parent=parent->parentWidget())if(auto *scroll=qobject_cast<QScrollArea *>(parent))scroll->ensureWidgetVisible(field);
   events();require(field->isVisible()&&field->isEnabled(),"Nested physical-unit input inaccessible");acceptanceView(*field);
}
static QString labelText(QWidget &owner,QWidget *field)
{
   QString text;
   if(qobject_cast<QHBoxLayout *>(field->parentWidget()->layout()))for(auto *label:field->parentWidget()->findChildren<QLabel *>(QString(),Qt::FindDirectChildrenOnly))if(label->isVisible()){reveal(label);text+=' '+label->text();}
   for(auto *parent=field;parent&&parent!=&owner;parent=parent->parentWidget())for(auto *form:owner.findChildren<QFormLayout *>())if(auto *label=qobject_cast<QLabel *>(form->labelForField(parent)))if(label->isVisible()){reveal(label);text+=' '+label->text();}
   return text.trimmed();
}
static QJsonObject graph()
{
   QJsonObject result;auto *moderator=Moderator::Instance();
   for(const auto &name:moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT))if(auto *object=moderator->GetConfiguredObject(name))result[QString::fromStdString(name)]=QString::fromStdString(object->GetGeneratingString(Gmat::SHOW_SCRIPT));
   return result;
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NestedPhysicalUnits");
   try {
      require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString source="Create ForceModel Forces;\nForces.PrimaryBodies = {Earth};\nForces.Drag = NRLMSISE00;\nForces.Drag.HistoricWeatherSource = ConstantFluxAndGeoMag;\nForces.Drag.PredictedWeatherSource = ConstantFluxAndGeoMag;\nBeginMissionSequence;\n";
      const auto saved=rig.files.filePath("Nested unit views Ω.script");write(saved,source.toUtf8());require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Nested unit fixture failed");
      auto *force=Moderator::Instance()->GetConfiguredObject("Forces");require(force,"Nested unit force model missing");const auto native=graph();const int undo=rig.source->document()->availableUndoSteps();
      auto stable=[&] {require(graph()==native&&rig.source->toPlainText()==source&&read(saved)==source.toUtf8()&&rig.source->document()->availableUndoSteps()==undo,"Nested unit inspection changed source/native/history/file");};
      QJsonArray observations;int failures=0;
      auto observe=[&](const QString &key,const QString &actual,const QString &expected) {const bool passed=actual.contains(expected);failures+=!passed;observations.append(QJsonObject{{"key",key},{"expected",expected},{"actual",actual},{"passed",passed}});rig.save("unit-observations.json",QJsonDocument(observations).toJson());stable();};
      {
         AtmosphereDialog dialog(*force,{},&rig.window);dialog.show();dialog.resize(1050,750);events();
         for(const auto &spec:QList<QStringList>{{"F107","sfu"},{"F107A","sfu"},{"MagneticIndex","0–9"}}) {
            auto *field=dialog.findChild<QLineEdit *>("atmosphere_"+spec[0]);reveal(field);const auto before=field->text();observe("atmosphere-"+spec[0],labelText(dialog,field),spec[1]);require(field->text()==before,"Atmosphere unit observation converted value");
         }
         dialog.reject();stable();
      }
      {
         GravityDialog dialog(*force,{},&rig.window);dialog.show();dialog.resize(1050,750);events();
         for(const auto &spec:QList<QStringList>{{"Degree","Degree"},{"Order","Order"},{"StmLimit","STM degree/order limit"}}) {
            auto *field=dialog.findChild<QLineEdit *>("gravity_"+spec[0]);reveal(field);const auto before=field->text();observe("gravity-"+spec[0],labelText(dialog,field),spec[1]);require(field->text()==before,"Gravity count observation converted value");
         }
         dialog.reject();stable();
      }
      {
         const QJsonArray rows={QJsonObject{{"body","Earth"},{"shape","earth-shape.txt"},{"density","2000"}},QJsonObject{{"body","Mars"},{"shape","mars-shape.txt"},{"density","1000"}}};
         PolyhedronDialog dialog(rows,&rig.window);dialog.show();dialog.resize(1100,650);events();auto *table=dialog.findChild<QTableWidget *>("polyhedronTable");reveal(table);require(table->rowCount()==2,"Polyhedron repeated inputs missing");
         require(table->horizontalHeader()->isVisible(),"Polyhedron unit headers hidden");
         observe("polyhedron-shape",table->horizontalHeaderItem(1)->text(),"coordinates in km");
         for(int row=0;row<2;++row){auto *cell=table->item(row,2);require(cell&&cell->flags().testFlag(Qt::ItemIsEditable),"Polyhedron density cell not editable");table->scrollToItem(cell);events();require(table->visualItemRect(cell).intersects(table->viewport()->rect()),"Polyhedron density input offscreen");acceptanceView(*table);require(cell->text()==(row?"1000":"2000"),"Polyhedron unit observation converted density");}
         observe("polyhedron-density",table->horizontalHeaderItem(2)->text(),"kg/m³");dialog.reject();stable();
      }
      require(observations.size()==8,"Nested unit observation inventory differs");const QJsonObject proof{{"status",failures?"failed":"passed"},{"observations",observations},{"mismatches",failures},{"source_native_history_file_unchanged",true},{"mission_executions",0},{"new_requirement_credit",0}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return failures?1:0;
   }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';}return 1;
}
