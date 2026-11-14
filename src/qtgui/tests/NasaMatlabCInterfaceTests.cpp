#include "FormationAcceptance.hpp"
#include "MatlabAcceptanceStartup.hpp"
#include "MatlabInterface.hpp"
#include "ReportViewer.hpp"
#include "TimeParameters.hpp"
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QSet>
using namespace formaccept;
static QString quote(QString value){return "'"+value.replace("'","''")+"'";}

int main(int argc,char **argv)
{
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
 app.setOrganizationName("GMATTests");app.setApplicationName("NasaMatlabCInterface");
 try {
  require(argc==6,"Supply startup,MATLAB plugin,C-interface library,helpers and generated wrapper directory");
  QTemporaryDir runtime;require(runtime.isValid(),"C-interface private runtime missing");
  const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),runtime.path(),QFileInfo(argv[2]).absoluteFilePath());
  Rig rig(startup);rig.save("startup.txt",read(startup));
  const auto library=QFileInfo(argv[3]).canonicalFilePath(),helpers=QFileInfo(argv[4]).canonicalFilePath(),wrapper=QFileInfo(argv[5]).canonicalFilePath();
  require(QFileInfo(library).isFile()&&QFileInfo(helpers+"/startgmat.m").isFile()&&QFileInfo(wrapper+"/interfacewrapper.m").isFile(),"Actual C-interface prerequisites missing");
  const auto isolated=rig.files.filePath("C interface runtime Ω");require(QDir().mkpath(isolated),"C-interface MATLAB runtime directory failed");
  const auto capiStartup=isolated+"/gmat_startup_file.txt";
  QString capiText=QString::fromUtf8(read(startup));
  capiText.replace(QRegularExpression("^[ \\t]*PLUGIN[ \\t]*=[^\\n]*(?:MatlabInterface|FminconOptimizer)[^\\n]*$",QRegularExpression::MultilineOption),"# The reverse C-interface instance does not recursively open MATLAB");
  capiText.replace(QRegularExpression("^MATLAB_MODE[ \\t]*=[^\\n]*$",QRegularExpression::MultilineOption),"MATLAB_MODE = NO_MATLAB");
  capiText.replace(runtime.path()+"/output/",isolated+"/output/");require(QDir().mkpath(isolated+"/output"),"C-interface output directory failed");
  write(capiStartup,capiText.toUtf8());rig.save("capi-startup.txt",capiText.toUtf8());
  const auto caller=rig.files.filePath("MATLAB to GMAT caller Ω.script");
  constexpr double mu=398600.4415,radius=7000,seconds=60;
  const double speed=std::sqrt(mu/radius),n=std::sqrt(mu/(radius*radius*radius));
  const QString script="% Reverse MATLAB C-interface witness Ω € 𐐨\nCreate Variable Keep;\nKeep = 41;\n"
   "Create Spacecraft Probe;\nProbe.DateFormat = A1ModJulian;\nProbe.Epoch = '21545';\nProbe.CoordinateSystem = EarthMJ2000Eq;\nProbe.DisplayStateType = Cartesian;\n"
   "Probe.X = 7000;\nProbe.Y = 0;\nProbe.Z = 0;\nProbe.VX = 0;\nProbe.VY = "+number(speed)+";\nProbe.VZ = 0;\n"
   "Create ForceModel EarthForce;\nEarthForce.CentralBody = Earth;\nEarthForce.PrimaryBodies = {};\nEarthForce.PointMasses = {Earth};\nEarthForce.Drag = None;\nEarthForce.SRP = Off;\n"
   "Create Propagator Integrator;\nIntegrator.FM = EarthForce;\nIntegrator.Type = RungeKutta89;\nIntegrator.InitialStepSize = 10;\nIntegrator.Accuracy = 1e-12;\n"
   "Create ReportFile Values;\nValues.Filename = "+quote(rig.report)+";\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\n"
   "BeginMissionSequence;\nKeep = Keep + 1;\nPropagate Integrator(Probe) {Probe.ElapsedSecs = 60, StopTolerance = 1e-10};\nReport Values Keep Probe.X Probe.Y Probe.Z Probe.VX Probe.VY Probe.VZ Probe.A1ModJulian;\n% Preserve reverse caller tail Ω\n";
  rig.source->setPlainText(script);require(rig.window.buildScript()&&rig.window.saveScriptTo(caller),"Reverse caller Build/Unicode Save failed");rig.save("caller.script",script.toUtf8());
  auto *prototype=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("Probe"));require(prototype,"Precision control spacecraft absent");
  Spacecraft precisionClock(*prototype);precisionClock.SetName("PrecisionClock");precisionClock.SetPrecisionTimeFlag(true);
  GmatTime precisionAnchor(21545.0);precisionAnchor.AddSeconds(0.123456789);precisionClock.SetEpochGT(precisionAnchor);
  ElapsedSecs preciseSeconds("PrecisionClock.ElapsedSecs",&precisionClock);ElapsedDays preciseDays("PrecisionClock.ElapsedDays",&precisionClock);
  require(preciseSeconds.Initialize()&&preciseDays.Initialize()&&preciseSeconds.EvaluateReal()==0&&preciseDays.EvaluateReal()==0,"Precision controls did not initialize to zero elapsed time");
  QJsonArray precisionRecords;
  for(const double offset:{1e-8,1e-6,0.125,-0.25,60.0}) {auto epoch=precisionAnchor;epoch.AddSeconds(offset);precisionClock.SetEpochGT(epoch);
   const double sec=preciseSeconds.EvaluateReal(),day=preciseDays.EvaluateReal();precisionRecords.append(QJsonObject{{"offset_seconds",offset},{"ElapsedSecs",sec},{"ElapsedDays",day}});rig.save("precision-controls.json",QJsonDocument(precisionRecords).toJson());
   require(std::abs(sec-offset)<1e-12&&std::abs(day-offset/86400)<1e-17,"Elapsed time lost the high-precision owner/reference epoch");
  }
  ElapsedSecs copiedSeconds(preciseSeconds);ElapsedDays assignedDays;assignedDays=preciseDays;
  require(std::abs(copiedSeconds.EvaluateReal()-60)<1e-12&&std::abs(assignedDays.EvaluateReal()-60.0/86400)<1e-17,"Elapsed time copy/assignment lost its precise reference");
  preciseSeconds.SetIsGlobal(true);require(preciseSeconds.Initialize()&&std::abs(preciseSeconds.EvaluateReal()-60)<1e-12,"Global elapsed initialization reset its precise reference");
  require(preciseDays.Initialize()&&preciseDays.EvaluateReal()==0,"Private elapsed initialization did not reset its reference");
  preciseSeconds.SetInitialEpoch(21545.0);auto explicitEpoch=GmatTime(21545.0);explicitEpoch.AddSeconds(0.125);precisionClock.SetEpochGT(explicitEpoch);
  require(std::abs(preciseSeconds.EvaluateReal()-0.125)<1e-12,"Explicit Real initial epoch was not preserved exactly");
  preciseSeconds.ClearIsInitialEpochSet();require(preciseSeconds.Initialize()&&preciseSeconds.EvaluateReal()==0,"Cleared elapsed reference did not recover");
  precisionClock.SetPrecisionTimeFlag(false);precisionClock.SetEpochGT(precisionAnchor);
  ElapsedSecs switchingSeconds("PrecisionClock.ElapsedSecs",&precisionClock);
  require(switchingSeconds.Initialize()&&switchingSeconds.EvaluateReal()==0,"Nonprecision reference did not start at zero");
  precisionClock.SetPrecisionTimeFlag(true);const double switched=switchingSeconds.EvaluateReal();
  QJsonObject modeSwitch{{"before_enable",0},{"after_enable",switched},{"Real_epoch",precisionClock.GetEpoch()},{"GmatTime_epoch",QString::fromStdString(precisionClock.GetEpochGT().ToString())}};
  rig.save("precision-mode-switch.json",QJsonDocument(modeSwitch).toJson());require(std::abs(switched)<1e-12,"Enabling precision time changed the elapsed reference");
  const int history=rig.source->document()->availableUndoSteps();
  const auto malformed=isolated+"/malformed.script",runtimeError=isolated+"/runtime-error.script";
  const QByteArray bad="Create Variable Keep;\nKeep.NonexistentProperty = 12;\nBeginMissionSequence;\n",badRun="Create Variable Keep;\nBeginMissionSequence;\nKeep = sqrt(-1);\n";
  write(malformed,bad);write(runtimeError,badRun);rig.save("malformed.script",bad);rig.save("runtime-error.script",badRun);
  auto *matlab=MatlabInterface::Instance();require(matlab,"Real MATLAB interface absent");
  auto evaluate=[&](const std::string &code) {
   rig.save("last-evaluation.m",QByteArray::fromStdString(code));
   try {matlab->RunMatlabString(code);} catch(BaseException &) {
    try {matlab->RunMatlabString("gmat_capi_last_failure='C-interface not loaded'; if libisloaded('libCInterface'); gmat_capi_last_failure=calllib('libCInterface','getLastMessage'); end;");
     std::string detail;if(matlab->GetString("gmat_capi_last_failure",detail)==1){rig.save("C-interface-last-failure.txt",QByteArray::fromStdString(detail));std::cerr<<"C_API_DETAIL "<<detail<<std::endl;}
    }catch(...){}throw;
   }
  };
  const auto bootstrap="gmat_capi_old_directory = pwd; cd("+quote(isolated)+"); addpath("+quote(helpers)+"); addpath("+quote(wrapper)+"); addpath("+quote(QFileInfo(library).absolutePath())+");\n"
   "assert(strcmp(which('startgmat'),fullfile("+quote(helpers)+",'startgmat.m')));\n"
   "assert(strcmp(which('opengmat'),fullfile("+quote(helpers)+",'opengmat.m')));\n"
   "assert(strcmp(which('preparescript'),fullfile("+quote(helpers)+",'preparescript.m')));\n"
   "assert(strcmp(which('closegmat'),fullfile("+quote(helpers)+",'closegmat.m')));\n"
   "assert(~libisloaded('libCInterface')); gmat_capi_pid = feature('getpid');\n";
  rig.save("bootstrap.m",bootstrap.toUtf8());acceptanceView(rig.window);evaluate(bootstrap.toStdString());
  QJsonArray phases;QByteArray firstReport;QJsonObject errorProof;
  auto inspect=[&](const QString &phase) {
   const auto raw=read(rig.report);rig.save(phase+"-report.csv",raw);
   const auto cells=QString::fromUtf8(raw).trimmed().split(',');require(cells.size()==8,"Reverse C-interface raw report shape differs");
   const std::vector<double> expected{42,radius*std::cos(n*seconds),radius*std::sin(n*seconds),0,-speed*std::sin(n*seconds),speed*std::cos(n*seconds),0,21545+seconds/86400};
   QJsonArray values;for(int i=0;i<8;++i){bool ok=false;const auto value=cells[i].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-expected[i])<(i==7?2e-10:2e-7),"Reverse C-interface result differs from independent circular-orbit/control oracle");values.append(value);}
   require(rig.source->toPlainText()==script&&rig.source->document()->availableUndoSteps()==history&&read(caller)==script.toUtf8()&&Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value")==41,"MATLAB reverse execution mutated the GUI caller/control/history/disk");
   evaluate("gmat_capi_summary = calllib('libCInterface','GetRunSummary'); assert(contains(gmat_capi_summary,'Propagate') && contains(gmat_capi_summary,'Report')); gmat_capi_count = calllib('libCInterface','CountObjects'); assert(gmat_capi_count >= 5);");
   std::string summary;require(matlab->GetString("gmat_capi_summary",summary)==1,"Actual C-interface command summary absent");rig.save(phase+"-summary.txt",QByteArray::fromStdString(summary));
   phases.append(QJsonObject{{"phase",phase},{"actual_report",values},{"GUI_source_history_and_control_unchanged",true},{"actual_command_summary",true}});rig.save("phase-results.json",QJsonDocument(phases).toJson());
   if(firstReport.isEmpty())firstReport=raw;else require(raw==firstReport,"Reverse repeat/recovery/reopen changed deterministic report bytes");
  };
  const auto first="gmat_capi_ode = startgmat("+quote(caller)+",'EarthForce'); assert(gmat_capi_ode >= 0 && libisloaded('libCInterface')); assert(calllib('libCInterface','GetStateSize') == 6);";
  rig.save("first.m",first.toUtf8());require(matlab->SetOutputBuffer(65536)>0,"MATLAB diagnostic output buffer missing");evaluate(first.toStdString());
  if(const auto *buffer=matlab->GetOutputBuffer())rig.save("first-MATLAB-output.txt",QByteArray(buffer));
  const auto capiLog=isolated+"/output/GmatLog.txt";if(QFileInfo::exists(capiLog))rig.save("initial-C-API-engine-log.txt",read(capiLog));
  const auto earlyMaps=rig.files.filePath("early-MATLAB-process-maps.txt");
  evaluate(("gmat_capi_native_pointer=calllib('libCInterface','GetState'); setdatatype(gmat_capi_native_pointer,'doublePtr',6,1); gmat_capi_native_state_json=jsonencode(gmat_capi_native_pointer.Value); gmat_capi_early_maps=fileread('/proc/self/maps'); gmat_capi_early_file=fopen("+quote(earlyMaps)+",'w'); assert(gmat_capi_early_file>=0); fwrite(gmat_capi_early_file,gmat_capi_early_maps,'char'); fclose(gmat_capi_early_file);").toStdString());
  std::string nativeState;require(matlab->GetString("gmat_capi_native_state_json",nativeState)==1,"Actual C-interface state diagnostic missing");rig.save("initial-C-API-state.json",QByteArray::fromStdString(nativeState));rig.save("early-MATLAB-process-maps.txt",read(earlyMaps));
  inspect("initial");
  evaluate(("assert(preparescript("+quote(caller)+")==0);").toStdString());inspect("repeat");
  const auto negatives="gmat_capi_errors = struct;\n"
   "try; preparescript("+quote(isolated+"/missing.script")+"); error('GMAT:TestMiss','missing accepted'); catch e; assert(~strcmp(e.identifier,'GMAT:TestMiss')); gmat_capi_errors.missing=e.message; end;\n"
   "try; preparescript("+quote(malformed)+"); error('GMAT:TestParse','invalid accepted'); catch e; assert(~strcmp(e.identifier,'GMAT:TestParse')); gmat_capi_errors.parse=e.message; end;\n"
   "try; preparescript("+quote(runtimeError)+"); error('GMAT:TestRun','runtime error accepted'); catch e; assert(~strcmp(e.identifier,'GMAT:TestRun')); gmat_capi_errors.run=e.message; end;\n"
   "gmat_capi_error_json = jsonencode(gmat_capi_errors);";
  rig.save("negative-controls.m",negatives.toUtf8());evaluate(negatives.toStdString());std::string errors;require(matlab->GetString("gmat_capi_error_json",errors)==1,"Reverse interface error evidence missing");
  errorProof=QJsonDocument::fromJson(QByteArray::fromStdString(errors)).object();rig.save("actual-errors.json",QByteArray::fromStdString(errors));
  require(errorProof.size()==3&&errorProof["missing"].toString().contains("does not exist")&&errorProof["parse"].toString().contains("failed to load")&&errorProof["run"].toString().contains("failed to run"),"Actual high-level file/parser/runtime error controls differ");
  QJsonArray encodingErrors;
  const QList<QByteArray> invalidEncodings{QByteArray::fromHex("80"),QByteArray::fromHex("c0af"),QByteArray::fromHex("e080af"),QByteArray::fromHex("eda080"),QByteArray::fromHex("f4908080"),QByteArray::fromHex("e282"),QByteArray::fromHex("e228a1"),QByteArray::fromHex("f5808080"),QByteArray::fromHex("ff")};
  for(int i=0;i<invalidEncodings.size();++i){const auto path=isolated+"/invalid-encoding-"+QString::number(i)+".script";const auto bytes=script.toUtf8()+"% Invalid encoding "+invalidEncodings[i];write(path,bytes);rig.save("invalid-encoding-"+QString::number(i)+".script",bytes);
   const auto command="gmat_capi_encoding_rejected=false; try; preparescript("+quote(path)+"); catch e; assert(contains(e.message,'failed to load')); gmat_capi_encoding_detail=calllib('libCInterface','getLastMessage'); assert(contains(gmat_capi_encoding_detail,'not valid UTF-8')); gmat_capi_encoding_rejected=true; end; assert(gmat_capi_encoding_rejected);";
   evaluate(command.toStdString());std::string detail;require(matlab->GetString("gmat_capi_encoding_detail",detail)==1,"Actual encoding rejection detail missing");rig.save("invalid-encoding-"+QString::number(i)+"-error.txt",QByteArray::fromStdString(detail));
   require(read(rig.report)==firstReport&&rig.source->toPlainText()==script&&rig.source->document()->availableUndoSteps()==history&&read(caller)==script.toUtf8()&&Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value")==41,"Rejected encoding executed or mutated GUI/caller/output");encodingErrors.append(QJsonObject{{"bytes_hex",QString::fromLatin1(invalidEncodings[i].toHex())},{"rejected",true},{"unchanged_output_source_control",true}});
  }
  rig.save("encoding-controls.json",QJsonDocument(encodingErrors).toJson());
  evaluate(("assert(preparescript("+quote(caller)+")==0);").toStdString());inspect("recovered");
  evaluate("closegmat; assert(~libisloaded('libCInterface')); assert(~isfile('gmat_startup_file.txt')); gmat_capi_unloaded=true;");
  require(!QFileInfo::exists(capiStartup),"High-level Close did not remove only its private startup copy");
  const QRegularExpression rootLine("^ROOT_PATH[ \\t]*=[^\\n]*$",QRegularExpression::MultilineOption);
  const auto rootMatch=rootLine.match(capiText);require(rootMatch.hasMatch(),"Actual C-interface ROOT_PATH missing");
  const auto absoluteRoot=rootMatch.captured().section('=',1).trimmed();
  QString invalidRoot=capiText;invalidRoot.replace(rootLine,"ROOT_PATH = "+isolated+"/missing-install");write(capiStartup,invalidRoot.toUtf8());rig.save("capi-invalid-root-startup.txt",invalidRoot.toUtf8());
  evaluate("gmat_capi_invalid_root_rejected=false; try; opengmat; catch e; assert(contains(e.message,'does not point to a valid GMAT installation')); gmat_capi_invalid_root_rejected=true; end; assert(gmat_capi_invalid_root_rejected && ~libisloaded('libCInterface'));");
  QString relativeRoot=capiText;relativeRoot.replace(rootLine,"ROOT_PATH = "+QDir(isolated).relativeFilePath(absoluteRoot));write(capiStartup,relativeRoot.toUtf8());rig.save("capi-relative-root-startup.txt",relativeRoot.toUtf8());
  evaluate(("assert(opengmat()==0); assert(preparescript("+quote(caller)+")==0); assert(feature('getpid')==gmat_capi_pid);").toStdString());inspect("reopened");
  const auto mapsFile=rig.files.filePath("MATLAB-process-maps.txt");
  evaluate(("gmat_capi_maps = fileread('/proc/self/maps'); gmat_capi_maps_file=fopen("+quote(mapsFile)+",'w'); assert(gmat_capi_maps_file>=0); fwrite(gmat_capi_maps_file,gmat_capi_maps,'char'); fclose(gmat_capi_maps_file); gmat_capi_result = jsonencode(struct('pid',gmat_capi_pid,'unloaded_then_reopened',gmat_capi_unloaded,'loaded',libisloaded('libCInterface')));").toStdString());
  const auto maps=read(mapsFile);std::string result;require(matlab->GetString("gmat_capi_result",result)==1&&!maps.isEmpty(),"Real MATLAB C-interface process/mapping evidence missing");rig.save("MATLAB-process-maps.txt",maps);rig.save("actual-MATLAB-state.json",QByteArray::fromStdString(result));
  require(QString::fromUtf8(maps).contains(library)&&QString::fromUtf8(maps).contains(wrapper+"/libCInterface_thunk_"),"MATLAB did not map the actual C-interface library/generated thunk");
  evaluate("closegmat; assert(~libisloaded('libCInterface')); cd(gmat_capi_old_directory);");
  require(read(caller)==script.toUtf8()&&read(malformed)==bad&&read(runtimeError)==badRun,"High-level execution changed caller/negative input files");
  require(rig.window.runMission()==MainWindow::RunResult::Completed&&read(rig.report)==firstReport,"Affected GUI caller did not execute the same independent result after reverse-interface use");
  auto *output=rig.window.findChild<QTreeWidget *>("Output");const auto reports=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"Affected GUI caller report missing");output->itemDoubleClicked(reports.first(),0);events();
  auto *viewer=rig.area->activeSubWindow()?dynamic_cast<ReportViewer *>(rig.area->activeSubWindow()->widget()):nullptr;auto *text=viewer?viewer->findChild<QPlainTextEdit *>():nullptr;
  require(text&&text->isReadOnly()&&text->toPlainText().trimmed()==QString::fromUtf8(firstReport).trimmed(),"Actual affected GUI ReportViewer differs");acceptanceView(*rig.area->activeSubWindow());rig.area->activeSubWindow()->close();events();
  require(rig.window.loadScript(caller)&&rig.window.buildScript()&&rig.source->toPlainText()==script&&read(caller)==script.toUtf8(),"Reverse caller Unicode reopen/source differs");
  const QJsonObject proof{{"checkpoint","actual-MATLAB-high-level-C-interface"},{"phases",phases},{"errors",errorProof},{"precision_controls",precisionRecords},{"precision_mode_switch",modeSwitch},{"precision_copy_Global_private_clear_and_explicit_reference",true},{"encoding_controls",encodingErrors},{"absolute_and_relative_ROOT_PATH",true},{"invalid_ROOT_PATH_rejected",true},{"actual_MATLAB_state",QJsonDocument::fromJson(QByteArray::fromStdString(result)).object()},{"actual_library",library},{"actual_helpers",helpers},{"actual_wrapper",wrapper},{"actual_ReportViewer",true},{"Unicode_reopen",true},{"new_requirement_credit",0}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
