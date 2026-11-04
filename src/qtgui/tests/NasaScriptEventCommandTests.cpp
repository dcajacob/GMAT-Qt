// Reuse the original event helpers and preserve that driver's assertions.
#define main retainedScriptEventMain
#include "NasaScriptEventTests.cpp"
#undef main
#include "FileManager.hpp"
#include "FiniteBurn.hpp"
#include "QtPlotReceiver.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "Spacecraft.hpp"
#include "GmatCommand.hpp"
#include "Debugger.hpp"
#include "Sandbox.hpp"
#include "GmatGlobal.hpp"
#include "ScriptStatements.hpp"
#include "PropSetup.hpp"
#include "ODEModel.hpp"
#include "PhysicalModel.hpp"
#include <QTableWidget>
#include <QStatusBar>
#include <QDateTime>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QPointer>
#include <functional>
#include <algorithm>
#include <set>

int main(int argc, char **argv)
{
 QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
 app.setOrganizationName("GMATTests"); app.setApplicationName("NasaScriptEventCommands");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Supply startup, existing command fixture and family");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath();
  const auto fixture=read(QFileInfo(argv[2]).absoluteFilePath());
  const auto spec=QJsonDocument::fromJson(fixture).object(); const QString family=argv[3];
  require(spec["family"]==family,"Wrong ScriptEvent command fixture");
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Event fixture directory unavailable");
  QDir::setCurrent(QFileInfo(startup).absolutePath());
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
  auto save=[&](const QString &name,const QByteArray &bytes){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),bytes);};
  constexpr double radius=7000,mu=398600.4415;
  const double speed=std::sqrt(mu/radius),omega=std::sqrt(mu/(radius*radius*radius));
  const auto report=files.filePath("values.csv"),extra=files.filePath("extra.csv"),missionSave=files.filePath("native mission Δ.script");
  auto expand=[&](QString value){
   for(const auto &p:QList<QPair<QString,QString>>{{"@SPEED@",QString::number(speed,'g',17)},
       {"@VALUES@",report},{"@EXTRA@",extra},{"@DC@",files.filePath("dc.txt")},{"@OPT@",files.filePath("opt.txt")},
       {"@MISSION@",missionSave},{"@FUNCTION@",files.filePath("Accept.gmf")},
       {"@INPUT@",files.filePath("input.sv")},{"@HISTORY@",files.filePath("history.thf")},
       {"@EVENT@",files.filePath("events.txt")},{"@OBS@",files.filePath("navigation.gmd")},
       {"@FILTER@",files.filePath("filter.txt")},{"@SMOOTHER@",files.filePath("smoother.txt")},
       {"@WARM@",files.filePath("warm.csv")},{"@SCRATCH@",files.filePath("unused.csv")},
       {"@DATA_ROOT@",QFileInfo(startup).absoluteDir().absoluteFilePath("../data")}})value.replace(p.first,p.second);
   require(!value.contains('@'),"Unresolved additional event fixture placeholder");return value;
  };
  for(const auto &v:spec["support_files"].toArray()){
   const auto object=v.toObject();const auto name=object["name"].toString();
   require(!name.isEmpty()&&QFileInfo(name).fileName()==name&&name!="."&&name!="..","Unsafe event support filename");
   const auto content=expand(object["text"].toString()).toUtf8();write(files.filePath(name),content);save("support-"+name,content);
  }
  QString plain=spec["script"].toString();
  plain=expand(plain);
  const bool markerControl=spec["marker_plain_control"].toBool();
  const bool uniqueStarter=spec["unique_starter"].toBool();
  if(uniqueStarter)require(markerControl&&(family=="BeginMissionSequence"||family=="PrepareMissionSequence")&&!spec["duplicate_starter"].toBool()&&spec["expected_prepared"].toBool()==(family=="PrepareMissionSequence"),"Unexpected unique-starter contract");
  if(markerControl)require(QStringList{"NoOp","BeginMissionSequence","PrepareMissionSequence"}.contains(family)&&spec["command"].toString()==family,"Unexpected marker plain control contract");
  require(!plain.contains('@')&&plain.count("BeginMissionSequence;")==(uniqueStarter?(family=="BeginMissionSequence"?1:0):(markerControl&&family=="BeginMissionSequence"?2:1)),"Command fixture placeholders or sequence boundary differ");
  const auto physical=spec["physical_inputs"].toObject();
  for(auto it=physical.begin();it!=physical.end();++it){require(it.key().startsWith("application/data/"),"Unknown physical input root");
   const auto path=QFileInfo(startup).absoluteDir().absoluteFilePath("../data/"+it.key().mid(QString("application/data/").size()));
   require(QString::fromLatin1(QCryptographicHash::hash(read(path),QCryptographicHash::Sha256).toHex())==it.value().toString(),"Contact oracle physical input changed");
  }
  const auto boundary=uniqueStarter?family+";":QString("BeginMissionSequence;");
  require(plain.indexOf(boundary)>=0,"Missing explicit starter boundary");
  const auto split=plain.indexOf(boundary)+(uniqueStarter?0:boundary.size());
  const auto setup=plain.left(split)+"\n",body=plain.mid(split).trimmed()+"\n";
  const QString before="BeginScript 'Event'; % fixed begin\n"+(uniqueStarter?boundary+"\n":QString())+"Keep = Keep; % retained baseline\nEndScript; % fixed end\n";
  const QString after="BeginScript 'Event'; % fixed begin\n"+body+"EndScript; % fixed end\n";
  const QString suffix=spec.contains("suffix")?expand(spec["suffix"].toString()):QString("% Retain outside event sentinel Ω\n");
  const auto baseline=setup+before+suffix,wanted=setup+after+suffix;
  MainWindow window; require(window.initialize(startup),"Event runtime unavailable"); window.show(); events(); acceptanceView(window);
  FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",files.path().toStdString());
  require(std::abs(Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant()-mu)<1e-9,"Independent circle gravity differs");
  QJsonArray factory; for(const auto &n:Moderator::Instance()->GetListOfFactoryItems(Gmat::COMMAND))factory.append(QString::fromStdString(n));
  save("factory-commands.json",QJsonDocument(factory).toJson()); save("case.json",fixture);
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");
  auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");
  require(source&&messages&&tree&&area,"Event command workspace unavailable");
  if(spec["duplicate_starter"].toBool()||spec["unsupported_definition"].toBool()){
   const bool definition=spec["unsupported_definition"].toBool();
   require(definition?(family=="Create"&&spec["command"].toString()=="Create"&&!spec["duplicate_starter"].toBool()):
    (markerControl&&(family=="BeginMissionSequence"||family=="PrepareMissionSequence")),"Unexpected context rejection contract");
   const QString reason=definition?"GMAT currently requires that all objects are created before the mission sequence begins":"Too many Mission Sequence start";
   QJsonArray rejections;
   for(const auto &pair:QList<QPair<QString,QString>>{{"plain",plain+suffix},{"grouped",wanted}}){
    messages->clear();source->setPlainText(pair.second);const auto initialUndo=source->document()->availableUndoSteps();
    const bool built=window.buildScript();events();acceptanceView(window);
    const auto diagnostic=messages->toPlainText();save(pair.first+"-rejected-source.script",pair.second.toUtf8());save(pair.first+"-rejection.txt",diagnostic.toUtf8());
    require(!built&&diagnostic.contains(reason),"Context did not reject for the specific native rule");
    require(source->toPlainText()==pair.second&&source->document()->availableUndoSteps()==initialUndo&&!QFileInfo::exists(report),"Rejected duplicate starter changed source/history or executed following report");
    rejections.append(QJsonObject{{"context",pair.first},{"build_accepted",built},{"diagnostic",diagnostic}});
   }
   const QJsonObject proof{{"checkpoint","script-event-context-rejection-"+family},{"outcome","passed"},{"family",family},{"rejections",rejections},
    {"fixture_sha256",QString::fromLatin1(QCryptographicHash::hash(fixture,QCryptographicHash::Sha256).toHex())},
    {"scope","Specific native context rejection parity only; legal unique-starter applicability remains open; zero whole FRC!E138 credit."}};
   save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
  }
  QByteArray plainSaved,markerPlainReport;
  auto preparedState=[&](){
   require(uniqueStarter&&family=="PrepareMissionSequence"&&!QFileInfo::exists(report),"Preparation executed the following report");
   auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));
   require(sat&&std::abs(sat->GetEpoch()-21545)<1e-11,"Preparation advanced epoch or omitted sandbox spacecraft");
   const char *axes[]={"X","Y","Z","VX","VY","VZ"};const double expected[]={radius,0,0,0,speed,0};QJsonArray state;
   for(int i=0;i<6;++i){const auto v=sat->GetRealParameter(axes[i]);require(std::isfinite(v)&&std::abs(v-expected[i])<1e-10,"Preparation changed initial Cartesian state");state.append(v);}
   auto *count=Moderator::Instance()->GetInternalObject("Count");require(count&&count->GetRealParameter("Value")==0,"Preparation executed following assignment");
   return QJsonObject{{"epoch",sat->GetEpoch()},{"cartesian",state},{"count",count->GetRealParameter("Value")},{"report_exists",false}};
  };
  QJsonObject plainPrepared,nativePrepared;
  if(markerControl){
   source->setPlainText(plain+suffix);save("plain-control-source.script",(plain+suffix).toUtf8());
   const bool built=window.buildScript();events();save("plain-control-messages.txt",messages->toPlainText().toUtf8());if(!built)std::cerr<<messages->toPlainText().toStdString();
   require(built,"Plain marker control Build failed");acceptanceView(window);
   require(!QFileInfo::exists(report)&&window.runMission()==MainWindow::RunResult::Completed,"Plain marker control did not complete");
   if(uniqueStarter&&spec["expected_prepared"].toBool()){
    plainPrepared=preparedState();save("plain-prepared.json",QJsonDocument(plainPrepared).toJson());
    QString nativeSource;for(const auto &line:(plain+suffix).split('\n'))if(!line.trimmed().startsWith('%'))nativeSource+=line+"\n";
    const auto guiStatements=scriptStatements(plain+suffix),nativeStatements=scriptStatements(nativeSource);
    require(guiStatements.size()==nativeStatements.size(),"Native ASCII control changed statement count");
    for(int i=0;i<guiStatements.size();++i)require(guiStatements[i].code.trimmed()==nativeStatements[i].code.trimmed(),"Native ASCII control changed executable statement");
    for(const auto ch:nativeSource)require(ch.unicode()<128,"Native filename control contains non-ASCII executable text");
    const auto nativePath=files.filePath("native preparation control.script");write(nativePath,nativeSource.toUtf8());save("native-filename-source.script",nativeSource.toUtf8());
    const bool nativeBuilt=Moderator::Instance()->InterpretScript(nativePath.toStdString());const auto nativeStatus=nativeBuilt?Moderator::Instance()->RunMission():-99;events();
    save("native-filename-control.json",QJsonDocument(QJsonObject{{"built",nativeBuilt},{"run_status",nativeStatus},{"messages",messages->toPlainText()}}).toJson());
    if(!nativeBuilt||nativeStatus!=1)std::cerr<<messages->toPlainText().toStdString();require(nativeBuilt&&nativeStatus==1,"Native filename preparation control failed");
    nativePrepared=preparedState();save("native-filename-prepared.json",QJsonDocument(nativePrepared).toJson());require(nativePrepared==plainPrepared,"GUI stream and native filename preparation differ");
   }else{
    markerPlainReport=read(report);save("plain-control-results.csv",markerPlainReport);
    require(QFile::remove(report)&&!QFileInfo::exists(report),"Plain marker control left stale output");
   }
  }
  if(spec["plain_control"].toBool()){
   require(family=="SaveMissionTerminal"&&spec["mission_save"].toBool()&&spec["expect_saved"].toBool(),"Unexpected plain control contract");
   const auto mainSave=files.filePath("plain native mission Δ.script");auto controlSource=plain+suffix;controlSource.replace(missionSave,mainSave);
   source->setPlainText(controlSource);require(window.buildScript(),"Plain SaveMission control Build failed");acceptanceView(window);
   require(!QFileInfo::exists(mainSave)&&window.runMission()==MainWindow::RunResult::Completed,"Plain terminal SaveMission did not complete");
   plainSaved=read(mainSave);require(plainSaved.contains("Create Spacecraft Sat")&&plainSaved.contains("BeginMissionSequence")&&plainSaved.contains("Propagate")&&plainSaved.contains("SaveMission"),"Plain terminal SaveMission did not export mission");
   save("plain-control-source.script",controlSource.toUtf8());save("plain-control-export.script",plainSaved);save("plain-control-results.csv",read(report));
   require(QFile::remove(report)&&!QFileInfo::exists(report)&&!QFileInfo::exists(missionSave),"Plain control left stale grouped output");
   if(QFileInfo::exists(extra))require(QFile::remove(extra),"Cannot clear plain control extra report");
  }
  source->setPlainText(baseline);save("baseline.script",baseline.toUtf8());const bool baselineBuilt=window.buildScript();
  save("baseline-messages.txt",messages->toPlainText().toUtf8());if(!baselineBuilt)std::cerr<<messages->toPlainText().toStdString();require(baselineBuilt,"Event baseline Build failed");
  int openCount=0;auto open=[&]() -> QPointer<QMdiSubWindow> {
   auto *tabs=window.findChild<QTabWidget *>();for(int i=0;tabs&&i<tabs->count();++i)if(tabs->tabText(i)=="Mission")tabs->setCurrentIndex(i);
   const auto snapshot=window.missionSnapshot();int index=-1;
   for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].type=="BeginScript"){
    require(index<0&&snapshot.nodes[i].children.isEmpty(),"Event contents leaked into grouped Mission nodes");index=i;
   }
   QJsonArray mapping;for(const auto &node:snapshot.nodes)mapping.append(QJsonObject{{"type",node.type},{"statement",node.statement},{"editable",node.editable},{"start",double(node.start)},{"end",double(node.end)}});
   save("open-"+QString::number(++openCount)+"-mapping.json",QJsonDocument(QJsonObject{{"nodes",mapping},{"source",snapshot.sourceScript},{"canonical",snapshot.canonicalScript}}).toJson());
   require(index>=0,"Event grouping absent");QTreeWidgetItem *item=nullptr;
   for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index)item=*it;
   require(item&&item->childCount()==0,"Event must have one grouped tree icon");tree->setCurrentItem(item);tree->scrollToItem(item);events();acceptanceView(window);tree->itemDoubleClicked(item,0);events();
   save("open-status.txt",window.statusBar()->currentMessage().toUtf8());
   auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Actual event editor absent");
   child->showNormal();child->move(0,0);child->resize(area->viewport()->size());events();acceptanceView(*child);return child;
  };
  auto child=open();auto *panel=dynamic_cast<CommandEditor *>(child->widget());
  auto *preview=panel->findChild<QPlainTextEdit *>("commandSource");auto *buttons=panel->findChild<QDialogButtonBox *>();
  require(preview&&buttons,"Event panel controls absent");
  auto edit=[&](bool accept){bool visited=false;std::exception_ptr failure;
   QTimer::singleShot(0,child,[&]{auto *dialog=dynamic_cast<ScriptEventDialog *>(QApplication::activeModalWidget());
    try{require(dialog,"Actual event body dialog absent");visited=true;
     require(dialog->findChild<QLabel *>("scriptEventBegin")->text()=="BeginScript 'Event'; % fixed begin"&&dialog->findChild<QLabel *>("scriptEventEnd")->text()=="EndScript; % fixed end","Fixed event boundaries changed");
     dialog->findChild<QPlainTextEdit *>("scriptEventBody")->setPlainText(body);events();acceptanceView(*dialog);
     dialog->findChild<QDialogButtonBox *>()->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel)->click();
    }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});
   auto *button=panel->findChild<QPushButton *>("editScriptEvent");require(button&&button->isEnabled(),"Body dialog action unavailable");button->click();if(failure)std::rethrow_exception(failure);require(visited,"Event body dialog unvisited");events();};
  const auto originalDraft=preview->toPlainText();const auto undo=source->document()->availableUndoSteps();
  edit(false);require(preview->toPlainText()==originalDraft&&!panel->hasChanges()&&source->toPlainText()==baseline&&source->document()->availableUndoSteps()==undo,"Event command Cancel changed source/draft/history");
  edit(true);require(panel->hasChanges()&&source->toPlainText()==baseline&&preview->toPlainText().trimmed()==after.trimmed(),"Pending event lost contained commands");
  buttons->button(QDialogButtonBox::Apply)->click();events();save("apply-messages.txt",messages->toPlainText().toUtf8());
  panel=dynamic_cast<CommandEditor *>(child->widget());require(panel,"Applied event editor absent");
  preview=panel->findChild<QPlainTextEdit *>("commandSource");buttons=panel->findChild<QDialogButtonBox *>();require(preview&&buttons,"Applied event controls absent");
  require(source->toPlainText()==wanted&&!panel->hasChanges(),"Event command Apply failed or changed unrelated source");
  const auto appliedUndo=source->document()->availableUndoSteps();buttons->button(QDialogButtonBox::Apply)->click();events();
  require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==appliedUndo,"Repeated Apply changed source/history");
  require(child->close(),"Clean event editor did not close");events();
  source->undo();require(source->toPlainText()==baseline&&window.buildScript(),"Event command Undo failed");
  source->redo();require(source->toPlainText()==wanted&&window.buildScript(),"Event command Redo failed");
  const auto path=files.filePath("Event commands Δ.script"),copy=files.filePath("Event commands copy ü.script");
  require(window.saveScriptTo(path)&&read(path)==wanted.toUtf8()&&window.saveScriptTo(copy)&&read(copy)==wanted.toUtf8()&&window.loadScript(copy)&&window.buildScript()&&source->toPlainText()==wanted,"Event command Unicode Save/SaveAs/reopen changed source");
  child=open();require(child->close(),"Reopened event editor failed to close");events();save("accepted.script",wanted.toUtf8());
  QJsonArray callRejections,starterMappingRejections;
  if(uniqueStarter){
   QStringList changedSources;auto differentLabel=wanted;differentLabel.replace("BeginScript 'Event'","BeginScript 'Different event'");changedSources.append(differentLabel);
   auto differentArithmetic=wanted;differentArithmetic.replace("Count = Count * 7;","Count = Count * 8;");changedSources.append(differentArithmetic);
   auto additionalBoundary=wanted;additionalBoundary.replace("Count = 3;",family+";\nCount = 3;");changedSources.append(additionalBoundary);
   for(const auto &changed:changedSources){require(changed!=wanted,"Starter mapping rejection mutation absent");
    const auto mapped=snapshotMission(Moderator::Instance()->GetFirstCommand(),window.missionSnapshot().canonicalScript,changed);
    int found=0;for(const auto &node:mapped.nodes)if(node.type=="BeginScript"){++found;require(!node.editable,"Different event label/arithmetic/boundary mapped as equivalent");}
    require(found==1&&source->toPlainText()==wanted,"Starter mapping rejection lost group or changed source");starterMappingRejections.append(changed);
   }save("starter-mapping-rejections.json",QJsonDocument(starterMappingRejections).toJson());
  }
  if(family=="CallBuiltinGmatFunction"){
   const QString originalCall="Product = QuatHelper(Qx, Qy);";require(wanted.count(originalCall)==1,"Builtin call mapping target ambiguous");
   for(const auto &changed:QStringList{"Product = OtherHelper(Qx, Qy);","Count = QuatHelper(Qx, Qy);",
       "Product = QuatHelper(Qy, Qx);","'Different label' Product = QuatHelper(Qx, Qy);",
       "Product = QuatHelper;","Product = Sat(Qx, Qy);"}){
    auto unsafe=wanted;unsafe.replace(originalCall,changed);
    const auto mapped=snapshotMission(Moderator::Instance()->GetFirstCommand(),window.missionSnapshot().canonicalScript,unsafe);
    int eventsFound=0;for(const auto &node:mapped.nodes)if(node.type=="BeginScript"){++eventsFound;require(!node.editable,"Different builtin callee/output/order/label/input mapped as equivalent");}
    require(eventsFound==1&&source->toPlainText()==wanted,"Builtin mapping rejection lost group or changed source");callRejections.append(changed);
   }
   save("call-mapping-rejections.json",QJsonDocument(callRejections).toJson());
  }
  // Inspect the actual native command graph, including branch children and closing nodes.
  QJsonArray native,contained;std::set<GmatCommand *> seen;
  std::function<void(GmatCommand *,GmatCommand *,int)> walk=[&](GmatCommand *cmd,GmatCommand *owner,int depth){
   while(cmd&&cmd!=owner&&seen.insert(cmd).second){const auto type=QString::fromStdString(cmd->GetTypeName());
    if(type=="BeginScript")++depth;native.append(type);if(depth>0)contained.append(type);
    for(int i=0;auto *branch=cmd->GetChildCommand(i);++i)walk(branch,cmd,depth);
    if(type=="EndScript"){require(depth>0,"Native event closing marker crossed owner");--depth;}
    cmd=cmd->GetNextWhileExecuting();
   }
  };
  walk(Moderator::Instance()->GetFirstCommand(),nullptr,0);save("native-commands.json",QJsonDocument(QJsonObject{{"all",native},{"inside_event",contained}}).toJson());
  require(contained.contains("BeginScript")&&contained.contains("EndScript")&&contained.contains(spec["command"].toString()),"Native command not contained inside event markers");
  for(const auto &type:spec["required_types"].toArray())require(contained.contains(type),"Required backend command missing inside event");
  QJsonObject backendInputs;
  if(family=="Navigation"){
   auto *model=dynamic_cast<ODEModel *>(Moderator::Instance()->GetConfiguredObject("NavigationFM"));
   auto *prop=dynamic_cast<PropSetup *>(Moderator::Instance()->GetConfiguredObject("NavProp"));
   require(model&&prop&&prop->GetODEModel(),"Navigation force definitions absent");
   for(const auto &pair:QList<QPair<QString,ODEModel *>>{{"configured",model},{"propagator_owned",prop->GetODEModel()}}){
    require(pair.second->GetNumForces()==1&&pair.second->GetForce(0)->GetTypeName()=="PointMassForce","Navigation propagator is not the independent point-mass model");
    backendInputs[pair.first]=QJsonObject{{"force_count",pair.second->GetNumForces()},{"type",QString::fromStdString(pair.second->GetForce(0)->GetTypeName())},{"name",QString::fromStdString(pair.second->GetForce(0)->GetName())}};
   }save("navigation-force-census.json",QJsonDocument(backendInputs).toJson());
  }
  if(family=="FindEvents"){
   auto *locator=Moderator::Instance()->GetConfiguredObject("Events");auto *earth=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth");
   require(locator&&locator->GetStringArrayParameter("OccultingBodies").empty()&&locator->GetStringParameter("RunMode")=="Manual"&&earth->GetStringParameter(earth->GetParameterID("SpiceFrameId"))=="ITRF93","FindEvents physical/default mode differs from oracle");
   for(const auto &pair:QList<QPair<QString,QString>>{{QString::fromStdString(FileManager::Instance()->GetFullPathname(FileManager::ICRF_FILE)),"application/data/icrf/ICRF_Table.txt"},{QString::fromStdString(FileManager::Instance()->GetFullPathname(FileManager::PLANETARY_PCK_FILE)),"application/data/planetary_coeff/SPICEPlanetaryConstantsKernel.tpc"}}){
    require(QString::fromLatin1(QCryptographicHash::hash(read(pair.first),QCryptographicHash::Sha256).toHex())==physical[pair.second].toString(),"Runtime-selected contact physical input differs");backendInputs[pair.second]=physical[pair.second];
   }
  }
  if(spec.contains("marker_pairs"))require(std::count(contained.begin(),contained.end(),QJsonValue("BeginScript"))==spec["marker_pairs"].toInt()&&std::count(contained.begin(),contained.end(),QJsonValue("EndScript"))==spec["marker_pairs"].toInt(),"Nested event native boundaries missing");
  const bool observe=family=="BeginFiniteBurn"||family=="EndFiniteBurn"||family=="Global"||family=="CommandEcho";
  Debugger *debug=nullptr;for(auto *object:window.children())if(auto *d=dynamic_cast<Debugger *>(object))debug=d;
  require(debug,"Actual GUI debugger absent");QJsonArray phases;std::exception_ptr phaseFailure;
  auto previousPause=debug->paused;GmatBase *globalKeepBefore=nullptr;
  if(observe)debug->paused=[&](const QString &type){
   if(previousPause)previousPause(type);
   try{
    if(type=="BeginFiniteBurn"||type=="EndFiniteBurn"||type=="Report"||type=="Global"){
     if(family=="BeginFiniteBurn"||family=="EndFiniteBurn"){
      auto *burn=dynamic_cast<FiniteBurn *>(Moderator::Instance()->GetInternalObject("Burn"));require(burn,"Live phase burn absent");
      const bool firing=type=="EndFiniteBurn";require(burn->IsFiring()==firing,"Event burn state before command differs from Begin/End contract");
      phases.append(QJsonObject{{"before_command",type},{"burn_firing",burn->IsFiring()}});
     }else if(family=="Global"&&(type=="Global"||type=="Report")){
      auto *keep=Moderator::Instance()->GetInternalObject("Keep"),*count=Moderator::Instance()->GetInternalObject("Count");
      require(keep&&keep->IsGlobal()&&count&&!count->IsGlobal(),"Event Global did not select runtime Keep exclusively");
      auto *sandbox=Moderator::Instance()->GetSandbox();require(sandbox,"Global runtime sandbox absent");
      const auto local=sandbox->GetObjectMap(),global=sandbox->GetGlobalObjectMap();
      require(local.count("Count")==1&&local.at("Count")==count&&global.count("Count")==0,"Event Global changed unselected Count store");
      if(type=="Global"){
       require(local.count("Keep")==1&&local.at("Keep")==keep&&global.count("Keep")==0,"Global pre-execution local membership differs");globalKeepBefore=keep;
      }else require(globalKeepBefore&&keep==globalKeepBefore&&local.count("Keep")==0&&global.count("Keep")==1&&global.at("Keep")==globalKeepBefore,"Global did not migrate the same Keep object from local to global store");
      phases.append(QJsonObject{{"before_command",type},{"Keep_global",keep->IsGlobal()},{"Count_global",count->IsGlobal()},
       {"Keep_in_local",bool(local.count("Keep"))},{"Keep_in_global",bool(global.count("Keep"))},
       {"Count_in_local",bool(local.count("Count"))},{"Count_in_global",bool(global.count("Count"))},
       {"same_Keep_pointer",keep==globalKeepBefore}});
     }else if(family=="CommandEcho"&&type=="Report"){
      const bool expected=phases.isEmpty();const bool actual=GmatGlobal::Instance()->EchoCommands();
      require(actual==expected,"CommandEcho did not turn on then off before selected reports");
      phases.append(QJsonObject{{"before_command",type},{"echo_commands",actual}});
     }
    }
    if(auto *dialog=window.findChild<QDialog *>("debuggerDialog"))acceptanceView(*dialog);
    debug->step();
   }catch(...){phaseFailure=std::current_exception();debug->stop();}
  };
  const auto runUndo=source->document()->availableUndoSteps();QTimer watchdog;watchdog.setSingleShot(true);
  QObject::connect(&watchdog,&QTimer::timeout,&window,&MainWindow::stopMission);watchdog.start(30000);acceptanceView(window);
  require(!QFileInfo::exists(report),"Stale report exists before grouped execution");messages->clear();
  const auto outcome=observe?window.debugMission():window.runMission();const bool timedOut=!watchdog.isActive();watchdog.stop();events();debug->paused=previousPause;
  save("phase-observations.json",QJsonDocument(phases).toJson());if(phaseFailure)std::rethrow_exception(phaseFailure);
  if(observe)require(phases.size()==((family=="Global"||family=="CommandEcho")?2:3),"Contained command phase witness missing");
  save("messages.txt",messages->toPlainText().toUtf8());
  const bool stopped=spec["expected_outcome"].toString()=="Stopped";
  require(!stopped||family=="Stop","Only Stop context may require stopped outcome");
  require(!timedOut&&outcome==(stopped?MainWindow::RunResult::Stopped:MainWindow::RunResult::Completed),"Contained command mission failed or timed out");
  if(uniqueStarter&&spec["expected_prepared"].toBool()){
   const auto grouped=preparedState();require(grouped==plainPrepared&&grouped==nativePrepared,"Grouped preparation differs from plain/native control");
   require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==runUndo&&!source->document()->isModified(),"Preparation changed source/history");
   auto resumedSource=plain+suffix;require(resumedSource.count("PrepareMissionSequence;")==1,"Preparation transition boundary differs");resumedSource.replace("PrepareMissionSequence;","BeginMissionSequence;");
   source->setPlainText(resumedSource);require(window.buildScript()&&window.runMission()==MainWindow::RunResult::Completed,"Prepare-to-normal GUI stream transition failed");events();
   const auto resumedRaw=read(report);save("post-preparation-source.script",resumedSource.toUtf8());save("post-preparation-results.csv",resumedRaw);
   const auto resumedLine=QString::fromUtf8(resumedRaw).trimmed();const auto resumedCells=resumedLine.split(',');require(!resumedLine.contains('\n')&&resumedCells.size()==12,"Prepare-to-normal report shape differs");
   const double angle=omega*20;const double resumedExpected[]={20,radius*std::cos(angle),radius*std::sin(angle),0,-speed*std::sin(angle),speed*std::cos(angle),0,41,21,1,0,0};QJsonArray resumedValues;
   for(int i=0;i<12;++i){bool ok=false;const auto v=resumedCells[i].toDouble(&ok);require(ok&&std::isfinite(v)&&std::abs(v-resumedExpected[i])<(i==0?1e-4:2e-5),"Stale preparation mode suppressed normal commands or changed circle/scalar result");resumedValues.append(v);}
   auto *resumedCount=Moderator::Instance()->GetInternalObject("Count");auto *resumedSat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));require(resumedCount&&resumedCount->GetRealParameter("Value")==21&&resumedSat&&std::abs(resumedSat->GetEpoch()-(21545+20.0/86400))<2e-10,"Prepare-to-normal runtime state differs");
   const QJsonObject proof{{"checkpoint","script-event-unique-preparation"},{"outcome","passed"},{"family",family},
    {"fixture_sha256",QString::fromLatin1(QCryptographicHash::hash(fixture,QCryptographicHash::Sha256).toHex())},
    {"native_types",native},{"contained_types",contained},{"factory_types",factory},{"plain_prepared",plainPrepared},{"native_filename_prepared",nativePrepared},{"grouped_prepared",grouped},{"starter_mapping_rejections",starterMappingRejections},{"post_preparation_values",resumedValues},
    {"scope","Unique root PrepareMissionSequence preserves native filename preparation and grouped GUI lifecycle; no following command executes. Whole original requirement still requires inventory conjunction."}};
   save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
  }
  if(spec["skipped_extra_report"].toBool())require(family=="Stop"&&!QFileInfo::exists(extra),"Stop executed following report");
  const auto raw=read(report);save("results.csv",raw);const auto line=QString::fromUtf8(raw).trimmed();
  require(!line.contains('\n'),"Contained command report row count differs");const auto cells=line.split(',');require(cells.size()==12,"Contained command report width differs");
  QJsonArray expected,observed;const auto impulse=spec["velocity_increment"].toArray();require(impulse.size()==3&&spec["expected_tail"].toArray().size()==4,"Missing independent command reference");
  const double duration=spec.contains("orbit_duration")?spec["orbit_duration"].toDouble():20;
  require((duration==20||family=="FindEvents")&&duration>0,"Unexpected orbit-duration contract");
  const double angle=omega*duration;double orbit[]={duration,radius*std::cos(angle),radius*std::sin(angle),0,-speed*std::sin(angle)+impulse[0].toDouble(),speed*std::cos(angle)+impulse[1].toDouble(),impulse[2].toDouble(),41};
  if(spec.contains("expected_cartesian")){const auto cart=spec["expected_cartesian"].toArray();require(family=="FindEvents"&&cart.size()==6,"Unexpected independent Cartesian override");for(int i=0;i<6;++i)orbit[i+1]=cart[i].toDouble();}
  for(double v:orbit)expected.append(v);for(const auto &v:spec["expected_tail"].toArray())expected.append(v);
  for(int i=0;i<12;++i){bool ok=false;const auto value=cells[i].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-expected[i].toDouble())<(i==0?1e-4:2e-5),"Contained result differs from independent circle, impulse, algebra or constraint oracle");observed.append(value);}
  if(markerControl){
   const auto controlLine=QString::fromUtf8(markerPlainReport).trimmed();const auto controlCells=controlLine.split(',');
   require(!controlLine.contains('\n')&&controlCells.size()==12,"Plain marker control report shape differs");
   for(int i=0;i<12;++i){bool ok=false;const auto value=controlCells[i].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-expected[i].toDouble())<(i==0?1e-4:2e-5)&&std::abs(value-observed[i].toDouble())<2e-8,"Plain/grouped marker results differ from each other or independent reference");}
  }
  if(family=="FindEvents")for(int i=4;i<7;++i)require(std::abs(observed[i].toDouble()-expected[i].toDouble())<2e-8,"Contact final velocity differs from retained independent orbit bounds");
  auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));require(sat,"Contained runtime spacecraft missing");
  const char *axes[]={"X","Y","Z","VX","VY","VZ"};for(int i=0;i<6;++i)require(std::abs(sat->GetRealParameter(axes[i])-expected[i+1].toDouble())<2e-5,"Contained native spacecraft differs from independent orbit");
  int k=7;for(const auto &n:QStringList{"Keep","Count","U","Cost","Quantity"}){auto *v=Moderator::Instance()->GetInternalObject(n.toStdString());require(v&&std::abs(v->GetRealParameter("Value")-expected[k++].toDouble())<2e-5,"Contained native scalar differs from independent result");}
  QJsonObject effects;const auto trace=window.plotReceiver()->model("Trace"),control=window.plotReceiver()->model("Control");
  require(trace&&control&&trace->curves.size()==1&&control->curves.size()==1,"Contained publication models missing");
  for(const auto &m:{trace,control}){require(m->curves[0].points.size()>=2,"Contained publication samples absent");for(const auto &p:m->curves[0].points)require(std::isfinite(p.x)&&std::abs(p.x-p.y)<1e-10,"Contained elapsed publication differs");require(std::abs(m->curves[0].points.back().x-duration)<1e-4,"Contained final publication epoch differs");}
  if(spec.contains("expected_extra_rows")){
   const auto data=read(extra);save("backend-values.csv",data);const auto lines=QString::fromUtf8(data).trimmed().split('\n');
   const auto wantedRows=spec["expected_extra_rows"].toArray(),bounds=spec["extra_tolerances"].toArray();require(lines.size()==wantedRows.size(),"Backend result row count differs");QJsonArray observedRows;
   for(int i=0;i<lines.size();++i){const auto cells=lines[i].trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);const auto row=wantedRows[i].toArray();require(cells.size()==row.size()&&bounds.size()==row.size(),"Backend result width differs");QJsonArray values;
    for(int j=0;j<cells.size();++j){bool ok=false;const double value=cells[j].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-row[j].toDouble())<=bounds[j].toDouble(),"Backend result differs from independent literal/circle/epoch oracle");values.append(value);}observedRows.append(values);
   }effects["backend_values"]=observedRows;
  }
  if(family=="FindEvents"){
   const auto data=read(files.filePath("events.txt"));save("contact-events.txt",data);const auto text=QString::fromUtf8(data);
   const auto reference=spec["contact_reference"].toObject();const auto wanted=reference["events"].toObject()["East"].toArray();
   require(text.count("Observer: ")==1&&text.contains("Observer: East"),"FindEvents observer identity differs");
   const QRegularExpression rows(R"((\d{2} [A-Za-z]{3} \d{4} \d{2}:\d{2}:\d{2}\.\d+)\s+(\d{2} [A-Za-z]{3} \d{4} \d{2}:\d{2}:\d{2}\.\d+)\s+([0-9.eE+\-]+))");
   auto seconds=[](const QString &date){const auto time=QDateTime::fromString(date,"dd MMM yyyy HH:mm:ss.zzz");const auto base=QDateTime::fromString("01 Jan 2000 12:00:00.000","dd MMM yyyy HH:mm:ss.zzz");require(time.isValid(),"Invalid contact UTC epoch");return base.msecsTo(time)/1000.0;};
   QJsonArray actual;auto matches=rows.globalMatch(text);int i=0;while(matches.hasNext()){const auto match=matches.next();require(i<wanted.size(),"Extra FindEvents interval");const auto e=wanted[i++].toObject();const auto start=seconds(match.captured(1)),end=seconds(match.captured(2)),length=match.captured(3).toDouble();
    require(std::abs(start-e["start"].toDouble())<=reference["time_tolerance"].toDouble()&&std::abs(end-e["end"].toDouble())<=reference["time_tolerance"].toDouble()&&std::abs(length-e["duration"].toDouble())<=reference["duration_tolerance"].toDouble(),"FindEvents interval differs from existing independent kernel-pinned oracle");actual.append(QJsonObject{{"start",start},{"end",end},{"duration",length}});
   }require(i==wanted.size()&&i>0,"Missing positive FindEvents interval");effects["contact_intervals"]=actual;
  }
  if(family=="Navigation"){
   for(const auto &pair:QList<QPair<QString,QString>>{{"navigation.gmd","navigation.gmd"},{"filter.txt","filter-report.txt"},{"smoother.txt","smoother-report.txt"},{"warm.csv","filter-warm.csv"}}){const auto data=read(files.filePath(pair.first));require(!data.isEmpty(),"Navigation backend output absent");save(pair.second,data);}
   QJsonObject rowCounts,reportRecords;
   for(const auto &name:QStringList{"filter.txt","smoother.txt"}){
    int rows=0;bool residualTable=false;QMap<int,QVector<double>> observed,computed;QMap<int,QString> epochs;
    const auto epoch0=QDateTime::fromString("10 Jun 2010 00:00:00.000","dd MMM yyyy HH:mm:ss.zzz");
    for(const auto &line:QString::fromUtf8(read(files.filePath(name))).split('\n')){
     if(line.contains("RecNum")){const auto column=name=="filter.txt"?QString("Pre-Update (O-C)"):QString("Residual (O-C)");residualTable=line.contains("Observed (O)")&&line.contains("Computed (C)")&&line.contains(column);continue;}
     if(!residualTable)continue;
     const auto match=QRegularExpression(R"(^\s*(\d+)\s+(\d{2} [A-Za-z]{3} \d{4} \d{2}:\d{2}:\d{2}\.\d+)\s+GPS_PosVec\s+\S+\s+(.*)$)").match(line);if(!match.hasMatch())continue;
     const int record=match.captured(1).toInt();const auto date=QDateTime::fromString(match.captured(2),"dd MMM yyyy HH:mm:ss.zzz");
     require(record>=0&&record<spec["expected_observations"].toInt()&&date.isValid()&&std::abs(epoch0.msecsTo(date)-record*60000LL)<=1,"Navigation record identity/epoch differs");
     const auto parts=match.captured(3).trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(parts.size()==3,"Navigation normal unedited GPS result columns differ");
     bool ok=false;const auto observation=parts[0].toDouble(&ok);require(ok&&std::isfinite(observation),"Navigation observation is not finite");const auto calculation=parts[1].toDouble(&ok);require(ok&&std::isfinite(calculation),"Navigation calculation is not finite");const auto residual=parts[2].toDouble(&ok);require(ok&&std::isfinite(residual)&&std::abs(residual)<2e-5,"Noiseless navigation GPS residual differs");
     observed[record].append(observation);computed[record].append(calculation);epochs[record]=match.captured(2);++rows;
    }
    require(rows==3*spec["expected_observations"].toInt()&&observed.size()==spec["expected_observations"].toInt(),"Navigation filter/smoother did not process every GPS component");
    QJsonArray records;for(int i=0;i<spec["expected_observations"].toInt();++i){require(observed[i].size()==3&&computed[i].size()==3,"Duplicate/missing navigation component");double r2=0,c2=0;QJsonArray ov,cv;
     for(int j=0;j<3;++j){r2+=observed[i][j]*observed[i][j];c2+=computed[i][j]*computed[i][j];ov.append(observed[i][j]);cv.append(computed[i][j]);}
     require(std::abs(std::sqrt(r2)-radius)<2e-5&&std::abs(std::sqrt(c2)-radius)<2e-5,"Navigation GPS norm differs from independent circle");records.append(QJsonObject{{"record",i},{"utc",epochs[i]},{"observed",ov},{"computed",cv}});
    }rowCounts[name]=rows;reportRecords[name]=records;
   }effects["navigation_rows"]=rowCounts;effects["navigation_records"]=reportRecords;
  }
  if(QStringList{"ClearPlot","PenUp","PenDown","MarkPoint"}.contains(family)){
   QJsonArray points,controlPoints;int marks=0,gaps=0,links=0,earlyGaps=0,earlyControl=0;
   for(const auto &p:trace->curves[0].points){points.append(QJsonArray{p.x,p.y,p.connect,p.highlighted});
    if(p.highlighted){++marks;require(std::abs(p.x-10)<1e-4,"Event MarkPoint at wrong epoch");}
    if(p.x>10+1e-5){if(p.connect)++links;else++gaps;}
    if(p.x>1e-5&&p.x<10+1e-5&&!p.connect)++earlyGaps;
    if(family=="ClearPlot")require(p.x>=10-1e-5,"Event ClearPlot retained pre-clear data");
   }
   for(const auto &p:control->curves[0].points){controlPoints.append(QJsonArray{p.x,p.y,p.connect,p.highlighted});if(p.x<10-1e-5)++earlyControl;if(family=="MarkPoint")require(!p.highlighted,"Event MarkPoint affected unselected control");}
   require(earlyControl>0&&control->penDown,"Event plot command changed unselected control");
   if(family=="MarkPoint")require(marks==1,"Event MarkPoint missing/extra endpoint");
   if(family=="PenUp")require(!trace->penDown&&gaps>0&&links==0,"Event PenUp failed interval");
   if(family=="PenDown")require(trace->penDown&&earlyGaps>0&&links>0&&gaps==0,"Event PenDown failed initial-up/resumed interval");
   effects["trace_points"]=points;effects["control_points"]=controlPoints;effects["trace_pen_down"]=trace->penDown;
   save("plot-observations.json",QJsonDocument(effects).toJson());
  }
  if(family=="Write"){
   const auto wantedMessage=spec["expected_message"].toString();require(!wantedMessage.isEmpty()&&messages->toPlainText().contains(wantedMessage),"Event Write omitted selected string value");effects["written_message"]=wantedMessage;
  }
  if(family=="UpdateDynamicData"){
   require(window.plotReceiver()->show("Display"),"Event dynamic display absent");events();
   auto *display=area->activeSubWindow();auto *table=display?qobject_cast<QTableWidget *>(display->widget()):nullptr;
   require(table&&table->rowCount()==1&&table->columnCount()==2&&table->item(0,0)&&table->item(0,1),"Event dynamic display shape differs");
   require(table->item(0,0)->text().section(" = ",1).toDouble()==41&&table->item(0,1)->text().section(" = ",1).toDouble()==7,"Event UpdateDynamicData failed captured values or refreshed after later assignment");
   acceptanceView(*display);effects["display_values"]=QJsonArray{table->item(0,0)->text(),table->item(0,1)->text()};
  }
  if(family=="Toggle"){require(!trace->active&&control->active,"Contained Toggle changed wrong subscriber");effects["trace_active"]=trace->active;}
  if(family=="BeginFiniteBurn"||family=="EndFiniteBurn"){auto *burn=dynamic_cast<FiniteBurn *>(Moderator::Instance()->GetInternalObject("Burn"));require(burn&&!burn->IsFiring(),"Contained finite-burn pair left burn active");effects["burn_active"]=burn->IsFiring();}
  if(family=="Report"){const auto data=read(extra);save("extra-values.txt",data);bool ok=false;const auto value=QString::fromUtf8(data).trimmed().toDouble(&ok);require(ok&&value==41,"Contained Report selected value differs");effects["extra_value"]=value;}
  if(family=="Save"){const auto data=read(files.filePath("Sat.Spacecraft.data"));save("saved-runtime.txt",data);require(data.contains("Create Spacecraft Sat")&&data.contains("Sat.X")&&data.contains("Sat.VX"),"Contained Save did not export spacecraft");
   QJsonArray saved;const auto text=QString::fromUtf8(data);
   for(int i=0;i<6;++i){const auto m=QRegularExpression("(?:^|\\n)(?:GMAT )?Sat\\."+QString(axes[i])+" = ([^;\\n]+);").match(text);bool ok=false;const auto value=m.captured(1).toDouble(&ok);require(m.hasMatch()&&ok&&std::isfinite(value)&&std::abs(value-expected[i+1].toDouble())<2e-5,"Contained Save exported stale/wrong runtime orbit");saved.append(value);}
   effects["saved_cartesian"]=saved;effects["saved_bytes"]=data.size();}
  if(spec["mission_save"].toBool()){
   const bool exists=QFileInfo::exists(missionSave);require(exists==spec["expect_saved"].toBool(),"SaveMission terminal-marker/function/following-command contract differs");effects["mission_saved"]=exists;
   if(exists){const auto exported=read(missionSave);save("native-mission-export.script",exported);
    require(exported.contains("Create Spacecraft Sat")&&exported.contains("BeginMissionSequence")&&exported.contains("Propagate")&&exported.contains("SaveMission"),"SaveMission exported incomplete mission");
    if(!plainSaved.isEmpty()){
     auto normalize=[&](QString text){QJsonArray commands;for(const auto &s:scriptStatements(text)){
      auto code=s.code.trimmed();if(code.startsWith("BeginScript")||code.startsWith("EndScript"))continue;
      if(code.startsWith("SaveMission"))code="SaveMission '<controlled path>';";commands.append(code.simplified());
     }return commands;};
     const auto reference=normalize(QString::fromUtf8(plainSaved)),observed=normalize(QString::fromUtf8(exported));
     save("mission-export-equivalence.json",QJsonDocument(QJsonObject{{"plain",reference},{"grouped",observed}}).toJson());
     require(!reference.isEmpty()&&reference==observed,"Grouped SaveMission changed normalized native mission content");
    }
   }
  }
  require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==runUndo&&!source->document()->isModified(),"Contained execution changed source/history");
  const QJsonObject proof{{"checkpoint","script-event-command-"+family},{"outcome","passed"},{"family",family},
   {"fixture_sha256",QString::fromLatin1(QCryptographicHash::hash(fixture,QCryptographicHash::Sha256).toHex())},
   {"native_types",native},{"contained_types",contained},{"factory_types",factory},{"expected",expected},{"observed",observed},{"effects",effects},{"phase_observations",phases},{"native_outcome",stopped?"Stopped":"Completed"},{"call_mapping_rejections",callRejections},{"starter_mapping_rejections",starterMappingRejections},{"backend_inputs",backendInputs},
   {"scope","Additional command contexts inside actual ScriptEvent body editor and grouped Mission tree; independent closed-form orbit, impulse, scalar and side-effect checks. Original six branch fixtures reused separately. Full current command inventory, nested events, functions and plugin commands remain open; zero whole FRC!E138 credit."}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
