#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "QtPlotReceiver.hpp"
#include "IntegrationStepSampler.hpp"
#include "Debugger.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include "PlotStepLiveObservation.hpp"
#include "PlotStepCadenceObservation.hpp"
#include "PlotStepBurnOracle.hpp"
#include "PlotStepEphemerisOracle.hpp"
#include "PlotStepLifetimeOracle.hpp"
#include "PlotStepRedrawOracle.hpp"
#include "PlotStepBoundaryOracle.hpp"
#include "PlotStepFunctionOracle.hpp"
#include "PlotStepOriginOracle.hpp"
#include "PlotStepRotatingOracle.hpp"
#include "PlotStepLacOracle.hpp"
#include "PlotStepGravityOracle.hpp"
#include <memory>
#include <QTimer>
#include <QDialog>
#include <set>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QTest>
#include <limits>
using namespace formaccept;
namespace {
void samplerContract() {
 Spacecraft one("A"),two("B");IntegrationStepSampler sampler;
 auto select=[&](Spacecraft &sat,double t,bool initial,int skip,double delta=0.) {
  Publisher::IntegrationStep step;step.initial=initial;step.startSeconds=t-1;step.endSeconds=t;
  step.epoch=GmatTime(21545);step.epoch.AddSeconds(t);
  Publisher::CartesianSample sample{&sat,step.epoch,Rvector6(t+delta,2,3,4,5,6)};
  return sampler.Select(step,sample,skip);
 };
 require(select(one,0,true,1)&&!select(one,1,false,1),"Sampler initial/first skip differs");
 require(!select(one,1,true,1)&&select(two,0,true,1)&&select(one,2,false,1),"Repeated initial or other trajectory consumed cadence");
 sampler.Action("SolverScope","outer");sampler.Action("MarkBreak","");
 require(!select(one,3,false,1)&&select(one,4,false,1),"Sampler trial cadence differs");
 sampler.Action("ClearFromBreak","-1");
 require(!select(one,3,false,1)&&select(one,4,false,1),"Sampler rollback failed to restore cadence");
 require(select(one,4,true,1,10)&&!select(one,5,false,1),"Same-epoch assignment did not begin a new arc");
 sampler.Reset();const int largest=(std::numeric_limits<int>::max)();
 require(select(one,0,true,largest),"INT_MAX initial rejected");
 for(int i=1;i<100;++i)require(!select(one,i,false,largest),"INT_MAX countdown overflowed");
 sampler.Reset();require(select(one,0,true,0)&&select(one,-1,false,0)&&!select(one,-1,true,0)&&select(one,-1.25,false,0),"Zero skip/backward/partial step differs");
 Publisher::IntegrationStep zero;zero.epoch=GmatTime(21545);Publisher::CartesianSample sample{&one,zero.epoch,Rvector6()};
 require(!sampler.Select(zero,sample,0),"Zero advance consumed cadence");
 sampler.Reset();require(select(one,0,true,1)&&!select(one,10,false,1),"Rounded initial setup failed");
 Publisher::IntegrationStep rounded;rounded.initial=true;rounded.epoch=GmatTime(21545.+10./86400.);
 Publisher::CartesianSample roundedSample{&one,rounded.epoch,Rvector6(10,2,3,4,5,6)};
 require(!sampler.Select(rounded,roundedSample,1)&&select(one,20,false,1),"Real-MJD command initial changed native-step cadence");
}
std::array<double,6> translatedOrbit(int n,double seconds) {
 const double radius=1500.+500.*n, inclination=std::acos(-1.)/6., angle=.7+.2*n+std::sqrt(1000./(radius*radius*radius))*seconds, speed=std::sqrt(1000./radius);
 return {radius*std::cos(angle),radius*std::sin(angle)*std::cos(inclination),radius*std::sin(angle)*std::sin(inclination),-speed*std::sin(angle),speed*std::cos(angle)*std::cos(inclination),speed*std::cos(angle)*std::sin(inclination)};
}
QString config(const QString &report,int skip,bool multi,bool formation) {
 QString text="Create Spacecraft One Two;\n";const QStringList fields{"X","Y","Z","VX","VY","VZ"};
 for(int n=0;n<2;++n) {
  const auto &o=orbits[n];text+=o.name+".DateFormat = A1ModJulian;\n"+o.name+".Epoch = '21545';\n"+o.name+".CoordinateSystem = EarthMJ2000Eq;\n"+o.name+".DisplayStateType = Cartesian;\n";
  const auto state=circular(o,0);for(int c=0;c<6;++c)text+=o.name+"."+fields[c]+" = "+number(state[c])+";\n";
 }
 if(formation)text+="Create Formation Fleet;\nFleet.Add = {Two, One};\n";
 text+="Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\nP.Type = RungeKutta89;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nP.Accuracy = 1e-10;\n";
 if(multi)text+="Create Propagator Q;\nQ.FM = FM;\nQ.Type = RungeKutta89;\nQ.InitialStepSize = 3;\nQ.MinStep = 3;\nQ.MaxStep = 3;\nQ.Accuracy = 1e-10;\n";
 text+="Create OrbitView Orb;\nOrb.CoordinateSystem = EarthMJ2000Eq;\nCreate GroundTrackPlot Map;\n";
 for(const auto &plot:QStringList{"Orb","Map"})text+=plot+".Add = {One, Two};\n"+plot+".DataCollectFrequency = 2;\n"+plot+".IntegrationStepsToSkip = "+QString::number(skip)+";\n"+plot+".UpdatePlotFrequency = 2;\n"+plot+".MaxPlotPoints = 1000;\n";
 text+="Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nBeginMissionSequence;\n";return text;
}
void editInteger(Rig &rig,const QString &name,const QString &field,int value) {
 const auto before=rig.source->toPlainText();const QString prefix=name=="Orb"?"orbit_":"ground_";
 auto matches=rig.resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Plot resource missing");rig.resources->itemDoubleClicked(matches[0],0);events();
 auto *child=rig.area->activeSubWindow();auto *panel=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(panel,"Plot primary form missing");
 auto *spin=panel->findChild<QSpinBox *>(prefix+field);require(spin,"Explicit skip control missing");
 for(QWidget *p=spin;p;p=p->parentWidget()) {for(auto *tabs:panel->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(spin);}
 events();require(spin->isVisible()&&spin->isEnabled(),"Skip control inaccessible");acceptanceView(*spin);
 spin->setFocus();QTest::keyClick(spin,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(spin,QString::number(value));QTest::keyClick(spin,Qt::Key_Tab);events();
 require(spin->value()==value&&panel->hasChanges()&&rig.source->toPlainText()==before,"Skip typed draft isolation failed");
 panel->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Apply)->click();events();require(!panel->hasChanges(),"Skip Apply failed");rig.closePanels();
 const auto applied=rig.source->toPlainText();rig.source->undo();require(rig.source->toPlainText()==before&&rig.window.buildScript(),"Skip Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Skip Redo failed");
 require(Moderator::Instance()->GetConfiguredObject(name.toStdString())->GetIntegerParameter(field.toStdString())==value,"Skip native value differs");
}
}
int main(int argc,char **argv) {
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("PlotIntegrationSteps");
 try {
  require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());samplerContract();
  struct Case {QString name,mission;int skip;QVector<double> one,two;bool multi=false,formation=false;QString solverMode;int retained=1000;};
  std::vector<Case> cases{
   {"ForwardSplit","Propagate P(One) {One.ElapsedSecs = 10};\nPropagate P(One) {One.ElapsedSecs = 27};\n",1,{0,20,37},{}},
   {"BackwardSplit","Propagate BackProp P(One) {One.ElapsedSecs = -10};\nPropagate BackProp P(One) {One.ElapsedSecs = -27};\n",1,{0,-20,-37},{}},
   {"SingleZero","Propagate P(One);\nPropagate P(One);\n",0,{0,10,20},{}},
   {"IntMax","Propagate P(One) {One.ElapsedSecs = 37};\n",(std::numeric_limits<int>::max)(),{0},{}},
   {"Providers","Propagate P(One) {One.ElapsedSecs = 10};\nPropagate Q(Two) {Two.ElapsedSecs = 9};\nPropagate P(One) {One.ElapsedSecs = 27};\n",1,{0,20,37},{0,6},true},
   {"Formation","Propagate P(Fleet) {One.ElapsedSecs = 25};\n",1,{0,20},{0,20},false,true},
   {"Synchronized","Propagate Synchronized P(One) Q(Two) {One.ElapsedSecs = 23};\n",1,{0,20},{0,6,10,16,20},true}
  };
  for(const auto &mode:QStringList{"All","Current","None"})cases.push_back({"Solver"+mode,"Propagate P(One) {One.ElapsedSecs = 10};\nTarget DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary DC(Alpha = 0, {Perturbation = 0.01, Lower = -5, Upper = 5, MaxStep = 10});\nReport 'Count solver pass' Values Alpha;\nPropagate P(One) {One.ElapsedSecs = 27};\nAchieve DC(Alpha = 2, {Tolerance = 0.00000001});\nEndTarget;\nPropagate P(One) {One.ElapsedSecs = 13};\n",1,{0,20,37,50},{},false,false,mode});
  for(const auto &kind:QStringList{"Optimizer","Nested","Mixed"})for(const auto &mode:QStringList{"All","Current","None"}) {
   const bool optimize=kind!="Nested",nested=kind!="Optimizer";
   QString mission="Propagate P(One) {One.ElapsedSecs = 10};\n";
   mission+=optimize?"Optimize Opt {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary Opt(Alpha = 0, {Perturbation = 0.001, Lower = -5, Upper = 5, MaxStep = 10});\n":"Target DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary DC(Alpha = 0, {Perturbation = 0.01, Lower = -5, Upper = 5, MaxStep = 10});\n";
   if(nested)mission+="Target Inner {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary Inner(Beta = 0, {Perturbation = 0.01, Lower = -5, Upper = 5, MaxStep = 10});\n";
   mission+="Report 'Count solver pass' Values Alpha;\nPropagate P(One) {One.ElapsedSecs = 27};\n";
   if(nested)mission+="Achieve Inner(Beta = 3, {Tolerance = 0.00000001});\nEndTarget;\n";
   mission+=optimize?"Cost = (Alpha - 2)^2;\nMinimize Opt(Cost);\nEndOptimize;\n":"Achieve DC(Alpha = 2, {Tolerance = 0.00000001});\nEndTarget;\n";
   mission+="Propagate P(One) {One.ElapsedSecs = 13};\n";
   cases.push_back({kind+mode,mission,1,{0,20,37,50},{},false,false,mode});
  }
  const auto originalCases=cases;
  for(auto test:originalCases)if(!test.solverMode.isEmpty()) {
   test.name="Live"+test.name;
   test.mission="Report 'Arm live observer' Values Alpha;\n"+test.mission;
   test.mission.replace("Propagate P(One) {One.ElapsedSecs = 13};", "Report 'Regular after solver' Values Alpha;\nPropagate P(One) {One.ElapsedSecs = 13};");
   cases.push_back(test);
  }
  for(auto test:originalCases)if(!test.solverMode.isEmpty()) {
   test.name="Paint"+test.name;
   test.retained=6;
   test.one={1000,1200,1400,1600,1800,2000};
   test.mission.replace("ElapsedSecs = 10}", "ElapsedSecs = 400}");
   test.mission.replace("ElapsedSecs = 27};", "ElapsedSecs = 800};\nReport 'Inspect solver paint' Values Alpha;");
   test.mission.replace("ElapsedSecs = 13}", "ElapsedSecs = 800}");
   // A completed For block issues a native publisher flush while the first
   // trial is still active. Inspect the buffered and flushed Current displays.
   if(test.solverMode=="Current")test.mission.replace("Report 'Inspect solver paint' Values Alpha;",
      "For PaintIndex = 1:1;\nReport 'Inspect solver paint' Values Alpha;\nEndFor;");
   cases.push_back(test);
  }
  const auto liveCases=cases;
  for(auto test:liveCases)if(test.name.startsWith("Live")) {test.name="Retain2"+test.name;test.retained=2;cases.push_back(test);}
  cases.push_back({"TranslatedOrigin","Propagate P(One) {One.ElapsedSecs = 10};\nPropagate P(One) {One.ElapsedSecs = 27};\n",1,{0,20,37},{}});
  for(double stop:{37.123,37.25}) {
   QVector<double> expected{0};for(int t=2;t<=36;t+=2)expected<<t;expected<<stop;
   cases.push_back({stop==37.123?"NonlinearABM":"NonlinearABMQuarter","Propagate P(One) {One.EarthMJ2000Eq.X = "+number(circular(orbits[0],stop)[0])+", StopTolerance = 1e-10};\n",1,expected,{}});
  }
  cases.push_back({"CadenceRetention","Report 'Arm cadence observer' Values One.X;\nPropagate P(One) {One.ElapsedSecs = 10};\nPropagate P(One) {One.ElapsedSecs = 590};\n",1,{420,440,460,480,500,520,540,560,580,600},{}});
  for(const auto &name:QStringList{"BurnChemicalFixed","BurnChemicalMass","BurnElectricFixed","BurnElectricMass","BurnChemicalRestart","BurnChemicalZero","BurnChemicalNonlinear"}) {
   const bool zero=name.endsWith("Zero");
   cases.push_back({name,plotburn::mission(plotburn::settings(name)),zero?0:1,zero?QVector<double>{0,10,20,30,37,47,50}:QVector<double>{0,20,plotburn::settings(name).stop(),plotburn::settings(name).stop()+13},{}});
  }
  for(const auto &name:QStringList{"EphemType9Forward","EphemType13Backward","EphemType13Zero","EphemStkForward","EphemStkBackward","EphemOemForward","EphemOemBackward","EphemCode500LittleForward","EphemCode500BigBackward"}) {
   const bool back=name.endsWith("Backward"),zero=name.endsWith("Zero");const QString direction=back?"BackProp ":"",sign=back?"-":"";
   cases.push_back({name,"Report 'Arm ephemeris observer' Values One.X;\nPropagate "+direction+"P(One) {One.ElapsedSecs = "+sign+"10};\nPropagate "+direction+"P(One) {One.ElapsedSecs = "+sign+"27};\n",zero?0:1,zero?QVector<double>{0,10,20,30,37}:back?QVector<double>{0,-20,-37}:QVector<double>{0,20,37},{}});
  }
  for(const auto &name:QStringList{"EphemType13Mixed","EphemStkMixed","EphemOemMixed","EphemCode500Mixed"})cases.push_back({name, "Report 'Arm ephemeris observer' Values One.X;\nPropagate Synchronized P(One) Q(Two) {One.ElapsedSecs = 23};\n", 1, {0,20}, {0,6,10,16,20}, true});
  cases.push_back({"LifetimeToggleGap", "Propagate P(One) {One.ElapsedSecs = 20};\nToggle Orb Map Off;\nPropagate P(One) {One.ElapsedSecs = 10};\nToggle Orb Map On;\nPropagate P(One) {One.ElapsedSecs = 27};\n",1,{0,20,30,50},{}});
  cases.push_back({"LifetimeInitiallyOff", "Toggle Orb Map Off;\nPropagate P(One) {One.ElapsedSecs = 10};\nToggle Orb Map On;\nPropagate P(One) {One.ElapsedSecs = 27};\n",1,{10,30},{}});
  cases.push_back({"LifetimeFinishOff", "Propagate P(One) {One.ElapsedSecs = 20};\nToggle Orb Map Off;\nPropagate P(One) {One.ElapsedSecs = 10};\n",1,{0,20},{}});
  for (int boundary : {10, 20}) {
   const auto name = boundary == 10 ? QString("ResetSkipped") : QString("ResetCollected");
   QString mission = QString("Propagate P(One) {One.ElapsedSecs = %1};\n").arg(boundary);
   const QStringList fields{"X","Y","Z","VX","VY","VZ"};
   const auto after = translatedOrbit(0,boundary+2000);
   for (int c=0;c<6;++c) mission += "One.ProbeEq." + fields[c] + " = " + number(after[c]) + ";\n";
   mission += "Propagate P(One) {One.ElapsedSecs = 23};\n";
   const QVector<double> grid = boundary == 10 ? QVector<double>{0,10,30} : QVector<double>{0,20,20,40};
   cases.push_back({name,mission,1,grid,{}});
   if (boundary==10)cases.push_back({"LegacyChangedState",mission,1,grid,{}});
   if (boundary == 20) {
    mission.replace("P(One)","P(One, Two)");
    cases.push_back({"ResetPeer",mission,1,grid,{0,20,40}});
   }
  }
  for(const auto &name:QStringList{"RedrawLastTwo","RedrawAll","RedrawRetained"}) {
   cases.push_back({name,"Propagate P(One) {One.ElapsedSecs = 1000};\nReport 'Inspect live redraw' Values One.ElapsedSecs;\n",9,{0,100,200,300,400,500,600,700,800,900,1000},{},false,false,{},name=="RedrawRetained"?4:1000});
  }
  cases.push_back({"LegacyEveryPacket","Propagate P(One) {One.ElapsedSecs = 10};\nPropagate P(One) {One.ElapsedSecs = 27};\n",0,{0,10,20,30,37},{}});
  cases.push_back({"LegacyReverseBoundary","Propagate P(One) {One.ElapsedSecs = 10};\nPropagate BackProp P(One) {One.ElapsedSecs = -20};\n",1,{0,0},{}});
  for(const auto &name:QStringList{"FunctionShadow","FunctionGlobalTwice","FunctionNested","FunctionLocalPlot"}) {
   const QVector<double> epochs=(name=="FunctionShadow"||name=="FunctionLocalPlot")?QVector<double>{0,20,37}:name=="FunctionGlobalTwice"?QVector<double>{0,20,37,57,74}:QVector<double>{0,20,40,57};
   cases.push_back({name,plotfunction::mission(name),1,epochs,{}});
  }
  for(const auto &name:QStringList{"FrameFollowerForward","FrameFollowerBackward","FrameLeaderMissing"}) {
   const bool back=name.endsWith("Backward");const QString dir=back?"BackProp ":"",sign=back?"-":"";
   QVector<double> leader{0,10,20,23},follower{0,3,6,9,10,13,16,19,20,23};
   if(back){for(auto &v:leader)v=-v;for(auto &v:follower)v=-v;}
   cases.push_back({name,"Propagate "+dir+"Synchronized P(One) Q(Two) {One.ElapsedSecs = "+sign+"23};\n",0,leader,follower,true});
  }
  cases.push_back({"FrameLeaderThinned","Propagate Synchronized P(One) Q(Two) {One.ElapsedSecs = 23};\n",3,{0},{0,10,20},true});
  cases.push_back({"FrameLiveOrigin","Propagate P(One) {One.ElapsedSecs = 23};\n",0,{0,10,20,23},{}});
  QString jump="Propagate Synchronized P(One) Q(Two) {One.ElapsedSecs = 10};\nPropagate Q(Two) {Two.ElapsedSecs = 6};\n";
  const auto newOrbit=circular(orbits[0],2010);
  const QStringList fields{"X","Y","Z","VX","VY","VZ"};
  for(int c=0;c<6;++c)jump+="One.EarthMJ2000Eq."+fields[c]+" = "+number(newOrbit[c])+";\n";
  jump+="Propagate P(One) {One.ElapsedSecs = 10};\n";
  cases.push_back({"FrameNewArcMissing",jump,0,{0,10,10,20},{0,3,6,9,10,13,16},true});
  for(const auto &name:QStringList{"RotatingOriginForward","RotatingOriginBackward","RotatingEarthForward","RotatingEarthBackward","RotatingVelocityForward","RotatingVelocityBackward","RotatingPairForward","RotatingPairBackward"}) {
   const bool back=name.endsWith("Backward");
   QVector<double> leader{0,10,20,23},follower{0,3,6,9,10,13,16,19,20,23};
   if(back){for(auto &v:leader)v=-v;for(auto &v:follower)v=-v;}
   cases.push_back({name,"Propagate "+QString(back?"BackProp ":"")+"Synchronized P(One) Q(Two) {One.ElapsedSecs = "+QString(back?"-":"")+"23};\n",0,leader,follower,true});
  }
  cases.push_back({"RotatingPairThinned","Propagate Synchronized P(One) Q(Two) {One.ElapsedSecs = 23};\n",3,{0},{0,10,20},true});
  for(const auto &name:QStringList{"RotatingOriginGravityForward","RotatingOriginGravityBackward","RotatingEarthGravityForward","RotatingEarthGravityBackward","RotatingVelocityGravityForward","RotatingVelocityGravityBackward","RotatingPairGravityForward","RotatingPairGravityBackward"}) {
   const bool back=name.endsWith("Backward");
   QVector<double> leader{0,10,20,23},follower{0,3,6,9,10,13,16,19,20,23};
   if(back){for(auto &v:leader)v=-v;for(auto &v:follower)v=-v;}
   cases.push_back({name,"Propagate "+QString(back?"BackProp ":"")+"Synchronized P(One) Q(Two) {One.ElapsedSecs = "+QString(back?"-":"")+"23};\n",0,leader,follower,true});
  }
  cases.push_back({"RotatingPairGravityThinned","Propagate Synchronized P(One) Q(Two) {One.ElapsedSecs = 23};\n",3,{0},{0,10,20},true});
  for(const auto &name:QStringList{"LACReferenceForward","LACReferenceBackward","LACConstraintForward","LACConstraintBackward"}) {
   const bool back=name.endsWith("Backward");
   QVector<double> leader{0,10,20,23},follower{0,3,6,9,10,13,16,19,20,23};
   if(back){for(auto &v:leader)v=-v;for(auto &v:follower)v=-v;}
   cases.push_back({name,"Propagate "+QString(back?"BackProp ":"")+"Synchronized P(One) Q(Two) {One.ElapsedSecs = "+QString(back?"-":"")+"23};\n",0,leader,follower,true});
  }
  cases.push_back({"LACConstraintThinned","Propagate Synchronized P(One) Q(Two) {One.ElapsedSecs = 23};\n",3,{0},{0,10,20},true});
  cases.push_back({"LACLiveReference","Propagate P(One) {One.ElapsedSecs = 23};\n",0,{0,10,20,23},{}});
  QJsonArray completed;const auto filter=qEnvironmentVariable("GMAT_PLOT_STEP_CASE");
  for(const auto &test:cases) {
   if(!filter.isEmpty()&&!filter.split(',').contains(test.name))continue;
   const bool live=test.name.startsWith("Live")||test.name.startsWith("Retain2Live");const bool paint=test.name.startsWith("Paint");const QString kind=paint?test.name.mid(5):live?test.name.mid(test.name.indexOf("Live")+4):test.name;
   const bool lac=test.name.startsWith("LAC"),lacNested=test.name.startsWith("LACConstraint");
   const bool rotating=test.name.startsWith("Rotating");
   const bool gravity=test.name.contains("Gravity");
   const bool rotatingPair=test.name.startsWith("RotatingPair");
   const bool rotatingVelocity=test.name.startsWith("RotatingVelocity");
   const bool rotatingOrigin=test.name.startsWith("RotatingOrigin")||rotatingPair||rotatingVelocity;
   const bool originFrame=test.name.startsWith("Frame");
   const auto originChecks=test.name=="FrameFollowerForward"?plotorigin::protocol():QJsonArray{};
   const bool leaderOrigin=test.name.startsWith("FrameLeader");
   const bool function=test.name.startsWith("Function");
   const bool redraw=test.name.startsWith("Redraw");
   const int redrawLimit=test.name=="RedrawAll"?0:2;
   const bool legacy=test.name.startsWith("Legacy");
   const auto boundaryProtocol=test.name=="LegacyEveryPacket"?plotboundary::checkPacketScope():QJsonArray{};
   const bool resetArc=test.name.startsWith("Reset")||test.name=="LegacyChangedState";
   const bool lifetime=test.name.startsWith("Lifetime");
   const bool ephem=test.name.startsWith("Ephem");
   const bool burn=test.name.startsWith("Burn");const auto burnSettings=plotburn::settings(test.name);
   auto setup=config(rig.report,-1,test.multi,test.formation);
   if(gravity)setup=plotgravity::configure(setup);
   if(originFrame)setup.replace("Orb.CoordinateSystem = EarthMJ2000Eq;", "Create CoordinateSystem RelativeFrame;\nRelativeFrame.Origin = "+QString(leaderOrigin?"One":"Two")+";\nRelativeFrame.Axes = MJ2000Eq;\nOrb.CoordinateSystem = RelativeFrame;");
   if(rotating)setup.replace("Orb.CoordinateSystem = EarthMJ2000Eq;", "Create CoordinateSystem RotatingFrame;\nRotatingFrame.Origin = "+QString(rotatingOrigin?"Two":"Earth")+";\nRotatingFrame.Axes = ObjectReferenced;\nRotatingFrame.Primary = Earth;\nRotatingFrame.Secondary = Two;\nRotatingFrame.XAxis = R;\nRotatingFrame.ZAxis = N;\nOrb.CoordinateSystem = RotatingFrame;");
   if(rotatingPair)setup.replace("RotatingFrame.Primary = Earth;", "RotatingFrame.Primary = One;");
   if(rotatingVelocity)setup.replace("RotatingFrame.XAxis = R;", "RotatingFrame.XAxis = V;");
   if(lac)setup=plotlac::configure(setup,lacNested);
   if(function)setup=plotfunction::configure(setup,rig,test.name);
   if(legacy)setup.replace("Map.DataCollectFrequency = 2;", "Map.DataCollectFrequency = 4;");
   if(paint&&test.solverMode=="Current")setup.replace("BeginMissionSequence;", "Create Variable PaintIndex;\nBeginMissionSequence;");
   if(paint)for(const auto &field:QStringList{"InitialStepSize","MinStep","MaxStep"})setup.replace("P."+field+" = 10;", "P."+field+" = 100;");
   if(redraw||paint)setup.replace("BeginMissionSequence;", "Orb.NumPointsToRedraw = 3;\nMap.NumPointsToRedraw = 3;\nBeginMissionSequence;");
   if(burn)setup=plotburn::configure(setup,burnSettings);
   if(ephem)setup=plotephem::configure(setup,rig,test.name);
   if(test.name.startsWith("NonlinearABM")) {
    setup.replace("P.Type = RungeKutta89;", "P.Type = AdamsBashforthMoulton;");
    for(const auto &field:QStringList{"InitialStepSize","MinStep","MaxStep"})setup.replace("P."+field+" = 10;", "P."+field+" = 1;");
   }
   if(test.name=="TranslatedOrigin"||resetArc) {
    const auto kernel=QFileInfo("../data/planetary_ephem/spk/ceres_1900_2100.bsp").canonicalFilePath();require(!kernel.isEmpty(),"Translated-origin shared ephemeris kernel missing");
    const QString body="Create Asteroid Probe;\nProbe.NAIFId = 2000001;\nProbe.OrbitSpiceKernelName = {'"+kernel+"'};\nProbe.Mu = 1000;\nProbe.EquatorialRadius = 1000;\nProbe.Flattening = 0;\nProbe.OrientationEpoch = 21545;\nProbe.SpinAxisRAConstant = -90;\nProbe.SpinAxisRARate = 0;\nProbe.SpinAxisDECConstant = 90;\nProbe.SpinAxisDECRate = 0;\nProbe.RotationConstant = 0;\nProbe.RotationRate = 0;\nCreate CoordinateSystem ProbeEq;\nProbeEq.Origin = Probe;\nProbeEq.Axes = MJ2000Eq;\n";
    setup.replace(".CoordinateSystem = EarthMJ2000Eq;", ".CoordinateSystem = ProbeEq;");
    setup.replace("FM.PrimaryBodies = {};", "FM.CentralBody = Probe;\nFM.PrimaryBodies = {};");setup.replace("FM.PointMasses = {Earth};", "FM.PointMasses = {Probe};");
    setup.replace("Create GroundTrackPlot Map;", "Create GroundTrackPlot Map;\nMap.CentralBody = Probe;");
    const QStringList fields{"X","Y","Z","VX","VY","VZ"};for(int n=0;n<2;++n)for(int c=0;c<6;++c)setup.replace(orbits[n].name+"."+fields[c]+" = "+number(circular(orbits[n],0)[c])+";", orbits[n].name+"."+fields[c]+" = "+number(translatedOrbit(n,0)[c])+";");
    setup=body+setup;
   }
   if(!test.solverMode.isEmpty())setup.replace("BeginMissionSequence;", "Create Variable Alpha;\nAlpha = 0;\nCreate DifferentialCorrector DC;\nDC.MaximumIterations = 15;\nDC.ReportFile = '"+rig.files.filePath("solver.txt")+"';\nOrb.SolverIterations = "+test.solverMode+";\nMap.SolverIterations = "+test.solverMode+";\nBeginMissionSequence;");
   if(kind.startsWith("Optimizer")||kind.startsWith("Nested")||kind.startsWith("Mixed")) {
    setup.replace("BeginMissionSequence;", "Create Variable Beta Cost;\nBeta = 0;\nCost = 0;\nCreate DifferentialCorrector Inner;\nInner.MaximumIterations = 15;\nInner.ReportFile = '"+rig.files.filePath("inner.txt")+"';\nCreate Yukon Opt;\nOpt.MaximumIterations = 20;\nOpt.ShowProgress = false;\nOpt.UseCentralDifferences = true;\nOpt.FunctionTolerance = 1e-10;\nOpt.OptimalityTolerance = 1e-10;\nOpt.ReportFile = '"+rig.files.filePath("opt.txt")+"';\nBeginMissionSequence;");
   }
   const auto script=setup+test.mission+(function?QString():lifetime?plotlifetime::reportCommand():burn?QString():QString("Report Values One.A1ModJulian Two.A1ModJulian;\n"));
   rig.source->setPlainText(script);require(rig.window.buildScript(),"Plot-step fixture build failed");editInteger(rig,"Orb","IntegrationStepsToSkip",test.skip);
   if(legacy)editInteger(rig,"Map","DataCollectFrequency",test.name=="LegacyEveryPacket"?1:2);
   else editInteger(rig,"Map","IntegrationStepsToSkip",test.skip);
   if(legacy)require(Moderator::Instance()->GetConfiguredObject("Map")->GetIntegerParameter("IntegrationStepsToSkip")==-1,"Legacy interval selected explicit mode");
   if(redraw||paint)for(const auto &plot:QStringList{"Orb","Map"})editInteger(rig,plot,"NumPointsToRedraw",redrawLimit);
   if(test.name=="CadenceRetention")for(const auto &name:QStringList{"Orb","Map"}) {editInteger(rig,name,"UpdatePlotFrequency",name=="Orb"?3:4);editInteger(rig,name,"MaxPlotPoints",10);}
   if(test.retained!=1000)for(const auto &name:QStringList{"Orb","Map"})editInteger(rig,name,"MaxPlotPoints",test.retained);
   const auto applied=rig.source->toPlainText();const auto saved=rig.files.filePath("step Ω.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Skip Unicode save/reload failed");rig.save(test.name+".script",applied.toUtf8());
   QJsonArray livePixels,finalPixels,flushPixels;
   std::unique_ptr<PlotStepStream> paintObserver;
   int trials=0,acceptedPasses=0;QJsonArray solverStates;Debugger *debug=nullptr;std::function<void(const QString&)> prior;std::exception_ptr observerFailure;
   std::unique_ptr<PlotStepCadence> cadence;QJsonObject cadenceProof;
   if(test.name=="CadenceRetention") {
    cadence=std::make_unique<PlotStepCadence>(rig);
    for(auto *child:rig.window.children())if(auto *candidate=dynamic_cast<Debugger *>(child))debug=candidate;
    require(debug,"Step cadence debugger missing");int arm=-1;const auto snapshot=rig.window.missionSnapshot();for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].statement.contains("Arm cadence observer"))arm=i;
    require(arm>=0&&rig.window.setBreakpoint(arm,true),"Step cadence arm boundary missing");prior=debug->paused;
    debug->paused=[&](const QString &type){if(prior)prior(type);cadence->arm();QTimer::singleShot(0,&rig.window,[&]{debug->resume();});};
   }
   std::unique_ptr<PlotStepMembership> membership;QJsonObject liveProof;
   if(live)membership=std::make_unique<PlotStepMembership>(rig,test.name,test.solverMode,test.retained);
   if(!test.solverMode.isEmpty()) {
    for(auto *child:rig.window.children())if(auto *candidate=dynamic_cast<Debugger *>(child))debug=candidate;
    require(debug,"Solver-step debugger missing");int arm=-1;const auto snapshot=rig.window.missionSnapshot();for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].statement.contains("Count solver pass"))arm=i;
    require(arm>=0&&rig.window.setBreakpoint(arm,true),"Solver-step pass boundary absent");prior=debug->paused;
    QMap<int,QString> liveBoundaries;
    if(live)for(int i=0;i<snapshot.nodes.size();++i)for(const auto &label:QStringList{"Arm live observer","Regular after solver"})
     if(snapshot.nodes[i].statement.contains(label)){require(rig.window.setBreakpoint(i,true),"Live solver boundary missing");liveBoundaries[i]=label;}
    if(paint) {
     for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].type=="Report"&&snapshot.nodes[i].statement.contains("Inspect solver paint")) {
      require(rig.window.setBreakpoint(i,true),"Solver paint breakpoint missing");liveBoundaries[i]="paint";
     }
     require(liveBoundaries.size()==1,"Solver paint boundary absent");
    }
    if(live)require(liveBoundaries.size()==2,"Live solver boundaries incomplete");
    debug->paused=[&,liveBoundaries](const QString &commandKind){if(prior)prior(commandKind);try {
     require(commandKind=="Report","Solver-step unexpected breakpoint");
     auto *dialog=rig.window.findChild<QDialog *>("debuggerDialog");
     require(dialog,"Solver-step debugger dialog missing");
     const auto boundary=liveBoundaries.value(dialog->property("commandIndex").toInt());
     if(boundary=="Arm live observer")membership->arm();
     else if(boundary=="Regular after solver")membership->regularAfter();
     else if(boundary=="paint") {
      if(trials==1&&acceptedPasses==0&&livePixels.isEmpty()) {
       livePixels=plotredraw::inspect(rig,test.name,redrawLimit,test.retained,false,test.solverMode);
       if(test.solverMode=="Current") {
        paintObserver=std::make_unique<PlotStepStream>();
        paintObserver->flush=[&] {
         if(!flushPixels.isEmpty()||observerFailure)return;
         try {
          require(trials==1&&acceptedPasses==0,"First trial not flushed before next pass");
          flushPixels=plotredraw::inspect(rig,test.name,redrawLimit,test.retained,false,test.solverMode,true);
         } catch(...) {observerFailure=std::current_exception();}
        };
        require(Publisher::Instance()->Subscribe(paintObserver.get()),"Solver flush observer attach failed");
       }
      }
     }
     else {
     std::set<GmatCommand *> visited;std::vector<GmatCommand *> todo{Moderator::Instance()->GetFirstCommand()};QJsonArray states;bool accepted=true;
     while(!todo.empty()) {
      auto *c=todo.back();todo.pop_back();if(!c||!visited.insert(c).second)continue;
      if(auto *branch=dynamic_cast<SolverBranchCommand *>(c)) {auto *solver=dynamic_cast<Solver *>(branch->GetClone(0));require(solver,"Solver-step native clone absent");const int state=solver->GetState();states.append(state);accepted=accepted&&state==Solver::FINISHED;}
      todo.push_back(c->GetNextWhileExecuting());for(int i=0;auto *child=c->GetChildCommand(i);++i)todo.push_back(child);
     }
     const bool nested=kind.startsWith("Nested")||kind.startsWith("Mixed");require(states.size()==(nested?2:1),"Solver-step native clone count differs");
     if(accepted)++acceptedPasses;else ++trials;
     if(nested)solverStates.append(states);else solverStates.append(states[0]);
     if(membership)membership->enterPass(accepted);
     }
    }catch(...){observerFailure=std::current_exception();rig.window.stopMission();}QTimer::singleShot(0,&rig.window,[&]{debug->resume();});};
   }
   std::unique_ptr<PlotStepStream> ephemStream;QJsonArray ephemRecords;
   if(ephem) {
    ephemStream=std::make_unique<PlotStepStream>();
    ephemStream->sample=[&](const Real *data,Integer count) {
     if(const auto *steps=Publisher::GetPublishedIntegrationSteps(data,count))for(const auto &step:*steps) {
      QJsonArray objects;for(const auto &sample:step.states)objects.append(QString::fromStdString(sample.object->GetName()));
      QJsonObject endpoint={{"initial",step.initial},{"ephemeris",step.ephemeris},{"method",QString::fromStdString(step.method)},{"elapsed",double((step.epoch-GmatTime(21545)).GetTimeInSec())},{"start",step.startSeconds},{"end",step.endSeconds},{"objects",objects}};
      if(plotephem::provider(test.name)!="SPK") {
       QJsonArray states;
       for(const auto &sample:step.states) {
        QJsonArray components;
        for(int i=0;i<6;++i)components.append(sample.state[i]);
        states.append(components);
       }
       endpoint["states"]=states;
      }
      ephemRecords.append(endpoint);
     }
    };
    for(auto *child:rig.window.children())if(auto *candidate=dynamic_cast<Debugger *>(child))debug=candidate;
    require(debug,"Ephemeris debugger missing");int arm=-1;const auto snapshot=rig.window.missionSnapshot();for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].statement.contains("Arm ephemeris observer"))arm=i;
    require(arm>=0&&rig.window.setBreakpoint(arm,true),"Ephemeris observation boundary missing");prior=debug->paused;
    debug->paused=[&](const QString &type){if(prior)prior(type);if(!Publisher::Instance()->Subscribe(ephemStream.get()))observerFailure=std::make_exception_ptr(std::runtime_error("Ephemeris observer attach failed"));QTimer::singleShot(0,&rig.window,[&]{debug->resume();});};
   }

   if(redraw) {
    for(auto *child:rig.window.children())if(auto *candidate=dynamic_cast<Debugger *>(child))debug=candidate;
    int inspect=-1;const auto snapshot=rig.window.missionSnapshot();
    for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].statement.contains("Inspect live redraw"))inspect=i;
    require(debug&&inspect>=0&&rig.window.setBreakpoint(inspect,true),"Live redraw boundary missing");
    prior=debug->paused;
    debug->paused=[&](const QString &kind) {
     if(prior)prior(kind);
     try {livePixels=plotredraw::inspect(rig,test.name,redrawLimit,test.retained,false);}
     catch(...) {observerFailure=std::current_exception();rig.window.stopMission();}
     QTimer::singleShot(0,&rig.window,[&]{debug->resume();});
    };
   }
   std::unique_ptr<plotfunction::Observer> fnObserver;
   QJsonArray functionPackets;
   if(function) {
    fnObserver=std::make_unique<plotfunction::Observer>(rig,test.name=="FunctionLocalPlot");
    for(auto *child:rig.window.children())if(auto *candidate=dynamic_cast<Debugger *>(child))debug=candidate;
    int arm=-1;const auto snapshot=rig.window.missionSnapshot();
    for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].statement.contains("Arm function observer"))arm=i;
    require(debug&&arm>=0&&rig.window.setBreakpoint(arm,true),"Function observer boundary missing");prior=debug->paused;
    debug->paused=[&](const QString &type){if(prior)prior(type);if(!Publisher::Instance()->Subscribe(fnObserver.get()))observerFailure=std::make_exception_ptr(std::runtime_error("Function observer attach failed"));QTimer::singleShot(0,&rig.window,[&]{debug->resume();});};
   }
   QJsonArray lifetimeRuns;
   if(lifetime) {
    const auto firstStatus=rig.window.runMission();events();
    rig.save(test.name+"-first-messages.txt",rig.messages->toPlainText().toUtf8());
    require(firstStatus==MainWindow::RunResult::Completed,"First lifetime run failed");
    lifetimeRuns.append(plotlifetime::inspect(rig,test.name,test.one));
    require(rig.source->toPlainText()==applied,"First lifetime run modified source");
   }
   const QString messagesBeforeOrigin=(originFrame||rotating||lac)?rig.messages->toPlainText():QString();
   const auto status=rig.window.runMission();if(debug)debug->paused=prior;events();
   if(fnObserver) {
    functionPackets=fnObserver->records;
    rig.save(test.name+"-packets.json",QJsonDocument(functionPackets).toJson());
    Publisher::Instance()->Unsubscribe(fnObserver.get());
    QJsonArray viewers;
    for(const auto &plot:QStringList{"Orb","Map"})if(const auto model=rig.window.plotReceiver()->model(plot))for(const auto &curve:model->curves) {
     QJsonArray epochs;for(const auto &pt:curve.points)epochs.append((pt.epoch-21545)*86400);
     viewers.append(QJsonObject{{"plot",plot},{"object",curve.name},{"epochs",epochs}});
    }
    rig.save(test.name+"-viewer-epochs.json",QJsonDocument(viewers).toJson());
   }
   if(redraw&&status==MainWindow::RunResult::Completed&&!observerFailure)finalPixels=plotredraw::inspect(rig,test.name,redrawLimit,test.retained,true);
   if(paint&&status==MainWindow::RunResult::Completed&&!observerFailure) {
    if(test.solverMode=="Current")require(flushPixels.size()==2,"Missing native Current flush framebuffer observations");
    require(livePixels.size()==2,"Missing first-trial framebuffer observations");
    finalPixels=plotredraw::inspect(rig,test.name,redrawLimit,test.retained,true,test.solverMode);
   }
   if(lifetime&&status==MainWindow::RunResult::Completed) {
    lifetimeRuns.append(plotlifetime::inspect(rig,test.name,test.one));
    rig.save(test.name+"-runs.json",QJsonDocument(lifetimeRuns).toJson());
    require(lifetimeRuns[0]==lifetimeRuns[1],"Second run changed retained arcs, state or final report");
   }
   if(ephemStream) {
    Publisher::Instance()->Unsubscribe(ephemStream.get());
    rig.save(test.name+"-endpoints.json",QJsonDocument(ephemRecords).toJson());
    if(test.name.endsWith("Mixed")) {
     plotephem::checkMixedEndpoints(ephemRecords,plotephem::provider(test.name));
    } else {
    const QVector<double> wanted=test.name.endsWith("Backward")?QVector<double>{0,-10,-10,-20,-30,-37}:QVector<double>{0,10,10,20,30,37};
    require(ephemRecords.size()==wanted.size(),"Ephemeris committed endpoint count differs");
    for(int i=0;i<wanted.size();++i) {
     const auto row=ephemRecords[i].toObject();
     require(row["ephemeris"].toBool()&&row["method"]==plotephem::provider(test.name)&&row["objects"].toArray()==QJsonArray{"One"},"Ephemeris endpoint provenance differs");
     if(plotephem::provider(test.name)!="SPK") {
      const auto states=row["states"].toArray();
      const auto components=states.size()==1?states.first().toArray():QJsonArray{};
      const auto truth=plotephem::state(wanted[i]);
      require(components.size()==6,"Missing ephemeris table endpoint state");
      for(int c=0;c<6;++c)require(std::abs(components[c].toDouble()-truth[c])<(c<3?4e-5:5e-8),"Ephemeris table endpoint differs from the independent cubic");
     }
     require(row["initial"].toBool()==(i==0||i==2)&&std::abs(row["elapsed"].toDouble()-wanted[i])<.0001,"Ephemeris endpoint epochs or initial flags differ");
    }
    }
   }
   if(membership)liveProof=membership->finish();
   if(cadence)cadenceProof=cadence->finish();
   rig.save(test.name+"-solver-states.json",QJsonDocument(solverStates).toJson());if(observerFailure)std::rethrow_exception(observerFailure);
   if(!test.solverMode.isEmpty())require(trials>=2&&acceptedPasses==1,"Solver-step trial/accepted matrix incomplete");rig.save(test.name+"-messages.txt",rig.messages->toPlainText().toUtf8());require(status==MainWindow::RunResult::Completed,"Plot-step mission failed");
   QJsonArray functionReport;if(function) {functionReport=plotfunction::checkReport(rig,test.name);plotfunction::checkPackets(test.name,functionPackets); }
   QJsonArray gravityRuntime;if(gravity){gravityRuntime=plotgravity::runtime();rig.save(test.name+"-gravity-runtime.json",QJsonDocument(gravityRuntime).toJson());}
   QJsonArray burnReport;if(burn)burnReport=plotburn::checkReport(rig,test.name,burnSettings);
   QJsonObject lacRaw;
   if(lac){lacRaw=plotlac::capture(rig,lacNested);rig.save(test.name+"-raw.json",QJsonDocument(lacRaw).toJson());}
   QJsonArray outputs;
   for(const auto &name:QStringList{"Orb","Map"}) {
    const auto model=rig.window.plotReceiver()->model(name);require(model&&model->endOfRun,"Plot-step model absent/incomplete");
    for(int n=0;n<2;++n) {
     auto expected=n==0?test.one:test.two;
     if(lacNested&&name=="Orb"&&n==1&&test.skip==0)expected=test.one;
     if(lac&&test.name=="LACLiveReference"&&name=="Orb"&&n==0)expected={0};
     if(rotatingPair&&name=="Orb"&&n==1&&test.skip==0)expected=test.one;
     if(test.name=="FrameLeaderMissing"&&name=="Orb"&&n==1)expected=test.one;
     if(test.name=="FrameLiveOrigin"&&name=="Orb"&&n==0)expected={0};
     if(test.name=="FrameNewArcMissing"&&name=="Orb"&&n==0)expected={0,10};
     if(n==0&&name=="Map"&&test.name=="LegacyEveryPacket")expected={0,10,10,20,30,37};
     if(n==0&&name=="Map"&&test.name=="LegacyReverseBoundary")expected={0,10,-10};
     if(n==0&&!paint&&test.solverMode=="All") {expected={0};for(int trial=0;trial<trials;++trial)expected<<20<<37;expected<<20<<37<<50;}
     while(expected.size()>test.retained)expected.removeFirst();
const PlotCurve *curve=nullptr;for(const auto &candidate:model->curves)if(candidate.name==orbits[n].name)curve=&candidate;
     QJsonArray actual; if(curve)for(const auto &point:curve->points) {QJsonObject row{{"elapsed",(point.epoch-21545)*86400},{"x",point.x},{"y",point.y},{"z",point.z}};if(resetArc)row["connect"]=point.connect;if(originFrame||rotating||lac)row["connect"]=point.connect;if((burn||ephem||resetArc||originFrame||rotating||lac)&&name=="Orb")row["velocity"]=QJsonArray{point.viewVelocity[0],point.viewVelocity[1],point.viewVelocity[2]};actual.append(row);}
     outputs.append(QJsonObject{{"plot",name},{"object",orbits[n].name},{"points",actual}});rig.save(test.name+"-points.json",QJsonDocument(outputs).toJson());
     require((curve?curve->points.size():0)==size_t(expected.size()),"Collected trajectory length differs from literal accepted-step oracle");
     for(int i=0;i<expected.size();++i) {
      const auto &point=curve->points[i];require(std::abs((point.epoch-21545)*86400-expected[i])<.0001,"Collected epoch differs from literal accepted-step oracle");
      require(std::isfinite(point.x)&&std::isfinite(point.y)&&std::isfinite(point.z),"Plot-step coordinates nonfinite");
      if(lac)require(point.connect==((lacNested&&name=="Orb"&&n==1&&test.skip==0)?i==3:i>0),"LAC frame connects across unavailable input or breaks a continuous arc");
      if(rotating)require(point.connect==((rotatingPair&&name=="Orb"&&n==1&&test.skip==0)?i==3:i>0),"Rotating frame broke a continuous trajectory");
      if(originFrame&&!(test.name=="FrameLeaderMissing"&&n==1&&name=="Orb"))require(point.connect==(i>0&&!(test.name=="FrameNewArcMissing"&&name=="Map"&&n==0&&i==2)),"Independent provider traffic broke a continuous trajectory");
      if(test.name=="FrameLeaderThinned"&&n==1&&name=="Orb")require(point.connect==(i!=0),"Intentionally skipped origin epochs broke the selected arc");
      if(test.name=="FrameLeaderMissing"&&n==1&&name=="Orb")require(point.connect==(i==3),"Follower line bridges an unavailable origin epoch or loses its final segment");
      if(burn) {
       const auto truth=plotburn::state(burnSettings,burnSettings.nonlinear?(point.epoch-21545)*86400:expected[i],burnSettings.nonlinear?burnReport[2].toArray()[0].toDouble():burnSettings.stop());
       if(name=="Orb") {
        require(std::abs(point.x-truth[0])<2e-6&&std::abs(point.y-truth[1])<2e-6&&std::abs(point.z-truth[2])<2e-6,"Burn sampled position differs from analytical thrust solution");
        require(point.hasVelocity,"Burn sampled velocity absent");for(int c=0;c<3;++c)require(std::abs(point.viewVelocity[c]-truth[c+3])<2e-9,"Burn sampled velocity differs from analytical thrust solution");
       } else {
        const double degrees=180./std::acos(-1.);const double lon=std::atan2(truth[1],truth[0])*degrees,lat=std::atan2(truth[2],std::hypot(truth[0],truth[1]))*degrees;
        require(std::abs(point.x-lon)<2e-6&&std::abs(point.y-lat)<2e-6,"Burn GroundTrack differs from independent spherical geometry");
       }
      }
      const int newArcIndex=n==1?-1:(test.name=="ResetSkipped"||test.name=="LegacyChangedState")?1:2;
      const auto translatedTime=expected[i]+(resetArc&&newArcIndex>=0&&i>=newArcIndex?2000:0);
      if(resetArc&&!(legacy&&name=="Map")) require(point.connect==(i>0&&i!=newArcIndex),"Trajectory line connects across a state assignment or drops an uninterrupted segment");
      if(name=="Orb"&&!burn) {auto state=(ephem&&n==0)?plotephem::state(expected[i]):(test.name=="TranslatedOrigin"||resetArc)?translatedOrbit(n,translatedTime):circular(orbits[n],expected[i]);if(lac)state=plotlac::state(n,expected[i],lacNested);if(rotating)state=rotatingPair?plotrotating::pairState(n,expected[i]):plotrotating::state(n,expected[i],rotatingOrigin,rotatingVelocity);if(originFrame) {const auto origin=circular(orbits[leaderOrigin?0:1],expected[i]);for(int c=0;c<6;++c)state[c]-=origin[c];}require(std::abs(point.x-state[0])<.00004&&std::abs(point.y-state[1])<.00004&&std::abs(point.z-state[2])<.00004,"Plot coordinates differ from independent trajectory");require(point.hasVelocity,"Collected velocity absent");for(int c=0;c<3;++c)require(std::abs(point.viewVelocity[c]-state[c+3])<.00000005,"Plot velocity differs from independent trajectory");}
      if(name=="Map"&&(test.name=="TranslatedOrigin"||resetArc)) {
       const auto state=translatedOrbit(n,translatedTime);const double degrees=180./std::acos(-1.);
       const double longitude=std::atan2(state[1],state[0])*degrees,latitude=std::atan2(state[2],std::hypot(state[0],state[1]))*degrees;
       require(std::abs(point.x-longitude)<.000002&&std::abs(point.y-latitude)<.000002,"Translated GroundTrack differs from independent static spherical-body longitude/latitude");
      }
     }
    }
   }
   if(lac) {
    const auto messages=rig.messages->toPlainText().mid(messagesBeforeOrigin.size());
    rig.save(test.name+"-run-messages.txt",messages.toUtf8());
    require(messages.count("has no coherent state for frame 'LacFrame'")==(test.name=="LACLiveReference"?1:lacNested&&test.skip==0?2:0),"LAC reference warning count differs");
   }
   if(rotating) {
    const auto messages=rig.messages->toPlainText().mid(messagesBeforeOrigin.size());
    rig.save(test.name+"-run-messages.txt",messages.toUtf8());
    require(messages.count("has no coherent state for frame 'RotatingFrame'")==(rotatingPair&&test.skip==0?2:0),"Rotating reference warning count differs");
   }
   if(originFrame) {
    const auto history=rig.messages->toPlainText();
    require(history.startsWith(messagesBeforeOrigin),"Origin message history changed during mission");
    const auto messages=history.mid(messagesBeforeOrigin.size());
    rig.save(test.name+"-run-messages.txt",messages.toUtf8());
    const QString diagnostic="has no coherent state for frame 'RelativeFrame'";
    const int expectedWarnings=test.name=="FrameLeaderMissing"?2:(test.name=="FrameLiveOrigin"||test.name=="FrameNewArcMissing")?1:0;
    require(messages.count(diagnostic)==expectedWarnings,"Current mission's spacecraft-origin diagnostic count differs");
   }
   if(burn&&burnSettings.nonlinear) {
    const double target=plotburn::state(burnSettings,burnSettings.stop())[0],achieved=burnReport[2].toArray()[1].toDouble();
    rig.save(test.name+"-stop-diagnostics.json",QJsonDocument(QJsonObject{{"target_elapsed",burnSettings.stop()},{"measured_elapsed",burnReport[2].toArray()[0]},{"target_x",target},{"measured_x",achieved},{"position_stop_residual",achieved-target},{"requested_stop_tolerance",1e-10},{"state_oracle_epoch","measured epoch within original literal-grid tolerance"},{"burn_report",burnReport},{"outputs",outputs}}).toJson());
    require(std::abs(achieved-target)<=1e-10,"Nonlinear finite-burn stop missed its requested position tolerance");
   }
   require(rig.source->toPlainText()==applied&&read(saved)==applied.toUtf8(),"Plot-step execution changed script");
   const auto rotatingChecks=test.name=="RotatingOriginForward"?plotrotating::contextChecks():QJsonArray{};
   const QJsonObject proof{{"case",test.name},{"lac_raw",lacRaw},{"rotating_context_checks",rotatingChecks},{"origin_checks",originChecks},{"function_packets",functionPackets},{"function_report",functionReport},{"boundary_protocol",boundaryProtocol},{"solver_flush_pixels",flushPixels},{"redraw_live",livePixels},{"redraw_final",finalPixels},{"lifetime_runs",lifetimeRuns},{"ephemeris_endpoints",ephemRecords},{"burn_report",burnReport},{"retention_limit",test.retained},{"cadence_retention",cadenceProof},{"live_membership",liveProof},{"skip",test.skip},{"solver_states",solverStates},{"trial_passes",trials},{"accepted_passes",acceptedPasses},{"outputs",outputs},{"outcome","passed"},{"limits","TranslatedOrigin checks both coordinate consumers analytically; other GroundTrack coordinates are finite-only. Live-prefixed cases check transient membership at publications and native flushes. CadenceRetention checks intermediate redraw and retention in a regular split arc. Burn cases check prescribed fixed-axis chemical/electric thrust, optional mass flow, restart and coast. General frames, ephemeris, other burn models and remaining solver/redraw conjunctions remain open."}};
   rig.save(test.name+"-proof.json",QJsonDocument(proof).toJson());completed.append(proof);std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
  }
  require(!completed.isEmpty(),"No selected plot-step cases");rig.save("proof.json",QJsonDocument(QJsonObject{{"outcome","passed"},{"cases",completed},{"new_requirement_credit",0}}).toJson());return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
