#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include "Debugger.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QLabel>
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

// Reuse the native context-menu, draft/typed-field, history and debugger paths
// from NasaSolverContextTests/NasaSolverTreeTests without replaying those cases.
static void reveal(QWidget *w)
{
   require(w,"Solver leaf control absent");
   for(QWidget *p=w;p;p=p->parentWidget())
      if(auto *s=qobject_cast<QScrollArea *>(p))s->ensureWidgetVisible(w);
   events();require(w->isVisible()&&w->isEnabled(),"Solver leaf control unreachable");
}
static GmatCommand *walk(GmatCommand *first,const QString &name,const QString &parent,
                        QSet<GmatCommand *> &seen,QJsonArray &records)
{
   GmatCommand *found=nullptr;
   for(auto *c=first;c&&!seen.contains(c);c=c->GmatCommand::GetNext()) {
      seen.insert(c);const auto label=QString::fromStdString(c->GetName());
      records.append(QJsonObject{{"label",label},{"type",QString::fromStdString(c->GetTypeName())},{"parent",parent}});
      if(label==name)found=c;
      for(int branch=0;auto *child=c->GetChildCommand(branch);++branch)
         if(auto *match=walk(child,name,label,seen,records))found=match;
   }
   return found;
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaSolverLeaf");
   try {
      require(argc==4&&acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen",
              "Supply startup,cases and key on native widgets");
      QJsonObject spec;
      for(const auto &v:QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object()["cases"].toArray())
         if(v.toObject()["key"].toString()==argv[3])spec=v.toObject();
      require(!spec.isEmpty(),"Solver leaf case absent");
      const QString id=spec["key"].toString(),family=spec["family"].toString(),position=spec["position"].toString();
      const bool target=family=="VaryTarget"||family=="Achieve",vary=family.startsWith("Vary"),two=vary||family=="Achieve";
      const QString type=vary?"Vary":family,solver=target?"ZDC":"ZOpt";
      require(QStringList{"VaryTarget","VaryOptimize","Achieve","Minimize","NonlinearConstraint"}.contains(family),"Solver leaf family absent");
      TestSettings settings;QTemporaryDir files;require(files.isValid(),"Solver leaf fixture absent");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(),saved=files.filePath("Solver leaf Ω.script"),report=files.filePath("Leaf Δ.csv");
      QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;window.resize(1200,850);window.show();
      require(window.initialize(startup),"Solver leaf runtime absent");events();acceptanceView(window);
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");
      auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");
      require(source&&messages&&tree&&area,"Solver leaf workspace absent");
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &bytes){if(!evidence.isEmpty())write(evidence+"/"+name,bytes);};
      save("case.json",QJsonDocument(spec).toJson());
      const QStringList names{"U","V","Quantity","Cost","Keep","I","Bound"};
      auto configured=[&]{QJsonArray a;for(const auto &name:names)a.append(Moderator::Instance()->GetConfiguredObject(name.toStdString())->GetRealParameter("Value"));return a;};
      QString initial="% Solver leaf Ω\nCreate Variable U V Quantity Cost Keep I Bound;\nU = 1;\nV = 0;\nQuantity = 0;\nCost = 0;\nKeep = 41;\nI = 0;\nBound = 3;\nCreate Spacecraft Sat;\nCreate DifferentialCorrector ADC ZDC;\nADC.ShowProgress = false;\nZDC.ShowProgress = false;\nZDC.MaximumIterations = 30;\nZDC.DerivativeMethod = CentralDifference;\nCreate Yukon AOpt ZOpt;\nZOpt.ShowProgress = false;\nZOpt.FeasibilityTolerance = 0.00000001;\nZOpt.OptimalityTolerance = 0.00000001;\nZOpt.FunctionTolerance = 0.00000001;\n";
      for(const auto &name:QStringList{"ADC","ZDC","AOpt","ZOpt"})initial+=name+".ReportFile = '"+files.filePath(name+".txt")+"';\n";
      initial+="Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\n";
      initial+=(target?"Target":"Optimize")+QString(" 'Solver' ")+solver+" {SolveMode = Solve, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\n";
      auto varyStatement=[&](const QString &label,const QString &variable,const QString &value){return "Vary '"+label+"' "+solver+"("+variable+" = "+value+", {Perturbation = "+(target?"0.01":"0.000001")+", Lower = -10, Upper = 10, MaxStep = 10});";};
      initial+=varyStatement("Original control","U","1")+"\n";
      if(family=="Achieve")initial+=varyStatement("Second control","V","0")+"\n";
      initial+="I = 0;\nBound = 3;\nQuantity = 2 * U + 1;\n";
      if(!target)initial+=family=="VaryOptimize"?"Cost = (U - 2)^2 + (V + 2)^2;\n":"Cost = (U - 2)^2;\n";
      const QString outer="For 'Outer' I = 1:1:2;\n",inner="If 'Inner' I > 0;\n",leaf="'Leaf' Keep = Keep + 0;\n",endFor="EndFor;\n";
      initial+=outer+"Bound = 3 + I;\n";
      if(family=="Minimize")initial+="Cost = (U - 2 - I)^2;\n";
      initial+=inner+leaf+"EndIf;\n"+endFor+"Quantity = 2 * U + 1;\n";
      if(target) {
         if(vary)initial+="Achieve 'Original goal' ZDC(Quantity = 9, {Tolerance = 0.000000001});\n";
         initial+="Achieve 'Second goal' ZDC(V = -2, {Tolerance = 0.000000001});\nEndTarget;\n";
      } else {
         if(family=="VaryOptimize")initial+="Cost = (U - 2)^2 + (V + 2)^2;\n";
         if(family=="Minimize")initial+="NonlinearConstraint 'Original constraint' ZOpt(U >= 3);\n";
         else initial+="Minimize 'Original objective' ZOpt(Cost);\n";
         initial+="EndOptimize;\n";
      }
      initial+="Report 'Witness' Values U V Quantity Cost Keep I Bound;\n% Preserve tail Δ\n";
      write(saved,initial.toUtf8());save("initial.script",initial.toUtf8());
      require(window.loadScript(saved)&&window.buildScript(),"Solver leaf initial build failed");
      const auto base=configured();require(base==QJsonArray{1,0,0,0,41,0,3},"Solver leaf literal baseline differs");
      source->document()->setModified(false);
      auto named=[&](const QString &label){const auto s=window.missionSnapshot();int at=-1;for(int i=0;i<s.nodes.size();++i)if(missionCommandLabel(s.nodes[i].statement).name==label){require(at<0,"Solver leaf label ambiguous");at=i;}require(at>=0,"Solver leaf node absent");return at;};
      auto item=[&](int at){for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==at)return *it;throw std::runtime_error("Solver leaf tree item absent");};
      auto open=[&]{
         for(auto *tabs:window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree))tabs->setCurrentIndex(i);
         int at;QString action;
         if(position=="BranchAppend"){at=named("Outer");action="appendInsideMissionBranch";}
         else if(position=="BlockBefore"||position=="BlockAfter"){at=named("Outer");action=position=="BlockBefore"?"Insert before…":"Insert after…";}
         else if(position=="LeafBefore"||position=="LeafAfter"){at=named("Leaf");action=position=="LeafBefore"?"Insert before…":"Insert after…";}
         else {const auto s=window.missionSnapshot();at=missionBranchEnd(s,named("Outer"));require(at>=0&&s.nodes[at].type=="EndFor","Solver leaf terminator absent");action=position=="EndBefore"?"Insert before…":"Insert after…";}
         auto *selected=item(at);tree->expandAll();tree->setCurrentItem(selected);tree->scrollToItem(selected);events();reveal(tree);acceptanceView(window);
         std::exception_ptr failure;bool visited=false;
         QTimer::singleShot(0,&window,[&]{auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());try{require(menu,"Solver leaf popup absent");QAction *choice=nullptr;for(auto *a:menu->actions())if(a->text()==action||a->objectName()==action)choice=a;require(choice&&choice->isEnabled(),"Solver leaf insertion action absent");menu->setActiveAction(choice);acceptanceView(*menu);visited=true;QTest::keyClick(menu,Qt::Key_Return);}catch(...){failure=std::current_exception();if(menu)menu->close();}});
         const auto point=tree->visualItemRect(selected).center();QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));QApplication::sendEvent(tree->viewport(),&event);events();if(failure)std::rethrow_exception(failure);require(visited,"Solver leaf popup not visited");
         auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Solver leaf editor absent");child->showNormal();child->move(0,0);child->resize(area->viewport()->size());events();acceptanceView(*child);return child;
      };
      const QString statement=vary?varyStatement("Probe","V","0"):family=="Achieve"?"Achieve 'Probe' ZDC(Quantity = 9, {Tolerance = 0.000000001});":family=="Minimize"?"Minimize 'Probe' ZOpt(Cost);":"NonlinearConstraint 'Probe' ZOpt(U >= Bound);";
      QJsonArray typedFields;
      auto configure=[&](QMdiSubWindow *child){
         auto *combo=child->findChild<QComboBox *>("commandTemplate");reveal(combo);const int selected=combo->findText(type);require(selected>=0,"Solver leaf legal template absent");
         QTest::mouseClick(combo,Qt::LeftButton,Qt::NoModifier,QPoint(combo->width()-10,combo->height()/2));events();require(combo->view()->isVisible(),"Solver leaf template popup absent");acceptanceView(*combo->view()->window());QTest::keyClick(combo->view(),Qt::Key_Home);for(int i=0;i<selected;++i)QTest::keyClick(combo->view(),Qt::Key_Down);QTest::keyClick(combo->view(),Qt::Key_Return);events();
         auto *draft=child->findChild<QPlainTextEdit *>("commandSource");require(draft&&draft->toPlainText().startsWith(type+" "+solver+"("),"Solver leaf template binds wrong enclosing solver");
         QString seed=statement;
         if(vary){seed=varyStatement("Probe","U","1");}
         else if(family=="Achieve")seed.replace("Quantity = 9","Cost = 1");
         else if(family=="Minimize")seed.replace("(Cost)","(Quantity)");
         else seed.replace("U >= Bound","U >= 0");
         draft->setPlainText(seed);events();
         auto field=[&](const QString &name,const QString &value){auto *w=child->findChild<QLineEdit *>("commandField_"+name);reveal(w);acceptanceView(*child);QTest::mouseClick(w,Qt::LeftButton);w->selectAll();QTest::keyClicks(w,value);events();require(w->text()==value,"Solver leaf typed field differs");typedFields.append(QJsonObject{{"field",name},{"value",value}});};
         if(vary){field("Variable","V");field("Initial value","0");}
         else if(family=="Achieve"){field("Goal","Quantity");field("Value","9");}
         else if(family=="Minimize")field("Objective","Cost");
         else field("Right side","Bound");
         require(draft->toPlainText()==statement&&dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Solver leaf typed draft differs from literal source oracle");acceptanceView(*child);
      };
      auto click=[&](QMdiSubWindow *child,QDialogButtonBox::StandardButton which){auto *box=child->findChild<QDialogButtonBox *>();auto *button=box?box->button(which):nullptr;reveal(button);acceptanceView(*child);QTest::mouseClick(button,Qt::LeftButton);events();};
      const int undo=source->document()->availableUndoSteps();QPointer<QMdiSubWindow> child=open();configure(child);
      require(source->toPlainText()==initial&&source->document()->availableUndoSteps()==undo&&configured()==base&&read(saved)==initial.toUtf8(),"Solver leaf pending draft escaped isolation");
      click(child,QDialogButtonBox::Cancel);require(!child&&source->toPlainText()==initial&&configured()==base,"Solver leaf Cancel escaped isolation");child=open();configure(child);
      const QString anchor=position=="BlockBefore"?outer:position=="LeafBefore"||position=="LeafAfter"?leaf:endFor;
      const bool after=position=="BlockAfter"||position=="EndAfter"||position=="LeafAfter";
      QString wanted=initial;require(wanted.count(anchor)==1,"Solver leaf literal insertion anchor ambiguous");wanted.insert(wanted.indexOf(anchor)+(after?anchor.size():0),statement+"\n");
      click(child,QDialogButtonBox::Ok);
      if(child||source->toPlainText()!=wanted||source->document()->availableUndoSteps()<=undo){
         save("insertion-actual.script",source->toPlainText().toUtf8());save("insertion-expected.script",wanted.toUtf8());save("insertion-messages.txt",messages->toPlainText().toUtf8());
         auto *label=child?child->findChild<QLabel *>("commandStatus"):nullptr;
         const QString diagnostic=label?label->text():QString();
         save("insertion-failure.json",QJsonDocument(QJsonObject{{"owner_retained",bool(child)},{"status",diagnostic},{"source_equal",source->toPlainText()==wanted},{"undo_before",undo},{"undo_after",source->document()->availableUndoSteps()}}).toJson());
         std::cerr<<"LEAF_INSERT case="<<id.toStdString()<<" owner="<<bool(child)<<" status="<<diagnostic.toStdString()<<std::endl;
      }
      require(!child&&source->toPlainText()==wanted&&source->document()->availableUndoSteps()>undo,"Solver leaf positive insertion differs from literal source/history");
      source->undo();require(source->toPlainText()==initial&&window.buildScript()&&configured()==base,"Solver leaf Undo failed");source->redo();require(source->toPlainText()==wanted&&window.buildScript()&&configured()==base,"Solver leaf Redo failed");
      require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==wanted&&read(saved)==wanted.toUtf8()&&configured()==base,"Solver leaf Unicode persistence failed");save("mission.script",wanted.toUtf8());save("typed-fields.json",QJsonDocument(typedFields).toJson());
      const auto model=window.missionSnapshot();const int probe=named("Probe");const QString parent=position=="LeafBefore"||position=="LeafAfter"?"Inner":position=="BranchAppend"||position=="EndBefore"?"Outer":"Solver";
      require(model.nodes[probe].type==type&&model.nodes[probe].parent==named(parent)&&item(probe)->parent()==item(named(parent)),"Solver leaf native/visible parent differs");
      if(position=="BlockBefore")require(probe+1==named("Outer"),"Solver leaf before-block order differs");
      if(position=="BlockAfter"||position=="EndAfter")require(probe==missionBranchEnd(model,named("Outer"))+1,"Solver leaf after-block order differs");
      if(position=="BranchAppend"||position=="EndBefore")require(probe+1==missionBranchEnd(model,named("Outer")),"Solver leaf before-end order differs");
      if(position=="LeafBefore"||position=="LeafAfter")require(probe==named("Leaf")+(after?1:-1),"Solver leaf sibling order differs");
      QJsonArray hierarchy;for(const auto &n:model.nodes)hierarchy.append(QJsonObject{{"type",n.type},{"label",missionCommandLabel(n.statement).name},{"parent",n.parent},{"statement",n.statement}});save("hierarchy.json",QJsonDocument(hierarchy).toJson());
      Debugger *debug=nullptr;for(auto *object:window.children())if(auto *d=dynamic_cast<Debugger *>(object))debug=d;require(debug&&window.setBreakpoint(probe,true),"Solver leaf breakpoint unavailable");
      const auto previous=debug->paused;QJsonArray samples;std::exception_ptr failure;
      debug->paused=[&](const QString &command){if(previous)previous(command);try{auto *dialog=window.findChild<QDialog *>("debuggerDialog");require(dialog&&dialog->isVisible()&&dialog->property("commandIndex").toInt()==probe&&command==type,"Solver leaf actual execution point differs");acceptanceView(*dialog);QJsonArray actual;for(const auto &name:names)actual.append(Moderator::Instance()->GetInternalObject(name.toStdString())->GetRealParameter("Value"));const int index=actual[5].toInt();const bool inside=parent!="Solver";const int expectedIndex=inside?1+samples.size()%2:position=="BlockBefore"?0:2;require(index==expectedIndex,"Solver leaf runtime order/loop index differs");samples.append(QJsonObject{{"index",index},{"state",actual}});save("samples.json",QJsonDocument(samples).toJson());}catch(...){failure=std::current_exception();QTimer::singleShot(0,&window,[&]{window.stopMission();});}QTimer::singleShot(0,&window,[&]{debug->resume();});};
      QTimer deadline;deadline.setSingleShot(true);QObject::connect(&deadline,&QTimer::timeout,&window,[&]{failure=std::make_exception_ptr(std::runtime_error("Solver leaf native watchdog expired"));window.stopMission();});
      const int beforeRun=source->document()->availableUndoSteps();messages->clear();deadline.start(180000);acceptanceView(window);const auto status=window.runMission();deadline.stop();debug->paused=previous;events();save("messages.txt",messages->toPlainText().toUtf8());if(failure)std::rethrow_exception(failure);
      require(status==MainWindow::RunResult::Completed&&!samples.isEmpty()&&(parent=="Solver"||samples.size()%2==0),"Solver leaf mission failed or execution count differs");
      const double u=target?4:family=="VaryOptimize"?2:family=="Minimize"?(position=="BlockBefore"?3:4):(position=="BlockBefore"?3:5),v=two?-2:0,cost=target||family=="VaryOptimize"?0:family=="Minimize"?(u-4)*(u-4):(u-2)*(u-2);
      const QJsonArray expected{u,v,2*u+1,cost,41,2,5};const auto raw=read(report);save("values.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==1,"Solver leaf witness row count differs");const auto cells=lines[0].split(',');require(cells.size()==7,"Solver leaf witness width differs");QJsonArray actual,native;
      for(int i=0;i<7;++i){bool ok=false;const double value=cells[i].toDouble(&ok),n=Moderator::Instance()->GetInternalObject(names[i].toStdString())->GetRealParameter("Value");if(!ok||!std::isfinite(value)||std::abs(value-expected[i].toDouble())>(i==3?1e-5:2e-6))std::cerr<<"LEAF_DIFF case="<<id.toStdString()<<" column="<<i<<" actual="<<value<<" expected="<<expected[i].toDouble()<<std::endl;require(ok&&std::isfinite(value)&&std::abs(value-expected[i].toDouble())<(i==3?1e-5:2e-6)&&std::abs(n-value)<1e-12,"Solver leaf result differs from independent position-dependent solution");actual.append(value);native.append(n);}
      QSet<GmatCommand *> seen;QJsonArray nativeTree;auto *branch=dynamic_cast<SolverBranchCommand *>(walk(Moderator::Instance()->GetFirstCommand(),"Solver","",seen,nativeTree));require(branch,"Solver leaf actual branch absent");
      int matches=0;for(const auto &n:nativeTree)if(n.toObject()["label"]=="Probe"){require(n.toObject()["type"]==type&&n.toObject()["parent"]==parent,"Solver leaf actual engine hierarchy differs");++matches;}require(matches==1,"Solver leaf actual command identity count differs");save("native-tree.json",QJsonDocument(nativeTree).toJson());
      auto *clone=dynamic_cast<Solver *>(branch->GetClone(0));require(clone&&clone->GetName()==solver.toStdString()&&clone->GetTypeName()==(target?"DifferentialCorrector":"Yukon")&&clone->GetIntegerParameter(clone->GetParameterID("IntegerSolverStatus"))==Gmat::CONVERGED,"Solver leaf actual solver did not converge");
      QJsonArray registered,values;for(const auto &name:clone->GetStringArrayParameter(clone->GetParameterID("Variables")))registered.append(QString::fromStdString(name));const int count=clone->GetIntegerParameter(clone->GetParameterID("NumberOfVariables"));require(count==(two?2:1)&&registered==(two?QJsonArray{"U","V"}:QJsonArray{"U"}),"Solver leaf actual control registration order differs");for(int i=0;i<count;++i){const double value=clone->GetSolverVariable(i);require(std::isfinite(value)&&std::abs(value-(i==0?u:v))<2e-6,"Solver leaf actual control value differs");values.append(value);}
      require(window.setBreakpoint(probe,false)&&source->toPlainText()==wanted&&source->document()->availableUndoSteps()==beforeRun&&read(saved)==wanted.toUtf8()&&configured()==base&&!window.isRunning(),"Solver leaf Run mutated source/history/disk/configured state");
      const QJsonObject result{{"case",id},{"family",family},{"position",position},{"actual",actual},{"native",native},{"expected",expected},{"samples",samples},{"parent",parent},{"solver",QJsonObject{{"name",solver},{"status",1},{"variables",registered},{"values",values},{"count",count}}}};save("native-result.json",QJsonDocument(result).toJson());
      const QJsonObject proof{{"checkpoint","solver-leaf-"+id},{"outcome","passed"},{"case",id},{"family",family},{"position",position},{"historical_key",spec["historical_key"]},{"insertions",1},{"cancelled_drafts",1},{"typed_fields",typedFields.size()},{"runtime_samples",samples.size()},{"actual",actual},{"facets",QJsonArray{"actual-context-popup-template-and-positive-typed-solver-leaf","cancel-source-history-undo-redo-unicode-isolation","literal-visible-model-and-native-engine-parent-order","actual-debugger-leaf-execution-and-two-pass-loop-order","independent-position-dependent-linear-quadratic-result","actual-converged-solver-control-registration-and-state"}},{"scope","Selected scalar solver leaves at literal two-pass For/true If control-flow positions inside Target/Optimize;actual typed commit,source/history/native hierarchy/execution and independent final results. Not private historical keyword reconstruction,all multiplicities/conditions/solvers/numerical regimes/native OS/GPU/hardware/platform/stress/current NASA/full newest-build qualification."}};
      save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;source->document()->setModified(false);return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
    catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
   return 1;
}
