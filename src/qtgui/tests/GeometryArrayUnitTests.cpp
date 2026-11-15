#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTableWidget>
#include <QTest>
using namespace formaccept;
static void reveal(QWidget *widget) {
   require(widget,"Angle-unit widget missing");
   for(auto *p=widget;p;p=p->parentWidget()) {
      for(auto *tabs:widget->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(widget);
   }
   events();require(widget->isVisible(),"Angle-unit widget hidden");acceptanceView(*widget);
}
int main(int argc,char **argv) {
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("GeometryArrayUnits");
   try {
      require(argc==3,"Supply startup and angle-unit fixtures");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto fixture=QJsonDocument::fromJson(read(argv[2])).object();QJsonArray observations;int mismatches=0;
      for(const auto &entry:fixture["cases"].toArray()) {
         const auto spec=entry.toObject();const auto type=spec["type"].toString();const bool fov=type=="CustomFOV";
         const auto source="% Geometry array units Δ\n"+spec["fixture"].toString()+"BeginMissionSequence;\n";const auto saved=rig.files.filePath(type+" units Ω.script");write(saved,source.toUtf8());
         require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Geometry unit fixture failed Build");auto *native=Moderator::Instance()->GetConfiguredObject("Probe");require(native,"Geometry unit object missing");
         const auto before=native->GetGeneratingString(Gmat::SHOW_SCRIPT);const int undo=rig.source->document()->availableUndoSteps();
         auto stable=[&] {require(rig.source->toPlainText()==source&&read(saved)==source.toUtf8()&&rig.source->document()->availableUndoSteps()==undo&&Moderator::Instance()->GetConfiguredObject("Probe")==native&&native->GetGeneratingString(Gmat::SHOW_SCRIPT)==before,"Geometry units changed original source/native/history/file");};
         for(auto *tabs:rig.window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==rig.resources||tabs->widget(i)->isAncestorOf(rig.resources))tabs->setCurrentIndex(i);
         const auto items=rig.resources->findItems("Probe",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Geometry unit resource absent");reveal(rig.resources);rig.resources->scrollToItem(items[0]);const auto point=rig.resources->visualItemRect(items[0]).center();
         QTest::mouseClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();QTest::mouseDClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();
         auto *owner=rig.area->activeSubWindow();auto *panel=owner?dynamic_cast<ResourceEditor *>(owner->widget()):nullptr;require(panel,"Geometry unit editor absent");owner->showNormal();owner->resize(rig.area->viewport()->size());events();
         auto *grid=panel->findChild<QTableWidget *>(fov?"customFovVertices":"regionVertices");reveal(grid);require(grid->rowCount()==(fov?3:4)&&grid->columnCount()==2,"Geometry unit grid shape differs");
         auto inspect=[&](const QString &mode,const QStringList &fields,const QStringList &labels,const std::vector<std::vector<double>> &values) {
            reveal(grid);
            for(int column=0;column<2;++column) {
               const auto actual=grid->horizontalHeaderItem(column)->text();const auto expected=labels[column]+" (deg)";const bool passed=actual==expected;if(!passed)++mismatches;
               for(int row=0;row<grid->rowCount();++row) {auto *cell=grid->item(row,column);require(cell&&cell->flags().testFlag(Qt::ItemIsEditable),"Geometry angle cell not writable");grid->scrollToItem(cell);events();require(grid->visualItemRect(cell).intersects(grid->viewport()->rect()),"Geometry angle cell offscreen");acceptanceView(*grid);bool valid=false;const auto value=cell->text().toDouble(&valid);require(valid&&std::abs(value-values[row][column])<=1e-12,"Degree-valued geometry draft differs");}
               observations.append(QJsonObject{{"type",type},{"mode",mode},{"field",fields[column]},{"unit","deg"},{"actual_header",actual},{"expected_header",expected},{"rows",grid->rowCount()},{"passed",passed}});rig.save("unit-observations.json",QJsonDocument(observations).toJson());stable();
            }
         };
         if(fov) {
            inspect("ConeClock",{"ConeAngles","ClockAngles"},{"Cone angle","Clock angle"},{{10,0},{10,120},{10,240}});
            auto *step=panel->findChild<QLineEdit *>("customFov_InterpolationStepSize");reveal(step);QString label;
            for(auto *form:panel->findChildren<QFormLayout *>())if(auto *text=qobject_cast<QLabel *>(form->labelForField(step))){reveal(text);label=text->text();}
            require(step->text()=="0.40000000000000002"||step->text()=="0.4","Interpolation value changed");const bool passed=label=="Interpolation step (deg)";if(!passed)++mismatches;
            observations.append(QJsonObject{{"type",type},{"mode","Both"},{"field","InterpolationStepSize"},{"unit","deg"},{"actual_header",label},{"expected_header","Interpolation step (deg)"},{"rows",1},{"passed",passed}});rig.save("unit-observations.json",QJsonDocument(observations).toJson());stable();
            auto *choice=panel->findChild<QComboBox *>("customFovCoordinates");reveal(choice);const int index=choice->findData("AzimuthElevation");require(index>=0,"Azimuth/elevation choice missing");
            QTest::mouseClick(choice,Qt::LeftButton,Qt::NoModifier,QPoint(choice->width()-10,choice->height()/2));events();QTest::keyClick(choice->view(),Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(choice->view(),Qt::Key_Down);QTest::keyClick(choice->view(),Qt::Key_Return);events();
            require(choice->currentData()=="AzimuthElevation","Azimuth/elevation selection failed");inspect("AzimuthElevation",{"Azimuth","Elevation"},{"Azimuth","Elevation"},{{0,80},{120,80},{240,80}});
         }else inspect("Vertices",{"Latitude","Longitude"},{"Latitude","Longitude"},{{-10,-10},{20,-10},{20,20},{-10,20}});
         rig.closePanels();stable();
      }
      require(observations.size()==7,"Geometry field/unit coverage differs");const QJsonObject proof{{"status",mismatches?"failed":"passed"},{"observations",observations},{"mismatches",mismatches},{"source_native_history_file_unchanged",true},{"absolute_bound",1e-12},{"mission_executions",0},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return mismatches?1:0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
