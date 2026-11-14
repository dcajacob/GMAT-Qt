#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
using namespace formaccept;
static void reveal(QWidget *widget) {
   require(widget,"Polynomial unit widget missing");
   for(auto *p=widget;p;p=p->parentWidget()) {
      for(auto *tabs:widget->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(widget);
   }
   events();require(widget->isVisible(),"Polynomial unit control hidden");acceptanceView(*widget);
}
int main(int argc,char **argv) {
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("PolynomialArrayUnits");
   try {
      require(argc==3,"Supply startup and polynomial unit fixture");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto fixture=QJsonDocument::fromJson(read(argv[2])).object();QJsonArray observations;int mismatches=0;
      auto record=[&](const QString &type,const QString &field,const QString &surface,int length,int order,const QString &actual,const QString &expected) {
         const bool passed=actual==expected;if(!passed)++mismatches;
         observations.append(QJsonObject{{"type",type},{"field",field},{"surface",surface},{"length",length},{"order",order},{"actual",actual},{"expected",expected},{"passed",passed}});
         rig.save("unit-observations.json",QJsonDocument(observations).toJson());
      };
      for(const auto &entry:fixture["cases"].toArray()) {
         const auto spec=entry.toObject();const auto type=spec["type"].toString(),base=spec["base_unit"].toString();const bool resize=spec["allow_resize"].toBool();
         const auto source="% Polynomial unit display Δ\n"+spec["fixture"].toString()+"Create Variable Keep;\nKeep = 41;\nBeginMissionSequence;\n";
         const auto saved=rig.files.filePath(type+" units Ω.script");write(saved,source.toUtf8());
         require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Polynomial fixture Build failed");
         auto *native=Moderator::Instance()->GetConfiguredObject("Probe");require(native&&QString::fromStdString(native->GetTypeName())==type,"Wrong polynomial resource");
         const auto nativeBefore=native->GetGeneratingString(Gmat::SHOW_SCRIPT);const int undo=rig.source->document()->availableUndoSteps();
         auto stable=[&] {require(rig.source->toPlainText()==source&&read(saved)==source.toUtf8()&&rig.source->document()->availableUndoSteps()==undo&&Moderator::Instance()->GetConfiguredObject("Probe")==native&&native->GetGeneratingString(Gmat::SHOW_SCRIPT)==nativeBefore,"Polynomial display changed source/native/history/file");};
         for(auto *tabs:rig.window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==rig.resources||tabs->widget(i)->isAncestorOf(rig.resources))tabs->setCurrentIndex(i);
         const auto items=rig.resources->findItems("Probe",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Polynomial resource missing");
         reveal(rig.resources);rig.resources->scrollToItem(items[0]);const auto point=rig.resources->visualItemRect(items[0]).center();
         QTest::mouseClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();QTest::mouseDClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();
         auto *owner=rig.area->activeSubWindow();auto *panel=owner?dynamic_cast<ResourceEditor *>(owner->widget()):nullptr;require(panel,"Polynomial editor missing");owner->showNormal();owner->resize(rig.area->viewport()->size());events();
         auto *table=panel->findChild<QTableWidget *>("resourceProperties");require(table,"Polynomial backing table missing");
         for(const auto &value:spec["fields"].toArray()) {
            const auto field=value.toString(),summary=base+", "+base+"/s, "+base+"/s^2, ...";
            int row=-1;for(int i=0;i<table->rowCount();++i)if(table->item(i,0)->text()==field)row=i;require(row>=0,"Polynomial field missing");
            auto *line=panel->findChild<QLineEdit *>("resource_"+field);reveal(line);QString visible;
            for(auto *label:line->parentWidget()->findChildren<QLabel *>(QString(),Qt::FindDirectChildrenOnly))if(label->isVisible())visible+=label->text();
            record(type,field,"metadata",3,-1,table->item(row,2)->text(),summary);record(type,field,"primary",3,-1,visible,summary);stable();
            QPushButton *button=nullptr;for(auto *candidate:line->parentWidget()->findChildren<QPushButton *>(QString(),Qt::FindDirectChildrenOnly))if(candidate->isVisible()) {require(!button,"Ambiguous polynomial grid action");button=candidate;}
            require(button,"Polynomial grid action missing");std::exception_ptr failure;bool visited=false;
            QTimer::singleShot(0,panel,[&] {
               auto *dialog=panel->findChild<QDialog *>("numericGridDialog");
               try {
                  require(dialog&&dialog->isVisible(),"Polynomial grid did not open");auto *grid=dialog->findChild<QTableWidget *>("numericGrid");require(grid&&grid->rowCount()==1&&grid->columnCount()==3,"Polynomial grid shape differs");
                  auto inspect=[&](int length) {
                     require(grid->columnCount()==length&&grid->horizontalHeader()->isVisible(),"Polynomial header unavailable");
                     for(int k=0;k<length;++k) {
                        grid->scrollToItem(grid->item(0,k));events();require(grid->visualItemRect(grid->item(0,k)).intersects(grid->viewport()->rect()),"Polynomial unit offscreen");acceptanceView(*grid);
                        const auto unit=base+(k==0?QString():k==1?"/s":"/s^"+QString::number(k));auto *header=grid->horizontalHeaderItem(k);
                        record(type,field,"grid",length,k,header?header->text():QString(),"c"+QString::number(k)+" ("+unit+")");stable();
                     }
                  };inspect(3);
                  auto *columns=dialog->findChild<QSpinBox *>("arrayColumns");require(bool(columns)==resize,"Polynomial resize capability changed");
                  if(resize)for(int length:{5,2,3}) {reveal(columns);columns->setFocus();QTest::keyClick(columns,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(columns,QString::number(length));QTest::keyClick(columns,Qt::Key_Tab);events();inspect(length);}
                  QTest::mouseClick(dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel),Qt::LeftButton);visited=true;
               }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}
            });
            QTest::mouseClick(button,Qt::LeftButton);events();if(failure)std::rethrow_exception(failure);require(visited&&!panel->hasChanges(),"Polynomial view/resize Cancel changed draft");stable();
         }
         rig.closePanels();stable();
      }
      require(observations.size()==70,"Polynomial unit observation count differs");
      const QJsonObject proof{{"status",mismatches?"failed":"passed"},{"observations",observations},{"mismatches",mismatches},{"native_source_history_file_unchanged",true},{"mission_executions",0},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return mismatches?1:0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
