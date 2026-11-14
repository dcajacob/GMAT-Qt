#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include "Debugger.hpp"
#include "FiniteBurn.hpp"
#include "Subscriber.hpp"
#include "FileManager.hpp"
#include "Sandbox.hpp"
#include <QRegularExpression>
#include <QSet>
#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidgetItemIterator>
#include <functional>
using namespace formaccept;

static QJsonObject entry(const QString &file,const QString &key)
{
   for (const auto &value:QJsonDocument::fromJson(read(file)).object()["cases"].toArray())
      if (value.toObject()["key"]==key) return value.toObject();
   throw std::runtime_error("Command tree case absent");
}
static void reveal(QWidget *widget)
{
   require(widget,"Command tree widget absent");
   for (QWidget *parent=widget;parent;parent=parent->parentWidget())
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(widget);
   events();require(widget->isVisible()&&widget->isEnabled(),"Command tree widget unreachable");
   acceptanceView(*widget);
}
// Reuse the established control-tree GUI insertion path, with source-bound
// NASA command fixtures and independent command-specific execution oracles.
static QString probe(const QString &family,int p)
{
 const auto label="Probe"+QString::number(p);QString statement;
 if(family=="Propagate")statement="Propagate '"+label+"' P(Sat) {Sat.ElapsedSecs = 10};";
 else if(family=="Maneuver")statement="Maneuver '"+label+"' Kick(Sat);";
 else if(family=="Report")statement="Report '"+label+"' Extra Quantity I;";
 else if(family=="Toggle")statement="Toggle '"+label+"' Trace Off;";
 else if(family=="Save")statement="Save '"+label+"' Quantity;";
 else if(family=="Global")statement="Global '"+label+"' Keep;";
 else statement=family+" '"+label+"' Burn(Sat);";
 return statement+"\n'Observe"+QString::number(p)+"' Count = Count + 0;";
}
static int calls(const QString &position){return position=="Many"?3:QStringList{"BranchAppend","EndBefore","LeafBefore","LeafAfter"}.contains(position)?2:1;}
static int loopIndex(const QString &position,int execution){return QStringList{"BranchAppend","EndBefore","LeafBefore","LeafAfter"}.contains(position)?execution+1:QStringList{"Many","BlockAfter","EndAfter"}.contains(position)?2:0;}
static QJsonObject expected(const QString &family,const QString &position,int serial,bool after)
{
 const int done=serial+(after?1:0);const double seconds=family=="Propagate"?done*10.:0,angle=std::sqrt(mu/(7000.*7000.*7000.))*seconds,speed=std::sqrt(mu/7000.);QJsonArray state{seconds,7000*std::cos(angle),7000*std::sin(angle),0,-speed*std::sin(angle),speed*std::cos(angle),0};
 if(family=="Maneuver"){state[4]=done*.01;state[5]=speed;state[6]=0;}
 const int i=loopIndex(position,serial);state.append(position=="Many"?42:41);state.append(i);state.append(i*10);state.append(0);
 QJsonArray report,saved;QSet<int> written;
 for(int j=0;j<done;++j){const int index=loopIndex(position,j);if(family=="Report")report.append(QJsonArray{index*10,index});if(family=="Save"){const int id=position=="Many"?j:0;if(!written.contains(id)){saved={};written.insert(id);}saved.append(index*10);}}
 QJsonObject result{{"state",state},{"keep_global",family=="Global"},{"trace_active",family!="Toggle"||done==0},{"control_active",true},{"firing",family=="BeginFiniteBurn"?done>0:family=="EndFiniteBurn"&&position!="Empty"&&done==0},{"report_rows",report},{"saved_quantities",saved}};
 if(family=="Global"){result["keep_local_store"]=done==0;result["keep_global_store"]=done>0;}return result;
}
static void equal(const QJsonObject &actual,const QJsonObject &want)
{
 const auto a=actual["state"].toArray(),b=want["state"].toArray();require(a.size()==11&&b.size()==11,"Command tree native state width differs");
 for(int i=0;i<11;++i){const double v=a[i].toDouble(),e=b[i].toDouble(),tol=i==0?1e-4:i<4?2e-5:i<7?2e-8:1e-12;if(!std::isfinite(v)||std::abs(v-e)>tol)std::cerr<<"TREE_DIFF column="<<i<<" actual="<<v<<" expected="<<e<<std::endl;require(std::isfinite(v)&&std::abs(v-e)<=tol,"Inserted command state differs from independent circle/impulse/scalar oracle");}
 for(const auto *key:{"keep_global","trace_active","control_active","firing","report_rows","saved_quantities"})require(actual[key]==want[key],("Inserted command immediate side effect differs: "+QString(key)).toUtf8().constData());
 if(want.contains("keep_local_store"))require(actual["keep_local_store"]==want["keep_local_store"]&&actual["keep_global_store"]==want["keep_global_store"],"Inserted Global did not move Keep from local to global store at execution");
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaCommandTree");
   try {
      require(argc==5&&acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen",
              "Supply startup, reused command cases, insertion cases and key on a native widget platform");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      const auto spec=entry(QFileInfo(argv[3]).absoluteFilePath(),argv[4]);
      const auto reused=entry(QFileInfo(argv[2]).absoluteFilePath(),spec["reused_reference_case"].toString());
      const auto family=spec["family"].toString(),id=spec["key"].toString();
      require(QStringList{"Propagate","Maneuver","Report","Toggle","Save","BeginFiniteBurn","EndFiniteBurn","Global"}.contains(family)&&reused["command"]==family,"Command tree reused command identity differs");
      TestSettings settings;QTemporaryDir files;require(files.isValid(),"Command tree fixture absent");
      QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;window.resize(1200,850);window.show();
      require(window.initialize(startup),"Command tree runtime unavailable");events();acceptanceView(window);
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");
      auto *tree=window.findChild<QTreeWidget *>("Mission"),*resources=window.findChild<QTreeWidget *>("Resources");
      auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&resources&&area,"Command tree workspace missing");
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &bytes) {if (!evidence.isEmpty()) write(evidence+"/"+name,bytes);};
      save("case.json",QJsonDocument(spec).toJson());
      require(std::abs(Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant()-mu)<1e-9,"Command tree literal Earth gravity differs");
      FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",files.path().toStdString());
      const auto report=files.filePath("extra.csv"),saveOutput=files.filePath("Quantity.Variable.data");
      auto resolve=[&](QString text){for(const auto &p:QList<QPair<QString,QString>>{{"@SPEED@",number(std::sqrt(mu/7000.))},{"@VALUES@",files.filePath("values.csv")},{"@EXTRA@",report},{"@DC@",files.filePath("dc.txt")},{"@OPT@",files.filePath("opt.txt")}})text.replace(p.first,p.second);require(!text.contains('@'),"Reused command fixture placeholder unresolved");return text;};
      const QString prefix=resolve(reused["script"].toString().section("BeginMissionSequence;",0,0))+"BeginMissionSequence;\n";
      require(prefix.contains("Keep = 41;")&&prefix.contains("I = 0;")&&prefix.contains("Quantity = 0;")&&prefix.contains("Sat.X = 7000;"),"Reused native fixture differs from independent literal inputs");
      const QString outer="For 'Outer' I = 1:1:2;\n",inner="If 'Inner' I > 0;\n",leaf="'Leaf' Keep = Keep + 0;\n";
      const QString endIf="EndIf;\n",endFor="EndFor;\n";
      const QString body=(family=="EndFiniteBurn"?QString("BeginFiniteBurn 'InitialFire' Burn(Sat);\n"):QString())+outer+"Quantity = I * 10;\n"+inner+leaf+endIf+endFor+"'After outer' Keep = Keep + 1;\n";
      int commits=0,cancels=0,rawValues=0;QJsonArray qualified;
      for (const auto &value:spec["positions"].toArray()) {
         const auto positionSpec=value.toObject();const auto position=positionSpec["position"].toString();
         const auto saved=files.filePath(position+" Ω.script");
         const QString initial=prefix+(position=="Empty"?QString():body)+"% Preserve tail Δ\n";
         write(saved,initial.toUtf8());require(window.loadScript(saved)&&window.buildScript(),"Command tree initial build failed");
         save(position+"-initial.script",initial.toUtf8());QString wanted=initial;QJsonArray edits;
         auto named=[&](const QString &name) {
            const auto snapshot=window.missionSnapshot();int at=-1;
            for (int i=0;i<snapshot.nodes.size();++i) if (missionCommandLabel(snapshot.nodes[i].statement).name==name) {
               require(at<0,"Command tree label ambiguous");at=i;
            }
            require(at>=0,"Command tree named node absent");return at;
         };
         auto item=[&](int at) {
            for (QTreeWidgetItemIterator it(tree);*it;++it)
               if ((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==at) return *it;
            throw std::runtime_error("Command tree tree item absent");
         };
         auto insertion=[&]() {
            for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i)
               if (tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i);
            QTreeWidgetItem *target=nullptr;QString action;
            if (position=="Empty"||position=="Many") {target=tree->topLevelItem(0);action="Append command…";}
            else if (position=="BranchAppend") {target=item(named("Outer"));action="appendInsideMissionBranch";}
            else if (position=="BlockBefore"||position=="BlockAfter") {target=item(named("Outer"));action=position=="BlockBefore"?"Insert before…":"Insert after…";}
            else if (position=="LeafBefore"||position=="LeafAfter") {target=item(named("Leaf"));action=position=="LeafBefore"?"Insert before…":"Insert after…";}
            else {
               const auto snapshot=window.missionSnapshot();const int parent=named("Outer");int closing=-1;
               for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="EndFor"&&snapshot.nodes[i].parent==parent) closing=i;
               require(closing>=0,"Command tree outer terminator absent");target=item(closing);action=position=="EndBefore"?"Insert before…":"Insert after…";
            }
            require(target,"Command tree target absent");tree->expandAll();tree->setCurrentItem(target);tree->scrollToItem(target);events();reveal(tree);
            std::exception_ptr failure;bool visited=false;
            QTimer::singleShot(0,&window,[&] {
               auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
               try {
                  require(menu,"Actual insertion context menu absent");QAction *choice=nullptr;
                  for (auto *candidate:menu->actions()) if (candidate->text()==action||candidate->objectName()==action) choice=candidate;
                  require(choice&&choice->isEnabled(),"Actual insertion action unavailable");menu->setActiveAction(choice);acceptanceView(*menu);visited=true;QTest::keyClick(menu,Qt::Key_Return);
               } catch (...) {failure=std::current_exception();if (menu) menu->close();}
            });
            const auto point=tree->visualItemRect(target).center();QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
            QApplication::sendEvent(tree->viewport(),&event);events();if (failure) std::rethrow_exception(failure);require(visited,"Insertion action not visited");
            auto *owner=area->activeSubWindow();require(owner&&dynamic_cast<CommandEditor *>(owner->widget()),"Actual insertion command editor absent");
            owner->showNormal();owner->move(0,0);owner->resize(area->viewport()->size());events();acceptanceView(*owner);return owner;
         };
         auto configure=[&](QMdiSubWindow *owner,int p) {
            auto *choice=owner->findChild<QComboBox *>("commandTemplate");reveal(choice);
            const auto wantedType=family;const int index=choice->findText(wantedType);require(index>=0,"Command tree command template absent");
            QTest::mouseClick(choice,Qt::LeftButton,Qt::NoModifier,QPoint(choice->width()-10,choice->height()/2));events();require(choice->view()->isVisible(),"Command tree command template popup absent");
            acceptanceView(*choice->view()->window());QTest::keyClick(choice->view(),Qt::Key_Home);
            for (int i=0;i<index;++i) QTest::keyClick(choice->view(),Qt::Key_Down);
            QTest::keyClick(choice->view(),Qt::Key_Return);events();require(choice->currentText()==wantedType,"Command tree command template selection differs");
            auto *draft=owner->findChild<QPlainTextEdit *>("commandSource");reveal(draft);draft->setPlainText(probe(family,p));events();
            require(draft->toPlainText()==probe(family,p)&&dynamic_cast<CommandEditor *>(owner->widget())->hasChanges(),"Command tree source draft differs");acceptanceView(*owner);
         };
         auto click=[&](QMdiSubWindow *owner,QDialogButtonBox::StandardButton which) {
            auto *box=owner->findChild<QDialogButtonBox *>();auto *button=box?box->button(which):nullptr;reveal(button);QTest::mouseClick(button,Qt::LeftButton);events();
         };
         const int count=position=="Many"?3:1;
         for (int p=0;p<count;++p) {
            const auto before=wanted;const int undo=source->document()->availableUndoSteps();
            const bool modified=source->document()->isModified();QPointer<QMdiSubWindow> owner=insertion();configure(owner,p);
            require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&source->document()->isModified()==modified,"Pending insertion escaped draft/history isolation");
            if (p==0) {
               click(owner,QDialogButtonBox::Cancel);require(!owner&&source->toPlainText()==before&&source->document()->availableUndoSteps()==undo,"Insertion Cancel changed source/history or retained owner");
               ++cancels;owner=insertion();configure(owner,p);
            }
            const auto statement=probe(family,p)+"\n";
            if (position=="Empty"||position=="Many") wanted+=statement;
            else {
               const auto anchor=position=="BlockBefore"?outer:position=="LeafBefore"||position=="LeafAfter"?leaf:endFor;
               require(wanted.count(anchor)==1,"Independent insertion source anchor ambiguous");
               const bool after=position=="BlockAfter"||position=="EndAfter"||position=="LeafAfter";
               wanted.insert(wanted.indexOf(anchor)+(after?anchor.size():0),statement);
            }
            QPointer<CommandEditor> old=dynamic_cast<CommandEditor *>(owner->widget());
            click(owner,position=="Many"?QDialogButtonBox::Apply:QDialogButtonBox::Ok);
            require(!old&&source->toPlainText()==wanted&&source->document()->availableUndoSteps()>undo,"Command tree committed wrong source/history or retained stale editor");
            if (position=="Many") {
               require(owner&&!dynamic_cast<CommandEditor *>(owner->widget())->hasChanges(),"Insertion Apply lost refreshed clean owner");
               const int steps=source->document()->availableUndoSteps();
               auto *clean=owner->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);
               require(clean&&clean->isVisible(),"Clean insertion Apply absent");
               acceptanceView(*owner);QTest::mouseClick(clean,Qt::LeftButton);events();
               require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==steps,"Repeated insertion Apply duplicated command/history");
               click(owner,QDialogButtonBox::Ok);
            }
            require(!owner,"Insertion OK did not close original owner");++commits;
            source->undo();require(source->toPlainText()==before&&window.buildScript(),"Insertion Undo failed exact source/model restoration");
            source->redo();require(source->toPlainText()==wanted&&window.buildScript(),"Insertion Redo failed exact source/model restoration");
            edits.append(QJsonObject{{"probe",p},{"statement",statement},{"mode",position=="Many"?"apply-clean-apply-ok":"positive-ok"},{"cancel_first",p==0},{"source_history_exact",true},{"stale_editor_retired",true},{"owner_closed",true}});
         }
         require(window.saveScriptTo(saved)&&read(saved)==wanted.toUtf8()&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==wanted,"Command tree Unicode save/reload changed exact source/model");
         save(position+"-accepted.script",wanted.toUtf8());save(position+"-edits.json",QJsonDocument(edits).toJson());
         const auto snapshot=window.missionSnapshot();QJsonArray hierarchy;QMap<int,QPair<int,bool>> observers;
         for(int i=0;i<snapshot.nodes.size();++i){const auto &node=snapshot.nodes[i];const auto label=missionCommandLabel(node.statement).name;hierarchy.append(QJsonObject{{"type",node.type},{"label",label},{"parent",node.parent},{"statement",node.statement}});for(int p=0;p<count;++p){if(label=="Probe"+QString::number(p))observers[i]={p,false};if(label=="Observe"+QString::number(p))observers[i]={p,true};}}
         const QString parent=position=="LeafBefore"||position=="LeafAfter"?"Inner":position=="BranchAppend"||position=="EndBefore"?"Outer":"";
         for(int p=0;p<count;++p){const int at=named("Probe"+QString::number(p)),following=named("Observe"+QString::number(p));require(snapshot.nodes[at].type==family&&following==at+1&&snapshot.nodes[following].parent==snapshot.nodes[at].parent,"Inserted command/observational successor native identity differs");require(parent.isEmpty()?snapshot.nodes[at].parent<0:snapshot.nodes[at].parent==named(parent),"Inserted native command has wrong parent");require(parent.isEmpty()?item(at)->parent()==tree->topLevelItem(0):item(at)->parent()==item(named(parent)),"Visible command tree parent differs from literal hierarchy");}
         require(observers.size()==count*2,"Native before/after observation point count differs");save(position+"-hierarchy.json",QJsonDocument(hierarchy).toJson());tree->expandAll();events();acceptanceView(*tree);
         auto sample=[&](const QString &tag){
            auto *engine=Moderator::Instance();auto *sat=dynamic_cast<Spacecraft *>(engine->GetInternalObject("Sat"));require(sat,"Command tree native spacecraft absent");QJsonArray state{(sat->GetEpoch()-21545)*86400};for(const char *axis:{"X","Y","Z","VX","VY","VZ"})state.append(sat->GetRealParameter(axis));for(const char *name:{"Keep","I","Quantity","Count"})state.append(engine->GetInternalObject(name)->GetRealParameter("Value"));
            auto *trace=dynamic_cast<Subscriber *>(engine->GetInternalObject("Trace")),*control=dynamic_cast<Subscriber *>(engine->GetInternalObject("Control"));require(trace&&control,"Command tree native control/target subscriber absent");bool firing=false;if(family=="BeginFiniteBurn"||family=="EndFiniteBurn"){auto *thruster=sat->GetRefObject(Gmat::THRUSTER,"Engine");require(thruster,"Command tree attached runtime thruster absent");firing=thruster->GetBooleanParameter("IsFiring");}
            QJsonArray rows,savedValues;if(QFileInfo::exists(report)){const auto bytes=read(report);save(position+"-"+tag+"-report.txt",bytes);const auto words=QString::fromUtf8(bytes).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()%2==0,"Inserted Report output width differs");for(int i=0;i<words.size();i+=2){bool a=false,b=false;const double x=words[i].toDouble(&a),y=words[i+1].toDouble(&b);require(a&&b&&std::isfinite(x)&&std::isfinite(y),"Inserted Report contains nonfinite result");rows.append(QJsonArray{x,y});}}
            if(QFileInfo::exists(saveOutput)){const auto bytes=read(saveOutput);save(position+"-"+tag+"-export.data",bytes);auto matches=QRegularExpression("(?m)^(?:GMAT\\s+)?Quantity\\s*=\\s*([^;]+);").globalMatch(QString::fromUtf8(bytes));while(matches.hasNext()){bool ok=false;const double v=matches.next().captured(1).toDouble(&ok);require(ok&&std::isfinite(v),"Inserted Save has invalid scalar value");savedValues.append(v);}require(!savedValues.isEmpty(),"Inserted Save omitted runtime scalar");}
            QJsonObject result{{"state",state},{"keep_global",engine->GetInternalObject("Keep")->IsGlobal()},{"trace_active",trace->IsActive()},{"control_active",control->IsActive()},{"firing",firing},{"report_rows",rows},{"saved_quantities",savedValues}};
            if(family=="Global"){auto *sandbox=engine->GetSandbox();require(sandbox,"Native Global sandbox absent");result["keep_local_store"]=sandbox->GetObjectMap().count("Keep")!=0;result["keep_global_store"]=sandbox->GetGlobalObjectMap().count("Keep")!=0;}return result;
         };
         QFile::remove(report);QFile::remove(saveOutput);QJsonArray samples;std::exception_ptr failure;Debugger *debug=nullptr;for(auto *object:window.children())if(auto *candidate=dynamic_cast<Debugger *>(object))debug=candidate;require(debug,"Command tree actual debugger absent");const auto previous=debug->paused;
         for(auto it=observers.begin();it!=observers.end();++it)require(window.setBreakpoint(it.key(),true),"Command tree breakpoint rejected");
         debug->paused=[&](const QString &type){if(previous)previous(type);try{auto *dialog=window.findChild<QDialog *>("debuggerDialog");require(dialog&&dialog->isVisible(),"Actual command tree native debugger absent");acceptanceView(*dialog);const int index=dialog->property("commandIndex").toInt();require(observers.contains(index),"Unselected command paused during tree witness");const auto selected=observers[index];const int serial=samples.size()/2;require(serial<calls(position)&&selected.second==(samples.size()%2==1)&&selected.first==(position=="Many"?serial:0),"Command tree execution order/count differs");const auto state=sample(QString::number(serial)+(selected.second?"-after":"-before"));const auto want=expected(family,position,serial,selected.second);equal(state,want);samples.append(QJsonObject{{"probe",selected.first},{"after",selected.second},{"serial",serial},{"actual",state},{"expected",want}});save(position+"-samples.json",QJsonDocument(samples).toJson());}catch(...){failure=std::current_exception();QTimer::singleShot(0,&window,[&]{window.stopMission();});}QTimer::singleShot(0,&window,[&]{debug->resume();});};
         QTimer deadline;deadline.setSingleShot(true);QObject::connect(&deadline,&QTimer::timeout,&window,[&]{failure=std::make_exception_ptr(std::runtime_error("Command tree native observation watchdog expired"));window.stopMission();});const int beforeRun=source->document()->availableUndoSteps();messages->clear();deadline.start(30000);acceptanceView(window);const auto status=window.runMission();deadline.stop();debug->paused=previous;events();save(position+"-messages.txt",messages->toPlainText().toUtf8());if(failure)std::rethrow_exception(failure);require(status==MainWindow::RunResult::Completed&&samples.size()==calls(position)*2,"Command tree mission failed or execution count differs");
         const auto final=sample("final");auto want=expected(family,position,calls(position)-1,true);auto scalar=want["state"].toArray();scalar[7]=position=="Empty"?41:42;scalar[8]=position=="Empty"?0:2;scalar[9]=position=="Empty"?0:20;want["state"]=scalar;
         // Finite-burn activation is qualified at the immediate native boundary;
         // completion cleanup is recorded independently rather than assumed.
         want["firing"]=final["firing"];equal(final,want);
         require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==beforeRun&&read(saved)==wanted.toUtf8()&&!window.isRunning(),"Command tree Run changed source/history/file or completion state");save(position+"-results.json",QJsonDocument(QJsonObject{{"position",position},{"family",family},{"samples",samples},{"final",final},{"expected_final",want},{"completed",true}}).toJson());rawValues+=11*(samples.size()+1);qualified.append(positionSpec);for(auto it=observers.begin();it!=observers.end();++it)require(window.setBreakpoint(it.key(),false),"Command tree breakpoint cleanup failed");
      }
      const QJsonObject proof{{"checkpoint","command-tree-"+id},{"outcome","passed"},{"case",id},{"family",family},{"group",spec["group"]},{"positions",qualified},{"missions",qualified.size()},{"insertions",commits},{"cancelled_drafts",cancels},{"raw_native_values",rawValues},{"facets",QJsonArray{"actual-context-menu-template-popup-and-source-editor-insertion","cancel-positive-ok-clean-apply-source-history-isolation","literal-source-undo-redo-unicode-save-reload","native-visible-parent-and-observational-successor-placement","independent-before-after-circle-impulse-scalar-execution-order","report-save-toggle-global-and-finite-activation-results"}},{"scope","Nine selected tree positions for eight ordinary command families,with one non-mutating Count assignment after each selected command for direct native observation. Reused command-reference resource prefixes and established control-tree insertion flow. Independent point-mass circle for Propagate,inertial impulse for Maneuver,Report/Save scalar values,Toggle/native Global flags,and finite-thruster immediate activation;finite cases have no propagated burn interval and inactive End in an empty mission is an explicit no-op domain. Source editor configures the command after actual template selection;other typed fields retain dedicated tests. Solver contexts,arbitrary branch families,full field-loop mutation,completion burn cleanup,native OS input/science/hardware/platform/stress/current NASA/full newest-build integration remain open."}};
      save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}
     catch (std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}
   return 1;
}
