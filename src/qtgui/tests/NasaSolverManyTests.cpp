#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidgetItemIterator>
using namespace formaccept;

// Reuse the established context-menu/typed editor/history/native clone flow.
// All inserted solver leaves are direct children of legal solver branches.
static void reveal(QWidget *w)
{
 require(w,"Solver-many control absent");
 for(QWidget *p=w;p;p=p->parentWidget())if(auto *s=qobject_cast<QScrollArea *>(p))s->ensureWidgetVisible(w);
 events();require(w->isVisible()&&w->isEnabled(),"Solver-many control unreachable");
}
static void nativeWalk(GmatCommand *first,const QString &parent,QSet<GmatCommand *> &seen,
                       QJsonArray &tree,QMap<QString,GmatCommand *> &named)
{
 for(auto *c=first;c&&!seen.contains(c);c=c->GmatCommand::GetNext()) {
  seen.insert(c);const auto name=QString::fromStdString(c->GetName());
  tree.append(QJsonObject{{"type",QString::fromStdString(c->GetTypeName())},{"label",name},{"parent",parent}});
  if(!name.isEmpty()){require(!named.contains(name),"Solver-many native label ambiguous");named[name]=c;}
  for(int branch=0;auto *child=c->GetChildCommand(branch);++branch)nativeWalk(child,name,seen,tree,named);
 }
}
int main(int argc,char **argv)
{
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaSolverMany");
 try {
  require(argc==4&&acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen","Supply startup,cases,key on native widgets");
  QJsonObject spec;for(const auto &v:QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object()["cases"].toArray())if(v.toObject()["key"].toString()==argv[3])spec=v.toObject();
  require(!spec.isEmpty(),"Solver-many case absent");const QString kind=spec["key"].toString();
  require(QStringList{"VaryTarget","VaryOptimize","Achieve","Minimize","NonlinearConstraint"}.contains(kind),"Solver-many kind absent");
  const bool target=kind=="VaryTarget"||kind=="Achieve",vary=kind.startsWith("Vary"),four=target||kind=="VaryOptimize",manyObjectives=kind=="Minimize";
  const QString type=vary?"Vary":kind,solver=target?"ZDC":"ZOpt";const int branches=manyObjectives?3:1;
  TestSettings settings;QTemporaryDir files;require(files.isValid(),"Solver-many fixture absent");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath(),saved=files.filePath("Solver many Ω.script"),report=files.filePath("Many Δ.csv");
  QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;window.resize(1200,850);window.show();require(window.initialize(startup),"Solver-many runtime absent");events();acceptanceView(window);
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&area,"Solver-many workspace absent");
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes){if(!evidence.isEmpty())write(evidence+"/"+name,bytes);};save("case.json",QJsonDocument(spec).toJson());
  const QStringList names{"U","V","W","X","Q1","Q2","Q3","Q4","Cost","Keep"};
  auto configured=[&]{QJsonArray a;for(const auto &name:names)a.append(Moderator::Instance()->GetConfiguredObject(name.toStdString())->GetRealParameter("Value"));return a;};
  auto control=[&](const QString &label,const QString &name,const QString &initial){return "Vary '"+label+"' "+solver+"("+name+" = "+initial+", {Perturbation = "+(target?"0.01":"0.000001")+", Lower = -10, Upper = 10, MaxStep = 10});";};
  QString initial="% Solver many Ω\nCreate Variable U V W X Q1 Q2 Q3 Q4 Cost Keep;\nU = 1;\nV = 0;\nW = 0;\nX = 0;\nQ1 = 0;\nQ2 = 0;\nQ3 = 0;\nQ4 = 0;\nCost = 0;\nKeep = 41;\nCreate Spacecraft Sat;\nCreate DifferentialCorrector ADC ZDC;\nADC.ShowProgress = false;\nZDC.ShowProgress = false;\nZDC.MaximumIterations = 30;\nZDC.DerivativeMethod = CentralDifference;\nCreate Yukon AOpt ZOpt;\nZOpt.ShowProgress = false;\nZOpt.FeasibilityTolerance = 0.00000001;\nZOpt.OptimalityTolerance = 0.00000001;\nZOpt.FunctionTolerance = 0.00000001;\n";
  for(const auto &name:QStringList{"ADC","ZDC","AOpt","ZOpt"})initial+=name+".ReportFile = '"+files.filePath(name+".txt")+"';\n";
  initial+="Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\n";
  QStringList closingAnchors;
  for(int b=0;b<branches;++b){
   const auto index=QString::number(b);initial+=(target?"Target":"Optimize")+QString(" 'Solver")+index+"' "+solver+" {SolveMode = Solve, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\n";
   initial+=control("Original control"+index,"U","1")+"\n";
   if(kind=="Achieve")for(const auto &name:QStringList{"V","W","X"})initial+=control("Original "+name,name,"0")+"\n";
   initial+="'Compute first"+index+"' Q1 = 2 * U + V;\nQ2 = V - W;\nQ3 = W + X;\nQ4 = X;\n";
   if(target){
    if(kind=="VaryTarget")initial+="Achieve 'Original Q1' ZDC(Q1 = 6, {Tolerance = 0.000000001});\nAchieve 'Original Q2' ZDC(Q2 = -5, {Tolerance = 0.000000001});\nAchieve 'Original Q3' ZDC(Q3 = 4, {Tolerance = 0.000000001});\n";
    initial+="Achieve 'Original Q4' ZDC(Q4 = 1, {Tolerance = 0.000000001});\n";
   } else {
    initial+=kind=="VaryOptimize"?"Cost = (U - 2)^2 + (V + 2)^2 + (W - 3)^2 + (X - 1)^2;\n":"Cost = (U - "+QString::number(b+2)+")^2;\n";
    if(manyObjectives)initial+="NonlinearConstraint 'Original constraint"+index+"' ZOpt(U >= "+QString::number(b+1)+");\n";
    else initial+="Minimize 'Original objective' ZOpt(Cost);\n";
   }
   const QString closing=(target?"EndTarget":"EndOptimize")+QString("; % Close ")+index+"\n";closingAnchors<<closing;initial+=closing+"Report 'Witness"+index+"' Values U V W X Q1 Q2 Q3 Q4 Cost Keep;\n";
  }
  initial+="% Preserve tail Δ\n";write(saved,initial.toUtf8());save("initial.script",initial.toUtf8());require(window.loadScript(saved)&&window.buildScript(),"Solver-many initial build failed");
  const auto base=configured();require(base==QJsonArray{1,0,0,0,0,0,0,0,0,41},"Solver-many baseline differs");source->document()->setModified(false);QString working=initial;
  QJsonArray edits,typedFields,rejections,capacity;
  auto named=[&](const QString &label){const auto s=window.missionSnapshot();int at=-1;for(int i=0;i<s.nodes.size();++i)if(missionCommandLabel(s.nodes[i].statement).name==label){require(at<0,"Solver-many label ambiguous");at=i;}require(at>=0,"Solver-many node absent");return at;};
  auto item=[&](int at){for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==at)return *it;throw std::runtime_error("Solver-many tree item absent");};
  auto open=[&](int at){
   for(auto *tabs:window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree))tabs->setCurrentIndex(i);
   auto *selected=item(at);tree->expandAll();tree->setCurrentItem(selected);tree->scrollToItem(selected);events();reveal(tree);acceptanceView(window);std::exception_ptr failure;bool visited=false;
   QTimer::singleShot(0,&window,[&]{auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());try{require(menu,"Solver-many actual popup absent");QAction *choice=nullptr;for(auto *a:menu->actions())if(a->text()=="Insert before…")choice=a;require(choice&&choice->isEnabled(),"Solver-many insertion action absent");menu->setActiveAction(choice);acceptanceView(*menu);visited=true;QTest::keyClick(menu,Qt::Key_Return);}catch(...){failure=std::current_exception();if(menu)menu->close();}});
   const auto point=tree->visualItemRect(selected).center();QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));QApplication::sendEvent(tree->viewport(),&event);events();if(failure)std::rethrow_exception(failure);require(visited,"Solver-many popup not visited");auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Solver-many actual editor absent");child->showNormal();child->move(0,0);child->resize(area->viewport()->size());events();acceptanceView(*child);return child;
  };
  auto click=[&](QMdiSubWindow *child,QDialogButtonBox::StandardButton which){auto *box=child->findChild<QDialogButtonBox *>();auto *button=box?box->button(which):nullptr;reveal(button);acceptanceView(*child);QTest::mouseClick(button,Qt::LeftButton);events();};
  auto branchEnd=[&](int b){return missionBranchEnd(window.missionSnapshot(),named("Solver"+QString::number(b)));};
  for(int p=0;p<3;++p){
   const int branch=manyObjectives?p:0;const auto index=QString::number(p),label="Probe"+index;
   QString statement;
   if(vary)statement=control(label,QStringList{"V","W","X"}[p],"0");
   else if(kind=="Achieve")statement="Achieve '"+label+"' ZDC(Q"+QString::number(p+1)+" = "+QStringList{"6","-5","4"}[p]+", {Tolerance = 0.000000001});";
   else if(manyObjectives)statement="Minimize '"+label+"' ZOpt(Cost);";
   else statement="NonlinearConstraint '"+label+"' ZOpt(U "+(p==1?"<=":">=")+" "+QStringList{"3","7","4"}[p]+");";
   auto configure=[&](QMdiSubWindow *child){
    auto *combo=child->findChild<QComboBox *>("commandTemplate");reveal(combo);const int at=combo->findText(type);require(at>=0,"Solver-many legal template absent");QTest::mouseClick(combo,Qt::LeftButton,Qt::NoModifier,QPoint(combo->width()-10,combo->height()/2));events();require(combo->view()->isVisible(),"Solver-many template popup absent");acceptanceView(*combo->view()->window());QTest::keyClick(combo->view(),Qt::Key_Home);for(int i=0;i<at;++i)QTest::keyClick(combo->view(),Qt::Key_Down);QTest::keyClick(combo->view(),Qt::Key_Return);events();
    auto *draft=child->findChild<QPlainTextEdit *>("commandSource");require(draft&&draft->toPlainText().startsWith(type+" "+solver+"("),"Solver-many enclosing solver default differs");QString seed=statement;
    if(vary)seed=control(label,"U","1");else if(kind=="Achieve")seed="Achieve '"+label+"' ZDC(Cost = 0, {Tolerance = 0.000000001});";else if(manyObjectives)seed.replace("(Cost)","(Q1)");else seed.replace(QStringList{"3","7","4"}[p]+");","0);");draft->setPlainText(seed);events();
    auto field=[&](const QString &name,const QString &value){auto *w=child->findChild<QLineEdit *>("commandField_"+name);reveal(w);acceptanceView(*child);QTest::mouseClick(w,Qt::LeftButton);w->selectAll();QTest::keyClicks(w,value);events();require(w->text()==value,"Solver-many typed field differs");typedFields.append(QJsonObject{{"insertion",p},{"field",name},{"value",value}});};
    if(vary){field("Variable",QStringList{"V","W","X"}[p]);field("Initial value","0");}else if(kind=="Achieve"){field("Goal","Q"+QString::number(p+1));field("Value",QStringList{"6","-5","4"}[p]);}else if(manyObjectives)field("Objective","Cost");else field("Right side",QStringList{"3","7","4"}[p]);
    require(draft->toPlainText()==statement&&dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Solver-many typed draft differs from literal source oracle");acceptanceView(*child);
   };
   auto location=[&]{return vary?named("Compute first0"):branchEnd(branch);};
   const auto before=working;const int steps=source->document()->availableUndoSteps();QPointer<QMdiSubWindow> child=open(location());configure(child);
   require(source->toPlainText()==before&&source->document()->availableUndoSteps()==steps&&configured()==base&&read(saved)==initial.toUtf8(),"Solver-many pending insertion escaped isolation");click(child,QDialogButtonBox::Cancel);require(!child&&source->toPlainText()==before&&configured()==base,"Solver-many Cancel escaped isolation");child=open(location());configure(child);
   const QString anchor=vary?"'Compute first0' Q1 = 2 * U + V;\n":closingAnchors[branch];require(working.count(anchor)==1,"Solver-many insertion anchor ambiguous");working.insert(working.indexOf(anchor),statement+"\n");
   QPointer<CommandEditor> retired=dynamic_cast<CommandEditor *>(child->widget());click(child,QDialogButtonBox::Apply);
   if(retired||!child||source->toPlainText()!=working){save("failed-actual.script",source->toPlainText().toUtf8());save("failed-expected.script",working.toUtf8());save("failed-messages.txt",messages->toPlainText().toUtf8());}
   require(!retired&&child&&source->toPlainText()==working&&source->document()->availableUndoSteps()>steps,"Solver-many Apply changed wrong source or retained stale editor");
   auto *owner=dynamic_cast<CommandEditor *>(child->widget());auto *draft=child->findChild<QPlainTextEdit *>("commandSource");require(owner&&!owner->hasChanges()&&draft&&draft->toPlainText()==statement,"Solver-many Apply did not rebind the clean editor to its accepted command");
   const int cleanSteps=source->document()->availableUndoSteps();click(child,QDialogButtonBox::Apply);require(source->toPlainText()==working&&source->document()->availableUndoSteps()==cleanSteps&&!owner->hasChanges(),"Solver-many clean Apply duplicated command/history");click(child,QDialogButtonBox::Ok);require(!child,"Solver-many OK retained owner");
   source->undo();require(source->toPlainText()==before&&window.buildScript()&&configured()==base,"Solver-many Undo failed");source->redo();require(source->toPlainText()==working&&window.buildScript()&&configured()==base,"Solver-many Redo failed");
   const auto model=window.missionSnapshot();const int at=named(label);require(model.nodes[at].type==type&&model.nodes[at].parent==named("Solver"+QString::number(branch))&&item(at)->parent()==item(named("Solver"+QString::number(branch))),"Solver-many visible/model command parent differs");
   edits.append(QJsonObject{{"insertion",p},{"branch",branch},{"statement",statement},{"cancelled",true},{"stale_editor_retired",true},{"clean_apply_isolated",true},{"source_history_exact",true}});
   if(manyObjectives){
    const int undo=source->document()->availableUndoSteps();child=open(branchEnd(branch));auto *combo=child->findChild<QComboBox *>("commandTemplate");require(combo&&combo->findText("Minimize")<0,"Solver-many second objective remains offered");capacity.append(QJsonObject{{"branch",branch},{"offered_after_first",false}});
    const auto invalid="Minimize 'Forbidden"+index+"' ZOpt(Cost);";draft=child->findChild<QPlainTextEdit *>("commandSource");require(draft,"Solver-many bypass source absent");draft->setPlainText(invalid);events();
    for(auto which:{QDialogButtonBox::Apply,QDialogButtonBox::Ok}){QPointer<CommandEditor> failedOwner=dynamic_cast<CommandEditor *>(child->widget());click(child,which);auto *status=failedOwner?failedOwner->findChild<QLabel *>("commandStatus"):nullptr;require(failedOwner&&child&&failedOwner->hasChanges()&&draft->toPlainText()==invalid&&status&&!status->text().isEmpty()&&source->toPlainText()==working&&source->document()->availableUndoSteps()==undo&&configured()==base&&read(saved)==initial.toUtf8(),"Solver-many duplicate objective changed accepted source/history/model/file or lost draft/diagnostic");require(status->text().contains("one Minimize")||messages->toPlainText().contains("one Minimize"),"Solver-many duplicate objective lacks native capacity diagnostic");rejections.append(QJsonObject{{"branch",branch},{"action",which==QDialogButtonBox::Apply?"Apply":"OK"},{"draft",invalid},{"diagnostic",status->text()},{"isolated",true}});}
    click(child,QDialogButtonBox::Cancel);require(!child,"Solver-many rejected objective did not cancel");
   }
  }
  require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==working&&read(saved)==working.toUtf8()&&configured()==base,"Solver-many Unicode persistence failed");save("mission.script",working.toUtf8());save("edits.json",QJsonDocument(edits).toJson());save("typed-fields.json",QJsonDocument(typedFields).toJson());save("rejections.json",QJsonDocument(rejections).toJson());save("capacity.json",QJsonDocument(capacity).toJson());
  const auto model=window.missionSnapshot();QJsonArray hierarchy;for(const auto &n:model.nodes)hierarchy.append(QJsonObject{{"type",n.type},{"label",missionCommandLabel(n.statement).name},{"parent",n.parent},{"statement",n.statement}});save("hierarchy.json",QJsonDocument(hierarchy).toJson());
  const int undo=source->document()->availableUndoSteps();messages->clear();acceptanceView(window);const auto status=window.runMission();events();save("messages.txt",messages->toPlainText().toUtf8());require(status==MainWindow::RunResult::Completed,"Solver-many witness failed");
  const auto raw=read(report);save("values.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==branches,"Solver-many witness row count differs");QJsonArray actualRows,expectedRows;
  for(int b=0;b<branches;++b){
   const double u=target?4:kind=="VaryOptimize"?2:manyObjectives?b+2:4,v=four?-2:0,w=four?3:0,x=four?1:0,cost=kind=="NonlinearConstraint"?4:0;
   const QJsonArray expected{u,v,w,x,2*u+v,v-w,w+x,x,cost,41};const auto cells=lines[b].split(',');require(cells.size()==10,"Solver-many witness width differs");QJsonArray actual;
   for(int i=0;i<10;++i){bool ok=false;const double value=cells[i].toDouble(&ok);if(!ok||!std::isfinite(value)||std::abs(value-expected[i].toDouble())>(i==8?1e-5:2e-6))std::cerr<<"MANY_DIFF case="<<kind.toStdString()<<" branch="<<b<<" column="<<i<<" actual="<<value<<" expected="<<expected[i].toDouble()<<std::endl;require(ok&&std::isfinite(value)&&std::abs(value-expected[i].toDouble())<(i==8?1e-5:2e-6),"Solver-many result differs from independent full-rank linear or constrained-quadratic solution");actual.append(value);}
   if(kind=="NonlinearConstraint")require(actual[0].toDouble()>=3-1e-7&&actual[0].toDouble()<=7+1e-7&&actual[0].toDouble()>=4-1e-7,"Solver-many violates an original independently checked constraint");
   actualRows.append(actual);expectedRows.append(expected);
  }
  QJsonArray native;for(int i=0;i<names.size();++i){const double value=Moderator::Instance()->GetInternalObject(names[i].toStdString())->GetRealParameter("Value");require(std::isfinite(value)&&std::abs(value-actualRows.last().toArray()[i].toDouble())<1e-12,"Solver-many native final state differs from report");native.append(value);}
  QJsonArray engine;QSet<GmatCommand *> seen;QMap<QString,GmatCommand *> commands;nativeWalk(Moderator::Instance()->GetFirstCommand(),"",seen,engine,commands);save("native-tree.json",QJsonDocument(engine).toJson());
  for(int p=0;p<3;++p){int count=0;for(const auto &v:engine){const auto n=v.toObject();if(n["label"]=="Probe"+QString::number(p)){require(n["type"]==type&&n["parent"]=="Solver"+QString::number(manyObjectives?p:0),"Solver-many native inserted parent/type differs");++count;}}require(count==1,"Solver-many native insertion duplicated or disappeared");}
  QJsonArray solvers;
  for(int b=0;b<branches;++b){
   auto *branch=dynamic_cast<SolverBranchCommand *>(commands.value("Solver"+QString::number(b)));auto *clone=branch?dynamic_cast<Solver *>(branch->GetClone(0)):nullptr;require(clone&&clone->GetName()==solver.toStdString()&&clone->GetTypeName()==(target?"DifferentialCorrector":"Yukon")&&clone->GetIntegerParameter(clone->GetParameterID("IntegerSolverStatus"))==Gmat::CONVERGED,"Solver-many actual clone did not converge");
   QJsonArray variables,values,goals;for(const auto &name:clone->GetStringArrayParameter(clone->GetParameterID("Variables")))variables.append(QString::fromStdString(name));const int count=clone->GetIntegerParameter(clone->GetParameterID("NumberOfVariables"));require(count==(four?4:1)&&variables==(four?QJsonArray{"U","V","W","X"}:QJsonArray{"U"}),"Solver-many actual control count/order differs");for(int i=0;i<count;++i){const double value=clone->GetSolverVariable(i);require(std::isfinite(value)&&std::abs(value-expectedRows[b].toArray()[i].toDouble())<2e-6,"Solver-many actual control differs from independent solution");values.append(value);}
   for(const auto &name:clone->GetStringArrayParameter(clone->GetParameterID(target?"Goals":"InequalityConstraintNames")))goals.append(QString::fromStdString(name));
   const QJsonArray expectedGoals=target?(kind=="Achieve"?QJsonArray{"Q4","Q1","Q2","Q3"}:QJsonArray{"Q1","Q2","Q3","Q4"}):kind=="NonlinearConstraint"?QJsonArray{"U","U","U"}:manyObjectives?QJsonArray{"U"}:QJsonArray{};
   if(goals!=expectedGoals)std::cerr<<"MANY_GOALS "<<QJsonDocument(goals).toJson(QJsonDocument::Compact).constData()<<std::endl;
   require(goals==expectedGoals,"Solver-many actual goal registration identity/order differs");
   const auto objective=target?QString():QString::fromStdString(clone->GetStringParameter(clone->GetParameterID("ObjectiveFunction")));require(target||objective=="Cost","Solver-many actual objective identity differs");
   solvers.append(QJsonObject{{"branch",b},{"name",solver},{"type",QString::fromStdString(clone->GetTypeName())},{"status",1},{"variables",variables},{"values",values},{"goals",goals},{"objective",objective},{"count",count}});
  }
  require(source->toPlainText()==working&&source->document()->availableUndoSteps()==undo&&read(saved)==working.toUtf8()&&configured()==base&&!window.isRunning(),"Solver-many Run mutated source/history/file/configured state");
  const QJsonObject result{{"case",kind},{"actual",actualRows},{"expected",expectedRows},{"native",native},{"solvers",solvers}};save("native-result.json",QJsonDocument(result).toJson());
  QJsonArray facets{"actual-popup-template-and-repeated-positive-typed-insertion","cancel-pending-source-history-unicode-isolation","apply-rebind-stale-editor-retirement-and-clean-apply-idempotency","literal-model-visible-and-native-command-count-order","independent-full-rank-linear-and-convex-quadratic-results","actual-converged-control-goal-objective-registration"};
  if(manyObjectives)facets.append("one-objective-per-sequence-capacity-bypass-rejection");
  const QJsonObject proof{{"checkpoint","solver-many-"+kind},{"outcome","passed"},{"case",kind},{"historical_key",spec["historical_key"]},{"insertions",3},{"cancelled_drafts",3},{"clean_apply_checks",3},{"branches",branches},{"rejected_actions",rejections.size()},{"typed_fields",typedFields.size()},{"actual",actualRows},{"facets",facets},{"scope","Three bounded repeated direct-solver insertions. Four-variable linear Target/separable quadratic Optimize and three intersecting scalar constraints;one Minimize in each of three independent sequences plus second-objective Apply/OK rejection. No exact private AddMany recipe,arbitrary multiplicity/solver/regime,nested source-conflict resolution,native OS/GPU/hardware/platform/stress/current NASA/full newest-build inference."}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;source->document()->setModified(false);return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
  catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
 return 1;
}
