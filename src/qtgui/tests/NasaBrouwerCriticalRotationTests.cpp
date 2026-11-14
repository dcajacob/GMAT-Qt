#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTabWidget>
using namespace formaccept;
static void reveal(QWidget *control) {
 for(auto *p=control;p;p=p->parentWidget()) {
  for(auto *tabs:control->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
  if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(control);
 }
 events();require(control->isVisible()&&control->isEnabled(),"Critical mean control unreachable");
}
static QLineEdit *field(QWidget *panel,const QString &name) {
 QLineEdit *found=nullptr;int count=0;
 for(auto *form:panel->findChildren<QFormLayout *>())for(int row=0;row<form->rowCount();++row) {
  auto *labelItem=form->itemAt(row,QFormLayout::LabelRole),*valueItem=form->itemAt(row,QFormLayout::FieldRole);
  auto *label=labelItem?qobject_cast<QLabel *>(labelItem->widget()):nullptr;auto *widget=valueItem?valueItem->widget():nullptr;
  if(!label||label->text()!=name||!widget)continue;
  auto *edit=qobject_cast<QLineEdit *>(widget);if(!edit)edit=widget->findChild<QLineEdit *>();
  if(!edit||edit->isReadOnly())continue;found=edit;++count;
 }
 require(count==1,"Critical mean field absent/ambiguous");reveal(found);return found;
}
int main(int argc,char **argv) {
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaBrouwerCriticalRotation");
 try {
  require(argc==4,"Supply startup, independent Cartesian fixture and inclination key");
  const auto matrix=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();QJsonObject selected;
  for(auto value:matrix["cases"].toArray())if(value.toObject()["key"].toString()==argv[3])selected=value.toObject();require(!selected.isEmpty(),"Unknown critical case");
  Rig rig(QFileInfo(argv[1]).absoluteFilePath());const auto original=selected["cartesian"].toArray(),expected=selected["rotated_cartesian"].toArray();const QStringList names{"X","Y","Z","VX","VY","VZ"};
  QString source="% Critical Brouwer longitude symmetry Ω\nEarth.Mu = 398600.4418;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\n";
  for(int i=0;i<6;++i)source+="Sat."+names[i]+" = "+number(original[i].toDouble())+";\n";
  source+="Create Variable Keep;\nKeep = 41;\nCreate ReportFile States;\nStates.Filename = '"+rig.report+"';\nStates.WriteHeaders = false;\nStates.FixedWidth = false;\nStates.Delimiter = ',';\nStates.Precision = 17;\n";
  QString mission="BeginMissionSequence;\nReport States";for(auto name:names)mission+=" Sat.EarthMJ2000Eq."+name;mission+=";\nKeep = Keep + 1; % retain following command\n";source+=mission;rig.source->setPlainText(source);require(rig.window.buildScript(),"Critical Cartesian build failed");rig.save("initial.script",source.toUtf8());
  auto found=rig.resources->findItems("Sat",Qt::MatchExactly|Qt::MatchRecursive);require(found.size()==1,"Critical spacecraft absent");rig.resources->itemDoubleClicked(found.first(),0);events();auto *child=rig.area->activeSubWindow();auto *panel=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(panel,"Critical orbit editor absent");child->showNormal();child->resize(rig.area->viewport()->size());acceptanceView(*child);
  auto *representation=panel->findChild<QComboBox *>("resource_DisplayStateType");require(representation&&representation->findText("BrouwerMeanLong")>=0,"Critical mean choice absent");reveal(representation);representation->setCurrentText("BrouwerMeanLong");events();acceptanceView(*child);
  const QStringList elements{"SMA","ECC","INC","RAAN","AOP","MA"};QJsonArray mean;
  for(auto element:elements){bool ok=false;double value=field(panel,"BrouwerLong"+element)->text().toDouble(&ok);require(ok&&std::isfinite(value),"Critical mean preview nonfinite");mean.append(value);}
  require(rig.source->toPlainText()==source,"Critical mean preview committed source");
  auto *node=field(panel,"BrouwerLongRAAN");const double changed=std::fmod(mean[3].toDouble()+90.,360.);node->setText(number(changed));events();require(panel->hasChanges()&&rig.source->toPlainText()==source,"Critical pending rotation changed source");acceptanceView(*child);panel->requestApply();events();
  const auto applied=rig.source->toPlainText();require(applied!=source&&applied.endsWith(mission)&&!panel->hasChanges(),"Critical Apply failed");rig.closePanels();rig.source->undo();require(rig.source->toPlainText()==source&&rig.window.buildScript(),"Critical Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Critical Redo failed");
  QJsonArray phases;double maxp=0,maxv=0;
  for(int phase=0;phase<2;++phase) {
   if(phase){const auto path=rig.files.filePath("Critical Ω.script");require(rig.window.saveScriptTo(path)&&rig.window.loadScript(path)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Critical Unicode reopen failed");}
   rig.messages->clear();const auto result=rig.window.runMission();events();rig.save(QString::number(phase)+"-messages.txt",rig.messages->toPlainText().toUtf8());require(result==MainWindow::RunResult::Completed,"Critical mission failed");const auto bytes=read(rig.report);rig.save(QString::number(phase)+"-states.csv",bytes);const auto columns=QString::fromUtf8(bytes).trimmed().split(',');require(columns.size()==6,"Critical report not one Cartesian state");QJsonArray raw;
   for(int i=0;i<6;++i){bool ok=false;double value=columns[i].toDouble(&ok);require(ok&&std::isfinite(value),"Critical report nonfinite");raw.append(value);const double error=std::abs(value-expected[i].toDouble());if(i<3)maxp=std::max(maxp,error);else maxv=std::max(maxv,error);if(error>(i<3?1e-6:1e-9))std::cerr<<"CRITICAL_DIFF component="<<i<<" error="<<error<<std::endl;require(error<=(i<3?1e-6:1e-9),"Critical rotation disagrees with independent Cartesian geometry");}
   require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42&&rig.source->toPlainText()==applied,"Critical mission changed following command/source");
   auto *output=rig.window.findChild<QTreeWidget *>("Output");auto entries=output->findItems("States",Qt::MatchExactly|Qt::MatchRecursive);require(entries.size()==1,"Critical output entry absent");output->itemDoubleClicked(entries.first(),0);events();auto *viewer=rig.area->activeSubWindow();require(viewer&&dynamic_cast<ReportViewer *>(viewer->widget()),"Critical actual report viewer absent");auto *shown=viewer->findChild<QPlainTextEdit *>();require(shown&&shown->isReadOnly()&&shown->toPlainText().trimmed()==QString::fromUtf8(bytes).trimmed(),"Critical report viewer differs");acceptanceView(*viewer);rig.closePanels();phases.append(QJsonObject{{"phase",phase},{"raw",raw}});
  }
  rig.save("final.script",applied.toUtf8());QJsonObject proof{{"checkpoint","brouwer-critical-rotation"},{"outcome","passed"},{"case",selected["key"]},{"initial_cartesian",original},{"initial_mean_preview",mean},{"new_raan",changed},{"expected_rotated_cartesian",expected},{"phases",phases},{"max_position_error_km",maxp},{"max_velocity_error_km_s",maxv},{"scope","Critical-angle inverse/forward rotation covariance, not prototype parity or independent physical averaging."}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
