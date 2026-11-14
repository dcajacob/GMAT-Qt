#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
using namespace formaccept;

static void reveal(QWidget *widget)
{
   require(widget,"Attitude unit control missing");
   for (auto *parent=widget;parent;parent=parent->parentWidget()) {
      for (auto *tabs:widget->window()->findChildren<QTabWidget *>())
         if (tabs->indexOf(parent)>=0) tabs->setCurrentWidget(parent);
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(widget);
   }
   events();require(widget->isVisible(),"Attitude unit control hidden");acceptanceView(*widget);
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("AttitudeUnits");
   try {
      require(argc==2 || (argc==3&&QString(argv[2])=="--ballistics"),"Supply startup");
      const bool ballistics=argc==3;Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString text="Create Spacecraft Probe;\nProbe.Attitude = Spinner;\n"
         "Probe.AttitudeCoordinateSystem = EarthMJ2000Eq;\nProbe.AttitudeDisplayStateType = Quaternion;\n"
         "Probe.Q1 = 0;\nProbe.Q2 = 0;\nProbe.Q3 = 0;\nProbe.Q4 = 1;\n"
         "Probe.AngularVelocityX = 0;\nProbe.AngularVelocityY = 0;\nProbe.AngularVelocityZ = 0;\n"
         "Probe.Cd = 2.25;\nProbe.Cr = 1.75;\nProbe.SPADDragScaleFactor = 0.9;\nProbe.SPADSRPScaleFactor = 1.1;\n"
         "Create Variable Keep;\nKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+rig.report+"';\n"
         "Values.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\n"
         "BeginMissionSequence;\nKeep = Keep+1;\nReport Values Probe.Q1 Probe.Q2 Probe.Q3 Probe.Q4 Keep;\n";
      rig.source->setPlainText(text);require(rig.window.buildScript(),"Attitude units fixture Build failed");
      auto *native=Moderator::Instance()->GetConfiguredObject("Probe");require(native,"Attitude owner missing");
      const auto canonical=QString::fromStdString(native->GetGeneratingString(Gmat::SCRIPTING));
      const int undo=rig.source->document()->availableUndoSteps();rig.save("initial.script",text.toUtf8());
      auto invariant=[&] {
         require(rig.source->toPlainText()==text&&rig.source->document()->availableUndoSteps()==undo&&
            native==Moderator::Instance()->GetConfiguredObject("Probe")&&
            QString::fromStdString(native->GetGeneratingString(Gmat::SCRIPTING))==canonical,
            "Attitude units view changed source/history/configured owner");
      };
      auto items=rig.resources->findItems("Probe",Qt::MatchExactly|Qt::MatchRecursive);
      require(items.size()==1,"Attitude units resource entry absent");rig.resources->scrollToItem(items[0]);reveal(rig.resources);
      QTest::mouseClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,rig.resources->visualItemRect(items[0]).center());events();
      QTest::mouseDClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,rig.resources->visualItemRect(items[0]).center());events();
      auto *child=rig.area->activeSubWindow();auto *panel=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;
      require(panel,"Actual spacecraft editor absent");child->showNormal();child->resize(rig.area->viewport()->size());
      QJsonArray observations;
      if (ballistics) {
         const QStringList names{"Cd","Cr","SPADDragScaleFactor","SPADSRPScaleFactor"};
         const double expected[]={2.25,1.75,.9,1.1};
         for(int index=0;index<names.size();++index) {
            const auto name=names[index];auto *control=panel->findChild<QLineEdit *>("ballistics_"+name);reveal(control);
            auto *label=panel->findChild<QLabel *>("ballisticsUnit_"+name);
            observations.append(QJsonObject{{"field",name},{"unit",label?label->text():QString()}});
            rig.save("unit-observations.json",QJsonDocument(observations).toJson());
            require(label&&label->text()=="dimensionless","Ballistic dimensionless unit label omitted");reveal(label);
            bool ok=false;const double displayed=control->text().toDouble(&ok);
            require(ok&&std::abs(displayed-expected[index])<1e-12&&std::abs(native->GetRealParameter(name.toStdString())-expected[index])<1e-12,"Ballistic dimensionless coefficient was rescaled");invariant();
         }
      } else for (const auto &representation:QStringList{"Quaternion","MRPs","DirectionCosineMatrix"}) {
         auto *selector=panel->findChild<QComboBox *>("attitude_AttitudeDisplayStateType");reveal(selector);
         selector->setCurrentText(representation);events();
         QStringList fields;
         if (representation=="Quaternion") fields={"Q1","Q2","Q3","Q4"};
         else if (representation=="MRPs") fields={"MRP1","MRP2","MRP3"};
         else for (int row=1;row<=3;++row) for (int column=1;column<=3;++column) fields.append("DCM"+QString::number(row)+QString::number(column));
         for (const auto &name:fields) {
            auto *control=panel->findChild<QLineEdit *>("attitude_"+name);reveal(control);
            auto *label=panel->findChild<QLabel *>("attitudeUnit_"+name);
            observations.append(QJsonObject{{"representation",representation},{"field",name},{"unit",label?label->text():QString()}});
            rig.save("unit-observations.json",QJsonDocument(observations).toJson());
            require(label&&label->text()=="dimensionless","Attitude dimensionless unit label omitted");reveal(label);
            bool ok=false;const double actual=control->text().toDouble(&ok);
            const double expected=name=="Q4"||name=="DCM11"||name=="DCM22"||name=="DCM33"?1:0;
            require(ok&&std::isfinite(actual)&&std::abs(actual-expected)<1e-12,"Display conversion changed independent identity attitude");
            invariant();
         }
      }
      rig.closePanels();invariant();
      require(rig.window.runMission()==MainWindow::RunResult::Completed,"Attitude unit witness Run failed");
      const auto columns=QString::fromUtf8(read(rig.report)).trimmed().split(',');require(columns.size()==5,"Attitude witness report shape differs");
      for (int i=0;i<5;++i) {bool ok=false;const double value=columns[i].trimmed().toDouble(&ok),expected=i==4?42:i==3?1:0;
         require(ok&&std::abs(value-expected)<1e-12,"Unit browsing changed independent native quaternion/sentinel");}
      rig.save("values.csv",read(rig.report));
      const QJsonObject proof{{"status","passed"},{"unit_labels",observations},{"orientation_fields_checked",ballistics?0:observations.size()},
         {"ballistic_fields_checked",ballistics?observations.size():0},
         {"configured_source_unchanged",true},{"independent_identity_attitude",true},{"numeric_sentinel",42}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';return 0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}
     catch (std::exception &error) {std::cerr<<"FAIL "<<error.what()<<'\n';}
   return 1;
}
