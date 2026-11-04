#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTableWidget>
#include <QTest>
using namespace formaccept;

static void near(double actual,double expected,double tolerance)
{
   if(!std::isfinite(actual)||std::abs(actual-expected)>tolerance)
      throw std::runtime_error(QString("Unit result %1 differs from independent %2 (tolerance %3)").arg(actual,0,'g',17).arg(expected,0,'g',17).arg(tolerance,0,'g',17).toStdString());
}
static void reveal(QWidget *control)
{
   require(control,"Unit input absent");
   for(auto *p=control;p;p=p->parentWidget()) {
      for(auto *tabs:control->window()->findChildren<QTabWidget *>()) if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(control);
   }
   events();require(control->isVisible()&&control->isEnabled(),"Unit input inaccessible");acceptanceView(*control);
}
static std::array<double,6> keplerian()
{
   // Independent perifocal-to-inertial rotation, literal km and degree inputs.
   const double radians=std::acos(-1.)/180.,i=30*radians,O=40*radians,w=50*radians,v=60*radians;
   const double p=7200*(1-.1*.1),r=p/(1+.1*std::cos(v)),speed=std::sqrt(398600.4415/p);
   const std::array<double,3> P{std::cos(O)*std::cos(w)-std::sin(O)*std::sin(w)*std::cos(i),std::sin(O)*std::cos(w)+std::cos(O)*std::sin(w)*std::cos(i),std::sin(w)*std::sin(i)};
   const std::array<double,3> Q{-std::cos(O)*std::sin(w)-std::sin(O)*std::cos(w)*std::cos(i),-std::sin(O)*std::sin(w)+std::cos(O)*std::cos(w)*std::cos(i),std::cos(w)*std::sin(i)};
   std::array<double,6> result;
   for(int axis=0;axis<3;++axis){result[axis]=r*(std::cos(v)*P[axis]+std::sin(v)*Q[axis]);result[axis+3]=speed*(-std::sin(v)*P[axis]+(.1+std::cos(v))*Q[axis]);}
   return result;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaUnitInputs");
   try {
      require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());QJsonArray phases,inputs;
      const QStringList xyz{"X","Y","Z","VX","VY","VZ"};
      const std::array<double,6> cartesian{7000,-1200.25,400.5,.125,7.25,-.5};
      for(const QString phase:QStringList{"Cartesian","Keplerian","Burn","Representation"}) {
         const bool burn=phase=="Burn",preserve=phase=="Representation",degrees=phase=="Keplerian"||preserve;
         QString script="% Unit-sensitive GUI acceptance Ω\nCreate Spacecraft Sat;\nSat.NAIFId = -10009999;\nSat.NAIFIdReferenceFrame = -9009999;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nCreate Variable Keep;\nKeep = 41;\n";
         if(degrees)script+="Sat.DisplayStateType = Keplerian;\nSat.SMA = 6800;\nSat.ECC = .05;\nSat.INC = 10;\nSat.RAAN = 20;\nSat.AOP = 30;\nSat.TA = 40;\n";
         else script+="Sat.DisplayStateType = Cartesian;\nSat.X = 6800;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 0;\n";
         if(preserve)script.replace("Sat.SMA = 6800;\nSat.ECC = .05;\nSat.INC = 10;\nSat.RAAN = 20;\nSat.AOP = 30;\nSat.TA = 40;","Sat.SMA = 7200;\nSat.ECC = .1;\nSat.INC = 30;\nSat.RAAN = 40;\nSat.AOP = 50;\nSat.TA = 60;");
         if(burn) {for(int n=0;n<6;++n)script+="Sat."+xyz[n]+" = "+number(cartesian[n])+";\n";script+="Create ImpulsiveBurn Kick;\nKick.CoordinateSystem = EarthMJ2000Eq;\nKick.DecrementMass = false;\nKick.Element1 = 0;\nKick.Element2 = 0;\nKick.Element3 = 0;\n";}
         script+="Create ReportFile Values;\nValues.Filename = '"+rig.report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nBeginMissionSequence;\n";
         if(burn)script+="Maneuver 'Literal inertial km per second' Kick(Sat);\n";
         script+="Report Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Keep;\n% preserved unit tail Ω\n";
         rig.source->setPlainText(script);rig.save(phase+".input.script",script.toUtf8());const bool built=rig.window.buildScript();if(!built)rig.save(phase+".build-error.txt",rig.messages->toPlainText().toUtf8());require(built,"Unit fixture build failed");rig.source->document()->setModified(false);
         const QString object=burn?"Kick":"Sat";auto *configured=Moderator::Instance()->GetConfiguredObject(object.toStdString());const auto canonical=configured->GetGeneratingString(Gmat::SCRIPTING);
         const auto matches=rig.resources->findItems(object,Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Unit resource missing");rig.resources->itemDoubleClicked(matches.first(),0);events();
         auto *child=rig.area->activeSubWindow();auto *panel=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(panel,"Unit resource editor absent");
         const QStringList names=preserve?QStringList{}:burn?QStringList{"Element1","Element2","Element3"}:degrees?QStringList{"SMA","ECC","INC","RAAN","AOP","TA"}:xyz;
         const std::vector<double> values=burn?std::vector<double>{.125,-.25,.5}:degrees?std::vector<double>{7200,.1,30,40,50,60}:std::vector<double>(cartesian.begin(),cartesian.end());
         if(preserve) {
            auto *choice=panel->findChild<QComboBox *>("resource_DisplayStateType");require(choice&&choice->currentText()=="Keplerian","Representation-only source selector differs");reveal(choice);choice->showPopup();QTest::keyClick(choice,Qt::Key_Home);for(int n=0;n<choice->findText("Cartesian");++n)QTest::keyClick(choice,Qt::Key_Down);QTest::keyClick(choice,Qt::Key_Return);events();
            require(choice->currentText()=="Cartesian"&&rig.source->toPlainText()==script&&configured->GetGeneratingString(Gmat::SCRIPTING)==canonical,"Representation-only draft changed source/configured state");
         }
         for(int n=0;n<names.size();++n) {
            auto *line=panel->findChild<QLineEdit *>((burn?"burn_":"resource_")+names[n]);require(line&&!line->isReadOnly(),"Writable unit input missing");reveal(line);
            const QString unit=burn?"km/s":degrees?(n==0?"km":n==1?"":"deg"):n<3?"km":"km/s";
            bool displayed=unit.isEmpty();
            for(auto *form:panel->findChildren<QFormLayout *>())for(int row=0;row<form->rowCount();++row) {
               auto *f=form->itemAt(row,QFormLayout::FieldRole),*l=form->itemAt(row,QFormLayout::LabelRole);auto *widget=f?f->widget():nullptr;
               if(!widget||(widget!=line&&!widget->isAncestorOf(line)))continue;
               if(auto *label=l?qobject_cast<QLabel *>(l->widget()):nullptr)displayed=displayed||(label->isVisible()&&label->text().contains(unit));
               for(auto *label:widget->findChildren<QLabel *>())displayed=displayed||(label->isVisible()&&label->text()==unit);
            }
            require(displayed,"Expected units not visible beside input");QTest::mouseClick(line,Qt::LeftButton);QTest::keyClick(line,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(line,number(values[n]));QTest::keyClick(line,Qt::Key_Tab);events();
            require(line->text()==number(values[n])&&rig.source->toPlainText()==script&&configured->GetGeneratingString(Gmat::SCRIPTING)==canonical,"Pending unit input mutated configured/source graph");
            inputs.append(QJsonObject{{"phase",phase},{"field",names[n]},{"unit",unit},{"typed",values[n]},{"visible_label",displayed}});
         }
         require(panel->hasChanges(),"Unit draft not dirty");auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");require(buttons,"Unit Apply missing");reveal(buttons->button(QDialogButtonBox::Apply));QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();if(panel->hasChanges()){QStringList labels;for(auto *label:panel->findChildren<QLabel *>())if(label->isVisible()&&!label->text().isEmpty())labels.append(label->text());rig.save(phase+".apply-error.txt",labels.join("\n").toUtf8());rig.save(phase+".apply-messages.txt",rig.messages->toPlainText().toUtf8());}require(!panel->hasChanges(),"Unit Apply failed");
         const auto applied=rig.source->toPlainText();require(applied!=script&&applied.mid(applied.indexOf("BeginMissionSequence;"))==script.mid(script.indexOf("BeginMissionSequence;")),"Unit Apply changed mission");rig.closePanels();
         rig.source->undo();require(rig.source->toPlainText()==script&&rig.window.buildScript(),"Unit Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Unit Redo failed");
         const auto saved=rig.files.filePath(phase+" input Ω.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Unit Unicode roundtrip failed");
         const auto sourceBefore=rig.source->toPlainText();const int undo=rig.source->document()->availableUndoSteps();const auto satBefore=Moderator::Instance()->GetConfiguredObject("Sat")->GetGeneratingString(Gmat::SCRIPTING);
         require(rig.window.runMission()==MainWindow::RunResult::Completed,"Unit mission failed");events();const auto raw=read(rig.report);rig.save(phase+".csv",raw);rig.save(phase+".script",applied.toUtf8());
         const auto rows=QString::fromUtf8(raw).trimmed().split('\n');require(rows.size()==1,"Unit report must contain one row");const auto cols=rows[0].split(',');require(cols.size()==7,"Unit report schema differs");
         auto expected=degrees?keplerian():cartesian;if(burn){expected[3]+=.125;expected[4]-=.25;expected[5]+=.5;}
         QJsonArray actual,want;for(int n=0;n<7;++n){bool ok=false;const double result=cols[n].toDouble(&ok);require(ok,"Unit report not numeric");near(result,n<6?expected[n]:41,n<3?1e-7:n<6?1e-10:0);actual.append(result);if(n<6)want.append(expected[n]);else want.append(41);}
         const auto satAfter=Moderator::Instance()->GetConfiguredObject("Sat")->GetGeneratingString(Gmat::SCRIPTING);
         rig.save(phase+".configured-before.txt",QByteArray::fromStdString(satBefore));rig.save(phase+".configured-after.txt",QByteArray::fromStdString(satAfter));
         require(rig.source->toPlainText()==sourceBefore,"Unit Run mutated source");require(rig.source->document()->availableUndoSteps()==undo,"Unit Run mutated source history");require(satAfter==satBefore,"Unit Run mutated configured graph");
         phases.append(QJsonObject{{"phase",phase},{"actual",actual},{"independent_expected",want}});
      }
      const QJsonObject proof{{"checkpoint","unit-sensitive-inputs"},{"outcome","passed"},{"phases",phases},{"phase_count",4},{"inputs",inputs},{"input_count",15},{"unit_families",QJsonArray{"km","km/s","deg"}},
         {"facets",QJsonArray{"actual-tree-keyboard-entry-and-visible-units","pending-apply-undo-redo-unicode-roundtrip","literal-cartesian-km-and-km-per-second-state","independent-degree-keplerian-inertial-rotation","literal-inertial-km-per-second-maneuver","source-configured-graph-and-sentinel-preservation","selector-only-change-preserves-all-six-input-components"}}};
      require(inputs.size()==15&&phases.size()==4,"Unit input matrix incomplete");rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
