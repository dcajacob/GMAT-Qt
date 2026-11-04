#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "ODEModel.hpp"
#include "DragForce.hpp"
#include "GmatCommand.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "AtmosphereModel.hpp"
#include "BaseException.hpp"
#include <QAction>
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
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static void write(const QString &path,const QByteArray &bytes) {QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Drag evidence write failed");}
static QByteArray read(const QString &path) {QFile f(path);require(f.open(QIODevice::ReadOnly),"Drag evidence missing");return f.readAll();}
using V=std::array<double,3>;
using S=std::array<double,6>;
static double norm(V v) {return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);}
template<size_t N> static QJsonArray json(const std::array<double,N> &v) {QJsonArray a;for (double x:v) a.append(x);return a;}
static void reveal(QWidget *w) {
   require(w,"Drag control missing");
   for (auto *p=w;p;p=p->parentWidget()) {
      for (auto *tabs:w->window()->findChildren<QTabWidget *>()) if (tabs->indexOf(p)>=0) tabs->setCurrentWidget(p);
      if (auto *scroll=qobject_cast<QScrollArea *>(p)) scroll->ensureWidgetVisible(w);
   }
   events();require(w->isVisible()&&w->isEnabled(),"Drag control not reachable");
}
// Independent SI force equation, with explicit km-to-m conversion. The synthetic
// table makes density constant to floating-point precision (H=1e300 km).
static V acceleration(const S &s,const V &omega,double cd,double area,double mass) {
   V relative{};for (int i=0;i<3;++i) relative[i]=1000*(s[i+3]-(omega[(i+1)%3]*s[(i+2)%3]-omega[(i+2)%3]*s[(i+1)%3]));
   const double speed=norm(relative),newtonsPerSpeed=.5*1e-8*cd*area*speed;
   V a{};for (int i=0;i<3;++i) a[i]=-newtonsPerSpeed*relative[i]/mass/1000;
   return a;
}
static S integrate(S state,double seconds,double mu,const V &w0,const V &w1,double cd,double area,double mass,int steps) {
   const double h=seconds/steps;
   auto rate=[&](const S &s,double t) {V w{};for (int i=0;i<3;++i) w[i]=w0[i]+(w1[i]-w0[i])*t/seconds;const auto a=acceleration(s,w,cd,area,mass);const double r=norm({s[0],s[1],s[2]});S d{};for (int i=0;i<3;++i) {d[i]=s[i+3];d[i+3]=-mu*s[i]/(r*r*r)+a[i];}return d;};
   for (int n=0;n<steps;++n) {
      const double t=n*h;const auto k1=rate(state,t);S s2{},s3{},s4{};
      for (int i=0;i<6;++i) s2[i]=state[i]+h*k1[i]/2;const auto k2=rate(s2,t+h/2);
      for (int i=0;i<6;++i) s3[i]=state[i]+h*k2[i]/2;const auto k3=rate(s3,t+h/2);
      for (int i=0;i<6;++i) s4[i]=state[i]+h*k3[i];const auto k4=rate(s4,t+h);
      for (int i=0;i<6;++i) state[i]+=h*(k1[i]+2*k2[i]+2*k3[i]+k4[i])/6;
   }
   return state;
}
struct Trial {const char *name;double cd,area,dry,fuel,second;S initial;};
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaCannonballDrag");
   try {
      require(argc==2&&acceptanceWidgetPlatform(),"Supply startup and widget platform");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Drag fixture directory missing");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &bytes) {if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes);};
      const auto density=files.filePath("Constant density Δ.txt");write(density,"0, 1e-8, 1e300\n10000, 1e-8, 1e300\n");save("constant-density.txt",read(density));
      MainWindow window;require(window.initialize(startup),"Drag runtime initialization failed");window.show();events();acceptanceView(window);
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&area,"Drag workspace missing");
      const S planar{7000,0,0,0,7.5,0},reverse{7000,0,0,0,-7.5,0},inclined{4200,0,5600,-6,1,4.5};
      const QList<Trial> trials={{"Base",2.2,20,800,150,50,planar},{"ZeroArea",2.2,0,800,150,50,planar},{"ZeroCd",0,20,800,150,50,planar},{"DoubleArea",2.2,40,800,150,50,planar},{"DoubleCd",4.4,20,800,150,50,planar},{"DryHeavy",2.2,20,1800,150,50,planar},{"FuelHeavy",2.2,20,800,1150,50,planar},{"SecondHeavy",2.2,20,800,150,1050,planar},{"Reverse",2.2,20,800,150,50,reverse},{"Inclined",2.2,20,800,150,50,inclined}};
      QJsonArray observations;int resourceApplies=0,atmosphereDialogs=0;QList<S> finalStates;
      for (const auto &trial:trials) {
         const QString name=trial.name;const auto report=files.filePath(name+" drag Δ.csv"),saved=files.filePath(name+" drag Ω.script");
         QString initial="% Synthetic cannonball acceptance, not measured atmosphere Δ\nCreate Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\n";
         const QStringList axes{"X","Y","Z","VX","VY","VZ"};for (int i=0;i<6;++i) initial+="Sat."+axes[i]+" = "+QString::number(trial.initial[i],'g',17)+";\n";
         initial+="Create ChemicalTank Fuel Second Unattached;\nFuel.Volume = 10;\nSecond.Volume = 10;\nUnattached.Volume = 20;\nUnattached.FuelMass = 9999;\nSat.Tanks = {Fuel, Second};\nCreate ForceModel FM;\nFM.PrimaryBodies = {Earth};\nFM.PointMasses = {};\nFM.GravityField.Earth.Degree = 0;\nFM.GravityField.Earth.Order = 0;\nFM.Drag.AtmosphereModel = None;\nFM.SRP = Off;\nFM.RelativisticCorrection = Off;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 1;\nP.MinStep = 0;\nP.MaxStep = 1;\nP.Accuracy = 1e-12;\nCreate Variable Keep;\nKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nSat.DryMass = 799;\nSat.Cd = 1;\nSat.DragArea = 19;\nFuel.FuelMass = 149;\nSecond.FuelMass = 49;\nBeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 10};\nReport Values Sat.ElapsedSecs Sat.TotalMass Sat.FM.AtmosDensity Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Keep;\n";
         source->setPlainText(initial);save(name+"-initial.script",initial.toUtf8());if (!window.buildScript()) {save(name+"-build-messages.txt",messages->toPlainText().toUtf8());std::cerr<<messages->toPlainText().toStdString();throw std::runtime_error("Drag initial build failed");}
         QPointer<QMdiSubWindow> child;
         auto view=[&] {child->showNormal();child->setGeometry(area->viewport()->rect().adjusted(4,4,-4,-4));area->setActiveSubWindow(child);child->raise();events();acceptanceView(*child);};
         auto open=[&](const QString &resource) {for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->tabText(i)=="Resources") tabs->setCurrentIndex(i);auto items=tree->findItems(resource,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Drag resource missing");tree->itemDoubleClicked(items.first(),0);events();child=area->activeSubWindow();auto *p=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(p&&!p->hasChanges(),"Actual drag resource editor missing/dirty");view();return p;};
         auto apply=[&](ResourceEditor *p,const QString &before) {require(p->hasChanges()&&source->toPlainText()==before,"Drag draft escaped Apply");auto *button=p->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);reveal(button);QTest::mouseClick(button,Qt::LeftButton);events();p=dynamic_cast<ResourceEditor *>(child->widget());if (p&&p->hasChanges()) std::cerr<<p->findChild<QLabel *>("resourceStatus")->text().toStdString();require(p&&!p->hasChanges(),"Drag resource Apply failed");const auto after=source->toPlainText();require(after!=before&&child->close(),"Drag Apply unchanged or clean close failed");events();source->undo();require(source->toPlainText()==before&&window.buildScript(),"Drag exact Undo failed");source->redo();require(source->toPlainText()==after&&window.buildScript(),"Drag exact Redo failed");++resourceApplies;};
         auto *panel=open("Sat");auto before=source->toPlainText();auto *configured=Moderator::Instance()->GetConfiguredObject("Sat");
         for (const auto &pair:QList<QPair<QString,double>>{{"DryMass",trial.dry},{"Cd",trial.cd},{"DragArea",trial.area}}) {auto *edit=panel->findChild<QLineEdit *>("ballistics_"+pair.first);reveal(edit);edit->setText(QString::number(pair.second,'g',17));acceptanceView(*child);require(source->toPlainText()==before&&configured->GetRealParameter("DryMass")==799,"Drag ballistics draft changed configured source/mass");}
         apply(panel,before);
         for (const auto &pair:QList<QPair<QString,double>>{{"Fuel",trial.fuel},{"Second",trial.second}}) {panel=open(pair.first);before=source->toPlainText();auto *edit=panel->findChild<QLineEdit *>("resource_FuelMass");reveal(edit);edit->setText(QString::number(pair.second,'g',17));acceptanceView(*child);apply(panel,before);}
         panel=open("FM");before=source->toPlainText();auto *atmosphere=panel->findChild<QPushButton *>("forceAtmosphere");reveal(atmosphere);
         auto configure=[&](bool commit) {acceptanceView(*child);std::exception_ptr failure;bool visited=false;QTimer::singleShot(0,panel,[&] {auto *dialog=panel->findChild<QDialog *>("atmosphereDialog");try {require(dialog,"Actual drag atmosphere dialog missing");acceptanceView(*dialog);visited=true;auto *model=dialog->findChild<QComboBox *>("atmosphere_AtmosphereModel"),*shape=dialog->findChild<QComboBox *>("atmosphere_DragModel");require(model&&shape&&model->findText("Exponential")>=0&&shape->findText("Spherical")>=0,"Cannonball drag choices missing");model->setCurrentText("Exponential");shape->setCurrentText("Spherical");auto *file=dialog->findChild<QLineEdit *>("atmosphere_InputFile");reveal(file);file->setText(density);acceptanceView(*dialog);require(source->toPlainText()==before,"Drag dialog changed source before Apply");QTest::mouseClick(dialog->findChild<QDialogButtonBox *>()->button(commit?QDialogButtonBox::Ok:QDialogButtonBox::Cancel),Qt::LeftButton);if (dialog->result()!=(commit?QDialog::Accepted:QDialog::Rejected)) {auto *error=dialog->findChild<QLabel *>("atmosphereError");std::cerr<<"DRAG_DIALOG commit="<<commit<<" result="<<dialog->result()<<" error="<<(error?error->text().toStdString():"missing diagnostic")<<std::endl;}require(dialog->result()==(commit?QDialog::Accepted:QDialog::Rejected),"Drag dialog decision differs");}catch (...) {failure=std::current_exception();if (dialog) dialog->reject();}});QTest::mouseClick(atmosphere,Qt::LeftButton);if (failure) std::rethrow_exception(failure);require(visited,"Drag atmosphere callback not exercised");++atmosphereDialogs;};
         configure(false);require(!panel->hasChanges()&&source->toPlainText()==before,"Drag atmosphere Cancel changed source/draft");configure(true);apply(panel,before);
         const auto wanted=source->toPlainText();save(name+"-export.script",wanted.toUtf8());require(wanted.contains("FM.Drag.AtmosphereModel = Exponential;")&&wanted.contains("FM.Drag.DragModel = 'Spherical';")&&wanted.contains(density)&&wanted.contains("Unattached.FuelMass = 9999;")&&wanted.contains("% Synthetic cannonball acceptance"),"Drag export lost shape, file, sentinel or unrelated source");
         require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==wanted,"Drag Unicode save/reopen failed");save(name+"-mission.script",wanted.toUtf8());panel=open("Sat");for (const auto &pair:QList<QPair<QString,double>>{{"DryMass",trial.dry},{"Cd",trial.cd},{"DragArea",trial.area}}) {auto *edit=panel->findChild<QLineEdit *>("ballistics_"+pair.first);reveal(edit);require(edit->text().toDouble()==pair.second,"Drag reopened ballistics values differ");}acceptanceView(*child);require(child->close(),"Drag reopened editor close failed");events();
         const int undo=source->document()->availableUndoSteps();messages->clear();const auto outcome=window.runMission();events();save(name+"-messages.txt",messages->toPlainText().toUtf8());if (outcome!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(outcome==MainWindow::RunResult::Completed,"Cannonball propagation failed");
         const auto raw=read(report);save(name+"-values.csv",raw);const auto words=QString::fromUtf8(raw).trimmed().split(',');require(words.size()==10,"Drag report cardinality differs");QJsonArray columns;for (const auto &word:words) {bool ok=false;const auto number=word.toDouble(&ok);require(ok&&std::isfinite(number),"Drag report contains nonfinite value");columns.append(number);}
         const double mass=trial.dry+trial.fuel+trial.second,reportedSeconds=columns[0].toDouble();require(std::abs(reportedSeconds-10)<1e-4&&columns[1].toDouble()==mass&&std::abs(columns[2].toDouble()-10)<=1e-12&&columns[9].toDouble()==41,"Drag time, attached mass, kg/km3 density or sentinel differs");
         auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));auto *fm=dynamic_cast<ODEModel *>(Moderator::Instance()->GetInternalObject("FM"));auto *force=fm?dynamic_cast<DragForce *>(fm->GetForceOfType("DragForce")):nullptr;require(sat&&force&&force->GetStringParameter("DragModel")=="Spherical","Native cannonball drag force missing/wrong shape");
         require(sat->GetRealParameter("TotalMass")==mass&&sat->GetRealParameter("Cd")==trial.cd&&sat->GetRealParameter("DragArea")==trial.area,"Native drag ballistics or attached mass differs");
         GmatCommand *propagate=Moderator::Instance()->GetFirstCommand();while (propagate&&!propagate->IsOfType("Propagate")) propagate=propagate->GmatCommand::GetNext();auto *setup=propagate?dynamic_cast<PropSetup *>(propagate->GetClone(0)):nullptr;require(setup&&setup->GetPropagator(),"Drag live propagation clone missing");const double seconds=setup->GetPropagator()->GetTime();require(std::abs(seconds-10)<=1e-9,"Drag integrator duration differs from requested10seconds");save(name+"-clock.json",QJsonDocument(QJsonObject{{"integrator_seconds",seconds},{"reported_elapsed_seconds",reportedSeconds},{"difference_seconds",reportedSeconds-seconds}}).toJson());
         const auto native=force->GetDerivativesForSpacecraft(sat);auto *atmos=force->GetAtmosphereModel();require(atmos,"Native drag atmosphere missing");V w0{},w1{};auto *omega=atmos->GetAngularVelocity(21545);for (int i=0;i<3;++i) w0[i]=omega[i];omega=atmos->GetAngularVelocity(sat->GetEpoch());for (int i=0;i<3;++i) w1[i]=omega[i];S actual{};for (int i=0;i<6;++i) actual[i]=columns[3+i].toDouble();
         const auto expectedAcceleration=acceleration(actual,w1,trial.cd,trial.area,mass);V actualAcceleration{};
         for (int i=0;i<3;++i) {actualAcceleration[i]=native[3+i];require(native[i]==0&&std::abs(native[3+i]-expectedAcceleration[i])<=1e-12*norm(expectedAcceleration)+1e-18,"Cannonball native force differs from independent SI drag law");}
         const double mu=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant();const auto expected=integrate(trial.initial,seconds,mu,w0,w1,trial.cd,trial.area,mass,1000),fine=integrate(trial.initial,seconds,mu,w0,w1,trial.cd,trial.area,mass,2000);S errors{},convergence{};
         for (int i=0;i<6;++i) {errors[i]=std::abs(actual[i]-fine[i]);convergence[i]=std::abs(expected[i]-fine[i]);if (errors[i]>(i<3?1e-6:1e-9)) std::cerr<<"DRAG_DIFF "<<trial.name<<" component="<<i<<" actual="<<actual[i]<<" expected="<<fine[i]<<" error="<<errors[i]<<std::endl;require(convergence[i]<=(i<3?1e-9:1e-12),"Independent drag RK4 refinement not converged");require(errors[i]<=(i<3?1e-6:1e-9),"Cannonball propagated state differs from independent RK4 force integration");}
         require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==undo&&!window.isRunning()&&window.findChild<QAction *>("runMission")->isEnabled(),"Drag run altered source/history or retained lock");
         auto *outputs=window.findChild<QTreeWidget *>("Output");auto items=outputs->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Drag output report missing");outputs->itemDoubleClicked(items.first(),0);events();child=area->activeSubWindow();require(child&&dynamic_cast<ReportViewer *>(child->widget()),"Actual drag report viewer missing");view();auto *shown=child->findChild<QPlainTextEdit *>();require(shown&&shown->isReadOnly()&&shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Drag viewer differs from checked raw report");require(child->close(),"Drag viewer close failed");events();
         observations.append(QJsonObject{{"trial",name},{"initial_state",json(trial.initial)},{"cd",trial.cd},{"area_m2",trial.area},{"dry_mass_kg",trial.dry},{"fuel_mass_kg",trial.fuel},{"second_tank_mass_kg",trial.second},{"total_mass_kg",mass},{"density_kg_m3",1e-8},{"mu_km3_s2",mu},{"omega_start_rad_s",json(w0)},{"omega_final_rad_s",json(w1)},{"seconds",seconds},{"reported_elapsed_seconds",reportedSeconds},{"report",columns},{"native_acceleration_km_s2",json(actualAcceleration)},{"independent_acceleration_km_s2",json(expectedAcceleration)},{"independent_state",json(fine)},{"state_errors",json(errors)},{"RK4_refinement_errors",json(convergence)}});save("observations.json",QJsonDocument(observations).toJson());finalStates.append(actual);
      }
      for (int i=0;i<6;++i) {require(std::abs(finalStates[1][i]-finalStates[2][i])<=1e-12,"Zero Cd and zero area trajectories differ");require(std::abs(finalStates[3][i]-finalStates[4][i])<=1e-12,"Equal Cd-area product changed propagation");require(std::abs(finalStates[5][i]-finalStates[6][i])<=1e-12&&std::abs(finalStates[5][i]-finalStates[7][i])<=1e-12,"Equal total mass allocations changed drag propagation");}
      require(resourceApplies==40&&atmosphereDialogs==20,"Drag GUI operation count differs");
      const QJsonObject proof{{"checkpoint","cannonball-drag-law"},{"outcome","passed"},{"trial_count",10},{"resource_applies",resourceApplies},{"atmosphere_dialogs",atmosphereDialogs},{"observations",observations},{"facets",QJsonArray{"actual-ballistics-two-tanks-and-atmosphere-shape-controls","cancel-pending-apply-exact-undo-redo-unicode-readback","all-attached-no-unattached-total-mass","independent-SI-spherical-drag-force","independent-converged-RK4-integrated-state","zero-Cd-zero-area-equal-product-mass-allocation-boundaries","actual-checked-report-viewer"}},{"scope","Ten synthetic Earth point-mass/constant-density spherical-drag missions: independent SI force and 10s RK4 state integration, zero Cd/area, doubled coefficient/area, three equal heavier dry/fuel/tank allocations, prograde/reverse/inclined states. Native Earth mu and atmosphere angular velocity endpoints are explicit shared environmental inputs, not independent Earth orientation truth. Constant synthetic density is1e-8kg/m3 via H1e300km, not empirical atmosphere validation. No MSISE86, wind/weather/epoch/long-duration/all-body/all-frame/native-OS-input/GPU/hardware/platform/stress/current-private NASA/full-build qualification."}};
      save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
