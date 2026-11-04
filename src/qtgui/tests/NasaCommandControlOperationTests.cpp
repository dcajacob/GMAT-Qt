// Reuse the established actual Mission-tree action helpers unchanged.
#define main retainedCommandLifecycleMain
#include "NasaCommandLifecycleTests.cpp"
#undef main
#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "FileManager.hpp"
#include "Moderator.hpp"
#include <QCryptographicHash>
#include <QRegularExpression>
#include "QtPlotReceiver.hpp"
#include "Debugger.hpp"
#include "FiniteBurn.hpp"
#include "Sandbox.hpp"
#include "GmatGlobal.hpp"
static QByteArray bytes(const QString &path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Operation input absent");return f.readAll();}
static void writeBytes(const QString &path,const QByteArray &v){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(v)==v.size(),"Operation evidence write failed");}
int main(int argc,char **argv){
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaCommandControlOperations");
 try {
  require(argc==5&&acceptanceWidgetPlatform(),"Supply native startup, retained fixture, family and position");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Operation work directory absent");const QString family=argv[3],position=argv[4];require(position=="Original"||position=="OuterIf","Unknown operation position");const auto fixture=bytes(QFileInfo(argv[2]).absoluteFilePath());const auto spec=QJsonDocument::fromJson(fixture).object();require(spec["family"]==family,"Wrong operation fixture");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());CommandCase test(startup,QJsonObject{{"key",family+position}});acceptanceView(test.window);FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",files.path().toStdString());const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &n,const QByteArray &v){if(!evidence.isEmpty())writeBytes(QDir(evidence).filePath(n),v);};
  const auto impulse=spec["velocity_increment"].toArray();require(impulse.size()==3,"Impulse reference absent");constexpr double mu=398600.4415,radius=7000.;const double duration=spec["orbit_duration"].toDouble(),inclination=spec["orbit_inclination_degrees"].toDouble()*std::acos(-1.)/180.;const double speed=std::sqrt(mu/radius),omega=std::sqrt(mu/(radius*radius*radius)),angle=omega*duration,summaryAngle=omega*spec["operation_summary_seconds"].toDouble();const QJsonArray summaryTruth{radius*std::cos(summaryAngle),radius*std::sin(summaryAngle)*std::cos(inclination),radius*std::sin(summaryAngle)*std::sin(inclination),-speed*std::sin(summaryAngle)+impulse[0].toDouble(),speed*std::cos(summaryAngle)*std::cos(inclination)+impulse[1].toDouble(),speed*std::cos(summaryAngle)*std::sin(inclination)+impulse[2].toDouble()};const auto report=files.filePath("values.csv"),missionSave=files.filePath("native mission Δ.script");const QString effect=spec["effect_family"].toString();
  auto expand=[&](QString s){for(const auto &p:QList<QPair<QString,QString>>{{"@SPEED@",QString::number(speed,'g',17)},{"@VALUES@",report},{"@DC@",files.filePath("dc.txt")},{"@OPT@",files.filePath("opt.txt")},{"@MISSION@",missionSave},{"@EXTRA@",files.filePath("extra.csv")},{"@FUNCTION@",files.filePath("Accept.gmf")},{"@INPUT@",files.filePath("input.sv")},{"@HISTORY@",files.filePath("history.thf")},{"@EVENT@",files.filePath("events.txt")},{"@DATA_ROOT@",QFileInfo(startup).absoluteDir().absoluteFilePath("../data")}})s.replace(p.first,p.second);require(!s.contains('@'),"Unresolved operation input");return s;};
  for(auto v:spec["support_files"].toArray()){auto file=v.toObject();auto name=file["name"].toString();require(QFileInfo(name).fileName()==name,"Unsafe support name");auto content=expand(file["text"].toString()).toUtf8();writeBytes(files.filePath(name),content);save("support-"+name,content);}
  QString original=expand(spec["script"].toString());test.script->setPlainText(original);require(test.window.buildScript(),"Operation original Build failed");
  auto index=[&](){int result=-1;const auto snapshot=test.window.missionSnapshot();for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].type==spec["command"].toString()&&(!spec.contains("operation_statement")||snapshot.nodes[i].statement.trimmed()==spec["operation_statement"].toString())&&(!spec.contains("operation_label")||missionCommandLabel(snapshot.nodes[i].statement).name==spec["operation_label"].toString())){require(result<0,"Operation target ambiguous");result=i;}require(result>=0,"Operation target absent");return result;};
  auto node=test.window.missionSnapshot().nodes[index()];require(node.editable,"Executable operation target is not editable");
  const auto originalSnapshot=test.window.missionSnapshot();const QString originalParent=node.parent<0?"Root":originalSnapshot.nodes[node.parent].type;require(originalParent==spec["original_parent"].toString(),"Original required parent differs");
  if(position=="OuterIf"){original.insert(node.end,"\nEndIf; % operation branch end\n");original.insert(node.start,"If Keep == 41; % operation branch begin\n");test.script->setPlainText(original);require(test.window.buildScript(),"Nested operation Build failed");node=test.window.missionSnapshot().nodes[index()];require(node.parent>=0,"Operation did not enter branch");}
  const auto positioned=test.window.missionSnapshot();const QString selectedParent=node.parent<0?"Root":positioned.nodes[node.parent].type;require(selectedParent==(position=="OuterIf"?"If":originalParent),"Selected parent differs");
  save("initial.script",original.toUtf8());save("fixture.json",fixture);QJsonArray factory;for(auto n:Moderator::Instance()->GetListOfFactoryItems(Gmat::COMMAND))factory.append(QString::fromStdString(n));save("factory-commands.json",QJsonDocument(factory).toJson());const auto statement=node.statement;QJsonArray operations;
  test.context(index(),"Edit command…");auto *owner=test.child();owner->showNormal();owner->resize(test.area->viewport()->size());acceptanceView(*owner);require(test.preview()->toPlainText().trimmed()==statement.trimmed(),"Context Open differs from selected command");QPointer<QMdiSubWindow> retained=owner;test.context(index(),"Edit command…");require(retained==test.child(),"Repeated Open duplicated owner");operations.append("open-context-reuse");test.context(index(),"missionCloseCommand");require(!retained&&test.script->toPlainText()==original,"Tree Close failed or changed source");operations.append("close-tree");
  test.tree->itemDoubleClicked(test.item(index()),0);test.events();require(test.preview()->toPlainText().trimmed()==statement.trimmed(),"Double click opened wrong command");acceptanceView(*test.child());test.close();operations.append("open-double-click");
  for(const QString side:{"before","after"}){
   const auto n=test.window.missionSnapshot().nodes[index()];const QString insertion="'OperationProbe' Keep = Keep + 0;";QString wanted=original;const int offset=side=="before"?n.start:n.end;wanted.insert(offset,side=="before"?insertion+"\n":"\n"+insertion);
   test.context(index(),side=="before"?"Insert before…":"Insert after…");test.preview()->setPlainText(insertion);acceptanceView(*test.child());require(test.script->toPlainText()==original,"Pending insertion committed");test.apply();const auto actual=test.script->toPlainText();save(side+"-inserted.script",actual.toUtf8());
   // Preserve every original source byte; the insertion policy owns its newline.
   QString removed=actual;require(removed.count(insertion)==1,"Insertion absent/duplicated");removed.remove(insertion+"\n");if(removed!=original){removed=actual;removed.remove("\n"+insertion);}require(removed==original,"Insertion changed unrelated source bytes");const auto snap=test.window.missionSnapshot();int probe=test.index("OperationProbe");require(snap.nodes[probe].parent==snap.nodes[index()].parent&&(side=="before"?snap.nodes[probe].start<snap.nodes[index()].start:snap.nodes[probe].start>snap.nodes[index()].start),"Insertion crossed selected branch/order");test.close();test.script->undo();require(test.script->toPlainText()==original&&test.window.buildScript(),"Insertion Undo failed");test.script->redo();require(test.script->toPlainText()==actual&&test.window.buildScript(),"Insertion Redo failed");test.script->undo();require(test.script->toPlainText()==original&&test.window.buildScript(),"Insertion restore failed");operations.append("insert-"+side);
  }
  const auto selected=test.window.missionSnapshot().nodes[index()];QString deleted=original;deleted.remove(selected.start,selected.end-selected.start);test.context(index(),"Delete command",[&]{auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());require(box&&box->button(QMessageBox::Cancel),"Delete Cancel absent");acceptanceView(*box);box->button(QMessageBox::Cancel)->click();});require(test.script->toPlainText()==original,"Delete Cancel changed source");
  test.context(index(),"Delete command",[&]{auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());require(box&&box->button(QMessageBox::Yes),"Delete confirmation absent");acceptanceView(*box);box->button(QMessageBox::Yes)->click();});const auto deletion=test.script->toPlainText();save("deleted.script",deletion.toUtf8());require(deletion==deleted,"Delete changed wrong source interval");test.script->undo();require(test.script->toPlainText()==original&&test.window.buildScript(),"Delete Undo failed");test.script->redo();require(test.script->toPlainText()==deletion&&test.window.buildScript(),"Delete Redo failed");test.script->undo();require(test.script->toPlainText()==original&&test.window.buildScript(),"Delete restore failed");operations.append("delete-cancel-confirm");
  QJsonArray results;for(int phase=0;phase<2;++phase){if(phase){auto path=files.filePath("Operations Ω.script");require(test.window.saveScriptTo(path)&&test.window.loadScript(path)&&test.window.buildScript()&&test.script->toPlainText()==original,"Operation save/reopen failed");}
   auto *messages=test.window.findChild<QPlainTextEdit *>("messageWindow");require(messages,"Messages absent");QFile::remove(missionSave);require(!QFileInfo::exists(missionSave),"Stale mission export");
  const bool observe=effect=="BeginFiniteBurn"||effect=="EndFiniteBurn"||effect=="Global"||effect=="CommandEcho";
  Debugger *debug=nullptr;for(auto *object:test.window.children())if(auto *d=dynamic_cast<Debugger *>(object))debug=d;
  require(debug,"Actual GUI debugger absent");QJsonArray phases;std::exception_ptr phaseFailure;
  auto previousPause=debug->paused;GmatBase *globalKeepBefore=nullptr;
  if(observe)debug->paused=[&](const QString &type){
   if(previousPause)previousPause(type);
   try{
    if(type=="BeginFiniteBurn"||type=="EndFiniteBurn"||type=="Report"||type=="Global"){
     if(effect=="BeginFiniteBurn"||effect=="EndFiniteBurn"){
      auto *burn=dynamic_cast<FiniteBurn *>(Moderator::Instance()->GetInternalObject("Burn"));require(burn,"Live phase burn absent");
      const bool firing=type=="EndFiniteBurn";require(burn->IsFiring()==firing,"Event burn state before command differs from Begin/End contract");
      phases.append(QJsonObject{{"before_command",type},{"burn_firing",burn->IsFiring()}});
     }else if(effect=="Global"&&(type=="Global"||type=="Report")){
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
     }else if(effect=="CommandEcho"&&type=="Report"){
      const bool expected=phases.isEmpty();const bool actual=GmatGlobal::Instance()->EchoCommands();
      require(actual==expected,"CommandEcho did not turn on then off before selected reports");
      phases.append(QJsonObject{{"before_command",type},{"echo_commands",actual}});
     }
    }
    if(auto *dialog=test.window.findChild<QDialog *>("debuggerDialog"))acceptanceView(*dialog);
    debug->step();
   }catch(...){phaseFailure=std::current_exception();debug->stop();}
  };
  const auto runUndo=test.script->document()->availableUndoSteps();QTimer watchdog;watchdog.setSingleShot(true);
  QObject::connect(&watchdog,&QTimer::timeout,&test.window,&MainWindow::stopMission);watchdog.start(30000);acceptanceView(test.window);
  QFile::remove(report);require(!QFileInfo::exists(report),"Stale output report");messages->clear();
  const auto outcome=observe?test.window.debugMission():test.window.runMission();const bool timedOut=!watchdog.isActive();watchdog.stop();test.events();debug->paused=previousPause;
  save(QString::number(phase)+"-phase-observations.json",QJsonDocument(phases).toJson());if(phaseFailure)std::rethrow_exception(phaseFailure);
  if(observe)require(phases.size()==((effect=="Global"||effect=="CommandEcho")?2:3),"Contained command phase witness missing");
   require(outcome==(family=="Stop"?MainWindow::RunResult::Stopped:MainWindow::RunResult::Completed)&&!timedOut,"Operation restored mission failed");if(family=="Stop")require(!QFileInfo::exists(files.filePath("extra.csv")),"Stop executed following report");const auto raw=bytes(report);save(QString::number(phase)+"-states.csv",raw);const auto fields=QString::fromUtf8(raw).trimmed().split(',');require(fields.size()==12,"Operation report width differs");QJsonArray expected{duration,radius*std::cos(angle),radius*std::sin(angle)*std::cos(inclination),radius*std::sin(angle)*std::sin(inclination),-speed*std::sin(angle)+impulse[0].toDouble(),speed*std::cos(angle)*std::cos(inclination)+impulse[1].toDouble(),speed*std::cos(angle)*std::sin(inclination)+impulse[2].toDouble(),41.};for(auto v:spec["expected_tail"].toArray())expected.append(v);QJsonArray observed;
   for(int i=0;i<12;++i){bool ok=false;double value=fields[i].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-expected[i].toDouble())<=(i==0?1e-4:i<4?2e-5:i<7?2e-8:spec["scalar_tolerance"].toDouble()),"Restored mission differs from independent circle/scalar truth");observed.append(value);}
   QJsonArray sideEffects;
   if(spec.contains("expected_extra_rows")){const auto content=bytes(files.filePath("extra.csv"));save(QString::number(phase)+"-extra.csv",content);const auto lines=QString::fromUtf8(content).trimmed().split('\n');const auto expectedRows=spec["expected_extra_rows"].toArray(),tolerances=spec["extra_tolerances"].toArray();require(lines.size()==expectedRows.size(),"Backend extra row count differs");for(int row=0;row<lines.size();++row){const auto values=lines[row].split(QRegularExpression("[,\\s]+"),Qt::SkipEmptyParts);const auto want=expectedRows[row].toArray();require(values.size()==want.size()&&tolerances.size()==want.size(),"Backend extra width differs");QJsonArray actual;for(int col=0;col<values.size();++col){bool ok=false;const double value=values[col].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-want[col].toDouble())<=tolerances[col].toDouble(),"Selective import or file-thrust mass result differs");actual.append(value);}sideEffects.append(actual);}}
   if(family=="FindEvents"){const auto content=bytes(files.filePath("events.txt"));save(QString::number(phase)+"-events.txt",content);require(!content.trimmed().isEmpty(),"FindEvents did not retain its event report");}
   QJsonObject effects;effects["phase_observations"]=phases;const auto trace=test.window.plotReceiver()->model("Trace"),control=test.window.plotReceiver()->model("Control");require(trace&&control&&trace->curves.size()==1&&control->curves.size()==1,"Plot pair absent");
  if(QStringList{"ClearPlot","PenUp","PenDown","MarkPoint"}.contains(effect)){
   QJsonArray points,controlPoints;int marks=0,gaps=0,links=0,earlyGaps=0,earlyControl=0;
   for(const auto &p:trace->curves[0].points){points.append(QJsonArray{p.x,p.y,p.connect,p.highlighted});
    if(p.highlighted){++marks;require(std::abs(p.x-10)<1e-4,"Event MarkPoint at wrong epoch");}
    if(p.x>10+1e-5){if(p.connect)++links;else++gaps;}
    if(p.x>1e-5&&p.x<10+1e-5&&!p.connect)++earlyGaps;
    if(effect=="ClearPlot")require(p.x>=10-1e-5,"Event ClearPlot retained pre-clear data");
   }
   for(const auto &p:control->curves[0].points){controlPoints.append(QJsonArray{p.x,p.y,p.connect,p.highlighted});if(p.x<10-1e-5)++earlyControl;if(effect=="MarkPoint")require(!p.highlighted,"Event MarkPoint affected unselected control");}
   require(earlyControl>0&&control->penDown,"Event plot command changed unselected control");
   if(effect=="MarkPoint")require(marks==1,"Event MarkPoint missing/extra endpoint");
   if(effect=="PenUp")require(!trace->penDown&&gaps>0&&links==0,"Event PenUp failed interval");
   if(effect=="PenDown")require(trace->penDown&&earlyGaps>0&&links>0&&gaps==0,"Event PenDown failed initial-up/resumed interval");
   effects["trace_points"]=points;effects["control_points"]=controlPoints;effects["trace_pen_down"]=trace->penDown;
   save(QString::number(phase)+"-plot-observations.json",QJsonDocument(effects).toJson());
  }
  if(effect=="Write"){
   const auto wantedMessage=spec["expected_message"].toString();require(!wantedMessage.isEmpty()&&messages->toPlainText().contains(wantedMessage),"Event Write omitted selected string value");effects["written_message"]=wantedMessage;
  }
  if(effect=="Toggle"){require(!trace->active&&control->active,"Toggle changed wrong subscriber");effects["trace_active"]=trace->active;}
   if(effect=="Report"){const auto data=bytes(files.filePath("extra.csv"));save(QString::number(phase)+"-reported-value.txt",data);bool ok=false;const auto value=QString::fromUtf8(data).trimmed().toDouble(&ok);require(ok&&value==41,"Report selected value differs");effects["extra_value"]=value;}
   if(effect=="Save"){const auto data=bytes(files.filePath("Sat.Spacecraft.data"));save(QString::number(phase)+"-saved-runtime.txt",data);require(data.contains("Create Spacecraft Sat"),"Saved resource absent");QJsonArray saved;for(int i=0;i<6;++i){const auto label=QStringList{"X","Y","Z","VX","VY","VZ"}[i];const auto match=QRegularExpression("(?:^|\\n)(?:GMAT )?Sat\\."+label+" = ([^;\\n]+);").match(QString::fromUtf8(data));bool ok=false;const double value=match.captured(1).toDouble(&ok);require(match.hasMatch()&&ok&&std::isfinite(value)&&std::abs(value-summaryTruth[i].toDouble())<=2e-5,"Saved runtime state differs");saved.append(value);}effects["saved_cartesian"]=saved;}
   if(effect=="SaveMission"){const bool exists=QFileInfo::exists(missionSave);require(exists==(position=="Original"),"SaveMission terminal/branch contract differs");effects["mission_saved"]=exists;if(exists){const auto data=bytes(missionSave);save(QString::number(phase)+"-mission-export.script",data);require(data.contains("Create Spacecraft Sat")&&data.contains("BeginMissionSequence")&&data.contains("SaveMission"),"Mission export incomplete");}}
   if(family=="BeginFiniteBurn"||family=="EndFiniteBurn"){auto *burn=dynamic_cast<FiniteBurn *>(Moderator::Instance()->GetInternalObject("Burn"));require(burn&&!burn->IsFiring(),"Restored burn pair left thrust enabled");effects["burn_active_after_pair"]=burn->IsFiring();}
   if(family=="Stop")effects["following_report_absent"]=!QFileInfo::exists(files.filePath("extra.csv"));
   QString summary;test.context(index(),"missionCommandSummary",[&]{auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());auto *text=dialog?dialog->findChild<QPlainTextEdit *>("inspectionText"):nullptr;require(dialog&&text&&text->isReadOnly(),"Actual context summary absent");acceptanceView(*dialog);summary=text->toPlainText();save(QString::number(phase)+"-summary.txt",summary.toUtf8());require(summary.contains("Spacecraft       : Sat")&&summary.contains("Coordinate System: EarthMJ2000Eq"),"Context summary lacks selected spacecraft/frame");for(int i=0;i<6;++i){const QString label=QStringList{"X","Y","Z","VX","VY","VZ"}[i];auto match=QRegularExpression("(?:^|\\s)"+label+"\\s*=\\s*([-+0-9.eE]+)").globalMatch(summary);require(match.hasNext(),"Summary state field absent");bool ok=false;double value=match.next().captured(1).toDouble(&ok);require(ok&&!match.hasNext()&&std::abs(value-summaryTruth[i].toDouble())<=2e-5,"Summary state differs from independent circle");}dialog->accept();});save(QString::number(phase)+"-summary.txt",summary.toUtf8());require(test.script->toPlainText()==original,"Summary changed source");results.append(QJsonObject{{"phase",phase},{"raw",observed},{"expected",expected},{"summary_expected",summaryTruth},{"side_effects",sideEffects},{"effects",effects}});
  }
  QJsonArray exportReplay;if(effect=="SaveMission"&&position=="Original"){require(test.window.loadScript(missionSave)&&test.window.buildScript()&&test.window.runMission()==MainWindow::RunResult::Completed,"Native mission export did not rebuild/run");const auto data=bytes(report);save("export-replayed-state.csv",data);const auto values=QString::fromUtf8(data).trimmed().split(',');require(values.size()==12,"Export replay report width differs");const auto wanted=results[0].toObject()["expected"].toArray();for(int i=0;i<12;++i){bool ok=false;const double value=values[i].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-wanted[i].toDouble())<=(i==0?1e-4:i<4?2e-5:i<7?2e-8:spec["scalar_tolerance"].toDouble()),"Export replay differs from independent initial-condition mission");exportReplay.append(value);}}
  operations.append("summary-context");operations.append("unicode-reopen");
  QJsonObject proof{{"checkpoint","command-control-operations"},{"outcome","passed"},{"family",family},{"position",position},{"original_parent",originalParent},{"selected_parent",selectedParent},{"operations",operations},{"results",results},{"export_replay",exportReplay},{"fixture_sha256",QString::fromLatin1(QCryptographicHash::hash(fixture,QCryptographicHash::Sha256).toHex())},{"scope","Selected missing executable command operations at root/nested positions; complete any-command parent still requires its full census."}};save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
