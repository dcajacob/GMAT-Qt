#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "CelestialBodyPanel.hpp"
#include "BodyFixedPoint.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "CelestialBody.hpp"
#include "SolarSystem.hpp"
#include "A1Mjd.hpp"
#include "TimeSystemConverter.hpp"
extern "C" {
#include "SpiceUsr.h"
}
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
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QScrollArea>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QTimer>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
using V=std::array<double,3>;using State=std::array<double,6>;using Matrix=std::array<V,3>;
static const State input{1000,-200,300,1.25,-.5,.75};
static const QStringList components{"X","Y","Z","VX","VY","VZ"};
static constexpr double pi=3.1415926535897932384626433832795;
static double dot(const V &a,const V &b) {return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static V cross(const V &a,const V &b) {return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
static V scale(V a,double x) {for (auto &v:a) v*=x;return a;}
static QJsonArray json(const State &s) {QJsonArray a;for (auto x:s) a.append(x);return a;}
static QJsonArray json(const Matrix &m) {QJsonArray a;for (auto x:m) a.append(QJsonArray{x[0],x[1],x[2]});return a;}
struct Configuration {QString horizon;double flattening=.01,radius=6400,spin=120;V fixed{5000,-3000,4000};};
static Configuration configuration(const QString &mode,int stage)
{
   Configuration c;const bool changed=stage==1;
   c.horizon=mode=="HorizonSphere" || mode.endsWith("Sphere") ? "Sphere":"Ellipsoid";
   if (mode=="HorizonSphere" && changed) c.horizon="Ellipsoid";
   if (mode=="HorizonEllipsoid") {c.spin=0;if (changed) c.horizon="Sphere";}
   if (mode=="Flattening" && changed) c.flattening=.04;
   if (mode=="Radius") {c.spin=0;if (changed) c.radius=7200;}
   if (mode.startsWith("North") || mode.startsWith("South")) c.fixed={.0015,0,mode.startsWith("North") ? 6401.2:-6401.2};
   return c;
}
static double geodetic(const V &p,double radius,double flattening)
{
   const double rho=std::hypot(p[0],p[1]),e2=flattening*(2-flattening);
   if (!flattening) return std::atan2(p[2],rho);
   // Solve the normal/position relation by bracketing, independently of
   // TopocentricAxes' fixed-point atan iteration.
   auto residual=[&](double latitude) {const double s=std::sin(latitude),c=std::cos(latitude),N=radius/std::sqrt(1-e2*s*s);return rho*s-p[2]*c-e2*N*s*c;};
   double lo=-pi/2,hi=pi/2;require(residual(lo)<0 && residual(hi)>0,"Independent latitude bracket invalid");
   for (int i=0;i<160;++i) {const double mid=(lo+hi)/2;if (mid==lo || mid==hi) break;if (residual(mid)>0) hi=mid;else lo=mid;}return (lo+hi)/2;
}
struct Model {State origin{};Matrix rows{},rates{};double latitude=0,longitude=0,angle=0,rate=0;};
static Model model(const Configuration &c,double epoch)
{
   Model m;m.latitude=geodetic(c.fixed,c.radius,c.horizon=="Sphere" ? 0:c.flattening);m.longitude=std::atan2(c.fixed[1],c.fixed[0]);
   const auto tdb=[](double t) {return TimeSystemConverter::Instance()->Convert(t,TimeSystemConverter::A1MJD,TimeSystemConverter::TDBMJD,2430000.0);};m.angle=(30+c.spin*(tdb(epoch)-tdb(21545)))*pi/180;m.rate=c.spin*pi/(180*86400);
   auto rotate=[&](V p) {return V{std::cos(m.angle)*p[0]-std::sin(m.angle)*p[1],std::sin(m.angle)*p[0]+std::cos(m.angle)*p[1],p[2]};};const auto r=rotate(c.fixed),v=scale(cross(V{0,0,1},r),m.rate);for (int i=0;i<3;++i) {m.origin[i]=r[i];m.origin[i+3]=v[i];}
   const double s=std::sin(m.latitude),co=std::cos(m.latitude),sl=std::sin(m.longitude),cl=std::cos(m.longitude);const Matrix local{{{s*cl,s*sl,-co},{-sl,cl,0},{co*cl,co*sl,s}}};for (int i=0;i<3;++i) {m.rows[i]=rotate(local[i]);m.rates[i]=scale(cross(V{0,0,1},m.rows[i]),m.rate);}return m;
}
static QTableWidgetItem *cell(QWidget *owner,const QString &key)
{
   auto *table=owner->findChild<QTableWidget *>("resourceProperties");if (!table) return nullptr;
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==key) return table->item(row,1);
   return nullptr;
}
static QString field(QWidget *owner,const QString &key)
{
   if (auto *c=owner->findChild<QComboBox *>("resource_"+key)) return c->currentText();
   if (auto *e=owner->findChild<QLineEdit *>("resource_"+key)) return e->text();
   if (auto *v=cell(owner,key)) return v->text();throw std::runtime_error(("Missing object-axis field "+key).toStdString());
}
static void setField(QWidget *owner,const QString &key,const QString &value)
{
   if (auto *c=owner->findChild<QComboBox *>("resource_"+key)) {require(c->isEnabled() && c->findText(value)>=0,"Object-axis choice absent or disabled");c->setCurrentText(value);}
   else if (auto *e=owner->findChild<QLineEdit *>("resource_"+key)) {require(e->isEnabled() && !e->isReadOnly(),"Object-axis field not editable");e->setText(value);}
   else if (auto *v=cell(owner,key)) {require(v->flags() & Qt::ItemIsEditable,"Object-axis table field not editable");v->setText(value);}
   else throw std::runtime_error(("Missing object-axis control "+key).toStdString());
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaTopoRuntime");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and Topocentric runtime case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Topocentric runtime directory missing");const QString mode=argv[2];const bool polar=mode.startsWith("North") || mode.startsWith("South");require(QStringList{"HorizonSphere","HorizonEllipsoid","Flattening","Radius","NorthSphere","SouthSphere","NorthEllipsoid","SouthEllipsoid"}.contains(mode),"Unknown Topocentric runtime case");const auto base=configuration(mode,0);
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto kernel=files.filePath("Runtime station body Δ.bsp"),report=files.filePath("Runtime Topocentric Δ.txt");SpiceInt handle;SpiceDouble times[]{-86400,0,172800},states[3][6]{};spkopn_c(kernel.toUtf8().constData(),"Known runtime station origin",0,&handle);spkw09_c(handle,2000001,399,"J2000",-86400,172800,"ZERO EARTH RELATIVE",1,3,states,times);spkcls_c(handle);require(!failed_c(),"Cannot create runtime station ephemeris");
      QString initial="% Retain Topocentric runtime acceptance Δ\nCreate Asteroid Probe;\nProbe.NAIFId = 2000001;\nProbe.OrbitSpiceKernelName = {'"+kernel+"'};\nProbe.Mu = 63;\nProbe.EquatorialRadius = 6400;\nProbe.Flattening = 0.01;\nProbe.OrientationEpoch = 21545;\nProbe.SpinAxisRAConstant = 270;\nProbe.SpinAxisRARate = 0;\nProbe.SpinAxisDECConstant = 90;\nProbe.SpinAxisDECRate = 0;\nProbe.RotationConstant = 30;\nProbe.RotationRate = "+QString::number(base.spin,'g',17)+";\nCreate GroundStation Site;\nSite.CentralBody = Probe;\nSite.StateType = Cartesian;\nSite.HorizonReference = "+base.horizon+";\n";
      for (int i=0;i<3;++i) initial+="Site.Location"+QString::number(i+1)+" = "+QString::number(base.fixed[i],'g',17)+"; % fixed station input\n";
      initial+="Create CoordinateSystem Baseline;\nBaseline.Origin = Earth;\nBaseline.Axes = MJ2000Eq;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\n";for (int i=0;i<6;++i) initial+="Sat."+components[i]+" = "+QString::number(input[i],'g',17)+";\n";
      initial+="Create ReportFile States;\nStates.Filename = '"+report+"';\nStates.WriteHeaders = false;\nStates.Precision = 16;\nCreate Variable Keep;\nKeep = 41; % preserve initializer\nBeginMissionSequence;\n";
      for (double epoch:{21545.,21545.5,21546.25}) {
         initial+="Sat.A1ModJulian = "+QString::number(epoch,'g',17)+";\n";for (int i=0;i<6;++i) initial+="Sat.EarthMJ2000Eq."+components[i]+" = "+QString::number(input[i],'g',17)+";\n";
         initial+="Report States Sat.A1ModJulian";for (const auto &frame:QStringList{"EarthMJ2000Eq","Baseline"}) for (const auto &component:components) initial+=" Sat."+frame+"."+component;initial+=";\n";
      }initial+="Keep = Keep + 1; % preserve mission\n";
      MainWindow window;require(window.initialize(startup),"Topocentric runtime initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");require(source && tree && area && messages,"Topocentric runtime workspace missing");source->setPlainText(initial);require(window.buildScript(),"Topocentric runtime baseline failed");const auto initialUndo=source->document()->availableUndoSteps();int cancels=0,pickerCancels=0,runtimeFailures=0,forbiddenBuilds=0,configurationApplies=0,phase=0;double maxp=0,maxv=0;QJsonArray samples,failures;
      auto configured=[&] {return Moderator::Instance()->GetConfiguredObject("CreatedFrame");};auto unchanged=[&] {require(!configured() && source->toPlainText()==initial && source->document()->availableUndoSteps()==initialUndo,"Runtime coordinate draft changed registration/source/history");};
      auto view=[&](QMdiSubWindow *child) {if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);child->showNormal();events();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);child->raise();events();require(area->activeSubWindow()==child,"Runtime view is obscured");}acceptanceView(*child);};
      auto pick=[&](ResourceEditor *owner) {auto *line=owner->findChild<QLineEdit *>("resource_Origin");require(line && line->isVisible(),"Runtime Origin field not visible");QPushButton *button=nullptr;for (auto *p:line->parentWidget()->findChildren<QPushButton *>(QString(),Qt::FindDirectChildrenOnly)) if (p->text()=="Select…") button=p;require(button && button->isVisible(),"Runtime Origin Select action missing");const auto before=line->text();for (bool accept:{false,true}) {acceptanceView(*owner->window());std::exception_ptr error;QTimer::singleShot(0,owner,[&] {auto *dialog=owner->findChild<QInputDialog *>();try {require(dialog && dialog->comboBoxItems().contains("Site") && !dialog->comboBoxItems().contains("States"),"Runtime Origin choices incorrect");dialog->setTextValue("Site");acceptanceView(*dialog);if (accept) dialog->accept();else {dialog->reject();++pickerCancels;}}catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});button->click();if (error) std::rethrow_exception(error);require(line->text()==(accept ? "Site":before),"Runtime Origin Cancel/accept changed wrong value");}};
      auto axes=[&](ResourceEditor *parent,bool accept) {auto *button=parent->findChild<QPushButton *>("editCoordinateAxes");auto *tabs=parent->findChild<QTabWidget *>("resourceEditorTabs");require(button && tabs,"Runtime Axes action missing");for (int i=0;i<tabs->count();++i) if (tabs->widget(i)->isAncestorOf(button)) tabs->setCurrentIndex(i);events();acceptanceView(*parent->window());require(button->isVisible(),"Runtime Axes action not reachable");std::exception_ptr error;QTimer::singleShot(0,parent,[&] {auto *dialog=parent->findChild<QDialog *>("coordinateAxesDialog");try {require(dialog,"Runtime axes dialog missing");auto *choice=dialog->findChild<QComboBox *>("coordinateAxisType");require(choice && choice->findText("Topocentric")>=0,"Topocentric choice missing");choice->setCurrentText("Topocentric");events();acceptanceView(*dialog);if (accept) {dialog->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Apply)->click();events();require(dialog->result()==QDialog::Accepted,"Runtime axes OK failed");}else {dialog->reject();++cancels;}}catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});button->click();if (error) std::rethrow_exception(error);};
      auto create=[&](bool commit) {std::exception_ptr error;QTimer::singleShot(0,&window,[&] {auto *dialog=window.findChild<QDialog *>("newResourceDialog");try {require(dialog,"Runtime coordinate creator missing");dialog->findChild<QComboBox *>("resourceType")->setCurrentText("CoordinateSystem");dialog->findChild<QLineEdit *>("resourceName")->setText("CreatedFrame");ResourceEditor *panel=nullptr;for (auto *w:dialog->findChildren<QWidget *>()) if (auto *p=dynamic_cast<ResourceEditor *>(w);p && p->isVisible()) panel=p;require(panel,"Runtime coordinate creator form missing");pick(panel);axes(panel,false);unchanged();axes(panel,true);unchanged();if (!commit) {dialog->reject();++cancels;return;}acceptanceView(*dialog);dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok)->click();events();require(dialog->result()==QDialog::Accepted,"Runtime coordinate Create failed");}catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});window.findChild<QAction *>("createResource")->trigger();if (error) std::rethrow_exception(error);};
      create(false);unchanged();create(true);require(configured() && configured()->GetStringParameter("Origin")=="Site" && configured()->GetStringParameter("Axes")=="Topocentric","Runtime coordinate definition differs");const auto created=source->toPlainText();QString stripped=created;stripped.remove(QRegularExpression("^(?:Create CoordinateSystem CreatedFrame;|(?:GMAT[ \\t]+)?CreatedFrame\\.[^;\\n]*;)[ \\t]*\\n",QRegularExpression::MultilineOption));require(stripped==initial,"Runtime Create rewrote unrelated source");for (auto *w:area->subWindowList()) if (w->property("resourceName").toString()=="CreatedFrame") {require(w->close(),"Clean runtime creator did not close");events();break;}source->undo();require(source->toPlainText()==initial && window.buildScript() && !configured(),"Runtime Create Undo changed exact source/model");source->redo();require(source->toPlainText()==created && window.buildScript() && configured(),"Runtime Create Redo did not restore source/model");
      QString bound=created;bound.replace("Sat.Baseline.","Sat.CreatedFrame.");{QTextCursor group(source->document());group.beginEditBlock();QTextCursor cursor(source->document());int count=0;while (!(cursor=source->document()->find("Sat.Baseline.",cursor)).isNull()) {cursor.insertText("Sat.CreatedFrame.");++count;}group.endEditBlock();require(count==18 && source->toPlainText()==bound,"Runtime report binding changed unrelated source");}require(window.buildScript(),"Runtime frame/report build failed");const auto path=files.filePath("Runtime Topocentric Δ.script");require(window.saveScriptTo(path) && window.loadScript(path) && window.buildScript() && source->toPlainText()==bound,"Runtime Unicode save/reopen changed source/model");
      const auto destination=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes) {if (destination.isEmpty()) return;QFile file(QDir(destination).filePath(name));require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Cannot retain runtime evidence");};{QFile f(kernel);require(f.open(QIODevice::ReadOnly),"Cannot retain runtime ephemeris");save("station-origin.bsp",f.readAll());}
      auto observe=[&](const Configuration &c,bool running) {
         auto *site=dynamic_cast<BodyFixedPoint *>(running ? Moderator::Instance()->GetInternalObject("Site") : Moderator::Instance()->GetConfiguredObject("Site"));require(site,"Configured/running station missing");
         require(site->GetStringParameter("StateType")=="Cartesian" && site->GetStringParameter("HorizonReference")==c.horizon.toStdString(),"Configured station shape/type differs");
         const auto fixed=site->GetBodyFixedLocation(A1Mjd(21545));for (int i=0;i<3;++i) require(std::abs(fixed[i]-c.fixed[i])<=1e-12,"Configuration change moved the fixed Cartesian station");
         auto *body=dynamic_cast<CelestialBody *>(site->GetRefObject(Gmat::CELESTIAL_BODY,"Probe"));require(body && std::abs(body->GetFlattening()-c.flattening)<=1e-14 && std::abs(body->GetEquatorialRadius()-c.radius)<=1e-10,"Station central-body configuration differs");
      };
      std::array<State,3> originalStates{};
      auto run=[&](const Configuration &c,const QString &label) {messages->clear();const auto result=window.runMission();events();save(QString::number(phase)+"-"+label+".script",source->toPlainText().toUtf8());save(QString::number(phase)+"-"+label+"-messages.txt",messages->toPlainText().toUtf8());if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Topocentric configuration mission failed");observe(c,true);QFile file(report);require(file.open(QIODevice::ReadOnly),"Configuration report missing");const auto raw=file.readAll();save(QString::number(phase)+"-"+label+"-states.txt",raw);const auto words=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==39,"Configuration report needs three epochs with input and transformed six-states");
         for (int row=0;row<3;++row) {bool ok=false;const double epoch=words[row*13].toDouble(&ok);require(ok && std::abs(epoch-std::array<double,3>{21545,21545.5,21546.25}[row])<=1e-10,"Configuration report epoch differs");const auto m=model(c,epoch);V p{},v{};State expected{},actual{};for (int i=0;i<3;++i) {p[i]=input[i]-m.origin[i];v[i]=input[i+3]-m.origin[i+3];}for (int i=0;i<3;++i) {expected[i]=dot(m.rows[i],p);expected[i+3]=dot(m.rows[i],v)+dot(m.rates[i],p);}for (int i=0;i<6;++i) {const double in=words[row*13+1+i].toDouble(&ok);require(ok && std::abs(in-input[i])<=1e-8,"Configuration fixture inertial input changed");actual[i]=words[row*13+7+i].toDouble(&ok);require(ok && std::isfinite(actual[i]),"Configuration result nonfinite");const double error=std::abs(actual[i]-expected[i]);if (i<3) maxp=std::max(maxp,error);else maxv=std::max(maxv,error);if (error>(i<3 ? 2e-7:2e-9)) std::cerr<<"TOPO_CONFIG_DIFF "<<mode.toStdString()<<" phase="<<phase<<" row="<<row<<" component="<<i<<" actual="<<actual[i]<<" expected="<<expected[i]<<" error="<<error<<std::endl;require(error<=(i<3 ? 2e-7:2e-9),"Topocentric result differs from independent current-horizon/shape oracle");}
            if (phase==0) originalStates[row]=actual;else if (label.endsWith("restored") || polar) {for (int i=0;i<6;++i) require(std::abs(originalStates[row][i]-actual[i])<=(i<3 ? 2e-7:2e-9),"Restored configuration did not return original state");}else require(std::abs(originalStates[row][0]-actual[0])>.001,"Configuration change did not exercise a changed horizon normal");
            samples.append(QJsonObject{{"phase",phase},{"label",label},{"epoch_a1mjd",epoch},{"horizon",c.horizon},{"flattening",c.flattening},{"radius_km",c.radius},{"fixed_station",QJsonArray{c.fixed[0],c.fixed[1],c.fixed[2]}},{"latitude_rad",m.latitude},{"longitude_rad",m.longitude},{"rotation_angle_rad",m.angle},{"spin_rate_rad_s",m.rate},{"origin_offset",json(m.origin)},{"basis_rows",json(m.rows)},{"basis_rates",json(m.rates)},{"actual",json(actual)},{"expected",json(expected)}});
         }require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Configuration mission did not continue");auto *output=window.findChild<QTreeWidget *>("Output");const auto items=output->findItems("States",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Configuration output tree missing");output->itemDoubleClicked(items.front(),0);events();QMdiSubWindow *viewer=nullptr;for (auto *w:area->subWindowList()) if (dynamic_cast<ReportViewer *>(w->widget())) viewer=w;require(viewer,"Configuration report viewer missing");view(viewer);auto *shown=viewer->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Configuration viewer differs from checked report");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*viewer);require(viewer->close(),"Configuration report did not close");events();++phase;
      };
      run(base,"initial");
      if (!polar) {
         const auto normal=source->toPlainText();const QString assignment=mode.startsWith("Horizon") ? "Site.HorizonReference = '"+configuration(mode,1).horizon+"';" : mode=="Flattening" ? "Probe.Flattening = 0.04;" : "Probe.EquatorialRadius = 7200;";
         {QTextCursor group(source->document());group.beginEditBlock();auto cursor=source->document()->find("BeginMissionSequence;");require(!cursor.isNull(),"Mission boundary missing");cursor.movePosition(QTextCursor::EndOfBlock);cursor.insertText("\n"+assignment);group.endEditBlock();}
         messages->clear();require(!window.buildScript(),"Forbidden mission configuration assignment accepted");events();save("forbidden-command.script",source->toPlainText().toUtf8());save("forbidden-command-messages.txt",messages->toPlainText().toUtf8());require(messages->toPlainText().contains("cannot be set after the Mission Sequence has started",Qt::CaseInsensitive),"Forbidden mission assignment lacks expected diagnostic");++forbiddenBuilds;messages->verticalScrollBar()->setValue(messages->verticalScrollBar()->maximum());acceptanceView(window);source->undo();require(source->toPlainText()==normal && window.buildScript(),"Forbidden command recovery changed source/model");
         auto reveal=[&](QWidget *control) {require(control,"Configuration control missing");for (auto *page=control;page;page=page->parentWidget()) {for (auto *tabs:control->window()->findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);if (auto *scroll=qobject_cast<QScrollArea *>(page)) scroll->ensureWidgetVisible(control);}events();require(control->isVisible() && control->isEnabled(),"Configuration control not reachable");};
         for (int stage:{1,2}) {const auto c=configuration(mode,stage),previous=configuration(mode,stage==1 ? 0:1);const auto before=source->toPlainText();const int undo=source->document()->availableUndoSteps();const QString name=mode.startsWith("Horizon") ? "Site":"Probe";const auto items=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Configuration resource tree entry missing");tree->itemDoubleClicked(items.front(),0);events();QMdiSubWindow *child=nullptr;for (auto *w:area->subWindowList()) if (w->property("resourceName").toString()==name) child=w;require(child,"Configuration resource panel missing");view(child);
            if (name=="Site") {auto *panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel,"Actual station resource panel missing");auto *form=panel->findChild<QWidget *>("groundStationDialog");require(form,"Actual station coordinate form missing");auto select=[&](const QString &key,const QString &value) {auto *control=form->findChild<QComboBox *>("station_"+key);reveal(control);require(control->findText(value)>=0,"Station configuration choice missing");control->setCurrentText(value);events();};auto *horizon=form->findChild<QComboBox *>("station_HorizonReference");require(horizon && !horizon->isEnabled(),"Cartesian station should disable horizon selector");select("StateType","Spherical");select("HorizonReference",c.horizon);select("StateType","Cartesian");for (int i=0;i<3;++i) {auto *control=form->findChild<QLineEdit *>("station_Location"+QString::number(i+1));reveal(control);require(std::abs(control->text().toDouble()-c.fixed[i])<=1e-8,"Station preview changed Cartesian position");control->setText(QString::number(c.fixed[i],'g',17));}acceptanceView(*child);require(source->toPlainText()==before && source->document()->availableUndoSteps()==undo,"Station pending configuration altered source/history");observe(previous,false);panel->requestApply();events();panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel && !panel->hasChanges(),"Station configuration Apply failed");}
            else {auto *panel=dynamic_cast<CelestialBodyPanel *>(child->widget());require(panel,"Actual celestial-body panel missing");auto *control=panel->findChild<QLineEdit *>(mode=="Flattening" ? "body_Flattening":"body_EquatorialRadius");reveal(control);require(!control->isReadOnly(),"Body shape field unexpectedly read-only");control->setText(QString::number(mode=="Flattening" ? c.flattening:c.radius,'g',17));acceptanceView(*child);require(source->toPlainText()==before && source->document()->availableUndoSteps()==undo,"Body pending configuration altered source/history");observe(previous,false);panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();panel=dynamic_cast<CelestialBodyPanel *>(child->widget());if (panel && panel->hasChanges()) std::cerr<<panel->findChild<QLabel *>("bodyStatus")->text().toStdString();require(panel && !panel->hasChanges(),"Body configuration Apply failed");}
            ++configurationApplies;observe(c,false);acceptanceView(*child);const auto applied=source->toPlainText();require(applied!=before && applied.mid(applied.indexOf("BeginMissionSequence;"))==before.mid(before.indexOf("BeginMissionSequence;")),"Configuration Apply changed mission or failed to update source");require(child->close(),"Clean configuration panel did not close");events();source->undo();require(source->toPlainText()==before && window.buildScript(),"Configuration Undo changed exact source/model");observe(previous,false);source->redo();require(source->toPlainText()==applied && window.buildScript(),"Configuration Redo changed exact source/model");observe(c,false);require(window.saveScriptTo(path) && window.loadScript(path) && window.buildScript() && source->toPlainText()==applied,"Configuration Unicode save/reopen changed source/model");run(c,stage==1 ? "gui-changed":"gui-restored");
         }
      }
      if (polar) {const auto normal=source->toPlainText();for (double x:{0.,.0005}) {{QTextCursor group(source->document());group.beginEditBlock();const QString needle="Site.Location1 = "+QString::number(base.fixed[0],'g',17)+"; % fixed station input";auto cursor=source->document()->find(needle);require(!cursor.isNull(),"Polar boundary input missing");cursor.insertText("Site.Location1 = "+QString::number(x,'g',17)+"; % fixed station input");group.endEditBlock();}require(window.buildScript(),"Polar boundary configuration failed before runtime");messages->clear();const auto result=window.runMission();events();save("polar-"+QString::number(runtimeFailures)+".script",source->toPlainText().toUtf8());save("polar-"+QString::number(runtimeFailures)+"-messages.txt",messages->toPlainText().toUtf8());require(result==MainWindow::RunResult::Failed && messages->toPlainText().contains("singularity at the poles",Qt::CaseInsensitive),"Polar singularity was not rejected with its diagnostic");++runtimeFailures;failures.append(x);messages->verticalScrollBar()->setValue(messages->verticalScrollBar()->maximum());acceptanceView(window);source->undo();require(source->toPlainText()==normal && window.buildScript(),"Polar recovery did not restore exact source/model");}run(base,"polar-recovered");}
      const QJsonObject proof{{"checkpoint","topo-runtime-"+mode},{"outcome","passed"},{"mode",mode},{"cancels",cancels},{"picker_cancels",pickerCancels},{"runtime_failures",runtimeFailures},{"forbidden_builds",forbiddenBuilds},{"configuration_applies",configurationApplies},{"rejected_rxy_km",failures},{"samples",samples},{"max_position_error_km",maxp},{"max_velocity_error_km_s",maxv},{"facets",QJsonArray{"actual-coordinate-origin-create","axes-picker-and-cancel","exact-create-history","unicode-persistence","gui-configuration-and-mission-restriction","fixed-cartesian-invariance","independent-configuration-six-state","restore-or-polar-recovery","actual-report-viewer"}},{"scope","Permitted GUI horizon/flattening/radius configuration changes across runs at identical Cartesian station positions, forbidden mission-command diagnostics, and bounded near-pole/singularity recovery. Independent bracketed geodetic normal and synthetic known constant body rotation; shared numerical TDB conversion is input, not measured Earth orientation or clock qualification. Native API/plugin cache mutations, broader dynamics/origin/input, exact guard-boundary rounding, native OS input/platform/stress/current NASA corpus and full regression remain open."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
