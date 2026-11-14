// Supplementary execution witnesses; protected command and function workers stay unchanged.
#define main retainedScriptEventMain
#include "NasaScriptEventTests.cpp"
#undef main
#include "FileManager.hpp"
#include "QtPlotReceiver.hpp"
#include "GmatCommand.hpp"
#include "UserDefinedFunction.hpp"
#include "SandboxException.hpp"
#include "GmatGlobal.hpp"
#include "DynamicDataDisplay.hpp"
#include <QTableWidget>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <functional>
#include <set>

int main(int argc,char **argv)
{
 QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
 app.setOrganizationName("GMATTests");app.setApplicationName("FunctionCommandMatrix");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Supply startup, frozen case and family");TestSettings settings;
  const auto startup=QFileInfo(argv[1]).absoluteFilePath(),output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
  require(!output.isEmpty(),"Evidence directory required");QDir::setCurrent(QFileInfo(startup).absolutePath());
  const auto bytes=read(QFileInfo(argv[2]).absoluteFilePath());const auto spec=QJsonDocument::fromJson(bytes).object();const QString family=argv[3];
  require(spec["family"]==family&&spec["context"]=="Function","Wrong function context fixture");
  auto save=[&](const QString &name,const QByteArray &data){write(QDir(output).filePath(name),data);};
  constexpr double radius=7000,mu=398600.4415;const double speed=std::sqrt(mu/radius);
  const auto functionPath=output+"/MatrixFunction.gmf",report=output+"/values.csv",extra=output+"/extra.csv";
  auto expand=[&](QString text){
   for(const auto &pair:QList<QPair<QString,QString>>{{"@SPEED@",QString::number(speed,'g',17)},
      {"@VALUES@",report},{"@EXTRA@",extra},{"@OPT@",output+"/optimizer.txt"},{"@DC@",output+"/dc.txt"},
      {"@FUNCTION@",functionPath},{"@INPUT@",output+"/input.sv"}})text.replace(pair.first,pair.second);
   require(!text.contains('@'),"Unexpanded function matrix placeholder");return text;
  };
  const auto function=expand(spec["function_body"].toString()),mission=expand(spec["main_script"].toString());
  save("MatrixFunction.gmf",function.toUtf8());save("mission.script",mission.toUtf8());save("case.json",bytes);
  for(const auto &v:spec["support_files"].toArray()){const auto file=v.toObject();const auto name=file["name"].toString();
   require(QFileInfo(name).fileName()==name&&name!="."&&name!="..","Unsafe support filename");save(name,expand(file["text"].toString()).toUtf8());}
  MainWindow window;require(window.initialize(startup),"Function matrix runtime failed");window.show();events();acceptanceView(window);
  FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",output.toStdString());
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");
  auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&area,"Function workspace missing");
  source->setPlainText(mission);const bool built=window.buildScript();save("build-messages.txt",messages->toPlainText().toUtf8());require(built,"Function matrix Build failed");
  const auto saved=output+"/Function commands Δ.script";
  require(window.saveScriptTo(saved)&&read(saved)==mission.toUtf8()&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==mission,"Function Unicode save/reopen changed source");
  const auto undo=source->document()->availableUndoSteps();messages->clear();
  const auto result=window.runMission();events();save("messages.txt",messages->toPlainText().toUtf8());
  const bool stopped=family=="Stop";
  require(result==(stopped?MainWindow::RunResult::Stopped:MainWindow::RunResult::Completed),"Function command did not produce expected completion/Stop");
  const auto raw=read(report);save("observed-values.csv",raw);const auto rows=QString::fromUtf8(raw).trimmed().split('\n');
  const auto expected=spec["expected_report"].toArray(),bounds=spec["column_tolerances"].toArray();
  require(rows.size()==1&&expected.size()==12&&bounds.size()==12,"Function result shape differs");QJsonArray observed;
  const auto cells=rows.first().trimmed().split(',');require(cells.size()==12,"Function report width differs");
  for(int i=0;i<12;++i){bool ok=false;const double value=cells[i].trimmed().toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-expected[i].toDouble())<bounds[i].toDouble(),"Function result differs from retained independent circle/scalar oracle");observed.append(value);}
  auto *engine=Moderator::Instance();auto *sat=engine->GetInternalObject("Sat");require(sat,"Function global spacecraft missing");
  const QStringList axes{"X","Y","Z","VX","VY","VZ"},scalars{"Keep","Count","U","Cost","Quantity"};
  for(int i=0;i<6;++i)require(std::abs(sat->GetRealParameter(axes[i].toStdString())-expected[i+1].toDouble())<bounds[i+1].toDouble(),"Function global native orbit differs");
  for(int i=0;i<scalars.size();++i){auto *v=engine->GetInternalObject(scalars[i].toStdString());require(v&&std::abs(v->GetRealParameter("Value")-expected[i+7].toDouble())<bounds[i+7].toDouble(),"Function native scalar effect differs");}
  auto *returned=engine->GetInternalObject("Returned"),*after=engine->GetInternalObject("CallerAfter");
  require(returned&&after&&after->GetRealParameter("Value")== (stopped?41:42)&&returned->GetRealParameter("Value")== (stopped?-999:42),"Function continuation/return crossed Stop or failed normally");
  bool privateAbsent=false;try{privateAbsent=!engine->GetInternalObject("PrivateFunctionSentinel");}catch(SandboxException &e){require(QString::fromStdString(e.GetFullMessage()).contains("PrivateFunctionSentinel"),"Unexpected private lookup rejection");privateAbsent=true;}
  require(privateAbsent&&!engine->GetConfiguredObject("PrivateFunctionSentinel"),"Function private declaration escaped");
  if(stopped)require(!QFileInfo::exists(extra),"Function Stop executed its following report");
  UserDefinedFunction *executed=nullptr;
  for(auto *cmd=engine->GetFirstCommand();cmd;cmd=cmd->GetNextWhileExecuting())if(cmd->IsOfType("CallFunction")){
   auto *fn=dynamic_cast<UserDefinedFunction *>(cmd->GetRefObject(Gmat::FUNCTION,""));if(fn&&fn->GetName()=="MatrixFunction"){require(!executed,"Duplicate function invocation");executed=fn;}}
  require(executed&&executed->GetFunctionControlSequence()&&executed->GetGlobalObjectStore(),"Executed function native graph absent");
  QJsonArray native;std::set<GmatCommand *> seen;
  std::function<void(GmatCommand *,GmatCommand *)> walk=[&](GmatCommand *cmd,GmatCommand *owner){while(cmd&&cmd!=owner&&seen.insert(cmd).second){native.append(QString::fromStdString(cmd->GetTypeName()));for(int i=0;auto *child=cmd->GetChildCommand(i);++i)walk(child,cmd);cmd=cmd->GetNextWhileExecuting();}};
  walk(executed->GetFunctionControlSequence(),nullptr);
  for(const auto &type:spec["required_function_types"].toArray())require(native.contains(type),"Required command absent from actual function graph");
  QJsonArray identities;const auto *globals=executed->GetGlobalObjectStore();
  for(const auto &entry:spec["global_names"].toArray()){const auto name=entry.toString().toStdString();auto *runtime=engine->GetInternalObject(name);require(runtime&&globals->count(name)&&globals->at(name)==runtime,"Function did not share the exact explicit-global object");identities.append(QJsonObject{{"name",entry},{"runtime_pointer",QString::number(quintptr(runtime),16)},{"function_global_pointer",QString::number(quintptr(globals->at(name)),16)}});}
  QJsonObject effects;const auto trace=window.plotReceiver()->model("Trace"),control=window.plotReceiver()->model("Control");
  require(trace&&control&&trace->curves.size()==1&&control->curves.size()==1,"Function publication models absent");
  for(const auto &model:{trace,control}){require(model->curves[0].points.size()>=2,"Function samples absent");for(const auto &point:model->curves[0].points)require(std::isfinite(point.x)&&std::abs(point.x-point.y)<1e-10,"Function elapsed publication differs");require(std::abs(model->curves[0].points.back().x-20)<1e-4,"Function final publication epoch differs");}
  if(QStringList{"ClearPlot","PenUp","PenDown","MarkPoint"}.contains(family)){
   QJsonArray points,controlPoints;int marks=0,gaps=0,links=0,earlyGaps=0,earlyControl=0;
   for(const auto &p:trace->curves[0].points){points.append(QJsonArray{p.x,p.y,p.connect,p.highlighted});if(p.highlighted){++marks;require(std::abs(p.x-10)<1e-4,"Function marked wrong epoch");}if(p.x>10+1e-5){if(p.connect)++links;else++gaps;}if(p.x>1e-5&&p.x<10+1e-5&&!p.connect)++earlyGaps;if(family=="ClearPlot")require(p.x>=10-1e-5,"Function ClearPlot retained old data");}
   for(const auto &p:control->curves[0].points){controlPoints.append(QJsonArray{p.x,p.y,p.connect,p.highlighted});if(p.x<10-1e-5)++earlyControl;if(family=="MarkPoint")require(!p.highlighted,"Function MarkPoint changed unselected plot");}
   require(earlyControl>0&&control->penDown,"Function plot command changed unselected control");
   if(family=="MarkPoint")require(marks==1,"Function mark count differs");
   if(family=="PenUp")require(!trace->penDown&&gaps>0&&links==0,"Function PenUp interval differs");
   if(family=="PenDown")require(trace->penDown&&earlyGaps>0&&links>0&&gaps==0,"Function PenDown interval differs");
   effects["trace_points"]=points;effects["control_points"]=controlPoints;effects["trace_pen_down"]=trace->penDown;save("plot-observations.json",QJsonDocument(effects).toJson());
  }
  if(family=="UpdateDynamicData"){
   require(window.plotReceiver()->show("Display"),"Function display missing");events();auto *child=area->activeSubWindow();auto *table=child?qobject_cast<QTableWidget *>(child->widget()):nullptr;
   require(table&&table->rowCount()==1&&table->columnCount()==2&&table->item(0,0)&&table->item(0,1),"Function display shape differs");
   auto *nativeDisplay=dynamic_cast<DynamicDataDisplay *>(engine->GetInternalObject("Display"));require(nativeDisplay,"Native display absent");
   QJsonArray nativeCells;for(const auto &row:nativeDisplay->GetDynamicDataStruct())for(const auto &cell:row)nativeCells.append(QJsonObject{{"name",QString::fromStdString(cell.paramName)},{"captured",QString::fromStdString(cell.paramValue)},{"ref_pointer",QString::number(quintptr(cell.paramRef),16)},{"runtime_pointer",QString::number(quintptr(engine->GetInternalObject(cell.paramName)),16)},{"ref_value",cell.paramRef?cell.paramRef->GetRealParameter("Value"):-999}});
   save("display-observed.json",QJsonDocument(QJsonObject{{"first",table->item(0,0)->text()},{"second",table->item(0,1)->text()},{"native_cells",nativeCells},{"retained_value_context",bool(nativeDisplay->GetFunctionValueContext())}}).toJson());
   require(table->item(0,0)->text().section(" = ",1).toDouble()==41&&table->item(0,1)->text().section(" = ",1).toDouble()==7,"Function display missed selected capture or refreshed later");acceptanceView(*child);effects["display_values"]=QJsonArray{table->item(0,0)->text(),table->item(0,1)->text()};
  }
  if(spec.contains("expected_extra_rows")){
   const auto data=read(extra);save("backend-values.csv",data);const auto lines=QString::fromUtf8(data).trimmed().split('\n');const auto wanted=spec["expected_extra_rows"].toArray(),limits=spec["extra_tolerances"].toArray();require(lines.size()==wanted.size(),"Function backend row count differs");QJsonArray actual;
   for(int i=0;i<lines.size();++i){const auto fields=lines[i].trimmed().split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);const auto reference=wanted[i].toArray();require(fields.size()==reference.size()&&limits.size()==reference.size(),"Function backend width differs");QJsonArray row;for(int j=0;j<fields.size();++j){bool ok=false;const auto value=fields[j].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-reference[j].toDouble())<=limits[j].toDouble(),"Function Set result differs from original independent oracle");row.append(value);}actual.append(row);}effects["backend_values"]=actual;
  }
  if(family=="Save"){
   const auto data=read(output+"/Sat.Spacecraft");save("saved-runtime.txt",data);const auto text=QString::fromUtf8(data);QJsonArray values;
   require(data.contains("Create Spacecraft Sat"),"Function Save omitted spacecraft");
   for(int i=0;i<6;++i){const auto m=QRegularExpression("(?:^|\\n)(?:GMAT )?Sat\\."+axes[i]+" = ([^;\\n]+);").match(text);bool ok=false;const auto value=m.captured(1).toDouble(&ok);require(m.hasMatch()&&ok&&std::isfinite(value)&&std::abs(value-expected[i+1].toDouble())<2e-5,"Function Save exported wrong Cartesian state");values.append(value);}effects["saved_cartesian"]=values;
  }
  if(family=="CommandEcho"){
   const auto text=messages->toPlainText();require(!GmatGlobal::Instance()->EchoCommands()&&text.contains("Report Extra Keep;")&&!text.contains("Report 'Witness' Values"),"Function command echo did not publish selected command and stop before Witness");
   require(QString::fromUtf8(read(extra)).trimmed().toDouble()==41,"Echo selected report did not execute");effects["selected_command_echoed"]=true;effects["echo_off_at_return"]=true;
  }
  require(source->toPlainText()==mission&&source->document()->availableUndoSteps()==undo&&!source->document()->isModified()&&read(saved)==mission.toUtf8()&&read(functionPath)==function.toUtf8(),"Function execution changed source/history/files");
  QJsonArray registry;for(const auto &name:engine->GetListOfFactoryItems(Gmat::COMMAND))registry.append(QString::fromStdString(name));
  const QJsonObject proof{{"status","passed"},{"family",family},{"context","Function"},{"fixture_sha256",QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())},{"native_function_types",native},{"factory_types",registry},{"expected",expected},{"observed",observed},{"column_tolerances",bounds},{"global_identities",identities},{"private_name_absent",privateAbsent},{"effects",effects},{"caller_after",after->GetRealParameter("Value")},{"returned",returned->GetRealParameter("Value")},{"native_outcome",stopped?"Stopped":"Completed"},{"source_history_files_unchanged",true},{"new_requirement_credit",0},{"scope","Selected missing function contexts with actual execution, explicit-global identity, independent original numerical/effect oracles and Unicode reopen; no universal requirement or whole-current-build qualification."}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
