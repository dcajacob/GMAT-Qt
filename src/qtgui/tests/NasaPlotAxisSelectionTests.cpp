#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "QtPlotReceiver.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>
#include <QTimer>
#include <functional>
using namespace formaccept;

static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action)
{
   QTimer::singleShot(0,owner,[owner,&failure,action] {
      try {action();} catch (...) {
         failure=std::current_exception();
         for(auto *dialog:owner->findChildren<QDialog *>())dialog->reject();
         if(auto *dialog=qobject_cast<QDialog *>(owner))dialog->reject();
      }
   });
}
static void finish(QDialog *dialog,bool accept=true)
{
   require(dialog,"Axis picker/dialog absent");
   auto *buttons=dialog->findChild<QDialogButtonBox *>();
   require(buttons,"Axis dialog buttons absent");auto *button=buttons->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel);
   require(button&&button->isEnabled(),"Axis dialog acceptance disabled");acceptanceView(*dialog);QTest::mouseClick(button,Qt::LeftButton);
}
static void select(ReportParameterDialog *picker,int type,bool multiple)
{
   require(picker,"Axis parameter picker absent");acceptanceView(*picker);
   auto *entry=picker->findChild<QComboBox *>("reportParameterEntry");require(entry&&entry->isEditable(),"Axis parameter entry absent");
   if(multiple)picker->findChild<QListWidget *>("reportSelectedParameters")->clear();
   if(type==0) {
      picker->findChild<QComboBox *>("reportPropertyObject")->setCurrentText("Sat");
      auto *property=picker->findChild<QComboBox *>("reportPropertyType");require(property->findText("X")>=0,"Scalar X browser entry absent");property->setCurrentText("X");
      picker->findChild<QComboBox *>("reportPropertyDependency")->setCurrentText("EarthMJ2000Eq");
      QTest::mouseClick(picker->findChild<QPushButton *>("reportUseReference"),Qt::LeftButton);
      require(entry->currentText()=="Sat.EarthMJ2000Eq.X","Scalar property browser produced wrong reference");
   } else {
      QTest::mouseClick(entry->lineEdit(),Qt::LeftButton);QTest::keyClick(entry->lineEdit(),Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(entry->lineEdit(),type==1?"V":"A");
   }
   if(type==2) {
      picker->findChild<QSpinBox *>("reportArrayRow")->setValue(1);picker->findChild<QSpinBox *>("reportArrayColumn")->setValue(1);
      QTest::mouseClick(picker->findChild<QPushButton *>("reportAddElement"),Qt::LeftButton);
   } else if(multiple)QTest::mouseClick(picker->findChild<QPushButton *>("reportAddParameter"),Qt::LeftButton);
   finish(picker);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaPlotAxisSelection");
   try {
      require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QStringList references{"Sat.EarthMJ2000Eq.X","V","A(1,1)"};const double truth[]={7000,3,2};QJsonArray cases;
      for(int x=0;x<3;++x)for(int y=0;y<3;++y) {
         const QString script="% Both-axis numeric family witness Ω\nCreate Spacecraft Sat Runner;\nSat.NAIFId = -10009999;\nSat.NAIFIdReferenceFrame = -9009999;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 100;\nSat.Z = 200;\nSat.VX = 0;\nSat.VY = 7;\nSat.VZ = 1;\nCreate Variable V I Keep T;\nV = 3;\nKeep = 41;\nCreate Array A[1,1];\nA(1,1) = 2;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nCreate XYPlot Graph;\nGraph.XVariable = Sat.ElapsedSecs;\nGraph.YVariables = {Sat.EarthMJ2000Eq.Y};\nCreate ReportFile Values;\nValues.Filename = '"+rig.report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nBeginMissionSequence;\nFor I = 0:2;\nT = 10*(I+1);\nPropagate P(Runner) {Runner.ElapsedSecs = T};\nReport Values Sat.EarthMJ2000Eq.X V A(1,1) I Keep;\nEndFor;\n% preserve axis tail Ω\n";
         rig.source->setPlainText(script);require(rig.window.buildScript(),"Axis fixture build failed");rig.source->document()->setModified(false);
         auto *configured=Moderator::Instance()->GetConfiguredObject("Graph");const auto original=configured->GetGeneratingString(Gmat::SCRIPTING);
         const auto matches=rig.resources->findItems("Graph",Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"XY resource tree item absent");rig.resources->itemDoubleClicked(matches.first(),0);events();
         auto *child=rig.area->activeSubWindow();auto *panel=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(panel,"XY resource editor absent");acceptanceView(*panel);
         auto *setup=panel->findChild<QPushButton *>("editXYPlot");require(setup,"XY setup absent");std::exception_ptr failure;
         later(panel,failure,[&] {
            auto *dialog=panel->findChild<QDialog *>("xyPlotDialog");require(dialog,"XY setup dialog absent");acceptanceView(*dialog);
            later(dialog,failure,[&] {select(dynamic_cast<ReportParameterDialog *>(dialog->findChild<QDialog *>("reportParameterDialog")),x,false);});
            QTest::mouseClick(dialog->findChild<QPushButton *>("xySelectX"),Qt::LeftButton);if(failure)std::rethrow_exception(failure);
            require(dialog->findChild<QLineEdit *>("xyXVariable")->text()==references[x],"Accepted X selection differs");
            // A cancelled subsequent picker must preserve the committed draft.
            later(dialog,failure,[&] {auto *picker=dialog->findChild<QDialog *>("reportParameterDialog");picker->findChild<QComboBox *>("reportParameterEntry")->setEditText("V");finish(picker,false);});
            QTest::mouseClick(dialog->findChild<QPushButton *>("xySelectX"),Qt::LeftButton);if(failure)std::rethrow_exception(failure);
            require(dialog->findChild<QLineEdit *>("xyXVariable")->text()==references[x],"Cancelled X picker changed draft");
            later(dialog,failure,[&] {select(dynamic_cast<ReportParameterDialog *>(dialog->findChild<QDialog *>("reportParameterDialog")),y,true);});
            QTest::mouseClick(dialog->findChild<QPushButton *>("xySelectY"),Qt::LeftButton);if(failure)std::rethrow_exception(failure);
            auto *ys=dialog->findChild<QListWidget *>("xyYVariables");require(ys->count()==1&&ys->item(0)->text()==references[y],"Accepted Y selection differs");finish(dialog);
         });
         QTest::mouseClick(setup,Qt::LeftButton);if(failure)std::rethrow_exception(failure);
         require(panel->hasChanges()&&rig.source->toPlainText()==script&&configured->GetGeneratingString(Gmat::SCRIPTING)==original,"Axis draft changed applied source/resource");
         auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();require(!panel->hasChanges(),"Axis Apply failed");
         const auto applied=rig.source->toPlainText();require(applied.mid(applied.indexOf("BeginMissionSequence;"))==script.mid(script.indexOf("BeginMissionSequence;")),"Axis Apply changed mission");
         rig.closePanels();rig.source->undo();require(rig.source->toPlainText()==script&&rig.window.buildScript(),"Axis Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Axis Redo failed");
         const QString tag=QString::number(x)+"-"+QString::number(y);const auto saved=rig.files.filePath("axes "+tag+" Ω.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Axis Unicode roundtrip failed");
         configured=Moderator::Instance()->GetConfiguredObject("Graph");require(configured->GetStringParameter("XVariable")==references[x].toStdString()&&configured->GetStringArrayParameter("YVariables")==StringArray{references[y].toStdString()},"Reopened native axis references differ");
         const int undo=rig.source->document()->availableUndoSteps();require(rig.window.runMission()==MainWindow::RunResult::Completed,"Axis mission failed");events();
         auto model=rig.window.plotReceiver()->model("Graph");require(model&&model->curves.size()==1,"Selected XY output absent");QJsonArray points;
         for(const auto &curve:model->curves)for(const auto &point:curve.points) {require(point.x==truth[x]&&point.y==truth[y],"Axis output differs from independent literal values");points.append(QJsonArray{point.x,point.y});}
         rig.save("axis-points-diagnostic.json",QJsonDocument(points).toJson());require(points.size()>=3,"Axis output has insufficient report samples");const auto raw=read(rig.report);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==3,"Axis raw report row count differs");QJsonArray report;
         for(int row=0;row<3;++row) {const auto columns=lines[row].split(',');require(columns.size()==5,"Axis raw report schema differs");QJsonArray values;for(int col=0;col<5;++col){bool ok=false;const double value=columns[col].toDouble(&ok);require(ok&&value==(col<3?truth[col]:col==3?row:41),"Axis raw report differs from independent literals/index/sentinel");values.append(value);}report.append(values);}
         require(rig.source->toPlainText()==applied&&rig.source->document()->availableUndoSteps()==undo&&read(saved)==applied.toUtf8(),"Axis Run mutated source/history/file");
         rig.save("axes-"+tag+".script",applied.toUtf8());rig.save("axes-"+tag+".csv",raw);
         cases.append(QJsonObject{{"x",references[x]},{"y",references[y]},{"independent_expected",QJsonArray{truth[x],truth[y]}},{"points",points},{"raw_report",report}});rig.save("axis-cases.json",QJsonDocument(cases).toJson());
      }
      require(cases.size()==9,"Axis family cross-product incomplete");const QJsonObject proof{{"checkpoint","both-axis-numeric-families"},{"outcome","passed"},{"axis_families",3},{"matrix_cases",9},{"cases",cases},{"facets",QJsonArray{"accepted-resource-picker-both-axes-three-numeric-families","scalar-property-frame-browser-and-one-by-one-array-element","cancel-pending-apply-exact-undo-redo-unicode-reopen","independent-literal-model-samples-and-raw-report","source-history-file-and-sentinel-preservation"}}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
