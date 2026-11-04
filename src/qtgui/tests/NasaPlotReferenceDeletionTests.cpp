#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "BaseException.hpp"
#include "CommandEditor.hpp"
#include "Debugger.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "PlotWidget.hpp"
#include "QtPlotReceiver.hpp"
#include "Spacecraft.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
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

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }

int main(int argc,char **argv)
{
 QApplication app(argc,argv);
 app.setOrganizationName("GMATTests"); app.setApplicationName("NasaPlotReferenceDeletion");
 try {
  require(argc==3&&acceptanceWidgetPlatform(),"Supply startup and plot command for reference deletion");
  const QString startup=QFileInfo(argv[1]).absoluteFilePath(),command=argv[2];
  require(QStringList{"ClearPlot","MarkPoint","PenUp","PenDown"}.contains(command),"Unknown plot deletion command");
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Plot deletion fixture directory absent");
  QDir::setCurrent(QFileInfo(startup).absolutePath());
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
  auto save=[&](const QString &name,const QByteArray &bytes) {
   if (evidence.isEmpty()) return;
   QFile f(QDir(evidence).filePath(name)); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Plot deletion artifact write failed");
  };
  MainWindow window; require(window.initialize(startup),"Plot deletion runtime unavailable");
  window.show(); events(); acceptanceView(window);
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");
  auto *tree=window.findChild<QTreeWidget *>("Mission");
  auto *area=window.findChild<QMdiArea *>("workspace");
  auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");
  require(source&&tree&&area&&messages,"Plot deletion workspace missing");
  constexpr double mu=398600.4415,radius=7000;
  const double speed=std::sqrt(mu/radius),omega=std::sqrt(mu/(radius*radius*radius));
  const QString probe=command+" 'DeleteMe' Trace; % remove only this command\n";
  const QString reference=(command=="PenDown"?QString("PenUp"):QString("PenDown"))+" 'Surviving reference' Trace;\n";
  QString initial="% Preserve deletion source Δ\nCreate Variable Keep;\nKeep = 41;\nCreate Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = 21545;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+QString::number(speed,'g',17)+";\nSat.VZ = 0;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.SRP = Off;\nFM.Drag = None;\nCreate Propagator P;\nP.FM = FM;\nP.Type = RungeKutta89;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nP.Accuracy = 1e-12;\n";
  for (const auto &name:QStringList{"Trace","Control"}) initial+="Create XYPlot "+name+";\n"+name+".XVariable = Sat.ElapsedSecs;\n"+name+".YVariables = {Sat.ElapsedSecs};\n"+name+".ShowPlot = true;\n";
  initial+="BeginMissionSequence;\n"+reference+"Propagate 'FirstArc' P(Sat) {Sat.ElapsedSecs = 20};\n"+probe+"'Unrelated' Keep = Keep + 0;\nPropagate 'LastArc' P(Sat) {Sat.ElapsedSecs = 20};\n% Preserve deletion tail Ω\n";
  QString wanted=initial; require(wanted.count(probe)==1,"Plot deletion independent source anchor ambiguous"); wanted.remove(wanted.indexOf(probe),probe.size());
  source->setPlainText(initial); require(window.buildScript(),"Plot deletion baseline build failed"); source->document()->setModified(false);
  save("initial.script",initial.toUtf8()); save("expected.script",wanted.toUtf8());
  auto missionTab=[&] { for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i); events(); };
  auto item=[&](const QString &label) {
   const auto snapshot=window.missionSnapshot(); int index=-1;
   for (int i=0;i<snapshot.nodes.size();++i) if (missionCommandLabel(snapshot.nodes[i].statement).name==label) index=i;
   require(index>=0,"Plot deletion named mission node absent");
   for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index) return *it;
   throw std::runtime_error("Plot deletion visible mission item absent");
  };
  QJsonArray actions;
  auto invoke=[&](const QString &label,const QString &action,const std::function<void()> &after=std::function<void()>()) {
   missionTab(); auto *target=item(label); tree->expandAll(); tree->setCurrentItem(target); tree->scrollToItem(target); events(); acceptanceView(*tree);
   std::exception_ptr failure; bool visited=false;
   QTimer::singleShot(0,&window,[&] {
    auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
    try {
     require(menu,"Plot deletion context menu absent"); QAction *selected=nullptr;
     for (auto *a:menu->actions()) if (a->text()==action) selected=a;
     require(selected&&selected->isEnabled(),"Plot deletion context action unavailable");
     acceptanceView(*menu); menu->setActiveAction(selected); visited=true;
     if (after) QTimer::singleShot(0,&window,[&] { try { after(); } catch (...) { failure=std::current_exception(); if (auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget())) dialog->reject(); } });
     QTest::keyClick(menu,Qt::Key_Return);
    } catch (...) { failure=std::current_exception(); if (menu) menu->close(); }
   });
   const auto point=tree->visualItemRect(target).center();
   QContextMenuEvent context(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
   QApplication::sendEvent(tree->viewport(),&context); events();
   if (failure) std::rethrow_exception(failure); require(visited,"Plot deletion context menu was not selected");
   actions.append(action);
  };
  auto resourceWitness=[&] {
   QJsonObject result;
   for (const auto &name:QStringList{"Trace","Control"}) {
    auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());
    require(object&&object->IsOfType("XYPlot"),"Deleting plot command removed referenced resource");
    result[name]=QString::fromStdString(object->GetStringParameter("XVariable"));
   }
   require(result["Trace"]=="Sat.ElapsedSecs"&&result["Control"]=="Sat.ElapsedSecs","Plot deletion changed configured plot references");
   return result;
  };
  const auto resources=resourceWitness();
  QJsonArray protectedResourceDeletions;
  auto rejectReferenceDeletion=[&](const QString &phase) {
   auto *resourcesTree=window.findChild<QTreeWidget *>("Resources"); require(resourcesTree,"Plot deletion resource tree absent");
   for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==resourcesTree||tabs->widget(i)->isAncestorOf(resourcesTree)) tabs->setCurrentIndex(i);
   const auto matches=resourcesTree->findItems("Trace",Qt::MatchExactly|Qt::MatchRecursive); require(matches.size()==1,"Referenced plot resource tree item absent");
   auto *target=matches.first(); resourcesTree->expandAll(); resourcesTree->setCurrentItem(target); resourcesTree->scrollToItem(target); events(); acceptanceView(*resourcesTree);
   const auto before=source->toPlainText(); const int steps=source->document()->availableUndoSteps(); const bool wasModified=source->document()->isModified();
   std::exception_ptr failure; int stage=0; QString rejection; bool selected=false; QTimer modal;
   modal.setInterval(10);
   QObject::connect(&modal,&QTimer::timeout,&window,[&] {
    auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); if (!box) return;
    try {
     if (stage==0&&box->windowTitle()=="Delete resource") {
      stage=1; require(box->button(QMessageBox::Yes),"Referenced plot deletion confirmation lacks Yes"); acceptanceView(*box); QTest::mouseClick(box->button(QMessageBox::Yes),Qt::LeftButton);
     } else if (stage==1&&box->windowTitle()=="Cannot delete resource") {
      stage=2; rejection=box->text(); require(rejection.contains("used by another resource or mission command"),"Referenced plot resource deletion has wrong rejection");
      acceptanceView(*box); require(box->button(QMessageBox::Ok),"Referenced plot deletion rejection lacks OK"); QTest::mouseClick(box->button(QMessageBox::Ok),Qt::LeftButton); modal.stop();
     }
    } catch (...) { failure=std::current_exception(); modal.stop(); box->reject(); }
   });
   QTimer::singleShot(0,&window,[&] {
    auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
    try {
     require(menu,"Referenced plot resource context menu absent"); QAction *remove=nullptr; for (auto *a:menu->actions()) if (a->text()=="Delete resource…") remove=a;
     require(remove&&remove->isEnabled(),"Referenced plot resource delete action unavailable"); acceptanceView(*menu); menu->setActiveAction(remove); selected=true; modal.start(); QTest::keyClick(menu,Qt::Key_Return);
    } catch (...) { failure=std::current_exception(); modal.stop(); if (menu) menu->close(); }
   });
   const auto point=resourcesTree->visualItemRect(target).center(); QContextMenuEvent context(QContextMenuEvent::Mouse,point,resourcesTree->viewport()->mapToGlobal(point)); QApplication::sendEvent(resourcesTree->viewport(),&context); events(); modal.stop();
   if (failure) std::rethrow_exception(failure);
   require(selected&&stage==2&&source->toPlainText()==before&&source->document()->availableUndoSteps()==steps&&source->document()->isModified()==wasModified&&resourceWitness()==resources,"Rejected referenced resource deletion changed source/history/resources");
   protectedResourceDeletions.append(QJsonObject{{"phase",phase},{"rejected",true},{"message",rejection},{"atomic_source_history",true}});
  };
  rejectReferenceDeletion("before-command-delete");
  invoke("Unrelated","Edit command…");
  QPointer<QMdiSubWindow> pending=area->currentSubWindow();
  require(pending&&dynamic_cast<CommandEditor *>(pending->widget()),"Plot deletion pending companion editor absent");
  auto *pendingText=pending->findChild<QPlainTextEdit *>("commandSource");
  require(pendingText,"Plot deletion pending source absent");
  pendingText->setPlainText("'Unrelated' Keep = Keep + 99;");
  const auto pendingValue=pendingText->toPlainText(); acceptanceView(*pending);
  const int undo=source->document()->availableUndoSteps(); const bool modified=source->document()->isModified();
  auto answer=[&](QMessageBox::StandardButton choice) {
   auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
   require(box&&box->windowTitle()=="Delete command"&&box->button(choice),"Plot deletion confirmation absent");
   acceptanceView(*box); QTest::mouseClick(box->button(choice),Qt::LeftButton);
  };
  invoke("DeleteMe","Delete command",[&] { answer(QMessageBox::Cancel); });
  require(source->toPlainText()==initial&&source->document()->availableUndoSteps()==undo&&source->document()->isModified()==modified,"Plot deletion Cancel changed source/history");
  require(pending&&pendingText->toPlainText()==pendingValue&&resourceWitness()==resources,"Plot deletion Cancel changed pending editor or resources");
  invoke("DeleteMe","Delete command",[&] { answer(QMessageBox::Yes); });
  require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()>undo,"Plot deletion differs from independent exact source");
  require(pending&&pendingText->toPlainText()==pendingValue&&resourceWitness()==resources,"Plot deletion lost companion pending edit or resources");
  rejectReferenceDeletion("after-command-delete");
  require(pending&&pendingText->toPlainText()==pendingValue,"Rejected plot resource deletion lost unrelated pending owner");
  auto *box=pending->findChild<QDialogButtonBox *>(); require(box&&box->button(QDialogButtonBox::Apply),"Plot deletion pending Apply absent");
  const int committed=source->document()->availableUndoSteps();
  acceptanceView(*pending); QTest::mouseClick(box->button(QDialogButtonBox::Apply),Qt::LeftButton); events();
  auto *status=pending->findChild<QLabel *>("commandStatus");
  require(status&&status->text().contains("mission has changed",Qt::CaseInsensitive)&&source->toPlainText()==wanted&&source->document()->availableUndoSteps()==committed&&pendingText->toPlainText()==pendingValue,"Stale companion Apply overwrote plot deletion or lost pending values");
  save("stale-owner-status.txt",status->text().toUtf8());
  dynamic_cast<CommandEditor *>(pending->widget())->discardChanges(); pending->close(); events(); require(!pending,"Plot deletion companion did not close after discard");
  source->undo(); require(source->toPlainText()==initial&&window.buildScript()&&resourceWitness()==resources,"Plot deletion exact Undo failed");
  source->redo(); require(source->toPlainText()==wanted&&window.buildScript()&&resourceWitness()==resources,"Plot deletion exact Redo failed");
  const auto saved=files.filePath("Plot deletion "+command+" Δ.script");
  require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==wanted,"Plot deletion Unicode save/load failed");
  save("mission.script",source->toPlainText().toUtf8());
  const auto snapshot=window.missionSnapshot(); QJsonArray hierarchy; int observationNode=-1;
  for (int i=0;i<snapshot.nodes.size();++i) {
   const auto &node=snapshot.nodes[i];
   const auto label=missionCommandLabel(node.statement).name;
   require(label!="DeleteMe","Deleted plot command remains in native mission snapshot");
   if (label=="Unrelated") observationNode=i;
   hierarchy.append(QJsonObject{{"type",node.type},{"name",label},{"statement",node.statement},{"parent",node.parent}});
  }
  require(wanted.contains(reference)&&item("Surviving reference")&&item("Unrelated"),"Plot deletion removed another reference or unrelated command");
  require(observationNode==3,"Plot deletion independent native observation index differs");
  missionTab(); tree->expandAll(); acceptanceView(*tree); save("hierarchy.json",QJsonDocument(hierarchy).toJson());
  auto plot=[&](const QString &name) {
   const auto model=window.plotReceiver()->model(name); require(bool(model)&&model->curves.size()==1,"Plot deletion live XY output absent");
   QJsonArray points; for (const auto &point:model->curves.first().points) points.append(QJsonArray{point.x,point.y,point.connect,point.highlighted});
   return QJsonObject{{"pen_down",model->penDown},{"points",points}};
  };
  auto state=[&] {
   auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));
   auto *keep=Moderator::Instance()->GetInternalObject("Keep"); require(sat&&keep,"Plot deletion native witness absent");
   QJsonArray values{(sat->GetEpoch()-21545)*86400};
   for (const char *axis:{"X","Y","Z","VX","VY","VZ"}) values.append(sat->GetRealParameter(axis));
   values.append(keep->GetRealParameter("Value")); return values;
  };
  auto observe=[&](double time) {
   const auto values=state(); const double a=omega*time;
   const double expected[]={time,radius*std::cos(a),radius*std::sin(a),0,-speed*std::sin(a),speed*std::cos(a),0,41};
   require(values.size()==8,"Plot deletion native witness shape differs");
   for (int i=0;i<8;++i) require(std::isfinite(values[i].toDouble())&&std::abs(values[i].toDouble()-expected[i])<(i==0?1e-4:2e-5),"Plot deletion result differs from independent circular/scalar witness");
   const auto trace=plot("Trace"),control=plot("Control"); const auto tp=trace["points"].toArray(),cp=control["points"].toArray();
   require(cp.size()>=3&&tp.size()==cp.size(),"Deleted plot command still cleared or removed accepted history");
   require(trace["pen_down"].toBool()==(command!="PenDown")&&control["pen_down"].toBool(),"Deleted pen command still changed pen state");
   double last=-1; bool sawZero=false,sawTen=false;
   for (int i=0;i<cp.size();++i) {
    const auto c=cp[i].toArray(),t=tp[i].toArray();
    require(c.size()==4&&t.size()==4&&std::isfinite(c[0].toDouble())&&std::abs(c[0].toDouble()-c[1].toDouble())<1e-10&&c[0].toDouble()>=last-1e-5,"Plot deletion control violates elapsed-time diagonal");
    last=c[0].toDouble(); sawZero=sawZero||std::abs(last)<1e-4; sawTen=sawTen||std::abs(last-10)<1e-4;
    require(t[0]==c[0]&&t[1]==c[1]&&!t[3].toBool()&&!c[3].toBool(),"Deleted MarkPoint still marked data or changed another plot");
    require(command=="PenDown"?!t[2].toBool():t[2]==c[2],"Deleted plot command still changes line connectivity");
   }
   require(sawZero&&sawTen&&std::abs(last-time)<1e-4,"Plot deletion lost early data or final independent epoch");
   return QJsonObject{{"elapsed",time},{"state",values},{"trace",trace},{"control",control}};
  };
  Debugger *debug=nullptr; for (auto *object:window.children()) if (auto *d=dynamic_cast<Debugger *>(object)) debug=d;
  require(debug,"Plot deletion debugger absent");
  for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type!="BeginMissionSequence") require(window.setBreakpoint(i,true),"Plot deletion native breakpoint failed");
  auto previous=debug->paused; QJsonArray nativeCommands,observations; std::exception_ptr failure;
  debug->paused=[&](const QString &type) {
   if (previous) previous(type);
   try {
    auto *dialog=window.findChild<QDialog *>("debuggerDialog"); auto *text=dialog?dialog->findChild<QPlainTextEdit *>("debuggerCommand"):nullptr;
    require(dialog&&dialog->isVisible()&&text,"Plot deletion native debugger view absent"); acceptanceView(*dialog);
    const auto literal=text->toPlainText(); require(!literal.contains("DeleteMe"),"Deleted command still executed in native mission");
    const int node=dialog->property("commandIndex").toInt();
    nativeCommands.append(QJsonObject{{"type",type},{"command",literal},{"node",node}});
    if (node==observationNode) {
     require(type=="GMAT"&&literal=="Keep = Keep + 0;","Plot deletion native observation boundary has wrong command");
     observations.append(observe(20));
    }
   } catch (...) { failure=std::current_exception(); QTimer::singleShot(0,&window,[&] { window.stopMission(); }); }
   QTimer::singleShot(0,&window,[&] { debug->resume(); });
  };
  QTimer deadline; deadline.setSingleShot(true); QObject::connect(&deadline,&QTimer::timeout,&window,[&] { failure=std::make_exception_ptr(std::runtime_error("Plot deletion observer timed out")); window.stopMission(); });
  const int runUndo=source->document()->availableUndoSteps(); messages->clear(); deadline.start(60000);
  const auto result=window.runMission(); deadline.stop(); debug->paused=previous;
  save("messages.txt",messages->toPlainText().toUtf8()); if (failure) std::rethrow_exception(failure);
  save("execution-diagnostic.json",QJsonDocument(QJsonObject{{"result",int(result)},{"native_commands",nativeCommands},{"observations",observations}}).toJson());
  require(result==MainWindow::RunResult::Completed&&nativeCommands.size()==4&&observations.size()==1,"Plot deletion native mission execution differs");
  const QStringList expectedCommands{reference.trimmed(),"Propagate 'FirstArc' P(Sat) {Sat.ElapsedSecs = 20};","Keep = Keep + 0;","Propagate 'LastArc' P(Sat) {Sat.ElapsedSecs = 20};"};
  for (int i=0;i<expectedCommands.size();++i) require(nativeCommands[i].toObject()["command"]==expectedCommands[i]&&nativeCommands[i].toObject()["node"].toInt()==i+1,"Plot deletion native command sequence differs from independent literals");
  observations.append(observe(40)); save("observations.json",QJsonDocument(observations).toJson()); save("native-commands.json",QJsonDocument(nativeCommands).toJson());
  for (const auto &name:QStringList{"Trace","Control"}) {
   const auto before=plot(name); require(window.plotReceiver()->show(name),"Plot deletion output viewer absent"); events();
   auto *child=area->currentSubWindow(); auto *view=child?dynamic_cast<PlotWidget *>(child->widget()):nullptr;
   require(view&&child->property("plotName")==name,"Plot deletion selected viewer identity differs");
   child->showNormal(); view->refresh(); events(); acceptanceView(*child); require(plot(name)==before,"Plot deletion viewer changed retained data");
  }
  require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==runUndo&&!window.isRunning()&&resourceWitness()==resources,"Plot deletion run changed applied source/history/resources");
  const QJsonObject proof{{"checkpoint","plot-reference-deletion-"+command},{"outcome","passed"},{"family",command},{"numeric_sentinel",41},{"actions",actions},{"cancel_atomic",true},{"exact_delete",true},{"stale_companion_rejected",true},{"undo_redo",true},{"unicode_reopen",true},{"resources",resources},{"protected_resource_deletions",protectedResourceDeletions},{"hierarchy",hierarchy},{"native_commands",nativeCommands},{"observations",observations},{"facets",QJsonArray{"actual-tree-delete-cancel-confirmation","referenced-xyplot-and-companion-pending-owner-survive","referenced-resource-deletion-rejected-before-and-after","stale-apply-atomicity-exact-source-history-unicode","native-deleted-command-absence-and-surviving-reference","independent-circle-full-history-marker-and-connectivity"}},{"scope","One named ClearPlot/MarkPoint/PenUp/PenDown deleted through the real Qt context menu while a second native command still references Trace. Deleting Trace through its own resource menu is rejected atomically before and after command deletion. Cancel, stale unrelated pending editor, source/history/reopen, native command order, 20/40s circular state and two XY histories/marker/pen/connectivity checked. No successful deletion of referenced resources, arbitrary branch families, original unavailable keyword payloads, native OS input/focus, rendering pixels, platform/hardware/stress or full-current-build qualification."}};
  save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
  return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
 catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
 return 1;
}
