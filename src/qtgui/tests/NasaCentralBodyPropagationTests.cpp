#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "ODEModel.hpp"
#include "PhysicalModel.hpp"
#include "CoordinateSystem.hpp"
#include "CoordinateConverter.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QPointer>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <algorithm>
using namespace formaccept;

static void reveal(QWidget *w) {
 require(w,"Central-body widget missing");
 for(auto *p=w;p;p=p->parentWidget()) {
  for(auto *tabs:w->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
  if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(w);
 }
 events();require(w->isVisible()&&w->isEnabled(),"Central-body control unreachable");
}
static QJsonObject configured() {
 QJsonObject result;auto *m=Moderator::Instance();
 for(const auto &name:m->GetListOfObjects(Gmat::UNKNOWN_OBJECT))if(auto *o=m->GetConfiguredObject(name))
  result[QString::fromStdString(name)]=QString::fromStdString(o->GetGeneratingString(Gmat::SCRIPTING));
 return result;
}
static std::array<double,6> circular(double gm,double radius,double inclination,double seconds) {
 const double omega=std::sqrt(gm/(radius*radius*radius)),a=omega*seconds,c=std::cos(inclination),s=std::sin(inclination);
 return {radius*std::cos(a),radius*std::sin(a)*c,radius*std::sin(a)*s,
         -radius*omega*std::sin(a),radius*omega*std::cos(a)*c,radius*omega*std::cos(a)*s};
}
int main(int argc,char **argv) {
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
 app.setOrganizationName("GMATTests");app.setApplicationName("NasaCentralBodyPropagation");
 try {
  require(argc==4,"Expected startup, body matrix and case key");
  const auto matrix=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();QJsonObject spec;
  for(const auto &v:matrix["cases"].toArray())if(v.toObject()["key"].toString()==argv[3])spec=v.toObject();
  require(!spec.isEmpty(),"Central-body case absent");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
  const auto body=spec["body"].toString(),key=spec["key"].toString();const bool user=spec["user"].toBool();
  const double gm=spec["mu_km3_s2"].toDouble(),radius=spec["radius_km"].toDouble(),inclination=matrix["inclination_degrees"].toDouble()*std::acos(-1.)/180.;
  require(gm>0&&radius>0,"Central-body independent inputs invalid");
  const double period=2*std::acos(-1.)*std::sqrt(radius*radius*radius/gm),epoch=matrix["epoch_a1_mjd"].toDouble();
  const QStringList fields{"X","Y","Z","VX","VY","VZ"};
  QString source="% Central-body propagation binding Ω\n";
  if(user) {
   const auto kernel=QFileInfo(argv[1]).absolutePath()+"/../data/planetary_ephem/spk/ceres_1900_2100.bsp";
   require(QFileInfo(kernel).isFile(),"Central-body existing SPK fixture absent");
   source+="Create "+spec["type"].toString()+" "+body+";\n"+body+".CentralBody = "+QString(spec["type"].toString()=="Moon"?"Earth":"Sun")+";\n"+body+".NAIFId = 2000001;\n"+body+".OrbitSpiceKernelName = {'"+kernel+"'};\n"+body+".EquatorialRadius = 470;\n";
  }
  source+=body+".Mu = "+number(gm)+";\nCreate CoordinateSystem BodyFrame;\nBodyFrame.Origin = "+body+";\nBodyFrame.Axes = MJ2000Eq;\nCreate Spacecraft Sat;\nSat.NAIFId = -100200;\nSat.NAIFIdReferenceFrame = -900200;\nSat.CoordinateSystem = BodyFrame;\nSat.DisplayStateType = Cartesian;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '"+number(epoch)+"';\n";
  const auto initial=circular(gm,radius,inclination,0);for(int i=0;i<6;++i)source+="Sat."+fields[i]+" = "+number(initial[i])+";\n";
  source+="Create ForceModel Forces;\nForces.CentralBody = Earth;\nForces.PrimaryBodies = {};\nForces.PointMasses = {"+body+"};\nForces.Drag = None;\nForces.SRP = Off;\nForces.RelativisticCorrection = Off;\nCreate Propagator P;\nP.FM = Forces;\nP.Type = RungeKutta89;\nP.InitialStepSize = "+number(period/1000)+";\nP.MinStep = 0;\nP.MaxStep = "+number(period/100)+";\nP.Accuracy = 1e-13;\nCreate Variable Keep;\nKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+rig.report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\n";
  QString mission="BeginMissionSequence;\n";double previousFraction=0;
  for(const auto &fraction:matrix["phase_fractions"].toArray()) {
   if(fraction.toDouble()>0)mission+="Propagate P(Sat) {Sat.ElapsedSecs = "+number(period*(fraction.toDouble()-previousFraction))+"};\n";
   previousFraction=fraction.toDouble();
   mission+="Report Values Sat.A1ModJulian Sat.ElapsedSecs";for(const auto &field:fields)mission+=" Sat.BodyFrame."+field;mission+=" Keep;\n";
  }
  mission+="Keep = Keep + 1;\nReport Values Keep;\n% Preserve central-body tail Ω\n";source+=mission;
  rig.source->setPlainText(source);rig.save("baseline.script",source.toUtf8());rig.save("case.json",QJsonDocument(spec).toJson());
  const bool built=rig.window.buildScript();rig.save("build.messages.txt",rig.messages->toPlainText().toUtf8());require(built,"Central-body baseline Build failed");
  const auto baseline=configured();const int beforeUndo=rig.source->document()->availableUndoSteps();
  auto stable=[&]{require(rig.source->toPlainText()==source&&configured()==baseline&&rig.source->document()->availableUndoSteps()==beforeUndo,"Central-body pending edit escaped source/native/history isolation");};
  QPointer<QMdiSubWindow> owner;QPointer<ResourceEditor> panel;
  auto open=[&]{auto items=rig.resources->findItems("Forces",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Central-body resource entry ambiguous");rig.resources->expandAll();rig.resources->scrollToItem(items[0]);rig.resources->itemDoubleClicked(items[0],0);events();owner=rig.area->activeSubWindow();panel=owner?dynamic_cast<ResourceEditor *>(owner->widget()):nullptr;require(panel&&!panel->hasChanges(),"Central-body clean editor absent");owner->showNormal();owner->move(0,0);owner->resize(rig.area->viewport()->size());acceptanceView(*owner);};
  int typedEntries=0;
  auto select=[&]{auto *edit=panel->findChild<QLineEdit *>("resource_CentralBody");require(edit&&edit->text()=="Earth","Central-body baseline text field differs");reveal(edit);QTest::mouseClick(edit,Qt::LeftButton);QTest::keyClick(edit,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(edit,body);events();require(edit->text()==body&&panel->hasChanges(),"Central-body typed entry did not preserve pending selection");++typedEntries;acceptanceView(*owner);stable();};
  open();select();panel->discardChanges();require(owner->close(),"Central-body Cancel failed");events();stable();
  open();select();auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");require(buttons&&buttons->button(QDialogButtonBox::Apply)->isEnabled(),"Central-body Apply unavailable");reveal(buttons->button(QDialogButtonBox::Apply));QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();
  panel=dynamic_cast<ResourceEditor *>(owner->widget());rig.save("apply.messages.txt",rig.messages->toPlainText().toUtf8());require(panel&&!panel->hasChanges(),"Central-body Apply failed");
  const auto applied=rig.source->toPlainText();const auto accepted=configured();rig.save("accepted.script",applied.toUtf8());
  require(applied!=source&&applied.endsWith(mission),"Central-body Apply changed mission or made no edit");
  auto withoutForce=[](QString value){value.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?Forces\\.[^\\n]*\\n",QRegularExpression::MultilineOption));return value;};
  require(withoutForce(applied)==withoutForce(source),"Central-body Apply rewrote unrelated source");
  auto *fm=dynamic_cast<ODEModel *>(Moderator::Instance()->GetConfiguredObject("Forces"));require(fm&&fm->GetStringParameter("CentralBody")==body.toStdString(),"Central-body applied native identity differs");
  require(owner->close(),"Central-body applied editor Close failed");events();rig.source->undo();
  rig.save("undo.script",rig.source->toPlainText().toUtf8());rig.save("baseline.graph.json",QJsonDocument(baseline).toJson());
  const bool undoBuilt=rig.window.buildScript();rig.save("undo.graph.json",QJsonDocument(configured()).toJson());rig.save("undo.messages.txt",rig.messages->toPlainText().toUtf8());
  require(rig.source->toPlainText()==source&&undoBuilt&&configured()==baseline,"Central-body exact Undo/native restore failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript()&&configured()==accepted,"Central-body exact Redo/native restore failed");
  const auto saved=rig.files.filePath("Central body "+key+" Ω.script");require(rig.window.saveScriptTo(saved)&&read(saved)==applied.toUtf8()&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied&&configured()==accepted,"Central-body Unicode save/reopen differs");
  const int history=rig.source->document()->availableUndoSteps();const auto stateBefore=configured();const auto status=rig.window.runMission();events();rig.save("run.messages.txt",rig.messages->toPlainText().toUtf8());require(status==MainWindow::RunResult::Completed,"Central-body mission failed");
  const auto raw=read(rig.report);rig.save("results.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==5&&lines.last().trimmed()=="42","Central-body row count or following sentinel differs");QJsonArray rows,truth;double maxPosition=0,maxVelocity=0;
  QJsonArray timing;
  for(auto *command=Moderator::Instance()->GetFirstCommand();command;command=command->GetNext())if(command->IsOfType("Propagate")) {
   auto *setup=dynamic_cast<PropSetup *>(command->GetClone(0));require(setup&&setup->GetPropagator()&&setup->GetODEModel(),"Central-body timing clone absent");
   timing.append(QJsonObject{{"propagator_time_seconds",setup->GetPropagator()->GetTime()},{"ODE_time_seconds",setup->GetODEModel()->GetTime()}});
  }
  rig.save("propagation-timing.json",QJsonDocument(timing).toJson());
  require(timing.size()==3,"Central-body propagation timing cardinality differs");double dynamicsElapsed=0;QJsonArray dynamicsTimes;
  for(int i=0;i<4;++i) {
   const auto words=lines[i].split(',');require(words.size()==9,"Central-body Cartesian/clock/sentinel fields omitted");QJsonArray values;for(const auto &word:words){bool valid=false;const double value=word.toDouble(&valid);require(valid&&std::isfinite(value),"Central-body raw value invalid");values.append(value);}rows.append(values);
   const double seconds=period*matrix["phase_fractions"].toArray()[i].toDouble();require(std::abs(values[1].toDouble()-seconds)<matrix["elapsed_bound_seconds"].toDouble()&&std::abs((values[0].toDouble()-epoch)*86400-seconds)<matrix["elapsed_bound_seconds"].toDouble()&&values[8].toDouble()==41,"Central-body epoch/elapsed/sentinel differs");
   if(i>0){const auto segment=timing[i-1].toObject();const double duration=segment["propagator_time_seconds"].toDouble();require(std::isfinite(duration)&&duration>0&&duration==segment["ODE_time_seconds"].toDouble(),"Central-body native propagation/ODE duration differs");dynamicsElapsed+=duration;}
   require(std::abs(dynamicsElapsed-seconds)<matrix["elapsed_bound_seconds"].toDouble()&&std::abs(dynamicsElapsed-values[1].toDouble())<matrix["elapsed_bound_seconds"].toDouble(),"Central-body unrounded dynamics time differs from requested/reported clock");dynamicsTimes.append(dynamicsElapsed);
   const auto expected=circular(gm,radius,inclination,dynamicsElapsed);QJsonArray wanted;
   for(int c=0;c<6;++c){wanted.append(expected[c]);const double error=std::abs(values[c+2].toDouble()-expected[c]);if(c<3)maxPosition=std::max(maxPosition,error);else maxVelocity=std::max(maxVelocity,error);require(error<matrix[c<3?"position_bound_km":"velocity_bound_km_s"].toDouble(),"Central-body trajectory differs from independent inclined circular solution");}truth.append(wanted);
  }
  auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));auto *runtimeBody=Moderator::Instance()->GetSolarSystemInUse()->GetBody(body.toStdString());require(sat&&runtimeBody&&runtimeBody->GetTypeName()==spec["type"].toString().toStdString()&&runtimeBody->IsUserDefined()==user&&runtimeBody->GetGravitationalConstant()==gm,"Central-body native body type/model input differs");
  auto *frame=dynamic_cast<CoordinateSystem *>(sat->GetRefObject(Gmat::COORDINATE_SYSTEM,"BodyFrame"));require(frame&&frame->GetOrigin()->GetName()==body.toStdString(),"Central-body spacecraft report frame origin differs");Rvector6 internal;for(int c=0;c<6;++c)internal[c]=sat->GetState()[c];Rvector6 native;CoordinateConverter converter;converter.Convert(A1Mjd(sat->GetEpoch()),internal,sat->GetInternalCoordSystem(),native,frame);QJsonArray nativeValues;
  for(int c=0;c<6;++c){nativeValues.append(native[c]);require(std::abs(native[c]-rows.last().toArray()[c+2].toDouble())<(c<3?2e-6:1e-9),"Central-body actual native state differs from raw report");}
  QJsonArray bindings;int propagates=0;
  for(auto *command=Moderator::Instance()->GetFirstCommand();command;command=command->GetNext())if(command->IsOfType("Propagate")) {
   ++propagates;auto *setup=dynamic_cast<PropSetup *>(command->GetClone(0));auto *forces=setup?setup->GetODEModel():nullptr;require(forces&&forces->GetStringParameter("CentralBody")==body.toStdString(),"Actual Propagate clone central-body binding differs");int points=0;for(int i=0;i<forces->GetNumForces();++i){auto *force=forces->GetForce(i);if(force->IsOfType("PointMassForce")){++points;require(force->GetBodyName()==body.toStdString(),"Actual point-mass contributor uses another body");}else require(force->IsOfType("PointMassForce"),"Unexpected actual force contributor");}require(points==1,"Actual central point-mass cardinality differs");bindings.append(QJsonObject{{"central_body",body},{"point_mass",body},{"contributors",points}});
  }
  require(propagates==3&&rig.source->toPlainText()==applied&&rig.source->document()->availableUndoSteps()==history&&read(saved)==applied.toUtf8()&&configured()==stateBefore,"Central-body run changed configured graph/source/history/persistence");
  auto *outputs=rig.window.findChild<QTreeWidget *>("Output");auto items=outputs->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Central-body output tree entry missing");outputs->itemDoubleClicked(items[0],0);events();auto *viewer=rig.area->activeSubWindow();auto *text=viewer?viewer->findChild<QPlainTextEdit *>():nullptr;require(viewer&&dynamic_cast<ReportViewer *>(viewer->widget())&&text&&text->isReadOnly()&&text->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Central-body visible report/raw bytes differ");viewer->showNormal();viewer->resize(rig.area->viewport()->size());acceptanceView(*viewer);
  const QJsonObject proof{{"checkpoint","central-body-propagation"},{"case",key},{"body",body},{"type",spec["type"]},{"user",user},{"outcome","passed"},{"mu_km3_s2",gm},{"radius_km",radius},{"inclination_degrees",matrix["inclination_degrees"]},{"period_seconds",period},{"rows",rows},{"truth",truth},{"dynamics_elapsed_seconds",dynamicsTimes},{"propagation_timing",timing},{"native_final_state",nativeValues},{"bindings",bindings},{"typed_body_entries",typedEntries},{"maximum_position_error_km",maxPosition},{"maximum_velocity_error_km_s",maxVelocity},{"scope",matrix["scope"]}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
