#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "CelestialBodyPanel.hpp"
#include <QAction>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QStatusBar>
#include <QTabWidget>
#include <QTest>
#include <QTextCursor>
#include <QTimer>
#include <QTreeWidgetItemIterator>
using namespace formaccept;

int main(int argc,char **argv)
{
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
 app.setOrganizationName("GMATTests"); app.setApplicationName("NasaClosedLoad");
 try {
  require(argc==4,"Supply closed-load startup, cases and key"); const QString key=argv[3],startup=QFileInfo(argv[1]).absoluteFilePath(); QJsonObject spec;
  for (auto v:QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object()["cases"].toArray()) if (v.toObject()["key"]==key) spec=v.toObject();
  require(!spec.isEmpty(),"Unknown closed-load case"); Rig rig(startup); const bool spacecraft=key=="SpacecraftVisual",user=spec["user"].toBool();
  const QString name=spec["body"].toString(),field=spec["field"].toString();
  const auto pathA=rig.files.filePath("First closed mission Δ.script"),pathB=rig.files.filePath("Loaded closed mission Ω.script"),pathBad=rig.files.filePath("Repairable load é.script"),report=rig.files.filePath("closed load.csv");
  QString custom;
  if (user) {
   const auto kernel=QFileInfo(startup).absolutePath()+"/../data/planetary_ephem/spk/ceres_1900_2100.bsp"; require(QFileInfo(kernel).isFile(),"Closed-load shared Ceres kernel absent");
   custom="Create "+spec["type"].toString()+" "+name+";\n"+name+".CentralBody = Sun;\n"+name+".NAIFId = 2000001;\n"+name+".OrbitSpiceKernelName = {'"+kernel+"'};\n"+name+".SpinAxisDECConstant = 90;\n";
  } else if (spacecraft) custom="Create Spacecraft Sat;\nSat.DryMass = 913.25;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 1;\n";
  auto recipe=[&](bool second) {
   const QString variable=second?"LoadedOnly":"FirstOnly",value=second?spec["loaded"].toString():spec["initial"].toString();
   return "% Closed-load source Ω\n"+custom+name+"."+field+" = "+value+";\nCreate Variable Keep "+variable+";\nKeep = "+QString(second?"73":"41")+";\n"+variable+" = "+QString(second?"29":"17")+";\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nKeep = Keep + 1;\n"+variable+" = "+variable+" + 2;\nReport Values Keep "+variable+";\n% Closed-load tail Δ\n";
  };
  const auto initial=recipe(false),loaded=recipe(true); const QString assignment=name+"."+field+" = "+spec["loaded"].toString()+";";
  QString malformed="Create Variable BrokenOnly;\nBrokenOnly = 997;\n"+loaded; require(malformed.count(assignment)==1,"Closed-load malformed source anchor ambiguous"); malformed.replace(assignment,name+"."+field+" = not-a-number;");
  QString repaired=malformed; repaired.replace("not-a-number",spec["loaded"].toString());
  write(pathA,initial.toUtf8()); write(pathB,loaded.toUtf8()); write(pathBad,malformed.toUtf8());
  for (auto pair:QList<QPair<QString,QString>>{{"initial.script",initial},{"loaded.script",loaded},{"malformed.script",malformed},{"expected-repaired.script",repaired}}) rig.save(pair.first,pair.second.toUtf8()); rig.save("case.json",QJsonDocument(spec).toJson());
  require(rig.window.loadScript(pathA)&&rig.window.buildScript(),"Closed-load baseline failed"); QJsonArray operations,dialogs,runs,safeStates;
  auto closed=[&] { for (auto *child:rig.area->subWindowList()) require(!dynamic_cast<EditablePanel *>(child->widget()),"Ordinary File/Open fixture has a configuration panel open"); };
  auto native=[&](bool second,bool repairedState=false) {
   auto *engine=Moderator::Instance(); GmatBase *target=spacecraft?engine->GetConfiguredObject(name.toStdString()):engine->GetSolarSystemInUse()->GetBody(name.toStdString());
   require(target&&target->GetTypeName()==spec["type"].toString().toStdString(),"Closed-load native resource identity/type differs");
   if (!spacecraft) require(dynamic_cast<CelestialBody *>(target)->IsUserDefined()==user,"Closed-load native user/default body status differs");
   const double value=target->GetRealParameter(field.toStdString()); require(std::abs(value-spec[second?"loaded":"initial"].toString().toDouble())<1e-12,"Closed-load native configured field differs");
   const auto current=second?"LoadedOnly":"FirstOnly",stale=second?"FirstOnly":"LoadedOnly"; auto *keep=engine->GetConfiguredObject("Keep"),*other=engine->GetConfiguredObject(current);
   require(keep&&other&&!engine->GetConfiguredObject(stale)&&keep->GetRealParameter("Value")== (second?73:41)&&other->GetRealParameter("Value")== (second?29:17),"Closed-load retained stale namespace or wrong continuation configuration");
   auto *broken=engine->GetConfiguredObject("BrokenOnly"); require(repairedState?(broken&&broken->GetRealParameter("Value")==997):!broken,"Closed-load retained or omitted malformed/repaired-only native object"); return value;
  };
  auto rawTree=[&] { QJsonArray names; for (QTreeWidgetItemIterator it(rig.resources);*it;++it) if (!(*it)->data(0,Qt::UserRole).toString().isEmpty()) names.append((*it)->data(0,Qt::UserRole).toString()); return names; };
  auto run=[&](const QString &phase,bool second,bool repairedState=false) {
   closed(); const auto before=rig.source->toPlainText(); const int undo=rig.source->document()->availableUndoSteps(); write(report,"unexecuted sentinel\n"); acceptanceView(rig.window);
   require(rig.window.runMission()==MainWindow::RunResult::Completed,"Closed-load witness mission failed"); events(); const auto raw=read(report); const auto truth=second?QString("74,31"):QString("42,19"); require(QString::fromUtf8(raw).trimmed()==truth,"Closed-load raw result differs from independent continuation");
   auto *engine=Moderator::Instance(); const auto other=second?"LoadedOnly":"FirstOnly"; require(engine->GetInternalObject("Keep")->GetRealParameter("Value")== (second?74:42)&&engine->GetInternalObject(other)->GetRealParameter("Value")== (second?31:19),"Closed-load native runtime continuation differs");
   require(rig.source->toPlainText()==before&&rig.source->document()->availableUndoSteps()==undo,"Closed-load Run changed source/history"); const double value=native(second,repairedState);
   rig.save(phase+".csv",raw); runs.append(QJsonObject{{"phase",phase},{"report",second?QJsonArray{74,31}:QJsonArray{42,19}},{"native_field",value},{"namespace",other},{"repaired",repairedState}}); rig.save("runs.json",QJsonDocument(runs).toJson());
  };
  auto menuAction=[&](const QString &actionName) {
   auto *action=rig.window.findChild<QAction *>(actionName); require(action&&action->isEnabled(),"Closed-load menu action unavailable"); QMenu *menu=nullptr;
   for (auto *a:rig.window.menuBar()->actions()) if (a->menu()&&a->menu()->actions().contains(action)) menu=a->menu(); require(menu,"Closed-load action menu absent");
   auto *bar=rig.window.menuBar(); acceptanceView(*bar); QTest::mouseClick(bar,Qt::LeftButton,Qt::NoModifier,bar->actionGeometry(menu->menuAction()).center()); require(QTest::qWaitFor([&] {return menu->isVisible();},1500),"Closed-load actual menu popup absent");
   menu->setActiveAction(action); acceptanceView(*menu); QTest::keyClick(menu,Qt::Key_Return); events();
  };
  auto fileOpen=[&](const QString &path) {
   closed(); const auto before=rig.source->toPlainText(); const int undo=rig.source->document()->availableUndoSteps(); const bool dirty=rig.source->document()->isModified(); bool chosen=false; std::exception_ptr failure; QTimer timer; timer.setInterval(10);
   QObject::connect(&timer,&QTimer::timeout,&rig.window,[&] {
    auto *dialog=qobject_cast<QFileDialog *>(QApplication::activeModalWidget()); if (!dialog) return; timer.stop();
    try {
     require(dialog->windowTitle()=="Open GMAT script","Closed-load unexpected file dialog"); closed(); require(rig.source->toPlainText()==before&&rig.source->document()->availableUndoSteps()==undo&&rig.source->document()->isModified()==dirty,"Closed-load file chooser committed before a decision"); acceptanceView(*dialog);
     auto *buttons=dialog->findChild<QDialogButtonBox *>(); require(buttons,"Closed-load file dialog buttons absent");
     if (path.isEmpty()) { chosen=true; QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton); }
     else { dialog->selectFile(path); auto *filename=dialog->findChild<QLineEdit *>("fileNameEdit"); require(filename&&filename->isVisible(),"Closed-load filename editor absent"); filename->setText(path); require(QTest::qWaitFor([&] {return dialog->selectedFiles()==QStringList{path};},1500),"Closed-load chooser selected wrong Unicode file"); auto *open=buttons->button(QDialogButtonBox::Open); require(open&&open->isVisible()&&open->isEnabled(),"Closed-load chooser Open unavailable"); chosen=true; QTest::mouseClick(open,Qt::LeftButton); }
    } catch (...) { failure=std::current_exception(); dialog->reject(); }
   });
   // Open the File popup before starting a timer that expects the later modal chooser.
   auto *action=rig.window.findChild<QAction *>("openMissionScript"); QMenu *file=nullptr; for (auto *a:rig.window.menuBar()->actions()) if (a->menu()&&a->menu()->actions().contains(action)) file=a->menu(); require(action&&file&&action->isEnabled(),"Closed-load File/Open absent");
   auto *bar=rig.window.menuBar(); acceptanceView(*bar); QTest::mouseClick(bar,Qt::LeftButton,Qt::NoModifier,bar->actionGeometry(file->menuAction()).center()); require(QTest::qWaitFor([&] {return file->isVisible();},1500),"Closed-load File popup absent"); file->setActiveAction(action); acceptanceView(*file); timer.start(); QTest::keyClick(file,Qt::Key_Return); timer.stop(); events(); if (failure) std::rethrow_exception(failure); require(chosen&&!QApplication::activeModalWidget(),"Closed-load chooser did not complete");
   dialogs.append(QJsonObject{{"cancel",path.isEmpty()},{"path",path},{"no_configuration_panels",true}}); rig.save("dialogs.json",QJsonDocument(dialogs).toJson()); closed();
  };
  closed(); run("initial",false); const auto rawInitial=read(report); const auto treeInitial=rawTree(); const int undo=rig.source->document()->availableUndoSteps(); fileOpen({});
  require(rig.source->toPlainText()==initial&&!rig.source->document()->isModified()&&rig.source->document()->availableUndoSteps()==undo&&rawTree()==treeInitial&&read(report)==rawInitial&&read(pathA)==initial.toUtf8()&&read(pathB)==loaded.toUtf8(),"Closed-load picker Cancel changed source/model/history/files/output"); native(false); operations.append("closed-picker-cancel-atomic");
  fileOpen(pathB); require(rig.source->toPlainText()==loaded&&!rig.source->document()->isModified()&&read(pathA)==initial.toUtf8()&&read(pathB)==loaded.toUtf8(),"Closed-load different mission source/file differs"); native(true); operations.append("closed-load-different-namespace"); run("loaded",true);
  // Inspect the newly loaded value through its actual resource editor, then close it before the next File/Open.
  for (auto *tabs:rig.window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->tabText(i)=="Resources") tabs->setCurrentIndex(i);
  auto entries=rig.resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(entries.size()==1,"Closed-load replacement resource tree identity absent"); rig.resources->expandAll(); rig.resources->setCurrentItem(entries[0]); rig.resources->scrollToItem(entries[0]); events(); acceptanceView(*rig.resources); rig.resources->itemDoubleClicked(entries[0],0); events(); QPointer<QMdiSubWindow> owner=rig.area->activeSubWindow(); require(owner&&owner->property("resourceName")==name,"Closed-load replacement editor owner differs");
  owner->showNormal(); owner->resize(rig.area->viewport()->size()); owner->move(0,0); if (spacecraft) for (auto *tabs:owner->findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->tabText(i)=="Visual model") tabs->setCurrentIndex(i);
  auto *input=owner->findChild<QLineEdit *>(spec["control"].toString()); require(input&&std::abs(input->text().toDouble()-spec["loaded"].toString().toDouble())<1e-12,"Closed-load actual reopened field stale"); for (QWidget *p=input;p;p=p->parentWidget()) { for (auto *tabs:owner->findChildren<QTabWidget *>()) if (tabs->indexOf(p)>=0) tabs->setCurrentWidget(p); if (auto *scroll=qobject_cast<QScrollArea *>(p)) scroll->ensureWidgetVisible(input); } events(); require(input->isVisible(),"Closed-load reopened field hidden"); acceptanceView(*owner);
  auto *buttons=owner->findChild<QDialogButtonBox *>(); require(buttons&&buttons->button(QDialogButtonBox::Close),"Closed-load replacement Close absent"); QTest::mouseClick(buttons->button(QDialogButtonBox::Close),Qt::LeftButton); events(); require(!owner,"Closed-load clean replacement panel did not retire"); operations.append("loaded-editor-current-value-closed");
  const auto lastReport=read(report); rig.messages->clear(); fileOpen(pathBad); require(rig.source->toPlainText()==malformed&&!rig.source->document()->isModified()&&read(pathBad)==malformed.toUtf8()&&read(report)==lastReport,"Malformed File/Open lost editable source or changed files/output");
  auto safe=[&](const QString &phase) {
   const auto names=rawTree(); auto *create=rig.window.findChild<QAction *>("createResource"); const auto nodes=rig.window.missionSnapshot().nodes;
   QJsonArray commands; for (const auto &n:nodes) commands.append(n.statement);
   QJsonObject state{{"phase",phase},{"resource_names",names},{"commands",commands},{"create_enabled",create&&create->isEnabled()},{"messages",rig.messages->toPlainText()},{"status",rig.window.statusBar()->currentMessage()}}; safeStates.append(state); rig.save("safe-states.json",QJsonDocument(safeStates).toJson());
   require(nodes.isEmpty(),"Malformed load exposed stale or partial mission commands"); for (auto n:names) require(n.toString()=="SolarSystem","Malformed load exposed a partial configured resource instead of empty base state"); require(create&&!create->isEnabled(),"Malformed load left resource creation enabled in script-error state");
   require(rig.messages->toPlainText().contains("not-a-number")&&rig.source->isEnabled()&&!rig.source->isReadOnly(),"Malformed load lacks diagnostic or editable correction path"); closed();
  };
  safe("loaded-error"); operations.append("malformed-load-minimal-safe-state"); require(rig.window.runMission()==MainWindow::RunResult::Failed,"Malformed script ran a stale mission"); events(); safe("run-refused"); require(read(report)==lastReport&&rig.source->toPlainText()==malformed,"Refused malformed Run overwrote previous result/source"); operations.append("malformed-run-refused-output-preserved");
  auto cursor=rig.source->document()->find("not-a-number"); require(!cursor.isNull()&&malformed.count("not-a-number")==1,"Malformed correction token ambiguous"); rig.source->setTextCursor(cursor); rig.source->setFocus(); acceptanceView(*rig.source); QTest::keyClicks(rig.source,spec["loaded"].toString()); events(); require(rig.source->toPlainText()==repaired&&rig.source->document()->isModified()&&read(pathBad)==malformed.toUtf8(),"Typed correction changed unexpected source or prematurely saved");
  menuAction("saveBuildScript"); require(rig.source->toPlainText()==repaired&&!rig.source->document()->isModified()&&read(pathBad)==repaired.toUtf8(),"Corrected Save-and-build failed or saved wrong source"); native(true,true); auto *create=rig.window.findChild<QAction *>("createResource"); require(create&&create->isEnabled()&&!rig.window.missionSnapshot().nodes.isEmpty(),"Correction did not restore normal GUI state"); operations.append("typed-correction-save-and-build"); run("repaired",true,true);
  require(read(pathA)==initial.toUtf8()&&read(pathB)==loaded.toUtf8()&&read(pathBad)==repaired.toUtf8(),"Closed-load recovery changed an unrelated fixture file"); rig.save("final.script",rig.source->toPlainText().toUtf8()); rig.save("operations.json",QJsonDocument(operations).toJson());
  const QJsonObject proof{{"checkpoint","closed-load-"+key},{"outcome","passed"},{"case",key},{"body",name},{"field",field},{"type",spec["type"]},{"user",user},{"dialogs",dialogs},{"operations",operations},{"runs",runs},{"safe_states",safeStates},{"facets",QJsonArray{"actual-file-open-with-all-configuration-panels-closed","picker-cancel-exact-source-native-tree-history-file-output-isolation","different-loaded-resource-namespace-and-current-editor-value","malformed-source-editable-empty-base-state-and-run-refusal","actual-typed-correction-save-and-build-recovery","independent-initial-loaded-repaired-native-and-raw-continuation-results"}},{"scope","Fourteen ordinary Load contexts reuse explicit body/visual fixtures. Actual closed-panel File/Open, Cancel, new namespace/native field, malformed partial-parse safe GUI, typed correction and Save-and-build recovery; three independent continuation missions. No original ExitNo/Yes or body-alias equivalence, arbitrary scripts/fields/science/native OS/GPU/hardware/platform/stress/current private NASA or complete newest-build qualification."}};
  rig.save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
