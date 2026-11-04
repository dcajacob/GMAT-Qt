#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "CommandEditor.hpp"
#include "ReportViewer.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QComboBox>
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
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QTabBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *m) {if (!ok) throw std::runtime_error(m);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static void write(const QString &p,const QByteArray &b) {QFile f(p);require(f.open(QIODevice::WriteOnly) && f.write(b)==b.size(),"Integrated thruster evidence write failed");}
static QByteArray read(const QString &p) {QFile f(p);require(f.open(QIODevice::ReadOnly),"Integrated thruster report missing");return f.readAll();}
static void reveal(QWidget *c) {require(c,"Integrated thruster control missing");for (QWidget *p=c;p;p=p->parentWidget()) {for (auto *t:c->window()->findChildren<QTabWidget *>()) if (t->indexOf(p)>=0) t->setCurrentWidget(p);if (auto *s=qobject_cast<QScrollArea *>(p)) s->ensureWidgetVisible(c);}events();require(c->isVisible() && c->isEnabled(),"Integrated thruster control unreachable");}
using V=std::array<double,3>;
static const V initialPosition{7000,-1200,900},initialVelocity{.25,-.1,.05},direction{2./7,-3./7,6./7};
struct Model {bool regulated=false,massOff=false;double k2=0;std::array<double,4> duties{.4,.4,.4,.4},scales{1.5,1.5,1.5,1.5};};
struct State {double t=0,mass=1000,dv=0,displacement=0;};
static Model model(const QString &mode) {Model m;require(QStringList{"Regulated","BlowDown","VariableIsp","MassOff","DutySwitch","ScaleSwitch"}.contains(mode),"Unknown integrated thruster case");m.regulated=mode=="Regulated";m.massOff=mode=="MassOff";m.k2=mode=="Regulated" || mode=="BlowDown" ? 0:.05;if (mode=="DutySwitch") m.duties={.4,0,.8,.8};if (mode=="ScaleSwitch") m.scales={1.5,0,2.25,2.25};return m;}
// NASA's pressure/force/mass-flow equations separate analytically for
// F=.1P, Isp=300+k2*P, P*gasVolume=1800, density=1000.
static double massAt(const Model &m,double initial,double dt,double duty) {
   if (m.massOff || duty==0 || dt==0) return initial;
   if (m.regulated) return initial-duty*.1*1800/(10*(300+m.k2*1800))*dt;
   const double gas=2-initial/1000,S=300*gas+m.k2*1800,C=duty*.1*1800/(10*1000);
   const double delta=2*C*dt/(std::sqrt(S*S+2*300*C*dt)+S);
   return 1000*(2-gas-delta);
}
static double deltaVelocity(const Model &m,double initial,double dt,double duty,double scale) {
   if (duty==0 || scale==0 || dt==0) return 0;
   if (m.massOff) return duty*scale*.1*1800/(1000*(937+initial))*dt;
   const double final=massAt(m,initial,dt,duty),M0=937+initial,M=937+final,logMass=std::log1p((M0-M)/M);
   if (m.regulated) return scale*10*(300+m.k2*1800)/1000*logMass;
   const double B=2937;
   return scale*10/1000*(300*logMass+m.k2*1800*1000/B*(logMass+std::log1p((M0-M)/(B-M0))));
}
static void advance(const Model &m,State &s,double dt,bool firing,double duty,double scale) {
   double extra=0;
   if (firing) {constexpr int n=256;double integral=0;for (int i=0;i<=n;++i) integral+=(i==0 || i==n ? 1:i%2 ? 4:2)*deltaVelocity(m,s.mass,dt*i/n,duty,scale);extra=integral*dt/(3*n);}
   s.displacement+=s.dv*dt+extra;
   if (firing) {s.dv+=deltaVelocity(m,s.mass,dt,duty,scale);s.mass=massAt(m,s.mass,dt,duty);}s.t+=dt;
}
static QJsonArray values(const Model &m,const State &s,int index,bool firing,double duty,double scale) {
   const double pressure=m.regulated ? 1800:1800/(2-s.mass/1000),isp=firing ? 300+m.k2*pressure:0,force=firing ? duty*scale*.1*pressure:0,flow=firing && !m.massOff ? -duty*.1*pressure/(10*isp):0;
   QJsonArray a{index,s.t,s.mass,pressure,20,20,137,937+s.mass};for (int i=0;i<3;++i) a.append(initialPosition[i]+initialVelocity[i]*s.t+direction[i]*s.displacement);for (int i=0;i<3;++i) a.append(initialVelocity[i]+direction[i]*s.dv);a.append(duty);a.append(scale);a.append(force);a.append(isp);a.append(flow);for (auto x:direction) a.append(x*force);for (auto x:direction) a.append(x*force/(1000*(937+s.mass)));a.append(flow);for (double x:{0.,0.,0.,41.}) a.append(x);require(a.size()==30,"Integrated thruster oracle column count differs");return a;
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaThrusterIntegration");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and integrated thruster case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Integrated thruster fixture missing");const QString mode=argv[2];const auto m=model(mode);const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto report=files.filePath("Integrated thrust Δ.txt"),saved=files.filePath("Integrated thrust Δ.script");
      const QString columns="Index Sat.ElapsedSecs Sat.Fuel.FuelMass Sat.Fuel.Pressure Sat.Fuel.Temperature Sat.Fuel.RefTemperature Sat.Decoy.FuelMass Sat.TotalMass Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Sat.Engine.DutyCycle Sat.Engine.ThrustScaleFactor Sat.Engine.ThrustMagnitude Sat.Engine.Isp Sat.Engine.MassFlowRate Burn.TotalThrust1 Burn.TotalThrust2 Burn.TotalThrust3 Burn.TotalAcceleration1 Burn.TotalAcceleration2 Burn.TotalAcceleration3 Burn.TotalMassFlowRate Sat.Idle.ThrustMagnitude Sat.Idle.Isp Sat.Idle.MassFlowRate Keep";const QString output="Report Values "+columns+";\n";
      QString mission="BeginMissionSequence;\nIndex = 0;\n"+output+"BeginFiniteBurn 'Ignite' IdleBurn(Sat); % preserve ignition Δ\nIndex = 1;\n"+output;
      for (int i=0;i<3;++i) {mission+="Sat.Engine.DutyCycle = "+QString::number(m.duties[i],'g',17)+";\nSat.Engine.ThrustScaleFactor = "+QString::number(m.scales[i],'g',17)+";\nPropagate Prop(Sat) {Sat.ElapsedSecs = "+QString("600")+"};\nIndex = "+QString::number(i+2)+";\n"+output;}
      mission+="EndFiniteBurn 'Shutdown' IdleBurn(Sat); % preserve shutdown\nIndex = 5;\n"+output+"Propagate Prop(Sat) {Sat.ElapsedSecs = 600};\nIndex = 6;\n"+output+"BeginFiniteBurn 'Resume' Burn(Sat);\nPropagate Prop(Sat) {Sat.ElapsedSecs = 600};\nIndex = 7;\n"+output+"EndFiniteBurn 'Final shutdown' Burn(Sat);\nIndex = 8;\n"+output+"Propagate Prop(Sat) {Sat.ElapsedSecs = 600};\nIndex = 9;\n"+output+"Keep = Keep + 1; % preserve final command\n";
      QString initial="% Synthetic negligible-gravity propulsion acceptance Δ\nEarth.Mu = 1e-20;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\nSat.DryMass = 800;\n";const QStringList components{"X","Y","Z","VX","VY","VZ"};for (int i=0;i<6;++i) initial+="Sat."+components[i]+" = "+QString::number(i<3 ? initialPosition[i]:initialVelocity[i-3],'g',17)+";\n";
      initial+="Create ChemicalTank Fuel Decoy;\nFuel.FuelMass = 1000;\nFuel.Pressure = 2000;\nFuel.Volume = 2;\nFuel.FuelDensity = 1000;\nFuel.Temperature = 20;\nFuel.RefTemperature = 20;\nFuel.PressureModel = PressureRegulated;\nDecoy.FuelMass = 137;\nCreate ChemicalThruster Engine Idle;\nEngine.Tank = {Fuel};\nEngine.DecrementMass = false;\nEngine.CoordinateSystem = EarthMJ2000Eq;\nIdle.Tank = {Decoy};\nSat.Tanks = {Fuel, Decoy};\nSat.Thrusters = {Engine, Idle};\nCreate FiniteBurn Burn IdleBurn;\nBurn.Thrusters = {Engine};\nIdleBurn.Thrusters = {Idle};\nCreate ForceModel Forces;\nForces.PrimaryBodies = {};\nForces.PointMasses = {Earth};\nForces.Drag = None;\nForces.SRP = Off;\nCreate Propagator Prop;\nProp.Type = RungeKutta89;\nProp.FM = Forces;\nProp.InitialStepSize = 17;\nProp.MinStep = 0;\nProp.MaxStep = 60;\nProp.Accuracy = 1e-10;\nCreate Variable Index Keep;\nKeep = 41; % preserve initializer\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 17;\nValues.WriteHeaders = false;\n"+mission;
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &b) {if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),b);};save("initial.script",initial.toUtf8());MainWindow window;require(window.initialize(startup),"Integrated thruster initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *resources=window.findChild<QTreeWidget *>("Resources"),*tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(source && messages && resources && tree && area,"Integrated thruster workspace missing");source->setPlainText(initial);if (!window.buildScript()) {std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("Integrated thruster baseline failed");}QMdiSubWindow *child=nullptr;
      auto view=[&] {if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);child->showNormal();events();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);child->raise();events();}acceptanceView(*child);};
      auto open=[&](const QString &name) {auto matches=resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Integrated thruster resource missing");resources->itemDoubleClicked(matches.front(),0);events();child=area->activeSubWindow();auto *p=dynamic_cast<ResourceEditor *>(child->widget());require(p && !p->hasChanges(),"Actual integrated resource editor missing/dirty");view();return p;};
      auto set=[&](ResourceEditor *p,const QString &name,const QString &value) {if (auto *line=p->findChild<QLineEdit *>("resource_"+name)) {reveal(line);require(!line->isReadOnly(),"Integrated resource field readonly");line->setText(value);}else {auto *choice=p->findChild<QComboBox *>("resource_"+name);reveal(choice);require(choice->findText(value)>=0,"Integrated resource choice missing");choice->setCurrentText(value);}events();};
      int resourceApplies=0,commandApplies=0;
      auto apply=[&](ResourceEditor *p,const QString &before) {require(p->hasChanges() && source->toPlainText()==before,"Integrated resource draft escaped Apply");acceptanceView(*child);p->requestApply();events();p=dynamic_cast<ResourceEditor *>(child->widget());if (p && p->hasChanges()) std::cerr<<p->findChild<QLabel *>("resourceStatus")->text().toStdString();require(p && !p->hasChanges(),"Integrated resource Apply failed");const auto after=source->toPlainText();require(after!=before && child->close(),"Integrated resource source unchanged or close failed");events();source->undo();require(source->toPlainText()==before && window.buildScript(),"Integrated resource exact Undo failed");source->redo();require(source->toPlainText()==after && window.buildScript(),"Integrated resource exact Redo failed");++resourceApplies;};
      auto *panel=open("Fuel");auto before=source->toPlainText();set(panel,"Pressure","1800");set(panel,"PressureModel",m.regulated ? "PressureRegulated":"BlowDown");apply(panel,before);
      panel=open("Engine");before=source->toPlainText();const auto *registered=Moderator::Instance()->GetConfiguredObject("Engine");for (const auto &pair:QList<QPair<QString,QString>>{{"DutyCycle","0.4"},{"ThrustScaleFactor","1.5"},{"GravitationalAccel","10"},{"ThrustDirection1","2"},{"ThrustDirection2","-3"},{"ThrustDirection3","6"}}) {auto *e=panel->findChild<QLineEdit *>("thruster_"+pair.first);reveal(e);e->setText(pair.second);}auto *sections=panel->findChild<QTabBar *>("propertySections");require(sections,"Integrated engine sections missing");for (int i=0;i<sections->count();++i) if (sections->tabText(i)=="All Properties") sections->setCurrentIndex(i);auto *table=panel->findChild<QTableWidget *>("resourceProperties");QComboBox *mass=nullptr;for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="DecrementMass") {mass=qobject_cast<QComboBox *>(table->cellWidget(row,1));reveal(mass);table->scrollToItem(table->item(row,0));}require(mass,"Integrated mass-decrement control missing");mass->setCurrentText(m.massOff ? "false":"true");auto *coefficient=panel->findChild<QPushButton *>("thrusterCoefficients");reveal(coefficient);acceptanceView(*child);std::exception_ptr failure;
      QTimer::singleShot(0,panel,[&] {auto *d=panel->findChild<QDialog *>("thrusterCoefficientDialog");try {require(d,"Actual integrated coefficient dialog missing");auto *tabs=d->findChild<QTabWidget *>();auto *c=d->findChild<QTableWidget *>("thrusterThrustCoefficients"),*k=d->findChild<QTableWidget *>("thrusterSecondaryCoefficients");require(tabs && c && k && c->rowCount()==16 && k->rowCount()==16,"Integrated coefficient families incomplete");tabs->setCurrentIndex(0);c->item(0,1)->setText("0");c->item(1,1)->setText("0.1");c->scrollToItem(c->item(0,1));acceptanceView(*d);tabs->setCurrentIndex(1);k->item(0,1)->setText("300");k->item(1,1)->setText(QString::number(m.k2,'g',17));k->scrollToItem(k->item(0,1));acceptanceView(*d);require(source->toPlainText()==before && registered->GetRealParameter("C1")==10,"Integrated coefficient pending state escaped Apply");d->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();}catch (...) {failure=std::current_exception();if (d) d->reject();}});coefficient->click();if (failure) std::rethrow_exception(failure);apply(panel,before);
      panel=open("Prop");before=source->toPlainText();set(panel,"InitialStepSize","5");set(panel,"MaxStep","5");set(panel,"Accuracy","1e-12");apply(panel,before);
      for (const auto &label:QStringList{"Ignite","Shutdown"}) {const auto snapshot=window.missionSnapshot();int at=-1;for (int i=0;i<snapshot.nodes.size();++i) if (missionCommandLabel(snapshot.nodes[i].statement).name==label) at=i;require(at>=0,"Integrated burn command label missing");QTreeWidgetItem *item=nullptr;for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==at) item=*it;require(item,"Integrated burn command tree item missing");tree->itemDoubleClicked(item,0);events();child=area->activeSubWindow();auto *command=dynamic_cast<CommandEditor *>(child->widget());require(command && !command->hasChanges(),"Actual integrated burn command editor missing/dirty");view();auto *input=command->findChild<QLineEdit *>("commandField_Burn");reveal(input);before=source->toPlainText();input->setText("Burn");require(command->hasChanges() && source->toPlainText()==before,"Integrated burn command draft escaped Apply");acceptanceView(*child);command->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();require(!command->hasChanges(),"Integrated burn command Apply failed");const auto after=source->toPlainText();require(after==QString(before).replace("'"+label+"' IdleBurn(Sat)","'"+label+"' Burn(Sat)") && child->close(),"Integrated burn command changed unrelated source");events();source->undo();require(source->toPlainText()==before && window.buildScript(),"Integrated burn command exact Undo failed");source->redo();require(source->toPlainText()==after && window.buildScript(),"Integrated burn command exact Redo failed");++commandApplies;}
      const auto finalSource=source->toPlainText();require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==finalSource,"Integrated thruster Unicode persistence failed");save("mission.script",finalSource.toUtf8());messages->clear();const auto result=window.runMission();events();save("messages.txt",messages->toPlainText().toUtf8());if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed && Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Integrated thruster mission failed");
      QJsonArray expected;auto append=[&](const QJsonArray &a) {for (auto x:a) expected.append(x);};State state;append(values(m,state,0,false,.4,1.5));append(values(m,state,1,true,.4,1.5));for (int i=0;i<3;++i) {advance(m,state,600,true,m.duties[i],m.scales[i]);append(values(m,state,i+2,true,m.duties[i],m.scales[i]));}append(values(m,state,5,false,m.duties[2],m.scales[2]));advance(m,state,600,false,m.duties[2],m.scales[2]);append(values(m,state,6,false,m.duties[2],m.scales[2]));advance(m,state,600,true,m.duties[3],m.scales[3]);append(values(m,state,7,true,m.duties[3],m.scales[3]));append(values(m,state,8,false,m.duties[3],m.scales[3]));advance(m,state,600,false,m.duties[3],m.scales[3]);append(values(m,state,9,false,m.duties[3],m.scales[3]));const auto raw=read(report);save("values.txt",raw);const auto tokens=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(tokens.size()==expected.size() && expected.size()==300,"Integrated thruster report cardinality differs");QJsonArray actual;double maxError=0;
      for (int i=0;i<tokens.size();++i) {bool ok=false;const double x=tokens[i].toDouble(&ok),error=std::abs(x-expected[i].toDouble());const int col=i%30;const double tol=col==1 ? 1e-5:col==2 || col==7 ? 2e-6:col==3 || (col>=8 && col<=10) ? 5e-6:col>=11 && col<=13 ? 1e-9:col==17 ? 2e-6:col==16 || (col>=19 && col<=21) ? 1e-7:(col>=22 && col<=24) ? 5e-12:1e-9;actual.append(x);maxError=std::max(maxError,error);if (!ok || error>tol) std::cerr<<"INTEGRATED_THRUSTER_DIFF "<<mode.toStdString()<<" row="<<i/30<<" col="<<col<<" actual="<<x<<" expected="<<expected[i].toDouble()<<" error="<<error<<std::endl;require(ok && std::isfinite(x) && error<=tol,"Integrated thruster results differ from independent mass-time, rocket and trajectory oracle");}
      auto *outputTree=window.findChild<QTreeWidget *>("Output");auto reports=outputTree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"Integrated output report missing");outputTree->itemDoubleClicked(reports.front(),0);events();child=area->activeSubWindow();require(dynamic_cast<ReportViewer *>(child->widget()),"Actual integrated report viewer missing");view();auto *shown=child->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Integrated viewer differs from checked report bytes");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*child);
      QJsonArray duties,scales;for (auto d:m.duties) duties.append(d);for (auto s:m.scales) scales.append(s);QJsonObject proof{{"checkpoint","thruster-integration-"+mode},{"outcome","passed"},{"mode",mode},{"regulated",m.regulated},{"mass_off",m.massOff},{"k2",m.k2},{"duties",duties},{"scales",scales},{"resource_applies",resourceApplies},{"command_applies",commandApplies},{"expected",expected},{"actual",actual},{"max_numeric_error",maxError},{"facets",QJsonArray{"actual-tank-performance-and-coefficient-configuration","actual-integrator-and-burn-command-editors","pending-isolation-exact-history-unicode","analytic-regulated-and-blowdown-mass-time","variable-isp-rocket-and-trajectory","active-duty-scale-massoff-coast-restart","actual-report-viewer"}},{"scope","Synthetic single-engine negligible-gravity integrated propulsion with constant/C2 pressure-dependent thrust, constant/linear pressure-dependent Isp, regulated/blowdown pressure, duty/scale/mass-off, shutoff/coast/restart. Closed-form mass and velocity plus independent trajectory quadrature, no measured physics, arbitrary polynomial/tank/frame/solver/native-input/platform/stress/current private NASA corpus or final full/unattended regression. Earth mu1e-20 contributes under2e-21km over3600s with radius>=6000km; this is not Earth gravity qualification."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
