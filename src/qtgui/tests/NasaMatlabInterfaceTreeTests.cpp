#include "FormationAcceptance.hpp"
#include "MatlabAcceptanceStartup.hpp"
#include "MatlabInterface.hpp"
#include "ReportViewer.hpp"
#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QRegularExpression>
#include <QTabBar>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
using namespace formaccept;
static QString quoted(QString s){return "'"+s.replace("'","''")+"'";}

int main(int argc,char **argv)
{
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
 app.setOrganizationName("GMATTests");app.setApplicationName("NasaMatlabInterfaceTree");
 try {
  require(argc==4,"Supply startup,MATLAB plugin and interface availability profile");
  const QString profile=argv[3];require(profile=="Available"||profile=="Unavailable","Unknown interface availability profile");
  const bool available=profile=="Available";QTemporaryDir runtime;require(runtime.isValid(),"Interface private runtime failed");
  const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),runtime.path(),QFileInfo(argv[2]).absoluteFilePath());
  if(!available){auto text=QString::fromUtf8(read(startup));const QRegularExpression plugin("^[ \\t]*PLUGIN[ \\t]*=[^\\n]*MatlabInterface[^\\n]*$",QRegularExpression::MultilineOption);
   require(text.contains(plugin),"Actual provider startup line missing");text.replace(plugin,"# MATLAB provider disabled for this actual startup");write(startup,text.toUtf8());}
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!evidence.isEmpty())write(evidence+"/startup-before-initialize.txt",read(startup));
  Rig rig(startup);rig.save("startup.txt",read(startup));
  const auto function=rig.files.filePath("TreeEcho.m"),saved=rig.files.filePath("Interface caller Ω.script");
  const QString body="function out = TreeEcho(x)\nout = 3*x + evalin('base','gmat_tree_marker');\nend\n";
  write(function,body.toUtf8());rig.save("TreeEcho.m",body.toUtf8());
  const QString script="% Resource Tree interface witness Ω\nCreate Variable Keep;\nKeep = 41;\n"+
   (available?"Create MatlabFunction TreeEcho;\nTreeEcho.FunctionPath = "+quoted(function)+";\n":QString())+
   "Create ReportFile Values;\nValues.Filename = "+quoted(rig.report)+";\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nBeginMissionSequence;\n"+
   (available?"[Keep] = TreeEcho(Keep);\n":"Keep = Keep + 1;\n")+"Report Values Keep;\n% Preserve interface caller tail Ω\n";
  rig.source->setPlainText(script);require(rig.window.buildScript(),"Interface witness Build failed");require(rig.window.saveScriptTo(saved),"Interface Unicode caller Save failed");rig.save("caller.script",script.toUtf8());
  auto locate=[&] {QTreeWidgetItem *found=nullptr;for(QTreeWidgetItemIterator it(rig.resources);*it;++it)if((*it)->data(0,Qt::UserRole+2).toString()=="matlab-interface") {require(!found,"Ambiguous runtime MATLAB interface node");found=*it;}return found;};
  QJsonArray census;auto groups=rig.resources->findItems("Interfaces",Qt::MatchExactly|Qt::MatchRecursive);require(groups.size()==1,"Actual Interfaces category absent");
  for(int i=0;i<groups.first()->childCount();++i){auto *item=groups.first()->child(i);census.append(QJsonObject{{"name",item->text(0)},{"configured_name",item->data(0,Qt::UserRole).toString()},{"runtime_kind",item->data(0,Qt::UserRole+2).toString()}});}
  rig.save("capability.json",QJsonDocument(QJsonObject{{"Matlab_available",GmatGlobal::Instance()->IsMatlabAvailable()},{"native_interface_present",bool(Moderator::Instance()->GetMatlabInterface())},{"children",census}}).toJson());
  require(GmatGlobal::Instance()->IsMatlabAvailable()==available,"Actual startup/provider availability differs");
  if(!available){require(!locate()&&!Moderator::Instance()->GetMatlabInterface(),"Unavailable provider exposed a runtime MATLAB route");acceptanceView(rig.window);require(rig.window.runMission()==MainWindow::RunResult::Completed,"Unavailable-provider ordinary mission failed");
   require(read(rig.report)=="42\n"&&rig.source->toPlainText()==script&&read(saved)==script.toUtf8(),"Unavailable-provider control mission/source differs");
   const QJsonObject proof{{"checkpoint","matlab-interface-tree-unavailable"},{"actual_provider_unregistered",true},{"runtime_node_absent",true},{"ordinary_mission_result",42},{"new_requirement_credit",0}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;}
  require(locate(),"Actual Resource Tree MATLAB interface node missing");
  require(locate()->parent()==groups.first()&&locate()->data(0,Qt::UserRole).toString().isEmpty(),"Runtime MATLAB interface masquerades as a configured resource");
  auto *matlab=MatlabInterface::Instance();require(!matlab->IsOpen(),"Interface witness unexpectedly opened MATLAB before its menu action");
  QJsonArray actions,pids;int busyChecks=0;std::exception_ptr busyFailure;QTimer heartbeat;heartbeat.setInterval(20);
  QObject::connect(&heartbeat,&QTimer::timeout,&rig.window,[&] {if(!rig.window.isRunning())return;try {
   require(!rig.resources->isEnabled()&&rig.source->isReadOnly()&&!rig.window.buildScript()&&rig.window.runMission()==MainWindow::RunResult::Busy,"Interface connection action permitted engine/UI reentry");++busyChecks;
  }catch(...){busyFailure=std::current_exception();}});heartbeat.start();
  auto menu=[&](const QString &operation) {
   auto *item=locate();require(item,"Runtime MATLAB node lost after model refresh");
   for(auto *tabs:rig.window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->tabText(i)=="Resources"){tabs->setCurrentIndex(i);events();}
   rig.resources->expandAll();rig.resources->scrollToItem(item);rig.resources->setCurrentItem(item);events();
   const auto before=rig.source->toPlainText();const int history=rig.source->document()->availableUndoSteps();
   std::exception_ptr failure;bool seen=false;QTimer timer;timer.setInterval(20);
   QObject::connect(&timer,&QTimer::timeout,&rig.window,[&] {auto *popup=qobject_cast<QMenu *>(QApplication::activePopupWidget());if(!popup)return;timer.stop();try {
    require(popup->objectName()=="resourceContextMenu","Wrong interface popup");auto *open=popup->findChild<QAction *>("openMatlabInterface");auto *close=popup->findChild<QAction *>("closeMatlabInterface");
    require(open&&close&&open->isEnabled()&&close->isEnabled()&&popup->actions().size()==2,"Actual MATLAB interface Open/Close choices differ");
    seen=true;acceptanceView(*popup);if(operation=="Cancel")QTest::keyClick(popup,Qt::Key_Escape);else QTest::mouseClick(popup,Qt::LeftButton,Qt::NoModifier,popup->actionGeometry(operation=="Open"?open:close).center());
   }catch(...){failure=std::current_exception();popup->close();}});
   timer.start();const auto point=rig.resources->visualItemRect(item).center();QContextMenuEvent event(QContextMenuEvent::Mouse,point,rig.resources->viewport()->mapToGlobal(point));QApplication::sendEvent(rig.resources->viewport(),&event);timer.stop();events();if(failure)std::rethrow_exception(failure);if(busyFailure)std::rethrow_exception(busyFailure);
   require(seen&&rig.source->toPlainText()==before&&rig.source->document()->availableUndoSteps()==history&&read(saved)==script.toUtf8(),"Interface popup/action changed source/history/saved caller");
   actions.append(QJsonObject{{"operation",operation},{"actual_popup",true},{"source_unchanged",true},{"history_unchanged",true}});rig.save("actions.json",QJsonDocument(actions).toJson());
  };
  auto state=[&](int marker) {
   matlab->RunMatlabString("gmat_tree_state = [feature('getpid') exist('gmat_tree_marker','var')];");double values[2];Integer rows=0,cols=0;require(matlab->GetRealArray("gmat_tree_state",2,values,rows,cols)==2&&rows==1&&cols==2&&values[0]>1&&values[0]==std::floor(values[0])&&values[1]==marker,"Actual MATLAB process/workspace state differs");return values[0];
  };
  menu("Cancel");require(!matlab->IsOpen(),"Canceled interface popup opened MATLAB");menu("Close");require(!matlab->IsOpen(),"Close on inactive interface opened MATLAB");
  menu("Open");require(matlab->IsOpen(),"Actual interface Open did not connect the Engine");const double first=state(0);pids.append(first);
  matlab->RunMatlabString("gmat_tree_marker = 41;");menu("Open");require(state(1)==first,"Repeated interface Open replaced its actual process/workspace");
  rig.source->appendPlainText("% Pending interface caller draft Ω");const auto dirty=rig.source->toPlainText();menu("Open");require(state(1)==first&&rig.source->toPlainText()==dirty,"Dirty-source interface action altered the draft/connection");
  menu("Close");require(!matlab->IsOpen(),"Actual interface Close left the Engine open");menu("Close");require(!matlab->IsOpen(),"Repeated interface Close changed its inactive state");
  rig.source->undo();require(rig.source->toPlainText()==script&&rig.window.buildScript(),"Interface draft Undo/Build differs");
  menu("Open");require(matlab->IsOpen(),"Interface reopen failed");const double second=state(0);require(second!=first,"Interface Close/reopen reused the terminated process");pids.append(second);matlab->RunMatlabString("gmat_tree_marker = 7;");
  const int history=rig.source->document()->availableUndoSteps();acceptanceView(rig.window);require(rig.window.runMission()==MainWindow::RunResult::Completed,"GUI-opened Engine did not serve the actual MATLAB mission");events();
  const auto raw=read(rig.report);rig.save("report.csv",raw);require(raw=="130\n"&&Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==130&&state(1)==second,"Actual mission did not use the GUI-opened MATLAB workspace");
  auto *output=rig.window.findChild<QTreeWidget *>("Output");const auto reports=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"Interface mission Output report missing");output->itemDoubleClicked(reports.first(),0);events();
  auto *viewer=rig.area->activeSubWindow()?dynamic_cast<ReportViewer *>(rig.area->activeSubWindow()->widget()):nullptr;auto *text=viewer?viewer->findChild<QPlainTextEdit *>():nullptr;
  require(text&&text->isReadOnly()&&text->toPlainText().trimmed()=="130","Actual interface mission ReportViewer differs");acceptanceView(*rig.area->activeSubWindow());rig.area->activeSubWindow()->close();events();
  require(rig.source->toPlainText()==script&&rig.source->document()->availableUndoSteps()==history&&read(saved)==script.toUtf8()&&read(function)==body.toUtf8(),"Interface mission changed source/history/external files");
  require(rig.window.loadScript(saved)&&rig.window.buildScript()&&locate(),"Unicode caller reopen lost the runtime interface route");menu("Open");require(state(1)==second,"Caller reopen replaced the connected Engine");
  menu("Close");require(!matlab->IsOpen()&&busyChecks>=3&&!rig.window.isRunning()&&rig.resources->isEnabled()&&!rig.source->isReadOnly(),"Interface lifecycle/busy restoration failed");heartbeat.stop();
  const QJsonObject proof{{"checkpoint","matlab-interface-tree-available"},{"actions",actions},{"actual_MATLAB_pids",pids},{"busy_reentry_checks",busyChecks},{"actual_MATLAB_call_result",130},{"same_engine_workspace_used",true},{"actual_ReportViewer",true},{"Unicode_reopen",true},{"Engine_closed",true},{"new_requirement_credit",0}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
