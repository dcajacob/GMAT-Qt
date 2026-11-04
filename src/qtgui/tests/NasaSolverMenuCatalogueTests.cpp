#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include "GmatGlobal.hpp"
#include "QtPlotReceiver.hpp"
#include "UpdateDynamicData.hpp"
#include "StringUtil.hpp"
#include <QTableWidget>
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
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidgetItemIterator>
using namespace formaccept;
static void reveal(QWidget *w){require(w,"Solver-context control absent");for(QWidget *p=w;p;p=p->parentWidget())if(auto *s=qobject_cast<QScrollArea *>(p))s->ensureWidgetVisible(w);events();require(w->isVisible()&&w->isEnabled(),"Solver-context control unreachable");}
int main(int argc,char **argv)
{
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaSolverMenuCatalogue");
 try {
  require(argc==3&&acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen","Supply startup and solver-context key on native widgets");const QString mode=argv[2];require(QStringList{"Target","OptimizeAddObjective","OptimizeCapacity","Commands"}.contains(mode),"Solver-context key absent");const bool target=mode=="Target",root=mode=="Commands",objective=mode=="OptimizeCapacity";
  TestSettings environment;QTemporaryDir files;require(files.isValid(),"Solver-context fixture absent");const auto startup=QFileInfo(argv[1]).absoluteFilePath(),report=files.filePath("Solver-context Δ.csv"),saved=files.filePath("Solver context Ω.script");QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;window.resize(1200,850);window.show();require(window.initialize(startup),"Solver-context runtime absent");events();acceptanceView(window);
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&area,"Solver-context workspace absent");const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes){if(!evidence.isEmpty())write(evidence+"/"+name,bytes);};
  QString initial="% Solver context Ω\nCreate Variable U V Quantity Cost Keep I;\nU = 1;\nV = 0;\nQuantity = 0;\nCost = 0;\nKeep = 41;\nI = 0;\nCreate Spacecraft Sat;\nCreate DifferentialCorrector ADC ZDC;\nADC.ShowProgress = false;\nZDC.ShowProgress = false;\nZDC.MaximumIterations = 20;\nZDC.DerivativeMethod = CentralDifference;\nCreate Yukon AOpt ZOpt;\nZOpt.ShowProgress = false;\nZOpt.FeasibilityTolerance = 0.00000001;\nZOpt.OptimalityTolerance = 0.00000001;\nZOpt.FunctionTolerance = 0.00000001;\n";
  for(const auto &name:QStringList{"ADC","ZDC","AOpt","ZOpt"})initial+=name+".ReportFile = '"+files.filePath(name+".txt")+"';\n";
  if(root)initial+="Sat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = -120;\nSat.Z = 350;\nSat.VX = -1;\nSat.VY = 7;\nSat.VZ = 2;\nCreate Array State[6,1];\nCreate DynamicDataDisplay Display;\nDisplay.AddParameters = {1, U, Keep, State(1,1)};\n";
  initial+="Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\n";
  const auto begin=target?QString("Target 'Solver' ZDC {SolveMode = Solve, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\n"):QString("Optimize 'Solver' ZOpt {SolveMode = Solve, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\n");
  const auto varying=target?QString("Vary 'Original control' ZDC(U = 1, {Perturbation = 0.01, Lower = -10, Upper = 10, MaxStep = 10});\n"):QString("Vary 'Original control' ZOpt(U = 1, {Perturbation = 0.000001, Lower = 0, Upper = 10, MaxStep = 10});\n");
  const auto tail=target?QString("Achieve 'Original goal' ZDC(Quantity = 9, {Tolerance = 0.000000001});\n"):QString("NonlinearConstraint 'Original constraint' ZOpt(U >= 3);\n")+(objective?QString("Minimize 'Original objective' ZOpt(Cost);\n"):QString());const auto closing=target?QString("EndTarget;\n"):QString("EndOptimize;\n");
  const QString loop="For 'Loop' I = 1:1:1;\n",leaf="'Leaf' Keep = Keep + 0;\n";
  if(root)initial+=leaf;else initial+=begin+varying+(target?"Quantity = 2 * U + 1;\n":"Cost = (U - 2)^2;\n")+loop+"If 'Inner' U > -10;\n"+leaf+"EndIf;\nEndFor;\n"+tail+closing;
  initial+="Report 'Witness' Values U V Quantity Cost Keep I;\n% Preserve tail Δ\n";
  if(root)initial.replace("Values U V Quantity Cost Keep I;","Values U V Quantity Cost Keep I State(1,1) State(2,1) State(3,1) State(4,1) State(5,1) State(6,1);");
  write(saved,initial.toUtf8());save("initial.script",initial.toUtf8());require(window.loadScript(saved)&&window.buildScript(),"Solver-context initial build failed");source->document()->setModified(false);QString working=initial;QJsonArray queries,rejections,edits;
  auto configured=[&]{QJsonArray a;for(const char *name:{"U","V","Quantity","Cost","Keep","I"})a.append(Moderator::Instance()->GetConfiguredObject(name)->GetRealParameter("Value"));return a;};const auto base=configured();require(base==QJsonArray{1,0,0,0,41,0},"Solver-context literal baseline differs");
  auto named=[&](const QString &label){const auto snap=window.missionSnapshot();int found=-1;for(int i=0;i<snap.nodes.size();++i)if(missionCommandLabel(snap.nodes[i].statement).name==label){require(found<0,"Solver-context label ambiguous");found=i;}require(found>=0,"Solver-context named command absent");return found;};
  auto node=[&](int at){for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==at)return *it;throw std::runtime_error("Solver-context tree node absent");};
  auto open=[&](int at,const QString &action){for(auto *tabs:window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree))tabs->setCurrentIndex(i);auto *item=at<0?tree->topLevelItem(0):node(at);tree->expandAll();tree->setCurrentItem(item);tree->scrollToItem(item);events();reveal(tree);acceptanceView(window);std::exception_ptr failure;bool visited=false;
   QTimer::singleShot(0,&window,[&]{auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());try{require(menu,"Solver-context actual menu absent");QAction *choice=nullptr;for(auto *a:menu->actions())if(a->text()==action||a->objectName()==action)choice=a;require(choice&&choice->isEnabled(),"Solver-context insertion action absent");menu->setActiveAction(choice);acceptanceView(*menu);visited=true;QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(choice).center());}catch(...){failure=std::current_exception();if(menu)menu->close();}});
   const auto point=tree->visualItemRect(item).center();QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));QApplication::sendEvent(tree->viewport(),&event);events();if(failure)std::rethrow_exception(failure);require(visited,"Solver-context menu not visited");auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Solver-context actual editor absent");child->showNormal();child->move(0,0);child->resize(area->viewport()->size());events();acceptanceView(*child);return child;
  };
  auto click=[&](QMdiSubWindow *child,QDialogButtonBox::StandardButton action){auto *box=child->findChild<QDialogButtonBox *>();require(box&&box->button(action),"Solver-context editor action absent");reveal(box->button(action));acceptanceView(*child);QTest::mouseClick(box->button(action),Qt::LeftButton);events();};
  auto choose=[&](QMdiSubWindow *child,const QString &type){auto *combo=child->findChild<QComboBox *>("commandTemplate");require(combo,"Solver-context actual chooser absent");const int at=combo->findText(type);require(at>=0,"Solver-context legal template missing");reveal(combo);QTest::mouseClick(combo,Qt::LeftButton,Qt::NoModifier,QPoint(combo->width()-10,combo->height()/2));events();require(combo->view()->isVisible(),"Solver-context chooser popup absent");acceptanceView(*combo->view()->window());const auto entry=combo->model()->index(at,0);combo->view()->scrollTo(entry,QAbstractItemView::PositionAtCenter);events();const auto rect=combo->view()->visualRect(entry).intersected(combo->view()->viewport()->rect());require(!rect.isEmpty(),"Solver catalogue popup item is outside its viewport");QTest::keyClick(combo->view(),Qt::Key_Home);for(int i=0;i<at;++i)QTest::keyClick(combo->view(),Qt::Key_Down);QTest::keyClick(combo->view(),Qt::Key_Return);events();save("last-template-choice.json",QJsonDocument(QJsonObject{{"expected",type},{"actual",combo->currentText()},{"requested_index",at}}).toJson());require(combo->currentText()==type,"Solver-context selected template differs");};

  if(root) {
   QJsonArray parsed;
   for(const auto &text:QStringList{"UpdateDynamicData Display U Keep State(1,1);","UpdateDynamicData Display U, Keep, State(1, 1);","UpdateDynamicData\tDisplay\tU\tKeep\tState(1,\t1);"}) {
    UpdateDynamicData command;command.SetGeneratingString(text.toStdString());require(command.InterpretAction(),"Dynamic update array parser rejected supported syntax");const auto canonical=QString::fromStdString(command.GetGeneratingString(Gmat::NO_COMMENTS));require(canonical=="UpdateDynamicData Display U Keep State(1,1);","Dynamic update array parser changed names/index syntax");parsed.append(QJsonObject{{"input",text},{"canonical",canonical}});
   }
   for(const auto &bad:QStringList{"UpdateDynamicData Display State(1,1;","UpdateDynamicData Display State1,1);"}) {
    UpdateDynamicData command;command.SetGeneratingString(bad.toStdString());bool rejected=false;try{command.InterpretAction();}catch(BaseException&){rejected=true;}require(rejected,"Dynamic update accepted unbalanced array indices");parsed.append(QJsonObject{{"input",bad},{"rejected",true}});
   }
   save("parser-cases.json",QJsonDocument(parsed).toJson());
   const QString witness="Report 'Witness' Values U V Quantity Cost Keep I State(1,1) State(2,1) State(3,1) State(4,1) State(5,1) State(6,1);\n";
   auto insert=[&](const QString &type,const QString &field,const QString &value,const QString &expected){
    const auto before=working;const int undo=source->document()->availableUndoSteps();QPointer<QMdiSubWindow> child;
    for(bool accept:{false,true}){
     child=open(named("Witness"),"Insert before…");choose(child,type);auto *input=child->findChild<QLineEdit *>("commandField_"+field);QJsonArray names;for(auto *control:child->findChildren<QLineEdit *>())names.append(control->objectName());save("pending-command.json",QJsonDocument(QJsonObject{{"template",type},{"wanted_field",field},{"fields",names},{"source",child->findChild<QPlainTextEdit *>("commandSource")->toPlainText()}}).toJson());require(input,"New catalogue typed field absent;see pending-command.json");reveal(input);input->setText(value);events();auto *draft=child->findChild<QPlainTextEdit *>("commandSource");require(draft&&draft->toPlainText()==expected,"New catalogue template form generated unexpected source");
     require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&configured()==base,"New catalogue pending form changed mission/configuration");click(child,accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel);
     require(!child,"New catalogue command editor did not close");if(!accept)require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&configured()==base,"New catalogue Cancel changed mission/history/model");
    }
    require(working.count(witness)==1,"New catalogue witness anchor ambiguous");working.replace(witness,expected+"\n"+witness);require(source->toPlainText()==working,"New catalogue insertion changed unrelated source");source->undo();require(source->toPlainText()==before&&window.buildScript()&&configured()==base,"New catalogue command Undo failed");source->redo();require(source->toPlainText()==working&&window.buildScript()&&configured()==base,"New catalogue command Redo failed");
   };
   insert("Call built-in function","Outputs","[State]","[State] = GetLastState(Sat);");
   insert("UpdateDynamicData","Parameters","U Keep State(1,1)","UpdateDynamicData Display U Keep State(1,1);");
   QJsonArray runs;const QJsonArray truth{1,0,0,0,41,0,7000,-120,350,-1,7,2};
   for(const auto &phase:QStringList{"gui","reopened"}){
    require(window.saveScriptTo(saved)&&read(saved)==working.toUtf8(),"New catalogue Unicode Save changed source");
    if(phase=="reopened")require(window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==working&&configured()==base,"New catalogue Unicode reopen changed source/model");
    const int undo=source->document()->availableUndoSteps();messages->clear();acceptanceView(window);save("mission.script",working.toUtf8());const auto status=window.runMission();events();save(phase+".messages.txt",messages->toPlainText().toUtf8());require(status==MainWindow::RunResult::Completed,"New catalogue command witness mission failed");const auto raw=read(report);save(phase+".csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==1,"New catalogue witness row count differs");const auto cells=lines[0].split(',');require(cells.size()==12,"New catalogue witness column count differs");QJsonArray values;
    for(int i=0;i<12;++i){bool ok=false;const double v=cells[i].toDouble(&ok);require(ok&&std::isfinite(v)&&std::abs(v-truth[i].toDouble())<1e-12,"New catalogue built-in state report differs from literal Cartesian/scalar oracle");values.append(v);}
    auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));auto *state=Moderator::Instance()->GetInternalObject("State");require(sat&&state,"New catalogue native witness objects absent");const auto &array=state->GetRmatrixParameter("RmatValue");require(array.GetNumRows()==6&&array.GetNumColumns()==1,"New catalogue built-in output shape differs");QJsonArray native;
    for(int i=0;i<6;++i){require(std::abs(array(i,0)-truth[i+6].toDouble())<1e-12&&std::abs(sat->GetState()[i]-array(i,0))<1e-12,"New catalogue GetLastState output differs from native spacecraft");native.append(array(i,0));}
    QStringList executed;int guard=0;for(auto *command=Moderator::Instance()->GetFirstCommand();command&&++guard<100;command=command->GmatCommand::GetNext())executed<<QString::fromStdString(command->GetTypeName());require(guard<100&&executed.count("CallBuiltinGmatFunction")==1&&executed.count("UpdateDynamicData")==1,"New catalogue inserted command did not use actual native built-in/update types");
    require(window.plotReceiver()->show("Display"),"New catalogue live display absent");auto *active=area->activeSubWindow();auto *table=active?qobject_cast<QTableWidget *>(active->widget()):nullptr;require(table&&table->rowCount()==1&&table->columnCount()==3,"New catalogue live table shape differs");QJsonArray shown;for(int i=0;i<3;++i)shown.append(table->item(0,i)?table->item(0,i)->text():QString());QJsonArray expectedCells;for(const auto &pair:QList<QPair<QString,double>>{{"U",1},{"Keep",41},{"State(1,1)",7000}})expectedCells.append(pair.first+" = "+QString::fromStdString(GmatStringUtil::ToString(pair.second)));save(phase+".display-cells.json",QJsonDocument(QJsonObject{{"actual",shown},{"expected",expectedCells},{"format_policy","NASA DynamicDataDisplay native GmatStringUtil::ToString"}}).toJson());require(shown==expectedCells,"New catalogue actual visible cells differ from expected report/native state and NASA numeric formatting");acceptanceView(*active);
    require(source->toPlainText()==working&&source->document()->availableUndoSteps()==undo&&read(saved)==working.toUtf8()&&configured()==base&&!window.isRunning(),"New catalogue Run mutated source/history/file/configured model");
    runs.append(QJsonObject{{"phase",phase},{"actual",values},{"expected",truth},{"native_array",native},{"live_cells",shown},{"native_command_types",QJsonArray::fromStringList(executed)}});save("command-results.json",QJsonDocument(runs).toJson());
   }
   save("mission.script",working.toUtf8());const QJsonObject proof{{"checkpoint","solver-menu-new-commands"},{"outcome","passed"},{"case",mode},{"missions",2},{"facets",QJsonArray{"actual-built-in-and-dynamic-update-template-form-insertion","cancel-source-exact-history-unicode-save-reopen","literal-cartesian-result-native-array-spacecraft-and-command-identity","actual-dynamic-display-cells-and-unchanged-configured-model"}},{"scope","Two previously missing menu templates:one GetLastState six-component built-in and one explicit UpdateDynamicData display refresh. No propagation/coordinate transformation or general built-in/input/scientific/native OS input/platform/stress/current NASA qualification."}};save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;source->document()->setModified(false);return 0;
  }
  // NASA wx MissionTree builds the ordinary menu from the engine's viewable
  // command registry, adds control logic, then adds context/capacity leaves.
  // Derive the expected catalogue there, independently of Qt's template map.
  QStringList nativeTypes,hidden,expectedBase;
  const QMap<QString,QString> aliases{{"Equation","Assignment"},{"ScriptEvent","Script event"},{"CallGmatFunction","Call function"},{"CallMatlabFunction","Call MATLAB function"},{"CallBuiltinGmatFunction","Call built-in function"},{"CallPythonFunction","Call Python function"},{"Set","Set (file import)"}};
  for(const auto &name:Moderator::Instance()->GetListOfViewableItems(Gmat::COMMAND)) {
   const auto type=QString::fromStdString(name);nativeTypes<<type;
   if(GmatGlobal::Instance()->IsHiddenCommand(name)){hidden<<type;continue;}
   expectedBase<<aliases.value(type,type);
  }
  expectedBase<<"If"<<"For"<<"While";
  expectedBase.removeDuplicates();expectedBase.sort();nativeTypes.sort();hidden.sort();
  require(expectedBase.contains("Target")&&expectedBase.contains("Optimize")&&expectedBase.contains("Propagate")&&expectedBase.contains("Assignment"),"NASA native general-command registry incomplete");
  // Qt's If template contains Else and its source permits no-Else editing;
  // this catalogue checks command identities, not distinct If/If-Else widgets.
  QJsonArray catalogue,missingAll,unexpectedAll;int visitedTemplates=0;
  auto inspect=[&](const QString &tag,int at,const QString &action,const QStringList &leaves,bool exercise){
   const auto before=source->toPlainText();const auto disk=read(saved);const int undo=source->document()->availableUndoSteps();
   QPointer<QMdiSubWindow> child=open(at,action);auto *combo=child->findChild<QComboBox *>("commandTemplate");auto *draft=child->findChild<QPlainTextEdit *>("commandSource");require(combo&&draft,"Solver catalogue chooser/source absent");
   require(combo->count()>1&&combo->itemText(0)=="Choose a command template…","Solver catalogue placeholder differs");
   QStringList actual;for(int i=1;i<combo->count();++i){require(!combo->itemText(i).isEmpty()&&!actual.contains(combo->itemText(i)),"Solver catalogue empty/duplicate command choice");actual<<combo->itemText(i);}actual.sort();
   auto expected=expectedBase;expected.append(leaves);expected.sort();QStringList missing,unexpected;
   for(const auto &name:expected)if(!actual.contains(name))missing<<name;
   for(const auto &name:actual)if(!expected.contains(name))unexpected<<name;
   QJsonObject defaults;
   if(exercise)for(const auto &name:actual){choose(child,name);const auto text=draft->toPlainText();require(!text.trimmed().isEmpty(),"Solver catalogue selected empty template");defaults[name]=text;++visitedTemplates;
    if(leaves.contains(name))require(text.startsWith(name+" "+(target?"ZDC":"ZOpt")+"("),"Solver catalogue leaf default bound to another configured solver");
    require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&read(saved)==disk&&configured()==base,"Solver catalogue preview changed source/history/file/configuration");
   }
   click(child,QDialogButtonBox::Cancel);require(!child&&source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&read(saved)==disk&&configured()==base,"Solver catalogue Cancel changed source/history/file/configuration");
   catalogue.append(QJsonObject{{"tag",tag},{"expected",QJsonArray::fromStringList(expected)},{"actual",QJsonArray::fromStringList(actual)},{"missing",QJsonArray::fromStringList(missing)},{"unexpected",QJsonArray::fromStringList(unexpected)},{"defaults",defaults},{"cancel_isolated",true}});
   for(const auto &name:missing)missingAll.append(tag+":"+name);for(const auto &name:unexpected)unexpectedAll.append(tag+":"+name);
   save("catalogue.json",QJsonDocument(catalogue).toJson());
  };
  const QStringList leaves=target?QStringList{"Vary","Achieve"}:objective?QStringList{"Vary","NonlinearConstraint"}:QStringList{"Vary","Minimize","NonlinearConstraint"};
  const int end=missionBranchEnd(window.missionSnapshot(),named("Solver"));
  inspect("root",-1,"Append command…",{},false);
  inspect("branch-before",named("Solver"),"Insert before…",{},false);
  inspect("branch-after",named("Solver"),"Insert after…",{},false);
  inspect("branch-append",named("Solver"),"appendInsideMissionBranch",leaves,true);
  inspect("end-before",end,"Insert before…",leaves,false);
  inspect("end-after",end,"Insert after…",{},false);
  inspect("nested-leaf-before",named("Leaf"),"Insert before…",leaves,false);
  inspect("nested-leaf-after",named("Leaf"),"Insert after…",leaves,false);
  inspect("loop-before",named("Loop"),"Insert before…",leaves,false);
  inspect("loop-after",named("Loop"),"Insert after…",leaves,false);
  const QJsonObject result{{"case",mode},{"native_viewable",QJsonArray::fromStringList(nativeTypes)},{"hidden",QJsonArray::fromStringList(hidden)},{"expected_base",QJsonArray::fromStringList(expectedBase)},{"missing",missingAll},{"unexpected",unexpectedAll},{"query_count",catalogue.size()},{"visited_templates",visitedTemplates},{"configured",configured()},{"no_mission_replay",true}};
  save("catalogue-result.json",QJsonDocument(result).toJson());
  require(missingAll.isEmpty()&&unexpectedAll.isEmpty(),"Solver catalogue differs from NASA native menu registry; see catalogue-result.json missing/unexpected lists");
  const QJsonObject proof{{"checkpoint","solver-menu-catalogue-"+mode},{"outcome","passed"},{"case",mode},{"query_count",10},{"visited_templates",visitedTemplates},{"facets",QJsonArray{"entire-native-viewable-registry-and-hidden-command-policy","root-branch-terminator-nested-context-and-objective-capacity","every-listed-template-preview-enclosing-solver-defaults","cancel-source-history-file-configured-isolation"}},{"scope","All command-template identities of this loaded engine/plugin registry in ten fixed Target/Optimize positions and every listed preview in branch append. No execution replay;positive mission results reuse separate receipts. Distinct If/If-Else widget paths,private NASA keyword completeness,all command execution/input domains/native OS input/platform/stress/current NASA remain open."}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;source->document()->setModified(false);return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
