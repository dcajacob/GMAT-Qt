#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QTabBar>
#include <QTest>
#include <QTimer>
using namespace formaccept;
static void reveal(QWidget *widget) {
   require(widget,"Covariance unit control missing");
   for(auto *p=widget;p;p=p->parentWidget()) {
      for(auto *tabs:widget->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(widget);
   }
   events();require(widget->isVisible(),"Covariance unit control hidden");acceptanceView(*widget);
}
int main(int argc,char **argv) {
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("CovarianceUnitDisplay");
   try {
      require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString source="% Covariance unit display Δ\nCreate Spacecraft Probe;\nProbe.NAIFId = -100078;\nProbe.NAIFIdReferenceFrame = -900078;\nProbe.SolveFors = {CartesianState};\nBeginMissionSequence;\n";
      const auto saved=rig.files.filePath("Covariance units Ω.script");write(saved,source.toUtf8());require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Covariance unit fixture Build failed");
      auto *native=Moderator::Instance()->GetConfiguredObject("Probe");const auto before=native->GetGeneratingString(Gmat::SHOW_SCRIPT);const int undo=rig.source->document()->availableUndoSteps();
      auto stable=[&] {require(rig.source->toPlainText()==source&&read(saved)==source.toUtf8()&&rig.source->document()->availableUndoSteps()==undo&&Moderator::Instance()->GetConfiguredObject("Probe")==native&&native->GetGeneratingString(Gmat::SHOW_SCRIPT)==before,"Covariance label inspection changed original state");};
      for(auto *tabs:rig.window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==rig.resources||tabs->widget(i)->isAncestorOf(rig.resources))tabs->setCurrentIndex(i);
      auto items=rig.resources->findItems("Probe",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Covariance resource absent");reveal(rig.resources);rig.resources->scrollToItem(items[0]);const auto point=rig.resources->visualItemRect(items[0]).center();
      QTest::mouseClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();QTest::mouseDClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();
      auto *owner=rig.area->activeSubWindow();auto *panel=owner?dynamic_cast<ResourceEditor *>(owner->widget()):nullptr;require(panel,"Covariance resource editor missing");owner->showNormal();owner->resize(rig.area->viewport()->size());events();
      auto *table=panel->findChild<QTableWidget *>("resourceProperties");auto *button=panel->findChild<QPushButton *>("editCells_OrbitErrorCovariance");require(table&&button,"Covariance cell editor absent");
      auto *sections=panel->findChild<QTabBar *>("propertySections");require(sections,"Covariance property sections absent");
      int row=-1;for(int i=0;i<table->rowCount();++i)if(table->item(i,0)->text()=="OrbitErrorCovariance")row=i;require(row>=0,"Covariance input row absent");const auto matrix=table->item(row,1)->text();QJsonArray observations;
      const auto showAll=[&] {int all=-1;for(int i=0;i<sections->count();++i)if(sections->tabText(i)=="All Properties")all=i;require(all>=0,"Covariance All Properties section absent");reveal(sections);QTest::mouseClick(sections,Qt::LeftButton,Qt::NoModifier,sections->tabRect(all).center());events();table->scrollToItem(table->item(row,1));events();};
      for(const auto &mode:QStringList{"CartesianState","KeplerianState"}) {
         if(mode=="KeplerianState") {
            auto *solve=panel->findChild<QLineEdit *>("resource_SolveFors");reveal(solve);QTest::mouseClick(solve,Qt::LeftButton);QTest::keyClick(solve,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(solve,"KeplerianState");QTest::keyClick(solve,Qt::Key_Tab);events();
         }
         showAll();reveal(button);std::exception_ptr failure;bool visited=false;
         QTimer::singleShot(0,panel,[&] {
            auto *dialog=panel->findChild<QDialog *>("numericGridDialog");
            try {
               require(dialog&&dialog->isVisible(),"Covariance dialog hidden");auto *grid=dialog->findChild<QTableWidget *>("numericGrid");require(grid&&grid->rowCount()==6&&grid->columnCount()==6,"Covariance grid is not 6x6");reveal(grid);
               const QStringList expected=mode=="CartesianState"?QStringList{"X (km)","Y (km)","Z (km)","VX (km/s)","VY (km/s)","VZ (km/s)"}:QStringList{"SMA (km)","ECC","INC (deg)","RAAN (deg)","AOP (deg)","MA (deg)"};
               for(int i=0;i<6;++i) {
                  grid->scrollToItem(grid->item(i,i));events();acceptanceView(*grid);
                  for(const auto &axis:QStringList{"row","column"}) {const auto actual=(axis=="row"?grid->verticalHeaderItem(i):grid->horizontalHeaderItem(i))->text();require(actual==expected[i],"Covariance unit basis label differs");observations.append(QJsonObject{{"mode",mode},{"axis",axis},{"index",i},{"label",actual}});}
               }
               QString help;for(auto *label:dialog->findChildren<QLabel *>())if(label->text().contains("Each cell's units"))help=label->text();require(help.contains("product of its row and column units")&&help.contains("does not convert these values"),"Covariance product/pending-unit explanation missing");observations.append(QJsonObject{{"mode",mode},{"axis","product-rule"},{"help",help}});
               QTest::mouseClick(dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel),Qt::LeftButton);visited=true;
            }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}
         });
         QTest::mouseClick(button,Qt::LeftButton);events();if(failure)std::rethrow_exception(failure);require(visited&&table->item(row,1)->text()==matrix,"Covariance units converted or changed matrix");stable();
      }
      rig.closePanels();stable();require(observations.size()==26,"Covariance unit observations incomplete");
      const QJsonObject proof{{"status","passed"},{"observations",observations},{"matrix_unchanged",true},{"source_native_history_file_unchanged",true},{"mission_executions",0},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
