#include "FormationAcceptance.hpp"
#include "AttitudeDialog.hpp"
#include "Attitude.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "CoordinateSystem.hpp"
#include <QAbstractItemView>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
using namespace formaccept;

static void reveal(QWidget *widget) {
 require(widget,"Declared-frame attitude control missing");
 for(auto *parent=widget;parent;parent=parent->parentWidget()) {
  if(auto *tabs=qobject_cast<QTabWidget *>(parent))for(int i=0;i<tabs->count();++i)
   if(tabs->widget(i)==widget||tabs->widget(i)->isAncestorOf(widget))tabs->setCurrentIndex(i);
  if(auto *scroll=qobject_cast<QScrollArea *>(parent))scroll->ensureWidgetVisible(widget);
 }
 events();require(widget->isVisible()&&widget->isEnabled(),"Declared-frame attitude control unreachable");acceptanceView(*widget);
}
static QJsonObject graph() {
 QJsonObject result;auto *moderator=Moderator::Instance();
 for(const auto &name:moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT))if(auto *object=moderator->GetConfiguredObject(name))
  result[QString::fromStdString(name)]=QString::fromStdString(object->GetGeneratingString(Gmat::SCRIPTING));
 return result;
}
int main(int argc,char **argv) {
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
 app.setOrganizationName("GMATTests");app.setApplicationName("NasaAttitudeDeclaredFrames");
 try {
  require(argc==4,"Expected startup, declared-frame fixture and case key");
  const auto fixturePath=QFileInfo(argv[2]).absoluteFilePath();const auto matrix=QJsonDocument::fromJson(read(fixturePath)).object();
  const QString key=argv[3];QJsonObject spec;for(auto entry:matrix["cases"].toArray())if(entry.toObject()["key"]==key)spec=entry.toObject();
  require(!spec.isEmpty(),"Unknown declared attitude frame");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
  auto setup=spec["setup"].toString();setup.replace("@CERES@",QFileInfo(argv[1]).absolutePath()+"/../data/planetary_ephem/spk/ceres_1900_2100.bsp");
  setup.replace("@STATION@",QFileInfo(fixturePath).absolutePath()+"/frame-consumer-station-origin.bsp");
  // Preserve complete native-graph identity across script rebuilds. Default
  // NAIF IDs are allocated afresh, including on unused fixture spacecraft.
  auto spacecraftDeclarations=QRegularExpression("Create\\s+Spacecraft\\s+([^;]+);").globalMatch(setup);int spacecraftIdentity=0;QString identityAssignments;
  while(spacecraftDeclarations.hasNext())for(const auto &name:spacecraftDeclarations.next().captured(1).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) {
   ++spacecraftIdentity;if(name!="Sat")identityAssignments+=name+".NAIFId = "+QString::number(-100800-spacecraftIdentity)+";\n"+name+".NAIFIdReferenceFrame = "+QString::number(-900800-spacecraftIdentity)+";\n";
  }
  setup+=identityAssignments;
  setup+="Sat.NAIFId = -100715;\nSat.NAIFIdReferenceFrame = -9000715;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\n";
  const QStringList fields{"X","Y","Z","VX","VY","VZ"};const auto state=matrix["input_state"].toArray();
  for(int i=0;i<6;++i)setup+="Sat."+fields[i]+" = "+number(state[i].toDouble())+";\n";
  setup+="Sat.Attitude = Spinner;\nSat.AttitudeCoordinateSystem = EarthMJ2000Eq;\nSat.AttitudeDisplayStateType = Quaternion;\nSat.AttitudeRateDisplayStateType = AngularVelocity;\nSat.Q1 = 0.1;\nSat.Q2 = 0.2;\nSat.Q3 = 0.3;\nSat.Q4 = "+number(std::sqrt(.86))+";\nSat.AngularVelocityX = 0.1;\nSat.AngularVelocityY = 0.2;\nSat.AngularVelocityZ = 0.3;\nCreate Variable AttitudeKeep;\nAttitudeKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+rig.report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\n";
  QString columns="Sat.A1ModJulian";for(int i=1;i<=3;++i)for(int j=1;j<=3;++j)columns+=" Sat.DCM"+QString::number(i)+QString::number(j);
  columns+=" Sat.AngularVelocityX Sat.AngularVelocityY Sat.AngularVelocityZ";for(auto field:fields)columns+=" Sat.EarthMJ2000Eq."+field;columns+=" AttitudeKeep";
  QString mission="BeginMissionSequence;\n";for(auto sample:spec["samples"].toArray())mission+="Sat.A1ModJulian = "+number(sample.toObject()["epoch"].toDouble())+";\nReport Values "+columns+";\n";
  mission+="AttitudeKeep = AttitudeKeep + 1;\nReport Values AttitudeKeep;\n% Preserve declared attitude tail Ω\n";
  const QString source=setup+mission,saved=rig.files.filePath("Declared attitude Ω.script");rig.source->setPlainText(source);rig.save("initial.script",source.toUtf8());rig.save("case.json",QJsonDocument(spec).toJson());
  const bool built=rig.window.buildScript();events();rig.save("build.messages.txt",rig.messages->toPlainText().toUtf8());if(!built)std::cerr<<rig.messages->toPlainText().toStdString();require(built,"Declared attitude baseline Build failed");
  require(rig.window.saveScriptTo(saved)&&read(saved)==source.toUtf8(),"Declared attitude baseline save differs");const auto baseline=graph();const int undo=rig.source->document()->availableUndoSteps();
  QPointer<QMdiSubWindow> owner;QPointer<ResourceEditor> panel;QPointer<AttitudeDialog> editor;int selections=0;
  auto stable=[&]{require(rig.source->toPlainText()==source&&rig.source->document()->availableUndoSteps()==undo&&read(saved)==source.toUtf8()&&graph()==baseline,"Declared attitude draft escaped source/history/file/native isolation");};
  auto open=[&]{auto items=rig.resources->findItems("Sat",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Declared attitude spacecraft tree identity differs");rig.resources->expandAll();rig.resources->scrollToItem(items[0]);rig.resources->setCurrentItem(items[0]);reveal(rig.resources);rig.resources->setFocus();QTest::keyClick(rig.resources,Qt::Key_Return);events();owner=rig.area->activeSubWindow();panel=owner?dynamic_cast<ResourceEditor *>(owner->widget()):nullptr;editor=panel?dynamic_cast<AttitudeDialog *>(panel->findChild<QDialog *>("attitudeDialog")):nullptr;require(owner&&panel&&editor,"Actual declared attitude editor missing");owner->showNormal();owner->move(0,0);owner->resize(rig.area->viewport()->size());reveal(editor);};
  auto select=[&](const QString &name,const QString &value){auto *combo=editor->findChild<QComboBox *>(name);require(combo&&combo->findText(value)>=0,"Declared attitude choice absent");reveal(combo);QTest::mouseClick(combo,Qt::LeftButton);events();auto *view=combo->view();require(view&&view->isVisible(),"Actual attitude popup hidden");acceptanceView(*view);QTest::keyClick(view,Qt::Key_Home);for(int i=0;i<combo->findText(value);++i)QTest::keyClick(view,Qt::Key_Down);QTest::keyClick(view,Qt::Key_Return);events();require(editor->findChild<QComboBox *>(name)->currentText()==value,"Actual attitude popup did not retain choice");++selections;};
  auto inspect=[&]{require(editor->findChild<QComboBox *>("attitudeModel")->currentText()=="CoordinateSystemFixed","Declared attitude model differs");auto *reference=editor->findChild<QComboBox *>("attitude_AttitudeCoordinateSystem");require(reference&&!reference->isEditable()&&reference->currentText()=="CreatedFrame"&&reference->findText("AttitudeKeep")<0&&reference->findText("Sat")<0,"Declared attitude reference/type binding differs");for(const char *name:{"Q1","Q2","Q3","Q4","AngularVelocityX","AngularVelocityY","AngularVelocityZ"}){auto *widget=editor->findChild<QWidget *>(QString("attitude_")+name);require(!widget||!widget->isEnabled()||(qobject_cast<QLineEdit *>(widget)&&qobject_cast<QLineEdit *>(widget)->isReadOnly()),"Fixed model exposes ineffective editable state/rate");}};
  auto fill=[&]{select("attitudeModel","CoordinateSystemFixed");select("attitude_AttitudeCoordinateSystem","CreatedFrame");select("attitude_EulerAngleSequence","123");inspect();stable();};
  open();fill();panel->discardChanges();require(owner->close(),"Declared attitude draft discard failed");events();stable();open();fill();
  auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");auto *apply=buttons?buttons->button(QDialogButtonBox::Apply):nullptr;reveal(apply);QTest::mouseClick(apply,Qt::LeftButton);events();panel=dynamic_cast<ResourceEditor *>(owner->widget());require(panel&&!panel->hasChanges()&&owner->isVisible(),"Declared attitude Apply did not remain clean/open");
  auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("Sat"));auto *frame=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("CreatedFrame"));
  require(sat&&frame&&sat->GetStringParameter("Attitude")=="CoordinateSystemFixed"&&sat->GetStringParameter("AttitudeCoordinateSystem")=="CreatedFrame"&&sat->GetStringParameter("EulerAngleSequence")=="123"&&frame->GetStringParameter("Axes")==spec["axis"].toString().toStdString(),"Declared attitude native model/sequence/frame/axes differs");
  auto *origin=frame->GetOrigin();require(origin&&origin->IsOfType(spec["origin_class"].toString().toStdString()),"Declared attitude native origin category differs");
  const QString applied=rig.source->toPlainText();rig.save("accepted.script",applied.toUtf8());require(applied!=source&&applied.endsWith(mission),"Declared attitude Apply changed mission");const auto accepted=graph();require(owner->close(),"Declared attitude clean Close failed");events();
  rig.source->undo();try {require(rig.source->toPlainText()==source&&rig.window.buildScript()&&graph()==baseline,"Declared attitude exact Undo/native graph failed");} catch(...) {rig.save("undo-source.script",rig.source->toPlainText().toUtf8());rig.save("undo-before-native.json",QJsonDocument(baseline).toJson());rig.save("undo-after-native.json",QJsonDocument(graph()).toJson());throw;}rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript()&&graph()==accepted,"Declared attitude exact Redo/native graph failed");
  require(rig.window.saveScriptTo(saved)&&read(saved)==applied.toUtf8()&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied&&graph()==accepted,"Declared attitude Unicode save/reopen/native identity failed");open();inspect();require(!panel->hasChanges()&&owner->close(),"Reopened declared attitude editor dirty or unclosable");events();
  const auto before=graph();const int history=rig.source->document()->availableUndoSteps();const auto status=rig.window.runMission();events();rig.save("run.messages.txt",rig.messages->toPlainText().toUtf8());if(status!=MainWindow::RunResult::Completed)std::cerr<<rig.messages->toPlainText().toStdString();require(status==MainWindow::RunResult::Completed,"Declared attitude mission failed");
  const auto raw=read(rig.report);rig.save("values.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==4&&lines.last().trimmed()=="42","Declared attitude report rows/following sentinel differ");QJsonArray actual,checks;double matrixError=0,rateError=0;
  for(int row=0;row<3;++row){const auto sample=spec["samples"].toArray()[row].toObject();const auto words=lines[row].split(',');require(words.size()==20,"Declared attitude report column count differs");QJsonArray values;for(auto word:words){bool ok=false;double value=word.toDouble(&ok);require(ok&&std::isfinite(value),"Declared attitude report nonfinite value");values.append(value);}require(std::abs(values[0].toDouble()-sample["epoch"].toDouble())<1e-10&&values.last().toDouble()==41,"Declared attitude report epoch/sentinel differs");
   for(int i=0;i<3;++i){for(int j=0;j<3;++j)matrixError=std::max(matrixError,std::abs(values[1+3*i+j].toDouble()-sample["rows"].toArray()[i].toArray()[j].toDouble()));rateError=std::max(rateError,std::abs(values[10+i].toDouble()-sample["angular_velocity_degrees_s"].toArray()[i].toDouble()));}
   checks.append(QJsonObject{{"sample",row},{"maximum_matrix_error",matrixError},{"maximum_rate_error_degrees_s",rateError}});rig.save("checks.json",QJsonDocument(checks).toJson());
   if(matrixError>spec["matrix_bound"].toDouble()||rateError>spec["rate_bound_degrees_s"].toDouble())std::cerr<<"ATTITUDE_FRAME_DIFF "<<key.toStdString()<<" row="<<row<<" matrix="<<matrixError<<" rate="<<rateError<<std::endl;
   require(matrixError<=spec["matrix_bound"].toDouble(),"Declared attitude matrix differs from independent frame oracle");require(rateError<=spec["rate_bound_degrees_s"].toDouble(),"Declared attitude angular rate differs from independent basis derivative/NASA convention");
   for(int i=0;i<6;++i)require(std::abs(values[13+i].toDouble()-state[i].toDouble())<1e-9,"Declared attitude changed unrelated inertial spacecraft state");actual.append(values);
  }
  auto *runtime=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));require(runtime&&runtime->GetStringParameter("AttitudeCoordinateSystem")=="CreatedFrame","Runtime attitude frame binding differs");const auto nativeMatrix=runtime->GetAttitude(runtime->GetEpoch());const auto nativeRate=runtime->GetAngularVelocity(runtime->GetEpoch());const auto last=actual.last().toArray();
  for(int i=0;i<3;++i){for(int j=0;j<3;++j)require(std::abs(nativeMatrix(i,j)-last[1+3*i+j].toDouble())<1e-13,"Native and reported attitude matrices differ");require(std::abs(nativeRate[i]*180/std::acos(-1.)-last[10+i].toDouble())<1e-13,"Native and reported attitude rates differ");}
  require(rig.source->toPlainText()==applied&&rig.source->document()->availableUndoSteps()==history&&read(saved)==applied.toUtf8()&&graph()==before,"Declared attitude mission changed configured source/history/file/native graph");
  auto *outputs=rig.window.findChild<QTreeWidget *>("Output");auto entries=outputs->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(entries.size()==1,"Declared attitude output tree identity differs");outputs->itemDoubleClicked(entries[0],0);events();auto *viewer=rig.area->activeSubWindow();auto *text=viewer?viewer->findChild<QPlainTextEdit *>():nullptr;require(viewer&&dynamic_cast<ReportViewer *>(viewer->widget())&&text&&text->isReadOnly()&&text->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Declared attitude actual viewer/raw report differs");viewer->showNormal();viewer->resize(rig.area->viewport()->size());acceptanceView(*viewer);
  QJsonObject proof{{"checkpoint","attitude-declared-frame"},{"outcome","passed"},{"case",key},{"axis",spec["axis"]},{"origin_class",spec["origin_class"]},{"model","CoordinateSystemFixed"},{"reference","CreatedFrame"},{"actual_popups",selections},{"actual",actual},{"checks",checks},{"maximum_matrix_error",matrixError},{"maximum_rate_error_degrees_s",rateError},{"rate_convention",spec["rate_convention"]},{"facets",QJsonArray{"actual-attitude-model-reference-sequence-popups-and-origin-axis-binding","inactive-ineffective-inputs-pending-discard-Apply-exact-Undo-Redo","Unicode-reopen-complete-native-graph-source-history-isolation","independent-three-epoch-frame-matrix-angular-rate-and-state-witness","actual-native-attitude-and-complete-read-only-output-viewer"}},{"scope",matrix["scope"]}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
