#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "GmatGlobal.hpp"
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTableWidget>
#include <QTest>
using namespace formaccept;
static void reveal(QWidget *widget) {
   require(widget,"Error unit widget missing");
   for(auto *p=widget;p;p=p->parentWidget()) {
      for(auto *tabs:widget->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(widget);
   }
   events();require(widget->isVisible(),"Error unit control hidden");acceptanceView(*widget);
}
int main(int argc,char **argv) {
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("ErrorModelUnits");
   try {
      require(argc==4,"Supply startup, error unit fixture and mode");const QString mode=argv[3];require(mode=="Static"||mode=="Switch"||mode=="Testing","Unknown error unit mode");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());require(!GmatGlobal::Instance()->InTestingMode(),"Expected ordinary startup mode");
      if(mode=="Testing")GmatGlobal::Instance()->SetRunMode(GmatGlobal::TESTING);
      const auto fixture=QJsonDocument::fromJson(read(argv[2])).object(),units=fixture["units"].toObject();
      QJsonArray setups=mode=="Static"?fixture["static_types"].toArray():QJsonArray{"Range"};QJsonArray observations,selected;int mismatches=0;
      for(const auto &setup:setups) {
         const auto source="% Error model units Δ\nCreate ErrorModel Probe;\nProbe.Type = "+setup.toString()+";\nProbe.Bias = 0.125;\nProbe.BiasSigma = 2.5;\nProbe.NoiseSigma = 1.25;\nBeginMissionSequence;\n";
         const auto saved=rig.files.filePath("Error units Ω.script");write(saved,source.toUtf8());require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Error unit fixture Build failed");
         auto *native=Moderator::Instance()->GetConfiguredObject("Probe");require(native,"Error model missing");const auto before=native->GetGeneratingString(Gmat::SHOW_SCRIPT);const int undo=rig.source->document()->availableUndoSteps();
         auto stable=[&] {require(rig.source->toPlainText()==source&&read(saved)==source.toUtf8()&&rig.source->document()->availableUndoSteps()==undo&&Moderator::Instance()->GetConfiguredObject("Probe")==native&&native->GetGeneratingString(Gmat::SHOW_SCRIPT)==before,"Error unit inspection mutated original state");};
         for(auto *tabs:rig.window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==rig.resources||tabs->widget(i)->isAncestorOf(rig.resources))tabs->setCurrentIndex(i);
         const auto items=rig.resources->findItems("Probe",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Error model resource absent");reveal(rig.resources);rig.resources->scrollToItem(items[0]);const auto point=rig.resources->visualItemRect(items[0]).center();
         QTest::mouseClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();QTest::mouseDClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();
         auto *owner=rig.area->activeSubWindow();auto *panel=owner?dynamic_cast<ResourceEditor *>(owner->widget()):nullptr;require(panel,"Error model editor absent");owner->showNormal();owner->resize(rig.area->viewport()->size());events();
         auto *table=panel->findChild<QTableWidget *>("resourceProperties");auto *type=panel->findChild<QComboBox *>("resource_Type");require(table&&type,"Error model type selector absent");
         auto inspect=[&](const QString &kind) {
            require(units.contains(kind),"Unmapped error measurement type");selected.append(kind);const auto expected=units[kind].toString();
            for(const auto &v:fixture["fields"].toArray()) {
               const auto name=v.toString();auto *input=panel->findChild<QLineEdit *>("resource_"+name);reveal(input);QString label;
               for(auto *unit:input->parentWidget()->findChildren<QLabel *>(QString(),Qt::FindDirectChildrenOnly))if(unit->isVisible())label+=unit->text();
               int row=-1;for(int i=0;i<table->rowCount();++i)if(table->item(i,0)->text()==name)row=i;require(row>=0,"Error input row absent");
               for(const auto &surface:QStringList{"metadata","primary"}) {
                  const auto actual=surface=="metadata"?table->item(row,2)->text():label;const bool passed=actual==expected;if(!passed)++mismatches;
                  observations.append(QJsonObject{{"type",kind},{"field",name},{"surface",surface},{"actual",actual},{"expected",expected},{"passed",passed}});
                  rig.save("unit-observations.json",QJsonDocument(observations).toJson());stable();
               }
               require(input->text()==(name=="Bias"?"0.125":name=="BiasSigma"?"2.5":"1.25"),"Type switch converted stored error values");
            }
         };
         if(mode=="Static")inspect(setup.toString());
         else {
            QStringList names;for(int i=0;i<type->count();++i)names.append(type->itemText(i));require(names.size()==(mode=="Testing"?21:19),"Error measurement choice inventory changed");
            QStringList targets=mode=="Testing"?QStringList{"RightAscension","Declination"}:names;
            for(const auto &kind:targets) {
               const int index=type->findText(kind);require(index>=0,"Expected error measurement choice absent");reveal(type);QTest::mouseClick(type,Qt::LeftButton,Qt::NoModifier,QPoint(type->width()-10,type->height()/2));events();
               require(type->view()->isVisible(),"Error type popup failed");QTest::keyClick(type->view(),Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(type->view(),Qt::Key_Down);QTest::keyClick(type->view(),Qt::Key_Return);events();require(type->currentText()==kind,"Error type selection failed");inspect(kind);
            }
         }
         rig.closePanels();stable();
      }
      const int count=mode=="Static"?30:mode=="Testing"?12:114;require(observations.size()==count,"Error unit observation count differs");
      const QJsonObject proof{{"status",mismatches?"failed":"passed"},{"mode",mode},{"selected_types",selected},{"observations",observations},{"mismatches",mismatches},{"source_native_history_file_unchanged",true},{"values_not_converted",true},{"mission_executions",0},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return mismatches?1:0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
