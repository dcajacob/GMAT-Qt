#include "FormationAcceptance.hpp"
#include "CommandEditor.hpp"
#include "Debugger.hpp"
#include <QAbstractItemView>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
using namespace formaccept;

static void reveal(QWidget *w) {
 require(w,"Batch mode control missing");
 for(auto*p=w;p;p=p->parentWidget())if(auto*s=qobject_cast<QScrollArea*>(p))s->ensureWidgetVisible(w);
 events();require(w->isVisible()&&w->isEnabled(),"Batch mode control unreachable");acceptanceView(*w);
}
static void click(QAbstractButton *w){reveal(w);QTest::mouseClick(w,Qt::LeftButton);events();}
static void choose(QComboBox*w,const QString&value){
 reveal(w);const int i=w->findText(value);require(i>=0,"Batch mode option missing");
 QTest::mouseClick(w,Qt::LeftButton,Qt::NoModifier,QPoint(w->width()-10,w->height()/2));events();
 auto*v=w->view();require(v->isVisible(),"Batch mode popup did not open");acceptanceView(*v->window());
 QTest::qWait(QApplication::doubleClickInterval()+20);const auto item=w->model()->index(i,w->modelColumn(),w->rootModelIndex());
 v->scrollTo(item);events();const auto hit=v->visualRect(item).intersected(v->viewport()->rect());require(!hit.isEmpty(),"Batch mode option unreachable");
 QTest::mouseClick(v->viewport(),Qt::LeftButton,Qt::NoModifier,hit.center());events();require(w->currentText()==value&&!QApplication::activePopupWidget(),"Batch mode popup did not select option");
}
static std::array<double,6> circle(double t,double phase=0){
 const double radius=7000,rate=std::sqrt(398600.4415/(radius*radius*radius)),v=std::sqrt(398600.4415/radius),a=rate*t+phase,c=std::sqrt(.5);
 return {radius*std::cos(a),radius*std::sin(a)*c,radius*std::sin(a)*c,-v*std::sin(a),v*std::cos(a)*c,v*std::cos(a)*c};
}
static GmatCommand* estimatorCommand(GmatCommand*first,QSet<GmatCommand*>&seen){
 for(auto*c=first;c&&!seen.contains(c);c=c->GetNext()){
  seen.insert(c);if(c->GetTypeName()=="RunEstimator")return c;
  if(auto*found=estimatorCommand(c->GetChildCommand(0),seen))return found;
 }return nullptr;
}
int main(int argc,char**argv){
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaBatchModes");QLocale::setDefault(QLocale::c());
 try{
  require(argc==4&&acceptanceWidgetPlatform(),"Supply batch mode startup,shared cases and mode");
  const QString mode=argv[3];require(QStringList{"InitialGuess","IterationLimit","Reentry"}.contains(mode),"Unknown batch mode case");
  const bool guess=mode=="InitialGuess",repeat=mode=="Reentry";const QString selected=guess?"RunInitialGuess":"Solve";
  const auto fixture=QJsonDocument::fromJson(read(argv[2])).object();Rig rig(QFileInfo(argv[1]).absoluteFilePath());
  const auto observations=rig.files.filePath("Measurements Δ.gmd"),fit=rig.files.filePath("Fit Δ.txt"),stateFile=rig.files.filePath("State Δ.csv"),saved=rig.files.filePath("Modes Ω.script");
  auto script=QString::fromUtf8(read(QFileInfo(argv[1]).absolutePath()+"/../samples/Navigation/Ex_Estimate_RangeSkin.script"));
  auto set=[&](const QString&field,const QString&value){QRegularExpression re("^"+QRegularExpression::escape(field)+"\\s*=[^\\n]*",QRegularExpression::MultilineOption);require(re.match(script).hasMatch(),"Batch mode shipped field missing");script.replace(re,field+" = "+value+";");};
  script.replace("'Ex_Estimate_RangeSkin.gmd'","'"+observations+"'");
  set("sim.InitialEpoch","'10 Jun 2012 00:00:00.000'");set("sim.FinalEpoch","'10 Jun 2012 08:00:00.000'");set("sim.MeasurementTimeStep","60");set("sim.AddNoise","Off");
  set("simData.UseLightTime","False");set("estData.UseLightTime","False");set("bat.ReportFile","'"+fit+"'");set("bat.ShowAllResiduals","Off");
  set("bat.AbsoluteTol","0.0001");set("bat.RelativeTol","0.0001");set("bat.MaximumIterations",mode=="IterationLimit"?"1":"12");set("bat.OLSEAdditiveConstant","1");set("ODProp.MaxStep","20");set("ODProp.InitialStepSize","20");
  const auto truth=circle(0);auto initialState=guess?circle(0,.001):truth;
  if(!guess){const double bias[]{1,-.5,.25,.0005,-.0003,.0002};for(int i=0;i<6;++i)initialState[i]+=bias[i];}
  const QStringList components{"X","Y","Z","VX","VY","VZ"};for(int i=0;i<6;++i){set("SimSat."+components[i],number(truth[i]));set("EstSat."+components[i],number(initialState[i]));}
  script.remove(QRegularExpression("^(?:simData|estData)\\.AddTrackingConfig[^\\n]*\\n",QRegularExpression::MultilineOption));QString network;
  for(auto v:fixture["stations"].toArray()){
   const auto station=v.toObject();const auto name=station["name"].toString();network+="Create GroundStation "+name+";\n"+name+".CentralBody = Earth;\n"+name+".StateType = Spherical;\n"+name+".HorizonReference = Ellipsoid;\n"+name+".Location1 = "+number(station["latitude"].toDouble())+";\n"+name+".Location2 = "+number(station["longitude"].toDouble())+";\n"+name+".Location3 = 0;\n"+name+".Id = '"+name+"';\n"+name+".AddHardware = {Transmitter1, Receiver1, Antenna1};\n"+name+".MinimumElevationAngle = 0;\n"+name+".ErrorModels = {RangeModel};\nsimData.AddTrackingConfig = {{"+name+", SimSat, "+name+"}, 'Range_Skin'};\nestData.AddTrackingConfig = {{"+name+", EstSat, "+name+"}, 'Range_Skin'};\n";
  }
  const QString declarations="% Batch mode source Ω\nEarth.Mu = 398600.4415;\nODProp_ForceModel.PrimaryBodies = {};\nODProp_ForceModel.PointMasses = {Earth};\nODProp_ForceModel.RelativisticCorrection = Off;\nCreate Variable Keep Pass;\nKeep = 9;\nPass = 1;\nCreate ReportFile StateReport;\nStateReport.Filename = '"+stateFile+"';\nStateReport.WriteHeaders = false;\nStateReport.FixedWidth = false;\nStateReport.Delimiter = ',';\nStateReport.Precision = 17;\n";
  const QString command="RunEstimator 'Mode witness' bat; % retain mode label and comment";
  const QString report="Report StateReport Pass EstSat.A1ModJulian EstSat.EarthMJ2000Eq.X EstSat.EarthMJ2000Eq.Y EstSat.EarthMJ2000Eq.Z EstSat.EarthMJ2000Eq.VX EstSat.EarthMJ2000Eq.VY EstSat.EarthMJ2000Eq.VZ Keep;";
  script.replace("BeginMissionSequence",network+declarations+"BeginMissionSequence");script.replace("RunEstimator bat;",(repeat?"For Pass = 1:2;\n":"")+command+"\n"+report+(repeat?"\nEndFor;":""));
  rig.source->setPlainText(script);require(rig.window.buildScript(),"Batch mode initial source failed build");rig.source->document()->setModified(false);rig.save("initial.script",script.toUtf8());
  int index=-1;const auto mission=rig.window.missionSnapshot();for(int i=0;i<mission.nodes.size();++i)if(mission.nodes[i].type=="RunEstimator")index=i;
  auto*tree=rig.window.findChild<QTreeWidget*>("Mission");for(auto*tabs:rig.window.findChildren<QTabWidget*>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree))tabs->setCurrentIndex(i);
  QTreeWidgetItem*item=nullptr;for(auto*v:tree->findItems("",Qt::MatchContains|Qt::MatchRecursive))if(v->data(0,Qt::UserRole).isValid()&&v->data(0,Qt::UserRole).toInt()==index)item=v;
  require(index>=0&&item,"Batch mode mission tree command missing");tree->expandAll();tree->scrollToItem(item);events();reveal(tree);const auto point=tree->visualItemRect(item).center();QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();
  QPointer<QMdiSubWindow>owner=rig.area->activeSubWindow();require(owner&&dynamic_cast<CommandEditor*>(owner->widget()),"Batch mode command editor missing");
  auto*defaults=owner->widget()->findChild<QPushButton*>("commandAddOptions");require(defaults,"RunEstimator has no structured SolveMode default control");click(defaults);
  auto*choice=owner->widget()->findChild<QComboBox*>("commandChoice_SolveMode");require(choice&&choice->count()==2&&choice->findText("Solve")>=0&&choice->findText("RunInitialGuess")>=0&&choice->findText("RunCorrected")==-1,"RunEstimator SolveMode choices disagree with NASA guide");
  choose(choice,selected);require(rig.source->toPlainText()==script&&!QFileInfo::exists(observations),"Pending batch mode escaped source or executed mission");
  auto*local=owner->widget()->findChild<QPlainTextEdit*>("commandSource");require(local&&local->toPlainText()==QString(command).replace("bat;","bat {SolveMode = "+selected+"};"),"Batch mode pending edit changed label/comment or unsupported options");
  click(owner->widget()->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply));
  const auto applied=QString(script).replace(command,QString(command).replace("bat;","bat {SolveMode = "+selected+"};"));require(rig.source->toPlainText()==applied&&!dynamic_cast<CommandEditor*>(owner->widget())->hasChanges(),"Batch mode Apply did not commit exactly");
  owner->close();events();rig.source->undo();require(rig.source->toPlainText()==script&&rig.window.buildScript(),"Batch mode Undo lost exact source");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Batch mode Redo lost exact source");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Batch mode Unicode reopen failed");rig.save("mission.script",applied.toUtf8());
  Debugger*debug=nullptr;for(auto*o:rig.window.children())if(auto*d=dynamic_cast<Debugger*>(o))debug=d;int probe=-1;const auto current=rig.window.missionSnapshot();for(int i=0;i<current.nodes.size();++i)if(current.nodes[i].type=="Report"&&current.nodes[i].statement.contains("StateReport"))probe=i;
  require(debug&&probe>=0&&rig.window.setBreakpoint(probe,true),"Batch mode native-clone breakpoint unavailable");const auto oldPaused=debug->paused;std::exception_ptr failure;QJsonArray clones;
  debug->paused=[&](const QString&name){if(oldPaused)oldPaused(name);try{
   require(name=="Report","Batch mode stopped at wrong command");QSet<GmatCommand*>seen;auto*c=estimatorCommand(Moderator::Instance()->GetFirstCommand(),seen);auto*estimator=c?c->GetRefObject(Gmat::SOLVER,"bat"):nullptr;require(estimator&&estimator!=Moderator::Instance()->GetConfiguredObject("bat"),"Batch mode actual native clone missing");
   const auto status=QString::fromStdString(estimator->GetStringParameter("ConvergentStatus"));require(guess?status=="Ran Initial Guess":mode=="IterationLimit"?status=="Exceed maximum number of iterations":status.startsWith("Meet "),"Batch mode native terminal status incorrect");clones.append(QJsonObject{{"status",status},{"before_run_complete",true}});rig.save("clones.json",QJsonDocument(clones).toJson());if(auto*d=rig.window.findChild<QDialog*>("debuggerDialog"))acceptanceView(*d);
  }catch(...){failure=std::current_exception();}QTimer::singleShot(0,&rig.window,[&]{debug->resume();});};
  auto*messages=rig.window.findChild<QPlainTextEdit*>("messageWindow");messages->clear();const auto result=rig.window.runMission();debug->paused=oldPaused;rig.window.setBreakpoint(probe,false);events();rig.save("messages.txt",messages->toPlainText().toUtf8());
  for(auto pair:QList<QPair<QString,QString>>{{stateFile,"states.csv"},{observations,"observations.gmd"},{fit,"estimator.txt"}})if(QFileInfo::exists(pair.first))rig.save(pair.second,read(pair.first));
  if(failure)std::rethrow_exception(failure);require(result==MainWindow::RunResult::Completed,"Batch mode execution failed");require(clones.size()==(repeat?2:1),"Batch mode did not execute expected invocation count");require(rig.source->toPlainText()==applied&&!rig.window.isRunning()&&!rig.source->isReadOnly(),"Batch mode completion changed source or retained execution lock");
  const auto progress=messages->toPlainText();require(guess?progress.contains("Estimation ran initial guess only")&&!progress.contains("Estimation converged!")&&!progress.contains("Iteration 1"):mode=="IterationLimit"?progress.contains("Estimation did not converge!")&&!progress.contains("Estimation converged!"):progress.count("Estimation converged!")==2,"Batch mode progress/stopping result incorrect");
  QJsonArray rows;const double a1=fixture["a1_epoch"].toDouble();double maxPositionError=0;
  for(auto line:QString::fromUtf8(read(stateFile)).split('\n',Qt::SkipEmptyParts)){
   auto fields=line.trimmed().split(',');require(fields.size()==9,"Batch mode state report schema incorrect");QJsonArray row;for(auto f:fields){bool ok=false;const double v=f.toDouble(&ok);require(ok&&std::isfinite(v),"Batch mode state nonfinite or invalid");row.append(v);}
   const double t=(row[1].toDouble()-a1)*86400;auto wanted=circle(t,guess?.001:0);
   if(mode=="IterationLimit"){
    const auto reference=fixture["one_step_reference"].toObject();const auto expected=reference["expected_state"].toArray();
    require(expected.size()==6&&reference["range_legs"].toInt()==2&&reference["accepted_observations"].toInt()==58,"Independent one-step fixture missing or inconsistent");
    for(int i=0;i<6;++i)wanted[i]=expected[i].toDouble();
   }
   require(row[0].toInt()==rows.size()+1&&row[8].toDouble()==9,"Batch mode pass or continuation sentinel incorrect");
   if(!guess)require(std::abs(t)<1e-4,"Batch mode corrected state not restored to estimation epoch");
   for(int i=0;i<6;++i){const double error=std::abs(row[i+2].toDouble()-wanted[i]);const double bound=mode=="IterationLimit"?(i<3?.01:.00001):(i<3?2e-5:2e-8);require(error<bound,"Batch mode result differs from independent circular-orbit witness");if(mode=="IterationLimit")require(error<fixture["one_step_reference"].toObject()[i<3?"position_tolerance":"velocity_tolerance"].toDouble(),"Batch one-step state differs from independent least-squares reference");if(i<3)maxPositionError=std::max(maxPositionError,error);}rows.append(row);
  }
  require(rows.size()==clones.size(),"Batch mode state reports lost repeated invocation");
  QJsonObject oneStep;
  if(mode=="IterationLimit"){
   const auto reference=fixture["one_step_reference"].toObject();const auto mask=reference["accepted_mask"].toArray(),computed=reference["computed_ranges"].toArray(),observed=reference["observed_ranges"].toArray();
   require(mask.size()==358&&computed.size()==358&&observed.size()==358,"Independent one-step observation reference incomplete");
   const auto expectedHash=reference["inputs"].toObject()["src/qtgui/tests/batch-one-step/observations.gmd"].toString();
   require(QString::fromLatin1(QCryptographicHash::hash(read(observations),QCryptographicHash::Sha256).toHex())==expectedHash,"One-step observations changed from shared independent-reference inputs");
   int count=0,accepted=0;double maximumRangeError=0,maximumResidualError=0;
   for(auto line:QString::fromUtf8(read(fit)).split('\n')){
    if(!QRegularExpression("^\\s*0\\s+\\d+\\s+\\d\\d [A-Z][a-z]{2} 2012 .*Range_Skin").match(line).hasMatch())continue;
    const auto f=line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require((f.size()==12||f.size()==13)&&f[1].toInt()==count&&count<358,"One-step native observation report schema/order invalid");
    const bool kept=f.size()==12;const int start=kept?8:9;require(kept==mask[count].toBool(),"One-step observation editing differs from independent residual selection");accepted+=kept;
    const double rangeError=std::abs(f[start+1].toDouble()-computed[count].toDouble());
    const double residualError=std::abs(f[start+2].toDouble()-(observed[count].toDouble()-computed[count].toDouble()));
    require(rangeError<reference["native_print_tolerance_km"].toDouble()&&residualError<reference["native_print_tolerance_km"].toDouble(),"One-step native computed range/residual differs from independent two-leg geometry");maximumRangeError=std::max(maximumRangeError,rangeError);maximumResidualError=std::max(maximumResidualError,residualError);++count;
   }
   require(count==358&&accepted==58,"One-step native report omitted observations or accepted set differs");
   const auto row=rows[0].toArray();double truthError=0;const auto truthState=circle(0);for(int i=0;i<3;++i)truthError=std::max(truthError,std::abs(row[i+2].toDouble()-truthState[i]));
   require(truthError>.2&&!progress.contains("Estimation converged!"),"One-step witness lost its deliberately nonconverged state");
   oneStep=QJsonObject{{"records",count},{"accepted",accepted},{"computed_range_max_error_km",maximumRangeError},{"residual_max_error_km",maximumResidualError},{"truth_position_error_km",truthError},{"reference_position_error_km",maxPositionError},{"station_geometry_is_shared_input",true}};rig.save("one-step.json",QJsonDocument(oneStep).toJson());
  }
  if(guess){double separation=0;const auto zero=circle((rows[0].toArray()[1].toDouble()-a1)*86400);for(int i=0;i<3;++i)separation+=std::pow(rows[0].toArray()[i+2].toDouble()-zero[i],2);require(std::sqrt(separation)>6,"RunInitialGuess silently corrected toward simulated truth");}
  rig.save("rows.json",QJsonDocument(rows).toJson());
  const QJsonObject proof{{"checkpoint","batch-mode-"+mode},{"outcome","passed"},{"mode",mode},{"solve_mode",selected},{"invocations",clones.size()},{"rows",rows.size()},{"numeric_sentinel",9},{"max_position_error_km",maxPositionError},{"one_step",oneStep},{"facets",QJsonArray{"actual-mission-tree-estimator-solve-mode-default-and-choice","pending-exact-apply-clean-undo-redo-unicode-reopen","actual-native-clone-terminal-status-before-destruction","independent-circular-initial-guess-or-recovered-state-and-continuation","initial-guess-no-correction-or-iteration-limit-or-reentrant-command"}},{"scope","Three selected BatchEstimator mode witnesses with shared native noiseless observations: phase-offset circular RunInitialGuess, one-iteration termination and two invocations of the same command in a For loop. The one-iteration case checks every two-leg computed range/residual and the accepted set against independent point-mass propagation, plus the six-component independent scaled-SVD correction; its .24km remaining truth error is explicitly nonconverged. Shared station/frame/time inputs and native simulated observations are not independent measurement physics. No output-format/failed/stopped/native-OS/hardware/platform/stress/current-private NASA or full-build qualification."}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException&e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
