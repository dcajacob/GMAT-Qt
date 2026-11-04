#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CelestialBodyPanel.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "CelestialBody.hpp"
#include "TimeSystemConverter.hpp"
#include "BaseException.hpp"
#include <QApplication>
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
#include <QScrollBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
using Vec=std::array<double,3>;using State=std::array<double,6>;
static const QStringList names{"SpinAxisRAConstant","SpinAxisRARate","SpinAxisDECConstant","SpinAxisDECRate","RotationConstant","RotationRate"};
static const State inertial{1000,-200,300,1.25,-.5,.75};
static double dot(const Vec &a,const Vec &b) {return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static Vec add(const Vec &a,const Vec &b) {return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
static Vec scale(const Vec &a,double s) {return {a[0]*s,a[1]*s,a[2]*s};}
static double tdb(double a1) {return TimeSystemConverter::Instance()->Convert(a1,TimeSystemConverter::A1MJD,TimeSystemConverter::TDBMJD,2430000.0);}
// Independent geometric basis: node = north(J2000) cross pole, tangent =
// pole cross node. No GMAT rotation, cartographic or coordinate utilities.
static State truth(const State &p,double reference,double epoch)
{
   constexpr double rad=3.1415926535897932384626433832795/180.0;
   const double days=tdb(epoch)-tdb(reference),a=(p[0]+p[1]*days/36525)*rad,d=(p[2]+p[3]*days/36525)*rad,w=(p[4]+p[5]*days)*rad;
   const double ad=p[1]*rad/(36525.0*86400.0),dd=p[3]*rad/(36525.0*86400.0),wd=p[5]*rad/86400;
   const double ca=std::cos(a),sa=std::sin(a),cd=std::cos(d),sd=std::sin(d),cw=std::cos(w),sw=std::sin(w);
   const Vec n{-sa,ca,0},m{-sd*ca,-sd*sa,cd},z{cd*ca,cd*sa,sd};
   const Vec nd{-ca*ad,-sa*ad,0},md{sd*sa*ad-cd*ca*dd,-sd*ca*ad-cd*sa*dd,-sd*dd},zd{-cd*sa*ad-sd*ca*dd,cd*ca*ad-sd*sa*dd,cd*dd};
   const Vec x=add(scale(n,cw),scale(m,sw)),y=add(scale(n,-sw),scale(m,cw));
   const Vec xd=add(add(scale(n,-sw*wd),scale(nd,cw)),add(scale(m,cw*wd),scale(md,sw)));
   const Vec yd=add(add(scale(n,-cw*wd),scale(nd,-sw)),add(scale(m,-sw*wd),scale(md,cw)));
   const Vec r{inertial[0],inertial[1],inertial[2]},v{inertial[3],inertial[4],inertial[5]};State result{};
   const std::array<Vec,3> axes{x,y,z},rates{xd,yd,zd};for (int i=0;i<3;++i) {result[i]=dot(axes[i],r);result[i+3]=dot(axes[i],v)+dot(rates[i],r);}return result;
}
static CelestialBody *body() {auto *p=dynamic_cast<CelestialBody *>(Moderator::Instance()->GetConfiguredObject("Probe"));require(p,"Orientation body missing");return p;}
static double value(const QString &key) {return body()->GetRealParameter(body()->GetParameterID(key.toStdString()));}
static QJsonArray json(const State &s) {QJsonArray a;for (auto v:s) a.append(v);return a;}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaBodyOrientation");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and orientation case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Orientation fixture directory missing");const QString mode=argv[2];const int field=names.indexOf(mode);require(field>=0 || mode=="Coupled" || mode=="Epoch","Unknown orientation case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto kernel=QFileInfo("../data/planetary_ephem/spk/ceres_1900_2100.bsp").canonicalFilePath();require(!kernel.isEmpty(),"Ceres orientation fixture missing");
      const auto report=files.filePath("Orientation Δ.txt");const double reference=21547.25;State original{10,0,65,0,20,0},wanted=original;if (mode=="Epoch") original[5]=wanted[5]=75;
      if (field>=0) wanted[field]=State{35,73050,-30,-36525,-73,-120}[field];if (mode=="Coupled") wanted={37,36525,53,-18262.5,-40,73};const double wantedEpoch=mode=="Epoch" || mode=="Coupled" ? reference+.375 : reference;
      QString initial="% Preserve body orientation mission Δ\nCreate Asteroid Probe;\nProbe.NAIFId = 2000001;\nProbe.OrbitSpiceKernelName = {'"+kernel+"'};\nProbe.OrientationEpoch = "+QString::number(reference,'g',17)+";\n";
      for (int i=0;i<6;++i) initial+="Probe."+names[i]+" = "+QString::number(original[i],'g',17)+";\n";
      initial+="Create CoordinateSystem Inertial Fixed;\nInertial.Origin = Probe;\nInertial.Axes = MJ2000Eq;\nFixed.Origin = Probe;\nFixed.Axes = BodyFixed;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = Inertial;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21547.25';\nSat.DisplayStateType = Cartesian;\nSat.X = 1000;\nSat.Y = -200;\nSat.Z = 300;\nSat.VX = 1.25;\nSat.VY = -0.5;\nSat.VZ = 0.75;\nCreate ReportFile States;\nStates.Filename = '"+report+"';\nStates.WriteHeaders = false;\nStates.Precision = 16;\nCreate Variable Keep;\nKeep = 41; % unrelated initializer\nBeginMissionSequence;\n";
      const std::array<double,3> epochs{reference,reference+.375,reference+1.25};for (auto e:epochs) initial+="Sat.A1ModJulian = "+QString::number(e,'g',17)+";\nSat.Inertial.X = 1000;\nSat.Inertial.Y = -200;\nSat.Inertial.Z = 300;\nSat.Inertial.VX = 1.25;\nSat.Inertial.VY = -0.5;\nSat.Inertial.VZ = 0.75;\nReport States Sat.A1ModJulian Sat.Inertial.X Sat.Inertial.Y Sat.Inertial.Z Sat.Inertial.VX Sat.Inertial.VY Sat.Inertial.VZ Sat.Fixed.X Sat.Fixed.Y Sat.Fixed.Z Sat.Fixed.VX Sat.Fixed.VY Sat.Fixed.VZ;\n";initial+="Keep = Keep + 1; % unrelated mission\n";
      MainWindow window;require(window.initialize(startup),"Orientation runtime initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source && tree && area,"Orientation workspace missing");source->setPlainText(initial);require(window.buildScript(),"Orientation fixture failed to build");
      const auto items=tree->findItems("Probe",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Orientation resource entry missing");tree->itemDoubleClicked(items.front(),0);events();QMdiSubWindow *child=nullptr;for (auto *w:area->subWindowList()) if (w->property("resourceName").toString()=="Probe") child=w;require(child,"Actual orientation editor missing");auto *panel=dynamic_cast<CelestialBodyPanel *>(child->widget());require(panel,"Actual celestial-body editor missing");panel->findChild<QTabWidget *>("bodyPages")->setCurrentIndex(2);
      auto entry=[&](const QString &key) {auto *f=panel->findChild<QLineEdit *>("body_"+key);require(f && f->isVisible() && !f->isReadOnly(),qPrintable("Editable orientation control missing: "+key));return f;};
      auto view=[&] {if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);child->showNormal();events();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);child->raise();events();require(area->activeSubWindow()==child,"Orientation panel is obscured");}acceptanceView(*child);};view();
      require(entry("OrientationEpoch")->text().toDouble()==reference,"Orientation epoch display is not the configured A1 epoch");for (int i=0;i<6;++i) require(entry(names[i])->text().toDouble()==original[i],"Orientation control differs from configured value");const auto undo=source->document()->availableUndoSteps();int invalid=0;
      auto unchanged=[&] {require(source->toPlainText()==initial && source->document()->availableUndoSteps()==undo && value("OrientationEpoch")==reference,"Pending/invalid orientation changed source/history/epoch");for (int i=0;i<6;++i) require(value(names[i])==original[i],"Pending/invalid orientation changed configured constants/rates");};
      auto reject=[&](const QString &key,const QString &text) {auto *e=entry(key);const auto before=e->text();e->setText(text);panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();require(panel==child->widget() && panel->hasChanges() && !panel->findChild<QLabel *>("bodyStatus")->text().isEmpty(),"Invalid orientation accepted or lacks diagnostic");unchanged();++invalid;view();e->setText(before);require(!panel->hasChanges(),"Restored orientation field remains dirty");};
      if (mode=="Epoch") {for (const auto &key:QStringList(names)+QStringList{"OrientationEpoch"}) for (const auto &bad:QStringList{"nan","inf"}) reject(key,bad);reject("OrientationEpoch","6115.999");reject("OrientationEpoch","58127.501");}
      else reject(field>=0 ? names[field] : "RotationConstant","not-a-number");
      for (int i=0;i<6;++i) entry(names[i])->setText(QString::number(wanted[i],'g',17));entry("OrientationEpoch")->setText(QString::number(wantedEpoch,'g',17));unchanged();require(panel->hasChanges(),"Orientation edit was not pending");view();
      panel->findChild<QPushButton *>("cancelPanel")->click();events();unchanged();tree->itemDoubleClicked(items.front(),0);events();child=nullptr;for (auto *w:area->subWindowList()) if (w->property("resourceName").toString()=="Probe") child=w;require(child,"Orientation editor did not reopen after Cancel");panel=dynamic_cast<CelestialBodyPanel *>(child->widget());panel->findChild<QTabWidget *>("bodyPages")->setCurrentIndex(2);for (int i=0;i<6;++i) entry(names[i])->setText(QString::number(wanted[i],'g',17));entry("OrientationEpoch")->setText(QString::number(wantedEpoch,'g',17));panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();panel=dynamic_cast<CelestialBodyPanel *>(child->widget());require(panel && !panel->hasChanges(),"Corrected orientation Apply failed");for (int i=0;i<6;++i) require(value(names[i])==wanted[i],"Applied orientation value differs");require(value("OrientationEpoch")==wantedEpoch,"Applied orientation reference epoch differs");panel->findChild<QTabWidget *>("bodyPages")->setCurrentIndex(2);view();require(child->close(),"Clean orientation panel failed to close");events();const auto applied=source->toPlainText();require(applied.contains("Keep = 41; % unrelated initializer") && applied.contains("Keep = Keep + 1; % unrelated mission"),"Orientation Apply changed unrelated source");
      source->undo();require(source->toPlainText()==initial && window.buildScript(),"Orientation Undo did not restore exact source/model");for (int i=0;i<6;++i) require(value(names[i])==original[i],"Orientation Undo restored wrong values");source->redo();require(source->toPlainText()==applied && window.buildScript(),"Orientation Redo did not restore exact applied source/model");const auto saved=files.filePath("Orientation Δ.script");require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==applied,"Orientation Unicode save/reopen changed exact source/model");for (int i=0;i<6;++i) require(value(names[i])==wanted[i],"Reopened orientation values differ");require(value("OrientationEpoch")==wantedEpoch,"Reopened orientation epoch differs");
      const auto result=window.runMission();if (result!=MainWindow::RunResult::Completed) std::cerr<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Orientation report mission failed");require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Orientation mission failed to continue");QFile data(report);require(data.open(QIODevice::ReadOnly),"Orientation report missing");const auto raw=data.readAll();const auto words=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==39,"Orientation report must have three rows of epoch plus two six-states");QJsonArray samples;double positionError=0,velocityError=0;
      for (int row=0;row<3;++row) {bool ok=false;const double epoch=words[row*13].toDouble(&ok);require(ok && std::abs(epoch-epochs[row])<=1e-10,"Orientation report epoch differs");State actual{};const auto expected=truth(wanted,wantedEpoch,epoch);for (int i=0;i<6;++i) {const double input=words[row*13+1+i].toDouble(&ok);require(ok && std::abs(input-inertial[i])<=1e-9,"Orientation fixture inertial state changed with epoch assignment");actual[i]=words[row*13+7+i].toDouble(&ok);require(ok && std::isfinite(actual[i]),"Orientation report state is nonfinite");const auto error=std::abs(actual[i]-expected[i]);if (i<3) positionError=std::max(positionError,error);else velocityError=std::max(velocityError,error);if (error>(i<3 ? 2e-7 : 2e-9)) std::cerr<<"ORIENTATION_DIFF mode="<<mode.toStdString()<<" row="<<row<<" component="<<i<<" actual="<<actual[i]<<" expected="<<expected[i]<<" error="<<error<<std::endl;require(error<=(i<3 ? 2e-7 : 2e-9),"Body-fixed state differs from independent pole/node/prime-meridian derivative oracle");}samples.append(QJsonObject{{"epoch_a1mjd",epoch},{"actual",json(actual)},{"expected",json(expected)}});}
      auto *output=window.findChild<QTreeWidget *>("Output");const auto reports=output->findItems("States",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"Orientation output tree entry missing");output->itemDoubleClicked(reports.front(),0);events();QMdiSubWindow *reportWindow=nullptr;for (auto *w:area->subWindowList()) if (dynamic_cast<ReportViewer *>(w->widget())) reportWindow=w;require(reportWindow,"Orientation report viewer missing");reportWindow->showNormal();reportWindow->move(0,0);reportWindow->resize(area->viewport()->size());area->setActiveSubWindow(reportWindow);reportWindow->raise();events();acceptanceView(*reportWindow);auto *shown=reportWindow->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Visible orientation report differs from checked independent results");
      if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();require(shown->horizontalScrollBar()->value()==shown->horizontalScrollBar()->maximum() && area->activeSubWindow()==reportWindow,"Rightmost orientation result columns are not viewable");acceptanceView(*reportWindow);}
      const auto destination=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if (!destination.isEmpty()) {for (const auto &file:QStringList{report,saved}) require(QFile::copy(file,QDir(destination).filePath(QFileInfo(file).fileName())),"Cannot retain orientation report/source");}
      const QJsonObject proof{{"checkpoint","body-orientation-"+mode},{"outcome","passed"},{"mode",mode},{"invalid_rejections",invalid},{"orientation",json(wanted)},{"reference_epoch_a1mjd",wantedEpoch},{"samples",samples},{"max_position_error_km",positionError},{"max_velocity_error_km_s",velocityError},{"facets",QJsonArray{"actual-orientation-controls","pending-cancel-apply","invalid-atomicity","exact-undo-redo","unicode-save-reopen","independent-three-epoch-six-state","pole-rate-velocity-terms","actual-report-viewer"}},{"scope","Synthetic user-body IAU simplified orientation and report transformation only; shared A1/TDB conversion is an input, not independent time qualification. No physical measured orientation, ICRF bias, general SPK propagation or native/platform/stress/provenance claim."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
