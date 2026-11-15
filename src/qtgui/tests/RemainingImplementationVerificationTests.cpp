#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "CelestialBodyPanel.hpp"
#include "MainWindow.hpp"
#include "ResourceProperties.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "Spacecraft.hpp"
#include "StateConversionUtil.hpp"
#include "Parameter.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "Propagate.hpp"
#include "FileManager.hpp"
#include "EventRootRefinement.hpp"
#include "EphemManager.hpp"
#include "TimeSystemConverter.hpp"
#include "BaseException.hpp"
#include "GmatCommand.hpp"
extern "C" {
#include "SpiceUsr.h"
}
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTreeWidget>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {
void require(bool ok,const std::string &message) { if(!ok)throw std::runtime_error(message); }
void write(const QString &path,const QByteArray &data) {
   QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"Cannot retain verification output");
}
QByteArray read(const QString &path) {
   QFile f(path);require(f.open(QIODevice::ReadOnly),"Cannot read verification output");return f.readAll();
}
void events() { QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
struct Rig {
   MainWindow window;QPlainTextEdit *source=nullptr,*messages=nullptr;
   int buildNumber=0;
   QString artifact;
   explicit Rig(const QString &startup) {
      require(window.initialize(startup),"Runtime initialization failed");window.show();acceptanceView(window);
      source=window.findChild<QPlainTextEdit *>("scriptEditor");messages=window.findChild<QPlainTextEdit *>("messageWindow");
      require(source&&messages,"Workspace missing");
      artifact=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   }
   ~Rig() { if(source)source->document()->setModified(false); }
   bool build(const QString &script) {
      source->setPlainText(script);messages->clear();++buildNumber;
      if(!artifact.isEmpty())write(artifact+"/build-"+QString::number(buildNumber)+".script",script.toUtf8());
      const bool ok=window.buildScript();
      if(!artifact.isEmpty())write(artifact+"/build-"+QString::number(buildNumber)+"-messages.txt",messages->toPlainText().toUtf8());return ok;
   }
   void buildGood(const QString &script) {
      if(!build(script))throw std::runtime_error("Build failed: "+messages->toPlainText().toStdString());
   }
   MainWindow::RunResult run() {
      const auto before=source->toPlainText();const int undo=source->document()->availableUndoSteps();
      const auto status=window.runMission();
      events();
      if(!artifact.isEmpty())write(artifact+"/run-"+QString::number(buildNumber)+"-messages.txt",messages->toPlainText().toUtf8());
      require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo,"Run changed source or history");return status;
   }
   void runGood() {if(run()!=MainWindow::RunResult::Completed){std::cerr<<messages->toPlainText().toStdString()<<std::endl;throw std::runtime_error("Run failed: "+messages->toPlainText().toStdString());}}
};
GmatBase *configured(const char *name) {auto *p=Moderator::Instance()->GetConfiguredObject(name);require(p,"Configured object missing");return p;}
double value(const char *name) {
   auto *p=dynamic_cast<Parameter *>(Moderator::Instance()->GetInternalObject(name));require(p,"Runtime parameter missing");return p->EvaluateReal();
}
using State=std::array<double,6>;
constexpr double mu=398600.4415;
constexpr double pi=3.1415926535897932384626433832795;
// Independent initial conic geometry followed by Cartesian RK4 integration.
// This oracle does not call GMAT conversions or its anomaly propagator.
State initial(double a,double e,double inc,double node,double peri,double anomaly,double gravity) {
   inc*=pi/180;node*=pi/180;peri*=pi/180;anomaly*=pi/180;
   const double p=a*(1-e*e),radius=p/(1+e*std::cos(anomaly)),speed=std::sqrt(gravity/p);
   const double x=radius*std::cos(anomaly),y=radius*std::sin(anomaly),vx=-speed*std::sin(anomaly),vy=speed*(e+std::cos(anomaly));
   State result{};
   for(int k=0;k<2;++k) {
      const double xx=k?vx:x,yy=k?vy:y;
      const double u=std::cos(peri)*xx-std::sin(peri)*yy,v=std::sin(peri)*xx+std::cos(peri)*yy;
      result[k*3]=std::cos(node)*u-std::sin(node)*std::cos(inc)*v;
      result[k*3+1]=std::sin(node)*u+std::cos(node)*std::cos(inc)*v;
      result[k*3+2]=std::sin(inc)*v;
   }
   return result;
}
State integrate(State y,double seconds,double gravity) {
   const int steps=std::max(1,int(std::ceil(std::abs(seconds)/.5)));const double h=seconds/steps;
   auto derivative=[&](const State &s) {
      const double r=std::hypot(s[0],s[1],s[2]),f=-gravity/(r*r*r);return State{s[3],s[4],s[5],f*s[0],f*s[1],f*s[2]};
   };
   auto add=[](State s,const State &d,double h) {for(int i=0;i<6;++i)s[i]+=h*d[i];return s;};
   for(int n=0;n<steps;++n) {
      const auto k1=derivative(y),k2=derivative(add(y,k1,h/2)),k3=derivative(add(y,k2,h/2)),k4=derivative(add(y,k3,h));
      for(int i=0;i<6;++i)y[i]+=h*(k1[i]+2*k2[i]+2*k3[i]+k4[i])/6;
   }
   return y;
}
void stateNear(const Rvector6 &actual,const State &expected,double position,double velocity) {
   for(int i=0;i<6;++i)require(std::isfinite(actual[i])&&std::abs(actual[i]-expected[i])<=(i<3?position:velocity),
      "Cartesian oracle mismatch at component "+std::to_string(i)+": "+std::to_string(actual[i])+" vs "+std::to_string(expected[i]));
}
QString number(double x) {return QString::number(x,'g',17);}
QString bodyScript(double a,double e,double inc=31,double node=50,double peri=21,double ta=40) {
   return "Earth.Mu = "+number(mu)+";\nSolarSystem.EphemerisUpdateInterval = 0;\nCreate Moon Probe;\nProbe.CentralBody = Earth;\nProbe.Mu = 1;\nProbe.PosVelSource = TwoBodyPropagation;\nProbe.InitialEpoch = 21545;\nProbe.SMA = "+number(a)+";\nProbe.ECC = "+number(e)+";\nProbe.INC = "+number(inc)+";\nProbe.RAAN = "+number(node)+";\nProbe.AOP = "+number(peri)+";\nProbe.TA = "+number(ta)+";\nBeginMissionSequence;\n";
}
QJsonArray twoBody(const QString &startup,const QString &directory) {
   Rig rig(startup);QJsonArray result;
   for(const auto &elements:std::vector<State>{{20000,0,0,0,0,0},{20000,.3,31,50,21,40},{200000,.95,160,112,53,15},{-30000,1.4,47,112,38,-20}}) {
      rig.buildGood(bodyScript(elements[0],elements[1],elements[2],elements[3],elements[4],elements[5]));
      auto *system=Moderator::Instance()->GetSolarSystemInUse();auto *body=system->GetBody("Probe"),*earth=system->GetBody("Earth");require(body&&earth,"Two-body references missing");
      const auto start=initial(elements[0],elements[1],elements[2],elements[3],elements[4],elements[5],mu+1);
      for(double dt:{0.,600.,-400.,1200.}) {
         GmatTime epoch(21545.);epoch.AddSeconds(dt);
         const auto relative=body->GetMJ2000State(epoch)-earth->GetMJ2000State(epoch);
         stateNear(relative,integrate(start,dt,mu+1),2e-6,2e-9);
         const auto acceleration=body->GetMJ2000Acceleration(epoch)-earth->GetMJ2000Acceleration(epoch);
         const double radius=std::hypot(relative[0],relative[1],relative[2]);
         for(int i=0;i<3;++i)require(std::abs(acceleration[i]+(mu+1)*relative[i]/(radius*radius*radius))<1e-13,"Two-body acceleration mismatch");
         result.append(QJsonObject{{"sma",elements[0]},{"ecc",elements[1]},{"seconds",dt},{"position_tolerance_km",2e-6},{"velocity_tolerance_km_s",2e-9}});
      }
   }
   const auto baseline=bodyScript(20000,.2);rig.buildGood(baseline);
   auto *tree=rig.window.findChild<QTreeWidget *>("Resources");require(tree,"Resource tree missing");
   auto items=tree->findItems("Probe",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Body tree entry missing");tree->itemDoubleClicked(items.front(),0);events();
   auto *child=rig.window.findChild<QMdiArea *>("workspace")->activeSubWindow();auto *panel=child?dynamic_cast<CelestialBodyPanel *>(child->widget()):nullptr;require(panel,"Body editor missing");
   QMap<QString,QString> changes{{"InitialEpoch","21546"},{"SMA","24000"},{"ECC","0.4"},{"INC","67"},{"RAAN","123"},{"AOP","87"},{"TA","55"}};
   for(auto it=changes.cbegin();it!=changes.cend();++it) {auto *field=panel->findChild<QLineEdit *>("body_"+it.key());require(field&&!field->isReadOnly(),"Body orbit field unavailable");field->setText(it.value());}
   require(panel->hasChanges()&&rig.source->toPlainText()==baseline,"Pending body edits changed source");
   auto *buttons=panel->findChild<QDialogButtonBox *>();require(buttons&&buttons->button(QDialogButtonBox::Apply),"Body Apply missing");buttons->button(QDialogButtonBox::Apply)->click();events();
   const auto edited=rig.source->toPlainText();require(edited!=baseline,"Body Apply did not commit");
   auto check=[&] {auto *body=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Probe");for(auto it=changes.cbegin();it!=changes.cend();++it)require(std::abs(body->GetRealParameter(body->GetParameterID(it.key().toStdString()))-it.value().toDouble())<1e-12,"Applied orbit property missing");};check();
   require(child->close(),"Clean body panel did not close");events();rig.source->undo();require(rig.source->toPlainText()==baseline&&rig.window.buildScript(),"Body Undo failed");rig.source->redo();require(rig.source->toPlainText()==edited&&rig.window.buildScript(),"Body Redo failed");check();
   const auto path=directory+"/body-orbit.script";require(rig.window.saveScriptTo(path)&&rig.window.loadScript(path)&&rig.window.buildScript(),"Body save/reopen failed");check();
   require(!rig.window.applyResourceChanges("Probe",{{"ECC","1"}},edited).isEmpty()&&rig.source->toPlainText()==edited,"Invalid conic changed source");
   result.append(QJsonObject{{"gui_orbit_fields",changes.size()},{"pending_apply_undo_redo_reopen",true}});return result;
}
QJsonArray bulirsch(const QString &startup) {
   Rig rig(startup);rig.buildGood("Create Spacecraft Sat;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator Prop;\nProp.FM = FM;\nProp.Type = BulirschStoer;\nProp.Accuracy = 1e-12;\nProp.MinimumTolerance = 1e-8;\nBeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 60};\n");
   auto check=[](GmatBase *object) {auto *setup=dynamic_cast<PropSetup *>(object);require(setup&&setup->GetPropagator(),"Bulirsch setup missing");auto *p=setup->GetPropagator();require(std::abs(p->GetRealParameter("Accuracy")-1e-12)<1e-25&&std::abs(p->GetRealParameter("MinimumTolerance")-1e-8)<1e-20,"Bulirsch floor reset requested accuracy");return p;};
   auto *p=check(configured("Prop"));const auto fields=resourceProperties(*p);require(std::any_of(fields.begin(),fields.end(),[](const auto &f){return f.name=="MinimumTolerance"&&f.unit=="dimensionless";}),"Minimum tolerance not exposed");
   std::unique_ptr<GmatBase> clone(configured("Prop")->Clone());auto *copy=check(clone.get());
   for(double bad:{-1.,1.1,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {bool rejected=false;try{copy->SetRealParameter("MinimumTolerance",bad);}catch(BaseException &){rejected=true;}require(rejected,"Invalid minimum tolerance accepted");check(clone.get());}
   rig.runGood();check(Moderator::Instance()->GetInternalObject("Prop"));
   auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));require(sat,"Runtime spacecraft missing");const auto state=sat->GetCartesianState();for(int i=0;i<6;++i)require(std::isfinite(state[i]),"Bulirsch state nonfinite");
   return {QJsonObject{{"accuracy_requested",1e-12},{"minimum_tolerance",1e-8},{"cloned_and_executed",true}}};
}
QString spacecraft(double a,double e) {
   return "Earth.Mu = "+number(mu)+";\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Keplerian;\nSat.SMA = "+number(a)+";\nSat.ECC = "+number(e)+";\nSat.INC = 47;\nSat.RAAN = 112;\nSat.AOP = 38;\nSat.TA = 20;\n";
}
QJsonArray anomalies(const QString &startup) {
   Rig rig(startup);QJsonArray result;
   for(const auto &kind:QStringList{"EA","MA","HA"})for(double degrees:{-40.,70.}) {
      const bool hyper=kind=="HA";const double a=hyper?-30000:20000,e=hyper?1.4:.3;double anomaly=degrees*pi/180;
      if(kind=="MA") {const double mean=anomaly;double lo=-pi,hi=pi;for(int i=0;i<100;++i){anomaly=(lo+hi)/2;if(anomaly-e*std::sin(anomaly)<mean)lo=anomaly;else hi=anomaly;}}
      const double ta=hyper?2*std::atan(std::sqrt((e+1)/(e-1))*std::tanh(anomaly/2)):2*std::atan2(std::sqrt(1+e)*std::sin(anomaly/2),std::sqrt(1-e)*std::cos(anomaly/2));
      if(kind!="MA") {
         const double wrapped=StateConversionUtil::ConvertToTrueAnomaly(kind.toStdString(),anomaly,e,true);
         require(std::abs(wrapped-(ta<0?ta+2*pi:ta))<1e-12,"Wrapped anomaly conversion mismatch");
      }
      const auto script=spacecraft(a,e)+"Create Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nSat.Earth."+kind+" = "+number(degrees)+";\nKeep = Keep + 1;\n";
      rig.buildGood(script);rig.runGood();auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));require(sat,"Runtime spacecraft missing");
      stateNear(sat->GetCartesianState(),initial(a,e,47,112,38,ta*180/pi,mu),3e-6,3e-9);require(value("Keep")==42,"Anomaly assignment skipped following command");
      result.append(QJsonObject{{"anomaly",kind},{"assigned_degrees",degrees},{"expected_true_anomaly",ta*180/pi}});
   }
   return result;
}
QJsonArray worksheet(const QString &startup,const QString &directory) {
   Rig rig(startup);QJsonArray result;
   for(const auto &mode:QStringList{"Native","Worksheet"})for(const auto &property:QStringList{"RMAG","SemilatusRectum"}) {
      const auto script=spacecraft(20000,.3)+"Sat.OrbitParameterAccess = "+mode+";\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nSat.Earth."+property+" = 16000;\nKeep = Keep + 1;\n";
      if(mode=="Native") {rig.buildGood(script);rig.runGood();require(value("Keep")==42,"Native write skipped following command");}
      else {
         require(!rig.build(script),"Worksheet accepted a read-only assignment");
         const auto reading=spacecraft(20000,.3)+"Sat.OrbitParameterAccess = Worksheet;\nCreate Variable Readback;\nBeginMissionSequence;\nReadback = Sat.Earth."+property+";\n";
         rig.buildGood(reading);rig.runGood();require(std::isfinite(value("Readback"))&&value("Readback")>0,"Worksheet blocked parameter reads");
      }
      result.append(QJsonObject{{"access",mode},{"property",property},{"write_completed",mode=="Native"}});
   }
   for(const auto &style:QStringList{"Worksheet","LegacyData"}) {
      const auto tail=style=="Worksheet"?QString():" {FilenameStyle = LegacyData}";
      rig.buildGood("Create Variable Saved;\nSaved = 17;\nBeginMissionSequence;\nSave Saved"+tail+";\n");rig.runGood();
      const auto filename="Saved.Variable"+(style=="LegacyData"?QString(".data"):QString());const auto path=QString::fromStdString(FileManager::Instance()->GetAbsPathname(FileManager::OUTPUT_PATH))+filename;
      const auto data=read(path);require(data.contains("Saved")&&data.contains("17"),"Save filename has no serialized object");write(directory+'/'+filename,data);result.append(QJsonObject{{"filename_style",style},{"filename",filename}});
   }
   for(const auto &style:QStringList{"Worksheet","LegacyData"})for(bool explicitFile:{false,true}) {
      const auto output=QString::fromStdString(FileManager::Instance()->GetAbsPathname(FileManager::OUTPUT_PATH));
      const auto path=explicitFile?directory+"/Save's Δ "+style+".txt":output+"Saved.Variable"+(style=="LegacyData"?QString(".data"):QString());
      QString options=style=="LegacyData"?"FilenameStyle = LegacyData":QString();
      if(explicitFile) {auto quoted=path;quoted.replace("'","''");if(!options.isEmpty())options+=", ";options+="Filename = '"+quoted+"'";}
      const auto tail=options.isEmpty()?QString():" {"+options+"}";
      const auto statement="Save 'Named snapshot' Saved"+tail+";";
      rig.buildGood("Create Variable Saved Keep;\nSaved = 17;\nKeep = 41;\nBeginMissionSequence;\n"+statement+"\nKeep = Keep + 1;\n");
      GmatCommand *save=nullptr;for(auto *command=Moderator::Instance()->GetFirstCommand();command;command=command->GetNext())if(command->GetTypeName()=="Save"){require(!save,"Duplicate Save command");save=command;}
      require(save&&save->GetName()=="Named snapshot","Save command label was lost");
      const auto generated=QString::fromStdString(save->GetGeneratingString(Gmat::NO_COMMENTS));
      require(generated.startsWith("Save 'Named snapshot' Saved")&&!generated.contains("'Named snapshot' 'Named snapshot'"),"Save serialization lost or duplicated its label");
      QFile::remove(path);require(!QFileInfo::exists(path),"Save output was not isolated");rig.runGood();
      const auto data=read(path);require(data.contains("Saved")&&data.contains("17")&&value("Keep")==42,"Named Save did not serialize the selected object and continue");
      rig.buildGood("Create Variable Saved;\nSaved = 19;\nBeginMissionSequence;\n"+generated+"\n");rig.runGood();
      require(read(path).contains("19"),"Named Save native serialization did not roundtrip");
      result.append(QJsonObject{{"save_label","Named snapshot"},{"filename_style",style},{"explicit_unicode_apostrophe_path",explicitFile},{"native_serialization_roundtrip",true}});
   }
   for(const auto &end:QStringList{"EndOptimize","EndTarget"}) {
      rig.buildGood("Create Variable X Cost Keep;\nX = 0;\nKeep = 41;\nCreate Yukon Opt;\nOpt.ShowProgress = false;\nOpt.ReportFile = '"+directory+"/opt.txt';\nBeginMissionSequence;\nOptimize Opt {ShowProgressWindow = false, ExitMode = SaveAndContinue};\nVary Opt(X = 0, {Perturbation = 0.00001, Lower = -10, Upper = 10, MaxStep = 10});\nCost = (X - 3)^2;\nMinimize Opt(Cost);\n"+end+";\nKeep = Keep + 1;\n");rig.runGood();require(std::abs(value("X")-3)<2e-5&&value("Keep")==42,"Optimizer ending missed analytic solution");result.append(QJsonObject{{"optimizer_end",end},{"x",value("X")}});
   }
   const QString target="Create Variable X Keep;\nX = 1;\nKeep = 41;\nCreate DifferentialCorrector DC;\nDC.ShowProgress = false;\nDC.ReportFile = '"+directory+"/dc.txt';\nBeginMissionSequence;\nTarget DC {ShowProgressWindow = false};\nVary DC(X = 1, {Perturbation = 0.0001, Lower = -10, Upper = 10, MaxStep = 10});\nAchieve DC(5 = 5, {Tolerance = 0.000001%1});\nEndTarget;\nKeep = Keep + 1;\n";
   rig.buildGood(target.arg(", AllowConstantGoal = true"));rig.runGood();require(value("Keep")==42,"Numeric Achieve goal did not complete");require(!rig.build(target.arg("")),"Numeric goal accepted without opt-in");result.append(QJsonObject{{"numeric_goal_opt_in",true}});return result;
}
QJsonArray policies(const QString &startup) {
   Rig rig(startup);QJsonArray result;
   for(const auto &policy:QStringList{"Native","SingleSetup","SingleGroup"}) {
      QString script="Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\n";QString groups;
      for(int i=0;i<16;++i) {const auto suffix=QString::number(i);script+="Create Spacecraft S"+suffix+";\nCreate Propagator P"+suffix+";\nP"+suffix+".FM = FM;\nP"+suffix+".InitialStepSize = 10;\n";groups+="P"+suffix+"(S"+suffix+") ";}
      script+="BeginMissionSequence;\nPropagate "+groups+"{S0.ElapsedSecs = 20, EphemerisPolicy = "+policy+"};\n";rig.buildGood(script);
      auto *initialSat=dynamic_cast<Spacecraft *>(configured("S0"));require(initialSat,"Configured group spacecraft missing");const double start=initialSat->GetEpoch();rig.runGood();
      for(int i=0;i<16;++i) {auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("S"+std::to_string(i)));require(sat,"Numerical group lost spacecraft");require(std::abs((sat->GetEpoch()-start)*86400-20)<1e-5,"Numerical group did not advance to the requested stop");const auto state=sat->GetCartesianState();for(int j=0;j<6;++j)require(std::isfinite(state[j]),"Nonfinite numerical-group state");}
      auto *first=Moderator::Instance()->GetFirstCommand();GmatCommand *boundary=nullptr;
      for(auto *command=first;command;command=command->GetNext())if(command->GetTypeName()=="BeginMissionSequence"){require(!boundary,"Duplicate mission boundary");boundary=command;}
      require(first&&boundary,"Mission boundary unavailable for summary inspection");
      boundary->SetupSummary("EarthMJ2000Eq",false,false);const auto single=QString::fromStdString(boundary->GetStringParameter("Summary"));
      require(single.contains("BeginMissionSequence Command:")&&single.count("Spacecraft       :")==16,"Individual boundary summary omitted captured spacecraft");
      first->SetupSummary("EarthMJ2000Eq",true,false);const auto aggregate=QString::fromStdString(first->GetStringParameter("MissionSummary"));
      require(aggregate.contains("Spacecraft       :")&&!aggregate.contains("BeginMissionSequence Command:"),"Mission aggregate changed boundary inclusion");
      result.append(QJsonObject{{"policy",policy},{"numerical_groups_executed",16}});
   }
   rig.buildGood("Create Propagator A B;\nA.Type = SPK;\nA.CentralBody = Mars;\nB.Type = SPK;\nBeginMissionSequence;\n");
   auto *a=dynamic_cast<PropSetup *>(configured("A")),*b=dynamic_cast<PropSetup *>(configured("B"));require(a&&b&&a->GetPropagator()->GetStringParameter("CentralBody")=="Mars","SPK origin setting lost");
   std::vector<PropSetup *> same{a,a},distinct{a,b};
   require(Propagate::EphemerisPolicyError("SingleSetup",same).empty()&&!Propagate::EphemerisPolicyError("SingleSetup",distinct).empty(),"SingleSetup count policy incorrect");
   require(!Propagate::EphemerisPolicyError("SingleGroup",same).empty()&&Propagate::EphemerisPolicyError("Native",distinct).empty(),"SingleGroup/native policy incorrect");
   const auto before=rig.source->toPlainText();require(rig.window.applyResourceChanges("A",{{"CentralBody","Earth"}},before).isEmpty(),"SPK GUI origin Apply failed");
   a=dynamic_cast<PropSetup *>(configured("A"));require(a&&a->GetPropagator()->GetStringParameter("CentralBody")=="Earth","SPK origin Apply lost setting");result.append(QJsonObject{{"spk_origin_apply",true},{"count_policies",true}});return result;
}
QJsonArray brent() {
   QJsonArray result;
   // An unused/failed recording setup has no writers. Cleanup must be safe
   // and idempotent, including when Spacecraft stops it before destruction.
   {EphemManager idle(false);idle.StopRecording();idle.StopRecording(false);}
   result.append(QJsonObject{{"unstarted_recording_cleanup",true}});
   for(double root:{-1000.,-0.001,0.,0.001,1000.}) {
      const double found=GmatEventRoot::Refine([=](double x){const double y=x-root;return y+y*y*y;},root-4,root+7,1e-10);
      require(std::abs(found-root)<1e-9,"Brent missed known continuous root");result.append(QJsonObject{{"root",root},{"found",found}});
   }
   const double found=GmatEventRoot::Refine([](double x){return std::cos(x)-x;},0,1,1e-12);require(std::abs(found-.7390851332151607)<1e-11,"Brent transcendental root mismatch");
   bool rejected=false;try {GmatEventRoot::Refine([](double x){return x*x+1;},-1,1,1e-6);}catch(BaseException &){rejected=true;}require(rejected,"Brent accepted nonbracketed root");
   result.append(QJsonObject{{"cos_x_equals_x",found},{"invalid_bracket_rejected",true}});return result;
}
QJsonArray sourcePolicy(const QString &startup,const QString &directory) {
   Rig rig(startup);QJsonArray result;int index=0;
   for(bool strict:{false,true})for(bool suffix:{false,true})for(bool unicode:{false,true}) {
      const auto folder=directory+"/source-"+QString::number(++index);require(QDir().mkpath(folder),"Source fixture directory failed");
      const auto path=folder+"/Encoded.gmf"+(suffix?QString(".txt"):QString());
      const auto expected=unicode?QString::fromUtf8("café Ω"):QString("plain");
      write(path,("function [out] = Encoded()\nCreate String out;\nBeginMissionSequence;\nout = '"+expected+"';\n").toUtf8());
      const auto script="Create GmatFunction Exercise;\nExercise.FunctionPath = '"+path+"';\nExercise.SourceFormat = "+(strict?"LegacyASCII":"Native")+";\nCreate String Result;\nBeginMissionSequence;\n[Result] = Exercise();\n";
      const bool accepted=rig.build(script),wanted=!strict||(!suffix&&!unicode);require(accepted==wanted,"Function source policy boundary changed");
      if(accepted) {rig.runGood();auto *out=Moderator::Instance()->GetInternalObject("Result");require(out&&QString::fromStdString(out->GetStringParameter("Value"))==expected,"Function source literal differs");}
      result.append(QJsonObject{{"strict",strict},{"trailing_suffix",suffix},{"unicode_literal",unicode},{"accepted",accepted}});
   }
   for(bool strict:{false,true})for(bool suffix:{false,true}) {
      const auto folder=directory+"/source-"+QString::number(++index);require(QDir().mkpath(folder),"Include fixture directory failed");
      const auto fragment=folder+(suffix?"/body.txt":"/body.gmf"),path=folder+"/Encoded.gmf";
      write(fragment,"out = 17;\n");write(path,("function [out] = Encoded()\nCreate Variable out;\nBeginMissionSequence;\n#Include '"+fragment+"'\n").toUtf8());
      const bool accepted=rig.build("Create GmatFunction Exercise;\nExercise.FunctionPath = '"+path+"';\nExercise.SourceFormat = "+(strict?"LegacyASCII":"Native")+";\nCreate Variable Result;\nBeginMissionSequence;\n[Result] = Exercise();\n");
      require(accepted==(!strict||!suffix),"Function include suffix policy changed");if(accepted){rig.runGood();require(value("Result")==17,"Function included assignment differs");}
      result.append(QJsonObject{{"strict",strict},{"include_txt",suffix},{"accepted",accepted}});
   }
   return result;
}
QJsonArray spkOrigins(const QString &startup,const QString &directory) {
   Rig rig(startup);QJsonArray result;const auto kernel=directory+"/origin.bsp";SpiceInt handle;
   spkopn_c(kernel.toUtf8().constData(),"Origin verification",0,&handle);require(!failed_c(),"SPK fixture open failed");
   SpiceDouble epochs[]={0,150,900},states[3][6];for(int i=0;i<3;++i){const double t=epochs[i];const State s{7000+t,-100+2*t,200-.5*t,1,2,-.5};for(int j=0;j<6;++j)states[i][j]=s[j];}
   spkw09_c(handle,-199990,399,"J2000",0,900,"AFFINE EARTH STATE",1,3,states,epochs);spkcls_c(handle);require(!failed_c(),"SPK fixture write failed");
   for(const auto &origin:QStringList{"Earth","Mars","Probe"}) {
      const auto report=directory+"/origin-"+origin+".csv";QString prefix;
      if(origin=="Probe"){prefix=bodyScript(20000,.2);prefix.remove("BeginMissionSequence;\n");}
      rig.buildGood(prefix+"Create CoordinateSystem EarthIcrf;\nEarthIcrf.Origin = Earth;\nEarthIcrf.Axes = ICRF;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthIcrf;\nSat.NAIFId = -199990;\nSat.DateFormat = TDBModJulian;\nSat.Epoch = '21545';\nSat.OrbitSpiceKernelName = {'"+kernel+"'};\nCreate Propagator Read;\nRead.Type = SPK;\nRead.CentralBody = "+origin+";\nRead.StepSize = 60;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nPropagate Read(Sat);\nReport Values Sat.A1ModJulian Sat.EarthIcrf.X Sat.EarthIcrf.Y Sat.EarthIcrf.Z Sat.EarthIcrf.VX Sat.EarthIcrf.VY Sat.EarthIcrf.VZ;\n");rig.runGood();
      const auto columns=QString::fromUtf8(read(report)).trimmed().split(',');require(columns.size()==7,"SPK origin report cardinality differs");
      const double epoch=columns[0].toDouble(),t=(TimeSystemConverter::Instance()->Convert(epoch,TimeSystemConverter::A1MJD,TimeSystemConverter::TDBMJD,2430000.)-21545)*86400;
      require(std::abs(t-60)<2e-5,"SPK origin changed query epoch");const State expected{7000+t,-100+2*t,200-.5*t,1,2,-.5};double maxPosition=0,maxVelocity=0;
      for(int j=0;j<6;++j) {bool valid=false;const double actual=columns[j+1].toDouble(&valid),error=std::abs(actual-expected[j]);require(valid&&std::isfinite(actual)&&error<(j<3?2e-5:2e-9),"SPK origin translation changed independent Earth-relative state");if(j<3)maxPosition=std::max(maxPosition,error);else maxVelocity=std::max(maxVelocity,error);}
      result.append(QJsonObject{{"origin",origin},{"seconds",t},{"max_position_error_km",maxPosition},{"max_velocity_error_km_s",maxVelocity}});
   }
   return result;
}
QJsonArray eclipseBrent(const QString &startup,const QString &directory) {
   Rig rig(startup);QJsonArray result;const auto fixture=QJsonDocument::fromJson(read(QString::fromUtf8(GMAT_REMAINING_VERIFICATION_FIXTURES)+"/NasaEclipseResultCases.json")).object();QJsonObject spec;
   for(auto row:fixture["cases"].toArray())if(row.toObject()["key"]=="Umbra")spec=row.toObject();require(!spec.isEmpty(),"Independent eclipse case missing");
   const QStringList fields{"X","Y","Z","VX","VY","VZ"};QString prefix="SolarSystem.SPKFilename = '"+QString::fromStdString(FileManager::Instance()->GetRootPath())+"data/planetary_ephem/spk/DE421AllPlanets.bsp';\nEarth.Mu = "+number(spec["mu"].toDouble())+";\nCreate Spacecraft Sat;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2000 12:00:00.000';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.NAIFId = -100070;\n";
   for(int i=0;i<6;++i)prefix+="Sat."+fields[i]+" = "+number(spec["initial_state"].toArray()[i].toDouble())+";\n";
   prefix+="Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 10;\nProp.MinStep = 10;\nProp.MaxStep = 10;\nProp.Accuracy = 1e-12;\nCreate EclipseLocator Events;\nEvents.Spacecraft = Sat;\nEvents.OccultingBodies = {Earth};\nEvents.EclipseTypes = {Umbra};\nEvents.RunMode = Automatic;\nEvents.UseLightTimeDelay = false;\nEvents.UseStellarAberration = false;\nEvents.StepSize = 5;\nEvents.WriteReport = true;\nEvents.RootFindingMethod = Brent;\n";
   for(double tolerance:{1e-4,1e-6}) {
      const auto report=directory+"/brent-"+number(tolerance)+".txt";rig.buildGood(prefix+"Events.RootTolerance = "+number(tolerance)+";\nEvents.Filename = '"+report+"';\nBeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 6500};\n");rig.runGood();
      const auto text=QString::fromUtf8(read(report));const QString stamp="(\\d{2} [A-Za-z]{3} \\d{4} \\d{2}:\\d{2}:\\d{2}\\.\\d{3})";const auto pattern=QRegularExpression("^"+stamp+"\\s+"+stamp+"\\s+([\\deE.+-]+)\\s+Earth\\s+Umbra\\s+1\\s+([\\deE.+-]+)\\s*$",QRegularExpression::MultilineOption);auto matches=pattern.globalMatch(text);require(matches.hasNext(),"Brent eclipse interval missing");const auto row=matches.next();require(!matches.hasNext(),"Brent invented eclipse intervals");
      const auto baseline=QDateTime::fromString("01 Jan 2000 12:00:00.000","dd MMM yyyy HH:mm:ss.zzz"),begin=QDateTime::fromString(row.captured(1),"dd MMM yyyy HH:mm:ss.zzz"),end=QDateTime::fromString(row.captured(2),"dd MMM yyyy HH:mm:ss.zzz");require(begin.isValid()&&end.isValid(),"Brent report times invalid");const double a=baseline.msecsTo(begin)/1000.,b=baseline.msecsTo(end)/1000.;const auto expected=spec["events"].toArray()[0].toObject();require(std::abs(a-expected["start"].toDouble())<spec["time_tolerance"].toDouble()&&std::abs(b-expected["end"].toDouble())<spec["time_tolerance"].toDouble()&&std::abs(row.captured(3).toDouble()-expected["duration"].toDouble())<spec["duration_tolerance"].toDouble(),"Brent eclipse differs from independent ray/ellipsoid oracle");result.append(QJsonObject{{"tolerance_seconds",tolerance},{"start_utc_seconds",a},{"end_utc_seconds",b}});
   }
   return result;
}
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("RemainingImplementationVerification");
   try {
      require(argc==3&&acceptanceWidgetPlatform(),"Supply startup and verification case");TestSettings settings;QTemporaryDir scratch;require(scratch.isValid(),"Temporary directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(),mode=QString::fromLocal8Bit(argv[2]);QDir::setCurrent(QFileInfo(startup).absolutePath());const auto retained=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"),directory=retained.isEmpty()?scratch.path():retained;QJsonArray observations;
      if(mode=="TwoBody")observations=twoBody(startup,directory);else if(mode=="BulirschFloor")observations=bulirsch(startup);else if(mode=="AnomalyWrites")observations=anomalies(startup);else if(mode=="WorksheetModes")observations=worksheet(startup,directory);else if(mode=="PropagatePolicy")observations=policies(startup);else if(mode=="EventBrent")observations=brent();else if(mode=="SourcePolicy")observations=sourcePolicy(startup,directory);else if(mode=="SpkOrigins")observations=spkOrigins(startup,directory);else if(mode=="EclipseBrent")observations=eclipseBrent(startup,directory);else throw std::runtime_error("Unknown case");
      write(directory+"/verification.json",QJsonDocument(QJsonObject{{"case",mode},{"status","passed"},{"observations",observations},{"new_requirement_credit",0},{"whole_row_qualified",false}}).toJson());std::cout<<mode.toStdString()<<" passed: "<<observations.size()<<" observations\n";return 0;
   }catch(BaseException &e){std::cerr<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<e.what()<<'\n';}return 1;
}
