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
static QByteArray read(const QString &p){QFile f(p);require(f.open(QIODevice::ReadOnly),"Scalar family input/report missing");return f.readAll();}
static void write(const QString &p,const QByteArray &data){QFile f(p);require(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"Scalar family evidence write failed");}
static QString number(double x){return QString::number(x,'g',17);}
static void near(double x,double y,double tolerance,const char *message){if(!(std::isfinite(x)&&std::isfinite(y)&&std::abs(x-y)<=tolerance))std::cerr<<message<<" actual="<<number(x).toStdString()<<" expected="<<number(y).toStdString()<<" tolerance="<<number(tolerance).toStdString()<<'\n';require(std::isfinite(x)&&std::isfinite(y)&&std::abs(x-y)<=tolerance,message);}

int main(int argc,char **argv){
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaConstraintScalar");
 try{
  require(argc==4&&acceptanceWidgetPlatform(),"Supply startup, family fixture and exact case key");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath(),fixture=QFileInfo(argv[2]).absoluteFilePath(),key=QString::fromLocal8Bit(argv[3]);
  const auto all=QJsonDocument::fromJson(read(fixture)).object();QJsonObject spec;
  for(const auto &value:all["cases"].toArray())if(value.toObject()["key"].toString()==key){require(spec.isEmpty(),"Duplicate scalar case key");spec=value.toObject();}
  require(!spec.isEmpty(),"Unknown scalar case key");
  const QString reference=spec["reference"].toString(),type=spec["parameter_type"].toString();
  const double expected=spec["expected"].toDouble(),tolerance=spec["absolute_tolerance"].toDouble(),controlTolerance=spec["solver_control_tolerance"].toDouble(),scale=std::max(1.,std::abs(expected)*.001);
  require(std::isfinite(expected)&&tolerance>0&&controlTolerance>0,"Invalid independent scalar oracle");
  const auto originalStartup=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_STARTUP_ORIGINAL",startup);const auto root=QDir::cleanPath(QDir(QFileInfo(originalStartup).absolutePath()).absoluteFilePath("../.."));
  const auto physical=all["physical_inputs"].toObject();for(auto it=physical.begin();it!=physical.end();++it)require(QString::fromLatin1(QCryptographicHash::hash(read(QDir(root).filePath(it.key())),QCryptographicHash::Sha256).toHex())==it.value().toString(),"Scalar physical input differs from independent oracle");
  QDir::setCurrent(QFileInfo(startup).absolutePath());TestSettings settings;QTemporaryDir files;require(files.isValid(),"Scalar family private directory unavailable");
  const auto report=files.filePath("scalar result Δ.csv"),saved=files.filePath("scalar accepted Δ.script");
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &data){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),data);};
  QString prefix=all["base_setup"].toString().replace("@ROOT@",root)+spec["setup"].toString();
  prefix+="Create Variable Control Bound Cost Keep Seed;\nControl = "+number(expected+scale)+";\nSeed = "+number(expected+scale)+";\nBound = "+number(expected+20*scale)+";\nKeep = 41;\nCreate String Label;\nLabel = 'not numeric';\nCreate Array Grid[1,2];\nGrid(1,1) = 3;\nCreate Yukon Opt;\nOpt.ShowProgress = true;\nOpt.ReportFile = '"+files.filePath("solver.txt")+"';\nOpt.FeasibilityTolerance = 1e-10;\nOpt.OptimalityTolerance = 1e-10;\nOpt.FunctionTolerance = 1e-10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\n"+all["mission_prefix"].toString()+"Keep = Keep + 1;\n";
  const auto vary="Vary 'Independent control' Opt(Control = Seed, {Perturbation = "+number(scale*1e-5)+", Lower = "+number(expected-10*scale)+", Upper = "+number(expected+10*scale)+", MaxStep = "+number(5*scale)+", AdditiveScaleFactor = "+number(-expected)+", MultiplicativeScaleFactor = "+number(1/scale)+"});\n";
  const auto cost="Cost = ((Control - "+number(expected+2*scale)+") / "+number(scale)+")^2;\nMinimize 'Objective' Opt(Cost);\n";
  const QString initial=prefix+"Optimize 'RightRole' Opt {ExitMode = SaveAndContinue, ShowProgressWindow = true};\n"+vary+cost+"NonlinearConstraint 'RightBound' Opt(Control <= Bound);\nEndOptimize;\nReport 'Right result' Values Control "+reference+" Cost Keep;\nOptimize 'LeftRole' Opt {ExitMode = SaveAndContinue, ShowProgressWindow = true};\n"+vary+cost+"NonlinearConstraint 'LeftBound' Opt(Bound >= Control);\nEndOptimize;\nReport 'Left result' Values Control "+reference+" Cost Keep;\n% retain scalar family tail Ω\n";
  MainWindow window;require(window.initialize(startup),"Scalar family runtime initialization failed");window.show();acceptanceView(window);events();
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *area=window.findChild<QMdiArea *>("workspace");auto *tree=window.findChild<QTreeWidget *>("Mission");require(source&&messages&&area&&tree,"Scalar family workspace missing");
  source->setPlainText(initial);const bool built=window.buildScript();if(!built){save("build.messages.txt",messages->toPlainText().toUtf8());std::cerr<<messages->toPlainText().toStdString();}require(built,"Scalar family baseline did not build");save("baseline.script",initial.toUtf8());
  auto close=[&]{for(auto *sub:area->subWindowList())if(auto *panel=dynamic_cast<CommandEditor *>(sub->widget())){panel->discardChanges();require(sub->close(),"Scalar family clean panel did not close");}events();require(window.findChild<QPlainTextEdit *>("scriptEditor")==source,"Scalar family panel closure lost the script document");};
  auto open=[&](const QString &label){
   const auto snapshot=window.missionSnapshot();int index=-1;for(int i=0;i<snapshot.nodes.size();++i)if(missionCommandLabel(snapshot.nodes[i].statement).name==label)index=i;require(index>=0,"Scalar family named constraint missing");
   QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index)item=*it;
   require(item,"Scalar family Mission tree node missing");tree->expandAll();tree->setCurrentItem(item);tree->scrollToItem(item);events();tree->itemDoubleClicked(item,0);events();
   QMdiSubWindow *child=nullptr;for(auto *w:area->subWindowList())if(w->property("commandIndex").isValid()&&w->property("commandIndex").toInt()==index&&dynamic_cast<CommandEditor *>(w->widget()))child=w;
   if(!child){save("unmapped-source.script",snapshot.sourceScript.toUtf8());save("unmapped-canonical.script",snapshot.canonicalScript.toUtf8());save("editor.messages.txt",messages->toPlainText().toUtf8());}
   require(child,"Scalar family actual constraint editor missing");child->showNormal();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);acceptanceView(*child);return child;
  };
  QString applied=initial;QJsonArray picks;
  for(const auto &role:QStringList{"Right","Left"}){
   const auto label=role+"Bound";auto *child=open(label);auto *field=child->findChild<QLineEdit *>(role=="Right"?"commandField_Right side":"commandField_Left side");auto *choose=child->findChild<QPushButton *>(role=="Right"?"commandChoose_Right side":"commandChoose_Left side");
   require(field&&choose&&field->isVisible()&&choose->isVisible()&&field->text()=="Bound","Scalar family typed operand controls missing");
   std::exception_ptr failure;bool visited=false;
   QTimer::singleShot(0,child,[&]{auto *dialog=dynamic_cast<ReportParameterDialog *>(QApplication::activeModalWidget());try{
    require(dialog,"Scalar family actual numeric picker missing");visited=true;acceptanceView(*dialog);
    auto *entry=dialog->findChild<QComboBox *>("reportParameterEntry"),*owner=dialog->findChild<QComboBox *>("reportPropertyObject"),*property=dialog->findChild<QComboBox *>("reportPropertyType"),*dependency=dialog->findChild<QComboBox *>("reportPropertyDependency");auto *buttons=dialog->findChild<QDialogButtonBox *>();auto *use=dialog->findChild<QPushButton *>("reportUseReference");require(entry&&owner&&property&&dependency&&buttons&&use,"Scalar family property browser missing");
    for(const auto &bad:QStringList{"nan","inf","Label","Grid","Grid(2,1)","MissingScalar.Unknown"}){entry->setEditText(bad);events();require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Scalar family picker accepted non-scalar or missing reference");}
    const auto parts=reference.split('.');require(owner->findText(parts[0])>=0,"Scalar family owner missing from actual picker");owner->setCurrentText(parts[0]);events();require(property->findText(type)>=0,"Scalar family type missing from actual picker");property->setCurrentText(type);events();
    if(parts.size()==3){require(dependency->isVisible()&&dependency->findText(parts[1])>=0,"Scalar family dependency missing from actual picker");dependency->setCurrentText(parts[1]);}
    require(use->isEnabled(),"Scalar family use-reference action disabled");use->click();events();
    if(parts[0]=="Burn"&&parts.size()==2)entry->setEditText(reference); // Native frameless burn components are an explicit supported reference.
    require(entry->currentText()==reference&&buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Scalar family picker did not accept the exact expected reference");
    buttons->button(QDialogButtonBox::Ok)->click();
   }catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});
   choose->click();if(failure)std::rethrow_exception(failure);require(visited&&field->text()==reference&&source->toPlainText()==applied,"Scalar family picker changed applied source prematurely");
   auto *panel=dynamic_cast<CommandEditor *>(child->widget());require(panel&&panel->hasChanges(),"Scalar family operand did not become pending");
   const auto before=applied;const QString old=role=="Right"?"NonlinearConstraint 'RightBound' Opt(Control <= Bound);":"NonlinearConstraint 'LeftBound' Opt(Bound >= Control);";
   const QString replacement=role=="Right"?"NonlinearConstraint 'RightBound' Opt(Control <= "+reference+");":"NonlinearConstraint 'LeftBound' Opt("+reference+" >= Control);";
   require(applied.count(old)==1,"Scalar family replacement boundary ambiguous");applied.replace(old,replacement);
   child->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();require(source->toPlainText()==applied,"Scalar family Apply changed unrelated source or lost operand");
   source->undo();require(source->toPlainText()==before,"Scalar family Undo not exact");source->redo();require(source->toPlainText()==applied,"Scalar family Redo not exact");close();picks.append(QJsonObject{{"role",role},{"reference",reference}});
  }
  require(window.saveScriptTo(saved)&&read(saved)==applied.toUtf8()&&window.loadScript(saved),"Scalar family Unicode save/reopen not exact");save("accepted.script",applied.toUtf8());const int undo=source->document()->availableUndoSteps();
  const auto result=window.runMission();save("run.messages.txt",messages->toPlainText().toUtf8());if(result!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Scalar family active constraint mission failed");
  require(source->toPlainText()==applied&&source->document()->availableUndoSteps()==undo,"Scalar family Run changed source/history");
  const auto raw=read(report);save("results.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==2,"Scalar family requires both operand roles in raw report");QJsonArray results;
  for(const auto &line:lines){const auto cells=line.split(',');require(cells.size()==4,"Scalar family raw result lost fields");QJsonArray values;double v[4];for(int j=0;j<4;++j){bool ok=false;v[j]=cells[j].trimmed().toDouble(&ok);require(ok&&std::isfinite(v[j]),"Scalar family result nonfinite");values.append(v[j]);}
   near(v[0],expected,controlTolerance,"Scalar family optimum differs from independent active bound");near(v[1],expected,tolerance,"Scalar family operand differs from independent physical input");near(v[2],4,std::max(2e-6,8*controlTolerance/scale),"Scalar family constrained objective differs from independent quadratic");require(v[3]==42,"Scalar family non-graphical sentinel changed");results.append(values);}
  QJsonArray progress;bool leftSeen=false,rightSeen=false;
  for(auto *table:window.findChildren<QTableWidget *>("solverProgress")){
   auto *parent=table->parentWidget();auto *status=parent?parent->findChild<QLabel *>("solverStatus"):nullptr;require(status&&status->property("converged").isValid()&&status->property("converged").toBool(),"Scalar family actual optimizer did not converge");
   for(int row=0;row<table->rowCount();++row){if(!table->item(row,0)||!table->item(row,0)->text().startsWith("Goal"))continue;
    const auto name=table->item(row,1)->text();bool ok=false;const double current=table->item(row,2)->text().toDouble(&ok);require(ok,"Scalar actual constraint current missing");near(current,expected,name==reference?tolerance:controlTolerance,"Scalar actual evaluated constraint value differs");const double desired=table->item(row,3)->text().toDouble(&ok);require(ok,"Scalar actual constraint desired missing");near(desired,expected,name==reference?controlTolerance:tolerance,"Scalar actual evaluated desired value differs");
    if(name==reference){leftSeen=true;require(table->item(row,0)->text()==QString::fromUtf8("Goal ≥"),"Scalar parameter-left relation changed");}else{require(name=="Control","Unexpected scalar constraint identity");rightSeen=true;require(table->item(row,0)->text()==QString::fromUtf8("Goal ≤"),"Scalar parameter-right relation changed");}
    progress.append(QJsonObject{{"name",name},{"current",current},{"desired",desired},{"kind",table->item(row,0)->text()}});table->scrollToItem(table->item(row,1));acceptanceView(*table);
   }
  }
  require(leftSeen&&rightSeen&&progress.size()==2,"Scalar family actual left/right evaluation progress missing");
  QJsonObject proof{{"checkpoint","constraint-scalar-family"},{"case",key},{"outcome","passed"},{"parameter_type",type},{"reference",reference},{"expected",expected},{"absolute_tolerance",tolerance},{"solver_control_tolerance",controlTolerance},{"results",results},{"progress",progress},{"picks",picks},{"assertions",assertions},{"roles",QJsonArray{"left","right"}},{"source_rows",spec["source_rows"]},{"facets",QJsonArray{"complete-declared-scalar-family-member","actual-typed-picker-and-transaction","independent-physical-operand-and-active-optimum","both-operand-roles","actual-evaluated-optimizer-progress","raw-results-and-exact-source"}}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';
 }catch(BaseException &e){std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n';return 1;}catch(std::exception &e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
 return 0;
}
