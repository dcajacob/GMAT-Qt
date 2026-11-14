#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "BaseException.hpp"
#include "CommandEditor.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "QtPlotReceiver.hpp"
#include "FileManager.hpp"
#include "FiniteBurn.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "Spacecraft.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QCryptographicHash>
#include <QContextMenuEvent>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Reference artifact unavailable"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Reference artifact write failed"); }

int main(int argc,char **argv)
{
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
 app.setOrganizationName("GMATTests"); app.setApplicationName("NasaCommandReferences");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Supply startup, reference case JSON and family");
  const QString family=argv[3],startup=QFileInfo(argv[1]).absoluteFilePath();
  const auto caseBytes=read(QFileInfo(argv[2]).absoluteFilePath()); QJsonObject spec;
  for (auto c:QJsonDocument::fromJson(caseBytes).object()["cases"].toArray()) if (c.toObject()["key"]==family) spec=c.toObject();
  require(!spec.isEmpty(),"Reference family absent"); const QString oldName=spec["reference"].toString(),newName=spec["renamed_reference"].toString();
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Reference fixture unavailable"); QDir::setCurrent(QFileInfo(startup).absolutePath());
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
  auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
  auto json=[&](const QString &name,const auto &v) { save(name,QJsonDocument(v).toJson()); };
  constexpr double radius=7000,mu=398600.4415; const double speed=std::sqrt(mu/radius),omega=std::sqrt(mu/(radius*radius*radius));
  const auto report=files.filePath("values.csv"),extra=files.filePath("extra.csv"),script=files.filePath("Reference lifecycle Δ.script");
  auto resolve=[&](QString text) {
   for (const auto &p:QList<QPair<QString,QString>>{{"@SPEED@",QString::number(speed,'g',17)},{"@VALUES@",report},{"@EXTRA@",extra},{"@DC@",files.filePath("dc.txt")},{"@OPT@",files.filePath("opt.txt")}}) text.replace(p.first,p.second);
   require(!text.contains('@'),"Reference placeholder unresolved"); return text;
  };
  const auto initial=resolve(spec["script"].toString()),renamed=resolve(spec["renamed_script"].toString()),deleted=resolve(spec["deleted_script"].toString());
  write(script,initial.toUtf8()); save("initial.script",initial.toUtf8()); save("expected-renamed.script",renamed.toUtf8()); save("expected-deleted.script",deleted.toUtf8()); json("case.json",spec);
  MainWindow window; require(window.initialize(startup),"Reference runtime unavailable"); window.show(); events(); acceptanceView(window);
  FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",files.path().toStdString());
  require(std::abs(Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant()-mu)<1e-9,"Reference literal gravity differs");
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");
  auto *mission=window.findChild<QTreeWidget *>("Mission"),*resources=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace");
  require(source&&messages&&mission&&resources&&area,"Reference GUI absent");
  if (!window.loadScript(script)||!window.buildScript()) { save("build-messages.txt",messages->toPlainText().toUtf8()); throw std::runtime_error("Reference initial build failed"); }
  QJsonArray operations,rejections,runs,histories; QString currentName=oldName;
  auto tab=[&](QTreeWidget *tree) {
   for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i);
   events();
  };
  auto resourceItem=[&] {
   tab(resources); const auto entries=resources->findItems(currentName,Qt::MatchExactly|Qt::MatchRecursive); require(entries.size()==1,"Reference resource tree identity absent"); return entries.first();
  };
  auto commandItem=[&](const QString &name) {
   tab(mission); int index=-1; const auto snap=window.missionSnapshot();
   for (int i=0;i<snap.nodes.size();++i) if (missionCommandLabel(snap.nodes[i].statement).name==name) { require(index<0,"Reference command label ambiguous"); index=i; }
   require(index>=0,"Reference command snapshot identity absent");
   for (QTreeWidgetItemIterator it(mission);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index) return *it;
   throw std::runtime_error("Reference command tree item absent");
  };
  auto context=[&](QTreeWidget *tree,QTreeWidgetItem *item,const QString &action) {
   tree->expandAll(); tree->setCurrentItem(item); tree->scrollToItem(item); events(); acceptanceView(*tree); std::exception_ptr failure; bool visited=false;
   QTimer::singleShot(0,&window,[&] {
    auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
    try {
     require(menu,"Reference actual context menu absent"); QAction *choice=nullptr;
     for (auto *a:menu->actions()) if (a->objectName()==action||a->text()==action) choice=a;
     require(choice&&choice->isEnabled(),"Reference context action disabled or absent"); acceptanceView(*menu); menu->setActiveAction(choice); visited=true; QTest::keyClick(menu,Qt::Key_Return);
    } catch (...) { failure=std::current_exception(); if (menu) menu->close(); }
   });
   const auto point=tree->visualItemRect(item).center(); QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point)); QApplication::sendEvent(tree->viewport(),&event); events();
   if (failure) std::rethrow_exception(failure); require(visited,"Reference context action not exercised");
  };
  auto objects=[&] {
   auto *engine=Moderator::Instance(); auto *object=engine->GetConfiguredObject(currentName.toStdString());
   require(object&&engine->GetConfiguredObject((oldName+"Suffix").toStdString())->GetRealParameter("Value")==43,"Reference resource/prefix sentinel changed");
   auto *literal=engine->GetConfiguredObject("ReferenceLiteral"); require(literal&&literal->GetStringParameter("Value")==oldName.toStdString(),"Reference quoted literal changed");
   require(!engine->GetConfiguredObject((currentName==oldName?newName:oldName).toStdString()),"Reference old/new resource identity not unique");
   resourceItem(); require(commandItem("Witness"),"Reference surviving witness missing"); return object->GetTypeName();
  };
  const auto originalType=objects();
  auto stable=[&](const QString &before,int undo,bool modified) {
   require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&source->document()->isModified()==modified,"Reference rejected operation changed source/history"); require(objects()==originalType,"Reference operation changed resource type");
  };
  auto refuseResourceDelete=[&](const QString &phase) {
   const auto before=source->toPlainText(); const int undo=source->document()->availableUndoSteps(); const bool modified=source->document()->isModified();
   std::exception_ptr failure; int stage=0; QString diagnostic; QTimer timer; timer.setInterval(10);
   QObject::connect(&timer,&QTimer::timeout,&window,[&] {
    auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); if (!box) return;
    try {
     if (stage==0&&box->windowTitle()=="Delete resource") { stage=1; acceptanceView(*box); require(box->button(QMessageBox::Yes),"Resource Delete lacks Yes"); QTest::mouseClick(box->button(QMessageBox::Yes),Qt::LeftButton); }
     else if (stage==1&&box->windowTitle()=="Cannot delete resource") {
      diagnostic=box->text(); require(diagnostic.contains("used by another resource or mission command"),"Resource reference refusal diagnostic differs"); stage=2; acceptanceView(*box); QTest::mouseClick(box->button(QMessageBox::Ok),Qt::LeftButton); timer.stop();
     }
    } catch (...) { failure=std::current_exception(); timer.stop(); box->reject(); }
   });
   timer.start(); context(resources,resourceItem(),"Delete resource…"); timer.stop(); if (failure) std::rethrow_exception(failure);
   require(stage==2,"Referenced resource deletion not refused"); stable(before,undo,modified);
   rejections.append(QJsonObject{{"phase",phase},{"diagnostic",diagnostic},{"source_history_preserved",true}}); operations.append("resource-delete-refused-"+phase); json("rejections.json",rejections);
  };
  auto openProbe=[&] {
   context(mission,commandItem("Probe"),"Edit command…"); QPointer<QMdiSubWindow> owner=area->currentSubWindow();
   require(owner&&dynamic_cast<CommandEditor *>(owner->widget())&&owner->findChild<QPlainTextEdit *>("commandSource"),"Reference command editor absent"); acceptanceView(*owner); return owner;
  };
  auto discard=[&](QPointer<QMdiSubWindow> owner) {
   require(owner,"Reference editor already disappeared"); dynamic_cast<CommandEditor *>(owner->widget())->discardChanges(); owner->close(); events(); require(!owner,"Reference editor retained after discard");
  };
  auto renameResource=[&](bool cancel) {
   const auto before=source->toPlainText(); const int undo=source->document()->availableUndoSteps(); const bool modified=source->document()->isModified(); bool visited=false; std::exception_ptr failure; QTimer timer; timer.setInterval(10);
   QObject::connect(&timer,&QTimer::timeout,&window,[&] {
    auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return; timer.stop();
    try {
     require(dialog->objectName()=="renameResourceDialog","Reference resource Rename dialog absent"); auto *name=dialog->findChild<QLineEdit *>("resourceName"); auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceRenameButtons");
     require(name&&buttons&&name->text()==oldName,"Reference Rename owner/name differs"); acceptanceView(*dialog); name->setText(newName); visited=true; QTest::mouseClick(buttons->button(cancel?QDialogButtonBox::Cancel:QDialogButtonBox::Ok),Qt::LeftButton);
     require(!dialog->isVisible(),"Reference resource rename rejected valid input");
    } catch (...) { failure=std::current_exception(); dialog->reject(); }
   });
   timer.start(); context(resources,resourceItem(),"renameResource"); timer.stop(); if (failure) std::rethrow_exception(failure); require(visited,"Reference Rename dialog not exercised");
   if (cancel) stable(before,undo,modified);
   else { currentName=newName; require(source->toPlainText()==renamed&&source->document()->availableUndoSteps()>undo,"Reference Rename differs from independent exact source/history"); objects(); }
   operations.append(cancel?"resource-rename-cancel":"resource-rename-accepted");
  };
  refuseResourceDelete("before-rename"); renameResource(true);
  auto owner=openProbe(); auto *draft=owner->findChild<QPlainTextEdit *>("commandSource"); const auto originalDraft=draft->toPlainText(); draft->setPlainText(originalDraft+"\n% pending reference edit Δ\n");
  const auto pendingDraft=draft->toPlainText(); const int guardUndo=source->document()->availableUndoSteps(); context(resources,resourceItem(),"renameResource");
  require(!QApplication::activeModalWidget()&&window.statusBar()->currentMessage().contains("Apply or discard")&&owner&&draft->toPlainText()==pendingDraft,"Resource Rename bypassed pending command protection");
  stable(initial,guardUndo,false); operations.append("pending-command-rename-guard"); discard(owner);
  owner=openProbe(); renameResource(false); require(owner,"Resource Rename unexpectedly closed clean command owner");
  draft=owner->findChild<QPlainTextEdit *>("commandSource"); draft->setPlainText(originalDraft+"\n% stale reference edit Ω\n"); const auto staleDraft=draft->toPlainText();
  const int renamedUndo=source->document()->availableUndoSteps(); auto *buttons=owner->findChild<QDialogButtonBox *>(); auto *apply=buttons?buttons->button(QDialogButtonBox::Apply):nullptr; require(apply,"Reference editor Apply absent"); acceptanceView(*owner); QTest::mouseClick(apply,Qt::LeftButton); events();
  auto *status=owner->findChild<QLabel *>("commandStatus"); require(status&&status->text().contains("mission has changed",Qt::CaseInsensitive)&&draft->toPlainText()==staleDraft,"Stale reference editor did not preserve input/reject Apply");
  stable(renamed,renamedUndo,true); save("stale-status.txt",status->text().toUtf8()); operations.append("stale-command-apply-refused"); discard(owner); refuseResourceDelete("after-rename");
  source->undo(); require(source->toPlainText()==initial&&window.buildScript(),"Resource Rename exact Undo/rebuild failed"); currentName=oldName; objects();
  source->redo(); require(source->toPlainText()==renamed&&window.buildScript(),"Resource Rename exact Redo/rebuild failed"); currentName=newName; objects(); operations.append("resource-rename-undo-redo");
  const auto saved=files.filePath("Renamed references Ω.script"); require(window.saveScriptTo(saved)&&read(saved)==renamed.toUtf8()&&window.loadScript(saved)&&window.buildScript(),"Reference Unicode save/reload failed"); objects(); save("renamed.script",source->toPlainText().toUtf8());
  auto runtimeName=[&](const QString &name) { return name==oldName?newName:name; };
  auto run=[&](const QString &phase,bool removed) {
   QFile::remove(report); QFile::remove(extra); const auto exported=files.filePath(runtimeName("Sat")+".Spacecraft.data"); QFile::remove(exported);
   const auto before=source->toPlainText(); const int undo=source->document()->availableUndoSteps(); const auto outcome=window.runMission(); const auto diagnostic=messages->toPlainText(); save(phase+"-messages.txt",diagnostic.toUtf8());
   require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo,"Reference execution changed source/history");
   const auto failure=removed?spec["deleted_failure"].toString():QString();
   if (!failure.isEmpty()) {
    events(); const auto failureDiagnostic=messages->toPlainText(); save(phase+"-messages.txt",failureDiagnostic.toUtf8());
    require(outcome==MainWindow::RunResult::Failed&&failureDiagnostic.contains(failure)&&(!QFileInfo::exists(report)||read(report).trimmed().isEmpty()),"Deleting essential solver command failed for an unexpected reason or produced witness output");
    runs.append(QJsonObject{{"phase",phase},{"expected_failure",failure},{"diagnostic",failureDiagnostic},{"completed",false}}); json("runs.json",runs); return;
   }
   require(outcome==MainWindow::RunResult::Completed,"Reference mission failed"); const auto raw=read(report); save(phase+".csv",raw); const auto line=QString::fromUtf8(raw).trimmed(); require(!line.contains('\n'),"Reference report row count differs"); const auto cells=line.split(','); require(cells.size()==12,"Reference report width differs");
   const double elapsed=removed?spec["deleted_elapsed"].toDouble():20,angle=omega*elapsed; const auto impulse=spec[removed?"deleted_velocity_increment":"velocity_increment"].toArray();
   QJsonArray expected{elapsed,radius*std::cos(angle),radius*std::sin(angle),0,-speed*std::sin(angle)+impulse[0].toDouble(),speed*std::cos(angle)+impulse[1].toDouble(),impulse[2].toDouble(),41};
   for (auto v:spec[removed?"deleted_tail":"expected_tail"].toArray()) expected.append(v);
   QJsonArray values,native; for (int i=0;i<12;++i) { bool ok=false; const double v=cells[i].toDouble(&ok); if (!ok||!std::isfinite(v)||std::abs(v-expected[i].toDouble())>=(i==0?1e-4:2e-5)) std::cerr<<"Reference "<<family.toStdString()<<" "<<phase.toStdString()<<" field "<<i<<" actual="<<v<<" expected="<<expected[i].toDouble()<<std::endl; require(ok&&std::isfinite(v)&&std::abs(v-expected[i].toDouble())<(i==0?1e-4:2e-5),"Reference result differs from independent orbit/control/solver oracle"); values.append(v); }
   auto *engine=Moderator::Instance(); auto *sat=dynamic_cast<Spacecraft *>(engine->GetInternalObject(runtimeName("Sat").toStdString())); require(sat,"Reference native spacecraft missing"); native.append((sat->GetEpoch()-21545)*86400);
   for (const char *axis:{"X","Y","Z","VX","VY","VZ"}) native.append(sat->GetRealParameter(axis));
   for (const auto &name:QStringList{"Keep","Count","U","Cost","Quantity"}) { auto *v=engine->GetInternalObject(runtimeName(name).toStdString()); require(v,"Reference native variable missing"); native.append(v->GetRealParameter("Value")); }
   for (int i=0;i<12;++i) require(std::isfinite(native[i].toDouble())&&std::abs(native[i].toDouble()-expected[i].toDouble())<(i==0?1e-4:2e-5),"Reference native state differs from independent oracle");
   auto *literal=engine->GetInternalObject("ReferenceLiteral"),*suffix=engine->GetInternalObject((oldName+"Suffix").toStdString()); require(literal&&literal->GetStringParameter("Value")==oldName.toStdString()&&suffix&&suffix->GetRealParameter("Value")==43,"Reference execution lost literal or prefix sentinel");
   QJsonObject effects; const auto trace=window.plotReceiver()->model(runtimeName("Trace")),control=window.plotReceiver()->model("Control"); require(trace&&control&&trace->curves.size()==1&&control->curves.size()==1,"Reference plot publication absent");
   for (const auto &m:{trace,control}) { require(m->curves[0].points.size()>=2,"Reference plot samples absent"); for (const auto &p:m->curves[0].points) require(std::isfinite(p.x)&&std::abs(p.x-p.y)<1e-10,"Reference plot elapsed diagonal differs"); require(std::abs(m->curves[0].points.back().x-elapsed)<1e-4,"Reference plot final epoch differs"); }
   if (family=="Toggle") { require(trace->active==removed&&control->active,"Reference Toggle effect does not discriminate deletion"); effects["toggle_active"]=trace->active; }
   if (family=="BeginFiniteBurn"||family=="EndFiniteBurn") { auto *burn=dynamic_cast<FiniteBurn *>(engine->GetInternalObject(runtimeName("Burn").toStdString())); const bool firing=removed&&family=="EndFiniteBurn"; require(burn&&burn->IsFiring()==firing,"Reference finite-burn activation differs"); effects["finite_burn_firing"]=burn->IsFiring(); }
   if (family=="Report") { const auto bytes=QFileInfo::exists(extra)?read(extra):QByteArray(); save(phase+"-extra.txt",bytes); bool ok=false; const double v=QString::fromUtf8(bytes).trimmed().toDouble(&ok); require(removed?bytes.trimmed().isEmpty():(ok&&v==41),"Reference Report effect does not discriminate deletion"); effects["extra_report_present"]=!bytes.trimmed().isEmpty(); }
   if (family=="Save") { const bool exists=QFileInfo::exists(exported); require(exists!=removed,"Reference Save file existence does not discriminate deletion"); if (exists) { const auto bytes=read(exported); require(bytes.contains(("Create Spacecraft "+newName).toUtf8())&&bytes.contains((newName+".X").toUtf8())&&bytes.contains((newName+".VX").toUtf8()),"Reference Save exported wrong object identity"); save(phase+"-runtime.txt",bytes); } effects["save_present"]=exists; }
   if (family=="Global") { auto *keep=engine->GetInternalObject(newName.toStdString()); require(keep&&keep->IsGlobal()==!removed,"Reference Global flag does not discriminate deletion"); effects["global"]=keep->IsGlobal(); }
   runs.append(QJsonObject{{"phase",phase},{"completed",true},{"values",values},{"native",native},{"expected",expected},{"effects",effects}}); json("runs.json",runs);
  };
  run("renamed",false);
  auto deleteCommand=[&](QMessageBox::StandardButton choice) {
   const auto before=source->toPlainText(); const int undo=source->document()->availableUndoSteps(); const bool modified=source->document()->isModified(); bool visited=false; std::exception_ptr failure; QTimer timer; timer.setInterval(10);
   QObject::connect(&timer,&QTimer::timeout,&window,[&] {
    auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); if (!box) return; timer.stop();
    try { require(box->windowTitle()=="Delete command"&&box->button(choice),"Reference command Delete confirmation absent"); acceptanceView(*box); visited=true; QTest::mouseClick(box->button(choice),Qt::LeftButton); }
    catch (...) { failure=std::current_exception(); box->reject(); }
   }); timer.start(); context(mission,commandItem("Probe"),"Delete command"); timer.stop(); if (failure) std::rethrow_exception(failure); require(visited,"Reference command deletion not exercised");
   if (choice==QMessageBox::Cancel) stable(before,undo,modified);
   else require(source->toPlainText()==deleted&&source->document()->availableUndoSteps()>undo,"Reference command Delete differs from independent exact source/history");
   histories.append(QJsonObject{{"operation",choice==QMessageBox::Cancel?"delete-cancel":"delete-yes"},{"undo_before",undo},{"undo_after",source->document()->availableUndoSteps()},{"modified_before",modified},{"modified_after",source->document()->isModified()}}); operations.append(choice==QMessageBox::Cancel?"command-delete-cancel":"command-delete-yes");
  };
  deleteCommand(QMessageBox::Cancel); deleteCommand(QMessageBox::Yes); objects();
  QJsonArray hierarchy; for (const auto &node:window.missionSnapshot().nodes) { const auto label=missionCommandLabel(node.statement).name; require(label!="Probe","Deleted referencing command remains in snapshot"); hierarchy.append(QJsonObject{{"type",node.type},{"label",label},{"statement",node.statement},{"parent",node.parent}}); }
  json("deleted-hierarchy.json",hierarchy); save("deleted.script",source->toPlainText().toUtf8());
  source->undo(); require(source->toPlainText()==renamed&&window.buildScript(),"Reference command Delete exact Undo failed"); objects(); commandItem("Probe");
  run("recovered",false);
  source->redo(); require(source->toPlainText()==deleted&&window.buildScript(),"Reference command Delete exact Redo failed"); operations.append("command-delete-undo-redo-recovery");
  require(window.saveScriptTo(saved)&&read(saved)==deleted.toUtf8()&&window.loadScript(saved)&&window.buildScript(),"Deleted reference Unicode save/reopen failed"); run("deleted",true);
  save("final.script",source->toPlainText().toUtf8()); json("operations.json",operations); json("history.json",histories);
  const QJsonObject proof{{"checkpoint","command-reference-"+family},{"outcome","passed"},{"family",family},{"reference",oldName},{"renamed_reference",newName},{"case_sha256",QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(spec).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex())},{"rejections",rejections},{"runs",runs},{"operations",operations},{"history",histories},{"facets",QJsonArray{"actual-resource-tree-delete-refusal-before-and-after-rename","actual-resource-rename-cancel-exact-token-literal-prefix-history","pending-command-rename-guard-and-stale-apply-rejection","command-delete-cancel-yes-native-absence-and-exact-source","unicode-save-reopen-command-delete-undo-redo-recovery","independent-orbit-control-solver-native-and-side-effect-results"}},{"scope","Finite documented resource-reference and referencing-command lifecycle. Private historical keyword bodies remain unavailable; RenameRef remains held. No general solver/science/native-input/platform/stress qualification."}};
  json("proof.json",proof); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
