#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include <QAction>
#include <QContextMenuEvent>
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
using namespace formaccept;

// Complement the retained NasaControlLogic If/Else and While sweeps with the
// missing no-Else path; do not replay those already-qualified visible cases.
static void reveal(QWidget *w)
{
 require(w,"Conditional-loop control absent");
 for(QWidget *p=w;p;p=p->parentWidget())if(auto *s=qobject_cast<QScrollArea *>(p))s->ensureWidgetVisible(w);
 events();require(w->isVisible()&&w->isEnabled(),"Conditional-loop control unreachable");
}
int main(int argc,char **argv)
{
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaConditionalObjectLoop");
 try {
  require(argc==3&&acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen","Supply startup and operand kind on native widgets");
  const QString kind=argv[2];require(QStringList{"Variable","Array","Property"}.contains(kind),"Conditional-loop operand kind absent");
  TestSettings settings;QTemporaryDir files;require(files.isValid(),"Conditional-loop fixture absent");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath(),saved=files.filePath("Conditional loop Ω.script"),report=files.filePath("Conditional Δ.csv");
  QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;window.resize(1200,850);window.show();require(window.initialize(startup),"Conditional-loop runtime absent");events();acceptanceView(window);
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&area,"Conditional-loop workspace absent");
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes){if(!evidence.isEmpty())write(evidence+"/"+name,bytes);};
  const QString before="If 'Decision' Input > 5; % preserve header",operand=kind=="Variable"?"Input":kind=="Array"?"Grid(1,1)":"Sat.EarthMJ2000Eq.X",condition=operand+" >= 0",after="If 'Decision' "+condition+"; % preserve header";
  QString initial="% Conditional object loop Ω\nCreate Variable Input Count Keep;\nInput = 0;\nCount = 0;\nKeep = 41;\nCreate Array Grid[1,1];\nGrid(1,1) = 0;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 7000;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7;\nSat.VZ = 0;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nFor 'Input sweep' Input = -1:1;\nGrid(1,1) = Input;\nSat.X = Input;\nCount = 0;\n"+before+"\nCount = Count + 1; % preserve body\nEndIf; % preserve end\nReport 'Witness' Values Input Count Grid(1,1) Sat.EarthMJ2000Eq.X Keep;\nEndFor;\n% Preserve tail Δ\n";
  QString wanted=initial;require(wanted.count(before)==1&&!wanted.contains("Else;"),"Conditional-loop literal no-Else fixture differs");wanted.replace(before,after);
  write(saved,initial.toUtf8());save("initial.script",initial.toUtf8());require(window.loadScript(saved)&&window.buildScript(),"Conditional-loop initial build failed");source->document()->setModified(false);
  auto state=[&](bool configured){auto *m=Moderator::Instance();auto object=[&](const char *name){return configured?m->GetConfiguredObject(name):m->GetInternalObject(name);};return QJsonArray{object("Input")->GetRealParameter("Value"),object("Count")->GetRealParameter("Value"),object("Grid")->GetRmatrixParameter("RmatValue")(0,0),object("Sat")->GetRealParameter("X"),object("Keep")->GetRealParameter("Value")};};
  const auto base=state(true);require(base==QJsonArray{0,0,0,7000,41},"Conditional-loop configured baseline differs");
  auto named=[&](const QString &label){const auto model=window.missionSnapshot();for(int i=0;i<model.nodes.size();++i)if(missionCommandLabel(model.nodes[i].statement).name==label)return i;throw std::runtime_error("Conditional-loop node absent");};
  auto open=[&]{
   for(auto *tabs:window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree))tabs->setCurrentIndex(i);
   QTreeWidgetItem *selected=nullptr;const int index=named("Decision");for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index)selected=*it;
   require(selected,"Conditional-loop tree entry absent");tree->expandAll();tree->setCurrentItem(selected);tree->scrollToItem(selected);events();reveal(tree);acceptanceView(window);std::exception_ptr failure;bool visited=false;
   QTimer::singleShot(0,&window,[&]{auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());try{require(menu,"Conditional-loop actual popup absent");QAction *choice=nullptr;for(auto *a:menu->actions())if(a->text()=="Edit command…")choice=a;require(choice&&choice->isEnabled(),"Conditional-loop edit action absent");menu->setActiveAction(choice);acceptanceView(*menu);visited=true;QTest::keyClick(menu,Qt::Key_Return);}catch(...){failure=std::current_exception();if(menu)menu->close();}});
   const auto point=tree->visualItemRect(selected).center();QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));QApplication::sendEvent(tree->viewport(),&event);events();if(failure)std::rethrow_exception(failure);require(visited,"Conditional-loop popup not visited");auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Conditional-loop actual editor absent");child->showNormal();child->move(0,0);child->resize(area->viewport()->size());events();acceptanceView(*child);return child;
  };
  auto configure=[&](QMdiSubWindow *child){auto *field=child->findChild<QLineEdit *>("commandField_Condition");reveal(field);acceptanceView(*child);QTest::mouseClick(field,Qt::LeftButton);field->selectAll();QTest::keyClicks(field,condition);events();auto *draft=child->findChild<QPlainTextEdit *>("commandSource");require(field->text()==condition&&draft&&draft->toPlainText().contains(after)&&!draft->toPlainText().contains("Else;")&&dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Conditional-loop typed no-Else draft differs");acceptanceView(*child);};
  auto click=[&](QMdiSubWindow *child,QDialogButtonBox::StandardButton which){auto *box=child->findChild<QDialogButtonBox *>();auto *button=box?box->button(which):nullptr;reveal(button);acceptanceView(*child);QTest::mouseClick(button,Qt::LeftButton);events();};
  const int undo=source->document()->availableUndoSteps();QPointer<QMdiSubWindow> child=open();configure(child);require(source->toPlainText()==initial&&source->document()->availableUndoSteps()==undo&&state(true)==base&&read(saved)==initial.toUtf8(),"Conditional-loop pending draft escaped isolation");click(child,QDialogButtonBox::Cancel);require(!child&&source->toPlainText()==initial&&state(true)==base,"Conditional-loop Cancel escaped isolation");
  child=open();configure(child);QPointer<CommandEditor> retired=dynamic_cast<CommandEditor *>(child->widget());click(child,QDialogButtonBox::Apply);require(!retired&&child&&source->toPlainText()==wanted,"Conditional-loop Apply changed unrelated source or retained stale editor");
  const int clean=source->document()->availableUndoSteps();click(child,QDialogButtonBox::Apply);require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==clean,"Conditional-loop clean Apply changed source/history");click(child,QDialogButtonBox::Ok);require(!child,"Conditional-loop OK retained owner");
  source->undo();require(source->toPlainText()==initial&&window.buildScript()&&state(true)==base,"Conditional-loop Undo failed");source->redo();require(source->toPlainText()==wanted&&window.buildScript()&&state(true)==base,"Conditional-loop Redo failed");
  require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==wanted&&read(saved)==wanted.toUtf8()&&state(true)==base,"Conditional-loop Unicode persistence failed");save("mission.script",wanted.toUtf8());
  const auto model=window.missionSnapshot();const int at=named("Decision"),parent=named("Input sweep");require(model.nodes[at].type=="If"&&model.nodes[at].parent==parent,"Conditional-loop command has wrong parent");for(const auto &n:model.nodes)require(n.type!="Else","Conditional-loop acquired an Else branch");
  const int steps=source->document()->availableUndoSteps();messages->clear();acceptanceView(window);const auto status=window.runMission();events();save("messages.txt",messages->toPlainText().toUtf8());require(status==MainWindow::RunResult::Completed,"Conditional-loop witness failed");
  const auto raw=read(report);save("values.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==3,"Conditional-loop sweep row count differs");QJsonArray rows,expected;
  for(int r=0;r<3;++r){const int input=r-1,count=input>=0?1:0;const QJsonArray want{input,count,input,input,41};const auto cells=lines[r].split(',');require(cells.size()==5,"Conditional-loop sweep width differs");QJsonArray actual;
   for(int c=0;c<5;++c){bool ok=false;const double value=cells[c].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-want[c].toDouble())<1e-10,"Conditional-loop result differs from independent changed-object truth oracle");actual.append(value);}rows.append(actual);expected.append(want);
  }
  const auto native=state(false);require(native==rows.last().toArray()&&state(true)==base,"Conditional-loop native/configured state differs");
  require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==steps&&read(saved)==wanted.toUtf8()&&!window.isRunning(),"Conditional-loop Run mutated source/history/file");
  save("native-result.json",QJsonDocument(QJsonObject{{"kind",kind},{"actual",rows},{"expected",expected},{"native",native},{"configured",base},{"operand",operand},{"no_else",true}}).toJson());
  const QJsonObject proof{{"checkpoint","conditional-object-loop-"+kind},{"outcome","passed"},{"kind",kind},{"operand",operand},{"rows",3},{"cancelled_drafts",1},{"clean_apply_checks",1},{"actual",rows},{"facets",QJsonArray{"actual-mission-tree-typed-condition-and-no-else-context","cancel-pending-apply-rebind-clean-history-unicode-isolation","changed-variable-array-and-same-frame-property-in-outer-loop","independent-negative-zero-positive-no-else-truth-oracle","final-native-and-configured-state-retention"}},{"scope","Selected no-Else If with Variable,Array element or same-frame Spacecraft property reread across an enclosing three-pass For sweep. Not If/Else equivalence,arbitrary object fields/expressions,inner-body property mutation,scientific frames/native OS/GPU/hardware/platform/stress/current NASA/full newest-build qualification."}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;source->document()->setModified(false);return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
  catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
 return 1;
}
