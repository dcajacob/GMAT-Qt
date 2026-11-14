// Native filter/smoother format restrictions, with supported STK controls.
#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "Debugger.hpp"
#include "GmatGlobal.hpp"
#include <QAbstractItemView>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
using namespace formaccept;
static void reveal(QWidget *w) {
 require(w,"Filter format control missing");
 for(QWidget*p=w;p;p=p->parentWidget())if(auto*s=qobject_cast<QScrollArea*>(p))s->ensureWidgetVisible(w);
 events();require(w->isVisible()&&w->isEnabled(),"Filter format control unreachable");acceptanceView(*w);
}
static void choose(QComboBox *box,const QString &value) {
 reveal(box);int index=box->findText(value);require(index>=0,"Filter format choice missing");
 QTest::mouseClick(box,Qt::LeftButton,Qt::NoModifier,QPoint(box->width()-10,box->height()/2));events();
 auto*view=box->view();require(view->isVisible(),"Filter format popup absent");acceptanceView(*view->window());
 QTest::keyClick(view,Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(view,Qt::Key_Down);
 QTest::keyClick(view,Qt::Key_Return);events();require(box->currentText()==value,"Filter format popup selection failed");
}
static void flagClick(QCheckBox *box) {
 reveal(box);QStyleOptionButton option;option.initFrom(box);
 QTest::mouseClick(box,Qt::LeftButton,Qt::NoModifier,box->style()->subElementRect(QStyle::SE_CheckBoxIndicator,&option,box).center());events();
}
static QString unrelated(QString text) {
 text.remove(QRegularExpression("^Export\\.[^\\n]*\\n",QRegularExpression::MultilineOption));return text;
}
int main(int argc,char**argv) {
 QApplication app(argc,argv);app.setAttribute(Qt::AA_DontUseNativeDialogs);QLocale::setDefault(QLocale::c());
 app.setOrganizationName("GMATTests");app.setApplicationName("NasaFilterFormat");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Supply filter format startup,fixture and key");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath();const auto fixture=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();
  const QString key=argv[3];QJsonObject spec;for(auto c:fixture["cases"].toArray())if(c.toObject()["key"]==key)spec=c.toObject();
  require(!spec.isEmpty(),"Unknown filter format case");const bool smoother=spec["family"]=="Smoother",allowed=spec["allowed"].toBool();
  Rig rig(startup);const int runMode=GmatGlobal::Instance()->GetRunModeStartUp();
  require(runMode!=GmatGlobal::TESTING,"Filter format case must exercise normal startup restrictions");
  const auto observations=rig.files.filePath("GPS observations Δ.gmd"),filterReport=rig.files.filePath("Filter results Δ.txt"),smoothReport=rig.files.filePath("Smoother results Δ.txt");
  const auto simControl=rig.files.filePath("Simulation completed Δ.csv"),continuation=rig.files.filePath("Continuation Δ.csv"),covariance=rig.files.filePath("Assigned covariance Δ.csv"),forwardControl=rig.files.filePath("Forward filter completed Δ.csv");
  const auto exported=rig.files.filePath("Estimator output Δ."+spec["extension"].toString()),saved=rig.files.filePath("Filter format Ω.script");
  auto script=QString::fromUtf8(read(QFileInfo(startup).absoluteDir().absoluteFilePath("../samples/Navigation/Ex_FilterSmoother_GpsPosVec.script")));
  const int begin=script.indexOf("BeginMissionSequence");require(begin>0,"Shipped filter/smoother mission boundary missing");script=script.left(begin);
  auto set=[&](const QString&name,const QString&value){QRegularExpression r("^"+QRegularExpression::escape(name)+"\\s*=[^\\n]*",QRegularExpression::MultilineOption);require(r.match(script).hasMatch(),"Shipped filter format field missing");script.replace(r,name+" = "+value+";");};
  script.replace("'Ex_FilterSmoother_GpsPosVec.gmd'","'"+observations+"'");
  set("Sim.FinalEpoch","'10 Jun 2010 00:10:00.000'");set("Sim.MeasurementTimeStep","60");set("Sim.AddNoise","Off");
  set("EKF.ReportFile","'"+filterReport+"'");set("FPS.ReportFile","'"+smoothReport+"'");
  set("EKF.OutputWarmStartFile","'"+rig.files.filePath("Filter warm start Δ.csv")+"'");
  set("FilterCdReport.Filename","'"+rig.files.filePath("Unused drag Δ.csv")+"'");
  script+="% Preserve filter format source Δ\nCreate Variable Keep;\nKeep = 11;\nCreate Array AssignedCovariance[6,6];\nCreate ReportFile CovarianceReport;\nCovarianceReport.Filename = '"+covariance+"';\nCovarianceReport.WriteHeaders = false;\nCovarianceReport.FixedWidth = false;\nCovarianceReport.Delimiter = ',';\nCovarianceReport.Precision = 17;\nCreate ReportFile ForwardControl;\nForwardControl.Filename = '"+forwardControl+"';\nForwardControl.WriteHeaders = false;\nCreate ReportFile SimControl Continuation;\nSimControl.Filename = '"+simControl+"';\nSimControl.WriteHeaders = false;\nContinuation.Filename = '"+continuation+"';\nContinuation.WriteHeaders = false;\nContinuation.FixedWidth = false;\nContinuation.Delimiter = ',';\nContinuation.Precision = 17;\nCreate EphemerisFile Export;\nExport.Spacecraft = EstSat;\nExport.FileFormat = CCSDS-OEM;\nExport.Filename = '"+exported+"';\nExport.CoordinateSystem = EarthMJ2000Eq;\nExport.StepSize = 20;\nExport.InterpolationOrder = 7;\nExport.WriteEphemeris = false;\nBeginMissionSequence;\nSetSeed(1);\nRunSimulator Sim;\nReport SimControl Keep;\nEstSat.OrbitErrorCovariance = diag([1.0e-2 1.0e-2 1.0e-2 2.5e-7 2.5e-7 2.5e-7]);\n";
  script+="AssignedCovariance = EstSat.OrbitErrorCovariance;\nReport CovarianceReport";for(int r=1;r<=6;++r)for(int c=1;c<=6;++c)script+=" AssignedCovariance("+QString::number(r)+","+QString::number(c)+")";script+=";\n";
  if(smoother&&spec["format"]=="Code-500")script+="Toggle Export Off;\nRunEstimator EKF;\nReport ForwardControl Keep;\nToggle Export On;\n";
  if(!smoother||allowed)script+="RunEstimator EKF;\n";
  if(smoother)script+="RunSmoother FPS;\n";
  script+="Keep = 99;\nReport Continuation Keep EstSat.A1ModJulian EstSat.EarthMJ2000Eq.X EstSat.EarthMJ2000Eq.Y EstSat.EarthMJ2000Eq.Z EstSat.EarthMJ2000Eq.VX EstSat.EarthMJ2000Eq.VY EstSat.EarthMJ2000Eq.VZ;\n% Preserve filter tail Ω\n";
  const auto initial=script;rig.save("initial.script",initial.toUtf8());rig.source->setPlainText(initial);
  const bool built=rig.window.buildScript();events();rig.save("initial-messages.txt",rig.messages->toPlainText().toUtf8());
  if(!built)std::cerr<<rig.messages->toPlainText().toStdString();require(built,"Filter format sample build failed");
  rig.source->document()->setModified(false);const int undo=rig.source->document()->availableUndoSteps();
  auto pristine=[&]{return rig.source->toPlainText()==initial&&rig.source->document()->availableUndoSteps()==undo&&!rig.source->document()->isModified()&&!Moderator::Instance()->GetConfiguredObject("Export")->GetBooleanParameter("WriteEphemeris");};
  QPointer<QMdiSubWindow> child;ResourceEditor*panel=nullptr;
  auto open=[&]{
   for(auto*tabs:rig.window.findChildren<QTabWidget*>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==rig.resources||tabs->widget(i)->isAncestorOf(rig.resources))tabs->setCurrentIndex(i);
   auto found=rig.resources->findItems("Export",Qt::MatchExactly|Qt::MatchRecursive);require(found.size()==1,"Filter format resource tree identity missing");rig.resources->scrollToItem(found.first());events();reveal(rig.resources);
   auto at=rig.resources->visualItemRect(found.first()).center();QTest::mouseClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,at);events();QTest::mouseDClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,at);events();
   child=rig.area->currentSubWindow();panel=child?dynamic_cast<ResourceEditor*>(child->widget()):nullptr;require(panel,"Actual filter format editor missing");
  };
  auto configure=[&]{
   auto*form=panel->findChild<QDialog*>("ephemerisDialog");require(form&&form->isVisible(),"Filter format embedded form missing");
   auto*writeFlag=form->findChild<QCheckBox*>("ephemeris_WriteEphemeris");flagClick(writeFlag);require(writeFlag->isChecked(),"Filter format write checkbox did not enable");choose(form->findChild<QComboBox*>("ephemeris_FileFormat"),spec["format"].toString());
   auto*step=form->findChild<QComboBox*>("ephemeris_StepSize");auto*interpolator=form->findChild<QLineEdit*>("ephemeris_Interpolator");auto*boundaries=form->findChild<QCheckBox*>("ephemeris_IncludeEventBoundaries");require(step&&interpolator&&boundaries,"Filter format dependent controls missing");
   if(spec["format"]=="SPK")require(!step->isEnabled()&&step->currentText()=="IntegratorSteps"&&interpolator->text()=="Hermite","Filter SPK raw-step dependency differs");
   else if(spec["format"]=="Code-500"){require(step->isEnabled(),"Filter Code500 step control disabled");step->setCurrentText("20");require(step->currentText()=="20"&&interpolator->text()=="Lagrange","Filter Code500 fixed-step dependency differs");}
   else {require(step->isEnabled(),"Filter variable-step control disabled");step->setCurrentText("IntegratorSteps");require(step->currentText()=="IntegratorSteps","Filter variable-step edit differs");}
   if(allowed){require(boundaries->isEnabled(),"Filter STK event-boundary control disabled");if(!boundaries->isChecked())flagClick(boundaries);require(boundaries->isChecked(),"Filter STK event boundaries not enabled");}
   auto*name=form->findChild<QLineEdit*>("ephemeris_Filename");reveal(name);name->selectAll();QApplication::clipboard()->setText(exported);QTest::keyClick(name,Qt::Key_V,Qt::ControlModifier);events();require(name->text()==exported&&pristine()&&panel->hasChanges(),"Filter format pending edit escaped isolation");acceptanceView(*child);
  };
  auto action=[&](QDialogButtonBox::StandardButton which){auto*b=panel->findChild<QDialogButtonBox*>("resourceButtons")->button(which);require(b,"Filter format resource action missing");if(b->isEnabled())reveal(b);QTest::mouseClick(b,Qt::LeftButton);events();};
  open();configure();action(QDialogButtonBox::Cancel);require(!child&&pristine(),"Filter format Cancel changed source/model/history");
  open();configure();action(QDialogButtonBox::Apply);panel=child?dynamic_cast<ResourceEditor*>(child->widget()):nullptr;require(panel&&!panel->hasChanges(),"Filter format Apply did not rebuild a clean editor");const auto applied=rig.source->toPlainText();
  require(applied!=initial&&unrelated(applied)==unrelated(initial),"Filter format Apply changed unrelated source");
  auto*object=Moderator::Instance()->GetConfiguredObject("Export");require(object&&object->GetStringParameter("FileFormat")==spec["format"].toString().toStdString()&&object->GetBooleanParameter("WriteEphemeris"),"Filter format configured native resource differs");
  const int appliedUndo=rig.source->document()->availableUndoSteps();auto*clean=panel->findChild<QDialogButtonBox*>("resourceButtons")->button(QDialogButtonBox::Apply);require(clean&&!clean->isEnabled(),"Filter format clean Apply not disabled");action(QDialogButtonBox::Apply);require(rig.source->toPlainText()==applied&&rig.source->document()->availableUndoSteps()==appliedUndo,"Filter format clean Apply changed history");action(QDialogButtonBox::Close);require(!child,"Filter format clean Close retained panel");
  rig.source->undo();require(rig.source->toPlainText()==initial&&rig.window.buildScript(),"Filter format Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Filter format Redo failed");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Filter format Unicode reopen failed");rig.save("mission.script",applied.toUtf8());rig.messages->clear();
  QJsonArray clones;QJsonArray nativeState;double nativeEpoch=0;std::exception_ptr observerFailure;Debugger*debug=nullptr;int probe=-1;std::function<void(const QString&)> previous;
  if(allowed){
   for(auto*c:rig.window.children())if(auto*d=dynamic_cast<Debugger*>(c))debug=d;const auto snapshot=rig.window.missionSnapshot();for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].type=="Report"&&snapshot.nodes[i].statement.contains("Continuation"))probe=i;
   require(debug&&probe>=0&&rig.window.setBreakpoint(probe,true),"Filter format native-clone breakpoint unavailable");previous=debug->paused;
   debug->paused=[&](const QString&name){if(previous)previous(name);try{
    require(name=="Report","Filter format inspection reached wrong command");auto*command=Moderator::Instance()->GetFirstCommand();
    for(int guard=0;command&&guard<100;++guard,command=command->GetNext())if(command->GetTypeName()=="RunEstimator"||command->GetTypeName()=="RunSmoother"){
     const std::string solverName=command->GetTypeName()=="RunSmoother"?"FPS":"EKF";auto*clone=command->GetRefObject(Gmat::SOLVER,solverName);const std::string expected=solverName=="FPS"?"Smoother":"ExtendedKalmanFilter";
     require(clone&&clone!=Moderator::Instance()->GetConfiguredObject(solverName)&&clone->GetTypeName()==expected,"Filter format actual native solver clone missing");clones.append(QJsonObject{{"name",QString::fromStdString(solverName)},{"type",QString::fromStdString(clone->GetTypeName())},{"before_run_complete",true}});
    }
    require(clones.size()==(smoother?2:1),"Filter format native command-clone count differs");auto*sat=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("EstSat"));require(sat&&sat->GetState().GetSize()==6,"Filter format terminal native spacecraft missing");nativeEpoch=sat->GetEpoch();for(int i=0;i<6;++i){require(std::isfinite(sat->GetState()[i]),"Filter format native terminal state not finite");nativeState.append(sat->GetState()[i]);}if(auto*d=rig.window.findChild<QDialog*>("debuggerDialog"))acceptanceView(*d);
   }catch(...){observerFailure=std::current_exception();}QTimer::singleShot(0,&rig.window,[&]{debug->resume();});};
  }
  const auto run=rig.window.runMission();if(allowed){debug->paused=previous;rig.window.setBreakpoint(probe,false);}events();rig.save("messages.txt",rig.messages->toPlainText().toUtf8());
  for(const auto&pair:QList<QPair<QString,QString>>{{forwardControl,"forward-filter.csv"},{covariance,"covariance.csv"},{observations,"observations.gmd"},{filterReport,"filter.txt"},{smoothReport,"smoother.txt"},{simControl,"simulation.csv"},{continuation,"continuation.csv"},{exported,"mission."+spec["extension"].toString()}})if(QFileInfo::exists(pair.first))rig.save(pair.second,read(pair.first));
  const auto messages=rig.messages->toPlainText();const bool expectedDiagnostic=messages.contains(spec["diagnostic"].toString());
  rig.save("actual-outcome.json",QJsonDocument(QJsonObject{{"allowed",allowed},{"completed",run==MainWindow::RunResult::Completed},{"failed",run==MainWindow::RunResult::Failed},{"expected_diagnostic",expectedDiagnostic},{"continuation_exists",QFileInfo::exists(continuation)},{"run_mode",runMode},{"family",spec["family"]},{"format",spec["format"]},{"clones",clones},{"native_epoch",nativeEpoch},{"native_state",nativeState}}).toJson());
  if(observerFailure)std::rethrow_exception(observerFailure);require(rig.source->toPlainText()==applied,"Filter format run changed source");
  require(QFileInfo::exists(observations)&&QString::fromUtf8(read(observations)).contains("GPS_PosVec")&&QString::fromUtf8(read(simControl)).trimmed()=="11","Filter format simulator prerequisite did not complete");
  const auto covarianceValues=QString::fromUtf8(read(covariance)).trimmed().split(',');require(covarianceValues.size()==36,"Native NASA covariance assignment report dimensions differ");for(int r=0;r<6;++r)for(int c=0;c<6;++c){bool ok=false;const double actual=covarianceValues[6*r+c].toDouble(&ok),wanted=r==c?(r<3?1e-2:2.5e-7):0;require(ok&&std::isfinite(actual)&&std::abs(actual-wanted)<1e-18,"Native NASA covariance assignment differs from independent diagonal matrix");}
  if(smoother&&spec["format"]=="Code-500")require(QFileInfo::exists(forwardControl)&&QString::fromUtf8(read(forwardControl)).trimmed()=="11","Code500 smoother prerequisite forward filter did not complete with export toggled off");
  int points=0,boundaryCount=0;
  if(!allowed){
   if(run!=MainWindow::RunResult::Failed||!expectedDiagnostic)std::cerr<<messages.toStdString();
   require(run==MainWindow::RunResult::Failed&&expectedDiagnostic,"Native filter/smoother did not reject the forbidden format with its required diagnostic");
   require(!QFileInfo::exists(continuation),"Forbidden filter format reached mission continuation");
  }else{
   if(run!=MainWindow::RunResult::Completed)std::cerr<<messages.toStdString();require(run==MainWindow::RunResult::Completed,"Supported STK filter/smoother mission failed");
   const auto values=QString::fromUtf8(read(continuation)).trimmed().split(',');require(values.size()==8&&values[0].toDouble()==99&&std::abs(values[1].toDouble()-nativeEpoch)<1e-10,"Supported filter continuation/sentinel/epoch differs");
   for(int i=0;i<6;++i)require(std::abs(values[i+2].toDouble()-nativeState[i].toDouble())<1e-9,"Supported filter native/report terminal state differs");
   const auto payload=QString::fromUtf8(read(exported));require(payload.startsWith("stk.v.")&&payload.contains("CoordinateSystem        J2000")&&payload.contains("DistanceUnit            Kilometers")&&payload.contains("END Ephemeris"),"Supported filter STK metadata/terminator differs");
   bool data=false,boundaries=false;for(auto line:payload.split('\n')){line=line.trimmed();if(line=="BEGIN SegmentBoundaryTimes"){boundaries=true;continue;}if(line=="END SegmentBoundaryTimes"){boundaries=false;continue;}if(line=="EphemerisTimePosVel"){data=true;continue;}if(line=="END Ephemeris")data=false;if(line.isEmpty())continue;if(boundaries){bool ok=false;const double v=line.toDouble(&ok);require(ok&&std::isfinite(v),"Filter STK boundary time not finite");++boundaryCount;}else if(data){const auto cols=line.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(cols.size()==7,"Filter STK Cartesian row width differs");for(const auto&c:cols){bool ok=false;const auto v=c.toDouble(&ok);require(ok&&std::isfinite(v),"Filter STK Cartesian row not finite");}++points;}}
   require(points>10&&boundaryCount>1,"Supported filter STK lacks trajectory or measurement event boundaries");rig.save("clones.json",QJsonDocument(clones).toJson());
   auto*output=rig.window.findChild<QTreeWidget*>("Output");require(output,"Filter STK Output tree missing");for(auto*tabs:rig.window.findChildren<QTabWidget*>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)==output||tabs->widget(i)->isAncestorOf(output))tabs->setCurrentIndex(i);auto found=output->findItems("Export",Qt::MatchExactly|Qt::MatchRecursive);require(found.size()==1,"Filter STK Output item missing");output->scrollToItem(found.first());events();reveal(output);auto point=output->visualItemRect(found.first()).center();QTest::mouseClick(output->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();QTest::mouseDClick(output->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();auto views=rig.window.findChildren<QPlainTextEdit*>("report:Export");require(!views.isEmpty()&&views.last()->isReadOnly()&&views.last()->toPlainText()==QString(payload).replace("\r\n","\n"),"Supported filter STK viewer differs from complete file");acceptanceView(*rig.area->currentSubWindow());
  }
  const QJsonObject proof{{"checkpoint","filter-format-"+key},{"outcome","passed"},{"family",spec["family"]},{"format",spec["format"]},{"allowed",allowed},{"run_mode",runMode},{"diagnostic_checked",!allowed},{"clones",clones.size()},{"points",points},{"event_boundaries",boundaryCount},{"forward_filter_setup",smoother&&spec["format"]=="Code-500"},{"covariance_components",36},{"simulation_sentinel",11},{"continuation_sentinel",allowed?99:0},{"facets",QJsonArray{"actual-format-popup-write-and-dependent-controls","pending-cancel-apply-clean-undo-redo-unicode-isolation","normal-startup-native-format-guard-or-supported-stk-control","completed-simulator-and-mission-continuation-or-stop-membership","exact-native-diagnostic-or-clone-terminal-report-and-full-output-viewer"}},{"scope",fixture["scope"]}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException&e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
