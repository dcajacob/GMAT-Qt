#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "CelestialBody.hpp"
#include "TimeSystemConverter.hpp"
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
static Matrix multiply(const Matrix &a,const Matrix &b) {Matrix r{};for (int i=0;i<3;++i) for (int j=0;j<3;++j) for (int k=0;k<3;++k) r[i][j]+=a[i][k]*b[k][j];return r;}
static Matrix transpose(const Matrix &m) {Matrix r{};for (int i=0;i<3;++i) for (int j=0;j<3;++j) r[i][j]=m[j][i];return r;}
static Matrix model(const QString &type,double epoch,const QString &oracle)
{
   // CSPICE supplies independent IAU1976/1980 geometry. The numerical TT
   // argument is shared input, rather than a claim of independent clocks.
   const double tt=TimeSystemConverter::Instance()->Convert(epoch,TimeSystemConverter::A1MJD,TimeSystemConverter::TTMJD,2430000.0),seconds=(tt-21545)*86400;
   furnsh_c(oracle.toUtf8().constData());spiceCheck();auto get=[&](const char *name) {SpiceDouble raw[3][3];pxform_c("J2000",name,seconds,raw);spiceCheck();Matrix m{};for (int i=0;i<3;++i) for (int j=0;j<3;++j) m[i][j]=raw[i][j];return m;};
   const auto mean=get("ACCEPT_MEAN"),ecliptic=get("ACCEPT_ECLIPTIC"),trueEq=get("ACCEPT_TRUE");Matrix result{};
   if (type=="MODEq") result=mean;else if (type=="MODEc") result=ecliptic;else if (type=="TODEq") result=trueEq;
   else {require(type=="TODEc","Unknown epoch oracle type");const auto n=multiply(trueEq,transpose(mean)),ob=multiply(ecliptic,transpose(mean));const double e=std::atan2(ob[1][2],ob[1][1]),psi=std::atan2(-(n[0][1]*std::cos(e)+n[0][2]*std::sin(e)),n[0][0]);const Matrix longitude{{{std::cos(psi),-std::sin(psi),0},{std::sin(psi),std::cos(psi),0},{0,0,1}}};result=multiply(longitude,ecliptic);}
   unload_c(oracle.toUtf8().constData());spiceCheck();return result;
}
static State transformed(const Matrix &m) {State r{};for (int group=0;group<2;++group) for (int i=0;i<3;++i) for (int j=0;j<3;++j) r[group*3+i]+=m[i][j]*input[group*3+j];return r;}
static QJsonArray json(const State &s) {QJsonArray a;for (auto x:s) a.append(x);return a;}
static QJsonArray json(const Matrix &m) {QJsonArray a;for (const auto &row:m) {QJsonArray r;for (auto x:row) r.append(x);a.append(r);}return a;}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaDynamicAxes");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and dynamic axis case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Dynamic axes fixture directory missing");const QString mode=argv[2];const QString type=mode,origin="Earth";
      require(QStringList{"MODEq","MODEc","TODEq","TODEc"}.contains(mode),"Unknown dynamic axis case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto oracle=files.filePath("Independent dynamic FK5 Δ.tf"),report=files.filePath("Dynamic axes Δ.txt");
      QString fk="KPL/FK\n\\begindata\n";const QStringList families{"MEAN_EQUATOR_AND_EQUINOX_OF_DATE","MEAN_ECLIPTIC_AND_EQUINOX_OF_DATE","TRUE_EQUATOR_AND_EQUINOX_OF_DATE"},names{"ACCEPT_MEAN","ACCEPT_ECLIPTIC","ACCEPT_TRUE"};for (int i=0;i<3;++i) {const int id=149901+i;fk+=QString("FRAME_%1 = %2\nFRAME_%2_NAME = '%1'\nFRAME_%2_CLASS = 5\nFRAME_%2_CLASS_ID = %2\nFRAME_%2_CENTER = 399\nFRAME_%2_RELATIVE = 'J2000'\nFRAME_%2_DEF_STYLE = 'PARAMETERIZED'\nFRAME_%2_FAMILY = '%3'\nFRAME_%2_PREC_MODEL = 'EARTH_IAU_1976'\nFRAME_%2_ROTATION_STATE = 'INERTIAL'\n").arg(names[i]).arg(id).arg(families[i]);if (i==1) fk+=QString("FRAME_%1_OBLIQ_MODEL = 'EARTH_IAU_1980'\n").arg(id);if (i==2) fk+=QString("FRAME_%1_NUT_MODEL = 'EARTH_IAU_1980'\n").arg(id);}fk+="\\begintext\n";{QFile f(oracle);require(f.open(QIODevice::WriteOnly) && f.write(fk.toUtf8())==fk.toUtf8().size(),"Cannot write independent epoch-frame fixture");}
      const QString baseline="% Retain independent dynamic axes mission Δ\nCreate CoordinateSystem InputFrame;\nInputFrame.Origin = Earth;\nInputFrame.Axes = MJ2000Eq;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = InputFrame;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\nSat.X = 1000;\nSat.Y = -200;\nSat.Z = 300;\nSat.VX = 1.25;\nSat.VY = -0.5;\nSat.VZ = 0.75;\nCreate ReportFile States;\nStates.Filename = '"+report+"';\nStates.WriteHeaders = false;\nStates.Precision = 16;\nCreate Variable Keep;\nKeep = 41; % preserve initializer\nBeginMissionSequence;\n";
      QString initial=baseline;for (double epoch:{21545.,25300.25,30000.75}) initial+="Sat.A1ModJulian = "+QString::number(epoch,'g',17)+";\nSat.InputFrame.X = 1000;\nSat.InputFrame.Y = -200;\nSat.InputFrame.Z = 300;\nSat.InputFrame.VX = 1.25;\nSat.InputFrame.VY = -0.5;\nSat.InputFrame.VZ = 0.75;\nReport States Sat.A1ModJulian Sat.InputFrame.X Sat.InputFrame.Y Sat.InputFrame.Z Sat.InputFrame.VX Sat.InputFrame.VY Sat.InputFrame.VZ Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";initial+="Keep = Keep + 1; % preserve mission\n";
      MainWindow window;require(window.initialize(startup),"Dynamic axis runtime initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");require(source && tree && area && messages,"Dynamic workspace missing");source->setPlainText(initial);require(window.buildScript(),"Dynamic axes baseline failed");const auto initialUndo=source->document()->availableUndoSteps();int cancels=0;QJsonArray samples;int phase=0;double maxp=0,maxv=0;
      auto configured=[&] {return Moderator::Instance()->GetConfiguredObject("CreatedFrame");};auto unchanged=[&] {require(!configured() && source->toPlainText()==initial && source->document()->availableUndoSteps()==initialUndo,"Axes draft/invalid Create changed registration/source/history");};
      auto view=[&](QMdiSubWindow *w) {if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);w->showNormal();events();w->move(0,0);w->resize(area->viewport()->size());area->setActiveSubWindow(w);w->raise();events();require(area->activeSubWindow()==w,"Dynamic axes view is obscured");}acceptanceView(*w);};
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
            try {require(dialog,"Actual dynamic axes chooser missing");check(dialog);}
            catch (...) {error=std::current_exception();if (dialog) dialog->reject();}
         });
         button->click();if (error) std::rethrow_exception(error);
      };
      auto nestedAccept=[&](QDialog *d) {d->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Apply)->click();events();require(d->result()==QDialog::Accepted,"Nested dynamic axes OK failed");};
      auto choose=[&](ResourceEditor *parent,const QString &axis,bool accept) {
         axes(parent,[&](QDialog *dialog) {
            auto *choice=dialog->findChild<QComboBox *>("coordinateAxisType");
            require(choice && choice->isEnabled() && choice->findText(axis)>=0,"Required dynamic axes choice absent or disabled");
            choice->setCurrentText(axis); events();
            require(!hasField(dialog,"Epoch"),"Dynamic of-date axis exposes a fixed reference Epoch");
            acceptanceView(*dialog);
            if (accept) nestedAccept(dialog); else {dialog->reject();++cancels;}
         });
      };
      auto create=[&](bool accept) {std::exception_ptr error;QTimer::singleShot(0,&window,[&] {auto *d=window.findChild<QDialog *>("newResourceDialog");try {require(d,"Actual coordinate creator missing");d->findChild<QComboBox *>("resourceType")->setCurrentText("CoordinateSystem");d->findChild<QLineEdit *>("resourceName")->setText("CreatedFrame");ResourceEditor *p=nullptr;for (auto *w:d->findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<ResourceEditor *>(w);candidate && candidate->isVisible()) p=candidate;require(p,"Actual coordinate creation form missing");setField(p,"Origin",origin);choose(p,type,false);require(field(p,"Axes")=="MJ2000Eq","Nested Cancel changed parent axes");unchanged();choose(p,type,true);if (!accept) {d->reject();++cancels;return;}auto *ok=d->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok);setField(p,"Origin",origin);acceptanceView(*d);ok->click();events();if (d->result()!=QDialog::Accepted) std::cerr<<d->findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<std::endl;require(d->result()==QDialog::Accepted,"Valid dynamic axes Create failed");}catch (...) {error=std::current_exception();if (d) d->reject();}});window.findChild<QAction *>("createResource")->trigger();if (error) std::rethrow_exception(error);};
      create(false);unchanged();create(true);require(configured() && configured()->GetStringParameter("Axes")==type.toStdString() && configured()->GetStringParameter("Origin")==origin.toStdString(),"Created dynamic axes type/origin differ");const auto created=source->toPlainText();QString stripped=created;stripped.remove(QRegularExpression("^(?:Create CoordinateSystem CreatedFrame;|(?:GMAT[ \\t]+)?CreatedFrame\\.[^;\\n]*;)[ \\t]*\\n",QRegularExpression::MultilineOption));require(stripped==initial,"Axes Create changed unrelated source bytes");for (auto *w:area->subWindowList()) if (w->property("resourceName").toString()=="CreatedFrame") {require(w->close(),"Clean creator panel did not close");events();break;}
      // Bind report expressions only after real GUI creation; an undefined
      // coordinate system cannot be a valid report reference in the baseline.
      {QTextCursor group(source->document());group.beginEditBlock();QTextCursor cursor(source->document());while (!(cursor=source->document()->find("Sat.EarthMJ2000Eq.",cursor)).isNull()) cursor.insertText("Sat.CreatedFrame.");group.endEditBlock();}const auto active=source->toPlainText();require(active==QString(created).replace("Sat.EarthMJ2000Eq.","Sat.CreatedFrame.") && window.buildScript(),"Report binding changed more than newly created-frame references");
      const auto destination=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes) {if (destination.isEmpty()) return;QFile f(QDir(destination).filePath(name));require(f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size(),"Cannot retain dynamic axis evidence");};{QFile f(oracle);require(f.open(QIODevice::ReadOnly),"Cannot retain independent model input");save("oracle.tf",f.readAll());}
      auto run=[&](const QString &axis,const QString &label) {messages->clear();const auto result=window.runMission();events();save(QString::number(phase)+"-"+label+".script",source->toPlainText().toUtf8());save(QString::number(phase)+"-"+label+"-messages.txt",messages->toPlainText().toUtf8());if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Dynamic axes report mission failed");QFile f(report);require(f.open(QIODevice::ReadOnly),"Dynamic report missing");const auto raw=f.readAll();save(QString::number(phase)+"-"+label+"-states.txt",raw);const auto words=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==39,"Dynamic report needs three epochs and two six-states");State first{};for (int row=0;row<3;++row) {bool ok=false;const double time=words[row*13].toDouble(&ok);require(ok && std::abs(time-std::array<double,3>{21545,25300.25,30000.75}[row])<=1e-10,"Dynamic report clock differs");const auto matrix=model(axis,time,oracle);const auto truth=transformed(matrix);State actual{};for (int i=0;i<6;++i) {const double in=words[row*13+1+i].toDouble(&ok);require(ok && std::abs(in-input[i])<=1e-8,"Dynamic fixture inputs changed");actual[i]=words[row*13+7+i].toDouble(&ok);require(ok && std::isfinite(actual[i]),"Dynamic report state nonfinite");const double error=std::abs(actual[i]-truth[i]),tol=i<3 ? (axis.startsWith("TOD") ? 2e-5 : 2e-7) : (axis.startsWith("TOD") ? 2e-8 : 2e-9);if (i<3) maxp=std::max(maxp,error);else maxv=std::max(maxv,error);if (error>tol) std::cerr<<"AXIS_DIFF mode="<<mode.toStdString()<<" axis="<<axis.toStdString()<<" row="<<row<<" component="<<i<<" actual="<<actual[i]<<" expected="<<truth[i]<<" error="<<error<<std::endl;require(error<=tol,"Dynamic axes state differs from independent geometric/CSPICE model");if (row==0) first[i]=actual[i];else if (row==2 && i==0) require(std::abs(actual[i]-first[i])>0.001,"Dynamic axis did not advance with mission date");}samples.append(QJsonObject{{"phase",phase},{"label",label},{"axis",axis},{"origin","Earth"},{"mission_epoch_a1mjd",time},{"oracle_matrix",json(matrix)},{"actual",json(actual)},{"expected",json(truth)}});}require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Dynamic mission did not continue");auto *output=window.findChild<QTreeWidget *>("Output");const auto items=output->findItems("States",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Dynamic output tree missing");output->itemDoubleClicked(items.front(),0);events();QMdiSubWindow *rw=nullptr;for (auto *w:area->subWindowList()) if (dynamic_cast<ReportViewer *>(w->widget())) rw=w;require(rw,"Actual dynamic report viewer missing");view(rw);auto *shown=rw->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Dynamic report viewer differs from independently checked data");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*rw);require(rw->close(),"Dynamic report view failed to close");events();++phase;};
      run(type,"created");
      auto open=[&] {const auto matches=tree->findItems("CreatedFrame",Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Created dynamic frame tree entry missing");tree->itemDoubleClicked(matches.front(),0);events();QMdiSubWindow *w=nullptr;for (auto *c:area->subWindowList()) if (c->property("resourceName").toString()=="CreatedFrame") w=c;require(w,"Actual dynamic edit panel missing");view(w);return w;};
      auto *child=open();auto *panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel && !panel->hasChanges(),"Reopened dynamic axes panel missing/dirty");const auto undo=source->document()->availableUndoSteps();const QString editedAxis=type=="MODEq" ? "MODEc" : type=="MODEc" ? "MODEq" : type=="TODEq" ? "TODEc" : "TODEq";
      choose(panel,editedAxis,true);
      require(source->toPlainText()==active && source->document()->availableUndoSteps()==undo && panel->hasChanges() && configured()->GetStringParameter("Axes")==type.toStdString(),"Dynamic edit escaped parent Apply or changed history");panel->requestApply();events();panel=dynamic_cast<ResourceEditor *>(child->widget());if (panel && panel->hasChanges()) std::cerr<<panel->findChild<QLabel *>("resourceStatus")->text().toStdString()<<std::endl;require(panel && !panel->hasChanges(),"Dynamic axes edit/epoch retirement Apply failed");const QString finalAxis=editedAxis;require(configured()->GetStringParameter("Axes")==finalAxis.toStdString() && configured()->GetStringParameter("Origin")==origin.toStdString(),"Applied dynamic axis/origin differs");require(!source->toPlainText().contains("CreatedFrame.Epoch"),"Dynamic axis acquired a fixed Epoch assignment");view(child);require(child->close(),"Clean dynamic edit panel did not close");events();const auto applied=source->toPlainText();source->undo();require(source->toPlainText()==active && window.buildScript() && configured()->GetStringParameter("Axes")==type.toStdString(),"Dynamic edit Undo did not restore exact source/model");source->redo();require(source->toPlainText()==applied && window.buildScript() && configured()->GetStringParameter("Axes")==finalAxis.toStdString(),"Dynamic edit Redo did not restore exact source/model");const auto saved=files.filePath("Dynamic axes Δ.script");require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==applied,"Dynamic axes Unicode save/reopen changed source/model");run(finalAxis,"edited-reopened");
      const QJsonObject proof{{"checkpoint","dynamic-axis-"+mode},{"outcome","passed"},{"mode",mode},{"cancels",cancels},{"samples",samples},{"max_position_error_km",maxp},{"max_velocity_error_km_s",maxv},{"facets",QJsonArray{"actual-coordinate-create","actual-axes-selector","no-fixed-epoch","nested-and-parent-cancel","pending-apply-isolation","exact-source-history","unicode-save-reopen","independent-date-dependent-six-state","actual-report-viewer"}},{"scope","Bounded four dynamic-of-date Earth axis families, three dates spanning J2000 to 2023 and two configuration phases. NASA MathSpec deliberately neglects rotation derivatives for these axes; velocity expectations preserve that convention. Shared TT conversion is input, not independent clocks or measured orientation. Update intervals, general origins/time/native/platform/stress/current NASA corpus and full regression remain open."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
