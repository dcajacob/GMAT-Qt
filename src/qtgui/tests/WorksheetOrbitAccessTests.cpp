#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ResourceEditor.hpp"
#include "Parameter.hpp"
#include <QTest>
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
static QJsonArray oracle(const QString &mode,const State &s,int) {
   const double radius=std::hypot(s[0],s[1],s[2]);
   if(mode=="RMAG")return {radius};
   require(mode=="SemilatusRectum","Unknown worksheet orbit property");
   const double hx=s[1]*s[5]-s[2]*s[4],hy=s[2]*s[3]-s[0]*s[5],hz=s[0]*s[4]-s[1]*s[3];
   return {(hx*hx+hy*hy+hz*hz)/400000.};
}

int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("WorksheetOrbitAccess");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup/read-only case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Read-only fixture directory missing");
      const QString mode=argv[2];require(QStringList{"RMAG","SemilatusRectum"}.contains(mode),"Unknown read-only mode");
      const QStringList types{mode};
      QStringList refs,fresh;for (const auto &type:types) {const QString dep=mode=="Angular" ? ".EarthMJ2000Eq.":mode=="TotalMass" || mode=="Elapsed" ? ".":".Earth.";refs<<"Sat"+dep+type;fresh<<"Fresh"+dep+type;}
      const State first{7000,-1000,2000,1,6,2},second{8100,1200,-600,-1.5,5.75,.25};const QStringList fields{"X","Y","Z","VX","VY","VZ"};
      require(QDir().mkpath(files.filePath("functions Δ")),"Read-only Unicode function directory failed");
      const auto function=files.filePath("functions Δ/Echo.gmf"),report=files.filePath("Read-only Δ.txt"),saved=files.filePath("Read-only Δ.script");
      const QByteArray body="function [out] = Echo(input)\nCreate Variable out;\nBeginMissionSequence;\nout = input + 1;\n";write(function,body);
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      QString initial="% Preserve read-only acceptance source Δ\nEarth.Mu = 400000;\nCreate Spacecraft Sat Fresh;\nCreate ChemicalTank Fuel Reserve;\nFuel.FuelMass = 23;\nReserve.FuelMass = 7;\nSat.Tanks = {Fuel, Reserve};\nSat.DryMass = 317;\n";
      for (const auto &name:QStringList{"Sat","Fresh"}) {initial+=name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DateFormat = A1ModJulian;\n"+name+".Epoch = '21545';\n"+name+".DisplayStateType = Cartesian;\n";for (int i=0;i<6;++i) initial+=name+"."+fields[i]+" = "+QString::number(first[i],'g',17)+";\n";}
      initial+="Create Variable Result FunctionResult Keep;\nKeep = 41; % preserve initializer\nCreate GmatFunction Echo;\nEcho.FunctionPath = '"+function+"';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nBeginMissionSequence;\n'Read boundary' Result = "+refs.first()+"; % preserve read\n'Output boundary' [FunctionResult] = Echo(2); % preserve function\nReport 'Report boundary' Values Result FunctionResult; % preserve report\n";
      for (int i=0;i<6;++i) initial+="Sat."+fields[i]+" = "+QString::number(second[i],'g',17)+";\n";
      if (mode=="TotalMass") initial+="Sat.DryMass = 401;\nSat.Fuel.FuelMass = 43;\n";
      if (mode=="Elapsed") initial+="Sat.A1ModJulian = 21545.125;\n";
      initial+="Result = "+refs.first()+";\nReport Values Result FunctionResult "+refs.join(' ')+";\nKeep = Keep + 1; % preserve mission\n";
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &b){if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),b);};save("initial.script",initial.toUtf8());save("echo.gmf",body);
      MainWindow window;require(window.initialize(startup),"Read-only initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(source && messages && tree && area,"Read-only workspace missing");source->setPlainText(initial);if (!window.buildScript()) {events();save("baseline-messages.txt",messages->toPlainText().toUtf8());std::cerr<<"BASELINE_STATUS "<<window.statusBar()->currentMessage().toStdString()<<" source chars="<<source->toPlainText().size()<<std::endl<<messages->toPlainText().toStdString();try {const auto log=QString::fromStdString(FileManager::Instance()->GetAbsPathname("LOG_FILE"));std::cerr<<"LOG_PATH "<<log.toStdString()<<std::endl;if (QFileInfo::exists(log)) {const auto bytes=read(log);save("baseline-runtime.log",bytes);std::cerr<<bytes.right(12000).constData()<<std::endl;}}catch (...) {}throw std::runtime_error("Read-only baseline failed");}
      // Exercise the compatible native setter before selecting the worksheet policy.
      const auto native=initial.left(initial.indexOf("BeginMissionSequence;"))+"BeginMissionSequence;\n"+"["+refs.first()+"] = Echo(15999);\nReport Values "+refs.first()+" Sat.X Sat.Y Sat.Z Sat.VX Sat.VY Sat.VZ;\n";
      source->setPlainText(native);require(window.buildScript(),"Native compatibility fixture rejected");
      require(Moderator::Instance()->GetConfiguredObject("Sat")->GetStringParameter("OrbitParameterAccess")=="Native","Native compatibility default changed");
      require(window.runMission()==MainWindow::RunResult::Completed,"Native compatible setter failed");events();
      const auto nativeRaw=read(report);save("native-write.script",native.toUtf8());save("native-write.txt",nativeRaw);
      const auto nativeColumns=QString::fromUtf8(nativeRaw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);
      require(nativeColumns.size()==7,"Native compatibility output shape differs");State nativeState{};
      for(int i=0;i<6;++i){bool valid=false;nativeState[i]=nativeColumns[i+1].toDouble(&valid);require(valid&&std::isfinite(nativeState[i]),"Native compatible state is invalid");}
      require(std::abs(nativeColumns[0].toDouble()-16000)<1e-7&&std::abs(oracle(mode,nativeState,0).first().toDouble()-16000)<1e-7,"Native compatible setter differs from independent Cartesian calculation");
      source->setPlainText(initial);require(window.buildScript(),"Native compatibility fixture did not restore initial state");
      auto *resources=window.findChild<QTreeWidget *>("Resources");require(resources,"Worksheet Resource Tree missing");
      for(const auto &ownerName:QStringList{"Sat","Fresh"}) {
         const auto items=resources->findItems(ownerName,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Worksheet spacecraft tree item missing");
         resources->itemDoubleClicked(items.first(),0);events();auto *resourceChild=area->activeSubWindow();
         auto *resource=resourceChild?dynamic_cast<ResourceEditor *>(resourceChild->widget()):nullptr;require(resource,"Actual spacecraft Resource Editor missing");
         auto *choice=resource->findChild<QComboBox *>("resource_OrbitParameterAccess");reveal(choice);acceptanceView(*resourceChild);
         require(choice->currentText()=="Native"&&choice->findText("Worksheet")>=0,"Orbit access modes unavailable in GUI");
         const auto before=source->toPlainText();const auto undo=source->document()->availableUndoSteps();
         QTest::keyClick(choice,Qt::Key_End);events();require(choice->currentText()=="Worksheet"&&resource->hasChanges(),"Worksheet GUI mode selection failed");
         require(source->toPlainText()==before&&Moderator::Instance()->GetConfiguredObject(ownerName.toStdString())->GetStringParameter("OrbitParameterAccess")=="Native","Pending worksheet mode changed model/source");
         resource->discardChanges();events();require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&!resource->hasChanges(),"Discarded worksheet mode changed source/history");
         // discardChanges acknowledges abandoning this snapshot when closing it.
         require(resourceChild->close(),"Discarded worksheet editor did not close");events();
         resources->itemDoubleClicked(items.first(),0);events();resourceChild=area->activeSubWindow();
         resource=resourceChild?dynamic_cast<ResourceEditor *>(resourceChild->widget()):nullptr;require(resource,"Reopened worksheet resource editor missing");
         choice=resource->findChild<QComboBox *>("resource_OrbitParameterAccess");reveal(choice);require(choice->currentText()=="Native","Discarded mode leaked into reopened model");
         QTest::keyClick(choice,Qt::Key_End);events();require(resource->hasChanges(),"Reopened worksheet selection did not become pending");
         auto *buttons=resource->findChild<QDialogButtonBox *>("resourceButtons");require(buttons,"Worksheet resource buttons missing");
         acceptanceView(*resourceChild);buttons->button(QDialogButtonBox::Apply)->click();events();
         resource=dynamic_cast<ResourceEditor *>(resourceChild->widget());require(resource&&!resource->hasChanges(),"Worksheet mode Apply failed");
         const auto applied=source->toPlainText();const auto policy=QString::fromStdString(Moderator::Instance()->GetConfiguredObject(ownerName.toStdString())->GetStringParameter("OrbitParameterAccess"));
         save("policy-"+ownerName+"-applied.script",applied.toUtf8());
         save("policy-"+ownerName+"-apply.json",QJsonDocument(QJsonObject{{"owner",ownerName},{"requested","Worksheet"},{"configured",policy},{"source_changed",applied!=before}}).toJson());
         require(applied!=before&&policy=="Worksheet","Worksheet mode was not committed");
         require(resourceChild->close(),"Clean worksheet spacecraft editor did not close");events();
         source->undo();require(source->toPlainText()==before&&window.buildScript()&&Moderator::Instance()->GetConfiguredObject(ownerName.toStdString())->GetStringParameter("OrbitParameterAccess")=="Native","Worksheet mode Undo failed");
         source->redo();require(source->toPlainText()==applied&&window.buildScript()&&Moderator::Instance()->GetConfiguredObject(ownerName.toStdString())->GetStringParameter("OrbitParameterAccess")=="Worksheet","Worksheet mode Redo failed");
      }
      initial=source->toPlainText();save("worksheet-initial.script",initial.toUtf8());
      QMdiSubWindow *child=nullptr;CommandEditor *panel=nullptr;QJsonArray guards,diagnostics;int cancels=0,rejections=0;
      auto view=[&](QMdiSubWindow *w) {if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);w->showNormal();events();w->move(0,0);w->resize(area->viewport()->size());area->setActiveSubWindow(w);w->raise();events();}acceptanceView(*w);};
      auto open=[&](const QString &label) {const auto snapshot=window.missionSnapshot();int index=-1;for (int i=0;i<snapshot.nodes.size();++i) if (missionCommandLabel(snapshot.nodes[i].statement).name==label) index=i;require(index>=0,"Read-only mission command missing");QTreeWidgetItem *item=nullptr;for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid() && (*it)->data(0,Qt::UserRole).toInt()==index) item=*it;require(item,"Read-only mission tree item missing");tree->itemDoubleClicked(item,0);events();child=area->activeSubWindow();panel=dynamic_cast<CommandEditor *>(child->widget());require(panel && !panel->hasChanges(),"Read-only actual clean command panel missing");view(child);};
      auto close=[&] {require(child->close(),"Read-only clean command did not close");events();child=nullptr;panel=nullptr;};
      auto picker=[&](const QString &field,const QStringList &references,bool writable,bool commit) {
         auto *button=panel->findChild<QPushButton *>("commandChoose_"+field);reveal(button);acceptanceView(*child);std::exception_ptr error;
         QTimer::singleShot(0,panel,[&] {auto *dialog=dynamic_cast<ReportParameterDialog *>(panel->findChild<QDialog *>("reportParameterDialog"));try {require(dialog,"Actual read-only parameter picker missing");auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry");auto *list=dialog->findChild<QListWidget *>("reportSelectedParameters");auto *buttons=dialog->findChild<QDialogButtonBox *>();auto *owner=dialog->findChild<QComboBox *>("reportPropertyObject"),*property=dialog->findChild<QComboBox *>("reportPropertyType");require(entry && list && buttons && owner && property,"Read-only picker controls missing");owner->setCurrentText("Sat");require(owner->currentText()=="Sat","Read-only picker spacecraft absent");
            if (writable) {for (const auto &type:types) require(property->findText(type)<0,"Read-only property offered in writable browser");for (const auto &ref:refs) require(entry->findText(ref)<0,"Read-only configured parameter offered as writable");require(property->findText("X")>=0,"Writable positive-control X missing");}
            if (field=="Outputs" || field=="Parameters") dialog->findChild<QPushButton *>("reportParameterRemoveAll")->click();
            for (const auto &ref:references) {entry->setEditText(ref);if (field=="Outputs" || field=="Parameters") dialog->findChild<QPushButton *>("reportAddParameter")->click();events();if (writable) {require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Read-only reference accepted as writable destination");guards.append(QJsonObject{{"field",field},{"reference",ref},{"configured",refs.contains(ref)},{"accepted",false}});acceptanceView(*dialog);if (field=="Outputs") dialog->findChild<QPushButton *>("reportParameterRemoveAll")->click();}}
            if (writable) {entry->setEditText("Result");if (field=="Outputs") dialog->findChild<QPushButton *>("reportAddParameter")->click();events();require(buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Valid writable destination disabled by read-only filtering");acceptanceView(*dialog);}
            if (commit) {require(buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Valid read-only report selection disabled");acceptanceView(*dialog);buttons->button(QDialogButtonBox::Ok)->click();}else {acceptanceView(*dialog);buttons->button(QDialogButtonBox::Cancel)->click();++cancels;}
         }catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});button->click();if (error) std::rethrow_exception(error);
      };
      auto badApply=[&](const QString &field,const QString &ref) {auto *input=panel->findChild<QLineEdit *>("commandField_"+field);reveal(input);const auto prior=input->text(),before=source->toPlainText();const auto undo=source->document()->availableUndoSteps();input->setText(ref);events();acceptanceView(*child);panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();input=panel->findChild<QLineEdit *>("commandField_"+field);require(input,"Read-only Apply removed its correction field");if (!panel->hasChanges() || source->toPlainText()!=before || input->text()!=ref) {std::cerr<<"READONLY_APPLY "<<field.toStdString()<<" ref="<<ref.toStdString()<<" dirty="<<panel->hasChanges()<<" source_equal="<<(source->toPlainText()==before)<<" pending="<<input->text().toStdString()<<std::endl;save("unexpected-apply.script",source->toPlainText().toUtf8());}
         require(panel->hasChanges() && source->toPlainText()==before && source->document()->availableUndoSteps()==undo && input->text()==ref && read(function)==body,"Rejected read-only command changed source/history/file or discarded draft");const auto diagnostic=panel->findChild<QLabel *>("commandStatus")->text();require(!diagnostic.isEmpty() && diagnostic!="Apply validates the complete mission and updates the script. Branches include their enclosed commands.","Read-only rejection omitted diagnostic");diagnostics.append(QJsonObject{{"field",field},{"reference",ref},{"diagnostic",diagnostic}});++rejections;acceptanceView(*child);input->setText(prior);events();};
      open("Read boundary");const auto initialUndo=source->document()->availableUndoSteps();picker("Destination",refs+fresh,true,false);require(source->toPlainText()==initial && !panel->hasChanges() && source->document()->availableUndoSteps()==initialUndo,"Writable picker Cancel changed source/history");for (const auto &ref:refs+fresh) badApply("Destination",ref);
      auto *name=panel->findChild<QLineEdit *>("commandName");name->setText("Checked read");acceptanceView(*child);panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();require(!panel->hasChanges(),"Read-only read/name correction failed");const auto renamed=source->toPlainText();require(renamed==QString(initial).replace("'Read boundary'","'Checked read'"),"Read-only recovery changed unrelated source");close();
      open("Output boundary");picker("Outputs",refs+fresh,true,false);require(!panel->hasChanges() && source->toPlainText()==renamed,"Function-output picker Cancel changed source");for (const auto &ref:refs+fresh) badApply("Outputs",ref);panel->discardChanges();close();
      open("Report boundary");picker("Parameters",refs,false,false);require(!panel->hasChanges(),"Read-only report picker Cancel changed draft");picker("Parameters",QStringList{"Result","FunctionResult"}+refs,false,true);require(panel->hasChanges() && source->toPlainText()==renamed,"Read-only report draft escaped Apply");acceptanceView(*child);panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();if (panel->hasChanges()) std::cerr<<panel->findChild<QLabel *>("commandStatus")->text().toStdString();require(!panel->hasChanges(),"Read-only report selection Apply failed");const auto applied=source->toPlainText();require(applied==QString(renamed).replace("Report 'Report boundary' Values Result FunctionResult;","Report 'Report boundary' Values Result FunctionResult "+refs.join(' ')+";"),"Read-only report Apply changed unrelated source");close();source->undo();require(source->toPlainText()==renamed && window.buildScript(),"Read-only report exact Undo failed");source->redo();require(source->toPlainText()==applied && window.buildScript(),"Read-only report exact Redo failed");require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==applied && read(function)==body,"Read-only Unicode persistence/file isolation failed");save("mission.script",source->toPlainText().toUtf8());save("guards.json",QJsonDocument(guards).toJson());save("diagnostics.json",QJsonDocument(diagnostics).toJson());
      messages->clear();const auto outcome=window.runMission();events();save("messages.txt",messages->toPlainText().toUtf8());if (outcome!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(outcome==MainWindow::RunResult::Completed && Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Read-only corrected mission did not complete");
      const auto raw=read(report);save("values.txt",raw);const auto tokens=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);QJsonArray expected,actual;for (int phase=0;phase<2;++phase) {const auto values=oracle(mode,phase==0 ? first:second,phase);expected.append(values.first());expected.append(3);for (auto v:values) expected.append(v);}require(tokens.size()==expected.size(),"Read-only result cardinality differs");double maximum=0;for (int i=0;i<tokens.size();++i) {bool ok=false;const double x=tokens[i].toDouble(&ok),error=std::abs(x-expected[i].toDouble());actual.append(x);maximum=std::max(maximum,error);if (!ok || error>1e-9) std::cerr<<"READONLY_DIFF "<<mode.toStdString()<<" index="<<i<<" actual="<<x<<" expected="<<expected[i].toDouble()<<" error="<<error<<std::endl;require(ok && std::isfinite(x) && error<=1e-9,"Read-only results differ from independent orbital/mass/time oracle");}
      auto *output=window.findChild<QTreeWidget *>("Output");const auto reports=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"Read-only report tree item missing");output->itemDoubleClicked(reports.front(),0);events();QMdiSubWindow *viewer=nullptr;for (auto *w:area->subWindowList()) if (dynamic_cast<ReportViewer *>(w->widget())) viewer=w;require(viewer,"Actual read-only report viewer missing");view(viewer);auto *shown=viewer->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Read-only report viewer differs from checked bytes");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*viewer);
      const QJsonObject proof{{"checkpoint","worksheet-orbit-access-"+mode},{"mode",mode},{"outcome","passed"},{"references",QJsonArray::fromStringList(refs)},{"fresh_references",QJsonArray::fromStringList(fresh)},{"guards",guards},{"diagnostics",diagnostics},{"cancel_count",cancels},{"apply_rejections",rejections},{"expected",expected},{"actual",actual},{"max_numeric_error",maximum},{"facets",QJsonArray{"actual-writable-and-output-pickers","configured-and-new-readonly-rejections","typed-apply-rollback-and-recovery","report-picker-read-acceptance","exact-history-unicode-file-isolation","independent-orbital-mass-time-results","actual-report-viewer"}},{"scope","Worksheet RMAG/SemilatusRectum at two prescribed Earth states, explicit GUI policy selection and Native setter compatibility. Independent norm/cross-product with synthetic mu=400000. Any-body/frame, native OS input/platform/scientific and full-build gates remain open."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
