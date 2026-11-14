#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "BaseException.hpp"
#include "CommandEditor.hpp"
#include "MainWindow.hpp"
#include "MissionModel.hpp"
#include "ReportParameterDialog.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Optimizer.hpp"
#include "SolverBranchCommand.hpp"
#include <QApplication>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>

static int assertions=0;
static void require(bool ok,const char *message){++assertions;if(!ok)throw std::runtime_error(message);}
static void events(){QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();}
static QByteArray read(const QString &p){QFile f(p);require(f.open(QIODevice::ReadOnly),"Minimize family input/report missing");return f.readAll();}
static void write(const QString &p,const QByteArray &data){QFile f(p);require(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"Minimize family evidence write failed");}
static QString number(double x){return QString::number(x,'g',17);}
static void near(double x,double y,double tolerance,const char *message){if(!(std::isfinite(x)&&std::isfinite(y)&&std::abs(x-y)<=tolerance))std::cerr<<message<<" actual="<<number(x).toStdString()<<" expected="<<number(y).toStdString()<<" tolerance="<<number(tolerance).toStdString()<<'\n';require(std::isfinite(x)&&std::isfinite(y)&&std::abs(x-y)<=tolerance,message);}

int main(int argc,char **argv){
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaMinimizeScalar");
 try{
  require(argc==4&&acceptanceWidgetPlatform(),"Supply startup, family fixture and exact case key");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath(),fixture=QFileInfo(argv[2]).absoluteFilePath(),key=QString::fromLocal8Bit(argv[3]);
  const auto all=QJsonDocument::fromJson(read(fixture)).object();QJsonObject spec;
  for(const auto &value:all["cases"].toArray())if(value.toObject()["key"].toString()==key){require(spec.isEmpty(),"Duplicate scalar case key");spec=value.toObject();}
  require(!spec.isEmpty(),"Unknown scalar case key");
  const QString reference=spec["reference"].toString(),type=spec["parameter_type"].toString();
  const double expected=spec["expected"].toDouble(),tolerance=spec["absolute_tolerance"].toDouble(),controlTolerance=spec["solver_control_tolerance"].toDouble(),scale=std::max(1.,std::abs(expected)*.001),dummy=-(std::abs(expected)+987.25);
  require(std::isfinite(expected)&&tolerance>0&&controlTolerance>0,"Invalid independent scalar oracle");
  const auto originalStartup=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_STARTUP_ORIGINAL",startup);const auto root=QDir::cleanPath(QDir(QFileInfo(originalStartup).absolutePath()).absoluteFilePath("../.."));
  const auto physical=all["physical_inputs"].toObject();for(auto it=physical.begin();it!=physical.end();++it)require(QString::fromLatin1(QCryptographicHash::hash(read(QDir(root).filePath(it.key())),QCryptographicHash::Sha256).toHex())==it.value().toString(),"Minimize physical input differs from independent oracle");
  QDir::setCurrent(QFileInfo(startup).absolutePath());TestSettings settings;QTemporaryDir files;require(files.isValid(),"Minimize family private directory unavailable");
  const auto report=files.filePath("scalar result Δ.csv"),saved=files.filePath("scalar accepted Δ.script");
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &data){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),data);};
  QString prefix=all["base_setup"].toString().replace("@ROOT@",root)+spec["setup"].toString();
  prefix+="Create Variable Control Bound Cost Keep Seed;\nControl = "+number(expected+scale)+";\nSeed = "+number(expected+scale)+";\nBound = "+number(expected+20*scale)+";\nKeep = 41;\nCost = "+number(dummy)+";\nCreate String Label;\nLabel = 'not numeric';\nCreate Array Grid[1,2];\nGrid(1,1) = 3;\nCreate Yukon Opt;\nOpt.ShowProgress = true;\nOpt.ReportFile = '"+files.filePath("solver.txt")+"';\nOpt.FeasibilityTolerance = 1e-10;\nOpt.OptimalityTolerance = 1e-10;\nOpt.FunctionTolerance = 1e-10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\n"+all["mission_prefix"].toString()+"Keep = Keep + 1;\n";
  const auto vary="Vary 'Independent control' Opt(Control = Seed, {Perturbation = "+number(scale*1e-5)+", Lower = "+number(expected-10*scale)+", Upper = "+number(expected+10*scale)+", MaxStep = "+number(5*scale)+", AdditiveScaleFactor = "+number(-expected)+", MultiplicativeScaleFactor = "+number(1/scale)+"});\n";
  const QString initial=prefix+"Optimize 'ObjectiveRole' Opt {ExitMode = SaveAndContinue, ShowProgressWindow = true};\n"+vary+"Minimize 'Objective' Opt(Cost);\nNonlinearConstraint 'Unique feasible control' Opt(Control = "+reference+");\nEndOptimize;\nReport 'Objective result' Values Control "+reference+" Cost Keep;\n% retain minimize family tail Ω\n";
  MainWindow window;require(window.initialize(startup),"Minimize family runtime initialization failed");window.show();acceptanceView(window);events();
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *area=window.findChild<QMdiArea *>("workspace");auto *tree=window.findChild<QTreeWidget *>("Mission");require(source&&messages&&area&&tree,"Minimize family workspace missing");
  source->setPlainText(initial);const bool built=window.buildScript();if(!built){save("build.messages.txt",messages->toPlainText().toUtf8());std::cerr<<messages->toPlainText().toStdString();}require(built,"Minimize family baseline did not build");save("baseline.script",initial.toUtf8());
  auto close=[&]{for(auto *sub:area->subWindowList())if(auto *panel=dynamic_cast<CommandEditor *>(sub->widget())){panel->discardChanges();require(sub->close(),"Minimize family clean panel did not close");}events();require(window.findChild<QPlainTextEdit *>("scriptEditor")==source,"Minimize family panel closure lost the script document");};
  auto open=[&](const QString &label){
   const auto snapshot=window.missionSnapshot();int index=-1;for(int i=0;i<snapshot.nodes.size();++i)if(missionCommandLabel(snapshot.nodes[i].statement).name==label)index=i;require(index>=0,"Minimize family named constraint missing");
   QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index)item=*it;
   require(item,"Minimize family Mission tree node missing");tree->expandAll();tree->setCurrentItem(item);tree->scrollToItem(item);events();tree->itemDoubleClicked(item,0);events();
   QMdiSubWindow *child=nullptr;for(auto *w:area->subWindowList())if(w->property("commandIndex").isValid()&&w->property("commandIndex").toInt()==index&&dynamic_cast<CommandEditor *>(w->widget()))child=w;
   if(!child){save("unmapped-source.script",snapshot.sourceScript.toUtf8());save("unmapped-canonical.script",snapshot.canonicalScript.toUtf8());save("editor.messages.txt",messages->toPlainText().toUtf8());}
   require(child,"Minimize family actual constraint editor missing");child->showNormal();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);acceptanceView(*child);return child;
  };
  QString applied=initial;QJsonArray picks;
  for(const auto &role:QStringList{"Objective"}){
   const auto label=role;auto *child=open(label);auto *field=child->findChild<QLineEdit *>("commandField_Objective");auto *choose=child->findChild<QPushButton *>("commandChoose_Objective");
   require(field&&choose&&field->isVisible()&&choose->isVisible()&&field->text()=="Cost","Minimize family typed operand controls missing");
   std::exception_ptr failure;bool visited=false;
   QTimer::singleShot(0,child,[&]{auto *dialog=dynamic_cast<ReportParameterDialog *>(QApplication::activeModalWidget());try{
    require(dialog,"Minimize family actual numeric picker missing");visited=true;acceptanceView(*dialog);
    auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry"),*owner=dialog->findChild<QComboBox *>("reportPropertyObject"),*property=dialog->findChild<QComboBox *>("reportPropertyType"),*dependency=dialog->findChild<QComboBox *>("reportPropertyDependency");auto *buttons=dialog->findChild<QDialogButtonBox *>();auto *use=dialog->findChild<QPushButton *>("reportUseReference");require(entry&&owner&&property&&dependency&&buttons&&use,"Minimize family property browser missing");
    for(const auto &bad:QStringList{"17","nan","inf","Label","Grid","Grid(2,1)","MissingScalar.Unknown"}){entry->setEditText(bad);events();require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Minimize family picker accepted non-scalar or missing reference");}
    const auto parts=reference.split('.');require(owner->findText(parts[0])>=0,"Minimize family owner missing from actual picker");owner->setCurrentText(parts[0]);events();require(property->findText(type)>=0,"Minimize family type missing from actual picker");property->setCurrentText(type);events();
    if(parts.size()==3){require(dependency->isVisible()&&dependency->findText(parts[1])>=0,"Minimize family dependency missing from actual picker");dependency->setCurrentText(parts[1]);}
    require(use->isEnabled(),"Minimize family use-reference action disabled");use->click();events();
    if(parts[0]=="Burn"&&parts.size()==2)entry->setEditText(reference); // Native frameless burn components are an explicit supported reference.
    require(entry->currentText()==reference&&buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Minimize family picker did not accept the exact expected reference");
    buttons->button(QDialogButtonBox::Ok)->click();
   }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});
   choose->click();if(failure)std::rethrow_exception(failure);require(visited&&field->text()==reference&&source->toPlainText()==applied,"Minimize family picker changed applied source prematurely");
   auto *panel=dynamic_cast<CommandEditor *>(child->widget());require(panel&&panel->hasChanges(),"Minimize family operand did not become pending");
   const auto before=applied;const QString old="Minimize 'Objective' Opt(Cost);";
   const QString replacement="Minimize 'Objective' Opt("+reference+");";
   require(applied.count(old)==1,"Minimize family replacement boundary ambiguous");applied.replace(old,replacement);
   child->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();require(source->toPlainText()==applied,"Minimize family Apply changed unrelated source or lost operand");
   source->undo();require(source->toPlainText()==before,"Minimize family Undo not exact");source->redo();require(source->toPlainText()==applied,"Minimize family Redo not exact");close();picks.append(QJsonObject{{"role",role},{"reference",reference}});
  }
  require(window.saveScriptTo(saved)&&read(saved)==applied.toUtf8()&&window.loadScript(saved),"Minimize family Unicode save/reopen not exact");save("accepted.script",applied.toUtf8());const int undo=source->document()->availableUndoSteps();
  const auto result=window.runMission();save("run.messages.txt",messages->toPlainText().toUtf8());if(result!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Minimize family active constraint mission failed");
  require(source->toPlainText()==applied&&source->document()->availableUndoSteps()==undo,"Minimize family Run changed source/history");
  const auto raw=read(report);save("results.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==1,"Minimize family requires one complete raw objective report");QJsonArray results;
  for(const auto &line:lines){const auto cells=line.split(',');require(cells.size()==4,"Minimize family raw result lost fields");QJsonArray values;double v[4];for(int j=0;j<4;++j){bool ok=false;v[j]=cells[j].trimmed().toDouble(&ok);require(ok&&std::isfinite(v[j]),"Minimize family result nonfinite");values.append(v[j]);}
   near(v[0],expected,controlTolerance,"Minimize family optimum differs from independent active bound");near(v[1],expected,tolerance,"Minimize family operand differs from independent physical input");near(v[2],dummy,tolerance,"Minimize family dummy cost changed or substituted for the objective");require(v[3]==42,"Minimize family non-graphical sentinel changed");results.append(values);}
  Optimizer *native=nullptr;for(auto *command=Moderator::Instance()->GetFirstCommand();command;command=command->GetNext())if(command->GetTypeName()=="Optimize"){auto *branch=dynamic_cast<SolverBranchCommand *>(command);require(branch&&!native,"Minimize family native branch ambiguous");native=dynamic_cast<Optimizer *>(branch->GetClone(0));}
  require(native&&native->GetTypeName()=="Yukon","Minimize family actual optimizer clone missing");
  const auto nativeObjective=QString::fromStdString(native->GetStringParameter(native->GetParameterID("ObjectiveFunction")));
  require(nativeObjective==reference,"Minimize family actual objective identity differs");
  require(native->GetIntegerParameter(native->GetParameterID("IntegerSolverStatus"))==Gmat::CONVERGED,"Minimize family native clone did not converge");
  QJsonArray progress;int objectives=0,goals=0;
  for(auto *table:window.findChildren<QTableWidget *>("solverProgress")){
   auto *parent=table->parentWidget();auto *status=parent?parent->findChild<QLabel *>("solverStatus"):nullptr;require(status&&status->property("converged").isValid()&&status->property("converged").toBool(),"Minimize family optimizer did not converge");
   for(int row=0;row<table->rowCount();++row){
    require(table->item(row,0)&&table->item(row,1)&&table->item(row,2),"Minimize family progress cell missing");const auto kind=table->item(row,0)->text(),name=table->item(row,1)->text();
    if(kind!="Objective"&&!kind.startsWith("Goal"))continue;
    bool ok=false;const double current=table->item(row,2)->text().toDouble(&ok);require(ok,"Minimize family actual progress current missing");
    if(kind=="Objective"){++objectives;require(name=="Cost","Minimize family actual objective progress label differs");near(current,expected,tolerance,"Minimize family native objective cost differs from independent physical input");require(std::abs(current-dummy)>100,"Minimize family dummy objective was executed");}
    else{++goals;require(kind==QString::fromUtf8("Goal =")&&name=="Control","Minimize family unique-feasible-control relation changed");near(current,expected,controlTolerance,"Minimize family actual feasible control differs");bool valid=false;const double desired=table->item(row,3)->text().toDouble(&valid);require(valid,"Minimize family actual constraint target missing");near(desired,expected,tolerance,"Minimize family actual constraint target differs");}
    progress.append(QJsonObject{{"name",name},{"current",current},{"kind",kind}});table->scrollToItem(table->item(row,1));acceptanceView(*table);
   }
  }
  require(objectives==1&&goals==1&&progress.size()==2,"Minimize family actual objective/feasibility progress missing");
  QJsonObject proof{{"checkpoint","minimize-scalar-family"},{"case",key},{"outcome","passed"},{"parameter_type",type},{"reference",reference},{"expected",expected},{"absolute_tolerance",tolerance},{"solver_control_tolerance",controlTolerance},{"dummy_cost",dummy},{"native_objective",nativeObjective},{"results",results},{"progress",progress},{"picks",picks},{"assertions",assertions},{"source_rows",spec["source_rows"]},{"facets",QJsonArray{"complete-declared-scalar-objective-member","actual-objective-picker-and-transaction","independent-physical-objective-value","unique-feasible-control","actual-native-objective-progress","raw-results-and-exact-source"}}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';
 }catch(BaseException &e){std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n';return 1;}catch(std::exception &e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
 return 0;
}
