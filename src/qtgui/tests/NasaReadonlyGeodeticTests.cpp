#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
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
using State=std::array<double,6>;
// Prescribe ellipsoidal geodetic coordinates, then construct Cartesian inputs
// forward. The oracle never inverts the native Cartesian result or calls GMAT.
static State fixed(int phase) {
   const double pi=std::acos(-1.),lat=(phase?-40.:25.)*pi/180,lon=(phase?-160.:170.)*pi/180,h=phase?100.:50.,a=1000.,f=.2,e2=f*(2-f),n=a/std::sqrt(1-e2*std::sin(lat)*std::sin(lat));
   return{(n+h)*std::cos(lat)*std::cos(lon),(n+h)*std::cos(lat)*std::sin(lon),(n*(1-e2)+h)*std::sin(lat),.1,.2,.3};
}
static State inertial(int phase) {
   const auto p=fixed(phase);const double w=57*std::acos(-1.)/180,c=std::cos(w),s=std::sin(w);
   return{c*p[0]-s*p[1],s*p[0]+c*p[1],p[2],c*p[3]-s*p[4],s*p[3]+c*p[4],p[5]};
}
static QJsonArray oracle(const QString &mode,const State &,int phase) {
   if(mode=="Altitude")return{phase?100.:50.};if(mode=="Latitude")return{phase?-40.:25.};if(mode=="Longitude")return{phase?-160.:170.};
   if(mode=="MHA")return{57.};require(mode=="LST","Unknown geodetic mode");return{phase?257.:227.};
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaReadonlyGeodetic");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup/read-only case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Read-only fixture directory missing");
      const QString mode=argv[2];require(QStringList{"Altitude","Latitude","Longitude","LST","MHA"}.contains(mode),"Unknown read-only mode");
      const QStringList types{mode};
      QStringList refs,fresh;for (const auto &type:types) {const QString dep=".Probe.";refs<<"Sat"+dep+type;fresh<<"Fresh"+dep+type;}
      const State first=inertial(0),second=inertial(1);const QStringList fields{"X","Y","Z","VX","VY","VZ"};
      require(QDir().mkpath(files.filePath("functions Δ")),"Read-only Unicode function directory failed");
      const auto function=files.filePath("functions Δ/Echo.gmf"),report=files.filePath("Read-only Δ.txt"),saved=files.filePath("Read-only Δ.script");
      const QByteArray body="function [out] = Echo(input)\nCreate Variable out;\nBeginMissionSequence;\nout = input + 1;\n";write(function,body);
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto kernel=QFileInfo("../data/planetary_ephem/spk/ceres_1900_2100.bsp").canonicalFilePath();require(!kernel.isEmpty(),"Geodetic shared position kernel missing");
      QString initial="% Preserve geodetic acceptance source Δ\nCreate Asteroid Probe;\nProbe.NAIFId = 2000001;\nProbe.OrbitSpiceKernelName = {'"+kernel+"'};\nProbe.Mu = 1000;\nProbe.EquatorialRadius = 1000;\nProbe.Flattening = 0.2;\nProbe.OrientationEpoch = 21545;\nProbe.SpinAxisRAConstant = -90;\nProbe.SpinAxisRARate = 0;\nProbe.SpinAxisDECConstant = 90;\nProbe.SpinAxisDECRate = 0;\nProbe.RotationConstant = 417;\nProbe.RotationRate = 0;\nCreate CoordinateSystem ProbeEq ProbeFixed;\nProbeEq.Origin = Probe;\nProbeEq.Axes = MJ2000Eq;\nProbeFixed.Origin = Probe;\nProbeFixed.Axes = BodyFixed;\nCreate Spacecraft Sat Fresh;\n";
      for (const auto &name:QStringList{"Sat","Fresh"}) {initial+=name+".CoordinateSystem = ProbeEq;\n"+name+".DateFormat = A1ModJulian;\n"+name+".Epoch = '21545';\n"+name+".DisplayStateType = Cartesian;\n";for (int i=0;i<6;++i) initial+=name+"."+fields[i]+" = "+QString::number(first[i],'g',17)+";\n";}
      QStringList witness;for(const auto &frame:QStringList{"ProbeEq","ProbeFixed"})for(const auto &field:fields)witness<<"Sat."+frame+"."+field;witness<<"Keep";const auto witnessFields=witness.join(' ');
      initial+="Create Variable Result FunctionResult Keep;\nKeep = 41; % preserve initializer\nCreate GmatFunction Echo;\nEcho.FunctionPath = '"+function+"';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nBeginMissionSequence;\n'Read boundary' Result = "+refs.first()+"; % preserve read\n'Output boundary' [FunctionResult] = Echo(2); % preserve function\nReport 'Report boundary' Values Result FunctionResult "+witnessFields+"; % preserve report\n";
      for (int i=0;i<6;++i) initial+="Sat."+fields[i]+" = "+QString::number(second[i],'g',17)+";\n";
      
      
      initial+="Result = "+refs.first()+";\nReport Values Result FunctionResult "+refs.join(' ')+" "+witnessFields+";\nKeep = Keep + 1; % preserve mission\n";
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
      const auto raw=read(report);save("values.txt",raw);const auto tokens=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);QJsonArray expected,actual;for (int phase=0;phase<2;++phase) {const auto values=oracle(mode,phase==0 ? first:second,phase);expected.append(values.first());expected.append(3);for (auto v:values) expected.append(v);for(double v:inertial(phase))expected.append(v);for(double v:fixed(phase))expected.append(v);expected.append(41);}require(tokens.size()==expected.size(),"Read-only result cardinality differs");double maximum=0;for (int i=0;i<tokens.size();++i) {bool ok=false;const double x=tokens[i].toDouble(&ok),error=std::abs(x-expected[i].toDouble());actual.append(x);maximum=std::max(maximum,error);if (!ok || error>2e-5) std::cerr<<"READONLY_DIFF "<<mode.toStdString()<<" index="<<i<<" actual="<<x<<" expected="<<expected[i].toDouble()<<" error="<<error<<std::endl;require(ok && std::isfinite(x) && error<=2e-5,"Read-only results differ from independent prescribed geodetic/static-frame oracle");}
      auto *output=window.findChild<QTreeWidget *>("Output");const auto reports=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"Read-only report tree item missing");output->itemDoubleClicked(reports.front(),0);events();QMdiSubWindow *viewer=nullptr;for (auto *w:area->subWindowList()) if (dynamic_cast<ReportViewer *>(w->widget())) viewer=w;require(viewer,"Actual read-only report viewer missing");view(viewer);auto *shown=viewer->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Read-only report viewer differs from checked bytes");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*viewer);
      const QJsonObject proof{{"checkpoint","readonly-geodetic-"+mode},{"mode",mode},{"outcome","passed"},{"references",QJsonArray::fromStringList(refs)},{"fresh_references",QJsonArray::fromStringList(fresh)},{"guards",guards},{"diagnostics",diagnostics},{"cancel_count",cancels},{"apply_rejections",rejections},{ "apply_owner_replacements",applyOwnerReplacements},{"rejected_apply_owner_preservations",rejectedOwnerPreservations},{"expected",expected},{"actual",actual},{"max_numeric_error",maximum},{"facets",QJsonArray{"actual-writable-and-output-pickers","configured-and-new-readonly-rejections","typed-apply-rollback-and-recovery","report-picker-read-acceptance","exact-history-unicode-file-isolation","owner-editor-retirement-and-rebinding-after-apply","independent-forward-ellipsoid-static-rotation-results","actual-report-viewer"}},{"scope","Selected read-only Altitude/Latitude/Longitude/LST/MHA parameters at two prescribed geodetic points on a synthetic oblate body (radius1000,flattening0.2), with static417-degree rotation normalized to57. Independent forward ellipsoid and rotation inputs plus raw inertial/body-fixed state witnesses; no inverse GMAT converter oracle. Shared Ceres SPK is explicit positional input only. No propagation, independent empirical ephemeris or default-Earth/time-dependent-rotation qualification; arbitrary bodies/frames/invalid references, native OS input/platform/stress/current private NASA provenance and full latest-build suite remain open."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
