#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "CelestialBodyPanel.hpp"
#include "QtPlotReceiver.hpp"
#include "SpacePoint.hpp"
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
using namespace formaccept;
static void reveal(QWidget *control)
{
   require(control,"Trajectory color control absent");
   for(auto *p=control;p;p=p->parentWidget()) {
      for(auto *tabs:control->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(control);
   }
   events();require(control->isVisible()&&control->isEnabled(),"Trajectory color control inaccessible");acceptanceView(*control);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaTrajectoryColors");
   try {
      require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QStringList names{"Sat","Earth","Luna","Mars","Sun"};
      const QList<QColor> colors{{23,145,210},{31,61,91},{115,45,165},{160,80,25},{90,170,35}};
      const double speed=std::sqrt(mu/7000.);
      const QString mission="BeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 20};\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Keep;\n% preserve colors Ω\n";
      const QString script="Create Spacecraft Sat;\nSat.NAIFId = -10009999;\nSat.NAIFIdReferenceFrame = -9009999;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+number(speed)+";\nSat.VZ = 0;\nCreate Variable Keep;\nKeep = 41;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.Accuracy = 0.000000000001;\nCreate OrbitView Orb;\nOrb.Add = {Sat, Earth, Luna, Mars, Sun};\nOrb.CoordinateSystem = EarthMJ2000Eq;\nOrb.UpdatePlotFrequency = 1;\nCreate ReportFile Values;\nValues.Filename = '"+rig.report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\n"+mission;
      rig.source->setPlainText(script);require(rig.window.buildScript(),"Trajectory color fixture build failed");rig.source->document()->setModified(false);QJsonArray edits;
      for(int index=0;index<names.size();++index) {
         const auto name=names[index];const auto before=rig.source->toPlainText();auto *point=Moderator::Instance()->GetSpacePoint(name.toStdString());require(point,"Trajectory color source object absent");const auto nativeBefore=point->GetCurrentOrbitColor();
         const auto matches=rig.resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Trajectory color resource tree entry ambiguous");rig.resources->itemDoubleClicked(matches.first(),0);events();
         auto *child=rig.area->activeSubWindow();QPointer<EditablePanel> panel=child?dynamic_cast<EditablePanel *>(child->widget()):nullptr;require(panel,"Trajectory color resource panel absent");
         require(index==0?dynamic_cast<ResourceEditor *>(panel.data())!=nullptr:dynamic_cast<CelestialBodyPanel *>(panel.data())!=nullptr,"Wrong actual trajectory color panel");
         auto *choose=panel->findChild<QPushButton *>((index==0?"visualChoose_":"bodyChoose_")+QString("OrbitColor"));reveal(choose);
         auto pick=[&](bool accept) {
            std::exception_ptr failure;QTimer::singleShot(0,panel,[&] {
               try {auto *dialog=panel->findChild<QColorDialog *>();require(dialog,"Trajectory color picker absent");dialog->setCurrentColor(colors[index]);acceptanceView(*dialog);auto *buttons=dialog->findChild<QDialogButtonBox *>();require(buttons,"Trajectory color picker buttons absent");QTest::mouseClick(buttons->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel),Qt::LeftButton);}
               catch(...){failure=std::current_exception();for(auto *dialog:panel->findChildren<QDialog *>())dialog->reject();}
            });QTest::mouseClick(choose,Qt::LeftButton);if(failure)std::rethrow_exception(failure);events();
         };
         pick(false);require(!panel->hasChanges()&&rig.source->toPlainText()==before&&point->GetCurrentOrbitColor()==nativeBefore,"Trajectory color Cancel leaked mutation");
         pick(true);require(panel->hasChanges()&&rig.source->toPlainText()==before&&point->GetCurrentOrbitColor()==nativeBefore,"Trajectory color pending edit leaked mutation");
         auto *buttons=panel->findChild<QDialogButtonBox *>();require(buttons,"Trajectory color Apply absent");reveal(buttons->button(QDialogButtonBox::Apply));QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();require(!panel||!panel->hasChanges(),"Trajectory color Apply failed");
         const auto applied=rig.source->toPlainText();require(applied!=before&&applied.endsWith(mission),"Trajectory color changed mission source");rig.closePanels();
         rig.source->undo();require(rig.source->toPlainText()==before&&rig.window.buildScript(),"Trajectory color exact Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Trajectory color exact Redo failed");
         point=Moderator::Instance()->GetSpacePoint(name.toStdString());require(point&&QColor::fromRgb(point->GetCurrentOrbitColor()&0xffffff)==colors[index],"Native trajectory color differs after Redo");
         edits.append(QJsonObject{{"name",name},{"rgb",QJsonArray{colors[index].red(),colors[index].green(),colors[index].blue()}}});
      }
      const auto applied=rig.source->toPlainText();const auto path=rig.files.filePath("all trajectory colors Ω.script");require(rig.window.saveScriptTo(path)&&rig.window.loadScript(path)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Trajectory color Unicode roundtrip failed");
      const int undo=rig.source->document()->availableUndoSteps();require(rig.window.runMission()==MainWindow::RunResult::Completed,"Trajectory color mission failed");events();auto model=rig.window.plotReceiver()->model("Orb");require(model&&model->curves.size()==names.size(),"Trajectory family model incomplete");QJsonArray results;
      for(int index=0;index<names.size();++index) {
         const PlotCurve *selected=nullptr;for(const auto &curve:model->curves)if(curve.name==names[index])selected=&curve;
         require(selected&&selected->color==colors[index]&&selected->points.size()>=3,"Trajectory color missing from actual curve");
         for(const auto &point:selected->points)require(point.color==colors[index],"Trajectory sample lost selected color");
         results.append(QJsonObject{{"name",names[index]},{"color",selected->color.name()},{"sample_count",int(selected->points.size())}});
      }
      const auto raw=read(rig.report);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==1,"Trajectory color report row count differs");const auto columns=lines[0].split(',');require(columns.size()==5,"Trajectory color report width differs");const auto expected=circular(orbits[0],20);const double truth[]={expected[0],expected[1],expected[3],expected[4],41};QJsonArray values;
      for(int i=0;i<5;++i){bool ok=false;const double value=columns[i].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-truth[i])<(i<2?.00002:i<4?.00000003:1e-12),"Trajectory color edit changed independent circular state/sentinel");values.append(value);}
      require(rig.source->toPlainText()==applied&&rig.source->document()->availableUndoSteps()==undo&&read(path)==applied.toUtf8(),"Trajectory color Run mutated source/history/file");
      rig.save("colors.script",applied.toUtf8());rig.save("colors.csv",raw);const QJsonObject proof{{"checkpoint","all-configured-trajectory-colors"},{"outcome","passed"},{"trajectory_count",5},{"celestial_body_count",4},{"edits",edits},{"results",results},{"report",values},{"facets",QJsonArray{"actual-spacecraft-and-celestial-body-color-pickers","cancel-pending-apply-exact-undo-redo-unicode-roundtrip","every-configured-curve-and-sample-has-selected-rgb","independent-circular-state-and-sentinel-source-preservation"}}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
