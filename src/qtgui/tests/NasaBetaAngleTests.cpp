#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
extern "C" {
#include "SpiceUsr.h"
}
#include "FileManager.hpp"
#include <QStatusBar>
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
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
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

static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static void write(const QString &p,const QByteArray &b) {QFile f(p);require(f.open(QIODevice::WriteOnly) && f.write(b)==b.size(),"Read-only evidence write failed");}
static QByteArray read(const QString &p) {QFile f(p);require(f.open(QIODevice::ReadOnly),"Read-only evidence missing");return f.readAll();}
static void reveal(QWidget *control) {
   require(control,"Read-only control missing");
   for (QWidget *page=control;page;page=page->parentWidget()) {
      for (auto *tabs:control->window()->findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
      if (auto *scroll=qobject_cast<QScrollArea *>(page)) scroll->ensureWidgetVisible(control);
   }
   events();require(control->isVisible() && control->isEnabled(),"Read-only control unreachable");
}
using V=std::array<double,3>;using State=std::array<double,6>;
static constexpr double pi=3.1415926535897932384626433832795;
static double dot(const V&a,const V&b){double v=0;for(int i=0;i<3;++i)v+=a[i]*b[i];return v;}
static V cross(const V&a,const V&b){return{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
static V unit(V a){const double n=std::sqrt(dot(a,a));require(n>1e-12,"Degenerate beta geometry");for(auto&v:a)v/=n;return a;}
static void spiceCheck(){if(failed_c()){char text[1841];getmsg_c("LONG",sizeof(text),text);reset_c();throw std::runtime_error(text);}}
static QJsonArray json(const V&a){QJsonArray v;for(double x:a)v.append(x);return v;}
static QJsonArray json(const State&a){QJsonArray v;for(double x:a)v.append(x);return v;}
struct Sample{double epoch,beta,et;V sun;State state;};
static QList<Sample> geometry(const QString&origin){
 QList<Sample> result;
 // CSPICE ephemeris and time conversions are explicit inputs. Neither the
 // GMAT BetaAngle parameter nor its coordinate/calculation helpers are used.
 for(double epoch:{21545.,25300.25,30000.75}){
  const double et=unitim_c((epoch-21545.)*86400.-0.0343817,"TAI","ET");spiceCheck();double state[6],lt;spkezr_c("SUN",et,"J2000","NONE",origin.toUtf8().constData(),state,&lt);spiceCheck();const V sun{state[0],state[1],state[2]},direction=unit(sun);const V reference=std::abs(direction[2])<.8?V{0,0,1}:V{0,1,0};const V perpendicular=unit(cross(direction,reference));
  for(double beta:{0.,35.,-55.,90.,-90.,89.99,-89.99}){
   V normal{};for(int i=0;i<3;++i)normal[i]=std::sin(beta*pi/180)*direction[i]+std::cos(beta*pi/180)*perpendicular[i];normal=unit(normal);const V axis=std::abs(normal[0])<.8?V{1,0,0}:V{0,1,0};const V radial=unit(cross(normal,axis)),tangent=unit(cross(normal,radial));State craft{};for(int i=0;i<3;++i){craft[i]=7000*radial[i];craft[i+3]=7*tangent[i];}
   result.append({epoch,beta,et,sun,craft});
  }
 }
 return result;
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaBetaAngle");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup/read-only case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Read-only fixture directory missing");
      const QString mode=argv[2];require(QStringList{"Earth","Mars"}.contains(mode),"Unknown beta origin");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto kernel=QFileInfo("../data/planetary_ephem/spk/DE421AllPlanets.bsp").canonicalFilePath(),lsk=QFileInfo("../data/time/SPICELeapSecondKernel.tls").canonicalFilePath();require(!kernel.isEmpty()&&!lsk.isEmpty(),"Explicit beta SPICE inputs absent");furnsh_c(kernel.toUtf8().constData());spiceCheck();furnsh_c(lsk.toUtf8().constData());spiceCheck();const auto samples=geometry(mode);require(samples.size()==21,"Beta sample inventory differs");
      const QStringList types{"BetaAngle"};
      QStringList refs,fresh;for (const auto &type:types) {const QString dep="."+mode+".";refs<<"Sat"+dep+type;fresh<<"Fresh"+dep+type;}
      const State first=samples.first().state;const QStringList fields{"X","Y","Z","VX","VY","VZ"};
      require(QDir().mkpath(files.filePath("functions Δ")),"Read-only Unicode function directory failed");
      const auto function=files.filePath("functions Δ/Echo.gmf"),report=files.filePath("Read-only Δ.txt"),saved=files.filePath("Read-only Δ.script");
      const QByteArray body="function [out] = Echo(input)\nCreate Variable out;\nBeginMissionSequence;\nout = input + 1;\n";write(function,body);
      QString initial="% Preserve BetaAngle acceptance source Δ\nSolarSystem.EphemerisSource = SPICE;\nSolarSystem.SPKFilename = '"+kernel+"';\nSolarSystem.UseTTForEphemeris = false;\nSolarSystem.EphemerisUpdateInterval = 0;\nCreate CoordinateSystem ReferenceEq;\nReferenceEq.Origin = "+mode+";\nReferenceEq.Axes = MJ2000Eq;\nCreate Spacecraft Sat Fresh;\n";
      for(const auto&name:QStringList{"Sat","Fresh"}){initial+=name+".CoordinateSystem = ReferenceEq;\n"+name+".DateFormat = A1ModJulian;\n"+name+".Epoch = '21545';\n"+name+".DisplayStateType = Cartesian;\n";for(int i=0;i<6;++i)initial+=name+"."+fields[i]+" = "+QString::number(first[i],'g',17)+";\n";}
      QStringList witness;for(const auto&field:fields)witness<<"Sat.ReferenceEq."+field;witness<<"Keep"<<"Sat.A1ModJulian";const auto witnessFields=witness.join(' ');
      initial+="Create Variable Result FunctionResult Keep;\nKeep = 41; % preserve initializer\nCreate GmatFunction Echo;\nEcho.FunctionPath = '"+function+"';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 17;\nBeginMissionSequence;\n'Read boundary' Result = "+refs.first()+"; % preserve read\n'Output boundary' [FunctionResult] = Echo(2); % preserve function\nReport 'Report boundary' Values Result FunctionResult "+witnessFields+"; % preserve report\n";
      for(int j=1;j<samples.size();++j){const auto&sample=samples[j];initial+="Sat.A1ModJulian = "+QString::number(sample.epoch,'g',17)+";\n";for(int i=0;i<6;++i)initial+="Sat.ReferenceEq."+fields[i]+" = "+QString::number(sample.state[i],'g',17)+";\n";initial+="Result = "+refs.first()+";\nReport Values Result FunctionResult "+refs.join(' ')+" "+witnessFields+";\n";}
      initial+="Keep = Keep + 1; % preserve mission\n";
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &b){if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),b);};save("initial.script",initial.toUtf8());save("echo.gmf",body);
      MainWindow window;require(window.initialize(startup),"Read-only initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(source && messages && tree && area,"Read-only workspace missing");source->setPlainText(initial);if (!window.buildScript()) {events();save("baseline-messages.txt",messages->toPlainText().toUtf8());std::cerr<<"BASELINE_STATUS "<<window.statusBar()->currentMessage().toStdString()<<" source chars="<<source->toPlainText().size()<<std::endl<<messages->toPlainText().toStdString();try {const auto log=QString::fromStdString(FileManager::Instance()->GetAbsPathname("LOG_FILE"));std::cerr<<"LOG_PATH "<<log.toStdString()<<std::endl;if (QFileInfo::exists(log)) {const auto bytes=read(log);save("baseline-runtime.log",bytes);std::cerr<<bytes.right(12000).constData()<<std::endl;}}catch (...) {}throw std::runtime_error("Read-only baseline failed");}
      QPointer<QMdiSubWindow> child;QPointer<CommandEditor> panel;QJsonArray guards,diagnostics;int cancels=0,rejections=0,applyOwnerReplacements=0,rejectedOwnerPreservations=0;
      auto view=[&](QMdiSubWindow *w) {if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);w->showNormal();events();w->move(0,0);w->resize(area->viewport()->size());area->setActiveSubWindow(w);w->raise();events();}acceptanceView(*w);};
      auto open=[&](const QString &label) {const auto snapshot=window.missionSnapshot();int index=-1;for (int i=0;i<snapshot.nodes.size();++i) if (missionCommandLabel(snapshot.nodes[i].statement).name==label) index=i;require(index>=0,"Read-only mission command missing");QTreeWidgetItem *item=nullptr;for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==index) item=*it;require(item,"Read-only mission tree item missing");tree->itemDoubleClicked(item,0);events();QList<QMdiSubWindow *> matches;for (auto *owner:area->subWindowList()) if (owner->property("commandIndex").isValid() && owner->property("commandIndex").toInt()==index && dynamic_cast<CommandEditor *>(owner->widget())) matches.append(owner);require(matches.size()==1,"Read-only command owner missing, duplicated or bound to a different mission node");child=matches.first();area->setActiveSubWindow(child.data());events();panel=dynamic_cast<CommandEditor *>(child->widget());require(panel && !panel->hasChanges(),"Read-only actual clean command panel missing");view(child);};
      auto close=[&] {require(child->close(),"Read-only clean command did not close");events();child=nullptr;panel=nullptr;};
      auto apply=[&](bool accepted) {
         require(child && panel && child->widget()==panel.data(),"Read-only Apply owner/editor identity missing");
         QPointer<CommandEditor> previous=panel;auto *buttons=panel->findChild<QDialogButtonBox *>();require(buttons,"Read-only Apply buttons missing");
         buttons->button(QDialogButtonBox::Apply)->click();events();
         auto *current=child ? dynamic_cast<CommandEditor *>(child->widget()):nullptr;require(current,"Read-only Apply lost its actual editor owner");
         if (accepted) {require(previous.isNull(),"Successful Apply did not retire the previous editor");++applyOwnerReplacements;}
         else {require(previous && current==previous.data(),"Rejected Apply replaced or deleted the correction editor");++rejectedOwnerPreservations;}
         panel=current;require(panel && child->widget()==panel.data(),"Read-only Apply did not bind the current editor");
      };
      auto picker=[&](const QString &field,const QStringList &references,bool writable,bool commit) {
         auto *button=panel->findChild<QPushButton *>("commandChoose_"+field);reveal(button);acceptanceView(*child);std::exception_ptr error;
         QTimer::singleShot(0,panel.data(),[&] {auto *dialog=dynamic_cast<ReportParameterDialog *>(panel->findChild<QDialog *>("reportParameterDialog"));try {require(dialog,"Actual read-only parameter picker missing");auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry");auto *list=dialog->findChild<QListWidget *>("reportSelectedParameters");auto *buttons=dialog->findChild<QDialogButtonBox *>();auto *owner=dialog->findChild<QComboBox *>("reportPropertyObject"),*property=dialog->findChild<QComboBox *>("reportPropertyType");require(entry && list && buttons && owner && property,"Read-only picker controls missing");owner->setCurrentText("Sat");require(owner->currentText()=="Sat","Read-only picker spacecraft absent");
            if (writable) {for (const auto &type:types) require(property->findText(type)<0,"Read-only property offered in writable browser");for (const auto &ref:refs) require(entry->findText(ref)<0,"Read-only configured parameter offered as writable");require(property->findText("X")>=0,"Writable positive-control X missing");}
            if (field=="Outputs" || field=="Parameters") dialog->findChild<QPushButton *>("reportParameterRemoveAll")->click();
            for (const auto &ref:references) {entry->setEditText(ref);if (field=="Outputs" || field=="Parameters") dialog->findChild<QPushButton *>("reportAddParameter")->click();events();if (writable) {require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Read-only reference accepted as writable destination");guards.append(QJsonObject{{"field",field},{"reference",ref},{"configured",refs.contains(ref)},{"accepted",false}});acceptanceView(*dialog);if (field=="Outputs") dialog->findChild<QPushButton *>("reportParameterRemoveAll")->click();}}
            if (writable) {entry->setEditText("Result");if (field=="Outputs") dialog->findChild<QPushButton *>("reportAddParameter")->click();events();require(buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Valid writable destination disabled by read-only filtering");acceptanceView(*dialog);}
            if (commit) {require(buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Valid read-only report selection disabled");acceptanceView(*dialog);buttons->button(QDialogButtonBox::Ok)->click();}else {acceptanceView(*dialog);buttons->button(QDialogButtonBox::Cancel)->click();++cancels;}
         }catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});button->click();if (error) std::rethrow_exception(error);
      };
      auto badApply=[&](const QString &field,const QString &ref) {auto *input=panel->findChild<QLineEdit *>("commandField_"+field);reveal(input);const auto prior=input->text(),before=source->toPlainText();const auto undo=source->document()->availableUndoSteps();input->setText(ref);events();acceptanceView(*child);apply(false);input=panel->findChild<QLineEdit *>("commandField_"+field);require(input,"Read-only Apply removed its correction field");if (!panel->hasChanges() || source->toPlainText()!=before || input->text()!=ref) {std::cerr<<"READONLY_APPLY "<<field.toStdString()<<" ref="<<ref.toStdString()<<" dirty="<<panel->hasChanges()<<" source_equal="<<(source->toPlainText()==before)<<" pending="<<input->text().toStdString()<<std::endl;save("unexpected-apply.script",source->toPlainText().toUtf8());}
         require(panel->hasChanges() && source->toPlainText()==before && source->document()->availableUndoSteps()==undo && input->text()==ref && read(function)==body,"Rejected read-only command changed source/history/file or discarded draft");const auto diagnostic=panel->findChild<QLabel *>("commandStatus")->text();require(!diagnostic.isEmpty() && diagnostic!="Apply validates the complete mission and updates the script. Branches include their enclosed commands.","Read-only rejection omitted diagnostic");diagnostics.append(QJsonObject{{"field",field},{"reference",ref},{"diagnostic",diagnostic}});++rejections;acceptanceView(*child);input->setText(prior);events();};
      open("Read boundary");const auto initialUndo=source->document()->availableUndoSteps();picker("Destination",refs+fresh,true,false);require(source->toPlainText()==initial && !panel->hasChanges() && source->document()->availableUndoSteps()==initialUndo,"Writable picker Cancel changed source/history");for (const auto &ref:refs+fresh) badApply("Destination",ref);
      auto *name=panel->findChild<QLineEdit *>("commandName");name->setText("Checked read");acceptanceView(*child);apply(true);require(!panel->hasChanges(),"Read-only read/name correction failed");const auto renamed=source->toPlainText();require(renamed==QString(initial).replace("'Read boundary'","'Checked read'"),"Read-only recovery changed unrelated source");close();
      open("Output boundary");picker("Outputs",refs+fresh,true,false);require(!panel->hasChanges() && source->toPlainText()==renamed,"Function-output picker Cancel changed source");for (const auto &ref:refs+fresh) badApply("Outputs",ref);panel->discardChanges();close();
      open("Report boundary");picker("Parameters",refs,false,false);require(!panel->hasChanges(),"Read-only report picker Cancel changed draft");picker("Parameters",QStringList{"Result","FunctionResult"}+refs+witness,false,true);require(panel->hasChanges() && source->toPlainText()==renamed,"Read-only report draft escaped Apply");acceptanceView(*child);apply(true);if (panel->hasChanges()) std::cerr<<panel->findChild<QLabel *>("commandStatus")->text().toStdString();require(!panel->hasChanges(),"Read-only report selection Apply failed");const auto applied=source->toPlainText();require(applied==QString(renamed).replace("Report 'Report boundary' Values Result FunctionResult "+witnessFields+";","Report 'Report boundary' Values Result FunctionResult "+refs.join(' ')+" "+witnessFields+";"),"Read-only report Apply changed unrelated source");close();source->undo();require(source->toPlainText()==renamed && window.buildScript(),"Read-only report exact Undo failed");source->redo();require(source->toPlainText()==applied && window.buildScript(),"Read-only report exact Redo failed");require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==applied && read(function)==body,"Read-only Unicode persistence/file isolation failed");save("mission.script",source->toPlainText().toUtf8());save("guards.json",QJsonDocument(guards).toJson());save("diagnostics.json",QJsonDocument(diagnostics).toJson());
      messages->clear();const auto outcome=window.runMission();events();save("messages.txt",messages->toPlainText().toUtf8());if (outcome!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(outcome==MainWindow::RunResult::Completed && Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Read-only corrected mission did not complete");
      const auto raw=read(report);save("values.txt",raw);const auto tokens=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);QJsonArray expected,actual,inputSamples;for(const auto&sample:samples){expected.append(sample.beta);expected.append(3);expected.append(sample.beta);for(double v:sample.state)expected.append(v);expected.append(41);expected.append(sample.epoch);inputSamples.append(QJsonObject{{"epoch_a1mjd",sample.epoch},{"et_seconds",sample.et},{"prescribed_beta_degrees",sample.beta},{"sun_from_origin_j2000_km",json(sample.sun)},{"state",json(sample.state)}});}save("geometry-inputs.json",QJsonDocument(inputSamples).toJson());require(tokens.size()==expected.size(),"Beta result cardinality differs");double maximum=0;for(int i=0;i<tokens.size();++i){bool ok=false;const double x=tokens[i].toDouble(&ok),error=std::abs(x-expected[i].toDouble());actual.append(x);maximum=std::max(maximum,error);if(!ok||error>2e-5)std::cerr<<"BETA_DIFF "<<mode.toStdString()<<" index="<<i<<" actual="<<x<<" expected="<<expected[i].toDouble()<<" error="<<error<<std::endl;require(ok&&std::isfinite(x)&&error<=2e-5,"Beta results differ from independent prescribed plane/sun geometry oracle");if(i%11==0||i%11==2)require(x>=-90.0000001&&x<=90.0000001,"Beta output outside documented range");}
      auto *output=window.findChild<QTreeWidget *>("Output");const auto reports=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"Read-only report tree item missing");output->itemDoubleClicked(reports.front(),0);events();QMdiSubWindow *viewer=nullptr;for (auto *w:area->subWindowList()) if (dynamic_cast<ReportViewer *>(w->widget())) viewer=w;require(viewer,"Actual read-only report viewer missing");view(viewer);auto *shown=viewer->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Read-only report viewer differs from checked bytes");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*viewer);
      const QJsonObject proof{{"checkpoint","beta-angle-"+mode},{"mode",mode},{"outcome","passed"},{"references",QJsonArray::fromStringList(refs)},{"fresh_references",QJsonArray::fromStringList(fresh)},{"guards",guards},{"diagnostics",diagnostics},{"cancel_count",cancels},{"apply_rejections",rejections},{ "apply_owner_replacements",applyOwnerReplacements},{"rejected_apply_owner_preservations",rejectedOwnerPreservations},{"expected",expected},{"actual",actual},{"samples",inputSamples},{"raw_report_values",expected.size()},{"max_numeric_error",maximum},{"kernel",kernel},{"lsk",lsk},{"facets",QJsonArray{"actual-writable-and-output-pickers","configured-and-new-readonly-rejections","typed-apply-rollback-and-recovery","report-picker-read-acceptance","exact-history-unicode-file-isolation","owner-editor-retirement-and-rebinding-after-apply","independent-prescribed-orbit-plane-direct-cspice-inputs","actual-report-viewer"}},{"scope","Read-only BetaAngle for Earth/Mars at21 prescribed orbit planes per case:three A1 epochs and0,+35,-55,+90,-90,+89.99,-89.99 degrees. Direct CSPICE J2000 geometric Sun vectors and TAI-to-ET conversion use explicit shared DE421/LSK inputs; construct independent right-handed orbital planes and Cartesian states without querying GMAT BetaAngle. Native ephemeris accuracy, arbitrary bodies/frames, degenerate states, general dynamics/science, native OS input/GPU/hardware/platform/stress/current private NASA and full latest-build qualification remain open."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
