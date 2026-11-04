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
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaSolverNestingRejection");
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
      require(QStringList{"BranchAppend","EndBefore","LeafBefore","LeafAfter"}.contains(position),"This is not an unsupported nesting position");
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
         window.raise();window.activateWindow();
         require(QTest::qWaitForWindowActive(&window,5000),"Rejection parent window did not activate");
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

      // This is a new negative contract, not a changed interpretation of the
      // archived positive test. Require the specific native interpreter error.
      const QString nesting=position.startsWith("Leaf")?"If":"For";
      const QString diagnostic="Solver command, and cannot be nested inside a \""+nesting+"\" Control Logic command";
      auto nativeTree=[&] {
         QSet<GmatCommand *> seen;QJsonArray result;
         walk(Moderator::Instance()->GetFirstCommand(),"__absent__","",seen,result);
         return result;
      };
      auto resources=[&] {
         QJsonObject result;
         for(const auto &name:QStringList{"Sat","ADC","ZDC","AOpt","ZOpt","Values"}) {
            auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());
            require(object,"Rejection resource disappeared");
            result[name]=QString::fromStdString(object->GetGeneratingString(Gmat::NO_COMMENTS));
         }
         return result;
      };
      const auto baselineTree=nativeTree();const auto baselineResources=resources();
      const auto canonical=window.missionSnapshot().canonicalScript;
      const int undo=source->document()->availableUndoSteps(),redo=source->document()->availableRedoSteps();
      const bool modified=source->document()->isModified();
      int isolationCheck=0;
      auto unchanged=[&] {
         const QJsonObject checks{{"source",source->toPlainText()==initial},{"undo",source->document()->availableUndoSteps()==undo},
            {"redo",source->document()->availableRedoSteps()==redo},{"modified",source->document()->isModified()==modified},
            {"configured",configured()==base},{"disk",read(saved)==initial.toUtf8()},{"native_tree",nativeTree()==baselineTree},
            {"resources",resources()==baselineResources},{"model_source",window.missionSnapshot().sourceScript==initial},
            {"canonical",window.missionSnapshot().canonicalScript==canonical},{"idle",!window.isRunning()}};
         ++isolationCheck;
         for(auto at=checks.begin();at!=checks.end();++at)if(!at.value().toBool()) {
            std::cerr<<"ISOLATION check="<<isolationCheck<<" field="<<at.key().toStdString()<<std::endl;
            save("isolation-check.json",QJsonDocument(checks).toJson());
            save("before-resources.json",QJsonDocument(baselineResources).toJson());
            save("after-resources.json",QJsonDocument(resources()).toJson());
            save("before-native-tree.json",QJsonDocument(baselineTree).toJson());
            save("after-native-tree.json",QJsonDocument(nativeTree()).toJson());
            save("model-source.script",window.missionSnapshot().sourceScript.toUtf8());
            save("before-canonical.script",canonical.toUtf8());
            save("after-canonical.script",window.missionSnapshot().canonicalScript.toUtf8());
         }
         require(source->toPlainText()==initial&&source->document()->availableUndoSteps()==undo&&
                 source->document()->availableRedoSteps()==redo&&source->document()->isModified()==modified&&
                 configured()==base&&read(saved)==initial.toUtf8()&&nativeTree()==baselineTree&&
                 resources()==baselineResources&&window.missionSnapshot().sourceScript==initial&&
                 window.missionSnapshot().canonicalScript==canonical&&!window.isRunning(),
                 "Rejected nesting changed source/history/file/configuration/native hierarchy or model");
      };
      QPointer<QMdiSubWindow> child=open();configure(child);unchanged();
      click(child,QDialogButtonBox::Cancel);require(!child,"Pending rejection draft did not cancel");unchanged();
      child=open();configure(child);unchanged();QJsonArray rejected;
      auto *draft=child->findChild<QPlainTextEdit *>("commandSource");
      QPointer<CommandEditor> owner=dynamic_cast<CommandEditor *>(child->widget());
      require(draft&&owner,"Rejection draft/owner absent");
      for(auto action:{QDialogButtonBox::Apply,QDialogButtonBox::Ok}) {
         messages->clear();click(child,action);
         require(child&&owner&&child->widget()==owner&&owner->hasChanges()&&draft->toPlainText()==statement,
                 "Rejected nesting lost its exact pending draft or original editor owner");
         auto *status=owner->findChild<QLabel *>("commandStatus");
         const auto nativeMessages=messages->toPlainText();
         save(action==QDialogButtonBox::Apply?"apply.messages.txt":"ok.messages.txt",nativeMessages.toUtf8());
         require(status&&!status->text().isEmpty()&&nativeMessages.contains(diagnostic)&&
                 nativeMessages.contains(type),"Nesting did not fail for the specific native solver/control-logic restriction");
         unchanged();acceptanceView(*child);
         rejected.append(QJsonObject{{"action",action==QDialogButtonBox::Apply?"Apply":"OK"},
            {"panel_diagnostic",status->text()},{"native_restriction",diagnostic},{"draft_retained",true},
            {"source_history_file_configuration_native_hierarchy_unchanged",true}});
      }
      click(child,QDialogButtonBox::Cancel);require(!child,"Rejected nesting editor did not cancel");unchanged();
      require(window.buildScript()&&configured()==base&&nativeTree()==baselineTree,
              "Accepted baseline could not be rebuilt after rejected edits");
      save("initial.script",initial.toUtf8());save("rejected-draft.script",statement.toUtf8());
      save("native-hierarchy.json",QJsonDocument(baselineTree).toJson());
      save("configured-resources.json",QJsonDocument(baselineResources).toJson());
      const QJsonObject proof{{"checkpoint","solver-nesting-rejection-"+id},{"outcome","passed"},
         {"case",id},{"positive_hypothesis",QString("QtGui.NativeNasaSolverLeaf")+id},
         {"positive_hypothesis_executed",false},{"historical_key",spec["historical_key"]},
         {"family",family},{"position",position},{"control_logic",nesting},{"native_rejections",rejected},
         {"pending_and_rejected_cancel_paths",2},{"typed_fields",typedFields},
         {"facets",QJsonArray{"actual-context-popup-and-typed-command-editor","specific-native-interpreter-nesting-rejection-on-Apply-and-OK",
            "exact-pending-draft-owner-retention","unchanged-source-undo-redo-disk-configured-resources-and-native-hierarchy",
            "cancel-isolation-and-accepted-baseline-rebuild"}},
         {"scope","Current native interpreter conformance for twelve unsupported positive nesting hypotheses. No positive nesting support, historical private keyword reconstruction, solver convergence, or full NASA qualification credit."}};
      save("proof.json",QJsonDocument(proof).toJson());
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      source->document()->setModified(false);return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
   catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
