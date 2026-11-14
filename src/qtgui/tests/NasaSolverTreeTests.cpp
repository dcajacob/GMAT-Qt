#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include "Debugger.hpp"
#include "FiniteBurn.hpp"
#include "Subscriber.hpp"
#include "FileManager.hpp"
#include "Sandbox.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
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
// Reuse the ordinary command-tree GUI flow without replaying its passing cases.
// The selected command is a complete, legal solver branch. Its observer follows
// the branch terminator,so the after boundary proves actual completed solving.
static QString probe(const QString &family,int p)
{
 const auto label="Probe"+QString::number(p),offset=QString::number(2*p),optimum=QString::number(2+p);QString statement;
 if(family=="Target")statement="Target '"+label+"' DC {SolveMode = Solve, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\nVary DC(U = 1, {Perturbation = 0.01, Lower = -10, Upper = 10, MaxStep = 10});\nQuantity = 2 * U + 1 - 2 * I - "+offset+";\nAchieve DC(Quantity = 9, {Tolerance = 0.000000001});\nEndTarget;";
 else statement="Optimize '"+label+"' Opt {SolveMode = Solve, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\nVary Opt(U = 1, {Perturbation = 0.000001, Lower = 0, Upper = 10, MaxStep = 10});\nCost = (U - I - "+optimum+")^2;\nMinimize Opt(Cost);\nEndOptimize;";
 return statement+"\n'Observe"+QString::number(p)+"' Count = Count + 0;";
}
static int calls(const QString &position,const QString &parentKind="If"){return parentKind=="IfElse"?1:position=="Many"?3:QStringList{"BranchAppend","EndBefore","LeafBefore","LeafAfter"}.contains(position)?2:1;}
static int loopIndex(const QString &position,int execution,const QString &parentKind="If"){return parentKind=="IfElse"?((position=="ElseBefore"||position=="BranchAppend")?1:2):QStringList{"BranchAppend","EndBefore","LeafBefore","LeafAfter"}.contains(position)?execution+1:QStringList{"Many","BlockAfter","EndAfter"}.contains(position)?2:0;}
static QJsonObject expected(const QString &family,const QString &position,int serial,bool after,const QString &parentKind="If")
{
 const int i=loopIndex(position,serial,parentKind),p=position=="Many"?serial:0;
 double u=1,quantity=(position=="Empty"||position=="BlockBefore")?0:i*10;
 if(serial>0){const int previousI=loopIndex(position,serial-1,parentKind),previousP=position=="Many"?serial-1:0;u=(family=="Target"?4:2)+previousI+previousP;if(family=="Target"&&position=="Many")quantity=9;}
 if(after){u=(family=="Target"?4:2)+i+p;if(family=="Target")quantity=9;}
 QJsonArray state{0,7000,0,0,0,std::sqrt(mu/7000.),0,position=="Many"?42:41,i,quantity,0,u,0};
 return {{"state",state}};
}
static void equal(const QJsonObject &actual,const QJsonObject &want,bool after)
{
 const auto a=actual["state"].toArray(),b=want["state"].toArray();require(a.size()==13&&b.size()==13,"Solver tree native state width differs");
 for(int i=0;i<13;++i){const double v=a[i].toDouble(),e=b[i].toDouble(),tol=i==11?2e-6:i==12?1e-10:i==9?1e-8:1e-10;if(!std::isfinite(v)||std::abs(v-e)>tol)std::cerr<<"SOLVER_TREE_DIFF column="<<i<<" actual="<<v<<" expected="<<e<<std::endl;require(std::isfinite(v)&&std::abs(v-e)<=tol,"Solver tree state differs from independent linear/quadratic oracle");}
 if(after){const auto clone=actual["solver"].toObject();require(clone["status"].toInt()==Gmat::CONVERGED&&clone["variables"].toArray()==QJsonArray{"U"}&&clone["count"].toInt()==1,"Inserted solver branch did not converge with one actual registered control");require(std::isfinite(clone["u"].toDouble())&&std::abs(clone["u"].toDouble()-b[11].toDouble())<2e-6,"Actual solver control differs from independent linear/quadratic optimum");}
}
// Follow the source-owned links explicitly: virtual GetNext can return a solver
// dispatcher itself while executing. This read-only walk cannot advance it.
static GmatCommand *namedNative(GmatCommand *first,const QString &name,QSet<GmatCommand *> &seen)
{
 for(auto *command=first;command&&!seen.contains(command);command=command->GmatCommand::GetNext()){
  seen.insert(command);if(QString::fromStdString(command->GetName())==name)return command;
  for(int branch=0;auto *child=command->GetChildCommand(branch);++branch)if(auto *found=namedNative(child,name,seen))return found;
 }
 return nullptr;
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaSolverTree");
   try {
      require(argc==5&&acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen",
              "Supply startup, reused command cases, insertion cases and key on a native widget platform");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      const auto spec=entry(QFileInfo(argv[3]).absoluteFilePath(),argv[4]);
      const auto reused=entry(QFileInfo(argv[2]).absoluteFilePath(),spec["reused_reference_case"].toString());
      const auto family=spec["family"].toString(),id=spec["key"].toString();
      const auto parentLabel=spec["parent"].toString("Outer");require(parentLabel=="Outer"||parentLabel=="Inner","Explicit insertion parent identity differs");
      const auto parentKind=spec["parent_type"].toString("If");require(parentKind=="If"||parentKind=="While"||parentKind=="IfElse","Explicit inner parent type differs");require(parentKind!="While"||parentLabel=="Inner","While parent requires explicit Inner target");require(parentKind!="IfElse"||parentLabel=="Inner","IfElse parent requires explicit Inner target");
      require(QStringList{"Target","Optimize"}.contains(family)&&reused["command"]==family,"Command tree reused command identity differs");
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
      QString prefix=resolve(reused["script"].toString().section("BeginMissionSequence;",0,0))+"BeginMissionSequence;\n";
      require(prefix.contains("Keep = 41;")&&prefix.contains("I = 0;")&&prefix.contains("Quantity = 0;")&&prefix.contains("Sat.X = 7000;"),"Reused native fixture differs from independent literal inputs");
      if(parentKind=="While")prefix.replace("BeginMissionSequence;\n","Create Variable ParentPass ParentEntries;\nParentPass = 0;\nParentEntries = 0;\nBeginMissionSequence;\n");
      if(parentKind=="IfElse")prefix.replace("BeginMissionSequence;\n","Create Variable ParentTrue ParentFalse;\nParentTrue = 0;\nParentFalse = 0;\nBeginMissionSequence;\n");
      const QString outer="For 'Outer' I = 1:1:2;\n",inner=parentKind=="IfElse"?"If 'Inner' I == 1;\nParentTrue = ParentTrue + 1;\n":parentKind=="While"?"While 'Inner' ParentPass < 1;\nParentPass = ParentPass + 1;\nParentEntries = ParentEntries + 1;\n":"If 'Inner' I > 0;\n",leaf="'Leaf' Keep = Keep + 0;\n";
      const QString endIf="EndIf;\n",endWhile="EndWhile;\n",endFor="EndFor;\n";
      const QString body=outer+"Quantity = I * 10;\n"+(parentKind=="While"?QString("ParentPass = 0;\n"):QString())+inner+leaf+(parentKind=="IfElse"?QString("Else;\nParentFalse = ParentFalse + 1;\n'Alternate' Keep = Keep + 0;\n"):QString())+(parentKind=="While"?endWhile:endIf)+endFor+"'After outer' Keep = Keep + 1;\n";
      int commits=0,cancels=0,rawValues=0;QJsonArray qualified;
      for (const auto &value:spec["positions"].toArray()) {
         const auto positionSpec=value.toObject();const auto position=positionSpec["position"].toString();
         require(parentLabel=="Outer"||position=="BranchAppend"||position=="EndBefore"||(parentKind=="IfElse"&&(position=="ElseBefore"||position=="ElseAfter")),"Inner parent only supports explicitly qualified positions");
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
            else if (position=="BranchAppend") {target=item(named(parentLabel));action="appendInsideMissionBranch";}
            else if(position=="ElseBefore"||position=="ElseAfter") {
               const auto snapshot=window.missionSnapshot();const int parent=named("Inner");int selected=-1;
               for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].type=="Else"&&snapshot.nodes[i].parent==parent){require(selected<0,"Parent Else identity ambiguous");selected=i;}
               require(selected>=0,"Parent Else absent");target=item(selected);action=position=="ElseBefore"?"Insert before…":"Insert after…";
            }
            else if (position=="BlockBefore"||position=="BlockAfter") {target=item(named("Outer"));action=position=="BlockBefore"?"Insert before…":"Insert after…";}
            else if (position=="LeafBefore"||position=="LeafAfter") {target=item(named("Leaf"));action=position=="LeafBefore"?"Insert before…":"Insert after…";}
            else {
               const auto snapshot=window.missionSnapshot();const int parent=named(parentLabel);int closing=-1;
               for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type==(parentLabel=="Inner"?(parentKind=="While"?"EndWhile":"EndIf"):"EndFor")&&snapshot.nodes[i].parent==parent) closing=i;
               require(closing>=0,"Command tree outer terminator absent");target=item(closing);action=position=="EndBefore"?"Insert before…":"Insert after…";
            }
            require(target,"Command tree target absent");tree->expandAll();tree->setCurrentItem(target);tree->scrollToItem(target);events();reveal(tree);
            if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {
               window.raise();window.activateWindow();
               require(QTest::qWaitForWindowActive(window.windowHandle(),5000),"Native context-menu parent did not become active");
               events();
            }
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
               const auto anchor=position=="ElseBefore"||position=="ElseAfter"||(parentKind=="IfElse"&&position=="BranchAppend")?QString("Else;\n"):position=="BlockBefore"?outer:position=="LeafBefore"||position=="LeafAfter"?leaf:parentLabel=="Inner"?(parentKind=="While"?endWhile:endIf):endFor;
               require(wanted.count(anchor)==1,"Independent insertion source anchor ambiguous");
               const bool after=position=="BlockAfter"||position=="EndAfter"||position=="LeafAfter"||position=="ElseAfter";
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
         auto parentLoopState=[&]{auto value=[](const char *name){auto *v=Moderator::Instance()->GetInternalObject(name);require(v,"Native parent loop counter absent");return v->GetRealParameter("Value");};return QJsonObject{{"i",value("I")},{"pass",value("ParentPass")},{"entries",value("ParentEntries")}};};QJsonArray parentLoopSamples;
         auto parentBranchState=[&]{auto value=[](const char *name){auto *v=Moderator::Instance()->GetInternalObject(name);require(v,"Native parent branch counter absent");return v->GetRealParameter("Value");};return QJsonObject{{"i",value("I")},{"true",value("ParentTrue")},{"false",value("ParentFalse")}};};QJsonArray parentBranchSamples;
         const auto snapshot=window.missionSnapshot();QJsonArray hierarchy;QMap<int,QPair<int,bool>> observers;
         for(int i=0;i<snapshot.nodes.size();++i){const auto &node=snapshot.nodes[i];const auto label=missionCommandLabel(node.statement).name;hierarchy.append(QJsonObject{{"type",node.type},{"label",label},{"parent",node.parent},{"statement",node.statement}});for(int p=0;p<count;++p){if(label=="Probe"+QString::number(p))observers[i]={p,false};if(label=="Observe"+QString::number(p))observers[i]={p,true};}}
         const QString parent=position=="LeafBefore"||position=="LeafAfter"||position=="ElseBefore"||position=="ElseAfter"?"Inner":position=="BranchAppend"||position=="EndBefore"?parentLabel:QString();
         for(int p=0;p<count;++p){const int at=named("Probe"+QString::number(p)),following=named("Observe"+QString::number(p));require(snapshot.nodes[at].type==family&&following==missionBranchEnd(snapshot,at)+1&&snapshot.nodes[following].parent==snapshot.nodes[at].parent,"Inserted command/observational successor native identity differs");require(parent.isEmpty()?snapshot.nodes[at].parent<0:snapshot.nodes[at].parent==named(parent),"Inserted native command has wrong parent");require(parent.isEmpty()?item(at)->parent()==tree->topLevelItem(0):item(at)->parent()==item(named(parent)),"Visible command tree parent differs from literal hierarchy");}
         require(observers.size()==count*2,"Native before/after observation point count differs");save(position+"-hierarchy.json",QJsonDocument(hierarchy).toJson());tree->expandAll();events();acceptanceView(*tree);
         auto sample=[&](int p){
            auto *engine=Moderator::Instance();auto *sat=dynamic_cast<Spacecraft *>(engine->GetInternalObject("Sat"));require(sat,"Solver tree native spacecraft absent");QJsonArray state{(sat->GetEpoch()-21545)*86400};for(const char *axis:{"X","Y","Z","VX","VY","VZ"})state.append(sat->GetRealParameter(axis));for(const char *name:{"Keep","I","Quantity","Count","U","Cost"}){auto *object=engine->GetInternalObject(name);require(object,"Solver tree native scalar absent");state.append(object->GetRealParameter("Value"));}
            QSet<GmatCommand *> seen;auto *branch=dynamic_cast<SolverBranchCommand *>(namedNative(engine->GetFirstCommand(),"Probe"+QString::number(p),seen));auto *solver=branch?dynamic_cast<Solver *>(branch->GetClone(0)):nullptr;require(solver&&solver->GetTypeName()==(family=="Target"?"DifferentialCorrector":"Yukon"),"Inserted solver branch actual clone absent or wrong backend");QJsonArray variables;for(const auto &name:solver->GetStringArrayParameter(solver->GetParameterID("Variables")))variables.append(QString::fromStdString(name));const int count=solver->GetIntegerParameter(solver->GetParameterID("NumberOfVariables"));QJsonObject clone{{"type",QString::fromStdString(solver->GetTypeName())},{"name",QString::fromStdString(solver->GetName())},{"status",solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"))},{"variables",variables},{"count",count}};if(count==1)clone["u"]=solver->GetSolverVariable(0);
            return QJsonObject{{"state",state},{"solver",clone}};
         };
         QFile::remove(report);QFile::remove(saveOutput);QJsonArray samples;std::exception_ptr failure;Debugger *debug=nullptr;for(auto *object:window.children())if(auto *candidate=dynamic_cast<Debugger *>(object))debug=candidate;require(debug,"Command tree actual debugger absent");const auto previous=debug->paused;
         for(auto it=observers.begin();it!=observers.end();++it)require(window.setBreakpoint(it.key(),true),"Command tree breakpoint rejected");
         debug->paused=[&](const QString &type){if(previous)previous(type);try{auto *dialog=window.findChild<QDialog *>("debuggerDialog");require(dialog&&dialog->isVisible(),"Actual command tree native debugger absent");acceptanceView(*dialog);const int index=dialog->property("commandIndex").toInt();require(observers.contains(index),"Unselected command paused during tree witness");const auto selected=observers[index];const int serial=samples.size()/2;require(serial<calls(position,parentKind)&&selected.second==(samples.size()%2==1)&&selected.first==(position=="Many"?serial:0),"Command tree execution order/count differs");const auto state=sample(selected.first);const auto want=expected(family,position,serial,selected.second,parentKind);equal(state,want,selected.second);samples.append(QJsonObject{{"probe",selected.first},{"after",selected.second},{"serial",serial},{"actual",state},{"expected",want}});save(position+"-samples.json",QJsonDocument(samples).toJson());if(parentKind=="IfElse"){const auto q=parentBranchState();require(q["i"].toDouble()==((position=="ElseBefore"||position=="BranchAppend")?1:2)&&q["true"].toDouble()==1&&q["false"].toDouble()==(position=="ElseBefore"||position=="ElseAfter"||position=="BranchAppend"?0:1),"Native parent branch order/count differs");parentBranchSamples.append(q);}if(parentKind=="While"){const auto q=parentLoopState();require(q["pass"].toDouble()==1&&q["entries"]==q["i"],"Native While parent entry/order differs");parentLoopSamples.append(q);}}catch(...){failure=std::current_exception();QTimer::singleShot(0,&window,[&]{window.stopMission();});}QTimer::singleShot(0,&window,[&]{debug->resume();});};
         QTimer deadline;deadline.setSingleShot(true);QObject::connect(&deadline,&QTimer::timeout,&window,[&]{failure=std::make_exception_ptr(std::runtime_error("Command tree native observation watchdog expired"));window.stopMission();});const int beforeRun=source->document()->availableUndoSteps();messages->clear();deadline.start(30000);acceptanceView(window);const auto status=window.runMission();deadline.stop();debug->paused=previous;events();save(position+"-messages.txt",messages->toPlainText().toUtf8());if(failure)std::rethrow_exception(failure);require(status==MainWindow::RunResult::Completed&&samples.size()==calls(position,parentKind)*2,"Command tree mission failed or execution count differs");
         const auto final=sample(position=="Many"?2:0);auto want=expected(family,position,calls(position,parentKind)-1,true,parentKind);auto scalar=want["state"].toArray();scalar[7]=position=="Empty"?41:42;scalar[8]=position=="Empty"?0:2;if(position=="BlockBefore"||(parentKind=="IfElse"&&(position=="ElseBefore"||position=="BranchAppend")))scalar[9]=20;want["state"]=scalar;equal(final,want,true);
         require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==beforeRun&&read(saved)==wanted.toUtf8()&&!window.isRunning(),"Solver tree Run changed source/history/file or completion state");save(position+"-results.json",QJsonDocument(QJsonObject{{"position",position},{"family",family},{"samples",samples},{"final",final},{"expected_final",want},{"completed",true}}).toJson());rawValues+=13*(samples.size()+1);qualified.append(positionSpec);if(parentKind=="IfElse"){const auto q=parentBranchState();require(q["i"].toDouble()==2&&q["true"].toDouble()==1&&q["false"].toDouble()==1,"Native parent final true/false counts differ");save(position+"-parent-branches.json",QJsonDocument(QJsonObject{{"samples",parentBranchSamples},{"final",q}}).toJson());}if(parentKind=="While"){const auto q=parentLoopState();require(q["pass"].toDouble()==1&&q["entries"].toDouble()==2&&q["i"].toDouble()==2,"Native While parent final entry/count differs");save(position+"-parent-loop.json",QJsonDocument(QJsonObject{{"samples",parentLoopSamples},{"final",q}}).toJson());}for(auto it=observers.begin();it!=observers.end();++it)require(window.setBreakpoint(it.key(),false),"Solver tree breakpoint cleanup failed");
      }
      const QJsonObject proof{{"checkpoint","solver-tree-"+id},{"outcome","passed"},{"case",id},{"family",family},{"group",spec["group"]},{"parent",parentLabel},{"parent_type",parentKind},{"positions",qualified},{"missions",qualified.size()},{"insertions",commits},{"cancelled_drafts",cancels},{"raw_native_values",rawValues},{"facets",QJsonArray{"actual-context-menu-template-and-complete-solver-branch-insertion","cancel-positive-ok-clean-apply-source-history-isolation","literal-source-undo-redo-unicode-save-reload","native-visible-parent-and-after-terminator-observer-placement","independent-before-after-linear-target-and-quadratic-optimum","actual-converged-solver-clone-control-registration"}},{"scope","Two legal Target/Optimize branch families at nine finite ordinary control-tree positions,with one non-mutating Count assignment after each complete branch for immediate native before/after observation. Reused immutable command-reference resource prefixes and ordinary GUI insertion path;literal linear target and quadratic optimum plus actual DifferentialCorrector/Yukon clone status,one registered U control,and unchanged spacecraft/sentinels. DiscardAndContinue resets initial guesses for each branch reentry. No solver leaf illegal-root equivalence,all solver contexts/backends/domains,private keyword reconstruction,native OS/GPU/scientific/hardware/platform/stress/current NASA or full newest-build inference."}};
      save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}
     catch (std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}
   return 1;
}
