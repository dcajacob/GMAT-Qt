#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "Debugger.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include "CoordinateConverter.hpp"
#include "CoordinateSystem.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QPointer>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <set>
using namespace formaccept;

class StreamObserver final : public Subscriber {
public:
   std::function<void(const Real *,Integer)> sample;
   std::function<void()> flush;
   StreamObserver():Subscriber("AcceptanceStream","AcceptanceStream"){}
   ~StreamObserver()override{Publisher::Instance()->Unsubscribe(this);}
   GmatBase *Clone()const override{return new StreamObserver(*this);}
   bool ReceiveData(const Real *v,Integer n=0)override{if(v&&n>=7&&sample)sample(v,n);return true;}
   bool FlushData(bool end=true)override{if(flush)flush();return Subscriber::FlushData(end);}
protected:
   bool Distribute(Integer)override{return true;}
   bool Distribute(const Real *,Integer)override{return true;}
};
struct Sample {double epoch;std::array<double,6> state;double longitude,latitude;int pass;bool accepted;};
static void reveal(QWidget *control) {
   require(control,"Iteration control absent");
   for(auto *p=control;p;p=p->parentWidget()) {
      for(auto *tabs:control->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(control);
   }
   events();
   if(!control->isVisible()||!control->isEnabled()) {
      QStringList chain;for(auto *p=control;p;p=p->parentWidget())chain<<p->metaObject()->className()+QString(":")+p->objectName()+QString(" visible=%1 enabled=%2").arg(p->isVisible()).arg(p->isEnabled());
      throw std::runtime_error(("Iteration control inaccessible: "+chain.join(" -> ")).toStdString());
   }
   acceptanceView(*control);
}
static void choose(QComboBox *choice,const QString &mode) {
   reveal(choice);const int index=choice->findText(mode);require(index>=0,"Iteration mode unavailable");
   choice->setFocus();QTest::keyClick(choice,Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(choice,Qt::Key_Down);QTest::keyClick(choice,Qt::Key_Tab);events();require(choice->currentText()==mode,"Keyboard iteration selection differs");
}
static void edit(Rig &rig,const QString &name,const QString &kind,const QString &mode) {
   const auto before=rig.source->toPlainText();auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());const auto original=object->GetGeneratingString(Gmat::SCRIPTING);
   auto items=rig.resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Iteration resource tree item ambiguous");rig.resources->itemDoubleClicked(items.first(),0);events();
   auto *panel=dynamic_cast<ResourceEditor *>(rig.area->activeSubWindow()->widget());require(panel,"Iteration resource form absent");
   const auto field=kind=="Orb"?"orbit_SolverIterations":kind=="Map"?"groundSolverIterations":"xySolverIterations";
   choose(panel->findChild<QComboBox *>(field),mode);
   require(panel->hasChanges()&&rig.source->toPlainText()==before&&object->GetGeneratingString(Gmat::SCRIPTING)==original,"Iteration draft escaped Cancel");
   QPointer<ResourceEditor> cancelled=panel;auto *cancel=panel->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Cancel);reveal(cancel);QTest::mouseClick(cancel,Qt::LeftButton);events();
   require(!cancelled&&rig.source->toPlainText()==before&&object->GetGeneratingString(Gmat::SCRIPTING)==original,"Cancelled iteration setup changed source/native state or retained owner");
   items=rig.resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Iteration resource absent after Cancel");rig.resources->itemDoubleClicked(items.first(),0);events();panel=dynamic_cast<ResourceEditor *>(rig.area->activeSubWindow()->widget());require(panel&&!panel->hasChanges(),"Cancelled iteration setup reopened dirty");choose(panel->findChild<QComboBox *>(field),mode);
   require(panel->hasChanges()&&rig.source->toPlainText()==before&&object->GetGeneratingString(Gmat::SCRIPTING)==original,"Iteration draft escaped Apply");
   auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");auto *apply=buttons?buttons->button(QDialogButtonBox::Apply):nullptr;reveal(apply);QTest::mouseClick(apply,Qt::LeftButton);events();require(!panel->hasChanges(),"Iteration Apply failed");
   const auto applied=rig.source->toPlainText();require(applied.mid(applied.indexOf("BeginMissionSequence;"))==before.mid(before.indexOf("BeginMissionSequence;")),"Iteration Apply changed mission");rig.closePanels();
   rig.source->undo();require(rig.source->toPlainText()==before&&rig.window.buildScript(),"Iteration exact Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Iteration exact Redo failed");
}
static std::vector<SolverBranchCommand *> solvers() {
   std::vector<SolverBranchCommand *> result;std::set<GmatCommand *> seen;std::vector<GmatCommand *> todo{Moderator::Instance()->GetFirstCommand()};
   while(!todo.empty()){auto *c=todo.back();todo.pop_back();if(!c||!seen.insert(c).second)continue;if(auto *s=dynamic_cast<SolverBranchCommand *>(c))result.push_back(s);todo.push_back(c->GetNext());for(int i=0;auto *child=c->GetChildCommand(i);++i)todo.push_back(child);}
   return result;
}
static const PlotCurve &curve(const PlotModel &model) {
   for(const auto &c:model.curves)if(model.kind==PlotModel::Kind::XY||c.name=="Sat")return c;
   throw std::runtime_error("Iteration spacecraft curve absent");
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaSolverPublication");
   try {
      require(argc==3,"Supply startup and DC/Yukon/Nested/Mixed");const QString solver=argv[2];require(QStringList{"DC","Yukon","Nested","Mixed"}.contains(solver),"Unknown solver publication case");const bool optimizer=solver=="Yukon"||solver=="Mixed",nested=solver=="Nested"||solver=="Mixed";Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      QString script="Create Variable Alpha Beta Cost Keep;\nAlpha = 0;\nBeta = 0;\nCost = 0;\nKeep = 41;\nCreate Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+number(std::sqrt(mu/7000))+";\nSat.VZ = 0;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 5;\nP.MinStep = 5;\nP.MaxStep = 5;\nP.Accuracy = 0.000000000001;\nCreate DifferentialCorrector DC Inner;\nDC.MaximumIterations = 15;\nInner.MaximumIterations = 15;\nDC.ReportFile = '"+rig.files.filePath("dc.txt")+"';\nInner.ReportFile = '"+rig.files.filePath("inner.txt")+"';\nCreate Yukon Opt;\nOpt.MaximumIterations = 20;\nOpt.ShowProgress = false;\nOpt.UseCentralDifferences = true;\nOpt.FunctionTolerance = 0.0000000001;\nOpt.OptimalityTolerance = 0.0000000001;\nOpt.ReportFile = '"+rig.files.filePath("opt.txt")+"';\n";
      QStringList names;const QStringList kinds{"Orb","Map","XY"},modes{"All","Current","None"};
      for(const auto &kind:kinds)for(const auto &mode:modes) {
         const auto name=kind+mode;names<<name;script+="Create "+QString(kind=="Orb"?"OrbitView":kind=="Map"?"GroundTrackPlot":"XYPlot")+" "+name+";\n";
         if(kind=="XY")script+=name+".XVariable = Sat.A1ModJulian;\n"+name+".YVariables = {Sat.EarthMJ2000Eq.X};\n";
         else script+=name+".Add = {Sat};\n"+name+".DataCollectFrequency = 1;\n"+name+".UpdatePlotFrequency = 1;\n";
         if(kind=="Orb")script+=name+".CoordinateSystem = EarthMJ2000Eq;\n";
         script+=name+".SolverIterations = "+(mode=="All"?"None":"All")+";\n";
      }
      script+="Create ReportFile Values;\nValues.Filename = '"+rig.report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nValues.SolverIterations = All;\nBeginMissionSequence;\nReport 'Arm' Values Keep;\nPropagate 'Before' P(Sat) {Sat.ElapsedSecs = 10};\n";
      script+=optimizer?"Optimize 'Outer' Opt {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary Opt(Alpha = 0, {Perturbation = 0.001, Lower = -5, Upper = 5, MaxStep = 10});\n":"Target 'Outer' DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary DC(Alpha = 0, {Perturbation = 0.01, Lower = -5, Upper = 5, MaxStep = 10});\n";
      if(nested)script+="Target 'Inner' Inner {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary Inner(Beta = 0, {Perturbation = 0.01, Lower = -5, Upper = 5, MaxStep = 10});\n";
      script+="Sat.X = 7000 + 100 * Alpha;\nSat.Y = 20 + 100 * Beta;\nSat.Z = 0;\nSat.VX = 0.2;\nSat.VY = 7.2;\nSat.VZ = 0;\nReport 'Pass enter' Values Alpha Beta Keep;\nPropagate 'Trial piece' P(Sat) {Sat.ElapsedSecs = 10};\nReport 'Pass result' Values Alpha Beta Sat.A1ModJulian Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Keep;\n";
      if(nested)script+="Achieve Inner(Beta = 3, {Tolerance = 0.00000001});\nEndTarget;\n";
      script+=optimizer?"Cost = (Alpha - 2)^2;\nMinimize Opt(Cost);\nEndOptimize;\n":"Achieve DC(Alpha = 2, {Tolerance = 0.00000001});\nEndTarget;\n";
      script+="Report 'Regular after' Values Alpha Beta Keep;\nPropagate 'After' P(Sat) {Sat.ElapsedSecs = 10};\nReport 'Final' Values Alpha Beta Cost Keep;\n% preserve iteration membership Ω\n";
      rig.source->setPlainText(script);require(rig.window.buildScript(),"Publication fixture build failed");rig.source->document()->setModified(false);
      for(const auto &kind:kinds)for(const auto &mode:modes)edit(rig,kind+mode,kind,mode);
      const auto applied=rig.source->toPlainText();const auto saved=rig.files.filePath("iteration membership Ω.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Publication Unicode reopen failed");
      for(const auto &kind:kinds)for(const auto &mode:modes)require(Moderator::Instance()->GetConfiguredObject((kind+mode).toStdString())->GetStringParameter("SolverIterations")==mode.toStdString(),"Reopened native iteration mode differs");
      Debugger *debug=nullptr;for(auto *o:rig.window.children())if(auto *d=dynamic_cast<Debugger *>(o))debug=d;require(debug,"Publication debugger absent");
      QMap<int,QString> boundaries;const auto snap=rig.window.missionSnapshot();for(int i=0;i<snap.nodes.size();++i)for(const auto &label:QStringList{"Arm","Pass enter","Regular after"})if(snap.nodes[i].statement.startsWith("Report '")&&snap.nodes[i].statement.contains("'"+label+"'")){require(rig.window.setBreakpoint(i,true),"Publication breakpoint failed");boundaries[i]=label;}require(boundaries.size()==3,"Publication boundaries incomplete");
      StreamObserver observer;QVector<Sample> all;int pass=0,flushes=0,trials=0,acceptedPasses=0,nestedLocalAcceptedTrials=0;bool armed=false,accepted=true,regular=true;QJsonArray passes,observations;std::exception_ptr failure;
      auto check=[&](const QString &boundary,bool flushed) {
         QJsonObject entry{{"boundary",boundary},{"pass",pass},{"raw_samples",all.size()},{"accepted",accepted}};
         for(const auto &kind:kinds)for(const auto &mode:modes) {
            const auto name=kind+mode;auto model=rig.window.plotReceiver()->model(name);require(bool(model),"Publication viewer model absent");const auto &points=curve(*model).points;
            if(kind=="Orb"&&mode=="Current"&&!flushed)continue;
            QVector<const Sample *> expected;
            for(const auto &s:all)if(mode=="All"||s.accepted||(mode=="Current"&&s.pass==pass&&!s.accepted))expected<<&s;
            entry[name]=QJsonObject{{"actual",int(points.size())},{"expected",expected.size()}};observations.append(entry);rig.save("live-membership-diagnostics.json",QJsonDocument(observations).toJson());
            if(points.size()!=size_t(expected.size()))throw std::runtime_error((name+" live membership count differs at "+boundary+": actual "+QString::number(points.size())+", expected "+QString::number(expected.size())).toStdString());
            for(int i=0;i<expected.size();++i) {
               const auto &s=*expected[i];const auto &p=points[i];
               const double x=kind=="Orb"?s.state[0]:kind=="Map"?s.longitude:s.epoch,y=kind=="Orb"?s.state[1]:kind=="Map"?s.latitude:s.state[0],z=kind=="Orb"?s.state[2]:0;
               require(std::abs(p.x-x)<(kind=="Map"?2e-6:2e-8)&&std::abs(p.y-y)<(kind=="Map"?2e-6:2e-8)&&std::abs(p.z-z)<2e-8,"Live iteration content/order differs from independent raw publication");
               if(kind!="XY")require(std::abs(p.epoch-s.epoch)<1e-11&&p.solver==!s.accepted,"Live iteration epoch/trial classification differs from numerical solver state oracle");
            }
         }
         events();
      };
      observer.sample=[&](const Real *data,Integer count){if(failure)return;try{
         require(armed&&count>=7,"Unexpected live publication");Sample s{data[0],{},0,0,pass,accepted};for(int i=0;i<6;++i){require(std::isfinite(data[i+1]),"Nonfinite solver publication");s.state[i]=data[i+1];}
         auto *from=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("EarthMJ2000Eq")),*to=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("EarthFixed"));require(from&&to,"Ground membership reference frames absent");Rvector6 input(data[1],data[2],data[3],data[4],data[5],data[6]),fixed;CoordinateConverter converter;require(converter.Convert(A1Mjd(data[0]),input,from,fixed,to),"Ground reference conversion failed");
         const double pi=std::acos(-1.),r=std::hypot(fixed[0],fixed[1]);auto *earth=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth");const double a=earth->GetEquatorialRadius(),f=earth->GetFlattening(),b=a*(1-f),e2=f*(2-f),theta=std::atan2(fixed[2]*a,r*b);
         s.longitude=std::atan2(fixed[1],fixed[0])*180/pi;s.latitude=std::atan2(fixed[2]+e2/(1-e2)*b*std::pow(std::sin(theta),3),r-e2*a*std::pow(std::cos(theta),3))*180/pi;
         all<<s;check("publication",false);
      }catch(...){failure=std::current_exception();QTimer::singleShot(0,&rig.window,[&]{rig.window.stopMission();});}};
      observer.flush=[&]{if(!armed||failure||all.isEmpty())return;try{++flushes;check("native-flush",true);}catch(...){failure=std::current_exception();QTimer::singleShot(0,&rig.window,[&]{rig.window.stopMission();});}};
      auto previous=debug->paused;struct Restore{Debugger *d;std::function<void(const QString &)> p;~Restore(){d->paused=p;}}restore{debug,previous};
      debug->paused=[&](const QString &type){if(previous)previous(type);if(failure){QTimer::singleShot(0,&rig.window,[&]{rig.window.stopMission();debug->resume();});return;}try{
         auto *dialog=rig.window.findChild<QDialog *>("debuggerDialog");require(dialog&&type=="Report","Publication command boundary invalid");const auto label=boundaries.value(dialog->property("commandIndex").toInt());require(!label.isEmpty(),"Unknown publication breakpoint");
         if(label=="Arm") {require(!armed&&Publisher::Instance()->Subscribe(&observer),"Publication observer attach failed");armed=true;rig.area->tileSubWindows();}
         else if(label=="Pass enter") {
            ++pass;regular=false;accepted=true;QJsonArray states;auto commands=solvers();require(commands.size()==size_t(nested?2:1),"Actual solver clone inventory differs");bool localFinished=false;
            for(auto *command:commands){auto *clone=dynamic_cast<Solver *>(command->GetClone(0));require(clone,"Numerical solver clone absent");const int state=clone->GetState();states.append(QJsonObject{{"solver",QString::fromStdString(clone->GetName())},{"state",state}});accepted=accepted&&state==Solver::FINISHED;if(clone->GetName()=="Inner")localFinished=state==Solver::FINISHED;}
            if(accepted)++acceptedPasses;else ++trials;if(localFinished&&!accepted)++nestedLocalAcceptedTrials;
            passes.append(QJsonObject{{"pass",pass},{"states",states},{"accepted",accepted},{"publisher_state",int(Publisher::Instance()->GetRunState())}});rig.save("solver-pass-states.json",QJsonDocument(passes).toJson());
         } else {regular=true;accepted=true;pass=0;}
         acceptanceView(*dialog);
      }catch(...){if(!failure)failure=std::current_exception();QTimer::singleShot(0,&rig.window,[&]{rig.window.stopMission();});}QTimer::singleShot(0,&rig.window,[&]{debug->resume();});};
      const int undo=rig.source->document()->availableUndoSteps();const auto status=rig.window.runMission();debug->paused=previous;Publisher::Instance()->Unsubscribe(&observer);if(failure)std::rethrow_exception(failure);require(status==MainWindow::RunResult::Completed&&regular&&trials>=2&&acceptedPasses==1&&flushes>=pass,"Solver live membership matrix incomplete");if(nested)require(nestedLocalAcceptedTrials>0,"Nested inner accepted/outer trial partition unexercised");check("completed",true);
      auto *alpha=Moderator::Instance()->GetInternalObject("Alpha"),*beta=Moderator::Instance()->GetInternalObject("Beta"),*cost=Moderator::Instance()->GetInternalObject("Cost");require(alpha&&beta&&cost&&std::abs(alpha->GetRealParameter("Value")-2)<1e-6,"Solver result differs from independent scalar goal");if(nested)require(std::abs(beta->GetRealParameter("Value")-3)<1e-6,"Nested result differs from independent scalar goal");if(optimizer)require(std::abs(cost->GetRealParameter("Value"))<1e-10,"Quadratic optimum differs");
      require(rig.source->toPlainText()==applied&&rig.source->document()->availableUndoSteps()==undo&&read(saved)==applied.toUtf8(),"Iteration Run changed source/history/file");rig.save("iteration.script",applied.toUtf8());rig.save("iteration.csv",read(rig.report));QJsonArray raw;for(const auto &s:all){QJsonArray state;for(auto x:s.state)state.append(x);raw.append(QJsonObject{{"epoch",s.epoch},{"state",state},{"longitude",s.longitude},{"latitude",s.latitude},{"pass",s.pass},{"accepted",s.accepted}});}rig.save("raw-publications.json",QJsonDocument(raw).toJson());
      const QJsonObject proof{{"checkpoint","native-live-solver-membership"},{"outcome","passed"},{"solver",solver},{"plots",9},{"raw_count",all.size()},{"trial_passes",trials},{"accepted_passes",acceptedPasses},{"nested_inner_accepted_outer_trial",nestedLocalAcceptedTrials},{"flush_observations",flushes},{"passes",passes},{"facets",QJsonArray{"actual-resource-mode-pending-cancel-apply-undo-redo-unicode-reopen","unfiltered-raw-publication-membership-and-order","numerical-solver-clone-state-accepted-oracle","all-current-none-transient-membership-at-native-flush","independent-scalar-goals-and-source-report-preservation"}}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
