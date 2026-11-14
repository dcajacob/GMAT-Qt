#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "CelestialBody.hpp"
#include "TimeSystemConverter.hpp"
#include "SolarSystem.hpp"
#include "CoordinateSystem.hpp"
#include "A1Mjd.hpp"
#include "BaseException.hpp"
extern "C" {
#include "SpiceUsr.h"
}
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
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTableWidget>
#include <QTabWidget>
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
static constexpr double pi=3.1415926535897932384626433832795;
static const State input{1000,-200,300,1.25,-.5,.75};
static void spiceCheck() {if (failed_c()) {char m[1841];getmsg_c("LONG",sizeof(m),m);reset_c();throw std::runtime_error(m);}}
static QTableWidgetItem *cell(QWidget *owner,const QString &key) {auto *t=owner->findChild<QTableWidget *>("resourceProperties");if (!t) return nullptr;for (int row=0;row<t->rowCount();++row) if (t->item(row,0) && t->item(row,0)->text()==key) return t->item(row,1);return nullptr;}
static bool hasField(QWidget *owner,const QString &key) {return owner->findChild<QComboBox *>("resource_"+key) || owner->findChild<QLineEdit *>("resource_"+key) || cell(owner,key);}
static QString field(QWidget *owner,const QString &key) {if (auto *c=owner->findChild<QComboBox *>("resource_"+key)) return c->currentText();if (auto *e=owner->findChild<QLineEdit *>("resource_"+key)) return e->text();if (auto *v=cell(owner,key)) return v->text();throw std::runtime_error(("Missing actual axes field "+key).toStdString());}
static void setField(QWidget *owner,const QString &key,const QString &value) {if (auto *c=owner->findChild<QComboBox *>("resource_"+key)) {require(c->isEnabled() && c->findText(value)>=0,qPrintable("Required axes/origin choice absent or disabled: "+value));c->setCurrentText(value);}else if (auto *e=owner->findChild<QLineEdit *>("resource_"+key)) {require(e->isEnabled() && !e->isReadOnly(),"Actual axes text control is not editable");e->setText(value);}else if (auto *v=cell(owner,key)) {require(v->flags() & Qt::ItemIsEditable,"Actual axes table field is not editable");v->setText(value);}else throw std::runtime_error(("Missing editable axes field "+key).toStdString());}
static double dot(const V &a,const V &b) {double s=0;for (int i=0;i<3;++i) s+=a[i]*b[i];return s;}
static V cross(const V &a,const V &b) {return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
static V unit(V a) {const double n=std::sqrt(dot(a,a));require(n>1e-14,"Degenerate independent solar geometry");for (auto &x:a) x/=n;return a;}
static V advance(V a,const V &v,double h) {for (int i=0;i<3;++i) a[i]+=h*v[i];return a;}
struct Model {State relative{},origin{};Matrix rows{},rates{},fixed{},fixedRate{};State result{};};
static Model model(const QString &type,const QString &origin,double epoch)
{
   // Ephemerides and Earth-fixed orientation/rate are explicit shared inputs.
   // Never query the GSE/GSM rotation being tested. Compose the documented
   // geometric axes independently; differentiate a local tangent construction
   // with a five-point rule instead of the production derivative expressions.
   Model m;const A1Mjd time(epoch);auto *solar=Moderator::Instance()->GetSolarSystemInUse();auto *earth=solar->GetBody("Earth"),*sun=solar->GetBody("Sun"),*body=solar->GetBody(origin.toStdString());require(earth && sun && body,"Solar input bodies missing");const auto e=earth->GetMJ2000State(time),s=sun->GetMJ2000State(time),o=body->GetMJ2000State(time);for (int i=0;i<6;++i) {m.relative[i]=s[i]-e[i];m.origin[i]=o[i]-e[i];}
   auto *fixed=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("ReferenceFixed"));require(fixed,"Explicit Earth-fixed reference input missing");const Rvector zero(6,0.,0.,0.,0.,0.,0.);fixed->ToBaseSystem(time,zero,true,true);const auto rotation=fixed->GetLastRotationMatrix(),derivative=fixed->GetLastRotationDotMatrix();for (int i=0;i<3;++i) for (int j=0;j<3;++j) {m.fixed[i][j]=rotation(i,j);m.fixedRate[i][j]=derivative(i,j);}
   const V r{m.relative[0],m.relative[1],m.relative[2]},v{m.relative[3],m.relative[4],m.relative[5]};const double lat=78.6*pi/180,lon=-70.1*pi/180;const V dipole{std::cos(lat)*std::cos(lon),std::cos(lat)*std::sin(lon),std::sin(lat)};
   auto basis=[&](double h) {const auto x=unit(advance(r,v,h));V z{},y{};if (type=="GSE") {z=unit(cross(r,v));y=unit(cross(z,x));}else {require(type=="GSM","Unknown solar axis");V d{};for (int i=0;i<3;++i) for (int j=0;j<3;++j) d[i]+=(m.fixed[i][j]+h*m.fixedRate[i][j])*dipole[j];y=unit(cross(d,x));z=unit(cross(x,y));}return Matrix{x,y,z};};
   m.rows=basis(0);const double h=2;const auto a=basis(-2*h),b=basis(-h),c=basis(h),d=basis(2*h);for (int i=0;i<3;++i) for (int j=0;j<3;++j) m.rates[i][j]=(a[i][j]-8*b[i][j]+8*c[i][j]-d[i][j])/(12*h);
   for (int i=0;i<3;++i) {require(std::abs(dot(m.rows[i],m.rows[i])-1)<1e-12,"Solar basis lost normalization");for (int j=0;j<3;++j) require(std::abs(dot(m.rows[i],m.rows[j])-(i==j))<1e-12,"Solar basis lost orthogonality");}require(dot(m.rows[0],cross(m.rows[1],m.rows[2]))>1-1e-12,"Solar basis lost handedness");
   V p{},velocity{};for (int i=0;i<3;++i) {p[i]=input[i]-m.origin[i];velocity[i]=input[i+3]-m.origin[i+3];}for (int i=0;i<3;++i) {m.result[i]=dot(m.rows[i],p);m.result[i+3]=dot(m.rows[i],velocity)+dot(m.rates[i],p);}return m;
}
static QJsonArray json(const State &s) {QJsonArray a;for (auto x:s) a.append(x);return a;}
static QJsonArray json(const Matrix &m) {QJsonArray a;for (const auto &row:m) {QJsonArray r;for (auto x:row) r.append(x);a.append(r);}return a;}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaSolarAxes");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and solar axis case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Solar axes fixture directory missing");const QString mode=argv[2];const QString type="GSE",origin=mode;
      require(QStringList{"Earth","Mars"}.contains(mode),"Unknown solar axis case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto report=files.filePath("Solar axes Δ.txt");
      const QString baseline="% Retain independent solar axes mission Δ\nCreate CoordinateSystem ReferenceFixed;\nReferenceFixed.Origin = Earth;\nReferenceFixed.Axes = BodyFixed;\nCreate CoordinateSystem InputFrame;\nInputFrame.Origin = Earth;\nInputFrame.Axes = MJ2000Eq;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = InputFrame;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\nSat.X = 1000;\nSat.Y = -200;\nSat.Z = 300;\nSat.VX = 1.25;\nSat.VY = -0.5;\nSat.VZ = 0.75;\nCreate ReportFile States;\nStates.Filename = '"+report+"';\nStates.WriteHeaders = false;\nStates.Precision = 16;\nCreate Variable Keep;\nKeep = 41; % preserve initializer\nBeginMissionSequence;\n";
      QString initial=baseline;for (double epoch:{21545.,25300.25,30000.75}) initial+="Sat.A1ModJulian = "+QString::number(epoch,'g',17)+";\nSat.InputFrame.X = 1000;\nSat.InputFrame.Y = -200;\nSat.InputFrame.Z = 300;\nSat.InputFrame.VX = 1.25;\nSat.InputFrame.VY = -0.5;\nSat.InputFrame.VZ = 0.75;\nReport States Sat.A1ModJulian Sat.InputFrame.X Sat.InputFrame.Y Sat.InputFrame.Z Sat.InputFrame.VX Sat.InputFrame.VY Sat.InputFrame.VZ Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";initial+="Keep = Keep + 1; % preserve mission\n";
      MainWindow window;require(window.initialize(startup),"Solar axis runtime initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");require(source && tree && area && messages,"Solar workspace missing");source->setPlainText(initial);require(window.buildScript(),"Solar axes baseline failed");const auto initialUndo=source->document()->availableUndoSteps();int cancels=0;QJsonArray samples;int phase=0;double maxp=0,maxv=0;
      auto configured=[&] {return Moderator::Instance()->GetConfiguredObject("CreatedFrame");};auto unchanged=[&] {require(!configured() && source->toPlainText()==initial && source->document()->availableUndoSteps()==initialUndo,"Axes draft/invalid Create changed registration/source/history");};
      auto view=[&](QMdiSubWindow *w) {if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);w->showNormal();events();w->move(0,0);w->resize(area->viewport()->size());area->setActiveSubWindow(w);w->raise();events();require(area->activeSubWindow()==w,"Solar axes view is obscured");}acceptanceView(*w);};
      auto axes=[&](ResourceEditor *parent,const std::function<void(QDialog *)> &check) {
         auto *button=parent->findChild<QPushButton *>("editCoordinateAxes");
         auto *tabs=parent->findChild<QTabWidget *>("resourceEditorTabs");
         require(button && button->isEnabled() && tabs,"Actual axes action unavailable");
         for (int i=0;i<tabs->count();++i) if (tabs->widget(i)->isAncestorOf(button)) tabs->setCurrentIndex(i);
         events(); acceptanceView(*parent->window());
         require(button->isVisible(),"Axes action is not reachable in the selected tab");
         std::exception_ptr error;
         QTimer::singleShot(0,parent,[&] {
            auto *dialog=parent->findChild<QDialog *>("coordinateAxesDialog");
            try {require(dialog,"Actual solar axes chooser missing");check(dialog);}
            catch (...) {error=std::current_exception();if (dialog) dialog->reject();}
         });
         button->click();if (error) std::rethrow_exception(error);
      };
      auto nestedAccept=[&](QDialog *d) {d->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Apply)->click();events();require(d->result()==QDialog::Accepted,"Nested solar axes OK failed");};
      auto choose=[&](ResourceEditor *parent,const QString &axis,bool accept) {
         axes(parent,[&](QDialog *dialog) {
            auto *choice=dialog->findChild<QComboBox *>("coordinateAxisType");
            require(choice && choice->isEnabled() && choice->findText(axis)>=0,"Required solar axes choice absent or disabled");
            choice->setCurrentText(axis); events();
            require(!hasField(dialog,"Epoch"),"Solar of-date axis exposes a fixed reference Epoch");
            acceptanceView(*dialog);
            if (accept) nestedAccept(dialog); else {dialog->reject();++cancels;}
         });
      };
      auto create=[&](bool accept) {std::exception_ptr error;QTimer::singleShot(0,&window,[&] {auto *d=window.findChild<QDialog *>("newResourceDialog");try {require(d,"Actual coordinate creator missing");d->findChild<QComboBox *>("resourceType")->setCurrentText("CoordinateSystem");d->findChild<QLineEdit *>("resourceName")->setText("CreatedFrame");ResourceEditor *p=nullptr;for (auto *w:d->findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<ResourceEditor *>(w);candidate && candidate->isVisible()) p=candidate;require(p,"Actual coordinate creation form missing");setField(p,"Origin",origin);choose(p,type,false);require(field(p,"Axes")=="MJ2000Eq","Nested Cancel changed parent axes");unchanged();choose(p,type,true);if (!accept) {d->reject();++cancels;return;}auto *ok=d->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok);setField(p,"Origin",origin);acceptanceView(*d);ok->click();events();if (d->result()!=QDialog::Accepted) std::cerr<<d->findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<std::endl;require(d->result()==QDialog::Accepted,"Valid solar axes Create failed");}catch (...) {error=std::current_exception();if (d) d->reject();}});window.findChild<QAction *>("createResource")->trigger();if (error) std::rethrow_exception(error);};
      create(false);unchanged();create(true);require(configured() && configured()->GetStringParameter("Axes")==type.toStdString() && configured()->GetStringParameter("Origin")==origin.toStdString(),"Created solar axes type/origin differ");const auto created=source->toPlainText();QString stripped=created;stripped.remove(QRegularExpression("^(?:Create CoordinateSystem CreatedFrame;|(?:GMAT[ \\t]+)?CreatedFrame\\.[^;\\n]*;)[ \\t]*\\n",QRegularExpression::MultilineOption));require(stripped==initial,"Axes Create changed unrelated source bytes");for (auto *w:area->subWindowList()) if (w->property("resourceName").toString()=="CreatedFrame") {require(w->close(),"Clean creator panel did not close");events();break;}
      // Bind report expressions only after real GUI creation; an undefined
      // coordinate system cannot be a valid report reference in the baseline.
      {QTextCursor group(source->document());group.beginEditBlock();QTextCursor cursor(source->document());while (!(cursor=source->document()->find("Sat.EarthMJ2000Eq.",cursor)).isNull()) cursor.insertText("Sat.CreatedFrame.");group.endEditBlock();}const auto active=source->toPlainText();require(active==QString(created).replace("Sat.EarthMJ2000Eq.","Sat.CreatedFrame.") && window.buildScript(),"Report binding changed more than newly created-frame references");
      const auto destination=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes) {if (destination.isEmpty()) return;QFile f(QDir(destination).filePath(name));require(f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size(),"Cannot retain solar axis evidence");};
      auto run=[&](const QString &axis,const QString &label) {messages->clear();const auto result=window.runMission();events();save(QString::number(phase)+"-"+label+".script",source->toPlainText().toUtf8());save(QString::number(phase)+"-"+label+"-messages.txt",messages->toPlainText().toUtf8());if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Solar axes report mission failed");QFile f(report);require(f.open(QIODevice::ReadOnly),"Solar report missing");const auto raw=f.readAll();save(QString::number(phase)+"-"+label+"-states.txt",raw);const auto words=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==39,"Solar report needs three epochs and two six-states");State first{};for (int row=0;row<3;++row) {bool ok=false;const double time=words[row*13].toDouble(&ok);require(ok && std::abs(time-std::array<double,3>{21545,25300.25,30000.75}[row])<=1e-10,"Solar report clock differs");const auto m=model(axis,origin,time);const auto truth=m.result;State actual{};for (int i=0;i<6;++i) {const double in=words[row*13+1+i].toDouble(&ok);require(ok && std::abs(in-input[i])<=1e-8,"Solar fixture inputs changed");actual[i]=words[row*13+7+i].toDouble(&ok);require(ok && std::isfinite(actual[i]),"Solar report state nonfinite");const double error=std::abs(actual[i]-truth[i]),tol=i<3 ? 2e-6:2e-8;if (i<3) maxp=std::max(maxp,error);else maxv=std::max(maxv,error);if (error>tol) std::cerr<<"AXIS_DIFF mode="<<mode.toStdString()<<" axis="<<axis.toStdString()<<" row="<<row<<" component="<<i<<" actual="<<actual[i]<<" expected="<<truth[i]<<" error="<<error<<std::endl;require(error<=tol,"Solar axes state differs from independent geometric/tangent model");if (row==0) first[i]=actual[i];else if (row==2 && i==0) require(std::abs(actual[i]-first[i])>0.001,"Solar axis did not advance with mission date");}samples.append(QJsonObject{{"phase",phase},{"label",label},{"axis",axis},{"origin",origin},{"mission_epoch_a1mjd",time},{"sun_relative_earth",json(m.relative)},{"origin_relative_earth",json(m.origin)},{"earth_fixed_to_inertial",json(m.fixed)},{"earth_fixed_rate",json(m.fixedRate)},{"basis_rows",json(m.rows)},{"basis_rates",json(m.rates)},{"actual",json(actual)},{"expected",json(truth)}});}require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Solar mission did not continue");auto *output=window.findChild<QTreeWidget *>("Output");const auto items=output->findItems("States",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Solar output tree missing");output->itemDoubleClicked(items.front(),0);events();QMdiSubWindow *rw=nullptr;for (auto *w:area->subWindowList()) if (dynamic_cast<ReportViewer *>(w->widget())) rw=w;require(rw,"Actual solar report viewer missing");view(rw);auto *shown=rw->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Solar report viewer differs from independently checked data");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*rw);require(rw->close(),"Solar report view failed to close");events();++phase;};
      run(type,"created");
      auto open=[&] {const auto matches=tree->findItems("CreatedFrame",Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Created solar frame tree entry missing");tree->itemDoubleClicked(matches.front(),0);events();QMdiSubWindow *w=nullptr;for (auto *c:area->subWindowList()) if (c->property("resourceName").toString()=="CreatedFrame") w=c;require(w,"Actual solar edit panel missing");view(w);return w;};
      auto *child=open();auto *panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel && !panel->hasChanges(),"Reopened solar axes panel missing/dirty");const auto undo=source->document()->availableUndoSteps();const QString editedAxis="GSM";
      choose(panel,editedAxis,true);
      require(source->toPlainText()==active && source->document()->availableUndoSteps()==undo && panel->hasChanges() && configured()->GetStringParameter("Axes")==type.toStdString(),"Solar edit escaped parent Apply or changed history");panel->requestApply();events();panel=dynamic_cast<ResourceEditor *>(child->widget());if (panel && panel->hasChanges()) std::cerr<<panel->findChild<QLabel *>("resourceStatus")->text().toStdString()<<std::endl;require(panel && !panel->hasChanges(),"Solar axes edit/epoch retirement Apply failed");const QString finalAxis=editedAxis;require(configured()->GetStringParameter("Axes")==finalAxis.toStdString() && configured()->GetStringParameter("Origin")==origin.toStdString(),"Applied solar axis/origin differs");require(!source->toPlainText().contains("CreatedFrame.Epoch"),"Solar axis acquired a fixed Epoch assignment");view(child);require(child->close(),"Clean solar edit panel did not close");events();const auto applied=source->toPlainText();source->undo();require(source->toPlainText()==active && window.buildScript() && configured()->GetStringParameter("Axes")==type.toStdString(),"Solar edit Undo did not restore exact source/model");source->redo();require(source->toPlainText()==applied && window.buildScript() && configured()->GetStringParameter("Axes")==finalAxis.toStdString(),"Solar edit Redo did not restore exact source/model");const auto saved=files.filePath("Solar axes Δ.script");require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==applied,"Solar axes Unicode save/reopen changed source/model");run(finalAxis,"edited-reopened");
      const QJsonObject proof{{"checkpoint","solar-axis-"+mode},{"outcome","passed"},{"mode",mode},{"cancels",cancels},{"samples",samples},{"max_position_error_km",maxp},{"max_velocity_error_km_s",maxv},{"facets",QJsonArray{"actual-coordinate-create","actual-axes-selector","no-fixed-epoch","nested-and-parent-cancel","pending-apply-isolation","exact-source-history","unicode-save-reopen","independent-solar-geometry-and-local-rate","actual-report-viewer"}},{"scope","Bounded GSE/GSM create and GUI axis-switch paths at Earth/Mars origins, three epochs, independently composed axes and five-point local tangent rates. Ephemerides and Earth-fixed orientation/rate are explicit shared inputs; this does not independently qualify those physical inputs, measured magnetic fields, EOP or clocks. GSE preserves the documented zero normal-derivative approximation. Singular geometries, update intervals, broad origins/input/time/native/platform/stress/current private NASA corpus and full regression remain open."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
