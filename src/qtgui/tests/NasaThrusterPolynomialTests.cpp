#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "CommandEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "ReportViewer.hpp"
#include "Moderator.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *m) {if (!ok) throw std::runtime_error(m);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static void write(const QString &p,const QByteArray &b) {QFile f(p);require(f.open(QIODevice::WriteOnly) && f.write(b)==b.size(),"Polynomial evidence write failed");}
static QByteArray read(const QString &p) {QFile f(p);require(f.open(QIODevice::ReadOnly),"Polynomial evidence missing");return f.readAll();}
static void reveal(QWidget *c) {require(c,"Polynomial GUI control missing");for (QWidget *p=c;p;p=p->parentWidget()) {for (auto *t:c->window()->findChildren<QTabWidget *>()) if (t->indexOf(p)>=0) t->setCurrentWidget(p);if (auto *s=qobject_cast<QScrollArea *>(p)) s->ensureWidgetVisible(c);}events();require(c->isVisible() && c->isEnabled(),"Polynomial GUI control unreachable");}
using Coeff=std::array<double,16>;
struct Sample {double p,t,ref,duty,scale;};
struct Profile {Coeff c{},k{};QList<Sample> samples;double initialTemperature=24;};
static Profile profile(const QString &mode) {
   Profile f;for (auto *a:{&f.c,&f.k}) {(*a)[6]=(*a)[8]=(*a)[10]=1;(*a)[12]=2;}
   f.samples={{2,6,12,.4,1.5},{4,18,12,.4,1.5},{7,30,12,.4,1.5}};
   if (mode=="Pressure") {f.c[0]=2;f.c[1]=.5;f.c[2]=1;f.c[3]=.25;f.c[4]=.125;f.c[14]=-1;f.k[0]=1000;f.k[1]=20;f.k[2]=50;f.k[3]=5;f.k[4]=2;f.k[14]=-1;}
   else if (mode=="Temperature") {f.c[0]=1;f.c[2]=2;f.c[14]=1;f.k[0]=200;f.k[2]=100;for (auto &s:f.samples) s.p=4;}
   else if (mode=="Coupled") {f.c[0]=.5;f.c[1]=.2;f.c[2]=1;f.c[3]=.25;f.c[4]=.125;f.c[14]=.5;f.c[15]=.125;f.k[0]=500;f.k[1]=7;f.k[2]=10;f.k[3]=4;f.k[4]=.5;f.k[14]=.25;f.k[15]=.0625;}
   else if (mode=="Domain") {f.c[0]=1;f.c[2]=2;f.c[14]=-.5;f.k[0]=400;f.k[2]=50;f.initialTemperature=-6;f.samples={{4,6,12,.4,1.5}};}
   else {require(mode=="General" || mode=="Duty" || mode=="Scale","Unknown polynomial case");f.c={1,.2,.75,.1,.025,.05,1.5,.02,2.5,.01,-.5,.125,2,.25,.2,.05};f.k={300,10,20,3,.25,2,.5,.5,1.5,1,-1,4,1.5,.1,.1,.025};if (mode=="Duty") {f.samples.clear();for (double d:{.2,.65,0.,1.}) f.samples.append({4,18,12,d,1.5});}if (mode=="Scale") {f.samples.clear();for (double s:{0.,.5,1.75,3.}) f.samples.append({4,18,12,.4,s});}}
   return f;
}
static double polynomial(const Coeff &a,double p,double ratio) {
   const double bracket=a[2]+a[3]*p+a[4]*p*p+a[5]*std::pow(p,a[6])+a[7]*std::pow(p,a[8])+a[9]*std::pow(p,a[10])+a[11]*std::pow(a[12],a[13]*p);
   return a[0]+a[1]*p+bracket*std::pow(ratio,1+a[14]+a[15]*p);
}
static QJsonArray values(const Profile &f,const Sample &s,int id,bool on) {
   const double raw=on ? polynomial(f.c,s.p,s.t/s.ref):0,isp=on ? polynomial(f.k,s.p,s.t/s.ref):0,force=raw*s.duty*s.scale,flow=on ? -s.duty*raw/(8*isp):0;
   QJsonArray a{id,s.p,s.t,s.ref,s.duty,s.scale,force,isp,flow};for (double d:{2.,-3.,6.}) a.append(force*d/7);for (double d:{2.,-3.,6.}) a.append(force*d/(7*537*1000));a.append(flow);for (double x:{100.,37.,537.,0.,0.,0.,41.}) a.append(x);return a;
}
static QJsonArray json(const Coeff &a) {QJsonArray v;for (double x:a) v.append(x);return v;}
static QTableWidgetItem *cell(ResourceEditor *panel,const QString &name) {auto *table=panel->findChild<QTableWidget *>("resourceProperties");require(table,"Underlying thruster table missing");for (int r=0;r<table->rowCount();++r) if (table->item(r,0)->text()==name) return table->item(r,1);throw std::runtime_error("Underlying coefficient missing");}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaThrusterPolynomials");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and polynomial case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Polynomial fixture directory missing");const QString mode=argv[2];const auto f=profile(mode);const auto report=files.filePath("Thruster polynomial Δ.txt"),saved=files.filePath("Thruster polynomial Δ.script");const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      const QString columns="Index Sat.Fuel.Pressure Sat.Fuel.Temperature Sat.Fuel.RefTemperature Sat.Engine.DutyCycle Sat.Engine.ThrustScaleFactor Sat.Engine.ThrustMagnitude Sat.Engine.Isp Sat.Engine.MassFlowRate Burn.TotalThrust1 Burn.TotalThrust2 Burn.TotalThrust3 Burn.TotalAcceleration1 Burn.TotalAcceleration2 Burn.TotalAcceleration3 Burn.TotalMassFlowRate Sat.Fuel.FuelMass Sat.Decoy.FuelMass Sat.TotalMass Sat.Idle.ThrustMagnitude Sat.Idle.Isp Sat.Idle.MassFlowRate Keep";
      QString initial="% Preserve polynomial acceptance Δ\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DryMass = 400;\nCreate ChemicalTank Fuel Decoy;\nFuel.FuelMass = 100;\nFuel.Pressure = 4;\nFuel.Temperature = "+QString::number(f.initialTemperature,'g',17)+";\nFuel.RefTemperature = 12;\nFuel.Volume = 4;\nFuel.FuelDensity = 1000;\nFuel.PressureModel = PressureRegulated;\nDecoy.FuelMass = 37;\nCreate ChemicalThruster Engine Idle;\nEngine.Tank = {Fuel};\nIdle.Tank = {Decoy};\nEngine.CoordinateSystem = EarthMJ2000Eq;\nEngine.DutyCycle = 1;\nEngine.ThrustScaleFactor = 1;\nEngine.GravitationalAccel = 9.80665;\nEngine.ThrustDirection1 = 1;\nEngine.ThrustDirection2 = 0;\nEngine.ThrustDirection3 = 0;\nEngine.DecrementMass = true;\nSat.Tanks = {Fuel, Decoy};\nSat.Thrusters = {Engine, Idle};\n";
      for (const auto &prefix:QStringList{"C","K"}) for (int i=0;i<16;++i) initial+="Engine."+prefix+QString::number(i+1)+" = "+QString::number(i==0 ? (prefix=="C" ? 10:300):0)+";\n";
      initial+="Create FiniteBurn Burn;\nBurn.Thrusters = {Engine};\nCreate Variable Index Keep;\nKeep = 41; % preserve initializer\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.Precision = 17;\nValues.WriteHeaders = false;\nBeginMissionSequence;\nIndex = -1;\nReport 'Coefficient report' Values Keep; % preserve report\n";
      QJsonArray expected;auto append=[&](const QJsonArray &a) {for (auto v:a) expected.append(v);};append(values(f,{4,mode=="Domain" ? 6.:f.initialTemperature,12,.4,1.5},-1,false));int id=0;
      for (const auto &s:f.samples) {initial+="Sat.Fuel.Pressure = "+QString::number(s.p,'g',17)+";\n";if (mode!="Domain") initial+="Sat.Fuel.Temperature = "+QString::number(s.t,'g',17)+";\n";initial+="Sat.Fuel.RefTemperature = "+QString::number(s.ref,'g',17)+";\nSat.Engine.DutyCycle = "+QString::number(s.duty,'g',17)+";\nSat.Engine.ThrustScaleFactor = "+QString::number(s.scale,'g',17)+";\nIndex = "+QString::number(id)+";\nBeginFiniteBurn Burn(Sat);\nReport Values "+columns+";\nEndFiniteBurn Burn(Sat);\n";append(values(f,s,id++,true));}
      initial+="Index = 99;\nReport Values "+columns+";\nKeep = Keep + 1; % preserve mission\n";append(values(f,f.samples.last(),99,false));
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &b) {if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),b);};save("initial.script",initial.toUtf8());
      MainWindow window;require(window.initialize(startup),"Polynomial initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *resources=window.findChild<QTreeWidget *>("Resources"),*tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(source && messages && resources && tree && area,"Polynomial workspace missing");source->setPlainText(initial);const bool built=window.buildScript();events();if (!built) std::cerr<<messages->toPlainText().toStdString();require(built,"Polynomial baseline failed");QMdiSubWindow *child=nullptr;int setupCancels=0,coefficientCancels=0,reportCancels=0;QJsonArray invalids;
      auto view=[&](QMdiSubWindow *w) {if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);w->showNormal();events();w->move(0,0);w->resize(area->viewport()->size());area->setActiveSubWindow(w);w->raise();events();}acceptanceView(*w);};
      auto open=[&](const QString &name) {const auto items=resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Polynomial resource tree item missing");resources->itemDoubleClicked(items.front(),0);events();child=area->activeSubWindow();auto *p=dynamic_cast<ResourceEditor *>(child->widget());require(p && !p->hasChanges(),"Actual clean polynomial resource editor missing");view(child);return p;};
      auto close=[&] {require(child->close(),"Clean polynomial editor did not close");events();child=nullptr;};
      auto *panel=open("Engine");auto *registered=Moderator::Instance()->GetConfiguredObject("Engine");const int undo=source->document()->availableUndoSteps();
      for (bool commit:{false,true}) {
         auto *d=panel->findChild<QDialog *>("thrusterDialog");require(d && d->isVisible() && !d->isWindow(),"Actual embedded thruster performance form missing");
         auto set=[&](const QString &name,const QString &text) {auto *e=d->findChild<QLineEdit *>("thruster_"+name);reveal(e);e->setText(text);};
         set("DutyCycle","0.4");set("ThrustScaleFactor","1.5");set("GravitationalAccel","8");set("ThrustDirection1","2");set("ThrustDirection2","-3");set("ThrustDirection3","6");
         require(panel->hasChanges() && source->toPlainText()==initial && source->document()->availableUndoSteps()==undo && registered->GetRealParameter("DutyCycle")==1,"Thruster setup draft changed model/source/history");
         if (!commit) {
            acceptanceView(*child);auto *cancel=panel->findChild<QPushButton *>("cancelPanel");reveal(cancel);cancel->click();events();child=nullptr;++setupCancels;
            require(source->toPlainText()==initial && source->document()->availableUndoSteps()==undo && registered->GetRealParameter("DutyCycle")==1,"Thruster setup Cancel changed model/source/history");panel=open("Engine");require(panel->findChild<QLineEdit *>("thruster_DutyCycle")->text().toDouble()==1,"Thruster discarded performance draft survived reopening");
         } else {
            QList<QPair<QString,QString>> bad;if (mode=="Duty") bad={{"DutyCycle","-0.000001"},{"DutyCycle","1.000001"}};if (mode=="Scale") bad={{"ThrustScaleFactor","-1"}};if (mode=="General") bad={{"GravitationalAccel","0"}};
            for (const auto &v:bad) {set(v.first,v.second);acceptanceView(*child);panel->requestApply();events();require(panel->hasChanges() && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty() && source->toPlainText()==initial && source->document()->availableUndoSteps()==undo && registered->GetRealParameter("DutyCycle")==1,"Invalid setup value escaped form or changed model/source/history");invalids.append(QJsonObject{{"field",v.first},{"value",v.second}});acceptanceView(*child);set(v.first,v.first=="DutyCycle" ? "0.4":v.first=="ThrustScaleFactor" ? "1.5":"8");}
         }
      }
      for (bool commit:{false,true}) {auto *button=panel->findChild<QPushButton *>("thrusterCoefficients");reveal(button);acceptanceView(*child);std::exception_ptr error;QTimer::singleShot(0,panel,[&] {auto *d=panel->findChild<QDialog *>("thrusterCoefficientDialog");try {require(d,"Actual coefficient dialog missing");auto *tabs=d->findChild<QTabWidget *>();auto *b=d->findChild<QDialogButtonBox *>();QTableWidget *grids[2]={d->findChild<QTableWidget *>("thrusterThrustCoefficients"),d->findChild<QTableWidget *>("thrusterSecondaryCoefficients")};require(tabs && tabs->count()==2,"Both coefficient families missing");for (int family=0;family<2;++family) {auto *g=grids[family];require(g && g->rowCount()==16 && g->columnCount()==3,"Coefficient family cardinality differs");tabs->setCurrentIndex(family);for (int i=0;i<16;++i) {const QString name=QString(family ? "K":"C")+QString::number(i+1);require(g->item(i,0)->text()==name && !(g->item(i,0)->flags()&Qt::ItemIsEditable) && !(g->item(i,2)->flags()&Qt::ItemIsEditable) && (g->item(i,1)->flags()&Qt::ItemIsEditable),"Coefficient identity/unit/value editability differs");g->item(i,1)->setText(QString::number((family ? f.k:f.c)[i],'g',17));}g->scrollToItem(g->item(0,1));acceptanceView(*d);g->scrollToItem(g->item(15,1));acceptanceView(*d);}
            if (commit && mode=="General") for (int family=0;family<2;++family) {auto *g=grids[family];g->item(15,1)->setText(family ? "inf":"nan");tabs->setCurrentIndex(1-family);b->button(QDialogButtonBox::Ok)->click();require(d->isVisible() && tabs->currentIndex()==family && g->currentRow()==15 && d->findChild<QLabel *>("thrusterCoefficientError")->text().contains(family ? "K16":"C16") && source->toPlainText()==initial,"Nonfinite final coefficient did not identify family/cell atomically");invalids.append(QJsonObject{{"field",family ? "K16":"C16"},{"value",family ? "inf":"nan"}});acceptanceView(*d);g->item(15,1)->setText(QString::number((family ? f.k:f.c)[15],'g',17));}
            acceptanceView(*d);b->button(commit ? QDialogButtonBox::Ok:QDialogButtonBox::Cancel)->click();if (!commit) ++coefficientCancels;
         }catch (...) {error=std::current_exception();if (d) d->reject();}});button->click();if (error) std::rethrow_exception(error);for (int family=0;family<2;++family) for (int i=0;i<16;++i) {const QString name=QString(family ? "K":"C")+QString::number(i+1);const double original=i==0 ? (family ? 300:10):0;require(registered->GetRealParameter(name.toStdString())==original && cell(panel,name)->text().toDouble()==(commit ? (family ? f.k:f.c)[i]:original),"Coefficient draft/Cancel escaped parent Apply or copied incompletely");}require(source->toPlainText()==initial && source->document()->availableUndoSteps()==undo,"Coefficient dialog changed main history");}
      acceptanceView(*child);panel->requestApply();events();panel=dynamic_cast<ResourceEditor *>(child->widget());if (panel && panel->hasChanges()) std::cerr<<panel->findChild<QLabel *>("resourceStatus")->text().toStdString();require(panel && !panel->hasChanges(),"Thruster polynomial parent Apply failed");const auto configured=source->toPlainText();save("configured.script",configured.toUtf8());close();source->undo();require(source->toPlainText()==initial && window.buildScript(),"Polynomial configuration exact Undo failed");source->redo();require(source->toPlainText()==configured && window.buildScript(),"Polynomial configuration exact Redo failed");
      const auto snapshot=window.missionSnapshot();int at=-1;for (int i=0;i<snapshot.nodes.size();++i) if (missionCommandLabel(snapshot.nodes[i].statement).name=="Coefficient report") at=i;require(at>=0,"Polynomial report command missing");QTreeWidgetItem *item=nullptr;for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==at) item=*it;require(item,"Polynomial report tree item missing");tree->itemDoubleClicked(item,0);events();child=area->activeSubWindow();auto *command=dynamic_cast<CommandEditor *>(child->widget());require(command,"Actual polynomial report editor missing");view(child);
      for (bool commit:{false,true}) {auto *button=command->findChild<QPushButton *>("commandChoose_Parameters");reveal(button);acceptanceView(*child);std::exception_ptr error;QTimer::singleShot(0,command,[&] {auto *d=dynamic_cast<ReportParameterDialog *>(command->findChild<QDialog *>("reportParameterDialog"));try {require(d,"Actual polynomial report picker missing");auto *entry=d->findChild<QComboBox *>("reportParameterEntry");d->findChild<QPushButton *>("reportParameterRemoveAll")->click();for (const auto &name:columns.split(' ')) {entry->setEditText(name);d->findChild<QPushButton *>("reportAddParameter")->click();}require(d->findChild<QListWidget *>("reportSelectedParameters")->count()==23,"Polynomial report selection lost fields");acceptanceView(*d);d->findChild<QDialogButtonBox *>()->button(commit ? QDialogButtonBox::Ok:QDialogButtonBox::Cancel)->click();if (!commit) ++reportCancels;}catch (...) {error=std::current_exception();if (d) d->reject();}});button->click();if (error) std::rethrow_exception(error);require(source->toPlainText()==configured && command->hasChanges()==commit,"Polynomial report draft/Cancel changed source");}
      acceptanceView(*child);command->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();require(!command->hasChanges(),"Polynomial report Apply failed");const auto applied=source->toPlainText();require(applied==QString(configured).replace("Report 'Coefficient report' Values Keep;","Report 'Coefficient report' Values "+columns+";"),"Polynomial report Apply changed unrelated source");close();source->undo();require(source->toPlainText()==configured && window.buildScript(),"Polynomial report exact Undo failed");source->redo();require(source->toPlainText()==applied && window.buildScript(),"Polynomial report exact Redo failed");
      if (mode=="Domain") {messages->clear();const auto failed=window.runMission();events();save("domain.script",source->toPlainText().toUtf8());save("domain-messages.txt",messages->toPlainText().toUtf8());require(failed==MainWindow::RunResult::Failed && messages->toPlainText().contains("non-finite thrust or Isp") && Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==41,"Non-real polynomial did not reject before subsequent main command");acceptanceView(window);auto *fuel=open("Fuel");auto *temperature=fuel->findChild<QLineEdit *>("resource_Temperature");reveal(temperature);temperature->setText("6");acceptanceView(*child);fuel->requestApply();events();require(!dynamic_cast<ResourceEditor *>(child->widget())->hasChanges(),"Polynomial domain GUI temperature correction failed");close();save("domain-corrected.script",source->toPlainText().toUtf8());// Changed resource fields are written at the configuration boundary.
         // Require exactly one old assignment, one corrected assignment, and
         // byte-for-byte preservation of every other source character.
         require(applied.count("Fuel.Temperature = -6;\n")==1,"Polynomial domain fixture temperature assignment is ambiguous");
         auto corrected=applied;corrected.remove("Fuel.Temperature = -6;\n");corrected.replace("BeginMissionSequence;","Fuel.Temperature = 6;\nBeginMissionSequence;");
         require(source->toPlainText()==corrected,"Polynomial domain correction changed unrelated source");}
      const auto finalSource=source->toPlainText();require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==finalSource,"Polynomial Unicode persistence failed");save("mission.script",finalSource.toUtf8());save("invalids.json",QJsonDocument(invalids).toJson());messages->clear();const auto result=window.runMission();events();save("messages.txt",messages->toPlainText().toUtf8());if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed && Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Polynomial mission/recovery failed");
      const auto raw=read(report);save("values.txt",raw);const auto tokens=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(tokens.size()==expected.size(),"Polynomial report cardinality differs");QJsonArray actual;double maximum=0;for (int i=0;i<tokens.size();++i) {bool ok=false;const double x=tokens[i].toDouble(&ok),error=std::abs(x-expected[i].toDouble());const int column=i%23;const double tolerance=(column==8 || (column>=12 && column<=15)) ? 1e-12:1e-9;actual.append(x);maximum=std::max(maximum,error);if (!ok || error>tolerance) std::cerr<<"POLYNOMIAL_DIFF "<<mode.toStdString()<<" index="<<i<<" actual="<<x<<" expected="<<expected[i].toDouble()<<" error="<<error<<std::endl;require(ok && std::isfinite(x) && error<=tolerance,"Thruster results differ from independent published polynomial and force/mass-flow oracle");}
      auto *output=window.findChild<QTreeWidget *>("Output");const auto reports=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"Polynomial report tree entry missing");output->itemDoubleClicked(reports.front(),0);events();QMdiSubWindow *viewer=nullptr;for (auto *w:area->subWindowList()) if (dynamic_cast<ReportViewer *>(w->widget())) viewer=w;require(viewer,"Actual polynomial report viewer missing");view(viewer);auto *shown=viewer->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Polynomial viewer differs from checked bytes");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*viewer);
      QJsonArray samples;for (const auto &s:f.samples) samples.append(QJsonArray{s.p,s.t,s.ref,s.duty,s.scale});const QJsonObject proof{{"checkpoint","thruster-polynomial-"+mode},{"outcome","passed"},{"mode",mode},{"C",json(f.c)},{"K",json(f.k)},{"samples",samples},{"setup_cancels",setupCancels},{"coefficient_cancels",coefficientCancels},{"report_cancels",reportCancels},{"invalids",invalids},{"expected",expected},{"actual",actual},{"max_numeric_error",maximum},{"facets",QJsonArray{"actual-performance-and-two-coefficient-tabs","all32-coefficients-and-pending-isolation","exact-history-unicode-report-picker","independent-pressure-temperature-polynomials","applied-force-acceleration-and-massflow","inactive-engine-and-no-depletion-controls","actual-report-viewer"}},{"scope","Selected chemical pressure/temperature and all16-term samples, duty and scale boundaries, real-domain rejection/recovery. Published Celsius ratio and normalized inertial direction give independent force/acceleration/signed-flow checks; no time integration, arbitrary domains/frames/mixture thermodynamics, empirical physics/native input/platform/stress/current private NASA corpus or final full/unattended regression."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
