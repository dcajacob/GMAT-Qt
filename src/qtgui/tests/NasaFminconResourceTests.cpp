#include "FormationAcceptance.hpp"
#include "MatlabAcceptanceStartup.hpp"
#include "MatlabInterface.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include <QAction>
#include <QClipboard>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
using namespace formaccept;
static QString quoted(QString value) { return "'"+value.replace("'","''")+"'"; }

int main(int argc,char **argv) {
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaFminconResource");
 try {
  require(argc==3,"Supply startup and actual MATLAB plugin");
  QTemporaryDir runtime;require(runtime.isValid(),"Fmincon resource startup isolation failed");
  const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),runtime.path(),QFileInfo(argv[2]).absoluteFilePath());
  Rig rig(startup);rig.save("startup.txt",read(startup));
  require(rig.window.availableEngineTypes().contains("FminconOptimizer"),"Actual fmincon factory absent");
  const auto trials=rig.files.filePath("Resource evaluations Ω.csv"),solverReport=rig.files.filePath("Resource solver Ω.txt");
  const QString seed="% Actual Resource Tree fmincon configuration Ω\nCreate Variable Alpha Cost Constraint Calls Keep;\n"
   "Alpha = -1.25;\nCost = -99;\nConstraint = -99;\nCalls = 0;\nKeep = 41;\n"
   "Create ReportFile Trials Values;\nTrials.Filename = "+quoted(trials)+";\nTrials.SolverIterations = All;\n"
   "Trials.WriteHeaders = false;\nTrials.FixedWidth = false;\nTrials.Delimiter = ',';\nTrials.Precision = 25;\n"
   "Values.Filename = "+quoted(rig.report)+";\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\n"
   "BeginMissionSequence;\nReport Values Keep;\n% Retain resource caller tail Ω\n";
  rig.source->setPlainText(seed);require(rig.window.buildScript(),"Fmincon resource seed Build failed");
  auto opt=[] {auto *o=Moderator::Instance()->GetConfiguredObject("Opt");require(o&&o->GetTypeName()=="FminconOptimizer","Actual native fmincon resource missing");return o;};
  const QStringList optionNames{"DiffMaxChange","DiffMinChange","MaxFunEvals","TolX","TolFun","TolCon"};
  auto native=[&] {auto *o=opt();QJsonObject state{{"MaximumIterations",o->GetIntegerParameter("MaximumIterations")},{"ShowProgress",o->GetBooleanParameter("ShowProgress")},{"ReportStyle",QString::fromStdString(o->GetStringParameter("ReportStyle"))},{"ReportFile",QString::fromStdString(o->GetStringParameter("ReportFile"))},{"FunctionPath",QString::fromStdString(o->GetStringParameter("FunctionPath"))},{"generated",QString::fromStdString(o->GetGeneratingString(Gmat::SHOW_SCRIPT))}};for(const auto &n:optionNames)state[n]=QString::fromStdString(o->GetStringParameter(n.toStdString()));return state;};
  QJsonArray creation;
  auto create=[&](bool accept) {
   auto entries=rig.resources->findItems("Solvers",Qt::MatchExactly|Qt::MatchRecursive);require(entries.size()==1,"Actual Solvers category missing");auto *entry=entries.first();
   rig.resources->expandAll();rig.resources->scrollToItem(entry);rig.resources->setCurrentItem(entry);events();
   const int history=rig.source->document()->availableUndoSteps();std::exception_ptr failure;bool menuSeen=false,dialogSeen=false;QTimer timer;timer.setInterval(20);
   QObject::connect(&timer,&QTimer::timeout,&rig.window,[&] {try {
    if(!menuSeen) {auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());if(!menu)return;auto *action=menu->findChild<QAction *>("addResource_FminconOptimizer");require(action&&action->isEnabled(),"Available fmincon absent from actual Add menu");acceptanceView(*menu);menuSeen=true;QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());return;}
    auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());if(!dialog||dialog->objectName()!="newResourceDialog")return;timer.stop();dialogSeen=true;acceptanceView(*dialog);
    auto *types=dialog->findChild<QComboBox *>("resourceType");auto *name=dialog->findChild<QLineEdit *>("resourceName");auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons");
    require(types&&types->count()==1&&types->currentText()=="FminconOptimizer"&&name&&name->isVisible()&&buttons,"Actual fmincon creator type/controls differ");
    name->selectAll();QTest::keyClicks(name,"Opt");
    require(rig.source->toPlainText()==seed&&!Moderator::Instance()->GetConfiguredObject("Opt")&&rig.source->document()->availableUndoSteps()==history,"Pending fmincon creation changed source/native/history");
    acceptanceView(*dialog);QTest::mouseClick(buttons->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel),Qt::LeftButton);
    require(dialog->result()==(accept?QDialog::Accepted:QDialog::Rejected),"Fmincon creator accept/Cancel failed");
   }catch(...) {failure=std::current_exception();timer.stop();if(auto *d=qobject_cast<QDialog *>(QApplication::activeModalWidget()))d->reject();if(auto *m=qobject_cast<QMenu *>(QApplication::activePopupWidget()))m->close();}});
   timer.start();const auto point=rig.resources->visualItemRect(entry).center();QContextMenuEvent event(QContextMenuEvent::Mouse,point,rig.resources->viewport()->mapToGlobal(point));QApplication::sendEvent(rig.resources->viewport(),&event);timer.stop();if(failure)std::rethrow_exception(failure);events();
   require(menuSeen&&dialogSeen,"Actual fmincon creator route not reached");creation.append(QJsonObject{{"accepted",accept},{"menu",menuSeen},{"dialog",dialogSeen}});rig.save("creation.json",QJsonDocument(creation).toJson());
  };
  create(false);require(rig.source->toPlainText()==seed&&!Moderator::Instance()->GetConfiguredObject("Opt"),"Fmincon creation Cancel mutated caller/model");
  create(true);const auto created=rig.source->toPlainText();const auto baseline=native();require(created!=seed&&created.contains("Create FminconOptimizer Opt;"),"Fmincon creation declaration absent");rig.save("created.script",created.toUtf8());
  QPointer<QMdiSubWindow> child;
  auto panel=[&] {auto *p=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(p,"Actual fmincon resource editor missing");return p;};
  auto open=[&] {
   for(auto *tabs:rig.window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->tabText(i)=="Resources"&&tabs->currentIndex()!=i){QTest::mouseClick(tabs->tabBar(),Qt::LeftButton,Qt::NoModifier,tabs->tabBar()->tabRect(i).center());events();}
   auto entries=rig.resources->findItems("Opt",Qt::MatchExactly|Qt::MatchRecursive);require(entries.size()==1,"Created fmincon tree item missing");rig.resources->expandAll();rig.resources->scrollToItem(entries.first());rig.resources->setCurrentItem(entries.first());events();const auto point=rig.resources->visualItemRect(entries.first()).center();
   QTest::mouseClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);QTest::mouseDClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);QTest::mouseRelease(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();child=rig.area->activeSubWindow();
   require(child&&child->property("resourceName").toString()=="Opt"&&dynamic_cast<ResourceEditor *>(child->widget()),"Resource Tree opened wrong fmincon editor");child->showNormal();child->move(0,0);child->resize(rig.area->viewport()->size());acceptanceView(*child);
  };
  open();auto *table=panel()->findChild<QTableWidget *>("resourceProperties");require(table,"Actual fmincon property table absent");QJsonArray fields,metadata;
  for(int i=0;i<table->rowCount();++i)fields.append(QJsonObject{{"name",table->item(i,0)->text()},{"value",table->item(i,1)?table->item(i,1)->text():(qobject_cast<QComboBox *>(table->cellWidget(i,1))?qobject_cast<QComboBox *>(table->cellWidget(i,1))->currentText():QString())},{"named_input",bool(panel()->findChild<QWidget *>("resource_"+table->item(i,0)->text()))}});
  for(int id=0;id<opt()->GetParameterCount();++id)metadata.append(QJsonObject{{"id",id},{"name",QString::fromStdString(opt()->GetParameterText(id))},{"type",int(opt()->GetParameterType(id))},{"readonly",opt()->IsParameterReadOnly(id)}});
  QJsonArray synthetic;for(const auto &n:optionNames)synthetic.append(QJsonObject{{"name",n},{"id",opt()->GetParameterID(n.toStdString())},{"value",baseline[n]},{"input_present",bool(panel()->findChild<QWidget *>("resource_"+n))}});
  rig.save("field-census.json",QJsonDocument(QJsonObject{{"fields",fields},{"native_metadata",metadata},{"native_option_aliases",synthetic},{"baseline",baseline}}).toJson());
  for(const auto &n:optionNames)require(panel()->findChild<QWidget *>("resource_"+n),"Actual fmincon option has no editable named GUI input (see field-census.json)");
  auto control=[&](const QString &name) {auto *v=panel()->findChild<QWidget *>("resource_"+name);require(v&&v->isEnabled(),"Fmincon configuration input missing/disabled");for(auto *tabs:panel()->findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)->isAncestorOf(v))tabs->setCurrentIndex(i);for(auto *p=v->parentWidget();p;p=p->parentWidget())if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(v);events();require(v->isVisible(),"Fmincon configuration input hidden");return v;};
  auto enter=[&](const QString &name,const QString &value) {auto *v=control(name);const auto source=rig.source->toPlainText();const auto state=native();const int history=rig.source->document()->availableUndoSteps();if(auto *line=qobject_cast<QLineEdit *>(v)){require(!line->isReadOnly(),"Fmincon configuration input readonly");line->setFocus();line->selectAll();if(value.isEmpty())QTest::keyClick(line,Qt::Key_Backspace);else {QApplication::clipboard()->setText(value);QTest::keyClick(line,Qt::Key_V,Qt::ControlModifier);}require(line->text()==value,"Fmincon pasted keyboard input differs");}else if(auto *combo=qobject_cast<QComboBox *>(v)){const int index=combo->findText(value);require(index>=0,"Fmincon configuration choice absent");combo->showPopup();QTest::keyClick(combo,Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(combo,Qt::Key_Down);QTest::keyClick(combo,Qt::Key_Return);require(combo->currentText()==value,"Fmincon configuration choice differs");}else throw std::runtime_error("Unsupported fmincon configuration widget");require(rig.source->toPlainText()==source&&native()==state&&rig.source->document()->availableUndoSteps()==history,"Pending fmincon edit changed caller/native/history");};
  auto apply=[&] {acceptanceView(*child);auto *buttons=panel()->findChild<QDialogButtonBox *>("resourceButtons");require(buttons&&buttons->button(QDialogButtonBox::Apply)->isVisible(),"Actual fmincon Apply hidden");QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();child=rig.area->activeSubWindow();};
  enter("TolX","9e-8");require(panel()->hasChanges(),"Fmincon draft not marked pending");rig.closePanels();require(rig.source->toPlainText()==created&&native()==baseline,"Discarded fmincon draft changed applied state");open();require(qobject_cast<QLineEdit *>(control("TolX"))->text()==baseline["TolX"].toString(),"Discarded fmincon draft leaked into reopened editor");
  QJsonArray rejections;const int history=rig.source->document()->availableUndoSteps();
  const QStringList validationNames=optionNames+QStringList{"MaximumIterations"};
  for(const auto &name:validationNames)for(const auto &bad:QStringList{"","not-a-number","nan","inf","0","-1","1.5junk"}) {
   enter(name,bad);apply();auto *status=panel()->findChild<QLabel *>("resourceStatus");
   const bool rejected=panel()->hasChanges()&&rig.source->toPlainText()==created&&native()==baseline&&rig.source->document()->availableUndoSteps()==history&&status&&status->isVisible()&&!status->text().isEmpty();
   rejections.append(QJsonObject{{"field",name},{"input",bad},{"rejected_atomically",rejected},{"diagnostic",status?status->text():QString()}});rig.save("rejections.json",QJsonDocument(rejections).toJson());require(rejected,"Invalid fmincon option changed caller/native/history or lacks visible diagnostic");
   enter(name,name=="MaximumIterations"?QString::number(baseline[name].toInt()):baseline[name].toString());
  }
  const QMap<QString,QString> settings{{"DiffMaxChange","0.2"},{"DiffMinChange","2e-7"},{"MaxFunEvals","400"},{"TolX","2e-9"},{"TolFun","3e-10"},{"TolCon","4e-10"},{"MaximumIterations","64"},{"ShowProgress","false"},{"ReportStyle","Verbose"},{"ReportFile",solverReport},{"FunctionPath",QDir::cleanPath(QFileInfo(argv[1]).absoluteDir().filePath("../matlab/gmat_fmincon"))}};
  for(auto i=settings.begin();i!=settings.end();++i)enter(i.key(),i.value());apply();const auto applied=rig.source->toPlainText();auto accepted=native();
  require(!panel()->hasChanges()&&applied!=created&&accepted["MaximumIterations"].toInt()==64&&!accepted["ShowProgress"].toBool()&&accepted["ReportStyle"].toString()=="Verbose"&&accepted["ReportFile"].toString()==solverReport&&accepted["FunctionPath"].toString()==settings["FunctionPath"],"Fmincon GUI Apply lost common configuration");for(const auto &n:optionNames)require(accepted[n].toString()==settings[n],"Fmincon GUI Apply lost a native option");
  auto strip=[](QString s){s.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?Opt\\.[A-Za-z0-9_]+[ \\t]*=[^;]*;[ \\t]*(?:\\n|$)",QRegularExpression::MultilineOption));return s;};require(strip(applied)==strip(created),"Fmincon Apply changed unrelated source");
  rig.save("applied.script",applied.toUtf8());rig.save("native-settings.json",QJsonDocument(QJsonObject{{"before",baseline},{"after",accepted}}).toJson());rig.closePanels();rig.source->undo();require(rig.source->toPlainText()==created&&rig.window.buildScript()&&native()==baseline,"Fmincon resource Undo not exact");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript()&&native()==accepted,"Fmincon resource Redo not exact");
  const auto saved=rig.files.filePath("Fmincon resource Ω.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied&&native()==accepted,"Fmincon resource Unicode save/reopen differs");
  const auto boundary=applied.indexOf("BeginMissionSequence;");require(boundary>0,"Fmincon resource mission boundary missing");
  QString numerical=applied.left(boundary)+"BeginMissionSequence;\nOptimize 'GUI configured fmincon' Opt {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary Opt(Alpha = -1.25, {Lower = -5, Upper = 3, AdditiveScaleFactor = 0, MultiplicativeScaleFactor = 1});\nCost = (Alpha - 4)^2;\nConstraint = Alpha - 1;\nCalls = Calls + 1;\nReport Trials Alpha Cost Constraint Calls;\nMinimize Opt(Cost);\nNonlinearConstraint Opt(Constraint >= 0);\nEndOptimize;\nReport Values Alpha Cost Constraint Calls Keep;\n% Retain resource caller tail Ω\n";
  rig.source->setPlainText(numerical);require(rig.window.buildScript()&&rig.window.saveScriptTo(saved),"GUI configured fmincon numerical caller failed");rig.save("caller.script",numerical.toUtf8());
  QJsonArray phases;
  for(const auto &phase:QStringList{"gui-configured","reopened","progress-enabled"}) {
   if(phase=="reopened")require(rig.window.loadScript(saved)&&rig.window.buildScript()&&native()==accepted,"Fmincon configured numerical reopen lost settings");
   if(phase=="progress-enabled") {open();enter("ShowProgress","true");apply();const auto progressSource=rig.source->toPlainText();auto progressNative=native();auto expected=accepted;expected["ShowProgress"]=true;expected["generated"]=progressNative["generated"];require(progressNative==expected&&progressSource!=numerical,"Actual GUI ShowProgress toggle changed other native settings");numerical=progressSource;accepted=progressNative;rig.closePanels();require(rig.window.saveScriptTo(saved),"Progress-enabled caller Save failed");rig.save("progress-enabled-caller.script",numerical.toUtf8());}
   require(accepted["ShowProgress"].toBool()==(phase=="progress-enabled"),"Configured progress enable/disable phase differs");
   const int undo=rig.source->document()->availableUndoSteps();acceptanceView(rig.window);const auto result=rig.window.runMission();events();rig.save(phase+"-messages.txt",rig.messages->toPlainText().toUtf8());if(result!=MainWindow::RunResult::Completed)std::cerr<<rig.messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"GUI configured actual fmincon solve failed");
   rig.save(phase+"-file-census.json",QJsonDocument(QJsonObject{{"final_report",rig.report},{"final_exists",QFileInfo::exists(rig.report)},{"evaluations",trials},{"evaluations_exist",QFileInfo::exists(trials)},{"solver_report",solverReport},{"solver_report_exists",QFileInfo::exists(solverReport)},{"runtime_files",QJsonArray::fromStringList(QDir(rig.files.path()).entryList(QDir::Files))}}).toJson());
   require(QFileInfo::exists(rig.report),"Configured fmincon final ReportFile missing");require(QFileInfo::exists(trials),"Configured fmincon evaluation ReportFile missing");require(QFileInfo::exists(solverReport)==accepted["ShowProgress"].toBool(),"Configured fmincon solver report presence differs from documented ShowProgress setting");
   const auto raw=read(rig.report),evaluations=read(trials);rig.save(phase+"-report.csv",raw);rig.save(phase+"-evaluations.csv",evaluations);if(accepted["ShowProgress"].toBool()) {const auto text=read(solverReport);require(text.contains("Fmincon Text File")&&text.contains("optimizer scaled value"),"Enabled Verbose fmincon report lacks its documented physical/scaled control data");rig.save(phase+"-solver.txt",text);}
   QJsonObject state;for(const auto &n:QStringList{"Alpha","Cost","Constraint","Calls","Keep"}){auto *o=Moderator::Instance()->GetInternalObject(n.toStdString());require(o&&o->GetTypeName()=="Variable","Configured fmincon result absent");state[n]=o->GetRealParameter("Value");}
   require(std::abs(state["Alpha"].toDouble()-3)<1e-6&&std::abs(state["Cost"].toDouble()-1)<1e-8&&std::abs(state["Constraint"].toDouble()-2)<1e-6&&state["Keep"].toDouble()==41,"GUI configured fmincon disagrees with analytic active-bound optimum");
   const auto columns=QString::fromUtf8(raw).trimmed().split(',');const QStringList names{"Alpha","Cost","Constraint","Calls","Keep"};require(columns.size()==names.size(),"Configured fmincon final report shape differs");for(int i=0;i<names.size();++i){bool ok=false;const auto v=columns[i].toDouble(&ok);require(ok&&std::isfinite(v)&&std::abs(v-state[names[i]].toDouble())<1e-12,"Configured fmincon raw/native final result differs");}
   const auto rows=QString::fromUtf8(evaluations).trimmed().split('\n');double previous=0;for(const auto &row:rows){const auto cols=row.split(',');require(cols.size()==4,"Configured fmincon evaluation shape differs");std::array<double,4> v;for(int i=0;i<4;++i){bool ok=false;v[i]=cols[i].toDouble(&ok);require(ok&&std::isfinite(v[i]),"Configured fmincon evaluation nonfinite");}require(std::abs(v[1]-std::pow(v[0]-4,2))<1e-10&&std::abs(v[2]-(v[0]-1))<1e-12&&v[3]==previous+1,"Configured fmincon branch trace differs from analytic equations/sequence");if(previous==0)require(v[0]==-1.25,"Configured fmincon initial control differs");previous=v[3];}require(previous==state["Calls"].toDouble()&&rows.size()>=3,"Configured fmincon evaluation count differs");
   auto *matlab=MatlabInterface::Instance();matlab->RunMatlabString("gmat_resource_options = [GMAToptions.DiffMaxChange GMAToptions.DiffMinChange GMAToptions.MaxFunEvals GMAToptions.TolX GMAToptions.TolFun GMAToptions.TolCon GMAToptions.MaxIter];");double options[7];Integer nrows=0,ncols=0;require(matlab->GetRealArray("gmat_resource_options",7,options,nrows,ncols)==7&&nrows==1&&ncols==7,"Actual MATLAB options readback shape differs");QJsonArray actualOptions;for(int i=0;i<7;++i){const auto expected=i==6?64:settings[optionNames[i]].toDouble();require(std::isfinite(options[i])&&std::abs(options[i]-expected)<=std::abs(expected)*1e-14,"Actual MATLAB option differs from GUI/native setting");actualOptions.append(options[i]);}
   double x=0,cost=0,flag=0;require(matlab->GetRealArray("X",1,&x,nrows,ncols)==1&&std::abs(x-3)<1e-6&&matlab->GetRealArray("fVal",1,&cost,nrows,ncols)==1&&std::abs(cost-1)<1e-8&&matlab->GetRealArray("exitFlag",1,&flag,nrows,ncols)==1&&flag>0,"Actual MATLAB solve differs from analytic active-bound optimum");
   auto *output=rig.window.findChild<QTreeWidget *>("Output");const auto items=output?output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive):QList<QTreeWidgetItem *>();require(items.size()==1,"Configured fmincon report Output item absent");output->itemDoubleClicked(items.first(),0);events();auto *viewer=rig.area->activeSubWindow()?dynamic_cast<ReportViewer *>(rig.area->activeSubWindow()->widget()):nullptr;auto *text=viewer?viewer->findChild<QPlainTextEdit *>():nullptr;require(text&&text->isReadOnly()&&text->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Configured fmincon ReportViewer differs from raw file");rig.area->activeSubWindow()->close();events();
   require(rig.source->toPlainText()==numerical&&rig.source->document()->availableUndoSteps()==undo&&read(saved)==numerical.toUtf8()&&native()==accepted,"Configured fmincon Run changed caller/history/disk/settings");state["phase"]=phase;state["MATLAB_options"]=actualOptions;state["MATLAB_X"]=x;state["MATLAB_fVal"]=cost;state["MATLAB_exitFlag"]=flag;phases.append(state);rig.save("results-values.json",QJsonDocument(phases).toJson());
  }
  const QJsonObject proof{{"family","fmincon-resource"},{"checkpoint","fmincon-resource-creation-and-configuration"},{"outcome","passed"},{"creation_dialogs",2},{"invalid_rejections",rejections.size()},{"configured_options",6},{"phases",phases},{"new_requirement_credit",0},{"limits","Actual Resource Tree creation and all editable fmincon resource settings, atomic invalid correction, detached draft, exact history, Unicode save/reopen, three real active-upper-bound solves, documented ShowProgress off/on report behavior and actual MATLAB option readback. No GUI Vary/solver selection, discriminating limit/tolerance cases, actual optimizer iteration/progress/report conjunction, general scientific/core/current-build or NASA qualification."}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
