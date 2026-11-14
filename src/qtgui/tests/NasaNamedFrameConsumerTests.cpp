#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include <QAbstractItemView>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStatusBar>
#include <QTabWidget>
#include <QTest>
#include <algorithm>
using namespace formaccept;
static void reveal(QWidget *w) {
 require(w,"Frame consumer widget missing");for(auto *p=w;p;p=p->parentWidget()) {
  for(auto *tabs:w->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
  if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(w);
 }events();require(w->isVisible()&&w->isEnabled(),"Frame consumer widget unreachable");
}
static QJsonObject graph() {
 QJsonObject result;auto *m=Moderator::Instance();for(const auto &name:m->GetListOfObjects(Gmat::UNKNOWN_OBJECT))
  if(auto *o=m->GetConfiguredObject(name))result[QString::fromStdString(name)]=QString::fromStdString(o->GetGeneratingString(Gmat::SCRIPTING));return result;
}
int main(int argc,char **argv) {
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaNamedFrameConsumers");
 try {
  require(argc==5,"Expected startup, independent frame table, frame key and consumer");
  const auto fixturePath=QFileInfo(argv[2]).absoluteFilePath();const auto matrix=QJsonDocument::fromJson(read(fixturePath)).object();const QString key=argv[3],consumer=argv[4];require(consumer=="Thruster"||consumer=="Impulse","Unknown frame consumer");const bool thrust=consumer=="Thruster";QJsonObject spec;
  for(const auto &c:matrix["cases"].toArray())if(c.toObject()["key"].toString()==key)spec=c.toObject();require(!spec.isEmpty(),"Unknown named frame case");
  Rig rig(QFileInfo(argv[1]).absoluteFilePath());const QString actor=thrust?"TestEngine":"TestKick",control=thrust?"thruster_CoordinateSystem":"burn_CoordinateSystem";
  QString setup=spec["setup"].toString();const QString ceres=QFileInfo(argv[1]).absolutePath()+"/../data/planetary_ephem/spk/ceres_1900_2100.bsp",station=QFileInfo(fixturePath).absolutePath()+"/frame-consumer-station-origin.bsp";
  require(QFileInfo(ceres).isFile()&&QFileInfo(station).isFile(),"Frame consumer physical input missing");setup.replace("@CERES@",ceres);setup.replace("@STATION@",station);
  int identity=0;auto declarations=QRegularExpression("Create\\s+Spacecraft\\s+([^;]+);").globalMatch(setup);QStringList identityAssignments;
  while(declarations.hasNext())for(const auto &name:declarations.next().captured(1).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) {++identity;identityAssignments<<name+".NAIFId = "+QString::number(-100600-identity)+";\n"+name+".NAIFIdReferenceFrame = "+QString::number(-900600-identity)+";\n";}
  require(identity>0,"Frame fixture lacks spacecraft");setup+=identityAssignments.join("");
  setup+="Sat.Tanks = {TestFuel};\n"; // declaration precedes assignment below
  setup.replace("Sat.Tanks = {TestFuel};\n","Create ChemicalTank TestFuel;\nTestFuel.Volume = 2;\nTestFuel.FuelMass = 1000;\nSat.Tanks = {TestFuel};\nSat.DryMass = 400;\n");
  if(thrust)setup+="Create ChemicalThruster TestEngine;\nTestEngine.CoordinateSystem = EarthMJ2000Eq;\nTestEngine.Tank = {TestFuel};\nTestEngine.DecrementMass = true;\nTestEngine.C1 = 10;\nTestEngine.K1 = 300;\nTestEngine.DutyCycle = 0.4;\nTestEngine.ThrustScaleFactor = 1.5;\nTestEngine.GravitationalAccel = 8;\nSat.Thrusters = {TestEngine};\nCreate FiniteBurn TestBurn;\nTestBurn.Thrusters = {TestEngine};\n";
  else setup+="Create ImpulsiveBurn TestKick;\nTestKick.CoordinateSystem = EarthMJ2000Eq;\nTestKick.DecrementMass = false;\nTestKick.Tank = {TestFuel};\n";
  setup+="Create Variable ConsumerSample ConsumerPhase ConsumerKeep;\nConsumerKeep = 41;\nCreate ReportFile ConsumerValues;\nConsumerValues.Filename = '"+rig.report+"';\nConsumerValues.WriteHeaders = false;\nConsumerValues.FixedWidth = false;\nConsumerValues.Delimiter = ',';\nConsumerValues.Precision = 25;\n";
  QStringList columns{"ConsumerSample","ConsumerPhase","Sat.A1ModJulian","ConsumerKeep","Sat.TestFuel.FuelMass","Sat.TotalMass"};const QStringList fields{"X","Y","Z","VX","VY","VZ"};for(const auto &field:fields)columns<<"Sat.EarthMJ2000Eq."+field;
  if(thrust){columns<<"Sat.TestEngine.ThrustMagnitude"<<"Sat.TestEngine.Isp"<<"Sat.TestEngine.MassFlowRate";for(int i=1;i<=3;++i)columns<<"TestBurn.TotalThrust"+QString::number(i);for(int i=1;i<=3;++i)columns<<"TestBurn.TotalAcceleration"+QString::number(i);columns<<"TestBurn.TotalMassFlowRate";}
  const QString report="Report ConsumerValues "+columns.join(' ')+";\n";const std::array<double,6> state{7000,-1200,900,1.5,6.75,-2};QString mission="BeginMissionSequence;\n";int sample=0;
  for(const auto &epochValue:spec["samples"].toArray())for(const auto &directionValue:matrix["directions"].toArray()) {
   const auto direction=directionValue.toArray();mission+="Sat.A1ModJulian = "+number(epochValue.toObject()["epoch"].toDouble())+";\n";
   for(int i=0;i<6;++i)mission+="Sat.EarthMJ2000Eq."+fields[i]+" = "+number(state[i])+";\n";
   for(int i=0;i<3;++i)mission+=(thrust?"Sat.TestEngine.ThrustDirection":"TestKick.Element")+QString::number(i+1)+" = "+number(direction[i].toDouble()*(thrust?1:matrix["impulse_scale_km_s"].toDouble()))+";\n";
   mission+="ConsumerSample = "+QString::number(sample++)+";\nConsumerPhase = 0;\n"+report;
   mission+=(thrust?"BeginFiniteBurn TestBurn(Sat);\n":"Maneuver TestKick(Sat);\n");mission+="ConsumerPhase = 1;\n"+report;
   mission+=(thrust?"EndFiniteBurn TestBurn(Sat);\n":"Maneuver BackProp TestKick(Sat);\n");mission+="ConsumerPhase = 2;\n"+report;
  }
  mission+="ConsumerKeep = ConsumerKeep + 1;\nReport ConsumerValues ConsumerKeep;\n% Preserve frame consumer tail Ω\n";const QString source=setup+mission;
  rig.source->setPlainText(source);rig.save("baseline.script",source.toUtf8());rig.save("case.json",QJsonDocument(spec).toJson());const bool built=rig.window.buildScript();events();rig.save("build.messages.txt",rig.messages->toPlainText().toUtf8());rig.save("build.status.txt",rig.window.statusBar()->currentMessage().toUtf8());if(!built)std::cerr<<rig.window.statusBar()->currentMessage().toStdString()<<'\n'<<rig.messages->toPlainText().toStdString();require(built,"Named frame consumer baseline Build failed");
  const auto baseline=graph();const int historyBefore=rig.source->document()->availableUndoSteps();auto stable=[&]{require(rig.source->toPlainText()==source&&graph()==baseline&&rig.source->document()->availableUndoSteps()==historyBefore,"Named-frame draft escaped source/graph/history isolation");};QPointer<QMdiSubWindow> owner;QPointer<ResourceEditor> panel;int selections=0;
  auto open=[&]{auto items=rig.resources->findItems(actor,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Frame consumer resource ambiguous");rig.resources->expandAll();rig.resources->scrollToItem(items[0]);rig.resources->itemDoubleClicked(items[0],0);events();owner=rig.area->activeSubWindow();panel=owner?dynamic_cast<ResourceEditor *>(owner->widget()):nullptr;require(panel&&!panel->hasChanges(),"Named-frame clean resource editor missing");owner->showNormal();owner->move(0,0);owner->resize(rig.area->viewport()->size());acceptanceView(*owner);};
  auto choose=[&]{auto *combo=panel->findChild<QComboBox *>(control);require(combo&&combo->currentText()=="EarthMJ2000Eq"&&combo->findText("CreatedFrame")>=0,"Named-frame actual choice unavailable");reveal(combo);combo->showPopup();events();require(combo->view()->isVisible(),"Named-frame popup hidden");combo->setCurrentText("CreatedFrame");QTest::keyClick(combo->view(),Qt::Key_Return);events();require(combo->currentText()=="CreatedFrame"&&!combo->view()->isVisible()&&panel->hasChanges(),"Named-frame popup selection did not stay pending");for(const auto &suffix:QStringList{"Axes","Origin"}){auto *local=panel->findChild<QComboBox *>((thrust?"thruster_":"burn_")+suffix);require(local&&!local->isEnabled(),"Named frame left ineffective local controls active");}++selections;acceptanceView(*owner);stable();};
  open();choose();panel->discardChanges();require(owner->close(),"Named-frame Cancel failed");events();stable();open();choose();auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");auto *apply=buttons?buttons->button(QDialogButtonBox::Apply):nullptr;reveal(apply);QTest::mouseClick(apply,Qt::LeftButton);events();panel=dynamic_cast<ResourceEditor *>(owner->widget());rig.save("apply.messages.txt",rig.messages->toPlainText().toUtf8());require(panel&&!panel->hasChanges(),"Named-frame Apply failed");
  const auto acceptedSource=rig.source->toPlainText();const auto accepted=graph();rig.save("accepted.script",acceptedSource.toUtf8());auto stripActor=[&](QString s){s.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?"+actor+"\\.[^\\n]*\\n",QRegularExpression::MultilineOption));return s;};require(source!=acceptedSource&&stripActor(source)==stripActor(acceptedSource)&&acceptedSource.endsWith(mission),"Named-frame Apply changed unrelated source");
  require(owner->close(),"Named-frame applied editor Close failed");events();rig.source->undo();require(rig.source->toPlainText()==source&&rig.window.buildScript()&&graph()==baseline,"Named-frame exact Undo/native graph failed");rig.source->redo();require(rig.source->toPlainText()==acceptedSource&&rig.window.buildScript()&&graph()==accepted,"Named-frame exact Redo/native graph failed");
  const auto saved=rig.files.filePath(key+" "+consumer+" Ω.script");require(rig.window.saveScriptTo(saved)&&read(saved)==acceptedSource.toUtf8()&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==acceptedSource&&graph()==accepted,"Named-frame Unicode save/reopen differs");const int history=rig.source->document()->availableUndoSteps();const auto beforeRun=graph();const auto status=rig.window.runMission();events();rig.save("run.messages.txt",rig.messages->toPlainText().toUtf8());require(status==MainWindow::RunResult::Completed,"Named-frame mission failed");
  const auto raw=read(rig.report);rig.save("results.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==28&&lines.last().trimmed()=="42","Named-frame row count/following sentinel differs");QJsonArray actual,expected;double maximumForce=0,maximumVelocity=0;
  for(int row=0;row<27;++row) {
   const int group=row/3,phase=row%3,epochIndex=group/3,directionIndex=group%3;const auto epochSpec=spec["samples"].toArray()[epochIndex].toObject();const auto rows=epochSpec["rows"].toArray(),direction=matrix["directions"].toArray()[directionIndex].toArray();double norm=0,l1=0;for(int i=0;i<3;++i){norm+=direction[i].toDouble()*direction[i].toDouble();l1+=std::abs(direction[i].toDouble());}norm=std::sqrt(norm);std::array<double,3> rotated{};for(int i=0;i<3;++i)for(int j=0;j<3;++j)rotated[i]+=rows[j].toArray()[i].toDouble()*direction[j].toDouble();
   QJsonArray truth{group,phase,epochSpec["epoch"],41,1000,1400};for(int i=0;i<6;++i)truth.append(state[i]+(!thrust&&phase==1&&i>=3?rotated[i-3]*matrix["impulse_scale_km_s"].toDouble():0));
   if(thrust){const bool firing=phase==1;truth.append(firing?6:0);truth.append(firing?300:0);truth.append(firing?-.4*10/(8*300):0);for(const auto v:rotated)truth.append(firing?6*v/norm:0);for(const auto v:rotated)truth.append(firing?6*v/(norm*1400*1000):0);truth.append(firing?-.4*10/(8*300):0);}
   const auto columns=lines[row].split(',');require(columns.size()==truth.size(),"Named-frame raw column count differs");QJsonArray values;
   for(int c=0;c<columns.size();++c){bool ok=false;const double value=columns[c].toDouble(&ok);require(ok&&std::isfinite(value),"Named-frame raw result invalid");values.append(value);double tolerance=1e-9;if(c==2)tolerance=1e-10;if(thrust&&c>=15&&c<=17)tolerance=6*l1/norm*spec["matrix_bound"].toDouble()+1e-9;if(thrust&&c>=18&&c<=20)tolerance=(6*l1/norm*spec["matrix_bound"].toDouble())/(1400*1000)+1e-14;if(thrust&&(c==14||c==21))tolerance=1e-12;if(!thrust&&c>=9&&c<=11)tolerance=l1*matrix["impulse_scale_km_s"].toDouble()*spec["matrix_bound"].toDouble()+1e-9;const double error=std::abs(value-truth[c].toDouble());if(thrust&&c>=15&&c<=17)maximumForce=std::max(maximumForce,error);if(!thrust&&c>=9&&c<=11)maximumVelocity=std::max(maximumVelocity,error);if(error>tolerance)std::cerr<<"FRAME_CONSUMER_DIFF "<<key.toStdString()<<" "<<consumer.toStdString()<<" row="<<row<<" col="<<c<<" actual="<<value<<" truth="<<truth[c].toDouble()<<" tolerance="<<tolerance<<'\n';require(error<=tolerance,"Named-frame result differs from independent rotation/normalization/free-vector oracle");}actual.append(values);expected.append(truth);
  }
  auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));auto *native=thrust?(sat?sat->GetRefObject(Gmat::THRUSTER,"TestEngine"):nullptr):Moderator::Instance()->GetInternalObject("TestKick");require(native&&native->GetStringParameter("CoordinateSystem")=="CreatedFrame","Actual native consumer uses another frame");if(thrust)require(native->GetRealParameter("DutyCycle")==.4&&native->GetRealParameter("ThrustScaleFactor")==1.5&&native->GetRealParameter("C1")==10&&native->GetRealParameter("K1")==300,"Actual native thruster coefficients differ");
  require(rig.source->toPlainText()==acceptedSource&&rig.source->document()->availableUndoSteps()==history&&read(saved)==acceptedSource.toUtf8()&&graph()==beforeRun,"Named-frame Run changed source/history/configured graph/file");
  auto *outputs=rig.window.findChild<QTreeWidget *>("Output");auto items=outputs->findItems("ConsumerValues",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Named-frame output tree entry missing");outputs->itemDoubleClicked(items[0],0);events();auto *viewer=rig.area->activeSubWindow();auto *text=viewer?viewer->findChild<QPlainTextEdit *>():nullptr;require(viewer&&dynamic_cast<ReportViewer *>(viewer->widget())&&text&&text->isReadOnly()&&text->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Named-frame visible viewer/raw bytes differ");viewer->showNormal();viewer->resize(rig.area->viewport()->size());acceptanceView(*viewer);
  const QJsonObject proof{{"checkpoint","named-frame-consumer"},{"outcome","passed"},{"case",key},{"consumer",consumer},{"axis",spec["axis"]},{"origin_class",spec["origin_class"]},{"selections",selections},{"actual",actual},{"expected",expected},{"maximum_force_error_newtons",maximumForce},{"maximum_velocity_error_km_s",maximumVelocity},{"facets",QJsonArray{"actual-named-frame-popup-and-local-dependencies","pending-Cancel-Apply-exact-graph-Undo-Redo","Unicode-save-reopen-and-read-only-viewer","independent-rotation-three-spanning-directions-three-epochs","actual-native-consumer-frame-and-force-configuration","free-vector-normalization-and-zero-translation","following-sentinel-and-source-native-isolation"}},{"scope",matrix["scope"]}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
