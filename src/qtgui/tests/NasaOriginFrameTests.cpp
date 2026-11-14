#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
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
static V add(V a,const V &b,double factor=1) {for (int i=0;i<3;++i) a[i]+=factor*b[i];return a;}
static V scale(V a,double s) {for (auto &x:a) x*=s;return a;}
static double dot(const V &a,const V &b) {double r=0;for (int i=0;i<3;++i) r+=a[i]*b[i];return r;}
static V cross(const V &a,const V &b) {return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
static V unit(const V &a) {const double n=std::sqrt(dot(a,a));require(n>0,"Independent origin vector is degenerate");return scale(a,1/n);}
static QJsonArray json(const State &v) {QJsonArray a;for (auto x:v) a.append(x);return a;}
static QJsonArray json(const Matrix &v) {QJsonArray a;for (auto x:v) a.append(QJsonArray{x[0],x[1],x[2]});return a;}
static double equilibrium(double x,double mu) {return x-mu-(1-mu)*x/std::pow(std::abs(x),3)-mu*(x-1)/std::pow(std::abs(x-1),3);}
static std::array<double,2> coordinates(const QString &point,double mu)
{
   if (point=="L4" || point=="L5") return {.5,(point=="L4" ? 1:-1)*std::sqrt(3.)/2};
   double lo=point=="L3" ? -4:point=="L2" ? 1+1e-12:1e-12,hi=point=="L3" ? -1e-12:point=="L2" ? 4:1-1e-12;
   require(equilibrium(lo,mu)<0 && equilibrium(hi,mu)>0,"Independent origin equilibrium bracket invalid");
   for (int i=0;i<160;++i) {const double mid=(lo+hi)/2;if (mid==lo || mid==hi) break;if (equilibrium(mid,mu)>0) hi=mid;else lo=mid;}
   const double x=(lo+hi)/2;require(std::abs(equilibrium(x,mu))<1e-9,"Independent origin equilibrium root failed");return {x,0};
}
static V offset(const V &r,const V &v,const std::array<double,2> &xy)
{
   const auto transverse=cross(unit(cross(r,v)),unit(r));return add(scale(r,xy[0]),scale(transverse,std::sqrt(dot(r,r))*xy[1]));
}
struct Model {State origin{};Matrix rows{{{1,0,0},{0,1,0},{0,0,1}}},rates{};QJsonObject inputs;};
static Model model(const QString &mode,const QString &axes,double epoch)
{
   Model result;
   if (mode.startsWith('L')) {
      auto *solar=Moderator::Instance()->GetSolarSystemInUse();auto *earth=solar->GetBody("Earth"),*moon=solar->GetBody("Luna");const auto p=earth->GetMJ2000State(A1Mjd(epoch)),s=moon->GetMJ2000State(A1Mjd(epoch));
      const double gmE=earth->GetGravitationalConstant(),gmM=moon->GetGravitationalConstant(),mu=gmM/(gmE+gmM);const auto xy=coordinates(mode,mu);State primary{},secondary{};V r{},v{};
      for (int i=0;i<6;++i) {primary[i]=p[i];secondary[i]=s[i];}for (int i=0;i<3;++i) {r[i]=s[i]-p[i];v[i]=s[i+3]-p[i+3];}
      const auto position=offset(r,v,xy);constexpr double h=16;const auto m2=offset(add(r,v,-2*h),v,xy),m1=offset(add(r,v,-h),v,xy),p1=offset(add(r,v,h),v,xy),p2=offset(add(r,v,2*h),v,xy);
      for (int i=0;i<3;++i) {result.origin[i]=position[i];result.origin[i+3]=(m2[i]-8*m1[i]+8*p1[i]-p2[i])/(12*h);}
      result.inputs={{"earth_state",json(primary)},{"luna_state",json(secondary)},{"earth_mu",gmE},{"luna_mu",gmM},{"point_xy",QJsonArray{xy[0],xy[1]}}};
   } else {
      const bool ellipse=mode.startsWith("Ellipsoid"),spin=mode.endsWith("Spin") || mode=="TopoOrigins";const double lat=31.25*pi/180,lon=-81.5*pi/180,height=1.2,a=6400,f=ellipse ? .01:0,e2=f*(2-f),N=a/std::sqrt(1-e2*std::sin(lat)*std::sin(lat));
      const V fixed{(N+height)*std::cos(lat)*std::cos(lon),(N+height)*std::cos(lat)*std::sin(lon),(N*(1-e2)+height)*std::sin(lat)};
      const auto tdb=[](double t) {return TimeSystemConverter::Instance()->Convert(t,TimeSystemConverter::A1MJD,TimeSystemConverter::TDBMJD,2430000.0);};const double rate=spin ? 120*pi/(180*86400):0,angle=30*pi/180+(spin ? 120*pi/180*(tdb(epoch)-tdb(21545)):0);
      auto rotate=[&](V p) {return V{std::cos(angle)*p[0]-std::sin(angle)*p[1],std::sin(angle)*p[0]+std::cos(angle)*p[1],p[2]};};const auto position=rotate(fixed),velocity=scale(cross(V{0,0,1},position),rate);
      for (int i=0;i<3;++i) {result.origin[i]=position[i];result.origin[i+3]=velocity[i];}
      if (axes=="Topocentric") {
         const Matrix fixedRows{{{std::sin(lat)*std::cos(lon),std::sin(lat)*std::sin(lon),-std::cos(lat)},{-std::sin(lon),std::cos(lon),0},{std::cos(lat)*std::cos(lon),std::cos(lat)*std::sin(lon),std::sin(lat)}}};
         for (int i=0;i<3;++i) {result.rows[i]=rotate(fixedRows[i]);result.rates[i]=scale(cross(V{0,0,1},result.rows[i]),rate);}
      }
      result.inputs={{"fixed_station",QJsonArray{fixed[0],fixed[1],fixed[2]}},{"latitude_deg",31.25},{"longitude_deg",-81.5},{"height_km",height},{"body_radius_km",a},{"body_flattening",.01},{"horizon_reference",ellipse ? "Ellipsoid":"Sphere"},{"rotation_angle_rad",angle},{"spin_rate_rad_s",rate}};
   }
   if (axes=="MJ2000Ec") {const double e=84381.448*pi/(180*3600);result.rows={{{1,0,0},{0,std::cos(e),std::sin(e)},{0,-std::sin(e),std::cos(e)}}};}
   return result;
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
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaOriginFrames");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and origin-frame case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Origin-frame fixture directory missing");const QString mode=argv[2];const bool lunar=mode.startsWith('L'),bounds=mode=="TopoOrigins";
      require(QStringList{"L1","L2","L3","L4","L5","SphereStatic","SphereSpin","EllipsoidStatic","EllipsoidSpin","TopoOrigins"}.contains(mode),"Unknown origin-frame case");const QString origin=lunar ? "LP":"Site",firstAxes=bounds ? "Topocentric":"MJ2000Eq",secondAxes=lunar ? "MJ2000Ec":bounds ? "MJ2000Eq":"Topocentric";
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto kernel=files.filePath("Earth-coincident station body Δ.bsp"),report=files.filePath("Origin frame Δ.txt");
      SpiceInt handle;SpiceDouble times[]{-86400,0,172800},states[3][6]{};spkopn_c(kernel.toUtf8().constData(),"Synthetic station body origin",0,&handle);spkw09_c(handle,2000001,399,"J2000",-86400,172800,"ZERO EARTH RELATIVE STATION BODY",1,3,states,times);spkcls_c(handle);require(!failed_c(),"Cannot write synthetic station-body ephemeris");
      const bool spin=mode.endsWith("Spin") || bounds;QString initial="% Preserve independent origin frame acceptance Δ\nEarth.Mu = 400000;\nLuna.Mu = 5000;\nCreate Asteroid Probe;\nProbe.NAIFId = 2000001;\nProbe.OrbitSpiceKernelName = {'"+kernel+"'};\nProbe.Mu = 63;\nProbe.EquatorialRadius = 6400;\nProbe.Flattening = 0.01;\nProbe.OrientationEpoch = 21545;\nProbe.SpinAxisRAConstant = 270;\nProbe.SpinAxisRARate = 0;\nProbe.SpinAxisDECConstant = 90;\nProbe.SpinAxisDECRate = 0;\nProbe.RotationConstant = 30;\nProbe.RotationRate = "+QString(spin ? "120":"0")+";\nCreate Barycenter Center;\nCenter.BodyNames = {Earth, Luna};\nCreate LibrationPoint LP;\nLP.Primary = Earth;\nLP.Secondary = Luna;\nLP.Point = "+QString(lunar ? mode:"L1")+";\nCreate GroundStation Site;\nSite.CentralBody = Probe;\nSite.StateType = Spherical;\nSite.HorizonReference = "+QString(mode.startsWith("Ellipsoid") ? "Ellipsoid":"Sphere")+";\nSite.Location1 = 31.25;\nSite.Location2 = -81.5;\nSite.Location3 = 1.2;\nCreate CoordinateSystem Baseline;\nBaseline.Origin = Earth;\nBaseline.Axes = MJ2000Eq;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\n";
      for (int i=0;i<6;++i) initial+="Sat."+components[i]+" = "+QString::number(input[i],'g',17)+";\n";
      initial+="Create ReportFile States;\nStates.Filename = '"+report+"';\nStates.WriteHeaders = false;\nStates.Precision = 16;\nCreate Variable Keep;\nKeep = 41; % preserve initializer\nBeginMissionSequence;\n";
      for (double epoch:{21545.,21545.5,21546.25}) {initial+="Sat.A1ModJulian = "+QString::number(epoch,'g',17)+";\n";for (int i=0;i<6;++i) initial+="Sat.EarthMJ2000Eq."+components[i]+" = "+QString::number(input[i],'g',17)+";\n";initial+="Report States Sat.A1ModJulian";for (const auto &frame:QStringList{"EarthMJ2000Eq","Baseline"}) for (const auto &c:components) initial+=" Sat."+frame+"."+c;initial+=";\n";}initial+="Keep = Keep + 1; % preserve mission\n";
      MainWindow window;require(window.initialize(startup),"Origin-frame runtime initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");require(source && tree && area && messages,"Origin-frame workspace missing");source->setPlainText(initial);require(window.buildScript(),"Origin-frame baseline did not build");const auto initialUndo=source->document()->availableUndoSteps();int cancels=0,pickerCancels=0,invalid=0,phase=0;double maxp=0,maxv=0;QJsonArray samples;
      auto configured=[&] {return Moderator::Instance()->GetConfiguredObject("CreatedFrame");};auto unchanged=[&] {require(!configured() && source->toPlainText()==initial && source->document()->availableUndoSteps()==initialUndo,"Origin draft changed registration/source/history");};
      auto view=[&](QMdiSubWindow *child) {if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);child->showNormal();events();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);child->raise();events();require(area->activeSubWindow()==child,"Origin-frame view is obscured");}acceptanceView(*child);};
      auto pick=[&](ResourceEditor *owner,const QString &name) {
         auto *line=owner->findChild<QLineEdit *>("resource_Origin");require(line && line->isVisible(),"Actual Origin field not visible");QPushButton *button=nullptr;for (auto *p:line->parentWidget()->findChildren<QPushButton *>(QString(),Qt::FindDirectChildrenOnly)) if (p->text()=="Select…") button=p;require(button && button->isVisible(),"Actual Origin Select action missing");const auto before=line->text(),sourceBefore=source->toPlainText();const auto undo=source->document()->availableUndoSteps();
         for (bool accept:{false,true}) {acceptanceView(*owner->window());std::exception_ptr error;QTimer::singleShot(0,owner,[&] {auto *dialog=owner->findChild<QInputDialog *>();try {require(dialog && dialog->comboBoxItems().contains(name) && dialog->comboBoxItems().contains("LP") && dialog->comboBoxItems().contains("Site") && !dialog->comboBoxItems().contains("States"),"Origin picker omits a valid origin or includes a subscriber");dialog->setTextValue(name);acceptanceView(*dialog);if (accept) dialog->accept();else {dialog->reject();++pickerCancels;}}catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});button->click();if (error) std::rethrow_exception(error);require(line->text()==(accept ? name:before),"Origin picker Cancel/accept changed wrong field");require(source->toPlainText()==sourceBefore && source->document()->availableUndoSteps()==undo,"Origin picker committed before parent Create/Apply");}
      };
      auto axes=[&](ResourceEditor *parent,const QString &type,bool accept) {
         auto *button=parent->findChild<QPushButton *>("editCoordinateAxes");auto *tabs=parent->findChild<QTabWidget *>("resourceEditorTabs");require(button && tabs,"Origin-frame Axes action missing");for (int i=0;i<tabs->count();++i) if (tabs->widget(i)->isAncestorOf(button)) tabs->setCurrentIndex(i);events();acceptanceView(*parent->window());require(button->isVisible(),"Origin-frame Axes action is not reachable");std::exception_ptr error;
         QTimer::singleShot(0,parent,[&] {auto *dialog=parent->findChild<QDialog *>("coordinateAxesDialog");try {require(dialog,"Actual origin-frame axes dialog missing");auto *choice=dialog->findChild<QComboBox *>("coordinateAxisType");require(choice && choice->findText(type)>=0,"Required origin-frame axes type absent");choice->setCurrentText(type);events();acceptanceView(*dialog);if (accept) {dialog->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Apply)->click();events();require(dialog->result()==QDialog::Accepted,"Origin-frame nested axes OK failed");}else {dialog->reject();++cancels;}}catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});button->click();if (error) std::rethrow_exception(error);
      };
      auto create=[&](bool commit) {
         std::exception_ptr error;QTimer::singleShot(0,&window,[&] {auto *dialog=window.findChild<QDialog *>("newResourceDialog");try {require(dialog,"Actual origin-frame creator missing");dialog->findChild<QComboBox *>("resourceType")->setCurrentText("CoordinateSystem");dialog->findChild<QLineEdit *>("resourceName")->setText("CreatedFrame");ResourceEditor *panel=nullptr;for (auto *w:dialog->findChildren<QWidget *>()) if (auto *p=dynamic_cast<ResourceEditor *>(w);p && p->isVisible()) panel=p;require(panel,"Actual origin-frame creation panel missing");pick(panel,origin);axes(panel,firstAxes,false);unchanged();axes(panel,firstAxes,true);unchanged();auto *ok=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok);
            if (bounds && commit) for (const auto &bad:QStringList{"Earth","LP","Center","Sat"}) {auto *tabs=panel->findChild<QTabWidget *>("resourceEditorTabs");tabs->setCurrentIndex(0);pick(panel,bad);ok->click();events();require(dialog->isVisible() && dialog->result()!=QDialog::Accepted && !dialog->findChild<QLabel *>("resourceCreationStatus")->text().isEmpty(),"Non-station Topocentric origin accepted at Create or lacks diagnostic");unchanged();++invalid;acceptanceView(*dialog);}if (bounds && commit) {panel->findChild<QTabWidget *>("resourceEditorTabs")->setCurrentIndex(0);pick(panel,origin);}
            if (!commit) {dialog->reject();++cancels;return;}acceptanceView(*dialog);ok->click();events();if (dialog->result()!=QDialog::Accepted) std::cerr<<dialog->findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<std::endl;require(dialog->result()==QDialog::Accepted,"Valid origin-frame Create failed");}catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});window.findChild<QAction *>("createResource")->trigger();if (error) std::rethrow_exception(error);
      };
      create(false);unchanged();create(true);require(configured() && configured()->GetStringParameter("Origin")==origin.toStdString(),"Created coordinate origin differs");QString stripped=source->toPlainText();stripped.remove(QRegularExpression("^(?:Create CoordinateSystem CreatedFrame;|(?:GMAT[ \\t]+)?CreatedFrame\\.[^;\\n]*;)[ \\t]*\\n",QRegularExpression::MultilineOption));require(stripped==initial,"Origin Create changed unrelated source");for (auto *w:area->subWindowList()) if (w->property("resourceName").toString()=="CreatedFrame") {require(w->close(),"Clean origin creator did not close");events();break;}
      const auto created=source->toPlainText();QString bound=created;bound.replace("Sat.Baseline.","Sat.CreatedFrame.");{QTextCursor group(source->document());group.beginEditBlock();QTextCursor cursor(source->document());int count=0;while (!(cursor=source->document()->find("Sat.Baseline.",cursor)).isNull()) {cursor.insertText("Sat.CreatedFrame.");++count;}group.endEditBlock();require(count==18 && source->toPlainText()==bound,"Origin report binding changed unrelated text");}require(window.buildScript(),"GUI-created origin frame did not build");
      const auto destination=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes) {if (destination.isEmpty()) return;QFile file(QDir(destination).filePath(name));require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Cannot retain origin-frame evidence");};{QFile f(kernel);require(f.open(QIODevice::ReadOnly),"Cannot retain station-body ephemeris");save("station-origin.bsp",f.readAll());}
      auto run=[&](const QString &type,const QString &label) {
         messages->clear();const auto result=window.runMission();events();save(QString::number(phase)+"-"+label+".script",source->toPlainText().toUtf8());save(QString::number(phase)+"-"+label+"-messages.txt",messages->toPlainText().toUtf8());if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Origin-frame report mission failed");QFile file(report);require(file.open(QIODevice::ReadOnly),"Origin-frame report missing");const auto raw=file.readAll();save(QString::number(phase)+"-"+label+"-states.txt",raw);const auto words=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==39,"Origin report needs three dates and two states");
         for (int row=0;row<3;++row) {bool ok=false;const double epoch=words[row*13].toDouble(&ok);require(ok && std::abs(epoch-std::array<double,3>{21545,21545.5,21546.25}[row])<=1e-10,"Origin report clock differs");const auto m=model(mode,type,epoch);State actual{},expected{};V p{},v{};for (int i=0;i<3;++i) {p[i]=input[i]-m.origin[i];v[i]=input[i+3]-m.origin[i+3];}for (int i=0;i<3;++i) {expected[i]=dot(m.rows[i],p);expected[i+3]=dot(m.rows[i],v)+dot(m.rates[i],p);}
            for (int i=0;i<6;++i) {const double in=words[row*13+1+i].toDouble(&ok);require(ok && std::abs(in-input[i])<=1e-8,"Origin fixture inertial input changed");actual[i]=words[row*13+7+i].toDouble(&ok);require(ok && std::isfinite(actual[i]),"Origin-frame state is nonfinite");const double error=std::abs(actual[i]-expected[i]),tol=i<3 ? (lunar ? 2e-6:2e-7):(lunar ? 2e-8:2e-9);if (i<3) maxp=std::max(maxp,error);else maxv=std::max(maxv,error);if (error>tol) std::cerr<<"ORIGIN_DIFF "<<mode.toStdString()<<" axes="<<type.toStdString()<<" row="<<row<<" component="<<i<<" actual="<<actual[i]<<" expected="<<expected[i]<<" error="<<error<<std::endl;require(error<=tol,"Coordinate origin/Topocentric six-state differs from independent root/shape/rotation oracle");}
            samples.append(QJsonObject{{"phase",phase},{"label",label},{"axes",type},{"epoch_a1mjd",epoch},{"origin_offset",json(m.origin)},{"basis_rows",json(m.rows)},{"basis_rates",json(m.rates)},{"oracle_inputs",m.inputs},{"actual",json(actual)},{"expected",json(expected)}});
         }require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Origin-frame mission did not continue");auto *output=window.findChild<QTreeWidget *>("Output");const auto reports=output->findItems("States",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"Origin output tree missing");output->itemDoubleClicked(reports.front(),0);events();QMdiSubWindow *viewer=nullptr;for (auto *w:area->subWindowList()) if (dynamic_cast<ReportViewer *>(w->widget())) viewer=w;require(viewer,"Actual origin report viewer missing");view(viewer);auto *shown=viewer->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Origin viewer differs from checked report");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*viewer);require(viewer->close(),"Origin report did not close");events();++phase;
      };
      run(firstAxes,"created");
      auto open=[&] {const auto items=tree->findItems("CreatedFrame",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Origin tree entry missing");tree->itemDoubleClicked(items.front(),0);events();QMdiSubWindow *child=nullptr;for (auto *w:area->subWindowList()) if (w->property("resourceName").toString()=="CreatedFrame") child=w;require(child,"Actual origin editor missing");view(child);return child;};
      if (bounds) for (const auto &bad:QStringList{"Earth","LP","Center","Sat"}) {const auto before=source->toPlainText();const auto undo=source->document()->availableUndoSteps();auto *child=open();auto *panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel,"Topocentric boundary editor missing");pick(panel,bad);panel->requestApply();events();require(panel->hasChanges() && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty() && source->toPlainText()==before && source->document()->availableUndoSteps()==undo && configured()->GetStringParameter("Origin")=="Site","Non-station Topocentric Apply accepted or was not atomic");++invalid;acceptanceView(*child);panel->discardChanges();require(child->close(),"Topocentric invalid draft did not close");events();}
      const auto before=source->toPlainText();const auto undo=source->document()->availableUndoSteps();auto *child=open();auto *panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel && !panel->hasChanges(),"Origin edit panel missing or dirty");pick(panel,origin);axes(panel,secondAxes,true);require(source->toPlainText()==before && source->document()->availableUndoSteps()==undo && configured()->GetStringParameter("Axes")==firstAxes.toStdString(),"Origin-frame nested edit escaped parent Apply");panel->requestApply();events();panel=dynamic_cast<ResourceEditor *>(child->widget());if (panel && panel->hasChanges()) std::cerr<<panel->findChild<QLabel *>("resourceStatus")->text().toStdString()<<std::endl;require(panel && !panel->hasChanges() && configured()->GetStringParameter("Axes")==secondAxes.toStdString(),"Origin-frame axis Apply failed");view(child);require(child->close(),"Clean origin editor did not close");events();const auto applied=source->toPlainText();source->undo();require(source->toPlainText()==before && window.buildScript() && configured()->GetStringParameter("Axes")==firstAxes.toStdString(),"Origin Undo did not restore exact source/model");source->redo();require(source->toPlainText()==applied && window.buildScript() && configured()->GetStringParameter("Axes")==secondAxes.toStdString(),"Origin Redo did not restore exact source/model");const auto path=files.filePath("Origin frame Δ.script");require(window.saveScriptTo(path) && window.loadScript(path) && window.buildScript() && source->toPlainText()==applied,"Origin Unicode persistence changed source/model");run(secondAxes,"edited-reopened");
      const QJsonObject proof{{"checkpoint","origin-frame-"+mode},{"outcome","passed"},{"mode",mode},{"cancels",cancels},{"picker_cancels",pickerCancels},{"invalid_rejections",invalid},{"samples",samples},{"max_position_error_km",maxp},{"max_velocity_error_km_s",maxv},{"facets",QJsonArray{"actual-origin-picker-create","actual-axes-selector","nested-parent-picker-cancel","pending-apply-isolation","invalid-origin-atomicity","exact-source-history","unicode-save-reopen","independent-origin-frame-six-state","actual-report-viewer"}},{"scope","Five Earth/Luna libration origins use independent bracketed equilibria and local finite-difference velocity. Station origins/Topocentric use independent sphere/ellipsoid geodesy and a synthetic Earth-coincident body with known constant spin. Ephemeris/masses and numerical TDB conversion are explicit shared inputs, not independent clocks/physical ephemeris or measured orientation. Broader origins/input/trajectory/near-pole/native/platform/stress/current NASA corpus and full regression remain open."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
