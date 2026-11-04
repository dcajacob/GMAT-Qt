#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "ODEModel.hpp"
#include "PhysicalModel.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static void events(){QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static QByteArray read(const QString &p){QFile f(p);require(f.open(QIODevice::ReadOnly),"SRP artifact missing");return f.readAll();}
static void write(const QString &p,const QByteArray &b){QFile f(p);require(f.open(QIODevice::WriteOnly)&&f.write(b)==b.size(),"SRP artifact write failed");}
using V=std::array<double,3>;
static double norm(V a){return std::sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);}
static QJsonArray json(V a){return {a[0],a[1],a[2]};}
static V sunRelative(double epoch){auto *s=Moderator::Instance()->GetSolarSystemInUse();const auto sun=s->GetBody("Sun")->GetMJ2000State(A1Mjd(epoch)),earth=s->GetBody("Earth")->GetMJ2000State(A1Mjd(epoch));return {sun[0]-earth[0],sun[1]-earth[1],sun[2]-earth[2]};}
static void reveal(QWidget *w){require(w,"SRP input missing");for(auto *p=w;p;p=p->parentWidget()){for(auto *t:w->window()->findChildren<QTabWidget *>())if(t->indexOf(p)>=0)t->setCurrentWidget(p);if(auto *s=qobject_cast<QScrollArea *>(p))s->ensureWidgetVisible(w);}events();require(w->isVisible()&&w->isEnabled(),"SRP input not reachable");}
struct Trial{const char *name;double dry,fuel,decoy;bool enabled;};
int main(int argc,char **argv){
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaBallisticSrp");
 try{
  require(argc==2&&acceptanceWidgetPlatform(),"Supply startup and widget platform");TestSettings settings;QTemporaryDir files;require(files.isValid(),"SRP fixture unavailable");const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &p,const QByteArray &b){if(!evidence.isEmpty())write(QDir(evidence).filePath(p),b);};
  MainWindow window;require(window.initialize(startup),"SRP initialization failed");window.resize(window.screen()->availableGeometry().size()-QSize(60,60));window.show();events();acceptanceView(window);
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *area=window.findChild<QMdiArea *>("workspace");auto *tree=window.findChild<QTreeWidget *>("Resources");require(source&&messages&&area&&tree,"SRP workspace missing");
  const auto solar=sunRelative(21545);const double distance=norm(solar),mu=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant(),speed=std::sqrt(mu/7000.);
  V position{},velocity{};for(int i=0;i<3;++i)position[i]=7000*solar[i]/distance;V tangent{-position[1],position[0],0};const auto tangentLength=norm(tangent);for(int i=0;i<3;++i)velocity[i]=speed*tangent[i]/tangentLength;
  const QList<Trial> trials={{"Off",100,50,25,false},{"Light",100,50,25,true},{"FuelHeavy",100,150,25,true},{"DryHeavy",200,50,25,true},{"SecondTankHeavy",100,50,125,true}};
  QJsonArray results,edits;QList<QJsonArray> finalRows;int applies=0;
  for(const auto &trial:trials){
   const QString name=trial.name;const auto path=files.filePath(name+" SRP Δ.csv"),saved=files.filePath(name+" SRP Ω.script");
   QString initial="% Total-mass spherical SRP witness Δ\nCreate Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.DryMass = 99;\nSat.Cr = 1;\nSat.SRPArea = 999;\n";
   const QStringList axes={"X","Y","Z","VX","VY","VZ"};for(int i=0;i<6;++i)initial+="Sat."+axes[i]+" = "+QString::number(i<3?position[i]:velocity[i-3],'g',17)+";\n";
   initial+="Create ChemicalTank Fuel Decoy Unattached;\nFuel.FuelMass = 49;\nDecoy.FuelMass = 24;\nUnattached.Volume = 100;\nUnattached.FuelMass = 9999;\nSat.Tanks = {Fuel, Decoy};\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = "+QString(trial.enabled?"On":"Off")+";\n";
   if(trial.enabled)initial+="FM.SRP.SRPModel = Spherical;\nFM.SRP.Flux = 1367;\nFM.SRP.Nominal_Sun = 149597870.691;\n";
   initial+="Create Propagator P;\nP.FM = FM;\nP.InitialStepSize = 1;\nP.MinStep = 1;\nP.MaxStep = 1;\nP.Accuracy = 1e-12;\nCreate Variable Keep;\nKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+path+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\n";
   // Resource commits group owned assignments immediately before the mission.
   // Start there so the exact-source oracle changes values without relocating inputs.
   QString owned;for(const auto &line:QStringList{"Sat.DryMass = 99;","Sat.Cr = 1;","Sat.SRPArea = 999;","Fuel.FuelMass = 49;","Decoy.FuelMass = 24;"}){require(initial.count(line+"\n")==1,"SRP initial owned assignment ambiguous");initial.remove(line+"\n");owned+=line+"\n";}initial.replace("BeginMissionSequence;\n",owned+"BeginMissionSequence;\n");
   const QString report="Report Values Sat.ElapsedSecs Sat.TotalMass Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Keep;\n";initial+=report+"Propagate P(Sat) {Sat.ElapsedSecs = 10};\n"+report+"% Preserve final source Ω\n";
   source->setPlainText(initial);save(name+"-initial.script",initial.toUtf8());if(!window.buildScript()){save(name+"-build-messages.txt",messages->toPlainText().toUtf8());std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("SRP initial build failed");}source->document()->setModified(false);
   QPointer<QMdiSubWindow> child;
   auto open=[&](const QString &resource){for(auto *tabs:window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->tabText(i)=="Resources")tabs->setCurrentIndex(i);auto items=tree->findItems(resource,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"SRP resource entry missing");tree->itemDoubleClicked(items.first(),0);events();child=area->activeSubWindow();auto *p=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(p&&!p->hasChanges(),"SRP real resource editor missing");child->showNormal();child->setGeometry(area->viewport()->rect().adjusted(4,4,-4,-4));area->setActiveSubWindow(child);child->raise();events();acceptanceView(*child);return p;};
   auto apply=[&](ResourceEditor *panel,const QString &before){require(panel->hasChanges()&&source->toPlainText()==before,"SRP pending edit changed source before Apply");auto *buttons=panel->findChild<QDialogButtonBox *>();require(buttons&&buttons->button(QDialogButtonBox::Apply),"SRP Apply button missing");reveal(buttons->button(QDialogButtonBox::Apply));QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();panel=dynamic_cast<ResourceEditor *>(child->widget());if(panel&&panel->hasChanges())std::cerr<<panel->findChild<QLabel *>("resourceStatus")->text().toStdString();require(panel&&!panel->hasChanges(),"SRP GUI Apply failed");const auto after=source->toPlainText();require(after!=before,"SRP Apply did not change source");require(child->close(),"SRP clean owner did not close");events();source->undo();require(source->toPlainText()==before&&window.buildScript(),"SRP Undo not exact");source->redo();require(source->toPlainText()==after&&window.buildScript(),"SRP Redo not exact");++applies;};
   auto *panel=open("Sat");auto before=source->toPlainText();auto *configured=Moderator::Instance()->GetConfiguredObject("Sat");require(configured&&configured->GetRealParameter("DryMass")==99,"SRP starting dry mass wrong");
   for(const auto &pair:QList<QPair<QString,double>>{{"DryMass",trial.dry},{"Cr",2},{"SRPArea",1000}}){auto *field=panel->findChild<QLineEdit *>("ballistics_"+pair.first);reveal(field);field->setText(QString::number(pair.second,'g',17));acceptanceView(*child);require(source->toPlainText()==before&&configured->GetRealParameter("DryMass")==99,"SRP inline draft changed configured mass/source");}
   apply(panel,before);
   for(const auto &pair:QList<QPair<QString,double>>{{"Fuel",trial.fuel},{"Decoy",trial.decoy}}){panel=open(pair.first);before=source->toPlainText();auto *field=panel->findChild<QLineEdit *>("resource_FuelMass");reveal(field);field->setText(QString::number(pair.second,'g',17));acceptanceView(*child);apply(panel,before);}
   const auto wanted=source->toPlainText();auto expected=initial;for(const auto &pair:QList<QPair<QString,QString>>{{"Sat.DryMass = 99;","Sat.DryMass = "+QString::number(trial.dry,'g',17)+";"},{"Sat.Cr = 1;","Sat.Cr = 2;"},{"Sat.SRPArea = 999;","Sat.SRPArea = 1000;"},{"Fuel.FuelMass = 49;","Fuel.FuelMass = "+QString::number(trial.fuel,'g',17)+";"},{"Decoy.FuelMass = 24;","Decoy.FuelMass = "+QString::number(trial.decoy,'g',17)+";"}}){require(expected.count(pair.first)==1,"SRP exact replacement ambiguous");expected.replace(pair.first,pair.second);}save(name+"-source-expected.script",expected.toUtf8());save(name+"-source-actual.script",wanted.toUtf8());require(wanted==expected,"SRP GUI changed unrelated source or wrong mass property");
   require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==wanted,"SRP Unicode save/reopen failed");save(name+"-mission.script",wanted.toUtf8());panel=open("Sat");require(Moderator::Instance()->GetConfiguredObject("Sat")->GetRealParameter("DryMass")==trial.dry,"SRP dry mass readback differs");require(child->close(),"SRP reopened clean owner close failed");events();
   const int undo=source->document()->availableUndoSteps();messages->clear();const auto run=window.runMission();events();save(name+"-messages.txt",messages->toPlainText().toUtf8());if(run!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(run==MainWindow::RunResult::Completed,"SRP mission failed");const auto raw=read(path);save(name+"-values.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==2,"SRP two report rows missing");QJsonArray rows;
   for(int row=0;row<2;++row){const auto columns=lines[row].split(',');require(columns.size()==9,"SRP report columns differ");QJsonArray values;for(const auto &column:columns){bool ok=false;const double v=column.trimmed().toDouble(&ok);require(ok&&std::isfinite(v),"SRP raw value is nonfinite");values.append(v);}require(std::abs(values[0].toDouble()-10*row)<1e-4&&values[1].toDouble()==trial.dry+trial.fuel+trial.decoy&&values[8].toDouble()==41,"SRP total mass, elapsed time or sentinel differs");rows.append(values);}
   auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));auto *force=dynamic_cast<ODEModel *>(Moderator::Instance()->GetInternalObject("FM"));require(sat&&force,"SRP internal spacecraft/ODE model missing");require(sat->GetRealParameter("TotalMass")==trial.dry+trial.fuel+trial.decoy,"SRP native total mass omitted a tank or included unattached fuel");const auto last=rows.last().toArray();const auto finalSun=sunRelative(sat->GetEpoch());V sunToSat{};for(int i=0;i<3;++i)sunToSat[i]=last[2+i].toDouble()-finalSun[i];const double r=norm(sunToSat),mass=trial.dry+trial.fuel+trial.decoy;
   V expectedAcceleration{},actualAcceleration{};const double factor=(1367./299792458.)*149597870.691*149597870.691*2*1000/(mass*1000*r*r*r);
   for(int i=0;i<3;++i)expectedAcceleration[i]=trial.enabled?factor*sunToSat[i]:0.;
   if(trial.enabled){auto *srp=force->GetForceOfType("SolarRadiationPressure");require(srp&&srp->GetStringParameter("SRPModel")=="Spherical","SRP spherical force not active");const auto derivatives=srp->GetDerivativesForSpacecraft(sat);for(int i=0;i<3;++i){actualAcceleration[i]=derivatives[i+3];require(std::abs(actualAcceleration[i]-expectedAcceleration[i])<norm(expectedAcceleration)*1e-8+1e-18,"SRP native acceleration disagrees with independent total-mass spherical equation");}}
   else require(!force->GetForceOfType("SolarRadiationPressure"),"SRP Off control still has radiation force");
   require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==undo&&!window.isRunning()&&window.findChild<QAction *>("runMission")->isEnabled(),"SRP mission changed source/history or retained run lock");
   results.append(QJsonObject{{"trial",name},{"dry_mass",trial.dry},{"fuel_mass",trial.fuel},{"second_tank_mass",trial.decoy},{"total_mass",mass},{"srp_enabled",trial.enabled},{"rows",rows},{"sun_relative_earth_final",json(finalSun)},{"expected_acceleration",json(expectedAcceleration)},{"native_acceleration",json(actualAcceleration)}});finalRows.append(last);save("results.json",QJsonDocument(results).toJson());
  }
  const auto control=finalRows.first();const double t=control[0].toDouble(),angle=std::sqrt(mu/std::pow(7000.,3))*t;for(int i=0;i<3;++i){const double p=position[i]*std::cos(angle)+velocity[i]/std::sqrt(mu/std::pow(7000.,3))*std::sin(angle),v=-position[i]*std::sqrt(mu/std::pow(7000.,3))*std::sin(angle)+velocity[i]*std::cos(angle);require(std::abs(control[2+i].toDouble()-p)<1e-5&&std::abs(control[5+i].toDouble()-v)<1e-8,"SRP Off control differs from independent circular orbit");}
  QJsonArray impulses;
  for(int index=1;index<trials.size();++index){const auto &trial=trials[index];const auto row=finalRows[index];require(row[0]==control[0],"SRP differential runs ended at different epochs");V fromSun{},accel{},dv{},dr{};for(int i=0;i<3;++i)fromSun[i]=position[i]-solar[i];const double r=norm(fromSun),mass=trial.dry+trial.fuel+trial.decoy,factor=(1367./299792458.)*149597870.691*149597870.691*2*1000/(mass*1000*r*r*r);for(int i=0;i<3;++i){accel[i]=factor*fromSun[i];dv[i]=row[5+i].toDouble()-control[5+i].toDouble();dr[i]=row[2+i].toDouble()-control[2+i].toDouble();require(std::abs(dv[i]-accel[i]*t)<norm(accel)*t*2e-4+2e-12&&std::abs(dr[i]-.5*accel[i]*t*t)<norm(accel)*t*t*2e-3+2e-9,"SRP integrated deflection disagrees with independent spherical force/mass impulse");}impulses.append(QJsonObject{{"trial",trial.name},{"delta_velocity",json(dv)},{"delta_position",json(dr)},{"initial_acceleration",json(accel)}});}
  for(int index=3;index<5;++index)for(int j=2;j<8;++j)require(std::abs(finalRows[index][j].toDouble()-finalRows[2][j].toDouble())<1e-12,"Equal total mass with different dry/fuel/tank allocation changed ballistic propagation");require(applies==15,"SRP actual resource Apply count differs");
  const QJsonObject proof{{"checkpoint","ballistic-srp-total-mass"},{"outcome","passed"},{"trials",results},{"initial_position",json(position)},{"initial_velocity",json(velocity)},{"sun_relative_earth_initial",json(solar)},{"earth_mu",mu},{"impulses",impulses},{"resource_applies",applies},{"trial_count",trials.size()},{"facets",QJsonArray{"actual-ballistics-and-two-tank-resource-edits","pending-exact-apply-undo-redo-unicode-readback","all-attached-and-no-unattached-total-mass","independent-spherical-srp-native-force-equation","independent-integrated-srp-impulse-and-off-circular-control","equal-total-mass-distinct-dry-fuel-allocation-invariance"}},{"scope","Five short Earth point-mass/spherical SRP missions in full sunlight, fixed Cr2/area1000 and flux1367: Off control, lighter total175 and three total275 allocations across dry mass/two attached tanks; unattached9999 excluded. Shared Sun/Earth ephemeris states are explicit formula inputs, not independent ephemeris accuracy. Instantaneous physical-force helper and actual integrated state differences are checked. No drag, shadow/attitude/SPAD/NPlate/force/frame/epoch/long-duration/extreme-input generalization, native OS input, host GPU/platform/stress/current private NASA or full-current-build suite qualification."}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
