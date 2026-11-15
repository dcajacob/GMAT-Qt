#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
using namespace formaccept;

static void reveal(QWidget *widget) {
   require(widget,"Electric unit widget missing");
   for(auto *p=widget;p;p=p->parentWidget()) {
      for(auto *tabs:widget->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(widget);
   }
   events();require(widget->isVisible(),"Electric unit widget hidden");acceptanceView(*widget);
}
static QJsonArray strings(const StringArray &values) {
   QJsonArray result;for(const auto &v:values)result.append(QString::fromStdString(v));return result;
}
int main(int argc,char **argv) {
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("ElectricCoefficientUnits");
   try {
      require(argc==3||argc==4,"Supply startup, electric-unit fixture and optional Switch mode");
      const bool switched=argc==4;require(!switched||QString(argv[3])=="Switch","Unknown electric unit mode");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto fixture=QJsonDocument::fromJson(read(argv[2])).object();const auto spec=fixture["cases"].toArray().first().toObject();
      const auto wanted=spec["units"].toObject();require(wanted.size()==10,"Electric coefficient expectation count differs");
      QString setup=spec["fixture"].toString();if(switched)setup.replace("ThrustMassPolynomial","ConstantThrustAndIsp");
      const QString source="% Electric coefficient units Δ\n"+setup+"Create Variable Keep;\nKeep = 41;\nBeginMissionSequence;\n";
      const auto saved=rig.files.filePath("Electric units Ω.script");write(saved,source.toUtf8());
      require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Electric unit fixture failed Build");rig.save("initial.script",source.toUtf8());
      auto *native=Moderator::Instance()->GetConfiguredObject("Probe");require(native&&native->GetTypeName()=="ElectricThruster","Electric unit object differs");
      const auto nativeBefore=QString::fromStdString(native->GetGeneratingString(Gmat::SHOW_SCRIPT));
      const auto thrustBefore=strings(native->GetStringArrayParameter("T_UNITS")),massBefore=strings(native->GetStringArrayParameter("MF_UNITS"));
      const int undo=rig.source->document()->availableUndoSteps();
      auto stable=[&] {require(rig.source->toPlainText()==source&&read(saved)==source.toUtf8()&&rig.source->document()->availableUndoSteps()==undo&&
         Moderator::Instance()->GetConfiguredObject("Probe")==native&&QString::fromStdString(native->GetGeneratingString(Gmat::SHOW_SCRIPT))==nativeBefore&&
         strings(native->GetStringArrayParameter("T_UNITS"))==thrustBefore&&strings(native->GetStringArrayParameter("MF_UNITS"))==massBefore,
         "Electric unit viewing changed native/source/history/file state");};
      for(auto *tabs:rig.window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)
         if(tabs->widget(i)==rig.resources||tabs->widget(i)->isAncestorOf(rig.resources))tabs->setCurrentIndex(i);
      const auto items=rig.resources->findItems("Probe",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Electric unit resource missing");
      rig.resources->scrollToItem(items[0]);reveal(rig.resources);
      QTest::mouseClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,rig.resources->visualItemRect(items[0]).center());events();
      QTest::mouseDClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,rig.resources->visualItemRect(items[0]).center());events();
      auto *owner=rig.area->activeSubWindow();auto *panel=owner?dynamic_cast<ResourceEditor *>(owner->widget()):nullptr;require(panel,"Electric unit editor missing");
      owner->showNormal();owner->resize(rig.area->viewport()->size());events();
      if(switched) {
         auto *model=panel->findChild<QComboBox *>("thruster_ThrustModel");reveal(model);
         require(model&&model->currentText()=="ConstantThrustAndIsp","Wrong initial electric model");
         auto *coefficients=panel->findChild<QPushButton *>("thrusterCoefficients");
         require(coefficients&&!coefficients->isEnabled(),"Non-polynomial coefficient editor enabled");
         auto selectModel=[&](const QString &name) {
         QTest::mouseClick(model,Qt::LeftButton,Qt::NoModifier,QPoint(model->width()-10,model->height()/2));events();
         require(model->view()->isVisible(),"Electric model selector did not open");QTest::keyClick(model->view(),Qt::Key_Home);
         const int index=model->findText(name);require(index>=0,"Electric model missing");
         for(int i=0;i<index;++i)QTest::keyClick(model->view(),Qt::Key_Down);
         QTest::keyClick(model->view(),Qt::Key_Return);events();
         require(model->currentText()==name,"Electric model selection differs");
         require(coefficients->isEnabled()==(name=="ThrustMassPolynomial"),"Pending model coefficient availability differs");stable();
         };
         selectModel("ThrustMassPolynomial");selectModel("FixedEfficiency");selectModel("ConstantThrustAndIsp");selectModel("ThrustMassPolynomial");
         require(model->currentText()=="ThrustMassPolynomial"&&panel->hasChanges(),"Electric model switch was not kept pending");stable();
      }
      auto *table=panel->findChild<QTableWidget *>("resourceProperties");auto *sections=panel->findChild<QTabBar *>("propertySections");require(table&&sections,"Electric unit table missing");
      int all=-1;for(int i=0;i<sections->count();++i)if(sections->tabText(i)=="All Properties")all=i;
      require(all>=0,"Electric unit all-properties tab missing");reveal(sections);QTest::mouseClick(sections,Qt::LeftButton,Qt::NoModifier,sections->tabRect(all).center());events();
      QJsonArray observations;int mismatches=0;
      auto record=[&](const QString &route,const QString &field,const QString &actual) {
         const auto expected=wanted[field].toString();const bool passed=actual==expected;if(!passed)++mismatches;
         observations.append(QJsonObject{{"route",route},{"field",field},{"expected_unit",expected},{"actual_unit",actual},{"passed",passed}});
         rig.save("unit-observations.json",QJsonDocument(observations).toJson());stable();
      };
      for(auto i=wanted.begin();i!=wanted.end();++i) {
         int row=-1;for(int j=0;j<table->rowCount();++j)if(table->item(j,0)->text()==i.key())row=j;
         require(row>=0&&!table->isRowHidden(row),"Electric coefficient input row hidden");auto *unit=table->item(row,2);
         require(unit&&!unit->flags().testFlag(Qt::ItemIsEditable),"Electric unit cell editable or absent");
         reveal(table);table->scrollToItem(unit);events();require(table->visualItemRect(unit).intersects(table->viewport()->rect()),"Electric unit cell offscreen");acceptanceView(*table);
         record("advanced-table",i.key(),unit->text());
      }
      auto *button=panel->findChild<QPushButton *>("thrusterCoefficients");reveal(button);require(button->isEnabled(),"Electric polynomial editor disabled");
      std::exception_ptr failure;bool visited=false;
      QTimer::singleShot(0,panel,[&] {
         auto *dialog=panel->findChild<QDialog *>("thrusterCoefficientDialog");
         try {
            require(dialog&&dialog->isVisible(),"Electric unit coefficient dialog absent");auto *tabs=dialog->findChild<QTabWidget *>();require(tabs,"Electric coefficient tabs missing");
            for(int family=0;family<2;++family) {
               tabs->setCurrentIndex(family);events();auto *grid=dialog->findChild<QTableWidget *>(family?"thrusterSecondaryCoefficients":"thrusterThrustCoefficients");
               require(grid&&grid->rowCount()==5,"Electric coefficient family shape differs");reveal(grid);
               for(int row=0;row<5;++row) {
                  const auto field=QString(family?"MassFlowCoeff":"ThrustCoeff")+QString::number(row+1);auto *unit=grid->item(row,2);
                  require(unit&&!unit->flags().testFlag(Qt::ItemIsEditable)&&grid->item(row,0)->text()==field,"Electric dialog coefficient identity/unit differs");
                  grid->scrollToItem(unit);events();require(grid->visualItemRect(unit).intersects(grid->viewport()->rect()),"Electric dialog unit offscreen");acceptanceView(*grid);
                  record("coefficient-dialog",field,unit->text());
               }
            }
            QTest::mouseClick(dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel),Qt::LeftButton);visited=true;
         }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}
      });
      QTest::mouseClick(button,Qt::LeftButton);events();if(failure)std::rethrow_exception(failure);require(visited&&panel->hasChanges()==switched,"Electric coefficient viewing changed the draft");stable();
      rig.closePanels();stable();
      require(observations.size()==20,"Electric coefficient surface missing");
      const QJsonObject proof{{"status",mismatches?"failed":"passed"},{"mismatches",mismatches},{"observations",observations},
         {"native_thrust_units",thrustBefore},{"native_mass_units",massBefore},{"model_switched",switched},{"pending_switch_discarded",switched},
         {"model_availability_transitions",switched ? 5 : 0},
         {"native_source_history_file_unchanged",true},{"mission_executions",0},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return mismatches?1:0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
