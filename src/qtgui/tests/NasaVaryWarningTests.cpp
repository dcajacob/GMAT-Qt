#include "Vf13adAcceptance.hpp"
#include "CommandEditor.hpp"
#include "Vary.hpp"
#include "DifferentialCorrector.hpp"
#include "Variable.hpp"
#include "VariableWrapper.hpp"
#include "NumberWrapper.hpp"
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
#include <memory>
using namespace vfaccept;
namespace {
const QStringList fields{"Perturbation","Lower","Upper","MaxStep","AdditiveScaleFactor","MultiplicativeScaleFactor"};
const QStringList capabilities{"AllowVariablePertSetting","AllowRangeSettings","AllowRangeSettings","AllowStepsizeSetting","AllowScaleSetting","AllowScaleSetting"};
const QStringList values{"0.01","-100","100","0.5","0.0","1.0"};
QString warning(const QString &field,const QString &type="VF13ad",const QString &solver="Opt") {
 return "*** Warning *** Vary for 'X': setting '"+field+"' is not supported by solver '"+solver+"' ("+type+").";
}
QJsonArray warnings(QPlainTextEdit *messages,const QString &before,const QStringList &expected,const QString &type="VF13ad",const QString &solver="Opt") {
 events();
 const auto all=messages->toPlainText();require(all.startsWith(before),"Warning capture lost message history");
 const auto text=all.mid(before.size());QJsonArray found;
 for(const auto &field:fields){const int count=text.count(warning(field,type,solver));require(count==(expected.contains(field)?1:0),"Vary unsupported-setting warning count differs");if(count)found.append(field);}
 require(text.count("is not supported by solver")==expected.size(),"Unexpected unsupported-setting warning");return found;
}
class Capabilities final : public DifferentialCorrector {
public:
 Capabilities():DifferentialCorrector("Probe"){}
 void mask(int bits){AllowIndependentPerts=bits&1;AllowRangeLimits=bits&2;AllowStepsizeLimit=bits&4;AllowScaleFactors=bits&8;}
};
void wrappers(Vary &command,ObjectMap &objects,ObjectMap &global,Solver &solver) {
 command.SetObjectMap(&objects);command.SetGlobalObjectMap(&global);command.SetSolarSystem(Moderator::Instance()->GetSolarSystemInUse());
 require(command.SetRefObject(&solver,Gmat::SOLVER,solver.GetName()),"Vary solver binding failed");
 const auto names=command.GetWrapperObjectNameArray();
 for(const auto &name:names){ElementWrapper *wrapper=name=="X"?static_cast<ElementWrapper*>(new VariableWrapper):static_cast<ElementWrapper*>(new NumberWrapper);wrapper->SetDescription(name);require(command.SetElementWrapper(wrapper,name),"Vary wrapper binding failed");}
}
QJsonArray contract(Rig &rig,QPlainTextEdit *messages) {
 QJsonArray records;Capabilities solver;Variable variable("X","0");ObjectMap objects{{"X",&variable},{"Probe",&solver}},global;
 const int groups[]={1,2,2,4,8,8};
 for(int mask=0;mask<16;++mask)for(int field=-1;field<6;++field){
  solver.mask(mask);Vary command;
  const QString option=field<0?QString():", {"+fields[field]+" = "+values[field]+"}";
  command.SetGeneratingString(("Vary Probe(X = 1"+option+");").toStdString());require(command.InterpretAction(),"Vary capability fixture parse failed");wrappers(command,objects,global,solver);
  const auto before=messages->toPlainText();require(command.Initialize(),"Vary capability fixture initialization failed");
  const QStringList expected=field>=0&&!(mask&groups[field])?QStringList{fields[field]}:QStringList{};
  rig.save("contract-current.json",QJsonDocument(QJsonObject{{"mask",mask},{"field",field},{"messages",messages->toPlainText().mid(before.size())}}).toJson());
  records.append(QJsonObject{{"mask",mask},{"field",field<0?QString("omitted"):fields[field]},{"warnings",warnings(messages,before,expected,"DifferentialCorrector","Probe")}});
 }
 rig.save("capability-matrix.json",QJsonDocument(records).toJson());
 solver.mask(0);QJsonArray lifecycle;
 auto initialize=[&](Vary &command,const QString &phase,const QStringList &expected){wrappers(command,objects,global,solver);events();const auto before=messages->toPlainText();require(command.Initialize(),"Vary lifecycle initialization failed");lifecycle.append(QJsonObject{{"phase",phase},{"warnings",warnings(messages,before,expected,"DifferentialCorrector","Probe")}});};
 QStringList options;for(int i=0;i<fields.size();++i)options<<fields[i]+" = "+values[i];
 Vary original;original.SetGeneratingString(("Vary Probe(X = 1, {"+options.join(", ")+"});").toStdString());require(original.InterpretAction(),"Multi-option parse failed");initialize(original,"all-options",fields);
 original.GetGeneratingString(Gmat::NO_COMMENTS); // Serialization omits unsupported settings; provenance must survive.
 std::unique_ptr<Vary> clone(dynamic_cast<Vary*>(original.Clone()));require(bool(clone),"Vary clone type differs");initialize(*clone,"clone-after-serialization",fields);
 Vary assigned;assigned=original;initialize(assigned,"assignment",fields);
 Vary omitted;omitted.SetGeneratingString("Vary Probe(X = 1);");require(omitted.InterpretAction(),"Omitted-option parse failed");initialize(omitted,"omitted",{});assigned=omitted;initialize(assigned,"assignment-clears-options",{});
 original.SetGeneratingString("Vary Probe(X = 1);");require(original.InterpretAction(),"Reparse omitted options failed");initialize(original,"reparse-clears-options",{});
 original.SetStringParameter("Lower","-100");initialize(original,"string-setter",{"Lower"});
 const auto before=messages->toPlainText();original.SetNumber("Lower",-30);require(original.GetNumber(original.GetParameterID("Lower"))==-30,"Numeric setter changed semantics");lifecycle.append(QJsonObject{{"phase","numeric-setter"},{"warnings",warnings(messages,before,{"Lower"},"DifferentialCorrector","Probe")}});
 rig.save("option-lifecycle.json",QJsonDocument(lifecycle).toJson());
 records.append(QJsonObject{{"lifecycle",lifecycle}});
 QJsonArray census;QStringList eligible;
 const auto factories=Moderator::Instance()->GetListOfFactoryItems(Gmat::SOLVER);
 for(const auto &name:factories){std::unique_ptr<Solver> actual(Moderator::Instance()->CreateSolver(name,""));require(bool(actual),"Solver factory inventory failed");const bool vary=actual->IsOfType("BoundaryValueSolver")||actual->IsOfType("Optimizer");QJsonArray flags;
  if(vary){eligible.append(QString::fromStdString(name));for(int i=0;i<fields.size();++i)flags.append(actual->GetBooleanParameter(capabilities[i].toStdString()));}
  census.append(QJsonObject{{"type",QString::fromStdString(name)},{"vary_eligible",vary},{"capabilities",flags}});
 }
 eligible.sort();require(eligible==QStringList{"DifferentialCorrector","VF13ad","Yukon"},"A live Vary solver is not covered by GUI witnesses");
 rig.save("solver-factory-census.json",QJsonDocument(census).toJson());records.append(QJsonObject{{"factory_census",census}});return records;
}
}
int main(int argc,char **argv) {
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaVaryWarnings");
 try {
  require(argc==3,"Supply startup and Gui/Contract");Rig rig(QFileInfo(argv[1]).absoluteFilePath());const QString mode=argv[2];
  auto *messages=rig.window.findChild<QPlainTextEdit*>("messageWindow");require(messages,"Warning display unavailable");QJsonArray records;
  if(mode=="Contract")records=contract(rig,messages);
  else {
   require(mode=="Gui","Unknown Vary warning mode");Options settings;settings.initial=1;
   for(const auto &kind:QStringList{"Explicit","Omitted","DefaultExplicit","DCSupported","YukonUpper","YukonLower"}) {
    const bool dc=kind=="DCSupported",yukon=kind.startsWith("Yukon"),supported=dc||yukon;
    const QString solverType=dc?"DifferentialCorrector":yukon?"Yukon":"VF13ad";
    settings.initial=kind=="YukonLower"?5:1;
    const double optimum=kind=="YukonLower"?4:2;
    QString script=rig.script(settings);
    if(kind=="Omitted")script.replace("Lower = -100, Upper = 100, ","");
    if(kind=="DefaultExplicit")script.replace("Lower = -100, Upper = 100", "Lower = -9.999999e300, Upper = 9.999999e300");
    if(yukon){script.replace("NonlinearConstraint Opt(X <= 2);\n", "");script.replace("Lower = -100, Upper = 100",kind=="YukonLower"?"Lower = 4, Upper = 100":"Lower = -100, Upper = 2");script.replace("Create VF13ad Opt;","Create Yukon Opt;");script.replace("Opt.Tolerance =","Opt.OptimalityTolerance =");script.replace("BeginMissionSequence;","Opt.FunctionTolerance = 1e-10;\nBeginMissionSequence;");}
    if(dc)script="Create DifferentialCorrector Opt;\n"+rig.seed()+"Opt.MaximumIterations = 30;\nOpt.ReportFile = '"+rig.solverReport+"';\nBeginMissionSequence;\nTarget Opt {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\n"+vary(settings)+"\nCost = (X - 3)^2;\nReport Trials X Cost Keep;\nAchieve Opt(X = 2, {Tolerance = 1e-10});\nEndTarget;\nReport Final X Cost Keep;\n";
    rig.source->setPlainText(script);require(rig.window.buildScript(),"Vary warning fixture build failed");rig.save(kind+".script",script.toUtf8());
    auto *native=dynamic_cast<Solver*>(Moderator::Instance()->GetConfiguredObject("Opt"));require(native,"VF13ad solver missing");
    QJsonArray advertised;for(int i=0;i<fields.size();++i){const bool allow=native->GetBooleanParameter(capabilities[i].toStdString());require(allow==(supported||(i!=1&&i!=2)),"Solver capability contract changed");advertised.append(allow);}
    auto *item=static_cast<QTreeWidgetItem*>(nullptr);QTreeWidgetItemIterator it(rig.mission);while(*it){if((*it)->text(0).contains("VF control")){item=*it;break;}++it;}require(item,"Vary mission node missing");rig.mission->itemDoubleClicked(item,0);events();
    auto *child=rig.area->activeSubWindow();require(child,"Vary editor absent");acceptanceView(*child);
    if(kind!="Omitted")for(int i=0;i<fields.size();++i){auto *input=child->findChild<QLineEdit*>("commandField_"+fields[i]);require(input&&input->isEnabled()==advertised[i].toBool(),"Vary capability control state differs");if(!input->isEnabled())require(input->toolTip().contains("does not use"),"Unsupported Vary control lost explanation");}
    rig.closePanels();const auto saved=rig.files.filePath(kind+" warning Ω.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==script,"Vary warning Unicode source changed");
    for(const auto &phase:QStringList{"first","repeat"}) {
     const auto before=messages->toPlainText();QJsonObject result;
     if(!supported)result=rig.run(kind+"-"+phase,settings);
     else {
      const int undo=rig.source->document()->availableUndoSteps();require(rig.window.runMission()==MainWindow::RunResult::Completed,"Supported solver witness failed");events();
      rig.save(kind+"-"+phase+".final.csv",read(rig.final));rig.save(kind+"-"+phase+".trials.csv",read(rig.trials));rig.save(kind+"-"+phase+".optimizer.txt",read(rig.solverReport));rig.save(kind+"-"+phase+"-messages.txt",messages->toPlainText().mid(before.size()).toUtf8());
      const auto final=rows(read(rig.final)),trials=rows(read(rig.trials));require(final.size()==1&&trials.size()>=2&&std::abs(final[0][0]-optimum)<1e-6&&std::abs(final[0][1]-1)<2e-6&&std::abs(trials[0][0]-settings.initial)<1e-8,"Supported solver differs from independent scalar solution");
      auto *command=Moderator::Instance()->GetFirstCommand();while(command&&!command->IsOfType(dc?"Target":"Optimize"))command=command->GetNext();auto *branch=dynamic_cast<SolverBranchCommand*>(command);auto *solver=branch?dynamic_cast<Solver*>(branch->GetClone(0)):nullptr;
      require(solver&&solver->GetTypeName()==solverType.toStdString()&&solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"))==Gmat::CONVERGED,"Supported native solver did not converge");
      require(rig.source->toPlainText()==script&&rig.source->document()->availableUndoSteps()==undo,"Supported Run changed source/history");
      rig.save(kind+"-"+phase+".final.csv",read(rig.final));rig.save(kind+"-"+phase+".trials.csv",read(rig.trials));result={{"type",solverType},{"final",json(final)},{"trials",json(trials)},{"native_status",Gmat::CONVERGED}};
     }
     const auto expected=kind=="Omitted"||supported?QStringList{}:QStringList{"Lower","Upper"};
     rig.save(kind+"-"+phase+"-messages.txt",messages->toPlainText().mid(before.size()).toUtf8());
     records.append(QJsonObject{{"case",kind},{"phase",phase},{"warnings",warnings(messages,before,expected,solverType)},{"capabilities",advertised},{"result",result}});
    }
   }
  }
  const QJsonObject proof{{"outcome","passed"},{"mode",mode},{"records",records}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
