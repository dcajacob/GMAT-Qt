#include "Vf13adAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "Debugger.hpp"
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabBar>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
using namespace vfaccept;

static const QString numberPattern="([-+0-9.eE]+)";
static double field(const QString &text,const QString &prefix)
{
   const QRegularExpression re("^"+QRegularExpression::escape(prefix)+numberPattern+"$",QRegularExpression::MultilineOption);
   auto matches=re.globalMatch(text);require(matches.hasNext(),"Iteration field missing");
   const auto match=matches.next();bool ok=false;const double value=match.captured(1).toDouble(&ok);
   require(ok&&std::isfinite(value)&&!matches.hasNext(),"Iteration field nonfinite or duplicated");return value;
}
static QJsonArray frames(QString text)
{
   text.replace("\r\n","\n");QJsonArray result;
   const QRegularExpression re("VF13ad iteration result ([0-9]+)\\n(.*?)End VF13ad iteration result \\1(?:\\n|$)",QRegularExpression::DotMatchesEverythingOption);
   auto matches=re.globalMatch(text);
   while(matches.hasNext()) {
      const auto m=matches.next();const auto body=m.captured(2);QJsonObject row{{"iteration",m.captured(1).toInt()}};
      row["x"]=field(body,"   Control X = ");row["y"]=field(body,"   Control Y = ");
      row["scaled_x"]=field(body,"   Scaled control X = ");row["scaled_y"]=field(body,"   Scaled control Y = ");
      row["cost"]=field(body,"   Objective Cost = ");QJsonArray constraints;
      const QRegularExpression constraint("^   Constraint ([A-Za-z0-9_]+) (=|<=|>=); desired = "+numberPattern+"; achieved = "+numberPattern+"; residual = "+numberPattern+"$",QRegularExpression::MultilineOption);
      auto found=constraint.globalMatch(body);
      while(found.hasNext()) {const auto c=found.next();QJsonObject value{{"name",c.captured(1)},{"operator",c.captured(2)}};
         for(int i=3;i<=5;++i){bool ok=false;const double v=c.captured(i).toDouble(&ok);require(ok&&std::isfinite(v),"Constraint result nonfinite");value[QStringList{"desired","achieved","residual"}[i-3]]=v;}constraints.append(value);}
      require(constraints.size()==3&&body.count("   Control ")==2&&body.count("   Objective ")==1&&body.count("   Constraint ")==3,"Iteration schema cardinality differs");
      row["constraints"]=constraints;result.append(row);
   }
   require(text.count("VF13ad iteration result ")==2*result.size(),"Incomplete iteration block");return result;
}
static Solver *runtime()
{
   auto *c=Moderator::Instance()->GetFirstCommand();while(c&&!c->IsOfType("Optimize"))c=c->GetNext();
   auto *branch=dynamic_cast<SolverBranchCommand *>(c);auto *solver=branch?dynamic_cast<Solver *>(branch->GetClone(0)):nullptr;
   require(solver&&solver->GetTypeName()=="VF13ad","Live VF13ad clone absent");return solver;
}
static void near(double actual,double expected,double tolerance=1e-8)
{if(!std::isfinite(actual)||std::abs(actual-expected)>tolerance)throw std::runtime_error(QString("Progress value differs: actual %1 expected %2 tolerance %3").arg(actual,0,'g',17).arg(expected,0,'g',17).arg(tolerance,0,'g',17).toStdString());}
static void checkFrame(const QJsonObject &row)
{
   const double x=row["x"].toDouble(),y=row["y"].toDouble();near(row["cost"].toDouble(),(x-3)*(x-3)+(y+2)*(y+2));
   near(row["scaled_x"].toDouble(),.2*(x+5));near(row["scaled_y"].toDouble(),3*(y-4));
   const QMap<QString,QPair<QString,QPair<double,double>>> expected={
      {"Upper",{"<=",{7,x+5}}},{"Lower",{">=",{1,2*x-1}}},{"EqValue",{"=",{2,y+3}}}};
   QSet<QString> seen;
   for(const auto &entry:row["constraints"].toArray()) {const auto c=entry.toObject();const auto name=c["name"].toString();require(expected.contains(name)&&!seen.contains(name),"Constraint identity differs");seen.insert(name);
      const auto want=expected[name];require(c["operator"].toString()==want.first,"Constraint operator differs");near(c["desired"].toDouble(),want.second.first);near(c["achieved"].toDouble(),want.second.second);
      near(c["residual"].toDouble(),name=="Lower"?want.second.first-want.second.second:want.second.second-want.second.first);}
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaVf13adProgress");
   try {
      require(argc==3,"Supply startup and Forward/Central");const QString mode=argv[2];require(mode=="Forward"||mode=="Central","Unknown reporting mode");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto unused=rig.files.filePath("unused.txt");
      QString script="Create VF13ad Opt;\nCreate Variable X Y Upper Lower EqValue Cost Keep;\nX = 0;\nY = 0;\nKeep = 41;\nOpt.MaximumIterations = 100;\nOpt.ShowProgress = false;\nOpt.ReportStyle = Normal;\nOpt.ReportFile = '"+unused+"';\nOpt.UseCentralDifferences = "+QString(mode=="Central"?"true":"false")+";\nOpt.Tolerance = 1e-10;\nOpt.FeasibilityTolerance = 1e-10;\nCreate ReportFile Trials Final;\nTrials.Filename = '"+rig.trials+"';\nFinal.Filename = '"+rig.final+"';\n";
      for(const auto &name:QStringList{"Trials","Final"})script+=name+".WriteHeaders = false;\n"+name+".FixedWidth = false;\n"+name+".Delimiter = ',';\n"+name+".Precision = 25;\n"+name+".SolverIterations = All;\n";
      script+="BeginMissionSequence;\nOptimize 'Report contract' Opt {ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary Opt(X = 0, {Perturbation = .01, Lower = -10, Upper = 10, MaxStep = .5, AdditiveScaleFactor = 5, MultiplicativeScaleFactor = .2});\nVary Opt(Y = 0, {Perturbation = .01, Lower = -10, Upper = 10, MaxStep = .5, AdditiveScaleFactor = -4, MultiplicativeScaleFactor = 3});\nUpper = X + 5;\nLower = 2*X - 1;\nEqValue = Y + 3;\nCost = (X - 3)^2 + (Y + 2)^2;\nReport Trials X Y Upper Lower EqValue Cost Keep;\nNonlinearConstraint Opt(Upper <= 7);\nNonlinearConstraint Opt(Lower >= 1);\nNonlinearConstraint Opt(EqValue = 2);\nMinimize Opt(Cost);\nEndOptimize;\nReport Final X Y Upper Lower EqValue Cost Keep;\n";
      rig.source->setPlainText(script);require(rig.window.buildScript(),"Progress fixture did not build");
      QJsonArray phases;
      for(bool enabled:{true,false}) {
         const QString phase=enabled?"enabled":"disabled";const auto reportPath=rig.files.filePath(phase+" report Ω.txt");const auto before=rig.source->toPlainText();
         auto items=rig.resources->findItems("Opt",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Optimizer resource absent");rig.resources->expandAll();rig.resources->scrollToItem(items[0]);events();
         const auto position=rig.resources->visualItemRect(items[0]).center();QTest::mouseClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,position);QTest::mouseDClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,position);QTest::mouseRelease(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,position);events();
         auto *child=rig.area->activeSubWindow();auto *panel=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(panel,"Actual VF13ad resource editor absent");
         auto control=[&](const QString &name){auto *v=panel->findChild<QWidget *>("resource_"+name);require(v&&v->isEnabled(),"Progress input absent/disabled");for(auto *tabs:panel->findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->widget(i)->isAncestorOf(v))tabs->setCurrentIndex(i);for(auto *p=v->parentWidget();p;p=p->parentWidget())if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(v);events();require(v->isVisible(),"Progress input hidden");acceptanceView(*v);return v;};
         auto choose=[&](const QString &name,const QString &value){auto *combo=qobject_cast<QComboBox *>(control(name));require(combo&&combo->findText(value)>=0,"Progress choice absent");combo->showPopup();QTest::keyClick(combo,Qt::Key_Home);for(int i=0;i<combo->findText(value);++i)QTest::keyClick(combo,Qt::Key_Down);QTest::keyClick(combo,Qt::Key_Return);require(combo->currentText()==value,"Progress choice not delivered");};
         choose("ShowProgress",enabled?"true":"false");choose("ReportStyle","Verbose");auto *path=qobject_cast<QLineEdit *>(control("ReportFile"));require(path&&!path->isReadOnly(),"Report filename cannot be typed");QApplication::clipboard()->setText(reportPath);QTest::mouseClick(path,Qt::LeftButton);QTest::keyClick(path,Qt::Key_A,Qt::ControlModifier);QTest::keyClick(path,Qt::Key_V,Qt::ControlModifier);require(path->text()==reportPath&&rig.source->toPlainText()==before&&panel->hasChanges(),"Progress draft escaped pending isolation");
         auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");require(buttons,"Progress Apply absent");acceptanceView(*child);QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();require(!panel->hasChanges()&&rig.source->toPlainText()!=before,"Progress Apply failed");const auto applied=rig.source->toPlainText();rig.closePanels();rig.source->undo();require(rig.source->toPlainText()==before&&rig.window.buildScript(),"Progress Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Progress Redo failed");
         const auto saved=rig.files.filePath(phase+" mission Ω.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Progress Unicode roundtrip failed");
         auto *configured=Moderator::Instance()->GetConfiguredObject("Opt");require(configured->GetBooleanParameter("ShowProgress")==enabled&&configured->GetStringParameter("ReportStyle")=="Verbose"&&QString::fromStdString(configured->GetStringParameter("ReportFile"))==reportPath,"Progress GUI/native settings differ");
         auto *messages=rig.window.findChild<QPlainTextEdit *>("messageWindow");require(messages,"Message window absent");messages->clear();QJsonArray live,nominal;std::exception_ptr failure;
         // Debugger boundaries allow the production queued message delivery to
         // run during the mission, and independently sample the nominal native
         // controls before Report Trials. No solver callback is replaced.
         const auto snapshot=rig.window.missionSnapshot();int breakpoints=0;
         for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].statement.startsWith("Report Trials")||snapshot.nodes[i].statement.startsWith("Report Final")){require(rig.window.setBreakpoint(i,true),"Progress observation boundary unavailable");++breakpoints;}
         require(breakpoints==2,"Progress observation boundary count differs");Debugger *debugger=nullptr;for(auto *o:rig.window.children())if(auto *d=dynamic_cast<Debugger *>(o))debugger=d;require(debugger,"Progress observer debugger absent");
         auto connection=QObject::connect(messages,&QPlainTextEdit::textChanged,&rig.window,[&]{if(failure)return;try{const auto data=frames(messages->toPlainText());while(live.size()<data.size()){auto row=data[live.size()].toObject();row["delivered_while_running"]=rig.window.isRunning();live.append(row);}}catch(...){failure=std::current_exception();}});
         QTimer observer,timeout;observer.setInterval(1);timeout.setSingleShot(true);timeout.setInterval(20000);
         QObject::connect(&observer,&QTimer::timeout,&rig.window,[&]{if(!debugger->isWaiting())return;try{auto *solver=runtime();if(solver->GetState()==Solver::NOMINAL){const double x=Moderator::Instance()->GetInternalObject("X")->GetRealParameter("Value"),y=Moderator::Instance()->GetInternalObject("Y")->GetRealParameter("Value");near(solver->GetSolverVariable(0),.2*(x+5));near(solver->GetSolverVariable(1),3*(y-4));near(Moderator::Instance()->GetInternalObject("Cost")->GetRealParameter("Value"),(x-3)*(x-3)+(y+2)*(y+2));nominal.append(QJsonObject{{"x",x},{"y",y},{"native_x",solver->GetSolverVariable(0)},{"native_y",solver->GetSolverVariable(1)}});}rig.window.resumeMission();}catch(...){failure=std::current_exception();rig.window.stopMission();}});
         QObject::connect(&timeout,&QTimer::timeout,&rig.window,[&]{failure=std::make_exception_ptr(std::runtime_error("Progress observation timed out"));rig.window.stopMission();});
         observer.start();timeout.start();const auto status=rig.window.runMission();observer.stop();timeout.stop();events();QObject::disconnect(connection);
         const bool reportExists=QFileInfo::exists(reportPath);const auto reportData=reportExists?read(reportPath):QByteArray();require(!enabled||(reportExists&&!reportData.isEmpty()),"Enabled report file absent");
         const auto messageText=messages->toPlainText();rig.save(phase+".messages.txt",messageText.toUtf8());if(reportExists)rig.save(phase+".optimizer.txt",reportData);else rig.save(phase+".report-absent.json","{\"report_file_exists\":false}\n");rig.save(phase+".trials.csv",read(rig.trials));rig.save(phase+".final.csv",read(rig.final));rig.save(phase+".script",applied.toUtf8());rig.save(phase+".live.json",QJsonDocument(live).toJson());
         require(status==MainWindow::RunResult::Completed,"Progress mission failed");if(failure)std::rethrow_exception(failure);
         const auto messageFrames=frames(messageText),fileFrames=frames(QString::fromUtf8(reportData));require(messageFrames==fileFrames&&messageFrames.size()==live.size(),"Message/file/live iteration frames differ");
         for(const auto &entry:QString::fromUtf8(read(rig.trials)).trimmed().split('\n')){const auto values=entry.split(',');require(values.size()==7,"Progress trial columns differ");QVector<double> v;for(const auto &s:values){bool ok=false;double d=s.toDouble(&ok);require(ok&&std::isfinite(d),"Progress trial nonfinite");v.append(d);}near(v[2],v[0]+5);near(v[3],2*v[0]-1);near(v[4],v[1]+3);near(v[5],(v[0]-3)*(v[0]-3)+(v[1]+2)*(v[1]+2));near(v[6],41);}
         const auto final=QString::fromUtf8(read(rig.final)).trimmed().split(',');require(final.size()==7,"Progress final shape differs");near(final[0].toDouble(),2,1e-6);near(final[1].toDouble(),-1,1e-6);near(final[5].toDouble(),2,2e-6);near(final[6].toDouble(),41);
         require(runtime()->GetIntegerParameter(runtime()->GetParameterID("IntegerSolverStatus"))==Gmat::CONVERGED,"Progress solver not converged");near(runtime()->GetSolverVariable(0),.2*(final[0].toDouble()+5),1e-7);near(runtime()->GetSolverVariable(1),3*(final[1].toDouble()-4),1e-7);
         if(enabled){require(live.size()>=3&&!nominal.isEmpty(),"Per-iteration progress absent");int previous=-1;for(const auto &entry:live){const auto row=entry.toObject();checkFrame(row);require(row["iteration"].toInt()==previous+1,"Native iteration sequence missing/duplicated");previous=row["iteration"].toInt();require(row["delivered_while_running"].toBool(),"Progress was delivered only after mission completion");bool found=false;for(const auto &sample:nominal){const auto n=sample.toObject();if(std::abs(n["x"].toDouble()-row["x"].toDouble())<1e-8&&std::abs(n["y"].toDouble()-row["y"].toDouble())<1e-8){near(row["scaled_x"].toDouble(),n["native_x"].toDouble());near(row["scaled_y"].toDouble(),n["native_y"].toDouble());found=true;break;}}require(found,"Report frame has no independent nominal native witness");}near(live.last().toObject()["x"].toDouble(),2,1e-6);near(live.last().toObject()["y"].toDouble(),-1,1e-6);}
         if(enabled){const auto completed=QRegularExpression("Optimization Completed in ([0-9]+) iterations").match(QString::fromUtf8(reportData));require(completed.hasMatch()&&completed.captured(1).toInt()==live.last().toObject()["iteration"].toInt(),"Progress iteration count differs from native completion report");near(live.first().toObject()["x"].toDouble(),0);near(live.first().toObject()["y"].toDouble(),0);}
         else require(live.isEmpty(),"ShowProgress=false published iteration results");
         require(rig.source->toPlainText()==applied&&read(saved)==applied.toUtf8(),"Progress changed source/file");near(Moderator::Instance()->GetConfiguredObject("X")->GetRealParameter("Value"),0);near(Moderator::Instance()->GetConfiguredObject("Y")->GetRealParameter("Value"),0);
         if(auto *dock=rig.window.findChild<QDockWidget *>("messages")){dock->show();acceptanceView(*messages);}phases.append(QJsonObject{{"enabled",enabled},{"report_file_exists",reportExists},{"frames",live},{"independent_nominal_samples",nominal},{"optimum",QJsonArray{final[0].toDouble(),final[1].toDouble(),final[5].toDouble()}},{"source_and_configured_graph_unchanged",true}});
      }
      const QJsonObject proof{{"checkpoint","vf13ad-progress-schema"},{"outcome","passed"},{"mode",mode},{"phases",phases},{"operators",QJsonArray{"=","<=",">="}},{"facets",QJsonArray{"actual-resource-progress-style-unicode-file-input-and-history","live-native-nominal-decision-vector-and-every-iteration","independent-derived-equality-lower-upper-achieved-desired-residuals","independent-scaled-two-control-cost-and-optimum","identical-message-file-results-and-disabled-gating","source-configured-graph-preservation"}}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
