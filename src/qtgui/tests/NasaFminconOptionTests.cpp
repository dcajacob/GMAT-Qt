#include "FormationAcceptance.hpp"
#include "MatlabAcceptanceStartup.hpp"
#include "MatlabInterface.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabBar>
#include <QTabWidget>
#include <QTest>
#include <limits>
#include <algorithm>
using namespace formaccept;

static QString quoted(QString s) { return "'"+s.replace("'","''")+"'"; }

int main(int argc,char **argv)
{
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
 app.setOrganizationName("GMATTests");app.setApplicationName("NasaFminconOption");
 try {
  require(argc==4,"Supply startup,MATLAB plugin and actual option profile");
  const QString requested=argv[3];const bool rangeRecovery=requested=="DiffRangeRecovery";
  const QString profile=rangeRecovery?"DiffMinChange":requested;
  require(QStringList{"TolCon","TolFun","TolX","DiffMaxChange","DiffMinChange"}.contains(profile),"Unknown option profile");
  const bool difference=profile.startsWith("Diff"),constraintTolerance=profile=="TolCon";
  const double initial=difference?-1:0,target=difference?4:constraintTolerance?0:.05;
  const double constraintMinimum=constraintTolerance?.05:-2;
  const QString baseline=profile=="DiffMinChange"?"1e-12":"0.1";
  const QString edited=profile=="DiffMinChange"?"0.02":profile=="DiffMaxChange"?"1e-8":profile=="TolX"?"1e-9":"1e-10";
  QTemporaryDir runtime;require(runtime.isValid(),"Option private runtime failed");
  const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),runtime.path(),QFileInfo(argv[2]).absoluteFilePath());
  Rig rig(startup);rig.save("startup.txt",read(startup));
  const auto trials=rig.files.filePath("Option evaluations Ω.csv"),saved=rig.files.filePath("Option caller Ω.script");
  const QStringList names{"DiffMaxChange","DiffMinChange","MaxFunEvals","TolX","TolFun","TolCon"};
  QMap<QString,QString> settings{{"DiffMaxChange","0.1"},{"DiffMinChange","1e-12"},{"MaxFunEvals","600"},
      {"TolX","1e-9"},{"TolFun",constraintTolerance?"1e-6":"1e-10"},{"TolCon","1e-10"}};
  settings[profile]=edited;
  QString source="% Discriminating fmincon option Ω\nCreate Variable Alpha Cost Constraint Calls Keep;\nAlpha = "+number(initial)+";\nCost = -99;\nConstraint = -99;\nCalls = 0;\nKeep = 41;\nCreate FminconOptimizer Opt;\nOpt.MaximumIterations = 80;\nOpt.ShowProgress = false;\n";
  for(const auto &n:names)source+="Opt."+n+" = '"+settings[n]+"';\n";
  source+="Create ReportFile Trials Values;\nTrials.Filename = "+quoted(trials)+";\nValues.Filename = "+quoted(rig.report)+";\n";
  for(const auto &n:QStringList{"Trials","Values"})source+=n+".WriteHeaders = false;\n"+n+".FixedWidth = false;\n"+n+".Delimiter = ',';\n"+n+".Precision = 25;\n"+n+".SolverIterations = All;\n";
  source+="BeginMissionSequence;\nOptimize 'Option branch' Opt {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary 'Scaled control' Opt(Alpha = "+number(initial)+", {Lower = -5, Upper = 3, AdditiveScaleFactor = 3, MultiplicativeScaleFactor = 2.5});\nCost = (Alpha - "+number(target)+")^2;\nConstraint = Alpha - ("+number(constraintMinimum)+");\nCalls = Calls + 1;\nReport Trials Alpha Cost Constraint Calls;\nMinimize Opt(Cost);\nNonlinearConstraint Opt(Constraint >= 0);\nEndOptimize;\nReport Values Alpha Cost Constraint Calls Keep;\n% Preserve option caller tail Ω\n";
  rig.source->setPlainText(source);require(rig.window.buildScript(),"Option source Build failed");rig.save("seed.script",source.toUtf8());
  auto nativeOption=[&](const QString &name) {
   auto *o=Moderator::Instance()->GetConfiguredObject("Opt");require(o&&o->GetTypeName()=="FminconOptimizer","Actual option resource absent");
   return QString::fromStdString(o->GetStringParameter(name.toStdString()));
  };
  auto nativeSettings=[&] {QJsonObject result;for(const auto &n:names)result[n]=nativeOption(n);return result;};
  QJsonArray edits,phases;QPointer<QMdiSubWindow> child;
  auto open=[&] {
   for(auto *tabs:rig.window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->tabText(i)=="Resources") {
    if(tabs->currentIndex()!=i)QTest::mouseClick(tabs->tabBar(),Qt::LeftButton,Qt::NoModifier,tabs->tabBar()->tabRect(i).center());events();
   }
   auto entries=rig.resources->findItems("Opt",Qt::MatchExactly|Qt::MatchRecursive);require(entries.size()==1,"Option resource tree item missing");
   rig.resources->expandAll();rig.resources->scrollToItem(entries.first());rig.resources->setCurrentItem(entries.first());events();
   const auto point=rig.resources->visualItemRect(entries.first()).center();
   QTest::mouseClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);
   QTest::mouseDClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);
   QTest::mouseRelease(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();
   child=rig.area->activeSubWindow();auto *editor=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;
   require(editor&&child->property("resourceName").toString()=="Opt","Actual option editor absent");
   child->showNormal();child->move(0,0);child->resize(rig.area->viewport()->size());acceptanceView(*child);
   return editor;
  };
  auto edit=[&](const QString &phase,const QString &value) {
   const auto before=rig.source->toPlainText();const auto previous=nativeSettings();const int history=rig.source->document()->availableUndoSteps();
   auto *editor=open();auto *line=editor->findChild<QLineEdit *>("resource_"+profile);require(line&&line->isEnabled(),"Actual option line input missing");
   for(auto *tabs:editor->findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)->isAncestorOf(line))tabs->setCurrentIndex(i);
   for(auto *parent=line->parentWidget();parent;parent=parent->parentWidget())if(auto *scroll=qobject_cast<QScrollArea *>(parent))scroll->ensureWidgetVisible(line);
   events();require(line->isVisible(),"Actual option line input hidden");line->setFocus();line->selectAll();QTest::keyClicks(line,value);
   require(line->text()==value&&rig.source->toPlainText()==before&&nativeSettings()==previous&&rig.source->document()->availableUndoSteps()==history,"Option detached draft mutated caller/native/history");
   auto *buttons=editor->findChild<QDialogButtonBox *>("resourceButtons");require(buttons&&buttons->button(QDialogButtonBox::Apply)->isVisible(),"Option Apply absent");
   acceptanceView(*child);QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();
   const auto after=rig.source->toPlainText();auto expected=previous;expected[profile]=value;
   require(nativeSettings()==expected&&!editor->hasChanges()&&after!=before,"Actual option Apply differs from selected field");
   auto strip=[&](QString text) {text.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?Opt\\."+profile+"[ \\t]*=[^;]*;[ \\t]*(?:\\n|$)",QRegularExpression::MultilineOption));return text;};
   require(strip(before)==strip(after),"Option Apply changed unrelated caller bytes");rig.closePanels();
   rig.source->undo();require(rig.source->toPlainText()==before&&rig.window.buildScript()&&nativeSettings()==previous,"Option Undo lost exact source/native settings");
   rig.source->redo();require(rig.source->toPlainText()==after&&rig.window.buildScript()&&nativeSettings()==expected,"Option Redo lost exact source/native settings");
   source=after;settings[profile]=value;require(rig.window.saveScriptTo(saved),"Option Unicode Save failed");rig.save(phase+"-caller.script",source.toUtf8());
   edits.append(QJsonObject{{"phase",phase},{"option",profile},{"value",value},{"before",previous},{"after",expected},{"draft_isolated",true},{"exact_undo_redo",true}});
   rig.save("GUI-edits.json",QJsonDocument(edits).toJson());
  };
  auto *matlab=MatlabInterface::Instance();require(matlab->Open()==1,"Actual option MATLAB engine failed");
  matlab->RunMatlabString("global GMAT_QT_ITERATION_AUDIT; GMAT_QT_ITERATION_AUDIT = true;");
  QJsonArray recovery;
  if(rangeRecovery)for(const auto &bad:QStringList{"0.1","0.2"}) {
   const auto label=bad=="0.1"?"invalid-range-equal":"invalid-range-greater";edit(label,bad);
   const int history=rig.source->document()->availableUndoSteps();const auto nativeBefore=nativeSettings();
   rig.messages->clear();acceptanceView(rig.window);const auto failed=rig.window.runMission();events();
   const auto diagnostic=rig.messages->toPlainText();rig.save(QString(label)+"-messages.txt",diagnostic.toUtf8());
   require(failed==MainWindow::RunResult::Failed&&diagnostic.contains("DiffMinChange")&&diagnostic.contains("DiffMaxChange"),"Inconsistent finite-difference range was not rejected by actual MATLAB");
   matlab->RunMatlabString("gmat_option_recovery = 13;");double alive=0;Integer rows=0,cols=0;
   require(matlab->GetRealArray("gmat_option_recovery",1,&alive,rows,cols)==1&&alive==13,"Actual Engine did not recover after inconsistent range");
   require(rig.source->toPlainText()==source&&rig.source->document()->availableUndoSteps()==history&&read(saved)==source.toUtf8()&&nativeSettings()==nativeBefore,"Range rejection changed caller/history/disk/settings");
   recovery.append(QJsonObject{{"phase",label},{"DiffMinChange",bad},{"DiffMaxChange","0.1"},{"reported_failed",true},{"diagnostic",diagnostic},{"Engine_recovered",true},{"caller_unchanged",true}});
   rig.save("range-recovery.json",QJsonDocument(recovery).toJson());
  }
  for(const auto &phase:QStringList{"baseline","edited","reopened","repeat","restored"}) {
   const bool original=phase=="baseline"||phase=="restored";
   if(original||phase=="edited")edit(phase,original?baseline:edited);
   if(phase=="reopened")require(rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==source,"Option Unicode reopen differs");
   const auto expectedSettings=nativeSettings();const int history=rig.source->document()->availableUndoSteps();
   rig.messages->clear();acceptanceView(rig.window);const auto status=rig.window.runMission();events();rig.save(phase+"-messages.txt",rig.messages->toPlainText().toUtf8());
   if(status!=MainWindow::RunResult::Completed)std::cerr<<rig.messages->toPlainText().toStdString();
   require(status==MainWindow::RunResult::Completed,"Actual GUI-configured option mission failed");
   const auto raw=read(rig.report),evaluations=read(trials);rig.save(phase+"-report.csv",raw);rig.save(phase+"-evaluations.csv",evaluations);
   matlab->RunMatlabString("gmat_option_data = [GMAToptions.DiffMaxChange GMAToptions.DiffMinChange GMAToptions.MaxFunEvals GMAToptions.TolX GMAToptions.TolFun GMAToptions.TolCon GMAToptions.MaxIter fVal exitFlag GMATfminconOutput.iterations GMATfminconOutput.funcCount GMATfminconOutput.firstorderopt GMATfminconOutput.constrviolation X0 Lower Upper X]; gmat_option_iterations = jsonencode(GMAT_QT_ITERATION_HISTORY);");
   double values[17];Integer rows=0,cols=0;require(matlab->GetRealArray("gmat_option_data",17,values,rows,cols)==17&&rows==1&&cols==17,"Actual MATLAB option/result array shape differs");
   QJsonArray actual;for(double v:values){require(std::isfinite(v),"Actual option/result nonfinite");actual.append(v);}
   for(int i=0;i<6;++i){const double expected=settings[names[i]].toDouble();require(std::abs(values[i]-expected)<=std::abs(expected)*1e-14,"Actual MATLAB option does not match GUI/native value");}
   require(values[6]==80&&values[8]>0&&values[13]==2.5*(initial+3)&&values[14]==-5&&values[15]==15,"Actual MATLAB options/bounds/initial/status differ");
   std::string encoded;require(matlab->GetString("gmat_option_iterations",encoded)==1,"Actual option iteration history missing");
   QJsonParseError error;const auto document=QJsonDocument::fromJson(QByteArray::fromStdString(encoded),&error);
   require(error.error==QJsonParseError::NoError&&document.isArray(),"Actual option iteration history malformed");rig.save(phase+"-MATLAB-iterations.json",document.toJson());
   QJsonObject native;for(const auto &n:QStringList{"Alpha","Cost","Constraint","Calls","Keep"}){auto *o=Moderator::Instance()->GetInternalObject(n.toStdString());require(o,"Option native result absent");native[n]=o->GetRealParameter("Value");}
   const double alpha=native["Alpha"].toDouble(),cost=native["Cost"].toDouble();
   require(native["Keep"].toDouble()==41&&std::abs(cost-std::pow(alpha-target,2))<1e-10&&std::abs(native["Constraint"].toDouble()-(alpha-constraintMinimum))<1e-12,"Option native cost/constraint/sentinel formula differs");
   require(std::abs(values[7]-cost)<1e-10&&std::abs(values[16]-2.5*(alpha+3))<1e-10,"Actual MATLAB/native option result differs");
   const double optimum=difference?3:original?0:.05;
   require(std::abs(alpha-optimum)<1e-6&&std::abs(cost-std::pow(optimum-target,2))<1e-8,"Discriminating option did not produce independent expected control/cost");
   if(profile=="TolX"&&original)require(values[8]==4&&std::abs(alpha)<1e-12&&values[11]>1e-3,"Loose state tolerance did not stop a nonstationary problem");
   if(profile=="TolFun"&&original)require(values[8]==1&&std::abs(alpha)<1e-12&&values[11]>1e-3,"Loose cost tolerance did not accept the expected nonstationary point");
   if(constraintTolerance)require(original?(values[12]>.049&&values[12]<.051):(values[12]<1e-9),"Constraint tolerance did not distinguish the actual physical violation");
   const auto rawRows=QString::fromUtf8(evaluations).trimmed().split('\n');double count=0,probe=0;bool found=false;
   for(const auto &row:rawRows){const auto fields=row.split(',');require(fields.size()==4,"Option raw branch shape differs");std::array<double,4> v;
    for(int i=0;i<4;++i){bool ok=false;v[i]=fields[i].toDouble(&ok);require(ok&&std::isfinite(v[i]),"Option raw branch nonfinite");}
    require(std::abs(v[1]-std::pow(v[0]-target,2))<1e-10&&std::abs(v[2]-(v[0]-constraintMinimum))<1e-12&&v[3]==count+1,"Option raw cost/constraint/sequence differs");
    if(count==0)require(v[0]==initial,"Option physical initial guess differs");
    if(!found&&std::abs(v[0]-initial)>1e-14){probe=2.5*(v[0]-initial);found=true;}count=v[3];
   }
   require(found&&count==native["Calls"].toDouble(),"Option first perturbation/evaluation census missing");
   const double nominal=std::sqrt(std::numeric_limits<double>::epsilon())*std::max(1.0,std::abs(2.5*(initial+3)));
   const double expectedProbe=std::min(settings["DiffMaxChange"].toDouble(),std::max(settings["DiffMinChange"].toDouble(),nominal));
   require(std::abs(probe-expectedProbe)<1e-13,"Actual first scaled finite-difference probe differs from independent clamp formula");
   const auto fields=QString::fromUtf8(raw).trimmed().split(',');require(fields.size()==5,"Option final report shape differs");const QStringList outputNames{"Alpha","Cost","Constraint","Calls","Keep"};
   for(int i=0;i<5;++i){bool ok=false;const double value=fields[i].toDouble(&ok);require(ok&&std::abs(value-native[outputNames[i]].toDouble())<1e-12,"Option final raw/native report differs");}
   auto *output=rig.window.findChild<QTreeWidget *>("Output");const auto items=output?output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive):QList<QTreeWidgetItem *>();require(items.size()==1,"Option Output report item missing");
   output->itemDoubleClicked(items.first(),0);events();auto *viewer=rig.area->activeSubWindow()?dynamic_cast<ReportViewer *>(rig.area->activeSubWindow()->widget()):nullptr;
   auto *text=viewer?viewer->findChild<QPlainTextEdit *>():nullptr;require(text&&text->isReadOnly()&&text->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Actual option ReportViewer differs from raw file");
   acceptanceView(*rig.area->activeSubWindow());rig.area->activeSubWindow()->close();events();
   require(rig.source->toPlainText()==source&&rig.source->document()->availableUndoSteps()==history&&read(saved)==source.toUtf8()&&nativeSettings()==expectedSettings,"Option Run changed caller/history/disk/settings");
   phases.append(QJsonObject{{"phase",phase},{"native",native},{"settings",expectedSettings},{"MATLAB_data",actual},{"MATLAB_iterations",document.array()},
       {"first_scaled_probe",probe},{"independent_probe",expectedProbe},{"ReportViewer",true}});rig.save("phase-results.json",QJsonDocument(phases).toJson());
  }
  const QJsonObject proof{{"checkpoint","discriminating-fmincon-option"},{"profile",requested},{"GUI_edits",edits},{"phases",phases},{"range_recovery",recovery},{"new_requirement_credit",0}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
