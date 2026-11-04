#include "FormationAcceptance.hpp"
#include "ReportViewer.hpp"
#include "MatlabInterface.hpp"
#include "Array.hpp"
#include <QRegularExpression>
#include <QSet>
#include "MatlabAcceptanceStartup.hpp"
using namespace formaccept;

static QString quoted(QString text) {return "'"+text.replace("'","''")+"'";}
static QString declaration(const QString &kind,const QString &name) {
 if(kind=="Array")return "Create Array "+name+"[2,3];\n";
 return "Create "+QString(kind=="Object"?"ChemicalTank":kind=="String"?"String":"Variable")+" "+name+";\n";
}
static QJsonValue input(const QString &kind,const QString &mode,int index) {
 if(kind=="Array"){QJsonArray a;for(int i=1;i<=6;++i)a.append(i+index*10);return a;}
 if(kind=="String")return (mode=="QuotedString"?QStringList{"pilot''s Ω, log","local''s α","other''s β","nested''s γ"}:QStringList{"alpha, beta","private first","private second","nested value"})[index];
 if(kind=="Object")return QJsonValue(std::array<double,4>{12,30,45,8}[index]);
 return QJsonValue(std::array<double,4>{2.5,-3,8,4}[index]);
}
static QString seed(const QString &kind,const QString &name,const QJsonValue &value) {
 if(kind=="Array"){QString s;const auto a=value.toArray();for(int i=0;i<6;++i)s+=name+QString("(%1,%2) = ").arg(i/3+1).arg(i%3+1)+number(a[i].toDouble())+";\n";return s;}
 if(kind=="String")return name+" = '"+value.toString()+"';\n";
 return name+(kind=="Object"?".FuelMass":"")+" = "+number(value.toDouble())+";\n";
}
static QJsonValue truth(const QString &kind,const QString &mode,const QJsonValue &value) {
 if(mode=="NoInput")return 17;
 if(kind=="Array"){const auto a=value.toArray();return QJsonArray{a[5].toDouble()+1,a[1].toDouble()-2,a[3].toDouble()*2,a[0].toDouble()+3,-a[4].toDouble(),a[2].toDouble()+4};}
 if(kind=="String")return value.toString()+"|done";
 return value.toDouble()*(kind=="Object"?1:3)+(kind=="Object"?3:1);
}
static QStringList terms(const QString &kind,const QString &name) {
 if(kind=="Array"){QStringList result;for(int i=0;i<6;++i)result<<name+QString("(%1,%2)").arg(i/3+1).arg(i%3+1);return result;}
 return {name+(kind=="Object"?".FuelMass":"")};
}
static QStringList fields(const QJsonValue &value) {
 if(value.isString())return {value.toString()};
 if(value.isArray()){QStringList a;for(auto v:value.toArray())a<<number(v.toDouble());return a;}
 return {number(value.toDouble())};
}
static QJsonValue observed(GmatBase *object,const QString &kind) {
 require(object,"MATLAB I/O native object missing");
 require(object->GetTypeName()==(kind=="Array"?"Array":kind=="String"?"String":kind=="Object"?"ChemicalTank":"Variable"),"MATLAB I/O native object type differs");
 if(kind=="String")return QString::fromStdString(object->GetStringParameter("Value"));
 if(kind=="Array"){auto *a=dynamic_cast<Array*>(object);require(a&&a->GetRowCount()==2&&a->GetColCount()==3,"MATLAB I/O array dimensions differ");const auto &m=a->GetRmatrixParameter("RmatValue");QJsonArray result;for(int r=0;r<2;++r)for(int c=0;c<3;++c){require(std::isfinite(m(r,c)),"MATLAB I/O nonfinite matrix element");result.append(m(r,c));}return result;}
 const auto v=object->GetRealParameter(kind=="Object"?"FuelMass":"Value");require(std::isfinite(v),"MATLAB I/O nonfinite scalar");return v;
}

int main(int argc,char **argv) {
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaMatlabFunctionIo");
 try {
  require(argc==4,"Supply startup, MATLAB plugin and I/O case");const QString mode=argv[3];
  require(QStringList{"Scalar","ScalarGlobal","Array","String","QuotedString","NumericProperty","StringProperty","Object","NoInput","NoOutput","Literals","InOut","LiteralRecovery","LiteralClose","UndeclaredOutput","WrongAliasType"}.contains(mode),"Unknown MATLAB I/O case");
  const QString kind=mode=="Array"?"Array":QStringList{"String","QuotedString","StringProperty"}.contains(mode)?"String":mode=="Object"?"Object":"Scalar";
  QTemporaryDir runtime;require(runtime.isValid(),"MATLAB I/O runtime directory missing");const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),runtime.path(),QFileInfo(argv[2]).absoluteFilePath());
  const auto validator=QFileInfo(QFileInfo(argv[1]).dir().filePath("GmatQt")).canonicalFilePath();require(QFileInfo(validator).isExecutable(),"Actual GMAT function validator missing");FileManager::Instance()->SetBinDirectory("GmatQt",validator.toStdString());
  require(QDir(QString::fromStdString(FileManager::Instance()->GetBinDirectory("GmatQt"))).canonicalPath()==QFileInfo(validator).dir().canonicalPath(),"MATLAB I/O validator bin identity differs");
  Rig rig(startup);rig.save("validator-path.txt",validator.toUtf8());
  const auto directory=rig.files.filePath("MATLAB scope Ω");require(QDir().mkpath(directory),"MATLAB I/O fixture directory missing");
  const auto matlab=directory+"/MatIo.m",function=directory+"/Exercise.gmf",nested=directory+"/NestedExercise.gmf",sideEffects=directory+"/no-output.txt";
  QString body;
  if(mode=="NoInput")body="function out = MatIo()\nout = 17;\nend\n";
  else if(mode=="NoOutput")body="function MatIo(x)\nf = fopen("+quoted(sideEffects)+", 'a');\nassert(f >= 0);\nfprintf(f, '%.17g\\n', x);\nfclose(f);\nend\n";
  else if(kind=="Array")body="function out = MatIo(x)\nout = [x(2,3)+1, x(1,2)-2, x(2,1)*2; x(1,1)+3, -x(2,2), x(1,3)+4];\nx(1,1) = 999;\nend\n";
  else if(kind=="String")body="function out = MatIo(x)\nout = [x, '|done'];\nx = 'MATLAB private mutation';\nend\n";
  else if(kind=="Object")body="function out = MatIo(x)\nout = x;\nout.FuelMass = x.FuelMass + 3;\nx.FuelMass = 999;\nend\n";
  else body="function out = MatIo(x)\nout = 3*x + 1;\nx = 999;\nend\n";
  write(matlab,body.toUtf8());rig.save("MatIo.m",body.toUtf8());rig.save("startup.txt",read(startup));
  const auto matlabDeclaration="Create MatlabFunction MatIo;\nMatIo.FunctionPath = "+quoted(matlab)+";\n";
  QString functionBody="function [out] = Exercise("+QString(mode=="NoInput"?"":"input")+")\n"+declaration(kind,"out")+(mode=="ScalarGlobal"?"Global MatIo;\n":matlabDeclaration)+"Global FunctionCalls;\nBeginMissionSequence;\nFunctionCalls = FunctionCalls + 1;\n";
  if(mode=="NoOutput")functionBody+="MatIo(input);\nout = input + 1;\n";
  else if(mode=="Literals"||mode=="LiteralRecovery"||mode=="LiteralClose")functionBody+="[out] = MatIo(-3.5);\n";
  else if(mode=="InOut"||mode=="WrongAliasType")functionBody+="[input] = MatIo(input);\nout = input;\n";
  else functionBody+="[out] = MatIo("+QString(mode=="NoInput"?"":"input")+");\n";
  if(mode=="UndeclaredOutput")functionBody.remove("Create Variable out;\n");
  if(mode=="WrongAliasType")functionBody.replace("out = input;","out = 0;");
  const QString nestedBody="function [out] = NestedExercise("+QString(mode=="NoInput"?"":"input")+")\n"+declaration(kind,"out")+"Create GmatFunction Exercise;\nExercise.FunctionPath = "+quoted(function)+";\nGlobal FunctionCalls;\nBeginMissionSequence;\n[out] = Exercise("+QString(mode=="NoInput"?"":"input")+");\n";
  write(function,functionBody.toUtf8());write(nested,nestedBody.toUtf8());rig.save("Exercise.gmf",functionBody.toUtf8());rig.save("NestedExercise.gmf",nestedBody.toUtf8());
  const QStringList inputs{"Input","FirstInput","SecondInput","ThirdInput"},outputs{"Main","First","Second","Third"};
  QString script=matlabDeclaration+"Create GmatFunction Exercise NestedExercise;\nExercise.FunctionPath = "+quoted(function)+";\nNestedExercise.FunctionPath = "+quoted(nested)+";\nCreate Variable FunctionCalls Keep;\nFunctionCalls = 0;\nKeep = 41;\n";
  for(int i=0;i<4;++i)script+=declaration(kind,inputs[i])+seed(kind,inputs[i],input(kind,mode,i))+declaration(kind,outputs[i])+seed(kind,outputs[i],input(kind,mode,i));
  if(mode=="NoOutput")script+="Main = 41;\n";
  if(mode=="WrongAliasType")script+="Create ChemicalTank Payload;\nPayload.FuelMass = 12;\n";
  if(mode=="NumericProperty"||mode=="StringProperty") {
   script+="Create Spacecraft Carrier;\nCarrier.DisplayStateType = Cartesian;\nCarrier.X = 1000;\nCarrier.DateFormat = UTCGregorian;\nCarrier.Epoch = '01 Jan 2000 12:00:00.000';\n";
  }
  script+="Create ReportFile Values;\nValues.Filename = "+quoted(rig.report)+";\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nBeginMissionSequence;\nGlobal FunctionCalls;\n";
  if(mode=="ScalarGlobal")script+="Global MatIo;\n";
  if(mode=="WrongAliasType")script+="[Main] = Exercise(Payload);\n";
  else if(mode=="NoOutput")script+="MatIo(Input);\n";
  else script+="["+QString(mode=="InOut"?"Input":"Main")+"] = MatIo("+(mode=="NoInput"?QString():(mode=="Literals"||mode=="LiteralRecovery"||mode=="LiteralClose")?QString("-3.5"):mode=="NumericProperty"?QString("Carrier.X"):mode=="StringProperty"?QString("Carrier.UTCGregorian"):QString("Input"))+");\n";
  for(int i=1;i<4;++i)script+="["+outputs[i]+"] = "+QString(i==3?"NestedExercise":"Exercise")+"("+QString(mode=="NoInput"?"":inputs[i])+");\n";
  QStringList columns;for(const auto &name:inputs)columns+=terms(kind,name);for(const auto &name:outputs)columns+=terms(kind,name);columns<<"Keep"<<"FunctionCalls";
  script+="Report Values "+columns.join(' ')+";\nKeep = Keep + 1;\nReport Values Keep FunctionCalls;\n";
  rig.source->setPlainText(script);rig.save("caller.script",script.toUtf8());const auto built=rig.window.buildScript();events();rig.save("build-messages.txt",rig.messages->toPlainText().toUtf8());if(!built)std::cerr<<rig.messages->toPlainText().toStdString();
  if(mode=="UndeclaredOutput") {
   const auto diagnostic=rig.messages->toPlainText();require(!built&&diagnostic.contains("Missing output declaration of \"out\""),"Undeclared MATLAB function output was not rejected");
   require(rig.source->toPlainText()==script,"Undeclared output rejection changed source");
   const QJsonObject proof{{"checkpoint","matlab-function-undeclared-output"},{"case",mode},{"outcome","rejection-checked"},{"expected_rejection",true},{"new_requirement_credit",0}};
   rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
  }
  require(built,"MATLAB function I/O caller Build failed");
  if(mode=="WrongAliasType") {
   const auto rejected=rig.window.runMission();events();const auto diagnostic=rig.messages->toPlainText();rig.save("rejected-messages.txt",diagnostic.toUtf8());
   require(rejected==MainWindow::RunResult::Failed&&diagnostic.contains("unsupported MATLAB output type")&&diagnostic.contains("ChemicalTank"),"Actual unsupported MATLAB formal output type was not rejected");
   auto *payload=Moderator::Instance()->GetInternalObject("Payload");require(payload&&payload->GetTypeName()=="ChemicalTank"&&payload->GetRealParameter("FuelMass")==12,"MATLAB type rejection changed caller payload");
   require(Moderator::Instance()->GetInternalObject("Main")->GetRealParameter("Value")==2.5&&Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==41&&rig.source->toPlainText()==script,"MATLAB type rejection changed caller output/sentinel/source");
   const QJsonObject proof{{"checkpoint","matlab-function-output-type-rejection"},{"case",mode},{"outcome","rejection-checked"},{"actual_type","ChemicalTank"},{"caller_mass",12},{"caller_output",2.5},{"keep",41},{"new_requirement_credit",0}};
   rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
  }
  const auto saved=rig.files.filePath("MATLAB I-O caller Ω.script");require(rig.window.saveScriptTo(saved),"MATLAB I/O caller Save failed");
  QJsonObject recovery;
  if(mode=="LiteralRecovery"||mode=="LiteralClose") {
   const auto invalidScript=script;const auto invalidBody=functionBody;const auto undo=rig.source->document()->availableUndoSteps();
   const auto rejected=rig.window.runMission();events();const auto diagnostic=rig.messages->toPlainText();rig.save("rejected-messages.txt",diagnostic.toUtf8());rig.save("rejected-caller.script",invalidScript.toUtf8());rig.save("rejected-Exercise.gmf",invalidBody.toUtf8());
   require(rejected==MainWindow::RunResult::Failed&&diagnostic.contains("FunctionRunner::ValidateInputs() function is NULL"),"MATLAB literal rejection/diagnostic differs");
   require(rig.source->toPlainText()==invalidScript&&rig.source->document()->availableUndoSteps()==undo,"MATLAB literal rejection changed source/history");
   if(mode=="LiteralClose") {
    const QJsonObject proof{{"checkpoint","matlab-function-literal-close"},{"case",mode},{"outcome","rejection-checked"},{"expected_rejection",true},{"diagnostic","FunctionRunner::ValidateInputs() function is NULL"},{"new_requirement_credit",0},{"close_pending",true}};
    rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }
   script.replace("[Main] = MatIo(-3.5);","[Main] = MatIo(Input);");functionBody.replace("[out] = MatIo(-3.5);","[out] = MatIo(input);");write(function,functionBody.toUtf8());rig.source->setPlainText(script);
   const auto rebuilt=rig.window.buildScript();events();rig.save("recovery-build-messages.txt",rig.messages->toPlainText().toUtf8());require(rebuilt,"MATLAB literal rejection did not recover on Build");require(rig.window.saveScriptTo(saved),"MATLAB recovered caller Save failed");rig.save("recovered-caller.script",script.toUtf8());rig.save("recovered-Exercise.gmf",functionBody.toUtf8());
   recovery=QJsonObject{{"expected_rejection",true},{"diagnostic","FunctionRunner::ValidateInputs() function is NULL"},{"rebuilt",true},{"initialization_failure_cleanup",true}};
  }
  QJsonArray phases;
  for(const auto &phase:QStringList{"initial","no-build-repeat","reopened"}) {
   if(phase=="reopened")require(rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==script,"MATLAB I/O Unicode reopen differs");
   if(mode=="NoOutput")write(sideEffects,{});
   const auto beforeUndo=rig.source->document()->availableUndoSteps();acceptanceView(rig.window);const auto status=rig.window.runMission();events();rig.save(phase+"-messages.txt",rig.messages->toPlainText().toUtf8());if(status!=MainWindow::RunResult::Completed)std::cerr<<rig.messages->toPlainText().toStdString();require(status==MainWindow::RunResult::Completed,"Actual MATLAB function I/O mission failed");
   QJsonArray nativeInputs,nativeOutputs;QStringList expectedColumns;
   for(int i=0;i<4;++i){auto expected=input(kind,mode,i);if(mode=="InOut"&&i==0)expected=truth(kind,mode,expected);const auto actual=observed(Moderator::Instance()->GetInternalObject(inputs[i].toStdString()),kind);nativeInputs.append(actual);expectedColumns+=fields(expected);rig.save(phase+"-inputs.json",QJsonDocument(nativeInputs).toJson());require(actual==expected,"MATLAB function changed non-output caller input");}
   for(int i=0;i<4;++i){auto expected=truth(kind,mode,input(kind,mode,i));if(mode=="NoOutput")expected=(i==0?41:input(kind,mode,i).toDouble()+1);if(mode=="Literals")expected=-9.5;if(mode=="NumericProperty"&&i==0)expected=3001;if(mode=="StringProperty"&&i==0)expected="01 Jan 2000 12:00:00.000|done";if(mode=="InOut"&&i==0)expected=input(kind,mode,i);const auto actual=observed(Moderator::Instance()->GetInternalObject(outputs[i].toStdString()),kind);nativeOutputs.append(actual);expectedColumns+=fields(expected);rig.save(phase+"-outputs.json",QJsonDocument(nativeOutputs).toJson());if(actual!=expected)std::cerr<<"IO_DIFF "<<outputs[i].toStdString()<<" actual="<<QJsonDocument(QJsonArray{actual}).toJson().constData()<<" expected="<<QJsonDocument(QJsonArray{expected}).toJson().constData();require(actual==expected,"MATLAB result differs from independent typed/context oracle");}
   require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42&&Moderator::Instance()->GetInternalObject("FunctionCalls")->GetRealParameter("Value")==3,"MATLAB I/O continuation/function call counter differs");
   expectedColumns<<"41"<<"3";const auto raw=read(rig.report);rig.save(phase+"-report.csv",raw);const auto expected=(expectedColumns.join(',')+"\n42,3\n").toUtf8();if(raw!=expected)std::cerr<<"REPORT_DIFF actual="<<raw.constData()<<" expected="<<expected.constData();require(raw==expected,"MATLAB raw report differs from independent typed/context oracle");
   if(mode=="NoOutput"){const auto bytes=read(sideEffects);rig.save(phase+"-no-output.txt",bytes);require(bytes=="2.5\n-3\n8\n4\n","MATLAB zero-output side effects/call order differ");}
   auto *output=rig.window.findChild<QTreeWidget*>("Output");require(output,"MATLAB I/O Output Tree absent");const auto rows=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(rows.size()==1,"MATLAB I/O actual report absent");output->itemDoubleClicked(rows.first(),0);events();QMdiSubWindow *viewer=nullptr;for(auto *candidate:rig.area->subWindowList())if(dynamic_cast<ReportViewer*>(candidate->widget())){require(!viewer,"MATLAB I/O ambiguous report viewer");viewer=candidate;}require(viewer,"MATLAB I/O report viewer absent");auto *text=viewer->findChild<QPlainTextEdit*>();require(text&&text->isReadOnly()&&text->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"MATLAB I/O viewer differs from raw bytes");acceptanceView(*viewer);require(viewer->close(),"MATLAB I/O viewer did not close");events();
   require(rig.source->toPlainText()==script&&rig.source->document()->availableUndoSteps()==beforeUndo&&read(saved)==script.toUtf8()&&read(matlab)==body.toUtf8()&&read(function)==functionBody.toUtf8()&&read(nested)==nestedBody.toUtf8(),"MATLAB function I/O changed caller/history/external files");
   phases.append(QJsonObject{{"phase",phase},{"inputs",nativeInputs},{"outputs",nativeOutputs},{"report",QString::fromUtf8(raw)},{"function_calls",3},{"matlab_calls",4},{"viewer_matches",true}});rig.save("phase-results.json",QJsonDocument(phases).toJson());
  }
  const QJsonObject proof{{"checkpoint","matlab-function-io-"+mode},{"case",mode},{"kind",kind},{"outcome","passed"},{"recovery",recovery},{"phases",phases},{"new_requirement_credit",0},{"original_candidate_keys",QJsonArray{"FRR!E478","All Others!E241","FRC!E160"}},{"limits","Typed main/private/nested call constituents only. Original I/O reference ambiguity, whole function-resource universe, process modes, optimizer and reverse interface remain open."}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
